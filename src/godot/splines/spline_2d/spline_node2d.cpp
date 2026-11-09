#include "spline_node2d.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void SplineNode2D::_bind_methods() {
    using namespace godot;

    // --- Signals ---
    ADD_SIGNAL(MethodInfo(
        "position_evaluated",
        PropertyInfo(Variant::FLOAT, "percent"),
        PropertyInfo(Variant::VECTOR2, "position"),
        PropertyInfo(Variant::VECTOR2, "derivative"),
        PropertyInfo(Variant::VECTOR2, "tangent")
    ));
    ADD_SIGNAL(MethodInfo(
        "section_crossed",
        PropertyInfo(Variant::INT, "previous_section"),
        PropertyInfo(Variant::INT, "current_section")
    ));
    ADD_SIGNAL(MethodInfo("playback_finished"));
    ADD_SIGNAL(MethodInfo("playback_looped"));

    // --- Resource Binding ---
    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &SplineNode2D::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &SplineNode2D::get_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_spline",
        "get_spline"
    );

    // --- Playback & Sampling Group ---
    ADD_GROUP("Playback & Sampling", "");

    ClassDB::bind_method(D_METHOD("set_process_mode_type", "mode"), &SplineNode2D::set_process_mode_type);
    ClassDB::bind_method(D_METHOD("get_process_mode_type"), &SplineNode2D::get_process_mode_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "process_mode_type", PROPERTY_HINT_ENUM, "Idle:0,Physics:1,Manual:2"),
        "set_process_mode_type",
        "get_process_mode_type"
    );

    ClassDB::bind_method(D_METHOD("set_loop_mode", "mode"), &SplineNode2D::set_loop_mode);
    ClassDB::bind_method(D_METHOD("get_loop_mode"), &SplineNode2D::get_loop_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "loop_mode", PROPERTY_HINT_ENUM, "Once:0,Wrap:1,Ping-Pong:2"),
        "set_loop_mode",
        "get_loop_mode"
    );

    ClassDB::bind_method(D_METHOD("set_playing", "playing"), &SplineNode2D::set_playing);
    ClassDB::bind_method(D_METHOD("is_playing"), &SplineNode2D::is_playing);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing"), "set_playing", "is_playing");

    ClassDB::bind_method(D_METHOD("set_autoplay", "autoplay"), &SplineNode2D::set_autoplay);
    ClassDB::bind_method(D_METHOD("get_autoplay"), &SplineNode2D::get_autoplay);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

    ClassDB::bind_method(D_METHOD("set_use_weighted_sampling", "weighted"), &SplineNode2D::set_use_weighted_sampling);
    ClassDB::bind_method(D_METHOD("get_use_weighted_sampling"), &SplineNode2D::get_use_weighted_sampling);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "use_weighted_sampling"),
        "set_use_weighted_sampling",
        "get_use_weighted_sampling"
    );

    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &SplineNode2D::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &SplineNode2D::get_duration);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0.001,600.0,0.01,or_greater,suffix:s"),
        "set_duration",
        "get_duration"
    );

    ClassDB::bind_method(D_METHOD("set_speed_scale", "speed_scale"), &SplineNode2D::set_speed_scale);
    ClassDB::bind_method(D_METHOD("get_speed_scale"), &SplineNode2D::get_speed_scale);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "-10.0,10.0,0.01,or_less,or_greater"),
        "set_speed_scale",
        "get_speed_scale"
    );

    ClassDB::bind_method(D_METHOD("set_current_percent", "percent"), &SplineNode2D::set_current_percent);
    ClassDB::bind_method(D_METHOD("get_current_percent"), &SplineNode2D::get_current_percent);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "current_percent", PROPERTY_HINT_RANGE, "0.0,1.0,0.001"),
        "set_current_percent",
        "get_current_percent"
    );

    // --- 2D Output Mapping & Orientation Group ---
    ADD_GROUP("Output Mapping & Orientation", "");

    ClassDB::bind_method(D_METHOD("set_output_scale", "scale"), &SplineNode2D::set_output_scale);
    ClassDB::bind_method(D_METHOD("get_output_scale"), &SplineNode2D::get_output_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "output_scale"), "set_output_scale", "get_output_scale");

    ClassDB::bind_method(D_METHOD("set_output_offset", "offset"), &SplineNode2D::set_output_offset);
    ClassDB::bind_method(D_METHOD("get_output_offset"), &SplineNode2D::get_output_offset);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "output_offset"), "set_output_offset", "get_output_offset");

    ClassDB::bind_method(D_METHOD("set_rotate_target_to_tangent", "rotate"), &SplineNode2D::set_rotate_target_to_tangent);
    ClassDB::bind_method(D_METHOD("get_rotate_target_to_tangent"), &SplineNode2D::get_rotate_target_to_tangent);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "rotate_target_to_tangent"),
        "set_rotate_target_to_tangent",
        "get_rotate_target_to_tangent"
    );

    ClassDB::bind_method(D_METHOD("set_rotation_offset_radians", "offset_rad"), &SplineNode2D::set_rotation_offset_radians);
    ClassDB::bind_method(D_METHOD("get_rotation_offset_radians"), &SplineNode2D::get_rotation_offset_radians);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "rotation_offset_radians", PROPERTY_HINT_RANGE, "-6.28319,6.28319,0.01,radians_as_degrees"),
        "set_rotation_offset_radians",
        "get_rotation_offset_radians"
    );

    ClassDB::bind_method(D_METHOD("get_current_position"), &SplineNode2D::get_current_position);
    ClassDB::bind_method(D_METHOD("get_current_derivative"), &SplineNode2D::get_current_derivative);
    ClassDB::bind_method(D_METHOD("get_current_tangent"), &SplineNode2D::get_current_tangent);
    ClassDB::bind_method(D_METHOD("get_current_rotation"), &SplineNode2D::get_current_rotation);
    ClassDB::bind_method(D_METHOD("get_current_section_index"), &SplineNode2D::get_current_section_index);

    // --- 2D Viewport Drawing Group ---
    ADD_GROUP("Viewport Rendering", "");

    ClassDB::bind_method(D_METHOD("set_draw_in_viewport", "draw"), &SplineNode2D::set_draw_in_viewport);
    ClassDB::bind_method(D_METHOD("get_draw_in_viewport"), &SplineNode2D::get_draw_in_viewport);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_in_viewport"), "set_draw_in_viewport", "get_draw_in_viewport");

    ClassDB::bind_method(D_METHOD("set_draw_closed_fill", "fill"), &SplineNode2D::set_draw_closed_fill);
    ClassDB::bind_method(D_METHOD("get_draw_closed_fill"), &SplineNode2D::get_draw_closed_fill);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "draw_closed_fill"), "set_draw_closed_fill", "get_draw_closed_fill");

    ClassDB::bind_method(D_METHOD("set_draw_subdivisions", "subdivisions"), &SplineNode2D::set_draw_subdivisions);
    ClassDB::bind_method(D_METHOD("get_draw_subdivisions"), &SplineNode2D::get_draw_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "draw_subdivisions", PROPERTY_HINT_RANGE, "1,64,1"),
        "set_draw_subdivisions",
        "get_draw_subdivisions"
    );

    ClassDB::bind_method(D_METHOD("set_spline_line_width", "width"), &SplineNode2D::set_spline_line_width);
    ClassDB::bind_method(D_METHOD("get_spline_line_width"), &SplineNode2D::get_spline_line_width);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "spline_line_width", PROPERTY_HINT_RANGE, "0.5,32.0,0.5"),
        "set_spline_line_width",
        "get_spline_line_width"
    );

    ClassDB::bind_method(D_METHOD("set_spline_draw_color", "color"), &SplineNode2D::set_spline_draw_color);
    ClassDB::bind_method(D_METHOD("get_spline_draw_color"), &SplineNode2D::get_spline_draw_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "spline_draw_color"), "set_spline_draw_color", "get_spline_draw_color");

    ClassDB::bind_method(D_METHOD("set_fill_draw_color", "color"), &SplineNode2D::set_fill_draw_color);
    ClassDB::bind_method(D_METHOD("get_fill_draw_color"), &SplineNode2D::get_fill_draw_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "fill_draw_color"), "set_fill_draw_color", "get_fill_draw_color");

    // --- Target & Utilities Group ---
    ADD_GROUP("Target & Utilities", "");

    ClassDB::bind_method(D_METHOD("set_target_node_path", "path"), &SplineNode2D::set_target_node_path);
    ClassDB::bind_method(D_METHOD("get_target_node_path"), &SplineNode2D::get_target_node_path);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_node_path"), "set_target_node_path", "get_target_node_path");

    ClassDB::bind_method(D_METHOD("set_target_property", "property"), &SplineNode2D::set_target_property);
    ClassDB::bind_method(D_METHOD("get_target_property"), &SplineNode2D::get_target_property);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_property"), "set_target_property", "get_target_property");

    ClassDB::bind_method(D_METHOD("set_node_retargeter", "retargeter"), &SplineNode2D::set_node_retargeter);
    ClassDB::bind_method(D_METHOD("get_node_retargeter"), &SplineNode2D::get_node_retargeter);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "node_retargeter", PROPERTY_HINT_RESOURCE_TYPE, "NodeRetargeter"),
        "set_node_retargeter",
        "get_node_retargeter"
    );

    ClassDB::bind_method(D_METHOD("set_signal_connector", "connector"), &SplineNode2D::set_signal_connector);
    ClassDB::bind_method(D_METHOD("get_signal_connector"), &SplineNode2D::get_signal_connector);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "signal_connector", PROPERTY_HINT_RESOURCE_TYPE, "SignalConnector"),
        "set_signal_connector",
        "get_signal_connector"
    );

    // --- Playback & Spatial Methods ---
    ClassDB::bind_method(D_METHOD("play", "from_start"), &SplineNode2D::play, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("pause"), &SplineNode2D::pause);
    ClassDB::bind_method(D_METHOD("stop", "reset_to_start"), &SplineNode2D::stop, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("step", "delta_seconds"), &SplineNode2D::step);

    ClassDB::bind_method(D_METHOD("sample_at_percent", "percent"), &SplineNode2D::sample_at_percent);
    ClassDB::bind_method(D_METHOD("sample_transform_at_percent", "percent"), &SplineNode2D::sample_transform_at_percent);
    ClassDB::bind_method(
        D_METHOD("get_closest_point_to", "local_pos", "samples"),
        &SplineNode2D::get_closest_point_to,
        DEFVAL(10)
    );
    ClassDB::bind_method(
        D_METHOD("get_closest_percent_to", "local_pos", "samples"),
        &SplineNode2D::get_closest_percent_to,
        DEFVAL(10)
    );
    ClassDB::bind_method(D_METHOD("_on_spline_changed"), &SplineNode2D::_on_spline_changed);
}

