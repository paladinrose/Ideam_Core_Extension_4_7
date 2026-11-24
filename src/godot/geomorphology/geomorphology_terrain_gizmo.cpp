#include "geomorphology_terrain_gizmo.h"

#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>

#include <algorithm>

namespace ideam::godot_ext {

bool GeomorphologyTerrainGizmo::can_handle(godot::Node3D *p_target) const {
    return godot::Object::cast_to<GeomorphologyTerrainNode3D>(p_target) != nullptr;
}

void GeomorphologyTerrainGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    (void)p_view;
    auto *node = godot::Object::cast_to<GeomorphologyTerrainNode3D>(p_target);
    if (!node || node->get_terrain_resource().is_null()) return;

    auto res = node->get_terrain_resource();
    const float w_m = static_cast<float>(std::max(1, res->get_grid_width())) * res->get_cell_size_meters();
    const float h_m = static_cast<float>(std::max(1, res->get_grid_height())) * res->get_cell_size_meters();

    // Domain footprint wireframe & coarse elevation contour sample grid
    const godot::Vector3 center(w_m * 0.5f, 0.0f, h_m * 0.5f);
    r_out.add_wire_box(center, godot::Vector3(w_m * 0.5f, 0.25f, h_m * 0.5f), godot::Basis(), GIZMO_LINE_CLEARANCE);

    constexpr int kSamples = 8;
    const godot::Transform3D xf = node->get_global_transform();
    godot::PackedVector3Array grid_lines;
    for (int gz = 0; gz <= kSamples; ++gz) {
        const float z = (static_cast<float>(gz) / static_cast<float>(kSamples)) * h_m;
        for (int gx = 0; gx < kSamples; ++gx) {
            const float x0 = (static_cast<float>(gx) / static_cast<float>(kSamples)) * w_m;
            const float x1 = (static_cast<float>(gx + 1) / static_cast<float>(kSamples)) * w_m;
            const float y0 = node->sample_surface_elevation_at_world(xf.xform(godot::Vector3(x0, 0.0f, z)));
            const float y1 = node->sample_surface_elevation_at_world(xf.xform(godot::Vector3(x1, 0.0f, z)));
            grid_lines.append(godot::Vector3(x0, y0, z));
            grid_lines.append(godot::Vector3(x1, y1, z));
        }
    }
    if (!grid_lines.is_empty()) {
        r_out.add_lines(grid_lines, GIZMO_LINE_AXIS_NORMAL);
    }

    r_out.add_handle(
        HANDLE_CELL_SIZE_ID,
        godot::Vector3(w_m, 0.25f, h_m),
        GIZMO_LINE_RULER,
        HANDLE_SHAPE_CUBE,
        static_cast<real_t>(0.18),
        GIZMO_STATE_NONE,
        "Terrain Cell Size (m)"
    );
}

godot::String GeomorphologyTerrainGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_target;
    (void)p_handle_id;
    (void)p_secondary;
    return "Terrain Cell Size (m)";
}

godot::Variant GeomorphologyTerrainGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_handle_id;
    (void)p_secondary;
    auto *node = godot::Object::cast_to<GeomorphologyTerrainNode3D>(p_target);
    if (!node || node->get_terrain_resource().is_null()) return 1.0f;
    return node->get_terrain_resource()->get_cell_size_meters();
}

void GeomorphologyTerrainGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_handle_id;
    (void)p_ray;
    auto *node = godot::Object::cast_to<GeomorphologyTerrainNode3D>(p_target);
    if (!node || node->get_terrain_resource().is_null()) return;
    drag_initial_cell_size_[node->get_instance_id()] = node->get_terrain_resource()->get_cell_size_meters();
}

void GeomorphologyTerrainGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_handle_id;
    (void)p_modifiers;
    auto *node = godot::Object::cast_to<GeomorphologyTerrainNode3D>(p_target);
    if (!node || node->get_terrain_resource().is_null() || !p_ray.is_valid()) return;
    auto res = node->get_terrain_resource();
    if (!drag_initial_cell_size_.has(node->get_instance_id())) {
        on_handle_drag_begin(p_target, p_handle_id, p_ray);
    }

    const godot::Transform3D gt = node->get_global_transform();
    const godot::Plane ground_plane(gt.basis.get_column(1).normalized(), gt.origin);
    godot::Vector3 hit;
    if (ground_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit)) {
        const godot::Vector3 local_hit = gt.affine_inverse().xform(hit);
        const float gw = static_cast<float>(std::max(1, res->get_grid_width()));
        const float new_cell = std::clamp(local_hit.x / gw, 0.25f, 250.0f);
        res->set_cell_size_meters(new_cell);
        node->update_gizmos();
    }
}

void GeomorphologyTerrainGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    (void)p_handle_id;
    auto *node = godot::Object::cast_to<GeomorphologyTerrainNode3D>(p_target);
    if (!node || node->get_terrain_resource().is_null()) return;
    const uint64_t nid = node->get_instance_id();
    if (!drag_initial_cell_size_.has(nid)) return;
    const float prev = drag_initial_cell_size_[nid];
    drag_initial_cell_size_.erase(nid);
    auto res = node->get_terrain_resource();

    if (p_cancel) {
        res->set_cell_size_meters(prev);
        node->update_gizmos();
        return;
    }
    const float cur = res->get_cell_size_meters();
    if (auto ur = resolve_undo_redo(); ur.is_valid()) {
        ur->create_action("Resize Terrain Cell Size");
        ur->add_do_method(res.ptr(), "set_cell_size_meters", cur);
        ur->add_undo_method(res.ptr(), "set_cell_size_meters", prev);
        ur->commit_action(false);
    }
    node->update_gizmos();
}

} // namespace ideam::godot_ext
