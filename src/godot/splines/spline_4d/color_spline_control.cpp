#include "color_spline_control.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::godot_ext {

// ============================================================================
// ColorSplineWheelCanvas Implementation
// ============================================================================

void ColorSplineWheelCanvas::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &ColorSplineWheelCanvas::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &ColorSplineWheelCanvas::get_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_spline",
        "get_spline"
    );

    ClassDB::bind_method(D_METHOD("set_selected_stop_index", "index"), &ColorSplineWheelCanvas::set_selected_stop_index);
    ClassDB::bind_method(D_METHOD("get_selected_stop_index"), &ColorSplineWheelCanvas::get_selected_stop_index);

    ClassDB::bind_method(D_METHOD("set_hsva_mode", "hsva"), &ColorSplineWheelCanvas::set_hsva_mode);
    ClassDB::bind_method(D_METHOD("get_hsva_mode"), &ColorSplineWheelCanvas::get_hsva_mode);

    ADD_SIGNAL(MethodInfo("stop_selected", PropertyInfo(Variant::INT, "stop_index")));
    ADD_SIGNAL(MethodInfo("stops_modified"));
}

ColorSplineWheelCanvas::ColorSplineWheelCanvas() {
    set_custom_minimum_size(godot::Vector2(280.0f, 240.0f));
    set_clip_contents(true);
    set_focus_mode(FOCUS_CLICK);
}

ColorSplineWheelCanvas::~ColorSplineWheelCanvas() {
    if (spline.is_valid()) {
        const godot::Callable cb = callable_mp(this, &ColorSplineWheelCanvas::_on_spline_changed);
        if (spline->is_connected("changed", cb)) {
            spline->disconnect("changed", cb);
        }
    }
}

void ColorSplineWheelCanvas::set_spline(const godot::Ref<Spline4DResource> &p_spline) {
    if (spline == p_spline) {
        return;
    }
    const godot::Callable cb = callable_mp(this, &ColorSplineWheelCanvas::_on_spline_changed);
    if (spline.is_valid() && spline->is_connected("changed", cb)) {
        spline->disconnect("changed", cb);
    }
    spline = p_spline;
    if (spline.is_valid()) {
        if (!spline->is_connected("changed", cb)) {
            spline->connect("changed", cb);
        }
        hsva_mode = spline->get_color_gradient_mode();
        selected_stop_index = (spline->get_point_count() > 0) ? 0 : -1;
    } else {
        selected_stop_index = -1;
    }
    queue_redraw();
}

void ColorSplineWheelCanvas::_on_spline_changed() {
    if (spline.is_valid()) {
        const int n = spline->get_point_count();
        if (n <= 0) {
            selected_stop_index = -1;
        } else {
            selected_stop_index = std::clamp(selected_stop_index, 0, n - 1);
        }
    }
    queue_redraw();
}

void ColorSplineWheelCanvas::set_selected_stop_index(int p_index) {
    const int n = spline.is_valid() ? spline->get_point_count() : 0;
    const int clamped = (n > 0) ? std::clamp(p_index, 0, n - 1) : -1;
    if (selected_stop_index == clamped) {
        return;
    }
    selected_stop_index = clamped;
    emit_signal("stop_selected", selected_stop_index);
    queue_redraw();
}

void ColorSplineWheelCanvas::set_hsva_mode(bool p_hsva) {
    if (hsva_mode == p_hsva) {
        return;
    }
    hsva_mode = p_hsva;
    queue_redraw();
}

godot::Rect2 ColorSplineWheelCanvas::_get_gradient_bar_rect() const {
    const godot::Vector2 sz = get_size();
    return godot::Rect2(10.0f, 8.0f, std::max(40.0f, sz.x - 20.0f), 28.0f);
}

godot::Rect2 ColorSplineWheelCanvas::_get_wheel_rect() const {
    const godot::Vector2 sz = get_size();
    const float top = 46.0f;
    const float avail_h = std::max(64.0f, sz.y - top - 10.0f);
    const float avail_w = std::max(64.0f, sz.x - 86.0f);
    const float side = std::min(avail_w, avail_h);
    const float x = 10.0f + (avail_w - side) * 0.5f;
    const float y = top + (avail_h - side) * 0.5f;
    return godot::Rect2(x, y, side, side);
}

godot::Rect2 ColorSplineWheelCanvas::_get_value_track_rect() const {
    const godot::Vector2 sz = get_size();
    const godot::Rect2 wrect = _get_wheel_rect();
    return godot::Rect2(std::max(wrect.position.x + wrect.size.x + 12.0f, sz.x - 62.0f), wrect.position.y, 20.0f, wrect.size.y);
}

godot::Rect2 ColorSplineWheelCanvas::_get_alpha_track_rect() const {
    const godot::Rect2 vrect = _get_value_track_rect();
    return godot::Rect2(vrect.position.x + vrect.size.x + 10.0f, vrect.position.y, 20.0f, vrect.size.y);
}

std::vector<real_t> ColorSplineWheelCanvas::_compute_stop_norms() const {
    std::vector<real_t> norms;
    if (spline.is_null()) return norms;

    const int pt_count  = spline->get_point_count();
    const int sec_count = spline->get_section_count();
    if (pt_count <= 0) return norms;
    if (pt_count == 1 || sec_count <= 0) {
        norms.push_back(static_cast<real_t>(0.5));
        return norms;
    }

    const real_t total_w = std::max(static_cast<real_t>(1e-6), spline->get_total_weight());
    norms.reserve(static_cast<size_t>(pt_count));
    real_t accum = static_cast<real_t>(0.0);
    norms.push_back(static_cast<real_t>(0.0));

    for (int s = 0; s < pt_count - 1 && s < sec_count; ++s) {
        accum += std::max(static_cast<real_t>(1e-4), spline->get_section_weight(s));
        norms.push_back(std::clamp(accum / total_w, static_cast<real_t>(0.0), static_cast<real_t>(1.0)));
    }
    if (!spline->get_wrap()) {
        norms.back() = static_cast<real_t>(1.0);
    }
    return norms;
}

