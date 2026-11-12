#include "sdf_grid_2d_canvas.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// SDFGrid2DCanvas Implementation
// ============================================================================

void SDFGrid2DCanvas::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SDF_TOOL_PROBE_INSPECT);
    BIND_ENUM_CONSTANT(SDF_TOOL_STAMP_CIRCLE);
    BIND_ENUM_CONSTANT(SDF_TOOL_STAMP_BOX);
    BIND_ENUM_CONSTANT(SDF_TOOL_STAMP_SEGMENT);
    BIND_ENUM_CONSTANT(SDF_TOOL_CARVE_ERASE);

    ADD_SIGNAL(MethodInfo(
        "field_probed",
        PropertyInfo(Variant::VECTOR2, "world_pos"),
        PropertyInfo(Variant::FLOAT, "distance"),
        PropertyInfo(Variant::VECTOR2, "gradient")
    ));

    ClassDB::bind_method(D_METHOD("set_sdf_grid_resource", "sdf"), &SDFGrid2DCanvas::set_sdf_grid_resource);
    ClassDB::bind_method(D_METHOD("get_sdf_grid_resource"), &SDFGrid2DCanvas::get_sdf_grid_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "sdf_grid_resource", PROPERTY_HINT_RESOURCE_TYPE, "SDFGrid2DResource"),
        "set_sdf_grid_resource",
        "get_sdf_grid_resource"
    );

    ClassDB::bind_method(D_METHOD("set_brush_tool", "tool"), &SDFGrid2DCanvas::set_brush_tool);
    ClassDB::bind_method(D_METHOD("get_brush_tool"), &SDFGrid2DCanvas::get_brush_tool);

    ClassDB::bind_method(D_METHOD("set_combine_mode", "mode"), &SDFGrid2DCanvas::set_combine_mode);
    ClassDB::bind_method(D_METHOD("get_combine_mode"), &SDFGrid2DCanvas::get_combine_mode);

    ClassDB::bind_method(D_METHOD("set_brush_radius", "radius"), &SDFGrid2DCanvas::set_brush_radius);
    ClassDB::bind_method(D_METHOD("get_brush_radius"), &SDFGrid2DCanvas::get_brush_radius);

    ClassDB::bind_method(D_METHOD("set_brush_smooth_k", "smooth_k"), &SDFGrid2DCanvas::set_brush_smooth_k);
    ClassDB::bind_method(D_METHOD("get_brush_smooth_k"), &SDFGrid2DCanvas::get_brush_smooth_k);

    ClassDB::bind_method(D_METHOD("set_show_distance_field", "show"), &SDFGrid2DCanvas::set_show_distance_field);
    ClassDB::bind_method(D_METHOD("get_show_distance_field"), &SDFGrid2DCanvas::get_show_distance_field);

    ClassDB::bind_method(D_METHOD("set_show_distance_rings", "show"), &SDFGrid2DCanvas::set_show_distance_rings);
    ClassDB::bind_method(D_METHOD("get_show_distance_rings"), &SDFGrid2DCanvas::get_show_distance_rings);

    ClassDB::bind_method(D_METHOD("set_show_iso_contours", "show"), &SDFGrid2DCanvas::set_show_iso_contours);
    ClassDB::bind_method(D_METHOD("get_show_iso_contours"), &SDFGrid2DCanvas::get_show_iso_contours);

    ClassDB::bind_method(D_METHOD("set_show_gradients", "show"), &SDFGrid2DCanvas::set_show_gradients);
    ClassDB::bind_method(D_METHOD("get_show_gradients"), &SDFGrid2DCanvas::get_show_gradients);

    ClassDB::bind_method(D_METHOD("get_probe_world_pos"), &SDFGrid2DCanvas::get_probe_world_pos);
}

SDFGrid2DCanvas::SDFGrid2DCanvas() {
    set_lock_aspect_ratio(true);
    set_pan_zoom_axes(true, true, true, true);
    set_view_bounds(godot::Vector2(-5.5f, -5.5f), godot::Vector2(5.5f, 5.5f));
    set_custom_minimum_size(godot::Vector2(0.0f, 260.0f));
}

void SDFGrid2DCanvas::set_sdf_grid_resource(const godot::Ref<SDFGrid2DResource> &p_sdf) {
    set_geometry_resource(p_sdf);
}

godot::Ref<SDFGrid2DResource> SDFGrid2DCanvas::get_sdf_grid_resource() const {
    return geometry_resource;
}

void SDFGrid2DCanvas::set_brush_tool(SDFCanvasBrushTool p_tool) {
    brush_tool_ = p_tool;
    queue_redraw();
}

void SDFGrid2DCanvas::set_combine_mode(GeometrySDFCombineMode p_mode) {
    combine_mode_ = p_mode;
}

void SDFGrid2DCanvas::set_brush_radius(float p_radius) {
    brush_radius_ = std::max(0.05f, p_radius);
    queue_redraw();
}

void SDFGrid2DCanvas::set_brush_smooth_k(float p_smooth_k) {
    brush_smooth_k_ = std::max(0.0f, p_smooth_k);
}

void SDFGrid2DCanvas::set_show_distance_field(bool p_show) {
    show_distance_field_ = p_show;
    queue_redraw();
}

