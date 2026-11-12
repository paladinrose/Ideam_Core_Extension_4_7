#include "spline_path_container.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ideam::godot_ext {

void SplinePathContainer::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SPLINE_PATH_DISTRIBUTE_UNIFORM);
    BIND_ENUM_CONSTANT(SPLINE_PATH_DISTRIBUTE_CENTER_CLUSTER);
    BIND_ENUM_CONSTANT(SPLINE_PATH_DISTRIBUTE_ARC_LENGTH);

    ADD_GROUP("Spline Path Layout", "");

    ClassDB::bind_method(D_METHOD("set_path_spline_2d", "spline"), &SplinePathContainer::set_path_spline_2d);
    ClassDB::bind_method(D_METHOD("get_path_spline_2d"), &SplinePathContainer::get_path_spline_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "path_spline_2d", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_path_spline_2d",
        "get_path_spline_2d"
    );

    ClassDB::bind_method(D_METHOD("set_distribution_mode", "mode"), &SplinePathContainer::set_distribution_mode);
    ClassDB::bind_method(D_METHOD("get_distribution_mode"), &SplinePathContainer::get_distribution_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "distribution_mode",
            PROPERTY_HINT_ENUM,
            "Uniform Span:0,Center Clustered (Card Hand):1,Uniform Arc-Length:2"
        ),
        "set_distribution_mode",
        "get_distribution_mode"
    );

    ClassDB::bind_method(D_METHOD("set_normalize_to_container_rect", "normalize"), &SplinePathContainer::set_normalize_to_container_rect);
    ClassDB::bind_method(D_METHOD("get_normalize_to_container_rect"), &SplinePathContainer::get_normalize_to_container_rect);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "normalize_to_container_rect"),
        "set_normalize_to_container_rect",
        "get_normalize_to_container_rect"
    );

    ClassDB::bind_method(D_METHOD("set_align_rotation_to_tangent", "align"), &SplinePathContainer::set_align_rotation_to_tangent);
    ClassDB::bind_method(D_METHOD("get_align_rotation_to_tangent"), &SplinePathContainer::get_align_rotation_to_tangent);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "align_rotation_to_tangent"),
        "set_align_rotation_to_tangent",
        "get_align_rotation_to_tangent"
    );

    ClassDB::bind_method(D_METHOD("set_rotation_offset_degrees", "degrees"), &SplinePathContainer::set_rotation_offset_degrees);
    ClassDB::bind_method(D_METHOD("get_rotation_offset_degrees"), &SplinePathContainer::get_rotation_offset_degrees);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "rotation_offset_degrees"),
        "set_rotation_offset_degrees",
        "get_rotation_offset_degrees"
    );

    ClassDB::bind_method(D_METHOD("set_margin_start_percent", "margin"), &SplinePathContainer::set_margin_start_percent);
    ClassDB::bind_method(D_METHOD("get_margin_start_percent"), &SplinePathContainer::get_margin_start_percent);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "margin_start_percent", PROPERTY_HINT_RANGE, "0.0,0.49,0.01"),
        "set_margin_start_percent",
        "get_margin_start_percent"
    );

    ClassDB::bind_method(D_METHOD("set_margin_end_percent", "margin"), &SplinePathContainer::set_margin_end_percent);
    ClassDB::bind_method(D_METHOD("get_margin_end_percent"), &SplinePathContainer::get_margin_end_percent);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "margin_end_percent", PROPERTY_HINT_RANGE, "0.0,0.49,0.01"),
        "set_margin_end_percent",
        "get_margin_end_percent"
    );

    ClassDB::bind_method(D_METHOD("set_cluster_step_percent", "step"), &SplinePathContainer::set_cluster_step_percent);
    ClassDB::bind_method(D_METHOD("get_cluster_step_percent"), &SplinePathContainer::get_cluster_step_percent);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "cluster_step_percent", PROPERTY_HINT_RANGE, "0.01,0.5,0.01"),
        "set_cluster_step_percent",
        "get_cluster_step_percent"
    );

    ClassDB::bind_method(D_METHOD("set_scroll_offset", "offset"), &SplinePathContainer::set_scroll_offset);
    ClassDB::bind_method(D_METHOD("get_scroll_offset"), &SplinePathContainer::get_scroll_offset);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "scroll_offset", PROPERTY_HINT_RANGE, "-1.0,1.0,0.005"),
        "set_scroll_offset",
        "get_scroll_offset"
    );

    ADD_GROUP("Falloff Envelopes & Debug", "");

    ClassDB::bind_method(D_METHOD("set_scale_falloff_1d", "curve"), &SplinePathContainer::set_scale_falloff_1d);
    ClassDB::bind_method(D_METHOD("get_scale_falloff_1d"), &SplinePathContainer::get_scale_falloff_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "scale_falloff_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_scale_falloff_1d",
        "get_scale_falloff_1d"
    );

    ClassDB::bind_method(D_METHOD("set_opacity_falloff_1d", "curve"), &SplinePathContainer::set_opacity_falloff_1d);
    ClassDB::bind_method(D_METHOD("get_opacity_falloff_1d"), &SplinePathContainer::get_opacity_falloff_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "opacity_falloff_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_opacity_falloff_1d",
        "get_opacity_falloff_1d"
    );

    ClassDB::bind_method(D_METHOD("set_show_debug_curve", "show"), &SplinePathContainer::set_show_debug_curve);
    ClassDB::bind_method(D_METHOD("get_show_debug_curve"), &SplinePathContainer::get_show_debug_curve);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_debug_curve"), "set_show_debug_curve", "get_show_debug_curve");

    ClassDB::bind_method(
        D_METHOD("get_child_spline_percent", "visible_child_index", "visible_count"),
        &SplinePathContainer::get_child_spline_percent
    );
    ClassDB::bind_method(D_METHOD("_on_spline_changed"), &SplinePathContainer::_on_spline_changed);
}