godot::Color ColorSplineWheelCanvas::_evaluate_stop_display_color(int p_index) const {
    if (spline.is_null() || p_index < 0 || p_index >= spline->get_point_count()) {
        return godot::Color(1.0f, 1.0f, 1.0f, 1.0f);
    }
    const godot::Vector4 v = spline->get_point_position(p_index);
    if (hsva_mode) {
        float h = (v.x > 1.0f) ? std::fmod(v.x / 360.0f, 1.0f) : std::fmod(v.x, 1.0f);
        if (h < 0.0f) h += 1.0f;
        return godot::Color::from_hsv(
            h,
            std::clamp(v.y, 0.0f, 1.0f),
            std::clamp(v.z, 0.0f, 1.0f),
            std::clamp(v.w, 0.0f, 1.0f)
        );
    }
    return godot::Color(
        std::clamp(v.x, 0.0f, 1.0f),
        std::clamp(v.y, 0.0f, 1.0f),
        std::clamp(v.z, 0.0f, 1.0f),
        std::clamp(v.w, 0.0f, 1.0f)
    );
}

godot::Vector2 ColorSplineWheelCanvas::_stop_to_wheel_pos(
    int p_index,
    const godot::Vector2 &p_center,
    float p_radius
) const {
    if (spline.is_null() || p_index < 0 || p_index >= spline->get_point_count()) {
        return p_center;
    }
    const godot::Vector4 v = spline->get_point_position(p_index);
    float hue_deg = v.x;
    float sat     = std::clamp(v.y, 0.0f, 1.0f);
    if (!hsva_mode) {
        const godot::Color c(v.x, v.y, v.z, v.w);
        hue_deg = c.get_h() * 360.0f;
        sat     = c.get_s();
    } else if (hue_deg <= 1.0f && hue_deg >= 0.0f) {
        // Support both [0..360] (from Spline4D::create_gradient) and [0..1]
        bool any_over_one = false;
        for (int i = 0; i < spline->get_point_count(); ++i) {
            if (std::abs(spline->get_point_position(i).x) > 1.001f) {
                any_over_one = true;
                break;
            }
        }
        if (!any_over_one) {
            hue_deg *= 360.0f;
        }
    }

    const float rad = hue_deg * static_cast<float>(std::numbers::pi_v<double> / 180.0);
    return p_center + godot::Vector2(std::cos(rad), std::sin(rad)) * (sat * p_radius);
}

void ColorSplineWheelCanvas::_set_stop_from_wheel_pos(
    int p_index,
    const godot::Vector2 &p_pos,
    const godot::Vector2 &p_center,
    float p_radius
) {
    if (spline.is_null() || p_index < 0 || p_index >= spline->get_point_count()) {
        return;
    }
    const godot::Vector2 rel = p_pos - p_center;
    const float sat = std::clamp(rel.length() / std::max(1.0f, p_radius), 0.0f, 1.0f);
    float angle_deg = std::atan2(rel.y, rel.x) * static_cast<float>(180.0 / std::numbers::pi_v<double>);
    if (angle_deg < 0.0f) {
        angle_deg += 360.0f;
    }

    godot::Vector4 v = spline->get_point_position(p_index);
    if (hsva_mode) {
        v.x = angle_deg;
        v.y = sat;
    } else {
        const godot::Color cur(v.x, v.y, v.z, v.w);
        const godot::Color upd = godot::Color::from_hsv(angle_deg / 360.0f, sat, cur.get_v(), cur.a);
        v = godot::Vector4(upd.r, upd.g, upd.b, upd.a);
    }
    spline->set_point_position(p_index, v);
    emit_signal("stops_modified");
}

void ColorSplineWheelCanvas::_set_stop_norm_weight(int p_index, float p_target_norm) {
    if (spline.is_null()) return;
    const int n = spline->get_point_count();
    if (p_index <= 0 || p_index >= n - 1) return;

    const std::vector<real_t> norms = _compute_stop_norms();
    if (static_cast<int>(norms.size()) != n) return;

    const float prev_norm = static_cast<float>(norms[static_cast<size_t>(p_index - 1)]) + 0.02f;
    const float next_norm = static_cast<float>(norms[static_cast<size_t>(p_index + 1)]) - 0.02f;
    if (prev_norm >= next_norm) return;

    const float clamped = std::clamp(p_target_norm, prev_norm, next_norm);
    const real_t total_w = std::max(static_cast<real_t>(1e-4), spline->get_total_weight());
    const real_t pair_w  = spline->get_section_weight(p_index - 1) + spline->get_section_weight(p_index);
    const real_t span_norm = static_cast<real_t>(norms[static_cast<size_t>(p_index + 1)] - norms[static_cast<size_t>(p_index - 1)]);
    if (span_norm <= static_cast<real_t>(1e-5)) return;

    const real_t ratio = static_cast<real_t>(clamped - static_cast<float>(norms[static_cast<size_t>(p_index - 1)])) / span_norm;
    const real_t w_left  = std::max(static_cast<real_t>(0.01), pair_w * ratio);
    const real_t w_right = std::max(static_cast<real_t>(0.01), pair_w - w_left);
    (void)total_w;

    spline->set_section_weight(p_index - 1, w_left);
    spline->set_section_weight(p_index, w_right);
    emit_signal("stops_modified");
}

void ColorSplineWheelCanvas::_set_stop_value_channel(int p_index, float p_val) {
    if (spline.is_null() || p_index < 0 || p_index >= spline->get_point_count()) return;
    const float clamped = std::clamp(p_val, 0.0f, 1.0f);
    godot::Vector4 v = spline->get_point_position(p_index);
    if (hsva_mode) {
        v.z = clamped;
    } else {
        const godot::Color cur(v.x, v.y, v.z, v.w);
        const godot::Color upd = godot::Color::from_hsv(cur.get_h(), cur.get_s(), clamped, cur.a);
        v = godot::Vector4(upd.r, upd.g, upd.b, upd.a);
    }
    spline->set_point_position(p_index, v);
    emit_signal("stops_modified");
}

