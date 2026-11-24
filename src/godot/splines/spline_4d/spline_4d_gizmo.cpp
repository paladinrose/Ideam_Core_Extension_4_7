#include "spline_4d_gizmo.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::godot_ext {

void Spline4DGizmo::_bind_methods() {}

godot::Ref<Spline4DResource> Spline4DGizmo::_resolve_spline_4d(godot::Node3D *p_node) {
    if (!p_node) return godot::Ref<Spline4DResource>();
    if (SplineNode4D *n4d = godot::Object::cast_to<SplineNode4D>(p_node)) {
        return n4d->get_spline();
    }
    const godot::Variant prop = p_node->get("spline");
    if (prop.get_type() == godot::Variant::OBJECT) {
        return godot::Ref<Spline4DResource>(godot::Object::cast_to<Spline4DResource>(prop.operator godot::Object *()));
    }
    return godot::Ref<Spline4DResource>();
}

bool Spline4DGizmo::_is_w_banking_roll(godot::Node3D *p_node) {
    if (SplineNode4D *n4d = godot::Object::cast_to<SplineNode4D>(p_node)) {
        return n4d->get_w_channel_mode() == SPLINE_4D_W_BANKING_ROLL;
    }
    return false;
}

bool Spline4DGizmo::can_handle(godot::Node3D *p_target) const {
    if (!p_target) return false;
    if (godot::Object::cast_to<SplineNode4D>(p_target) != nullptr) return true;
    return _resolve_spline_4d(p_target).is_valid();
}

godot::String Spline4DGizmo::get_gizmo_name() const {
    return "SplineNode4D";
}

int32_t Spline4DGizmo::get_priority() const {
    return -1;
}

GizmoSemanticRole Spline4DGizmo::_role_for_curve_type(SplineCurveType p_type, bool p_selected) noexcept {
    if (p_selected) return GIZMO_SNAP_PREVIEW;
    switch (p_type) {
        case CURVE_LINEAR:       return GIZMO_LINE_AXIS_NORMAL;
        case CURVE_CUBIC_BEZIER: return GIZMO_HANDLE_SPLINE_TANGENT;
        case CURVE_STEP:         return GIZMO_HANDLE_FEATURE_MAJOR;
        case CURVE_CATMULL_ROM:
        default:                 return GIZMO_HANDLE_SPLINE_ANCHOR;
    }
}

godot::Basis Spline4DGizmo::_compute_anchor_frame_basis(
    const godot::Ref<Spline4DResource> &p_spline,
    int p_point_index,
    const godot::Vector3 &p_up_hint
) {
    if (p_spline.is_null() || p_spline->get_point_count() <= 0) return godot::Basis();
    const int sec_count = p_spline->get_section_count();
    real_t pct = static_cast<real_t>(0.0);
    if (sec_count > 0) {
        pct = (p_point_index < sec_count)
            ? p_spline->section_local_to_global_percent(p_point_index, static_cast<real_t>(0.0), true)
            : static_cast<real_t>(1.0);
    }
    return p_spline->get_transform_3d_at_percent(pct, true, true, p_up_hint).basis.orthonormalized();
}

godot::Vector3 Spline4DGizmo::_compute_w_ring_handle_pos(
    const godot::Ref<Spline4DResource> &p_spline,
    int p_point_index,
    bool p_w_is_roll,
    const godot::Vector3 &p_up_hint
) {
    const godot::Vector3 anchor = p_spline->get_point_xyz(p_point_index);
    const float w_val           = static_cast<float>(p_spline->get_point_w(p_point_index));
    const godot::Basis basis    = _compute_anchor_frame_basis(p_spline, p_point_index, p_up_hint);
    const godot::Vector3 right  = basis.get_column(0).normalized();
    const godot::Vector3 up     = basis.get_column(1).normalized();

    if (p_w_is_roll) {
        constexpr float ROLL_DIAL_RADIUS = 0.65f;
        return anchor + (right * std::cos(w_val) + up * std::sin(w_val)) * ROLL_DIAL_RADIUS;
    }
    const float radius = std::clamp(std::abs(w_val), 0.08f, 25.0f);
    return anchor + right * radius;
}

