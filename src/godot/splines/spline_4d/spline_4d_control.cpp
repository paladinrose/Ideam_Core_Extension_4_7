#include "spline_4d_control.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ideam::godot_ext {

// ============================================================================
// Spline4DWChannelCanvas Implementation (Specializing SplineCanvasBase)
// ============================================================================

void Spline4DWChannelCanvas::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &Spline4DWChannelCanvas::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &Spline4DWChannelCanvas::get_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_spline",
        "get_spline"
    );

    ClassDB::bind_method(D_METHOD("set_active_channel", "channel"), &Spline4DWChannelCanvas::set_active_channel);
    ClassDB::bind_method(D_METHOD("get_active_channel"), &Spline4DWChannelCanvas::get_active_channel);

    ClassDB::bind_method(D_METHOD("set_show_color_gradient", "show"), &Spline4DWChannelCanvas::set_show_color_gradient);
    ClassDB::bind_method(D_METHOD("get_show_color_gradient"), &Spline4DWChannelCanvas::get_show_color_gradient);

    ClassDB::bind_method(D_METHOD("set_hsva_gradient_preview", "hsva"), &Spline4DWChannelCanvas::set_hsva_gradient_preview);
    ClassDB::bind_method(D_METHOD("get_hsva_gradient_preview"), &Spline4DWChannelCanvas::get_hsva_gradient_preview);
}

Spline4DWChannelCanvas::Spline4DWChannelCanvas() {
    set_custom_minimum_size(godot::Vector2(0.0f, 175.0f));
    set_point_style(POINT_STYLE_FADER_CAP);
    set_anchor_drag_constraint(DRAG_AXIS_Y_ONLY);
    set_lock_aspect_ratio(false);
    set_pan_zoom_axes(false, true, false, true);
    set_plot_margins(44.0f, 18.0f, 14.0f, 32.0f);
    set_view_bounds(godot::Vector2(0.0f, -0.2f), godot::Vector2(1.0f, 1.2f));
}

Spline4DWChannelCanvas::~Spline4DWChannelCanvas() = default;

void Spline4DWChannelCanvas::set_spline(const godot::Ref<Spline4DResource> &p_spline) {
    set_spline_resource(p_spline);
}

godot::Ref<Spline4DResource> Spline4DWChannelCanvas::get_spline() const {
    return godot::Ref<Spline4DResource>(godot::Object::cast_to<Spline4DResource>(spline_resource.ptr()));
}

void Spline4DWChannelCanvas::set_active_channel(int p_channel) {
    const int clamped = std::clamp(p_channel, 0, 3);
    if (active_channel == clamped) return;
    active_channel = clamped;

    static const godot::Color CHANNEL_COLORS[4] = {
        godot::Color(0.95f, 0.45f, 0.45f, 0.96f), // X / Red
        godot::Color(0.45f, 0.90f, 0.50f, 0.96f), // Y / Green
        godot::Color(0.45f, 0.68f, 0.98f, 0.96f), // Z / Blue
        godot::Color(0.88f, 0.55f, 0.95f, 0.96f)  // W / Scale / Roll / Alpha
    };
    curve_color = CHANNEL_COLORS[active_channel];
    fit_view_to_data(false, true, static_cast<real_t>(0.15));
    queue_redraw();
}

void Spline4DWChannelCanvas::set_show_color_gradient(bool p_show) {
    show_color_gradient = p_show;
    queue_redraw();
}

void Spline4DWChannelCanvas::set_hsva_gradient_preview(bool p_hsva) {
    hsva_gradient_preview = p_hsva;
    queue_redraw();
}

float Spline4DWChannelCanvas::_extract_channel(const godot::Vector4 &p_v) const noexcept {
    switch (active_channel) {
        case 0: return p_v.x;
        case 1: return p_v.y;
        case 2: return p_v.z;
        case 3:
        default: return p_v.w;
    }
}

void Spline4DWChannelCanvas::_inject_channel(godot::Vector4 &r_v, float p_val) const noexcept {
    switch (active_channel) {
        case 0: r_v.x = p_val; break;
        case 1: r_v.y = p_val; break;
        case 2: r_v.z = p_val; break;
        case 3:
        default: r_v.w = p_val; break;
    }
}

godot::Rect2 Spline4DWChannelCanvas::_get_gradient_strip_rect(const godot::Rect2 &p_plot) const {
    return godot::Rect2(p_plot.position.x, p_plot.position.y + p_plot.size.y + 4.0f, p_plot.size.x, 22.0f);
}

