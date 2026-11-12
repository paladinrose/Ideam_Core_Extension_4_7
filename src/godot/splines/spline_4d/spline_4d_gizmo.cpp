#include "spline_4d_gizmo.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::godot_ext {

void Spline4DGizmo::_bind_methods() {
}

Spline4DGizmo::Spline4DGizmo() {
    _init_gizmo_materials();
}

Spline4DGizmo::~Spline4DGizmo() = default;

void Spline4DGizmo::_init_gizmo_materials() {
    create_material("curve_linear", godot::Color(0.32f, 0.75f, 0.98f, 0.95f), false, true, false);
    create_material("curve_catmull_rom", godot::Color(0.35f, 0.92f, 0.55f, 0.95f), false, true, false);
    create_material("curve_cubic_bezier", godot::Color(0.98f, 0.72f, 0.25f, 0.95f), false, true, false);
    create_material("curve_step", godot::Color(0.85f, 0.45f, 0.98f, 0.95f), false, true, false);
    create_material("curve_selected", godot::Color(1.0f, 0.94f, 0.35f, 1.0f), false, true, false);

    // 4D W-Ring and stalk materials
    create_material("w_ring", godot::Color(0.90f, 0.52f, 0.98f, 0.92f), false, true, false);
    create_material("w_ring_selected", godot::Color(1.0f, 0.88f, 0.32f, 1.0f), false, true, false);
    create_material("handle_stalk", godot::Color(0.98f, 0.52f, 0.32f, 0.85f), false, true, false);
    create_material("aabb_box", godot::Color(0.55f, 0.78f, 0.88f, 0.42f), false, true, false);
    create_material("frame_x", godot::Color(0.96f, 0.32f, 0.32f, 0.85f), false, true, false);
    create_material("frame_y", godot::Color(0.32f, 0.94f, 0.42f, 0.85f), false, true, false);
    create_material("frame_z", godot::Color(0.34f, 0.62f, 0.98f, 0.85f), false, true, false);

    create_handle_material("handles");
    create_handle_material("secondary_handles");
}

