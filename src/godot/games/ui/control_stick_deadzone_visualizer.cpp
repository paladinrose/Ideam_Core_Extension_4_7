#include "control_stick_deadzone_visualizer.h"

#include <cmath>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

namespace ideam::godot_ext {

void ControlStickDeadzoneVisualizer::_bind_methods() {
    godot::ClassDB::bind_method(godot::D_METHOD("set_control_map", "control_map"), &ControlStickDeadzoneVisualizer::set_control_map);
    godot::ClassDB::bind_method(godot::D_METHOD("get_control_map"), &ControlStickDeadzoneVisualizer::get_control_map);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "control_map", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayControlMap"), "set_control_map", "get_control_map");

    godot::ClassDB::bind_method(godot::D_METHOD("set_poll_live_hardware", "poll"), &ControlStickDeadzoneVisualizer::set_poll_live_hardware);
    godot::ClassDB::bind_method(godot::D_METHOD("get_poll_live_hardware"), &ControlStickDeadzoneVisualizer::get_poll_live_hardware);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "poll_live_hardware"), "set_poll_live_hardware", "get_poll_live_hardware");

    godot::ClassDB::bind_method(godot::D_METHOD("set_joypad_device_id", "device_id"), &ControlStickDeadzoneVisualizer::set_joypad_device_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_joypad_device_id"), &ControlStickDeadzoneVisualizer::get_joypad_device_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "joypad_device_id", godot::PROPERTY_HINT_RANGE, "0,15,1"), "set_joypad_device_id", "get_joypad_device_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_response_curve", "show"), &ControlStickDeadzoneVisualizer::set_show_response_curve);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_response_curve"), &ControlStickDeadzoneVisualizer::get_show_response_curve);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_response_curve"), "set_show_response_curve", "get_show_response_curve");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_trigger_bars", "show"), &ControlStickDeadzoneVisualizer::set_show_trigger_bars);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_trigger_bars"), &ControlStickDeadzoneVisualizer::get_show_trigger_bars);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_trigger_bars"), "set_show_trigger_bars", "get_show_trigger_bars");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_labels", "show"), &ControlStickDeadzoneVisualizer::set_show_labels);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_labels"), &ControlStickDeadzoneVisualizer::get_show_labels);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_labels"), "set_show_labels", "get_show_labels");

    godot::ClassDB::bind_method(godot::D_METHOD("set_simulated_stick_inputs", "left_raw", "right_raw"), &ControlStickDeadzoneVisualizer::set_simulated_stick_inputs);
    godot::ClassDB::bind_method(godot::D_METHOD("set_simulated_trigger_inputs", "left_trigger", "right_trigger"), &ControlStickDeadzoneVisualizer::set_simulated_trigger_inputs);

    godot::ClassDB::bind_method(godot::D_METHOD("get_raw_left_stick"), &ControlStickDeadzoneVisualizer::get_raw_left_stick);
    godot::ClassDB::bind_method(godot::D_METHOD("get_filtered_left_stick"), &ControlStickDeadzoneVisualizer::get_filtered_left_stick);
    godot::ClassDB::bind_method(godot::D_METHOD("get_raw_right_stick"), &ControlStickDeadzoneVisualizer::get_raw_right_stick);
    godot::ClassDB::bind_method(godot::D_METHOD("get_filtered_right_stick"), &ControlStickDeadzoneVisualizer::get_filtered_right_stick);

    ADD_GROUP("Custom Colors", "custom_");
    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_bg_color", "color"), &ControlStickDeadzoneVisualizer::set_custom_bg_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_bg_color"), &ControlStickDeadzoneVisualizer::get_custom_bg_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_bg_color"), "set_custom_bg_color", "get_custom_bg_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_deadzone_color", "color"), &ControlStickDeadzoneVisualizer::set_custom_deadzone_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_deadzone_color"), &ControlStickDeadzoneVisualizer::get_custom_deadzone_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_deadzone_color"), "set_custom_deadzone_color", "get_custom_deadzone_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_active_color", "color"), &ControlStickDeadzoneVisualizer::set_custom_active_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_active_color"), &ControlStickDeadzoneVisualizer::get_custom_active_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_active_color"), "set_custom_active_color", "get_custom_active_color");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_raw_color", "color"), &ControlStickDeadzoneVisualizer::set_custom_raw_color);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_raw_color"), &ControlStickDeadzoneVisualizer::get_custom_raw_color);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::COLOR, "custom_raw_color"), "set_custom_raw_color", "get_custom_raw_color");

    godot::ClassDB::bind_method(godot::D_METHOD("_on_control_map_changed"), &ControlStickDeadzoneVisualizer::_on_control_map_changed);
}

