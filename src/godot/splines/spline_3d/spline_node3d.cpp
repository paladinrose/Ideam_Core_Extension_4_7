#include "spline_node3d.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/quaternion.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void SplineNode3D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SPLINE_3D_ROTATION_NONE);
    BIND_ENUM_CONSTANT(SPLINE_3D_ROTATION_UP_VECTOR);
    BIND_ENUM_CONSTANT(SPLINE_3D_ROTATION_BISHOP_TRANSPORT);

    // --- Signals ---
    ADD_SIGNAL(MethodInfo(
        "transform_evaluated",
        PropertyInfo(Variant::FLOAT, "percent"),
        PropertyInfo(Variant::VECTOR3, "position"),
        PropertyInfo(Variant::VECTOR3, "derivative"),
        PropertyInfo(Variant::BASIS, "basis")
    ));
    ADD_SIGNAL(MethodInfo(
        "section_crossed",
        PropertyInfo(Variant::INT, "previous_section"),
        PropertyInfo(Variant::INT, "current_section")
    ));
    ADD_SIGNAL(MethodInfo("playback_finished"));
    ADD_SIGNAL(MethodInfo("playback_looped"));
    ADD_SIGNAL(MethodInfo(
        "gizmo_point_selected",
        PropertyInfo(Variant::INT, "point_index")
    ));
    ADD_SIGNAL(MethodInfo(
        "gizmo_section_selected",
        PropertyInfo(Variant::INT, "section_index")
    ));

    // --- Resource Binding ---
    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &SplineNode3D::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &SplineNode3D::get_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"),
        "set_spline",
        "get_spline"
    );

    // --- Playback & Sampling Group ---
    ADD_GROUP("Playback & Sampling", "");

    ClassDB::bind_method(D_METHOD("set_process_mode_type", "mode"), &SplineNode3D::set_process_mode_type);
    ClassDB::bind_method(D_METHOD("get_process_mode_type"), &SplineNode3D::get_process_mode_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "process_mode_type", PROPERTY_HINT_ENUM, "Idle:0,Physics:1,Manual:2"),
        "set_process_mode_type",
        "get_process_mode_type"
    );

    ClassDB::bind_method(D_METHOD("set_loop_mode", "mode"), &SplineNode3D::set_loop_mode);
    ClassDB::bind_method(D_METHOD("get_loop_mode"), &SplineNode3D::get_loop_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "loop_mode", PROPERTY_HINT_ENUM, "Once:0,Wrap:1,Ping-Pong:2"),
        "set_loop_mode",
        "get_loop_mode"
    );

    ClassDB::bind_method(D_METHOD("set_playing", "playing"), &SplineNode3D::set_playing);
    ClassDB::bind_method(D_METHOD("is_playing"), &SplineNode3D::is_playing);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing"), "set_playing", "is_playing");

    ClassDB::bind_method(D_METHOD("set_autoplay", "autoplay"), &SplineNode3D::set_autoplay);
    ClassDB::bind_method(D_METHOD("get_autoplay"), &SplineNode3D::get_autoplay);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

    ClassDB::bind_method(D_METHOD("set_use_weighted_sampling", "weighted"), &SplineNode3D::set_use_weighted_sampling);
    ClassDB::bind_method(D_METHOD("get_use_weighted_sampling"), &SplineNode3D::get_use_weighted_sampling);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "use_weighted_sampling"),
        "set_use_weighted_sampling",
        "get_use_weighted_sampling"
    );

    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &SplineNode3D::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &SplineNode3D::get_duration);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0.001,600.0,0.01,or_greater,suffix:s"),
        "set_duration",
        "get_duration"
    );

    ClassDB::bind_method(D_METHOD("set_speed_scale", "speed_scale"), &SplineNode3D::set_speed_scale);
    ClassDB::bind_method(D_METHOD("get_speed_scale"), &SplineNode3D::get_speed_scale);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "-10.0,10.0,0.01,or_less,or_greater"),
        "set_speed_scale",
        "get_speed_scale"
    );

    ClassDB::bind_method(D_METHOD("set_current_percent", "percent"), &SplineNode3D::set_current_percent);
    ClassDB::bind_method(D_METHOD("get_current_percent"), &SplineNode3D::get_current_percent);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "current_percent", PROPERTY_HINT_RANGE, "0.0,1.0,0.001"),
        "set_current_percent",
        "get_current_percent"
    );

    // --- 3D Output Mapping & Orientation Group ---
    ADD_GROUP("Output Mapping & Orientation", "");

    ClassDB::bind_method(D_METHOD("set_output_scale", "scale"), &SplineNode3D::set_output_scale);
    ClassDB::bind_method(D_METHOD("get_output_scale"), &SplineNode3D::get_output_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "output_scale"), "set_output_scale", "get_output_scale");

    ClassDB::bind_method(D_METHOD("set_output_offset", "offset"), &SplineNode3D::set_output_offset);
    ClassDB::bind_method(D_METHOD("get_output_offset"), &SplineNode3D::get_output_offset);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "output_offset"), "set_output_offset", "get_output_offset");

    ClassDB::bind_method(D_METHOD("set_rotation_mode", "mode"), &SplineNode3D::set_rotation_mode);
    ClassDB::bind_method(D_METHOD("get_rotation_mode"), &SplineNode3D::get_rotation_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "rotation_mode", PROPERTY_HINT_ENUM, "None:0,Up Vector:1,Bishop Parallel Transport:2"),
        "set_rotation_mode",
        "get_rotation_mode"
    );

    ClassDB::bind_method(D_METHOD("set_up_vector", "up"), &SplineNode3D::set_up_vector);
    ClassDB::bind_method(D_METHOD("get_up_vector"), &SplineNode3D::get_up_vector);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "up_vector"), "set_up_vector", "get_up_vector");

    ClassDB::bind_method(D_METHOD("set_roll_offset_radians", "roll_rad"), &SplineNode3D::set_roll_offset_radians);
    ClassDB::bind_method(D_METHOD("get_roll_offset_radians"), &SplineNode3D::get_roll_offset_radians);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "roll_offset_radians", PROPERTY_HINT_RANGE, "-6.28319,6.28319,0.01,radians_as_degrees"),
        "set_roll_offset_radians",
        "get_roll_offset_radians"
    );

    ClassDB::bind_method(D_METHOD("get_current_position"), &SplineNode3D::get_current_position);
    ClassDB::bind_method(D_METHOD("get_current_derivative"), &SplineNode3D::get_current_derivative);
    ClassDB::bind_method(D_METHOD("get_current_tangent"), &SplineNode3D::get_current_tangent);
    ClassDB::bind_method(D_METHOD("get_current_basis"), &SplineNode3D::get_current_basis);
    ClassDB::bind_method(D_METHOD("get_current_transform_on_spline"), &SplineNode3D::get_current_transform_on_spline);
    ClassDB::bind_method(D_METHOD("get_current_section_index"), &SplineNode3D::get_current_section_index);

    // --- 3D Editor Gizmo Hooks Group ---
    ADD_GROUP("3D Gizmo Settings", "");

    ClassDB::bind_method(D_METHOD("set_selected_gizmo_point_index", "index"), &SplineNode3D::set_selected_gizmo_point_index);
    ClassDB::bind_method(D_METHOD("get_selected_gizmo_point_index"), &SplineNode3D::get_selected_gizmo_point_index);

    ClassDB::bind_method(D_METHOD("set_selected_gizmo_section_index", "index"), &SplineNode3D::set_selected_gizmo_section_index);
    ClassDB::bind_method(D_METHOD("get_selected_gizmo_section_index"), &SplineNode3D::get_selected_gizmo_section_index);

    ClassDB::bind_method(D_METHOD("set_show_gizmo_frames", "show"), &SplineNode3D::set_show_gizmo_frames);
    ClassDB::bind_method(D_METHOD("get_show_gizmo_frames"), &SplineNode3D::get_show_gizmo_frames);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_gizmo_frames"), "set_show_gizmo_frames", "get_show_gizmo_frames");

    ClassDB::bind_method(D_METHOD("set_gizmo_subdivisions", "subdivisions"), &SplineNode3D::set_gizmo_subdivisions);
    ClassDB::bind_method(D_METHOD("get_gizmo_subdivisions"), &SplineNode3D::get_gizmo_subdivisions);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "gizmo_subdivisions", PROPERTY_HINT_RANGE, "2,64,1"),
        "set_gizmo_subdivisions",
        "get_gizmo_subdivisions"
    );

    // --- Target & Utilities Group ---
    ADD_GROUP("Target & Utilities", "");

    ClassDB::bind_method(D_METHOD("set_target_node_path", "path"), &SplineNode3D::set_target_node_path);
    ClassDB::bind_method(D_METHOD("get_target_node_path"), &SplineNode3D::get_target_node_path);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target_node_path"), "set_target_node_path", "get_target_node_path");

    ClassDB::bind_method(D_METHOD("set_target_property", "property"), &SplineNode3D::set_target_property);
    ClassDB::bind_method(D_METHOD("get_target_property"), &SplineNode3D::get_target_property);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "target_property"), "set_target_property", "get_target_property");

    ClassDB::bind_method(D_METHOD("set_node_retargeter", "retargeter"), &SplineNode3D::set_node_retargeter);
    ClassDB::bind_method(D_METHOD("get_node_retargeter"), &SplineNode3D::get_node_retargeter);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "node_retargeter", PROPERTY_HINT_RESOURCE_TYPE, "NodeRetargeter"),
        "set_node_retargeter",
        "get_node_retargeter"
    );

    ClassDB::bind_method(D_METHOD("set_signal_connector", "connector"), &SplineNode3D::set_signal_connector);
    ClassDB::bind_method(D_METHOD("get_signal_connector"), &SplineNode3D::get_signal_connector);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "signal_connector", PROPERTY_HINT_RESOURCE_TYPE, "SignalConnector"),
        "set_signal_connector",
        "get_signal_connector"
    );

    // --- Playback & Spatial Methods ---
    ClassDB::bind_method(D_METHOD("play", "from_start"), &SplineNode3D::play, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("pause"), &SplineNode3D::pause);
    ClassDB::bind_method(D_METHOD("stop", "reset_to_start"), &SplineNode3D::stop, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("step", "delta_seconds"), &SplineNode3D::step);

    ClassDB::bind_method(D_METHOD("sample_at_percent", "percent"), &SplineNode3D::sample_at_percent);
    ClassDB::bind_method(D_METHOD("sample_transform_at_percent", "percent"), &SplineNode3D::sample_transform_at_percent);
    ClassDB::bind_method(
        D_METHOD("get_closest_point_to", "local_pos", "samples"),
        &SplineNode3D::get_closest_point_to,
        DEFVAL(10)
    );
    ClassDB::bind_method(
        D_METHOD("get_closest_percent_to", "local_pos", "samples"),
        &SplineNode3D::get_closest_percent_to,
        DEFVAL(10)
    );
    ClassDB::bind_method(D_METHOD("_on_spline_changed"), &SplineNode3D::_on_spline_changed);
}