void SDFGrid2DCanvas::set_show_distance_rings(bool p_show) {
    show_distance_rings_ = p_show;
    queue_redraw();
}

void SDFGrid2DCanvas::set_show_iso_contours(bool p_show) {
    show_iso_contours_ = p_show;
    queue_redraw();
}

void SDFGrid2DCanvas::set_show_gradients(bool p_show) {
    show_gradients_ = p_show;
    queue_redraw();
}

bool SDFGrid2DCanvas::_has_non_point_content() const {
    return get_sdf_grid_resource().is_valid();
}

godot::PackedVector2Array SDFGrid2DCanvas::_sample_curve_screen_points(const godot::Rect2 &p_plot) const {
    (void)p_plot;
    return godot::PackedVector2Array();
}

bool SDFGrid2DCanvas::_compute_world_bounds(godot::Rect2 &r_bounds) const {
    const godot::Ref<SDFGrid2DResource> sdf = get_sdf_grid_resource();
    if (sdf.is_null()) return false;
    r_bounds = sdf->get_world_bounds();
    return r_bounds.size.x > 1e-4f || r_bounds.size.y > 1e-4f;
}

void SDFGrid2DCanvas::_apply_brush_at_world(const godot::Vector2 &p_world_pos, bool p_is_segment_drag) {
    godot::Ref<SDFGrid2DResource> sdf = get_sdf_grid_resource();
    if (sdf.is_null()) return;

    switch (brush_tool_) {
        case SDF_TOOL_STAMP_CIRCLE:
            sdf->stamp_circle(p_world_pos, brush_radius_, combine_mode_, brush_smooth_k_);
            drag_modified = true;
            break;
        case SDF_TOOL_STAMP_BOX:
            sdf->stamp_box(
                p_world_pos,
                godot::Vector2(brush_radius_, brush_radius_ * 0.75f),
                combine_mode_,
                brush_smooth_k_
            );
            drag_modified = true;
            break;
        case SDF_TOOL_STAMP_SEGMENT:
            if (p_is_segment_drag) {
                // Restore snapshot before stamping current segment from start to current pos
                if (!drag_before_snapshot.is_empty()) {
                    sdf->apply_snapshot_state(drag_before_snapshot);
                }
                sdf->stamp_segment(
                    segment_drag_start_world_,
                    p_world_pos,
                    brush_radius_ * 0.45f,
                    combine_mode_,
                    brush_smooth_k_
                );
                drag_modified = true;
            }
            break;
        case SDF_TOOL_CARVE_ERASE:
            sdf->stamp_circle(
                p_world_pos,
                brush_radius_,
                (brush_smooth_k_ > 1e-4f) ? SDF_COMBINE_SMOOTH_SUBTRACT : SDF_COMBINE_SUBTRACT,
                brush_smooth_k_
            );
            drag_modified = true;
            break;
        case SDF_TOOL_PROBE_INSPECT:
        default:
            break;
    }
}

bool SDFGrid2DCanvas::_try_begin_custom_drag(
    const godot::Ref<godot::InputEvent> &p_event,
    const godot::Rect2 &p_plot
) {
    using namespace godot;
    Ref<InputEventMouseButton> mb = p_event;
    if (mb.is_null() || !mb->is_pressed() || mb->get_button_index() != MOUSE_BUTTON_LEFT) {
        return false;
    }
    const Vector2 mpos = mb->get_position();
    if (!p_plot.has_point(mpos)) return false;

    const Ref<SDFGrid2DResource> sdf = get_sdf_grid_resource();
    if (sdf.is_null()) return false;

    const Vector2 wpos = screen_to_world(mpos, p_plot);
    probe_world_pos_   = wpos;
    has_probe_point_   = true;

    if (brush_tool_ == SDF_TOOL_PROBE_INSPECT) {
        emit_signal("field_probed", wpos, sdf->sample_distance(wpos), sdf->sample_gradient(wpos));
        queue_redraw();
        return true;
    }

    segment_drag_start_world_ = wpos;
    _begin_snapshot_drag(BASE_DRAG_CUSTOM, 0, mpos, "Sculpt 2D SDF Field", static_cast<int>(brush_tool_));
    _apply_brush_at_world(wpos, brush_tool_ == SDF_TOOL_STAMP_SEGMENT);
    emit_signal("field_probed", wpos, sdf->sample_distance(wpos), sdf->sample_gradient(wpos));
    queue_redraw();
    return true;
}

bool SDFGrid2DCanvas::_update_custom_drag(
    const godot::Ref<godot::InputEvent> &p_event,
    const godot::Rect2 &p_plot
) {
    using namespace godot;
    Ref<InputEventMouseMotion> mm = p_event;
    if (mm.is_null()) return false;

    const Vector2 mpos = mm->get_position();
    const Vector2 wpos = screen_to_world(mpos, p_plot);
    probe_world_pos_   = wpos;
    has_probe_point_   = true;

    const Ref<SDFGrid2DResource> sdf = get_sdf_grid_resource();
    if (sdf.is_null()) return false;

    if (brush_tool_ != SDF_TOOL_PROBE_INSPECT) {
        _apply_brush_at_world(wpos, brush_tool_ == SDF_TOOL_STAMP_SEGMENT);
    }
    emit_signal("field_probed", wpos, sdf->sample_distance(wpos), sdf->sample_gradient(wpos));
    queue_redraw();
    return true;
}

