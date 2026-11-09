#pragma once

/**
 * composite_tiling_resource.h
 * Multi-layer bounded tiling resource wrapping `ideam::core::TilingRegion2D<real_t>`,
 * `ideam::core::TilingSeamStitcher2D<real_t>`, and `ideam::core::TilingNavigation2D<real_t>`
 * (`src/godot/tiles/`).
 *
 * Supports:
 *   1. Filling single or multiple `Spline2DResource` boundary contours (or a fallback `Rect2`)
 *      with distinct `WallpaperTilingResource` layers.
 *   2. Priority-ordered `PopulateGroup` / `MergeGroup` seam stitching (`STITCH_QUAD_BRIDGE`,
 *      `STITCH_TOPOLOGICAL_SEAM_WELD`, and `STITCH_CURVILINEAR_METRIC_MORPH`).
 *   3. `Hexize` macro-hexagon aggregation (`HexSuperCellPOD`), `FindBorders` closed `Spline2D`
 *      outline extraction, O(N) cell removal, and directional edge ports.
 *   4. Dual-level A* pathfinding (fundamental cells & hex super-cells) and multi-goal
 *      Dijkstra/Eikonal wavefront flow-fields.
 */

#include "tiling_resource.h"
#include "wallpaper_tiling_resource.h"
#include "../splines/spline_2d/spline_2d_resource.h"
#include "../../core/tiles/tiling_region_2d.h"
#include "../../core/tiles/tiling_seam_stitcher_2d.h"
#include "../../core/tiles/tiling_navigation_2d.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class CompositeTilingResource : public TilingResource {
    GDCLASS(CompositeTilingResource, TilingResource)

private:
    using CoreRegion   = core::TilingRegion2D<real_t>;
    using CoreStitcher = core::TilingSeamStitcher2D<real_t>;
    using CoreNav      = core::TilingNavigation2D<real_t>;
    using CoreSpline   = core::Spline2D<real_t>;
    using Vec2         = typename CoreRegion::Vec2;

    godot::TypedArray<WallpaperTilingResource> tiling_layers_;
    godot::TypedArray<Spline2DResource> region_splines_;
    godot::PackedInt32Array layer_priorities_;
    godot::PackedVector2Array layer_seed_points_;

    TilingCoverageModeType coverage_mode_ = COVERAGE_INCLUDE_STRADDLING;
    TilingStitchModeType stitch_mode_     = STITCH_QUAD_BRIDGE;
    real_t max_bridge_span_               = static_cast<real_t>(0.0);
    godot::Rect2 fallback_aabb_           = godot::Rect2(-5.0f, -5.0f, 10.0f, 10.0f);
    bool auto_hexize_                     = true;
    int edge_subdivisions_                = 12;
    bool auto_rebuild_                    = true;

    mutable CoreRegion core_region_;
    mutable bool is_dirty_                = true;

    void _disconnect_dependencies();
    void _connect_dependencies();
    void _on_dependency_changed();
    void _ensure_built() const;

    [[nodiscard]] static inline Vec2 _to_glm(const godot::Vector2 &p_v) noexcept {
        return Vec2(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y));
    }
    [[nodiscard]] static inline godot::Vector2 _to_godot(const Vec2 &p_v) noexcept {
        return godot::Vector2(static_cast<float>(p_v.x), static_cast<float>(p_v.y));
    }
    [[nodiscard]] static inline godot::Vector2 _to_godot_f(const glm::vec2 &p_v) noexcept {
        return godot::Vector2(p_v.x, p_v.y);
    }

protected:
    static void _bind_methods();

