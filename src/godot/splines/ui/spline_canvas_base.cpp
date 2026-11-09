#include "spline_canvas_base.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void SplineCanvasBase::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(DRAG_AXIS_FREE_2D);
    BIND_ENUM_CONSTANT(DRAG_AXIS_X_ONLY);
    BIND_ENUM_CONSTANT(DRAG_AXIS_Y_ONLY);

    BIND_ENUM_CONSTANT(POINT_STYLE_CIRCLE);
    BIND_ENUM_CONSTANT(POINT_STYLE_SQUARE);
    BIND_ENUM_CONSTANT(POINT_STYLE_FADER_CAP);

    ADD_SIGNAL(MethodInfo(
        "point_selected",
        PropertyInfo(Variant::INT, "point_index")
    ));
    ADD_SIGNAL(MethodInfo(
        "section_selected",
        PropertyInfo(Variant::INT, "section_index")
    ));
    ADD_SIGNAL(MethodInfo(
        "view_bounds_changed",
        PropertyInfo(Variant::VECTOR2, "view_min"),
        PropertyInfo(Variant::VECTOR2, "view_max")
    ));

    ClassDB::bind_method(D_METHOD("set_spline_resource", "resource"), &SplineCanvasBase::set_spline_resource);
    ClassDB::bind_method(D_METHOD("get_spline_resource"), &SplineCanvasBase::get_spline_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline_resource", PROPERTY_HINT_RESOURCE_TYPE, "SplineResource"),
        "set_spline_resource",
        "get_spline_resource"
    );

    ClassDB::bind_method(D_METHOD("set_view_bounds", "min", "max"), &SplineCanvasBase::set_view_bounds);
    ClassDB::bind_method(D_METHOD("get_view_min"), &SplineCanvasBase::get_view_min);
    ClassDB::bind_method(D_METHOD("get_view_max"), &SplineCanvasBase::get_view_max);

    ClassDB::bind_method(D_METHOD("set_lock_aspect_ratio", "lock"), &SplineCanvasBase::set_lock_aspect_ratio);
    ClassDB::bind_method(D_METHOD("get_lock_aspect_ratio"), &SplineCanvasBase::get_lock_aspect_ratio);

    ClassDB::bind_method(D_METHOD("set_auto_fit_on_change", "auto_fit"), &SplineCanvasBase::set_auto_fit_on_change);
    ClassDB::bind_method(D_METHOD("get_auto_fit_on_change"), &SplineCanvasBase::get_auto_fit_on_change);

    ClassDB::bind_method(
        D_METHOD("fit_view_to_data", "fit_x", "fit_y", "padding_ratio"),
        &SplineCanvasBase::fit_view_to_data,
        DEFVAL(true),
        DEFVAL(true),
        DEFVAL(0.15)
    );

    ClassDB::bind_method(D_METHOD("set_anchor_drag_constraint", "constraint"), &SplineCanvasBase::set_anchor_drag_constraint);
    ClassDB::bind_method(D_METHOD("get_anchor_drag_constraint"), &SplineCanvasBase::get_anchor_drag_constraint);

    ClassDB::bind_method(D_METHOD("set_point_style", "style"), &SplineCanvasBase::set_point_style);
    ClassDB::bind_method(D_METHOD("get_point_style"), &SplineCanvasBase::get_point_style);

    ClassDB::bind_method(D_METHOD("set_show_grid", "show"), &SplineCanvasBase::set_show_grid);
    ClassDB::bind_method(D_METHOD("get_show_grid"), &SplineCanvasBase::get_show_grid);

    ClassDB::bind_method(D_METHOD("set_selected_point_index", "index"), &SplineCanvasBase::set_selected_point_index);
    ClassDB::bind_method(D_METHOD("get_selected_point_index"), &SplineCanvasBase::get_selected_point_index);
    ClassDB::bind_method(D_METHOD("set_selected_section_index", "index"), &SplineCanvasBase::set_selected_section_index);
    ClassDB::bind_method(D_METHOD("get_selected_section_index"), &SplineCanvasBase::get_selected_section_index);

    ClassDB::bind_method(D_METHOD("_on_bound_resource_changed"), &SplineCanvasBase::_on_bound_resource_changed);
}

SplineCanvasBase::SplineCanvasBase() {
    set_custom_minimum_size(godot::Vector2(0.0f, 220.0f));
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_clip_contents(true);
    set_focus_mode(Control::FOCUS_CLICK);
    set_mouse_filter(Control::MOUSE_FILTER_STOP);
}

