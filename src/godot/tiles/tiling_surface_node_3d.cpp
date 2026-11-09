#include "tiling_surface_node_3d.h"
#include "../../core/geometry/parallel_transport_3d.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>
#include <span>

namespace ideam::godot_ext {

void TilingSurfaceNode3D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(PROJECTION_XZ_GROUND);
    BIND_ENUM_CONSTANT(PROJECTION_XY_WALL);
    BIND_ENUM_CONSTANT(PROJECTION_YZ_SIDE);
    BIND_ENUM_CONSTANT(PROJECTION_SPLINE_3D_RIBBON);

    ClassDB::bind_method(D_METHOD("set_tiling", "tiling"), &TilingSurfaceNode3D::set_tiling);
    ClassDB::bind_method(D_METHOD("get_tiling"), &TilingSurfaceNode3D::get_tiling);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "tiling", PROPERTY_HINT_RESOURCE_TYPE, "TilingResource"),
        "set_tiling",
        "get_tiling"
    );

    ClassDB::bind_method(D_METHOD("set_boundary_spline_2d", "spline"), &TilingSurfaceNode3D::set_boundary_spline_2d);
    ClassDB::bind_method(D_METHOD("get_boundary_spline_2d"), &TilingSurfaceNode3D::get_boundary_spline_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "boundary_spline_2d", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_boundary_spline_2d",
        "get_boundary_spline_2d"
    );

    ClassDB::bind_method(D_METHOD("set_path_spline_3d", "spline"), &TilingSurfaceNode3D::set_path_spline_3d);
    ClassDB::bind_method(D_METHOD("get_path_spline_3d"), &TilingSurfaceNode3D::get_path_spline_3d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "path_spline_3d", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"),
        "set_path_spline_3d",
        "get_path_spline_3d"
    );

    ClassDB::bind_method(D_METHOD("set_fallback_region_rect", "rect"), &TilingSurfaceNode3D::set_fallback_region_rect);
    ClassDB::bind_method(D_METHOD("get_fallback_region_rect"), &TilingSurfaceNode3D::get_fallback_region_rect);
    ADD_PROPERTY(
        PropertyInfo(Variant::RECT2, "fallback_region_rect"),
        "set_fallback_region_rect",
        "get_fallback_region_rect"
    );

    ClassDB::bind_method(D_METHOD("set_projection_mode", "mode"), &TilingSurfaceNode3D::set_projection_mode);
    ClassDB::bind_method(D_METHOD("get_projection_mode"), &TilingSurfaceNode3D::get_projection_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "projection_mode",
            PROPERTY_HINT_ENUM,
            "XZ Ground Plane:0,XY Wall Plane:1,YZ Side Plane:2,3D Spline Ribbon:3"
        ),
        "set_projection_mode",
        "get_projection_mode"
    );

    ClassDB::bind_method(D_METHOD("set_extrude_depth", "depth"), &TilingSurfaceNode3D::set_extrude_depth);
    ClassDB::bind_method(D_METHOD("get_extrude_depth"), &TilingSurfaceNode3D::get_extrude_depth);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "extrude_depth", PROPERTY_HINT_RANGE, "-10.0,10.0,0.05"),
        "set_extrude_depth",
        "get_extrude_depth"
    );

    ClassDB::bind_method(D_METHOD("set_edge_subdivisions", "subdivs"), &TilingSurfaceNode3D::set_edge_subdivisions);
    ClassDB::bind_method(D_METHOD("get_edge_subdivisions"), &TilingSurfaceNode3D::get_edge_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "edge_subdivisions", PROPERTY_HINT_RANGE, "1,64,1"),
        "set_edge_subdivisions",
        "get_edge_subdivisions"
    );

    ADD_GROUP("Visual Styling", "");
    ClassDB::bind_method(D_METHOD("set_color_0", "color"), &TilingSurfaceNode3D::set_color_0);
    ClassDB::bind_method(D_METHOD("get_color_0"), &TilingSurfaceNode3D::get_color_0);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_0"), "set_color_0", "get_color_0");

    ClassDB::bind_method(D_METHOD("set_color_1", "color"), &TilingSurfaceNode3D::set_color_1);
    ClassDB::bind_method(D_METHOD("get_color_1"), &TilingSurfaceNode3D::get_color_1);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_1"), "set_color_1", "get_color_1");

    ClassDB::bind_method(D_METHOD("set_color_2", "color"), &TilingSurfaceNode3D::set_color_2);
    ClassDB::bind_method(D_METHOD("get_color_2"), &TilingSurfaceNode3D::get_color_2);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_2"), "set_color_2", "get_color_2");

    ClassDB::bind_method(D_METHOD("set_bridge_quad_color", "color"), &TilingSurfaceNode3D::set_bridge_quad_color);
    ClassDB::bind_method(D_METHOD("get_bridge_quad_color"), &TilingSurfaceNode3D::get_bridge_quad_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "bridge_quad_color"), "set_bridge_quad_color", "get_bridge_quad_color");

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &TilingSurfaceNode3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &TilingSurfaceNode3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("get_generated_mesh"), &TilingSurfaceNode3D::get_generated_mesh);
    ClassDB::bind_method(D_METHOD("rebuild_mesh"), &TilingSurfaceNode3D::rebuild_mesh);
    ClassDB::bind_method(D_METHOD("_on_dependency_changed"), &TilingSurfaceNode3D::_on_dependency_changed);
}