ControlStickDeadzoneVisualizer::ControlStickDeadzoneVisualizer() {
    set_clip_contents(true);
    set_process(true);
}

ControlStickDeadzoneVisualizer::~ControlStickDeadzoneVisualizer() {
}

godot::Vector2 ControlStickDeadzoneVisualizer::_get_minimum_size() const {
    float min_w = 180.0f;
    if (show_response_curve) min_w += 150.0f;
    if (show_trigger_bars) min_w += 180.0f;
    return godot::Vector2(min_w, 160.0f);
}

void ControlStickDeadzoneVisualizer::set_control_map(const godot::Ref<GameplayControlMap> &p_map) {
    if (control_map == p_map) return;

    if (control_map.is_valid() && control_map->is_connected("changed", godot::Callable(this, "_on_control_map_changed"))) {
        control_map->disconnect("changed", godot::Callable(this, "_on_control_map_changed"));
    }

    control_map = p_map;

    if (control_map.is_valid() && !control_map->is_connected("changed", godot::Callable(this, "_on_control_map_changed"))) {
        control_map->connect("changed", godot::Callable(this, "_on_control_map_changed"));
    }
    queue_redraw();
}

godot::Ref<GameplayControlMap> ControlStickDeadzoneVisualizer::get_control_map() const {
    return control_map;
}

void ControlStickDeadzoneVisualizer::set_poll_live_hardware(bool p_poll) {
    poll_live_hardware = p_poll;
    set_process(p_poll);
}
bool ControlStickDeadzoneVisualizer::get_poll_live_hardware() const { return poll_live_hardware; }

void ControlStickDeadzoneVisualizer::set_joypad_device_id(int p_device) {
    joypad_device_id = godot::Math::clamp(p_device, 0, 15);
}
int ControlStickDeadzoneVisualizer::get_joypad_device_id() const { return joypad_device_id; }

void ControlStickDeadzoneVisualizer::set_show_response_curve(bool p_show) {
    show_response_curve = p_show;
    update_minimum_size();
    queue_redraw();
}
bool ControlStickDeadzoneVisualizer::get_show_response_curve() const { return show_response_curve; }

void ControlStickDeadzoneVisualizer::set_show_trigger_bars(bool p_show) {
    show_trigger_bars = p_show;
    update_minimum_size();
    queue_redraw();
}
bool ControlStickDeadzoneVisualizer::get_show_trigger_bars() const { return show_trigger_bars; }

void ControlStickDeadzoneVisualizer::set_show_labels(bool p_show) {
    show_labels = p_show;
    queue_redraw();
}
bool ControlStickDeadzoneVisualizer::get_show_labels() const { return show_labels; }

