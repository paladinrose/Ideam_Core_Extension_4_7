#include "character_scaffold_gizmo.h"

#include "ideam_characters_plugin.h"
#include "loomis_character_node_3d.h"

#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

namespace {

const core::BodyPlanParameterBound *find_ruler_unit_bound(const core::BodyPlanCore &p_plan) {
    for (const auto &b : p_plan.parameter_bounds()) {
        if (b.category == core::BodyPlanParamCategory::PARAM_RULER_UNIT) {
            return &b;
        }
    }
    return nullptr;
}

} // namespace

CharacterScaffoldGizmo::CharacterScaffoldGizmo() {
    set_active_tool_mode(godot::StringName("scaffolding"));
}

bool CharacterScaffoldGizmo::can_handle(godot::Node3D *p_target) const {
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null()) {
        return false;
    }
    const godot::StringName mode = get_active_tool_mode();
    return mode == godot::StringName() || mode == godot::StringName("scaffolding");
}

void CharacterScaffoldGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null()) return;

    auto inst = ch->get_character_resource();
    if (inst->get_body_plan().is_null()) return;

    const auto &plan = inst->get_body_plan()->get_core_plan();
    const auto eval  = inst->get_core_instance().evaluate_stack(plan, 3u);
    const auto &sc   = eval.evaluated_scaffold;
    const auto &nodes = sc.nodes();
    const uint64_t inst_id = ch->get_instance_id();

    const bool is_dragging_this = active_drags_.has(inst_id) && active_drags_[inst_id].active;
    const bool limit_hit        = is_dragging_this && active_drags_[inst_id].limit_reached;

    // 1. Bone / Socket Hierarchy Links & Interactive Scaffold Handles
    if (!nodes.empty()) {
        godot::PackedVector3Array bone_lines;
        for (const auto &sn : nodes) {
            const godot::Vector3 wp(sn.world_transform[3][0], sn.world_transform[3][1], sn.world_transform[3][2]);
            const int32_t hid = HANDLE_SCAFFOLD_NODE_BASE + static_cast<int32_t>(sn.node_id);

            uint32_t hstate = GIZMO_STATE_NONE;
            if (p_view.active_handle_id == hid) {
                hstate |= GIZMO_STATE_DRAGGING;
                if (limit_hit) hstate |= GIZMO_STATE_LIMIT_REACHED;
            } else if (p_view.hovered_handle_id == hid) {
                hstate |= GIZMO_STATE_HOVER;
            }

            r_out.add_handle(
                hid,
                wp,
                GIZMO_HANDLE_SCAFFOLD,
                HANDLE_SHAPE_SPHERE,
                static_cast<real_t>(0.036),
                hstate,
                godot::String::utf8(sn.name.c_str()),
                false
            );

            if (sn.parent_id != 0xFFFFFFFFu && sn.parent_id < nodes.size()) {
                const auto &pn = nodes[sn.parent_id];
                const godot::Vector3 pp(pn.world_transform[3][0], pn.world_transform[3][1], pn.world_transform[3][2]);
                bone_lines.append(pp);
                bone_lines.append(wp);
            }
        }
        if (!bone_lines.is_empty()) {
            r_out.add_lines(bone_lines, GIZMO_LINE_BONE_LINK);
        }
    }

    // 2. Loomis Canonical-Unit Proportion Ruler Staff + Interactive Crown/Unit Handle
    const auto &rg = sc.get_ruler_graph();
    float max_h = rg.get_base_unit_meters();
    for (const auto &rn : rg.nodes()) {
        if (rn.is_landmark) max_h = std::max(max_h, rn.computed_value);
    }

    const float staff_x = -0.85f;
    godot::PackedVector3Array ruler_lines;
    ruler_lines.append(godot::Vector3(staff_x, 0.0f, 0.0f));
    ruler_lines.append(godot::Vector3(staff_x, max_h, 0.0f));

    const float unit = std::max(0.05f, rg.get_base_unit_meters());
    for (float y = 0.0f; y <= max_h + 1e-3f; y += unit) {
        ruler_lines.append(godot::Vector3(staff_x - 0.07f, y, 0.0f));
        ruler_lines.append(godot::Vector3(staff_x + 0.07f, y, 0.0f));
    }
    for (const auto &rn : rg.nodes()) {
        if (!rn.is_landmark) continue;
        ruler_lines.append(godot::Vector3(staff_x, rn.computed_value, 0.0f));
        ruler_lines.append(godot::Vector3(staff_x + 0.20f, rn.computed_value, 0.0f));
    }
    r_out.add_lines(ruler_lines, GIZMO_LINE_RULER);

    uint32_t ruler_state = GIZMO_STATE_NONE;
    if (p_view.active_handle_id == HANDLE_RULER_BASE_UNIT) {
        ruler_state |= GIZMO_STATE_DRAGGING;
        if (limit_hit) ruler_state |= GIZMO_STATE_LIMIT_REACHED;
    } else if (p_view.hovered_handle_id == HANDLE_RULER_BASE_UNIT) {
        ruler_state |= GIZMO_STATE_HOVER;
    }
    r_out.add_handle(
        HANDLE_RULER_BASE_UNIT,
        godot::Vector3(staff_x, max_h, 0.0f),
        GIZMO_LINE_RULER,
        HANDLE_SHAPE_DIAMOND,
        static_cast<real_t>(0.045),
        ruler_state,
        "Canonical Scale (BodyPlan Clamped)",
        false
    );

    // 3. Loomis Center-of-Gravity Plumb Line & Support Ring
    const auto &bal = eval.balance_report;
    const GizmoSemanticRole plumb_role = bal.is_balanced ? GIZMO_PLUMB_BALANCED : GIZMO_PLUMB_UNBALANCED;
    const godot::Vector3 cog(bal.center_of_gravity.x, bal.center_of_gravity.y, bal.center_of_gravity.z);
    const godot::Vector3 gnd(bal.plumb_ground_point.x, 0.0f, bal.plumb_ground_point.z);
    r_out.add_segment(cog, gnd, plumb_role);
    r_out.add_circle(
        godot::Vector3(bal.support_center.x, 0.005f, bal.support_center.z),
        godot::Vector3(0.0f, 1.0f, 0.0f),
        std::max(0.08f, bal.support_radius),
        20,
        plumb_role
    );
}

