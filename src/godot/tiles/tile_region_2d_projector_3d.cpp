#include "tile_region_2d_projector_3d.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace ideam::godot_ext {

TileRegion2DProjector3D::TileRegion2DProjector3D() {
    default_vertex_color_material_.instantiate();
    default_vertex_color_material_->set_flag(
        godot::BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR,
        true
    );
    default_vertex_color_material_->set_roughness(0.68f);
}

TileRegion2DProjector3D::~TileRegion2DProjector3D() {
    _disconnect_tiling_dependencies();
}

void TileRegion2DProjector3D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_tiling_resource", "tiling"), &TileRegion2DProjector3D::set_tiling_resource);
    ClassDB::bind_method(D_METHOD("get_tiling_resource"), &TileRegion2DProjector3D::get_tiling_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "tiling_resource", PROPERTY_HINT_RESOURCE_TYPE, "TilingResource"),
        "set_tiling_resource",
        "get_tiling_resource"
    );

    ClassDB::bind_method(D_METHOD("set_boundary_spline_2d", "spline"), &TileRegion2DProjector3D::set_boundary_spline_2d);
    ClassDB::bind_method(D_METHOD("get_boundary_spline_2d"), &TileRegion2DProjector3D::get_boundary_spline_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "boundary_spline_2d", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_boundary_spline_2d",
        "get_boundary_spline_2d"
    );

    ClassDB::bind_method(D_METHOD("set_fallback_region_rect", "rect"), &TileRegion2DProjector3D::set_fallback_region_rect);
    ClassDB::bind_method(D_METHOD("get_fallback_region_rect"), &TileRegion2DProjector3D::get_fallback_region_rect);
    ADD_PROPERTY(
        PropertyInfo(Variant::RECT2, "fallback_region_rect"),
        "set_fallback_region_rect",
        "get_fallback_region_rect"
    );

    ClassDB::bind_method(D_METHOD("set_tile_inset_ratio", "ratio"), &TileRegion2DProjector3D::set_tile_inset_ratio);
    ClassDB::bind_method(D_METHOD("get_tile_inset_ratio"), &TileRegion2DProjector3D::get_tile_inset_ratio);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "tile_inset_ratio", PROPERTY_HINT_RANGE, "0.0,0.6,0.01"),
        "set_tile_inset_ratio",
        "get_tile_inset_ratio"
    );

    ClassDB::bind_method(D_METHOD("set_edge_subdivisions", "subdivisions"), &TileRegion2DProjector3D::set_edge_subdivisions);
    ClassDB::bind_method(D_METHOD("get_edge_subdivisions"), &TileRegion2DProjector3D::get_edge_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "edge_subdivisions", PROPERTY_HINT_RANGE, "1,48,1"),
        "set_edge_subdivisions",
        "get_edge_subdivisions"
    );

    ClassDB::bind_method(D_METHOD("set_project_hex_super_cells", "project_hex"), &TileRegion2DProjector3D::set_project_hex_super_cells);
    ClassDB::bind_method(D_METHOD("get_project_hex_super_cells"), &TileRegion2DProjector3D::get_project_hex_super_cells);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "project_hex_super_cells"),
        "set_project_hex_super_cells",
        "get_project_hex_super_cells"
    );

    ClassDB::bind_method(D_METHOD("set_highlighted_cell_id", "cell_id"), &TileRegion2DProjector3D::set_highlighted_cell_id);
    ClassDB::bind_method(D_METHOD("get_highlighted_cell_id"), &TileRegion2DProjector3D::get_highlighted_cell_id);

    ClassDB::bind_method(D_METHOD("set_color_0", "color"), &TileRegion2DProjector3D::set_color_0);
    ClassDB::bind_method(D_METHOD("get_color_0"), &TileRegion2DProjector3D::get_color_0);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_0"), "set_color_0", "get_color_0");

    ClassDB::bind_method(D_METHOD("set_color_1", "color"), &TileRegion2DProjector3D::set_color_1);
    ClassDB::bind_method(D_METHOD("get_color_1"), &TileRegion2DProjector3D::get_color_1);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_1"), "set_color_1", "get_color_1");

    ClassDB::bind_method(D_METHOD("set_color_2", "color"), &TileRegion2DProjector3D::set_color_2);
    ClassDB::bind_method(D_METHOD("get_color_2"), &TileRegion2DProjector3D::get_color_2);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color_2"), "set_color_2", "get_color_2");

    ClassDB::bind_method(D_METHOD("set_bridge_quad_color", "color"), &TileRegion2DProjector3D::set_bridge_quad_color);
    ClassDB::bind_method(D_METHOD("get_bridge_quad_color"), &TileRegion2DProjector3D::get_bridge_quad_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "bridge_quad_color"), "set_bridge_quad_color", "get_bridge_quad_color");

    ClassDB::bind_method(D_METHOD("set_highlight_color", "color"), &TileRegion2DProjector3D::set_highlight_color);
    ClassDB::bind_method(D_METHOD("get_highlight_color"), &TileRegion2DProjector3D::get_highlight_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "highlight_color"), "set_highlight_color", "get_highlight_color");

    ClassDB::bind_method(D_METHOD("get_projected_cell_count"), &TileRegion2DProjector3D::get_projected_cell_count);
    ClassDB::bind_method(D_METHOD("get_projected_cell_center_3d", "cell_id"), &TileRegion2DProjector3D::get_projected_cell_center_3d);
    ClassDB::bind_method(D_METHOD("get_projected_cell_transform_3d", "cell_id"), &TileRegion2DProjector3D::get_projected_cell_transform_3d);
    ClassDB::bind_method(D_METHOD("get_projected_cell_polygon_3d", "cell_id"), &TileRegion2DProjector3D::get_projected_cell_polygon_3d);
    ClassDB::bind_method(
        D_METHOD("find_closest_projected_cell_3d", "local_point_3d", "max_distance"),
        &TileRegion2DProjector3D::find_closest_projected_cell_3d,
        DEFVAL(10.0f)
    );
    ClassDB::bind_method(
        D_METHOD("find_projected_path_points_3d", "start_cell", "goal_cell"),
        &TileRegion2DProjector3D::find_projected_path_points_3d
    );
    ClassDB::bind_method(
        D_METHOD("bake_projected_path_spline_3d", "start_cell", "goal_cell"),
        &TileRegion2DProjector3D::bake_projected_path_spline_3d
    );
}