void ControlStickDeadzoneVisualizer::set_simulated_stick_inputs(const godot::Vector2 &p_left_raw, const godot::Vector2 &p_right_raw) {
    raw_left_stick = p_left_raw;
    raw_right_stick = p_right_raw;

    float l_in = control_map.is_valid() ? control_map->get_left_stick_deadzone_inner() : 0.15f;
    float l_out = control_map.is_valid() ? control_map->get_left_stick_deadzone_outer() : 0.95f;
    float r_in = control_map.is_valid() ? control_map->get_right_stick_deadzone_inner() : 0.15f;
    float r_out = control_map.is_valid() ? control_map->get_right_stick_deadzone_outer() : 0.95f;
    DeadzoneCurveShape shape = control_map.is_valid() ? control_map->get_deadzone_curve_shape() : DeadzoneCurveShape::DEADZONE_RADIAL_SCALED;
    float exp = control_map.is_valid() ? control_map->get_gamepad_response_exponent() : 1.6f;

    filtered_left_stick = GameplayControlMap::apply_stick_deadzone(raw_left_stick, l_in, l_out, shape, exp);
    filtered_right_stick = GameplayControlMap::apply_stick_deadzone(raw_right_stick, r_in, r_out, shape, exp);
    queue_redraw();
}

void ControlStickDeadzoneVisualizer::set_simulated_trigger_inputs(float p_left_trigger, float p_right_trigger) {
    raw_left_trigger = godot::Math::clamp(p_left_trigger, 0.0f, 1.0f);
    raw_right_trigger = godot::Math::clamp(p_right_trigger, 0.0f, 1.0f);
    queue_redraw();
}

godot::Vector2 ControlStickDeadzoneVisualizer::get_raw_left_stick() const { return raw_left_stick; }
godot::Vector2 ControlStickDeadzoneVisualizer::get_filtered_left_stick() const { return filtered_left_stick; }
godot::Vector2 ControlStickDeadzoneVisualizer::get_raw_right_stick() const { return raw_right_stick; }
godot::Vector2 ControlStickDeadzoneVisualizer::get_filtered_right_stick() const { return filtered_right_stick; }

void ControlStickDeadzoneVisualizer::set_custom_bg_color(const godot::Color &p_color) {
    custom_bg_color = p_color;
    queue_redraw();
}
godot::Color ControlStickDeadzoneVisualizer::get_custom_bg_color() const { return custom_bg_color; }

void ControlStickDeadzoneVisualizer::set_custom_deadzone_color(const godot::Color &p_color) {
    custom_deadzone_color = p_color;
    queue_redraw();
}
godot::Color ControlStickDeadzoneVisualizer::get_custom_deadzone_color() const { return custom_deadzone_color; }

void ControlStickDeadzoneVisualizer::set_custom_active_color(const godot::Color &p_color) {
    custom_active_color = p_color;
    queue_redraw();
}
godot::Color ControlStickDeadzoneVisualizer::get_custom_active_color() const { return custom_active_color; }

void ControlStickDeadzoneVisualizer::set_custom_raw_color(const godot::Color &p_color) {
    custom_raw_color = p_color;
    queue_redraw();
}
godot::Color ControlStickDeadzoneVisualizer::get_custom_raw_color() const { return custom_raw_color; }

void ControlStickDeadzoneVisualizer::_on_control_map_changed() {
    queue_redraw();
}