void ColorSplineWheelCanvas::_set_stop_alpha_channel(int p_index, float p_alpha) {
    if (spline.is_null() || p_index < 0 || p_index >= spline->get_point_count()) return;
    godot::Vector4 v = spline->get_point_position(p_index);
    v.w = std::clamp(p_alpha, 0.0f, 1.0f);
    spline->set_point_position(p_index, v);
    emit_signal("stops_modified");
}

void ColorSplineWheelCanvas::_draw() {
    const godot::Vector2 sz = get_size();
    draw_rect(godot::Rect2(godot::Vector2(0.0f, 0.0f), sz), godot::Color(0.11f, 0.12f, 0.15f, 1.0f), true);

    const godot::Rect2 bar_rect = _get_gradient_bar_rect();
    const godot::Rect2 whl_rect = _get_wheel_rect();
    const godot::Rect2 val_rect = _get_value_track_rect();
    const godot::Rect2 alp_rect = _get_alpha_track_rect();

    // 1. Draw Top Gradient Preview Bar (with checkerboard alpha backdrop)
    draw_rect(bar_rect, godot::Color(0.22f, 0.22f, 0.25f, 1.0f), true);
    constexpr float CHECK_SZ = 7.0f;
    for (float y = bar_rect.position.y; y < bar_rect.position.y + bar_rect.size.y; y += CHECK_SZ) {
        for (float x = bar_rect.position.x; x < bar_rect.position.x + bar_rect.size.x; x += CHECK_SZ) {
            const int cx = static_cast<int>((x - bar_rect.position.x) / CHECK_SZ);
            const int cy = static_cast<int>((y - bar_rect.position.y) / CHECK_SZ);
            if (((cx + cy) & 1) == 0) {
                const float w = std::min(CHECK_SZ, bar_rect.position.x + bar_rect.size.x - x);
                const float h = std::min(CHECK_SZ, bar_rect.position.y + bar_rect.size.y - y);
                draw_rect(godot::Rect2(x, y, w, h), godot::Color(0.36f, 0.36f, 0.40f, 1.0f), true);
            }
        }
    }

    if (spline.is_valid() && spline->get_point_count() > 0) {
        constexpr int SAMPLES = 128;
        const float slice_w = bar_rect.size.x / static_cast<float>(SAMPLES);
        for (int i = 0; i < SAMPLES; ++i) {
            const real_t pct = (static_cast<real_t>(i) + static_cast<real_t>(0.5)) / static_cast<real_t>(SAMPLES);
            const godot::Color c = spline->sample_color(pct, true, hsva_mode);
            draw_rect(
                godot::Rect2(bar_rect.position.x + static_cast<float>(i) * slice_w, bar_rect.position.y, slice_w + 1.0f, bar_rect.size.y),
                c,
                true
            );
        }
    }
    draw_rect(bar_rect, godot::Color(0.45f, 0.48f, 0.55f, 1.0f), false, 1.5f);

    // Draw stop markers on the gradient bar
    const std::vector<real_t> norms = _compute_stop_norms();
    for (size_t i = 0; i < norms.size(); ++i) {
        const float sx = bar_rect.position.x + static_cast<float>(norms[i]) * bar_rect.size.x;
        const bool is_sel = (static_cast<int>(i) == selected_stop_index);
        const godot::Color stop_col = _evaluate_stop_display_color(static_cast<int>(i));

        draw_line(
            godot::Vector2(sx, bar_rect.position.y),
            godot::Vector2(sx, bar_rect.position.y + bar_rect.size.y),
            is_sel ? godot::Color(1.0f, 0.95f, 0.30f, 1.0f) : godot::Color(0.95f, 0.95f, 0.98f, 0.85f),
            is_sel ? 2.5f : 1.5f
        );
        draw_circle(
            godot::Vector2(sx, bar_rect.position.y + bar_rect.size.y + 4.0f),
            is_sel ? 5.5f : 4.2f,
            is_sel ? godot::Color(1.0f, 0.92f, 0.25f, 1.0f) : godot::Color(0.85f, 0.88f, 0.94f, 1.0f)
        );
        draw_circle(
            godot::Vector2(sx, bar_rect.position.y + bar_rect.size.y + 4.0f),
            is_sel ? 3.5f : 2.6f,
            godot::Color(stop_col.r, stop_col.g, stop_col.b, 1.0f)
        );
    }

    // 2. Draw 2D Hue-Saturation Wheel
    const godot::Vector2 wheel_center = whl_rect.position + whl_rect.size * 0.5f;
    const float wheel_radius = std::max(16.0f, whl_rect.size.x * 0.5f - 6.0f);

    constexpr int RINGS   = 8;
    constexpr int SECTORS = 48;
    const float two_pi = static_cast<float>(2.0 * std::numbers::pi_v<double>);

    float active_val = 0.92f;
    if (spline.is_valid() && selected_stop_index >= 0 && selected_stop_index < spline->get_point_count()) {
        const godot::Vector4 sv = spline->get_point_position(selected_stop_index);
        active_val = hsva_mode ? std::clamp(sv.z, 0.25f, 1.0f) : std::clamp(godot::Color(sv.x, sv.y, sv.z, sv.w).get_v(), 0.25f, 1.0f);
    }

    for (int r = RINGS; r >= 1; --r) {
        const float r_outer = (static_cast<float>(r) / static_cast<float>(RINGS)) * wheel_radius;
        const float sat     = static_cast<float>(r) / static_cast<float>(RINGS);

        for (int s = 0; s < SECTORS; ++s) {
            const float a0 = (static_cast<float>(s) / static_cast<float>(SECTORS)) * two_pi;
            const float a1 = (static_cast<float>(s + 1) / static_cast<float>(SECTORS)) * two_pi;
            const float hue_norm = (static_cast<float>(s) + 0.5f) / static_cast<float>(SECTORS);

            godot::PackedVector2Array tri;
            tri.resize(3);
            tri.set(0, wheel_center);
            tri.set(1, wheel_center + godot::Vector2(std::cos(a0), std::sin(a0)) * r_outer);
            tri.set(2, wheel_center + godot::Vector2(std::cos(a1), std::sin(a1)) * r_outer);

            godot::PackedColorArray cols;
            cols.resize(3);
            const godot::Color sector_col = godot::Color::from_hsv(hue_norm, sat, active_val, 1.0f);
            cols.set(0, sector_col);
            cols.set(1, sector_col);
            cols.set(2, sector_col);
            draw_polygon(tri, cols);
        }
    }
    draw_arc(wheel_center, wheel_radius, 0.0f, two_pi, 64, godot::Color(0.55f, 0.60f, 0.68f, 0.95f), 1.5f);

    // Plot connected spline trajectory across the Hue-Saturation wheel
    if (spline.is_valid() && spline->get_point_count() > 0) {
        if (spline->get_section_count() > 0) {
            constexpr int CURVE_SAMPLES = 96;
            godot::Vector2 prev_p;
            for (int i = 0; i <= CURVE_SAMPLES; ++i) {
                const real_t pct = static_cast<real_t>(i) / static_cast<real_t>(CURVE_SAMPLES);
                const godot::Color c = spline->sample_color(pct, true, hsva_mode);
                const float rad = c.get_h() * two_pi;
                const float sat = std::clamp(c.get_s(), 0.0f, 1.0f);
                const godot::Vector2 cur_p = wheel_center + godot::Vector2(std::cos(rad), std::sin(rad)) * (sat * wheel_radius);
                if (i > 0) {
                    draw_line(prev_p, cur_p, godot::Color(0.08f, 0.08f, 0.10f, 0.85f), 3.2f);
                    draw_line(prev_p, cur_p, godot::Color(1.0f, 1.0f, 1.0f, 0.92f), 1.6f);
                }
                prev_p = cur_p;
            }
        }

        // Draw stop handles on the Hue-Saturation wheel
        const int n = spline->get_point_count();
        for (int i = 0; i < n; ++i) {
            const godot::Vector2 wp = _stop_to_wheel_pos(i, wheel_center, wheel_radius);
            const godot::Color sc   = _evaluate_stop_display_color(i);
            const bool is_sel       = (i == selected_stop_index);
            const bool is_hov       = (i == hovered_stop_index);

            draw_circle(
                wp,
                is_sel ? 7.5f : (is_hov ? 6.5f : 5.5f),
                is_sel ? godot::Color(1.0f, 0.92f, 0.22f, 1.0f) : godot::Color(0.10f, 0.10f, 0.12f, 0.95f)
            );
            draw_circle(
                wp,
                is_sel ? 5.2f : 3.8f,
                godot::Color(sc.r, sc.g, sc.b, 1.0f)
            );
        }
    }

    // 3. Draw Value (Z) & Alpha (W) Vertical Tracks for the Selected Stop
    float sel_h = 0.0f;
    float sel_s = 1.0f;
    float sel_v = 1.0f;
    float sel_a = 1.0f;
    if (spline.is_valid() && selected_stop_index >= 0 && selected_stop_index < spline->get_point_count()) {
        const godot::Color sc = _evaluate_stop_display_color(selected_stop_index);
        sel_h = sc.get_h();
        sel_s = sc.get_s();
        sel_v = sc.get_v();
        sel_a = sc.a;
    }

    constexpr int TRACK_STEPS = 32;
    const float step_h = val_rect.size.y / static_cast<float>(TRACK_STEPS);
    for (int i = 0; i < TRACK_STEPS; ++i) {
        const float t_top = 1.0f - (static_cast<float>(i) + 0.5f) / static_cast<float>(TRACK_STEPS);
        const float y     = val_rect.position.y + static_cast<float>(i) * step_h;

        // Value track slice
        draw_rect(
            godot::Rect2(val_rect.position.x, y, val_rect.size.x, step_h + 1.0f),
            godot::Color::from_hsv(sel_h, sel_s, t_top, 1.0f),
            true
        );

        // Alpha track slice
        const godot::Color base_rgb = godot::Color::from_hsv(sel_h, sel_s, sel_v, 1.0f);
        const godot::Color checker  = ((i & 1) == 0) ? godot::Color(0.24f, 0.24f, 0.28f, 1.0f) : godot::Color(0.38f, 0.38f, 0.42f, 1.0f);
        draw_rect(
            godot::Rect2(alp_rect.position.x, y, alp_rect.size.x, step_h + 1.0f),
            checker.lerp(base_rgb, t_top),
            true
        );
    }

    draw_rect(val_rect, godot::Color(0.48f, 0.52f, 0.60f, 1.0f), false, 1.2f);
    draw_rect(alp_rect, godot::Color(0.48f, 0.52f, 0.60f, 1.0f), false, 1.2f);

    const float val_y = val_rect.position.y + (1.0f - std::clamp(sel_v, 0.0f, 1.0f)) * val_rect.size.y;
    const float alp_y = alp_rect.position.y + (1.0f - std::clamp(sel_a, 0.0f, 1.0f)) * alp_rect.size.y;

    draw_rect(
        godot::Rect2(val_rect.position.x - 2.0f, val_y - 3.0f, val_rect.size.x + 4.0f, 6.0f),
        godot::Color(1.0f, 0.92f, 0.25f, 1.0f),
        false,
        2.0f
    );
    draw_rect(
        godot::Rect2(alp_rect.position.x - 2.0f, alp_y - 3.0f, alp_rect.size.x + 4.0f, 6.0f),
        godot::Color(1.0f, 0.92f, 0.25f, 1.0f),
        false,
        2.0f
    );
}

