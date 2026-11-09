#pragma once

/**
 * spline_node4d.h
 * SceneTree 4D Spline Evaluator, 3D+W Path Follower, Color Gradient Driver & 4D Gizmo Host (`godot::Node3D`).
 *
 * Extends the `SplineNode` / `SplineNode3D` protocol into 4D space:
 *   1. Shares `SplineProcessMode`, `SplineLoopMode`, and `Spline3DRotationMode`.
 *   2. Evaluates `Vector4(X, Y, Z, W)`, `Color(R, G, B, A)`, and 3D `Transform3D` along a
 *      `Ref<Spline4DResource>`.
 *   3. Configurable `Spline4DWChannelMode` determines how the 4th component (`W`) is applied:
 *        - `SPLINE_4D_W_UNIFORM_SCALE`: `XYZ` drives 3D position/rotation; `W` scales the target `Node3D` uniformly.
 *        - `SPLINE_4D_W_BANKING_ROLL`: `XYZ` drives 3D position/rotation; `W` (radians) rolls the target around its forward tangent.
 *        - `SPLINE_4D_W_SECONDARY_PROPERTY`: `XYZ` drives the target `Node3D`; `W` drives `target_w_property` (e.g. light energy, FOV).
 *        - `SPLINE_4D_W_RAW_VECTOR4`: Drives `target_property` directly with the evaluated `Vector4`.
 *        - `SPLINE_4D_W_COLOR_RGBA`: Drives `target_property` (e.g. `"modulate"`, `"albedo_color"`) with the evaluated `Color`.
 *   4. Exposes **4D Editor Gizmo Hooks** (`selected_gizmo_point_index`, `selected_gizmo_section_index`,
 *      `show_gizmo_frames`, `gizmo_w_is_roll`, `update_gizmos()`) ready for `Spline4DGizmo`.
 */

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include "../spline_1d/spline_node.h"
#include "../spline_3d/spline_node3d.h"
#include "spline_4d_resource.h"
#include "../../utilities/node_retargeter.h"
#include "../../utilities/signal_connector.h"

namespace ideam::godot_ext {

enum Spline4DWChannelMode : int32_t {
    SPLINE_4D_W_UNIFORM_SCALE      = 0, // W scales the target Node3D uniformly
    SPLINE_4D_W_BANKING_ROLL       = 1, // W (radians) rolls the target Node3D around its forward tangent
    SPLINE_4D_W_SECONDARY_PROPERTY = 2, // XYZ moves target Node3D; W writes to target_w_property
    SPLINE_4D_W_RAW_VECTOR4        = 3, // Writes full Vector4(x,y,z,w) to target_property
    SPLINE_4D_W_COLOR_RGBA         = 4  // Writes evaluated Color(r,g,b,a) to target_property
};

class SplineNode4D : public godot::Node3D {
    GDCLASS(SplineNode4D, godot::Node3D)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::Ref<Spline4DResource> spline;

    // --- Playback & Parameter State ---
    SplineProcessMode process_mode = SPLINE_PROCESS_IDLE;
    SplineLoopMode loop_mode       = SPLINE_LOOP_ONCE;
    bool playing                   = false;
    bool autoplay                  = false;
    bool use_weighted_sampling     = false;

    real_t duration                = static_cast<real_t>(1.0);
    real_t speed_scale             = static_cast<real_t>(1.0);
    real_t current_percent         = static_cast<real_t>(0.0);
    real_t ping_pong_direction     = static_cast<real_t>(1.0);

    // --- 4D Output Mapping, W-Channel Routing & Cached Evaluation ---
    godot::Vector4 output_scale         = godot::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    godot::Vector4 output_offset        = godot::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    Spline3DRotationMode rotation_mode  = SPLINE_3D_ROTATION_BISHOP_TRANSPORT;
    Spline4DWChannelMode w_channel_mode = SPLINE_4D_W_UNIFORM_SCALE;
    godot::Vector3 up_vector            = godot::Vector3(0.0f, 1.0f, 0.0f);
    bool hsva_color_mode                = false;