SplineCanvasBase::~SplineCanvasBase() {
    if (spline_resource.is_valid()) {
        const godot::Callable cb(this, "_on_bound_resource_changed");
        if (spline_resource->is_connected("changed", cb)) {
            spline_resource->disconnect("changed", cb);
        }
    }
}

void SplineCanvasBase::set_spline_resource(const godot::Ref<SplineResource> &p_resource) {
    if (spline_resource == p_resource) return;

    const godot::Callable cb(this, "_on_bound_resource_changed");
    if (spline_resource.is_valid() && spline_resource->is_connected("changed", cb)) {
        spline_resource->disconnect("changed", cb);
    }

    spline_resource  = p_resource;
    base_drag_state  = BASE_DRAG_NONE;
    drag_target_index = -1;

    if (spline_resource.is_valid() && !spline_resource->is_connected("changed", cb)) {
        spline_resource->connect("changed", cb);
    }

    _on_resource_bound();

    if (auto_fit_on_change && _get_canvas_point_count() > 0) {
        fit_view_to_data(allow_zoom_x, allow_zoom_y);
    }
    queue_redraw();
}

void SplineCanvasBase::_on_bound_resource_changed() {
    if (auto_fit_on_change && base_drag_state == BASE_DRAG_NONE && _get_canvas_point_count() > 0) {
        fit_view_to_data(allow_zoom_x, allow_zoom_y);
    }
    queue_redraw();
}

// ============================================================================
// Coordinate Projection & Viewport Controls
// ============================================================================

godot::Rect2 SplineCanvasBase::get_plot_rect() const {
    const godot::Vector2 sz = get_size();
    const float w = std::max(16.0f, sz.x - margin_left - margin_right);
    const float h = std::max(16.0f, sz.y - margin_top - margin_bottom);
    return godot::Rect2(margin_left, margin_top, w, h);
}

godot::Vector2 SplineCanvasBase::world_to_screen(
    const godot::Vector2 &p_world,
    const godot::Rect2 &p_plot
) const {
    const float span_x = std::max(1e-5f, view_max.x - view_min.x);
    const float span_y = std::max(1e-5f, view_max.y - view_min.y);

    if (!lock_aspect_ratio) {
        const float nx = (p_world.x - view_min.x) / span_x;
        const float ny = (p_world.y - view_min.y) / span_y;
        return godot::Vector2(
            p_plot.position.x + nx * p_plot.size.x,
            p_plot.position.y + (1.0f - ny) * p_plot.size.y
        );
    }

    // Uniform aspect ratio mapping (centered in plot rect)
    const float scale_x = p_plot.size.x / span_x;
    const float scale_y = p_plot.size.y / span_y;
    const float uniform_scale = std::min(scale_x, scale_y);
    const godot::Vector2 world_center = (view_min + view_max) * 0.5f;
    const godot::Vector2 plot_center  = p_plot.position + p_plot.size * 0.5f;

    return godot::Vector2(
        plot_center.x + (p_world.x - world_center.x) * uniform_scale,
        plot_center.y - (p_world.y - world_center.y) * uniform_scale
    );
}

godot::Vector2 SplineCanvasBase::screen_to_world(
    const godot::Vector2 &p_screen,
    const godot::Rect2 &p_plot
) const {
    const float span_x = std::max(1e-5f, view_max.x - view_min.x);
    const float span_y = std::max(1e-5f, view_max.y - view_min.y);

    if (!lock_aspect_ratio) {
        const float nx = (p_screen.x - p_plot.position.x) / std::max(1.0f, p_plot.size.x);
        const float ny = 1.0f - ((p_screen.y - p_plot.position.y) / std::max(1.0f, p_plot.size.y));
        return godot::Vector2(
            view_min.x + nx * span_x,
            view_min.y + ny * span_y
        );
    }

    const float scale_x = p_plot.size.x / span_x;
    const float scale_y = p_plot.size.y / span_y;
    const float uniform_scale = std::max(1e-5f, std::min(scale_x, scale_y));
    const godot::Vector2 world_center = (view_min + view_max) * 0.5f;
    const godot::Vector2 plot_center  = p_plot.position + p_plot.size * 0.5f;

    return godot::Vector2(
        world_center.x + (p_screen.x - plot_center.x) / uniform_scale,
        world_center.y - (p_screen.y - plot_center.y) / uniform_scale
    );
}