void ColorSplineWheelCanvas::_gui_input(const godot::Ref<godot::InputEvent> &p_event) {
    if (spline.is_null() || p_event.is_null()) {
        return;
    }

    const godot::Rect2 bar_rect = _get_gradient_bar_rect();
    const godot::Rect2 whl_rect = _get_wheel_rect();
    const godot::Rect2 val_rect = _get_value_track_rect();
    const godot::Rect2 alp_rect = _get_alpha_track_rect();

    const godot::Vector2 wheel_center = whl_rect.position + whl_rect.size * 0.5f;
    const float wheel_radius = std::max(16.0f, whl_rect.size.x * 0.5f - 6.0f);

    if (const auto *mb = godot::Object::cast_to<godot::InputEventMouseButton>(p_event.ptr())) {
        const godot::Vector2 mpos = mb->get_position();

        if (mb->get_button_index() == godot::MOUSE_BUTTON_LEFT) {
            if (mb->is_pressed()) {
                // 1. Check stop handles on the 2D Hue-Saturation wheel
                const int n = spline->get_point_count();
                int best_wheel_idx = -1;
                float best_dist_sq = 12.0f * 12.0f;
                for (int i = 0; i < n; ++i) {
                    const float d2 = mpos.distance_squared_to(_stop_to_wheel_pos(i, wheel_center, wheel_radius));
                    if (d2 <= best_dist_sq) {
                        best_dist_sq   = d2;
                        best_wheel_idx = i;
                    }
                }

                if (best_wheel_idx >= 0) {
                    set_selected_stop_index(best_wheel_idx);
                    drag_target = DRAG_WHEEL_STOP;
                    drag_start_snapshot = spline->get_snapshot_state();
                    accept_event();
                    return;
                }

                // 2. Check Gradient Bar (double-click inserts a stop; single-click selects/drags stop)
                const godot::Rect2 expanded_bar(
                    bar_rect.position.x - 6.0f,
                    bar_rect.position.y - 2.0f,
                    bar_rect.size.x + 12.0f,
                    bar_rect.size.y + 12.0f
                );
                if (expanded_bar.has_point(mpos)) {
                    const std::vector<real_t> norms = _compute_stop_norms();
                    int hit_stop = -1;
                    float min_dx = 9.0f;
                    for (size_t i = 0; i < norms.size(); ++i) {
                        const float sx = bar_rect.position.x + static_cast<float>(norms[i]) * bar_rect.size.x;
                        const float dx = std::abs(mpos.x - sx);
                        if (dx <= min_dx) {
                            min_dx   = dx;
                            hit_stop = static_cast<int>(i);
                        }
                    }

                    if (mb->is_double_click()) {
                        const float pct = std::clamp((mpos.x - bar_rect.position.x) / std::max(1.0f, bar_rect.size.x), 0.0f, 1.0f);
                        const godot::Vector4 sampled = spline->sample(static_cast<real_t>(pct), true);
                        const godot::Vector2 sec_info = spline->get_section_at_percent(static_cast<real_t>(pct), true);
                        const int sec_idx = std::clamp(static_cast<int>(sec_info.x), 0, std::max(0, spline->get_section_count() - 1));

                        if (spline->get_undo_redo().is_null()) {
                            spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
                        }
                        const godot::Dictionary before = spline->get_snapshot_state();
                        spline->insert_point(sec_idx + 1, sampled, CURVE_CATMULL_ROM);
                        spline->commit_snapshot_action("Insert Color Spline Stop", before, spline->get_snapshot_state());
                        set_selected_stop_index(sec_idx + 1);
                        emit_signal("stops_modified");
                        accept_event();
                        return;
                    }

                    if (hit_stop >= 0) {
                        set_selected_stop_index(hit_stop);
                        drag_target = DRAG_GRADIENT_STOP;
                        drag_start_snapshot = spline->get_snapshot_state();
                        accept_event();
                        return;
                    }
                }

                // 3. Check Value Track
                if (val_rect.grow(4.0f).has_point(mpos) && selected_stop_index >= 0) {
                    drag_target = DRAG_VALUE_TRACK;
                    drag_start_snapshot = spline->get_snapshot_state();
                    const float v = 1.0f - std::clamp((mpos.y - val_rect.position.y) / std::max(1.0f, val_rect.size.y), 0.0f, 1.0f);
                    _set_stop_value_channel(selected_stop_index, v);
                    queue_redraw();
                    accept_event();
                    return;
                }

                // 4. Check Alpha Track
                if (alp_rect.grow(4.0f).has_point(mpos) && selected_stop_index >= 0) {
                    drag_target = DRAG_ALPHA_TRACK;
                    drag_start_snapshot = spline->get_snapshot_state();
                    const float a = 1.0f - std::clamp((mpos.y - alp_rect.position.y) / std::max(1.0f, alp_rect.size.y), 0.0f, 1.0f);
                    _set_stop_alpha_channel(selected_stop_index, a);
                    queue_redraw();
                    accept_event();
                    return;
                }

                // 5. Clicking inside the wheel moves the currently selected stop to that Hue/Saturation
                if (mpos.distance_to(wheel_center) <= wheel_radius + 4.0f && selected_stop_index >= 0) {
                    drag_target = DRAG_WHEEL_STOP;
                    drag_start_snapshot = spline->get_snapshot_state();
                    _set_stop_from_wheel_pos(selected_stop_index, mpos, wheel_center, wheel_radius);
                    queue_redraw();
                    accept_event();
                    return;
                }
            } else if (drag_target != DRAG_NONE) {
                if (spline->get_undo_redo().is_null()) {
                    spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
                }
                const godot::Dictionary after = spline->get_snapshot_state();
                if (after != drag_start_snapshot) {
                    spline->commit_snapshot_action("Edit Color Spline Stop", drag_start_snapshot, after);
                }
                drag_target = DRAG_NONE;
                queue_redraw();
                accept_event();
                return;
            }
        } else if (mb->get_button_index() == godot::MOUSE_BUTTON_RIGHT && mb->is_pressed()) {
            // Right-clicking a stop on the wheel or gradient bar deletes it (when > 2 stops exist)
            if (spline->get_point_count() > 2) {
                int hit_idx = -1;
                for (int i = 0; i < spline->get_point_count(); ++i) {
                    if (mpos.distance_to(_stop_to_wheel_pos(i, wheel_center, wheel_radius)) <= 10.0f) {
                        hit_idx = i;
                        break;
                    }
                }
                if (hit_idx < 0) {
                    const std::vector<real_t> norms = _compute_stop_norms();
                    for (size_t i = 0; i < norms.size(); ++i) {
                        const float sx = bar_rect.position.x + static_cast<float>(norms[i]) * bar_rect.size.x;
                        if (std::abs(mpos.x - sx) <= 8.0f && mpos.y >= bar_rect.position.y && mpos.y <= bar_rect.position.y + bar_rect.size.y + 10.0f) {
                            hit_idx = static_cast<int>(i);
                            break;
                        }
                    }
                }
                if (hit_idx >= 0) {
                    if (spline->get_undo_redo().is_null()) {
                        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
                    }
                    spline->commit_remove_point(hit_idx);
                    set_selected_stop_index(std::clamp(hit_idx, 0, spline->get_point_count() - 1));
                    emit_signal("stops_modified");
                    accept_event();
                    return;
                }
            }
        }
    } else if (const auto *mm = godot::Object::cast_to<godot::InputEventMouseMotion>(p_event.ptr())) {
        const godot::Vector2 mpos = mm->get_position();
        if (drag_target == DRAG_WHEEL_STOP && selected_stop_index >= 0) {
            _set_stop_from_wheel_pos(selected_stop_index, mpos, wheel_center, wheel_radius);
            queue_redraw();
            accept_event();
            return;
        }
        if (drag_target == DRAG_GRADIENT_STOP && selected_stop_index >= 0) {
            const float norm_x = std::clamp((mpos.x - bar_rect.position.x) / std::max(1.0f, bar_rect.size.x), 0.0f, 1.0f);
            _set_stop_norm_weight(selected_stop_index, norm_x);
            queue_redraw();
            accept_event();
            return;
        }
        if (drag_target == DRAG_VALUE_TRACK && selected_stop_index >= 0) {
            const float v = 1.0f - std::clamp((mpos.y - val_rect.position.y) / std::max(1.0f, val_rect.size.y), 0.0f, 1.0f);
            _set_stop_value_channel(selected_stop_index, v);
            queue_redraw();
            accept_event();
            return;
        }
        if (drag_target == DRAG_ALPHA_TRACK && selected_stop_index >= 0) {
            const float a = 1.0f - std::clamp((mpos.y - alp_rect.position.y) / std::max(1.0f, alp_rect.size.y), 0.0f, 1.0f);
            _set_stop_alpha_channel(selected_stop_index, a);
            queue_redraw();
            accept_event();
            return;
        }

        // Hover feedback
        int hov = -1;
        for (int i = 0; i < spline->get_point_count(); ++i) {
            if (mpos.distance_to(_stop_to_wheel_pos(i, wheel_center, wheel_radius)) <= 10.0f) {
                hov = i;
                break;
            }
        }
        if (hov != hovered_stop_index) {
            hovered_stop_index = hov;
            queue_redraw();
        }
    }
}