    godot::Vector4 current_value_4d   = godot::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    godot::Vector4 current_derivative = godot::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Basis current_basis;
    int last_section_index            = -1;

    mutable godot::TypedArray<godot::Transform3D> cached_bishop_transforms;
    mutable bool bishop_cache_dirty   = true;

    // --- 4D Editor Gizmo Selection & Display Hooks ---
    int selected_gizmo_point_index    = -1;
    int selected_gizmo_section_index  = -1;
    bool show_gizmo_frames            = false;
    int gizmo_subdivisions            = 12;

    // --- SceneTree Target & Ecosystem Utilities ---
    godot::NodePath target_node_path;
    godot::StringName target_property;
    godot::StringName target_w_property;
    godot::Ref<NodeRetargeter> node_retargeter;
    godot::Ref<SignalConnector> signal_connector;

    void _on_spline_changed();
    void _update_process_callbacks();
    void _ensure_bishop_cache() const;
    void _evaluate_and_apply(bool p_emit_signals = true);

public:
    SplineNode4D();
    ~SplineNode4D() override;

    void _ready() override;
    void _process(double p_delta) override;
    void _physics_process(double p_delta) override;

    // ========================================================================
    // Resource & Playback Configuration
    // ========================================================================

    void set_spline(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_spline() const;

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
    // 4D Output Mapping, W-Channel Mode & Orientation Configuration
    // ========================================================================

    void set_output_scale(const godot::Vector4 &p_scale);
    [[nodiscard]] godot::Vector4 get_output_scale() const;

    void set_output_offset(const godot::Vector4 &p_offset);
    [[nodiscard]] godot::Vector4 get_output_offset() const;

    void set_rotation_mode(Spline3DRotationMode p_mode);
    [[nodiscard]] Spline3DRotationMode get_rotation_mode() const;

    void set_w_channel_mode(Spline4DWChannelMode p_mode);
    [[nodiscard]] Spline4DWChannelMode get_w_channel_mode() const;

    void set_up_vector(const godot::Vector3 &p_up);
    [[nodiscard]] godot::Vector3 get_up_vector() const;

    void set_hsva_color_mode(bool p_hsva);
    [[nodiscard]] bool get_hsva_color_mode() const { return hsva_color_mode; }

    [[nodiscard]] godot::Vector4 get_current_value_4d() const { return current_value_4d; }
    [[nodiscard]] godot::Vector3 get_current_position_3d() const {
        return godot::Vector3(current_value_4d.x, current_value_4d.y, current_value_4d.z);
    }
    [[nodiscard]] real_t get_current_w() const { return static_cast<real_t>(current_value_4d.w); }
    [[nodiscard]] godot::Color get_current_color() const;
    [[nodiscard]] godot::Vector4 get_current_derivative() const { return current_derivative; }
    [[nodiscard]] godot::Basis get_current_basis() const { return current_basis; }
    [[nodiscard]] int get_current_section_index() const { return last_section_index; }

    // ========================================================================
    // 4D Gizmo Hooks (For Future `Spline4DGizmo` EditorNode3DGizmoPlugin)
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

    void set_target_w_property(const godot::StringName &p_property);
    [[nodiscard]] godot::StringName get_target_w_property() const;

    void set_node_retargeter(const godot::Ref<NodeRetargeter> &p_retargeter);
    [[nodiscard]] godot::Ref<NodeRetargeter> get_node_retargeter() const;

    void set_signal_connector(const godot::Ref<SignalConnector> &p_connector);
    [[nodiscard]] godot::Ref<SignalConnector> get_signal_connector() const;

    // ========================================================================
    // Playback Controls & 4D Evaluation Helpers
    // ========================================================================

    void play(bool p_from_start = false);
    void pause();
    void stop(bool p_reset_to_start = true);
    void step(real_t p_delta_seconds);

    [[nodiscard]] godot::Vector4 sample_at_percent(real_t p_percent) const;
    [[nodiscard]] godot::Transform3D sample_transform_3d_at_percent(real_t p_percent) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::Spline4DWChannelMode);