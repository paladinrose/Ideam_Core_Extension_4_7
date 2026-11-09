#include "spline_2d_control.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::godot_ext {

// ============================================================================
// Spline2DCanvas Implementation (Specializing SplineCanvasBase)
// ============================================================================

void Spline2DCanvas::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &Spline2DCanvas::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &Spline2DCanvas::get_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_spline",
        "get_spline"
    );

    ClassDB::bind_method(D_METHOD("set_show_polygon_fill", "show"), &Spline2DCanvas::set_show_polygon_fill);
    ClassDB::bind_method(D_METHOD("get_show_polygon_fill"), &Spline2DCanvas::get_show_polygon_fill);

    ClassDB::bind_method(D_METHOD("set_show_normals", "show"), &Spline2DCanvas::set_show_normals);
    ClassDB::bind_method(D_METHOD("get_show_normals"), &Spline2DCanvas::get_show_normals);

    ClassDB::bind_method(D_METHOD("set_show_direction", "show"), &Spline2DCanvas::set_show_direction);
    ClassDB::bind_method(D_METHOD("get_show_direction"), &Spline2DCanvas::get_show_direction);
}

Spline2DCanvas::Spline2DCanvas() {
    // Configure SplineCanvasBase for 2D Spatial Spline & Polygon editing:
    //   - Circular control nodes with free 2D X/Y dragging
    //   - Uniform 1:1 aspect ratio so circles/squares never distort
    //   - Full 2D middle-mouse panning and cursor-centered wheel zooming on both axes
    set_custom_minimum_size(godot::Vector2(0.0f, 250.0f));
    set_point_style(POINT_STYLE_CIRCLE);
    set_anchor_drag_constraint(DRAG_AXIS_FREE_2D);
    set_lock_aspect_ratio(true);
    set_pan_zoom_axes(true, true, true, true);
    set_plot_margins(44.0f, 18.0f, 14.0f, 22.0f);
    set_view_bounds(godot::Vector2(-75.0f, -75.0f), godot::Vector2(75.0f, 75.0f));
}

Spline2DCanvas::~Spline2DCanvas() = default;

void Spline2DCanvas::set_spline(const godot::Ref<Spline2DResource> &p_spline) {
    set_spline_resource(p_spline);
}

godot::Ref<Spline2DResource> Spline2DCanvas::get_spline() const {
    return godot::Ref<Spline2DResource>(godot::Object::cast_to<Spline2DResource>(spline_resource.ptr()));
}

void Spline2DCanvas::set_show_polygon_fill(bool p_show) {
    show_polygon_fill = p_show;
    queue_redraw();
}

void Spline2DCanvas::set_show_normals(bool p_show) {
    show_normals = p_show;
    queue_redraw();
}

void Spline2DCanvas::set_show_direction(bool p_show) {
    show_direction = p_show;
    queue_redraw();
}

// --- SplineCanvasBase Projection Overrides ---

godot::Vector2 Spline2DCanvas::_get_point_world_pos(int p_index) const {
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null()) return godot::Vector2();
    return s2d->get_point_position(p_index);
}

void Spline2DCanvas::_set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) {
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null()) return;
    s2d->set_point_position(p_index, p_world_pos);
}

godot::Vector2 Spline2DCanvas::_get_back_handle_screen_pos(
    int p_index,
    const godot::Rect2 &p_plot
) const {
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null()) return godot::Vector2();
    const godot::Vector2 anchor_w = s2d->get_point_position(p_index);
    const godot::Vector2 handle_w = s2d->get_point_back_handle(p_index);
    return world_to_screen(anchor_w + handle_w, p_plot);
}

void Spline2DCanvas::_set_back_handle_from_screen_pos(
    int p_index,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null()) return;
    const godot::Vector2 anchor_w = s2d->get_point_position(p_index);
    const godot::Vector2 target_w = screen_to_world(p_screen_pos, p_plot);
    s2d->set_point_back_handle(p_index, target_w - anchor_w, true);
}

