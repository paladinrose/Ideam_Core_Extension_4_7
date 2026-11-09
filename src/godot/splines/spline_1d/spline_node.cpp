#include "spline_node.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void SplineNode::_bind_methods() {
    using namespace godot;

    // --- Enums ---
    BIND_ENUM_CONSTANT(SPLINE_PROCESS_IDLE);
    BIND_ENUM_CONSTANT(SPLINE_PROCESS_PHYSICS);
    BIND_ENUM_CONSTANT(SPLINE_PROCESS_MANUAL);

    BIND_ENUM_CONSTANT(SPLINE_LOOP_ONCE);
    BIND_ENUM_CONSTANT(SPLINE_LOOP_WRAP);
    BIND_ENUM_CONSTANT(SPLINE_LOOP_PING_PONG);

    // --- Signals ---
    ADD_SIGNAL(MethodInfo(
        "value_evaluated",
        PropertyInfo(Variant::FLOAT, "percent"),
        PropertyInfo(Variant::FLOAT, "value"),
        PropertyInfo(Variant::FLOAT, "derivative")
    ));
    ADD_SIGNAL(MethodInfo(
        "section_crossed",
        PropertyInfo(Variant::INT, "previous_section"),
        PropertyInfo(Variant::INT, "current_section")
    ));
    ADD_SIGNAL(MethodInfo("playback_finished"));
    ADD_SIGNAL(MethodInfo("playback_looped"));

    // --- Resource Binding ---
    ClassDB::bind_method(D_METHOD("set_spline", "spline"), &SplineNode::set_spline);
    ClassDB::bind_method(D_METHOD("get_spline"), &SplineNode::get_spline);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_spline",
        "get_spline"
    );

    // --- Playback & Sampling Group ---
    ADD_GROUP("Playback & Sampling", "");

    ClassDB::bind_method(D_METHOD("set_process_mode_type", "mode"), &SplineNode::set_process_mode_type);
    ClassDB::bind_method(D_METHOD("get_process_mode_type"), &SplineNode::get_process_mode_type);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "process_mode_type", PROPERTY_HINT_ENUM, "Idle:0,Physics:1,Manual:2"),
        "set_process_mode_type",
        "get_process_mode_type"
    );

    ClassDB::bind_method(D_METHOD("set_loop_mode", "mode"), &SplineNode::set_loop_mode);
    ClassDB::bind_method(D_METHOD("get_loop_mode"), &SplineNode::get_loop_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "loop_mode", PROPERTY_HINT_ENUM, "Once:0,Wrap:1,Ping-Pong:2"),
        "set_loop_mode",
        "get_loop_mode"
    );

    ClassDB::bind_method(D_METHOD("set_playing", "playing"), &SplineNode::set_playing);
    ClassDB::bind_method(D_METHOD("is_playing"), &SplineNode::is_playing);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing"), "set_playing", "is_playing");

    ClassDB::bind_method(D_METHOD("set_autoplay", "autoplay"), &SplineNode::set_autoplay);
    ClassDB::bind_method(D_METHOD("get_autoplay"), &SplineNode::get_autoplay);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");

    ClassDB::bind_method(D_METHOD("set_use_weighted_sampling", "weighted"), &SplineNode::set_use_weighted_sampling);
    ClassDB::bind_method(D_METHOD("get_use_weighted_sampling"), &SplineNode::get_use_weighted_sampling);
    ADD_PROPERTY(
        PropertyInfo(Variant::BOOL, "use_weighted_sampling"),
        "set_use_weighted_sampling",
        "get_use_weighted_sampling"
    );

    ClassDB::bind_method(D_METHOD("set_duration", "duration"), &SplineNode::set_duration);
    ClassDB::bind_method(D_METHOD("get_duration"), &SplineNode::get_duration);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0.001,600.0,0.01,or_greater,suffix:s"),
        "set_duration",
        "get_duration"
    );

    ClassDB::bind_method(D_METHOD("set_speed_scale", "speed_scale"), &SplineNode::set_speed_scale);
    ClassDB::bind_method(D_METHOD("get_speed_scale"), &SplineNode::get_speed_scale);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "-10.0,10.0,0.01,or_less,or_greater"),
        "set_speed_scale",
        "get_speed_scale"
    );

    ClassDB::bind_method(D_METHOD("set_current_percent", "percent"), &SplineNode::set_current_percent);
    ClassDB::bind_method(D_METHOD("get_current_percent"), &SplineNode::get_current_percent);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "current_percent", PROPERTY_HINT_RANGE, "0.0,1.0,0.001"),
        "set_current_percent",
        "get_current_percent"
    );

    // --- Output Mapping Group ---
    ADD_GROUP("Output Mapping", "");

    ClassDB::bind_method(D_METHOD("set_output_scale", "scale"), &SplineNode::set_output_scale);
    ClassDB::bind_method(D_METHOD("get_output_scale"), &SplineNode::get_output_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "output_scale"), "set_output_scale", "get_output_scale");

    ClassDB::bind_method(D_METHOD("set_output_offset", "offset"), &SplineNode::set_output_offset);
    ClassDB::bind_method(D_METHOD("get_output_offset"), &SplineNode::get_output_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "output_offset"), "set_output_offset", "get_output_offset");

    ClassDB::bind_method(D_METHOD("get_current_value"), &SplineNode::get_current_value);
    ClassDB::bind_method(D_METHOD("get_current_derivative"), &SplineNode::get_current_derivative);
    ClassDB::bind_method(D_METHOD("get_current_section_index"), &SplineNode::get_current_section_index);

    // --- Target Property & Utilities Group ---
    ADD_GROUP("Target & Utilities", "");

    ClassDB::bind_method(D_METHOD("set_target_node_path", "path"), &SplineNode::set_target_node_path);
    ClassDB::bind_method(D_METHOD("get_target_node_path"), &SplineNode::get_target_node_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "target_node_path"),
        "set_target_node_path",
        "get_target_node_path"
    );

    ClassDB::bind_method(D_METHOD("set_target_property", "property"), &SplineNode::set_target_property);
    ClassDB::bind_method(D_METHOD("get_target_property"), &SplineNode::get_target_property);
    ADD_PROPERTY(
        PropertyInfo(Variant::STRING_NAME, "target_property"),
        "set_target_property",
        "get_target_property"
    );

    ClassDB::bind_method(D_METHOD("set_node_retargeter", "retargeter"), &SplineNode::set_node_retargeter);
    ClassDB::bind_method(D_METHOD("get_node_retargeter"), &SplineNode::get_node_retargeter);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "node_retargeter", PROPERTY_HINT_RESOURCE_TYPE, "NodeRetargeter"),
        "set_node_retargeter",
        "get_node_retargeter"
    );

    ClassDB::bind_method(D_METHOD("set_signal_connector", "connector"), &SplineNode::set_signal_connector);
    ClassDB::bind_method(D_METHOD("get_signal_connector"), &SplineNode::get_signal_connector);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "signal_connector", PROPERTY_HINT_RESOURCE_TYPE, "SignalConnector"),
        "set_signal_connector",
        "get_signal_connector"
    );

    // --- Playback & Query Methods ---
    ClassDB::bind_method(D_METHOD("play", "from_start"), &SplineNode::play, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("pause"), &SplineNode::pause);
    ClassDB::bind_method(D_METHOD("stop", "reset_to_start"), &SplineNode::stop, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("step", "delta_seconds"), &SplineNode::step);

    ClassDB::bind_method(D_METHOD("sample_at_percent", "percent"), &SplineNode::sample_at_percent);
    ClassDB::bind_method(
        D_METHOD("solve_percents_for_output_value", "target_output", "samples"),
        &SplineNode::solve_percents_for_output_value,
        DEFVAL(10)
    );
    ClassDB::bind_method(D_METHOD("_on_spline_changed"), &SplineNode::_on_spline_changed);
}