// ============================================================================
// ColorSplineControl Implementation
// ============================================================================

void ColorSplineControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_spline_4d", "spline"), &ColorSplineControl::set_spline_4d);
    ClassDB::bind_method(D_METHOD("get_spline_4d"), &ColorSplineControl::get_spline_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_spline_4d",
        "get_spline_4d"
    );

    ClassDB::bind_method(
        D_METHOD("apply_color_stops_via_create_gradient", "colors"),
        &ColorSplineControl::apply_color_stops_via_create_gradient
    );
}

ColorSplineControl::ColorSplineControl() {
    add_theme_constant_override("separation", 6);
    _build_ui();
}

ColorSplineControl::~ColorSplineControl() {
    if (spline.is_valid()) {
        const godot::Callable cb = callable_mp(this, &ColorSplineControl::_on_spline_changed);
        if (spline->is_connected("changed", cb)) {
            spline->disconnect("changed", cb);
        }
    }
}

void ColorSplineControl::_build_ui() {
    using namespace godot;

    // 1. Top Toolbar: Mode Toggles & HSVA Gradient Presets
    HBoxContainer *top_bar = memnew(HBoxContainer);
    top_bar->add_theme_constant_override("separation", 6);
    add_child(top_bar);

    hsva_mode_check = memnew(CheckBox);
    hsva_mode_check->set_text("HSVA Space");
    hsva_mode_check->set_pressed(true);
    hsva_mode_check->set_tooltip_text("Interpolate stops in perceptual HSVA space (Hue[0..360], Sat[0..1], Val[0..1], Alpha[0..1]) via Spline4D::create_gradient().");
    hsva_mode_check->connect("toggled", callable_mp(this, &ColorSplineControl::_on_hsva_toggled));
    top_bar->add_child(hsva_mode_check);

    wrap_check = memnew(CheckBox);
    wrap_check->set_text("Wrap Loop");
    wrap_check->connect("toggled", callable_mp(this, &ColorSplineControl::_on_wrap_toggled));
    top_bar->add_child(wrap_check);

    Button *sunset_btn = memnew(Button);
    sunset_btn->set_text("Sunset");
    sunset_btn->connect("pressed", callable_mp(this, &ColorSplineControl::_on_preset_sunset));
    top_bar->add_child(sunset_btn);

    Button *spec_btn = memnew(Button);
    spec_btn->set_text("Spectrum");
    spec_btn->connect("pressed", callable_mp(this, &ColorSplineControl::_on_preset_spectrum));
    top_bar->add_child(spec_btn);

    Button *neon_btn = memnew(Button);
    neon_btn->set_text("Neon");
    neon_btn->connect("pressed", callable_mp(this, &ColorSplineControl::_on_preset_cyber_neon));
    top_bar->add_child(neon_btn);

    // 2. Interactive Gradient Bar + 2D Hue-Saturation Wheel + Value/Alpha Track Canvas
    wheel_canvas = memnew(ColorSplineWheelCanvas);
    wheel_canvas->connect("stop_selected", callable_mp(this, &ColorSplineControl::_on_canvas_stop_selected));
    wheel_canvas->connect("stops_modified", callable_mp(this, &ColorSplineControl::_refresh_from_resource));
    add_child(wheel_canvas);

    // 3. Active Stop Inspector Row
    HBoxContainer *stop_bar = memnew(HBoxContainer);
    stop_bar->add_theme_constant_override("separation", 4);
    add_child(stop_bar);

    selected_stop_label = memnew(Label);
    selected_stop_label->set_text("Stop #0:");
    stop_bar->add_child(selected_stop_label);

    stop_color_picker = memnew(ColorPickerButton);
    stop_color_picker->set_custom_minimum_size(Vector2(38.0f, 24.0f));
    stop_color_picker->set_edit_alpha(true);
    stop_color_picker->connect("color_changed", callable_mp(this, &ColorSplineControl::_on_stop_color_picked));
    stop_bar->add_child(stop_color_picker);

    auto make_spin = [&](const char *p_prefix, double p_max, double p_step) -> SpinBox * {
        SpinBox *sb = memnew(SpinBox);
        sb->set_prefix(p_prefix);
        sb->set_min(0.0);
        sb->set_max(p_max);
        sb->set_step(p_step);
        sb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        sb->connect("value_changed", callable_mp(this, &ColorSplineControl::_on_channel_spin_changed));
        stop_bar->add_child(sb);
        return sb;
    };

    channel_x_spin = make_spin("H:", 360.0, 1.0);
    channel_y_spin = make_spin("S:", 1.0, 0.01);
    channel_z_spin = make_spin("V:", 1.0, 0.01);
    channel_w_spin = make_spin("A:", 1.0, 0.01);

    Button *add_btn = memnew(Button);
    add_btn->set_text("+");
    add_btn->set_tooltip_text("Append a new color stop.");
    add_btn->connect("pressed", callable_mp(this, &ColorSplineControl::_on_add_stop_pressed));
    stop_bar->add_child(add_btn);

    remove_stop_btn = memnew(Button);
    remove_stop_btn->set_text("-");
    remove_stop_btn->set_tooltip_text("Remove the selected color stop.");
    remove_stop_btn->connect("pressed", callable_mp(this, &ColorSplineControl::_on_remove_stop_pressed));
    stop_bar->add_child(remove_stop_btn);
}

