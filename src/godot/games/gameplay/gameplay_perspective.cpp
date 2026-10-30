#include "gameplay_perspective.h"

#include <cmath>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/basis.hpp>

#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayPerspective::_bind_methods() {
    // --- Enums ---
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_CUSTOM);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_FIRST_PERSON);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_THIRD_PERSON_OTS);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_THIRD_PERSON_ORBIT);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_TOP_DOWN);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_THREE_QUARTER);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_ISOMETRIC);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_DIMETRIC);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_SIDE_SCROLLER);
    BIND_ENUM_CONSTANT(PerspectiveType::PERSPECTIVE_FIXED_CINEMATIC);

    BIND_ENUM_CONSTANT(PerspectiveProjectionMode::PROJECTION_PERSPECTIVE);
    BIND_ENUM_CONSTANT(PerspectiveProjectionMode::PROJECTION_ORTHOGONAL);
    BIND_ENUM_CONSTANT(PerspectiveProjectionMode::PROJECTION_FRUSTUM);

    BIND_ENUM_CONSTANT(ShoulderSide::SHOULDER_CENTER);
    BIND_ENUM_CONSTANT(ShoulderSide::SHOULDER_RIGHT);
    BIND_ENUM_CONSTANT(ShoulderSide::SHOULDER_LEFT);

    BIND_ENUM_CONSTANT(MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE);
    BIND_ENUM_CONSTANT(MovementAlignmentMode::MOVEMENT_CHARACTER_RELATIVE);
    BIND_ENUM_CONSTANT(MovementAlignmentMode::MOVEMENT_WORLD_AXIS_ALIGNED);
    BIND_ENUM_CONSTANT(MovementAlignmentMode::MOVEMENT_CURSOR_DIRECTED);

    // --- 0. Preset & Projection ---
    ADD_GROUP("Preset & Projection", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_perspective_type", "type"), &GameplayPerspective::set_perspective_type);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspective_type"), &GameplayPerspective::get_perspective_type);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_perspective_preset", "type"), &GameplayPerspective::apply_perspective_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "perspective_type",
            godot::PROPERTY_HINT_ENUM,
            "Custom,First Person,Third Person Over the Shoulder,Third Person Orbit,Top Down,3/4 View,Isometric,Dimetric,Side Scroller,Fixed Cinematic"
        ),
        "set_perspective_type",
        "get_perspective_type"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_projection_mode", "mode"), &GameplayPerspective::set_projection_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_projection_mode"), &GameplayPerspective::get_projection_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "projection_mode", godot::PROPERTY_HINT_ENUM, "Perspective,Orthogonal,Frustum"),
        "set_projection_mode",
        "get_projection_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_fov", "fov"), &GameplayPerspective::set_fov);
    godot::ClassDB::bind_method(godot::D_METHOD("get_fov"), &GameplayPerspective::get_fov);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "fov", godot::PROPERTY_HINT_RANGE, "20.0,140.0,0.5"),
        "set_fov",
        "get_fov"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_aim_fov", "fov"), &GameplayPerspective::set_aim_fov);
    godot::ClassDB::bind_method(godot::D_METHOD("get_aim_fov"), &GameplayPerspective::get_aim_fov);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "aim_fov", godot::PROPERTY_HINT_RANGE, "15.0,120.0,0.5"),
        "set_aim_fov",
        "get_aim_fov"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_orthogonal_size", "size"), &GameplayPerspective::set_orthogonal_size);
    godot::ClassDB::bind_method(godot::D_METHOD("get_orthogonal_size"), &GameplayPerspective::get_orthogonal_size);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "orthogonal_size", godot::PROPERTY_HINT_RANGE, "0.5,200.0,0.5"),
        "set_orthogonal_size",
        "get_orthogonal_size"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_aim_orthogonal_size", "size"), &GameplayPerspective::set_aim_orthogonal_size);
    godot::ClassDB::bind_method(godot::D_METHOD("get_aim_orthogonal_size"), &GameplayPerspective::get_aim_orthogonal_size);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "aim_orthogonal_size", godot::PROPERTY_HINT_RANGE, "0.5,200.0,0.5"),
        "set_aim_orthogonal_size",
        "get_aim_orthogonal_size"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_near_clip", "near_clip"), &GameplayPerspective::set_near_clip);
    godot::ClassDB::bind_method(godot::D_METHOD("get_near_clip"), &GameplayPerspective::get_near_clip);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "near_clip", godot::PROPERTY_HINT_RANGE, "0.005,10.0,0.005"),
        "set_near_clip",
        "get_near_clip"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_far_clip", "far_clip"), &GameplayPerspective::set_far_clip);
    godot::ClassDB::bind_method(godot::D_METHOD("get_far_clip"), &GameplayPerspective::get_far_clip);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "far_clip", godot::PROPERTY_HINT_RANGE, "10.0,25000.0,10.0"),
        "set_far_clip",
        "get_far_clip"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_frustum_offset", "offset"), &GameplayPerspective::set_frustum_offset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_frustum_offset"), &GameplayPerspective::get_frustum_offset);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR2, "frustum_offset"), "set_frustum_offset", "get_frustum_offset");

    godot::ClassDB::bind_method(godot::D_METHOD("set_viewport_zoom_2d", "zoom"), &GameplayPerspective::set_viewport_zoom_2d);
    godot::ClassDB::bind_method(godot::D_METHOD("get_viewport_zoom_2d"), &GameplayPerspective::get_viewport_zoom_2d);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR2, "viewport_zoom_2d"), "set_viewport_zoom_2d", "get_viewport_zoom_2d");

    // --- 1. Rig & Spatial Offsets ---
    ADD_GROUP("Rig & Spatial Offsets", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_pivot_offset", "offset"), &GameplayPerspective::set_pivot_offset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_pivot_offset"), &GameplayPerspective::get_pivot_offset);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR3, "pivot_offset"), "set_pivot_offset", "get_pivot_offset");

    godot::ClassDB::bind_method(godot::D_METHOD("set_camera_local_offset", "offset"), &GameplayPerspective::set_camera_local_offset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_camera_local_offset"), &GameplayPerspective::get_camera_local_offset);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR3, "camera_local_offset"), "set_camera_local_offset", "get_camera_local_offset");

    godot::ClassDB::bind_method(godot::D_METHOD("set_aim_camera_local_offset", "offset"), &GameplayPerspective::set_aim_camera_local_offset);
    godot::ClassDB::bind_method(godot::D_METHOD("get_aim_camera_local_offset"), &GameplayPerspective::get_aim_camera_local_offset);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR3, "aim_camera_local_offset"), "set_aim_camera_local_offset", "get_aim_camera_local_offset");

    godot::ClassDB::bind_method(godot::D_METHOD("set_shoulder_side", "side"), &GameplayPerspective::set_shoulder_side);
    godot::ClassDB::bind_method(godot::D_METHOD("get_shoulder_side"), &GameplayPerspective::get_shoulder_side);
    godot::ClassDB::bind_method(godot::D_METHOD("swap_shoulder"), &GameplayPerspective::swap_shoulder);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "shoulder_side", godot::PROPERTY_HINT_ENUM, "Center,Right,Left"),
        "set_shoulder_side",
        "get_shoulder_side"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_allow_shoulder_swap", "allow"), &GameplayPerspective::set_allow_shoulder_swap);
    godot::ClassDB::bind_method(godot::D_METHOD("get_allow_shoulder_swap"), &GameplayPerspective::get_allow_shoulder_swap);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "allow_shoulder_swap"), "set_allow_shoulder_swap", "get_allow_shoulder_swap");

    godot::ClassDB::bind_method(godot::D_METHOD("set_arm_length", "length"), &GameplayPerspective::set_arm_length);
    godot::ClassDB::bind_method(godot::D_METHOD("get_arm_length"), &GameplayPerspective::get_arm_length);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "arm_length", godot::PROPERTY_HINT_RANGE, "0.0,100.0,0.05"),
        "set_arm_length",
        "get_arm_length"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_aim_arm_length", "length"), &GameplayPerspective::set_aim_arm_length);
    godot::ClassDB::bind_method(godot::D_METHOD("get_aim_arm_length"), &GameplayPerspective::get_aim_arm_length);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "aim_arm_length", godot::PROPERTY_HINT_RANGE, "0.0,100.0,0.05"),
        "set_aim_arm_length",
        "get_aim_arm_length"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_min_arm_length", "min_length"), &GameplayPerspective::set_min_arm_length);
    godot::ClassDB::bind_method(godot::D_METHOD("get_min_arm_length"), &GameplayPerspective::get_min_arm_length);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "min_arm_length", godot::PROPERTY_HINT_RANGE, "0.0,100.0,0.05"),
        "set_min_arm_length",
        "get_min_arm_length"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_arm_length", "max_length"), &GameplayPerspective::set_max_arm_length);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_arm_length"), &GameplayPerspective::get_max_arm_length);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "max_arm_length", godot::PROPERTY_HINT_RANGE, "0.0,150.0,0.1"),
        "set_max_arm_length",
        "get_max_arm_length"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_spring_arm_collision_enabled", "enabled"), &GameplayPerspective::set_spring_arm_collision_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_spring_arm_collision_enabled"), &GameplayPerspective::get_spring_arm_collision_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "spring_arm_collision_enabled"), "set_spring_arm_collision_enabled", "get_spring_arm_collision_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_spring_arm_collision_mask", "mask"), &GameplayPerspective::set_spring_arm_collision_mask);
    godot::ClassDB::bind_method(godot::D_METHOD("get_spring_arm_collision_mask"), &GameplayPerspective::get_spring_arm_collision_mask);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "spring_arm_collision_mask", godot::PROPERTY_HINT_LAYERS_3D_PHYSICS),
        "set_spring_arm_collision_mask",
        "get_spring_arm_collision_mask"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_spring_arm_margin", "margin"), &GameplayPerspective::set_spring_arm_margin);
    godot::ClassDB::bind_method(godot::D_METHOD("get_spring_arm_margin"), &GameplayPerspective::get_spring_arm_margin);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "spring_arm_margin", godot::PROPERTY_HINT_RANGE, "0.0,2.0,0.01"),
        "set_spring_arm_margin",
        "get_spring_arm_margin"
    );

    // --- 2. Orientation & Angular Constraints ---
    ADD_GROUP("Orientation & Angular Constraints", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_default_pitch_degrees", "pitch"), &GameplayPerspective::set_default_pitch_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_pitch_degrees"), &GameplayPerspective::get_default_pitch_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "default_pitch_degrees", godot::PROPERTY_HINT_RANGE, "-90.0,90.0,0.5"),
        "set_default_pitch_degrees",
        "get_default_pitch_degrees"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_default_yaw_degrees", "yaw"), &GameplayPerspective::set_default_yaw_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_yaw_degrees"), &GameplayPerspective::get_default_yaw_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "default_yaw_degrees", godot::PROPERTY_HINT_RANGE, "-180.0,180.0,0.5"),
        "set_default_yaw_degrees",
        "get_default_yaw_degrees"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_default_roll_degrees", "roll"), &GameplayPerspective::set_default_roll_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_roll_degrees"), &GameplayPerspective::get_default_roll_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "default_roll_degrees", godot::PROPERTY_HINT_RANGE, "-180.0,180.0,0.5"),
        "set_default_roll_degrees",
        "get_default_roll_degrees"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_min_pitch_degrees", "min_pitch"), &GameplayPerspective::set_min_pitch_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_min_pitch_degrees"), &GameplayPerspective::get_min_pitch_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "min_pitch_degrees", godot::PROPERTY_HINT_RANGE, "-90.0,90.0,0.5"),
        "set_min_pitch_degrees",
        "get_min_pitch_degrees"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_pitch_degrees", "max_pitch"), &GameplayPerspective::set_max_pitch_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_pitch_degrees"), &GameplayPerspective::get_max_pitch_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "max_pitch_degrees", godot::PROPERTY_HINT_RANGE, "-90.0,90.0,0.5"),
        "set_max_pitch_degrees",
        "get_max_pitch_degrees"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_lock_pitch", "lock"), &GameplayPerspective::set_lock_pitch);
    godot::ClassDB::bind_method(godot::D_METHOD("get_lock_pitch"), &GameplayPerspective::get_lock_pitch);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "lock_pitch"), "set_lock_pitch", "get_lock_pitch");

    godot::ClassDB::bind_method(godot::D_METHOD("set_lock_yaw", "lock"), &GameplayPerspective::set_lock_yaw);
    godot::ClassDB::bind_method(godot::D_METHOD("get_lock_yaw"), &GameplayPerspective::get_lock_yaw);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "lock_yaw"), "set_lock_yaw", "get_lock_yaw");

    godot::ClassDB::bind_method(godot::D_METHOD("set_yaw_limits_enabled", "enabled"), &GameplayPerspective::set_yaw_limits_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_yaw_limits_enabled"), &GameplayPerspective::get_yaw_limits_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "yaw_limits_enabled"), "set_yaw_limits_enabled", "get_yaw_limits_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_min_yaw_degrees", "min_yaw"), &GameplayPerspective::set_min_yaw_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_min_yaw_degrees"), &GameplayPerspective::get_min_yaw_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "min_yaw_degrees", godot::PROPERTY_HINT_RANGE, "-180.0,180.0,0.5"),
        "set_min_yaw_degrees",
        "get_min_yaw_degrees"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_yaw_degrees", "max_yaw"), &GameplayPerspective::set_max_yaw_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_yaw_degrees"), &GameplayPerspective::get_max_yaw_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "max_yaw_degrees", godot::PROPERTY_HINT_RANGE, "-180.0,180.0,0.5"),
        "set_max_yaw_degrees",
        "get_max_yaw_degrees"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_yaw_snap_step_degrees", "step"), &GameplayPerspective::set_yaw_snap_step_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_yaw_snap_step_degrees"), &GameplayPerspective::get_yaw_snap_step_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "yaw_snap_step_degrees", godot::PROPERTY_HINT_RANGE, "0.0,180.0,15.0"),
        "set_yaw_snap_step_degrees",
        "get_yaw_snap_step_degrees"
    );

    // --- 3. Smoothing, Transitions & Look Control ---
    ADD_GROUP("Smoothing & Control", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_position_follow_smoothing", "smoothing"), &GameplayPerspective::set_position_follow_smoothing);
    godot::ClassDB::bind_method(godot::D_METHOD("get_position_follow_smoothing"), &GameplayPerspective::get_position_follow_smoothing);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "position_follow_smoothing", godot::PROPERTY_HINT_RANGE, "0.0,60.0,0.5"),
        "set_position_follow_smoothing",
        "get_position_follow_smoothing"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_rotation_smoothing", "smoothing"), &GameplayPerspective::set_rotation_smoothing);
    godot::ClassDB::bind_method(godot::D_METHOD("get_rotation_smoothing"), &GameplayPerspective::get_rotation_smoothing);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "rotation_smoothing", godot::PROPERTY_HINT_RANGE, "0.0,60.0,0.5"),
        "set_rotation_smoothing",
        "get_rotation_smoothing"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_fov_transition_speed", "speed"), &GameplayPerspective::set_fov_transition_speed);
    godot::ClassDB::bind_method(godot::D_METHOD("get_fov_transition_speed"), &GameplayPerspective::get_fov_transition_speed);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "fov_transition_speed", godot::PROPERTY_HINT_RANGE, "0.1,60.0,0.5"),
        "set_fov_transition_speed",
        "get_fov_transition_speed"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_perspective_blend_duration", "duration"), &GameplayPerspective::set_perspective_blend_duration);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspective_blend_duration"), &GameplayPerspective::get_perspective_blend_duration);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "perspective_blend_duration", godot::PROPERTY_HINT_RANGE, "0.0,5.0,0.05"),
        "set_perspective_blend_duration",
        "get_perspective_blend_duration"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_look_sensitivity_x", "sensitivity"), &GameplayPerspective::set_look_sensitivity_x);
    godot::ClassDB::bind_method(godot::D_METHOD("get_look_sensitivity_x"), &GameplayPerspective::get_look_sensitivity_x);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "look_sensitivity_x", godot::PROPERTY_HINT_RANGE, "0.05,5.0,0.05"),
        "set_look_sensitivity_x",
        "get_look_sensitivity_x"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_look_sensitivity_y", "sensitivity"), &GameplayPerspective::set_look_sensitivity_y);
    godot::ClassDB::bind_method(godot::D_METHOD("get_look_sensitivity_y"), &GameplayPerspective::get_look_sensitivity_y);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "look_sensitivity_y", godot::PROPERTY_HINT_RANGE, "0.05,5.0,0.05"),
        "set_look_sensitivity_y",
        "get_look_sensitivity_y"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_aim_sensitivity_multiplier", "multiplier"), &GameplayPerspective::set_aim_sensitivity_multiplier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_aim_sensitivity_multiplier"), &GameplayPerspective::get_aim_sensitivity_multiplier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "aim_sensitivity_multiplier", godot::PROPERTY_HINT_RANGE, "0.05,2.0,0.05"),
        "set_aim_sensitivity_multiplier",
        "get_aim_sensitivity_multiplier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_invert_look_x", "invert"), &GameplayPerspective::set_invert_look_x);
    godot::ClassDB::bind_method(godot::D_METHOD("get_invert_look_x"), &GameplayPerspective::get_invert_look_x);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "invert_look_x"), "set_invert_look_x", "get_invert_look_x");

    godot::ClassDB::bind_method(godot::D_METHOD("set_invert_look_y", "invert"), &GameplayPerspective::set_invert_look_y);
    godot::ClassDB::bind_method(godot::D_METHOD("get_invert_look_y"), &GameplayPerspective::get_invert_look_y);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "invert_look_y"), "set_invert_look_y", "get_invert_look_y");

    // --- 4. Player Alignment & Visibility ---
    ADD_GROUP("Player Alignment & Visibility", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_movement_alignment", "mode"), &GameplayPerspective::set_movement_alignment);
    godot::ClassDB::bind_method(godot::D_METHOD("get_movement_alignment"), &GameplayPerspective::get_movement_alignment);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "movement_alignment",
            godot::PROPERTY_HINT_ENUM,
            "Camera Relative,Character Relative,World Axis Aligned,Cursor Directed"
        ),
        "set_movement_alignment",
        "get_movement_alignment"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_orient_player_to_camera", "orient"), &GameplayPerspective::set_orient_player_to_camera);
    godot::ClassDB::bind_method(godot::D_METHOD("get_orient_player_to_camera"), &GameplayPerspective::get_orient_player_to_camera);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "orient_player_to_camera"), "set_orient_player_to_camera", "get_orient_player_to_camera");

    godot::ClassDB::bind_method(godot::D_METHOD("set_orient_player_to_movement", "orient"), &GameplayPerspective::set_orient_player_to_movement);
    godot::ClassDB::bind_method(godot::D_METHOD("get_orient_player_to_movement"), &GameplayPerspective::get_orient_player_to_movement);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "orient_player_to_movement"), "set_orient_player_to_movement", "get_orient_player_to_movement");

    godot::ClassDB::bind_method(godot::D_METHOD("set_orient_player_on_aim", "orient"), &GameplayPerspective::set_orient_player_on_aim);
    godot::ClassDB::bind_method(godot::D_METHOD("get_orient_player_on_aim"), &GameplayPerspective::get_orient_player_on_aim);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "orient_player_on_aim"), "set_orient_player_on_aim", "get_orient_player_on_aim");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_mesh_visible", "visible"), &GameplayPerspective::set_player_mesh_visible);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_mesh_visible"), &GameplayPerspective::get_player_mesh_visible);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "player_mesh_visible"), "set_player_mesh_visible", "get_player_mesh_visible");

    godot::ClassDB::bind_method(godot::D_METHOD("set_first_person_hide_mesh_distance", "distance"), &GameplayPerspective::set_first_person_hide_mesh_distance);
    godot::ClassDB::bind_method(godot::D_METHOD("get_first_person_hide_mesh_distance"), &GameplayPerspective::get_first_person_hide_mesh_distance);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "first_person_hide_mesh_distance", godot::PROPERTY_HINT_RANGE, "0.0,2.0,0.05"),
        "set_first_person_hide_mesh_distance",
        "get_first_person_hide_mesh_distance"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_aim_reticle", "show"), &GameplayPerspective::set_show_aim_reticle);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_aim_reticle"), &GameplayPerspective::get_show_aim_reticle);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_aim_reticle"), "set_show_aim_reticle", "get_show_aim_reticle");

    godot::ClassDB::bind_method(godot::D_METHOD("set_occlude_environment_between_camera_and_player", "occlude"), &GameplayPerspective::set_occlude_environment_between_camera_and_player);
    godot::ClassDB::bind_method(godot::D_METHOD("get_occlude_environment_between_camera_and_player"), &GameplayPerspective::get_occlude_environment_between_camera_and_player);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::BOOL, "occlude_environment_between_camera_and_player"),
        "set_occlude_environment_between_camera_and_player",
        "get_occlude_environment_between_camera_and_player"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_group_tag", "tag"), &GameplayPerspective::set_player_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_group_tag"), &GameplayPerspective::get_player_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "player_group_tag"), "set_player_group_tag", "get_player_group_tag");

    // --- Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("get_effective_camera_offset", "is_aiming"), &GameplayPerspective::get_effective_camera_offset, DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("get_effective_fov", "is_aiming"), &GameplayPerspective::get_effective_fov, DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("get_effective_orthogonal_size", "is_aiming"), &GameplayPerspective::get_effective_orthogonal_size, DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("get_effective_arm_length", "is_aiming"), &GameplayPerspective::get_effective_arm_length, DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("get_effective_look_sensitivity", "is_aiming"), &GameplayPerspective::get_effective_look_sensitivity, DEFVAL(false));
    godot::ClassDB::bind_method(godot::D_METHOD("clamp_pitch_degrees", "pitch_deg"), &GameplayPerspective::clamp_pitch_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("clamp_yaw_degrees", "yaw_deg"), &GameplayPerspective::clamp_yaw_degrees);
    godot::ClassDB::bind_method(
        godot::D_METHOD("compute_camera_rig_transform", "target_origin", "yaw_deg", "pitch_deg", "is_aiming"),
        &GameplayPerspective::compute_camera_rig_transform,
        DEFVAL(false)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("interpolate_with", "target", "weight"), &GameplayPerspective::interpolate_with);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_perspective", "other"), &GameplayPerspective::apply_perspective);
    godot::ClassDB::bind_method(
        godot::D_METHOD("apply_to_camera_rig", "pivot", "spring_arm", "camera", "is_aiming"),
        &GameplayPerspective::apply_to_camera_rig,
        DEFVAL(false)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("apply_to_camera_2d", "camera_2d", "is_aiming"),
        &GameplayPerspective::apply_to_camera_2d,
        DEFVAL(false)
    );
}

