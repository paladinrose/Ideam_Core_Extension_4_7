#include "gameplay_control_map.h"

#include <cmath>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "../game_entities/game_agent.h"
#include "gameplay_perspective.h"
#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayControlMap::_bind_methods() {
    // --- Signals (preserving existing sequence lifecycle signals + unified input signals) ---
    ADD_SIGNAL(godot::MethodInfo("begun"));
    ADD_SIGNAL(godot::MethodInfo("timed_out"));
    ADD_SIGNAL(godot::MethodInfo("completed"));
    ADD_SIGNAL(godot::MethodInfo("failed"));

    ADD_SIGNAL(godot::MethodInfo(
        "device_family_changed",
        godot::PropertyInfo(godot::Variant::INT, "new_family"),
        godot::PropertyInfo(godot::Variant::INT, "previous_family")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "action_triggered",
        godot::PropertyInfo(godot::Variant::STRING_NAME, "action"),
        godot::PropertyInfo(godot::Variant::FLOAT, "strength")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "action_released",
        godot::PropertyInfo(godot::Variant::STRING_NAME, "action")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "custom_packet_received",
        godot::PropertyInfo(godot::Variant::DICTIONARY, "channel_values")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "vr_gesture_detected",
        godot::PropertyInfo(godot::Variant::STRING_NAME, "gesture"),
        godot::PropertyInfo(godot::Variant::INT, "hand"),
        godot::PropertyInfo(godot::Variant::FLOAT, "speed")
    ));

    // --- Enums ---
    BIND_ENUM_CONSTANT(ControlDeviceFamily::DEVICE_AUTO_DETECT);
    BIND_ENUM_CONSTANT(ControlDeviceFamily::DEVICE_KEYBOARD_MOUSE);
    BIND_ENUM_CONSTANT(ControlDeviceFamily::DEVICE_GAMEPAD);
    BIND_ENUM_CONSTANT(ControlDeviceFamily::DEVICE_CUSTOM_USB_HID);
    BIND_ENUM_CONSTANT(ControlDeviceFamily::DEVICE_VR_XR);
    BIND_ENUM_CONSTANT(ControlDeviceFamily::DEVICE_TOUCH_VIRTUAL);

    BIND_ENUM_CONSTANT(ControlContextMode::CONTROL_CONTEXT_EXPLORATION);
    BIND_ENUM_CONSTANT(ControlContextMode::CONTROL_CONTEXT_COMBAT);
    BIND_ENUM_CONSTANT(ControlContextMode::CONTROL_CONTEXT_VEHICLE_MOUNT);
    BIND_ENUM_CONSTANT(ControlContextMode::CONTROL_CONTEXT_PUZZLE_INSPECT);
    BIND_ENUM_CONSTANT(ControlContextMode::CONTROL_CONTEXT_UI_MENU);
    BIND_ENUM_CONSTANT(ControlContextMode::CONTROL_CONTEXT_VR_IMMERSIVE);
    BIND_ENUM_CONSTANT(ControlContextMode::CONTROL_CONTEXT_CUSTOM);

    BIND_ENUM_CONSTANT(DeadzoneCurveShape::DEADZONE_RADIAL_SCALED);
    BIND_ENUM_CONSTANT(DeadzoneCurveShape::DEADZONE_AXIAL_INDEPENDENT);
    BIND_ENUM_CONSTANT(DeadzoneCurveShape::DEADZONE_CUBIC_PRECISION);

    BIND_ENUM_CONSTANT(VRMovementLocomotionMode::VR_LOCOMOTION_CONTINUOUS_HEAD);
    BIND_ENUM_CONSTANT(VRMovementLocomotionMode::VR_LOCOMOTION_CONTINUOUS_HAND);
    BIND_ENUM_CONSTANT(VRMovementLocomotionMode::VR_LOCOMOTION_TELEPORT_BLINK);
    BIND_ENUM_CONSTANT(VRMovementLocomotionMode::VR_LOCOMOTION_ARM_SWING);

    BIND_ENUM_CONSTANT(VRTurnMode::VR_TURN_SNAP);
    BIND_ENUM_CONSTANT(VRTurnMode::VR_TURN_SMOOTH);
    BIND_ENUM_CONSTANT(VRTurnMode::VR_TURN_PHYSICAL_ONLY);

    BIND_ENUM_CONSTANT(CustomUSBProtocolFormat::USB_PROTOCOL_KEY_VALUE_ASCII);
    BIND_ENUM_CONSTANT(CustomUSBProtocolFormat::USB_PROTOCOL_CSV_CHANNELS);
    BIND_ENUM_CONSTANT(CustomUSBProtocolFormat::USB_PROTOCOL_JSON_OBJECT);
    BIND_ENUM_CONSTANT(CustomUSBProtocolFormat::USB_PROTOCOL_RAW_HID_BYTES);

    // --- Static Math Helpers ---
    godot::ClassDB::bind_static_method(
        "GameplayControlMap",
        godot::D_METHOD("apply_stick_deadzone", "raw", "inner_deadzone", "outer_deadzone", "curve", "response_exponent"),
        &GameplayControlMap::apply_stick_deadzone,
        DEFVAL(0.95f),
        DEFVAL(DeadzoneCurveShape::DEADZONE_RADIAL_SCALED),
        DEFVAL(1.0f)
    );
    godot::ClassDB::bind_static_method(
        "GameplayControlMap",
        godot::D_METHOD("apply_axis_deadzone", "raw_value", "inner_deadzone", "outer_deadzone"),
        &GameplayControlMap::apply_axis_deadzone,
        DEFVAL(0.95f)
    );

    // --- 0. Context, Sequence & Device Routing ---
    ADD_GROUP("Control Context & Sequence", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_root_input", "input"), &GameplayControlMap::set_root_input);
    godot::ClassDB::bind_method(godot::D_METHOD("get_root_input"), &GameplayControlMap::get_root_input);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "root_input"), "set_root_input", "get_root_input");

    godot::ClassDB::bind_method(godot::D_METHOD("set_root_action", "action"), &GameplayControlMap::set_root_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_root_action"), &GameplayControlMap::get_root_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "root_action"), "set_root_action", "get_root_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_priority", "priority"), &GameplayControlMap::set_priority);
    godot::ClassDB::bind_method(godot::D_METHOD("get_priority"), &GameplayControlMap::get_priority);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "priority"), "set_priority", "get_priority");

    godot::ClassDB::bind_method(godot::D_METHOD("set_timeout", "timeout"), &GameplayControlMap::set_timeout);
    godot::ClassDB::bind_method(godot::D_METHOD("get_timeout"), &GameplayControlMap::get_timeout);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "timeout", godot::PROPERTY_HINT_RANGE, "0.0,60.0,0.05"),
        "set_timeout",
        "get_timeout"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_context_mode", "mode"), &GameplayControlMap::set_context_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_context_mode"), &GameplayControlMap::get_context_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_context_preset", "mode"), &GameplayControlMap::apply_context_preset);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "context_mode",
            godot::PROPERTY_HINT_ENUM,
            "Exploration,Combat,Vehicle / Mount,Puzzle / Inspect,UI / Menu,VR Immersive,Custom"
        ),
        "set_context_mode",
        "get_context_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_preferred_device_family", "family"), &GameplayControlMap::set_preferred_device_family);
    godot::ClassDB::bind_method(godot::D_METHOD("get_preferred_device_family"), &GameplayControlMap::get_preferred_device_family);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "preferred_device_family",
            godot::PROPERTY_HINT_ENUM,
            "Auto Detect,Keyboard & Mouse,Gamepad / Console,Custom USB / Arduino,VR / XR,Touch / Virtual"
        ),
        "set_preferred_device_family",
        "get_preferred_device_family"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_active_device_family", "family"), &GameplayControlMap::set_active_device_family);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_device_family"), &GameplayControlMap::get_active_device_family);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "active_device_family",
            godot::PROPERTY_HINT_ENUM,
            "Auto Detect,Keyboard & Mouse,Gamepad / Console,Custom USB / Arduino,VR / XR,Touch / Virtual"
        ),
        "set_active_device_family",
        "get_active_device_family"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_device_id", "device_id"), &GameplayControlMap::set_device_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_device_id"), &GameplayControlMap::get_device_id);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "device_id", godot::PROPERTY_HINT_RANGE, "-1,16,1"),
        "set_device_id",
        "get_device_id"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_input_buffer_window", "window"), &GameplayControlMap::set_input_buffer_window);
    godot::ClassDB::bind_method(godot::D_METHOD("get_input_buffer_window"), &GameplayControlMap::get_input_buffer_window);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "input_buffer_window", godot::PROPERTY_HINT_RANGE, "0.0,1.0,0.01"),
        "set_input_buffer_window",
        "get_input_buffer_window"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_mode_transition_smoothing", "smoothing"), &GameplayControlMap::set_mode_transition_smoothing);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mode_transition_smoothing"), &GameplayControlMap::get_mode_transition_smoothing);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "mode_transition_smoothing", godot::PROPERTY_HINT_RANGE, "0.0,60.0,0.5"),
        "set_mode_transition_smoothing",
        "get_mode_transition_smoothing"
    );

    // --- 1. Unified Semantic Action Names & Agent Routing ---
    ADD_GROUP("Semantic Action Mappings", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_move_forward_action", "action"), &GameplayControlMap::set_move_forward_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_move_forward_action"), &GameplayControlMap::get_move_forward_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "move_forward_action"), "set_move_forward_action", "get_move_forward_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_move_backward_action", "action"), &GameplayControlMap::set_move_backward_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_move_backward_action"), &GameplayControlMap::get_move_backward_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "move_backward_action"), "set_move_backward_action", "get_move_backward_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_move_left_action", "action"), &GameplayControlMap::set_move_left_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_move_left_action"), &GameplayControlMap::get_move_left_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "move_left_action"), "set_move_left_action", "get_move_left_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_move_right_action", "action"), &GameplayControlMap::set_move_right_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_move_right_action"), &GameplayControlMap::get_move_right_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "move_right_action"), "set_move_right_action", "get_move_right_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_look_up_action", "action"), &GameplayControlMap::set_look_up_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_look_up_action"), &GameplayControlMap::get_look_up_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "look_up_action"), "set_look_up_action", "get_look_up_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_look_down_action", "action"), &GameplayControlMap::set_look_down_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_look_down_action"), &GameplayControlMap::get_look_down_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "look_down_action"), "set_look_down_action", "get_look_down_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_look_left_action", "action"), &GameplayControlMap::set_look_left_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_look_left_action"), &GameplayControlMap::get_look_left_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "look_left_action"), "set_look_left_action", "get_look_left_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_look_right_action", "action"), &GameplayControlMap::set_look_right_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_look_right_action"), &GameplayControlMap::get_look_right_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "look_right_action"), "set_look_right_action", "get_look_right_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_primary_action", "action"), &GameplayControlMap::set_primary_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_primary_action"), &GameplayControlMap::get_primary_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "primary_action"), "set_primary_action", "get_primary_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_secondary_action", "action"), &GameplayControlMap::set_secondary_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_secondary_action"), &GameplayControlMap::get_secondary_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "secondary_action"), "set_secondary_action", "get_secondary_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_interact_action", "action"), &GameplayControlMap::set_interact_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_interact_action"), &GameplayControlMap::get_interact_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "interact_action"), "set_interact_action", "get_interact_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_jump_action", "action"), &GameplayControlMap::set_jump_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_jump_action"), &GameplayControlMap::get_jump_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "jump_action"), "set_jump_action", "get_jump_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_sprint_action", "action"), &GameplayControlMap::set_sprint_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_sprint_action"), &GameplayControlMap::get_sprint_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "sprint_action"), "set_sprint_action", "get_sprint_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_crouch_action", "action"), &GameplayControlMap::set_crouch_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_crouch_action"), &GameplayControlMap::get_crouch_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "crouch_action"), "set_crouch_action", "get_crouch_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_pause_action", "action"), &GameplayControlMap::set_pause_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_pause_action"), &GameplayControlMap::get_pause_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "pause_action"), "set_pause_action", "get_pause_action");

    godot::ClassDB::bind_method(godot::D_METHOD("set_action_to_agent_action_map", "map"), &GameplayControlMap::set_action_to_agent_action_map);
    godot::ClassDB::bind_method(godot::D_METHOD("get_action_to_agent_action_map"), &GameplayControlMap::get_action_to_agent_action_map);
    godot::ClassDB::bind_method(godot::D_METHOD("map_input_to_agent_action", "input_action", "agent_action_title_or_id"), &GameplayControlMap::map_input_to_agent_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "action_to_agent_action_map"), "set_action_to_agent_action_map", "get_action_to_agent_action_map");

    godot::ClassDB::bind_method(godot::D_METHOD("set_action_remap_table", "table"), &GameplayControlMap::set_action_remap_table);
    godot::ClassDB::bind_method(godot::D_METHOD("get_action_remap_table"), &GameplayControlMap::get_action_remap_table);
    godot::ClassDB::bind_method(godot::D_METHOD("remap_action", "semantic_action", "target_input_action"), &GameplayControlMap::remap_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "action_remap_table"), "set_action_remap_table", "get_action_remap_table");

    // --- 2. Keyboard & Mouse Settings ---
    ADD_GROUP("Keyboard & Mouse", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_mouse_sensitivity", "sensitivity"), &GameplayControlMap::set_mouse_sensitivity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mouse_sensitivity"), &GameplayControlMap::get_mouse_sensitivity);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR2, "mouse_sensitivity"), "set_mouse_sensitivity", "get_mouse_sensitivity");

    godot::ClassDB::bind_method(godot::D_METHOD("set_mouse_raw_input", "raw"), &GameplayControlMap::set_mouse_raw_input);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mouse_raw_input"), &GameplayControlMap::get_mouse_raw_input);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "mouse_raw_input"), "set_mouse_raw_input", "get_mouse_raw_input");

    godot::ClassDB::bind_method(godot::D_METHOD("set_mouse_smoothing_factor", "factor"), &GameplayControlMap::set_mouse_smoothing_factor);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mouse_smoothing_factor"), &GameplayControlMap::get_mouse_smoothing_factor);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "mouse_smoothing_factor", godot::PROPERTY_HINT_RANGE, "0.0,0.95,0.05"),
        "set_mouse_smoothing_factor",
        "get_mouse_smoothing_factor"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_mouse_capture_in_gameplay", "capture"), &GameplayControlMap::set_mouse_capture_in_gameplay);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mouse_capture_in_gameplay"), &GameplayControlMap::get_mouse_capture_in_gameplay);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "mouse_capture_in_gameplay"), "set_mouse_capture_in_gameplay", "get_mouse_capture_in_gameplay");

    godot::ClassDB::bind_method(godot::D_METHOD("set_invert_mouse_x", "invert"), &GameplayControlMap::set_invert_mouse_x);
    godot::ClassDB::bind_method(godot::D_METHOD("get_invert_mouse_x"), &GameplayControlMap::get_invert_mouse_x);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "invert_mouse_x"), "set_invert_mouse_x", "get_invert_mouse_x");

    godot::ClassDB::bind_method(godot::D_METHOD("set_invert_mouse_y", "invert"), &GameplayControlMap::set_invert_mouse_y);
    godot::ClassDB::bind_method(godot::D_METHOD("get_invert_mouse_y"), &GameplayControlMap::get_invert_mouse_y);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "invert_mouse_y"), "set_invert_mouse_y", "get_invert_mouse_y");

    // --- 3. Console Controller & Gamepad Settings ---
    ADD_GROUP("Gamepad & Controller", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_left_stick_deadzone_inner", "deadzone"), &GameplayControlMap::set_left_stick_deadzone_inner);
    godot::ClassDB::bind_method(godot::D_METHOD("get_left_stick_deadzone_inner"), &GameplayControlMap::get_left_stick_deadzone_inner);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "left_stick_deadzone_inner", godot::PROPERTY_HINT_RANGE, "0.0,0.8,0.01"),
        "set_left_stick_deadzone_inner",
        "get_left_stick_deadzone_inner"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_left_stick_deadzone_outer", "deadzone"), &GameplayControlMap::set_left_stick_deadzone_outer);
    godot::ClassDB::bind_method(godot::D_METHOD("get_left_stick_deadzone_outer"), &GameplayControlMap::get_left_stick_deadzone_outer);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "left_stick_deadzone_outer", godot::PROPERTY_HINT_RANGE, "0.2,1.0,0.01"),
        "set_left_stick_deadzone_outer",
        "get_left_stick_deadzone_outer"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_right_stick_deadzone_inner", "deadzone"), &GameplayControlMap::set_right_stick_deadzone_inner);
    godot::ClassDB::bind_method(godot::D_METHOD("get_right_stick_deadzone_inner"), &GameplayControlMap::get_right_stick_deadzone_inner);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "right_stick_deadzone_inner", godot::PROPERTY_HINT_RANGE, "0.0,0.8,0.01"),
        "set_right_stick_deadzone_inner",
        "get_right_stick_deadzone_inner"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_right_stick_deadzone_outer", "deadzone"), &GameplayControlMap::set_right_stick_deadzone_outer);
    godot::ClassDB::bind_method(godot::D_METHOD("get_right_stick_deadzone_outer"), &GameplayControlMap::get_right_stick_deadzone_outer);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "right_stick_deadzone_outer", godot::PROPERTY_HINT_RANGE, "0.2,1.0,0.01"),
        "set_right_stick_deadzone_outer",
        "get_right_stick_deadzone_outer"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_trigger_deadzone", "deadzone"), &GameplayControlMap::set_trigger_deadzone);
    godot::ClassDB::bind_method(godot::D_METHOD("get_trigger_deadzone"), &GameplayControlMap::get_trigger_deadzone);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "trigger_deadzone", godot::PROPERTY_HINT_RANGE, "0.0,0.8,0.01"),
        "set_trigger_deadzone",
        "get_trigger_deadzone"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_trigger_hair_trigger_mode", "hair_trigger"), &GameplayControlMap::set_trigger_hair_trigger_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_trigger_hair_trigger_mode"), &GameplayControlMap::get_trigger_hair_trigger_mode);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "trigger_hair_trigger_mode"), "set_trigger_hair_trigger_mode", "get_trigger_hair_trigger_mode");

    godot::ClassDB::bind_method(godot::D_METHOD("set_deadzone_curve_shape", "shape"), &GameplayControlMap::set_deadzone_curve_shape);
    godot::ClassDB::bind_method(godot::D_METHOD("get_deadzone_curve_shape"), &GameplayControlMap::get_deadzone_curve_shape);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "deadzone_curve_shape",
            godot::PROPERTY_HINT_ENUM,
            "Radial Scaled,Axial Independent,Cubic Precision"
        ),
        "set_deadzone_curve_shape",
        "get_deadzone_curve_shape"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_gamepad_look_sensitivity", "sensitivity"), &GameplayControlMap::set_gamepad_look_sensitivity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gamepad_look_sensitivity"), &GameplayControlMap::get_gamepad_look_sensitivity);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::VECTOR2, "gamepad_look_sensitivity"), "set_gamepad_look_sensitivity", "get_gamepad_look_sensitivity");

    godot::ClassDB::bind_method(godot::D_METHOD("set_gamepad_response_exponent", "exponent"), &GameplayControlMap::set_gamepad_response_exponent);
    godot::ClassDB::bind_method(godot::D_METHOD("get_gamepad_response_exponent"), &GameplayControlMap::get_gamepad_response_exponent);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "gamepad_response_exponent", godot::PROPERTY_HINT_RANGE, "0.5,4.0,0.05"),
        "set_gamepad_response_exponent",
        "get_gamepad_response_exponent"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_invert_gamepad_x", "invert"), &GameplayControlMap::set_invert_gamepad_x);
    godot::ClassDB::bind_method(godot::D_METHOD("get_invert_gamepad_x"), &GameplayControlMap::get_invert_gamepad_x);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "invert_gamepad_x"), "set_invert_gamepad_x", "get_invert_gamepad_x");

    godot::ClassDB::bind_method(godot::D_METHOD("set_invert_gamepad_y", "invert"), &GameplayControlMap::set_invert_gamepad_y);
    godot::ClassDB::bind_method(godot::D_METHOD("get_invert_gamepad_y"), &GameplayControlMap::get_invert_gamepad_y);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "invert_gamepad_y"), "set_invert_gamepad_y", "get_invert_gamepad_y");

    godot::ClassDB::bind_method(godot::D_METHOD("set_swap_thumbsticks", "swap"), &GameplayControlMap::set_swap_thumbsticks);
    godot::ClassDB::bind_method(godot::D_METHOD("get_swap_thumbsticks"), &GameplayControlMap::get_swap_thumbsticks);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "swap_thumbsticks"), "set_swap_thumbsticks", "get_swap_thumbsticks");

    godot::ClassDB::bind_method(godot::D_METHOD("set_vibration_enabled", "enabled"), &GameplayControlMap::set_vibration_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vibration_enabled"), &GameplayControlMap::get_vibration_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "vibration_enabled"), "set_vibration_enabled", "get_vibration_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_vibration_strength", "strength"), &GameplayControlMap::set_vibration_strength);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vibration_strength"), &GameplayControlMap::get_vibration_strength);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "vibration_strength", godot::PROPERTY_HINT_RANGE, "0.0,2.0,0.05"),
        "set_vibration_strength",
        "get_vibration_strength"
    );

    godot::ClassDB::bind_method(
        godot::D_METHOD("trigger_haptic_pulse", "weak_magnitude", "strong_magnitude", "duration_sec"),
        &GameplayControlMap::trigger_haptic_pulse,
        DEFVAL(0.15f)
    );

    // --- 4. Arduino & Custom USB / Serial / HID Input ---
    ADD_GROUP("Custom USB & Arduino", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_usb_enabled", "enabled"), &GameplayControlMap::set_custom_usb_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_usb_enabled"), &GameplayControlMap::get_custom_usb_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "custom_usb_enabled"), "set_custom_usb_enabled", "get_custom_usb_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_usb_protocol", "protocol"), &GameplayControlMap::set_custom_usb_protocol);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_usb_protocol"), &GameplayControlMap::get_custom_usb_protocol);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "custom_usb_protocol",
            godot::PROPERTY_HINT_ENUM,
            "Key-Value ASCII (Arduino Serial),CSV Channels,JSON Object,Raw HID Bytes"
        ),
        "set_custom_usb_protocol",
        "get_custom_usb_protocol"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_usb_port_hint", "port"), &GameplayControlMap::set_custom_usb_port_hint);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_usb_port_hint"), &GameplayControlMap::get_custom_usb_port_hint);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "custom_usb_port_hint"), "set_custom_usb_port_hint", "get_custom_usb_port_hint");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_usb_baud_rate", "baud"), &GameplayControlMap::set_custom_usb_baud_rate);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_usb_baud_rate"), &GameplayControlMap::get_custom_usb_baud_rate);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "custom_usb_baud_rate"), "set_custom_usb_baud_rate", "get_custom_usb_baud_rate");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_usb_vendor_id", "vid"), &GameplayControlMap::set_custom_usb_vendor_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_usb_vendor_id"), &GameplayControlMap::get_custom_usb_vendor_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "custom_usb_vendor_id"), "set_custom_usb_vendor_id", "get_custom_usb_vendor_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_usb_product_id", "pid"), &GameplayControlMap::set_custom_usb_product_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_usb_product_id"), &GameplayControlMap::get_custom_usb_product_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "custom_usb_product_id"), "set_custom_usb_product_id", "get_custom_usb_product_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_adc_resolution_max", "max_value"), &GameplayControlMap::set_custom_adc_resolution_max);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_adc_resolution_max"), &GameplayControlMap::get_custom_adc_resolution_max);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "custom_adc_resolution_max", godot::PROPERTY_HINT_RANGE, "1,65535,1"),
        "set_custom_adc_resolution_max",
        "get_custom_adc_resolution_max"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_adc_center_value", "center"), &GameplayControlMap::set_custom_adc_center_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_adc_center_value"), &GameplayControlMap::get_custom_adc_center_value);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "custom_adc_center_value", godot::PROPERTY_HINT_RANGE, "0,65535,1"),
        "set_custom_adc_center_value",
        "get_custom_adc_center_value"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_usb_deadzone", "deadzone"), &GameplayControlMap::set_custom_usb_deadzone);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_usb_deadzone"), &GameplayControlMap::get_custom_usb_deadzone);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "custom_usb_deadzone", godot::PROPERTY_HINT_RANGE, "0.0,0.5,0.01"),
        "set_custom_usb_deadzone",
        "get_custom_usb_deadzone"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_channel_to_action_map", "map"), &GameplayControlMap::set_custom_channel_to_action_map);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_channel_to_action_map"), &GameplayControlMap::get_custom_channel_to_action_map);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "custom_channel_to_action_map"), "set_custom_channel_to_action_map", "get_custom_channel_to_action_map");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_channel_calibration", "calibration"), &GameplayControlMap::set_custom_channel_calibration);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_channel_calibration"), &GameplayControlMap::get_custom_channel_calibration);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "custom_channel_calibration"), "set_custom_channel_calibration", "get_custom_channel_calibration");

    godot::ClassDB::bind_method(godot::D_METHOD("feed_custom_serial_line", "line"), &GameplayControlMap::feed_custom_serial_line);
    godot::ClassDB::bind_method(
        godot::D_METHOD("feed_custom_hid_bytes", "bytes", "sixteen_bit_channels"),
        &GameplayControlMap::feed_custom_hid_bytes,
        DEFVAL(false)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("set_custom_channel_value", "channel", "value", "is_raw_adc"),
        &GameplayControlMap::set_custom_channel_value,
        DEFVAL(false)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_channel_value", "channel"), &GameplayControlMap::get_custom_channel_value);

    // --- 5. VR / OpenXR Tracked Controller Settings ---
    ADD_GROUP("VR & XR Controllers", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_input_enabled", "enabled"), &GameplayControlMap::set_vr_input_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_input_enabled"), &GameplayControlMap::get_vr_input_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "vr_input_enabled"), "set_vr_input_enabled", "get_vr_input_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_locomotion_mode", "mode"), &GameplayControlMap::set_vr_locomotion_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_locomotion_mode"), &GameplayControlMap::get_vr_locomotion_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "vr_locomotion_mode",
            godot::PROPERTY_HINT_ENUM,
            "Continuous Head-Oriented,Continuous Hand-Oriented,Teleport / Blink,Physical Arm-Swing"
        ),
        "set_vr_locomotion_mode",
        "get_vr_locomotion_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_turn_mode", "mode"), &GameplayControlMap::set_vr_turn_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_turn_mode"), &GameplayControlMap::get_vr_turn_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "vr_turn_mode",
            godot::PROPERTY_HINT_ENUM,
            "Snap Turn,Smooth Turn,Physical Turn Only"
        ),
        "set_vr_turn_mode",
        "get_vr_turn_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_snap_turn_degrees", "degrees"), &GameplayControlMap::set_vr_snap_turn_degrees);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_snap_turn_degrees"), &GameplayControlMap::get_vr_snap_turn_degrees);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "vr_snap_turn_degrees", godot::PROPERTY_HINT_RANGE, "15.0,90.0,5.0"),
        "set_vr_snap_turn_degrees",
        "get_vr_snap_turn_degrees"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_smooth_turn_speed", "speed"), &GameplayControlMap::set_vr_smooth_turn_speed);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_smooth_turn_speed"), &GameplayControlMap::get_vr_smooth_turn_speed);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "vr_smooth_turn_speed", godot::PROPERTY_HINT_RANGE, "15.0,360.0,5.0"),
        "set_vr_smooth_turn_speed",
        "get_vr_smooth_turn_speed"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_trigger_threshold", "threshold"), &GameplayControlMap::set_vr_trigger_threshold);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_trigger_threshold"), &GameplayControlMap::get_vr_trigger_threshold);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "vr_trigger_threshold", godot::PROPERTY_HINT_RANGE, "0.05,0.95,0.05"),
        "set_vr_trigger_threshold",
        "get_vr_trigger_threshold"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_grip_threshold", "threshold"), &GameplayControlMap::set_vr_grip_threshold);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_grip_threshold"), &GameplayControlMap::get_vr_grip_threshold);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "vr_grip_threshold", godot::PROPERTY_HINT_RANGE, "0.05,0.95,0.05"),
        "set_vr_grip_threshold",
        "get_vr_grip_threshold"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_dominant_hand", "hand"), &GameplayControlMap::set_vr_dominant_hand);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_dominant_hand"), &GameplayControlMap::get_vr_dominant_hand);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "vr_dominant_hand", godot::PROPERTY_HINT_ENUM, "Left Hand,Right Hand"),
        "set_vr_dominant_hand",
        "get_vr_dominant_hand"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_gesture_velocity_threshold", "threshold"), &GameplayControlMap::set_vr_gesture_velocity_threshold);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_gesture_velocity_threshold"), &GameplayControlMap::get_vr_gesture_velocity_threshold);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "vr_gesture_velocity_threshold", godot::PROPERTY_HINT_RANGE, "0.2,10.0,0.1"),
        "set_vr_gesture_velocity_threshold",
        "get_vr_gesture_velocity_threshold"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_comfort_vignette_on_move", "enabled"), &GameplayControlMap::set_vr_comfort_vignette_on_move);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_comfort_vignette_on_move"), &GameplayControlMap::get_vr_comfort_vignette_on_move);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "vr_comfort_vignette_on_move"), "set_vr_comfort_vignette_on_move", "get_vr_comfort_vignette_on_move");

    godot::ClassDB::bind_method(godot::D_METHOD("set_vr_gesture_to_action_map", "map"), &GameplayControlMap::set_vr_gesture_to_action_map);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_gesture_to_action_map"), &GameplayControlMap::get_vr_gesture_to_action_map);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::DICTIONARY, "vr_gesture_to_action_map"), "set_vr_gesture_to_action_map", "get_vr_gesture_to_action_map");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_group_tag", "tag"), &GameplayControlMap::set_player_group_tag);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_group_tag"), &GameplayControlMap::get_player_group_tag);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "player_group_tag"), "set_player_group_tag", "get_player_group_tag");

    godot::ClassDB::bind_method(
        godot::D_METHOD(
            "feed_vr_controller_state",
            "tracker_index",
            "pose",
            "linear_velocity",
            "angular_velocity",
            "thumbstick",
            "trigger",
            "grip",
            "button_mask"
        ),
        &GameplayControlMap::feed_vr_controller_state,
        DEFVAL(0)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_tracker_pose", "tracker_index"), &GameplayControlMap::get_vr_tracker_pose);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_tracker_linear_velocity", "tracker_index"), &GameplayControlMap::get_vr_tracker_linear_velocity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vr_tracker_angular_velocity", "tracker_index"), &GameplayControlMap::get_vr_tracker_angular_velocity);

    // --- Runtime & Application Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("feed_input_event", "event"), &GameplayControlMap::feed_input_event);
    godot::ClassDB::bind_method(godot::D_METHOD("process_input", "delta"), &GameplayControlMap::process_input);
    godot::ClassDB::bind_method(godot::D_METHOD("begin_sequence"), &GameplayControlMap::begin_sequence);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_sequence"), &GameplayControlMap::complete_sequence);
    godot::ClassDB::bind_method(godot::D_METHOD("fail_sequence"), &GameplayControlMap::fail_sequence);
    godot::ClassDB::bind_method(godot::D_METHOD("reset_runtime_state"), &GameplayControlMap::reset_runtime_state);

    godot::ClassDB::bind_method(godot::D_METHOD("get_unified_move_vector"), &GameplayControlMap::get_unified_move_vector);
    godot::ClassDB::bind_method(godot::D_METHOD("get_unified_look_vector"), &GameplayControlMap::get_unified_look_vector);
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_world_move_direction", "camera_or_head_transform", "agent_transform", "perspective"),
        &GameplayControlMap::get_world_move_direction,
        DEFVAL(godot::Ref<GameplayPerspective>())
    );

    godot::ClassDB::bind_method(godot::D_METHOD("get_action_strength", "action"), &GameplayControlMap::get_action_strength);
    godot::ClassDB::bind_method(godot::D_METHOD("is_action_pressed", "action"), &GameplayControlMap::is_action_pressed);
    godot::ClassDB::bind_method(
        godot::D_METHOD("is_action_just_pressed", "action", "consume_buffer"),
        &GameplayControlMap::is_action_just_pressed,
        DEFVAL(true)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("buffer_action_press", "action"), &GameplayControlMap::buffer_action_press);

    godot::ClassDB::bind_method(godot::D_METHOD("blend_with_control_map", "target", "weight"), &GameplayControlMap::blend_with_control_map);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_to_agent", "agent"), &GameplayControlMap::apply_to_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_mouse_mode_to_engine"), &GameplayControlMap::apply_mouse_mode_to_engine);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_control_map", "other"), &GameplayControlMap::apply_control_map);
}

