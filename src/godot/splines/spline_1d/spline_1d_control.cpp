#include "spline_1d_control.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// Spline1DEqualizerCanvas Implementation (Specializing SplineCanvasBase)
// ============================================================================

void Spline1DEqualizerCanvas::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "threshold_value_changed",
        PropertyInfo(Variant::FLOAT, "threshold_value")
    ));

    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &Spline1DEqualizerCanvas::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &Spline1DEqualizerCanvas::get_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_spline",
        "get_spline"
    );

    ClassDB::bind_method(D_METHOD("set_view_range_y", "min_y", "max_y"), &Spline1DEqualizerCanvas::set_view_range_y);
    ClassDB::bind_method(D_METHOD("get_view_min_y"), &Spline1DEqualizerCanvas::get_view_min_y);
    ClassDB::bind_method(D_METHOD("get_view_max_y"), &Spline1DEqualizerCanvas::get_view_max_y);
    ClassDB::bind_method(
        D_METHOD("fit_y_range_to_spline", "padding_ratio"),
        &Spline1DEqualizerCanvas::fit_y_range_to_spline,
        DEFVAL(0.15)
    );

    ClassDB::bind_method(D_METHOD("set_show_threshold_solver", "show"), &Spline1DEqualizerCanvas::set_show_threshold_solver);
    ClassDB::bind_method(D_METHOD("get_show_threshold_solver"), &Spline1DEqualizerCanvas::get_show_threshold_solver);

    ClassDB::bind_method(D_METHOD("set_threshold_value", "value"), &Spline1DEqualizerCanvas::set_threshold_value);
    ClassDB::bind_method(D_METHOD("get_threshold_value"), &Spline1DEqualizerCanvas::get_threshold_value);

    ClassDB::bind_method(D_METHOD("set_redistribute_weights", "redistribute"), &Spline1DEqualizerCanvas::set_redistribute_weights);
    ClassDB::bind_method(D_METHOD("get_redistribute_weights"), &Spline1DEqualizerCanvas::get_redistribute_weights);
}

Spline1DEqualizerCanvas::Spline1DEqualizerCanvas() {
    // Configure SplineCanvasBase for 1D Graphic Equalizer mode:
    //   - X axis is fixed to normalized cumulative weight [0, 1]
    //   - Y axis represents the 1D float value (supports vertical pan & wheel zoom)
    //   - Anchors render as horizontal fader caps on vertical tracks and drag strictly in Y
    //   - Reserve 34px bottom margin for the Section Weight Equalizer Strip
    set_point_style(POINT_STYLE_FADER_CAP);
    set_anchor_drag_constraint(DRAG_AXIS_Y_ONLY);
    set_lock_aspect_ratio(false);
    set_pan_zoom_axes(false, true, false, true);
    set_plot_margins(44.0f, 18.0f, 14.0f, 34.0f);
    set_view_bounds(godot::Vector2(0.0f, -0.2f), godot::Vector2(1.0f, 1.2f));
}

Spline1DEqualizerCanvas::~Spline1DEqualizerCanvas() = default;

void Spline1DEqualizerCanvas::set_spline(const godot::Ref<Spline1DResource> &p_spline) {
    set_spline_resource(p_spline);
}

godot::Ref<Spline1DResource> Spline1DEqualizerCanvas::get_spline() const {
    return godot::Ref<Spline1DResource>(godot::Object::cast_to<Spline1DResource>(spline_resource.ptr()));
}

void Spline1DEqualizerCanvas::set_view_range_y(real_t p_min_y, real_t p_max_y) {
    set_view_bounds(
        godot::Vector2(0.0f, static_cast<float>(p_min_y)),
        godot::Vector2(1.0f, static_cast<float>(p_max_y))
    );
}

void Spline1DEqualizerCanvas::fit_y_range_to_spline(real_t p_padding_ratio) {
    fit_view_to_data(false, true, p_padding_ratio);
}

void Spline1DEqualizerCanvas::set_show_threshold_solver(bool p_show) {
    show_threshold_solver = p_show;
    queue_redraw();
}

void Spline1DEqualizerCanvas::set_threshold_value(real_t p_value) {
    threshold_value = p_value;
    queue_redraw();
}

void Spline1DEqualizerCanvas::set_redistribute_weights(bool p_redistribute) {
    redistribute_weights = p_redistribute;
    queue_redraw();
}