SplinePathContainer::SplinePathContainer() {
    set_custom_minimum_size(godot::Vector2(180.0f, 90.0f));
}

SplinePathContainer::~SplinePathContainer() {
    const godot::Callable cb(this, "_on_spline_changed");
    if (path_spline_2d_.is_valid() && path_spline_2d_->is_connected("changed", cb)) {
        path_spline_2d_->disconnect("changed", cb);
    }
    if (scale_falloff_1d_.is_valid() && scale_falloff_1d_->is_connected("changed", cb)) {
        scale_falloff_1d_->disconnect("changed", cb);
    }
    if (opacity_falloff_1d_.is_valid() && opacity_falloff_1d_->is_connected("changed", cb)) {
        opacity_falloff_1d_->disconnect("changed", cb);
    }
}

void SplinePathContainer::_notification(int p_what) {
    if (p_what == NOTIFICATION_SORT_CHILDREN || p_what == NOTIFICATION_RESIZED) {
        _reflow_children();
        if (show_debug_curve_) {
            queue_redraw();
        }
    }
}

void SplinePathContainer::_on_spline_changed() {
    queue_sort();
    queue_redraw();
}

void SplinePathContainer::set_path_spline_2d(const godot::Ref<Spline2DResource> &p_spline) {
    if (path_spline_2d_ == p_spline) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (path_spline_2d_.is_valid() && path_spline_2d_->is_connected("changed", cb)) {
        path_spline_2d_->disconnect("changed", cb);
    }
    path_spline_2d_ = p_spline;
    if (path_spline_2d_.is_valid() && !path_spline_2d_->is_connected("changed", cb)) {
        path_spline_2d_->connect("changed", cb);
    }
    queue_sort();
    queue_redraw();
}

void SplinePathContainer::set_scale_falloff_1d(const godot::Ref<Spline1DResource> &p_curve) {
    if (scale_falloff_1d_ == p_curve) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (scale_falloff_1d_.is_valid() && scale_falloff_1d_->is_connected("changed", cb)) {
        scale_falloff_1d_->disconnect("changed", cb);
    }
    scale_falloff_1d_ = p_curve;
    if (scale_falloff_1d_.is_valid() && !scale_falloff_1d_->is_connected("changed", cb)) {
        scale_falloff_1d_->connect("changed", cb);
    }
    queue_sort();
}