std::vector<real_t> Spline4DWChannelCanvas::_compute_fader_norms() const {
    std::vector<real_t> norms;
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null()) return norms;

    const int pt_count  = s4d->get_point_count();
    const int sec_count = s4d->get_section_count();
    if (pt_count <= 0) return norms;

    if (pt_count == 1 || sec_count <= 0) {
        norms.push_back(static_cast<real_t>(0.5));
        return norms;
    }

    const real_t total_w = std::max(static_cast<real_t>(1e-6), s4d->get_total_weight());
    norms.reserve(static_cast<size_t>(sec_count + 1));
    real_t accum = static_cast<real_t>(0.0);
    norms.push_back(static_cast<real_t>(0.0));

    for (int s = 0; s < sec_count; ++s) {
        accum += std::max(static_cast<real_t>(0.0001), s4d->get_section_weight(s));
        norms.push_back(std::clamp(accum / total_w, static_cast<real_t>(0.0), static_cast<real_t>(1.0)));
    }
    norms.back() = static_cast<real_t>(1.0);
    return norms;
}

godot::Vector2 Spline4DWChannelCanvas::_get_point_world_pos(int p_index) const {
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null() || p_index < 0 || p_index >= s4d->get_point_count()) {
        return godot::Vector2();
    }
    const std::vector<real_t> norms = _compute_fader_norms();
    const float x = (p_index < static_cast<int>(norms.size()))
        ? static_cast<float>(norms[static_cast<size_t>(p_index)])
        : 0.0f;
    const float y = _extract_channel(s4d->get_point_position(p_index));
    return godot::Vector2(x, y);
}

void Spline4DWChannelCanvas::_set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) {
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null()) return;
    godot::Vector4 pos = s4d->get_point_position(p_index);
    _inject_channel(pos, p_world_pos.y);
    s4d->set_point_position(p_index, pos);
}

bool Spline4DWChannelCanvas::_has_wrap_ghost_endpoint(godot::Vector2 &r_ghost_world_pos) const {
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null() || !s4d->get_wrap() || s4d->get_point_count() < 2) {
        return false;
    }
    r_ghost_world_pos = godot::Vector2(1.0f, _extract_channel(s4d->get_point_position(0)));
    return true;
}

godot::Vector2 Spline4DWChannelCanvas::_get_back_handle_screen_pos(
    int p_index,
    const godot::Rect2 &p_plot
) const {
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null()) return godot::Vector2();
    const godot::Vector2 anchor_w   = _get_point_world_pos(p_index);
    const godot::Vector2 anchor_scr = world_to_screen(anchor_w, p_plot);
    const float bh_val              = anchor_w.y + _extract_channel(s4d->get_point_back_handle(p_index));
    const float bhy                 = world_to_screen(godot::Vector2(anchor_w.x, bh_val), p_plot).y;
    const float bhx                 = std::max(p_plot.position.x, anchor_scr.x - 18.0f);
    return godot::Vector2(bhx, bhy);
}

void Spline4DWChannelCanvas::_set_back_handle_from_screen_pos(
    int p_index,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null()) return;
    const float anchor_v   = _extract_channel(s4d->get_point_position(p_index));
    const float target_v   = screen_to_world(p_screen_pos, p_plot).y;
    godot::Vector4 handle  = s4d->get_point_back_handle(p_index);
    _inject_channel(handle, target_v - anchor_v);
    s4d->set_point_back_handle(p_index, handle, true);
}

godot::Vector2 Spline4DWChannelCanvas::_get_fore_handle_screen_pos(
    int p_index,
    const godot::Rect2 &p_plot
) const {
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null()) return godot::Vector2();
    const godot::Vector2 anchor_w   = _get_point_world_pos(p_index);
    const godot::Vector2 anchor_scr = world_to_screen(anchor_w, p_plot);
    const float fh_val              = anchor_w.y + _extract_channel(s4d->get_point_fore_handle(p_index));
    const float fhy                 = world_to_screen(godot::Vector2(anchor_w.x, fh_val), p_plot).y;
    const float fhx                 = std::min(p_plot.position.x + p_plot.size.x, anchor_scr.x + 18.0f);
    return godot::Vector2(fhx, fhy);
}