void ColorSplineControl::set_spline_4d(const godot::Ref<Spline4DResource> &p_spline) {
    if (spline == p_spline) {
        return;
    }
    const godot::Callable cb = callable_mp(this, &ColorSplineControl::_on_spline_changed);
    if (spline.is_valid() && spline->is_connected("changed", cb)) {
        spline->disconnect("changed", cb);
    }
    spline = p_spline;
    if (wheel_canvas) {
        wheel_canvas->set_spline(spline);
    }
    if (spline.is_valid()) {
        if (!spline->is_connected("changed", cb)) {
            spline->connect("changed", cb);
        }
    }
    _refresh_from_resource();
}

void ColorSplineControl::_on_spline_changed() {
    _refresh_from_resource();
}

void ColorSplineControl::_on_canvas_stop_selected(int p_stop_index) {
    (void)p_stop_index;
    _refresh_from_resource();
}

void ColorSplineControl::_refresh_from_resource() {
    if (updating_ui) {
        return;
    }
    updating_ui = true;

    const bool has_spline = spline.is_valid();
    const bool is_hsva    = wheel_canvas ? wheel_canvas->get_hsva_mode() : true;

    if (hsva_mode_check) {
        hsva_mode_check->set_pressed_no_signal(is_hsva);
    }
    if (wrap_check && has_spline) {
        wrap_check->set_pressed_no_signal(spline->get_wrap());
    }

    if (channel_x_spin) {
        channel_x_spin->set_prefix(is_hsva ? "H:" : "R:");
        channel_x_spin->set_max(is_hsva ? 360.0 : 1.0);
        channel_x_spin->set_step(is_hsva ? 1.0 : 0.01);
    }
    if (channel_y_spin) channel_y_spin->set_prefix(is_hsva ? "S:" : "G:");
    if (channel_z_spin) channel_z_spin->set_prefix(is_hsva ? "V:" : "B:");
    if (channel_w_spin) channel_w_spin->set_prefix("A:");

    const int stop_idx = wheel_canvas ? wheel_canvas->get_selected_stop_index() : -1;
    const int n        = has_spline ? spline->get_point_count() : 0;

    if (has_spline && stop_idx >= 0 && stop_idx < n) {
        if (selected_stop_label) {
            selected_stop_label->set_text(godot::String("Stop #") + godot::String::num_int64(stop_idx) + ":");
        }
        const godot::Vector4 pos = spline->get_point_position(stop_idx);
        if (channel_x_spin) channel_x_spin->set_value_no_signal(pos.x);
        if (channel_y_spin) channel_y_spin->set_value_no_signal(pos.y);
        if (channel_z_spin) channel_z_spin->set_value_no_signal(pos.z);
        if (channel_w_spin) channel_w_spin->set_value_no_signal(pos.w);

        if (stop_color_picker) {
            if (is_hsva) {
                float h = (pos.x > 1.0f) ? std::fmod(pos.x / 360.0f, 1.0f) : std::fmod(pos.x, 1.0f);
                if (h < 0.0f) h += 1.0f;
                stop_color_picker->set_pick_color(
                    godot::Color::from_hsv(
                        h,
                        std::clamp(pos.y, 0.0f, 1.0f),
                        std::clamp(pos.z, 0.0f, 1.0f),
                        std::clamp(pos.w, 0.0f, 1.0f)
                    )
                );
            } else {
                stop_color_picker->set_pick_color(godot::Color(pos.x, pos.y, pos.z, pos.w));
            }
        }
    }

    if (remove_stop_btn) {
        remove_stop_btn->set_disabled(!has_spline || n <= 2);
    }
    if (wheel_canvas) {
        wheel_canvas->queue_redraw();
    }

    updating_ui = false;
}