void SplinePathContainer::set_opacity_falloff_1d(const godot::Ref<Spline1DResource> &p_curve) {
    if (opacity_falloff_1d_ == p_curve) return;
    const godot::Callable cb(this, "_on_spline_changed");
    if (opacity_falloff_1d_.is_valid() && opacity_falloff_1d_->is_connected("changed", cb)) {
        opacity_falloff_1d_->disconnect("changed", cb);
    }
    opacity_falloff_1d_ = p_curve;
    if (opacity_falloff_1d_.is_valid() && !opacity_falloff_1d_->is_connected("changed", cb)) {
        opacity_falloff_1d_->connect("changed", cb);
    }
    queue_sort();
}

void SplinePathContainer::set_distribution_mode(SplinePathDistributionMode p_mode) {
    if (distribution_mode_ == p_mode) return;
    distribution_mode_ = p_mode;
    queue_sort();
}

void SplinePathContainer::set_normalize_to_container_rect(bool p_normalize) {
    if (normalize_to_container_rect_ == p_normalize) return;
    normalize_to_container_rect_ = p_normalize;
    queue_sort();
    queue_redraw();
}

void SplinePathContainer::set_align_rotation_to_tangent(bool p_align) {
    if (align_rotation_to_tangent_ == p_align) return;
    align_rotation_to_tangent_ = p_align;
    queue_sort();
}

void SplinePathContainer::set_rotation_offset_degrees(real_t p_deg) {
    rotation_offset_degrees_ = p_deg;
    queue_sort();
}

void SplinePathContainer::set_margin_start_percent(real_t p_margin) {
    margin_start_percent_ = std::clamp(p_margin, static_cast<real_t>(0.0), static_cast<real_t>(0.49));
    queue_sort();
}

void SplinePathContainer::set_margin_end_percent(real_t p_margin) {
    margin_end_percent_ = std::clamp(p_margin, static_cast<real_t>(0.0), static_cast<real_t>(0.49));
    queue_sort();
}

void SplinePathContainer::set_cluster_step_percent(real_t p_step) {
    cluster_step_percent_ = std::clamp(p_step, static_cast<real_t>(0.01), static_cast<real_t>(0.5));
    queue_sort();
}

void SplinePathContainer::set_scroll_offset(real_t p_offset) {
    scroll_offset_ = p_offset;
    queue_sort();
}

void SplinePathContainer::set_show_debug_curve(bool p_show) {
    show_debug_curve_ = p_show;
    queue_redraw();
}