godot::Rect2 Spline1DEqualizerCanvas::_get_weight_strip_rect(const godot::Rect2 &p_plot) const {
    return godot::Rect2(p_plot.position.x, p_plot.position.y + p_plot.size.y + 4.0f, p_plot.size.x, 24.0f);
}

std::vector<real_t> Spline1DEqualizerCanvas::_compute_fader_norms() const {
    std::vector<real_t> norms;
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return norms;

    const int pt_count  = s1d->get_point_count();
    const int sec_count = s1d->get_section_count();
    if (pt_count <= 0) return norms;

    if (pt_count == 1 || sec_count <= 0) {
        norms.push_back(static_cast<real_t>(0.5));
        return norms;
    }

    const real_t total_w = std::max(static_cast<real_t>(1e-6), s1d->get_total_weight());
    norms.reserve(static_cast<size_t>(sec_count + 1));
    real_t accum = static_cast<real_t>(0.0);
    norms.push_back(static_cast<real_t>(0.0));

    for (int s = 0; s < sec_count; ++s) {
        accum += std::max(static_cast<real_t>(0.0001), s1d->get_section_weight(s));
        norms.push_back(std::clamp(accum / total_w, static_cast<real_t>(0.0), static_cast<real_t>(1.0)));
    }
    norms.back() = static_cast<real_t>(1.0);
    return norms;
}

// --- SplineCanvasBase Projection Overrides ---

godot::Vector2 Spline1DEqualizerCanvas::_get_point_world_pos(int p_index) const {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null() || p_index < 0 || p_index >= s1d->get_point_count()) {
        return godot::Vector2(0.0f, 0.0f);
    }
    const std::vector<real_t> norms = _compute_fader_norms();
    const float x = (p_index < static_cast<int>(norms.size()))
        ? static_cast<float>(norms[static_cast<size_t>(p_index)])
        : 0.0f;
    const float y = static_cast<float>(s1d->get_point_position(p_index));
    return godot::Vector2(x, y);
}

void Spline1DEqualizerCanvas::_set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return;
    s1d->set_point_position(p_index, static_cast<real_t>(p_world_pos.y));
}

bool Spline1DEqualizerCanvas::_has_wrap_ghost_endpoint(godot::Vector2 &r_ghost_world_pos) const {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null() || !s1d->get_wrap() || s1d->get_point_count() < 2) {
        return false;
    }
    r_ghost_world_pos = godot::Vector2(1.0f, static_cast<float>(s1d->get_point_position(0)));
    return true;
}

godot::Vector2 Spline1DEqualizerCanvas::_get_back_handle_screen_pos(
    int p_index,
    const godot::Rect2 &p_plot
) const {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return godot::Vector2();
    const godot::Vector2 anchor_w   = _get_point_world_pos(p_index);
    const godot::Vector2 anchor_scr = world_to_screen(anchor_w, p_plot);
    const float bh_val              = anchor_w.y + static_cast<float>(s1d->get_point_back_handle(p_index));
    const float bhy                 = world_to_screen(godot::Vector2(anchor_w.x, bh_val), p_plot).y;
    const float bhx                 = std::max(p_plot.position.x, anchor_scr.x - 18.0f);
    return godot::Vector2(bhx, bhy);
}

void Spline1DEqualizerCanvas::_set_back_handle_from_screen_pos(
    int p_index,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return;
    const real_t anchor_v = s1d->get_point_position(p_index);
    const real_t target_v = static_cast<real_t>(screen_to_world(p_screen_pos, p_plot).y);
    s1d->set_point_back_handle(p_index, target_v - anchor_v, true);
}

godot::Vector2 Spline1DEqualizerCanvas::_get_fore_handle_screen_pos(
    int p_index,
    const godot::Rect2 &p_plot
) const {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return godot::Vector2();
    const godot::Vector2 anchor_w   = _get_point_world_pos(p_index);
    const godot::Vector2 anchor_scr = world_to_screen(anchor_w, p_plot);
    const float fh_val              = anchor_w.y + static_cast<float>(s1d->get_point_fore_handle(p_index));
    const float fhy                 = world_to_screen(godot::Vector2(anchor_w.x, fh_val), p_plot).y;
    const float fhx                 = std::min(p_plot.position.x + p_plot.size.x, anchor_scr.x + 18.0f);
    return godot::Vector2(fhx, fhy);
}

void Spline1DEqualizerCanvas::_set_fore_handle_from_screen_pos(
    int p_index,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return;
    const real_t anchor_v = s1d->get_point_position(p_index);
    const real_t target_v = static_cast<real_t>(screen_to_world(p_screen_pos, p_plot).y);
    s1d->set_point_fore_handle(p_index, target_v - anchor_v, true);
}