SplineNode::SplineNode() {
}

SplineNode::~SplineNode() {
    if (spline.is_valid()) {
        const godot::Callable cb(this, "_on_spline_changed");
        if (spline->is_connected("changed", cb)) {
            spline->disconnect("changed", cb);
        }
    }
}

void SplineNode::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _ready();
    } else if (p_what == NOTIFICATION_EXIT_TREE) {
        if (signal_connector.is_valid()) {
            signal_connector->disconnect_signals(this, get_parent() ? get_parent() : this);
        }
    }
}

void SplineNode::_ready() {
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
}

void SplineNode::_process(double p_delta) {
    if (process_mode == SPLINE_PROCESS_IDLE && playing) {
        step(static_cast<real_t>(p_delta));
    }
}

void SplineNode::_physics_process(double p_delta) {
    if (process_mode == SPLINE_PROCESS_PHYSICS && playing) {
        step(static_cast<real_t>(p_delta));
    }
}

void SplineNode::_on_spline_changed() {
    _evaluate_and_apply(true);
}

void SplineNode::_update_process_callbacks() {
    const bool active_idle    = playing && (process_mode == SPLINE_PROCESS_IDLE);
    const bool active_physics = playing && (process_mode == SPLINE_PROCESS_PHYSICS);
    set_process(active_idle);
    set_physics_process(active_physics);
}

