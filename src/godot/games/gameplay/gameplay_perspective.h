#pragma once

#include "gameplay_resource.h"

#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/spring_arm3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

/**
 * DOD NOTE: Extracting perspective enums at namespace scope with explicit int32_t
 * backing allows them to be stored in flat Structure of Arrays (SoA) configuration
 * columns without pulling in the full GameplayPerspective class definition.
 */
enum PerspectiveType : int32_t {
    PERSPECTIVE_CUSTOM = 0,
    PERSPECTIVE_FIRST_PERSON = 1,       // Eye-level, zero arm length, direct look
    PERSPECTIVE_THIRD_PERSON_OTS = 2,   // Over-the-Shoulder (lateral offset + aim zoom)
    PERSPECTIVE_THIRD_PERSON_ORBIT = 3, // Centered third-person follow / adventure orbit
    PERSPECTIVE_TOP_DOWN = 4,           // Directly overhead (-90 deg pitch)
    PERSPECTIVE_THREE_QUARTER = 5,      // 3/4 View angled overhead (~-50 deg pitch)
    PERSPECTIVE_ISOMETRIC = 6,          // True isometric (-35.264 deg pitch, 45 deg yaw, orthogonal)
    PERSPECTIVE_DIMETRIC = 7,           // 2:1 tactical dimetric (-30 deg pitch, 45 deg yaw, orthogonal)
    PERSPECTIVE_SIDE_SCROLLER = 8,      // 2.5D / side-view locked depth plane
    PERSPECTIVE_FIXED_CINEMATIC = 9     // Fixed world/room camera (classic survival horror angles)
};

enum PerspectiveProjectionMode : int32_t {
    PROJECTION_PERSPECTIVE = 0,
    PROJECTION_ORTHOGONAL = 1,
    PROJECTION_FRUSTUM = 2
};

enum ShoulderSide : int32_t {
    SHOULDER_CENTER = 0,
    SHOULDER_RIGHT = 1,
    SHOULDER_LEFT = 2
};

enum MovementAlignmentMode : int32_t {
    MOVEMENT_CAMERA_RELATIVE = 0,    // Directional input moves relative to camera yaw
    MOVEMENT_CHARACTER_RELATIVE = 1, // Tank / character-relative controls
    MOVEMENT_WORLD_AXIS_ALIGNED = 2, // Fixed world/screen-axis movement (isometric, top-down, 2.5D)
    MOVEMENT_CURSOR_DIRECTED = 3     // Twin-stick / cursor-targeted orientation
};

/**
 * GameplayPerspective encapsulates camera projection, spatial rig offsets,
 * spring-arm collision parameters, angular constraints, smoothing, look sensitivity,
 * and player movement/visibility alignment across standard game camera archetypes
 * (First Person, Third Person Over the Shoulder, Third Person Orbit, Top Down,
 * 3/4 View, Isometric, Dimetric, Side Scroller, and Fixed Cinematic).
 *
 * Beyond direct application to Godot 3D/2D camera rigs (`apply_to_camera_rig`,
 * `apply_to_camera_2d`) and smooth runtime blending (`interpolate_with`),
 * GameplayPerspective integrates with the `GameplayStyle` cascade via `collect_rules()`,
 * emitting declarative `GameplayRule` instances for player/entity properties.
 */
class GameplayPerspective : public GameplayResource {
    GDCLASS(GameplayPerspective, GameplayResource)

protected:
    static void _bind_methods();

private:
    // --- 0. Preset & Projection ---
    PerspectiveType perspective_type = PerspectiveType::PERSPECTIVE_THIRD_PERSON_OTS;
    PerspectiveProjectionMode projection_mode = PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;
    float fov = 75.0f;
    float aim_fov = 50.0f;
    float orthogonal_size = 12.0f;
    float aim_orthogonal_size = 9.0f;
    float near_clip = 0.05f;
    float far_clip = 4000.0f;
    godot::Vector2 frustum_offset = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 viewport_zoom_2d = godot::Vector2(1.0f, 1.0f);

    // --- 1. Rig & Spatial Offsets ---
    godot::Vector3 pivot_offset = godot::Vector3(0.0f, 1.6f, 0.0f);
    godot::Vector3 camera_local_offset = godot::Vector3(0.45f, 0.15f, 0.0f);
    godot::Vector3 aim_camera_local_offset = godot::Vector3(0.55f, 0.1f, 0.0f);
    ShoulderSide shoulder_side = ShoulderSide::SHOULDER_RIGHT;
    bool allow_shoulder_swap = true;

    float arm_length = 2.5f;
    float aim_arm_length = 1.35f;
    float min_arm_length = 0.5f;
    float max_arm_length = 6.0f;

    bool spring_arm_collision_enabled = true;
    uint32_t spring_arm_collision_mask = 1;
    float spring_arm_margin = 0.2f;

