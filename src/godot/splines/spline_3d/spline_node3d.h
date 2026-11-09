#pragma once

/**
 * spline_node_3d.h
 * SceneTree 3D Spline Evaluator, Path Follower & 3D Gizmo Host (`godot::Node3D`).
 *
 * Extends the `SplineNode` / `SplineNode2D` protocol into 3D space:
 *   1. Shares `SplineProcessMode` (`Idle`, `Physics`, `Manual`) and `SplineLoopMode`
 *      (`Once`, `Wrap`, `Ping-Pong`) from `spline_node.h`.
 *   2. Evaluates 3D position, derivative, tangent, and orthonormal 3D orientation
 *      (supporting both standard Up-Hint look-at and twist-free Rotation-Minimizing
 *      Bishop Parallel Transport frames + banking roll offset!).
 *   3. Automatically drives a target `Node3D` (`position` and `basis`) or any indexed
 *      `target_property` via `set_indexed`, with `NodeRetargeter` and `SignalConnector` support.
 *   4. Exposes dedicated **3D Editor Gizmo Hooks** (`selected_gizmo_point_index`,
 *      `selected_gizmo_section_index`, `update_gizmos()` triggers, and gizmo selection signals)
 *      ready for `Spline3DGizmo` (`EditorNode3DGizmoPlugin`).
 */

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "../spline_1d/spline_node.h"
#include "spline_3d_resource.h"
#include "../../utilities/node_retargeter.h"
#include "../../utilities/signal_connector.h"

namespace ideam::godot_ext {

enum Spline3DRotationMode : int32_t {
    SPLINE_3D_ROTATION_NONE            = 0, // Position only; leave target rotation untouched
    SPLINE_3D_ROTATION_UP_VECTOR       = 1, // Align -Z to tangent using a reference up_vector
    SPLINE_3D_ROTATION_BISHOP_TRANSPORT = 2 // Twist-free Wang Double-Reflection parallel transport
};

class SplineNode3D : public godot::Node3D {
    GDCLASS(SplineNode3D, godot::Node3D)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<Spline3DResource> spline;

    // --- Playback & Parameter State ---
    SplineProcessMode process_mode = SPLINE_PROCESS_IDLE;
    SplineLoopMode loop_mode       = SPLINE_LOOP_ONCE;
    bool playing                   = false;
    bool autoplay                  = false;
    bool use_weighted_sampling     = false; // Default to uniform arc-length distance for 3D paths

    real_t duration                = static_cast<real_t>(1.0);
    real_t speed_scale             = static_cast<real_t>(1.0);
    real_t current_percent         = static_cast<real_t>(0.0);
    real_t ping_pong_direction     = static_cast<real_t>(1.0);

    // --- 3D Output Mapping, Orientation & Cached Evaluation ---
    godot::Vector3 output_scale       = godot::Vector3(1.0f, 1.0f, 1.0f);
    godot::Vector3 output_offset      = godot::Vector3(0.0f, 0.0f, 0.0f);
    Spline3DRotationMode rotation_mode = SPLINE_3D_ROTATION_BISHOP_TRANSPORT;
    godot::Vector3 up_vector          = godot::Vector3(0.0f, 1.0f, 0.0f);
    real_t roll_offset_radians        = static_cast<real_t>(0.0);

    godot::Vector3 current_position   = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 current_derivative = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 current_tangent    = godot::Vector3(0.0f, 0.0f, -1.0f);
    godot::Basis current_basis;
    int last_section_index            = -1;

    // Cached twist-free Bishop frames for fast runtime lookup when `SPLINE_3D_ROTATION_BISHOP_TRANSPORT` is active
    mutable godot::TypedArray<godot::Transform3D> cached_bishop_transforms;
    mutable bool bishop_cache_dirty   = true;

    // --- 3D Editor Gizmo Selection & Display Hooks ---
    int selected_gizmo_point_index    = -1;
    int selected_gizmo_section_index  = -1;
    bool show_gizmo_frames            = false;
    int gizmo_subdivisions            = 12;

    // --- SceneTree Target & Ecosystem Utilities ---
    godot::NodePath target_node_path;
    godot::StringName target_property;
    godot::Ref<NodeRetargeter> node_retargeter;
    godot::Ref<SignalConnector> signal_connector;

    void _on_spline_changed();
    void _update_process_callbacks();
    void _ensure_bishop_cache() const;
    void _evaluate_and_apply(bool p_emit_signals = true);

public:
    SplineNode3D();
    ~SplineNode3D() override;

    void _ready() override;
    void _process(double p_delta) override;
    void _physics_process(double p_delta) override;

    // ========================================================================
    // Resource & Playback Configuration
    // ========================================================================

    void set_spline(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_spline() const;

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

    // ========================================================================
    // 3D Output Mapping & Orientation Configuration
    // ========================================================================

    void set_output_scale(const godot::Vector3 &p_scale);
    [[nodiscard]] godot::Vector3 get_output_scale() const;

    void set_output_offset(const godot::Vector3 &p_offset);
    [[nodiscard]] godot::Vector3 get_output_offset() const;

    void set_rotation_mode(Spline3DRotationMode p_mode);
    [[nodiscard]] Spline3DRotationMode get_rotation_mode() const;

    void set_up_vector(const godot::Vector3 &p_up);
    [[nodiscard]] godot::Vector3 get_up_vector() const;

    void set_roll_offset_radians(real_t p_roll_rad);
    [[nodiscard]] real_t get_roll_offset_radians() const;

    [[nodiscard]] godot::Vector3 get_current_position() const { return current_position; }
    [[nodiscard]] godot::Vector3 get_current_derivative() const { return current_derivative; }
    [[nodiscard]] godot::Vector3 get_current_tangent() const { return current_tangent; }
    [[nodiscard]] godot::Basis get_current_basis() const { return current_basis; }
    [[nodiscard]] godot::Transform3D get_current_transform_on_spline() const {
        return godot::Transform3D(current_basis, current_position);
    }
    [[nodiscard]] int get_current_section_index() const { return last_section_index; }

    // ========================================================================
    // 3D Gizmo Hooks (For Future `Spline3DGizmo` EditorNode3DGizmoPlugin)
    // ========================================================================

    void set_selected_gizmo_point_index(int p_index);
    [[nodiscard]] int get_selected_gizmo_point_index() const noexcept { return selected_gizmo_point_index; }

    void set_selected_gizmo_section_index(int p_index);
    [[nodiscard]] int get_selected_gizmo_section_index() const noexcept { return selected_gizmo_section_index; }

    void set_show_gizmo_frames(bool p_show);
    [[nodiscard]] bool get_show_gizmo_frames() const noexcept { return show_gizmo_frames; }

    void set_gizmo_subdivisions(int p_subdivisions);
    [[nodiscard]] int get_gizmo_subdivisions() const noexcept { return gizmo_subdivisions; }

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
    // Playback Controls & 3D Spatial Helpers
    // ========================================================================

    void play(bool p_from_start = false);
    void pause();
    void stop(bool p_reset_to_start = true);
    void step(real_t p_delta_seconds);

    [[nodiscard]] godot::Vector3 sample_at_percent(real_t p_percent) const;
    [[nodiscard]] godot::Transform3D sample_transform_at_percent(real_t p_percent) const;
    [[nodiscard]] godot::Vector3 get_closest_point_to(const godot::Vector3 &p_local_pos, int p_samples = 10) const;
    [[nodiscard]] real_t get_closest_percent_to(const godot::Vector3 &p_local_pos, int p_samples = 10) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::Spline3DRotationMode);