void SplineNode::_evaluate_and_apply(bool p_emit_signals) {
    if (spline.is_null() || spline->get_point_count() == 0) {
        current_value      = output_offset;
        current_derivative = static_cast<real_t>(0.0);
        return;
    }

    const real_t raw_val   = spline->sample(current_percent, use_weighted_sampling);
    const real_t raw_deriv = spline->sample_derivative(current_percent, use_weighted_sampling);

    current_value      = (raw_val * output_scale) + output_offset;
    current_derivative = raw_deriv * output_scale;

    const godot::Vector2 sec_info = spline->get_section_at_percent(current_percent, use_weighted_sampling);
    const int curr_sec = static_cast<int>(sec_info.x);
    if (curr_sec != last_section_index) {
        const int prev_sec = last_section_index;
        last_section_index = curr_sec;
        if (p_emit_signals && prev_sec >= 0) {
            emit_signal("section_crossed", prev_sec, curr_sec);
        }
    }

    // Drive target Node property if configured
    if (!target_node_path.is_empty() && !target_property.is_empty() && is_inside_tree()) {
        if (godot::Node *target = get_node_or_null(target_node_path)) {
            target->set_indexed(godot::NodePath(godot::String(target_property)), current_value);
        }
    }

    if (p_emit_signals) {
        emit_signal("value_evaluated", current_percent, current_value, current_derivative);
    }
}

// ============================================================================
// Setters & Getters
// ============================================================================

void SplineNode::set_spline(const godot::Ref<Spline1DResource> &p_spline) {
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
}

godot::Ref<Spline1DResource> SplineNode::get_spline() const {
    return spline;
}

void SplineNode::set_process_mode_type(SplineProcessMode p_mode) {
    if (process_mode == p_mode) return;
    process_mode = p_mode;
    _update_process_callbacks();
}

SplineProcessMode SplineNode::get_process_mode_type() const {
    return process_mode;
}

void SplineNode::set_loop_mode(SplineLoopMode p_mode) {
    loop_mode = p_mode;
}

SplineLoopMode SplineNode::get_loop_mode() const {
    return loop_mode;
}

void SplineNode::set_playing(bool p_playing) {
    if (playing == p_playing) return;
    playing = p_playing;
    _update_process_callbacks();
}

bool SplineNode::is_playing() const {
    return playing;
}

void SplineNode::set_autoplay(bool p_autoplay) {
    autoplay = p_autoplay;
}

bool SplineNode::get_autoplay() const {
    return autoplay;
}

void SplineNode::set_use_weighted_sampling(bool p_weighted) {
    if (use_weighted_sampling == p_weighted) return;
    use_weighted_sampling = p_weighted;
    _evaluate_and_apply(true);
}

bool SplineNode::get_use_weighted_sampling() const {
    return use_weighted_sampling;
}

void SplineNode::set_duration(real_t p_duration) {
    duration = std::max(static_cast<real_t>(0.0001), p_duration);
}