void ColorSplineControl::_on_hsva_toggled(bool p_hsva) {
    if (updating_ui) return;
    if (wheel_canvas) {
        wheel_canvas->set_hsva_mode(p_hsva);
    }
    if (spline.is_valid()) {
        spline->set_color_gradient_mode(p_hsva);
    }
    _refresh_from_resource();
}

void ColorSplineControl::_on_wrap_toggled(bool p_wrap) {
    if (updating_ui || spline.is_null()) return;
    if (spline->get_undo_redo().is_null()) {
        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
    }
    spline->commit_set_wrap(p_wrap);
}

void ColorSplineControl::_on_stop_color_picked(const godot::Color &p_color) {
    if (updating_ui || spline.is_null() || !wheel_canvas) return;
    const int idx = wheel_canvas->get_selected_stop_index();
    if (idx < 0 || idx >= spline->get_point_count()) return;

    if (spline->get_undo_redo().is_null()) {
        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
    }
    const bool is_hsva = wheel_canvas->get_hsva_mode();
    const godot::Vector4 next_pos = is_hsva
        ? godot::Vector4(p_color.get_h() * 360.0f, p_color.get_s(), p_color.get_v(), p_color.a)
        : godot::Vector4(p_color.r, p_color.g, p_color.b, p_color.a);
    spline->commit_set_point_position(idx, next_pos);
}