SplineNode2D::SplineNode2D() = default;

SplineNode2D::~SplineNode2D() {
    if (spline.is_valid()) {
        const godot::Callable cb(this, "_on_spline_changed");
        if (spline->is_connected("changed", cb)) {
            spline->disconnect("changed", cb);
        }
    }
}

void SplineNode2D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _ready();
    } else if (p_what == NOTIFICATION_DRAW) {
        _draw();
    } else if (p_what == NOTIFICATION_EXIT_TREE) {
        if (signal_connector.is_valid()) {
            signal_connector->disconnect_signals(this, get_parent() ? get_parent() : this);
        }
    }
}

void SplineNode2D::_ready() {
    if (node_retargeter.is_valid()) {
        node_retargeter->retarget(this, get_parent() ? get_parent() : this);
    }
    if (signal_connector.is_valid()) {
        signal_connector->connect_signals(this, get_parent() ? get_parent() : this);
    }

    if (autoplay && !godot::Engine::get_singleton()->is_editor_hint()) {
        playing = true;
    }

    _update_process_callbacks();
    _evaluate_and_apply(false);
    queue_redraw();
}

void SplineNode2D::_draw() {
    using namespace godot;
    if (!draw_in_viewport || spline.is_null() || spline->get_point_count() < 2) {
        return;
    }

    const PackedVector2Array pts = spline->get_smoothed_points(draw_subdivisions);
    if (pts.size() < 2) return;

    // Apply output_scale and output_offset to rendered vertices
    PackedVector2Array transformed_pts;
    transformed_pts.resize(pts.size());
    for (int i = 0; i < pts.size(); ++i) {
        transformed_pts.set(i, (pts[i] * output_scale) + output_offset);
    }

    // 1. Optional Closed Polygon Fill via ear-clipping triangulation
    if (draw_closed_fill && spline->get_wrap() && transformed_pts.size() >= 3) {
        const PackedInt32Array tris = spline->triangulate(draw_subdivisions);
        if (tris.size() >= 3) {
            PackedColorArray colors;
            colors.resize(transformed_pts.size());
            colors.fill(fill_draw_color);
            draw_polygon(transformed_pts, colors);
        }
    }

    // 2. Smoothed Curve Polyline
    draw_polyline(transformed_pts, spline_draw_color, spline_line_width, true);
}