godot::PackedVector2Array Spline1DEqualizerCanvas::_sample_curve_screen_points(
    const godot::Rect2 &p_plot
) const {
    godot::PackedVector2Array pts;
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null() || s1d->get_section_count() <= 0) return pts;

    constexpr int SAMPLES = 160;
    pts.resize(SAMPLES + 1);
    for (int i = 0; i <= SAMPLES; ++i) {
        const real_t pct = static_cast<real_t>(i) / static_cast<real_t>(SAMPLES);
        const real_t val = s1d->sample(pct, true);
        pts.set(i, world_to_screen(godot::Vector2(static_cast<float>(pct), static_cast<float>(val)), p_plot));
    }
    return pts;
}

bool Spline1DEqualizerCanvas::_compute_world_bounds(godot::Rect2 &r_bounds) const {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null() || s1d->get_point_count() <= 0) return false;
    const float min_v = static_cast<float>(s1d->get_min_value(true));
    const float max_v = static_cast<float>(s1d->get_max_value(true));
    r_bounds = godot::Rect2(0.0f, min_v, 1.0f, max_v - min_v);
    return true;
}

void Spline1DEqualizerCanvas::_on_double_click_plot(
    const godot::Vector2 &p_world_pos,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    (void)p_screen_pos;
    (void)p_plot;
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return;

    const int pt_count  = s1d->get_point_count();
    const int sec_count = s1d->get_section_count();
    const real_t clicked_val  = static_cast<real_t>(p_world_pos.y);
    const real_t clicked_norm = std::clamp(
        static_cast<real_t>(p_world_pos.x),
        static_cast<real_t>(0.0),
        static_cast<real_t>(1.0)
    );

    if (pt_count < 2 || sec_count == 0) {
        s1d->commit_add_point(clicked_val);
        return;
    }

    const std::vector<real_t> norms = _compute_fader_norms();
    int target_sec = 0;
    for (int s = 0; s < sec_count && s + 1 < static_cast<int>(norms.size()); ++s) {
        if (clicked_norm >= norms[static_cast<size_t>(s)] &&
            clicked_norm <= norms[static_cast<size_t>(s + 1)]) {
            target_sec = s;
            break;
        }
    }

    const real_t orig_w = s1d->get_section_weight(target_sec);
    const SplineCurveType ctype = s1d->get_section_curve_type(target_sec);
    if (target_sec + 1 < pt_count) {
        s1d->commit_insert_point(target_sec + 1, clicked_val, ctype, orig_w * static_cast<real_t>(0.5));
    } else {
        s1d->commit_add_point(clicked_val, ctype, orig_w * static_cast<real_t>(0.5));
    }
}

void Spline1DEqualizerCanvas::_draw_plot_background_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return;

    const int sec_count = s1d->get_section_count();
    const std::vector<real_t> norms = _compute_fader_norms();

    // Alternating / selected vertical section columns inside plot
    for (int s = 0; s < sec_count && s + 1 < static_cast<int>(norms.size()); ++s) {
        const float x0 = world_to_screen(Vector2(static_cast<float>(norms[static_cast<size_t>(s)]), 0.0f), p_plot).x;
        const float x1 = world_to_screen(Vector2(static_cast<float>(norms[static_cast<size_t>(s + 1)]), 0.0f), p_plot).x;
        const float w  = std::max(1.0f, x1 - x0);

        if (s == selected_section_index) {
            draw_rect(Rect2(x0, p_plot.position.y, w, p_plot.size.y), Color(0.24f, 0.42f, 0.66f, 0.18f), true);
        } else if (s % 2 == 1) {
            draw_rect(Rect2(x0, p_plot.position.y, w, p_plot.size.y), Color(0.18f, 0.20f, 0.25f, 0.25f), true);
        }
    }
}

