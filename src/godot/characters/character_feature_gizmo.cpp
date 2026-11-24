#include "character_feature_gizmo.h"

#include "ideam_characters_plugin.h"
#include "loomis_character_node_3d.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/plane.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void CharacterFeatureGizmo::_bind_methods() {
    using namespace godot;
    ADD_SIGNAL(MethodInfo("feature_focused", PropertyInfo(Variant::INT, "feature_id")));
    ClassDB::bind_method(D_METHOD("set_active_feature_id", "feature_id"), &CharacterFeatureGizmo::set_active_feature_id);
    ClassDB::bind_method(D_METHOD("get_active_feature_id"), &CharacterFeatureGizmo::get_active_feature_id);
    ClassDB::bind_method(D_METHOD("is_in_overview_state"), &CharacterFeatureGizmo::is_in_overview_state);
}

CharacterFeatureGizmo::CharacterFeatureGizmo() {
    set_active_tool_mode(godot::StringName("scaffolding"));
}

void CharacterFeatureGizmo::set_active_feature_id(int32_t p_feature_id) {
    if (active_feature_id_ == p_feature_id) return;
    active_feature_id_ = p_feature_id;
    emit_signal("feature_focused", active_feature_id_);
    request_redraw();
}

bool CharacterFeatureGizmo::can_handle(godot::Node3D *p_target) const {
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null()) return false;
    return get_active_tool_mode() == godot::StringName("features");
}

bool CharacterFeatureGizmo::on_modifier_state_changed(
    godot::Node3D *p_target,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_target;
    const bool shift_now = p_modifiers.has_flag(godot::KEY_MASK_SHIFT);
    if (shift_now != shift_overview_held_) {
        shift_overview_held_ = shift_now;
        return true; // Immediate transition between State A (Overview) and State B (Isolated Detail)
    }
    return false;
}

bool CharacterFeatureGizmo::on_handle_clicked(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_target;
    if (p_handle_id >= HANDLE_MAJOR_FEATURE_BASE && p_handle_id < HANDLE_DETAIL_SCALE) {
        if (p_modifiers.has_flag(godot::KEY_MASK_SHIFT) || active_feature_id_ < 0) {
            set_active_feature_id(p_handle_id - HANDLE_MAJOR_FEATURE_BASE);
            return true;
        }
    }
    return false;
}