TilingSurfaceNode3D::TilingSurfaceNode3D() {
    mesh_.instantiate();
    if (auto *rs = godot::RenderingServer::get_singleton()) {
        mesh_instance_rid_ = rs->instance_create();
        rs->instance_set_base(mesh_instance_rid_, mesh_->get_rid());
    }
}

TilingSurfaceNode3D::~TilingSurfaceNode3D() {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (tiling_.is_valid() && tiling_->is_connected("changed", cb)) {
        tiling_->disconnect("changed", cb);
    }
    if (boundary_spline_2d_.is_valid() && boundary_spline_2d_->is_connected("changed", cb)) {
        boundary_spline_2d_->disconnect("changed", cb);
    }
    if (path_spline_3d_.is_valid() && path_spline_3d_->is_connected("changed", cb)) {
        path_spline_3d_->disconnect("changed", cb);
    }
    if (mesh_instance_rid_.is_valid()) {
        if (auto *rs = godot::RenderingServer::get_singleton()) {
            rs->free_rid(mesh_instance_rid_);
        }
    }
}

void TilingSurfaceNode3D::_notification(int p_what) {
    auto *rs = godot::RenderingServer::get_singleton();
    if (!rs || !mesh_instance_rid_.is_valid()) return;

    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            if (get_world_3d().is_valid()) {
                rs->instance_set_scenario(mesh_instance_rid_, get_world_3d()->get_scenario());
            }
            rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
            rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            rebuild_mesh();
            break;
        }
        case NOTIFICATION_EXIT_WORLD: {
            rs->instance_set_scenario(mesh_instance_rid_, godot::RID());
            break;
        }
        case NOTIFICATION_TRANSFORM_CHANGED: {
            rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            break;
        }
        default:
            break;
    }
}

void TilingSurfaceNode3D::_on_dependency_changed() {
    local_cache_dirty_ = true;
    rebuild_mesh();
}

const TilingSurfaceNode3D::CoreRegion *TilingSurfaceNode3D::_resolve_active_region() const {
    if (!tiling_.is_valid()) {
        return nullptr;
    }

    godot::Ref<CompositeTilingResource> comp = tiling_;
    if (comp.is_valid()) {
        return &comp->get_core_region();
    }

    godot::Ref<WallpaperTilingResource> wp = tiling_;
    if (wp.is_valid()) {
        if (local_cache_dirty_) {
            local_cache_dirty_ = false;
            local_region_cache_.clear();
            local_region_cache_.set_tiling(wp->get_core_tiling());

            if (boundary_spline_2d_.is_valid() && boundary_spline_2d_->get_point_count() >= 3) {
                const CoreSpline &cs = boundary_spline_2d_->get_core_spline();
                local_region_cache_.build_from_splines(
                    std::span<const CoreSpline>(&cs, 1),
                    Vec2(static_cast<real_t>(0.0)),
                    core::TilingCoverageMode::INCLUDE_STRADDLING,
                    edge_subdivisions_,
                    false
                );
            } else {
                const core::AABB2D<real_t> bounds(
                    _to_glm(fallback_region_rect_.position),
                    _to_glm(fallback_region_rect_.position + fallback_region_rect_.size)
                );
                local_region_cache_.build_from_aabb(
                    bounds,
                    Vec2(static_cast<real_t>(0.0)),
                    false,
                    edge_subdivisions_
                );
            }
        }
        return &local_region_cache_;
    }

    return nullptr;
}