void TileRegion2DProjector3D::_disconnect_tiling_dependencies() {
    const godot::Callable cb = callable_mp(this, &TileRegion2DProjector3D::_on_tiling_dependency_changed);
    if (tiling_resource_.is_valid() && tiling_resource_->is_connected("changed", cb)) {
        tiling_resource_->disconnect("changed", cb);
    }
    if (boundary_spline_2d_.is_valid() && boundary_spline_2d_->is_connected("changed", cb)) {
        boundary_spline_2d_->disconnect("changed", cb);
    }
}

void TileRegion2DProjector3D::_connect_tiling_dependencies() {
    const godot::Callable cb = callable_mp(this, &TileRegion2DProjector3D::_on_tiling_dependency_changed);
    if (tiling_resource_.is_valid() && !tiling_resource_->is_connected("changed", cb)) {
        tiling_resource_->connect("changed", cb);
    }
    if (boundary_spline_2d_.is_valid() && !boundary_spline_2d_->is_connected("changed", cb)) {
        boundary_spline_2d_->connect("changed", cb);
    }
}

void TileRegion2DProjector3D::_on_tiling_dependency_changed() {
    synthesized_cache_dirty_ = true;
    rebuild_projection();
}

const TileRegion2DProjector3D::CoreRegion *TileRegion2DProjector3D::_resolve_active_region() const {
    if (tiling_resource_.is_null()) {
        return nullptr;
    }
    if (auto *comp = godot::Object::cast_to<CompositeTilingResource>(tiling_resource_.ptr())) {
        return &comp->get_core_region();
    }
    if (auto *hyp = godot::Object::cast_to<HyperbolicPoincareTilingResource>(tiling_resource_.ptr())) {
        return &hyp->get_core_region();
    }
    if (auto *wp = godot::Object::cast_to<WallpaperTilingResource>(tiling_resource_.ptr())) {
        if (synthesized_cache_dirty_) {
            synthesized_region_cache_.clear();
            const Vec2 seed(
                static_cast<real_t>(fallback_region_rect_.position.x + fallback_region_rect_.size.x * 0.5f),
                static_cast<real_t>(fallback_region_rect_.position.y + fallback_region_rect_.size.y * 0.5f)
            );
            if (boundary_spline_2d_.is_valid() && boundary_spline_2d_->get_point_count() >= 3) {
                synthesized_region_cache_.populate_from_spline(
                    wp->get_core_tiling(),
                    boundary_spline_2d_->get_core_spline(),
                    seed,
                    0,
                    core::TilingBoundaryCoverageMode::INCLUDE_STRADDLING,
                    edge_subdivisions_
                );
            } else {
                const Vec2 bmin(
                    static_cast<real_t>(fallback_region_rect_.position.x),
                    static_cast<real_t>(fallback_region_rect_.position.y)
                );
                const Vec2 bmax(
                    static_cast<real_t>(fallback_region_rect_.position.x + fallback_region_rect_.size.x),
                    static_cast<real_t>(fallback_region_rect_.position.y + fallback_region_rect_.size.y)
                );
                synthesized_region_cache_.populate_from_aabb(
                    wp->get_core_tiling(),
                    bmin,
                    bmax,
                    seed,
                    0,
                    edge_subdivisions_
                );
            }
            if (project_hex_super_cells_) {
                synthesized_region_cache_.build_hex_super_cells();
            }
            synthesized_cache_dirty_ = false;
        }
        return &synthesized_region_cache_;
    }
    return nullptr;
}