void Spline4DWChannelCanvas::_set_fore_handle_from_screen_pos(
    int p_index,
    const godot::Vector2 &p_screen_pos,
    const godot::Rect2 &p_plot
) {
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null()) return;
    const float anchor_v   = _extract_channel(s4d->get_point_position(p_index));
    const float target_v   = screen_to_world(p_screen_pos, p_plot).y;
    godot::Vector4 handle  = s4d->get_point_fore_handle(p_index);
    _inject_channel(handle, target_v - anchor_v);
    s4d->set_point_fore_handle(p_index, handle, true);
}

godot::PackedVector2Array Spline4DWChannelCanvas::_sample_curve_screen_points(
    const godot::Rect2 &p_plot
) const {
    godot::PackedVector2Array pts;
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null() || s4d->get_section_count() <= 0) return pts;

    constexpr int SAMPLES = 128;
    pts.resize(SAMPLES + 1);
    for (int i = 0; i <= SAMPLES; ++i) {
        const real_t pct = static_cast<real_t>(i) / static_cast<real_t>(SAMPLES);
        const float val  = _extract_channel(s4d->sample(pct, true));
        pts.set(i, world_to_screen(godot::Vector2(static_cast<float>(pct), val), p_plot));
    }
    return pts;
}

bool Spline4DWChannelCanvas::_compute_world_bounds(godot::Rect2 &r_bounds) const {
    godot::Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null() || s4d->get_point_count() <= 0) return false;

    const godot::PackedVector4Array pts = s4d->get_smoothed_points(8);
    if (pts.is_empty()) return false;

    float min_v = _extract_channel(pts[0]);
    float max_v = min_v;
    for (int i = 1; i < pts.size(); ++i) {
        const float v = _extract_channel(pts[i]);
        min_v = std::min(min_v, v);
        max_v = std::max(max_v, v);
    }
    r_bounds = godot::Rect2(0.0f, min_v, 1.0f, max_v - min_v);
    return true;
}

void Spline4DWChannelCanvas::_draw_plot_foreground_overlay(const godot::Rect2 &p_plot) {
    using namespace godot;
    Ref<Spline4DResource> s4d = get_spline();
    if (s4d.is_null() || !show_color_gradient) return;

    // Render live evaluated 4D RGBA / HSVA Color Gradient Strip along the bottom
    const Rect2 grad_rect = _get_gradient_strip_rect(p_plot);
    draw_rect(grad_rect, Color(0.10f, 0.12f, 0.15f, 1.0f), true);

    constexpr int BANDS = 96;
    const float band_w  = grad_rect.size.x / static_cast<float>(BANDS);
    for (int b = 0; b < BANDS; ++b) {
        const real_t pct = (static_cast<real_t>(b) + static_cast<real_t>(0.5)) / static_cast<real_t>(BANDS);
        const Color col  = s4d->sample_color(pct, true, hsva_gradient_preview);
        const Rect2 slice(
            grad_rect.position.x + static_cast<float>(b) * band_w,
            grad_rect.position.y + 1.0f,
            band_w + 1.0f,
            grad_rect.size.y - 2.0f
        );
        draw_rect(slice, col, true);
    }
    draw_rect(grad_rect, Color(0.32f, 0.38f, 0.46f, 1.0f), false, 1.0f);
}

// ============================================================================
// Spline4DControl Implementation
// ============================================================================

