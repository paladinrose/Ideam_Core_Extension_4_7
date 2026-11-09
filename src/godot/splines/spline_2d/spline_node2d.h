#pragma once

/**
 * spline_node2d.h
 * SceneTree 2D Spline Evaluator, Path Follower & Procedural Polygon Renderer (`godot::Node2D`).
 *
 * Extends the `SplineNode` protocol into 2D space:
 *   1. Shares `SplineProcessMode` (`Idle`, `Physics`, `Manual`) and `SplineLoopMode`
 *      (`Once`, `Wrap`, `Ping-Pong`) from `spline_node.h`.
 *   2. Evaluates 2D position, derivative, unit tangent, unit normal, and `Transform2D`
 *      along a `Ref<Spline2DResource>` (weighted or arc-length distance mode).
 *   3. Automatically drives a target `Node2D` (updating its `position` and optionally
 *      orienting its `rotation` along the curve tangent when `rotate_target_to_tangent` is true)
 *      or any custom `target_property` via `set_indexed`, with `NodeRetargeter` and
 *      `SignalConnector` support.
 *   4. Optional direct 2D viewport rendering (`draw_in_viewport`, `draw_closed_fill`,
 *      `spline_draw_color`, `fill_draw_color`, `spline_line_width`, `draw_subdivisions`).
 */

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "../spline_1d/spline_node.h"
#include "spline_2d_resource.h"
#include "../../utilities/node_retargeter.h"
#include "../../utilities/signal_connector.h"

namespace ideam::godot_ext {

class SplineNode2D : public godot::Node2D {
    GDCLASS(SplineNode2D, godot::Node2D)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<Spline2DResource> spline;

    // --- Playback & Parameter State ---
    SplineProcessMode process_mode = SPLINE_PROCESS_IDLE;
    SplineLoopMode loop_mode       = SPLINE_LOOP_ONCE;
    bool playing                   = false;
    bool autoplay                  = false;
    bool use_weighted_sampling     = false; // Default to uniform arc-length distance for 2D paths

    real_t duration                = static_cast<real_t>(1.0);
    real_t speed_scale             = static_cast<real_t>(1.0);
    real_t current_percent         = static_cast<real_t>(0.0);
    real_t ping_pong_direction     = static_cast<real_t>(1.0);

    // --- 2D Output Mapping, Orientation & Cached Evaluation ---
    godot::Vector2 output_scale    = godot::Vector2(1.0f, 1.0f);
    godot::Vector2 output_offset   = godot::Vector2(0.0f, 0.0f);
    bool rotate_target_to_tangent  = true;
    real_t rotation_offset_radians = static_cast<real_t>(0.0);

    godot::Vector2 current_position   = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 current_derivative = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 current_tangent    = godot::Vector2(1.0f, 0.0f);
    int last_section_index            = -1;

    // --- 2D Viewport Rendering Settings ---
    bool draw_in_viewport          = true;
    bool draw_closed_fill          = false;
    int draw_subdivisions          = 12;
    float spline_line_width        = 2.0f;
    godot::Color spline_draw_color = godot::Color(0.38f, 0.82f, 1.0f, 0.95f);
    godot::Color fill_draw_color   = godot::Color(0.24f, 0.56f, 0.88f, 0.25f);

    // --- SceneTree Target & Ecosystem Utilities ---
    godot::NodePath target_node_path;
    godot::StringName target_property;
    godot::Ref<NodeRetargeter> node_retargeter;
    godot::Ref<SignalConnector> signal_connector;

    void _on_spline_changed();
    void _update_process_callbacks();
    void _evaluate_and_apply(bool p_emit_signals = true);

public:
    SplineNode2D();
    ~SplineNode2D() override;

    void _ready() override;
    void _draw() override;
    void _process(double p_delta) override;
    void _physics_process(double p_delta) override;

    // ========================================================================
    // Resource & Playback Configuration
    // ========================================================================

    void set_spline(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_spline() const;

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

    void set_output_scale(const godot::Vector2 &p_scale);
    [[nodiscard]] godot::Vector2 get_output_scale() const;

    void set_output_offset(const godot::Vector2 &p_offset);
    [[nodiscard]] godot::Vector2 get_output_offset() const;

    void set_rotate_target_to_tangent(bool p_rotate);
    [[nodiscard]] bool get_rotate_target_to_tangent() const;

    void set_rotation_offset_radians(real_t p_offset_rad);
    [[nodiscard]] real_t get_rotation_offset_radians() const;

    [[nodiscard]] godot::Vector2 get_current_position() const { return current_position; }
    [[nodiscard]] godot::Vector2 get_current_derivative() const { return current_derivative; }
    [[nodiscard]] godot::Vector2 get_current_tangent() const { return current_tangent; }
    [[nodiscard]] real_t get_current_rotation() const;
    [[nodiscard]] int get_current_section_index() const { return last_section_index; }

    // ========================================================================
    // 2D Viewport Drawing Configuration
    // ========================================================================

    void set_draw_in_viewport(bool p_draw);
    [[nodiscard]] bool get_draw_in_viewport() const { return draw_in_viewport; }

    void set_draw_closed_fill(bool p_fill);
    [[nodiscard]] bool get_draw_closed_fill() const { return draw_closed_fill; }

    void set_draw_subdivisions(int p_subdivisions);
    [[nodiscard]] int get_draw_subdivisions() const { return draw_subdivisions; }

    void set_spline_line_width(float p_width);
    [[nodiscard]] float get_spline_line_width() const { return spline_line_width; }

    void set_spline_draw_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_spline_draw_color() const { return spline_draw_color; }

    void set_fill_draw_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_fill_draw_color() const { return fill_draw_color; }

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
    // Playback Controls & Spatial Helpers
    // ========================================================================

    void play(bool p_from_start = false);
    void pause();
    void stop(bool p_reset_to_start = true);
    void step(real_t p_delta_seconds);

    [[nodiscard]] godot::Vector2 sample_at_percent(real_t p_percent) const;
    [[nodiscard]] godot::Transform2D sample_transform_at_percent(real_t p_percent) const;
    [[nodiscard]] godot::Vector2 get_closest_point_to(const godot::Vector2 &p_local_pos, int p_samples = 10) const;
    [[nodiscard]] real_t get_closest_percent_to(const godot::Vector2 &p_local_pos, int p_samples = 10) const;
};

} // namespace ideam::godot_ext