void ControlStickDeadzoneVisualizer::_process(double p_delta) {
    if (!poll_live_hardware || !is_visible_in_tree()) {
        return;
    }

    godot::Input *inp = godot::Input::get_singleton();
    if (!inp) {
        return;
    }

    int dev = (control_map.is_valid() && control_map->get_device_id() >= 0)
        ? control_map->get_device_id()
        : joypad_device_id;

    godot::Vector2 new_l(
        inp->get_joy_axis(dev, godot::JOY_AXIS_LEFT_X),
        inp->get_joy_axis(dev, godot::JOY_AXIS_LEFT_Y)
    );
    godot::Vector2 new_r(
        inp->get_joy_axis(dev, godot::JOY_AXIS_RIGHT_X),
        inp->get_joy_axis(dev, godot::JOY_AXIS_RIGHT_Y)
    );
    float new_lt = godot::Math::clamp(inp->get_joy_axis(dev, godot::JOY_AXIS_TRIGGER_LEFT), 0.0f, 1.0f);
    float new_rt = godot::Math::clamp(inp->get_joy_axis(dev, godot::JOY_AXIS_TRIGGER_RIGHT), 0.0f, 1.0f);

    // Fallback to Custom USB channels ("AX"/"AY", "LX"/"LY") if custom USB is active and joypad is idle
    if (control_map.is_valid() && control_map->get_custom_usb_enabled()) {
        godot::Vector2 usb_l(
            control_map->get_custom_channel_value(godot::StringName("AX")),
            control_map->get_custom_channel_value(godot::StringName("AY"))
        );
        godot::Vector2 usb_r(
            control_map->get_custom_channel_value(godot::StringName("LX")),
            control_map->get_custom_channel_value(godot::StringName("LY"))
        );
        if (usb_l.length_squared() > new_l.length_squared()) new_l = usb_l;
        if (usb_r.length_squared() > new_r.length_squared()) new_r = usb_r;
    }

    if (control_map.is_valid() && control_map->get_swap_thumbsticks()) {
        SWAP(new_l, new_r);
    }

    if (!new_l.is_equal_approx(raw_left_stick) ||
        !new_r.is_equal_approx(raw_right_stick) ||
        !godot::Math::is_equal_approx(new_lt, raw_left_trigger) ||
        !godot::Math::is_equal_approx(new_rt, raw_right_trigger)) {
        raw_left_trigger = new_lt;
        raw_right_trigger = new_rt;
        set_simulated_stick_inputs(new_l, new_r);
    }
}

