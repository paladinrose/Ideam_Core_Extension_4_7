#include "spline_hitbox_button.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void SplineHitboxButton::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Spline Hitbox & Shape", "");

    ClassDB::bind_method(D_METHOD("set_hitbox_spline_2d", "spline"), &SplineHitboxButton::set_hitbox_spline_2d);
    ClassDB::bind_method(D_METHOD("get_hitbox_spline_2d"), &SplineHitboxButton::get_hitbox_spline_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "hitbox_spline_2d", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_hitbox_spline_2d",
        "get_hitbox_spline_2d"
    );

    ClassDB::bind_method(D_METHOD("set_state_gradient_4d", "gradient"), &SplineHitboxButton::set_state_gradient_4d);
    ClassDB::bind_method(D_METHOD("get_state_gradient_4d"), &SplineHitboxButton::get_state_gradient_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "state_gradient_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_state_gradient_4d",
        "get_state_gradient_4d"
    );

    ClassDB::bind_method(D_METHOD("set_curve_subdivisions", "subdivs"), &SplineHitboxButton::set_curve_subdivisions);
    ClassDB::bind_method(D_METHOD("get_curve_subdivisions"), &SplineHitboxButton::get_curve_subdivisions);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "curve_subdivisions"), "set_curve_subdivisions", "get_curve_subdivisions");

    ClassDB::bind_method(D_METHOD("set_fit_spline_to_control", "fit"), &SplineHitboxButton::set_fit_spline_to_control);
    ClassDB::bind_method(D_METHOD("get_fit_spline_to_control"), &SplineHitboxButton::get_fit_spline_to_control);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "fit_spline_to_control"),
        "set_fit_spline_to_control",
        "get_fit_spline_to_control"
    );

    ClassDB::bind_method(D_METHOD("set_border_width", "width"), &SplineHitboxButton::set_border_width);
    ClassDB::bind_method(D_METHOD("get_border_width"), &SplineHitboxButton::get_border_width);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "border_width"), "set_border_width", "get_border_width");

    ClassDB::bind_method(D_METHOD("set_label_text", "text"), &SplineHitboxButton::set_label_text);
    ClassDB::bind_method(D_METHOD("get_label_text"), &SplineHitboxButton::get_label_text);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "label_text"), "set_label_text", "get_label_text");

    ADD_GROUP("State Colors", "");

    ClassDB::bind_method(D_METHOD("set_normal_color", "color"), &SplineHitboxButton::set_normal_color);
    ClassDB::bind_method(D_METHOD("get_normal_color"), &SplineHitboxButton::get_normal_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "normal_color"), "set_normal_color", "get_normal_color");

    ClassDB::bind_method(D_METHOD("set_hover_color", "color"), &SplineHitboxButton::set_hover_color);
    ClassDB::bind_method(D_METHOD("get_hover_color"), &SplineHitboxButton::get_hover_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "hover_color"), "set_hover_color", "get_hover_color");

    ClassDB::bind_method(D_METHOD("set_pressed_color", "color"), &SplineHitboxButton::set_pressed_color);
    ClassDB::bind_method(D_METHOD("get_pressed_color"), &SplineHitboxButton::get_pressed_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "pressed_color"), "set_pressed_color", "get_pressed_color");

    ClassDB::bind_method(D_METHOD("set_disabled_color", "color"), &SplineHitboxButton::set_disabled_color);
    ClassDB::bind_method(D_METHOD("get_disabled_color"), &SplineHitboxButton::get_disabled_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "disabled_color"), "set_disabled_color", "get_disabled_color");

    ClassDB::bind_method(D_METHOD("set_border_color", "color"), &SplineHitboxButton::set_border_color);
    ClassDB::bind_method(D_METHOD("get_border_color"), &SplineHitboxButton::get_border_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "border_color"), "set_border_color", "get_border_color");

    ClassDB::bind_method(D_METHOD("_on_spline_changed"), &SplineHitboxButton::_on_spline_changed);
}

SplineHitboxButton::SplineHitboxButton() {
    set_custom_minimum_size(godot::Vector2(96.0f, 48.0f));
    _ensure_default_spline();
}

SplineHitboxButton::~SplineHitboxButton() {
    const godot::Callable cb(this, "_on_spline_changed");
    if (hitbox_spline_2d_.is_valid() && hitbox_spline_2d_->is_connected("changed", cb)) {
        hitbox_spline_2d_->disconnect("changed", cb);
    }
    if (state_gradient_4d_.is_valid() && state_gradient_4d_->is_connected("changed", cb)) {
        state_gradient_4d_->disconnect("changed", cb);
    }
}