void SplineCanvasBase::set_view_bounds(const godot::Vector2 &p_min, const godot::Vector2 &p_max) {
    view_min.x = p_min.x;
    view_min.y = p_min.y;
    view_max.x = (p_max.x - p_min.x < 1e-4f) ? (p_min.x + 1.0f) : p_max.x;
    view_max.y = (p_max.y - p_min.y < 1e-4f) ? (p_min.y + 1.0f) : p_max.y;
    emit_signal("view_bounds_changed", view_min, view_max);
    queue_redraw();
}

void SplineCanvasBase::set_plot_margins(float p_left, float p_right, float p_top, float p_bottom) {
    margin_left   = p_left;
    margin_right  = p_right;
    margin_top    = p_top;
    margin_bottom = p_bottom;
    queue_redraw();
}

void SplineCanvasBase::set_pan_zoom_axes(bool p_pan_x, bool p_pan_y, bool p_zoom_x, bool p_zoom_y) {
    allow_pan_x  = p_pan_x;
    allow_pan_y  = p_pan_y;
    allow_zoom_x = p_zoom_x;
    allow_zoom_y = p_zoom_y;
}

void SplineCanvasBase::set_lock_aspect_ratio(bool p_lock) {
    lock_aspect_ratio = p_lock;
    queue_redraw();
}

void SplineCanvasBase::set_auto_fit_on_change(bool p_auto_fit) {
    auto_fit_on_change = p_auto_fit;
    if (auto_fit_on_change) {
        fit_view_to_data(allow_zoom_x, allow_zoom_y);
    }
}

bool SplineCanvasBase::_compute_world_bounds(godot::Rect2 &r_bounds) const {
    const int n = _get_canvas_point_count();
    if (n <= 0) return false;

    godot::Vector2 min_pt = _get_point_world_pos(0);
    godot::Vector2 max_pt = min_pt;
    for (int i = 1; i < n; ++i) {
        const godot::Vector2 p = _get_point_world_pos(i);
        min_pt.x = std::min(min_pt.x, p.x);
        min_pt.y = std::min(min_pt.y, p.y);
        max_pt.x = std::max(max_pt.x, p.x);
        max_pt.y = std::max(max_pt.y, p.y);
    }
    r_bounds = godot::Rect2(min_pt, max_pt - min_pt);
    return true;
}

void SplineCanvasBase::fit_view_to_data(bool p_fit_x, bool p_fit_y, real_t p_padding_ratio) {
    godot::Rect2 data_bounds;
    if (!_compute_world_bounds(data_bounds)) {
        return;
    }

    godot::Vector2 new_min = view_min;
    godot::Vector2 new_max = view_max;
    const float pad_factor = std::max(0.05f, static_cast<float>(p_padding_ratio));

    if (p_fit_x) {
        float sx = data_bounds.size.x;
        float mx = data_bounds.position.x;
        if (sx < 0.1f) {
            mx -= 0.5f;
            sx  = 1.0f;
        }
        const float px = sx * pad_factor;
        new_min.x = mx - px;
        new_max.x = mx + sx + px;
    }

    if (p_fit_y) {
        float sy = data_bounds.size.y;
        float my = data_bounds.position.y;
        if (sy < 0.1f) {
            my -= 0.5f;
            sy  = 1.0f;
        }
        const float py = sy * pad_factor;
        new_min.y = my - py;
        new_max.y = my + sy + py;
    }

    set_view_bounds(new_min, new_max);
}

void SplineCanvasBase::set_anchor_drag_constraint(SplineCanvasAxisConstraint p_constraint) {
    anchor_drag_constraint = p_constraint;
}

void SplineCanvasBase::set_point_style(SplineCanvasPointStyle p_style) {
    point_style = p_style;
    queue_redraw();
}

void SplineCanvasBase::set_show_grid(bool p_show) {
    show_grid = p_show;
    queue_redraw();
}

void SplineCanvasBase::set_selected_point_index(int p_index) {
    if (selected_point_index == p_index && selected_section_index == -1) return;
    selected_point_index   = p_index;
    selected_section_index = -1;
    queue_redraw();
}

void SplineCanvasBase::set_selected_section_index(int p_index) {
    if (selected_section_index == p_index && selected_point_index == -1) return;
    selected_section_index = p_index;
    selected_point_index   = -1;
    queue_redraw();
}

// ============================================================================
// Default Virtual Implementations
// ============================================================================

int SplineCanvasBase::_get_canvas_point_count() const {
    return spline_resource.is_valid() ? spline_resource->get_point_count() : 0;
}

int SplineCanvasBase::_get_canvas_section_count() const {
    return spline_resource.is_valid() ? spline_resource->get_section_count() : 0;
}