void SplineNode2D::_process(double p_delta) {
    if (process_mode == SPLINE_PROCESS_IDLE && playing) {
        step(static_cast<real_t>(p_delta));
    }
}

void SplineNode2D::_physics_process(double p_delta) {
    if (process_mode == SPLINE_PROCESS_PHYSICS && playing) {
        step(static_cast<real_t>(p_delta));
    }
}

void SplineNode2D::_on_spline_changed() {
    _evaluate_and_apply(true);
    queue_redraw();
}

void SplineNode2D::_update_process_callbacks() {
    const bool active_idle    = playing && (process_mode == SPLINE_PROCESS_IDLE);
    const bool active_physics = playing && (process_mode == SPLINE_PROCESS_PHYSICS);
    set_process(active_idle);
    set_physics_process(active_physics);
}

void SplineNode2D::_evaluate_and_apply(bool p_emit_signals) {
    if (spline.is_null() || spline->get_point_count() == 0) {
        current_position   = output_offset;
        current_derivative = godot::Vector2();
        current_tangent    = godot::Vector2(1.0f, 0.0f);
        return;
    }

    const godot::Vector2 raw_pos   = spline->sample(current_percent, use_weighted_sampling);
    const godot::Vector2 raw_deriv = spline->sample_derivative(current_percent, use_weighted_sampling);

    current_position   = (raw_pos * output_scale) + output_offset;
    current_derivative = raw_deriv * output_scale;
    current_tangent    = (current_derivative.length_squared() > 1e-8f)
        ? current_derivative.normalized()
        : spline->get_tangent(current_percent, use_weighted_sampling);

    const godot::Vector2 sec_info = spline->get_section_at_percent(current_percent, use_weighted_sampling);
    const int curr_sec = static_cast<int>(sec_info.x);
    if (curr_sec != last_section_index) {
        const int prev_sec = last_section_index;
        last_section_index = curr_sec;
        if (p_emit_signals && prev_sec >= 0) {
            emit_signal("section_crossed", prev_sec, curr_sec);
        }
    }

    // Drive target Node2D or custom indexed property if configured
    if (!target_node_path.is_empty() && is_inside_tree()) {
        if (godot::Node *target = get_node_or_null(target_node_path)) {
            if (!target_property.is_empty()) {
                target->set_indexed(godot::NodePath(godot::String(target_property)), current_position);
            } else if (godot::Node2D *target_2d = godot::Object::cast_to<godot::Node2D>(target)) {
                target_2d->set_position(current_position);
                if (rotate_target_to_tangent) {
                    target_2d->set_rotation(get_current_rotation());
                }
            }
        }
    }

    if (p_emit_signals) {
        emit_signal("position_evaluated", current_percent, current_position, current_derivative, current_tangent);
    }
}