godot::Vector3 Spline4DGizmo::_project_ray_to_local_plane(
    const godot::Node3D *p_node,
    const GizmoRay &p_ray,
    const godot::Vector3 &p_plane_origin_local,
    const godot::Vector3 &p_custom_normal_local
) {
    if (!p_node || !p_ray.is_valid()) return p_plane_origin_local;

    const godot::Transform3D node_xform = p_node->get_global_transform();
    const godot::Transform3D inv_xform  = node_xform.affine_inverse();
    const godot::Vector3 plane_origin_world = node_xform.xform(p_plane_origin_local);

    godot::Vector3 plane_normal_world = -p_ray.normal.normalized();
    if (p_ray.camera) {
        plane_normal_world = -p_ray.camera->get_global_transform().basis.get_column(2).normalized();
    }
    if (p_custom_normal_local.length_squared() > 1e-4f) {
        const godot::Vector3 candidate = node_xform.basis.xform(p_custom_normal_local).normalized();
        if (std::abs(candidate.dot(p_ray.normal)) > 0.12f) {
            plane_normal_world = candidate;
        }
    }

    const godot::Plane plane(plane_normal_world, plane_origin_world);
    godot::Vector3 hit_world = plane_origin_world;
    if (plane.intersects_ray(p_ray.origin, p_ray.normal, &hit_world)) {
        return inv_xform.xform(hit_world);
    }
    return p_plane_origin_local;
}

