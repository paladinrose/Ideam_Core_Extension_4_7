#include "spline_envelope_slider.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void SplineEnvelopeSlider::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Spline Envelope Slider", "");

    ClassDB::bind_method(D_METHOD("set_track_spline_2d", "spline"), &SplineEnvelopeSlider::set_track_spline_2d);
    ClassDB::bind_method(D_METHOD("get_track_spline_2d"), &SplineEnvelopeSlider::get_track_spline_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "track_spline_2d", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_track_spline_2d",
        "get_track_spline_2d"
    );

    ClassDB::bind_method(D_METHOD("set_response_envelope_1d", "envelope"), &SplineEnvelopeSlider::set_response_envelope_1d);
    ClassDB::bind_method(D_METHOD("get_response_envelope_1d"), &SplineEnvelopeSlider::get_response_envelope_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "response_envelope_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_response_envelope_1d",
        "get_response_envelope_1d"
    );

    ClassDB::bind_method(D_METHOD("set_track_gradient_4d", "gradient"), &SplineEnvelopeSlider::set_track_gradient_4d);
    ClassDB::bind_method(D_METHOD("get_track_gradient_4d"), &SplineEnvelopeSlider::get_track_gradient_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "track_gradient_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_track_gradient_4d",
        "get_track_gradient_4d"
    );

    ClassDB::bind_method(D_METHOD("set_track_percent", "percent"), &SplineEnvelopeSlider::set_track_percent);
    ClassDB::bind_method(D_METHOD("get_track_percent"), &SplineEnvelopeSlider::get_track_percent);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "track_percent", PROPERTY_HINT_RANGE, "0.0,1.0,0.001"),
        "set_track_percent",
        "get_track_percent"
    );

    ClassDB::bind_method(D_METHOD("set_track_thickness", "thickness"), &SplineEnvelopeSlider::set_track_thickness);
    ClassDB::bind_method(D_METHOD("get_track_thickness"), &SplineEnvelopeSlider::get_track_thickness);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "track_thickness"), "set_track_thickness", "get_track_thickness");

    ClassDB::bind_method(D_METHOD("set_thumb_radius", "radius"), &SplineEnvelopeSlider::set_thumb_radius);
    ClassDB::bind_method(D_METHOD("get_thumb_radius"), &SplineEnvelopeSlider::get_thumb_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "thumb_radius"), "set_thumb_radius", "get_thumb_radius");

    ClassDB::bind_method(D_METHOD("set_show_value_label", "show"), &SplineEnvelopeSlider::set_show_value_label);
    ClassDB::bind_method(D_METHOD("get_show_value_label"), &SplineEnvelopeSlider::get_show_value_label);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_value_label"), "set_show_value_label", "get_show_value_label");

    ClassDB::bind_method(D_METHOD("_on_spline_changed"), &SplineEnvelopeSlider::_on_spline_changed);
}

SplineEnvelopeSlider::SplineEnvelopeSlider() {
    set_custom_minimum_size(godot::Vector2(140.0f, 52.0f));
    set_mouse_filter(Control::MOUSE_FILTER_STOP);
    set_focus_mode(Control::FOCUS_CLICK);
    _ensure_default_splines();
}

SplineEnvelopeSlider::~SplineEnvelopeSlider() {
    const godot::Callable cb(this, "_on_spline_changed");
    if (track_spline_2d_.is_valid() && track_spline_2d_->is_connected("changed", cb)) {
        track_spline_2d_->disconnect("changed", cb);
    }
    if (response_envelope_1d_.is_valid() && response_envelope_1d_->is_connected("changed", cb)) {
        response_envelope_1d_->disconnect("changed", cb);
    }
    if (track_gradient_4d_.is_valid() && track_gradient_4d_->is_connected("changed", cb)) {
        track_gradient_4d_->disconnect("changed", cb);
    }
}

void SplineEnvelopeSlider::_ensure_default_splines() {
    if (track_spline_2d_.is_null()) {
        godot::Ref<Spline2DResource> s2;
        s2.instantiate();
        s2->set_wrap(false);
        s2->set_default_curve_type(CURVE_CATMULL_ROM);
        s2->add_point(godot::Vector2(0.0f, 0.75f), CURVE_CATMULL_ROM);
        s2->add_point(godot::Vector2(0.5f, 0.25f), CURVE_CATMULL_ROM);
        s2->add_point(godot::Vector2(1.0f, 0.75f), CURVE_CATMULL_ROM);
        set_track_spline_2d(s2);
    }
    if (response_envelope_1d_.is_null()) {
        set_response_envelope_1d(Spline1DResource::create_zero_to_one());
    }
}

void SplineEnvelopeSlider::_on_spline_changed() {
    queue_redraw();
}