// ============================================================================
// Setters & Getters
// ============================================================================

void SplineNode2D::set_spline(const godot::Ref<Spline2DResource> &p_spline) {
    if (spline == p_spline) return;

    const godot::Callable cb(this, "_on_spline_changed");
    if (spline.is_valid() && spline->is_connected("changed", cb)) {
        spline->disconnect("changed", cb);
    }

    spline = p_spline;
    last_section_index = -1;

    if (spline.is_valid() && !spline->is_connected("changed", cb)) {
        spline->connect("changed", cb);
    }

    _evaluate_and_apply(false);
    queue_redraw();
}

godot::Ref<Spline2DResource> SplineNode2D::get_spline() const {
    return spline;
}

void SplineNode2D::set_process_mode_type(SplineProcessMode p_mode) {
    if (process_mode == p_mode) return;
    process_mode = p_mode;
    _update_process_callbacks();
}

SplineProcessMode SplineNode2D::get_process_mode_type() const {
    return process_mode;
}

void SplineNode2D::set_loop_mode(SplineLoopMode p_mode) {
    loop_mode = p_mode;
}

SplineLoopMode SplineNode2D::get_loop_mode() const {
    return loop_mode;
}

void SplineNode2D::set_playing(bool p_playing) {
    if (playing == p_playing) return;
    playing = p_playing;
    _update_process_callbacks();
}

bool SplineNode2D::is_playing() const {
    return playing;
}

void SplineNode2D::set_autoplay(bool p_autoplay) {
    autoplay = p_autoplay;
}

bool SplineNode2D::get_autoplay() const {
    return autoplay;
}