void CharacterFeatureGizmo::build_draw_list(
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

    const bool overview_mode = p_view.modifiers.has_flag(godot::KEY_MASK_SHIFT) || is_in_overview_state();

    auto find_host_frame = [&](uint32_t p_landmark_id, core::SurfaceFrame3DPOD<float> &r_out_frame) -> bool {
        for (const auto &lf : eval.landmark_frames) {
            if (lf.landmark_id == p_landmark_id && !lf.is_mirrored_copy) {
                r_out_frame = lf.world_frame;
                return true;
            }
        }
        return false;
    };

    if (overview_mode) {
        // State A — Feature Overview: show high-visibility major feature badges/handles, hide detail handles
        for (const auto &feat : plan.features()) {
            core::SurfaceFrame3DPOD<float> host{};
            if (!find_host_frame(feat.parent_landmark_id, host)) continue;

            const glm::vec3 org = host.offset_point(feat.local_offset_ntuv.x, feat.local_offset_ntuv.y, feat.local_offset_ntuv.z);
            const godot::Vector3 pos(org.x, org.y, org.z);
            const godot::Vector3 nrm(host.normal.x, host.normal.y, host.normal.z);
            const int32_t hid = HANDLE_MAJOR_FEATURE_BASE + static_cast<int32_t>(feat.id);

            uint32_t state = GIZMO_STATE_NONE;
            if (static_cast<int32_t>(feat.id) == active_feature_id_) state |= GIZMO_STATE_DRAGGING;
            else if (p_view.hovered_handle_id == hid) state |= GIZMO_STATE_HOVER;

            r_out.add_handle(
                hid,
                pos,
                GIZMO_HANDLE_FEATURE_MAJOR,
                HANDLE_SHAPE_RING,
                static_cast<real_t>(0.032),
                state,
                godot::String::utf8(feat.name.c_str()),
                false
            );
            r_out.add_circle(pos, nrm, static_cast<real_t>(0.038), 16, GIZMO_HANDLE_FEATURE_MAJOR, state);
        }
        return;
    }

    // State B — Isolated Feature Detail: render ONLY fine detail gizmos for `active_feature_id_`
    for (const auto &feat : plan.features()) {
        if (static_cast<int32_t>(feat.id) != active_feature_id_) continue;

        core::SurfaceFrame3DPOD<float> host{};
        if (!find_host_frame(feat.parent_landmark_id, host)) return;

        const glm::vec3 org = host.offset_point(feat.local_offset_ntuv.x, feat.local_offset_ntuv.y, feat.local_offset_ntuv.z);
        const godot::Vector3 pos(org.x, org.y, org.z);
        const godot::Vector3 tu(host.tangent_u.x, host.tangent_u.y, host.tangent_u.z);
        const godot::Vector3 tv(host.tangent_v.x, host.tangent_v.y, host.tangent_v.z);
        const godot::Vector3 nrm(host.normal.x, host.normal.y, host.normal.z);

        const godot::Vector3 scale_pos  = pos + tu * static_cast<real_t>(feat.base_half_extents.x * 1.4f);
        const godot::Vector3 sec_pos    = pos + tv * static_cast<real_t>(feat.base_half_extents.y * 1.4f);
        const godot::Vector3 offset_pos = pos + nrm * static_cast<real_t>(feat.base_half_extents.z * 1.5f);

        r_out.add_segment(pos, scale_pos, GIZMO_HANDLE_FEATURE_DETAIL);
        r_out.add_segment(pos, sec_pos, GIZMO_HANDLE_FEATURE_DETAIL);
        r_out.add_segment(pos, offset_pos, GIZMO_HANDLE_FEATURE_DETAIL);
        r_out.add_circle(pos, nrm, static_cast<real_t>(std::max(feat.base_half_extents.x, feat.base_half_extents.y)), 18, GIZMO_HANDLE_FEATURE_DETAIL);

        auto state_for = [&](int32_t p_hid) {
            uint32_t s = GIZMO_STATE_NONE;
            if (p_view.active_handle_id == p_hid) s |= GIZMO_STATE_DRAGGING;
            else if (p_view.hovered_handle_id == p_hid) s |= GIZMO_STATE_HOVER;
            return s;
        };

        r_out.add_handle(HANDLE_DETAIL_SCALE,     scale_pos,  GIZMO_HANDLE_FEATURE_DETAIL, HANDLE_SHAPE_CUBE,    static_cast<real_t>(0.018), state_for(HANDLE_DETAIL_SCALE),     "Feature Scale", false);
        r_out.add_handle(HANDLE_DETAIL_SECONDARY, sec_pos,    GIZMO_HANDLE_FEATURE_DETAIL, HANDLE_SHAPE_DIAMOND, static_cast<real_t>(0.018), state_for(HANDLE_DETAIL_SECONDARY), "Feature Shape Param", true);
        r_out.add_handle(HANDLE_DETAIL_OFFSET,    offset_pos, GIZMO_HANDLE_FEATURE_DETAIL, HANDLE_SHAPE_SPHERE,  static_cast<real_t>(0.018), state_for(HANDLE_DETAIL_OFFSET),    "Feature Projection", false);
        return;
    }
}

godot::String CharacterFeatureGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    if (p_handle_id == HANDLE_DETAIL_SCALE) return "Feature Scale";
    if (p_handle_id == HANDLE_DETAIL_SECONDARY) return "Feature Shape Param";
    if (p_handle_id == HANDLE_DETAIL_OFFSET) return "Feature Projection";

    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (ch && ch->get_character_resource().is_valid() && ch->get_character_resource()->get_body_plan().is_valid()) {
        const uint32_t fid = static_cast<uint32_t>(std::max(0, p_handle_id - HANDLE_MAJOR_FEATURE_BASE));
        for (const auto &f : ch->get_character_resource()->get_body_plan()->get_core_plan().features()) {
            if (f.id == fid) {
                return godot::String("Feature: ") + godot::String::utf8(f.name.c_str());
            }
        }
    }
    return "Feature";
}

godot::Variant CharacterFeatureGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_target;
    (void)p_handle_id;
    (void)p_secondary;
    return godot::Variant(active_feature_id_);
}

void CharacterFeatureGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    auto *ch = godot::Object::cast_to<LoomisCharacterNode3D>(p_target);
    if (!ch || ch->get_character_resource().is_null() || ch->get_character_resource()->get_body_plan().is_null()) return;

    // Clicking a major feature handle in Overview focuses it immediately
    if (p_handle_id >= HANDLE_MAJOR_FEATURE_BASE && p_handle_id < HANDLE_DETAIL_SCALE) {
        set_active_feature_id(p_handle_id - HANDLE_MAJOR_FEATURE_BASE);
        return;
    }

    if (active_feature_id_ < 0) return;

    auto inst = ch->get_character_resource();
    const auto &plan = inst->get_body_plan()->get_core_plan();
    const auto eval  = inst->get_core_instance().evaluate_stack(plan, 3u);

    for (const auto &feat : plan.features()) {
        if (static_cast<int32_t>(feat.id) != active_feature_id_) continue;
        DragState st{};
        st.feature_id        = active_feature_id_;
        st.detail_handle     = p_handle_id;
        st.initial_extents   = godot::Vector3(feat.base_half_extents.x, feat.base_half_extents.y, feat.base_half_extents.z);
        st.initial_offset    = godot::Vector3(feat.local_offset_ntuv.x, feat.local_offset_ntuv.y, feat.local_offset_ntuv.z);
        st.initial_secondary = static_cast<real_t>(feat.secondary_param);
        st.active            = true;

        const godot::Transform3D gt = ch->get_global_transform();
        for (const auto &lf : eval.landmark_frames) {
            if (lf.landmark_id == feat.parent_landmark_id && !lf.is_mirrored_copy) {
                const glm::vec3 org = lf.world_frame.offset_point(feat.local_offset_ntuv.x, feat.local_offset_ntuv.y, feat.local_offset_ntuv.z);
                st.anchor_world_pos = gt.xform(godot::Vector3(org.x, org.y, org.z));
                st.anchor_world_u   = gt.basis.xform(godot::Vector3(lf.world_frame.tangent_u.x, lf.world_frame.tangent_u.y, lf.world_frame.tangent_u.z)).normalized();
                st.anchor_world_v   = gt.basis.xform(godot::Vector3(lf.world_frame.tangent_v.x, lf.world_frame.tangent_v.y, lf.world_frame.tangent_v.z)).normalized();
                st.anchor_world_n   = gt.basis.xform(godot::Vector3(lf.world_frame.normal.x, lf.world_frame.normal.y, lf.world_frame.normal.z)).normalized();
                break;
            }
        }
        active_drags_[ch->get_instance_id()] = st;
        break;
    }
}

void CharacterFeatureGizmo::on_handle_drag_update(
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
    if (!drag_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit)) return;

    auto bp = ch->get_character_resource()->get_body_plan();
    const godot::Vector3 delta = hit - st.anchor_world_pos;

    godot::Vector3 next_ext = st.initial_extents;
    godot::Vector3 next_off = st.initial_offset;
    real_t next_sec         = st.initial_secondary;

    if (st.detail_handle == HANDLE_DETAIL_SCALE) {
        const real_t du = delta.dot(st.anchor_world_u);
        const real_t scale_f = std::clamp(static_cast<real_t>(1.0) + du * static_cast<real_t>(12.0), static_cast<real_t>(0.35), static_cast<real_t>(2.5));
        next_ext = st.initial_extents * scale_f;
    } else if (st.detail_handle == HANDLE_DETAIL_SECONDARY) {
        const real_t dv = delta.dot(st.anchor_world_v);
        next_sec = std::clamp(st.initial_secondary + dv * static_cast<real_t>(8.0), static_cast<real_t>(-1.0), static_cast<real_t>(1.5));
    } else if (st.detail_handle == HANDLE_DETAIL_OFFSET) {
        const real_t dn = delta.dot(st.anchor_world_n);
        next_off.x = std::clamp(st.initial_offset.x + dn, static_cast<real_t>(-0.05), static_cast<real_t>(0.08));
    }

    bp->update_feature_params(st.feature_id, next_ext, next_sec, next_off);
}

void CharacterFeatureGizmo::on_handle_drag_commit(
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
        bp->update_feature_params(st.feature_id, st.initial_extents, st.initial_secondary, st.initial_offset);
        return;
    }

    godot::Vector3 final_ext = st.initial_extents;
    godot::Vector3 final_off = st.initial_offset;
    real_t final_sec         = st.initial_secondary;
    for (const auto &f : bp->get_core_plan().features()) {
        if (static_cast<int32_t>(f.id) == st.feature_id) {
            final_ext = godot::Vector3(f.base_half_extents.x, f.base_half_extents.y, f.base_half_extents.z);
            final_off = godot::Vector3(f.local_offset_ntuv.x, f.local_offset_ntuv.y, f.local_offset_ntuv.z);
            final_sec = static_cast<real_t>(f.secondary_param);
            break;
        }
    }

    godot::Ref<IdeamUndoRedo> ur = resolve_undo_redo();
    if (ur.is_null()) ur = IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        bp->update_feature_params(st.feature_id, st.initial_extents, st.initial_secondary, st.initial_offset);
        ur->create_action("Refine Character Feature");
        ur->add_do_method(bp.ptr(), "update_feature_params", st.feature_id, final_ext, final_sec, final_off);
        ur->add_undo_method(bp.ptr(), "update_feature_params", st.feature_id, st.initial_extents, st.initial_secondary, st.initial_offset);
        ur->commit_action();
    }
}

} // namespace ideam::godot_ext