real_t SplineNode::get_duration() const {
    return duration;
}

void SplineNode::set_speed_scale(real_t p_speed_scale) {
    speed_scale = p_speed_scale;
}

real_t SplineNode::get_speed_scale() const {
    return speed_scale;
}

void SplineNode::set_current_percent(real_t p_percent) {
    current_percent = std::clamp(p_percent, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    _evaluate_and_apply(true);
}

real_t SplineNode::get_current_percent() const {
    return current_percent;
}

void SplineNode::set_output_scale(real_t p_scale) {
    output_scale = p_scale;
    _evaluate_and_apply(true);
}

real_t SplineNode::get_output_scale() const {
    return output_scale;
}

void SplineNode::set_output_offset(real_t p_offset) {
    output_offset = p_offset;
    _evaluate_and_apply(true);
}

real_t SplineNode::get_output_offset() const {
    return output_offset;
}

void SplineNode::set_target_node_path(const godot::NodePath &p_path) {
    target_node_path = p_path;
    _evaluate_and_apply(false);
}

godot::NodePath SplineNode::get_target_node_path() const {
    return target_node_path;
}

void SplineNode::set_target_property(const godot::StringName &p_property) {
    target_property = p_property;
    _evaluate_and_apply(false);
}

godot::StringName SplineNode::get_target_property() const {
    return target_property;
}

void SplineNode::set_node_retargeter(const godot::Ref<NodeRetargeter> &p_retargeter) {
    node_retargeter = p_retargeter;
}

godot::Ref<NodeRetargeter> SplineNode::get_node_retargeter() const {
    return node_retargeter;
}

void SplineNode::set_signal_connector(const godot::Ref<SignalConnector> &p_connector) {
    signal_connector = p_connector;
}

godot::Ref<SignalConnector> SplineNode::get_signal_connector() const {
    return signal_connector;
}

// ============================================================================
// Playback Controls & Evaluation Helpers
// ============================================================================

void SplineNode::play(bool p_from_start) {
    if (p_from_start) {
        current_percent     = (speed_scale >= static_cast<real_t>(0.0)) ? static_cast<real_t>(0.0) : static_cast<real_t>(1.0);
        ping_pong_direction = static_cast<real_t>(1.0);
        last_section_index  = -1;
    }
    set_playing(true);
    _evaluate_and_apply(true);
}

void SplineNode::pause() {
    set_playing(false);
}

void SplineNode::stop(bool p_reset_to_start) {
    set_playing(false);
    if (p_reset_to_start) {
        current_percent     = static_cast<real_t>(0.0);
        ping_pong_direction = static_cast<real_t>(1.0);
        last_section_index  = -1;
        _evaluate_and_apply(true);
    }
}

void SplineNode::step(real_t p_delta_seconds) {
    if (duration <= static_cast<real_t>(0.0001)) return;

    const real_t delta_pct = (p_delta_seconds * speed_scale * ping_pong_direction) / duration;
    real_t next_pct = current_percent + delta_pct;

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
                looped = true;
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
                next_pct = static_cast<real_t>(2.0) - next_pct;
                ping_pong_direction = -ping_pong_direction;
                bounced = true;
            } else if (next_pct <= static_cast<real_t>(0.0)) {
                next_pct = -next_pct;
                ping_pong_direction = -ping_pong_direction;
                bounced = true;
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

real_t SplineNode::sample_at_percent(real_t p_percent) const {
    if (spline.is_null()) return output_offset;
    return (spline->sample(p_percent, use_weighted_sampling) * output_scale) + output_offset;
}

godot::PackedFloat32Array SplineNode::solve_percents_for_output_value(
    real_t p_target_output,
    int p_samples
) const {
    if (spline.is_null() || std::abs(output_scale) <= static_cast<real_t>(1e-6)) {
        return {};
    }
    const real_t unscaled_val = (p_target_output - output_offset) / output_scale;
    return spline->get_percents_at_value(unscaled_val, p_samples, use_weighted_sampling);
}

} // namespace ideam::godot_ext