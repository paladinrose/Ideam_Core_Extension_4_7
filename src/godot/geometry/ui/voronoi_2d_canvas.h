#pragma once

/**
 * voronoi_2d_canvas.h
 * Interactive 2D Voronoi & Delaunay Dual Graph Canvas (`Voronoi2DCanvas`, Tier 2 Specific UI)
 * and Composite Authoring Control (`Voronoi2DControl`, Tier 3 UI Tool) in `src/godot/geometry/ui/`.
 *
 * Implements Section 3.2 of the Ideam Core Extension Master Plan:
 *   - Extends `GeometryCanvas2DBase` to interactively drag, add (double-click), and remove
 *     (right-click) Voronoi generator seeds with single-step `IdeamUndoRedo` snapshot commits.
 *   - Renders color-coded clipped Voronoi cells (`Cell Index Palette`, `Cell Area Heatmap`,
 *     `Neighbor Valence`, `Boundary Proximity`), Delaunay dual triangulation wireframe,
 *     CSR dual adjacency graph edges, Lloyd centroid displacement vectors, and optional
 *     `Polygon2DResource` clipping boundary contours.
 *   - `Voronoi2DControl` wraps `Voronoi2DCanvas` with procedural Grid/Poisson site generators,
 *     1-step / 5-step Lloyd centroidal relaxation, clip-bounds / `Polygon2DResource` clipping
 *     controls, cell-to-`Polygon2DResource` export, and CSR telemetry for `IdeamNarratives`
 *     (`Location` territory graphs) and `IdeamGames` (`TiledGameBoardArchitectSmartTool`).
 */

#include "geometry_canvas_2d_base.h"
#include "../resources/voronoi_diagram_2d_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

enum VoronoiCanvasColorMode : int32_t {
    VORONOI_COLOR_INDEX_PALETTE      = 0,
    VORONOI_COLOR_AREA_HEATMAP       = 1,
    VORONOI_COLOR_NEIGHBOR_VALENCE   = 2,
    VORONOI_COLOR_BOUNDARY_PROXIMITY = 3
};

class Voronoi2DCanvas : public GeometryCanvas2DBase {
    GDCLASS(Voronoi2DCanvas, GeometryCanvas2DBase)

private:
    VoronoiCanvasColorMode color_mode_ = VORONOI_COLOR_INDEX_PALETTE;
    bool show_voronoi_cells_           = true;
    bool show_delaunay_                = false;
    bool show_csr_dual_graph_          = true;
    bool show_centroids_               = true;
    bool show_clip_boundary_           = true;

    [[nodiscard]] godot::Color _compute_cell_fill_color(
        int p_site_idx,
        int p_site_count,
        real_t p_area,
        real_t p_max_area,
        int p_neighbor_count
    ) const;

protected:
    static void _bind_methods();

    [[nodiscard]] int _get_canvas_point_count() const override;
    [[nodiscard]] bool _has_non_point_content() const override;

    [[nodiscard]] godot::Vector2 _get_point_world_pos(int p_index) const override;
    void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) override;

    [[nodiscard]] godot::PackedVector2Array _sample_curve_screen_points(
        const godot::Rect2 &p_plot
    ) const override;
    [[nodiscard]] bool _compute_world_bounds(godot::Rect2 &r_bounds) const override;

    void _on_double_click_plot(
        const godot::Vector2 &p_world_pos,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) override;
    void _on_right_click_point(int p_index) override;

    void _draw_plot_background_overlay(const godot::Rect2 &p_plot) override;
    void _draw_plot_foreground_overlay(const godot::Rect2 &p_plot) override;

public:
    Voronoi2DCanvas();
    ~Voronoi2DCanvas() override = default;

    void set_voronoi_resource(const godot::Ref<VoronoiDiagram2DResource> &p_voronoi);
    [[nodiscard]] godot::Ref<VoronoiDiagram2DResource> get_voronoi_resource() const;

    void set_color_mode(VoronoiCanvasColorMode p_mode);
    [[nodiscard]] VoronoiCanvasColorMode get_color_mode() const noexcept { return color_mode_; }

    void set_show_voronoi_cells(bool p_show);
    [[nodiscard]] bool get_show_voronoi_cells() const noexcept { return show_voronoi_cells_; }

    void set_show_delaunay(bool p_show);
    [[nodiscard]] bool get_show_delaunay() const noexcept { return show_delaunay_; }

    void set_show_csr_dual_graph(bool p_show);
    [[nodiscard]] bool get_show_csr_dual_graph() const noexcept { return show_csr_dual_graph_; }

    void set_show_centroids(bool p_show);
    [[nodiscard]] bool get_show_centroids() const noexcept { return show_centroids_; }

    void set_show_clip_boundary(bool p_show);
    [[nodiscard]] bool get_show_clip_boundary() const noexcept { return show_clip_boundary_; }
};