void Spline1DEqualizerCanvas::_draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return;

    const Rect2 w_strip = _get_weight_strip_rect(p_plot);
    const int sec_count = s1d->get_section_count();
    const std::vector<real_t> norms = _compute_fader_norms();
    Ref<Font> font = get_theme_default_font();

    // 1. Draw Bottom Section Weight Equalizer Strip
    draw_rect(w_strip, Color(0.10f, 0.12f, 0.15f, 1.0f), true);
    for (int s = 0; s < sec_count && s + 1 < static_cast<int>(norms.size()); ++s) {
        const float x0 = world_to_screen(Vector2(static_cast<float>(norms[static_cast<size_t>(s)]), 0.0f), p_plot).x;
        const float x1 = world_to_screen(Vector2(static_cast<float>(norms[static_cast<size_t>(s + 1)]), 0.0f), p_plot).x;
        const float w  = std::max(1.0f, x1 - x0);

        const bool is_sec_sel = (s == selected_section_index);
        const bool is_sec_hov = (s == hovered_section_index);

        const Rect2 seg_rect(x0 + 1.0f, w_strip.position.y + 2.0f, std::max(2.0f, w - 2.0f), w_strip.size.y - 4.0f);
        const Color seg_col = is_sec_sel
            ? Color(0.28f, 0.56f, 0.88f, 0.95f)
            : (is_sec_hov ? Color(0.24f, 0.40f, 0.62f, 0.90f) : Color(0.19f, 0.24f, 0.32f, 0.90f));
        draw_rect(seg_rect, seg_col, true);
        draw_rect(seg_rect, Color(0.42f, 0.62f, 0.88f, 0.85f), false, 1.0f);

        if (font.is_valid() && w >= 42.0f) {
            const double sec_w = static_cast<double>(s1d->get_section_weight(s));
            const String lbl   = String("S") + String::num_int64(s) + String(" W:") + String::num(sec_w, 2);
            draw_string(
                font,
                Vector2(x0 + 6.0f, w_strip.position.y + 16.0f),
                lbl,
                HORIZONTAL_ALIGNMENT_LEFT,
                w - 10.0f,
                10,
                Color(0.92f, 0.95f, 1.0f, 0.95f)
            );
        }

        // Draggable Section Weight Divider Handle at x1
        const Rect2 grip_rect(x1 - 3.0f, w_strip.position.y, 6.0f, w_strip.size.y);
        const bool active_grip = (base_drag_state == BASE_DRAG_CUSTOM &&
                                  custom_drag_submode == SUBDRAG_SECTION_WEIGHT &&
                                  drag_target_index == s);
        draw_rect(
            grip_rect,
            active_grip ? Color(1.0f, 0.84f, 0.32f, 1.0f) : Color(0.58f, 0.78f, 1.0f, 0.95f),
            true
        );
    }
    draw_rect(w_strip, Color(0.28f, 0.33f, 0.40f, 1.0f), false, 1.0f);

    // 2. Optional Root-Solver Threshold Line (`get_percents_at_value`)
    if (show_threshold_solver) {
        const float ty = world_to_screen(Vector2(0.0f, static_cast<float>(threshold_value)), p_plot).y;
        if (ty >= p_plot.position.y && ty <= p_plot.position.y + p_plot.size.y) {
            draw_dashed_line(
                Vector2(p_plot.position.x, ty),
                Vector2(p_plot.position.x + p_plot.size.x, ty),
                Color(1.0f, 0.44f, 0.66f, 0.92f),
                1.5f,
                5.0f
            );
            const PackedFloat32Array roots = s1d->get_percents_at_value(threshold_value, 16, true);
            for (int r = 0; r < roots.size(); ++r) {
                const float rx = world_to_screen(Vector2(roots[r], 0.0f), p_plot).x;
                draw_circle(Vector2(rx, ty), 5.5f, Color(1.0f, 0.35f, 0.62f, 1.0f));
                draw_circle(Vector2(rx, ty), 2.5f, Color(1.0f, 0.95f, 0.98f, 1.0f));
            }
            if (font.is_valid()) {
                const String solver_txt = String("Target = ") +
                    String::num(static_cast<double>(threshold_value), 2) +
                    String(" (") + String::num_int64(roots.size()) + String(" roots)");
                draw_string(
                    font,
                    Vector2(p_plot.position.x + 8.0f, std::max(p_plot.position.y + 12.0f, ty - 5.0f)),
                    solver_txt,
                    HORIZONTAL_ALIGNMENT_LEFT,
                    -1.0f,
                    10,
                    Color(1.0f, 0.62f, 0.78f, 0.98f)
                );
            }
        }
    }
}