    // --- 2. Orientation & Angular Constraints ---
    float default_pitch_degrees = -10.0f;
    float default_yaw_degrees = 0.0f;
    float default_roll_degrees = 0.0f;

    float min_pitch_degrees = -80.0f;
    float max_pitch_degrees = 75.0f;
    bool lock_pitch = false;

    bool lock_yaw = false;
    bool yaw_limits_enabled = false;
    float min_yaw_degrees = -180.0f;
    float max_yaw_degrees = 180.0f;
    float yaw_snap_step_degrees = 0.0f; // e.g., 45.0 or 90.0 for stepped isometric rotation

    // --- 3. Smoothing, Transitions & Look Control ---
    float position_follow_smoothing = 12.0f;
    float rotation_smoothing = 15.0f;
    float fov_transition_speed = 10.0f;
    float perspective_blend_duration = 0.35f;

    float look_sensitivity_x = 1.0f;
    float look_sensitivity_y = 1.0f;
    float aim_sensitivity_multiplier = 0.6f;
    bool invert_look_x = false;
    bool invert_look_y = false;

    // --- 4. Player Alignment & Visibility ---
    MovementAlignmentMode movement_alignment = MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE;
    bool orient_player_to_camera = false;
    bool orient_player_to_movement = true;
    bool orient_player_on_aim = true;
    bool player_mesh_visible = true;
    float first_person_hide_mesh_distance = 0.35f;
    bool show_aim_reticle = true;
    bool occlude_environment_between_camera_and_player = false;
    godot::StringName player_group_tag = godot::StringName("Player");

    // Internal helper to synchronize semantic option_tags with the active perspective preset
    void _sync_perspective_option_tags();

public:
    GameplayPerspective();
    ~GameplayPerspective() override;

    // --- 0. Preset & Projection Setters / Getters ---
    void set_perspective_type(PerspectiveType p_type);
    PerspectiveType get_perspective_type() const;
    void apply_perspective_preset(PerspectiveType p_type);

    void set_projection_mode(PerspectiveProjectionMode p_mode);
    PerspectiveProjectionMode get_projection_mode() const;

    void set_fov(float p_fov);
    float get_fov() const;

    void set_aim_fov(float p_fov);
    float get_aim_fov() const;

    void set_orthogonal_size(float p_size);
    float get_orthogonal_size() const;

    void set_aim_orthogonal_size(float p_size);
    float get_aim_orthogonal_size() const;

    void set_near_clip(float p_near);
    float get_near_clip() const;

    void set_far_clip(float p_far);
    float get_far_clip() const;

    void set_frustum_offset(const godot::Vector2 &p_offset);
    godot::Vector2 get_frustum_offset() const;

    void set_viewport_zoom_2d(const godot::Vector2 &p_zoom);
    godot::Vector2 get_viewport_zoom_2d() const;

    // --- 1. Rig & Spatial Offsets Setters / Getters ---
    void set_pivot_offset(const godot::Vector3 &p_offset);
    godot::Vector3 get_pivot_offset() const;

    void set_camera_local_offset(const godot::Vector3 &p_offset);
    godot::Vector3 get_camera_local_offset() const;

    void set_aim_camera_local_offset(const godot::Vector3 &p_offset);
    godot::Vector3 get_aim_camera_local_offset() const;

    void set_shoulder_side(ShoulderSide p_side);
    ShoulderSide get_shoulder_side() const;
    void swap_shoulder();

    void set_allow_shoulder_swap(bool p_allow);
    bool get_allow_shoulder_swap() const;

    void set_arm_length(float p_length);
    float get_arm_length() const;

    void set_aim_arm_length(float p_length);
    float get_aim_arm_length() const;

    void set_min_arm_length(float p_min);
    float get_min_arm_length() const;

    void set_max_arm_length(float p_max);
    float get_max_arm_length() const;

    void set_spring_arm_collision_enabled(bool p_enabled);
    bool get_spring_arm_collision_enabled() const;

    void set_spring_arm_collision_mask(uint32_t p_mask);
    uint32_t get_spring_arm_collision_mask() const;

    void set_spring_arm_margin(float p_margin);
    float get_spring_arm_margin() const;

    // --- 2. Orientation & Angular Constraints Setters / Getters ---
    void set_default_pitch_degrees(float p_pitch);
    float get_default_pitch_degrees() const;

    void set_default_yaw_degrees(float p_yaw);
    float get_default_yaw_degrees() const;

    void set_default_roll_degrees(float p_roll);
    float get_default_roll_degrees() const;

    void set_min_pitch_degrees(float p_min_pitch);
    float get_min_pitch_degrees() const;

    void set_max_pitch_degrees(float p_max_pitch);
    float get_max_pitch_degrees() const;

    void set_lock_pitch(bool p_lock);
    bool get_lock_pitch() const;