void SplineNode2D::set_use_weighted_sampling(bool p_weighted) {
    if (use_weighted_sampling == p_weighted) return;
    use_weighted_sampling = p_weighted;
    _evaluate_and_apply(true);
}

bool SplineNode2D::get_use_weighted_sampling() const {
    return use_weighted_sampling;
}

void SplineNode2D::set_duration(real_t p_duration) {
    duration = std::max(static_cast<real_t>(0.0001), p_duration);
}

real_t SplineNode2D::get_duration() const {
    return duration;
}

void SplineNode2D::set_speed_scale(real_t p_speed_scale) {
    speed_scale = p_speed_scale;
}

real_t SplineNode2D::get_speed_scale() const {
    return speed_scale;
}

void SplineNode2D::set_current_percent(real_t p_percent) {
    current_percent = std::clamp(p_percent, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    _evaluate_and_apply(true);
}

real_t SplineNode2D::get_current_percent() const {
    return current_percent;
}

void SplineNode2D::set_output_scale(const godot::Vector2 &p_scale) {
    output_scale = p_scale;
    _evaluate_and_apply(true);
    queue_redraw();
}

godot::Vector2 SplineNode2D::get_output_scale() const {
    return output_scale;
}

void SplineNode2D::set_output_offset(const godot::Vector2 &p_offset) {
    output_offset = p_offset;
    _evaluate_and_apply(true);
    queue_redraw();
}

godot::Vector2 SplineNode2D::get_output_offset() const {
    return output_offset;
}

void SplineNode2D::set_rotate_target_to_tangent(bool p_rotate) {
    rotate_target_to_tangent = p_rotate;
    _evaluate_and_apply(false);
}

bool SplineNode2D::get_rotate_target_to_tangent() const {
    return rotate_target_to_tangent;
}

void SplineNode2D::set_rotation_offset_radians(real_t p_offset_rad) {
    rotation_offset_radians = p_offset_rad;
    _evaluate_and_apply(false);
}

real_t SplineNode2D::get_rotation_offset_radians() const {
    return rotation_offset_radians;
}

real_t SplineNode2D::get_current_rotation() const {
    return static_cast<real_t>(current_tangent.angle()) + rotation_offset_radians;
}

void SplineNode2D::set_draw_in_viewport(bool p_draw) {
    draw_in_viewport = p_draw;
    queue_redraw();
}

void SplineNode2D::set_draw_closed_fill(bool p_fill) {
    draw_closed_fill = p_fill;
    queue_redraw();
}

void SplineNode2D::set_draw_subdivisions(int p_subdivisions) {
    draw_subdivisions = std::clamp(p_subdivisions, 1, 64);
    queue_redraw();
}

void SplineNode2D::set_spline_line_width(float p_width) {
    spline_line_width = std::max(0.5f, p_width);
    queue_redraw();
}

void SplineNode2D::set_spline_draw_color(const godot::Color &p_color) {
    spline_draw_color = p_color;
    queue_redraw();
}

void SplineNode2D::set_fill_draw_color(const godot::Color &p_color) {
    fill_draw_color = p_color;
    queue_redraw();
}

void SplineNode2D::set_target_node_path(const godot::NodePath &p_path) {
    target_node_path = p_path;
    _evaluate_and_apply(false);
}

godot::NodePath SplineNode2D::get_target_node_path() const {
    return target_node_path;
}

void SplineNode2D::set_target_property(const godot::StringName &p_property) {
    target_property = p_property;
    _evaluate_and_apply(false);
}

godot::StringName SplineNode2D::get_target_property() const {
    return target_property;
}

void SplineNode2D::set_node_retargeter(const godot::Ref<NodeRetargeter> &p_retargeter) {
    node_retargeter = p_retargeter;
}

godot::Ref<NodeRetargeter> SplineNode2D::get_node_retargeter() const {
    return node_retargeter;
}

void SplineNode2D::set_signal_connector(const godot::Ref<SignalConnector> &p_connector) {
    signal_connector = p_connector;
}

godot::Ref<SignalConnector> SplineNode2D::get_signal_connector() const {
    return signal_connector;
}

// ============================================================================
// Playback Controls & Spatial Helpers
// ============================================================================

void SplineNode2D::play(bool p_from_start) {
    if (p_from_start) {
        current_percent     = (speed_scale >= static_cast<real_t>(0.0)) ? static_cast<real_t>(0.0) : static_cast<real_t>(1.0);
        ping_pong_direction = static_cast<real_t>(1.0);
        last_section_index  = -1;
    }
    set_playing(true);
    _evaluate_and_apply(true);
}

void SplineNode2D::pause() {
    set_playing(false);
}

void SplineNode2D::stop(bool p_reset_to_start) {
    set_playing(false);
    if (p_reset_to_start) {
        current_percent     = static_cast<real_t>(0.0);
        ping_pong_direction = static_cast<real_t>(1.0);
        last_section_index  = -1;
        _evaluate_and_apply(true);
    }
}

void SplineNode2D::step(real_t p_delta_seconds) {
    if (duration <= static_cast<real_t>(0.0001)) return;

    const real_t delta_pct = (p_delta_seconds * speed_scale * ping_pong_direction) / duration;
    real_t next_pct        = current_percent + delta_pct;

    switch (loop_mode) {
        case SPLINE_LOOP_ONCE: {
            if (next_pct >= static_cast<real_t>(1.0)) {
                current_percent = static_cast<real_t>(1.0);
                _evaluate_and_apply(true);
                set_playing(false);
                emit_signal("playback_finished");
                return;
            }
            if (next_pct <= static_cast<real_t>(0.0) && delta_pct < static_cast<real_t>(0.0)) {
                current_percent = static_cast<real_t>(0.0);
                _evaluate_and_apply(true);
                set_playing(false);
                emit_signal("playback_finished");
                return;
            }
            current_percent = std::clamp(next_pct, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
            break;
        }
        case SPLINE_LOOP_WRAP: {
            bool looped = false;
            if (next_pct > static_cast<real_t>(1.0) || next_pct < static_cast<real_t>(0.0)) {
                next_pct = next_pct - std::floor(next_pct);
                looped   = true;
            }
            current_percent = next_pct;
            _evaluate_and_apply(true);
            if (looped) {
                emit_signal("playback_looped");
            }
            return;
        }
        case SPLINE_LOOP_PING_PONG: {
            bool bounced = false;
            if (next_pct >= static_cast<real_t>(1.0)) {
                next_pct            = static_cast<real_t>(2.0) - next_pct;
                ping_pong_direction = -ping_pong_direction;
                bounced             = true;
            } else if (next_pct <= static_cast<real_t>(0.0)) {
                next_pct            = -next_pct;
                ping_pong_direction = -ping_pong_direction;
                bounced             = true;
            }
            current_percent = std::clamp(next_pct, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
            _evaluate_and_apply(true);
            if (bounced) {
                emit_signal("playback_looped");
            }
            return;
        }
    }

    _evaluate_and_apply(true);
}

godot::Vector2 SplineNode2D::sample_at_percent(real_t p_percent) const {
    if (spline.is_null()) return output_offset;
    return (spline->sample(p_percent, use_weighted_sampling) * output_scale) + output_offset;
}

godot::Transform2D SplineNode2D::sample_transform_at_percent(real_t p_percent) const {
    const godot::Vector2 pos = sample_at_percent(p_percent);
    if (spline.is_null()) {
        return godot::Transform2D(0.0f, pos);
    }
    const godot::Vector2 deriv = spline->sample_derivative(p_percent, use_weighted_sampling) * output_scale;
    const godot::Vector2 tan   = (deriv.length_squared() > 1e-8f)
        ? deriv.normalized()
        : spline->get_tangent(p_percent, use_weighted_sampling);
    const float rot = tan.angle() + static_cast<float>(rotation_offset_radians);
    return godot::Transform2D(rot, pos);
}

godot::Vector2 SplineNode2D::get_closest_point_to(const godot::Vector2 &p_local_pos, int p_samples) const {
    if (spline.is_null() || std::abs(output_scale.x) <= 1e-6f || std::abs(output_scale.y) <= 1e-6f) {
        return output_offset;
    }
    const godot::Vector2 unscaled = (p_local_pos - output_offset) / output_scale;
    return (spline->get_closest_point_on_spline(unscaled, p_samples) * output_scale) + output_offset;
}

real_t SplineNode2D::get_closest_percent_to(const godot::Vector2 &p_local_pos, int p_samples) const {
    if (spline.is_null() || std::abs(output_scale.x) <= 1e-6f || std::abs(output_scale.y) <= 1e-6f) {
        return static_cast<real_t>(0.0);
    }
    const godot::Vector2 unscaled = (p_local_pos - output_offset) / output_scale;
    return spline->get_closest_percent_on_spline(unscaled, p_samples, use_weighted_sampling);
}

} // namespace ideam::godot_ext