GameplayPerspective::GameplayPerspective() {
    set_category(godot::StringName("Perspective"));
    set_config_section("perspective");
    _sync_perspective_option_tags();
}

GameplayPerspective::~GameplayPerspective() {
}

// --- Internal Tag Helper ---

void GameplayPerspective::_sync_perspective_option_tags() {
    // Remove previous known perspective archetype tags before adding the current archetype tag
    remove_tag(godot::StringName("FirstPerson"));
    remove_tag(godot::StringName("ThirdPerson"));
    remove_tag(godot::StringName("OverTheShoulder"));
    remove_tag(godot::StringName("ThirdPersonOrbit"));
    remove_tag(godot::StringName("TopDown"));
    remove_tag(godot::StringName("ThreeQuarter"));
    remove_tag(godot::StringName("Isometric"));
    remove_tag(godot::StringName("Dimetric"));
    remove_tag(godot::StringName("SideScroller"));
    remove_tag(godot::StringName("FixedCinematic"));
    remove_tag(godot::StringName("Orthogonal"));

    switch (perspective_type) {
        case PerspectiveType::PERSPECTIVE_FIRST_PERSON:
            add_tag(godot::StringName("FirstPerson"));
            break;
        case PerspectiveType::PERSPECTIVE_THIRD_PERSON_OTS:
            add_tag(godot::StringName("ThirdPerson"));
            add_tag(godot::StringName("OverTheShoulder"));
            break;
        case PerspectiveType::PERSPECTIVE_THIRD_PERSON_ORBIT:
            add_tag(godot::StringName("ThirdPerson"));
            add_tag(godot::StringName("ThirdPersonOrbit"));
            break;
        case PerspectiveType::PERSPECTIVE_TOP_DOWN:
            add_tag(godot::StringName("TopDown"));
            break;
        case PerspectiveType::PERSPECTIVE_THREE_QUARTER:
            add_tag(godot::StringName("ThreeQuarter"));
            break;
        case PerspectiveType::PERSPECTIVE_ISOMETRIC:
            add_tag(godot::StringName("Isometric"));
            break;
        case PerspectiveType::PERSPECTIVE_DIMETRIC:
            add_tag(godot::StringName("Dimetric"));
            break;
        case PerspectiveType::PERSPECTIVE_SIDE_SCROLLER:
            add_tag(godot::StringName("SideScroller"));
            break;
        case PerspectiveType::PERSPECTIVE_FIXED_CINEMATIC:
            add_tag(godot::StringName("FixedCinematic"));
            break;
        case PerspectiveType::PERSPECTIVE_CUSTOM:
        default:
            break;
    }

    if (projection_mode == PerspectiveProjectionMode::PROJECTION_ORTHOGONAL) {
        add_tag(godot::StringName("Orthogonal"));
    }
}