GameplayControlMap::GameplayControlMap() {
    set_category(godot::StringName("Controls"));
    set_config_section("controls");

    // Default Arduino / Custom USB channel mappings so standard sketches work out of the box
    custom_channel_to_action_map["AX"] = godot::String(move_right_action);
    custom_channel_to_action_map["AY"] = godot::String(move_forward_action);
    custom_channel_to_action_map["LX"] = godot::String(look_right_action);
    custom_channel_to_action_map["LY"] = godot::String(look_up_action);
    custom_channel_to_action_map["BTN1"] = godot::String(primary_action);
    custom_channel_to_action_map["BTN2"] = godot::String(secondary_action);
    custom_channel_to_action_map["BTN3"] = godot::String(interact_action);
    custom_channel_to_action_map["BTN4"] = godot::String(jump_action);

    // Default VR gesture mappings
    vr_gesture_to_action_map["swing_right"] = godot::String(primary_action);
    vr_gesture_to_action_map["swing_left"] = godot::String(secondary_action);

    _sync_control_option_tags();
}

GameplayControlMap::~GameplayControlMap() {
}

// --- Static Deadzone & Curve Math Utilities ---

float GameplayControlMap::apply_axis_deadzone(float p_raw_value, float p_inner_deadzone, float p_outer_deadzone) {
    float abs_val = std::abs(p_raw_value);
    float inner = godot::Math::clamp(p_inner_deadzone, 0.0f, 0.95f);
    float outer = godot::Math::clamp(p_outer_deadzone, inner + 0.01f, 1.0f);

    if (abs_val <= inner) {
        return 0.0f;
    }
    float normalized = godot::Math::clamp((abs_val - inner) / (outer - inner), 0.0f, 1.0f);
    return (p_raw_value < 0.0f) ? -normalized : normalized;
}