void SDFGrid2DCanvas::_update_custom_hover(
    const godot::Vector2 &p_mouse_pos,
    const godot::Rect2 &p_plot
) {
    if (!p_plot.has_point(p_mouse_pos)) {
        if (has_probe_point_) {
            has_probe_point_ = false;
            queue_redraw();
        }
        return;
    }
    probe_world_pos_ = screen_to_world(p_mouse_pos, p_plot);
    has_probe_point_ = true;
    const godot::Ref<SDFGrid2DResource> sdf = get_sdf_grid_resource();
    if (sdf.is_valid()) {
        emit_signal(
            "field_probed",
            probe_world_pos_,
            sdf->sample_distance(probe_world_pos_),
            sdf->sample_gradient(probe_world_pos_)
        );
    }
    queue_redraw();
}

godot::Color SDFGrid2DCanvas::_evaluate_sdf_color(float p_d, float p_iso) const noexcept {
    using namespace godot;
    const float rel = p_d - p_iso;
    if (!std::isfinite(rel) || rel > 250.0f) {
        return Color(0.14f, 0.16f, 0.22f, 0.65f);
    }

    // Signed distance dual-polarity palette (Inigo Quilez style):
    // Negative (Inside): Teal / Deep Cyan; Positive (Outside): Warm Amber / Terracotta
    const Color base_col = (rel < 0.0f)
        ? Color(0.18f, 0.72f, 0.92f, 0.85f)
        : Color(0.94f, 0.56f, 0.24f, 0.85f);

    const float dist_atten = 1.0f - std::exp(-0.35f * std::abs(rel));
    Color col = base_col.darkened(dist_atten * 0.48f);

    if (show_distance_rings_) {
        const float wave = 0.82f + 0.18f * std::cos(rel * 12.56637f);
        col.r *= wave;
        col.g *= wave;
        col.b *= wave;
    }

    // Highlight zero-crossing iso band
    const float band = std::exp(-28.0f * std::abs(rel));
    col = col.lerp(Color(1.0f, 0.98f, 0.88f, 0.95f), band * 0.75f);
    return col;
}

void SDFGrid2DCanvas::_draw_plot_background_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    const Ref<SDFGrid2DResource> sdf = get_sdf_grid_resource();
    if (sdf.is_null()) return;

    const Vector2i res   = sdf->get_resolution();
    const Rect2 w_bounds = sdf->get_world_bounds();
    const float iso      = sdf->get_iso_value();
    std::span<const float> grid = sdf->get_grid_span();

    if (show_distance_field_ && res.x >= 2 && res.y >= 2 && grid.size() >= static_cast<size_t>(res.x * res.y)) {
        const float step_w = w_bounds.size.x / static_cast<float>(res.x - 1);
        const float step_h = w_bounds.size.y / static_cast<float>(res.y - 1);

        for (int y = 0; y < res.y - 1; ++y) {
            for (int x = 0; x < res.x - 1; ++x) {
                const Vector2 w0(
                    w_bounds.position.x + static_cast<float>(x) * step_w,
                    w_bounds.position.y + static_cast<float>(y) * step_h
                );
                const Vector2 w1(w0.x + step_w, w0.y + step_h);
                const Vector2 s0 = world_to_screen(w0, p_plot);
                const Vector2 s1 = world_to_screen(w1, p_plot);

                const Rect2 cell_rect(
                    Vector2(std::min(s0.x, s1.x), std::min(s0.y, s1.y)),
                    Vector2(std::max(1.0f, std::abs(s1.x - s0.x) + 0.75f), std::max(1.0f, std::abs(s1.y - s0.y) + 0.75f))
                );
                if (!p_plot.intersects(cell_rect)) continue;

                const float d00 = grid[static_cast<size_t>(y * res.x + x)];
                const float d10 = grid[static_cast<size_t>(y * res.x + (x + 1))];
                const float d01 = grid[static_cast<size_t>((y + 1) * res.x + x)];
                const float d11 = grid[static_cast<size_t>((y + 1) * res.x + (x + 1))];
                const float d_avg = 0.25f * (d00 + d10 + d01 + d11);

                draw_rect(cell_rect, _evaluate_sdf_color(d_avg, iso), true);
            }
        }
    }

    // Draw world_bounds frame
    const Vector2 b_min = world_to_screen(w_bounds.position, p_plot);
    const Vector2 b_max = world_to_screen(w_bounds.position + w_bounds.size, p_plot);
    const Rect2 scr_wb(
        Vector2(std::min(b_min.x, b_max.x), std::min(b_min.y, b_max.y)),
        Vector2(std::abs(b_max.x - b_min.x), std::abs(b_max.y - b_min.y))
    );
    draw_rect(scr_wb, Color(0.55f, 0.64f, 0.76f, 0.7f), false, 1.25f);
}

