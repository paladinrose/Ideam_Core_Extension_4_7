#pragma once

/**
 * spline_2d_control.h
 * 2D Spatial Curve & Polygon Visual Canvas (`Spline2DCanvas`, extending `SplineCanvasBase`)
 * and Composite Resource Editor (`Spline2DControl`, extending `SplineControlBase`).
 *
 * How `Spline2DCanvas` Specializes `SplineCanvasBase`:
 *   - Configures `SplineCanvasBase` with `POINT_STYLE_CIRCLE`, `DRAG_AXIS_FREE_2D`,
 *     `lock_aspect_ratio = true`, and full 2D pan/zoom (`allow_pan_x/y = true`,
 *     `allow_zoom_x/y = true`).
 *   - Projects each 2D anchor `i` and its 2D `back_handle` / `fore_handle` vectors
 *     directly into 2D world coordinates.
 *   - Renders 2D-specific overlays:
 *       1. **Ear-Clipped Closed Polygon Fill & Centroid Marker** (`show_polygon_fill`)
 *          powered by `Spline2DResource::triangulate()` and `get_centroid()`, with live
 *          Signed Area & Perimeter telemetry.
 *       2. **Directional Section Chevrons & Selected Section Highlight** showing traversal
 *          direction and polygon winding order (CW vs. CCW).
 *       3. **Normal Comb Vectors** (`show_normals`) sampled along the curve.
 *   - Double-clicking near an existing curve section splits that section at the clicked
 *     point; double-clicking in open space appends a new 2D point to the path.
 */

#include "../ui/spline_canvas_base.h"
#include "../ui/spline_control_base.h"
#include "spline_2d_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ideam::godot_ext {

/**
 * Standalone 2D curve & closed-polygon visual canvas built on `SplineCanvasBase`.
 */
class Spline2DCanvas : public SplineCanvasBase {
    GDCLASS(Spline2DCanvas, SplineCanvasBase)

private:
    bool show_polygon_fill = true;
    bool show_normals      = false;
    bool show_direction    = true;
    float normal_scale     = 18.0f;

protected:
    static void _bind_methods();

    // --- SplineCanvasBase Projection & Customization Overrides ---
    [[nodiscard]] godot::Vector2 _get_point_world_pos(int p_index) const override;
    void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) override;

    [[nodiscard]] godot::Vector2 _get_back_handle_screen_pos(
        int p_index,
        const godot::Rect2 &p_plot
    ) const override;
    void _set_back_handle_from_screen_pos(
        int p_index,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) override;

    [[nodiscard]] godot::Vector2 _get_fore_handle_screen_pos(
        int p_index,
        const godot::Rect2 &p_plot
    ) const override;
    void _set_fore_handle_from_screen_pos(
        int p_index,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) override;

    [[nodiscard]] godot::PackedVector2Array _sample_curve_screen_points(
        const godot::Rect2 &p_plot
    ) const override;
    [[nodiscard]] bool _compute_world_bounds(godot::Rect2 &r_bounds) const override;

    void _on_double_click_plot(
        const godot::Vector2 &p_world_pos,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) override;

    void _draw_plot_background_overlay(const godot::Rect2 &p_plot) override;
    void _draw_plot_foreground_overlay(const godot::Rect2 &p_plot) override;

public:
    Spline2DCanvas();
    ~Spline2DCanvas() override;

    // --- Typed 2D Resource Binding ---
    void set_spline(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_spline() const;

    // --- 2D Overlay Toggles ---
    void set_show_polygon_fill(bool p_show);
    [[nodiscard]] bool get_show_polygon_fill() const noexcept { return show_polygon_fill; }

    void set_show_normals(bool p_show);
    [[nodiscard]] bool get_show_normals() const noexcept { return show_normals; }

    void set_show_direction(bool p_show);
    [[nodiscard]] bool get_show_direction() const noexcept { return show_direction; }
};

/**
 * Full Editor & Runtime composite UI control for `Spline2DResource`.
 * Combines 2D shape presets, `Spline2DCanvas`, and the alternating 2D Point/Section chain.
 */
class Spline2DControl : public SplineControlBase {
    GDCLASS(Spline2DControl, SplineControlBase)

private:
    Spline2DCanvas *canvas_2d           = nullptr;
    godot::SpinBox *polygon_sides_spin  = nullptr;
    godot::CheckBox *lock_aspect_check  = nullptr;
    godot::CheckBox *poly_fill_check    = nullptr;
    godot::CheckBox *normals_check      = nullptr;
    godot::CheckBox *show_canvas_check  = nullptr;

    void _build_2d_extensions();

    // 2D Preset & Toolbar Callbacks
    void _on_preset_rectangle();
    void _on_preset_regular_polygon();
    void _on_preset_star();
    void _on_preset_circle();
    void _on_rotate_45_pressed();
    void _on_fit_2d_pressed();
    void _on_lock_aspect_toggled(bool p_lock);
    void _on_poly_fill_toggled(bool p_fill);
    void _on_normals_toggled(bool p_show);
    void _on_show_canvas_toggled(bool p_show);
    void _on_canvas_point_selected(int p_point_index);
    void _on_canvas_section_selected(int p_section_index);

protected:
    static void _bind_methods();

    // --- SplineControlBase Overrides ---
    [[nodiscard]] int _get_expected_dimension() const override { return 2; }
    void _on_resource_rebound() override;
    void _on_ui_refreshed() override;
    void _on_selection_updated() override;

    void _populate_point_field(int p_index, SplinePointField *p_field) override;
    void _commit_point_position(int p_index, const godot::Variant &p_position) override;
    void _commit_point_back_handle(int p_index, const godot::Variant &p_handle, bool p_enabled) override;
    void _commit_point_fore_handle(int p_index, const godot::Variant &p_handle, bool p_enabled) override;
    void _commit_add_default_point() override;
    void _commit_split_section(int p_section_index) override;

public:
    Spline2DControl();
    ~Spline2DControl() override;

    void set_spline_2d(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_spline_2d() const;

    [[nodiscard]] Spline2DCanvas *get_canvas_2d() const noexcept { return canvas_2d; }
};

} // namespace ideam::godot_ext