SplineNode3D::SplineNode3D() = default;

SplineNode3D::~SplineNode3D() {
    if (spline.is_valid()) {
        const godot::Callable cb(this, "_on_spline_changed");
        if (spline->is_connected("changed", cb)) {
            spline->disconnect("changed", cb);
        }
    }
}

void SplineNode3D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _ready();
    } else if (p_what == NOTIFICATION_EXIT_TREE) {
        if (signal_connector.is_valid()) {
            signal_connector->disconnect_signals(this, get_parent() ? get_parent() : this);
        }
    }
}

void SplineNode3D::_ready() {
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
    update_gizmos();
}

void SplineNode3D::_process(double p_delta) {
    if (process_mode == SPLINE_PROCESS_IDLE && playing) {
        step(static_cast<real_t>(p_delta));
    }
}

void SplineNode3D::_physics_process(double p_delta) {
    if (process_mode == SPLINE_PROCESS_PHYSICS && playing) {
        step(static_cast<real_t>(p_delta));
    }
}

void SplineNode3D::_on_spline_changed() {
    bishop_cache_dirty = true;
    _evaluate_and_apply(true);
    update_gizmos();
}

void SplineNode3D::_update_process_callbacks() {
    const bool active_idle    = playing && (process_mode == SPLINE_PROCESS_IDLE);
    const bool active_physics = playing && (process_mode == SPLINE_PROCESS_PHYSICS);
    set_process(active_idle);
    set_physics_process(active_physics);
}

