#pragma once

/**
 * tile_region_2d_projector_3d.h
 * 2D-to-3D Tiling Region Topology Projector Node (`TileRegion2DProjector3D`, extends
 * `Geometry2DProjector3D`) in `src/godot/tiles/`.
 *
 * Parallel to `Spline2DProjector3D` in `IdeamSplines`, `TileRegion2DProjector3D` leverages
 * the shared `Geometry2DProjector3D` base class in `IdeamGeometry` to drape any 2D
 * `TilingResource` onto 3D topology:
 *   1. Supports all `TilingResource` types:
 *      - `CompositeTilingResource` (multi-layer stitched regions & `Hexize` super-cells)
 *      - `WallpaperTilingResource` (17 crystallographic groups / 81 isohedral types bounded
 *        by an optional `Spline2DResource` contour or `fallback_region_rect`)
 *      - `HyperbolicPoincareTilingResource` (projects `{p, q}` Poincaré disk tilings onto
 *        3D planes, cylinders, spheres, physics terrain, or target `MeshInstance3D` surfaces)
 *   2. Supports all 6 `Geometry3DProjectionMode` modes inherited from `Geometry2DProjector3D`
 *      (`PROJ_PLANE_XZ_GROUND`, `PROJ_PLANE_XY_WALL`, `PROJ_CYLINDRICAL_WRAP`,
 *      `PROJ_SPHERICAL_WRAP`, `PROJ_PHYSICS_RAYCAST_DRAPE`, `PROJ_TARGET_MESH_SURFACE`).
 *   3. Provides per-cell `tile_inset_ratio` bevels, normal prism extrusion (`extrude_thickness`),
 *      colour-index / bridge-quad / hex-super-cell palette coloring, 3D cell transform &
 *      picking queries (`get_projected_cell_transform_3d`, `find_closest_projected_cell_3d`),
 *      and 3D A* path-to-`Spline3DResource` baking (`bake_projected_path_spline_3d`) so
 *      3D tactical boards on uneven terrain seamlessly bridge into `IdeamSplines` and `IdeamGames`.
 */

#include "../geometry/geometry_2d_projector_3d.h"
#include "tiling_resource.h"
#include "wallpaper_tiling_resource.h"
#include "composite_tiling_resource.h"
#include "hyperbolic_poincare_tiling_resource.h"
#include "../splines/spline_2d/spline_2d_resource.h"
#include "../splines/spline_3d/spline_3d_resource.h"
#include "../../core/tiles/tiling_navigation_2d.h"
#include "../../core/tiles/tiling_region_2d.h"

#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class TileRegion2DProjector3D : public Geometry2DProjector3D {
    GDCLASS(TileRegion2DProjector3D, Geometry2DProjector3D)

private:
    using CoreRegion = core::TilingRegion2D<real_t>;
    using CoreSpline = core::Spline2D<real_t>;
    using CoreNav    = core::TilingNavigation2D<real_t>;
    using Vec2       = typename CoreRegion::Vec2;

    godot::Ref<TilingResource> tiling_resource_;
    godot::Ref<Spline2DResource> boundary_spline_2d_;
    godot::Rect2 fallback_region_rect_ = godot::Rect2(-5.0f, -5.0f, 10.0f, 10.0f);

    real_t tile_inset_ratio_           = static_cast<real_t>(0.05);
    int edge_subdivisions_             = 12;
    bool project_hex_super_cells_      = false;
    int highlighted_cell_id_           = -1;

    godot::Color color_0_              = godot::Color(0.22f, 0.48f, 0.76f, 0.92f);
    godot::Color color_1_              = godot::Color(0.28f, 0.68f, 0.58f, 0.92f);
    godot::Color color_2_              = godot::Color(0.84f, 0.62f, 0.26f, 0.92f);
    godot::Color bridge_quad_color_    = godot::Color(0.78f, 0.36f, 0.50f, 0.92f);
    godot::Color highlight_color_      = godot::Color(0.98f, 0.90f, 0.28f, 0.98f);

    godot::Ref<godot::StandardMaterial3D> default_vertex_color_material_;

    mutable CoreRegion synthesized_region_cache_;
    mutable bool synthesized_cache_dirty_ = true;

    void _disconnect_tiling_dependencies();
    void _connect_tiling_dependencies();
    void _on_tiling_dependency_changed();
    [[nodiscard]] const CoreRegion *_resolve_active_region() const;

protected:
    static void _bind_methods();
    void _rebuild_projected_geometry() override;

public:
    TileRegion2DProjector3D();
    ~TileRegion2DProjector3D() override;

    // ========================================================================
    // Tiling & Region Properties
    // ========================================================================

    void set_tiling_resource(const godot::Ref<TilingResource> &p_tiling);
    [[nodiscard]] godot::Ref<TilingResource> get_tiling_resource() const { return tiling_resource_; }

    void set_boundary_spline_2d(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_boundary_spline_2d() const { return boundary_spline_2d_; }

    void set_fallback_region_rect(const godot::Rect2 &p_rect);
    [[nodiscard]] godot::Rect2 get_fallback_region_rect() const { return fallback_region_rect_; }

    void set_tile_inset_ratio(real_t p_ratio);
    [[nodiscard]] real_t get_tile_inset_ratio() const { return tile_inset_ratio_; }

    void set_edge_subdivisions(int p_subdivs);
    [[nodiscard]] int get_edge_subdivisions() const { return edge_subdivisions_; }

    void set_project_hex_super_cells(bool p_hex);
    [[nodiscard]] bool get_project_hex_super_cells() const { return project_hex_super_cells_; }

    void set_highlighted_cell_id(int p_cell_id);
    [[nodiscard]] int get_highlighted_cell_id() const { return highlighted_cell_id_; }

    void set_color_0(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_0() const { return color_0_; }

    void set_color_1(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_1() const { return color_1_; }

    void set_color_2(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_2() const { return color_2_; }

    void set_bridge_quad_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_bridge_quad_color() const { return bridge_quad_color_; }

    void set_highlight_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_highlight_color() const { return highlight_color_; }

    // ========================================================================
    // 3D Projected Board Cell & Navigation Queries
    // ========================================================================

    [[nodiscard]] int get_projected_cell_count() const;
    [[nodiscard]] godot::Vector3 get_projected_cell_center_3d(int p_cell_id) const;
    [[nodiscard]] godot::Transform3D get_projected_cell_transform_3d(int p_cell_id) const;
    [[nodiscard]] godot::PackedVector3Array get_projected_cell_polygon_3d(int p_cell_id) const;

    /**
     * Finds the projected cell whose 3D draped center is closest to `p_local_point_3d`
     * within `p_max_distance` (returns `-1` if no cell is within range).
     */
    [[nodiscard]] int find_closest_projected_cell_3d(
        const godot::Vector3 &p_local_point_3d,
        real_t p_max_distance = static_cast<real_t>(10.0)
    ) const;

    /**
     * Computes the A* shortest path between `p_start_cell` and `p_goal_cell` on the 2D
     * tiling topology and projects the path waypoints onto the 3D surface.
     */
    [[nodiscard]] godot::PackedVector3Array find_projected_path_points_3d(
        int p_start_cell,
        int p_goal_cell
    ) const;

    /**
     * Bakes the projected 3D A* shortest path between `p_start_cell` and `p_goal_cell`
     * into a ready-to-use `Spline3DResource` with surface-aligned up-vectors.
     */
    [[nodiscard]] godot::Ref<Spline3DResource> bake_projected_path_spline_3d(
        int p_start_cell,
        int p_goal_cell
    ) const;
};

} // namespace ideam::godot_ext