bool Spline1DEqualizerCanvas::_try_begin_custom_drag(
    const godot::Ref<godot::InputEvent> &p_event,
    const godot::Rect2 &p_plot
) {
    using namespace godot;
    Ref<InputEventMouseButton> mb = p_event;
    Ref<Spline1DResource> s1d     = get_spline();
    if (mb.is_null() || s1d.is_null()) return false;

    const Vector2 mpos  = mb->get_position();
    const Rect2 w_strip = _get_weight_strip_rect(p_plot);
    const int sec_count = s1d->get_section_count();
    const std::vector<real_t> norms = _compute_fader_norms();

    // 1. Check Section Weight Divider Grips in the bottom strip (or Alt+Click on fader)
    for (int s = 0; s < sec_count && s + 1 < static_cast<int>(norms.size()); ++s) {
        const float div_x = world_to_screen(Vector2(static_cast<float>(norms[static_cast<size_t>(s + 1)]), 0.0f), p_plot).x;
        const bool in_strip_grip =
            (mpos.y >= w_strip.position.y - 4.0f && mpos.y <= w_strip.position.y + w_strip.size.y + 4.0f) &&
            (std::abs(mpos.x - div_x) <= 8.0f);
        const bool alt_fader_drag = mb->is_alt_pressed() &&
            p_plot.has_point(mpos) && (std::abs(mpos.x - div_x) <= 10.0f);

        if (in_strip_grip || alt_fader_drag) {
            weight_drag_start_w0    = s1d->get_section_weight(s);
            weight_drag_start_w1    = (s + 1 < sec_count)
                ? s1d->get_section_weight(s + 1)
                : static_cast<real_t>(0.0);
            weight_drag_start_total = std::max(static_cast<real_t>(1e-5), s1d->get_total_weight());

            _begin_snapshot_drag(
                BASE_DRAG_CUSTOM,
                s,
                mpos,
                "Drag 1D Spline Section Weight",
                SUBDRAG_SECTION_WEIGHT
            );
            set_selected_section_index(s);
            emit_signal("section_selected", s);
            return true;
        }
    }

    // 2. Click inside bottom weight strip selects that section
    if (w_strip.has_point(mpos)) {
        const real_t norm = std::clamp(
            static_cast<real_t>(screen_to_world(mpos, p_plot).x),
            static_cast<real_t>(0.0),
            static_cast<real_t>(1.0)
        );
        for (int s = 0; s < sec_count && s + 1 < static_cast<int>(norms.size()); ++s) {
            if (norm >= norms[static_cast<size_t>(s)] && norm <= norms[static_cast<size_t>(s + 1)]) {
                set_selected_section_index(s);
                emit_signal("section_selected", s);
                return true;
            }
        }
    }

    // 3. Check Threshold Solver Line
    if (show_threshold_solver && p_plot.has_point(mpos)) {
        const float ty = world_to_screen(Vector2(0.0f, static_cast<float>(threshold_value)), p_plot).y;
        if (std::abs(mpos.y - ty) <= 7.0f) {
            base_drag_state     = BASE_DRAG_CUSTOM;
            custom_drag_submode = SUBDRAG_THRESHOLD_LINE;
            drag_modified       = false;
            return true;
        }
    }

    return false;
}

bool Spline1DEqualizerCanvas::_update_custom_drag(
    const godot::Ref<godot::InputEvent> &p_event,
    const godot::Rect2 &p_plot
) {
    using namespace godot;
    Ref<InputEventMouseMotion> mm = p_event;
    Ref<Spline1DResource> s1d     = get_spline();
    if (mm.is_null() || s1d.is_null()) return false;

    const Vector2 mpos  = mm->get_position();
    const int sec_count = s1d->get_section_count();

    if (custom_drag_submode == SUBDRAG_SECTION_WEIGHT &&
        drag_target_index >= 0 &&
        drag_target_index < sec_count) {
        const float dx_norm  = (mpos.x - drag_start_mouse.x) / std::max(1.0f, p_plot.size.x);
        const real_t delta_w = static_cast<real_t>(dx_norm) * weight_drag_start_total;

        if (redistribute_weights && (drag_target_index + 1 < sec_count)) {
            const real_t pair_sum = weight_drag_start_w0 + weight_drag_start_w1;
            const real_t w0 = std::clamp(
                weight_drag_start_w0 + delta_w,
                static_cast<real_t>(0.01),
                std::max(static_cast<real_t>(0.02), pair_sum - static_cast<real_t>(0.01))
            );
            const real_t w1 = std::max(static_cast<real_t>(0.01), pair_sum - w0);
            s1d->set_section_weight(drag_target_index, w0);
            s1d->set_section_weight(drag_target_index + 1, w1);
        } else {
            const real_t new_w = std::max(static_cast<real_t>(0.01), weight_drag_start_w0 + delta_w);
            s1d->set_section_weight(drag_target_index, new_w);
        }
        drag_modified = true;
        return true;
    }

    if (custom_drag_submode == SUBDRAG_THRESHOLD_LINE) {
        threshold_value = static_cast<real_t>(screen_to_world(mpos, p_plot).y);
        emit_signal("threshold_value_changed", threshold_value);
        queue_redraw();
        return true;
    }

    return false;
}

