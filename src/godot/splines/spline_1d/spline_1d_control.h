#pragma once

/**
 * spline_1d_control.h
 * 1D Graphic-Equalizer Visual Control (`Spline1DEqualizerCanvas`, extending `SplineCanvasBase`)
 * and Composite Resource Editor (`Spline1DControl`, extending `SplineControlBase`).
 *
 * How `Spline1DEqualizerCanvas` Specializes `SplineCanvasBase`:
 *   - Configures `SplineCanvasBase` with `POINT_STYLE_FADER_CAP`, `DRAG_AXIS_Y_ONLY`,
 *     and `view_min.x = 0.0, view_max.x = 1.0` (locking X pan/zoom while enabling Y pan/zoom).
 *   - Projects each 1D anchor `i` into 2D canvas space as:
 *       `Vector2(cumulative_section_weight_norm[i], point_value[i])`
 *   - Uses `SplineCanvasBase`'s overlay & custom-drag hooks to render and manipulate:
 *       1. The **Bottom Section Weight Equalizer Strip** (where horizontal column widths
 *          reflect and drag-edit `section_weight[i]`).
 *       2. The **Root-Solver Threshold Line** (`get_percents_at_value()`).
 */

#include "../ui/spline_canvas_base.h"
#include "../ui/spline_control_base.h"
#include "spline_1d_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace ideam::godot_ext {

/**
 * Graphic-equalizer style 1D visual curve and section-weight editor built on `SplineCanvasBase`.
 */
class Spline1DEqualizerCanvas : public SplineCanvasBase {
    GDCLASS(Spline1DEqualizerCanvas, SplineCanvasBase)

private:
    enum CustomDragSubmode : int32_t {
        SUBDRAG_NONE = 0,
        SUBDRAG_SECTION_WEIGHT = 1,
        SUBDRAG_THRESHOLD_LINE = 2
    };

    bool show_threshold_solver = false;
    real_t threshold_value     = static_cast<real_t>(0.5);
    bool redistribute_weights  = false;

    // Cached initial values when dragging a section weight divider
    real_t weight_drag_start_w0    = static_cast<real_t>(1.0);
    real_t weight_drag_start_w1    = static_cast<real_t>(1.0);
    real_t weight_drag_start_total = static_cast<real_t>(1.0);

    [[nodiscard]] godot::Rect2 _get_weight_strip_rect(const godot::Rect2 &p_plot) const;
    [[nodiscard]] std::vector<real_t> _compute_fader_norms() const;

protected:
    static void _bind_methods();

    // --- SplineCanvasBase Projection & Customization Overrides ---
    [[nodiscard]] godot::Vector2 _get_point_world_pos(int p_index) const override;
    void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) override;
    [[nodiscard]] bool _has_wrap_ghost_endpoint(godot::Vector2 &r_ghost_world_pos) const override;

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

public:
    Spline1DEqualizerCanvas();
    ~Spline1DEqualizerCanvas() override;

    // --- Typed 1D Resource Binding ---
    void set_spline(const godot::Ref<Spline1DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline1DResource> get_spline() const;

    // --- Y-Axis Range & Auto-Fit Convenience Helpers ---
    void set_view_range_y(real_t p_min_y, real_t p_max_y);
    [[nodiscard]] real_t get_view_min_y() const noexcept { return view_min.y; }
    [[nodiscard]] real_t get_view_max_y() const noexcept { return view_max.y; }
    void fit_y_range_to_spline(real_t p_padding_ratio = static_cast<real_t>(0.15));

    // --- 1D Threshold Root-Solver Line ---
    void set_show_threshold_solver(bool p_show);
    [[nodiscard]] bool get_show_threshold_solver() const noexcept { return show_threshold_solver; }

    void set_threshold_value(real_t p_value);
    [[nodiscard]] real_t get_threshold_value() const noexcept { return threshold_value; }

    // --- Section Weight Drag Mode ---
    void set_redistribute_weights(bool p_redistribute);
    [[nodiscard]] bool get_redistribute_weights() const noexcept { return redistribute_weights; }
};

/**
 * Full Editor & Runtime composite UI control for `Spline1DResource`.
 * Combines 1D presets, `Spline1DEqualizerCanvas`, and the alternating Point/Section chain.
 */
class Spline1DControl : public SplineControlBase {
    GDCLASS(Spline1DControl, SplineControlBase)

private:
    Spline1DEqualizerCanvas *equalizer_canvas = nullptr;
    godot::CheckBox *show_equalizer_check     = nullptr;
    godot::CheckBox *show_solver_check        = nullptr;
    godot::CheckBox *redistribute_w_check     = nullptr;
    godot::SpinBox *equidistant_count_spin    = nullptr;

    void _build_1d_extensions();

    // 1D Toolbar & Canvas Callbacks
    void _on_preset_zero_to_one();
    void _on_preset_one_to_zero();
    void _on_preset_equidistant();
    void _on_fit_y_pressed();
    void _on_show_equalizer_toggled(bool p_show);
    void _on_show_solver_toggled(bool p_show);
    void _on_redistribute_w_toggled(bool p_redistribute);
    void _on_canvas_point_selected(int p_point_index);
    void _on_canvas_section_selected(int p_section_index);

protected:
    static void _bind_methods();

    // --- SplineControlBase Overrides ---
    [[nodiscard]] int _get_expected_dimension() const override { return 1; }
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
    Spline1DControl();
    ~Spline1DControl() override;

    void set_spline_1d(const godot::Ref<Spline1DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline1DResource> get_spline_1d() const;

    [[nodiscard]] Spline1DEqualizerCanvas *get_equalizer_canvas() const noexcept { return equalizer_canvas; }
};

} // namespace ideam::godot_ext