/**
 * Voronoi2DControl (Tier 3 UI Tool)
 * Composite Editor & Runtime Control wrapping `Voronoi2DCanvas` with site generators,
 * Lloyd relaxation controls, `Polygon2DResource` clipping boundary presets, and CSR telemetry.
 */
class Voronoi2DControl : public godot::VBoxContainer {
    GDCLASS(Voronoi2DControl, godot::VBoxContainer)

private:
    godot::Ref<VoronoiDiagram2DResource> voronoi_resource_;
    bool is_updating_ui_ = false;

    // 1. Viewport & Overlay Toolbar
    godot::OptionButton *color_mode_opt_   = nullptr;
    godot::CheckBox *cells_check_          = nullptr;
    godot::CheckBox *delaunay_check_       = nullptr;
    godot::CheckBox *csr_dual_check_       = nullptr;
    godot::CheckBox *centroids_check_      = nullptr;
    godot::CheckBox *clip_boundary_check_  = nullptr;

    // 2. Embedded 2D Voronoi Canvas
    Voronoi2DCanvas *voronoi_canvas_ = nullptr;

    // 3. Procedural Site Generation & Lloyd Relaxation Controls
    godot::SpinBox *grid_cols_spin_      = nullptr;
    godot::SpinBox *grid_rows_spin_      = nullptr;
    godot::SpinBox *grid_jitter_spin_    = nullptr;
    godot::SpinBox *poisson_count_spin_  = nullptr;
    godot::SpinBox *poisson_dist_spin_   = nullptr;
    godot::SpinBox *lloyd_strength_spin_ = nullptr;

    // 4. Clip Bounds & Polygon2D Clipping Bridge Controls
    godot::SpinBox *bounds_x_spin_ = nullptr;
    godot::SpinBox *bounds_y_spin_ = nullptr;
    godot::SpinBox *bounds_w_spin_ = nullptr;
    godot::SpinBox *bounds_h_spin_ = nullptr;

    // 5. Selected Site & CSR Telemetry Footer
    godot::SpinBox *sel_site_x_spin_    = nullptr;
    godot::SpinBox *sel_site_y_spin_    = nullptr;
    godot::Label *telemetry_label_      = nullptr;
    godot::Label *cell_inspector_label_ = nullptr;

    void _build_ui();
    void _update_telemetry_labels();

    // --- Callbacks ---
    void _on_resource_changed();
    void _on_fit_view_pressed();
    void _on_color_mode_selected(int p_idx);
    void _on_cells_toggled(bool p_show);
    void _on_delaunay_toggled(bool p_show);
    void _on_csr_dual_toggled(bool p_show);
    void _on_centroids_toggled(bool p_show);
    void _on_clip_boundary_toggled(bool p_show);

    void _on_add_site_pressed();
    void _on_remove_selected_site_pressed();
    void _on_clear_sites_pressed();
    void _on_generate_grid_pressed();
    void _on_generate_poisson_pressed();
    void _on_lloyd_1_step_pressed();
    void _on_lloyd_5_steps_pressed();

    void _on_clip_bounds_edited(double p_unused);
    void _on_bind_octagon_clip_pressed();
    void _on_bind_circle_clip_pressed();
    void _on_clear_clip_polygon_pressed();

    void _on_canvas_point_selected(int p_site_index);
    void _on_selected_site_coords_edited(double p_unused);

protected:
    static void _bind_methods();

public:
    Voronoi2DControl();
    ~Voronoi2DControl() override;

    void set_voronoi_resource(const godot::Ref<VoronoiDiagram2DResource> &p_voronoi);
    [[nodiscard]] godot::Ref<VoronoiDiagram2DResource> get_voronoi_resource() const { return voronoi_resource_; }

    [[nodiscard]] Voronoi2DCanvas *get_voronoi_canvas() const noexcept { return voronoi_canvas_; }

    void refresh_ui();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::VoronoiCanvasColorMode);