void ColorSplineControl::_on_channel_spin_changed(double p_val) {
    (void)p_val;
    if (updating_ui || spline.is_null() || !wheel_canvas) return;
    const int idx = wheel_canvas->get_selected_stop_index();
    if (idx < 0 || idx >= spline->get_point_count()) return;

    if (spline->get_undo_redo().is_null()) {
        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
    }
    const godot::Vector4 next_pos(
        channel_x_spin ? static_cast<float>(channel_x_spin->get_value()) : 0.0f,
        channel_y_spin ? static_cast<float>(channel_y_spin->get_value()) : 0.0f,
        channel_z_spin ? static_cast<float>(channel_z_spin->get_value()) : 0.0f,
        channel_w_spin ? static_cast<float>(channel_w_spin->get_value()) : 1.0f
    );
    spline->commit_set_point_position(idx, next_pos);
}

void ColorSplineControl::_on_add_stop_pressed() {
    if (spline.is_null()) return;
    if (spline->get_undo_redo().is_null()) {
        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
    }
    const int n = spline->get_point_count();
    const godot::Vector4 new_pt = (n > 0)
        ? spline->get_point_position(n - 1)
        : (wheel_canvas && wheel_canvas->get_hsva_mode()
            ? godot::Vector4(180.0f, 0.85f, 0.95f, 1.0f)
            : godot::Vector4(0.2f, 0.8f, 0.95f, 1.0f));
    spline->commit_add_point(new_pt, CURVE_CATMULL_ROM);
    if (wheel_canvas) {
        wheel_canvas->set_selected_stop_index(spline->get_point_count() - 1);
    }
}

void ColorSplineControl::_on_remove_stop_pressed() {
    if (spline.is_null() || spline->get_point_count() <= 2 || !wheel_canvas) return;
    const int idx = wheel_canvas->get_selected_stop_index();
    if (idx < 0 || idx >= spline->get_point_count()) return;

    if (spline->get_undo_redo().is_null()) {
        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
    }
    spline->commit_remove_point(idx);
    wheel_canvas->set_selected_stop_index(std::clamp(idx, 0, spline->get_point_count() - 1));
}

void ColorSplineControl::apply_color_stops_via_create_gradient(const godot::PackedColorArray &p_colors) {
    if (spline.is_null()) return;
    if (spline->get_undo_redo().is_null()) {
        spline->set_undo_redo(IdeamSplinesPlugin::create_undo_redo_wrapper());
    }
    const bool is_hsva = wheel_canvas ? wheel_canvas->get_hsva_mode() : true;
    const godot::Dictionary before = spline->get_snapshot_state();
    spline->setup_color_gradient(p_colors, is_hsva);
    spline->commit_snapshot_action("Apply Color Spline Gradient Preset", before, spline->get_snapshot_state());
    if (wheel_canvas) {
        wheel_canvas->set_selected_stop_index(0);
    }
    _refresh_from_resource();
}

void ColorSplineControl::_on_preset_sunset() {
    godot::PackedColorArray cols;
    cols.push_back(godot::Color(0.16f, 0.08f, 0.36f, 1.0f));
    cols.push_back(godot::Color(0.76f, 0.18f, 0.42f, 1.0f));
    cols.push_back(godot::Color(0.98f, 0.52f, 0.18f, 1.0f));
    cols.push_back(godot::Color(1.00f, 0.88f, 0.38f, 1.0f));
    apply_color_stops_via_create_gradient(cols);
}

void ColorSplineControl::_on_preset_spectrum() {
    godot::PackedColorArray cols;
    cols.push_back(godot::Color::from_hsv(0.00f, 0.90f, 0.96f, 1.0f));
    cols.push_back(godot::Color::from_hsv(0.18f, 0.90f, 0.96f, 1.0f));
    cols.push_back(godot::Color::from_hsv(0.38f, 0.88f, 0.92f, 1.0f));
    cols.push_back(godot::Color::from_hsv(0.58f, 0.90f, 0.96f, 1.0f));
    cols.push_back(godot::Color::from_hsv(0.80f, 0.88f, 0.96f, 1.0f));
    apply_color_stops_via_create_gradient(cols);
}

void ColorSplineControl::_on_preset_cyber_neon() {
    godot::PackedColorArray cols;
    cols.push_back(godot::Color(0.05f, 0.92f, 0.98f, 1.0f));
    cols.push_back(godot::Color(0.52f, 0.24f, 0.98f, 1.0f));
    cols.push_back(godot::Color(0.98f, 0.18f, 0.68f, 1.0f));
    cols.push_back(godot::Color(0.98f, 0.92f, 0.20f, 1.0f));
    apply_color_stops_via_create_gradient(cols);
}

} // namespace ideam::godot_ext