void ControlStickDeadzoneVisualizer::_notification(int p_what) {
    if (p_what == NOTIFICATION_THEME_CHANGED || p_what == NOTIFICATION_RESIZED) {
        queue_redraw();
        return;
    }

    if (p_what != NOTIFICATION_DRAW) {
        return;
    }

    // 1. Resolve Palette
    godot::Color bg_col = (custom_bg_color.a > 0.0f)
        ? custom_bg_color
        : (has_theme_color("background_color", "ControlStickDeadzoneVisualizer")
               ? get_theme_color("background_color", "ControlStickDeadzoneVisualizer")
               : godot::Color(0.11f, 0.12f, 0.15f, 0.90f));

    godot::Color dz_col = (custom_deadzone_color.a > 0.0f)
        ? custom_deadzone_color
        : (has_theme_color("deadzone_color", "ControlStickDeadzoneVisualizer")
               ? get_theme_color("deadzone_color", "ControlStickDeadzoneVisualizer")
               : godot::Color(0.92f, 0.28f, 0.28f, 0.35f));

    godot::Color act_col = (custom_active_color.a > 0.0f)
        ? custom_active_color
        : (has_theme_color("active_color", "ControlStickDeadzoneVisualizer")
               ? get_theme_color("active_color", "ControlStickDeadzoneVisualizer")
               : godot::Color(0.18f, 0.82f, 0.96f, 1.0f));

    godot::Color raw_col = (custom_raw_color.a > 0.0f)
        ? custom_raw_color
        : (has_theme_color("raw_color", "ControlStickDeadzoneVisualizer")
               ? get_theme_color("raw_color", "ControlStickDeadzoneVisualizer")
               : godot::Color(0.75f, 0.78f, 0.82f, 0.65f));

    godot::Color grid_col(1.0f, 1.0f, 1.0f, 0.14f);
    godot::Color text_col = has_theme_color("font_color", "Label")
        ? get_theme_color("font_color", "Label")
        : godot::Color(0.92f, 0.93f, 0.95f, 1.0f);

    // 2. Read active deadzone parameters from bound GameplayControlMap
    float l_in = control_map.is_valid() ? control_map->get_left_stick_deadzone_inner() : 0.15f;
    float l_out = control_map.is_valid() ? control_map->get_left_stick_deadzone_outer() : 0.95f;
    float r_in = control_map.is_valid() ? control_map->get_right_stick_deadzone_inner() : 0.15f;
    float r_out = control_map.is_valid() ? control_map->get_right_stick_deadzone_outer() : 0.95f;
    float trig_dz = control_map.is_valid() ? control_map->get_trigger_deadzone() : 0.08f;
    bool hair_trig = control_map.is_valid() ? control_map->get_trigger_hair_trigger_mode() : false;
    DeadzoneCurveShape shape = control_map.is_valid() ? control_map->get_deadzone_curve_shape() : DeadzoneCurveShape::DEADZONE_RADIAL_SCALED;
    float exponent = control_map.is_valid() ? control_map->get_gamepad_response_exponent() : 1.6f;

    filtered_left_stick = GameplayControlMap::apply_stick_deadzone(raw_left_stick, l_in, l_out, shape, exponent);
    filtered_right_stick = GameplayControlMap::apply_stick_deadzone(raw_right_stick, r_in, r_out, shape, exponent);

    // 3. Partition horizontal layout into sub-panels
    godot::Vector2 sz = get_size();
    int panel_count = 1 + (show_response_curve ? 1 : 0) + (show_trigger_bars ? 1 : 0);
    float pad = 10.0f;
    float col_w = (sz.x - pad * static_cast<float>(panel_count + 1)) / static_cast<float>(panel_count);
    float col_h = sz.y - pad * 2.0f;
    float radius = godot::Math::min(col_w, col_h - (show_labels ? 22.0f : 4.0f)) * 0.45f;

    float cursor_x = pad;

    // Panel 1: Left Stick Gate
    godot::Vector2 l_center(cursor_x + col_w * 0.5f, pad + (col_h + (show_labels ? 16.0f : 0.0f)) * 0.5f);
    _draw_stick_gate(l_center, radius, l_in, l_out, shape, raw_left_stick, filtered_left_stick, "Left Stick", bg_col, dz_col, raw_col, act_col, grid_col, text_col);
    cursor_x += col_w + pad;

    // Panel 2: Response Curve
    if (show_response_curve) {
        godot::Rect2 curve_rect(cursor_x, pad, col_w, col_h);
        _draw_response_curve_panel(
            curve_rect,
            l_in,
            l_out,
            shape,
            exponent,
            godot::Math::clamp(raw_left_stick.length(), 0.0f, 1.0f),
            godot::Math::clamp(filtered_left_stick.length(), 0.0f, 1.0f),
            bg_col,
            dz_col,
            act_col,
            grid_col,
            text_col
        );
        cursor_x += col_w + pad;
    }

    // Panel 3: Right Stick Gate + Trigger Bars
    if (show_trigger_bars) {
        float stick_share = col_w * 0.72f;
        float trig_share = col_w * 0.25f;
        float r_radius = godot::Math::min(stick_share, col_h - (show_labels ? 22.0f : 4.0f)) * 0.45f;
        godot::Vector2 r_center(cursor_x + stick_share * 0.5f, pad + (col_h + (show_labels ? 16.0f : 0.0f)) * 0.5f);
        _draw_stick_gate(r_center, r_radius, r_in, r_out, shape, raw_right_stick, filtered_right_stick, "Right Stick", bg_col, dz_col, raw_col, act_col, grid_col, text_col);

        godot::Rect2 trig_rect(cursor_x + stick_share + 4.0f, pad, trig_share, col_h);
        _draw_trigger_bars(trig_rect, trig_dz, hair_trig, raw_left_trigger, raw_right_trigger, bg_col, dz_col, act_col, grid_col, text_col);
    }
}