void TileRegion2DProjector3D::set_tiling_resource(const godot::Ref<TilingResource> &p_tiling) {
    if (tiling_resource_ == p_tiling) {
        return;
    }
    _disconnect_tiling_dependencies();
    tiling_resource_ = p_tiling;
    _connect_tiling_dependencies();
    _on_tiling_dependency_changed();
}

void TileRegion2DProjector3D::set_boundary_spline_2d(const godot::Ref<Spline2DResource> &p_spline) {
    if (boundary_spline_2d_ == p_spline) {
        return;
    }
    _disconnect_tiling_dependencies();
    boundary_spline_2d_ = p_spline;
    _connect_tiling_dependencies();
    _on_tiling_dependency_changed();
}

void TileRegion2DProjector3D::set_fallback_region_rect(const godot::Rect2 &p_rect) {
    fallback_region_rect_    = p_rect;
    synthesized_cache_dirty_ = true;
    rebuild_projection();
}

void TileRegion2DProjector3D::set_tile_inset_ratio(real_t p_ratio) {
    tile_inset_ratio_ = std::clamp(p_ratio, static_cast<real_t>(0.0), static_cast<real_t>(0.6));
    rebuild_projection();
}

void TileRegion2DProjector3D::set_edge_subdivisions(int p_subdivs) {
    edge_subdivisions_       = std::clamp(p_subdivs, 1, 48);
    synthesized_cache_dirty_ = true;
    rebuild_projection();
}

void TileRegion2DProjector3D::set_project_hex_super_cells(bool p_hex) {
    project_hex_super_cells_ = p_hex;
    synthesized_cache_dirty_ = true;
    rebuild_projection();
}

void TileRegion2DProjector3D::set_highlighted_cell_id(int p_cell_id) {
    highlighted_cell_id_ = p_cell_id;
    rebuild_projection();
}

void TileRegion2DProjector3D::set_color_0(const godot::Color &p_color) {
    color_0_ = p_color;
    rebuild_projection();
}