public:
    CompositeTilingResource();
    ~CompositeTilingResource() override;

    // --- Direct C++ Core Accessors (For `TilingRegionNode2D`, `TilingSurfaceNode3D` & `TileDOD`) ---
    [[nodiscard]] const CoreRegion &get_core_region() const {
        _ensure_built();
        return core_region_;
    }
    [[nodiscard]] CoreRegion &get_core_region_mut() {
        _ensure_built();
        return core_region_;
    }

    // ========================================================================
    // TilingResource Virtual Overrides
    // ========================================================================

    [[nodiscard]] int get_vertex_count() const override;
    [[nodiscard]] int get_aspect_count() const override;
    [[nodiscard]] int get_edge_shape_count() const override;

    [[nodiscard]] godot::PackedVector2Array get_prototile_outline(int p_subdivisions = 12) const override;
    [[nodiscard]] godot::PackedInt32Array get_prototile_triangles(int p_subdivisions = 12) const override;

    [[nodiscard]] godot::Dictionary get_snapshot_state() const override;
    void apply_snapshot_state(const godot::Dictionary &p_state) override;

    // ========================================================================
    // Layer, Boundary Spline & Seam Stitching Configuration
    // ========================================================================

    void set_tiling_layers(const godot::TypedArray<WallpaperTilingResource> &p_layers);
    [[nodiscard]] godot::TypedArray<WallpaperTilingResource> get_tiling_layers() const { return tiling_layers_; }

    void set_region_splines(const godot::TypedArray<Spline2DResource> &p_splines);
    [[nodiscard]] godot::TypedArray<Spline2DResource> get_region_splines() const { return region_splines_; }

    void set_layer_priorities(const godot::PackedInt32Array &p_priorities);
    [[nodiscard]] godot::PackedInt32Array get_layer_priorities() const { return layer_priorities_; }

    void set_layer_seed_points(const godot::PackedVector2Array &p_seeds);
    [[nodiscard]] godot::PackedVector2Array get_layer_seed_points() const { return layer_seed_points_; }

    void set_coverage_mode(TilingCoverageModeType p_mode);
    [[nodiscard]] TilingCoverageModeType get_coverage_mode() const { return coverage_mode_; }

    void set_stitch_mode(TilingStitchModeType p_mode);
    [[nodiscard]] TilingStitchModeType get_stitch_mode() const { return stitch_mode_; }

    void set_max_bridge_span(real_t p_span);
    [[nodiscard]] real_t get_max_bridge_span() const { return max_bridge_span_; }

    void set_fallback_aabb(const godot::Rect2 &p_rect);
    [[nodiscard]] godot::Rect2 get_fallback_aabb() const { return fallback_aabb_; }

    void set_auto_hexize(bool p_hexize);
    [[nodiscard]] bool get_auto_hexize() const { return auto_hexize_; }

    void set_edge_subdivisions(int p_subdivs);
    [[nodiscard]] int get_edge_subdivisions() const { return edge_subdivisions_; }

    void set_auto_rebuild(bool p_auto);
    [[nodiscard]] bool get_auto_rebuild() const { return auto_rebuild_; }

    // ========================================================================
    // Rebuild & Topological Counts / Mesh Buffers
    // ========================================================================

    void rebuild();

    [[nodiscard]] int get_cell_count() const;
    [[nodiscard]] int get_edge_count() const;
    [[nodiscard]] int get_hex_super_cell_count() const;
    [[nodiscard]] int get_bridge_quad_count() const;

    [[nodiscard]] godot::PackedVector2Array get_shared_vertices() const;
    [[nodiscard]] godot::PackedInt32Array get_triangle_indices() const;
    [[nodiscard]] godot::Rect2 get_extents_rect() const;

    // ========================================================================
    // Per-Cell Queries (`IsoCellPOD` & `TileTransformPOD`)
    // ========================================================================

    [[nodiscard]] godot::PackedVector2Array get_cell_polygon(int p_cell_id) const;
    [[nodiscard]] godot::Vector2 get_cell_center(int p_cell_id) const;
    [[nodiscard]] godot::Transform2D get_cell_transform_2d(int p_cell_id) const;
    [[nodiscard]] int get_cell_colour(int p_cell_id) const;
    [[nodiscard]] int get_cell_generation(int p_cell_id) const;
    [[nodiscard]] int get_cell_flags(int p_cell_id) const;
    [[nodiscard]] int get_cell_group_slot(int p_cell_id) const;
    [[nodiscard]] godot::PackedInt32Array get_cell_neighbors(int p_cell_id) const;
    [[nodiscard]] godot::PackedInt32Array get_cells_by_generation(int p_generation = -1) const;

    // ========================================================================
    // Border Queries (`FindBorders` -> `Spline2DResource`)
    // ========================================================================

    [[nodiscard]] godot::PackedInt32Array get_border_cell_ids() const;
    [[nodiscard]] godot::PackedInt32Array get_border_edge_ids() const;
    [[nodiscard]] godot::PackedInt32Array get_border_vertex_ids() const;
    [[nodiscard]] godot::TypedArray<Spline2DResource> get_border_outline_splines() const;

    // ========================================================================
    // Hex Super-Cell (`Hexize`) Queries
    // ========================================================================

    int build_hex_super_cells();
    [[nodiscard]] godot::PackedVector2Array get_hex_center_points() const;
    [[nodiscard]] godot::PackedVector2Array get_hex_perimeter_polygon(int p_hex_id) const;
    [[nodiscard]] godot::PackedInt32Array get_hex_child_cells(int p_hex_id) const;
    [[nodiscard]] godot::PackedInt32Array get_hex_neighbors(int p_hex_id) const;

    // ========================================================================
    // Topological Mutations & Undo/Redo Counterparts
    // ========================================================================

    void remove_cells(const godot::PackedInt32Array &p_cell_ids);
    void commit_remove_cells(const godot::PackedInt32Array &p_cell_ids);

    void set_edge_port_direction(int p_edge_id, TilingPortDirectionType p_direction);
    void commit_set_edge_port_direction(int p_edge_id, TilingPortDirectionType p_direction);
    [[nodiscard]] TilingPortDirectionType get_edge_port_direction(int p_edge_id) const;

    void set_edge_cross_distance(int p_edge_id, real_t p_cost);
    [[nodiscard]] real_t get_edge_cross_distance(int p_edge_id) const;

    // ========================================================================
    // Spatial Containment & Navigation Queries (`TilingNavigation2D`)
    // ========================================================================

    [[nodiscard]] int contains_point(const godot::Vector2 &p_point) const;
    [[nodiscard]] int hex_contains_point(const godot::Vector2 &p_point) const;

    [[nodiscard]] godot::PackedInt32Array find_path_cells(
        int p_start_cell,
        int p_goal_cell,
        bool p_manhattan_heuristic = false
    ) const;
    [[nodiscard]] godot::PackedVector2Array find_path_world_points(
        const godot::Vector2 &p_start,
        const godot::Vector2 &p_goal,
        bool p_manhattan_heuristic = false
    ) const;
    [[nodiscard]] godot::PackedInt32Array find_path_hex_super_cells(
        int p_start_hex,
        int p_goal_hex,
        bool p_manhattan_heuristic = false
    ) const;
    [[nodiscard]] godot::PackedVector2Array compute_flow_field_vectors(
        const godot::PackedInt32Array &p_goal_cells
    ) const;
};

} // namespace ideam::godot_ext