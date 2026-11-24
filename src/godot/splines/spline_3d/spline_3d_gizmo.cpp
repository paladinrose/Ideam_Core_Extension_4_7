#include "spline_3d_gizmo.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void Spline3DGizmo::_bind_methods() {}

bool Spline3DGizmo::can_handle(godot::Node3D *p_target) const {
    return p_target != nullptr && godot::Object::cast_to<SplineNode3D>(p_target) != nullptr;
}

godot::String Spline3DGizmo::get_gizmo_name() const {
    return "SplineNode3D";
}

int32_t Spline3DGizmo::get_priority() const {
    return -1;
}

GizmoSemanticRole Spline3DGizmo::_role_for_curve_type(SplineCurveType p_type, bool p_selected) noexcept {
    if (p_selected) {
        return GIZMO_SNAP_PREVIEW;
    }
    switch (p_type) {
        case CURVE_LINEAR:
            return GIZMO_LINE_AXIS_NORMAL;
        case CURVE_CUBIC_BEZIER:
            return GIZMO_HANDLE_SPLINE_TANGENT;
        case CURVE_STEP:
            return GIZMO_HANDLE_FEATURE_MAJOR;
        case CURVE_CATMULL_ROM:
        default:
            return GIZMO_HANDLE_SPLINE_ANCHOR;
    }
}

godot::Vector3 Spline3DGizmo::_project_ray_to_local_plane(
    const SplineNode3D *p_node,
    const GizmoRay &p_ray,
    const godot::Vector3 &p_plane_origin_local
) {
    if (!p_node || !p_ray.is_valid()) {
        return p_plane_origin_local;
    }

    const godot::Transform3D node_xform = p_node->get_global_transform();
    const godot::Transform3D inv_xform  = node_xform.affine_inverse();

    const godot::Vector3 plane_origin_world = node_xform.xform(p_plane_origin_local);
    godot::Vector3 plane_normal_world = -p_ray.normal.normalized();
    if (p_ray.camera) {
        plane_normal_world = -p_ray.camera->get_global_transform().basis.get_column(2).normalized();
    }

    const godot::Plane view_plane(plane_normal_world, plane_origin_world);
    godot::Vector3 hit_world = plane_origin_world;
    if (view_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit_world)) {
        return inv_xform.xform(hit_world);
    }
    return p_plane_origin_local;
}