void Spline1DEqualizerCanvas::_update_custom_hover(
    const godot::Vector2 &p_mouse_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<Spline1DResource> s1d = get_spline();
    if (s1d.is_null()) return;

    const godot::Rect2 w_strip = _get_weight_strip_rect(p_plot);
    int new_hov_sec = -1;
    if (w_strip.has_point(p_mouse_pos)) {
        const real_t norm = std::clamp(
            static_cast<real_t>(screen_to_world(p_mouse_pos, p_plot).x),
            static_cast<real_t>(0.0),
            static_cast<real_t>(1.0)
        );
        const std::vector<real_t> norms = _compute_fader_norms();
        const int sec_count = s1d->get_section_count();
        for (int s = 0; s < sec_count && s + 1 < static_cast<int>(norms.size()); ++s) {
            if (norm >= norms[static_cast<size_t>(s)] && norm <= norms[static_cast<size_t>(s + 1)]) {
                new_hov_sec = s;
                break;
            }
        }
    }
    if (new_hov_sec != hovered_section_index) {
        hovered_section_index = new_hov_sec;
        queue_redraw();
    }
}

// ============================================================================
// Spline1DControl Implementation
// ============================================================================

void Spline1DControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_spline_1d", "spline"), &Spline1DControl::set_spline_1d);
    ClassDB::bind_method(D_METHOD("get_spline_1d"), &Spline1DControl::get_spline_1d);
    ClassDB::bind_method(D_METHOD("get_equalizer_canvas"), &Spline1DControl::get_equalizer_canvas);

    ClassDB::bind_method(D_METHOD("_on_preset_zero_to_one"), &Spline1DControl::_on_preset_zero_to_one);
    ClassDB::bind_method(D_METHOD("_on_preset_one_to_zero"), &Spline1DControl::_on_preset_one_to_zero);
    ClassDB::bind_method(D_METHOD("_on_preset_equidistant"), &Spline1DControl::_on_preset_equidistant);
    ClassDB::bind_method(D_METHOD("_on_fit_y_pressed"), &Spline1DControl::_on_fit_y_pressed);
    ClassDB::bind_method(D_METHOD("_on_show_equalizer_toggled", "show"), &Spline1DControl::_on_show_equalizer_toggled);
    ClassDB::bind_method(D_METHOD("_on_show_solver_toggled", "show"), &Spline1DControl::_on_show_solver_toggled);
    ClassDB::bind_method(D_METHOD("_on_redistribute_w_toggled", "redistribute"), &Spline1DControl::_on_redistribute_w_toggled);
    ClassDB::bind_method(D_METHOD("_on_canvas_point_selected", "point_index"), &Spline1DControl::_on_canvas_point_selected);
    ClassDB::bind_method(D_METHOD("_on_canvas_section_selected", "section_index"), &Spline1DControl::_on_canvas_section_selected);
}

Spline1DControl::Spline1DControl() {
    _build_1d_extensions();
}

Spline1DControl::~Spline1DControl() = default;