void Spline4DGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    (void)p_view;
    godot::Ref<Spline4DResource> spline = _resolve_spline_4d(p_target);
    if (spline.is_null() || spline->get_point_count() <= 0) return;

    SplineNode4D *n4d   = godot::Object::cast_to<SplineNode4D>(p_target);
    const int pt_count  = spline->get_point_count();
    const int sec_count = spline->get_section_count();
    const int subdivs   = n4d ? std::clamp(n4d->get_gizmo_subdivisions(), 2, 64) : 12;
    const int sel_pt    = n4d ? n4d->get_selected_gizmo_point_index() : -1;
    const int sel_sec   = n4d ? n4d->get_selected_gizmo_section_index() : -1;
    const bool w_roll   = _is_w_banking_roll(p_target);
    const godot::Vector3 up_hint = n4d ? n4d->get_up_vector() : godot::Vector3(0.0f, 1.0f, 0.0f);

    // 1. 3D spatial curve sections
    for (int s = 0; s < sec_count; ++s) {
        const SplineCurveType ctype = spline->get_section_curve_type(s);
        if (ctype == CURVE_NONE) continue;

        godot::PackedVector3Array pts;
        pts.resize(subdivs + 1);
        for (int step = 0; step <= subdivs; ++step) {
            const real_t local_t = static_cast<real_t>(step) / static_cast<real_t>(subdivs);
            const godot::Vector4 p4 = spline->sample_section(s, local_t);
            pts.set(step, godot::Vector3(p4.x, p4.y, p4.z));
        }
        r_out.add_polyline(pts, false, _role_for_curve_type(ctype, s == sel_sec));
    }

    // 2. Primary anchors, radial W-rings, and tangent stalks
    for (int i = 0; i < pt_count; ++i) {
        const godot::Vector3 anchor = spline->get_point_xyz(i);
        const float w_val           = static_cast<float>(spline->get_point_w(i));

        r_out.add_handle(
            HANDLE_ANCHOR_BASE + i,
            anchor,
            GIZMO_HANDLE_SPLINE_ANCHOR,
            HANDLE_SHAPE_SPHERE,
            static_cast<real_t>(0.055),
            GIZMO_STATE_NONE,
            godot::String("Spline4D Anchor #") + godot::String::num_int64(i),
            false
        );

        const godot::Basis frame_basis = _compute_anchor_frame_basis(spline, i, up_hint);
        const godot::Vector3 tangent   = frame_basis.get_column(2).normalized();
        const float ring_radius        = w_roll ? 0.65f : std::clamp(std::abs(w_val), 0.08f, 25.0f);
        const GizmoSemanticRole r_role = (i == sel_pt) ? GIZMO_SNAP_PREVIEW : GIZMO_HANDLE_FEATURE_MAJOR;

        r_out.add_circle(anchor, tangent, ring_radius, 24, r_role);

        const godot::Vector3 w_handle_pos = _compute_w_ring_handle_pos(spline, i, w_roll, up_hint);
        r_out.add_segment(anchor, w_handle_pos, r_role);
        r_out.add_handle(
            HANDLE_W_RING_OFFSET + i,
            w_handle_pos,
            r_role,
            HANDLE_SHAPE_RING,
            static_cast<real_t>(0.045),
            GIZMO_STATE_NONE,
            godot::String("W-Attribute #") + godot::String::num_int64(i),
            true
        );

        if (spline->get_point_use_back_handle(i)) {
            const godot::Vector4 bh4 = spline->get_point_back_handle(i);
            const godot::Vector3 bh_w = anchor + godot::Vector3(bh4.x, bh4.y, bh4.z);
            r_out.add_segment(anchor, bh_w, GIZMO_HANDLE_SPLINE_TANGENT);
            r_out.add_handle(
                HANDLE_BACK_OFFSET + i,
                bh_w,
                GIZMO_HANDLE_SPLINE_TANGENT,
                HANDLE_SHAPE_DIAMOND,
                static_cast<real_t>(0.04),
                GIZMO_STATE_NONE,
                godot::String("4D Back Tangent #") + godot::String::num_int64(i),
                true
            );
        }
        if (spline->get_point_use_fore_handle(i)) {
            const godot::Vector4 fh4 = spline->get_point_fore_handle(i);
            const godot::Vector3 fh_w = anchor + godot::Vector3(fh4.x, fh4.y, fh4.z);
            r_out.add_segment(anchor, fh_w, GIZMO_HANDLE_SPLINE_TANGENT);
            r_out.add_handle(
                HANDLE_FORE_OFFSET + i,
                fh_w,
                GIZMO_HANDLE_SPLINE_TANGENT,
                HANDLE_SHAPE_DIAMOND,
                static_cast<real_t>(0.04),
                GIZMO_STATE_NONE,
                godot::String("4D Fore Tangent #") + godot::String::num_int64(i),
                true
            );
        }
    }

    // Section midpoint split handles
    for (int s = 0; s < sec_count; ++s) {
        if (spline->get_section_curve_type(s) == CURVE_NONE) continue;
        const godot::Vector4 mid4 = spline->sample_section(s, static_cast<real_t>(0.5));
        r_out.add_handle(
            HANDLE_SECTION_SPLIT_OFFSET + s,
            godot::Vector3(mid4.x, mid4.y, mid4.z),
            GIZMO_SNAP_PREVIEW,
            HANDLE_SHAPE_CUBE,
            static_cast<real_t>(0.035),
            GIZMO_STATE_NONE,
            godot::String("Split 4D Section #") + godot::String::num_int64(s),
            true
        );
    }

    // 3. Optional Bishop Frame Triads
    if (n4d && n4d->get_show_gizmo_frames() && sec_count > 0) {
        const godot::TypedArray<godot::Transform3D> frames = spline->compute_bishop_transforms(
            std::max(2, subdivs / 2),
            up_hint,
            w_roll
        );
        constexpr float AXIS_LEN = 0.35f;
        for (int i = 0; i < frames.size(); ++i) {
            const godot::Transform3D xf = frames[i];
            r_out.add_segment(xf.origin, xf.origin + xf.basis.get_column(0) * AXIS_LEN, GIZMO_LINE_AXIS_TANGENT);
            r_out.add_segment(xf.origin, xf.origin + xf.basis.get_column(1) * AXIS_LEN, GIZMO_PLUMB_BALANCED);
            r_out.add_segment(xf.origin, xf.origin + xf.basis.get_column(2) * AXIS_LEN, GIZMO_LINE_AXIS_NORMAL);
        }
    }

    // 4. 3D AABB Cage & Scale/Rotate Handles
    if (pt_count >= 2) {
        const godot::AABB box = spline->get_aabb_3d(false);
        if (box.size.length_squared() > 1e-5f) {
            r_out.add_wire_box(box.get_center(), box.size * 0.5f, godot::Basis(), GIZMO_LINE_CLEARANCE);

            r_out.add_handle(
                HANDLE_AABB_SCALE_ID,
                box.position + box.size,
                GIZMO_HANDLE_SCAFFOLD,
                HANDLE_SHAPE_CUBE,
                static_cast<real_t>(0.05),
                GIZMO_STATE_NONE,
                "Scale Spline4D XYZ AABB",
                true
            );

            const godot::Vector3 center = box.get_center();
            r_out.add_handle(
                HANDLE_AABB_ROTATE_Y_ID,
                godot::Vector3(center.x + std::max(0.5f, box.size.x * 0.5f + 0.35f), center.y, center.z),
                GIZMO_LINE_RULER,
                HANDLE_SHAPE_RING,
                static_cast<real_t>(0.05),
                GIZMO_STATE_NONE,
                "Rotate Spline4D Y-Axis",
                true
            );
        }
    }
}