void TileRegion2DProjector3D::set_color_1(const godot::Color &p_color) {
    color_1_ = p_color;
    rebuild_projection();
}

void TileRegion2DProjector3D::set_color_2(const godot::Color &p_color) {
    color_2_ = p_color;
    rebuild_projection();
}

void TileRegion2DProjector3D::set_bridge_quad_color(const godot::Color &p_color) {
    bridge_quad_color_ = p_color;
    rebuild_projection();
}

void TileRegion2DProjector3D::set_highlight_color(const godot::Color &p_color) {
    highlight_color_ = p_color;
    rebuild_projection();
}

void TileRegion2DProjector3D::_rebuild_projected_geometry() {
    using namespace godot;
    const CoreRegion *region = _resolve_active_region();
    if (region == nullptr || region->cell_count() == 0) {
        Geometry2DProjector3D::_rebuild_projected_geometry();
        return;
    }

    Ref<ArrayMesh> mesh;
    mesh.instantiate();

    PackedVector3Array vertices;
    PackedVector3Array normals;
    PackedColorArray colors;
    PackedInt32Array indices;

    const Color palette[3] = { color_0_, color_1_, color_2_ };
    const float inset      = std::clamp(static_cast<float>(tile_inset_ratio_), 0.0f, 0.6f);
    const real_t thickness = get_extrude_thickness();

    auto append_projected_cell = [&](const std::vector<Vec2> &p_poly_2d, const Vec2 &p_center_2d, const Color &p_col) {
        const int n = static_cast<int>(p_poly_2d.size());
        if (n < 3) {
            return;
        }
        const int32_t base_idx = static_cast<int32_t>(vertices.size());
        std::vector<Vector3> top_pts(static_cast<size_t>(n));
        std::vector<Vector3> top_nrms(static_cast<size_t>(n));

        for (int i = 0; i < n; ++i) {
            const Vector2 raw_uv(static_cast<float>(p_poly_2d[static_cast<size_t>(i)].x), static_cast<float>(p_poly_2d[static_cast<size_t>(i)].y));
            const Vector2 ctr_uv(static_cast<float>(p_center_2d.x), static_cast<float>(p_center_2d.y));
            const Vector2 inset_uv = raw_uv.lerp(ctr_uv, inset);

            Vector3 pos_3d;
            Vector3 nrm_3d;
            project_point_2d_with_normal(inset_uv, pos_3d, nrm_3d);
            top_pts[static_cast<size_t>(i)]  = pos_3d + nrm_3d * static_cast<float>(thickness);
            top_nrms[static_cast<size_t>(i)] = nrm_3d;

            vertices.push_back(top_pts[static_cast<size_t>(i)]);
            normals.push_back(nrm_3d);
            colors.push_back(p_col);
        }

        for (int i = 1; i + 1 < n; ++i) {
            indices.push_back(base_idx);
            indices.push_back(base_idx + i);
            indices.push_back(base_idx + i + 1);
        }

        if (std::abs(thickness) > static_cast<real_t>(1e-4)) {
            const Color skirt_col = p_col.darkened(0.32f);
            for (int i = 0; i < n; ++i) {
                const int in = (i + 1) % n;
                const Vector3 t0 = top_pts[static_cast<size_t>(i)];
                const Vector3 t1 = top_pts[static_cast<size_t>(in)];
                const Vector3 b0 = t0 - top_nrms[static_cast<size_t>(i)] * static_cast<float>(thickness);
                const Vector3 b1 = t1 - top_nrms[static_cast<size_t>(in)] * static_cast<float>(thickness);
                const Vector3 sn = (t1 - t0).cross(b0 - t0).normalized();

                const int32_t s_idx = static_cast<int32_t>(vertices.size());
                vertices.push_back(t0);
                vertices.push_back(t1);
                vertices.push_back(b1);
                vertices.push_back(b0);
                for (int q = 0; q < 4; ++q) {
                    normals.push_back(sn);
                    colors.push_back(skirt_col);
                }
                indices.push_back(s_idx + 0);
                indices.push_back(s_idx + 2);
                indices.push_back(s_idx + 1);
                indices.push_back(s_idx + 0);
                indices.push_back(s_idx + 3);
                indices.push_back(s_idx + 2);
            }
        }
    };

    if (project_hex_super_cells_ && region->hex_super_cell_count() > 0) {
        for (size_t hid = 0; hid < region->hex_super_cell_count(); ++hid) {
            const auto &hex = region->hex_super_cells()[hid];
            std::vector<Vec2> poly;
            poly.reserve(hex.perimeter_vertex_count);
            for (uint16_t k = 0; k < hex.perimeter_vertex_count; ++k) {
                const uint32_t vid = region->hex_perimeter_vertex_pool()[hex.first_perimeter_vertex + k];
                if (vid < region->shared_vertices().size()) {
                    poly.push_back(region->shared_vertices()[vid]);
                }
            }
            const Vec2 ctr(static_cast<real_t>(hex.center_x), static_cast<real_t>(hex.center_y));
            const Color col = (static_cast<int>(hid) == highlighted_cell_id_)
                ? highlight_color_
                : palette[hid % 3];
            append_projected_cell(poly, ctr, col);
        }
    } else {
        for (size_t cid = 0; cid < region->cell_count(); ++cid) {
            const auto &cell = region->cells()[cid];
            const auto poly  = region->cell_polygon(static_cast<uint32_t>(cid));
            const Vec2 ctr(static_cast<real_t>(cell.center_x), static_cast<real_t>(cell.center_y));
            Color col = (cell.flags & core::CELL_FLAG_BRIDGE_QUAD) != 0
                ? bridge_quad_color_
                : palette[cell.colour_index % 3];
            if (static_cast<int>(cid) == highlighted_cell_id_) {
                col = highlight_color_;
            }
            append_projected_cell(poly, ctr, col);
        }
    }

    if (!vertices.is_empty() && !indices.is_empty()) {
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = vertices;
        arrays[Mesh::ARRAY_NORMAL] = normals;
        arrays[Mesh::ARRAY_COLOR]  = colors;
        arrays[Mesh::ARRAY_INDEX]  = indices;
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

        const Ref<Material> mat = get_material_override().is_valid()
            ? get_material_override()
            : Ref<Material>(default_vertex_color_material_);
        if (mat.is_valid()) {
            mesh->surface_set_material(0, mat);
        }
    }

    _commit_generated_mesh(mesh);
}