void Spline4DControl::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_spline_4d", "spline"), &Spline4DControl::set_spline_4d);
    ClassDB::bind_method(D_METHOD("get_spline_4d"), &Spline4DControl::get_spline_4d);
    ClassDB::bind_method(D_METHOD("get_w_channel_canvas"), &Spline4DControl::get_w_channel_canvas);

    ClassDB::bind_method(D_METHOD("set_bound_gizmo_node", "node"), &Spline4DControl::set_bound_gizmo_node);
    ClassDB::bind_method(D_METHOD("get_bound_gizmo_node"), &Spline4DControl::get_bound_gizmo_node);

    ClassDB::bind_method(D_METHOD("_on_preset_tapered_line"), &Spline4DControl::_on_preset_tapered_line);
    ClassDB::bind_method(D_METHOD("_on_preset_banked_circle"), &Spline4DControl::_on_preset_banked_circle);
    ClassDB::bind_method(D_METHOD("_on_preset_color_gradient"), &Spline4DControl::_on_preset_color_gradient);
    ClassDB::bind_method(D_METHOD("_on_rotate_y_45_pressed"), &Spline4DControl::_on_rotate_y_45_pressed);
    ClassDB::bind_method(D_METHOD("_on_rotate_x_45_pressed"), &Spline4DControl::_on_rotate_x_45_pressed);
    ClassDB::bind_method(D_METHOD("_on_channel_selected", "item_idx"), &Spline4DControl::_on_channel_selected);
    ClassDB::bind_method(D_METHOD("_on_fit_w_pressed"), &Spline4DControl::_on_fit_w_pressed);
    ClassDB::bind_method(D_METHOD("_on_hsva_preview_toggled", "hsva"), &Spline4DControl::_on_hsva_preview_toggled);
    ClassDB::bind_method(D_METHOD("_on_show_w_canvas_toggled", "show"), &Spline4DControl::_on_show_w_canvas_toggled);
    ClassDB::bind_method(D_METHOD("_on_canvas_point_selected", "point_index"), &Spline4DControl::_on_canvas_point_selected);
    ClassDB::bind_method(D_METHOD("_on_canvas_section_selected", "section_index"), &Spline4DControl::_on_canvas_section_selected);

    ClassDB::bind_method(D_METHOD("_on_gizmo_node_point_selected", "point_index"), &Spline4DControl::_on_gizmo_node_point_selected);
    ClassDB::bind_method(D_METHOD("_on_gizmo_node_section_selected", "section_index"), &Spline4DControl::_on_gizmo_node_section_selected);
}

Spline4DControl::Spline4DControl() {
    _build_4d_extensions();
}

Spline4DControl::~Spline4DControl() {
    set_bound_gizmo_node(nullptr);
}

void Spline4DControl::_build_4d_extensions() {
    using namespace godot;

    if (custom_header_slot) {
        // Row 1: 4D Presets, XYZ Rotations & Channel Equalizer Controls
        HBoxContainer *preset_bar = memnew(HBoxContainer);
        preset_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        preset_bar->add_theme_constant_override("separation", 6);
        custom_header_slot->add_child(preset_bar);

        Label *preset_lbl = memnew(Label);
        preset_lbl->set_text("4D Presets:");
        preset_bar->add_child(preset_lbl);

        preset_points_spin = memnew(SpinBox);
        preset_points_spin->set_min(2.0);
        preset_points_spin->set_max(64.0);
        preset_points_spin->set_step(1.0);
        preset_points_spin->set_value(6.0);
        preset_points_spin->set_custom_minimum_size(Vector2(52.0f, 0.0f));
        preset_points_spin->set_tooltip_text("Point count for Tapered Line and Banked Circle presets.");
        preset_bar->add_child(preset_points_spin);

        Button *btn_taper = memnew(Button);
        btn_taper->set_text("Tapered Line");
        btn_taper->set_tooltip_text("Generate a 3D line along -Z with W tapering from 1.0 to 0.25.");
        btn_taper->connect("pressed", Callable(this, "_on_preset_tapered_line"));
        preset_bar->add_child(btn_taper);

        Button *btn_bank = memnew(Button);
        btn_bank->set_text("Banked Circle");
        btn_bank->set_tooltip_text("Generate a closed XZ circle with oscillating W banking roll.");
        btn_bank->connect("pressed", Callable(this, "_on_preset_banked_circle"));
        preset_bar->add_child(btn_bank);

        Button *btn_grad = memnew(Button);
        btn_grad->set_text("RGBA Gradient");
        btn_grad->set_tooltip_text("Generate a 4-stop smooth RGBA color gradient.");
        btn_grad->connect("pressed", Callable(this, "_on_preset_color_gradient"));
        preset_bar->add_child(btn_grad);

        Button *btn_rot_y = memnew(Button);
        btn_rot_y->set_text("↻ Y 45°");
        btn_rot_y->set_tooltip_text("Rotate XYZ by 45° around Y while preserving W.");
        btn_rot_y->connect("pressed", Callable(this, "_on_rotate_y_45_pressed"));
        preset_bar->add_child(btn_rot_y);

        Control *spacer = memnew(Control);
        spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        preset_bar->add_child(spacer);

        channel_select = memnew(OptionButton);
        channel_select->add_item("Track: W (Scale/Roll/A)", 3);
        channel_select->add_item("Track: X (R/Hue)", 0);
        channel_select->add_item("Track: Y (G/Sat)", 1);
        channel_select->add_item("Track: Z (B/Val)", 2);
        channel_select->select(0);
        channel_select->set_tooltip_text("Select which 4D component is plotted on the equalizer fader tracks.");
        channel_select->connect("item_selected", Callable(this, "_on_channel_selected"));
        preset_bar->add_child(channel_select);

        Button *btn_fit = memnew(Button);
        btn_fit->set_text("Fit");
        btn_fit->set_tooltip_text("Fit the equalizer vertical axis to the active channel's range.");
        btn_fit->connect("pressed", Callable(this, "_on_fit_w_pressed"));
        preset_bar->add_child(btn_fit);

        hsva_preview_check = memnew(CheckBox);
        hsva_preview_check->set_text("HSVA");
        hsva_preview_check->set_tooltip_text("Interpret (X,Y,Z,W) as (Hue, Sat, Val, Alpha) in the bottom gradient bar.");
        hsva_preview_check->connect("toggled", Callable(this, "_on_hsva_preview_toggled"));
        preset_bar->add_child(hsva_preview_check);

        show_w_canvas_check = memnew(CheckBox);
        show_w_canvas_check->set_text("W/Gradient Canvas");
        show_w_canvas_check->set_pressed(true);
        show_w_canvas_check->set_tooltip_text("Show or hide the 4D Channel Equalizer & Color Gradient preview canvas.");
        show_w_canvas_check->connect("toggled", Callable(this, "_on_show_w_canvas_toggled"));
        preset_bar->add_child(show_w_canvas_check);

        // Row 2: Live 3D AABB + W-Range Telemetry Readout
        bounds_telemetry_label = memnew(Label);
        bounds_telemetry_label->set_text("3D AABB: (0, 0, 0) | W Range: [0.00, 1.00]");
        bounds_telemetry_label->add_theme_color_override("font_color", Color(0.84f, 0.68f, 0.96f, 0.92f));
        custom_header_slot->add_child(bounds_telemetry_label);
    }

    if (visual_editor_slot) {
        w_canvas = memnew(Spline4DWChannelCanvas);
        w_canvas->connect("point_selected", Callable(this, "_on_canvas_point_selected"));
        w_canvas->connect("section_selected", Callable(this, "_on_canvas_section_selected"));
        visual_editor_slot->add_child(w_canvas);
    }
}

