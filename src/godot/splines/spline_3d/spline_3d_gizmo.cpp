#include "spline_3d_gizmo.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void Spline3DGizmo::_bind_methods() {
}

Spline3DGizmo::Spline3DGizmo() {
    _init_gizmo_materials();
}

Spline3DGizmo::~Spline3DGizmo() = default;

void Spline3DGizmo::_init_gizmo_materials() {
    // Curve-type color coding matching 2D SplineCanvasBase palette
    create_material("curve_linear", godot::Color(0.32f, 0.75f, 0.98f, 0.95f), false, true, false);
    create_material("curve_catmull_rom", godot::Color(0.35f, 0.92f, 0.55f, 0.95f), false, true, false);
    create_material("curve_cubic_bezier", godot::Color(0.98f, 0.72f, 0.25f, 0.95f), false, true, false);
    create_material("curve_step", godot::Color(0.85f, 0.45f, 0.98f, 0.95f), false, true, false);
    create_material("curve_selected", godot::Color(1.0f, 0.94f, 0.35f, 1.0f), false, true, false);

    // Tangent stalk, Bishop frame triad, and AABB cage materials
    create_material("handle_stalk", godot::Color(0.98f, 0.52f, 0.32f, 0.85f), false, true, false);
    create_material("aabb_box", godot::Color(0.55f, 0.78f, 0.88f, 0.42f), false, true, false);
    create_material("frame_x", godot::Color(0.96f, 0.32f, 0.32f, 0.85f), false, true, false);
    create_material("frame_y", godot::Color(0.32f, 0.94f, 0.42f, 0.85f), false, true, false);
    create_material("frame_z", godot::Color(0.34f, 0.62f, 0.98f, 0.85f), false, true, false);
    create_material("playhead", godot::Color(1.0f, 0.85f, 0.20f, 0.98f), false, true, false);

    create_handle_material("handles");
    create_handle_material("secondary_handles");
}

bool Spline3DGizmo::_has_gizmo(godot::Node3D *p_for_node_3d) const {
    return p_for_node_3d != nullptr && godot::Object::cast_to<SplineNode3D>(p_for_node_3d) != nullptr;
}

godot::String Spline3DGizmo::_get_gizmo_name() const {
    return "SplineNode3D";
}

int32_t Spline3DGizmo::_get_priority() const {
    return -1;
}