godot::Ref<Spline4DResource> Spline4DGizmo::_resolve_spline_4d(godot::Node3D *p_node) {
    if (!p_node) {
        return godot::Ref<Spline4DResource>();
    }
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

bool Spline4DGizmo::_has_gizmo(godot::Node3D *p_for_node_3d) const {
    if (!p_for_node_3d) {
        return false;
    }
    if (godot::Object::cast_to<SplineNode4D>(p_for_node_3d) != nullptr) {
        return true;
    }
    return _resolve_spline_4d(p_for_node_3d).is_valid();
}

godot::String Spline4DGizmo::_get_gizmo_name() const {
    return "SplineNode4D";
}

int32_t Spline4DGizmo::_get_priority() const {
    return -1;
}

godot::Ref<godot::StandardMaterial3D> Spline4DGizmo::_get_curve_material(
    SplineCurveType p_type,
    bool p_selected,
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo
) {
    if (p_selected) {
        return get_material("curve_selected", p_gizmo);
    }
    switch (p_type) {
        case CURVE_LINEAR:
            return get_material("curve_linear", p_gizmo);
        case CURVE_CUBIC_BEZIER:
            return get_material("curve_cubic_bezier", p_gizmo);
        case CURVE_STEP:
            return get_material("curve_step", p_gizmo);
        case CURVE_CATMULL_ROM:
        default:
            return get_material("curve_catmull_rom", p_gizmo);
    }
}

godot::Basis Spline4DGizmo::_compute_anchor_frame_basis(
    const godot::Ref<Spline4DResource> &p_spline,
    int p_point_index,
    const godot::Vector3 &p_up_hint
) {
    if (p_spline.is_null() || p_spline->get_point_count() <= 0) {
        return godot::Basis();
    }
    const int sec_count = p_spline->get_section_count();
    real_t pct = static_cast<real_t>(0.0);
    if (sec_count > 0) {
        if (p_point_index < sec_count) {
            pct = p_spline->section_local_to_global_percent(p_point_index, static_cast<real_t>(0.0), true);
        } else {
            pct = static_cast<real_t>(1.0);
        }
    }
    const godot::Transform3D xf = p_spline->get_transform_3d_at_percent(pct, true, true, p_up_hint);
    return xf.basis.orthonormalized();
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

godot::Vector3 Spline4DGizmo::_project_screen_to_local_plane(
    const godot::Node3D *p_node,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos,
    const godot::Vector3 &p_plane_origin_local,
    const godot::Vector3 &p_custom_normal_local
) {
    if (!p_node || !p_camera) {
        return p_plane_origin_local;
    }

    const godot::Transform3D node_xform = p_node->get_global_transform();
    const godot::Transform3D inv_xform  = node_xform.affine_inverse();

    const godot::Vector3 ray_origin_world = p_camera->project_ray_origin(p_screen_pos);
    const godot::Vector3 ray_dir_world    = p_camera->project_ray_normal(p_screen_pos);
    const godot::Vector3 plane_origin_world = node_xform.xform(p_plane_origin_local);

    godot::Vector3 plane_normal_world = -p_camera->get_global_transform().basis.get_column(2).normalized();
    if (p_custom_normal_local.length_squared() > 1e-4f) {
        const godot::Vector3 candidate = node_xform.basis.xform(p_custom_normal_local).normalized();
        if ( std::abs(candidate.dot(ray_dir_world)) > 0.12f ) {
            plane_normal_world = candidate;
        }
    }

    const godot::Plane plane(plane_normal_world, plane_origin_world);
    godot::Vector3 hit_world = plane_origin_world;
    if (plane.intersects_ray(ray_origin_world, ray_dir_world, &hit_world)) {
        return inv_xform.xform(hit_world);
    }
    return p_plane_origin_local;
}

void Spline4DGizmo::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) {
    if (p_gizmo.is_null()) {
        return;
    }
    p_gizmo->clear();

    godot::Node3D *node = p_gizmo->get_node_3d();
    godot::Ref<Spline4DResource> spline = _resolve_spline_4d(node);
    if (spline.is_null() || spline->get_point_count() <= 0) {
        return;
    }

    SplineNode4D *n4d   = godot::Object::cast_to<SplineNode4D>(node);
    const int pt_count  = spline->get_point_count();
    const int sec_count = spline->get_section_count();
    const int subdivs   = n4d ? std::clamp(n4d->get_gizmo_subdivisions(), 2, 64) : 12;
    const int sel_pt    = n4d ? n4d->get_selected_gizmo_point_index() : -1;
    const int sel_sec   = n4d ? n4d->get_selected_gizmo_section_index() : -1;
    const bool w_roll   = _is_w_banking_roll(node);
    const godot::Vector3 up_hint = n4d ? n4d->get_up_vector() : godot::Vector3(0.0f, 1.0f, 0.0f);

    // 1. Draw 3D spatial curve sections
    for (int s = 0; s < sec_count; ++s) {
        const SplineCurveType ctype = spline->get_section_curve_type(s);
        if (ctype == CURVE_NONE) {
            continue;
        }

        godot::PackedVector3Array sec_lines;
        sec_lines.resize(subdivs * 2);
        const godot::Vector4 p0_4d = spline->sample_section(s, static_cast<real_t>(0.0));
        godot::Vector3 prev_pt(p0_4d.x, p0_4d.y, p0_4d.z);

        for (int step = 1; step <= subdivs; ++step) {
            const real_t local_t = static_cast<real_t>(step) / static_cast<real_t>(subdivs);
            const godot::Vector4 p4 = spline->sample_section(s, local_t);
            const godot::Vector3 next_pt(p4.x, p4.y, p4.z);
            sec_lines.set((step - 1) * 2, prev_pt);
            sec_lines.set((step - 1) * 2 + 1, next_pt);
            prev_pt = next_pt;
        }

        p_gizmo->add_lines(
            sec_lines,
            _get_curve_material(ctype, s == sel_sec, p_gizmo),
            false
        );
    }

    // 2. Collect primary 3D anchor handles, radial W-ring handles, and 3D tangent stalks
    godot::PackedVector3Array primary_handles;
    godot::PackedInt32Array primary_ids;

    godot::PackedVector3Array secondary_handles;
    godot::PackedInt32Array secondary_ids;

    godot::PackedVector3Array stalk_lines;
    godot::PackedVector3Array w_ring_lines;
    godot::PackedVector3Array w_ring_sel_lines;

    constexpr int RING_SEGMENTS = 24;
    const float two_pi = static_cast<float>(2.0 * std::numbers::pi_v<double>);

    for (int i = 0; i < pt_count; ++i) {
        const godot::Vector3 anchor = spline->get_point_xyz(i);
        const float w_val           = static_cast<float>(spline->get_point_w(i));

        primary_handles.push_back(anchor);
        primary_ids.push_back(HANDLE_ANCHOR_BASE + i);

        // Radial W-Ring around each 3D anchor
        const godot::Basis frame_basis = _compute_anchor_frame_basis(spline, i, up_hint);
        const godot::Vector3 right     = frame_basis.get_column(0).normalized();
        const godot::Vector3 up        = frame_basis.get_column(1).normalized();
        const float ring_radius        = w_roll ? 0.65f : std::clamp(std::abs(w_val), 0.08f, 25.0f);

        godot::PackedVector3Array &target_ring = (i == sel_pt) ? w_ring_sel_lines : w_ring_lines;
        godot::Vector3 prev_ring_pt = anchor + right * ring_radius;
        for (int r = 1; r <= RING_SEGMENTS; ++r) {
            const float angle = (static_cast<float>(r) / static_cast<float>(RING_SEGMENTS)) * two_pi;
            const godot::Vector3 next_ring_pt = anchor + (right * std::cos(angle) + up * std::sin(angle)) * ring_radius;
            target_ring.push_back(prev_ring_pt);
            target_ring.push_back(next_ring_pt);
            prev_ring_pt = next_ring_pt;
        }

        const godot::Vector3 w_handle_pos = _compute_w_ring_handle_pos(spline, i, w_roll, up_hint);
        target_ring.push_back(anchor);
        target_ring.push_back(w_handle_pos);

        secondary_handles.push_back(w_handle_pos);
        secondary_ids.push_back(HANDLE_W_RING_OFFSET + i);

        // 3D Back/Fore tangent handles
        if (spline->get_point_use_back_handle(i)) {
            const godot::Vector4 bh4 = spline->get_point_back_handle(i);
            const godot::Vector3 bh_world = anchor + godot::Vector3(bh4.x, bh4.y, bh4.z);
            stalk_lines.push_back(anchor);
            stalk_lines.push_back(bh_world);
            secondary_handles.push_back(bh_world);
            secondary_ids.push_back(HANDLE_BACK_OFFSET + i);
        }
        if (spline->get_point_use_fore_handle(i)) {
            const godot::Vector4 fh4 = spline->get_point_fore_handle(i);
            const godot::Vector3 fh_world = anchor + godot::Vector3(fh4.x, fh4.y, fh4.z);
            stalk_lines.push_back(anchor);
            stalk_lines.push_back(fh_world);
            secondary_handles.push_back(fh_world);
            secondary_ids.push_back(HANDLE_FORE_OFFSET + i);
        }
    }

    if (!w_ring_lines.is_empty()) {
        p_gizmo->add_lines(w_ring_lines, get_material("w_ring", p_gizmo), false);
    }
    if (!w_ring_sel_lines.is_empty()) {
        p_gizmo->add_lines(w_ring_sel_lines, get_material("w_ring_selected", p_gizmo), false);
    }
    if (!stalk_lines.is_empty()) {
        p_gizmo->add_lines(stalk_lines, get_material("handle_stalk", p_gizmo), false);
    }

    // Section midpoint split handles
    for (int s = 0; s < sec_count; ++s) {
        if (spline->get_section_curve_type(s) == CURVE_NONE) {
            continue;
        }
        const godot::Vector4 mid4 = spline->sample_section(s, static_cast<real_t>(0.5));
        secondary_handles.push_back(godot::Vector3(mid4.x, mid4.y, mid4.z));
        secondary_ids.push_back(HANDLE_SECTION_SPLIT_OFFSET + s);
    }

    // 3. Optional Bishop Frame Triads (scaled/rolled by W!)
    if (n4d && n4d->get_show_gizmo_frames() && sec_count > 0) {
        const godot::TypedArray<godot::Transform3D> frames = spline->compute_bishop_transforms(
            std::max(2, subdivs / 2),
            up_hint,
            w_roll
        );
        godot::PackedVector3Array x_lines;
        godot::PackedVector3Array y_lines;
        godot::PackedVector3Array z_lines;
        constexpr float AXIS_LEN = 0.35f;

        for (int i = 0; i < frames.size(); ++i) {
            const godot::Transform3D xf = frames[i];
            x_lines.push_back(xf.origin);
            x_lines.push_back(xf.origin + xf.basis.get_column(0) * AXIS_LEN);
            y_lines.push_back(xf.origin);
            y_lines.push_back(xf.origin + xf.basis.get_column(1) * AXIS_LEN);
            z_lines.push_back(xf.origin);
            z_lines.push_back(xf.origin + xf.basis.get_column(2) * AXIS_LEN);
        }

        if (!x_lines.is_empty()) p_gizmo->add_lines(x_lines, get_material("frame_x", p_gizmo), false);
        if (!y_lines.is_empty()) p_gizmo->add_lines(y_lines, get_material("frame_y", p_gizmo), false);
        if (!z_lines.is_empty()) p_gizmo->add_lines(z_lines, get_material("frame_z", p_gizmo), false);
    }

    // 4. 3D AABB Cage & Scale/Rotate Handles
    if (pt_count >= 2) {
        const godot::AABB box = spline->get_aabb_3d(false);
        if (box.size.length_squared() > 1e-5f) {
            const godot::Vector3 p0 = box.position;
            const godot::Vector3 p1 = box.position + box.size;
            const godot::Vector3 c[8] = {
                godot::Vector3(p0.x, p0.y, p0.z),
                godot::Vector3(p1.x, p0.y, p0.z),
                godot::Vector3(p1.x, p1.y, p0.z),
                godot::Vector3(p0.x, p1.y, p0.z),
                godot::Vector3(p0.x, p0.y, p1.z),
                godot::Vector3(p1.x, p0.y, p1.z),
                godot::Vector3(p1.x, p1.y, p1.z),
                godot::Vector3(p0.x, p1.y, p1.z)
            };
            static constexpr int EDGES[12][2] = {
                {0,1}, {1,2}, {2,3}, {3,0},
                {4,5}, {5,6}, {6,7}, {7,4},
                {0,4}, {1,5}, {2,6}, {3,7}
            };
            godot::PackedVector3Array box_lines;
            box_lines.resize(24);
            for (int e = 0; e < 12; ++e) {
                box_lines.set(e * 2, c[EDGES[e][0]]);
                box_lines.set(e * 2 + 1, c[EDGES[e][1]]);
            }
            p_gizmo->add_lines(box_lines, get_material("aabb_box", p_gizmo), false);

            secondary_handles.push_back(p1);
            secondary_ids.push_back(HANDLE_AABB_SCALE_ID);

            const godot::Vector3 center = box.get_center();
            const godot::Vector3 rot_handle(
                center.x + std::max(0.5f, box.size.x * 0.5f + 0.35f),
                center.y,
                center.z
            );
            secondary_handles.push_back(rot_handle);
            secondary_ids.push_back(HANDLE_AABB_ROTATE_Y_ID);
        }
    }

    if (!primary_handles.is_empty()) {
        p_gizmo->add_handles(
            primary_handles,
            get_material("handles", p_gizmo),
            primary_ids,
            false,
            false
        );
    }
    if (!secondary_handles.is_empty()) {
        p_gizmo->add_handles(
            secondary_handles,
            get_material("secondary_handles", p_gizmo),
            secondary_ids,
            false,
            true
        );
    }
}

godot::String Spline4DGizmo::_get_handle_name(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_gizmo;
    (void)p_secondary;
    if (p_handle_id == HANDLE_AABB_SCALE_ID) {
        return "Scale Spline4D XYZ AABB";
    }
    if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        return "Rotate Spline4D Y-Axis";
    }
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

godot::Variant Spline4DGizmo::_get_handle_value(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    if (p_gizmo.is_null()) {
        return godot::Variant();
    }
    godot::Node3D *node = p_gizmo->get_node_3d();
    godot::Ref<Spline4DResource> spline = _resolve_spline_4d(node);
    if (!node || spline.is_null()) {
        return godot::Variant();
    }

    const uint64_t nid = node->get_instance_id();
    if (!active_drags.has(nid) || !active_drags[nid].active) {
        DragState st;
        st.initial_snapshot = spline->get_snapshot_state();
        st.initial_aabb     = spline->get_aabb_3d(false);
        st.active           = true;

        if (p_handle_id == HANDLE_AABB_SCALE_ID) {
            st.initial_handle_pos = st.initial_aabb.position + st.initial_aabb.size;
        } else if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
            const godot::Vector3 c = st.initial_aabb.get_center();
            st.initial_handle_pos = godot::Vector3(c.x + std::max(0.5f, st.initial_aabb.size.x * 0.5f + 0.35f), c.y, c.z);
        }
        active_drags[nid] = st;
    }

    if (p_handle_id == HANDLE_AABB_SCALE_ID ||
        p_handle_id == HANDLE_AABB_ROTATE_Y_ID ||
        (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET && p_handle_id < HANDLE_W_RING_OFFSET)) {
        return active_drags[nid].initial_snapshot;
    }
    if (p_handle_id >= HANDLE_W_RING_OFFSET) {
        const int idx = p_handle_id - HANDLE_W_RING_OFFSET;
        return spline->get_point_w(idx);
    }
    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        return spline->get_point_fore_handle(idx);
    }
    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        const int idx = p_handle_id - HANDLE_BACK_OFFSET;
        return spline->get_point_back_handle(idx);
    }
    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count()) {
        return spline->get_point_position(p_handle_id);
    }
    return godot::Variant();
}