void SDFGrid2DCanvas::_draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    const Ref<SDFGrid2DResource> sdf = get_sdf_grid_resource();
    if (sdf.is_null()) return;

    const float iso = sdf->get_iso_value();

    // 1. Marching Squares Iso-Contour Segments
    if (show_iso_contours_) {
        const PackedVector2Array segs = sdf->extract_iso_contour_segments(iso);
        const Color iso_col(1.0f, 0.96f, 0.36f, 0.98f);
        for (int64_t i = 0; i + 1 < segs.size(); i += 2) {
            const Vector2 a_scr = world_to_screen(segs[i], p_plot);
            const Vector2 b_scr = world_to_screen(segs[i + 1], p_plot);
            draw_line(a_scr, b_scr, iso_col, 2.25f, true);
        }
    }

    // 2. Gradient Vector Arrows Sub-Grid
    if (show_gradients_) {
        const Rect2 wb = sdf->get_world_bounds();
        constexpr int GRAD_STEPS = 12;
        for (int gy = 1; gy < GRAD_STEPS; ++gy) {
            for (int gx = 1; gx < GRAD_STEPS; ++gx) {
                const Vector2 wp(
                    wb.position.x + (static_cast<float>(gx) / static_cast<float>(GRAD_STEPS)) * wb.size.x,
                    wb.position.y + (static_cast<float>(gy) / static_cast<float>(GRAD_STEPS)) * wb.size.y
                );
                const Vector2 g = sdf->sample_gradient(wp);
                if (g.length_squared() < 1e-6f) continue;
                const Vector2 dir = g.normalized();
                const Vector2 s0  = world_to_screen(wp, p_plot);
                const Vector2 s1  = world_to_screen(wp + dir * (wb.size.x * 0.035f), p_plot);
                draw_line(s0, s1, Color(0.88f, 0.95f, 1.0f, 0.72f), 1.2f);
                draw_circle(s1, 2.0f, Color(1.0f, 0.85f, 0.32f, 0.88f));
            }
        }
    }

    // 3. Interactive Brush Cursor / Probe Reticle
    if (has_probe_point_) {
        const Vector2 p_scr = world_to_screen(probe_world_pos_, p_plot);
        const Vector2 r_scr = world_to_screen(probe_world_pos_ + Vector2(brush_radius_, 0.0f), p_plot);
        const float px_rad  = std::max(4.0f, std::abs(r_scr.x - p_scr.x));

        if (brush_tool_ == SDF_TOOL_STAMP_BOX) {
            const Rect2 br(p_scr.x - px_rad, p_scr.y - px_rad * 0.75f, px_rad * 2.0f, px_rad * 1.5f);
            draw_rect(br, Color(0.45f, 0.96f, 0.68f, 0.85f), false, 1.5f);
        } else if (brush_tool_ != SDF_TOOL_PROBE_INSPECT) {
            draw_arc(
                p_scr,
                px_rad,
                0.0f,
                Math_TAU,
                32,
                (brush_tool_ == SDF_TOOL_CARVE_ERASE)
                    ? Color(0.98f, 0.42f, 0.42f, 0.9f)
                    : Color(0.45f, 0.96f, 0.68f, 0.9f),
                1.5f
            );
        } else {
            draw_circle(p_scr, 3.5f, Color(1.0f, 0.92f, 0.35f, 0.95f));
        }
    }
}

// ============================================================================
// SDFGrid2DControl Implementation
// ============================================================================

void SDFGrid2DControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_sdf_grid_resource", "sdf"), &SDFGrid2DControl::set_sdf_grid_resource);
    ClassDB::bind_method(D_METHOD("get_sdf_grid_resource"), &SDFGrid2DControl::get_sdf_grid_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "sdf_grid_resource", PROPERTY_HINT_RESOURCE_TYPE, "SDFGrid2DResource"),
        "set_sdf_grid_resource",
        "get_sdf_grid_resource"
    );

    ClassDB::bind_method(D_METHOD("get_sdf_canvas"), &SDFGrid2DControl::get_sdf_canvas);
    ClassDB::bind_method(D_METHOD("get_heatmap_grid"), &SDFGrid2DControl::get_heatmap_grid);
    ClassDB::bind_method(D_METHOD("refresh_ui"), &SDFGrid2DControl::refresh_ui);

    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &SDFGrid2DControl::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_fit_view_pressed"), &SDFGrid2DControl::_on_fit_view_pressed);
    ClassDB::bind_method(D_METHOD("_on_field_toggled", "show"), &SDFGrid2DControl::_on_field_toggled);
    ClassDB::bind_method(D_METHOD("_on_rings_toggled", "show"), &SDFGrid2DControl::_on_rings_toggled);
    ClassDB::bind_method(D_METHOD("_on_contours_toggled", "show"), &SDFGrid2DControl::_on_contours_toggled);
    ClassDB::bind_method(D_METHOD("_on_gradients_toggled", "show"), &SDFGrid2DControl::_on_gradients_toggled);
    ClassDB::bind_method(D_METHOD("_on_heatmap_view_toggled", "show"), &SDFGrid2DControl::_on_heatmap_view_toggled);
    ClassDB::bind_method(D_METHOD("_on_iso_value_changed", "val"), &SDFGrid2DControl::_on_iso_value_changed);

    ClassDB::bind_method(D_METHOD("_on_brush_tool_selected", "idx"), &SDFGrid2DControl::_on_brush_tool_selected);
    ClassDB::bind_method(D_METHOD("_on_combine_mode_selected", "idx"), &SDFGrid2DControl::_on_combine_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_brush_params_changed", "unused"), &SDFGrid2DControl::_on_brush_params_changed);
    ClassDB::bind_method(D_METHOD("_on_stamp_preset_ring_pressed"), &SDFGrid2DControl::_on_stamp_preset_ring_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_field_pressed"), &SDFGrid2DControl::_on_clear_field_pressed);

    ClassDB::bind_method(D_METHOD("_on_apply_resolution_pressed"), &SDFGrid2DControl::_on_apply_resolution_pressed);
    ClassDB::bind_method(D_METHOD("_on_evaluate_chladni_pressed"), &SDFGrid2DControl::_on_evaluate_chladni_pressed);
    ClassDB::bind_method(
        D_METHOD("_on_canvas_probed", "world_pos", "distance", "gradient"),
        &SDFGrid2DControl::_on_canvas_probed
    );
}

SDFGrid2DControl::SDFGrid2DControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _build_ui();
}