godot::Vector2 GameplayControlMap::apply_stick_deadzone(
    const godot::Vector2 &p_raw,
    float p_inner_deadzone,
    float p_outer_deadzone,
    DeadzoneCurveShape p_curve,
    float p_response_exponent
) {
    float inner = godot::Math::clamp(p_inner_deadzone, 0.0f, 0.95f);
    float outer = godot::Math::clamp(p_outer_deadzone, inner + 0.01f, 1.0f);
    float exp_val = godot::Math::clamp(p_response_exponent, 0.25f, 5.0f);

    if (p_curve == DeadzoneCurveShape::DEADZONE_AXIAL_INDEPENDENT) {
        float x = apply_axis_deadzone(p_raw.x, inner, outer);
        float y = apply_axis_deadzone(p_raw.y, inner, outer);
        if (!godot::Math::is_equal_approx(exp_val, 1.0f)) {
            x = (x < 0.0f ? -1.0f : 1.0f) * std::pow(std::abs(x), exp_val);
            y = (y < 0.0f ? -1.0f : 1.0f) * std::pow(std::abs(y), exp_val);
        }
        return godot::Vector2(x, y);
    }

    float mag = p_raw.length();
    if (mag <= inner || mag <= 0.00001f) {
        return godot::Vector2(0.0f, 0.0f);
    }

    float scaled_mag = godot::Math::clamp((mag - inner) / (outer - inner), 0.0f, 1.0f);
    if (p_curve == DeadzoneCurveShape::DEADZONE_CUBIC_PRECISION) {
        // Blend 65% cubic + 35% linear for smooth fine-aim control near center
        scaled_mag = 0.65f * (scaled_mag * scaled_mag * scaled_mag) + 0.35f * scaled_mag;
    }
    if (!godot::Math::is_equal_approx(exp_val, 1.0f)) {
        scaled_mag = std::pow(scaled_mag, exp_val);
    }

    return (p_raw / mag) * scaled_mag;
}

// --- Internal Helpers ---

void GameplayControlMap::_sync_control_option_tags() {
    const char *context_tags[] = {
        "Controls_Exploration",
        "Controls_Combat",
        "Controls_Vehicle",
        "Controls_Puzzle",
        "Controls_Menu",
        "Controls_VR",
        "Controls_Custom"
    };
    for (const char *t : context_tags) {
        remove_tag(godot::StringName(t));
    }

    const char *device_tags[] = {
        "Device_KeyboardMouse",
        "Device_Gamepad",
        "Device_CustomUSB",
        "Device_VR",
        "Device_Touch"
    };
    for (const char *t : device_tags) {
        remove_tag(godot::StringName(t));
    }

    switch (context_mode) {
        case ControlContextMode::CONTROL_CONTEXT_EXPLORATION:
            add_tag(godot::StringName("Controls_Exploration"));
            break;
        case ControlContextMode::CONTROL_CONTEXT_COMBAT:
            add_tag(godot::StringName("Controls_Combat"));
            break;
        case ControlContextMode::CONTROL_CONTEXT_VEHICLE_MOUNT:
            add_tag(godot::StringName("Controls_Vehicle"));
            break;
        case ControlContextMode::CONTROL_CONTEXT_PUZZLE_INSPECT:
            add_tag(godot::StringName("Controls_Puzzle"));
            break;
        case ControlContextMode::CONTROL_CONTEXT_UI_MENU:
            add_tag(godot::StringName("Controls_Menu"));
            break;
        case ControlContextMode::CONTROL_CONTEXT_VR_IMMERSIVE:
            add_tag(godot::StringName("Controls_VR"));
            break;
        case ControlContextMode::CONTROL_CONTEXT_CUSTOM:
        default:
            add_tag(godot::StringName("Controls_Custom"));
            break;
    }

    switch (active_device_family) {
        case ControlDeviceFamily::DEVICE_KEYBOARD_MOUSE:
            add_tag(godot::StringName("Device_KeyboardMouse"));
            break;
        case ControlDeviceFamily::DEVICE_GAMEPAD:
            add_tag(godot::StringName("Device_Gamepad"));
            break;
        case ControlDeviceFamily::DEVICE_CUSTOM_USB_HID:
            add_tag(godot::StringName("Device_CustomUSB"));
            break;
        case ControlDeviceFamily::DEVICE_VR_XR:
            add_tag(godot::StringName("Device_VR"));
            break;
        case ControlDeviceFamily::DEVICE_TOUCH_VIRTUAL:
            add_tag(godot::StringName("Device_Touch"));
            break;
        default:
            break;
    }
}

void GameplayControlMap::_set_active_device_if_auto(ControlDeviceFamily p_detected_family) {
    if (preferred_device_family != ControlDeviceFamily::DEVICE_AUTO_DETECT &&
        preferred_device_family != p_detected_family) {
        return;
    }
    if (active_device_family != p_detected_family) {
        ControlDeviceFamily prev = active_device_family;
        active_device_family = p_detected_family;
        _sync_control_option_tags();
        emit_signal("device_family_changed", static_cast<int>(active_device_family), static_cast<int>(prev));
    }
}

godot::StringName GameplayControlMap::_resolve_mapped_action(const godot::StringName &p_semantic_action) const {
    godot::String key(p_semantic_action);
    if (action_remap_table.has(key)) {
        godot::String mapped = action_remap_table[key];
        if (!mapped.is_empty()) {
            return godot::StringName(mapped);
        }
    }
    return p_semantic_action;
}

void GameplayControlMap::_update_semantic_action_state(
    const godot::StringName &p_action,
    float p_strength,
    bool p_just_pressed
) {
    if (p_action.is_empty()) return;

    float clamped = godot::Math::clamp(p_strength, 0.0f, 1.0f);
    bool was_pressed = _action_pressed_states.has(p_action) ? static_cast<bool>(_action_pressed_states[p_action]) : false;
    bool now_pressed = clamped > 0.5f;

    _action_strengths[p_action] = clamped;
    _action_pressed_states[p_action] = now_pressed;

    if ((now_pressed && !was_pressed) || p_just_pressed) {
        if (input_buffer_window > 0.0f) {
            _action_buffer_timers[p_action] = static_cast<double>(input_buffer_window);
        } else {
            _action_buffer_timers[p_action] = 0.001;
        }
        emit_signal("action_triggered", p_action, clamped);

        godot::StringName eff_root = !root_action.is_empty() ? root_action : godot::StringName(root_input);
        if (!eff_root.is_empty() && p_action == eff_root && !_sequence_active) {
            begin_sequence();
        }
    } else if (!now_pressed && was_pressed) {
        emit_signal("action_released", p_action);
    }
}

float GameplayControlMap::_normalize_custom_adc_value(
    const godot::String &p_channel,
    float p_raw_value,
    bool p_bipolar
) const {
    float min_v = 0.0f;
    float center_v = static_cast<float>(custom_adc_center_value);
    float max_v = static_cast<float>(godot::Math::max(1, custom_adc_resolution_max));

    if (custom_channel_calibration.has(p_channel)) {
        godot::Variant cal = custom_channel_calibration[p_channel];
        if (cal.get_type() == godot::Variant::VECTOR3) {
            godot::Vector3 v = cal;
            min_v = v.x;
            center_v = v.y;
            max_v = v.z;
        }
    }

    if (!p_bipolar) {
        float span = godot::Math::max(0.0001f, max_v - min_v);
        float norm = godot::Math::clamp((p_raw_value - min_v) / span, 0.0f, 1.0f);
        return apply_axis_deadzone(norm, custom_usb_deadzone, 1.0f);
    }

    float normalized = 0.0f;
    if (p_raw_value >= center_v) {
        float pos_span = godot::Math::max(0.0001f, max_v - center_v);
        normalized = godot::Math::clamp((p_raw_value - center_v) / pos_span, 0.0f, 1.0f);
    } else {
        float neg_span = godot::Math::max(0.0001f, center_v - min_v);
        normalized = -godot::Math::clamp((center_v - p_raw_value) / neg_span, 0.0f, 1.0f);
    }

    return apply_axis_deadzone(normalized, custom_usb_deadzone, 1.0f);
}

// --- 0. Context, Sequence & Device Routing Setters / Getters ---

void GameplayControlMap::set_root_input(const godot::String &p_input) {
    if (p_input == root_input) return;
    root_input = p_input;
    if (root_action.is_empty() && !p_input.is_empty()) {
        root_action = godot::StringName(p_input);
    }
    emit_changed();
}
godot::String GameplayControlMap::get_root_input() const { return root_input; }

void GameplayControlMap::set_root_action(const godot::StringName &p_action) {
    if (p_action == root_action) return;
    root_action = p_action;
    if (root_input.is_empty() && !p_action.is_empty()) {
        root_input = godot::String(p_action);
    }
    emit_changed();
}
godot::StringName GameplayControlMap::get_root_action() const { return root_action; }