void Spline4DGizmo::_set_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos
) {
    (void)p_secondary;
    if (p_gizmo.is_null() || !p_camera) {
        return;
    }
    godot::Node3D *node = p_gizmo->get_node_3d();
    godot::Ref<Spline4DResource> spline = _resolve_spline_4d(node);
    if (!node || spline.is_null()) {
        return;
    }
    SplineNode4D *n4d  = godot::Object::cast_to<SplineNode4D>(node);
    const uint64_t nid = node->get_instance_id();
    const bool w_roll  = _is_w_banking_roll(node);
    const godot::Vector3 up_hint = n4d ? n4d->get_up_vector() : godot::Vector3(0.0f, 1.0f, 0.0f);

    if (p_handle_id == HANDLE_AABB_SCALE_ID) {
        if (!active_drags.has(nid)) return;
        const DragState &st = active_drags[nid];
        spline->apply_snapshot_state(st.initial_snapshot);
        const godot::Vector3 center = st.initial_aabb.get_center();
        const godot::Vector3 hit    = _project_screen_to_local_plane(node, p_camera, p_screen_pos, st.initial_handle_pos);
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
        if (!active_drags.has(nid)) return;
        const DragState &st = active_drags[nid];
        spline->apply_snapshot_state(st.initial_snapshot);
        const godot::Vector3 center = st.initial_aabb.get_center();
        const godot::Vector3 hit    = _project_screen_to_local_plane(node, p_camera, p_screen_pos, st.initial_handle_pos);
        const godot::Vector3 rel    = hit - center;
        const float yaw_rad         = std::atan2(-rel.z, rel.x);
        spline->rotate_spline(godot::Vector3(0.0f, yaw_rad, 0.0f));
        node->update_gizmos();
        return;
    }

    // Radial W-Ring Handle
    if (p_handle_id >= HANDLE_W_RING_OFFSET) {
        const int idx = p_handle_id - HANDLE_W_RING_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        if (n4d) n4d->set_selected_gizmo_point_index(idx);

        const godot::Vector3 anchor      = spline->get_point_xyz(idx);
        const godot::Basis frame_basis   = _compute_anchor_frame_basis(spline, idx, up_hint);
        const godot::Vector3 right       = frame_basis.get_column(0).normalized();
        const godot::Vector3 up          = frame_basis.get_column(1).normalized();
        const godot::Vector3 tangent_nrm = frame_basis.get_column(2).normalized();

        const godot::Vector3 hit = _project_screen_to_local_plane(node, p_camera, p_screen_pos, anchor, tangent_nrm);
        const godot::Vector3 rel = hit - anchor;

        if (w_roll) {
            const float x_proj = rel.dot(right);
            const float y_proj = rel.dot(up);
            const real_t angle = static_cast<real_t>(std::atan2(y_proj, x_proj));
            spline->set_point_w(idx, angle);
        } else {
            const real_t radius = static_cast<real_t>(std::clamp(rel.length(), 0.02f, 25.0f));
            spline->set_point_w(idx, radius);
        }
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        const int sec_idx = p_handle_id - HANDLE_SECTION_SPLIT_OFFSET;
        if (n4d) n4d->set_selected_gizmo_section_index(sec_idx);
        return;
    }

    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        if (n4d) n4d->set_selected_gizmo_point_index(idx);
        const godot::Vector3 anchor = spline->get_point_xyz(idx);
        godot::Vector4 fh4          = spline->get_point_fore_handle(idx);
        const godot::Vector3 cur_w  = anchor + godot::Vector3(fh4.x, fh4.y, fh4.z);
        const godot::Vector3 new_w  = _project_screen_to_local_plane(node, p_camera, p_screen_pos, cur_w);
        const godot::Vector3 d3     = new_w - anchor;
        fh4.x = d3.x;
        fh4.y = d3.y;
        fh4.z = d3.z;
        spline->set_point_fore_handle(idx, fh4, true);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        const int idx = p_handle_id - HANDLE_BACK_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        if (n4d) n4d->set_selected_gizmo_point_index(idx);
        const godot::Vector3 anchor = spline->get_point_xyz(idx);
        godot::Vector4 bh4          = spline->get_point_back_handle(idx);
        const godot::Vector3 cur_w  = anchor + godot::Vector3(bh4.x, bh4.y, bh4.z);
        const godot::Vector3 new_w  = _project_screen_to_local_plane(node, p_camera, p_screen_pos, cur_w);
        const godot::Vector3 d3     = new_w - anchor;
        bh4.x = d3.x;
        bh4.y = d3.y;
        bh4.z = d3.z;
        spline->set_point_back_handle(idx, bh4, true);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count()) {
        if (n4d) n4d->set_selected_gizmo_point_index(p_handle_id);
        const godot::Vector3 cur_xyz = spline->get_point_xyz(p_handle_id);
        const godot::Vector3 new_xyz = _project_screen_to_local_plane(node, p_camera, p_screen_pos, cur_xyz);
        spline->set_point_xyz(p_handle_id, new_xyz);
        node->update_gizmos();
    }
}