void Spline3DGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    (void)p_view;
    auto *node = godot::Object::cast_to<SplineNode3D>(p_target);
    if (!node) return;

    godot::Ref<Spline3DResource> spline = node->get_spline();
    if (spline.is_null() || spline->get_point_count() <= 0) {
        return;
    }

    const int pt_count  = spline->get_point_count();
    const int sec_count = spline->get_section_count();
    const int subdivs   = std::clamp(node->get_gizmo_subdivisions(), 2, 64);
    const int sel_sec   = node->get_selected_gizmo_section_index();
    const godot::StringName mode = get_active_tool_mode();

    // 1. Draw each section with semantic curve-type roles
    for (int s = 0; s < sec_count; ++s) {
        const SplineCurveType ctype = spline->get_section_curve_type(s);
        if (ctype == CURVE_NONE) continue;

        godot::PackedVector3Array sec_pts;
        sec_pts.resize(subdivs + 1);
        for (int step = 0; step <= subdivs; ++step) {
            const real_t local_t = static_cast<real_t>(step) / static_cast<real_t>(subdivs);
            sec_pts.set(step, spline->sample_section(s, local_t));
        }
        r_out.add_polyline(sec_pts, false, _role_for_curve_type(ctype, s == sel_sec));
    }

    // 2. Anchor handles, tangent stalks, and section midpoint split handles
    const bool show_tangents = (mode == godot::StringName() || mode == godot::StringName("edit") || mode == godot::StringName("tangents"));
    const bool show_splits   = (mode == godot::StringName() || mode == godot::StringName("edit") || mode == godot::StringName("split"));

    for (int i = 0; i < pt_count; ++i) {
        const godot::Vector3 anchor = spline->get_point_position(i);
        r_out.add_handle(
            HANDLE_ANCHOR_BASE + i,
            anchor,
            GIZMO_HANDLE_SPLINE_ANCHOR,
            HANDLE_SHAPE_SPHERE,
            static_cast<real_t>(0.055),
            GIZMO_STATE_NONE,
            godot::String("Point #") + godot::String::num_int64(i),
            false
        );

        if (show_tangents && spline->get_point_use_back_handle(i)) {
            const godot::Vector3 bh_world = anchor + spline->get_point_back_handle(i);
            r_out.add_segment(anchor, bh_world, GIZMO_HANDLE_SPLINE_TANGENT);
            r_out.add_handle(
                HANDLE_BACK_OFFSET + i,
                bh_world,
                GIZMO_HANDLE_SPLINE_TANGENT,
                HANDLE_SHAPE_DIAMOND,
                static_cast<real_t>(0.04),
                GIZMO_STATE_NONE,
                godot::String("Back Tangent #") + godot::String::num_int64(i),
                true
            );
        }

        if (show_tangents && spline->get_point_use_fore_handle(i)) {
            const godot::Vector3 fh_world = anchor + spline->get_point_fore_handle(i);
            r_out.add_segment(anchor, fh_world, GIZMO_HANDLE_SPLINE_TANGENT);
            r_out.add_handle(
                HANDLE_FORE_OFFSET + i,
                fh_world,
                GIZMO_HANDLE_SPLINE_TANGENT,
                HANDLE_SHAPE_DIAMOND,
                static_cast<real_t>(0.04),
                GIZMO_STATE_NONE,
                godot::String("Fore Tangent #") + godot::String::num_int64(i),
                true
            );
        }
    }

    if (show_splits) {
        for (int s = 0; s < sec_count; ++s) {
            if (spline->get_section_curve_type(s) == CURVE_NONE) continue;
            const godot::Vector3 mid_pt = spline->sample_section(s, static_cast<real_t>(0.5));
            r_out.add_handle(
                HANDLE_SECTION_SPLIT_OFFSET + s,
                mid_pt,
                GIZMO_SNAP_PREVIEW,
                HANDLE_SHAPE_CUBE,
                static_cast<real_t>(0.035),
                GIZMO_STATE_NONE,
                godot::String("Split Section #") + godot::String::num_int64(s),
                true
            );
        }
    }

    // 3. Optional Bishop / Up-Vector Coordinate Frame Triads
    if ((node->get_show_gizmo_frames() || mode == godot::StringName("frames")) && sec_count > 0) {
        const godot::TypedArray<godot::Transform3D> frames = spline->compute_bishop_transforms(
            std::max(2, subdivs / 2),
            node->get_up_vector()
        );
        constexpr float AXIS_LEN = 0.35f;
        for (int i = 0; i < frames.size(); ++i) {
            const godot::Transform3D xf = frames[i];
            r_out.add_segment(xf.origin, xf.origin + xf.basis.get_column(0).normalized() * AXIS_LEN, GIZMO_LINE_AXIS_TANGENT);
            r_out.add_segment(xf.origin, xf.origin + xf.basis.get_column(1).normalized() * AXIS_LEN, GIZMO_PLUMB_BALANCED);
            r_out.add_segment(xf.origin, xf.origin + xf.basis.get_column(2).normalized() * AXIS_LEN, GIZMO_LINE_AXIS_NORMAL);
        }
    }

    // 4. AABB Rotate/Scale Cage when 2+ points exist
    if (pt_count >= 2) {
        const godot::AABB box = spline->get_aabb(false);
        if (box.size.length_squared() > 1e-5f) {
            r_out.add_wire_box(box.get_center(), box.size * 0.5f, godot::Basis(), GIZMO_LINE_CLEARANCE);

            const godot::Vector3 p1 = box.position + box.size;
            r_out.add_handle(
                HANDLE_AABB_SCALE_ID,
                p1,
                GIZMO_HANDLE_SCAFFOLD,
                HANDLE_SHAPE_CUBE,
                static_cast<real_t>(0.05),
                GIZMO_STATE_NONE,
                "Scale Spline3D AABB",
                true
            );

            const godot::Vector3 center = box.get_center();
            const godot::Vector3 rot_handle(
                center.x + std::max(0.5f, box.size.x * 0.5f + 0.35f),
                center.y,
                center.z
            );
            r_out.add_handle(
                HANDLE_AABB_ROTATE_Y_ID,
                rot_handle,
                GIZMO_LINE_RULER,
                HANDLE_SHAPE_RING,
                static_cast<real_t>(0.05),
                GIZMO_STATE_NONE,
                "Rotate Spline3D Y-Axis",
                true
            );
        }
    }

    // 5. Current Playback Pose Diamond
    if (sec_count > 0) {
        const godot::Transform3D play_xf = node->get_current_transform_on_spline();
        constexpr float R = 0.16f;
        const godot::Vector3 o = play_xf.origin;
        const godot::Vector3 x = play_xf.basis.get_column(0).normalized() * R;
        const godot::Vector3 y = play_xf.basis.get_column(1).normalized() * R;
        const godot::Vector3 z = -play_xf.basis.get_column(2).normalized() * (R * 1.6f);

        godot::PackedVector3Array play_lines;
        play_lines.push_back(o + x); play_lines.push_back(o + y);
        play_lines.push_back(o + y); play_lines.push_back(o - x);
        play_lines.push_back(o - x); play_lines.push_back(o - y);
        play_lines.push_back(o - y); play_lines.push_back(o + x);
        play_lines.push_back(o + x); play_lines.push_back(o + z);
        play_lines.push_back(o - x); play_lines.push_back(o + z);
        play_lines.push_back(o + y); play_lines.push_back(o + z);
        play_lines.push_back(o - y); play_lines.push_back(o + z);
        r_out.add_lines(play_lines, GIZMO_SNAP_PREVIEW);
    }
}