godot::Vector2 Spline2DCanvas::_get_fore_handle_screen_pos(
    int p_index,
    const godot::Rect2 &p_plot
) const {
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null()) return godot::Vector2();
    const godot::Vector2 anchor_w = s2d->get_point_position(p_index);
    const godot::Vector2 handle_w = s2d->get_point_fore_handle(p_index);
    return world_to_screen(anchor_w + handle_w, p_plot);
}

void Spline2DCanvas::_set_fore_handle_from_screen_pos(
    int p_index,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null()) return;
    const godot::Vector2 anchor_w = s2d->get_point_position(p_index);
    const godot::Vector2 target_w = screen_to_world(p_screen_pos, p_plot);
    s2d->set_point_fore_handle(p_index, target_w - anchor_w, true);
}

godot::PackedVector2Array Spline2DCanvas::_sample_curve_screen_points(
    const godot::Rect2 &p_plot
) const {
    godot::PackedVector2Array out;
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null() || s2d->get_point_count() < 2) return out;

    const godot::PackedVector2Array world_pts = s2d->get_smoothed_points(16);
    out.resize(world_pts.size());
    for (int i = 0; i < world_pts.size(); ++i) {
        out.set(i, world_to_screen(world_pts[i], p_plot));
    }
    return out;
}

bool Spline2DCanvas::_compute_world_bounds(godot::Rect2 &r_bounds) const {
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null() || s2d->get_point_count() == 0) return false;
    r_bounds = s2d->get_bounding_rect(true);
    return true;
}

void Spline2DCanvas::_on_double_click_plot(
    const godot::Vector2 &p_world_pos,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null()) return;

    const int pt_count  = s2d->get_point_count();
    const int sec_count = s2d->get_section_count();
    if (pt_count < 2 || sec_count == 0) {
        s2d->commit_add_point(p_world_pos);
        return;
    }

    // Check if the double-click was close to an existing section curve; if so, split that section.
    // Otherwise, append the clicked point to the end of the 2D spline.
    const godot::Vector2 closest_w   = s2d->get_closest_point_on_spline(p_world_pos, 16);
    const godot::Vector2 closest_scr = world_to_screen(closest_w, p_plot);

    if (p_screen_pos.distance_to(closest_scr) <= 18.0f) {
        const real_t pct              = s2d->get_closest_percent_on_spline(p_world_pos, 16, true);
        const godot::Vector2 sec_info = s2d->get_section_at_percent(pct, true);
        const int target_sec          = std::clamp(static_cast<int>(sec_info.x), 0, sec_count - 1);
        const SplineCurveType ctype   = s2d->get_section_curve_type(target_sec);
        const real_t half_w           = std::max(
            static_cast<real_t>(0.01),
            s2d->get_section_weight(target_sec) * static_cast<real_t>(0.5)
        );

        const godot::Dictionary before = s2d->get_snapshot_state();
        s2d->set_section_weight(target_sec, half_w);
        if (target_sec + 1 < pt_count) {
            s2d->insert_point(target_sec + 1, closest_w, ctype, half_w);
        } else {
            s2d->add_point(closest_w, ctype, half_w);
        }
        s2d->commit_snapshot_action("Split 2D Spline Section", before, s2d->get_snapshot_state());
    } else {
        s2d->commit_add_point(p_world_pos);
    }
}

void Spline2DCanvas::_draw_plot_background_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null() || !show_polygon_fill || !s2d->get_wrap() || s2d->get_point_count() < 3) {
        return;
    }

    // Render ear-clipped triangulated polygon fill + subtle triangulation mesh lines
    const PackedVector2Array smooth_w = s2d->get_smoothed_points(10);
    const PackedInt32Array tris       = s2d->triangulate(10);
    if (smooth_w.size() < 3 || tris.size() < 3) return;

    const Color fill_col(0.22f, 0.54f, 0.86f, 0.18f);
    const Color wire_col(0.32f, 0.64f, 0.94f, 0.14f);

    for (int t = 0; t + 2 < tris.size(); t += 3) {
        const int i0 = tris[t];
        const int i1 = tris[t + 1];
        const int i2 = tris[t + 2];
        if (i0 < 0 || i0 >= smooth_w.size() ||
            i1 < 0 || i1 >= smooth_w.size() ||
            i2 < 0 || i2 >= smooth_w.size()) {
            continue;
        }
        const Vector2 p0 = world_to_screen(smooth_w[i0], p_plot);
        const Vector2 p1 = world_to_screen(smooth_w[i1], p_plot);
        const Vector2 p2 = world_to_screen(smooth_w[i2], p_plot);

        PackedVector2Array tri_pts;
        tri_pts.push_back(p0);
        tri_pts.push_back(p1);
        tri_pts.push_back(p2);

        PackedColorArray tri_cols;
        tri_cols.push_back(fill_col);
        tri_cols.push_back(fill_col);
        tri_cols.push_back(fill_col);

        draw_polygon(tri_pts, tri_cols);
        draw_line(p0, p1, wire_col, 1.0f);
        draw_line(p1, p2, wire_col, 1.0f);
        draw_line(p2, p0, wire_col, 1.0f);
    }
}