void Spline4DGizmo::_commit_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    const godot::Variant &p_restore,
    bool p_cancel
) {
    (void)p_secondary;
    if (p_gizmo.is_null()) {
        return;
    }
    godot::Node3D *node = p_gizmo->get_node_3d();
    godot::Ref<Spline4DResource> spline = _resolve_spline_4d(node);
    if (!node || spline.is_null()) {
        return;
    }
    SplineNode4D *n4d  = godot::Object::cast_to<SplineNode4D>(node);
    const uint64_t nid = node->get_instance_id();

    if (spline->get_undo_redo().is_null()) {
        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
    }

    if (p_handle_id == HANDLE_AABB_SCALE_ID || p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        const godot::Dictionary before = p_restore;
        if (p_cancel) {
            spline->apply_snapshot_state(before);
        } else {
            spline->commit_snapshot_action(
                (p_handle_id == HANDLE_AABB_SCALE_ID) ? "Scale Spline4D via Gizmo" : "Rotate Spline4D via Gizmo",
                before,
                spline->get_snapshot_state()
            );
        }
        active_drags.erase(nid);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_W_RING_OFFSET) {
        const int idx = p_handle_id - HANDLE_W_RING_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count()) {
            const real_t prev_w = static_cast<real_t>(p_restore);
            const real_t cur_w  = spline->get_point_w(idx);
            spline->set_point_w(idx, prev_w);
            if (!p_cancel) {
                spline->commit_set_point_w(idx, cur_w);
            }
        }
        active_drags.erase(nid);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        const int sec_idx = p_handle_id - HANDLE_SECTION_SPLIT_OFFSET;
        if (!p_cancel && sec_idx >= 0 && sec_idx < spline->get_section_count()) {
            const godot::Vector4 mid_pt   = spline->sample_section(sec_idx, static_cast<real_t>(0.5));
            const SplineCurveType ctype   = spline->get_section_curve_type(sec_idx);
            const real_t half_weight      = std::max(static_cast<real_t>(0.01), spline->get_section_weight(sec_idx) * static_cast<real_t>(0.5));
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
        active_drags.erase(nid);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count()) {
            const godot::Vector4 prev_h = p_restore;
            const godot::Vector4 cur_h  = spline->get_point_fore_handle(idx);
            spline->set_point_fore_handle(idx, prev_h, true);
            if (!p_cancel) {
                spline->commit_set_point_fore_handle(idx, cur_h, true);
            }
        }
        active_drags.erase(nid);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        const int idx = p_handle_id - HANDLE_BACK_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count()) {
            const godot::Vector4 prev_h = p_restore;
            const godot::Vector4 cur_h  = spline->get_point_back_handle(idx);
            spline->set_point_back_handle(idx, prev_h, true);
            if (!p_cancel) {
                spline->commit_set_point_back_handle(idx, cur_h, true);
            }
        }
        active_drags.erase(nid);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count()) {
        const godot::Vector4 prev_pos = p_restore;
        const godot::Vector4 cur_pos  = spline->get_point_position(p_handle_id);
        spline->set_point_position(p_handle_id, prev_pos);
        if (!p_cancel) {
            spline->commit_set_point_position(p_handle_id, cur_pos);
        }
    }

    active_drags.erase(nid);
    node->update_gizmos();
}

} // namespace ideam::godot_ext