void SplineHitboxButton::_ensure_default_spline() {
    if (hitbox_spline_2d_.is_null()) {
        godot::Ref<Spline2DResource> s = Spline2DResource::create_regular_polygon(6, static_cast<real_t>(45.0));
        set_hitbox_spline_2d(s);
    }
}

void SplineHitboxButton::_notification(int p_what) {
    if (p_what == NOTIFICATION_MOUSE_ENTER ||
        p_what == NOTIFICATION_MOUSE_EXIT ||
        p_what == NOTIFICATION_FOCUS_ENTER ||
        p_what == NOTIFICATION_FOCUS_EXIT ||
        p_what == NOTIFICATION_RESIZED) {
        queue_redraw();
    }
}

void SplineHitboxButton::_on_spline_changed() {
    queue_redraw();
}

void SplineHitboxButton::set_hitbox_spline_2d(const godot::Ref<Spline2DResource> &p_spline) {
    if (hitbox_spline_2d_ == p_spline) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (hitbox_spline_2d_.is_valid() && hitbox_spline_2d_->is_connected("changed", cb)) {
        hitbox_spline_2d_->disconnect("changed", cb);
    }
    hitbox_spline_2d_ = p_spline;
    if (hitbox_spline_2d_.is_valid() && !hitbox_spline_2d_->is_connected("changed", cb)) {
        hitbox_spline_2d_->connect("changed", cb);
    }
    queue_redraw();
}

void SplineHitboxButton::set_state_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient) {
    if (state_gradient_4d_ == p_gradient) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (state_gradient_4d_.is_valid() && state_gradient_4d_->is_connected("changed", cb)) {
        state_gradient_4d_->disconnect("changed", cb);
    }
    state_gradient_4d_ = p_gradient;
    if (state_gradient_4d_.is_valid() && !state_gradient_4d_->is_connected("changed", cb)) {
        state_gradient_4d_->connect("changed", cb);
    }
    queue_redraw();
}

void SplineHitboxButton::set_curve_subdivisions(int p_subdivs) {
    curve_subdivisions_ = std::clamp(p_subdivs, 2, 64);
    queue_redraw();
}

void SplineHitboxButton::set_fit_spline_to_control(bool p_fit) {
    fit_spline_to_control_ = p_fit;
    queue_redraw();
}

void SplineHitboxButton::set_border_width(real_t p_width) {
    border_width_ = std::max(static_cast<real_t>(0.0), p_width);
    queue_redraw();
}

void SplineHitboxButton::set_label_text(const godot::String &p_text) {
    label_text_ = p_text;
    queue_redraw();
}

void SplineHitboxButton::set_normal_color(const godot::Color &p_color) {
    normal_color_ = p_color;
    queue_redraw();
}

void SplineHitboxButton::set_hover_color(const godot::Color &p_color) {
    hover_color_ = p_color;
    queue_redraw();
}

void SplineHitboxButton::set_pressed_color(const godot::Color &p_color) {
    pressed_color_ = p_color;
    queue_redraw();
}

void SplineHitboxButton::set_disabled_color(const godot::Color &p_color) {
    disabled_color_ = p_color;
    queue_redraw();
}

void SplineHitboxButton::set_border_color(const godot::Color &p_color) {
    border_color_ = p_color;
    queue_redraw();
}

godot::Vector2 SplineHitboxButton::_local_to_spline_space(const godot::Vector2 &p_local) const {
    if (!fit_spline_to_control_ || hitbox_spline_2d_.is_null()) return p_local;
    const godot::Rect2 sb = hitbox_spline_2d_->get_bounding_rect(true);
    const godot::Vector2 sz = get_size();
    const float pad = 4.0f;
    const float w   = std::max(1.0f, sz.x - pad * 2.0f);
    const float h   = std::max(1.0f, sz.y - pad * 2.0f);
    const float nx  = (p_local.x - pad) / w;
    const float ny  = (p_local.y - pad) / h;
    return godot::Vector2(
        sb.position.x + nx * sb.size.x,
        sb.position.y + ny * sb.size.y
    );
}

godot::Vector2 SplineHitboxButton::_spline_to_local_space(const godot::Vector2 &p_spline_pt) const {
    if (!fit_spline_to_control_ || hitbox_spline_2d_.is_null()) return p_spline_pt;
    const godot::Rect2 sb = hitbox_spline_2d_->get_bounding_rect(true);
    const godot::Vector2 sz = get_size();
    const float pad = 4.0f;
    const float w   = std::max(1.0f, sz.x - pad * 2.0f);
    const float h   = std::max(1.0f, sz.y - pad * 2.0f);
    const float bw  = std::max(1e-4f, sb.size.x);
    const float bh  = std::max(1e-4f, sb.size.y);
    const float nx  = (p_spline_pt.x - sb.position.x) / bw;
    const float ny  = (p_spline_pt.y - sb.position.y) / bh;
    return godot::Vector2(pad + nx * w, pad + ny * h);
}