godot::String Spline4DGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_target;
    (void)p_secondary;
    if (p_handle_id == HANDLE_AABB_SCALE_ID) return "Scale Spline4D XYZ AABB";
    if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) return "Rotate Spline4D Y-Axis";
    if (p_handle_id >= HANDLE_W_RING_OFFSET) {
        return godot::String("Sculpt 4D W-Attribute #") + godot::String::num_int64(p_handle_id - HANDLE_W_RING_OFFSET);
    }
    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        return godot::String("Split 4D Section #") + godot::String::num_int64(p_handle_id - HANDLE_SECTION_SPLIT_OFFSET);
    }
    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        return godot::String("4D Fore Tangent #") + godot::String::num_int64(p_handle_id - HANDLE_FORE_OFFSET);
    }
    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        return godot::String("4D Back Tangent #") + godot::String::num_int64(p_handle_id - HANDLE_BACK_OFFSET);
    }
    return godot::String("Spline4D Anchor #") + godot::String::num_int64(p_handle_id);
}

godot::Variant Spline4DGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto spline = _resolve_spline_4d(p_target);
    if (spline.is_null()) return godot::Variant();

    if (p_handle_id == HANDLE_AABB_SCALE_ID ||
        p_handle_id == HANDLE_AABB_ROTATE_Y_ID ||
        (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET && p_handle_id < HANDLE_W_RING_OFFSET)) {
        return spline->get_snapshot_state();
    }
    if (p_handle_id >= HANDLE_W_RING_OFFSET) {
        return spline->get_point_w(p_handle_id - HANDLE_W_RING_OFFSET);
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

void Spline4DGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    auto spline = _resolve_spline_4d(p_target);
    if (!p_target || spline.is_null()) return;

    DragState st{};
    st.initial_snapshot = spline->get_snapshot_state();
    st.initial_aabb     = spline->get_aabb_3d(false);
    st.initial_value    = get_handle_value(p_target, p_handle_id, false);
    st.active           = true;

    if (p_handle_id == HANDLE_AABB_SCALE_ID) {
        st.initial_handle_pos = st.initial_aabb.position + st.initial_aabb.size;
    } else if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        const godot::Vector3 c = st.initial_aabb.get_center();
        st.initial_handle_pos = godot::Vector3(c.x + std::max(0.5f, st.initial_aabb.size.x * 0.5f + 0.35f), c.y, c.z);
    }
    active_drags_[p_target->get_instance_id()] = st;
}