void ControlStickDeadzoneVisualizer::_draw_stick_gate(
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
) {
    if (p_radius <= 4.0f) return;

    // Background disc & outer boundary
    draw_circle(p_center, p_radius, p_bg_col);
    draw_arc(p_center, p_radius, 0.0f, Math_TAU, 64, p_grid_col, 1.5f);

    // Outer saturation ring
    float outer_r = p_radius * godot::Math::clamp(p_outer_dz, 0.1f, 1.0f);
    draw_arc(p_center, outer_r, 0.0f, Math_TAU, 64, godot::Color(p_act_col.r, p_act_col.g, p_act_col.b, 0.4f), 1.25f);

    // Inner deadzone visualization (axial crossband vs. radial disc)
    float inner_r = p_radius * godot::Math::clamp(p_inner_dz, 0.0f, 0.95f);
    if (p_shape == DeadzoneCurveShape::DEADZONE_AXIAL_INDEPENDENT) {
        draw_rect(godot::Rect2(p_center.x - inner_r, p_center.y - p_radius, inner_r * 2.0f, p_radius * 2.0f), p_dz_col, true);
        draw_rect(godot::Rect2(p_center.x - p_radius, p_center.y - inner_r, p_radius * 2.0f, inner_r * 2.0f), p_dz_col, true);
    } else if (inner_r > 0.5f) {
        draw_circle(p_center, inner_r, p_dz_col);
        draw_arc(p_center, inner_r, 0.0f, Math_TAU, 48, godot::Color(p_dz_col.r, p_dz_col.g, p_dz_col.b, 0.85f), 1.25f);
    }

    // Center hairlines
    draw_line(godot::Vector2(p_center.x - p_radius, p_center.y), godot::Vector2(p_center.x + p_radius, p_center.y), p_grid_col, 1.0f);
    draw_line(godot::Vector2(p_center.x, p_center.y - p_radius), godot::Vector2(p_center.x, p_center.y + p_radius), p_grid_col, 1.0f);

    // Raw & Filtered stick positions
    godot::Vector2 raw_pt = p_center + p_raw.limit_length(1.0f) * p_radius;
    godot::Vector2 filt_pt = p_center + p_filtered.limit_length(1.0f) * p_radius;

    draw_line(p_center, filt_pt, p_act_col, 2.0f);
    draw_circle(raw_pt, 4.0f, p_raw_col);
    draw_circle(filt_pt, 5.5f, p_act_col);

    if (show_labels) {
        godot::Ref<godot::Font> font = get_theme_default_font();
        int font_size = get_theme_default_font_size() > 0 ? get_theme_default_font_size() - 2 : 12;
        if (font.is_valid()) {
            draw_string(
                font,
                godot::Vector2(p_center.x - p_radius, p_center.y - p_radius - 6.0f),
                p_title + " (DZ: " + godot::String::num(p_inner_dz, 2) + ")",
                godot::HORIZONTAL_ALIGNMENT_CENTER,
                p_radius * 2.0f,
                font_size,
                p_text_col
            );
        }
    }
}