void Spline1DControl::_build_1d_extensions() {
    using namespace godot;

    if (custom_header_slot) {
        HBoxContainer *preset_bar = memnew(HBoxContainer);
        preset_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        preset_bar->add_theme_constant_override("separation", 6);
        custom_header_slot->add_child(preset_bar);

        Label *preset_lbl = memnew(Label);
        preset_lbl->set_text("1D Presets:");
        preset_bar->add_child(preset_lbl);

        Button *btn_01 = memnew(Button);
        btn_01->set_text("0 → 1");
        btn_01->set_tooltip_text("Reset spline to a linear 0.0 to 1.0 ramp.");
        btn_01->connect("pressed", Callable(this, "_on_preset_zero_to_one"));
        preset_bar->add_child(btn_01);

        Button *btn_10 = memnew(Button);
        btn_10->set_text("1 → 0");
        btn_10->set_tooltip_text("Reset spline to a linear 1.0 to 0.0 falloff.");
        btn_10->connect("pressed", Callable(this, "_on_preset_one_to_zero"));
        preset_bar->add_child(btn_10);

        equidistant_count_spin = memnew(SpinBox);
        equidistant_count_spin->set_min(2.0);
        equidistant_count_spin->set_max(64.0);
        equidistant_count_spin->set_step(1.0);
        equidistant_count_spin->set_value(4.0);
        equidistant_count_spin->set_custom_minimum_size(Vector2(56.0f, 0.0f));
        equidistant_count_spin->set_tooltip_text("Point count for the Equidistant preset.");
        preset_bar->add_child(equidistant_count_spin);

        Button *btn_equi = memnew(Button);
        btn_equi->set_text("Equidistant");
        btn_equi->set_tooltip_text("Generate N equidistant points from 0.0 to 1.0.");
        btn_equi->connect("pressed", Callable(this, "_on_preset_equidistant"));
        preset_bar->add_child(btn_equi);

        Control *spacer = memnew(Control);
        spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        preset_bar->add_child(spacer);

        Button *btn_fit = memnew(Button);
        btn_fit->set_text("Fit Y");
        btn_fit->set_tooltip_text("Fit the vertical equalizer axis to the spline's min/max values.");
        btn_fit->connect("pressed", Callable(this, "_on_fit_y_pressed"));
        preset_bar->add_child(btn_fit);

        redistribute_w_check = memnew(CheckBox);
        redistribute_w_check->set_text("Lock Total W");
        redistribute_w_check->set_tooltip_text(
            "When enabled, dragging a section divider trades weight with the adjacent section instead of stretching total weight."
        );
        redistribute_w_check->connect("toggled", Callable(this, "_on_redistribute_w_toggled"));
        preset_bar->add_child(redistribute_w_check);

        show_solver_check = memnew(CheckBox);
        show_solver_check->set_text("Root Solver");
        show_solver_check->set_tooltip_text("Show draggable threshold line highlighting all roots f(t) = Y.");
        show_solver_check->connect("toggled", Callable(this, "_on_show_solver_toggled"));
        preset_bar->add_child(show_solver_check);

        show_equalizer_check = memnew(CheckBox);
        show_equalizer_check->set_text("Equalizer");
        show_equalizer_check->set_pressed(true);
        show_equalizer_check->set_tooltip_text("Show or hide the 1D Graphic Equalizer visual curve canvas.");
        show_equalizer_check->connect("toggled", Callable(this, "_on_show_equalizer_toggled"));
        preset_bar->add_child(show_equalizer_check);
    }

    if (visual_editor_slot) {
        equalizer_canvas = memnew(Spline1DEqualizerCanvas);
        equalizer_canvas->connect("point_selected", Callable(this, "_on_canvas_point_selected"));
        equalizer_canvas->connect("section_selected", Callable(this, "_on_canvas_section_selected"));
        visual_editor_slot->add_child(equalizer_canvas);
    }
}

void Spline1DControl::set_spline_1d(const godot::Ref<Spline1DResource> &p_spline) {
    set_spline_resource(p_spline);
}

godot::Ref<Spline1DResource> Spline1DControl::get_spline_1d() const {
    return godot::Ref<Spline1DResource>(godot::Object::cast_to<Spline1DResource>(spline_resource.ptr()));
}

void Spline1DControl::_on_resource_rebound() {
    if (equalizer_canvas) {
        equalizer_canvas->set_spline(get_spline_1d());
    }
}

void Spline1DControl::_on_ui_refreshed() {
    if (equalizer_canvas) {
        equalizer_canvas->queue_redraw();
    }
}

void Spline1DControl::_on_selection_updated() {
    if (!equalizer_canvas) return;
    if (selected_point_index >= 0) {
        equalizer_canvas->set_selected_point_index(selected_point_index);
    } else if (selected_section_index >= 0) {
        equalizer_canvas->set_selected_section_index(selected_section_index);
    } else {
        equalizer_canvas->set_selected_point_index(-1);
    }
}

void Spline1DControl::_populate_point_field(int p_index, SplinePointField *p_field) {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (!p_field || s1d.is_null()) return;

    p_field->configure_point_1d(
        p_index,
        s1d->get_point_position(p_index),
        s1d->get_point_back_handle(p_index),
        s1d->get_point_use_back_handle(p_index),
        s1d->get_point_fore_handle(p_index),
        s1d->get_point_use_fore_handle(p_index),
        s1d->get_point_count() > 0
    );
}