godot::Vector3 TilingSurfaceNode3D::_project_point_2d_to_3d(
    const Vec2 &p_pt,
    real_t p_normal_offset,
    godot::Vector3 &r_normal
) const {
    const float px = static_cast<float>(p_pt.x);
    const float py = static_cast<float>(p_pt.y);
    const float noff = static_cast<float>(p_normal_offset);

    switch (projection_mode_) {
        case PROJECTION_XZ_GROUND: {
            r_normal = godot::Vector3(0.0f, 1.0f, 0.0f);
            return godot::Vector3(px, noff, py);
        }
        case PROJECTION_XY_WALL: {
            r_normal = godot::Vector3(0.0f, 0.0f, 1.0f);
            return godot::Vector3(px, py, noff);
        }
        case PROJECTION_YZ_SIDE: {
            r_normal = godot::Vector3(1.0f, 0.0f, 0.0f);
            return godot::Vector3(noff, py, px);
        }
        case PROJECTION_SPLINE_3D_RIBBON: {
            if (path_spline_3d_.is_valid() && path_spline_3d_->get_point_count() >= 2) {
                const real_t span_x = std::max<real_t>(
                    static_cast<real_t>(1e-4),
                    fallback_region_rect_.size.x
                );
                const real_t u_norm = std::clamp<real_t>(
                    (p_pt.x - fallback_region_rect_.position.x) / span_x,
                    static_cast<real_t>(0.0),
                    static_cast<real_t>(1.0)
                );

                const godot::Transform3D xf = path_spline_3d_->get_transform_at_percent(u_norm, true);
                const godot::Vector3 binormal = xf.basis.get_column(0);
                const godot::Vector3 up_norm  = xf.basis.get_column(1);
                r_normal = up_norm;
                return xf.origin + binormal * py + up_norm * noff;
            }
            r_normal = godot::Vector3(0.0f, 1.0f, 0.0f);
            return godot::Vector3(px, noff, py);
        }
    }
    r_normal = godot::Vector3(0.0f, 1.0f, 0.0f);
    return godot::Vector3(px, noff, py);
}

void TilingSurfaceNode3D::rebuild_mesh() {
    if (!mesh_.is_valid()) {
        mesh_.instantiate();
    }
    mesh_->clear_surfaces();

    const CoreRegion *region = _resolve_active_region();
    if (!region || region->get_cell_count() == 0) {
        return;
    }

    const godot::Color palette[3] = { color_0_, color_1_, color_2_ };
    const auto cells = region->get_cells();

    godot::PackedVector3Array vertices;
    godot::PackedVector3Array normals;
    godot::PackedVector2Array uvs;
    godot::PackedColorArray colors;
    godot::PackedInt32Array indices;

    const bool do_extrude = std::abs(extrude_depth_) > static_cast<real_t>(1e-4);

    for (uint32_t cid = 0; cid < static_cast<uint32_t>(cells.size()); ++cid) {
        const auto &cell = cells[cid];
        if ((cell.flags & core::CELL_FLAG_MASKED) != 0u) continue;

        const auto poly = region->get_cell_polygon_points(cid);
        if (poly.size() < 3) continue;

        const godot::Color col = ((cell.flags & core::CELL_FLAG_BRIDGE_QUAD) != 0u)
            ? bridge_quad_color_
            : palette[cell.colour_id % 3u];

        const int32_t top_base_idx = static_cast<int32_t>(vertices.size());
        for (size_t i = 0; i < poly.size(); ++i) {
            godot::Vector3 n;
            const godot::Vector3 p3 = _project_point_2d_to_3d(
                poly[i],
                do_extrude ? extrude_depth_ : static_cast<real_t>(0.0),
                n
            );
            vertices.push_back(p3);
            normals.push_back(n);
            uvs.push_back(godot::Vector2(static_cast<float>(poly[i].x), static_cast<float>(poly[i].y)));
            colors.push_back(col);
        }

        // Fan triangulation for cell top face
        for (size_t i = 1; i + 1 < poly.size(); ++i) {
            indices.push_back(top_base_idx);
            indices.push_back(top_base_idx + static_cast<int32_t>(i));
            indices.push_back(top_base_idx + static_cast<int32_t>(i + 1));
        }

        if (do_extrude) {
            const godot::Color side_col(col.r * 0.8f, col.g * 0.8f, col.b * 0.8f, col.a);
            for (size_t i = 0; i < poly.size(); ++i) {
                const size_t next_i = (i + 1) % poly.size();
                godot::Vector3 n_top, n_bot;
                const godot::Vector3 p_top_a = _project_point_2d_to_3d(poly[i], extrude_depth_, n_top);
                const godot::Vector3 p_top_b = _project_point_2d_to_3d(poly[next_i], extrude_depth_, n_top);
                const godot::Vector3 p_bot_a = _project_point_2d_to_3d(poly[i], static_cast<real_t>(0.0), n_bot);
                const godot::Vector3 p_bot_b = _project_point_2d_to_3d(poly[next_i], static_cast<real_t>(0.0), n_bot);

                const godot::Vector3 e1(p_top_b.x - p_top_a.x, p_top_b.y - p_top_a.y, p_top_b.z - p_top_a.z);
                const godot::Vector3 e2(p_bot_a.x - p_top_a.x, p_bot_a.y - p_top_a.y, p_bot_a.z - p_top_a.z);
                godot::Vector3 side_normal(
                    e1.y * e2.z - e1.z * e2.y,
                    e1.z * e2.x - e1.x * e2.z,
                    e1.x * e2.y - e1.y * e2.x
                );
                const float n_len = std::sqrt(
                    side_normal.x * side_normal.x +
                    side_normal.y * side_normal.y +
                    side_normal.z * side_normal.z
                );
                if (n_len > 1e-6f) {
                    side_normal.x /= n_len;
                    side_normal.y /= n_len;
                    side_normal.z /= n_len;
                }

                const int32_t side_base = static_cast<int32_t>(vertices.size());
                vertices.push_back(p_top_a);
                vertices.push_back(p_top_b);
                vertices.push_back(p_bot_b);
                vertices.push_back(p_bot_a);

                for (int k = 0; k < 4; ++k) {
                    normals.push_back(side_normal);
                    colors.push_back(side_col);
                }
                uvs.push_back(godot::Vector2(0.0f, 1.0f));
                uvs.push_back(godot::Vector2(1.0f, 1.0f));
                uvs.push_back(godot::Vector2(1.0f, 0.0f));
                uvs.push_back(godot::Vector2(0.0f, 0.0f));

                indices.push_back(side_base + 0);
                indices.push_back(side_base + 1);
                indices.push_back(side_base + 2);
                indices.push_back(side_base + 0);
                indices.push_back(side_base + 2);
                indices.push_back(side_base + 3);
            }
        }
    }

    if (vertices.size() >= 3 && indices.size() >= 3) {
        godot::Array arrays;
        arrays.resize(godot::Mesh::ARRAY_MAX);
        arrays[godot::Mesh::ARRAY_VERTEX] = vertices;
        arrays[godot::Mesh::ARRAY_NORMAL] = normals;
        arrays[godot::Mesh::ARRAY_TEX_UV] = uvs;
        arrays[godot::Mesh::ARRAY_COLOR]  = colors;
        arrays[godot::Mesh::ARRAY_INDEX]  = indices;

        mesh_->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES, arrays);
        if (material_override_.is_valid()) {
            mesh_->surface_set_material(0, material_override_);
        }
    }
}

