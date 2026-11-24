#include "character_topology_gizmo.h"

#include "ideam_characters_plugin.h"
#include "loomis_character_node_3d.h"

#include <godot_cpp/variant/plane.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

CharacterTopologyGizmo::CharacterTopologyGizmo() {
    set_active_tool_mode(godot::StringName("scaffolding"));
}

bool CharacterTopologyGizmo::can_handle(godot::Node3D *p_target) const {
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null()) return false;
    return get_active_tool_mode() == godot::StringName("topology");
}

void CharacterTopologyGizmo::build_draw_list(
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

    // 1. Render Topology Landmarks (anchored to FormScaffoldCore frames) + Sharpness Rings
    for (const auto &lf : eval.landmark_frames) {
        const godot::Vector3 pos(lf.world_frame.position.x, lf.world_frame.position.y, lf.world_frame.position.z);
        const godot::Vector3 nrm(lf.world_frame.normal.x, lf.world_frame.normal.y, lf.world_frame.normal.z);

        const int32_t hid = HANDLE_LANDMARK_BASE + static_cast<int32_t>(lf.landmark_id);
        uint32_t state = GIZMO_STATE_NONE;
        if (!lf.is_mirrored_copy) {
            if (p_view.active_handle_id == hid) state |= GIZMO_STATE_DRAGGING;
            else if (p_view.hovered_handle_id == hid) state |= GIZMO_STATE_HOVER;

            r_out.add_handle(
                hid,
                pos,
                GIZMO_HANDLE_SPLINE_ANCHOR,
                HANDLE_SHAPE_DIAMOND,
                static_cast<real_t>(0.026),
                state,
                godot::String::utf8(lf.name.c_str()),
                false
            );
        }
        r_out.add_circle(pos, nrm, static_cast<real_t>(0.032), 14, GIZMO_HANDLE_SPLINE_ANCHOR, state);
        r_out.add_cone_arrow(pos, nrm, static_cast<real_t>(0.065), static_cast<real_t>(0.012), GIZMO_HANDLE_SPLINE_TANGENT, state);
    }

    // 2. Highlight Separable Part Seam Loops (`PartSeamContract`) in distinct semantic style
    for (const auto &seam : plan.part_seams()) {
        const auto *na = eval.evaluated_scaffold.get_node(seam.part_a_id);
        const auto *nb = eval.evaluated_scaffold.get_node(seam.part_b_id);
        if (!na || !nb) continue;

        const godot::Vector3 pa(na->world_transform[3][0], na->world_transform[3][1], na->world_transform[3][2]);
        const godot::Vector3 pb(nb->world_transform[3][0], nb->world_transform[3][1], nb->world_transform[3][2]);
        const godot::Vector3 mid = (pa + pb) * static_cast<real_t>(0.5);
        const godot::Vector3 dir = (pb - pa).length_squared() > 1e-6f
            ? (pb - pa).normalized()
            : godot::Vector3(0.0f, 1.0f, 0.0f);
        const real_t ring_r = std::max(
            static_cast<real_t>(0.045),
            static_cast<real_t>(std::min(nb->evaluated_half_extents.x, nb->evaluated_half_extents.z)) * static_cast<real_t>(0.95)
        );
        r_out.add_circle(mid, dir, ring_r, static_cast<int>(seam.canonical_ring_count), GIZMO_LINE_SEAM_HOLE);
    }
}

godot::String CharacterTopologyGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (ch && ch->get_character_resource().is_valid() && ch->get_character_resource()->get_body_plan().is_valid()) {
        const uint32_t lid = static_cast<uint32_t>(std::max(0, p_handle_id - HANDLE_LANDMARK_BASE));
        for (const auto &lm : ch->get_character_resource()->get_body_plan()->get_core_plan().landmarks()) {
            if (lm.id == lid) {
                return godot::String("Landmark: ") + godot::String::utf8(lm.name.c_str());
            }
        }
    }
    return "Topology Landmark";
}