godot::String CharacterScaffoldGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    if (p_handle_id == HANDLE_RULER_BASE_UNIT) {
        return "Canonical Unit Scale";
    }
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (ch && ch->get_character_resource().is_valid() && ch->get_character_resource()->get_body_plan().is_valid()) {
        auto sc = ch->get_character_resource()->get_body_plan()->get_base_scaffold();
        if (sc.is_valid()) {
            return sc->get_node_name(p_handle_id - HANDLE_SCAFFOLD_NODE_BASE);
        }
    }
    return "Scaffold Volume";
}

godot::Variant CharacterScaffoldGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null() || ch->get_character_resource()->get_body_plan().is_null()) {
        return godot::Variant();
    }
    auto inst = ch->get_character_resource();
    const auto &plan = inst->get_body_plan()->get_core_plan();

    if (p_handle_id == HANDLE_RULER_BASE_UNIT) {
        if (const auto *b = find_ruler_unit_bound(plan)) {
            return inst->get_parameter_value(godot::String::utf8(b->name.c_str()));
        }
        return static_cast<real_t>(plan.get_base_scaffold().get_ruler_graph().get_base_unit_meters());
    }

    auto sc = inst->get_body_plan()->get_base_scaffold();
    if (sc.is_valid()) {
        return sc->get_node_local_offset(p_handle_id - HANDLE_SCAFFOLD_NODE_BASE);
    }
    return godot::Vector3();
}

void CharacterScaffoldGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null() || ch->get_character_resource()->get_body_plan().is_null()) return;

    auto inst = ch->get_character_resource();
    const auto &plan = inst->get_body_plan()->get_core_plan();

    DragState st{};
    st.handle_id = p_handle_id;
    st.active    = true;

    if (p_handle_id == HANDLE_RULER_BASE_UNIT) {
        if (const auto *b = find_ruler_unit_bound(plan)) {
            st.param_name          = godot::String::utf8(b->name.c_str());
            st.initial_param_value = inst->get_parameter_value(st.param_name);
        }
    } else {
        auto sc = inst->get_body_plan()->get_base_scaffold();
        if (sc.is_valid()) {
            st.initial_offset = sc->get_node_local_offset(p_handle_id - HANDLE_SCAFFOLD_NODE_BASE);
        }
    }
    active_drags_[ch->get_instance_id()] = st;
}

void CharacterScaffoldGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_modifiers;
    if (!p_ray.is_valid() || !p_ray.camera) return;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null() || ch->get_character_resource()->get_body_plan().is_null()) return;

    auto inst        = ch->get_character_resource();
    auto bp_res      = inst->get_body_plan();
    const auto &plan = bp_res->get_core_plan();
    const uint64_t inst_id = ch->get_instance_id();

    if (p_handle_id == HANDLE_RULER_BASE_UNIT) {
        const auto *bound = find_ruler_unit_bound(plan);
        if (!bound) return;

        const godot::Transform3D gt = ch->get_global_transform();
        const auto &rg = plan.get_base_scaffold().get_ruler_graph();
        float max_h = rg.get_base_unit_meters();
        for (const auto &rn : rg.nodes()) {
            if (rn.is_landmark) max_h = std::max(max_h, rn.computed_value);
        }
        const float height_per_unit = std::max(1.0f, max_h / std::max(0.05f, rg.get_base_unit_meters()));

        const godot::Vector3 staff_world = gt.xform(godot::Vector3(-0.85f, max_h, 0.0f));
        const godot::Vector3 cam_fwd     = -p_ray.camera->get_global_transform().basis.get_column(2);
        const godot::Plane drag_plane(cam_fwd, staff_world);

        godot::Vector3 hit;
        if (drag_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit)) {
            const godot::Vector3 local_hit = gt.affine_inverse().xform(hit);
            const float proposed_unit      = static_cast<float>(local_hit.y) / height_per_unit;
            const float clamped_unit       = std::clamp(proposed_unit, bound->min_value, bound->max_value);
            const bool hit_limit           = (std::abs(clamped_unit - proposed_unit) > 1e-4f);
            if (active_drags_.has(inst_id)) {
                active_drags_[inst_id].limit_reached = hit_limit;
            }
            inst->set_parameter_value(godot::String::utf8(bound->name.c_str()), static_cast<real_t>(clamped_unit));
        }
        return;
    }

    // Scaffold volume offset drag clamped within BodyPlan tolerance envelope
    auto sc = bp_res->get_base_scaffold();
    if (sc.is_null()) return;

    const int nid = p_handle_id - HANDLE_SCAFFOLD_NODE_BASE;
    const godot::Transform3D gt = ch->get_global_transform();
    const godot::Vector3 cur_world = gt.xform(sc->get_node_world_position(nid));
    const godot::Vector3 cam_fwd   = -p_ray.camera->get_global_transform().basis.get_column(2);
    const godot::Plane drag_plane(cam_fwd, cur_world);

    godot::Vector3 hit;
    if (drag_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit)) {
        const godot::Vector3 delta_world = hit - cur_world;
        const godot::Vector3 delta_local = gt.basis.inverse().xform(delta_world);
        godot::Vector3 proposed = sc->get_node_local_offset(nid) + delta_local;

        // Clamp deviation from initial offset against BodyPlan anatomical drift limit (default +/-0.35m)
        const real_t max_drift = static_cast<real_t>(0.35);
        bool hit_limit = false;
        if (active_drags_.has(inst_id)) {
            const godot::Vector3 base_off = active_drags_[inst_id].initial_offset;
            const godot::Vector3 diff     = proposed - base_off;
            if (diff.length() > max_drift) {
                proposed  = base_off + diff.normalized() * max_drift;
                hit_limit = true;
            }
            active_drags_[inst_id].limit_reached = hit_limit;
        }
        sc->set_node_local_offset(nid, proposed);
    }
}

void CharacterScaffoldGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    (void)p_handle_id;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null() || ch->get_character_resource()->get_body_plan().is_null()) return;

    const uint64_t inst_id = ch->get_instance_id();
    if (!active_drags_.has(inst_id)) return;
    const DragState st = active_drags_[inst_id];
    active_drags_.erase(inst_id);
    if (!st.active) return;

    auto inst   = ch->get_character_resource();
    auto bp_res = inst->get_body_plan();
    godot::Ref<IdeamUndoRedo> ur = resolve_undo_redo();
    if (ur.is_null()) {
        ur = IdeamCharactersPlugin::create_undo_redo_wrapper();
    }

    if (st.handle_id == HANDLE_RULER_BASE_UNIT) {
        if (st.param_name.is_empty()) return;
        if (p_cancel) {
            inst->set_parameter_value(st.param_name, st.initial_param_value);
            return;
        }
        const real_t final_val = inst->get_parameter_value(st.param_name);
        inst->set_parameter_value(st.param_name, st.initial_param_value);
        inst->set_undo_redo(ur);
        inst->commit_set_parameter_value(st.param_name, final_val);
        return;
    }

    auto sc = bp_res->get_base_scaffold();
    if (sc.is_null()) return;
    const int nid = st.handle_id - HANDLE_SCAFFOLD_NODE_BASE;
    if (p_cancel) {
        sc->set_node_local_offset(nid, st.initial_offset);
        return;
    }
    const godot::Vector3 final_off = sc->get_node_local_offset(nid);
    sc->set_node_local_offset(nid, st.initial_offset);
    sc->set_undo_redo(ur);
    sc->commit_set_node_local_offset(nid, final_off);
}

} // namespace ideam::godot_ext
