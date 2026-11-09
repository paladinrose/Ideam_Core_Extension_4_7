#pragma once

/**
 * spline_node.h
 * SceneTree 1D Spline Evaluator & Procedural Property Driver Node.
 *
 * Consumes a `Ref<Spline1DResource>` and provides:
 *   1. Time-based or manual parameter playback (`PROCESS_IDLE`, `PROCESS_PHYSICS`, `PROCESS_MANUAL`)
 *      with `LOOP_ONCE`, `LOOP_WRAP`, and `LOOP_PING_PONG` modes.
 *   2. Weighted vs. Arc-Length distance sampling (`use_weighted_sampling`) and output scaling/offsetting.
 *   3. Automatic SceneTree property driving (`target_node_path` + `target_property` via `set_indexed`,
 *      supporting sub-properties such as `"position:y"` or `"modulate:a"`), plus optional
 *      `NodeRetargeter` and `SignalConnector` integration.
 *   4. Reversible 1D root-finding (`solve_percents_for_output_value`) and section-crossing signals.
 */

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include "spline_1d_resource.h"
#include "../../utilities/node_retargeter.h"
#include "../../utilities/signal_connector.h"

namespace ideam::godot_ext {

enum SplineProcessMode : int32_t {
    SPLINE_PROCESS_IDLE    = 0,
    SPLINE_PROCESS_PHYSICS = 1,
    SPLINE_PROCESS_MANUAL  = 2
};

enum SplineLoopMode : int32_t {
    SPLINE_LOOP_ONCE      = 0,
    SPLINE_LOOP_WRAP      = 1,
    SPLINE_LOOP_PING_PONG = 2
};

class SplineNode : public godot::Node {
    GDCLASS(SplineNode, godot::Node)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<Spline1DResource> spline;

    // --- Playback & Parameter State ---
    SplineProcessMode process_mode = SPLINE_PROCESS_IDLE;
    SplineLoopMode loop_mode       = SPLINE_LOOP_ONCE;
    bool playing                   = false;
    bool autoplay                  = false;
    bool use_weighted_sampling     = true;

    real_t duration                = static_cast<real_t>(1.0);
    real_t speed_scale             = static_cast<real_t>(1.0);
    real_t current_percent         = static_cast<real_t>(0.0);
    real_t ping_pong_direction     = static_cast<real_t>(1.0);

    // --- Output Mapping & Cached Evaluation ---
    real_t output_scale            = static_cast<real_t>(1.0);
    real_t output_offset           = static_cast<real_t>(0.0);
    real_t current_value           = static_cast<real_t>(0.0);
    real_t current_derivative      = static_cast<real_t>(0.0);
    int last_section_index         = -1;

    // --- SceneTree Property Target & Ecosystem Utilities ---
    godot::NodePath target_node_path;
    godot::StringName target_property;
    godot::Ref<NodeRetargeter> node_retargeter;
    godot::Ref<SignalConnector> signal_connector;

    void _on_spline_changed();
    void _update_process_callbacks();
    void _evaluate_and_apply(bool p_emit_signals = true);

public:
    SplineNode();
    ~SplineNode() override;

    void _ready() override;
    void _process(double p_delta) override;
    void _physics_process(double p_delta) override;

    // ========================================================================
    // Resource & Playback Configuration
    // ========================================================================

    void set_spline(const godot::Ref<Spline1DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline1DResource> get_spline() const;

    void set_process_mode_type(SplineProcessMode p_mode);
    [[nodiscard]] SplineProcessMode get_process_mode_type() const;

    void set_loop_mode(SplineLoopMode p_mode);
    [[nodiscard]] SplineLoopMode get_loop_mode() const;

    void set_playing(bool p_playing);
    [[nodiscard]] bool is_playing() const;

    void set_autoplay(bool p_autoplay);
    [[nodiscard]] bool get_autoplay() const;

    void set_use_weighted_sampling(bool p_weighted);
    [[nodiscard]] bool get_use_weighted_sampling() const;

    void set_duration(real_t p_duration);
    [[nodiscard]] real_t get_duration() const;

    void set_speed_scale(real_t p_speed_scale);
    [[nodiscard]] real_t get_speed_scale() const;

    void set_current_percent(real_t p_percent);
    [[nodiscard]] real_t get_current_percent() const;

    void set_output_scale(real_t p_scale);
    [[nodiscard]] real_t get_output_scale() const;

    void set_output_offset(real_t p_offset);
    [[nodiscard]] real_t get_output_offset() const;

    [[nodiscard]] real_t get_current_value() const { return current_value; }
    [[nodiscard]] real_t get_current_derivative() const { return current_derivative; }
    [[nodiscard]] int get_current_section_index() const { return last_section_index; }

    // ========================================================================
    // Target Property & Utility Wiring
    // ========================================================================

    void set_target_node_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_target_node_path() const;

    void set_target_property(const godot::StringName &p_property);
    [[nodiscard]] godot::StringName get_target_property() const;

    void set_node_retargeter(const godot::Ref<NodeRetargeter> &p_retargeter);
    [[nodiscard]] godot::Ref<NodeRetargeter> get_node_retargeter() const;

    void set_signal_connector(const godot::Ref<SignalConnector> &p_connector);
    [[nodiscard]] godot::Ref<SignalConnector> get_signal_connector() const;

    // ========================================================================
    // Playback Controls & Evaluation Helpers
    // ========================================================================

    void play(bool p_from_start = false);
    void pause();
    void stop(bool p_reset_to_start = true);
    void step(real_t p_delta_seconds);

    [[nodiscard]] real_t sample_at_percent(real_t p_percent) const;

    /**
     * Inverse lookup accounting for `output_scale` and `output_offset`:
     * solves for all percentages `t` in [0, 1] where `sample(t) * output_scale + output_offset == p_target_output`.
     */
    [[nodiscard]] godot::PackedFloat32Array solve_percents_for_output_value(
        real_t p_target_output,
        int p_samples = 10
    ) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplineProcessMode);
VARIANT_ENUM_CAST(ideam::godot_ext::SplineLoopMode);