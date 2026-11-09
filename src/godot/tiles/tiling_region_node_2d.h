#pragma once

/**
 * tiling_region_node_2d.h
 * 2D Scene Node that renders, queries, and synchronizes a `TilingResource`
 * (either a single `WallpaperTilingResource` bounded by a `Spline2DResource`/`Rect2`
 * or a multi-layer `CompositeTilingResource`) (`src/godot/tiles/`).
 */

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "tiling_resource.h"
#include "wallpaper_tiling_resource.h"
#include "composite_tiling_resource.h"
#include "../splines/spline_2d/spline_2d_resource.h"
#include "../../core/tiles/tiling_region_2d.h"

namespace ideam::godot_ext {

class TilingRegionNode2D : public godot::Node2D {
    GDCLASS(TilingRegionNode2D, godot::Node2D)

private:
    using CoreRegion = core::TilingRegion2D<real_t>;
    using CoreSpline = core::Spline2D<real_t>;
    using Vec2       = typename CoreRegion::Vec2;

    godot::Ref<TilingResource> tiling_;
    godot::Ref<Spline2DResource> boundary_spline_;
    godot::NodePath boundary_polygon_node_path_;
    godot::Rect2 fallback_region_rect_ = godot::Rect2(-5.0f, -5.0f, 10.0f, 10.0f);
    TilingCoverageModeType coverage_mode_ = COVERAGE_INCLUDE_STRADDLING;
    int edge_subdivisions_ = 12;

    // Visual styling
    bool draw_cells_             = true;
    bool draw_cell_borders_      = true;
    bool draw_hex_super_cells_   = false;
    bool draw_region_outlines_   = false;
    bool draw_symmetry_elements_ = false;

    godot::Color color_0_           = godot::Color(0.22f, 0.45f, 0.72f, 0.85f);
    godot::Color color_1_           = godot::Color(0.32f, 0.65f, 0.58f, 0.85f);
    godot::Color color_2_           = godot::Color(0.82f, 0.62f, 0.28f, 0.85f);
    godot::Color bridge_quad_color_ = godot::Color(0.76f, 0.35f, 0.48f, 0.88f);
    godot::Color border_line_color_ = godot::Color(0.10f, 0.12f, 0.16f, 0.95f);
    godot::Color hex_line_color_    = godot::Color(0.95f, 0.80f, 0.20f, 0.95f);
    godot::Color outline_color_     = godot::Color(1.0f, 1.0f, 1.0f, 1.0f);

    real_t border_line_width_  = static_cast<real_t>(1.25);
    real_t hex_line_width_     = static_cast<real_t>(2.25);
    real_t outline_line_width_ = static_cast<real_t>(2.5);

    // Optional CollisionPolygon2D synchronization
    bool auto_sync_collision_ = false;
    godot::NodePath collision_polygon_path_;

    // Local region cache when bound directly to a WallpaperTilingResource
    mutable CoreRegion local_region_cache_;
    mutable bool local_cache_dirty_ = true;

    void _on_tiling_changed();
    void _on_boundary_changed();
    [[nodiscard]] const CoreRegion *_resolve_active_region() const;
    void _sync_collision_polygons();

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
    TilingRegionNode2D();
    ~TilingRegionNode2D() override;

    void _notification(int p_what);
    void _draw() override;

    // --- Resource & Boundary Properties ---
    void set_tiling(const godot::Ref<TilingResource> &p_tiling);
    [[nodiscard]] godot::Ref<TilingResource> get_tiling() const { return tiling_; }

    void set_boundary_spline(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_boundary_spline() const { return boundary_spline_; }

    void set_boundary_polygon_node_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_boundary_polygon_node_path() const { return boundary_polygon_node_path_; }

    void set_fallback_region_rect(const godot::Rect2 &p_rect);
    [[nodiscard]] godot::Rect2 get_fallback_region_rect() const { return fallback_region_rect_; }

    void set_coverage_mode(TilingCoverageModeType p_mode);
    [[nodiscard]] TilingCoverageModeType get_coverage_mode() const { return coverage_mode_; }

    void set_edge_subdivisions(int p_subdivs);
    [[nodiscard]] int get_edge_subdivisions() const { return edge_subdivisions_; }

    // --- Rendering Properties ---
    void set_draw_cells(bool p_draw);
    [[nodiscard]] bool get_draw_cells() const { return draw_cells_; }

    void set_draw_cell_borders(bool p_draw);
    [[nodiscard]] bool get_draw_cell_borders() const { return draw_cell_borders_; }

    void set_draw_hex_super_cells(bool p_draw);
    [[nodiscard]] bool get_draw_hex_super_cells() const { return draw_hex_super_cells_; }

    void set_draw_region_outlines(bool p_draw);
    [[nodiscard]] bool get_draw_region_outlines() const { return draw_region_outlines_; }

    void set_draw_symmetry_elements(bool p_draw);
    [[nodiscard]] bool get_draw_symmetry_elements() const { return draw_symmetry_elements_; }

    void set_color_0(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_0() const { return color_0_; }

    void set_color_1(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_1() const { return color_1_; }

    void set_color_2(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_2() const { return color_2_; }

    void set_bridge_quad_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_bridge_quad_color() const { return bridge_quad_color_; }

    void set_border_line_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_border_line_color() const { return border_line_color_; }

    void set_hex_line_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_hex_line_color() const { return hex_line_color_; }

    void set_outline_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_outline_color() const { return outline_color_; }

    void set_border_line_width(real_t p_width);
    [[nodiscard]] real_t get_border_line_width() const { return border_line_width_; }

    void set_hex_line_width(real_t p_width);
    [[nodiscard]] real_t get_hex_line_width() const { return hex_line_width_; }

    void set_outline_line_width(real_t p_width);
    [[nodiscard]] real_t get_outline_line_width() const { return outline_line_width_; }

    void set_auto_sync_collision(bool p_sync);
    [[nodiscard]] bool get_auto_sync_collision() const { return auto_sync_collision_; }

    void set_collision_polygon_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_collision_polygon_path() const { return collision_polygon_path_; }

    // --- Spatial & Pathfinding Helpers ---
    void rebuild_region();
    [[nodiscard]] int get_cell_count() const;
    [[nodiscard]] int get_cell_at_local_point(const godot::Vector2 &p_local_point) const;
    [[nodiscard]] int get_cell_at_global_point(const godot::Vector2 &p_global_point) const;
    [[nodiscard]] int get_hex_at_local_point(const godot::Vector2 &p_local_point) const;
    [[nodiscard]] int get_hex_at_global_point(const godot::Vector2 &p_global_point) const;
    [[nodiscard]] godot::PackedVector2Array find_local_path(
        const godot::Vector2 &p_local_start,
        const godot::Vector2 &p_local_goal
    ) const;
    [[nodiscard]] godot::PackedVector2Array find_global_path(
        const godot::Vector2 &p_global_start,
        const godot::Vector2 &p_global_goal
    ) const;
};

} // namespace ideam::godot_ext