void Spline2DCanvas::_draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<Spline2DResource> s2d = get_spline();
    if (s2d.is_null() || s2d->get_section_count() <= 0) return;

    const int sec_count = s2d->get_section_count();
    Ref<Font> font      = get_theme_default_font();

    // 1. Highlight Selected Section Curve Segment
    if (selected_section_index >= 0 && selected_section_index < sec_count) {
        constexpr int SEC_SAMPLES = 24;
        PackedVector2Array sel_pts;
        sel_pts.resize(SEC_SAMPLES + 1);
        for (int i = 0; i <= SEC_SAMPLES; ++i) {
            const real_t local_t = static_cast<real_t>(i) / static_cast<real_t>(SEC_SAMPLES);
            const Vector2 w_pt   = s2d->sample_section(selected_section_index, local_t);
            sel_pts.set(i, world_to_screen(w_pt, p_plot));
        }
        draw_polyline(sel_pts, Color(1.0f, 0.82f, 0.28f, 1.0f), 3.5f, true);
    }

    // 2. Section Midpoint Directional Chevrons
    if (show_direction) {
        for (int s = 0; s < sec_count; ++s) {
            const Vector2 mid_w   = s2d->sample_section(s, static_cast<real_t>(0.5));
            const Vector2 deriv_w = s2d->sample_section_derivative(s, static_cast<real_t>(0.5));
            const Vector2 mid_scr = world_to_screen(mid_w, p_plot);
            const Vector2 next_scr = world_to_screen(mid_w + deriv_w * 0.05f, p_plot);
            Vector2 dir = next_scr - mid_scr;
            if (dir.length_squared() > 1e-5f) {
                dir = dir.normalized();
                const Vector2 perp(-dir.y, dir.x);
                const Vector2 tip  = mid_scr + dir * 5.0f;
                const Vector2 left = mid_scr - dir * 4.0f + perp * 4.0f;
                const Vector2 rght = mid_scr - dir * 4.0f - perp * 4.0f;
                const Color chev_col = (s == selected_section_index)
                    ? Color(1.0f, 0.85f, 0.30f, 1.0f)
                    : Color(0.55f, 0.88f, 1.0f, 0.85f);
                draw_line(left, tip, chev_col, 1.6f);
                draw_line(rght, tip, chev_col, 1.6f);
            }
        }
    }

    // 3. Optional Normal Comb Vectors
    if (show_normals) {
        constexpr int NORMAL_SAMPLES = 32;
        for (int i = 0; i <= NORMAL_SAMPLES; ++i) {
            const real_t pct      = static_cast<real_t>(i) / static_cast<real_t>(NORMAL_SAMPLES);
            const Vector2 pos_w   = s2d->sample(pct, false);
            const Vector2 nrm_w   = s2d->get_normal(pct, false);
            const Vector2 pos_scr = world_to_screen(pos_w, p_plot);
            // Convert world normal to screen normal direction (accounting for Y flip)
            const Vector2 nrm_scr = Vector2(nrm_w.x, -nrm_w.y).normalized();
            draw_line(
                pos_scr,
                pos_scr + nrm_scr * normal_scale,
                Color(0.46f, 0.94f, 0.66f, 0.72f),
                1.2f
            );
        }
    }

    // 4. Closed Polygon Centroid Crosshair & Area/Perimeter Readout
    if (show_polygon_fill && s2d->get_wrap() && s2d->get_point_count() >= 3) {
        const Vector2 centroid_w   = s2d->get_centroid(10);
        const Vector2 centroid_scr = world_to_screen(centroid_w, p_plot);
        const Color c_col(0.98f, 0.56f, 0.84f, 0.95f);
        draw_line(centroid_scr - Vector2(6.0f, 0.0f), centroid_scr + Vector2(6.0f, 0.0f), c_col, 1.5f);
        draw_line(centroid_scr - Vector2(0.0f, 6.0f), centroid_scr + Vector2(0.0f, 6.0f), c_col, 1.5f);
        draw_circle(centroid_scr, 3.0f, c_col);

        if (font.is_valid()) {
            const double area  = static_cast<double>(s2d->get_area(10));
            const double perim = static_cast<double>(s2d->get_perimeter(10));
            const String poly_info = String("Polygon Area: ") + String::num( std::abs(area), 1) +
                (area >= 0.0 ? String(" (CCW)") : String(" (CW)")) +
                String("   |   Perimeter: ") + String::num(perim, 1);
            draw_string(
                font,
                Vector2(p_plot.position.x + 8.0f, p_plot.position.y + p_plot.size.y - 6.0f),
                poly_info,
                HORIZONTAL_ALIGNMENT_LEFT,
                -1.0f,
                10,
                Color(0.76f, 0.88f, 1.0f, 0.92f)
            );
        }
    }
}