bool SplineCanvasBase::_is_canvas_wrapped() const {
    return spline_resource.is_valid() && spline_resource->get_wrap();
}

bool SplineCanvasBase::_has_back_handle(int p_index) const {
    return spline_resource.is_valid() && spline_resource->get_point_use_back_handle(p_index);
}

godot::Vector2 SplineCanvasBase::_get_back_handle_screen_pos(
    int p_index,
    const godot::Rect2 &p_plot
) const {
    return world_to_screen(_get_point_world_pos(p_index), p_plot);
}

bool SplineCanvasBase::_has_fore_handle(int p_index) const {
    return spline_resource.is_valid() && spline_resource->get_point_use_fore_handle(p_index);
}

godot::Vector2 SplineCanvasBase::_get_fore_handle_screen_pos(
    int p_index,
    const godot::Rect2 &p_plot
) const {
    return world_to_screen(_get_point_world_pos(p_index), p_plot);
}

void SplineCanvasBase::_on_right_click_point(int p_index) {
    if (spline_resource.is_valid()) {
        spline_resource->commit_remove_point(p_index);
    }
}

// ============================================================================
// Snapshot Drag Transaction Lifecycle
// ============================================================================

void SplineCanvasBase::_begin_snapshot_drag(
    BaseDragState p_state,
    int p_target_index,
    const godot::Vector2 &p_mouse_pos,
    const godot::String &p_action_name,
    int p_custom_submode
) {
    base_drag_state         = p_state;
    drag_target_index       = p_target_index;
    custom_drag_submode     = p_custom_submode;
    drag_start_mouse        = p_mouse_pos;
    active_drag_action_name = p_action_name;
    drag_modified           = false;

    if (spline_resource.is_valid()) {
        drag_before_snapshot = spline_resource->get_snapshot_state();
    } else {
        drag_before_snapshot.clear();
    }
}

void SplineCanvasBase::_end_snapshot_drag() {
    if (base_drag_state != BASE_DRAG_NONE &&
        base_drag_state != BASE_DRAG_PAN_VIEW &&
        drag_modified &&
        spline_resource.is_valid()) {
        const godot::Dictionary after_state = spline_resource->get_snapshot_state();
        spline_resource->commit_snapshot_action(
            active_drag_action_name,
            drag_before_snapshot,
            after_state
        );
    }
    base_drag_state     = BASE_DRAG_NONE;
    drag_target_index   = -1;
    custom_drag_submode = 0;
    drag_modified       = false;
    queue_redraw();
}

// ============================================================================
// Rendering Pipeline (`_draw`)
// ============================================================================

void SplineCanvasBase::_draw_grid_and_axes(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<Font> font = get_theme_default_font();
    constexpr int FONT_SIZE = 11;

    if (show_grid) {
        constexpr int Y_STEPS = 4;
        for (int i = 0; i <= Y_STEPS; ++i) {
            const float ratio = static_cast<float>(i) / static_cast<float>(Y_STEPS);
            const float y     = p_plot.position.y + ratio * p_plot.size.y;
            const Vector2 w   = screen_to_world(Vector2(p_plot.position.x, y), p_plot);
            draw_line(
                Vector2(p_plot.position.x, y),
                Vector2(p_plot.position.x + p_plot.size.x, y),
                grid_color,
                1.0f
            );
            if (font.is_valid() && margin_left >= 28.0f) {
                draw_string(
                    font,
                    Vector2(4.0f, y + 4.0f),
                    String::num(static_cast<double>(w.y), 2),
                    HORIZONTAL_ALIGNMENT_LEFT,
                    margin_left - 6.0f,
                    FONT_SIZE,
                    Color(0.62f, 0.68f, 0.76f, 0.9f)
                );
            }
        }

        // Draw vertical X grid lines when in 2D free mode (`point_style != POINT_STYLE_FADER_CAP`)
        if (point_style != POINT_STYLE_FADER_CAP) {
            constexpr int X_STEPS = 4;
            for (int i = 0; i <= X_STEPS; ++i) {
                const float ratio = static_cast<float>(i) / static_cast<float>(X_STEPS);
                const float x     = p_plot.position.x + ratio * p_plot.size.x;
                const Vector2 w   = screen_to_world(Vector2(x, p_plot.position.y), p_plot);
                draw_line(
                    Vector2(x, p_plot.position.y),
                    Vector2(x, p_plot.position.y + p_plot.size.y),
                    grid_color,
                    1.0f
                );
                if (font.is_valid() && margin_bottom >= 14.0f) {
                    draw_string(
                        font,
                        Vector2(x - 14.0f, p_plot.position.y + p_plot.size.y + 13.0f),
                        String::num(static_cast<double>(w.x), 2),
                        HORIZONTAL_ALIGNMENT_LEFT,
                        40.0f,
                        10,
                        Color(0.62f, 0.68f, 0.76f, 0.85f)
                    );
                }
            }
        }
    }

    if (show_origin_axes) {
        const Vector2 origin_scr = world_to_screen(Vector2(0.0f, 0.0f), p_plot);
        if (origin_scr.y >= p_plot.position.y && origin_scr.y <= p_plot.position.y + p_plot.size.y) {
            draw_line(
                Vector2(p_plot.position.x, origin_scr.y),
                Vector2(p_plot.position.x + p_plot.size.x, origin_scr.y),
                origin_axis_color,
                1.5f
            );
        }
        if (point_style != POINT_STYLE_FADER_CAP &&
            origin_scr.x >= p_plot.position.x &&
            origin_scr.x <= p_plot.position.x + p_plot.size.x) {
            draw_line(
                Vector2(origin_scr.x, p_plot.position.y),
                Vector2(origin_scr.x, p_plot.position.y + p_plot.size.y),
                origin_axis_color,
                1.5f
            );
        }
    }
}