int TileRegion2DProjector3D::get_projected_cell_count() const {
    const CoreRegion *region = _resolve_active_region();
    return region ? static_cast<int>(region->cell_count()) : 0;
}

godot::Vector3 TileRegion2DProjector3D::get_projected_cell_center_3d(int p_cell_id) const {
    const CoreRegion *region = _resolve_active_region();
    if (region == nullptr || p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= region->cell_count()) {
        return godot::Vector3();
    }
    const auto &c = region->cells()[static_cast<size_t>(p_cell_id)];
    return project_point_2d(godot::Vector2(c.center_x, c.center_y));
}

godot::Transform3D TileRegion2DProjector3D::get_projected_cell_transform_3d(int p_cell_id) const {
    const CoreRegion *region = _resolve_active_region();
    if (region == nullptr || p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= region->cell_count()) {
        return godot::Transform3D();
    }
    const auto &c = region->cells()[static_cast<size_t>(p_cell_id)];
    return project_transform_2d(godot::Vector2(c.center_x, c.center_y), godot::Vector2(1.0f, 0.0f));
}

godot::PackedVector3Array TileRegion2DProjector3D::get_projected_cell_polygon_3d(int p_cell_id) const {
    const CoreRegion *region = _resolve_active_region();
    if (region == nullptr || p_cell_id < 0 || static_cast<size_t>(p_cell_id) >= region->cell_count()) {
        return {};
    }
    const auto poly2 = region->cell_polygon(static_cast<uint32_t>(p_cell_id));
    godot::PackedVector3Array out;
    out.resize(static_cast<int64_t>(poly2.size()));
    for (size_t i = 0; i < poly2.size(); ++i) {
        out.set(
            static_cast<int64_t>(i),
            project_point_2d(godot::Vector2(static_cast<float>(poly2[i].x), static_cast<float>(poly2[i].y)))
        );
    }
    return out;
}