// ============================================================================
// Spline2DControl Implementation
// ============================================================================

void Spline2DControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_spline_2d", "spline"), &Spline2DControl::set_spline_2d);
    ClassDB::bind_method(D_METHOD("get_spline_2d"), &Spline2DControl::get_spline_2d);
    ClassDB::bind_method(D_METHOD("get_canvas_2d"), &Spline2DControl::get_canvas_2d);

    ClassDB::bind_method(D_METHOD("_on_preset_rectangle"), &Spline2DControl::_on_preset_rectangle);
    ClassDB::bind_method(D_METHOD("_on_preset_regular_polygon"), &Spline2DControl::_on_preset_regular_polygon);
    ClassDB::bind_method(D_METHOD("_on_preset_star"), &Spline2DControl::_on_preset_star);
    ClassDB::bind_method(D_METHOD("_on_preset_circle"), &Spline2DControl::_on_preset_circle);
    ClassDB::bind_method(D_METHOD("_on_rotate_45_pressed"), &Spline2DControl::_on_rotate_45_pressed);
    ClassDB::bind_method(D_METHOD("_on_fit_2d_pressed"), &Spline2DControl::_on_fit_2d_pressed);
    ClassDB::bind_method(D_METHOD("_on_lock_aspect_toggled", "lock"), &Spline2DControl::_on_lock_aspect_toggled);
    ClassDB::bind_method(D_METHOD("_on_poly_fill_toggled", "fill"), &Spline2DControl::_on_poly_fill_toggled);
    ClassDB::bind_method(D_METHOD("_on_normals_toggled", "show"), &Spline2DControl::_on_normals_toggled);
    ClassDB::bind_method(D_METHOD("_on_show_canvas_toggled", "show"), &Spline2DControl::_on_show_canvas_toggled);
    ClassDB::bind_method(D_METHOD("_on_canvas_point_selected", "point_index"), &Spline2DControl::_on_canvas_point_selected);
    ClassDB::bind_method(D_METHOD("_on_canvas_section_selected", "section_index"), &Spline2DControl::_on_canvas_section_selected);
}

Spline2DControl::Spline2DControl() {
    _build_2d_extensions();
}

Spline2DControl::~Spline2DControl() = default;

