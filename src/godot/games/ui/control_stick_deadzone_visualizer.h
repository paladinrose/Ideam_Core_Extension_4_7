#pragma once

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../gameplay/gameplay_control_map.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: ControlStickDeadzoneVisualizer renders a hardware-agnostic 2D telemetry
 * and response-curve plot for `GameplayControlMap` using immediate-mode `CanvasItem`
 * primitives (`_notification(NOTIFICATION_DRAW)`), avoiding child `Control` node bloat.
 *
 * It visualizes:
 * 1. Left Stick Gate: Outer unit circle, `left_stick_deadzone_outer` ring, shaded
 *    `left_stick_deadzone_inner` zone (radial circle or axial crossband), raw input
 *    deflection dot, and filtered output deflection vector.
 * 2. Deadzone & Response Curve Plot: Maps input magnitude [0..1] -> output strength [0..1]
 *    across `DEADZONE_RADIAL_SCALED`, `DEADZONE_AXIAL_INDEPENDENT`, and
 *    `DEADZONE_CUBIC_PRECISION` combined with `gamepad_response_exponent`.
 * 3. Right Stick Gate & L2/R2 Analog Trigger Bars: Displays `right_stick_deadzone_inner`/`outer`
 *    and vertical trigger meters with `trigger_deadzone` threshold markers.
 */
class ControlStickDeadzoneVisualizer : public godot::Control {
    GDCLASS(ControlStickDeadzoneVisualizer, godot::Control)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<GameplayControlMap> control_map;

    bool poll_live_hardware = true;
    int joypad_device_id = 0;
    bool show_response_curve = true;
    bool show_trigger_bars = true;
    bool show_labels = true;

    // Live or simulated raw and filtered input vectors
    godot::Vector2 raw_left_stick = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 filtered_left_stick = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 raw_right_stick = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 filtered_right_stick = godot::Vector2(0.0f, 0.0f);
    float raw_left_trigger = 0.0f;
    float raw_right_trigger = 0.0f;

    // Custom Theme Color Overrides (alpha == 0 uses Theme fallback)
    godot::Color custom_bg_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_deadzone_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_active_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Color custom_raw_color = godot::Color(0.0f, 0.0f, 0.0f, 0.0f);

    // Internal Drawing Helpers
    void _on_control_map_changed();
    void _draw_stick_gate(
        const godot::Vector2 &p_center,
        float p_radius,
        float p_inner_dz,
        float p_outer_dz,
        DeadzoneCurveShape p_shape,
        const godot::Vector2 &p_raw,
        const godot::Vector2 &p_filtered,
        const godot::String &p_title,
        const godot::Color &p_bg_col,
        const godot::Color &p_dz_col,
        const godot::Color &p_raw_col,
        const godot::Color &p_act_col,
        const godot::Color &p_grid_col,
        const godot::Color &p_text_col
    );
    void _draw_response_curve_panel(
        const godot::Rect2 &p_rect,
        float p_inner_dz,
        float p_outer_dz,
        DeadzoneCurveShape p_shape,
        float p_exponent,
        float p_current_raw_mag,
        float p_current_out_mag,
        const godot::Color &p_bg_col,
        const godot::Color &p_dz_col,
        const godot::Color &p_act_col,
        const godot::Color &p_grid_col,
        const godot::Color &p_text_col
    );
    void _draw_trigger_bars(
        const godot::Rect2 &p_rect,
        float p_trigger_dz,
        bool p_hair_trigger,
        float p_left_val,
        float p_right_val,
        const godot::Color &p_bg_col,
        const godot::Color &p_dz_col,
        const godot::Color &p_act_col,
        const godot::Color &p_grid_col,
        const godot::Color &p_text_col
    );

public:
    ControlStickDeadzoneVisualizer();
    ~ControlStickDeadzoneVisualizer() override;

    virtual void _process(double p_delta) override;
    virtual godot::Vector2 _get_minimum_size() const override;

    // --- Resource & Polling Setters / Getters ---
    void set_control_map(const godot::Ref<GameplayControlMap> &p_map);
    godot::Ref<GameplayControlMap> get_control_map() const;

    void set_poll_live_hardware(bool p_poll);
    bool get_poll_live_hardware() const;

    void set_joypad_device_id(int p_device);
    int get_joypad_device_id() const;

    void set_show_response_curve(bool p_show);
    bool get_show_response_curve() const;

    void set_show_trigger_bars(bool p_show);
    bool get_show_trigger_bars() const;

    void set_show_labels(bool p_show);
    bool get_show_labels() const;

    // --- Manual / Synthetic Telemetry Injection ---
    void set_simulated_stick_inputs(const godot::Vector2 &p_left_raw, const godot::Vector2 &p_right_raw);
    void set_simulated_trigger_inputs(float p_left_trigger, float p_right_trigger);

    godot::Vector2 get_raw_left_stick() const;
    godot::Vector2 get_filtered_left_stick() const;
    godot::Vector2 get_raw_right_stick() const;
    godot::Vector2 get_filtered_right_stick() const;

    // --- Custom Color Overrides ---
    void set_custom_bg_color(const godot::Color &p_color);
    godot::Color get_custom_bg_color() const;

    void set_custom_deadzone_color(const godot::Color &p_color);
    godot::Color get_custom_deadzone_color() const;

    void set_custom_active_color(const godot::Color &p_color);
    godot::Color get_custom_active_color() const;

    void set_custom_raw_color(const godot::Color &p_color);
    godot::Color get_custom_raw_color() const;
};

} // namespace ideam::godot_ext