SDFGrid2DControl::~SDFGrid2DControl() {
    if (sdf_resource_.is_valid()) {
        const godot::Callable cb(this, "_on_resource_changed");
        if (sdf_resource_->is_connected("changed", cb)) {
            sdf_resource_->disconnect("changed", cb);
        }
    }
}

void SDFGrid2DControl::_build_ui() {
    using namespace godot;

    // 1. Top Overlay & Marching Squares Iso-Contour Scrubber Bar
    HBoxContainer *top_bar = memnew(HBoxContainer);
    top_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(top_bar);

    field_check_ = memnew(CheckBox);
    field_check_->set_text("Field");
    field_check_->set_pressed(true);
    field_check_->connect("toggled", Callable(this, "_on_field_toggled"));
    top_bar->add_child(field_check_);

    rings_check_ = memnew(CheckBox);
    rings_check_->set_text("Rings");
    rings_check_->set_pressed(true);
    rings_check_->connect("toggled", Callable(this, "_on_rings_toggled"));
    top_bar->add_child(rings_check_);

    contours_check_ = memnew(CheckBox);
    contours_check_->set_text("Marching Sq");
    contours_check_->set_pressed(true);
    contours_check_->connect("toggled", Callable(this, "_on_contours_toggled"));
    top_bar->add_child(contours_check_);

    gradients_check_ = memnew(CheckBox);
    gradients_check_->set_text("∇d Arrows");
    gradients_check_->set_pressed(false);
    gradients_check_->connect("toggled", Callable(this, "_on_gradients_toggled"));
    top_bar->add_child(gradients_check_);

    heatmap_view_check_ = memnew(CheckBox);
    heatmap_view_check_->set_text("HeatmapGrid");
    heatmap_view_check_->set_pressed(false);
    heatmap_view_check_->connect("toggled", Callable(this, "_on_heatmap_view_toggled"));
    top_bar->add_child(heatmap_view_check_);

    Button *fit_btn = memnew(Button);
    fit_btn->set_text("Fit View");
    fit_btn->connect("pressed", Callable(this, "_on_fit_view_pressed"));
    top_bar->add_child(fit_btn);

    // Iso-Value Scrubber Row
    HBoxContainer *iso_bar = memnew(HBoxContainer);
    iso_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(iso_bar);

    Label *iso_lbl = memnew(Label);
    iso_lbl->set_text("Iso-Contour Threshold:");
    iso_bar->add_child(iso_lbl);

    iso_slider_ = memnew(HSlider);
    iso_slider_->set_min(-3.0);
    iso_slider_->set_max(3.0);
    iso_slider_->set_step(0.02);
    iso_slider_->set_value(0.0);
    iso_slider_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    iso_slider_->connect("value_changed", Callable(this, "_on_iso_value_changed"));
    iso_bar->add_child(iso_slider_);

    iso_spin_ = memnew(SpinBox);
    iso_spin_->set_min(-20.0);
    iso_spin_->set_max(20.0);
    iso_spin_->set_step(0.02);
    iso_spin_->set_value(0.0);
    iso_spin_->connect("value_changed", Callable(this, "_on_iso_value_changed"));
    iso_bar->add_child(iso_spin_);

    // 2. Interactive 2D SDF Canvas + Collapsible HeatmapGrid Companion
    sdf_canvas_ = memnew(SDFGrid2DCanvas);
    sdf_canvas_->connect("field_probed", Callable(this, "_on_canvas_probed"));
    add_child(sdf_canvas_);

    heatmap_grid_ = memnew(HeatmapGrid);
    heatmap_grid_->set_custom_minimum_size(Vector2(0.0f, 160.0f));
    heatmap_grid_->set_visible(false);
    add_child(heatmap_grid_);

    // 3. Interactive CSG Brush & Boolean Combinator Toolbar
    HBoxContainer *brush_bar = memnew(HBoxContainer);
    brush_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(brush_bar);

    brush_tool_opt_ = memnew(OptionButton);
    brush_tool_opt_->add_item("Tool: Probe / Inspect", SDF_TOOL_PROBE_INSPECT);
    brush_tool_opt_->add_item("Tool: Stamp Circle", SDF_TOOL_STAMP_CIRCLE);
    brush_tool_opt_->add_item("Tool: Stamp Box", SDF_TOOL_STAMP_BOX);
    brush_tool_opt_->add_item("Tool: Drag Capsule", SDF_TOOL_STAMP_SEGMENT);
    brush_tool_opt_->add_item("Tool: Carve / Erase", SDF_TOOL_CARVE_ERASE);
    brush_tool_opt_->select(1);
    brush_tool_opt_->connect("item_selected", Callable(this, "_on_brush_tool_selected"));
    brush_bar->add_child(brush_tool_opt_);

    combine_mode_opt_ = memnew(OptionButton);
    combine_mode_opt_->add_item("CSG: Smooth Union", SDF_COMBINE_SMOOTH_UNION);
    combine_mode_opt_->add_item("CSG: Smooth Subtract", SDF_COMBINE_SMOOTH_SUBTRACT);
    combine_mode_opt_->add_item("CSG: Smooth Intersect", SDF_COMBINE_SMOOTH_INTERSECT);
    combine_mode_opt_->add_item("CSG: Sharp Union", SDF_COMBINE_UNION);
    combine_mode_opt_->add_item("CSG: Sharp Subtract", SDF_COMBINE_SUBTRACT);
    combine_mode_opt_->add_item("CSG: Sharp Intersect", SDF_COMBINE_INTERSECT);
    combine_mode_opt_->add_item("CSG: Replace", SDF_COMBINE_REPLACE);
    combine_mode_opt_->select(0);
    combine_mode_opt_->connect("item_selected", Callable(this, "_on_combine_mode_selected"));
    brush_bar->add_child(combine_mode_opt_);

    brush_radius_spin_ = memnew(SpinBox);
    brush_radius_spin_->set_min(0.05);
    brush_radius_spin_->set_max(20.0);
    brush_radius_spin_->set_step(0.05);
    brush_radius_spin_->set_value(1.25);
    brush_radius_spin_->set_prefix("R:");
    brush_radius_spin_->connect("value_changed", Callable(this, "_on_brush_params_changed"));
    brush_bar->add_child(brush_radius_spin_);

    smooth_k_spin_ = memnew(SpinBox);
    smooth_k_spin_->set_min(0.0);
    smooth_k_spin_->set_max(5.0);
    smooth_k_spin_->set_step(0.05);
    smooth_k_spin_->set_value(0.45);
    smooth_k_spin_->set_prefix("k:");
    smooth_k_spin_->connect("value_changed", Callable(this, "_on_brush_params_changed"));
    brush_bar->add_child(smooth_k_spin_);

    Button *preset_ring_btn = memnew(Button);
    preset_ring_btn->set_text("Demo CSG Ring");
    preset_ring_btn->connect("pressed", Callable(this, "_on_stamp_preset_ring_pressed"));
    brush_bar->add_child(preset_ring_btn);

    Button *clear_btn = memnew(Button);
    clear_btn->set_text("Clear");
    clear_btn->connect("pressed", Callable(this, "_on_clear_field_pressed"));
    brush_bar->add_child(clear_btn);

    // 4. Grid Resolution & Chladni Standing-Wave Modal Synthesizer (`IdeamCymatics` Bridge)
    HBoxContainer *chladni_bar = memnew(HBoxContainer);
    chladni_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(chladni_bar);

    Label *res_lbl = memnew(Label);
    res_lbl->set_text("Grid:");
    chladni_bar->add_child(res_lbl);

    res_x_spin_ = memnew(SpinBox);
    res_x_spin_->set_min(8.0);
    res_x_spin_->set_max(256.0);
    res_x_spin_->set_step(8.0);
    res_x_spin_->set_value(64.0);
    chladni_bar->add_child(res_x_spin_);

    res_y_spin_ = memnew(SpinBox);
    res_y_spin_->set_min(8.0);
    res_y_spin_->set_max(256.0);
    res_y_spin_->set_step(8.0);
    res_y_spin_->set_value(64.0);
    chladni_bar->add_child(res_y_spin_);

    Button *apply_res_btn = memnew(Button);
    apply_res_btn->set_text("Resize");
    apply_res_btn->connect("pressed", Callable(this, "_on_apply_resolution_pressed"));
    chladni_bar->add_child(apply_res_btn);

    Label *ch_lbl = memnew(Label);
    ch_lbl->set_text("  Chladni (n,m,a,b):");
    chladni_bar->add_child(ch_lbl);

    chladni_n_spin_ = memnew(SpinBox);
    chladni_n_spin_->set_min(1.0);
    chladni_n_spin_->set_max(16.0);
    chladni_n_spin_->set_step(0.5);
    chladni_n_spin_->set_value(3.0);
    chladni_bar->add_child(chladni_n_spin_);

    chladni_m_spin_ = memnew(SpinBox);
    chladni_m_spin_->set_min(1.0);
    chladni_m_spin_->set_max(16.0);
    chladni_m_spin_->set_step(0.5);
    chladni_m_spin_->set_value(5.0);
    chladni_bar->add_child(chladni_m_spin_);

    chladni_a_spin_ = memnew(SpinBox);
    chladni_a_spin_->set_min(-4.0);
    chladni_a_spin_->set_max(4.0);
    chladni_a_spin_->set_step(0.25);
    chladni_a_spin_->set_value(1.0);
    chladni_bar->add_child(chladni_a_spin_);

    chladni_b_spin_ = memnew(SpinBox);
    chladni_b_spin_->set_min(-4.0);
    chladni_b_spin_->set_max(4.0);
    chladni_b_spin_->set_step(0.25);
    chladni_b_spin_->set_value(-1.0);
    chladni_bar->add_child(chladni_b_spin_);

    Button *eval_chladni_btn = memnew(Button);
    eval_chladni_btn->set_text("Eval Chladni");
    eval_chladni_btn->connect("pressed", Callable(this, "_on_evaluate_chladni_pressed"));
    chladni_bar->add_child(eval_chladni_btn);

    // 5. Telemetry & Probe Footer
    telemetry_label_ = memnew(Label);
    telemetry_label_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    add_child(telemetry_label_);
}