void Spline2DControl::_build_2d_extensions() {
    using namespace godot;

    if (custom_header_slot) {
        // Row 1: 2D Shape Presets & Spatial Transforms
        HBoxContainer *preset_bar = memnew(HBoxContainer);
        preset_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        preset_bar->add_theme_constant_override("separation", 6);
        custom_header_slot->add_child(preset_bar);

        Label *preset_lbl = memnew(Label);
        preset_lbl->set_text("2D Shapes:");
        preset_bar->add_child(preset_lbl);

        Button *btn_rect = memnew(Button);
        btn_rect->set_text("Rect");
        btn_rect->set_tooltip_text("Replace with a closed 100x100 rectangle centered at the origin.");
        btn_rect->connect("pressed", Callable(this, "_on_preset_rectangle"));
        preset_bar->add_child(btn_rect);

        polygon_sides_spin = memnew(SpinBox);
        polygon_sides_spin->set_min(3.0);
        polygon_sides_spin->set_max(64.0);
        polygon_sides_spin->set_step(1.0);
        polygon_sides_spin->set_value(6.0);
        polygon_sides_spin->set_custom_minimum_size(Vector2(54.0f, 0.0f));
        polygon_sides_spin->set_tooltip_text("Number of sides/points for Polygon, Star, and Circle presets.");
        preset_bar->add_child(polygon_sides_spin);

        Button *btn_poly = memnew(Button);
        btn_poly->set_text("Polygon");
        btn_poly->set_tooltip_text("Generate a regular N-sided polygon.");
        btn_poly->connect("pressed", Callable(this, "_on_preset_regular_polygon"));
        preset_bar->add_child(btn_poly);

        Button *btn_star = memnew(Button);
        btn_star->set_text("Star");
        btn_star->set_tooltip_text("Generate a 2N-vertex star polygon.");
        btn_star->connect("pressed", Callable(this, "_on_preset_star"));
        preset_bar->add_child(btn_star);

        Button *btn_circle = memnew(Button);
        btn_circle->set_text("Circle");
        btn_circle->set_tooltip_text("Generate a smooth Catmull-Rom closed circle.");
        btn_circle->connect("pressed", Callable(this, "_on_preset_circle"));
        preset_bar->add_child(btn_circle);

        Button *btn_rot = memnew(Button);
        btn_rot->set_text("↻ 45°");
        btn_rot->set_tooltip_text("Rotate all points and handles by 45 degrees around the bounding-box center.");
        btn_rot->connect("pressed", Callable(this, "_on_rotate_45_pressed"));
        preset_bar->add_child(btn_rot);

        Control *spacer = memnew(Control);
        spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        preset_bar->add_child(spacer);

        Button *btn_fit = memnew(Button);
        btn_fit->set_text("Fit 2D");
        btn_fit->set_tooltip_text("Fit the 2D canvas viewport to the spline's bounding box.");
        btn_fit->connect("pressed", Callable(this, "_on_fit_2d_pressed"));
        preset_bar->add_child(btn_fit);

        lock_aspect_check = memnew(CheckBox);
        lock_aspect_check->set_text("1:1");
        lock_aspect_check->set_pressed(true);
        lock_aspect_check->set_tooltip_text("Lock 1:1 uniform aspect ratio in the 2D canvas.");
        lock_aspect_check->connect("toggled", Callable(this, "_on_lock_aspect_toggled"));
        preset_bar->add_child(lock_aspect_check);

        poly_fill_check = memnew(CheckBox);
        poly_fill_check->set_text("Fill");
        poly_fill_check->set_pressed(true);
        poly_fill_check->set_tooltip_text("Render ear-clipped polygon triangulation fill and centroid when Wrap is enabled.");
        poly_fill_check->connect("toggled", Callable(this, "_on_poly_fill_toggled"));
        preset_bar->add_child(poly_fill_check);

        normals_check = memnew(CheckBox);
        normals_check->set_text("Normals");
        normals_check->set_pressed(false);
        normals_check->set_tooltip_text("Display normal comb vectors along the 2D spline.");
        normals_check->connect("toggled", Callable(this, "_on_normals_toggled"));
        preset_bar->add_child(normals_check);

        show_canvas_check = memnew(CheckBox);
        show_canvas_check->set_text("Canvas");
        show_canvas_check->set_pressed(true);
        show_canvas_check->set_tooltip_text("Show or hide the interactive 2D spline canvas.");
        show_canvas_check->connect("toggled", Callable(this, "_on_show_canvas_toggled"));
        preset_bar->add_child(show_canvas_check);
    }

    if (visual_editor_slot) {
        canvas_2d = memnew(Spline2DCanvas);
        canvas_2d->connect("point_selected", Callable(this, "_on_canvas_point_selected"));
        canvas_2d->connect("section_selected", Callable(this, "_on_canvas_section_selected"));
        visual_editor_slot->add_child(canvas_2d);
    }
}