void Spline4DControl::set_spline_4d(const godot::Ref<Spline4DResource> &p_spline) {
    set_spline_resource(p_spline);
}

godot::Ref<Spline4DResource> Spline4DControl::get_spline_4d() const {
    return godot::Ref<Spline4DResource>(godot::Object::cast_to<Spline4DResource>(spline_resource.ptr()));
}

SplineNode4D *Spline4DControl::_get_bound_gizmo_node() const {
    if (bound_gizmo_node_id.is_null()) return nullptr;
    return godot::Object::cast_to<SplineNode4D>(godot::ObjectDB::get_instance(bound_gizmo_node_id));
}

void Spline4DControl::set_bound_gizmo_node(SplineNode4D *p_node) {
    SplineNode4D *prev = _get_bound_gizmo_node();
    if (prev == p_node) return;

    const godot::Callable cb_pt(this, "_on_gizmo_node_point_selected");
    const godot::Callable cb_sec(this, "_on_gizmo_node_section_selected");
    if (prev) {
        if (prev->is_connected("gizmo_point_selected", cb_pt)) {
            prev->disconnect("gizmo_point_selected", cb_pt);
        }
        if (prev->is_connected("gizmo_section_selected", cb_sec)) {
            prev->disconnect("gizmo_section_selected", cb_sec);
        }
    }

    bound_gizmo_node_id = p_node ? godot::ObjectID(p_node->get_instance_id()) : godot::ObjectID();

    if (p_node) {
        if (!p_node->is_connected("gizmo_point_selected", cb_pt)) {
            p_node->connect("gizmo_point_selected", cb_pt);
        }
        if (!p_node->is_connected("gizmo_section_selected", cb_sec)) {
            p_node->connect("gizmo_section_selected", cb_sec);
        }
    }
}

void Spline4DControl::_on_resource_rebound() {
    if (w_canvas) {
        w_canvas->set_spline(get_spline_4d());
        if (get_spline_4d().is_valid() && get_spline_4d()->get_point_count() > 0) {
            w_canvas->fit_view_to_data(false, true, static_cast<real_t>(0.15));
        }
    }
    _on_ui_refreshed();
}