void SplineNode3D::_ensure_bishop_cache() const {
    if (!bishop_cache_dirty) return;
    cached_bishop_transforms.clear();
    if (spline.is_valid() && spline->get_section_count() > 0) {
        cached_bishop_transforms = spline->compute_bishop_transforms(12, up_vector);
    }
    bishop_cache_dirty = false;
}

void SplineNode3D::_evaluate_and_apply(bool p_emit_signals) {
    if (spline.is_null() || spline->get_point_count() == 0) {
        current_position   = output_offset;
        current_derivative = godot::Vector3();
        current_tangent    = godot::Vector3(0.0f, 0.0f, -1.0f);
        current_basis      = godot::Basis();
        return;
    }

    const godot::Transform3D xform = sample_transform_at_percent(current_percent);
    current_position   = xform.origin;
    current_basis      = xform.basis;
    current_derivative = spline->sample_derivative(current_percent, use_weighted_sampling) * output_scale;
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

    // Drive target Node3D or custom indexed property if configured
    if (!target_node_path.is_empty() && is_inside_tree()) {
        if (godot::Node *target = get_node_or_null(target_node_path)) {
            if (!target_property.is_empty()) {
                target->set_indexed(godot::NodePath(godot::String(target_property)), current_position);
            } else if (godot::Node3D *target_3d = godot::Object::cast_to<godot::Node3D>(target)) {
                target_3d->set_position(current_position);
                if (rotation_mode != SPLINE_3D_ROTATION_NONE) {
                    target_3d->set_basis(current_basis);
                }
            }
        }
    }

    if (p_emit_signals) {
        emit_signal("transform_evaluated", current_percent, current_position, current_derivative, current_basis);
    }
}