void Spline4DGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_modifiers;
    auto spline = _resolve_spline_4d(p_target);
    if (!p_target || spline.is_null() || !p_ray.is_valid()) return;

    SplineNode4D *n4d  = godot::Object::cast_to<SplineNode4D>(p_target);
    const uint64_t nid = p_target->get_instance_id();
    const bool w_roll  = _is_w_banking_roll(p_target);
    const godot::Vector3 up_hint = n4d ? n4d->get_up_vector() : godot::Vector3(0.0f, 1.0f, 0.0f);

    if (!active_drags_.has(nid)) {
        on_handle_drag_begin(p_target, p_handle_id, p_ray);
    }

    if (p_handle_id == HANDLE_AABB_SCALE_ID) {
        const DragState &st = active_drags_[nid];
        spline->apply_snapshot_state(st.initial_snapshot);
        const godot::Vector3 center = st.initial_aabb.get_center();
        const godot::Vector3 hit    = _project_ray_to_local_plane(p_target, p_ray, st.initial_handle_pos);
        const godot::Vector3 half0  = (st.initial_handle_pos - center);
        const godot::Vector3 half1  = (hit - center);

        const godot::Vector3 scale_vec(
            (std::abs(half0.x) > 1e-3f) ? std::clamp(half1.x / half0.x, 0.05f, 20.0f) : 1.0f,
            (std::abs(half0.y) > 1e-3f) ? std::clamp(half1.y / half0.y, 0.05f, 20.0f) : 1.0f,
            (std::abs(half0.z) > 1e-3f) ? std::clamp(half1.z / half0.z, 0.05f, 20.0f) : 1.0f
        );
        spline->scale_spline(scale_vec);
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        const DragState &st = active_drags_[nid];
        spline->apply_snapshot_state(st.initial_snapshot);
        const godot::Vector3 center = st.initial_aabb.get_center();
        const godot::Vector3 hit    = _project_ray_to_local_plane(p_target, p_ray, st.initial_handle_pos);
        const godot::Vector3 rel    = hit - center;
        spline->rotate_spline(godot::Vector3(0.0f, std::atan2(-rel.z, rel.x), 0.0f));
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_W_RING_OFFSET) {
        const int idx = p_handle_id - HANDLE_W_RING_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        if (n4d) n4d->set_selected_gizmo_point_index(idx);

        const godot::Vector3 anchor      = spline->get_point_xyz(idx);
        const godot::Basis frame_basis   = _compute_anchor_frame_basis(spline, idx, up_hint);
        const godot::Vector3 right       = frame_basis.get_column(0).normalized();
        const godot::Vector3 up          = frame_basis.get_column(1).normalized();
        const godot::Vector3 tangent_nrm = frame_basis.get_column(2).normalized();

        const godot::Vector3 hit = _project_ray_to_local_plane(p_target, p_ray, anchor, tangent_nrm);
        const godot::Vector3 rel = hit - anchor;

        if (w_roll) {
            spline->set_point_w(idx, static_cast<real_t>(std::atan2(rel.dot(up), rel.dot(right))));
        } else {
            spline->set_point_w(idx, static_cast<real_t>(std::clamp(rel.length(), 0.02f, 25.0f)));
        }
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        if (n4d) n4d->set_selected_gizmo_section_index(p_handle_id - HANDLE_SECTION_SPLIT_OFFSET);
        return;
    }

    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        if (n4d) n4d->set_selected_gizmo_point_index(idx);
        const godot::Vector3 anchor = spline->get_point_xyz(idx);
        godot::Vector4 fh4          = spline->get_point_fore_handle(idx);
        const godot::Vector3 cur_w  = anchor + godot::Vector3(fh4.x, fh4.y, fh4.z);
        const godot::Vector3 d3     = _project_ray_to_local_plane(p_target, p_ray, cur_w) - anchor;
        fh4.x = d3.x; fh4.y = d3.y; fh4.z = d3.z;
        spline->set_point_fore_handle(idx, fh4, true);
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        const int idx = p_handle_id - HANDLE_BACK_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        if (n4d) n4d->set_selected_gizmo_point_index(idx);
        const godot::Vector3 anchor = spline->get_point_xyz(idx);
        godot::Vector4 bh4          = spline->get_point_back_handle(idx);
        const godot::Vector3 cur_w  = anchor + godot::Vector3(bh4.x, bh4.y, bh4.z);
        const godot::Vector3 d3     = _project_ray_to_local_plane(p_target, p_ray, cur_w) - anchor;
        bh4.x = d3.x; bh4.y = d3.y; bh4.z = d3.z;
        spline->set_point_back_handle(idx, bh4, true);
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count()) {
        if (n4d) n4d->set_selected_gizmo_point_index(p_handle_id);
        const godot::Vector3 cur_xyz = spline->get_point_xyz(p_handle_id);
        spline->set_point_xyz(p_handle_id, _project_ray_to_local_plane(p_target, p_ray, cur_xyz));
        p_target->update_gizmos();
    }
}

