#pragma once

/**
 * color_spline_control.h
 * Interactive 4D HSVA / RGBA Catmull-Rom Color Gradient Editor (`ColorSplineWheelCanvas`
 * and `ColorSplineControl`, `src/godot/splines/spline_4d/`).
 *
 * Architecture & Capabilities (Master Plan Section 3.1):
 *   1. Backed by `core::Spline4D<real_t>::create_gradient()` and `Spline4DResource::sample_color()`.
 *   2. `ColorSplineWheelCanvas` combines three synchronized interactive regions in a single
 *      hardware-accelerated custom `Control`:
 *        - **Top Gradient Preview Bar**: 256-sample continuous RGBA/HSVA gradient strip over a
 *          checkerboard alpha backdrop with draggable stop markers (drag horizontally to re-weight
 *          sections, double-click to insert a stop, right-click to remove a stop).
 *        - **2D Hue-Saturation Color Wheel**: Polar `(Hue = angle [0..360°], Saturation = radius [0..1])`
 *          wheel with the evaluated 4D Catmull-Rom / Bezier spline trajectory and draggable stop handles.
 *        - **Value (Z) & Alpha (W) Vertical Tracks**: Dual vertical fader bars for sculpting the
 *          selected stop's Value/Brightness (`Z`) and Alpha/Opacity (`W`) channels.
 *   3. `ColorSplineControl` wraps `ColorSplineWheelCanvas` with preset generators (`Sunset`, `Spectrum`,
 *      `Cyber Neon`), a `ColorPickerButton` and numeric `H/S/V/A` spinboxes for the active stop, and
 *      full `IdeamUndoRedo` integration.
 */

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/color_picker_button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <vector>

#include "spline_4d_resource.h"

namespace ideam::godot_ext {

class ColorSplineWheelCanvas : public godot::Control {
    GDCLASS(ColorSplineWheelCanvas, godot::Control)

public:
    enum DragTarget : int32_t {
        DRAG_NONE          = 0,
        DRAG_GRADIENT_STOP = 1,
        DRAG_WHEEL_STOP    = 2,
        DRAG_VALUE_TRACK   = 3,
        DRAG_ALPHA_TRACK   = 4
    };

private:
    godot::Ref<Spline4DResource> spline;
    int selected_stop_index = 0;
    int hovered_stop_index  = -1;
    bool hsva_mode          = true;

    DragTarget drag_target  = DRAG_NONE;
    godot::Dictionary drag_start_snapshot;

    // Layout sub-rectangles computed from control size
    [[nodiscard]] godot::Rect2 _get_gradient_bar_rect() const;
    [[nodiscard]] godot::Rect2 _get_wheel_rect() const;
    [[nodiscard]] godot::Rect2 _get_value_track_rect() const;
    [[nodiscard]] godot::Rect2 _get_alpha_track_rect() const;

    [[nodiscard]] std::vector<real_t> _compute_stop_norms() const;
    [[nodiscard]] godot::Vector2 _stop_to_wheel_pos(int p_index, const godot::Vector2 &p_center, float p_radius) const;
    [[nodiscard]] godot::Color _evaluate_stop_display_color(int p_index) const;

    void _set_stop_from_wheel_pos(int p_index, const godot::Vector2 &p_pos, const godot::Vector2 &p_center, float p_radius);
    void _set_stop_norm_weight(int p_index, float p_target_norm);
    void _set_stop_value_channel(int p_index, float p_val);
    void _set_stop_alpha_channel(int p_index, float p_alpha);

    void _on_spline_changed();

protected:
    static void _bind_methods();

public:
    ColorSplineWheelCanvas();
    ~ColorSplineWheelCanvas() override;

    void _draw() override;
    void _gui_input(const godot::Ref<godot::InputEvent> &p_event) override;

    void set_spline(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_spline() const { return spline; }

    void set_selected_stop_index(int p_index);
    [[nodiscard]] int get_selected_stop_index() const noexcept { return selected_stop_index; }

    void set_hsva_mode(bool p_hsva);
    [[nodiscard]] bool get_hsva_mode() const noexcept { return hsva_mode; }
};

class ColorSplineControl : public godot::VBoxContainer {
    GDCLASS(ColorSplineControl, godot::VBoxContainer)

private:
    godot::Ref<Spline4DResource> spline;

    ColorSplineWheelCanvas *wheel_canvas          = nullptr;
    godot::CheckBox *hsva_mode_check              = nullptr;
    godot::CheckBox *wrap_check                   = nullptr;
    godot::Label *selected_stop_label             = nullptr;
    godot::ColorPickerButton *stop_color_picker   = nullptr;
    godot::SpinBox *channel_x_spin                = nullptr;
    godot::SpinBox *channel_y_spin                = nullptr;
    godot::SpinBox *channel_z_spin                = nullptr;
    godot::SpinBox *channel_w_spin                = nullptr;
    godot::Button *remove_stop_btn                = nullptr;

    bool updating_ui = false;

    void _build_ui();
    void _refresh_from_resource();

    void _on_spline_changed();
    void _on_canvas_stop_selected(int p_stop_index);
    void _on_hsva_toggled(bool p_hsva);
    void _on_wrap_toggled(bool p_wrap);
    void _on_stop_color_picked(const godot::Color &p_color);
    void _on_channel_spin_changed(double p_val);
    void _on_add_stop_pressed();
    void _on_remove_stop_pressed();

    void _on_preset_sunset();
    void _on_preset_spectrum();
    void _on_preset_cyber_neon();

protected:
    static void _bind_methods();

public:
    ColorSplineControl();
    ~ColorSplineControl() override;

    void set_spline_4d(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_spline_4d() const { return spline; }

    [[nodiscard]] ColorSplineWheelCanvas *get_wheel_canvas() const noexcept { return wheel_canvas; }

    void apply_color_stops_via_create_gradient(const godot::PackedColorArray &p_colors);
};

} // namespace ideam::godot_ext