godot::String Spline3DGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_target;
    (void)p_secondary;
    if (p_handle_id == HANDLE_AABB_SCALE_ID) return "Scale Spline3D AABB";
    if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) return "Rotate Spline3D Y-Axis";
    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        return godot::String("Split Section #") + godot::String::num_int64(p_handle_id - HANDLE_SECTION_SPLIT_OFFSET);
    }
    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        return godot::String("Fore Tangent #") + godot::String::num_int64(p_handle_id - HANDLE_FORE_OFFSET);
    }
    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        return godot::String("Back Tangent #") + godot::String::num_int64(p_handle_id - HANDLE_BACK_OFFSET);
    }
    return godot::String("Spline3D Point #") + godot::String::num_int64(p_handle_id);
}

godot::Variant Spline3DGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<SplineNode3D>(p_target);
    if (!node || node->get_spline().is_null()) return godot::Variant();
    auto spline = node->get_spline();

    if (p_handle_id == HANDLE_AABB_SCALE_ID ||
        p_handle_id == HANDLE_AABB_ROTATE_Y_ID ||
        p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        return spline->get_snapshot_state();
    }
    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        return spline->get_point_fore_handle(p_handle_id - HANDLE_FORE_OFFSET);
    }
    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        return spline->get_point_back_handle(p_handle_id - HANDLE_BACK_OFFSET);
    }
    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count()) {
        return spline->get_point_position(p_handle_id);
    }
    return godot::Variant();
}