void Spline4DGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    auto spline = _resolve_spline_4d(p_target);
    if (!p_target || spline.is_null()) return;

    SplineNode4D *n4d  = godot::Object::cast_to<SplineNode4D>(p_target);
    const uint64_t nid = p_target->get_instance_id();

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
                (p_handle_id == HANDLE_AABB_SCALE_ID) ? "Scale Spline4D via Gizmo" : "Rotate Spline4D via Gizmo",
                st.initial_snapshot,
                spline->get_snapshot_state()
            );
        }
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_W_RING_OFFSET) {
        const int idx = p_handle_id - HANDLE_W_RING_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count() && had_state) {
            const real_t prev_w = static_cast<real_t>(st.initial_value);
            const real_t cur_w  = spline->get_point_w(idx);
            spline->set_point_w(idx, prev_w);
            if (!p_cancel) spline->commit_set_point_w(idx, cur_w);
        }
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        const int sec_idx = p_handle_id - HANDLE_SECTION_SPLIT_OFFSET;
        if (!p_cancel && sec_idx >= 0 && sec_idx < spline->get_section_count()) {
            const godot::Vector4 mid_pt    = spline->sample_section(sec_idx, static_cast<real_t>(0.5));
            const SplineCurveType ctype    = spline->get_section_curve_type(sec_idx);
            const real_t half_weight       = std::max(static_cast<real_t>(0.01), spline->get_section_weight(sec_idx) * static_cast<real_t>(0.5));
            const godot::Dictionary before = spline->get_snapshot_state();

            spline->set_section_weight(sec_idx, half_weight);
            if (sec_idx == spline->get_point_count() - 1 && spline->get_wrap()) {
                spline->add_point(mid_pt, ctype, half_weight);
                if (n4d) n4d->set_selected_gizmo_point_index(spline->get_point_count() - 1);
            } else {
                spline->insert_point(sec_idx + 1, mid_pt, ctype, half_weight);
                if (n4d) n4d->set_selected_gizmo_point_index(sec_idx + 1);
            }
            spline->commit_snapshot_action("Split Spline4D Section via Gizmo", before, spline->get_snapshot_state());
        }
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count() && had_state) {
            const godot::Vector4 prev_h = st.initial_value;
            const godot::Vector4 cur_h  = spline->get_point_fore_handle(idx);
            spline->set_point_fore_handle(idx, prev_h, true);
            if (!p_cancel) spline->commit_set_point_fore_handle(idx, cur_h, true);
        }
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        const int idx = p_handle_id - HANDLE_BACK_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count() && had_state) {
            const godot::Vector4 prev_h = st.initial_value;
            const godot::Vector4 cur_h  = spline->get_point_back_handle(idx);
            spline->set_point_back_handle(idx, prev_h, true);
            if (!p_cancel) spline->commit_set_point_back_handle(idx, cur_h, true);
        }
        p_target->update_gizmos();
        return;
    }

    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count() && had_state) {
        const godot::Vector4 prev_pos = st.initial_value;
        const godot::Vector4 cur_pos  = spline->get_point_position(p_handle_id);
        spline->set_point_position(p_handle_id, prev_pos);
        if (!p_cancel) spline->commit_set_point_position(p_handle_id, cur_pos);
    }

    p_target->update_gizmos();
}

} // namespace ideam::godot_ext