// ============================================================================
// Setters & Getters
// ============================================================================

void SplineNode3D::set_spline(const godot::Ref<Spline3DResource> &p_spline) {
    if (spline == p_spline) return;

    const godot::Callable cb(this, "_on_spline_changed");
    if (spline.is_valid() && spline->is_connected("changed", cb)) {
        spline->disconnect("changed", cb);
    }

    spline             = p_spline;
    last_section_index = -1;
    bishop_cache_dirty = true;

    if (spline.is_valid() && !spline->is_connected("changed", cb)) {
        spline->connect("changed", cb);
    }

    _evaluate_and_apply(false);
    update_gizmos();
}

godot::Ref<Spline3DResource> SplineNode3D::get_spline() const {
    return spline;
}

void SplineNode3D::set_process_mode_type(SplineProcessMode p_mode) {
    if (process_mode == p_mode) return;
    process_mode = p_mode;
    _update_process_callbacks();
}

SplineProcessMode SplineNode3D::get_process_mode_type() const {
    return process_mode;
}

void SplineNode3D::set_loop_mode(SplineLoopMode p_mode) {
    loop_mode = p_mode;
}

SplineLoopMode SplineNode3D::get_loop_mode() const {
    return loop_mode;
}

void SplineNode3D::set_playing(bool p_playing) {
    if (playing == p_playing) return;
    playing = p_playing;
    _update_process_callbacks();
}

bool SplineNode3D::is_playing() const {
    return playing;
}

void SplineNode3D::set_autoplay(bool p_autoplay) {
    autoplay = p_autoplay;
}

bool SplineNode3D::get_autoplay() const {
    return autoplay;
}

void SplineNode3D::set_use_weighted_sampling(bool p_weighted) {
    if (use_weighted_sampling == p_weighted) return;
    use_weighted_sampling = p_weighted;
    _evaluate_and_apply(true);
}

bool SplineNode3D::get_use_weighted_sampling() const {
    return use_weighted_sampling;
}

void SplineNode3D::set_duration(real_t p_duration) {
    duration = std::max(static_cast<real_t>(0.0001), p_duration);
}

real_t SplineNode3D::get_duration() const {
    return duration;
}