void SplineCanvasBase::_draw_handles_and_points(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<Font> font     = get_theme_default_font();
    const int pt_count = _get_canvas_point_count();

    auto render_point_visual = [&](int p_idx, const Vector2 &p_world_pos, bool p_is_ghost) {
        const Vector2 scr = world_to_screen(p_world_pos, p_plot);
        const bool is_sel = (p_idx == selected_point_index) && !p_is_ghost;
        const bool is_hov = (p_idx == hovered_point_index) && !p_is_ghost;

        // 1. Optional Fader Track & Equalizer Bar (when `POINT_STYLE_FADER_CAP`)
        if (point_style == POINT_STYLE_FADER_CAP) {
            draw_line(
                Vector2(scr.x, p_plot.position.y),
                Vector2(scr.x, p_plot.position.y + p_plot.size.y),
                is_sel ? Color(0.95f, 0.76f, 0.28f, 0.65f) : Color(0.32f, 0.36f, 0.44f, 0.55f),
                is_sel ? 2.0f : 1.0f
            );

            const float zero_y = world_to_screen(Vector2(0.0f, 0.0f), p_plot).y;
            const float base_y = std::clamp(zero_y, p_plot.position.y, p_plot.position.y + p_plot.size.y);
            const float top_y  = std::min(base_y, scr.y);
            const float bar_h  = std::abs(scr.y - base_y);
            draw_rect(
                Rect2(scr.x - 3.0f, top_y, 6.0f, bar_h),
                p_is_ghost
                    ? Color(0.55f, 0.62f, 0.74f, 0.28f)
                    : (is_sel ? Color(0.98f, 0.78f, 0.28f, 0.38f) : Color(0.36f, 0.72f, 0.96f, 0.28f)),
                true
            );
        }

        // 2. Back / Fore Tangent Handles
        if (!p_is_ghost) {
            if (_has_back_handle(p_idx)) {
                const Vector2 bh_scr = _get_back_handle_screen_pos(p_idx, p_plot);
                draw_line(scr, bh_scr, Color(0.95f, 0.52f, 0.36f, 0.9f), 1.5f);
                draw_circle(bh_scr, 4.5f, Color(0.95f, 0.52f, 0.36f, 1.0f));
            }
            if (_has_fore_handle(p_idx)) {
                const Vector2 fh_scr = _get_fore_handle_screen_pos(p_idx, p_plot);
                draw_line(scr, fh_scr, Color(0.46f, 0.92f, 0.56f, 0.9f), 1.5f);
                draw_circle(fh_scr, 4.5f, Color(0.46f, 0.92f, 0.56f, 1.0f));
            }
        }

        // 3. Anchor Point Cap / Node
        const Color node_fill = p_is_ghost
            ? Color(0.50f, 0.56f, 0.66f, 0.70f)
            : (is_sel
                ? Color(1.0f, 0.82f, 0.28f, 1.0f)
                : (is_hov ? Color(0.62f, 0.88f, 1.0f, 1.0f) : Color(0.88f, 0.92f, 0.98f, 0.96f)));

        switch (point_style) {
            case POINT_STYLE_FADER_CAP: {
                const Rect2 cap_rect(scr.x - 9.0f, scr.y - 5.0f, 18.0f, 10.0f);
                draw_rect(cap_rect, node_fill, true);
                draw_rect(cap_rect, Color(0.10f, 0.12f, 0.16f, 1.0f), false, 1.5f);
                draw_line(
                    Vector2(scr.x - 6.0f, scr.y),
                    Vector2(scr.x + 6.0f, scr.y),
                    Color(0.15f, 0.18f, 0.22f, 0.9f),
                    1.2f
                );
                break;
            }
            case POINT_STYLE_SQUARE: {
                const Rect2 sq(scr.x - 5.5f, scr.y - 5.5f, 11.0f, 11.0f);
                draw_rect(sq, node_fill, true);
                draw_rect(sq, Color(0.10f, 0.12f, 0.16f, 1.0f), false, 1.5f);
                break;
            }
            case POINT_STYLE_CIRCLE:
            default: {
                draw_circle(scr, is_sel ? 6.5f : 5.5f, Color(0.10f, 0.12f, 0.16f, 1.0f));
                draw_circle(scr, is_sel ? 5.0f : 4.0f, node_fill);
                break;
            }
        }

        // 4. Point Index Label
        if (show_point_labels && font.is_valid()) {
            const String lbl = p_is_ghost
                ? String("P0↻")
                : (String("P") + String::num_int64(p_idx));
            const Vector2 lbl_pos = (point_style == POINT_STYLE_FADER_CAP)
                ? Vector2(scr.x - 10.0f, p_plot.position.y + 11.0f)
                : Vector2(scr.x + 7.0f, scr.y - 7.0f);
            draw_string(
                font,
                lbl_pos,
                lbl,
                HORIZONTAL_ALIGNMENT_LEFT,
                -1.0f,
                10,
                is_sel ? Color(1.0f, 0.85f, 0.35f, 1.0f) : Color(0.75f, 0.82f, 0.90f, 0.85f)
            );
        }
    };

    for (int i = 0; i < pt_count; ++i) {
        render_point_visual(i, _get_point_world_pos(i), false);
    }

    Vector2 ghost_pos;
    if (_has_wrap_ghost_endpoint(ghost_pos)) {
        render_point_visual(0, ghost_pos, true);
    }
}