void GameplayControlMap::set_priority(int p_priority) {
    if (p_priority == priority) return;
    priority = p_priority;
    emit_changed();
}
int GameplayControlMap::get_priority() const { return priority; }

void GameplayControlMap::set_timeout(float p_timeout) {
    float clamped = godot::Math::max(0.0f, p_timeout);
    if (godot::Math::is_equal_approx(timeout, clamped)) return;
    timeout = clamped;
    emit_changed();
}
float GameplayControlMap::get_timeout() const { return timeout; }

void GameplayControlMap::set_context_mode(ControlContextMode p_mode) {
    if (context_mode == p_mode) return;
    context_mode = p_mode;
    if (p_mode != ControlContextMode::CONTROL_CONTEXT_CUSTOM) {
        apply_context_preset(p_mode);
    } else {
        _sync_control_option_tags();
        emit_changed();
    }
}
ControlContextMode GameplayControlMap::get_context_mode() const { return context_mode; }

void GameplayControlMap::apply_context_preset(ControlContextMode p_mode) {
    context_mode = p_mode;

    switch (p_mode) {
        case ControlContextMode::CONTROL_CONTEXT_EXPLORATION:
            mouse_capture_in_gameplay = true;
            input_buffer_window = 0.15f;
            deadzone_curve_shape = DeadzoneCurveShape::DEADZONE_RADIAL_SCALED;
            gamepad_response_exponent = 1.5f;
            break;
        case ControlContextMode::CONTROL_CONTEXT_COMBAT:
            mouse_capture_in_gameplay = true;
            input_buffer_window = 0.20f;
            deadzone_curve_shape = DeadzoneCurveShape::DEADZONE_CUBIC_PRECISION;
            gamepad_response_exponent = 1.65f;
            break;
        case ControlContextMode::CONTROL_CONTEXT_VEHICLE_MOUNT:
            mouse_capture_in_gameplay = true;
            input_buffer_window = 0.10f;
            deadzone_curve_shape = DeadzoneCurveShape::DEADZONE_AXIAL_INDEPENDENT;
            gamepad_response_exponent = 1.35f;
            break;
        case ControlContextMode::CONTROL_CONTEXT_PUZZLE_INSPECT:
            mouse_capture_in_gameplay = false;
            input_buffer_window = 0.10f;
            deadzone_curve_shape = DeadzoneCurveShape::DEADZONE_RADIAL_SCALED;
            gamepad_response_exponent = 1.2f;
            break;
        case ControlContextMode::CONTROL_CONTEXT_UI_MENU:
            mouse_capture_in_gameplay = false;
            input_buffer_window = 0.08f;
            deadzone_curve_shape = DeadzoneCurveShape::DEADZONE_AXIAL_INDEPENDENT;
            break;
        case ControlContextMode::CONTROL_CONTEXT_VR_IMMERSIVE:
            vr_input_enabled = true;
            active_device_family = ControlDeviceFamily::DEVICE_VR_XR;
            mouse_capture_in_gameplay = false;
            input_buffer_window = 0.15f;
            break;
        case ControlContextMode::CONTROL_CONTEXT_CUSTOM:
        default:
            break;
    }

    _sync_control_option_tags();
    emit_changed();
}

void GameplayControlMap::set_preferred_device_family(ControlDeviceFamily p_family) {
    if (preferred_device_family == p_family) return;
    preferred_device_family = p_family;
    if (p_family != ControlDeviceFamily::DEVICE_AUTO_DETECT) {
        set_active_device_family(p_family);
    }
    emit_changed();
}
ControlDeviceFamily GameplayControlMap::get_preferred_device_family() const { return preferred_device_family; }

void GameplayControlMap::set_active_device_family(ControlDeviceFamily p_family) {
    if (active_device_family == p_family) return;
    ControlDeviceFamily prev = active_device_family;
    active_device_family = p_family;
    _sync_control_option_tags();
    emit_signal("device_family_changed", static_cast<int>(active_device_family), static_cast<int>(prev));
    emit_changed();
}
ControlDeviceFamily GameplayControlMap::get_active_device_family() const { return active_device_family; }

void GameplayControlMap::set_device_id(int p_device_id) {
    int clamped = godot::Math::clamp(p_device_id, -1, 32);
    if (device_id == clamped) return;
    device_id = clamped;
    emit_changed();
}
int GameplayControlMap::get_device_id() const { return device_id; }

void GameplayControlMap::set_input_buffer_window(float p_window) {
    float clamped = godot::Math::clamp(p_window, 0.0f, 2.0f);
    if (godot::Math::is_equal_approx(input_buffer_window, clamped)) return;
    input_buffer_window = clamped;
    emit_changed();
}
float GameplayControlMap::get_input_buffer_window() const { return input_buffer_window; }

void GameplayControlMap::set_mode_transition_smoothing(float p_smoothing) {
    float clamped = godot::Math::clamp(p_smoothing, 0.0f, 120.0f);
    if (godot::Math::is_equal_approx(mode_transition_smoothing, clamped)) return;
    mode_transition_smoothing = clamped;
    emit_changed();
}
float GameplayControlMap::get_mode_transition_smoothing() const { return mode_transition_smoothing; }

// --- 1. Unified Semantic Action Names & Agent Routing Setters / Getters ---