godot::Ref<godot::StandardMaterial3D> Spline3DGizmo::_get_curve_material(
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

godot::Vector3 Spline3DGizmo::_project_screen_to_local_plane(
    const SplineNode3D *p_node,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos,
    const godot::Vector3 &p_plane_origin_local
) {
    if (!p_node || !p_camera) {
        return p_plane_origin_local;
    }

    const godot::Transform3D node_xform = p_node->get_global_transform();
    const godot::Transform3D inv_xform  = node_xform.affine_inverse();

    const godot::Vector3 ray_origin_world = p_camera->project_ray_origin(p_screen_pos);
    const godot::Vector3 ray_dir_world    = p_camera->project_ray_normal(p_screen_pos);

    const godot::Vector3 plane_origin_world = node_xform.xform(p_plane_origin_local);
    const godot::Vector3 cam_forward_world  = -p_camera->get_global_transform().basis.get_column(2).normalized();

    const godot::Plane view_plane(cam_forward_world, plane_origin_world);
    godot::Vector3 hit_world = plane_origin_world;
    if (view_plane.intersects_ray(ray_origin_world, ray_dir_world, &hit_world)) {
        return inv_xform.xform(hit_world);
    }
    return p_plane_origin_local;
}

void Spline3DGizmo::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) {
    if (p_gizmo.is_null()) {
        return;
    }
    p_gizmo->clear();

    SplineNode3D *node = godot::Object::cast_to<SplineNode3D>(p_gizmo->get_node_3d());
    if (!node) {
        return;
    }

    godot::Ref<Spline3DResource> spline = node->get_spline();
    if (spline.is_null() || spline->get_point_count() <= 0) {
        return;
    }

    const int pt_count  = spline->get_point_count();
    const int sec_count = spline->get_section_count();
    const int subdivs   = std::clamp(node->get_gizmo_subdivisions(), 2, 64);
    const int sel_sec   = node->get_selected_gizmo_section_index();

    // 1. Draw each section with curve-type color coding
    for (int s = 0; s < sec_count; ++s) {
        const SplineCurveType ctype = spline->get_section_curve_type(s);
        if (ctype == CURVE_NONE) {
            continue;
        }

        godot::PackedVector3Array sec_lines;
        sec_lines.resize(subdivs * 2);
        godot::Vector3 prev_pt = spline->sample_section(s, static_cast<real_t>(0.0));
        for (int step = 1; step <= subdivs; ++step) {
            const real_t local_t = static_cast<real_t>(step) / static_cast<real_t>(subdivs);
            const godot::Vector3 next_pt = spline->sample_section(s, local_t);
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

    // 2. Collect primary anchor handles, tangent stalks, and section midpoint handles
    godot::PackedVector3Array primary_handles;
    godot::PackedInt32Array primary_ids;

    godot::PackedVector3Array secondary_handles;
    godot::PackedInt32Array secondary_ids;

    godot::PackedVector3Array stalk_lines;

    for (int i = 0; i < pt_count; ++i) {
        const godot::Vector3 anchor = spline->get_point_position(i);
        primary_handles.push_back(anchor);
        primary_ids.push_back(HANDLE_ANCHOR_BASE + i);

        if (spline->get_point_use_back_handle(i)) {
            const godot::Vector3 bh_world = anchor + spline->get_point_back_handle(i);
            stalk_lines.push_back(anchor);
            stalk_lines.push_back(bh_world);
            secondary_handles.push_back(bh_world);
            secondary_ids.push_back(HANDLE_BACK_OFFSET + i);
        }

        if (spline->get_point_use_fore_handle(i)) {
            const godot::Vector3 fh_world = anchor + spline->get_point_fore_handle(i);
            stalk_lines.push_back(anchor);
            stalk_lines.push_back(fh_world);
            secondary_handles.push_back(fh_world);
            secondary_ids.push_back(HANDLE_FORE_OFFSET + i);
        }
    }

    // Section midpoint split handles
    for (int s = 0; s < sec_count; ++s) {
        if (spline->get_section_curve_type(s) == CURVE_NONE) {
            continue;
        }
        const godot::Vector3 mid_pt = spline->sample_section(s, static_cast<real_t>(0.5));
        secondary_handles.push_back(mid_pt);
        secondary_ids.push_back(HANDLE_SECTION_SPLIT_OFFSET + s);
    }

    if (!stalk_lines.is_empty()) {
        p_gizmo->add_lines(stalk_lines, get_material("handle_stalk", p_gizmo), false);
    }

    // 3. Optional Bishop / Up-Vector Coordinate Frame Triads
    if (node->get_show_gizmo_frames() && sec_count > 0) {
        const godot::TypedArray<godot::Transform3D> frames = spline->compute_bishop_transforms(
            std::max(2, subdivs / 2),
            node->get_up_vector()
        );
        godot::PackedVector3Array x_lines;
        godot::PackedVector3Array y_lines;
        godot::PackedVector3Array z_lines;
        constexpr float AXIS_LEN = 0.35f;

        for (int i = 0; i < frames.size(); ++i) {
            const godot::Transform3D xf = frames[i];
            x_lines.push_back(xf.origin);
            x_lines.push_back(xf.origin + xf.basis.get_column(0).normalized() * AXIS_LEN);
            y_lines.push_back(xf.origin);
            y_lines.push_back(xf.origin + xf.basis.get_column(1).normalized() * AXIS_LEN);
            z_lines.push_back(xf.origin);
            z_lines.push_back(xf.origin + xf.basis.get_column(2).normalized() * AXIS_LEN);
        }

        if (!x_lines.is_empty()) p_gizmo->add_lines(x_lines, get_material("frame_x", p_gizmo), false);
        if (!y_lines.is_empty()) p_gizmo->add_lines(y_lines, get_material("frame_y", p_gizmo), false);
        if (!z_lines.is_empty()) p_gizmo->add_lines(z_lines, get_material("frame_z", p_gizmo), false);
    }

    // 4. Draw AABB Rotate/Scale Cage when 2+ points exist
    if (pt_count >= 2) {
        const godot::AABB box = spline->get_aabb(false);
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

            // AABB Scale Handle at max corner (p1)
            secondary_handles.push_back(p1);
            secondary_ids.push_back(HANDLE_AABB_SCALE_ID);

            // AABB Y-Rotation Handle slightly outside top-Z edge
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

    // 5. Draw Current Playback Pose Diamond
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
        p_gizmo->add_lines(play_lines, get_material("playhead", p_gizmo), false);
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

godot::String Spline3DGizmo::_get_handle_name(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_gizmo;
    (void)p_secondary;
    if (p_handle_id == HANDLE_AABB_SCALE_ID) {
        return "Scale Spline3D AABB";
    }
    if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        return "Rotate Spline3D Y-Axis";
    }
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

godot::Variant Spline3DGizmo::_get_handle_value(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    if (p_gizmo.is_null()) {
        return godot::Variant();
    }
    SplineNode3D *node = godot::Object::cast_to<SplineNode3D>(p_gizmo->get_node_3d());
    if (!node || node->get_spline().is_null()) {
        return godot::Variant();
    }
    godot::Ref<Spline3DResource> spline = node->get_spline();

    const uint64_t nid = node->get_instance_id();
    if (!active_drags.has(nid) || !active_drags[nid].active) {
        DragState st;
        st.initial_snapshot = spline->get_snapshot_state();
        st.initial_aabb     = spline->get_aabb(false);
        st.active           = true;

        if (p_handle_id == HANDLE_AABB_SCALE_ID) {
            st.initial_handle_pos = st.initial_aabb.position + st.initial_aabb.size;
        } else if (p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
            const godot::Vector3 c = st.initial_aabb.get_center();
            st.initial_handle_pos = godot::Vector3(c.x + std::max(0.5f, st.initial_aabb.size.x * 0.5f + 0.35f), c.y, c.z);
        } else if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
            const int sec_idx = p_handle_id - HANDLE_SECTION_SPLIT_OFFSET;
            st.initial_handle_pos = spline->sample_section(sec_idx, static_cast<real_t>(0.5));
        }
        active_drags[nid] = st;
    }

    if (p_handle_id == HANDLE_AABB_SCALE_ID ||
        p_handle_id == HANDLE_AABB_ROTATE_Y_ID ||
        p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        return active_drags[nid].initial_snapshot;
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

void Spline3DGizmo::_set_handle(
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
    SplineNode3D *node = godot::Object::cast_to<SplineNode3D>(p_gizmo->get_node_3d());
    if (!node || node->get_spline().is_null()) {
        return;
    }
    godot::Ref<Spline3DResource> spline = node->get_spline();
    const uint64_t nid = node->get_instance_id();

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

    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        const int sec_idx = p_handle_id - HANDLE_SECTION_SPLIT_OFFSET;
        node->set_selected_gizmo_section_index(sec_idx);
        return;
    }

    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        node->set_selected_gizmo_point_index(idx);
        const godot::Vector3 anchor = spline->get_point_position(idx);
        const godot::Vector3 cur_world = anchor + spline->get_point_fore_handle(idx);
        const godot::Vector3 new_world = _project_screen_to_local_plane(node, p_camera, p_screen_pos, cur_world);
        spline->set_point_fore_handle(idx, new_world - anchor, true);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_BACK_OFFSET) {
        const int idx = p_handle_id - HANDLE_BACK_OFFSET;
        if (idx < 0 || idx >= spline->get_point_count()) return;
        node->set_selected_gizmo_point_index(idx);
        const godot::Vector3 anchor = spline->get_point_position(idx);
        const godot::Vector3 cur_world = anchor + spline->get_point_back_handle(idx);
        const godot::Vector3 new_world = _project_screen_to_local_plane(node, p_camera, p_screen_pos, cur_world);
        spline->set_point_back_handle(idx, new_world - anchor, true);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= 0 && p_handle_id < spline->get_point_count()) {
        node->set_selected_gizmo_point_index(p_handle_id);
        const godot::Vector3 cur_pos = spline->get_point_position(p_handle_id);
        const godot::Vector3 new_pos = _project_screen_to_local_plane(node, p_camera, p_screen_pos, cur_pos);
        spline->set_point_position(p_handle_id, new_pos);
        node->update_gizmos();
    }
}

void Spline3DGizmo::_commit_handle(
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
    SplineNode3D *node = godot::Object::cast_to<SplineNode3D>(p_gizmo->get_node_3d());
    if (!node || node->get_spline().is_null()) {
        return;
    }
    godot::Ref<Spline3DResource> spline = node->get_spline();
    const uint64_t nid = node->get_instance_id();

    if (spline->get_undo_redo().is_null()) {
        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
    }

    if (p_handle_id == HANDLE_AABB_SCALE_ID || p_handle_id == HANDLE_AABB_ROTATE_Y_ID) {
        const godot::Dictionary before = p_restore;
        if (p_cancel) {
            spline->apply_snapshot_state(before);
        } else {
            const godot::Dictionary after = spline->get_snapshot_state();
            spline->commit_snapshot_action(
                (p_handle_id == HANDLE_AABB_SCALE_ID) ? "Scale Spline3D via Gizmo" : "Rotate Spline3D via Gizmo",
                before,
                after
            );
        }
        active_drags.erase(nid);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_SECTION_SPLIT_OFFSET) {
        const int sec_idx = p_handle_id - HANDLE_SECTION_SPLIT_OFFSET;
        if (!p_cancel && sec_idx >= 0 && sec_idx < spline->get_section_count()) {
            const godot::Vector3 mid_pt   = spline->sample_section(sec_idx, static_cast<real_t>(0.5));
            const SplineCurveType ctype   = spline->get_section_curve_type(sec_idx);
            const real_t half_weight      = std::max(static_cast<real_t>(0.01), spline->get_section_weight(sec_idx) * static_cast<real_t>(0.5));
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
        active_drags.erase(nid);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= HANDLE_FORE_OFFSET) {
        const int idx = p_handle_id - HANDLE_FORE_OFFSET;
        if (idx >= 0 && idx < spline->get_point_count()) {
            const godot::Vector3 prev_h = p_restore;
            const godot::Vector3 cur_h  = spline->get_point_fore_handle(idx);
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
            const godot::Vector3 prev_h = p_restore;
            const godot::Vector3 cur_h  = spline->get_point_back_handle(idx);
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
        const godot::Vector3 prev_pos = p_restore;
        const godot::Vector3 cur_pos  = spline->get_point_position(p_handle_id);
        spline->set_point_position(p_handle_id, prev_pos);
        if (!p_cancel) {
            spline->commit_set_point_position(p_handle_id, cur_pos);
        }
    }

    active_drags.erase(nid);
    node->update_gizmos();
}

} // namespace ideam::godot_ext