void SplineCanvasBase::_draw() {
    using namespace godot;

    const Vector2 full_sz = get_size();
    const Rect2 plot      = get_plot_rect();
    Ref<Font> font        = get_theme_default_font();

    // 1. Canvas & Plot Background
    draw_rect(Rect2(Vector2(0.0f, 0.0f), full_sz), bg_color, true);
    draw_rect(plot, plot_bg_color, true);

    // 2. Grid & Origin Axes
    _draw_grid_and_axes(plot);

    // 3. Empty State Check
    if (_get_canvas_point_count() <= 0) {
        draw_rect(plot, Color(0.28f, 0.32f, 0.38f, 1.0f), false, 1.0f);
        if (font.is_valid()) {
            draw_string(
                font,
                plot.position + Vector2(14.0f, plot.size.y * 0.5f),
                "Empty Canvas — Double-click inside plot or press '+ Add Point' to add control points.",
                HORIZONTAL_ALIGNMENT_LEFT,
                -1.0f,
                12,
                Color(0.68f, 0.72f, 0.80f, 0.85f)
            );
        }
        return;
    }

    // 4. Subclass Background Overlay (e.g., Section bands, Polygon2D interior fill)
    _draw_plot_background_overlay(plot);

    // 5. Sampled Curve Polyline
    const PackedVector2Array curve_pts = _sample_curve_screen_points(plot);
    if (curve_pts.size() >= 2) {
        draw_polyline(curve_pts, curve_color, 2.25f, true);
    }

    // 6. Subclass Foreground Overlay (e.g., Root-solver threshold line, Equalizer weight strip)
    _draw_plot_foreground_overlay(plot);

    // 7. Handles & Anchor Points
    _draw_handles_and_points(plot);

    // 8. Plot Frame Border
    draw_rect(plot, Color(0.30f, 0.35f, 0.42f, 1.0f), false, 1.0f);
}

// ============================================================================
// Unified Mouse Input & Drag State Machine (`_gui_input`)
// ============================================================================