void GameplayControlMap::set_move_forward_action(const godot::StringName &p_action) {
    if (move_forward_action == p_action) return;
    move_forward_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_move_forward_action() const { return move_forward_action; }

void GameplayControlMap::set_move_backward_action(const godot::StringName &p_action) {
    if (move_backward_action == p_action) return;
    move_backward_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_move_backward_action() const { return move_backward_action; }

void GameplayControlMap::set_move_left_action(const godot::StringName &p_action) {
    if (move_left_action == p_action) return;
    move_left_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_move_left_action() const { return move_left_action; }

void GameplayControlMap::set_move_right_action(const godot::StringName &p_action) {
    if (move_right_action == p_action) return;
    move_right_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_move_right_action() const { return move_right_action; }

void GameplayControlMap::set_look_up_action(const godot::StringName &p_action) {
    if (look_up_action == p_action) return;
    look_up_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_look_up_action() const { return look_up_action; }

void GameplayControlMap::set_look_down_action(const godot::StringName &p_action) {
    if (look_down_action == p_action) return;
    look_down_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_look_down_action() const { return look_down_action; }

void GameplayControlMap::set_look_left_action(const godot::StringName &p_action) {
    if (look_left_action == p_action) return;
    look_left_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_look_left_action() const { return look_left_action; }

void GameplayControlMap::set_look_right_action(const godot::StringName &p_action) {
    if (look_right_action == p_action) return;
    look_right_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_look_right_action() const { return look_right_action; }

void GameplayControlMap::set_primary_action(const godot::StringName &p_action) {
    if (primary_action == p_action) return;
    primary_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_primary_action() const { return primary_action; }

void GameplayControlMap::set_secondary_action(const godot::StringName &p_action) {
    if (secondary_action == p_action) return;
    secondary_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_secondary_action() const { return secondary_action; }

void GameplayControlMap::set_interact_action(const godot::StringName &p_action) {
    if (interact_action == p_action) return;
    interact_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_interact_action() const { return interact_action; }

void GameplayControlMap::set_jump_action(const godot::StringName &p_action) {
    if (jump_action == p_action) return;
    jump_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_jump_action() const { return jump_action; }

void GameplayControlMap::set_sprint_action(const godot::StringName &p_action) {
    if (sprint_action == p_action) return;
    sprint_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_sprint_action() const { return sprint_action; }

void GameplayControlMap::set_crouch_action(const godot::StringName &p_action) {
    if (crouch_action == p_action) return;
    crouch_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_crouch_action() const { return crouch_action; }

void GameplayControlMap::set_pause_action(const godot::StringName &p_action) {
    if (pause_action == p_action) return;
    pause_action = p_action;
    emit_changed();
}
godot::StringName GameplayControlMap::get_pause_action() const { return pause_action; }

void GameplayControlMap::set_action_to_agent_action_map(const godot::Dictionary &p_map) {
    if (action_to_agent_action_map == p_map) return;
    action_to_agent_action_map = p_map.duplicate();
    emit_changed();
}
godot::Dictionary GameplayControlMap::get_action_to_agent_action_map() const { return action_to_agent_action_map; }

void GameplayControlMap::map_input_to_agent_action(
    const godot::StringName &p_input_action,
    const godot::Variant &p_agent_action_title_or_id
) {
    if (p_input_action.is_empty()) return;
    action_to_agent_action_map[godot::String(p_input_action)] = p_agent_action_title_or_id;
    emit_changed();
}

void GameplayControlMap::set_action_remap_table(const godot::Dictionary &p_table) {
    if (action_remap_table == p_table) return;
    action_remap_table = p_table.duplicate();
    emit_changed();
}
godot::Dictionary GameplayControlMap::get_action_remap_table() const { return action_remap_table; }

void GameplayControlMap::remap_action(
    const godot::StringName &p_semantic_action,
    const godot::StringName &p_target_input_action
) {
    if (p_semantic_action.is_empty()) return;
    action_remap_table[godot::String(p_semantic_action)] = godot::String(p_target_input_action);
    emit_changed();
}

// --- 2. Keyboard & Mouse Setters / Getters ---

void GameplayControlMap::set_mouse_sensitivity(const godot::Vector2 &p_sens) {
    godot::Vector2 clamped(
        godot::Math::clamp(p_sens.x, 0.005f, 10.0f),
        godot::Math::clamp(p_sens.y, 0.005f, 10.0f)
    );
    if (mouse_sensitivity.is_equal_approx(clamped)) return;
    mouse_sensitivity = clamped;
    emit_changed();
}
godot::Vector2 GameplayControlMap::get_mouse_sensitivity() const { return mouse_sensitivity; }

void GameplayControlMap::set_mouse_raw_input(bool p_raw) {
    if (mouse_raw_input == p_raw) return;
    mouse_raw_input = p_raw;
    emit_changed();
}
bool GameplayControlMap::get_mouse_raw_input() const { return mouse_raw_input; }

void GameplayControlMap::set_mouse_smoothing_factor(float p_factor) {
    float clamped = godot::Math::clamp(p_factor, 0.0f, 0.95f);
    if (godot::Math::is_equal_approx(mouse_smoothing_factor, clamped)) return;
    mouse_smoothing_factor = clamped;
    emit_changed();
}
float GameplayControlMap::get_mouse_smoothing_factor() const { return mouse_smoothing_factor; }

void GameplayControlMap::set_mouse_capture_in_gameplay(bool p_capture) {
    if (mouse_capture_in_gameplay == p_capture) return;
    mouse_capture_in_gameplay = p_capture;
    emit_changed();
}
bool GameplayControlMap::get_mouse_capture_in_gameplay() const { return mouse_capture_in_gameplay; }

void GameplayControlMap::set_invert_mouse_x(bool p_invert) {
    if (invert_mouse_x == p_invert) return;
    invert_mouse_x = p_invert;
    emit_changed();
}
bool GameplayControlMap::get_invert_mouse_x() const { return invert_mouse_x; }

void GameplayControlMap::set_invert_mouse_y(bool p_invert) {
    if (invert_mouse_y == p_invert) return;
    invert_mouse_y = p_invert;
    emit_changed();
}
bool GameplayControlMap::get_invert_mouse_y() const { return invert_mouse_y; }

// --- 3. Console Controller & Gamepad Setters / Getters ---

void GameplayControlMap::set_left_stick_deadzone_inner(float p_deadzone) {
    float clamped = godot::Math::clamp(p_deadzone, 0.0f, 0.85f);
    if (godot::Math::is_equal_approx(left_stick_deadzone_inner, clamped)) return;
    left_stick_deadzone_inner = clamped;
    emit_changed();
}
float GameplayControlMap::get_left_stick_deadzone_inner() const { return left_stick_deadzone_inner; }

void GameplayControlMap::set_left_stick_deadzone_outer(float p_deadzone) {
    float clamped = godot::Math::clamp(p_deadzone, 0.2f, 1.0f);
    if (godot::Math::is_equal_approx(left_stick_deadzone_outer, clamped)) return;
    left_stick_deadzone_outer = clamped;
    emit_changed();
}
float GameplayControlMap::get_left_stick_deadzone_outer() const { return left_stick_deadzone_outer; }

void GameplayControlMap::set_right_stick_deadzone_inner(float p_deadzone) {
    float clamped = godot::Math::clamp(p_deadzone, 0.0f, 0.85f);
    if (godot::Math::is_equal_approx(right_stick_deadzone_inner, clamped)) return;
    right_stick_deadzone_inner = clamped;
    emit_changed();
}
float GameplayControlMap::get_right_stick_deadzone_inner() const { return right_stick_deadzone_inner; }

void GameplayControlMap::set_right_stick_deadzone_outer(float p_deadzone) {
    float clamped = godot::Math::clamp(p_deadzone, 0.2f, 1.0f);
    if (godot::Math::is_equal_approx(right_stick_deadzone_outer, clamped)) return;
    right_stick_deadzone_outer = clamped;
    emit_changed();
}
float GameplayControlMap::get_right_stick_deadzone_outer() const { return right_stick_deadzone_outer; }

void GameplayControlMap::set_trigger_deadzone(float p_deadzone) {
    float clamped = godot::Math::clamp(p_deadzone, 0.0f, 0.85f);
    if (godot::Math::is_equal_approx(trigger_deadzone, clamped)) return;
    trigger_deadzone = clamped;
    emit_changed();
}
float GameplayControlMap::get_trigger_deadzone() const { return trigger_deadzone; }

void GameplayControlMap::set_trigger_hair_trigger_mode(bool p_hair_trigger) {
    if (trigger_hair_trigger_mode == p_hair_trigger) return;
    trigger_hair_trigger_mode = p_hair_trigger;
    emit_changed();
}
bool GameplayControlMap::get_trigger_hair_trigger_mode() const { return trigger_hair_trigger_mode; }

void GameplayControlMap::set_deadzone_curve_shape(DeadzoneCurveShape p_shape) {
    if (deadzone_curve_shape == p_shape) return;
    deadzone_curve_shape = p_shape;
    emit_changed();
}
DeadzoneCurveShape GameplayControlMap::get_deadzone_curve_shape() const { return deadzone_curve_shape; }

void GameplayControlMap::set_gamepad_look_sensitivity(const godot::Vector2 &p_sens) {
    godot::Vector2 clamped(
        godot::Math::clamp(p_sens.x, 0.05f, 20.0f),
        godot::Math::clamp(p_sens.y, 0.05f, 20.0f)
    );
    if (gamepad_look_sensitivity.is_equal_approx(clamped)) return;
    gamepad_look_sensitivity = clamped;
    emit_changed();
}
godot::Vector2 GameplayControlMap::get_gamepad_look_sensitivity() const { return gamepad_look_sensitivity; }

void GameplayControlMap::set_gamepad_response_exponent(float p_exponent) {
    float clamped = godot::Math::clamp(p_exponent, 0.25f, 5.0f);
    if (godot::Math::is_equal_approx(gamepad_response_exponent, clamped)) return;
    gamepad_response_exponent = clamped;
    emit_changed();
}
float GameplayControlMap::get_gamepad_response_exponent() const { return gamepad_response_exponent; }

void GameplayControlMap::set_invert_gamepad_x(bool p_invert) {
    if (invert_gamepad_x == p_invert) return;
    invert_gamepad_x = p_invert;
    emit_changed();
}
bool GameplayControlMap::get_invert_gamepad_x() const { return invert_gamepad_x; }

void GameplayControlMap::set_invert_gamepad_y(bool p_invert) {
    if (invert_gamepad_y == p_invert) return;
    invert_gamepad_y = p_invert;
    emit_changed();
}
bool GameplayControlMap::get_invert_gamepad_y() const { return invert_gamepad_y; }

void GameplayControlMap::set_swap_thumbsticks(bool p_swap) {
    if (swap_thumbsticks == p_swap) return;
    swap_thumbsticks = p_swap;
    emit_changed();
}
bool GameplayControlMap::get_swap_thumbsticks() const { return swap_thumbsticks; }

void GameplayControlMap::set_vibration_enabled(bool p_enabled) {
    if (vibration_enabled == p_enabled) return;
    vibration_enabled = p_enabled;
    emit_changed();
}
bool GameplayControlMap::get_vibration_enabled() const { return vibration_enabled; }

void GameplayControlMap::set_vibration_strength(float p_strength) {
    float clamped = godot::Math::clamp(p_strength, 0.0f, 2.0f);
    if (godot::Math::is_equal_approx(vibration_strength, clamped)) return;
    vibration_strength = clamped;
    emit_changed();
}
float GameplayControlMap::get_vibration_strength() const { return vibration_strength; }

void GameplayControlMap::trigger_haptic_pulse(
    float p_weak_magnitude,
    float p_strong_magnitude,
    float p_duration_sec
) const {
    if (!get_enabled() || !vibration_enabled || vibration_strength <= 0.001f) {
        return;
    }

    godot::Input *inp = godot::Input::get_singleton();
    if (!inp) {
        return;
    }

    float weak = godot::Math::clamp(p_weak_magnitude * vibration_strength, 0.0f, 1.0f);
    float strong = godot::Math::clamp(p_strong_magnitude * vibration_strength, 0.0f, 1.0f);
    int target_dev = (device_id >= 0) ? device_id : 0;
    inp->start_joy_vibration(target_dev, weak, strong, godot::Math::max(0.01f, p_duration_sec));
}

// --- 4. Arduino & Custom USB / Serial / HID Setters / Getters & Ingestion ---

void GameplayControlMap::set_custom_usb_enabled(bool p_enabled) {
    if (custom_usb_enabled == p_enabled) return;
    custom_usb_enabled = p_enabled;
    emit_changed();
}
bool GameplayControlMap::get_custom_usb_enabled() const { return custom_usb_enabled; }

void GameplayControlMap::set_custom_usb_protocol(CustomUSBProtocolFormat p_protocol) {
    if (custom_usb_protocol == p_protocol) return;
    custom_usb_protocol = p_protocol;
    emit_changed();
}
CustomUSBProtocolFormat GameplayControlMap::get_custom_usb_protocol() const { return custom_usb_protocol; }

void GameplayControlMap::set_custom_usb_port_hint(const godot::String &p_port) {
    if (custom_usb_port_hint == p_port) return;
    custom_usb_port_hint = p_port;
    emit_changed();
}
godot::String GameplayControlMap::get_custom_usb_port_hint() const { return custom_usb_port_hint; }

void GameplayControlMap::set_custom_usb_baud_rate(int p_baud) {
    int clamped = godot::Math::clamp(p_baud, 300, 4000000);
    if (custom_usb_baud_rate == clamped) return;
    custom_usb_baud_rate = clamped;
    emit_changed();
}
int GameplayControlMap::get_custom_usb_baud_rate() const { return custom_usb_baud_rate; }

void GameplayControlMap::set_custom_usb_vendor_id(int p_vid) {
    int clamped = godot::Math::clamp(p_vid, 0, 65535);
    if (custom_usb_vendor_id == clamped) return;
    custom_usb_vendor_id = clamped;
    emit_changed();
}
int GameplayControlMap::get_custom_usb_vendor_id() const { return custom_usb_vendor_id; }

void GameplayControlMap::set_custom_usb_product_id(int p_pid) {
    int clamped = godot::Math::clamp(p_pid, 0, 65535);
    if (custom_usb_product_id == clamped) return;
    custom_usb_product_id = clamped;
    emit_changed();
}
int GameplayControlMap::get_custom_usb_product_id() const { return custom_usb_product_id; }

void GameplayControlMap::set_custom_adc_resolution_max(int p_max) {
    int clamped = godot::Math::clamp(p_max, 1, 65535);
    if (custom_adc_resolution_max == clamped) return;
    custom_adc_resolution_max = clamped;
    emit_changed();
}
int GameplayControlMap::get_custom_adc_resolution_max() const { return custom_adc_resolution_max; }

void GameplayControlMap::set_custom_adc_center_value(int p_center) {
    int clamped = godot::Math::clamp(p_center, 0, 65535);
    if (custom_adc_center_value == clamped) return;
    custom_adc_center_value = clamped;
    emit_changed();
}
int GameplayControlMap::get_custom_adc_center_value() const { return custom_adc_center_value; }

void GameplayControlMap::set_custom_usb_deadzone(float p_deadzone) {
    float clamped = godot::Math::clamp(p_deadzone, 0.0f, 0.8f);
    if (godot::Math::is_equal_approx(custom_usb_deadzone, clamped)) return;
    custom_usb_deadzone = clamped;
    emit_changed();
}
float GameplayControlMap::get_custom_usb_deadzone() const { return custom_usb_deadzone; }

void GameplayControlMap::set_custom_channel_to_action_map(const godot::Dictionary &p_map) {
    if (custom_channel_to_action_map == p_map) return;
    custom_channel_to_action_map = p_map.duplicate();
    emit_changed();
}
godot::Dictionary GameplayControlMap::get_custom_channel_to_action_map() const { return custom_channel_to_action_map; }

void GameplayControlMap::set_custom_channel_calibration(const godot::Dictionary &p_calibration) {
    if (custom_channel_calibration == p_calibration) return;
    custom_channel_calibration = p_calibration.duplicate();
    emit_changed();
}
godot::Dictionary GameplayControlMap::get_custom_channel_calibration() const { return custom_channel_calibration; }

void GameplayControlMap::set_custom_channel_value(
    const godot::StringName &p_channel,
    float p_value,
    bool p_is_raw_adc
) {
    if (p_channel.is_empty()) return;
    godot::String ch_str(p_channel);

    // Determine whether this channel maps to a bipolar axis or unipolar button/trigger
    bool is_button = ch_str.begins_with("BTN") || ch_str.begins_with("SW") || ch_str.begins_with("DIG");
    float normalized = p_is_raw_adc
        ? (is_button ? (p_value > 0.5f ? 1.0f : 0.0f) : _normalize_custom_adc_value(ch_str, p_value, true))
        : godot::Math::clamp(p_value, -1.0f, 1.0f);

    custom_channel_values[ch_str] = normalized;
    _set_active_device_if_auto(ControlDeviceFamily::DEVICE_CUSTOM_USB_HID);

    if (custom_channel_to_action_map.has(ch_str)) {
        godot::StringName mapped_action = godot::StringName(godot::String(custom_channel_to_action_map[ch_str]));
        if (normalized >= 0.0f) {
            _update_semantic_action_state(mapped_action, normalized);
        } else {
            // Handle bipolar negative axis deflection for standard movement/look pairs
            if (mapped_action == move_right_action) {
                _update_semantic_action_state(move_right_action, 0.0f);
                _update_semantic_action_state(move_left_action, -normalized);
            } else if (mapped_action == move_forward_action) {
                _update_semantic_action_state(move_forward_action, 0.0f);
                _update_semantic_action_state(move_backward_action, -normalized);
            } else if (mapped_action == look_right_action) {
                _update_semantic_action_state(look_right_action, 0.0f);
                _update_semantic_action_state(look_left_action, -normalized);
            } else if (mapped_action == look_up_action) {
                _update_semantic_action_state(look_up_action, 0.0f);
                _update_semantic_action_state(look_down_action, -normalized);
            } else {
                _update_semantic_action_state(mapped_action, std::abs(normalized));
            }
        }
    }
}

float GameplayControlMap::get_custom_channel_value(const godot::StringName &p_channel) const {
    godot::String ch_str(p_channel);
    if (custom_channel_values.has(ch_str)) {
        return static_cast<float>(custom_channel_values[ch_str]);
    }
    return 0.0f;
}

bool GameplayControlMap::feed_custom_serial_line(const godot::String &p_line) {
    if (!get_enabled() || !custom_usb_enabled) {
        return false;
    }

    godot::String trimmed = p_line.strip_edges();
    if (trimmed.is_empty()) {
        return false;
    }

    bool parsed_any = false;

    if (custom_usb_protocol == CustomUSBProtocolFormat::USB_PROTOCOL_JSON_OBJECT || trimmed.begins_with("{")) {
        godot::Variant parsed = godot::JSON::parse_string(trimmed);
        if (parsed.get_type() == godot::Variant::DICTIONARY) {
            godot::Dictionary dict = parsed;
            godot::Array keys = dict.keys();
            for (int i = 0; i < keys.size(); ++i) {
                godot::String k = keys[i];
                float val = static_cast<float>(dict[k]);
                bool is_raw = std::abs(val) > 1.001f;
                set_custom_channel_value(godot::StringName(k), val, is_raw);
                parsed_any = true;
            }
        }
    } else if (custom_usb_protocol == CustomUSBProtocolFormat::USB_PROTOCOL_KEY_VALUE_ASCII || trimmed.contains(":")) {
        // Supports "AX:512,AY:768,BTN1:1" or space/semicolon-separated key:value pairs
        godot::PackedStringArray tokens = trimmed.split(",", false);
        for (int i = 0; i < tokens.size(); ++i) {
            godot::PackedStringArray kv = tokens[i].strip_edges().split(":", false);
            if (kv.size() == 2) {
                godot::String key = kv[0].strip_edges();
                float val = kv[1].strip_edges().to_float();
                bool is_raw = (std::abs(val) > 1.001f) || (!kv[1].contains(".") && std::abs(val) > 1.0f);
                set_custom_channel_value(godot::StringName(key), val, is_raw);
                parsed_any = true;
            }
        }
    } else if (custom_usb_protocol == CustomUSBProtocolFormat::USB_PROTOCOL_CSV_CHANNELS) {
        godot::PackedStringArray tokens = trimmed.split(",", false);
        for (int i = 0; i < tokens.size(); ++i) {
            godot::String token = tokens[i].strip_edges();
            if (token.is_valid_float() || token.is_valid_int()) {
                float val = token.to_float();
                bool is_raw = std::abs(val) > 1.001f;
                godot::String ch_name = "ch" + godot::String::num_int64(i);
                set_custom_channel_value(godot::StringName(ch_name), val, is_raw);
                parsed_any = true;
            }
        }
    }

    if (parsed_any) {
        emit_signal("custom_packet_received", custom_channel_values.duplicate());
    }
    return parsed_any;
}

bool GameplayControlMap::feed_custom_hid_bytes(const godot::PackedByteArray &p_bytes, bool p_sixteen_bit_channels) {
    if (!get_enabled() || !custom_usb_enabled || p_bytes.is_empty()) {
        return false;
    }

    if (p_sixteen_bit_channels) {
        int channel_count = p_bytes.size() / 2;
        for (int i = 0; i < channel_count; ++i) {
            uint16_t raw = static_cast<uint16_t>(p_bytes[i * 2]) |
                           (static_cast<uint16_t>(p_bytes[i * 2 + 1]) << 8);
            godot::String ch_name = "ch" + godot::String::num_int64(i);
            set_custom_channel_value(godot::StringName(ch_name), static_cast<float>(raw), true);
        }
    } else {
        for (int i = 0; i < p_bytes.size(); ++i) {
            uint8_t raw = p_bytes[i];
            godot::String ch_name = "ch" + godot::String::num_int64(i);
            set_custom_channel_value(godot::StringName(ch_name), static_cast<float>(raw), true);
        }
    }

    emit_signal("custom_packet_received", custom_channel_values.duplicate());
    return true;
}

// --- 5. VR / OpenXR Tracked Controller Setters / Getters & Ingestion ---

void GameplayControlMap::set_vr_input_enabled(bool p_enabled) {
    if (vr_input_enabled == p_enabled) return;
    vr_input_enabled = p_enabled;
    emit_changed();
}
bool GameplayControlMap::get_vr_input_enabled() const { return vr_input_enabled; }

void GameplayControlMap::set_vr_locomotion_mode(VRMovementLocomotionMode p_mode) {
    if (vr_locomotion_mode == p_mode) return;
    vr_locomotion_mode = p_mode;
    emit_changed();
}
VRMovementLocomotionMode GameplayControlMap::get_vr_locomotion_mode() const { return vr_locomotion_mode; }

void GameplayControlMap::set_vr_turn_mode(VRTurnMode p_mode) {
    if (vr_turn_mode == p_mode) return;
    vr_turn_mode = p_mode;
    emit_changed();
}
VRTurnMode GameplayControlMap::get_vr_turn_mode() const { return vr_turn_mode; }

void GameplayControlMap::set_vr_snap_turn_degrees(float p_degrees) {
    float clamped = godot::Math::clamp(p_degrees, 5.0f, 180.0f);
    if (godot::Math::is_equal_approx(vr_snap_turn_degrees, clamped)) return;
    vr_snap_turn_degrees = clamped;
    emit_changed();
}
float GameplayControlMap::get_vr_snap_turn_degrees() const { return vr_snap_turn_degrees; }

void GameplayControlMap::set_vr_smooth_turn_speed(float p_speed) {
    float clamped = godot::Math::clamp(p_speed, 5.0f, 720.0f);
    if (godot::Math::is_equal_approx(vr_smooth_turn_speed, clamped)) return;
    vr_smooth_turn_speed = clamped;
    emit_changed();
}
float GameplayControlMap::get_vr_smooth_turn_speed() const { return vr_smooth_turn_speed; }

void GameplayControlMap::set_vr_trigger_threshold(float p_threshold) {
    float clamped = godot::Math::clamp(p_threshold, 0.05f, 0.98f);
    if (godot::Math::is_equal_approx(vr_trigger_threshold, clamped)) return;
    vr_trigger_threshold = clamped;
    emit_changed();
}
float GameplayControlMap::get_vr_trigger_threshold() const { return vr_trigger_threshold; }

void GameplayControlMap::set_vr_grip_threshold(float p_threshold) {
    float clamped = godot::Math::clamp(p_threshold, 0.05f, 0.98f);
    if (godot::Math::is_equal_approx(vr_grip_threshold, clamped)) return;
    vr_grip_threshold = clamped;
    emit_changed();
}
float GameplayControlMap::get_vr_grip_threshold() const { return vr_grip_threshold; }

void GameplayControlMap::set_vr_dominant_hand(int p_hand) {
    int clamped = godot::Math::clamp(p_hand, 0, 1);
    if (vr_dominant_hand == clamped) return;
    vr_dominant_hand = clamped;
    emit_changed();
}
int GameplayControlMap::get_vr_dominant_hand() const { return vr_dominant_hand; }

void GameplayControlMap::set_vr_gesture_velocity_threshold(float p_threshold) {
    float clamped = godot::Math::clamp(p_threshold, 0.1f, 25.0f);
    if (godot::Math::is_equal_approx(vr_gesture_velocity_threshold, clamped)) return;
    vr_gesture_velocity_threshold = clamped;
    emit_changed();
}
float GameplayControlMap::get_vr_gesture_velocity_threshold() const { return vr_gesture_velocity_threshold; }

void GameplayControlMap::set_vr_comfort_vignette_on_move(bool p_enabled) {
    if (vr_comfort_vignette_on_move == p_enabled) return;
    vr_comfort_vignette_on_move = p_enabled;
    emit_changed();
}
bool GameplayControlMap::get_vr_comfort_vignette_on_move() const { return vr_comfort_vignette_on_move; }

void GameplayControlMap::set_vr_gesture_to_action_map(const godot::Dictionary &p_map) {
    if (vr_gesture_to_action_map == p_map) return;
    vr_gesture_to_action_map = p_map.duplicate();
    emit_changed();
}
godot::Dictionary GameplayControlMap::get_vr_gesture_to_action_map() const { return vr_gesture_to_action_map; }

void GameplayControlMap::set_player_group_tag(const godot::StringName &p_tag) {
    if (player_group_tag == p_tag) return;
    player_group_tag = p_tag;
    emit_changed();
}
godot::StringName GameplayControlMap::get_player_group_tag() const { return player_group_tag; }

void GameplayControlMap::feed_vr_controller_state(
    int p_tracker_index,
    const godot::Transform3D &p_pose,
    const godot::Vector3 &p_linear_velocity,
    const godot::Vector3 &p_angular_velocity,
    const godot::Vector2 &p_thumbstick,
    float p_trigger,
    float p_grip,
    uint32_t p_button_mask
) {
    if (!get_enabled() || !vr_input_enabled || p_tracker_index < 0 || p_tracker_index > 2) {
        return;
    }

    _vr_poses[p_tracker_index] = p_pose;
    _vr_linear_velocities[p_tracker_index] = p_linear_velocity;
    _vr_angular_velocities[p_tracker_index] = p_angular_velocity;

    if (p_tracker_index < 2) {
        _vr_thumbsticks[p_tracker_index] = apply_stick_deadzone(
            p_thumbstick,
            left_stick_deadzone_inner,
            left_stick_deadzone_outer,
            deadzone_curve_shape,
            1.0f
        );
        _vr_triggers[p_tracker_index] = godot::Math::clamp(p_trigger, 0.0f, 1.0f);
        _vr_grips[p_tracker_index] = godot::Math::clamp(p_grip, 0.0f, 1.0f);
        _vr_button_masks[p_tracker_index] = p_button_mask;

        _set_active_device_if_auto(ControlDeviceFamily::DEVICE_VR_XR);

        bool is_dom = (p_tracker_index == vr_dominant_hand);
        _update_semantic_action_state(
            is_dom ? primary_action : secondary_action,
            _vr_triggers[p_tracker_index] >= vr_trigger_threshold ? _vr_triggers[p_tracker_index] : 0.0f
        );
        _update_semantic_action_state(
            interact_action,
            _vr_grips[p_tracker_index] >= vr_grip_threshold ? _vr_grips[p_tracker_index] : 0.0f
        );

        // Evaluate physical controller swing velocity gestures
        float speed = p_linear_velocity.length();
        if (speed >= vr_gesture_velocity_threshold) {
            godot::String gesture_key = (p_tracker_index == 0) ? "swing_left" : "swing_right";
            emit_signal("vr_gesture_detected", godot::StringName(gesture_key), p_tracker_index, speed);
            if (vr_gesture_to_action_map.has(gesture_key)) {
                godot::StringName mapped_act = godot::StringName(godot::String(vr_gesture_to_action_map[gesture_key]));
                _update_semantic_action_state(mapped_act, 1.0f, true);
            }
        }
    }
}

godot::Transform3D GameplayControlMap::get_vr_tracker_pose(int p_tracker_index) const {
    if (p_tracker_index < 0 || p_tracker_index > 2) {
        return godot::Transform3D();
    }
    return _vr_poses[p_tracker_index];
}

godot::Vector3 GameplayControlMap::get_vr_tracker_linear_velocity(int p_tracker_index) const {
    if (p_tracker_index < 0 || p_tracker_index > 2) {
        return godot::Vector3();
    }
    return _vr_linear_velocities[p_tracker_index];
}

godot::Vector3 GameplayControlMap::get_vr_tracker_angular_velocity(int p_tracker_index) const {
    if (p_tracker_index < 0 || p_tracker_index > 2) {
        return godot::Vector3();
    }
    return _vr_angular_velocities[p_tracker_index];
}

// --- Unified Input Ingestion, Queries & Mode Adaptation ---

void GameplayControlMap::feed_input_event(const godot::Ref<godot::InputEvent> &p_event) {
    if (!get_enabled() || !p_event.is_valid()) {
        return;
    }

    if (device_id >= 0 && p_event->get_device() != device_id) {
        return;
    }

    // 1. Mouse Motion
    if (godot::InputEventMouseMotion *mm = godot::Object::cast_to<godot::InputEventMouseMotion>(p_event.ptr())) {
        _set_active_device_if_auto(ControlDeviceFamily::DEVICE_KEYBOARD_MOUSE);
        godot::Vector2 rel = mm->get_relative();
        rel.x *= mouse_sensitivity.x * (invert_mouse_x ? -1.0f : 1.0f);
        rel.y *= mouse_sensitivity.y * (invert_mouse_y ? -1.0f : 1.0f);
        _accumulated_mouse_delta += rel;
        return;
    }

    // 2. Keyboard or Mouse Button
    if (godot::Object::cast_to<godot::InputEventKey>(p_event.ptr()) != nullptr ||
        godot::Object::cast_to<godot::InputEventMouseButton>(p_event.ptr()) != nullptr) {
        _set_active_device_if_auto(ControlDeviceFamily::DEVICE_KEYBOARD_MOUSE);
    }

    // 3. Gamepad Button or Motion
    if (godot::Object::cast_to<godot::InputEventJoypadButton>(p_event.ptr()) != nullptr) {
        _set_active_device_if_auto(ControlDeviceFamily::DEVICE_GAMEPAD);
    } else if (godot::InputEventJoypadMotion *jm = godot::Object::cast_to<godot::InputEventJoypadMotion>(p_event.ptr())) {
        if (std::abs(jm->get_axis_value()) > left_stick_deadzone_inner) {
            _set_active_device_if_auto(ControlDeviceFamily::DEVICE_GAMEPAD);
        }
    }

    // 4. Check semantic actions against the incoming event
    const godot::StringName actions_to_check[] = {
        primary_action,
        secondary_action,
        interact_action,
        jump_action,
        sprint_action,
        crouch_action,
        pause_action,
        !root_action.is_empty() ? root_action : godot::StringName(root_input)
    };

    for (const godot::StringName &sem_act : actions_to_check) {
        if (sem_act.is_empty()) continue;
        godot::StringName engine_act = _resolve_mapped_action(sem_act);
        if (p_event->is_action(engine_act)) {
            float strength = p_event->get_action_strength(engine_act);
            bool pressed = p_event->is_action_pressed(engine_act);
            _update_semantic_action_state(sem_act, pressed ? godot::Math::max(strength, 1.0f) : 0.0f, pressed);
        }
    }
}

void GameplayControlMap::process_input(double p_delta) {
    if (!get_enabled() || p_delta <= 0.0) {
        return;
    }

    // 1. Advance input buffer timers
    godot::Array buffered_keys = _action_buffer_timers.keys();
    for (int i = 0; i < buffered_keys.size(); ++i) {
        godot::Variant key = buffered_keys[i];
        double rem = static_cast<double>(_action_buffer_timers[key]) - p_delta;
        if (rem <= 0.0) {
            _action_buffer_timers.erase(key);
        } else {
            _action_buffer_timers[key] = rem;
        }
    }

    // 2. Advance active sequence timeout if configured
    if (_sequence_active && timeout > 0.0f) {
        _sequence_elapsed += p_delta;
        if (_sequence_elapsed >= static_cast<double>(timeout)) {
            _sequence_active = false;
            _sequence_elapsed = 0.0;
            emit_signal("timed_out");
            emit_signal("failed");
        }
    }

    // 3. Poll Godot Input singleton for continuous movement & look actions
    godot::Input *inp = godot::Input::get_singleton();
    godot::Vector2 polled_move(0.0f, 0.0f);
    godot::Vector2 polled_look(0.0f, 0.0f);

    if (inp) {
        godot::InputMap *imap = godot::InputMap::get_singleton();
        auto get_safe_strength = [inp, imap](const godot::StringName &p_act) -> float {
            if (p_act.is_empty() || !imap || !imap->has_action(p_act)) {
                return 0.0f;
            }
            return inp->get_action_raw_strength(p_act);
        };

        float move_x = get_safe_strength(_resolve_mapped_action(move_right_action)) -
                       get_safe_strength(_resolve_mapped_action(move_left_action));
        float move_y = get_safe_strength(_resolve_mapped_action(move_forward_action)) -
                       get_safe_strength(_resolve_mapped_action(move_backward_action));
        polled_move = godot::Vector2(move_x, move_y);

        float look_x = get_safe_strength(_resolve_mapped_action(look_right_action)) -
                       get_safe_strength(_resolve_mapped_action(look_left_action));
        float look_y = get_safe_strength(_resolve_mapped_action(look_up_action)) -
                       get_safe_strength(_resolve_mapped_action(look_down_action));
        polled_look = godot::Vector2(look_x, look_y);

        if (swap_thumbsticks && active_device_family == ControlDeviceFamily::DEVICE_GAMEPAD) {
            godot::Vector2 tmp = polled_move;
            polled_move = polled_look;
            polled_look = tmp;
        }

        polled_move = apply_stick_deadzone(
            polled_move,
            left_stick_deadzone_inner,
            left_stick_deadzone_outer,
            deadzone_curve_shape,
            1.0f
        );

        polled_look = apply_stick_deadzone(
            polled_look,
            right_stick_deadzone_inner,
            right_stick_deadzone_outer,
            deadzone_curve_shape,
            gamepad_response_exponent
        );
        polled_look.x *= gamepad_look_sensitivity.x * (invert_gamepad_x ? -1.0f : 1.0f);
        polled_look.y *= gamepad_look_sensitivity.y * (invert_gamepad_y ? -1.0f : 1.0f);
    }

    // 4. Fold in Custom USB / Arduino normalized movement & look channels
    if (custom_usb_enabled) {
        godot::Vector2 usb_move(
            get_custom_channel_value(godot::StringName("AX")),
            get_custom_channel_value(godot::StringName("AY"))
        );
        godot::Vector2 usb_look(
            get_custom_channel_value(godot::StringName("LX")),
            get_custom_channel_value(godot::StringName("LY"))
        );
        if (usb_move.length_squared() > polled_move.length_squared()) {
            polled_move = usb_move.limit_length(1.0f);
        }
        if (usb_look.length_squared() > 0.0001f) {
            polled_look += usb_look * gamepad_look_sensitivity;
        }
    }

    // 5. Fold in VR thumbstick / arm-swing locomotion & snap/smooth turn
    if (vr_input_enabled) {
        int move_hand = 1 - vr_dominant_hand;
        int turn_hand = vr_dominant_hand;

        if (vr_locomotion_mode == VRMovementLocomotionMode::VR_LOCOMOTION_ARM_SWING) {
            float swing_speed = (_vr_linear_velocities[0].length() + _vr_linear_velocities[1].length()) * 0.5f;
            float forward_amount = godot::Math::clamp(swing_speed / godot::Math::max(0.2f, vr_gesture_velocity_threshold), 0.0f, 1.0f);
            if (forward_amount > polled_move.length()) {
                polled_move = godot::Vector2(0.0f, forward_amount);
            }
        } else if (_vr_thumbsticks[move_hand].length_squared() > polled_move.length_squared()) {
            polled_move = _vr_thumbsticks[move_hand];
        }

        float turn_x = _vr_thumbsticks[turn_hand].x;
        if (vr_turn_mode == VRTurnMode::VR_TURN_SNAP) {
            if (std::abs(turn_x) > 0.65f && !_vr_snap_turn_latched) {
                _vr_snap_turn_latched = true;
                polled_look.x += (turn_x > 0.0f ? 1.0f : -1.0f) * vr_snap_turn_degrees;
            } else if (std::abs(turn_x) < 0.35f) {
                _vr_snap_turn_latched = false;
            }
        } else if (vr_turn_mode == VRTurnMode::VR_TURN_SMOOTH) {
            polled_look.x += turn_x * vr_smooth_turn_speed * static_cast<float>(p_delta);
        }
    }

    // 6. Fold in smoothed/raw Mouse delta
    if (mouse_smoothing_factor > 0.001f) {
        _smoothed_mouse_delta = _smoothed_mouse_delta.lerp(_accumulated_mouse_delta, 1.0f - mouse_smoothing_factor);
        polled_look += _smoothed_mouse_delta;
    } else {
        polled_look += _accumulated_mouse_delta;
    }
    _accumulated_mouse_delta = godot::Vector2(0.0f, 0.0f);

    _unified_move_vector = polled_move.limit_length(1.0f);
    _unified_look_vector = polled_look;
}

void GameplayControlMap::begin_sequence() {
    _sequence_active = true;
    _sequence_elapsed = 0.0;
    emit_signal("begun");
}

void GameplayControlMap::complete_sequence() {
    if (!_sequence_active) return;
    _sequence_active = false;
    _sequence_elapsed = 0.0;
    emit_signal("completed");
}

void GameplayControlMap::fail_sequence() {
    if (!_sequence_active) return;
    _sequence_active = false;
    _sequence_elapsed = 0.0;
    emit_signal("failed");
}

void GameplayControlMap::reset_runtime_state() {
    _unified_move_vector = godot::Vector2(0.0f, 0.0f);
    _unified_look_vector = godot::Vector2(0.0f, 0.0f);
    _accumulated_mouse_delta = godot::Vector2(0.0f, 0.0f);
    _smoothed_mouse_delta = godot::Vector2(0.0f, 0.0f);
    _sequence_active = false;
    _sequence_elapsed = 0.0;
    _action_strengths.clear();
    _action_pressed_states.clear();
    _action_buffer_timers.clear();
}

godot::Vector2 GameplayControlMap::get_unified_move_vector() const {
    return _unified_move_vector;
}

godot::Vector2 GameplayControlMap::get_unified_look_vector() const {
    return _unified_look_vector;
}

godot::Vector3 GameplayControlMap::get_world_move_direction(
    const godot::Transform3D &p_camera_or_head_transform,
    const godot::Transform3D &p_agent_transform,
    const godot::Ref<GameplayPerspective> &p_perspective
) const {
    if (_unified_move_vector.length_squared() <= 0.00001f) {
        return godot::Vector3(0.0f, 0.0f, 0.0f);
    }

    // If VR hand-oriented locomotion is active, project along the off-hand controller basis
    if (vr_input_enabled && active_device_family == ControlDeviceFamily::DEVICE_VR_XR &&
        vr_locomotion_mode == VRMovementLocomotionMode::VR_LOCOMOTION_CONTINUOUS_HAND) {
        int move_hand = 1 - vr_dominant_hand;
        godot::Basis hand_basis = _vr_poses[move_hand].basis;
        godot::Vector3 forward = -hand_basis.get_column(2);
        forward.y = 0.0f;
        forward = forward.normalized();
        godot::Vector3 right = hand_basis.get_column(0);
        right.y = 0.0f;
        right = right.normalized();
        return (right * _unified_move_vector.x + forward * _unified_move_vector.y).limit_length(1.0f);
    }

    MovementAlignmentMode align_mode = MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE;
    if (p_perspective.is_valid()) {
        align_mode = p_perspective->get_movement_alignment();
    }

    switch (align_mode) {
        case MovementAlignmentMode::MOVEMENT_CHARACTER_RELATIVE: {
            godot::Basis agent_basis = p_agent_transform.basis;
            godot::Vector3 forward = -agent_basis.get_column(2);
            forward.y = 0.0f;
            forward = forward.normalized();
            godot::Vector3 right = agent_basis.get_column(0);
            right.y = 0.0f;
            right = right.normalized();
            return (right * _unified_move_vector.x + forward * _unified_move_vector.y).limit_length(1.0f);
        }
        case MovementAlignmentMode::MOVEMENT_WORLD_AXIS_ALIGNED: {
            return godot::Vector3(_unified_move_vector.x, 0.0f, -_unified_move_vector.y).limit_length(1.0f);
        }
        case MovementAlignmentMode::MOVEMENT_CURSOR_DIRECTED:
        case MovementAlignmentMode::MOVEMENT_CAMERA_RELATIVE:
        default: {
            godot::Basis cam_basis = p_camera_or_head_transform.basis;
            godot::Vector3 forward = -cam_basis.get_column(2);
            forward.y = 0.0f;
            if (forward.length_squared() < 0.0001f) {
                // Fallback for straight top-down (-90 deg pitch) cameras: use camera -Y up vector
                forward = cam_basis.get_column(1);
                forward.y = 0.0f;
            }
            forward = forward.normalized();
            godot::Vector3 right = cam_basis.get_column(0);
            right.y = 0.0f;
            right = right.normalized();
            return (right * _unified_move_vector.x + forward * _unified_move_vector.y).limit_length(1.0f);
        }
    }
}

float GameplayControlMap::get_action_strength(const godot::StringName &p_action) const {
    if (_action_strengths.has(p_action)) {
        return static_cast<float>(_action_strengths[p_action]);
    }
    return 0.0f;
}

bool GameplayControlMap::is_action_pressed(const godot::StringName &p_action) const {
    if (_action_pressed_states.has(p_action)) {
        return static_cast<bool>(_action_pressed_states[p_action]);
    }
    return false;
}

bool GameplayControlMap::is_action_just_pressed(const godot::StringName &p_action, bool p_consume_buffer) {
    if (!_action_buffer_timers.has(p_action)) {
        return false;
    }
    if (p_consume_buffer) {
        _action_buffer_timers.erase(p_action);
    }
    return true;
}

void GameplayControlMap::buffer_action_press(const godot::StringName &p_action) {
    if (p_action.is_empty()) return;
    _action_buffer_timers[p_action] = static_cast<double>(godot::Math::max(0.01f, input_buffer_window));
}

godot::Ref<GameplayControlMap> GameplayControlMap::blend_with_control_map(
    const godot::Ref<GameplayControlMap> &p_target,
    float p_weight
) const {
    godot::Ref<GameplayControlMap> blended;
    blended.instantiate();
    blended->apply_control_map(const_cast<GameplayControlMap *>(this));

    if (!p_target.is_valid()) {
        return blended;
    }

    float t = godot::Math::clamp(p_weight, 0.0f, 1.0f);
    if (t >= 0.5f) {
        blended->apply_control_map(p_target);
    }

    blended->set_mouse_sensitivity(mouse_sensitivity.lerp(p_target->get_mouse_sensitivity(), t));
    blended->set_mouse_smoothing_factor(godot::Math::lerp(mouse_smoothing_factor, p_target->get_mouse_smoothing_factor(), t));
    blended->set_left_stick_deadzone_inner(godot::Math::lerp(left_stick_deadzone_inner, p_target->get_left_stick_deadzone_inner(), t));
    blended->set_left_stick_deadzone_outer(godot::Math::lerp(left_stick_deadzone_outer, p_target->get_left_stick_deadzone_outer(), t));
    blended->set_right_stick_deadzone_inner(godot::Math::lerp(right_stick_deadzone_inner, p_target->get_right_stick_deadzone_inner(), t));
    blended->set_right_stick_deadzone_outer(godot::Math::lerp(right_stick_deadzone_outer, p_target->get_right_stick_deadzone_outer(), t));
    blended->set_gamepad_look_sensitivity(gamepad_look_sensitivity.lerp(p_target->get_gamepad_look_sensitivity(), t));
    blended->set_gamepad_response_exponent(godot::Math::lerp(gamepad_response_exponent, p_target->get_gamepad_response_exponent(), t));
    blended->set_vibration_strength(godot::Math::lerp(vibration_strength, p_target->get_vibration_strength(), t));
    blended->set_input_buffer_window(godot::Math::lerp(input_buffer_window, p_target->get_input_buffer_window(), t));

    return blended;
}

int GameplayControlMap::apply_to_agent(GameAgent *p_agent) {
    if (!p_agent || !get_enabled()) {
        return 0;
    }

    int triggered_count = 0;
    godot::Array mapped_inputs = action_to_agent_action_map.keys();

    for (int i = 0; i < mapped_inputs.size(); ++i) {
        godot::String input_key = mapped_inputs[i];
        godot::StringName sem_action(input_key);

        if (is_action_just_pressed(sem_action, true)) {
            godot::Variant target_spec = action_to_agent_action_map[input_key];
            int action_id = -1;
            if (target_spec.get_type() == godot::Variant::INT) {
                action_id = static_cast<int>(target_spec);
            } else {
                action_id = p_agent->get_action_id_from_title(godot::String(target_spec));
            }

            if (action_id >= 0) {
                if (sem_action == interact_action) {
                    p_agent->interact(action_id);
                } else {
                    p_agent->start_action(action_id);
                }
                ++triggered_count;
            }
        }
    }

    return triggered_count;
}

void GameplayControlMap::apply_mouse_mode_to_engine() const {
    if (!get_enabled()) return;
    if (godot::Input *inp = godot::Input::get_singleton()) {
        inp->set_mouse_mode(
            mouse_capture_in_gameplay
                ? godot::Input::MOUSE_MODE_CAPTURED
                : godot::Input::MOUSE_MODE_VISIBLE
        );
    }
}

// --- Resource & Cascade Overrides ---

void GameplayControlMap::apply_control_map(const godot::Ref<GameplayControlMap> &p_other) {
    if (!p_other.is_valid()) {
        return;
    }

    GameplayResource::apply_resource(p_other);

    root_input = p_other->get_root_input();
    root_action = p_other->get_root_action();
    priority = p_other->get_priority();
    timeout = p_other->get_timeout();

    context_mode = p_other->get_context_mode();
    preferred_device_family = p_other->get_preferred_device_family();
    active_device_family = p_other->get_active_device_family();
    device_id = p_other->get_device_id();
    input_buffer_window = p_other->get_input_buffer_window();
    mode_transition_smoothing = p_other->get_mode_transition_smoothing();

    move_forward_action = p_other->get_move_forward_action();
    move_backward_action = p_other->get_move_backward_action();
    move_left_action = p_other->get_move_left_action();
    move_right_action = p_other->get_move_right_action();

    look_up_action = p_other->get_look_up_action();
    look_down_action = p_other->get_look_down_action();
    look_left_action = p_other->get_look_left_action();
    look_right_action = p_other->get_look_right_action();

    primary_action = p_other->get_primary_action();
    secondary_action = p_other->get_secondary_action();
    interact_action = p_other->get_interact_action();
    jump_action = p_other->get_jump_action();
    sprint_action = p_other->get_sprint_action();
    crouch_action = p_other->get_crouch_action();
    pause_action = p_other->get_pause_action();

    action_to_agent_action_map = p_other->get_action_to_agent_action_map().duplicate();
    action_remap_table = p_other->get_action_remap_table().duplicate();

    mouse_sensitivity = p_other->get_mouse_sensitivity();
    mouse_raw_input = p_other->get_mouse_raw_input();
    mouse_smoothing_factor = p_other->get_mouse_smoothing_factor();
    mouse_capture_in_gameplay = p_other->get_mouse_capture_in_gameplay();
    invert_mouse_x = p_other->get_invert_mouse_x();
    invert_mouse_y = p_other->get_invert_mouse_y();

    left_stick_deadzone_inner = p_other->get_left_stick_deadzone_inner();
    left_stick_deadzone_outer = p_other->get_left_stick_deadzone_outer();
    right_stick_deadzone_inner = p_other->get_right_stick_deadzone_inner();
    right_stick_deadzone_outer = p_other->get_right_stick_deadzone_outer();
    trigger_deadzone = p_other->get_trigger_deadzone();
    trigger_hair_trigger_mode = p_other->get_trigger_hair_trigger_mode();
    deadzone_curve_shape = p_other->get_deadzone_curve_shape();
    gamepad_look_sensitivity = p_other->get_gamepad_look_sensitivity();
    gamepad_response_exponent = p_other->get_gamepad_response_exponent();
    invert_gamepad_x = p_other->get_invert_gamepad_x();
    invert_gamepad_y = p_other->get_invert_gamepad_y();
    swap_thumbsticks = p_other->get_swap_thumbsticks();
    vibration_enabled = p_other->get_vibration_enabled();
    vibration_strength = p_other->get_vibration_strength();

    custom_usb_enabled = p_other->get_custom_usb_enabled();
    custom_usb_protocol = p_other->get_custom_usb_protocol();
    custom_usb_port_hint = p_other->get_custom_usb_port_hint();
    custom_usb_baud_rate = p_other->get_custom_usb_baud_rate();
    custom_usb_vendor_id = p_other->get_custom_usb_vendor_id();
    custom_usb_product_id = p_other->get_custom_usb_product_id();
    custom_adc_resolution_max = p_other->get_custom_adc_resolution_max();
    custom_adc_center_value = p_other->get_custom_adc_center_value();
    custom_usb_deadzone = p_other->get_custom_usb_deadzone();
    custom_channel_to_action_map = p_other->get_custom_channel_to_action_map().duplicate();
    custom_channel_calibration = p_other->get_custom_channel_calibration().duplicate();

    vr_input_enabled = p_other->get_vr_input_enabled();
    vr_locomotion_mode = p_other->get_vr_locomotion_mode();
    vr_turn_mode = p_other->get_vr_turn_mode();
    vr_snap_turn_degrees = p_other->get_vr_snap_turn_degrees();
    vr_smooth_turn_speed = p_other->get_vr_smooth_turn_speed();
    vr_trigger_threshold = p_other->get_vr_trigger_threshold();
    vr_grip_threshold = p_other->get_vr_grip_threshold();
    vr_dominant_hand = p_other->get_vr_dominant_hand();
    vr_gesture_velocity_threshold = p_other->get_vr_gesture_velocity_threshold();
    vr_comfort_vignette_on_move = p_other->get_vr_comfort_vignette_on_move();
    vr_gesture_to_action_map = p_other->get_vr_gesture_to_action_map().duplicate();
    player_group_tag = p_other->get_player_group_tag();

    _sync_control_option_tags();
    emit_changed();
}

void GameplayControlMap::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    godot::Ref<GameplayControlMap> cm_other = p_other;
    if (cm_other.is_valid()) {
        apply_control_map(cm_other);
    } else {
        GameplayResource::apply_resource(p_other);
    }
}

godot::TypedArray<GameplayRule> GameplayControlMap::collect_rules() const {
    godot::TypedArray<GameplayRule> collected = GameplayResource::collect_rules();
    if (!get_enabled() || player_group_tag.is_empty()) {
        return collected;
    }

    // Cascade input buffer window onto matching Player GameEntities if they expose `input_buffer_window`
    {
        godot::Ref<GameplayRule> buf_rule;
        buf_rule.instantiate();
        buf_rule->set_title("ControlMap_InputBufferWindow");
        buf_rule->set_scope(RuleScope::DESCENDANTS);
        buf_rule->set_target_class(godot::StringName("GameEntity"));
        buf_rule->set_target_title_or_group(player_group_tag);
        buf_rule->set_target_property(godot::StringName("input_buffer_window"));
        buf_rule->set_operation(RuleOp::SET);
        buf_rule->set_value(input_buffer_window);
        buf_rule->set_priority(40 + priority);
        collected.append(buf_rule);
    }

    return collected;
}

godot::Dictionary GameplayControlMap::save_resource() const {
    godot::Dictionary data = GameplayResource::save_resource();

    data["root_input"] = root_input;
    data["root_action"] = godot::String(root_action);
    data["priority"] = priority;
    data["timeout"] = timeout;

    data["context_mode"] = static_cast<int>(context_mode);
    data["preferred_device_family"] = static_cast<int>(preferred_device_family);
    data["active_device_family"] = static_cast<int>(active_device_family);
    data["device_id"] = device_id;
    data["input_buffer_window"] = input_buffer_window;
    data["mode_transition_smoothing"] = mode_transition_smoothing;

    data["move_forward_action"] = godot::String(move_forward_action);
    data["move_backward_action"] = godot::String(move_backward_action);
    data["move_left_action"] = godot::String(move_left_action);
    data["move_right_action"] = godot::String(move_right_action);

    data["look_up_action"] = godot::String(look_up_action);
    data["look_down_action"] = godot::String(look_down_action);
    data["look_left_action"] = godot::String(look_left_action);
    data["look_right_action"] = godot::String(look_right_action);

    data["primary_action"] = godot::String(primary_action);
    data["secondary_action"] = godot::String(secondary_action);
    data["interact_action"] = godot::String(interact_action);
    data["jump_action"] = godot::String(jump_action);
    data["sprint_action"] = godot::String(sprint_action);
    data["crouch_action"] = godot::String(crouch_action);
    data["pause_action"] = godot::String(pause_action);

    data["action_to_agent_action_map"] = action_to_agent_action_map.duplicate();
    data["action_remap_table"] = action_remap_table.duplicate();

    data["mouse_sensitivity"] = mouse_sensitivity;
    data["mouse_raw_input"] = mouse_raw_input;
    data["mouse_smoothing_factor"] = mouse_smoothing_factor;
    data["mouse_capture_in_gameplay"] = mouse_capture_in_gameplay;
    data["invert_mouse_x"] = invert_mouse_x;
    data["invert_mouse_y"] = invert_mouse_y;

    data["left_stick_deadzone_inner"] = left_stick_deadzone_inner;
    data["left_stick_deadzone_outer"] = left_stick_deadzone_outer;
    data["right_stick_deadzone_inner"] = right_stick_deadzone_inner;
    data["right_stick_deadzone_outer"] = right_stick_deadzone_outer;
    data["trigger_deadzone"] = trigger_deadzone;
    data["trigger_hair_trigger_mode"] = trigger_hair_trigger_mode;
    data["deadzone_curve_shape"] = static_cast<int>(deadzone_curve_shape);
    data["gamepad_look_sensitivity"] = gamepad_look_sensitivity;
    data["gamepad_response_exponent"] = gamepad_response_exponent;
    data["invert_gamepad_x"] = invert_gamepad_x;
    data["invert_gamepad_y"] = invert_gamepad_y;
    data["swap_thumbsticks"] = swap_thumbsticks;
    data["vibration_enabled"] = vibration_enabled;
    data["vibration_strength"] = vibration_strength;

    data["custom_usb_enabled"] = custom_usb_enabled;
    data["custom_usb_protocol"] = static_cast<int>(custom_usb_protocol);
    data["custom_usb_port_hint"] = custom_usb_port_hint;
    data["custom_usb_baud_rate"] = custom_usb_baud_rate;
    data["custom_usb_vendor_id"] = custom_usb_vendor_id;
    data["custom_usb_product_id"] = custom_usb_product_id;
    data["custom_adc_resolution_max"] = custom_adc_resolution_max;
    data["custom_adc_center_value"] = custom_adc_center_value;
    data["custom_usb_deadzone"] = custom_usb_deadzone;
    data["custom_channel_to_action_map"] = custom_channel_to_action_map.duplicate();
    data["custom_channel_calibration"] = custom_channel_calibration.duplicate();

    data["vr_input_enabled"] = vr_input_enabled;
    data["vr_locomotion_mode"] = static_cast<int>(vr_locomotion_mode);
    data["vr_turn_mode"] = static_cast<int>(vr_turn_mode);
    data["vr_snap_turn_degrees"] = vr_snap_turn_degrees;
    data["vr_smooth_turn_speed"] = vr_smooth_turn_speed;
    data["vr_trigger_threshold"] = vr_trigger_threshold;
    data["vr_grip_threshold"] = vr_grip_threshold;
    data["vr_dominant_hand"] = vr_dominant_hand;
    data["vr_gesture_velocity_threshold"] = vr_gesture_velocity_threshold;
    data["vr_comfort_vignette_on_move"] = vr_comfort_vignette_on_move;
    data["vr_gesture_to_action_map"] = vr_gesture_to_action_map.duplicate();
    data["player_group_tag"] = godot::String(player_group_tag);

    return data;
}

void GameplayControlMap::load_resource(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    if (p_data.has("root_input")) set_root_input(p_data["root_input"]);
    if (p_data.has("root_action")) set_root_action(godot::StringName(godot::String(p_data["root_action"])));
    if (p_data.has("priority")) set_priority(p_data["priority"]);
    if (p_data.has("timeout")) set_timeout(p_data["timeout"]);

    if (p_data.has("context_mode")) {
        context_mode = static_cast<ControlContextMode>(static_cast<int>(p_data["context_mode"]));
    }
    if (p_data.has("preferred_device_family")) {
        preferred_device_family = static_cast<ControlDeviceFamily>(static_cast<int>(p_data["preferred_device_family"]));
    }
    if (p_data.has("active_device_family")) {
        active_device_family = static_cast<ControlDeviceFamily>(static_cast<int>(p_data["active_device_family"]));
    }
    if (p_data.has("device_id")) set_device_id(p_data["device_id"]);
    if (p_data.has("input_buffer_window")) set_input_buffer_window(p_data["input_buffer_window"]);
    if (p_data.has("mode_transition_smoothing")) set_mode_transition_smoothing(p_data["mode_transition_smoothing"]);

    if (p_data.has("move_forward_action")) move_forward_action = godot::StringName(godot::String(p_data["move_forward_action"]));
    if (p_data.has("move_backward_action")) move_backward_action = godot::StringName(godot::String(p_data["move_backward_action"]));
    if (p_data.has("move_left_action")) move_left_action = godot::StringName(godot::String(p_data["move_left_action"]));
    if (p_data.has("move_right_action")) move_right_action = godot::StringName(godot::String(p_data["move_right_action"]));

    if (p_data.has("look_up_action")) look_up_action = godot::StringName(godot::String(p_data["look_up_action"]));
    if (p_data.has("look_down_action")) look_down_action = godot::StringName(godot::String(p_data["look_down_action"]));
    if (p_data.has("look_left_action")) look_left_action = godot::StringName(godot::String(p_data["look_left_action"]));
    if (p_data.has("look_right_action")) look_right_action = godot::StringName(godot::String(p_data["look_right_action"]));

    if (p_data.has("primary_action")) primary_action = godot::StringName(godot::String(p_data["primary_action"]));
    if (p_data.has("secondary_action")) secondary_action = godot::StringName(godot::String(p_data["secondary_action"]));
    if (p_data.has("interact_action")) interact_action = godot::StringName(godot::String(p_data["interact_action"]));
    if (p_data.has("jump_action")) jump_action = godot::StringName(godot::String(p_data["jump_action"]));
    if (p_data.has("sprint_action")) sprint_action = godot::StringName(godot::String(p_data["sprint_action"]));
    if (p_data.has("crouch_action")) crouch_action = godot::StringName(godot::String(p_data["crouch_action"]));
    if (p_data.has("pause_action")) pause_action = godot::StringName(godot::String(p_data["pause_action"]));

    if (p_data.has("action_to_agent_action_map") && p_data["action_to_agent_action_map"].get_type() == godot::Variant::DICTIONARY) {
        action_to_agent_action_map = godot::Dictionary(p_data["action_to_agent_action_map"]).duplicate();
    }
    if (p_data.has("action_remap_table") && p_data["action_remap_table"].get_type() == godot::Variant::DICTIONARY) {
        action_remap_table = godot::Dictionary(p_data["action_remap_table"]).duplicate();
    }

    if (p_data.has("mouse_sensitivity")) set_mouse_sensitivity(p_data["mouse_sensitivity"]);
    if (p_data.has("mouse_raw_input")) mouse_raw_input = p_data["mouse_raw_input"];
    if (p_data.has("mouse_smoothing_factor")) set_mouse_smoothing_factor(p_data["mouse_smoothing_factor"]);
    if (p_data.has("mouse_capture_in_gameplay")) mouse_capture_in_gameplay = p_data["mouse_capture_in_gameplay"];
    if (p_data.has("invert_mouse_x")) invert_mouse_x = p_data["invert_mouse_x"];
    if (p_data.has("invert_mouse_y")) invert_mouse_y = p_data["invert_mouse_y"];

    if (p_data.has("left_stick_deadzone_inner")) set_left_stick_deadzone_inner(p_data["left_stick_deadzone_inner"]);
    if (p_data.has("left_stick_deadzone_outer")) set_left_stick_deadzone_outer(p_data["left_stick_deadzone_outer"]);
    if (p_data.has("right_stick_deadzone_inner")) set_right_stick_deadzone_inner(p_data["right_stick_deadzone_inner"]);
    if (p_data.has("right_stick_deadzone_outer")) set_right_stick_deadzone_outer(p_data["right_stick_deadzone_outer"]);
    if (p_data.has("trigger_deadzone")) set_trigger_deadzone(p_data["trigger_deadzone"]);
    if (p_data.has("trigger_hair_trigger_mode")) trigger_hair_trigger_mode = p_data["trigger_hair_trigger_mode"];
    if (p_data.has("deadzone_curve_shape")) {
        deadzone_curve_shape = static_cast<DeadzoneCurveShape>(static_cast<int>(p_data["deadzone_curve_shape"]));
    }
    if (p_data.has("gamepad_look_sensitivity")) set_gamepad_look_sensitivity(p_data["gamepad_look_sensitivity"]);
    if (p_data.has("gamepad_response_exponent")) set_gamepad_response_exponent(p_data["gamepad_response_exponent"]);
    if (p_data.has("invert_gamepad_x")) invert_gamepad_x = p_data["invert_gamepad_x"];
    if (p_data.has("invert_gamepad_y")) invert_gamepad_y = p_data["invert_gamepad_y"];
    if (p_data.has("swap_thumbsticks")) swap_thumbsticks = p_data["swap_thumbsticks"];
    if (p_data.has("vibration_enabled")) vibration_enabled = p_data["vibration_enabled"];
    if (p_data.has("vibration_strength")) set_vibration_strength(p_data["vibration_strength"]);

    if (p_data.has("custom_usb_enabled")) custom_usb_enabled = p_data["custom_usb_enabled"];
    if (p_data.has("custom_usb_protocol")) {
        custom_usb_protocol = static_cast<CustomUSBProtocolFormat>(static_cast<int>(p_data["custom_usb_protocol"]));
    }
    if (p_data.has("custom_usb_port_hint")) custom_usb_port_hint = p_data["custom_usb_port_hint"];
    if (p_data.has("custom_usb_baud_rate")) set_custom_usb_baud_rate(p_data["custom_usb_baud_rate"]);
    if (p_data.has("custom_usb_vendor_id")) set_custom_usb_vendor_id(p_data["custom_usb_vendor_id"]);
    if (p_data.has("custom_usb_product_id")) set_custom_usb_product_id(p_data["custom_usb_product_id"]);
    if (p_data.has("custom_adc_resolution_max")) set_custom_adc_resolution_max(p_data["custom_adc_resolution_max"]);
    if (p_data.has("custom_adc_center_value")) set_custom_adc_center_value(p_data["custom_adc_center_value"]);
    if (p_data.has("custom_usb_deadzone")) set_custom_usb_deadzone(p_data["custom_usb_deadzone"]);
    if (p_data.has("custom_channel_to_action_map") && p_data["custom_channel_to_action_map"].get_type() == godot::Variant::DICTIONARY) {
        custom_channel_to_action_map = godot::Dictionary(p_data["custom_channel_to_action_map"]).duplicate();
    }
    if (p_data.has("custom_channel_calibration") && p_data["custom_channel_calibration"].get_type() == godot::Variant::DICTIONARY) {
        custom_channel_calibration = godot::Dictionary(p_data["custom_channel_calibration"]).duplicate();
    }

    if (p_data.has("vr_input_enabled")) vr_input_enabled = p_data["vr_input_enabled"];
    if (p_data.has("vr_locomotion_mode")) {
        vr_locomotion_mode = static_cast<VRMovementLocomotionMode>(static_cast<int>(p_data["vr_locomotion_mode"]));
    }
    if (p_data.has("vr_turn_mode")) {
        vr_turn_mode = static_cast<VRTurnMode>(static_cast<int>(p_data["vr_turn_mode"]));
    }
    if (p_data.has("vr_snap_turn_degrees")) set_vr_snap_turn_degrees(p_data["vr_snap_turn_degrees"]);
    if (p_data.has("vr_smooth_turn_speed")) set_vr_smooth_turn_speed(p_data["vr_smooth_turn_speed"]);
    if (p_data.has("vr_trigger_threshold")) set_vr_trigger_threshold(p_data["vr_trigger_threshold"]);
    if (p_data.has("vr_grip_threshold")) set_vr_grip_threshold(p_data["vr_grip_threshold"]);
    if (p_data.has("vr_dominant_hand")) set_vr_dominant_hand(p_data["vr_dominant_hand"]);
    if (p_data.has("vr_gesture_velocity_threshold")) set_vr_gesture_velocity_threshold(p_data["vr_gesture_velocity_threshold"]);
    if (p_data.has("vr_comfort_vignette_on_move")) vr_comfort_vignette_on_move = p_data["vr_comfort_vignette_on_move"];
    if (p_data.has("vr_gesture_to_action_map") && p_data["vr_gesture_to_action_map"].get_type() == godot::Variant::DICTIONARY) {
        vr_gesture_to_action_map = godot::Dictionary(p_data["vr_gesture_to_action_map"]).duplicate();
    }
    if (p_data.has("player_group_tag")) player_group_tag = godot::StringName(godot::String(p_data["player_group_tag"]));

    _sync_control_option_tags();
    emit_changed();
}

} // namespace ideam::godot_ext