void Spline2DControl::set_spline_2d(const godot::Ref<Spline2DResource> &p_spline) {
    set_spline_resource(p_spline);
}

godot::Ref<Spline2DResource> Spline2DControl::get_spline_2d() const {
    return godot::Ref<Spline2DResource>(godot::Object::cast_to<Spline2DResource>(spline_resource.ptr()));
}

void Spline2DControl::_on_resource_rebound() {
    if (canvas_2d) {
        canvas_2d->set_spline(get_spline_2d());
        if (get_spline_2d().is_valid() && get_spline_2d()->get_point_count() > 0) {
            canvas_2d->fit_view_to_data(true, true, static_cast<real_t>(0.2));
        }
    }
}

void Spline2DControl::_on_ui_refreshed() {
    if (canvas_2d) {
        canvas_2d->queue_redraw();
    }
}

void Spline2DControl::_on_selection_updated() {
    if (!canvas_2d) return;
    if (selected_point_index >= 0) {
        canvas_2d->set_selected_point_index(selected_point_index);
    } else if (selected_section_index >= 0) {
        canvas_2d->set_selected_section_index(selected_section_index);
    } else {
        canvas_2d->set_selected_point_index(-1);
    }
}

void Spline2DControl::_populate_point_field(int p_index, SplinePointField *p_field) {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (!p_field || s2d.is_null()) return;

    p_field->set_dimension(2);
    p_field->configure_point_variant(
        p_index,
        s2d->get_point_position(p_index),
        s2d->get_point_back_handle(p_index),
        s2d->get_point_use_back_handle(p_index),
        s2d->get_point_fore_handle(p_index),
        s2d->get_point_use_fore_handle(p_index),
        s2d->get_point_count() > 0
    );
}

void Spline2DControl::_commit_point_position(int p_index, const godot::Variant &p_position) {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    s2d->commit_set_point_position(p_index, static_cast<godot::Vector2>(p_position));
}

void Spline2DControl::_commit_point_back_handle(
    int p_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    s2d->commit_set_point_back_handle(p_index, static_cast<godot::Vector2>(p_handle), p_enabled);
}

void Spline2DControl::_commit_point_fore_handle(
    int p_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    s2d->commit_set_point_fore_handle(p_index, static_cast<godot::Vector2>(p_handle), p_enabled);
}

void Spline2DControl::_commit_add_default_point() {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    const int n = s2d->get_point_count();
    godot::Vector2 next_pos(0.0f, 0.0f);
    if (n >= 2) {
        const godot::Vector2 last = s2d->get_point_position(n - 1);
        const godot::Vector2 prev = s2d->get_point_position(n - 2);
        const godot::Vector2 step = (last - prev).length_squared() > 1e-4f
            ? (last - prev).normalized() * 25.0f
            : godot::Vector2(25.0f, 0.0f);
        next_pos = last + step;
    } else if (n == 1) {
        next_pos = s2d->get_point_position(0) + godot::Vector2(25.0f, 0.0f);
    }
    s2d->commit_add_point(next_pos);
}

void Spline2DControl::_commit_split_section(int p_section_index) {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;

    const int sec_count = s2d->get_section_count();
    if (p_section_index < 0 || p_section_index >= sec_count) return;

    const godot::Vector2 mid_pos = s2d->sample_section(p_section_index, static_cast<real_t>(0.5));
    const SplineCurveType ctype  = s2d->get_section_curve_type(p_section_index);
    const real_t half_w          = std::max(
        static_cast<real_t>(0.01),
        s2d->get_section_weight(p_section_index) * static_cast<real_t>(0.5)
    );

    const godot::Dictionary before = s2d->get_snapshot_state();
    s2d->set_section_weight(p_section_index, half_w);
    if (p_section_index + 1 < s2d->get_point_count()) {
        s2d->insert_point(p_section_index + 1, mid_pos, ctype, half_w);
    } else {
        s2d->add_point(mid_pos, ctype, half_w);
    }
    s2d->commit_snapshot_action("Split 2D Spline Section", before, s2d->get_snapshot_state());
}