real_t SplinePathContainer::get_child_spline_percent(int p_visible_child_index, int p_visible_count) const noexcept {
    if (p_visible_count <= 0) return static_cast<real_t>(0.5);

    real_t base_t = static_cast<real_t>(0.5);
    if (p_visible_count == 1) {
        base_t = static_cast<real_t>(0.5);
    } else if (distribution_mode_ == SPLINE_PATH_DISTRIBUTE_CENTER_CLUSTER) {
        const real_t half_span = static_cast<real_t>(p_visible_count - 1) * static_cast<real_t>(0.5) * cluster_step_percent_;
        base_t = static_cast<real_t>(0.5) - half_span + static_cast<real_t>(p_visible_child_index) * cluster_step_percent_;
    } else {
        const real_t t0   = margin_start_percent_;
        const real_t t1   = std::max(t0 + static_cast<real_t>(0.01), static_cast<real_t>(1.0) - margin_end_percent_);
        const real_t frac = static_cast<real_t>(p_visible_child_index) / static_cast<real_t>(p_visible_count - 1);
        base_t = t0 + frac * (t1 - t0);
    }

    real_t t = base_t + scroll_offset_;
    if (path_spline_2d_.is_valid() && path_spline_2d_->get_wrap()) {
        t = std::fmod(t, static_cast<real_t>(1.0));
        if (t < static_cast<real_t>(0.0)) t += static_cast<real_t>(1.0);
    } else {
        t = std::clamp(t, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    }
    return t;
}

godot::Vector2 SplinePathContainer::_map_spline_point_to_local(
    const godot::Vector2 &p_spline_pt,
    const godot::Rect2 &p_spline_bounds
) const {
    if (!normalize_to_container_rect_) {
        return p_spline_pt;
    }
    const godot::Vector2 c_sz = get_size();
    const float bw = std::max(1e-4f, p_spline_bounds.size.x);
    const float bh = std::max(1e-4f, p_spline_bounds.size.y);
    const float nx = (p_spline_pt.x - p_spline_bounds.position.x) / bw;
    const float ny = (p_spline_pt.y - p_spline_bounds.position.y) / bh;
    return godot::Vector2(nx * c_sz.x, ny * c_sz.y);
}

void SplinePathContainer::_reflow_children() {
    using namespace godot;

    std::vector<Control *> vis_children;
    const int total = get_child_count();
    vis_children.reserve(static_cast<size_t>(total));
    for (int i = 0; i < total; ++i) {
        Control *c = Object::cast_to<Control>(get_child(i));
        if (c && c->is_visible() && !c->is_set_as_top_level()) {
            vis_children.push_back(c);
        }
    }

    const int n = static_cast<int>(vis_children.size());
    if (n <= 0) return;

    const bool has_spline = path_spline_2d_.is_valid() && path_spline_2d_->get_point_count() >= 2;
    const Rect2 s_bounds  = has_spline ? path_spline_2d_->get_bounding_rect(true) : Rect2(0.0f, 0.0f, 1.0f, 1.0f);
    const bool weighted   = (distribution_mode_ != SPLINE_PATH_DISTRIBUTE_ARC_LENGTH);

    for (int i = 0; i < n; ++i) {
        Control *child = vis_children[static_cast<size_t>(i)];
        const real_t t = get_child_spline_percent(i, n);

        Vector2 anchor_pos;
        float angle_rad = 0.0f;
        if (has_spline) {
            const Vector2 s_pt  = path_spline_2d_->sample(t, weighted);
            const Vector2 s_tan = path_spline_2d_->get_tangent(t, weighted);
            anchor_pos          = _map_spline_point_to_local(s_pt, s_bounds);
            if (align_rotation_to_tangent_) {
                const Vector2 local_tan = _map_spline_point_to_local(s_pt + s_tan, s_bounds) - anchor_pos;
                angle_rad = std::atan2(local_tan.y, local_tan.x) + Math::deg_to_rad(static_cast<float>(rotation_offset_degrees_));
            }
        } else {
            // Default gentle card-hand arc if no Spline2DResource is assigned yet
            const Vector2 sz = get_size();
            const float x    = sz.x * static_cast<float>(t);
            const float dx   = static_cast<float>(t) - 0.5f;
            const float y    = sz.y * (0.45f + dx * dx * 0.65f);
            anchor_pos       = Vector2(x, y);
            if (align_rotation_to_tangent_) {
                angle_rad = dx * 0.45f + Math::deg_to_rad(static_cast<float>(rotation_offset_degrees_));
            }
        }

        const Vector2 min_sz = child->get_combined_minimum_size();
        const Vector2 c_size(std::max(24.0f, min_sz.x), std::max(24.0f, min_sz.y));
        child->set_pivot_offset(c_size * 0.5f);
        fit_child_in_rect(child, Rect2(anchor_pos - c_size * 0.5f, c_size));
        child->set_rotation(align_rotation_to_tangent_ ? angle_rad : 0.0f);

        if (scale_falloff_1d_.is_valid() && scale_falloff_1d_->get_point_count() > 0) {
            const float sc = std::max(0.05f, static_cast<float>(scale_falloff_1d_->sample(t, true)));
            child->set_scale(Vector2(sc, sc));
        } else {
            child->set_scale(Vector2(1.0f, 1.0f));
        }

        if (opacity_falloff_1d_.is_valid() && opacity_falloff_1d_->get_point_count() > 0) {
            Color mod = child->get_modulate();
            mod.a     = std::clamp(static_cast<float>(opacity_falloff_1d_->sample(t, true)), 0.0f, 1.0f);
            child->set_modulate(mod);
        }
    }
}

void SplinePathContainer::_draw() {
    using namespace godot;
    if (!show_debug_curve_ || path_spline_2d_.is_null() || path_spline_2d_->get_point_count() < 2) return;

    const Rect2 s_bounds = path_spline_2d_->get_bounding_rect(true);
    const PackedVector2Array smoothed = path_spline_2d_->get_smoothed_points(12);
    if (smoothed.size() < 2) return;

    PackedVector2Array local_pts;
    local_pts.resize(smoothed.size());
    for (int64_t i = 0; i < smoothed.size(); ++i) {
        local_pts.set(i, _map_spline_point_to_local(smoothed[i], s_bounds));
    }
    draw_polyline(local_pts, debug_color_, 2.0f, true);
}

} // namespace ideam::godot_ext