    void set_lock_yaw(bool p_lock);
    bool get_lock_yaw() const;

    void set_yaw_limits_enabled(bool p_enabled);
    bool get_yaw_limits_enabled() const;

    void set_min_yaw_degrees(float p_min_yaw);
    float get_min_yaw_degrees() const;

    void set_max_yaw_degrees(float p_max_yaw);
    float get_max_yaw_degrees() const;

    void set_yaw_snap_step_degrees(float p_step);
    float get_yaw_snap_step_degrees() const;

    // --- 3. Smoothing, Transitions & Look Control Setters / Getters ---
    void set_position_follow_smoothing(float p_smoothing);
    float get_position_follow_smoothing() const;

    void set_rotation_smoothing(float p_smoothing);
    float get_rotation_smoothing() const;

    void set_fov_transition_speed(float p_speed);
    float get_fov_transition_speed() const;

    void set_perspective_blend_duration(float p_duration);
    float get_perspective_blend_duration() const;

    void set_look_sensitivity_x(float p_sens);
    float get_look_sensitivity_x() const;

    void set_look_sensitivity_y(float p_sens);
    float get_look_sensitivity_y() const;

    void set_aim_sensitivity_multiplier(float p_mult);
    float get_aim_sensitivity_multiplier() const;

    void set_invert_look_x(bool p_invert);
    bool get_invert_look_x() const;

    void set_invert_look_y(bool p_invert);
    bool get_invert_look_y() const;

    // --- 4. Player Alignment & Visibility Setters / Getters ---
    void set_movement_alignment(MovementAlignmentMode p_mode);
    MovementAlignmentMode get_movement_alignment() const;

    void set_orient_player_to_camera(bool p_orient);
    bool get_orient_player_to_camera() const;

    void set_orient_player_to_movement(bool p_orient);
    bool get_orient_player_to_movement() const;

    void set_orient_player_on_aim(bool p_orient);
    bool get_orient_player_on_aim() const;

    void set_player_mesh_visible(bool p_visible);
    bool get_player_mesh_visible() const;

    void set_first_person_hide_mesh_distance(float p_distance);
    float get_first_person_hide_mesh_distance() const;

    void set_show_aim_reticle(bool p_show);
    bool get_show_aim_reticle() const;

    void set_occlude_environment_between_camera_and_player(bool p_occlude);
    bool get_occlude_environment_between_camera_and_player() const;

    void set_player_group_tag(const godot::StringName &p_tag);
    godot::StringName get_player_group_tag() const;

    // --- Computed Rig Queries & Blending Helpers ---
    godot::Vector3 get_effective_camera_offset(bool p_is_aiming = false) const;
    float get_effective_fov(bool p_is_aiming = false) const;
    float get_effective_orthogonal_size(bool p_is_aiming = false) const;
    float get_effective_arm_length(bool p_is_aiming = false) const;
    godot::Vector2 get_effective_look_sensitivity(bool p_is_aiming = false) const;
    float clamp_pitch_degrees(float p_pitch_deg) const;
    float clamp_yaw_degrees(float p_yaw_deg) const;

    /**
     * Computes a world-space Transform3D for a camera given a target origin and look angles.
     */
    godot::Transform3D compute_camera_rig_transform(
        const godot::Vector3 &p_target_origin,
        float p_yaw_deg,
        float p_pitch_deg,
        bool p_is_aiming = false
    ) const;

    /**
     * Produces a new GameplayPerspective blended between this instance and `p_target` by `p_weight` (0.0 .. 1.0).
     */
    godot::Ref<GameplayPerspective> interpolate_with(
        const godot::Ref<GameplayPerspective> &p_target,
        float p_weight
    ) const;

    // --- Scene & Resource Application ---
    void apply_perspective(const godot::Ref<GameplayPerspective> &p_other);

    /**
     * Applies projection, FOV/orthogonal size, clip planes, pivot offset, spring arm length/mask,
     * and local camera offset directly to a 3D camera rig.
     */
    void apply_to_camera_rig(
        godot::Node3D *p_pivot,
        godot::SpringArm3D *p_spring_arm,
        godot::Camera3D *p_camera,
        bool p_is_aiming = false
    ) const;

    /**
     * Applies 2D zoom, offset, and smoothing settings to a Camera2D node.
     */
    void apply_to_camera_2d(godot::Camera2D *p_camera_2d, bool p_is_aiming = false) const;

    virtual godot::TypedArray<GameplayRule> collect_rules() const override;
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::PerspectiveType);
VARIANT_ENUM_CAST(ideam::godot_ext::PerspectiveProjectionMode);
VARIANT_ENUM_CAST(ideam::godot_ext::ShoulderSide);
VARIANT_ENUM_CAST(ideam::godot_ext::MovementAlignmentMode);