void SDFGrid2DControl::set_sdf_grid_resource(const godot::Ref<SDFGrid2DResource> &p_sdf) {
    if (sdf_resource_ == p_sdf) return;

    const godot::Callable cb(this, "_on_resource_changed");
    if (sdf_resource_.is_valid() && sdf_resource_->is_connected("changed", cb)) {
        sdf_resource_->disconnect("changed", cb);
    }

    sdf_resource_ = p_sdf;

    if (sdf_canvas_) {
        sdf_canvas_->set_sdf_grid_resource(sdf_resource_);
        sdf_canvas_->fit_view_to_data(true, true, 0.1f);
    }

    if (sdf_resource_.is_valid() && !sdf_resource_->is_connected("changed", cb)) {
        sdf_resource_->connect("changed", cb);
    }

    refresh_ui();
}

void SDFGrid2DControl::_on_resource_changed() {
    refresh_ui();
}

void SDFGrid2DControl::refresh_ui() {
    is_updating_ui_ = true;

    if (sdf_resource_.is_valid()) {
        const godot::Vector2i res = sdf_resource_->get_resolution();
        const float iso           = sdf_resource_->get_iso_value();
        if (res_x_spin_) res_x_spin_->set_value(res.x);
        if (res_y_spin_) res_y_spin_->set_value(res.y);
        if (iso_slider_) iso_slider_->set_value(iso);
        if (iso_spin_) iso_spin_->set_value(iso);
    }

    _sync_heatmap_and_telemetry();
    if (sdf_canvas_) {
        sdf_canvas_->queue_redraw();
    }

    is_updating_ui_ = false;
}