void ControlStickDeadzoneVisualizer::_draw_response_curve_panel(
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
) {
    float top_pad = show_labels ? 20.0f : 4.0f;
    float side_pad = 12.0f;
    godot::Rect2 plot(
        p_rect.position.x + side_pad,
        p_rect.position.y + top_pad,
        godot::Math::max(p_rect.size.x - side_pad * 2.0f, 20.0f),
        godot::Math::max(p_rect.size.y - top_pad - 12.0f, 20.0f)
    );

    draw_rect(plot, p_bg_col, true);

    // Shaded deadzone floor band [0 .. inner_dz]
    float dz_w = plot.size.x * godot::Math::clamp(p_inner_dz, 0.0f, 1.0f);
    if (dz_w > 1.0f) {
        draw_rect(godot::Rect2(plot.position.x, plot.position.y, dz_w, plot.size.height), p_dz_col, true);
    }

    // Grid lines
    for (int i = 1; i <= 3; ++i) {
        float t = static_cast<float>(i) * 0.25f;
        float gx = plot.position.x + plot.size.x * t;
        float gy = plot.position.y + plot.size.y * (1.0f - t);
        draw_line(godot::Vector2(gx, plot.position.y), godot::Vector2(gx, plot.position.y + plot.size.y), p_grid_col, 1.0f);
        draw_line(godot::Vector2(plot.position.x, gy), godot::Vector2(plot.position.x + plot.size.x, gy), p_grid_col, 1.0f);
    }
    draw_rect(plot, p_grid_col, false, 1.25f);

    // Sample response curve via GameplayControlMap::apply_stick_deadzone
    const int samples = 48;
    godot::PackedVector2Array pts;
    pts.resize(samples + 1);
    for (int i = 0; i <= samples; ++i) {
        float x = static_cast<float>(i) / static_cast<float>(samples);
        godot::Vector2 mapped = GameplayControlMap::apply_stick_deadzone(godot::Vector2(x, 0.0f), p_inner_dz, p_outer_dz, p_shape, p_exponent);
        float y = godot::Math::clamp(mapped.length(), 0.0f, 1.0f);
        pts.set(i, godot::Vector2(plot.position.x + x * plot.size.x, plot.position.y + (1.0f - y) * plot.size.y));
    }
    draw_polyline(pts, p_act_col, 2.25f);

    // Live operating point marker
    godot::Vector2 live_pt(
        plot.position.x + p_current_raw_mag * plot.size.x,
        plot.position.y + (1.0f - p_current_out_mag) * plot.size.y
    );
    draw_circle(live_pt, 4.5f, p_act_col);

    if (show_labels) {
        godot::Ref<godot::Font> font = get_theme_default_font();
        int font_size = get_theme_default_font_size() > 0 ? get_theme_default_font_size() - 2 : 12;
        if (font.is_valid()) {
            draw_string(
                font,
                godot::Vector2(plot.position.x, plot.position.y - 6.0f),
                "Response Curve (exp " + godot::String::num(p_exponent, 1) + ")",
                godot::HORIZONTAL_ALIGNMENT_CENTER,
                plot.size.x,
                font_size,
                p_text_col
            );
        }
    }
}

void ControlStickDeadzoneVisualizer::_draw_trigger_bars(
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
) {
    float top_pad = show_labels ? 20.0f : 4.0f;
    float bar_h = godot::Math::max(p_rect.size.y - top_pad - 12.0f, 20.0f);
    float bar_w = godot::Math::max((p_rect.size.x - 10.0f) * 0.5f, 8.0f);

    float vals[2] = {p_left_val, p_right_val};
    const char *names[2] = {"L2", "R2"};

    for (int i = 0; i < 2; ++i) {
        float bx = p_rect.position.x + i * (bar_w + 6.0f);
        float by = p_rect.position.y + top_pad;
        godot::Rect2 bar_rect(bx, by, bar_w, bar_h);
        draw_rect(bar_rect, p_bg_col, true);

        // Deadzone threshold at bottom of trigger bar
        float dz_h = bar_h * godot::Math::clamp(p_trigger_dz, 0.0f, 0.9f);
        if (dz_h > 1.0f) {
            draw_rect(godot::Rect2(bx, by + bar_h - dz_h, bar_w, dz_h), p_dz_col, true);
        }

        // Active trigger fill
        float filt = p_hair_trigger
            ? (vals[i] > p_trigger_dz ? 1.0f : 0.0f)
            : GameplayControlMap::apply_axis_deadzone(vals[i], p_trigger_dz, 0.98f);
        float fill_h = bar_h * godot::Math::clamp(filt, 0.0f, 1.0f);
        if (fill_h > 0.5f) {
            draw_rect(godot::Rect2(bx, by + bar_h - fill_h, bar_w, fill_h), p_act_col, true);
        }

        draw_rect(bar_rect, p_grid_col, false, 1.25f);

        if (show_labels) {
            godot::Ref<godot::Font> font = get_theme_default_font();
            int font_size = get_theme_default_font_size() > 0 ? get_theme_default_font_size() - 3 : 11;
            if (font.is_valid()) {
                draw_string(font, godot::Vector2(bx, by - 5.0f), names[i], godot::HORIZONTAL_ALIGNMENT_CENTER, bar_w, font_size, p_text_col);
            }
        }
    }
}

} // namespace ideam::godot_ext