void SplineEnvelopeSlider::set_track_spline_2d(const godot::Ref<Spline2DResource> &p_spline) {
    if (track_spline_2d_ == p_spline) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (track_spline_2d_.is_valid() && track_spline_2d_->is_connected("changed", cb)) {
        track_spline_2d_->disconnect("changed", cb);
    }
    track_spline_2d_ = p_spline;
    if (track_spline_2d_.is_valid() && !track_spline_2d_->is_connected("changed", cb)) {
        track_spline_2d_->connect("changed", cb);
    }
    queue_redraw();
}

void SplineEnvelopeSlider::set_response_envelope_1d(const godot::Ref<Spline1DResource> &p_envelope) {
    if (response_envelope_1d_ == p_envelope) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (response_envelope_1d_.is_valid() && response_envelope_1d_->is_connected("changed", cb)) {
        response_envelope_1d_->disconnect("changed", cb);
    }
    response_envelope_1d_ = p_envelope;
    if (response_envelope_1d_.is_valid() && !response_envelope_1d_->is_connected("changed", cb)) {
        response_envelope_1d_->connect("changed", cb);
    }
    queue_redraw();
}

void SplineEnvelopeSlider::set_track_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient) {
    if (track_gradient_4d_ == p_gradient) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (track_gradient_4d_.is_valid() && track_gradient_4d_->is_connected("changed", cb)) {
        track_gradient_4d_->disconnect("changed", cb);
    }
    track_gradient_4d_ = p_gradient;
    if (track_gradient_4d_.is_valid() && !track_gradient_4d_->is_connected("changed", cb)) {
        track_gradient_4d_->connect("changed", cb);
    }
    queue_redraw();
}

void SplineEnvelopeSlider::set_track_percent(real_t p_percent) {
    track_percent_ = std::clamp(p_percent, static_cast<real_t>(0.0), static_cast<real_t>(1.0));

    real_t env_val = track_percent_;
    if (response_envelope_1d_.is_valid() && response_envelope_1d_->get_point_count() > 0) {
        env_val = std::clamp(
            response_envelope_1d_->sample(track_percent_, true),
            static_cast<real_t>(0.0),
            static_cast<real_t>(1.0)
        );
    }

    is_syncing_value_ = true;
    const double min_v = get_min();
    const double max_v = get_max();
    set_value(min_v + static_cast<double>(env_val) * (max_v - min_v));
    is_syncing_value_ = false;
    queue_redraw();
}