int TileRegion2DProjector3D::find_closest_projected_cell_3d(
    const godot::Vector3 &p_local_point_3d,
    real_t p_max_distance
) const {
    const CoreRegion *region = _resolve_active_region();
    if (region == nullptr || region->cell_count() == 0) {
        return -1;
    }
    const real_t max_d2 = p_max_distance * p_max_distance;
    real_t best_d2      = max_d2;
    int best_cid        = -1;

    for (size_t cid = 0; cid < region->cell_count(); ++cid) {
        const auto &c = region->cells()[cid];
        const godot::Vector3 pos3 = project_point_2d(godot::Vector2(c.center_x, c.center_y));
        const real_t d2 = pos3.distance_squared_to(p_local_point_3d);
        if (d2 <= best_d2) {
            best_d2  = d2;
            best_cid = static_cast<int>(cid);
        }
    }
    return best_cid;
}

godot::PackedVector3Array TileRegion2DProjector3D::find_projected_path_points_3d(
    int p_start_cell,
    int p_goal_cell
) const {
    const CoreRegion *region = _resolve_active_region();
    if (region == nullptr || p_start_cell < 0 || p_goal_cell < 0 ||
        static_cast<size_t>(p_start_cell) >= region->cell_count() ||
        static_cast<size_t>(p_goal_cell) >= region->cell_count()) {
        return {};
    }
    const auto res = CoreNav::find_path_cells(
        *region,
        static_cast<uint32_t>(p_start_cell),
        static_cast<uint32_t>(p_goal_cell),
        false
    );
    if (!res.reachable || res.world_points.empty()) {
        return {};
    }
    godot::PackedVector2Array pts_2d;
    pts_2d.resize(static_cast<int64_t>(res.world_points.size()));
    for (size_t i = 0; i < res.world_points.size(); ++i) {
        pts_2d.set(
            static_cast<int64_t>(i),
            godot::Vector2(static_cast<float>(res.world_points[i].x), static_cast<float>(res.world_points[i].y))
        );
    }
    return project_polyline_2d(pts_2d, static_cast<real_t>(0.35));
}

godot::Ref<Spline3DResource> TileRegion2DProjector3D::bake_projected_path_spline_3d(
    int p_start_cell,
    int p_goal_cell
) const {
    godot::Ref<Spline3DResource> spline;
    spline.instantiate();

    const CoreRegion *region = _resolve_active_region();
    if (region == nullptr || p_start_cell < 0 || p_goal_cell < 0 ||
        static_cast<size_t>(p_start_cell) >= region->cell_count() ||
        static_cast<size_t>(p_goal_cell) >= region->cell_count()) {
        return spline;
    }

    const auto res = CoreNav::find_path_cells(
        *region,
        static_cast<uint32_t>(p_start_cell),
        static_cast<uint32_t>(p_goal_cell),
        false
    );
    if (!res.reachable || res.world_points.empty()) {
        return spline;
    }

    for (size_t i = 0; i < res.world_points.size(); ++i) {
        const godot::Vector2 uv(
            static_cast<float>(res.world_points[i].x),
            static_cast<float>(res.world_points[i].y)
        );
        godot::Vector3 pos_3d;
        godot::Vector3 nrm_3d;
        project_point_2d_with_normal(uv, pos_3d, nrm_3d);
        const int idx = spline->add_point(pos_3d);
        spline->set_point_up_vector(idx, nrm_3d);
    }
    return spline;
}

} // namespace ideam::godot_ext