void Spline3DGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    auto *node = godot::Object::cast_to<SplineNode3D>(p_target);
    if (!node || node->get_spline().is_null()) return;
    auto spline = node->get_spline();

    DragState st{};
    st.initial_snapshot = spline->get_snapshot_state();
    st.initial_aabb     = spline->get_aabb(false);
    st.initial_value    = get_handle_value(p_target, p_handle_id, false);
    st.active           = true;

    if (p_handle_id == HANDLE_AABB_SCALE_ID) {
        st.initial_handle_pos = st.initial_aabb.position + st.initial_aabb.size;
    } else if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        const godot::Vector3 c = st.initial_aabb.get_center();
        st.initial_handle_pos = godot::Vector3(c.x + std::max(0.5f, st.initial_aabb.size.x * 0.5f + 0.35f), c.y, c.z);
    } else if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        st.initial_handle_pos = spline->sample_section(p_handle_id - HANDLE_SECTION_SPLIT_OFFSET, static_cast<real_t>(0.5));
    }
    active_drags_[node->get_instance_id()] = st;
}

void Spline3DGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_modifiers;
    auto *node = godot::Object::cast_to<SplineNode3D>(p_target);
    if (!node || node->get_spline().is_null() || !p_ray.is_valid()) return;
    auto spline = node->get_spline();
    const uint64_t nid = node->get_instance_id();

    if (!active_drags_.has(nid)) {
        on_handle_drag_begin(p_target, p_handle_id, p_ray);
    }

    if (p_handle_id == HANDLE_AABB_SCALE_ID) {
        const DragState &st = active_drags_[nid];
        spline->apply_snapshot_state(st.initial_snapshot);
        const godot::Vector3 center = st.initial_aabb.get_center();
        const godot::Vector3 hit    = _project_ray_to_local_plane(node, p_ray, st.initial_handle_pos);
        const godot::Vector3 half0  = (st.initial_handle_pos - center);
        const godot::Vector3 half1  = (hit - center);

        const godot::Vector3 scale_vec(
            (std::abs(half0.x) > 1e-3f) ? std::clamp(half1.x / half0.x, 0.05f, 20.0f) : 1.0f,
            (std::abs(half0.y) > 1e-3f) ? std::clamp(half1.y / half0.y, 0.05f, 20.0f) : 1.0f,
            (std::abs(half0.z) > 1e-3f) ? std::clamp(half1.z / half0.z, 0.05f, 20.0f) : 1.0f
        );
        spline->scale_spline(scale_vec);
        node->update_gizmos();
        return;
    }

    if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        const DragState &st = active_drags_[nid];
        spline->apply_snapshot_state(st.initial_snapshot);
        const godot::Vector3 center = st.initial_aabb.get_center();
        const godot::Vector3 hit    = _project_ray_to_local_plane(node, p_ray, st.initial_handle_pos);
        const godot::Vector3 rel    = hit - center;
        const float yaw_rad         = std::atan2(-rel.z, rel.x);
        spline->rotate_spline(godot::Vector3(0.0f, yaw_rad, 0.0f));
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        node->set_selected_gizmo_section_index(p_handle_id - HANDLE_SECTION_SPLIT_OFFSET);
        return;
    }

    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        node->set_selected_gizmo_point_index(idx);
        const godot::Vector3 anchor = spline->get_point_position(idx);
        const godot::Vector3 cur_w  = anchor + spline->get_point_fore_handle(idx);
        const godot::Vector3 new_w  = _project_ray_to_local_plane(node, p_ray, cur_w);
        spline->set_point_fore_handle(idx, new_w - anchor, true);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        const int idx = p_handle_id - HANDLE_BACK_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        node->set_selected_gizmo_point_index(idx);
        const godot::Vector3 anchor = spline->get_point_position(idx);
        const godot::Vector3 cur_w  = anchor + spline->get_point_back_handle(idx);
        const godot::Vector3 new_w  = _project_ray_to_local_plane(node, p_ray, cur_w);
        spline->set_point_back_handle(idx, new_w - anchor, true);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count()) {
        node->set_selected_gizmo_point_index(p_handle_id);
        const godot::Vector3 cur_pos = spline->get_point_position(p_handle_id);
        const godot::Vector3 new_pos = _project_ray_to_local_plane(node, p_ray, cur_pos);
        spline->set_point_position(p_handle_id, new_pos);
        node->update_gizmos();
    }
}