void SDFGrid2DControl::_sync_heatmap_and_telemetry() {
    using namespace godot;
    if (sdf_resource_.is_null()) {
        if (telemetry_label_) telemetry_label_->set_text("No SDFGrid2DResource bound.");
        return;
    }

    const Vector2 min_max = sdf_resource_->get_min_max_distance();
    if (heatmap_grid_ && heatmap_grid_->is_visible()) {
        const float span = std::max(0.5f, std::max(std::abs(min_max.x), std::abs(min_max.y)));
        sdf_resource_->sync_to_heatmap_grid(heatmap_grid_, -span, span);
    }

    if (telemetry_label_) {
        const Vector2i res = sdf_resource_->get_resolution();
        const float iso    = sdf_resource_->get_iso_value();
        const int64_t segs = sdf_resource_->extract_iso_contour_segments(iso).size() / 2;
        telemetry_label_->set_text(
            "Resolution: " + String::num_int64(res.x) + "x" + String::num_int64(res.y) +
            "  |  d_min: " + String::num(min_max.x, 2) +
            ", d_max: " + String::num(min_max.y, 2) +
            "  |  Marching Squares Segments (iso=" + String::num(iso, 2) + "): " + String::num_int64(segs)
        );
    }
}

void SDFGrid2DControl::_on_fit_view_pressed() {
    if (sdf_canvas_) {
        sdf_canvas_->fit_view_to_data(true, true, 0.1f);
    }
}

void SDFGrid2DControl::_on_field_toggled(bool p_show) {
    if (sdf_canvas_) sdf_canvas_->set_show_distance_field(p_show);
}

void SDFGrid2DControl::_on_rings_toggled(bool p_show) {
    if (sdf_canvas_) sdf_canvas_->set_show_distance_rings(p_show);
}

void SDFGrid2DControl::_on_contours_toggled(bool p_show) {
    if (sdf_canvas_) sdf_canvas_->set_show_iso_contours(p_show);
}

void SDFGrid2DControl::_on_gradients_toggled(bool p_show) {
    if (sdf_canvas_) sdf_canvas_->set_show_gradients(p_show);
}

void SDFGrid2DControl::_on_heatmap_view_toggled(bool p_show) {
    if (heatmap_grid_) {
        heatmap_grid_->set_visible(p_show);
        _sync_heatmap_and_telemetry();
    }
}