godot::Variant CharacterTopologyGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null() || ch->get_character_resource()->get_body_plan().is_null()) {
        return static_cast<real_t>(0.0);
    }
    const uint32_t lid = static_cast<uint32_t>(std::max(0, p_handle_id - HANDLE_LANDMARK_BASE));
    for (const auto &lm : ch->get_character_resource()->get_body_plan()->get_core_plan().landmarks()) {
        if (lm.id == lid) {
            return static_cast<real_t>(lm.anchor.normal_offset);
        }
    }
    return static_cast<real_t>(0.0);
}

void CharacterTopologyGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null() || ch->get_character_resource()->get_body_plan().is_null()) return;

    auto inst = ch->get_character_resource();
    const auto &plan = inst->get_body_plan()->get_core_plan();
    const uint32_t lid = static_cast<uint32_t>(std::max(0, p_handle_id - HANDLE_LANDMARK_BASE));
    const auto eval = inst->get_core_instance().evaluate_stack(plan, 3u);

    DragState st{};
    st.landmark_id = static_cast<int32_t>(lid);
    st.active      = true;
    for (const auto &lm : plan.landmarks()) {
        if (lm.id == lid) {
            st.initial_normal_offset = static_cast<real_t>(lm.anchor.normal_offset);
            break;
        }
    }
    const godot::Transform3D gt = ch->get_global_transform();
    for (const auto &lf : eval.landmark_frames) {
        if (lf.landmark_id == lid && !lf.is_mirrored_copy) {
            st.anchor_world_pos    = gt.xform(godot::Vector3(lf.world_frame.position.x, lf.world_frame.position.y, lf.world_frame.position.z));
            st.anchor_world_normal = gt.basis.xform(godot::Vector3(lf.world_frame.normal.x, lf.world_frame.normal.y, lf.world_frame.normal.z)).normalized();
            break;
        }
    }
    active_drags_[ch->get_instance_id()] = st;
}

void CharacterTopologyGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_handle_id;
    (void)p_modifiers;
    if (!p_ray.is_valid() || !p_ray.camera) return;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null() || ch->get_character_resource()->get_body_plan().is_null()) return;

    const uint64_t inst_id = ch->get_instance_id();
    if (!active_drags_.has(inst_id)) return;
    const DragState &st = active_drags_[inst_id];
    if (!st.active) return;

    const godot::Vector3 cam_fwd = -p_ray.camera->get_global_transform().basis.get_column(2);
    const godot::Plane drag_plane(cam_fwd, st.anchor_world_pos);
    godot::Vector3 hit;
    if (drag_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit)) {
        const real_t proj = (hit - st.anchor_world_pos).dot(st.anchor_world_normal);
        const real_t next_off = std::clamp(st.initial_normal_offset + proj, static_cast<real_t>(-0.08), static_cast<real_t>(0.08));
        ch->get_character_resource()->get_body_plan()->update_topology_landmark_offset(st.landmark_id, next_off);
    }
}

void CharacterTopologyGizmo::on_handle_drag_commit(
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

    auto bp = ch->get_character_resource()->get_body_plan();
    if (p_cancel) {
        bp->update_topology_landmark_offset(st.landmark_id, st.initial_normal_offset);
        return;
    }

    real_t final_off = st.initial_normal_offset;
    for (const auto &lm : bp->get_core_plan().landmarks()) {
        if (lm.id == static_cast<uint32_t>(st.landmark_id)) {
            final_off = static_cast<real_t>(lm.anchor.normal_offset);
            break;
        }
    }

    godot::Ref<IdeamUndoRedo> ur = resolve_undo_redo();
    if (ur.is_null()) ur = IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        bp->update_topology_landmark_offset(st.landmark_id, st.initial_normal_offset);
        ur->create_action("Adjust Character Topology Landmark");
        ur->add_do_method(bp.ptr(), "update_topology_landmark_offset", st.landmark_id, final_off, static_cast<real_t>(-1.0));
        ur->add_undo_method(bp.ptr(), "update_topology_landmark_offset", st.landmark_id, st.initial_normal_offset, static_cast<real_t>(-1.0));
        ur->commit_action();
    }
}

} // namespace ideam::godot_ext