void Spline3DGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    auto *node = godot::Object::cast_to<SplineNode3D>(p_target);
    if (!node || node->get_spline().is_null()) return;
    auto spline = node->get_spline();
    const uint64_t nid = node->get_instance_id();

    DragState st{};
    const bool had_state = active_drags_.has(nid);
    if (had_state) {
        st = active_drags_[nid];
        active_drags_.erase(nid);
    }

    if (spline->get_undo_redo().is_null()) {
        godot::Ref<IdeamUndoRedo> ur = resolve_undo_redo();
        if (ur.is_null()) ur = IdeamSplinesPlugin::create_undo_redo_wrapper();
        spline->set_undo_redo(ur);
    }

    if (p_handle_id == HANDLE_AABB_SCALE_ID || p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        if (p_cancel && had_state) {
            spline->apply_snapshot_state(st.initial_snapshot);
        } else if (had_state) {
            spline->commit_snapshot_action(
                (p_handle_id == HANDLE_AABB_SCALE_ID) ? "Scale Spline3D via Gizmo" : "Rotate Spline3D via Gizmo",
                st.initial_snapshot,
                spline->get_snapshot_state()
            );
        }
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        const int sec_idx = p_handle_id - HANDLE_SECTION_SPLIT_OFFSET;
        if (!p_cancel && sec_idx >= 0 && sec_idx < spline->get_section_count()) {
            const godot::Vector3 mid_pt    = spline->sample_section(sec_idx, static_cast<real_t>(0.5));
            const SplineCurveType ctype    = spline->get_section_curve_type(sec_idx);
            const real_t half_weight       = std::max(static_cast<real_t>(0.01), spline->get_section_weight(sec_idx) * static_cast<real_t>(0.5));
            const godot::Dictionary before = spline->get_snapshot_state();

            spline->set_section_weight(sec_idx, half_weight);
            if (sec_idx == spline->get_point_count() - 1 && spline->get_wrap()) {
                spline->add_point(mid_pt, ctype, half_weight);
                node->set_selected_gizmo_point_index(spline->get_point_count() - 1);
            } else {
                spline->insert_point(sec_idx + 1, mid_pt, ctype, half_weight);
                node->set_selected_gizmo_point_index(sec_idx + 1);
            }
            spline->commit_snapshot_action("Split Spline3D Section via Gizmo", before, spline->get_snapshot_state());
        }
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count() && had_state) {
            const godot::Vector3 prev_h = st.initial_value;
            const godot::Vector3 cur_h  = spline->get_point_fore_handle(idx);
            spline->set_point_fore_handle(idx, prev_h, true);
            if (!p_cancel) spline->commit_set_point_fore_handle(idx, cur_h, true);
        }
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        const int idx = p_handle_id - HANDLE_BACK_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count() && had_state) {
            const godot::Vector3 prev_h = st.initial_value;
            const godot::Vector3 cur_h  = spline->get_point_back_handle(idx);
            spline->set_point_back_handle(idx, prev_h, true);
            if (!p_cancel) spline->commit_set_point_back_handle(idx, cur_h, true);
        }
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count() && had_state) {
        const godot::Vector3 prev_pos = st.initial_value;
        const godot::Vector3 cur_pos  = spline->get_point_position(p_handle_id);
        spline->set_point_position(p_handle_id, prev_pos);
        if (!p_cancel) spline->commit_set_point_position(p_handle_id, cur_pos);
    }

    node->update_gizmos();
}

} // namespace ideam::godot_ext