void SplineEnvelopeSlider::_value_changed(double p_new_value) {
    if (is_syncing_value_) return;

    const double span = std::max(1e-6, get_max() - get_min());
    const real_t norm = static_cast<real_t>(std::clamp((p_new_value - get_min()) / span, 0.0, 1.0));

    // Inverse lookup via Spline1DResource::get_percents_at_value (`SPL9` / Section 3.3)
    if (response_envelope_1d_.is_valid() && response_envelope_1d_->get_point_count() >= 2) {
        const godot::PackedFloat32Array roots = response_envelope_1d_->get_percents_at_value(norm, 12, true);
        if (!roots.is_empty()) {
            // Pick the root closest to current track_percent_ for smooth multi-modal continuity
            real_t best_t    = roots[0];
            real_t best_dist = std::abs(best_t - track_percent_);
            for (int64_t i = 1; i < roots.size(); ++i) {
                const real_t d = std::abs(static_cast<real_t>(roots[i]) - track_percent_);
                if (d < best_dist) {
                    best_dist = d;
                    best_t    = roots[i];
                }
            }
            track_percent_ = std::clamp(best_t, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
        } else {
            track_percent_ = norm;
        }
    } else {
        track_percent_ = norm;
    }
    queue_redraw();
}

void SplineEnvelopeSlider::set_track_thickness(real_t p_thickness) {
    track_thickness_ = std::max(static_cast<real_t>(1.0), p_thickness);
    queue_redraw();
}

void SplineEnvelopeSlider::set_thumb_radius(real_t p_radius) {
    thumb_radius_ = std::max(static_cast<real_t>(3.0), p_radius);
    queue_redraw();
}

void SplineEnvelopeSlider::set_show_value_label(bool p_show) {
    show_value_label_ = p_show;
    queue_redraw();
}

godot::Vector2 SplineEnvelopeSlider::_spline_to_local(const godot::Vector2 &p_spline_pt) const {
    if (track_spline_2d_.is_null()) return p_spline_pt;
    const godot::Rect2 sb   = track_spline_2d_->get_bounding_rect(true);
    const godot::Vector2 sz = get_size();
    const float pad         = static_cast<float>(thumb_radius_) + 6.0f;
    const float w           = std::max(4.0f, sz.x - pad * 2.0f);
    const float h           = std::max(4.0f, sz.y - pad * 2.0f);
    const float bw          = std::max(1e-4f, sb.size.x);
    const float bh          = std::max(1e-4f, sb.size.y);
    const float nx          = (p_spline_pt.x - sb.position.x) / bw;
    const float ny          = (p_spline_pt.y - sb.position.y) / bh;
    return godot::Vector2(pad + nx * w, pad + ny * h);
}

godot::Vector2 SplineEnvelopeSlider::_local_to_spline(const godot::Vector2 &p_local_pt) const {
    if (track_spline_2d_.is_null()) return p_local_pt;
    const godot::Rect2 sb   = track_spline_2d_->get_bounding_rect(true);
    const godot::Vector2 sz = get_size();
    const float pad         = static_cast<float>(thumb_radius_) + 6.0f;
    const float w           = std::max(4.0f, sz.x - pad * 2.0f);
    const float h           = std::max(4.0f, sz.y - pad * 2.0f);
    const float nx          = (p_local_pt.x - pad) / w;
    const float ny          = (p_local_pt.y - pad) / h;
    return godot::Vector2(sb.position.x + nx * sb.size.x, sb.position.y + ny * sb.size.y);
}

void SplineEnvelopeSlider::_update_percent_from_mouse(const godot::Vector2 &p_local_mouse) {
    if (track_spline_2d_.is_null() || track_spline_2d_->get_point_count() < 2) {
        const float w = std::max(1.0f, get_size().x);
        set_track_percent(static_cast<real_t>(p_local_mouse.x / w));
        return;
    }
    const godot::Vector2 s_mouse = _local_to_spline(p_local_mouse);
    const real_t closest_t       = track_spline_2d_->get_closest_percent_on_spline(s_mouse, 16, true);
    set_track_percent(closest_t);
}

void SplineEnvelopeSlider::_gui_input(const godot::Ref<godot::InputEvent> &p_event) {
    using namespace godot;
    if (!is_editable() || p_event.is_null()) return;

    if (Ref<InputEventMouseButton> mb = p_event; mb.is_valid()) {
        if (mb->get_button_index() == MOUSE_BUTTON_LEFT) {
            if (mb->is_pressed()) {
                is_dragging_thumb_ = true;
                _update_percent_from_mouse(mb->get_position());
                accept_event();
            } else if (is_dragging_thumb_) {
                is_dragging_thumb_ = false;
                accept_event();
            }
        }
    } else if (Ref<InputEventMouseMotion> mm = p_event; mm.is_valid() && is_dragging_thumb_) {
        _update_percent_from_mouse(mm->get_position());
        accept_event();
    }
}

void SplineEnvelopeSlider::_draw() {
    using namespace godot;
    if (track_spline_2d_.is_null() || track_spline_2d_->get_point_count() < 2) return;

    constexpr int SAMPLES = 48;
    PackedVector2Array bg_pts;
    PackedVector2Array fill_pts;
    bg_pts.resize(SAMPLES + 1);

    for (int i = 0; i <= SAMPLES; ++i) {
        const real_t t      = static_cast<real_t>(i) / static_cast<real_t>(SAMPLES);
        const Vector2 l_pt  = _spline_to_local(track_spline_2d_->sample(t, true));
        bg_pts.set(i, l_pt);
        if (t <= track_percent_ + static_cast<real_t>(1e-4)) {
            fill_pts.push_back(l_pt);
        }
    }

    const Vector2 thumb_local = _spline_to_local(track_spline_2d_->sample(track_percent_, true));
    if (fill_pts.size() >= 1 && fill_pts[fill_pts.size() - 1].distance_to(thumb_local) > 0.5f) {
        fill_pts.push_back(thumb_local);
    }

    draw_polyline(bg_pts, track_bg_color_, static_cast<float>(track_thickness_), true);

    if (fill_pts.size() >= 2) {
        const Color active_col = (track_gradient_4d_.is_valid() && track_gradient_4d_->get_point_count() > 0)
            ? track_gradient_4d_->sample_color(track_percent_, true)
            : track_fill_color_;
        draw_polyline(fill_pts, active_col, static_cast<float>(track_thickness_), true);
    }

    // Draw thumb cap
    draw_circle(thumb_local, static_cast<float>(thumb_radius_) + 2.0f, Color(0.10f, 0.12f, 0.16f, 1.0f));
    draw_circle(thumb_local, static_cast<float>(thumb_radius_), thumb_color_);

    if (show_value_label_) {
        Ref<Font> font = get_theme_default_font();
        if (font.is_valid()) {
            const String txt = String::num(get_value(), 2);
            draw_string(
                font,
                thumb_local + Vector2(-12.0f, -static_cast<float>(thumb_radius_) - 6.0f),
                txt,
                HORIZONTAL_ALIGNMENT_LEFT,
                -1.0f,
                11,
                Color(0.92f, 0.96f, 1.0f, 0.95f)
            );
        }
    }
}

} // namespace ideam::godot_ext