void SDFGrid2DControl::_on_iso_value_changed(double p_val) {
    if (is_updating_ui_ || sdf_resource_.is_null()) return;
    is_updating_ui_ = true;
    if (iso_slider_) iso_slider_->set_value(p_val);
    if (iso_spin_) iso_spin_->set_value(p_val);
    sdf_resource_->set_iso_value(static_cast<float>(p_val));
    _sync_heatmap_and_telemetry();
    is_updating_ui_ = false;
}

void SDFGrid2DControl::_on_brush_tool_selected(int p_idx) {
    if (!sdf_canvas_ || !brush_tool_opt_) return;
    sdf_canvas_->set_brush_tool(static_cast<SDFCanvasBrushTool>(brush_tool_opt_->get_item_id(p_idx)));
}

void SDFGrid2DControl::_on_combine_mode_selected(int p_idx) {
    if (!sdf_canvas_ || !combine_mode_opt_) return;
    sdf_canvas_->set_combine_mode(static_cast<GeometrySDFCombineMode>(combine_mode_opt_->get_item_id(p_idx)));
}

void SDFGrid2DControl::_on_brush_params_changed(double p_unused) {
    (void)p_unused;
    if (!sdf_canvas_) return;
    if (brush_radius_spin_) sdf_canvas_->set_brush_radius(static_cast<float>(brush_radius_spin_->get_value()));
    if (smooth_k_spin_) sdf_canvas_->set_brush_smooth_k(static_cast<float>(smooth_k_spin_->get_value()));
}

void SDFGrid2DControl::_on_stamp_preset_ring_pressed() {
    if (sdf_resource_.is_null()) return;
    const godot::Dictionary before = sdf_resource_->get_snapshot_state();
    const godot::Rect2 wb          = sdf_resource_->get_world_bounds();
    const godot::Vector2 center    = wb.position + wb.size * 0.5f;
    const float r_outer            = std::min(wb.size.x, wb.size.y) * 0.34f;
    sdf_resource_->clear_field(1000.0f);
    sdf_resource_->stamp_circle(center + godot::Vector2(-r_outer * 0.35f, 0.0f), r_outer, SDF_COMBINE_REPLACE, 0.0f);
    sdf_resource_->stamp_box(
        center + godot::Vector2(r_outer * 0.45f, 0.0f),
        godot::Vector2(r_outer * 0.78f, r_outer * 0.62f),
        SDF_COMBINE_SMOOTH_UNION,
        r_outer * 0.32f
    );
    sdf_resource_->stamp_circle(center, r_outer * 0.42f, SDF_COMBINE_SMOOTH_SUBTRACT, r_outer * 0.18f);
    sdf_resource_->commit_snapshot_action("Generate Demo Smooth CSG SDF", before, sdf_resource_->get_snapshot_state());
    refresh_ui();
}

void SDFGrid2DControl::_on_clear_field_pressed() {
    if (sdf_resource_.is_null()) return;
    sdf_resource_->commit_clear_field(1000.0f);
    refresh_ui();
}

void SDFGrid2DControl::_on_apply_resolution_pressed() {
    if (sdf_resource_.is_null() || !res_x_spin_ || !res_y_spin_) return;
    const godot::Vector2i new_res(
        static_cast<int32_t>(res_x_spin_->get_value()),
        static_cast<int32_t>(res_y_spin_->get_value())
    );
    sdf_resource_->commit_set_resolution(new_res);
    refresh_ui();
}

void SDFGrid2DControl::_on_evaluate_chladni_pressed() {
    if (sdf_resource_.is_null()) return;
    const float n = chladni_n_spin_ ? static_cast<float>(chladni_n_spin_->get_value()) : 3.0f;
    const float m = chladni_m_spin_ ? static_cast<float>(chladni_m_spin_->get_value()) : 5.0f;
    const float a = chladni_a_spin_ ? static_cast<float>(chladni_a_spin_->get_value()) : 1.0f;
    const float b = chladni_b_spin_ ? static_cast<float>(chladni_b_spin_->get_value()) : -1.0f;
    sdf_resource_->commit_evaluate_chladni_field(n, m, a, b);
    refresh_ui();
}

void SDFGrid2DControl::_on_canvas_probed(
    const godot::Vector2 &p_world_pos,
    double p_distance,
    const godot::Vector2 &p_gradient
) {
    using namespace godot;
    if (!telemetry_label_ || sdf_resource_.is_null()) return;
    const Vector2i res = sdf_resource_->get_resolution();
    const float iso    = sdf_resource_->get_iso_value();
    const int64_t segs = sdf_resource_->extract_iso_contour_segments(iso).size() / 2;

    telemetry_label_->set_text(
        "Grid: " + String::num_int64(res.x) + "x" + String::num_int64(res.y) +
        " | Iso Segments: " + String::num_int64(segs) +
        " | Probe (" + String::num(p_world_pos.x, 2) + ", " + String::num(p_world_pos.y, 2) +
        ") -> d = " + String::num(p_distance, 3) +
        ", ∇d = (" + String::num(p_gradient.x, 2) + ", " + String::num(p_gradient.y, 2) + ")"
    );
}

} // namespace ideam::godot_ext