void Spline4DControl::_on_ui_refreshed() {
    using namespace godot;
    if (w_canvas) {
        w_canvas->queue_redraw();
    }
    if (!bounds_telemetry_label) return;

    Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null() || s4d->get_point_count() == 0) {
        bounds_telemetry_label->set_text("4D Bounds: Empty");
        return;
    }

    const AABB box        = s4d->get_aabb_3d(true);
    const Vector3 sz      = box.size;
    const Vector2 w_range = s4d->get_w_range(true);
    bounds_telemetry_label->set_text(
        String("3D AABB Size: (") +
        String::num(sz.x, 2) + String(", ") + String::num(sz.y, 2) + String(", ") + String::num(sz.z, 2) +
        String(")   |   W Channel Range: [") +
        String::num(w_range.x, 2) + String(", ") + String::num(w_range.y, 2) + String("]")
    );
}

void Spline4DControl::_on_selection_updated() {
    if (w_canvas) {
        if (selected_point_index >= 0) {
            w_canvas->set_selected_point_index(selected_point_index);
        } else if (selected_section_index >= 0) {
            w_canvas->set_selected_section_index(selected_section_index);
        } else {
            w_canvas->set_selected_point_index(-1);
        }
    }
    if (SplineNode4D *gizmo_node = _get_bound_gizmo_node()) {
        if (selected_point_index >= 0) {
            gizmo_node->set_selected_gizmo_point_index(selected_point_index);
        } else if (selected_section_index >= 0) {
            gizmo_node->set_selected_gizmo_section_index(selected_section_index);
        } else {
            gizmo_node->set_selected_gizmo_point_index(-1);
        }
    }
}

void Spline4DControl::_populate_point_field(int p_index, SplinePointField *p_field) {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (!p_field || s4d.is_null()) return;

    p_field->set_dimension(4);
    p_field->configure_point_variant(
        p_index,
        s4d->get_point_position(p_index),
        s4d->get_point_back_handle(p_index),
        s4d->get_point_use_back_handle(p_index),
        s4d->get_point_fore_handle(p_index),
        s4d->get_point_use_fore_handle(p_index),
        s4d->get_point_count() > 0
    );
}

void Spline4DControl::_commit_point_position(int p_index, const godot::Variant &p_position) {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    s4d->commit_set_point_position(p_index, static_cast<godot::Vector4>(p_position));
}

void Spline4DControl::_commit_point_back_handle(
    int p_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    s4d->commit_set_point_back_handle(p_index, static_cast<godot::Vector4>(p_handle), p_enabled);
}

void Spline4DControl::_commit_point_fore_handle(
    int p_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    s4d->commit_set_point_fore_handle(p_index, static_cast<godot::Vector4>(p_handle), p_enabled);
}

void Spline4DControl::_commit_add_default_point() {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    const int n = s4d->get_point_count();
    godot::Vector4 next_pos(0.0f, 0.0f, 0.0f, 1.0f);
    if (n >= 2) {
        const godot::Vector4 last = s4d->get_point_position(n - 1);
        const godot::Vector4 prev = s4d->get_point_position(n - 2);
        const godot::Vector3 d3(last.x - prev.x, last.y - prev.y, last.z - prev.z);
        const godot::Vector3 step3 = d3.length_squared() > 1e-4f
            ? d3.normalized() * 2.5f
            : godot::Vector3(0.0f, 0.0f, -2.5f);
        next_pos = godot::Vector4(last.x + step3.x, last.y + step3.y, last.z + step3.z, last.w);
    } else if (n == 1) {
        const godot::Vector4 p0 = s4d->get_point_position(0);
        next_pos = godot::Vector4(p0.x, p0.y, p0.z - 2.5f, p0.w);
    }
    s4d->commit_add_point(next_pos);
}

void Spline4DControl::_commit_split_section(int p_section_index) {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;

    const int sec_count = s4d->get_section_count();
    if (p_section_index < 0 || p_section_index >= sec_count) return;

    const godot::Vector4 mid_pos = s4d->sample_section(p_section_index, static_cast<real_t>(0.5));
    const SplineCurveType ctype  = s4d->get_section_curve_type(p_section_index);
    const real_t half_w          = std::max(
        static_cast<real_t>(0.01),
        s4d->get_section_weight(p_section_index) * static_cast<real_t>(0.5)
    );

    const godot::Dictionary before = s4d->get_snapshot_state();
    s4d->set_section_weight(p_section_index, half_w);
    if (p_section_index + 1 < s4d->get_point_count()) {
        s4d->insert_point(p_section_index + 1, mid_pos, ctype, half_w);
    } else {
        s4d->add_point(mid_pos, ctype, half_w);
    }
    s4d->commit_snapshot_action("Split 4D Spline Section", before, s4d->get_snapshot_state());
}