// --- 0. Preset & Projection Setters / Getters ---

void GameplayPerspective::set_perspective_type(PerspectiveType p_type) {
    if (perspective_type == p_type) return;
    perspective_type = p_type;
    if (p_type != PerspectiveType::PERSPECTIVE_CUSTOM) {
        apply_perspective_preset(p_type);
    } else {
        _sync_perspective_option_tags();
        emit_changed();
    }
}

PerspectiveType GameplayPerspective::get_perspective_type() const {
    return perspective_type;
}

void GameplayPerspective::apply_perspective_preset(PerspectiveType p_type) {
    perspective_type = p_type;

    switch (p_type) {
        case PerspectiveType::PERSPECTIVE_FIRST_PERSON: {
            projection_mode = PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;
            fov = 90.0f;
            aim_fov = 60.0f;
            near_clip = 0.03f;
            pivot_offset = godot::Vector3(0.0f, 1.65f, 0.0f);
            camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_CENTER;
            allow_shoulder_swap = false;
            arm_length = 0.0f;
            aim_arm_length = 0.0f;
            min_arm_length = 0.0f;
            max_arm_length = 0.0f;
            spring_arm_collision_enabled = false;
            default_pitch_degrees = 0.0f;
            default_yaw_degrees = 0.0f;
            min_pitch_degrees = -89.0f;
            max_pitch_degrees = 89.0f;
            lock_pitch = false;
            lock_yaw = false;
            yaw_limits_enabled = false;
            yaw_snap_step_degrees = 0.0f;
            movement_alignment = MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE;
            orient_player_to_camera = true;
            orient_player_to_movement = false;
            orient_player_on_aim = true;
            player_mesh_visible = false;
            show_aim_reticle = true;
            occlude_environment_between_camera_and_player = false;
            break;
        }
        case PerspectiveType::PERSPECTIVE_THIRD_PERSON_OTS: {
            projection_mode = PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;
            fov = 75.0f;
            aim_fov = 50.0f;
            near_clip = 0.05f;
            pivot_offset = godot::Vector3(0.0f, 1.6f, 0.0f);
            camera_local_offset = godot::Vector3(0.45f, 0.15f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.55f, 0.1f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_RIGHT;
            allow_shoulder_swap = true;
            arm_length = 2.5f;
            aim_arm_length = 1.35f;
            min_arm_length = 0.8f;
            max_arm_length = 4.5f;
            spring_arm_collision_enabled = true;
            default_pitch_degrees = -10.0f;
            default_yaw_degrees = 0.0f;
            min_pitch_degrees = -75.0f;
            max_pitch_degrees = 65.0f;
            lock_pitch = false;
            lock_yaw = false;
            yaw_limits_enabled = false;
            yaw_snap_step_degrees = 0.0f;
            movement_alignment = MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE;
            orient_player_to_camera = false;
            orient_player_to_movement = true;
            orient_player_on_aim = true;
            player_mesh_visible = true;
            show_aim_reticle = true;
            occlude_environment_between_camera_and_player = false;
            break;
        }
        case PerspectiveType::PERSPECTIVE_THIRD_PERSON_ORBIT: {
            projection_mode = PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;
            fov = 75.0f;
            aim_fov = 60.0f;
            near_clip = 0.05f;
            pivot_offset = godot::Vector3(0.0f, 1.5f, 0.0f);
            camera_local_offset = godot::Vector3(0.0f, 0.25f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.35f, 0.15f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_CENTER;
            allow_shoulder_swap = false;
            arm_length = 4.0f;
            aim_arm_length = 2.2f;
            min_arm_length = 1.5f;
            max_arm_length = 8.0f;
            spring_arm_collision_enabled = true;
            default_pitch_degrees = -15.0f;
            default_yaw_degrees = 0.0f;
            min_pitch_degrees = -80.0f;
            max_pitch_degrees = 70.0f;
            lock_pitch = false;
            lock_yaw = false;
            yaw_limits_enabled = false;
            yaw_snap_step_degrees = 0.0f;
            movement_alignment = MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE;
            orient_player_to_camera = false;
            orient_player_to_movement = true;
            orient_player_on_aim = true;
            player_mesh_visible = true;
            show_aim_reticle = false;
            occlude_environment_between_camera_and_player = false;
            break;
        }
        case PerspectiveType::PERSPECTIVE_TOP_DOWN: {
            projection_mode = PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;
            fov = 60.0f;
            aim_fov = 50.0f;
            orthogonal_size = 16.0f;
            aim_orthogonal_size = 12.0f;
            pivot_offset = godot::Vector3(0.0f, 0.5f, 0.0f);
            camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_CENTER;
            allow_shoulder_swap = false;
            arm_length = 16.0f;
            aim_arm_length = 13.0f;
            min_arm_length = 8.0f;
            max_arm_length = 30.0f;
            spring_arm_collision_enabled = false;
            default_pitch_degrees = -89.0f;
            default_yaw_degrees = 0.0f;
            min_pitch_degrees = -89.0f;
            max_pitch_degrees = -75.0f;
            lock_pitch = true;
            lock_yaw = true;
            yaw_limits_enabled = false;
            yaw_snap_step_degrees = 90.0f;
            movement_alignment = MovementAlignmentMode::MOVEMENT_WORLD_AXIS_ALIGNED;
            orient_player_to_camera = false;
            orient_player_to_movement = true;
            orient_player_on_aim = true;
            player_mesh_visible = true;
            show_aim_reticle = true;
            occlude_environment_between_camera_and_player = true;
            break;
        }
        case PerspectiveType::PERSPECTIVE_THREE_QUARTER: {
            projection_mode = PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;
            fov = 55.0f;
            aim_fov = 45.0f;
            orthogonal_size = 14.0f;
            aim_orthogonal_size = 11.0f;
            pivot_offset = godot::Vector3(0.0f, 1.0f, 0.0f);
            camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_CENTER;
            allow_shoulder_swap = false;
            arm_length = 14.0f;
            aim_arm_length = 11.0f;
            min_arm_length = 7.0f;
            max_arm_length = 24.0f;
            spring_arm_collision_enabled = false;
            default_pitch_degrees = -50.0f;
            default_yaw_degrees = 0.0f;
            min_pitch_degrees = -65.0f;
            max_pitch_degrees = -35.0f;
            lock_pitch = true;
            lock_yaw = false;
            yaw_limits_enabled = false;
            yaw_snap_step_degrees = 45.0f;
            movement_alignment = MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE;
            orient_player_to_camera = false;
            orient_player_to_movement = true;
            orient_player_on_aim = true;
            player_mesh_visible = true;
            show_aim_reticle = false;
            occlude_environment_between_camera_and_player = true;
            break;
        }
        case PerspectiveType::PERSPECTIVE_ISOMETRIC: {
            // True isometric: arctan(1 / sqrt(2)) ~= 35.2643897 degrees pitch, 45 degrees yaw, orthogonal projection
            projection_mode = PerspectiveProjectionMode::PROJECTION_ORTHOGONAL;
            orthogonal_size = 14.0f;
            aim_orthogonal_size = 11.0f;
            fov = 45.0f;
            aim_fov = 38.0f;
            pivot_offset = godot::Vector3(0.0f, 1.0f, 0.0f);
            camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_CENTER;
            allow_shoulder_swap = false;
            arm_length = 25.0f;
            aim_arm_length = 25.0f;
            min_arm_length = 10.0f;
            max_arm_length = 50.0f;
            spring_arm_collision_enabled = false;
            default_pitch_degrees = -35.26439f;
            default_yaw_degrees = 45.0f;
            min_pitch_degrees = -35.26439f;
            max_pitch_degrees = -35.26439f;
            lock_pitch = true;
            lock_yaw = true;
            yaw_limits_enabled = false;
            yaw_snap_step_degrees = 90.0f;
            movement_alignment = MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE;
            orient_player_to_camera = false;
            orient_player_to_movement = true;
            orient_player_on_aim = true;
            player_mesh_visible = true;
            show_aim_reticle = false;
            occlude_environment_between_camera_and_player = true;
            break;
        }
        case PerspectiveType::PERSPECTIVE_DIMETRIC: {
            // 2:1 Dimetric: -30.0 degrees pitch, 45.0 degrees yaw, orthogonal projection
            projection_mode = PerspectiveProjectionMode::PROJECTION_ORTHOGONAL;
            orthogonal_size = 14.0f;
            aim_orthogonal_size = 11.0f;
            fov = 45.0f;
            aim_fov = 38.0f;
            pivot_offset = godot::Vector3(0.0f, 1.0f, 0.0f);
            camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_CENTER;
            allow_shoulder_swap = false;
            arm_length = 25.0f;
            aim_arm_length = 25.0f;
            min_arm_length = 10.0f;
            max_arm_length = 50.0f;
            spring_arm_collision_enabled = false;
            default_pitch_degrees = -30.0f;
            default_yaw_degrees = 45.0f;
            min_pitch_degrees = -30.0f;
            max_pitch_degrees = -30.0f;
            lock_pitch = true;
            lock_yaw = true;
            yaw_limits_enabled = false;
            yaw_snap_step_degrees = 90.0f;
            movement_alignment = MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE;
            orient_player_to_camera = false;
            orient_player_to_movement = true;
            orient_player_on_aim = true;
            player_mesh_visible = true;
            show_aim_reticle = false;
            occlude_environment_between_camera_and_player = true;
            break;
        }
        case PerspectiveType::PERSPECTIVE_SIDE_SCROLLER: {
            projection_mode = PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;
            fov = 60.0f;
            aim_fov = 50.0f;
            orthogonal_size = 12.0f;
            aim_orthogonal_size = 10.0f;
            pivot_offset = godot::Vector3(0.0f, 1.4f, 0.0f);
            camera_local_offset = godot::Vector3(0.0f, 0.2f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.5f, 0.2f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_CENTER;
            allow_shoulder_swap = false;
            arm_length = 10.0f;
            aim_arm_length = 8.5f;
            min_arm_length = 5.0f;
            max_arm_length = 20.0f;
            spring_arm_collision_enabled = false;
            default_pitch_degrees = 0.0f;
            default_yaw_degrees = 0.0f;
            min_pitch_degrees = -15.0f;
            max_pitch_degrees = 15.0f;
            lock_pitch = true;
            lock_yaw = true;
            yaw_limits_enabled = false;
            yaw_snap_step_degrees = 0.0f;
            movement_alignment = MovementAlignmentMode::MOVEMENT_WORLD_AXIS_ALIGNED;
            orient_player_to_camera = false;
            orient_player_to_movement = true;
            orient_player_on_aim = true;
            player_mesh_visible = true;
            show_aim_reticle = false;
            occlude_environment_between_camera_and_player = true;
            break;
        }
        case PerspectiveType::PERSPECTIVE_FIXED_CINEMATIC: {
            projection_mode = PerspectiveProjectionMode::PROJECTION_PERSPECTIVE;
            fov = 65.0f;
            aim_fov = 55.0f;
            pivot_offset = godot::Vector3(0.0f, 1.5f, 0.0f);
            camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            aim_camera_local_offset = godot::Vector3(0.0f, 0.0f, 0.0f);
            shoulder_side = ShoulderSide::SHOULDER_CENTER;
            allow_shoulder_swap = false;
            arm_length = 5.0f;
            aim_arm_length = 4.5f;
            spring_arm_collision_enabled = false;
            lock_pitch = true;
            lock_yaw = true;
            movement_alignment = MovementAlignmentMode::MOVEMENT_CHARACTER_RELATIVE;
            orient_player_to_camera = false;
            orient_player_to_movement = true;
            orient_player_on_aim = false;
            player_mesh_visible = true;
            show_aim_reticle = false;
            occlude_environment_between_camera_and_player = false;
            break;
        }
        case PerspectiveType::PERSPECTIVE_CUSTOM:
        default:
            break;
    }

    _sync_perspective_option_tags();
    emit_changed();
}

void GameplayPerspective::set_projection_mode(PerspectiveProjectionMode p_mode) {
    if (projection_mode == p_mode) return;
    projection_mode = p_mode;
    _sync_perspective_option_tags();
    emit_changed();
}
PerspectiveProjectionMode GameplayPerspective::get_projection_mode() const { return projection_mode; }

void GameplayPerspective::set_fov(float p_fov) {
    float clamped = godot::Math::clamp(p_fov, 10.0f, 160.0f);
    if (godot::Math::is_equal_approx(fov, clamped)) return;
    fov = clamped;
    emit_changed();
}
float GameplayPerspective::get_fov() const { return fov; }

void GameplayPerspective::set_aim_fov(float p_fov) {
    float clamped = godot::Math::clamp(p_fov, 10.0f, 160.0f);
    if (godot::Math::is_equal_approx(aim_fov, clamped)) return;
    aim_fov = clamped;
    emit_changed();
}
float GameplayPerspective::get_aim_fov() const { return aim_fov; }

void GameplayPerspective::set_orthogonal_size(float p_size) {
    float clamped = godot::Math::clamp(p_size, 0.1f, 500.0f);
    if (godot::Math::is_equal_approx(orthogonal_size, clamped)) return;
    orthogonal_size = clamped;
    emit_changed();
}
float GameplayPerspective::get_orthogonal_size() const { return orthogonal_size; }

void GameplayPerspective::set_aim_orthogonal_size(float p_size) {
    float clamped = godot::Math::clamp(p_size, 0.1f, 500.0f);
    if (godot::Math::is_equal_approx(aim_orthogonal_size, clamped)) return;
    aim_orthogonal_size = clamped;
    emit_changed();
}
float GameplayPerspective::get_aim_orthogonal_size() const { return aim_orthogonal_size; }

void GameplayPerspective::set_near_clip(float p_near) {
    float clamped = godot::Math::clamp(p_near, 0.001f, 50.0f);
    if (godot::Math::is_equal_approx(near_clip, clamped)) return;
    near_clip = clamped;
    emit_changed();
}
float GameplayPerspective::get_near_clip() const { return near_clip; }

void GameplayPerspective::set_far_clip(float p_far) {
    float clamped = godot::Math::clamp(p_far, 1.0f, 100000.0f);
    if (godot::Math::is_equal_approx(far_clip, clamped)) return;
    far_clip = clamped;
    emit_changed();
}
float GameplayPerspective::get_far_clip() const { return far_clip; }

void GameplayPerspective::set_frustum_offset(const godot::Vector2 &p_offset) {
    if (frustum_offset.is_equal_approx(p_offset)) return;
    frustum_offset = p_offset;
    emit_changed();
}
godot::Vector2 GameplayPerspective::get_frustum_offset() const { return frustum_offset; }

void GameplayPerspective::set_viewport_zoom_2d(const godot::Vector2 &p_zoom) {
    godot::Vector2 clamped(
        godot::Math::clamp(p_zoom.x, 0.05f, 20.0f),
        godot::Math::clamp(p_zoom.y, 0.05f, 20.0f)
    );
    if (viewport_zoom_2d.is_equal_approx(clamped)) return;
    viewport_zoom_2d = clamped;
    emit_changed();
}
godot::Vector2 GameplayPerspective::get_viewport_zoom_2d() const { return viewport_zoom_2d; }

// --- 1. Rig & Spatial Offsets Setters / Getters ---

void GameplayPerspective::set_pivot_offset(const godot::Vector3 &p_offset) {
    if (pivot_offset.is_equal_approx(p_offset)) return;
    pivot_offset = p_offset;
    emit_changed();
}
godot::Vector3 GameplayPerspective::get_pivot_offset() const { return pivot_offset; }

void GameplayPerspective::set_camera_local_offset(const godot::Vector3 &p_offset) {
    if (camera_local_offset.is_equal_approx(p_offset)) return;
    camera_local_offset = p_offset;
    emit_changed();
}
godot::Vector3 GameplayPerspective::get_camera_local_offset() const { return camera_local_offset; }

void GameplayPerspective::set_aim_camera_local_offset(const godot::Vector3 &p_offset) {
    if (aim_camera_local_offset.is_equal_approx(p_offset)) return;
    aim_camera_local_offset = p_offset;
    emit_changed();
}
godot::Vector3 GameplayPerspective::get_aim_camera_local_offset() const { return aim_camera_local_offset; }

void GameplayPerspective::set_shoulder_side(ShoulderSide p_side) {
    if (shoulder_side == p_side) return;
    shoulder_side = p_side;
    emit_changed();
}
ShoulderSide GameplayPerspective::get_shoulder_side() const { return shoulder_side; }

void GameplayPerspective::swap_shoulder() {
    if (!allow_shoulder_swap) return;
    if (shoulder_side == ShoulderSide::SHOULDER_RIGHT) {
        shoulder_side = ShoulderSide::SHOULDER_LEFT;
        emit_changed();
    } else if (shoulder_side == ShoulderSide::SHOULDER_LEFT) {
        shoulder_side = ShoulderSide::SHOULDER_RIGHT;
        emit_changed();
    }
}

void GameplayPerspective::set_allow_shoulder_swap(bool p_allow) {
    if (allow_shoulder_swap == p_allow) return;
    allow_shoulder_swap = p_allow;
    emit_changed();
}
bool GameplayPerspective::get_allow_shoulder_swap() const { return allow_shoulder_swap; }

void GameplayPerspective::set_arm_length(float p_length) {
    float clamped = godot::Math::clamp(p_length, 0.0f, 500.0f);
    if (godot::Math::is_equal_approx(arm_length, clamped)) return;
    arm_length = clamped;
    emit_changed();
}
float GameplayPerspective::get_arm_length() const { return arm_length; }

void GameplayPerspective::set_aim_arm_length(float p_length) {
    float clamped = godot::Math::clamp(p_length, 0.0f, 500.0f);
    if (godot::Math::is_equal_approx(aim_arm_length, clamped)) return;
    aim_arm_length = clamped;
    emit_changed();
}
float GameplayPerspective::get_aim_arm_length() const { return aim_arm_length; }

void GameplayPerspective::set_min_arm_length(float p_min) {
    float clamped = godot::Math::clamp(p_min, 0.0f, 500.0f);
    if (godot::Math::is_equal_approx(min_arm_length, clamped)) return;
    min_arm_length = clamped;
    emit_changed();
}
float GameplayPerspective::get_min_arm_length() const { return min_arm_length; }

void GameplayPerspective::set_max_arm_length(float p_max) {
    float clamped = godot::Math::clamp(p_max, 0.0f, 500.0f);
    if (godot::Math::is_equal_approx(max_arm_length, clamped)) return;
    max_arm_length = clamped;
    emit_changed();
}
float GameplayPerspective::get_max_arm_length() const { return max_arm_length; }

void GameplayPerspective::set_spring_arm_collision_enabled(bool p_enabled) {
    if (spring_arm_collision_enabled == p_enabled) return;
    spring_arm_collision_enabled = p_enabled;
    emit_changed();
}
bool GameplayPerspective::get_spring_arm_collision_enabled() const { return spring_arm_collision_enabled; }

void GameplayPerspective::set_spring_arm_collision_mask(uint32_t p_mask) {
    if (spring_arm_collision_mask == p_mask) return;
    spring_arm_collision_mask = p_mask;
    emit_changed();
}
uint32_t GameplayPerspective::get_spring_arm_collision_mask() const { return spring_arm_collision_mask; }

void GameplayPerspective::set_spring_arm_margin(float p_margin) {
    float clamped = godot::Math::clamp(p_margin, 0.0f, 5.0f);
    if (godot::Math::is_equal_approx(spring_arm_margin, clamped)) return;
    spring_arm_margin = clamped;
    emit_changed();
}
float GameplayPerspective::get_spring_arm_margin() const { return spring_arm_margin; }

// --- 2. Orientation & Angular Constraints Setters / Getters ---

void GameplayPerspective::set_default_pitch_degrees(float p_pitch) {
    float clamped = godot::Math::clamp(p_pitch, -90.0f, 90.0f);
    if (godot::Math::is_equal_approx(default_pitch_degrees, clamped)) return;
    default_pitch_degrees = clamped;
    emit_changed();
}
float GameplayPerspective::get_default_pitch_degrees() const { return default_pitch_degrees; }

void GameplayPerspective::set_default_yaw_degrees(float p_yaw) {
    float clamped = godot::Math::clamp(p_yaw, -360.0f, 360.0f);
    if (godot::Math::is_equal_approx(default_yaw_degrees, clamped)) return;
    default_yaw_degrees = clamped;
    emit_changed();
}
float GameplayPerspective::get_default_yaw_degrees() const { return default_yaw_degrees; }

void GameplayPerspective::set_default_roll_degrees(float p_roll) {
    float clamped = godot::Math::clamp(p_roll, -180.0f, 180.0f);
    if (godot::Math::is_equal_approx(default_roll_degrees, clamped)) return;
    default_roll_degrees = clamped;
    emit_changed();
}
float GameplayPerspective::get_default_roll_degrees() const { return default_roll_degrees; }

void GameplayPerspective::set_min_pitch_degrees(float p_min_pitch) {
    float clamped = godot::Math::clamp(p_min_pitch, -90.0f, 90.0f);
    if (godot::Math::is_equal_approx(min_pitch_degrees, clamped)) return;
    min_pitch_degrees = clamped;
    emit_changed();
}
float GameplayPerspective::get_min_pitch_degrees() const { return min_pitch_degrees; }

void GameplayPerspective::set_max_pitch_degrees(float p_max_pitch) {
    float clamped = godot::Math::clamp(p_max_pitch, -90.0f, 90.0f);
    if (godot::Math::is_equal_approx(max_pitch_degrees, clamped)) return;
    max_pitch_degrees = clamped;
    emit_changed();
}
float GameplayPerspective::get_max_pitch_degrees() const { return max_pitch_degrees; }

void GameplayPerspective::set_lock_pitch(bool p_lock) {
    if (lock_pitch == p_lock) return;
    lock_pitch = p_lock;
    emit_changed();
}
bool GameplayPerspective::get_lock_pitch() const { return lock_pitch; }

void GameplayPerspective::set_lock_yaw(bool p_lock) {
    if (lock_yaw == p_lock) return;
    lock_yaw = p_lock;
    emit_changed();
}
bool GameplayPerspective::get_lock_yaw() const { return lock_yaw; }

void GameplayPerspective::set_yaw_limits_enabled(bool p_enabled) {
    if (yaw_limits_enabled == p_enabled) return;
    yaw_limits_enabled = p_enabled;
    emit_changed();
}
bool GameplayPerspective::get_yaw_limits_enabled() const { return yaw_limits_enabled; }

void GameplayPerspective::set_min_yaw_degrees(float p_min_yaw) {
    float clamped = godot::Math::clamp(p_min_yaw, -360.0f, 360.0f);
    if (godot::Math::is_equal_approx(min_yaw_degrees, clamped)) return;
    min_yaw_degrees = clamped;
    emit_changed();
}
float GameplayPerspective::get_min_yaw_degrees() const { return min_yaw_degrees; }

void GameplayPerspective::set_max_yaw_degrees(float p_max_yaw) {
    float clamped = godot::Math::clamp(p_max_yaw, -360.0f, 360.0f);
    if (godot::Math::is_equal_approx(max_yaw_degrees, clamped)) return;
    max_yaw_degrees = clamped;
    emit_changed();
}
float GameplayPerspective::get_max_yaw_degrees() const { return max_yaw_degrees; }

void GameplayPerspective::set_yaw_snap_step_degrees(float p_step) {
    float clamped = godot::Math::clamp(p_step, 0.0f, 180.0f);
    if (godot::Math::is_equal_approx(yaw_snap_step_degrees, clamped)) return;
    yaw_snap_step_degrees = clamped;
    emit_changed();
}
float GameplayPerspective::get_yaw_snap_step_degrees() const { return yaw_snap_step_degrees; }

// --- 3. Smoothing, Transitions & Look Control Setters / Getters ---

void GameplayPerspective::set_position_follow_smoothing(float p_smoothing) {
    float clamped = godot::Math::clamp(p_smoothing, 0.0f, 120.0f);
    if (godot::Math::is_equal_approx(position_follow_smoothing, clamped)) return;
    position_follow_smoothing = clamped;
    emit_changed();
}
float GameplayPerspective::get_position_follow_smoothing() const { return position_follow_smoothing; }

void GameplayPerspective::set_rotation_smoothing(float p_smoothing) {
    float clamped = godot::Math::clamp(p_smoothing, 0.0f, 120.0f);
    if (godot::Math::is_equal_approx(rotation_smoothing, clamped)) return;
    rotation_smoothing = clamped;
    emit_changed();
}
float GameplayPerspective::get_rotation_smoothing() const { return rotation_smoothing; }

void GameplayPerspective::set_fov_transition_speed(float p_speed) {
    float clamped = godot::Math::clamp(p_speed, 0.1f, 120.0f);
    if (godot::Math::is_equal_approx(fov_transition_speed, clamped)) return;
    fov_transition_speed = clamped;
    emit_changed();
}
float GameplayPerspective::get_fov_transition_speed() const { return fov_transition_speed; }

void GameplayPerspective::set_perspective_blend_duration(float p_duration) {
    float clamped = godot::Math::clamp(p_duration, 0.0f, 10.0f);
    if (godot::Math::is_equal_approx(perspective_blend_duration, clamped)) return;
    perspective_blend_duration = clamped;
    emit_changed();
}
float GameplayPerspective::get_perspective_blend_duration() const { return perspective_blend_duration; }

void GameplayPerspective::set_look_sensitivity_x(float p_sens) {
    float clamped = godot::Math::clamp(p_sens, 0.01f, 10.0f);
    if (godot::Math::is_equal_approx(look_sensitivity_x, clamped)) return;
    look_sensitivity_x = clamped;
    emit_changed();
}
float GameplayPerspective::get_look_sensitivity_x() const { return look_sensitivity_x; }

void GameplayPerspective::set_look_sensitivity_y(float p_sens) {
    float clamped = godot::Math::clamp(p_sens, 0.01f, 10.0f);
    if (godot::Math::is_equal_approx(look_sensitivity_y, clamped)) return;
    look_sensitivity_y = clamped;
    emit_changed();
}
float GameplayPerspective::get_look_sensitivity_y() const { return look_sensitivity_y; }

void GameplayPerspective::set_aim_sensitivity_multiplier(float p_mult) {
    float clamped = godot::Math::clamp(p_mult, 0.01f, 5.0f);
    if (godot::Math::is_equal_approx(aim_sensitivity_multiplier, clamped)) return;
    aim_sensitivity_multiplier = clamped;
    emit_changed();
}
float GameplayPerspective::get_aim_sensitivity_multiplier() const { return aim_sensitivity_multiplier; }

void GameplayPerspective::set_invert_look_x(bool p_invert) {
    if (invert_look_x == p_invert) return;
    invert_look_x = p_invert;
    emit_changed();
}
bool GameplayPerspective::get_invert_look_x() const { return invert_look_x; }

void GameplayPerspective::set_invert_look_y(bool p_invert) {
    if (invert_look_y == p_invert) return;
    invert_look_y = p_invert;
    emit_changed();
}
bool GameplayPerspective::get_invert_look_y() const { return invert_look_y; }

// --- 4. Player Alignment & Visibility Setters / Getters ---

void GameplayPerspective::set_movement_alignment(MovementAlignmentMode p_mode) {
    if (movement_alignment == p_mode) return;
    movement_alignment = p_mode;
    emit_changed();
}
MovementAlignmentMode GameplayPerspective::get_movement_alignment() const { return movement_alignment; }

void GameplayPerspective::set_orient_player_to_camera(bool p_orient) {
    if (orient_player_to_camera == p_orient) return;
    orient_player_to_camera = p_orient;
    emit_changed();
}
bool GameplayPerspective::get_orient_player_to_camera() const { return orient_player_to_camera; }

void GameplayPerspective::set_orient_player_to_movement(bool p_orient) {
    if (orient_player_to_movement == p_orient) return;
    orient_player_to_movement = p_orient;
    emit_changed();
}
bool GameplayPerspective::get_orient_player_to_movement() const { return orient_player_to_movement; }

void GameplayPerspective::set_orient_player_on_aim(bool p_orient) {
    if (orient_player_on_aim == p_orient) return;
    orient_player_on_aim = p_orient;
    emit_changed();
}
bool GameplayPerspective::get_orient_player_on_aim() const { return orient_player_on_aim; }

void GameplayPerspective::set_player_mesh_visible(bool p_visible) {
    if (player_mesh_visible == p_visible) return;
    player_mesh_visible = p_visible;
    emit_changed();
}
bool GameplayPerspective::get_player_mesh_visible() const { return player_mesh_visible; }

void GameplayPerspective::set_first_person_hide_mesh_distance(float p_distance) {
    float clamped = godot::Math::clamp(p_distance, 0.0f, 5.0f);
    if (godot::Math::is_equal_approx(first_person_hide_mesh_distance, clamped)) return;
    first_person_hide_mesh_distance = clamped;
    emit_changed();
}
float GameplayPerspective::get_first_person_hide_mesh_distance() const { return first_person_hide_mesh_distance; }

void GameplayPerspective::set_show_aim_reticle(bool p_show) {
    if (show_aim_reticle == p_show) return;
    show_aim_reticle = p_show;
    emit_changed();
}
bool GameplayPerspective::get_show_aim_reticle() const { return show_aim_reticle; }

void GameplayPerspective::set_occlude_environment_between_camera_and_player(bool p_occlude) {
    if (occlude_environment_between_camera_and_player == p_occlude) return;
    occlude_environment_between_camera_and_player = p_occlude;
    emit_changed();
}
bool GameplayPerspective::get_occlude_environment_between_camera_and_player() const { return occlude_environment_between_camera_and_player; }

void GameplayPerspective::set_player_group_tag(const godot::StringName &p_tag) {
    if (player_group_tag == p_tag) return;
    player_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayPerspective::get_player_group_tag() const { return player_group_tag; }

// --- Computed Rig Queries & Blending Helpers ---

godot::Vector3 GameplayPerspective::get_effective_camera_offset(bool p_is_aiming) const {
    godot::Vector3 base_offset = p_is_aiming ? aim_camera_local_offset : camera_local_offset;
    switch (shoulder_side) {
        case ShoulderSide::SHOULDER_CENTER:
            base_offset.x = 0.0f;
            break;
        case ShoulderSide::SHOULDER_LEFT:
            base_offset.x = - std::abs(base_offset.x);
            break;
        case ShoulderSide::SHOULDER_RIGHT:
        default:
            base_offset.x = std::abs(base_offset.x);
            break;
    }
    return base_offset;
}

float GameplayPerspective::get_effective_fov(bool p_is_aiming) const {
    return p_is_aiming ? aim_fov : fov;
}

float GameplayPerspective::get_effective_orthogonal_size(bool p_is_aiming) const {
    return p_is_aiming ? aim_orthogonal_size : orthogonal_size;
}

float GameplayPerspective::get_effective_arm_length(bool p_is_aiming) const {
    float target_len = p_is_aiming ? aim_arm_length : arm_length;
    if (max_arm_length > min_arm_length) {
        return godot::Math::clamp(target_len, min_arm_length, max_arm_length);
    }
    return target_len;
}

godot::Vector2 GameplayPerspective::get_effective_look_sensitivity(bool p_is_aiming) const {
    float scale = p_is_aiming ? aim_sensitivity_multiplier : 1.0f;
    float sx = look_sensitivity_x * scale * (invert_look_x ? -1.0f : 1.0f);
    float sy = look_sensitivity_y * scale * (invert_look_y ? -1.0f : 1.0f);
    return godot::Vector2(sx, sy);
}

float GameplayPerspective::clamp_pitch_degrees(float p_pitch_deg) const {
    if (lock_pitch) {
        return default_pitch_degrees;
    }
    float low = godot::Math::min(min_pitch_degrees, max_pitch_degrees);
    float high = godot::Math::max(min_pitch_degrees, max_pitch_degrees);
    return godot::Math::clamp(p_pitch_deg, low, high);
}

float GameplayPerspective::clamp_yaw_degrees(float p_yaw_deg) const {
    if (lock_yaw) {
        return default_yaw_degrees;
    }
    float result = p_yaw_deg;
    if (yaw_snap_step_degrees > 0.001f) {
        result = std::round(result / yaw_snap_step_degrees) * yaw_snap_step_degrees;
    }
    if (yaw_limits_enabled) {
        float low = godot::Math::min(min_yaw_degrees, max_yaw_degrees);
        float high = godot::Math::max(min_yaw_degrees, max_yaw_degrees);
        result = godot::Math::clamp(result, low, high);
    }
    return result;
}

godot::Transform3D GameplayPerspective::compute_camera_rig_transform(
    const godot::Vector3 &p_target_origin,
    float p_yaw_deg,
    float p_pitch_deg,
    bool p_is_aiming
) const {
    float eff_yaw = godot::Math::deg_to_rad(clamp_yaw_degrees(p_yaw_deg));
    float eff_pitch = godot::Math::deg_to_rad(clamp_pitch_degrees(p_pitch_deg));
    float eff_roll = godot::Math::deg_to_rad(default_roll_degrees);

    godot::Basis rig_basis = godot::Basis::from_euler(godot::Vector3(eff_pitch, eff_yaw, eff_roll));
    godot::Vector3 pivot_world = p_target_origin + pivot_offset;
    godot::Vector3 local_cam = get_effective_camera_offset(p_is_aiming);
    local_cam.z += get_effective_arm_length(p_is_aiming);

    godot::Vector3 cam_world = pivot_world + rig_basis.xform(local_cam);
    return godot::Transform3D(rig_basis, cam_world);
}

godot::Ref<GameplayPerspective> GameplayPerspective::interpolate_with(
    const godot::Ref<GameplayPerspective> &p_target,
    float p_weight
) const {
    godot::Ref<GameplayPerspective> blended;
    blended.instantiate();
    blended->apply_perspective(const_cast<GameplayPerspective *>(this));

    if (!p_target.is_valid()) {
        return blended;
    }

    float t = godot::Math::clamp(p_weight, 0.0f, 1.0f);
    if (t >= 0.5f) {
        blended->apply_perspective(p_target);
    }

    blended->set_perspective_type(
        t < 0.5f ? perspective_type : p_target->get_perspective_type()
    );
    blended->set_projection_mode(
        t < 0.5f ? projection_mode : p_target->get_projection_mode()
    );

    blended->set_fov(godot::Math::lerp(fov, p_target->get_fov(), t));
    blended->set_aim_fov(godot::Math::lerp(aim_fov, p_target->get_aim_fov(), t));
    blended->set_orthogonal_size(godot::Math::lerp(orthogonal_size, p_target->get_orthogonal_size(), t));
    blended->set_aim_orthogonal_size(godot::Math::lerp(aim_orthogonal_size, p_target->get_aim_orthogonal_size(), t));
    blended->set_near_clip(godot::Math::lerp(near_clip, p_target->get_near_clip(), t));
    blended->set_far_clip(godot::Math::lerp(far_clip, p_target->get_far_clip(), t));
    blended->set_frustum_offset(frustum_offset.lerp(p_target->get_frustum_offset(), t));
    blended->set_viewport_zoom_2d(viewport_zoom_2d.lerp(p_target->get_viewport_zoom_2d(), t));

    blended->set_pivot_offset(pivot_offset.lerp(p_target->get_pivot_offset(), t));
    blended->set_camera_local_offset(camera_local_offset.lerp(p_target->get_camera_local_offset(), t));
    blended->set_aim_camera_local_offset(aim_camera_local_offset.lerp(p_target->get_aim_camera_local_offset(), t));
    blended->set_arm_length(godot::Math::lerp(arm_length, p_target->get_arm_length(), t));
    blended->set_aim_arm_length(godot::Math::lerp(aim_arm_length, p_target->get_aim_arm_length(), t));
    blended->set_min_arm_length(godot::Math::lerp(min_arm_length, p_target->get_min_arm_length(), t));
    blended->set_max_arm_length(godot::Math::lerp(max_arm_length, p_target->get_max_arm_length(), t));
    blended->set_spring_arm_margin(godot::Math::lerp(spring_arm_margin, p_target->get_spring_arm_margin(), t));

    blended->set_default_pitch_degrees(godot::Math::lerp(default_pitch_degrees, p_target->get_default_pitch_degrees(), t));
    blended->set_default_yaw_degrees(godot::Math::lerp(default_yaw_degrees, p_target->get_default_yaw_degrees(), t));
    blended->set_default_roll_degrees(godot::Math::lerp(default_roll_degrees, p_target->get_default_roll_degrees(), t));
    blended->set_min_pitch_degrees(godot::Math::lerp(min_pitch_degrees, p_target->get_min_pitch_degrees(), t));
    blended->set_max_pitch_degrees(godot::Math::lerp(max_pitch_degrees, p_target->get_max_pitch_degrees(), t));

    blended->set_position_follow_smoothing(godot::Math::lerp(position_follow_smoothing, p_target->get_position_follow_smoothing(), t));
    blended->set_rotation_smoothing(godot::Math::lerp(rotation_smoothing, p_target->get_rotation_smoothing(), t));
    blended->set_fov_transition_speed(godot::Math::lerp(fov_transition_speed, p_target->get_fov_transition_speed(), t));
    blended->set_look_sensitivity_x(godot::Math::lerp(look_sensitivity_x, p_target->get_look_sensitivity_x(), t));
    blended->set_look_sensitivity_y(godot::Math::lerp(look_sensitivity_y, p_target->get_look_sensitivity_y(), t));
    blended->set_aim_sensitivity_multiplier(godot::Math::lerp(aim_sensitivity_multiplier, p_target->get_aim_sensitivity_multiplier(), t));

    return blended;
}

// --- Scene & Resource Application ---

void GameplayPerspective::apply_perspective(const godot::Ref<GameplayPerspective> &p_other) {
    if (!p_other.is_valid()) {
        return;
    }

    GameplayResource::apply_resource(p_other);

    perspective_type = p_other->get_perspective_type();
    projection_mode = p_other->get_projection_mode();
    fov = p_other->get_fov();
    aim_fov = p_other->get_aim_fov();
    orthogonal_size = p_other->get_orthogonal_size();
    aim_orthogonal_size = p_other->get_aim_orthogonal_size();
    near_clip = p_other->get_near_clip();
    far_clip = p_other->get_far_clip();
    frustum_offset = p_other->get_frustum_offset();
    viewport_zoom_2d = p_other->get_viewport_zoom_2d();

    pivot_offset = p_other->get_pivot_offset();
    camera_local_offset = p_other->get_camera_local_offset();
    aim_camera_local_offset = p_other->get_aim_camera_local_offset();
    shoulder_side = p_other->get_shoulder_side();
    allow_shoulder_swap = p_other->get_allow_shoulder_swap();
    arm_length = p_other->get_arm_length();
    aim_arm_length = p_other->get_aim_arm_length();
    min_arm_length = p_other->get_min_arm_length();
    max_arm_length = p_other->get_max_arm_length();
    spring_arm_collision_enabled = p_other->get_spring_arm_collision_enabled();
    spring_arm_collision_mask = p_other->get_spring_arm_collision_mask();
    spring_arm_margin = p_other->get_spring_arm_margin();

    default_pitch_degrees = p_other->get_default_pitch_degrees();
    default_yaw_degrees = p_other->get_default_yaw_degrees();
    default_roll_degrees = p_other->get_default_roll_degrees();
    min_pitch_degrees = p_other->get_min_pitch_degrees();
    max_pitch_degrees = p_other->get_max_pitch_degrees();
    lock_pitch = p_other->get_lock_pitch();
    lock_yaw = p_other->get_lock_yaw();
    yaw_limits_enabled = p_other->get_yaw_limits_enabled();
    min_yaw_degrees = p_other->get_min_yaw_degrees();
    max_yaw_degrees = p_other->get_max_yaw_degrees();
    yaw_snap_step_degrees = p_other->get_yaw_snap_step_degrees();

    position_follow_smoothing = p_other->get_position_follow_smoothing();
    rotation_smoothing = p_other->get_rotation_smoothing();
    fov_transition_speed = p_other->get_fov_transition_speed();
    perspective_blend_duration = p_other->get_perspective_blend_duration();
    look_sensitivity_x = p_other->get_look_sensitivity_x();
    look_sensitivity_y = p_other->get_look_sensitivity_y();
    aim_sensitivity_multiplier = p_other->get_aim_sensitivity_multiplier();
    invert_look_x = p_other->get_invert_look_x();
    invert_look_y = p_other->get_invert_look_y();

    movement_alignment = p_other->get_movement_alignment();
    orient_player_to_camera = p_other->get_orient_player_to_camera();
    orient_player_to_movement = p_other->get_orient_player_to_movement();
    orient_player_on_aim = p_other->get_orient_player_on_aim();
    player_mesh_visible = p_other->get_player_mesh_visible();
    first_person_hide_mesh_distance = p_other->get_first_person_hide_mesh_distance();
    show_aim_reticle = p_other->get_show_aim_reticle();
    occlude_environment_between_camera_and_player = p_other->get_occlude_environment_between_camera_and_player();
    player_group_tag = p_other->get_player_group_tag();

    _sync_perspective_option_tags();
    emit_changed();
}

void GameplayPerspective::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    godot::Ref<GameplayPerspective> persp_other = p_other;
    if (persp_other.is_valid()) {
        apply_perspective(persp_other);
    } else {
        GameplayResource::apply_resource(p_other);
    }
}

void GameplayPerspective::apply_to_camera_rig(
    godot::Node3D *p_pivot,
    godot::SpringArm3D *p_spring_arm,
    godot::Camera3D *p_camera,
    bool p_is_aiming
) const {
    if (!get_enabled()) {
        return;
    }

    if (p_pivot) {
        p_pivot->set_position(pivot_offset);
        if (lock_pitch || lock_yaw) {
            godot::Vector3 rot_deg = p_pivot->get_rotation_degrees();
            if (lock_pitch) {
                rot_deg.x = default_pitch_degrees;
            }
            if (lock_yaw) {
                rot_deg.y = default_yaw_degrees;
            }
            rot_deg.z = default_roll_degrees;
            p_pivot->set_rotation_degrees(rot_deg);
        }
    }

    if (p_spring_arm) {
        p_spring_arm->set_length(get_effective_arm_length(p_is_aiming));
        p_spring_arm->set_collision_mask(spring_arm_collision_enabled ? spring_arm_collision_mask : 0);
        p_spring_arm->set_margin(spring_arm_margin);
    }

    if (p_camera) {
        p_camera->set_position(get_effective_camera_offset(p_is_aiming));
        switch (projection_mode) {
            case PerspectiveProjectionMode::PROJECTION_ORTHOGONAL:
                p_camera->set_orthogonal(
                    get_effective_orthogonal_size(p_is_aiming),
                    near_clip,
                    far_clip
                );
                break;
            case PerspectiveProjectionMode::PROJECTION_FRUSTUM:
                p_camera->set_frustum(
                    get_effective_orthogonal_size(p_is_aiming),
                    frustum_offset,
                    near_clip,
                    far_clip
                );
                break;
            case PerspectiveProjectionMode::PROJECTION_PERSPECTIVE:
            default:
                p_camera->set_perspective(
                    get_effective_fov(p_is_aiming),
                    near_clip,
                    far_clip
                );
                break;
        }
    }
}

void GameplayPerspective::apply_to_camera_2d(godot::Camera2D *p_camera_2d, bool p_is_aiming) const {
    if (!p_camera_2d || !get_enabled()) {
        return;
    }

    godot::Vector2 eff_zoom = viewport_zoom_2d;
    if (p_is_aiming && fov > 0.001f) {
        float zoom_ratio = fov / godot::Math::max(1.0f, aim_fov);
        eff_zoom *= zoom_ratio;
    }
    p_camera_2d->set_zoom(eff_zoom);

    godot::Vector3 eff_offset = get_effective_camera_offset(p_is_aiming);
    p_camera_2d->set_offset(godot::Vector2(eff_offset.x, -eff_offset.y));

    bool smooth_enabled = position_follow_smoothing > 0.001f;
    p_camera_2d->set_position_smoothing_enabled(smooth_enabled);
    if (smooth_enabled) {
        p_camera_2d->set_position_smoothing_speed(position_follow_smoothing);
    }
}

godot::TypedArray<GameplayRule> GameplayPerspective::collect_rules() const {
    godot::TypedArray<GameplayRule> collected = GameplayResource::collect_rules();
    if (!get_enabled() || player_group_tag.is_empty()) {
        return collected;
    }

    // 1. Cascade camera FOV onto matching Player GameEntities if they expose `camera_fov`
    {
        godot::Ref<GameplayRule> fov_rule;
        fov_rule.instantiate();
        fov_rule->set_title("Perspective_CameraFOV");
        fov_rule->set_scope(RuleScope::DESCENDANTS);
        fov_rule->set_target_class(godot::StringName("GameEntity"));
        fov_rule->set_target_title_or_group(player_group_tag);
        fov_rule->set_target_property(godot::StringName("camera_fov"));
        fov_rule->set_operation(RuleOp::SET);
        fov_rule->set_value(fov);
        fov_rule->set_priority(40);
        collected.append(fov_rule);
    }

    // 2. Cascade camera arm length onto matching Player GameEntities if they expose `camera_arm_length`
    {
        godot::Ref<GameplayRule> arm_rule;
        arm_rule.instantiate();
        arm_rule->set_title("Perspective_CameraArmLength");
        arm_rule->set_scope(RuleScope::DESCENDANTS);
        arm_rule->set_target_class(godot::StringName("GameEntity"));
        arm_rule->set_target_title_or_group(player_group_tag);
        arm_rule->set_target_property(godot::StringName("camera_arm_length"));
        arm_rule->set_operation(RuleOp::SET);
        arm_rule->set_value(arm_length);
        arm_rule->set_priority(40);
        collected.append(arm_rule);
    }

    return collected;
}

godot::Dictionary GameplayPerspective::save_resource() const {
    godot::Dictionary data = GameplayResource::save_resource();

    data["perspective_type"] = static_cast<int>(perspective_type);
    data["projection_mode"] = static_cast<int>(projection_mode);
    data["fov"] = fov;
    data["aim_fov"] = aim_fov;
    data["orthogonal_size"] = orthogonal_size;
    data["aim_orthogonal_size"] = aim_orthogonal_size;
    data["near_clip"] = near_clip;
    data["far_clip"] = far_clip;
    data["frustum_offset"] = frustum_offset;
    data["viewport_zoom_2d"] = viewport_zoom_2d;

    data["pivot_offset"] = pivot_offset;
    data["camera_local_offset"] = camera_local_offset;
    data["aim_camera_local_offset"] = aim_camera_local_offset;
    data["shoulder_side"] = static_cast<int>(shoulder_side);
    data["allow_shoulder_swap"] = allow_shoulder_swap;
    data["arm_length"] = arm_length;
    data["aim_arm_length"] = aim_arm_length;
    data["min_arm_length"] = min_arm_length;
    data["max_arm_length"] = max_arm_length;
    data["spring_arm_collision_enabled"] = spring_arm_collision_enabled;
    data["spring_arm_collision_mask"] = spring_arm_collision_mask;
    data["spring_arm_margin"] = spring_arm_margin;

    data["default_pitch_degrees"] = default_pitch_degrees;
    data["default_yaw_degrees"] = default_yaw_degrees;
    data["default_roll_degrees"] = default_roll_degrees;
    data["min_pitch_degrees"] = min_pitch_degrees;
    data["max_pitch_degrees"] = max_pitch_degrees;
    data["lock_pitch"] = lock_pitch;
    data["lock_yaw"] = lock_yaw;
    data["yaw_limits_enabled"] = yaw_limits_enabled;
    data["min_yaw_degrees"] = min_yaw_degrees;
    data["max_yaw_degrees"] = max_yaw_degrees;
    data["yaw_snap_step_degrees"] = yaw_snap_step_degrees;

    data["position_follow_smoothing"] = position_follow_smoothing;
    data["rotation_smoothing"] = rotation_smoothing;
    data["fov_transition_speed"] = fov_transition_speed;
    data["perspective_blend_duration"] = perspective_blend_duration;
    data["look_sensitivity_x"] = look_sensitivity_x;
    data["look_sensitivity_y"] = look_sensitivity_y;
    data["aim_sensitivity_multiplier"] = aim_sensitivity_multiplier;
    data["invert_look_x"] = invert_look_x;
    data["invert_look_y"] = invert_look_y;

    data["movement_alignment"] = static_cast<int>(movement_alignment);
    data["orient_player_to_camera"] = orient_player_to_camera;
    data["orient_player_to_movement"] = orient_player_to_movement;
    data["orient_player_on_aim"] = orient_player_on_aim;
    data["player_mesh_visible"] = player_mesh_visible;
    data["first_person_hide_mesh_distance"] = first_person_hide_mesh_distance;
    data["show_aim_reticle"] = show_aim_reticle;
    data["occlude_environment_between_camera_and_player"] = occlude_environment_between_camera_and_player;
    data["player_group_tag"] = godot::String(player_group_tag);

    return data;
}

void GameplayPerspective::load_resource(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    if (p_data.has("perspective_type")) {
        perspective_type = static_cast<PerspectiveType>(static_cast<int>(p_data["perspective_type"]));
    }
    if (p_data.has("projection_mode")) {
        projection_mode = static_cast<PerspectiveProjectionMode>(static_cast<int>(p_data["projection_mode"]));
    }
    if (p_data.has("fov")) set_fov(p_data["fov"]);
    if (p_data.has("aim_fov")) set_aim_fov(p_data["aim_fov"]);
    if (p_data.has("orthogonal_size")) set_orthogonal_size(p_data["orthogonal_size"]);
    if (p_data.has("aim_orthogonal_size")) set_aim_orthogonal_size(p_data["aim_orthogonal_size"]);
    if (p_data.has("near_clip")) set_near_clip(p_data["near_clip"]);
    if (p_data.has("far_clip")) set_far_clip(p_data["far_clip"]);
    if (p_data.has("frustum_offset")) set_frustum_offset(p_data["frustum_offset"]);
    if (p_data.has("viewport_zoom_2d")) set_viewport_zoom_2d(p_data["viewport_zoom_2d"]);

    if (p_data.has("pivot_offset")) set_pivot_offset(p_data["pivot_offset"]);
    if (p_data.has("camera_local_offset")) set_camera_local_offset(p_data["camera_local_offset"]);
    if (p_data.has("aim_camera_local_offset")) set_aim_camera_local_offset(p_data["aim_camera_local_offset"]);
    if (p_data.has("shoulder_side")) {
        shoulder_side = static_cast<ShoulderSide>(static_cast<int>(p_data["shoulder_side"]));
    }
    if (p_data.has("allow_shoulder_swap")) allow_shoulder_swap = p_data["allow_shoulder_swap"];
    if (p_data.has("arm_length")) set_arm_length(p_data["arm_length"]);
    if (p_data.has("aim_arm_length")) set_aim_arm_length(p_data["aim_arm_length"]);
    if (p_data.has("min_arm_length")) set_min_arm_length(p_data["min_arm_length"]);
    if (p_data.has("max_arm_length")) set_max_arm_length(p_data["max_arm_length"]);
    if (p_data.has("spring_arm_collision_enabled")) spring_arm_collision_enabled = p_data["spring_arm_collision_enabled"];
    if (p_data.has("spring_arm_collision_mask")) {
        spring_arm_collision_mask = static_cast<uint32_t>(static_cast<int64_t>(p_data["spring_arm_collision_mask"]));
    }
    if (p_data.has("spring_arm_margin")) set_spring_arm_margin(p_data["spring_arm_margin"]);

    if (p_data.has("default_pitch_degrees")) set_default_pitch_degrees(p_data["default_pitch_degrees"]);
    if (p_data.has("default_yaw_degrees")) set_default_yaw_degrees(p_data["default_yaw_degrees"]);
    if (p_data.has("default_roll_degrees")) set_default_roll_degrees(p_data["default_roll_degrees"]);
    if (p_data.has("min_pitch_degrees")) set_min_pitch_degrees(p_data["min_pitch_degrees"]);
    if (p_data.has("max_pitch_degrees")) set_max_pitch_degrees(p_data["max_pitch_degrees"]);
    if (p_data.has("lock_pitch")) lock_pitch = p_data["lock_pitch"];
    if (p_data.has("lock_yaw")) lock_yaw = p_data["lock_yaw"];
    if (p_data.has("yaw_limits_enabled")) yaw_limits_enabled = p_data["yaw_limits_enabled"];
    if (p_data.has("min_yaw_degrees")) set_min_yaw_degrees(p_data["min_yaw_degrees"]);
    if (p_data.has("max_yaw_degrees")) set_max_yaw_degrees(p_data["max_yaw_degrees"]);
    if (p_data.has("yaw_snap_step_degrees")) set_yaw_snap_step_degrees(p_data["yaw_snap_step_degrees"]);

    if (p_data.has("position_follow_smoothing")) set_position_follow_smoothing(p_data["position_follow_smoothing"]);
    if (p_data.has("rotation_smoothing")) set_rotation_smoothing(p_data["rotation_smoothing"]);
    if (p_data.has("fov_transition_speed")) set_fov_transition_speed(p_data["fov_transition_speed"]);
    if (p_data.has("perspective_blend_duration")) set_perspective_blend_duration(p_data["perspective_blend_duration"]);
    if (p_data.has("look_sensitivity_x")) set_look_sensitivity_x(p_data["look_sensitivity_x"]);
    if (p_data.has("look_sensitivity_y")) set_look_sensitivity_y(p_data["look_sensitivity_y"]);
    if (p_data.has("aim_sensitivity_multiplier")) set_aim_sensitivity_multiplier(p_data["aim_sensitivity_multiplier"]);
    if (p_data.has("invert_look_x")) invert_look_x = p_data["invert_look_x"];
    if (p_data.has("invert_look_y")) invert_look_y = p_data["invert_look_y"];

    if (p_data.has("movement_alignment")) {
        movement_alignment = static_cast<MovementAlignmentMode>(static_cast<int>(p_data["movement_alignment"]));
    }
    if (p_data.has("orient_player_to_camera")) orient_player_to_camera = p_data["orient_player_to_camera"];
    if (p_data.has("orient_player_to_movement")) orient_player_to_movement = p_data["orient_player_to_movement"];
    if (p_data.has("orient_player_on_aim")) orient_player_on_aim = p_data["orient_player_on_aim"];
    if (p_data.has("player_mesh_visible")) player_mesh_visible = p_data["player_mesh_visible"];
    if (p_data.has("first_person_hide_mesh_distance")) set_first_person_hide_mesh_distance(p_data["first_person_hide_mesh_distance"]);
    if (p_data.has("show_aim_reticle")) show_aim_reticle = p_data["show_aim_reticle"];
    if (p_data.has("occlude_environment_between_camera_and_player")) {
        occlude_environment_between_camera_and_player = p_data["occlude_environment_between_camera_and_player"];
    }
    if (p_data.has("player_group_tag")) {
        player_group_tag = godot::StringName(godot::String(p_data["player_group_tag"]));
    }

    _sync_perspective_option_tags();
    emit_changed();
}

} // namespace ideam::godot_ext