bool SplineHitboxButton::_has_point(const godot::Vector2 &p_point) const {
    if (hitbox_spline_2d_.is_null() || hitbox_spline_2d_->get_point_count() < 3) {
        return godot::Rect2(godot::Vector2(), get_size()).has_point(p_point);
    }
    const godot::Vector2 s_pt = _local_to_spline_space(p_point);
    return hitbox_spline_2d_->contains_point(s_pt, curve_subdivisions_);
}

godot::Color SplineHitboxButton::_resolve_current_fill_color() const {
    const DrawMode mode = get_draw_mode();
    if (state_gradient_4d_.is_valid() && state_gradient_4d_->get_point_count() > 0) {
        real_t t = static_cast<real_t>(0.25);
        if (mode == DRAW_DISABLED) t = static_cast<real_t>(0.0);
        else if (mode == DRAW_HOVER) t = static_cast<real_t>(0.65);
        else if (mode == DRAW_PRESSED || mode == DRAW_HOVER_PRESSED) t = static_cast<real_t>(1.0);
        return state_gradient_4d_->sample_color(t, true);
    }

    switch (mode) {
        case DRAW_DISABLED:
            return disabled_color_;
        case DRAW_PRESSED:
        case DRAW_HOVER_PRESSED:
            return pressed_color_;
        case DRAW_HOVER:
            return hover_color_;
        case DRAW_NORMAL:
        default:
            return normal_color_;
    }
}

void SplineHitboxButton::_draw() {
    using namespace godot;
    if (hitbox_spline_2d_.is_null() || hitbox_spline_2d_->get_point_count() < 3) return;

    const PackedVector2Array s_pts = hitbox_spline_2d_->get_smoothed_points(curve_subdivisions_);
    const PackedInt32Array tris    = hitbox_spline_2d_->triangulate(curve_subdivisions_);
    if (s_pts.size() < 3) return;

    PackedVector2Array local_pts;
    local_pts.resize(s_pts.size());
    for (int64_t i = 0; i < s_pts.size(); ++i) {
        local_pts.set(i, _spline_to_local_space(s_pts[i]));
    }

    const Color fill_col = _resolve_current_fill_color();
    for (int64_t t = 0; t + 2 < tris.size(); t += 3) {
        const int32_t i0 = tris[t];
        const int32_t i1 = tris[t + 1];
        const int32_t i2 = tris[t + 2];
        if (i0 < 0 || i1 < 0 || i2 < 0 ||
            i0 >= local_pts.size() || i1 >= local_pts.size() || i2 >= local_pts.size()) {
            continue;
        }
        PackedVector2Array tri_pts;
        tri_pts.push_back(local_pts[i0]);
        tri_pts.push_back(local_pts[i1]);
        tri_pts.push_back(local_pts[i2]);
        PackedColorArray cols;
        cols.push_back(fill_col);
        cols.push_back(fill_col);
        cols.push_back(fill_col);
        draw_polygon(tri_pts, cols);
    }

    if (border_width_ > static_cast<real_t>(0.0)) {
        PackedVector2Array border_pts = local_pts;
        border_pts.push_back(local_pts[0]);
        draw_polyline(border_pts, border_color_, static_cast<float>(border_width_), true);
    }

    if (!label_text_.is_empty()) {
        Ref<Font> font = get_theme_default_font();
        if (font.is_valid()) {
            const Vector2 c_local = _spline_to_local_space(hitbox_spline_2d_->get_centroid(curve_subdivisions_));
            const Vector2 text_sz = font->get_string_size(label_text_, HORIZONTAL_ALIGNMENT_CENTER, -1.0f, font_size_);
            draw_string(
                font,
                Vector2(c_local.x - text_sz.x * 0.5f, c_local.y + text_sz.y * 0.32f),
                label_text_,
                HORIZONTAL_ALIGNMENT_LEFT,
                -1.0f,
                font_size_,
                font_color_
            );
        }
    }
}

// ============================================================================
// SplineRegionControl Implementation
// ============================================================================

void SplineRegionControl::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_region_id", "id"), &SplineRegionControl::set_region_id);
    ClassDB::bind_method(D_METHOD("get_region_id"), &SplineRegionControl::get_region_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "region_id"), "set_region_id", "get_region_id");
}

void SplineRegionControl::set_region_id(int p_id) {
    region_id_ = p_id;
}

} // namespace ideam::godot_ext