void Spline1DControl::_commit_point_position(int p_index, const godot::Variant &p_position) {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (s1d.is_null()) return;
    s1d->commit_set_point_position(p_index, static_cast<real_t>(p_position));
}

void Spline1DControl::_commit_point_back_handle(
    int p_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (s1d.is_null()) return;
    s1d->commit_set_point_back_handle(p_index, static_cast<real_t>(p_handle), p_enabled);
}

void Spline1DControl::_commit_point_fore_handle(
    int p_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (s1d.is_null()) return;
    s1d->commit_set_point_fore_handle(p_index, static_cast<real_t>(p_handle), p_enabled);
}

void Spline1DControl::_commit_add_default_point() {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (s1d.is_null()) return;
    const int n = s1d->get_point_count();
    const real_t next_val = (n > 0) ? s1d->get_point_position(n - 1) : static_cast<real_t>(0.0);
    s1d->commit_add_point(next_val);
}

void Spline1DControl::_commit_split_section(int p_section_index) {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (s1d.is_null()) return;

    const int sec_count = s1d->get_section_count();
    if (p_section_index < 0 || p_section_index >= sec_count) return;

    const real_t mid_val = s1d->sample_section(p_section_index, static_cast<real_t>(0.5));
    const SplineCurveType ctype = s1d->get_section_curve_type(p_section_index);
    const real_t half_w = std::max(
        static_cast<real_t>(0.01),
        s1d->get_section_weight(p_section_index) * static_cast<real_t>(0.5)
    );

    const godot::Dictionary before = s1d->get_snapshot_state();
    s1d->set_section_weight(p_section_index, half_w);
    if (p_section_index + 1 < s1d->get_point_count()) {
        s1d->insert_point(p_section_index + 1, mid_val, ctype, half_w);
    } else {
        s1d->add_point(mid_val, ctype, half_w);
    }
    const godot::Dictionary after = s1d->get_snapshot_state();
    s1d->commit_snapshot_action("Split 1D Spline Section", before, after);
}

void Spline1DControl::_on_preset_zero_to_one() {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (s1d.is_null()) return;
    const godot::Dictionary before = s1d->get_snapshot_state();
    s1d->setup_zero_to_one();
    s1d->commit_snapshot_action("Preset 1D Spline 0 -> 1", before, s1d->get_snapshot_state());
    if (equalizer_canvas) equalizer_canvas->fit_y_range_to_spline();
}

void Spline1DControl::_on_preset_one_to_zero() {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (s1d.is_null()) return;
    const godot::Dictionary before = s1d->get_snapshot_state();
    s1d->setup_one_to_zero();
    s1d->commit_snapshot_action("Preset 1D Spline 1 -> 0", before, s1d->get_snapshot_state());
    if (equalizer_canvas) equalizer_canvas->fit_y_range_to_spline();
}

void Spline1DControl::_on_preset_equidistant() {
    godot::Ref<Spline1DResource> s1d = get_spline_1d();
    if (s1d.is_null()) return;
    const int count = equidistant_count_spin
        ? static_cast<int>(equidistant_count_spin->get_value())
        : 4;
    const godot::Dictionary before = s1d->get_snapshot_state();
    s1d->setup_equidistant(count, s1d->get_default_curve_type());
    s1d->commit_snapshot_action("Preset 1D Spline Equidistant", before, s1d->get_snapshot_state());
    if (equalizer_canvas) equalizer_canvas->fit_y_range_to_spline();
}

void Spline1DControl::_on_fit_y_pressed() {
    if (equalizer_canvas) {
        equalizer_canvas->fit_y_range_to_spline();
    }
}

void Spline1DControl::_on_show_equalizer_toggled(bool p_show) {
    if (visual_editor_slot) {
        visual_editor_slot->set_visible(p_show);
    }
}

void Spline1DControl::_on_show_solver_toggled(bool p_show) {
    if (equalizer_canvas) {
        equalizer_canvas->set_show_threshold_solver(p_show);
    }
}

void Spline1DControl::_on_redistribute_w_toggled(bool p_redistribute) {
    if (equalizer_canvas) {
        equalizer_canvas->set_redistribute_weights(p_redistribute);
    }
}

void Spline1DControl::_on_canvas_point_selected(int p_point_index) {
    select_point(p_point_index);
}

void Spline1DControl::_on_canvas_section_selected(int p_section_index) {
    select_section(p_section_index);
}

} // namespace ideam::godot_ext