// --- 4D Preset & Toolbar Callbacks ---

void Spline4DControl::_on_preset_tapered_line() {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    const int pts = preset_points_spin ? std::max(2, static_cast<int>(preset_points_spin->get_value())) : 4;
    const godot::Dictionary before = s4d->get_snapshot_state();
    s4d->setup_tapered_line(
        godot::Vector3(0.0f, 0.0f, 0.0f),
        static_cast<real_t>(1.0),
        godot::Vector3(0.0f, 0.0f, -10.0f),
        static_cast<real_t>(0.25),
        pts,
        s4d->get_default_curve_type()
    );
    s4d->commit_snapshot_action("Preset 4D Tapered Line", before, s4d->get_snapshot_state());
    if (w_canvas) w_canvas->fit_view_to_data(false, true, static_cast<real_t>(0.15));
}

void Spline4DControl::_on_preset_banked_circle() {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    const int pts = preset_points_spin ? std::max(4, static_cast<int>(preset_points_spin->get_value())) : 8;
    const godot::Dictionary before = s4d->get_snapshot_state();
    s4d->setup_banked_circle_xz(static_cast<real_t>(5.0), static_cast<real_t>(0.45), pts);
    s4d->commit_snapshot_action("Preset 4D Banked Circle", before, s4d->get_snapshot_state());
    if (w_canvas) w_canvas->fit_view_to_data(false, true, static_cast<real_t>(0.15));
}

void Spline4DControl::_on_preset_color_gradient() {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    const bool use_hsva = hsva_preview_check && hsva_preview_check->is_pressed();
    const godot::Dictionary before = s4d->get_snapshot_state();
    s4d->setup_color_gradient(godot::PackedColorArray(), use_hsva);
    s4d->commit_snapshot_action("Preset 4D Color Gradient", before, s4d->get_snapshot_state());
    if (w_canvas) w_canvas->fit_view_to_data(false, true, static_cast<real_t>(0.15));
}

void Spline4DControl::_on_rotate_y_45_pressed() {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    constexpr float DEG_45 = static_cast<float>(std::numbers::pi_v<double> * 0.25);
    s4d->commit_rotate_spline(godot::Vector3(0.0f, DEG_45, 0.0f));
}

void Spline4DControl::_on_rotate_x_45_pressed() {
    godot::Ref<Spline4DResource> s4d = get_spline_4d();
    if (s4d.is_null()) return;
    constexpr float DEG_45 = static_cast<float>(std::numbers::pi_v<double> * 0.25);
    s4d->commit_rotate_spline(godot::Vector3(DEG_45, 0.0f, 0.0f));
}

void Spline4DControl::_on_channel_selected(int p_item_idx) {
    if (!channel_select || !w_canvas) return;
    const int ch = channel_select->get_item_id(p_item_idx);
    w_canvas->set_active_channel(ch);
}

void Spline4DControl::_on_fit_w_pressed() {
    if (w_canvas) {
        w_canvas->fit_view_to_data(false, true, static_cast<real_t>(0.15));
    }
}

void Spline4DControl::_on_hsva_preview_toggled(bool p_hsva) {
    if (w_canvas) {
        w_canvas->set_hsva_gradient_preview(p_hsva);
    }
}

void Spline4DControl::_on_show_w_canvas_toggled(bool p_show) {
    if (visual_editor_slot) {
        visual_editor_slot->set_visible(p_show);
    }
}

void Spline4DControl::_on_canvas_point_selected(int p_point_index) {
    select_point(p_point_index);
}

void Spline4DControl::_on_canvas_section_selected(int p_section_index) {
    select_section(p_section_index);
}

void Spline4DControl::_on_gizmo_node_point_selected(int p_point_index) {
    select_point(p_point_index);
}

void Spline4DControl::_on_gizmo_node_section_selected(int p_section_index) {
    select_section(p_section_index);
}

} // namespace ideam::godot_ext