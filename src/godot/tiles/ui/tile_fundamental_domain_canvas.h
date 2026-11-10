#pragma once

/**
 * tile_fundamental_domain_canvas.h
 * Interactive 2D Crystallographic & Isohedral Prototile Canvas (`src/godot/tiles/ui/`),
 * extending `GeometryCanvas2DBase` (`src/godot/geometry/ui/`).
 *
 * Responsibilities:
 *   1. Renders the active `WallpaperTilingResource` prototile outline, ear-clipped
 *      triangulation, and a configurable `(2R+1) x (2R+1)` surrounding lattice
 *      neighborhood tinted by crystallographic 3-coloring (`get_cell_colour`).
 *   2. Visualizes translation basis vectors (`T1`, `T2`), unit-cell parallelogram,
 *      reflection axes (`axes_of_symmetry`), rotational pivots (`points_of_rotation`),
 *      glide-reflection axes (`glides`), and per-edge Tactile `J / U / S / I` symmetry
 *      constraint badges.
 *   3. Provides interactive custom-grip dragging (`BASE_DRAG_CUSTOM`) for `T1`/`T2`
 *      lattice basis tips, lattice origin, rotational pivots, reflection axes, and
 *      Tactile vertex parameters with single-step `IdeamUndoRedo` snapshot commits.
 */

#include "../../geometry/ui/geometry_canvas_2d_base.h"
#include "../wallpaper_tiling_resource.h"

#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

class TileFundamentalDomainCanvas : public GeometryCanvas2DBase {
    GDCLASS(TileFundamentalDomainCanvas, GeometryCanvas2DBase)

private:
    enum CustomGripKind : int32_t {
        GRIP_NONE           = 0,
        GRIP_LATTICE_ORIGIN = 1,
        GRIP_BASIS_T1       = 2,
        GRIP_BASIS_T2       = 3,
        GRIP_ROTATION_PIVOT = 4,
        GRIP_SYMMETRY_AXIS  = 5,
        GRIP_GLIDE_AXIS     = 6
    };

    bool show_lattice_neighborhood_ = true;
    bool show_three_coloring_       = true;
    bool show_lattice_vectors_      = true;
    bool show_symmetry_elements_    = true;
    bool show_edge_constraints_     = true;
    bool show_triangulation_        = false;
    int neighborhood_radius_        = 1;
    int edge_subdivisions_          = 12;
    int selected_edge_shape_id_     = 0;
    int hovered_edge_shape_id_      = -1;

    [[nodiscard]] static godot::Color _palette_color_for_index(int p_colour_idx, float p_alpha);
    [[nodiscard]] static godot::String _edge_shape_badge_text(TilingEdgeShapeType p_shape);
    [[nodiscard]] static godot::Color _edge_shape_badge_color(TilingEdgeShapeType p_shape);

protected:
    static void _bind_methods();

    // --- GeometryCanvas2DBase Overrides ---
    [[nodiscard]] int _get_canvas_point_count() const override;
    [[nodiscard]] int _get_canvas_section_count() const override;
    [[nodiscard]] bool _is_canvas_wrapped() const override { return true; }

    [[nodiscard]] godot::Vector2 _get_point_world_pos(int p_index) const override;
    void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) override;

    [[nodiscard]] godot::PackedVector2Array _sample_curve_screen_points(
        const godot::Rect2 &p_plot
    ) const override;
    [[nodiscard]] bool _compute_world_bounds(godot::Rect2 &r_bounds) const override;

    void _draw_plot_background_overlay(const godot::Rect2 &p_plot) override;
    void _draw_plot_foreground_overlay(const godot::Rect2 &p_plot) override;

    [[nodiscard]] bool _try_begin_custom_drag(
        const godot::Ref<godot::InputEvent> &p_event,
        const godot::Rect2 &p_plot
    ) override;
    [[nodiscard]] bool _update_custom_drag(
        const godot::Ref<godot::InputEvent> &p_event,
        const godot::Rect2 &p_plot
    ) override;
    void _update_custom_hover(
        const godot::Vector2 &p_mouse_pos,
        const godot::Rect2 &p_plot
    ) override;
    void _on_resource_bound() override;

public:
    TileFundamentalDomainCanvas();
    ~TileFundamentalDomainCanvas() override = default;

    // --- Typed Resource Binding ---
    void set_wallpaper_tiling(const godot::Ref<WallpaperTilingResource> &p_tiling);
    [[nodiscard]] godot::Ref<WallpaperTilingResource> get_wallpaper_tiling() const;

    // --- Visual Overlay & Neighborhood Controls ---
    void set_show_lattice_neighborhood(bool p_show);
    [[nodiscard]] bool get_show_lattice_neighborhood() const noexcept { return show_lattice_neighborhood_; }

    void set_show_three_coloring(bool p_show);
    [[nodiscard]] bool get_show_three_coloring() const noexcept { return show_three_coloring_; }

    void set_show_lattice_vectors(bool p_show);
    [[nodiscard]] bool get_show_lattice_vectors() const noexcept { return show_lattice_vectors_; }

    void set_show_symmetry_elements(bool p_show);
    [[nodiscard]] bool get_show_symmetry_elements() const noexcept { return show_symmetry_elements_; }

    void set_show_edge_constraints(bool p_show);
    [[nodiscard]] bool get_show_edge_constraints() const noexcept { return show_edge_constraints_; }

    void set_show_triangulation(bool p_show);
    [[nodiscard]] bool get_show_triangulation() const noexcept { return show_triangulation_; }

    void set_neighborhood_radius(int p_radius);
    [[nodiscard]] int get_neighborhood_radius() const noexcept { return neighborhood_radius_; }

    void set_edge_subdivisions(int p_subdivs);
    [[nodiscard]] int get_edge_subdivisions() const noexcept { return edge_subdivisions_; }

    void set_selected_edge_shape_id(int p_edge_shape_id);
    [[nodiscard]] int get_selected_edge_shape_id() const noexcept { return selected_edge_shape_id_; }
};

} // namespace ideam::godot_ext