void SplineCanvasBase::_gui_input(const godot::Ref<godot::InputEvent> &p_event) {
    using namespace godot;
    if (p_event.is_null()) return;

    const Rect2 plot   = get_plot_rect();
    const int pt_count = _get_canvas_point_count();

    // --- Mouse Button Events ---
    if (Ref<InputEventMouseButton> mb = p_event; mb.is_valid()) {
        const Vector2 mpos     = mb->get_position();
        const MouseButton bidx = mb->get_button_index();

        // 1. Mouse Wheel Zoom (on enabled zoom axes around cursor)
        if (mb->is_pressed() && (bidx == MOUSE_BUTTON_WHEEL_UP || bidx == MOUSE_BUTTON_WHEEL_DOWN)) {
            if ((allow_zoom_x || allow_zoom_y) && plot.has_point(mpos)) {
                const float zoom_factor = (bidx == MOUSE_BUTTON_WHEEL_UP) ? 0.88f : (1.0f / 0.88f);
                const Vector2 anchor_w  = screen_to_world(mpos, plot);
                Vector2 new_min         = view_min;
                Vector2 new_max         = view_max;
                if (allow_zoom_x) {
                    new_min.x = anchor_w.x + (view_min.x - anchor_w.x) * zoom_factor;
                    new_max.x = anchor_w.x + (view_max.x - anchor_w.x) * zoom_factor;
                }
                if (allow_zoom_y) {
                    new_min.y = anchor_w.y + (view_min.y - anchor_w.y) * zoom_factor;
                    new_max.y = anchor_w.y + (view_max.y - anchor_w.y) * zoom_factor;
                }
                set_view_bounds(new_min, new_max);
                accept_event();
                return;
            }
        }

        // 2. Middle-Mouse Pan
        if (bidx == MOUSE_BUTTON_MIDDLE) {
            if (mb->is_pressed() && (allow_pan_x || allow_pan_y)) {
                base_drag_state    = BASE_DRAG_PAN_VIEW;
                drag_start_mouse   = mpos;
                pan_start_view_min = view_min;
                pan_start_view_max = view_max;
                accept_event();
                return;
            } else if (!mb->is_pressed() && base_drag_state == BASE_DRAG_PAN_VIEW) {
                base_drag_state = BASE_DRAG_NONE;
                accept_event();
                return;
            }
        }

        // 3. Left Mouse Button Press / Release / Double-Click
        if (bidx == MOUSE_BUTTON_LEFT) {
            if (!mb->is_pressed()) {
                _end_snapshot_drag();
                return;
            }

            // Double-click inside plot -> Add or Split Point
            if (mb->is_double_click() && plot.has_point(mpos)) {
                _on_double_click_plot(screen_to_world(mpos, plot), mpos, plot);
                accept_event();
                return;
            }

            // Let subclass check custom interactive elements first (e.g., weight strip dividers, solver line)
            if (_try_begin_custom_drag(p_event, plot)) {
                accept_event();
                return;
            }

            // Check Back & Fore Tangent Handles
            for (int i = 0; i < pt_count; ++i) {
                if (_has_back_handle(i)) {
                    const Vector2 bh_scr = _get_back_handle_screen_pos(i, plot);
                    if (mpos.distance_to(bh_scr) <= handle_hit_radius) {
                        _begin_snapshot_drag(BASE_DRAG_BACK_HANDLE, i, mpos, "Drag Spline Back Handle");
                        set_selected_point_index(i);
                        emit_signal("point_selected", i);
                        accept_event();
                        return;
                    }
                }
                if (_has_fore_handle(i)) {
                    const Vector2 fh_scr = _get_fore_handle_screen_pos(i, plot);
                    if (mpos.distance_to(fh_scr) <= handle_hit_radius) {
                        _begin_snapshot_drag(BASE_DRAG_FORE_HANDLE, i, mpos, "Drag Spline Fore Handle");
                        set_selected_point_index(i);
                        emit_signal("point_selected", i);
                        accept_event();
                        return;
                    }
                }
            }

            // Check Anchor Points
            for (int i = 0; i < pt_count; ++i) {
                const Vector2 pt_scr = world_to_screen(_get_point_world_pos(i), plot);
                const bool hit = (point_style == POINT_STYLE_FADER_CAP)
                    ? Rect2(pt_scr.x - 11.0f, pt_scr.y - 8.0f, 22.0f, 16.0f).has_point(mpos)
                    : (mpos.distance_to(pt_scr) <= point_hit_radius);
                if (hit) {
                    _begin_snapshot_drag(BASE_DRAG_POINT, i, mpos, "Drag Spline Point");
                    set_selected_point_index(i);
                    emit_signal("point_selected", i);
                    accept_event();
                    return;
                }
            }

            // In `POINT_STYLE_FADER_CAP`, clicking anywhere along a fader's vertical slot jumps that fader
            if (point_style == POINT_STYLE_FADER_CAP && plot.has_point(mpos)) {
                for (int i = 0; i < pt_count; ++i) {
                    const Vector2 pt_scr = world_to_screen(_get_point_world_pos(i), plot);
                    if (std::abs(mpos.x - pt_scr.x) <= 7.0f) {
                        _begin_snapshot_drag(BASE_DRAG_POINT, i, mpos, "Adjust Spline Fader");
                        Vector2 w_pos = _get_point_world_pos(i);
                        w_pos.y       = screen_to_world(mpos, plot).y;
                        _set_point_world_pos(i, w_pos);
                        drag_modified = true;
                        set_selected_point_index(i);
                        emit_signal("point_selected", i);
                        accept_event();
                        return;
                    }
                }
            }
        } else if (bidx == MOUSE_BUTTON_RIGHT && mb->is_pressed()) {
            for (int i = 0; i < pt_count; ++i) {
                const Vector2 pt_scr = world_to_screen(_get_point_world_pos(i), plot);
                const bool hit = (point_style == POINT_STYLE_FADER_CAP)
                    ? Rect2(pt_scr.x - 11.0f, pt_scr.y - 8.0f, 22.0f, 16.0f).has_point(mpos)
                    : (mpos.distance_to(pt_scr) <= point_hit_radius);
                if (hit) {
                    _on_right_click_point(i);
                    accept_event();
                    return;
                }
            }
        }
    }

    // --- Mouse Motion Events ---
    if (Ref<InputEventMouseMotion> mm = p_event; mm.is_valid()) {
        const Vector2 mpos = mm->get_position();

        if (base_drag_state == BASE_DRAG_PAN_VIEW) {
            const Vector2 w_start = screen_to_world(drag_start_mouse, plot);
            const Vector2 w_curr  = screen_to_world(mpos, plot);
            const Vector2 delta   = w_start - w_curr;
            Vector2 new_min       = pan_start_view_min;
            Vector2 new_max       = pan_start_view_max;
            if (allow_pan_x) {
                new_min.x += delta.x;
                new_max.x += delta.x;
            }
            if (allow_pan_y) {
                new_min.y += delta.y;
                new_max.y += delta.y;
            }
            set_view_bounds(new_min, new_max);
            accept_event();
            return;
        }

        if (base_drag_state == BASE_DRAG_CUSTOM) {
            if (_update_custom_drag(p_event, plot)) {
                accept_event();
                return;
            }
        }

        if (base_drag_state == BASE_DRAG_POINT && drag_target_index >= 0 && drag_target_index < pt_count) {
            const Vector2 cur_w   = _get_point_world_pos(drag_target_index);
            Vector2 target_w      = screen_to_world(mpos, plot);

            if (mm->is_shift_pressed() && snap_step > 1e-5f) {
                target_w.x = std::round(target_w.x / snap_step) * snap_step;
                target_w.y = std::round(target_w.y / snap_step) * snap_step;
            }

            if (anchor_drag_constraint == DRAG_AXIS_X_ONLY) {
                target_w.y = cur_w.y;
            } else if (anchor_drag_constraint == DRAG_AXIS_Y_ONLY) {
                target_w.x = cur_w.x;
            }

            _set_point_world_pos(drag_target_index, target_w);
            drag_modified = true;
            accept_event();
            return;
        }

        if (base_drag_state == BASE_DRAG_BACK_HANDLE && drag_target_index >= 0 && drag_target_index < pt_count) {
            _set_back_handle_from_screen_pos(drag_target_index, mpos, plot);
            drag_modified = true;
            accept_event();
            return;
        }

        if (base_drag_state == BASE_DRAG_FORE_HANDLE && drag_target_index >= 0 && drag_target_index < pt_count) {
            _set_fore_handle_from_screen_pos(drag_target_index, mpos, plot);
            drag_modified = true;
            accept_event();
            return;
        }

        // Hover tracking
        int new_hov_pt = -1;
        for (int i = 0; i < pt_count; ++i) {
            const Vector2 pt_scr = world_to_screen(_get_point_world_pos(i), plot);
            const bool hit = (point_style == POINT_STYLE_FADER_CAP)
                ? Rect2(pt_scr.x - 11.0f, pt_scr.y - 8.0f, 22.0f, 16.0f).has_point(mpos)
                : (mpos.distance_to(pt_scr) <= point_hit_radius);
            if (hit) {
                new_hov_pt = i;
                break;
            }
        }
        if (new_hov_pt != hovered_point_index) {
            hovered_point_index = new_hov_pt;
            queue_redraw();
        }
        _update_custom_hover(mpos, plot);
    }
}

} // namespace ideam::godot_ext