void TilingSurfaceNode3D::set_tiling(const godot::Ref<TilingResource> &p_tiling) {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (tiling_.is_valid() && tiling_->is_connected("changed", cb)) {
        tiling_->disconnect("changed", cb);
    }
    tiling_ = p_tiling;
    if (tiling_.is_valid() && !tiling_->is_connected("changed", cb)) {
        tiling_->connect("changed", cb);
    }
    local_cache_dirty_ = true;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_boundary_spline_2d(const godot::Ref<Spline2DResource> &p_spline) {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (boundary_spline_2d_.is_valid() && boundary_spline_2d_->is_connected("changed", cb)) {
        boundary_spline_2d_->disconnect("changed", cb);
    }
    boundary_spline_2d_ = p_spline;
    if (boundary_spline_2d_.is_valid() && !boundary_spline_2d_->is_connected("changed", cb)) {
        boundary_spline_2d_->connect("changed", cb);
    }
    local_cache_dirty_ = true;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_path_spline_3d(const godot::Ref<Spline3DResource> &p_spline) {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (path_spline_3d_.is_valid() && path_spline_3d_->is_connected("changed", cb)) {
        path_spline_3d_->disconnect("changed", cb);
    }
    path_spline_3d_ = p_spline;
    if (path_spline_3d_.is_valid() && !path_spline_3d_->is_connected("changed", cb)) {
        path_spline_3d_->connect("changed", cb);
    }
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_fallback_region_rect(const godot::Rect2 &p_rect) {
    fallback_region_rect_ = p_rect;
    local_cache_dirty_ = true;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_projection_mode(int p_mode) {
    projection_mode_ = static_cast<Tiling3DProjectionMode>(std::clamp(p_mode, 0, 3));
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_extrude_depth(real_t p_depth) {
    extrude_depth_ = p_depth;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_edge_subdivisions(int p_subdivs) {
    edge_subdivisions_ = std::clamp(p_subdivs, 1, 64);
    local_cache_dirty_ = true;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_color_0(const godot::Color &p_color) {
    color_0_ = p_color;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_color_1(const godot::Color &p_color) {
    color_1_ = p_color;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_color_2(const godot::Color &p_color) {
    color_2_ = p_color;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_bridge_quad_color(const godot::Color &p_color) {
    bridge_quad_color_ = p_color;
    rebuild_mesh();
}

void TilingSurfaceNode3D::set_material_override(const godot::Ref<godot::Material> &p_material) {
    material_override_ = p_material;
    if (mesh_instance_rid_.is_valid()) {
        if (auto *rs = godot::RenderingServer::get_singleton()) {
            rs->instance_geometry_set_material_override(
                mesh_instance_rid_,
                material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
            );
        }
    }
    rebuild_mesh();
}

} // namespace ideam::godot_ext