// --- 2D Preset & Toolbar Callbacks ---

void Spline2DControl::_on_preset_rectangle() {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    const godot::Dictionary before = s2d->get_snapshot_state();
    s2d->setup_rectangle(godot::Vector2(0.0f, 0.0f), godot::Vector2(100.0f, 100.0f));
    s2d->commit_snapshot_action("Preset 2D Spline Rectangle", before, s2d->get_snapshot_state());
    if (canvas_2d) canvas_2d->fit_view_to_data(true, true, static_cast<real_t>(0.2));
}

void Spline2DControl::_on_preset_regular_polygon() {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    const int sides = polygon_sides_spin ? static_cast<int>(polygon_sides_spin->get_value()) : 6;
    const godot::Dictionary before = s2d->get_snapshot_state();
    s2d->setup_regular_polygon(sides, static_cast<real_t>(50.0), static_cast<real_t>(0.0));
    s2d->commit_snapshot_action("Preset 2D Regular Polygon", before, s2d->get_snapshot_state());
    if (canvas_2d) canvas_2d->fit_view_to_data(true, true, static_cast<real_t>(0.2));
}

void Spline2DControl::_on_preset_star() {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    const int sides = polygon_sides_spin ? static_cast<int>(polygon_sides_spin->get_value()) : 5;
    const godot::Dictionary before = s2d->get_snapshot_state();
    s2d->setup_star(sides, static_cast<real_t>(50.0), static_cast<real_t>(24.0), static_cast<real_t>(0.0));
    s2d->commit_snapshot_action("Preset 2D Star Polygon", before, s2d->get_snapshot_state());
    if (canvas_2d) canvas_2d->fit_view_to_data(true, true, static_cast<real_t>(0.2));
}

void Spline2DControl::_on_preset_circle() {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    const int pts = polygon_sides_spin ? std::max(4, static_cast<int>(polygon_sides_spin->get_value())) : 8;
    const godot::Dictionary before = s2d->get_snapshot_state();
    s2d->setup_circle(static_cast<real_t>(50.0), pts);
    s2d->commit_snapshot_action("Preset 2D Smooth Circle", before, s2d->get_snapshot_state());
    if (canvas_2d) canvas_2d->fit_view_to_data(true, true, static_cast<real_t>(0.2));
}

void Spline2DControl::_on_rotate_45_pressed() {
    godot::Ref<Spline2DResource> s2d = get_spline_2d();
    if (s2d.is_null()) return;
    constexpr real_t DEG_45 = static_cast<real_t>(std::numbers::pi_v<double> * 0.25);
    s2d->commit_rotate_spline(DEG_45);
}

void Spline2DControl::_on_fit_2d_pressed() {
    if (canvas_2d) {
        canvas_2d->fit_view_to_data(true, true, static_cast<real_t>(0.2));
    }
}

void Spline2DControl::_on_lock_aspect_toggled(bool p_lock) {
    if (canvas_2d) {
        canvas_2d->set_lock_aspect_ratio(p_lock);
    }
}

void Spline2DControl::_on_poly_fill_toggled(bool p_fill) {
    if (canvas_2d) {
        canvas_2d->set_show_polygon_fill(p_fill);
    }
}

void Spline2DControl::_on_normals_toggled(bool p_show) {
    if (canvas_2d) {
        canvas_2d->set_show_normals(p_show);
    }
}

void Spline2DControl::_on_show_canvas_toggled(bool p_show) {
    if (visual_editor_slot) {
        visual_editor_slot->set_visible(p_show);
    }
}

void Spline2DControl::_on_canvas_point_selected(int p_point_index) {
    select_point(p_point_index);
}

void Spline2DControl::_on_canvas_section_selected(int p_section_index) {
    select_section(p_section_index);
}

} // namespace ideam::godot_ext