void SplineNode3D::set_speed_scale(real_t p_speed_scale) {
    speed_scale = p_speed_scale;
}

real_t SplineNode3D::get_speed_scale() const {
    return speed_scale;
}

void SplineNode3D::set_current_percent(real_t p_percent) {
    current_percent = std::clamp(p_percent, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    _evaluate_and_apply(true);
}

real_t SplineNode3D::get_current_percent() const {
    return current_percent;
}

void SplineNode3D::set_output_scale(const godot::Vector3 &p_scale) {
    output_scale = p_scale;
    _evaluate_and_apply(true);
    update_gizmos();
}

godot::Vector3 SplineNode3D::get_output_scale() const {
    return output_scale;
}

void SplineNode3D::set_output_offset(const godot::Vector3 &p_offset) {
    output_offset = p_offset;
    _evaluate_and_apply(true);
    update_gizmos();
}

godot::Vector3 SplineNode3D::get_output_offset() const {
    return output_offset;
}

void SplineNode3D::set_rotation_mode(Spline3DRotationMode p_mode) {
    if (rotation_mode == p_mode) return;
    rotation_mode = p_mode;
    _evaluate_and_apply(false);
}

Spline3DRotationMode SplineNode3D::get_rotation_mode() const {
    return rotation_mode;
}

void SplineNode3D::set_up_vector(const godot::Vector3 &p_up) {
    up_vector          = (p_up.length_squared() > 1e-6f) ? p_up.normalized() : godot::Vector3(0.0f, 1.0f, 0.0f);
    bishop_cache_dirty = true;
    _evaluate_and_apply(false);
    update_gizmos();
}

godot::Vector3 SplineNode3D::get_up_vector() const {
    return up_vector;
}

void SplineNode3D::set_roll_offset_radians(real_t p_roll_rad) {
    roll_offset_radians = p_roll_rad;
    _evaluate_and_apply(false);
    update_gizmos();
}

real_t SplineNode3D::get_roll_offset_radians() const {
    return roll_offset_radians;
}

void SplineNode3D::set_selected_gizmo_point_index(int p_index) {
    if (selected_gizmo_point_index == p_index && selected_gizmo_section_index == -1) return;
    selected_gizmo_point_index   = p_index;
    selected_gizmo_section_index = -1;
    update_gizmos();
    emit_signal("gizmo_point_selected", selected_gizmo_point_index);
}

void SplineNode3D::set_selected_gizmo_section_index(int p_index) {
    if (selected_gizmo_section_index == p_index && selected_gizmo_point_index == -1) return;
    selected_gizmo_section_index = p_index;
    selected_gizmo_point_index   = -1;
    update_gizmos();
    emit_signal("gizmo_section_selected", selected_gizmo_section_index);
}

void SplineNode3D::set_show_gizmo_frames(bool p_show) {
    show_gizmo_frames = p_show;
    update_gizmos();
}

void SplineNode3D::set_gizmo_subdivisions(int p_subdivisions) {
    gizmo_subdivisions = std::clamp(p_subdivisions, 2, 64);
    update_gizmos();
}

void SplineNode3D::set_target_node_path(const godot::NodePath &p_path) {
    target_node_path = p_path;
    _evaluate_and_apply(false);
}

godot::NodePath SplineNode3D::get_target_node_path() const {
    return target_node_path;
}

void SplineNode3D::set_target_property(const godot::StringName &p_property) {
    target_property = p_property;
    _evaluate_and_apply(false);
}

godot::StringName SplineNode3D::get_target_property() const {
    return target_property;
}

void SplineNode3D::set_node_retargeter(const godot::Ref<NodeRetargeter> &p_retargeter) {
    node_retargeter = p_retargeter;
}

godot::Ref<NodeRetargeter> SplineNode3D::get_node_retargeter() const {
    return node_retargeter;
}

void SplineNode3D::set_signal_connector(const godot::Ref<SignalConnector> &p_connector) {
    signal_connector = p_connector;
}

godot::Ref<SignalConnector> SplineNode3D::get_signal_connector() const {
    return signal_connector;
}

// ============================================================================
// Playback Controls & 3D Spatial Helpers
// ============================================================================

void SplineNode3D::play(bool p_from_start) {
    if (p_from_start) {
        current_percent     = (speed_scale >= static_cast<real_t>(0.0)) ? static_cast<real_t>(0.0) : static_cast<real_t>(1.0);
        ping_pong_direction = static_cast<real_t>(1.0);
        last_section_index  = -1;
    }
    set_playing(true);
    _evaluate_and_apply(true);
}

void SplineNode3D::pause() {
    set_playing(false);
}

void SplineNode3D::stop(bool p_reset_to_start) {
    set_playing(false);
    if (p_reset_to_start) {
        current_percent     = static_cast<real_t>(0.0);
        ping_pong_direction = static_cast<real_t>(1.0);
        last_section_index  = -1;
        _evaluate_and_apply(true);
    }
}

void SplineNode3D::step(real_t p_delta_seconds) {
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

godot::Vector3 SplineNode3D::sample_at_percent(real_t p_percent) const {
    if (spline.is_null()) return output_offset;
    return (spline->sample(p_percent, use_weighted_sampling) * output_scale) + output_offset;
}

godot::Transform3D SplineNode3D::sample_transform_at_percent(real_t p_percent) const {
    const godot::Vector3 pos = sample_at_percent(p_percent);
    if (spline.is_null() || rotation_mode == SPLINE_3D_ROTATION_NONE) {
        return godot::Transform3D(godot::Basis(), pos);
    }

    godot::Basis basis;
    if (rotation_mode == SPLINE_3D_ROTATION_BISHOP_TRANSPORT) {
        _ensure_bishop_cache();
        const int frame_count = cached_bishop_transforms.size();
        if (frame_count >= 2) {
            const float f_idx = std::clamp(static_cast<float>(p_percent), 0.0f, 1.0f) * static_cast<float>(frame_count - 1);
            const int i0      = std::clamp(static_cast<int>(std::floor(f_idx)), 0, frame_count - 1);
            const int i1      = std::min(frame_count - 1, i0 + 1);
            const float frac  = f_idx - static_cast<float>(i0);
            const godot::Transform3D t0 = cached_bishop_transforms[i0];
            const godot::Transform3D t1 = cached_bishop_transforms[i1];
            const godot::Quaternion q0  = t0.basis.get_rotation_quaternion();
            const godot::Quaternion q1  = t1.basis.get_rotation_quaternion();
            basis = godot::Basis(q0.slerp(q1, frac));
        } else {
            basis = spline->get_transform_at_percent(p_percent, use_weighted_sampling, up_vector).basis;
        }
    } else {
        basis = spline->get_transform_at_percent(p_percent, use_weighted_sampling, up_vector).basis;
    }

    if (std::abs(roll_offset_radians) > static_cast<real_t>(1e-6)) {
        const godot::Vector3 forward_axis = -basis.get_column(2).normalized();
        basis = godot::Basis(forward_axis, static_cast<float>(roll_offset_radians)) * basis;
    }

    return godot::Transform3D(basis, pos);
}

godot::Vector3 SplineNode3D::get_closest_point_to(const godot::Vector3 &p_local_pos, int p_samples) const {
    if (spline.is_null() ||
        std::abs(output_scale.x) <= 1e-6f ||
        std::abs(output_scale.y) <= 1e-6f ||
        std::abs(output_scale.z) <= 1e-6f) {
        return output_offset;
    }
    const godot::Vector3 unscaled = (p_local_pos - output_offset) / output_scale;
    return (spline->get_closest_point_on_spline(unscaled, p_samples) * output_scale) + output_offset;
}

real_t SplineNode3D::get_closest_percent_to(const godot::Vector3 &p_local_pos, int p_samples) const {
    if (spline.is_null() ||
        std::abs(output_scale.x) <= 1e-6f ||
        std::abs(output_scale.y) <= 1e-6f ||
        std::abs(output_scale.z) <= 1e-6f) {
        return static_cast<real_t>(0.0);
    }
    const godot::Vector3 unscaled = (p_local_pos - output_offset) / output_scale;
    return spline->get_closest_percent_on_spline(unscaled, p_samples, use_weighted_sampling);
}

} // namespace ideam::godot_ext