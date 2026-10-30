#pragma once

#include "gameplay_resource.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameAgent;
class GameplayPerspective;

/**
 * DOD NOTE: Extracting control map enums at namespace scope with explicit int32_t
 * backing allows input routing systems to store device families, deadzone curves,
 * and locomotion states in contiguous Structure of Arrays (SoA) columns without
 * pulling in the full GameplayControlMap Resource definition.
 */
enum ControlDeviceFamily : int32_t {
    DEVICE_AUTO_DETECT = 0,     // Dynamically switches active device family based on latest input
    DEVICE_KEYBOARD_MOUSE = 1,  // Desktop Keyboard + Mouse
    DEVICE_GAMEPAD = 2,         // Console Controller / Gamepad (XInput, DualSense, Switch, SDL)
    DEVICE_CUSTOM_USB_HID = 3,  // Arduino, microcontroller, flight stick, arcade, or custom USB serial/HID
    DEVICE_VR_XR = 4,           // OpenXR / VR tracked 6DoF controllers & HMD
    DEVICE_TOUCH_VIRTUAL = 5    // Touch screen or virtual on-screen controls
};

enum ControlContextMode : int32_t {
    CONTROL_CONTEXT_EXPLORATION = 0,    // Standard world traversal & camera control
    CONTROL_CONTEXT_COMBAT = 1,         // High-responsiveness aiming, lock-on, and combat actions
    CONTROL_CONTEXT_VEHICLE_MOUNT = 2,  // Throttle/steering/mounted control layout
    CONTROL_CONTEXT_PUZZLE_INSPECT = 3, // Cursor-driven close-up puzzle & object manipulation
    CONTROL_CONTEXT_UI_MENU = 4,        // Menu, inventory, and dialogue navigation
    CONTROL_CONTEXT_VR_IMMERSIVE = 5,   // 6DoF hand-tracked spatial interaction & locomotion
    CONTROL_CONTEXT_CUSTOM = 6          // Custom user-defined gameplay mode context
};

enum DeadzoneCurveShape : int32_t {
    DEADZONE_RADIAL_SCALED = 0,     // Radial inner/outer deadzone rescaled smoothly to [0..1]
    DEADZONE_AXIAL_INDEPENDENT = 1, // Independent X/Y axial deadzones (tank controls, throttles, 2D)
    DEADZONE_CUBIC_PRECISION = 2    // Radial deadzone blended with a cubic (x^3) precision curve
};

enum VRMovementLocomotionMode : int32_t {
    VR_LOCOMOTION_CONTINUOUS_HEAD = 0, // Smooth movement oriented relative to the HMD forward vector
    VR_LOCOMOTION_CONTINUOUS_HAND = 1, // Smooth movement oriented relative to the movement hand controller
    VR_LOCOMOTION_TELEPORT_BLINK = 2,  // Comfort teleport / dash arc locomotion
    VR_LOCOMOTION_ARM_SWING = 3        // Physical arm-swing velocity locomotion
};

enum VRTurnMode : int32_t {
    VR_TURN_SNAP = 0,         // Discrete comfort snap-turns by `vr_snap_turn_degrees`
    VR_TURN_SMOOTH = 1,       // Continuous smooth yaw rotation
    VR_TURN_PHYSICAL_ONLY = 2 // Real-world physical turning only
};

enum CustomUSBProtocolFormat : int32_t {
    USB_PROTOCOL_KEY_VALUE_ASCII = 0, // Serial ASCII "AX:512,AY:512,BTN1:1\n" (standard Arduino Serial.println)
    USB_PROTOCOL_CSV_CHANNELS = 1,    // Serial CSV "512,512,1,0\n" mapped by channel index ("ch0", "ch1", ...)
    USB_PROTOCOL_JSON_OBJECT = 2,     // Serial JSON '{"move_x": 0.5, "primary_action": 1}'
    USB_PROTOCOL_RAW_HID_BYTES = 3    // Binary USB HID report frames (uint8 / uint16 channels)
};

/**
 * GameplayControlMap provides a unified, hardware-agnostic input abstraction and
 * command-routing resource that normalizes input across:
 * 1. Keyboard & Mouse (`InputEventKey`, `InputEventMouseButton`, `InputEventMouseMotion`)
 * 2. Console Controllers & Gamepads (`InputEventJoypadButton`, `InputEventJoypadMotion`)
 * 3. Arduino & Custom USB Serial / HID Hardware (ASCII `KEY:VALUE`, CSV, JSON, and raw binary HID bytes
 *    with configurable ADC bit depth, center offset, and per-channel calibration)
 * 4. VR / OpenXR Tracked Controllers (6DoF hand poses, physical swing/throw velocities,
 *    analog triggers/grips, thumbsticks, and comfort locomotion/turning)
 *
 * It adapts smoothly across `GameplayMode` transitions via `context_mode` presets,
 * `blend_with_control_map()`, and perspective-aware 3D movement projection
 * (`get_world_move_direction()`), and can directly drive a controlled `GameAgent`
 * (`apply_to_agent()`) or synthesize cascading `GameplayRule` modifiers (`collect_rules()`).
 */
class GameplayControlMap : public GameplayResource {
    GDCLASS(GameplayControlMap, GameplayResource)

protected:
    static void _bind_methods();

private:
    // --- 0. Context, Sequence & Device Routing ---
    godot::String root_input;
    godot::StringName root_action;
    int priority = 0;
    float timeout = 0.0f;

    ControlContextMode context_mode = ControlContextMode::CONTROL_CONTEXT_EXPLORATION;
    ControlDeviceFamily preferred_device_family = ControlDeviceFamily::DEVICE_AUTO_DETECT;
    ControlDeviceFamily active_device_family = ControlDeviceFamily::DEVICE_KEYBOARD_MOUSE;
    int device_id = -1; // -1 = any/all devices, >= 0 = specific controller/USB port index
    float input_buffer_window = 0.15f;
    float mode_transition_smoothing = 12.0f;

    // --- 1. Unified Semantic Action Names & Agent Routing ---
    godot::StringName move_forward_action = godot::StringName("move_forward");
    godot::StringName move_backward_action = godot::StringName("move_back");
    godot::StringName move_left_action = godot::StringName("move_left");
    godot::StringName move_right_action = godot::StringName("move_right");

    godot::StringName look_up_action = godot::StringName("look_up");
    godot::StringName look_down_action = godot::StringName("look_down");
    godot::StringName look_left_action = godot::StringName("look_left");
    godot::StringName look_right_action = godot::StringName("look_right");

    godot::StringName primary_action = godot::StringName("primary_action");
    godot::StringName secondary_action = godot::StringName("secondary_action");
    godot::StringName interact_action = godot::StringName("interact");
    godot::StringName jump_action = godot::StringName("jump");
    godot::StringName sprint_action = godot::StringName("sprint");
    godot::StringName crouch_action = godot::StringName("crouch");
    godot::StringName pause_action = godot::StringName("pause");

    // Maps semantic action names (StringName/String) -> GameAgentAction title (String) or index (int)
    godot::Dictionary action_to_agent_action_map;

    // Remapping table across modes/profiles: logical action (String) -> Godot InputMap action (String)
    godot::Dictionary action_remap_table;

    // --- 2. Keyboard & Mouse Settings ---
    godot::Vector2 mouse_sensitivity = godot::Vector2(0.25f, 0.25f);
    bool mouse_raw_input = true;
    float mouse_smoothing_factor = 0.0f;
    bool mouse_capture_in_gameplay = true;
    bool invert_mouse_x = false;
    bool invert_mouse_y = false;

    // --- 3. Console Controller & Gamepad Settings ---
    float left_stick_deadzone_inner = 0.15f;
    float left_stick_deadzone_outer = 0.95f;
    float right_stick_deadzone_inner = 0.15f;
    float right_stick_deadzone_outer = 0.95f;
    float trigger_deadzone = 0.08f;
    bool trigger_hair_trigger_mode = false;

    DeadzoneCurveShape deadzone_curve_shape = DeadzoneCurveShape::DEADZONE_RADIAL_SCALED;
    godot::Vector2 gamepad_look_sensitivity = godot::Vector2(2.5f, 2.0f);
    float gamepad_response_exponent = 1.6f;
    bool invert_gamepad_x = false;
    bool invert_gamepad_y = false;
    bool swap_thumbsticks = false;
    bool vibration_enabled = true;
    float vibration_strength = 1.0f;

    // --- 4. Arduino & Custom USB / Serial / HID Input ---
    bool custom_usb_enabled = false;
    CustomUSBProtocolFormat custom_usb_protocol = CustomUSBProtocolFormat::USB_PROTOCOL_KEY_VALUE_ASCII;
    godot::String custom_usb_port_hint = "";
    int custom_usb_baud_rate = 115200;
    int custom_usb_vendor_id = 0;
    int custom_usb_product_id = 0;
    int custom_adc_resolution_max = 1023; // 10-bit Arduino default (0..1023); 4095 for 12-bit ESP32/STM32
    int custom_adc_center_value = 512;
    float custom_usb_deadzone = 0.05f;

    // Maps custom hardware channel names ("AX", "AY", "ch0", "BTN1") -> semantic action names ("move_right", "primary_action")
    godot::Dictionary custom_channel_to_action_map;

    // Optional per-channel calibration overrides: channel name -> Vector3(min, center, max)
    godot::Dictionary custom_channel_calibration;

    // Latest normalized values received from custom USB/Arduino channels
    godot::Dictionary custom_channel_values;

    // --- 5. VR / OpenXR Tracked Controller Settings ---
    bool vr_input_enabled = false;
    VRMovementLocomotionMode vr_locomotion_mode = VRMovementLocomotionMode::VR_LOCOMOTION_CONTINUOUS_HEAD;
    VRTurnMode vr_turn_mode = VRTurnMode::VR_TURN_SNAP;
    float vr_snap_turn_degrees = 45.0f;
    float vr_smooth_turn_speed = 90.0f;
    float vr_trigger_threshold = 0.55f;
    float vr_grip_threshold = 0.60f;
    int vr_dominant_hand = 1; // 0 = Left Hand, 1 = Right Hand
    float vr_gesture_velocity_threshold = 1.8f;
    bool vr_comfort_vignette_on_move = true;

    // Maps physical VR gestures ("swing_left", "swing_right", "thrust_right", "grip_left") -> semantic actions
    godot::Dictionary vr_gesture_to_action_map;

    // --- Runtime Unified State (Non-Serialized Transient State) ---
    godot::Vector2 _unified_move_vector = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 _unified_look_vector = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 _accumulated_mouse_delta = godot::Vector2(0.0f, 0.0f);
    godot::Vector2 _smoothed_mouse_delta = godot::Vector2(0.0f, 0.0f);

    // Tracked VR poses, velocities, analog inputs (index 0 = Left Hand, index 1 = Right Hand, index 2 = Head/HMD)
    godot::Transform3D _vr_poses[3];
    godot::Vector3 _vr_linear_velocities[3];
    godot::Vector3 _vr_angular_velocities[3];
    godot::Vector2 _vr_thumbsticks[2];
    float _vr_triggers[2] = {0.0f, 0.0f};
    float _vr_grips[2] = {0.0f, 0.0f};
    uint32_t _vr_button_masks[2] = {0, 0};
    bool _vr_snap_turn_latched = false;

    // Sequence & Input Buffering Runtime State
    bool _sequence_active = false;
    double _sequence_elapsed = 0.0;
    godot::Dictionary _action_strengths;       // StringName -> float [0.0 .. 1.0]
    godot::Dictionary _action_pressed_states;  // StringName -> bool
    godot::Dictionary _action_buffer_timers;   // StringName -> double remaining buffer seconds

    godot::StringName player_group_tag = godot::StringName("Player");

    // Internal Helpers
    void _sync_control_option_tags();
    void _set_active_device_if_auto(ControlDeviceFamily p_detected_family);
    godot::StringName _resolve_mapped_action(const godot::StringName &p_semantic_action) const;
    void _update_semantic_action_state(const godot::StringName &p_action, float p_strength, bool p_just_pressed = false);
    float _normalize_custom_adc_value(const godot::String &p_channel, float p_raw_value, bool p_bipolar) const;

public:
    GameplayControlMap();
    ~GameplayControlMap() override;

    // --- Static Deadzone & Curve Math Utilities ---
    static godot::Vector2 apply_stick_deadzone(
        const godot::Vector2 &p_raw,
        float p_inner_deadzone,
        float p_outer_deadzone = 0.95f,
        DeadzoneCurveShape p_curve = DeadzoneCurveShape::DEADZONE_RADIAL_SCALED,
        float p_response_exponent = 1.0f
    );
    static float apply_axis_deadzone(float p_raw_value, float p_inner_deadzone, float p_outer_deadzone = 0.95f);

    // --- 0. Context, Sequence & Device Routing Setters / Getters ---
    void set_root_input(const godot::String &p_input);
    godot::String get_root_input() const;

    void set_root_action(const godot::StringName &p_action);
    godot::StringName get_root_action() const;

    void set_priority(int p_priority);
    int get_priority() const;

    void set_timeout(float p_timeout);
    float get_timeout() const;

    void set_context_mode(ControlContextMode p_mode);
    ControlContextMode get_context_mode() const;
    void apply_context_preset(ControlContextMode p_mode);

    void set_preferred_device_family(ControlDeviceFamily p_family);
    ControlDeviceFamily get_preferred_device_family() const;

    void set_active_device_family(ControlDeviceFamily p_family);
    ControlDeviceFamily get_active_device_family() const;

    void set_device_id(int p_device_id);
    int get_device_id() const;

    void set_input_buffer_window(float p_window);
    float get_input_buffer_window() const;

    void set_mode_transition_smoothing(float p_smoothing);
    float get_mode_transition_smoothing() const;

    // --- 1. Unified Semantic Action Names & Agent Routing Setters / Getters ---
    void set_move_forward_action(const godot::StringName &p_action);
    godot::StringName get_move_forward_action() const;

    void set_move_backward_action(const godot::StringName &p_action);
    godot::StringName get_move_backward_action() const;

    void set_move_left_action(const godot::StringName &p_action);
    godot::StringName get_move_left_action() const;

    void set_move_right_action(const godot::StringName &p_action);
    godot::StringName get_move_right_action() const;

    void set_look_up_action(const godot::StringName &p_action);
    godot::StringName get_look_up_action() const;

    void set_look_down_action(const godot::StringName &p_action);
    godot::StringName get_look_down_action() const;

    void set_look_left_action(const godot::StringName &p_action);
    godot::StringName get_look_left_action() const;

    void set_look_right_action(const godot::StringName &p_action);
    godot::StringName get_look_right_action() const;

    void set_primary_action(const godot::StringName &p_action);
    godot::StringName get_primary_action() const;

    void set_secondary_action(const godot::StringName &p_action);
    godot::StringName get_secondary_action() const;

    void set_interact_action(const godot::StringName &p_action);
    godot::StringName get_interact_action() const;

    void set_jump_action(const godot::StringName &p_action);
    godot::StringName get_jump_action() const;

    void set_sprint_action(const godot::StringName &p_action);
    godot::StringName get_sprint_action() const;

    void set_crouch_action(const godot::StringName &p_action);
    godot::StringName get_crouch_action() const;

    void set_pause_action(const godot::StringName &p_action);
    godot::StringName get_pause_action() const;

    void set_action_to_agent_action_map(const godot::Dictionary &p_map);
    godot::Dictionary get_action_to_agent_action_map() const;
    void map_input_to_agent_action(const godot::StringName &p_input_action, const godot::Variant &p_agent_action_title_or_id);

    void set_action_remap_table(const godot::Dictionary &p_table);
    godot::Dictionary get_action_remap_table() const;
    void remap_action(const godot::StringName &p_semantic_action, const godot::StringName &p_target_input_action);

    // --- 2. Keyboard & Mouse Setters / Getters ---
    void set_mouse_sensitivity(const godot::Vector2 &p_sens);
    godot::Vector2 get_mouse_sensitivity() const;

    void set_mouse_raw_input(bool p_raw);
    bool get_mouse_raw_input() const;

    void set_mouse_smoothing_factor(float p_factor);
    float get_mouse_smoothing_factor() const;

    void set_mouse_capture_in_gameplay(bool p_capture);
    bool get_mouse_capture_in_gameplay() const;

    void set_invert_mouse_x(bool p_invert);
    bool get_invert_mouse_x() const;

    void set_invert_mouse_y(bool p_invert);
    bool get_invert_mouse_y() const;

    // --- 3. Console Controller & Gamepad Setters / Getters ---
    void set_left_stick_deadzone_inner(float p_deadzone);
    float get_left_stick_deadzone_inner() const;

    void set_left_stick_deadzone_outer(float p_deadzone);
    float get_left_stick_deadzone_outer() const;

    void set_right_stick_deadzone_inner(float p_deadzone);
    float get_right_stick_deadzone_inner() const;

    void set_right_stick_deadzone_outer(float p_deadzone);
    float get_right_stick_deadzone_outer() const;

    void set_trigger_deadzone(float p_deadzone);
    float get_trigger_deadzone() const;

    void set_trigger_hair_trigger_mode(bool p_hair_trigger);
    bool get_trigger_hair_trigger_mode() const;

    void set_deadzone_curve_shape(DeadzoneCurveShape p_shape);
    DeadzoneCurveShape get_deadzone_curve_shape() const;

    void set_gamepad_look_sensitivity(const godot::Vector2 &p_sens);
    godot::Vector2 get_gamepad_look_sensitivity() const;

    void set_gamepad_response_exponent(float p_exponent);
    float get_gamepad_response_exponent() const;

    void set_invert_gamepad_x(bool p_invert);
    bool get_invert_gamepad_x() const;

    void set_invert_gamepad_y(bool p_invert);
    bool get_invert_gamepad_y() const;

    void set_swap_thumbsticks(bool p_swap);
    bool get_swap_thumbsticks() const;

    void set_vibration_enabled(bool p_enabled);
    bool get_vibration_enabled() const;

    void set_vibration_strength(float p_strength);
    float get_vibration_strength() const;

    void trigger_haptic_pulse(float p_weak_magnitude, float p_strong_magnitude, float p_duration_sec = 0.15f) const;

    // --- 4. Arduino & Custom USB / Serial / HID Setters / Getters & Ingestion ---
    void set_custom_usb_enabled(bool p_enabled);
    bool get_custom_usb_enabled() const;

    void set_custom_usb_protocol(CustomUSBProtocolFormat p_protocol);
    CustomUSBProtocolFormat get_custom_usb_protocol() const;

    void set_custom_usb_port_hint(const godot::String &p_port);
    godot::String get_custom_usb_port_hint() const;

    void set_custom_usb_baud_rate(int p_baud);
    int get_custom_usb_baud_rate() const;

    void set_custom_usb_vendor_id(int p_vid);
    int get_custom_usb_vendor_id() const;

    void set_custom_usb_product_id(int p_pid);
    int get_custom_usb_product_id() const;

    void set_custom_adc_resolution_max(int p_max);
    int get_custom_adc_resolution_max() const;

    void set_custom_adc_center_value(int p_center);
    int get_custom_adc_center_value() const;

    void set_custom_usb_deadzone(float p_deadzone);
    float get_custom_usb_deadzone() const;

    void set_custom_channel_to_action_map(const godot::Dictionary &p_map);
    godot::Dictionary get_custom_channel_to_action_map() const;

    void set_custom_channel_calibration(const godot::Dictionary &p_calibration);
    godot::Dictionary get_custom_channel_calibration() const;

    /**
     * Parses an incoming serial/USB line (Arduino `KEY:VALUE`, CSV channels, or JSON),
     * normalizes raw ADC readings, updates mapped semantic actions, and emits `custom_packet_received`.
     */
    bool feed_custom_serial_line(const godot::String &p_line);

    /**
     * Parses a binary USB HID byte frame into normalized channel values and mapped semantic actions.
     */
    bool feed_custom_hid_bytes(const godot::PackedByteArray &p_bytes, bool p_sixteen_bit_channels = false);

    void set_custom_channel_value(const godot::StringName &p_channel, float p_value, bool p_is_raw_adc = false);
    float get_custom_channel_value(const godot::StringName &p_channel) const;

    // --- 5. VR / OpenXR Tracked Controller Setters / Getters & Ingestion ---
    void set_vr_input_enabled(bool p_enabled);
    bool get_vr_input_enabled() const;

    void set_vr_locomotion_mode(VRMovementLocomotionMode p_mode);
    VRMovementLocomotionMode get_vr_locomotion_mode() const;

    void set_vr_turn_mode(VRTurnMode p_mode);
    VRTurnMode get_vr_turn_mode() const;

    void set_vr_snap_turn_degrees(float p_degrees);
    float get_vr_snap_turn_degrees() const;

    void set_vr_smooth_turn_speed(float p_speed);
    float get_vr_smooth_turn_speed() const;

    void set_vr_trigger_threshold(float p_threshold);
    float get_vr_trigger_threshold() const;

    void set_vr_grip_threshold(float p_threshold);
    float get_vr_grip_threshold() const;

    void set_vr_dominant_hand(int p_hand);
    int get_vr_dominant_hand() const;

    void set_vr_gesture_velocity_threshold(float p_threshold);
    float get_vr_gesture_velocity_threshold() const;

    void set_vr_comfort_vignette_on_move(bool p_enabled);
    bool get_vr_comfort_vignette_on_move() const;

    void set_vr_gesture_to_action_map(const godot::Dictionary &p_map);
    godot::Dictionary get_vr_gesture_to_action_map() const;

    void set_player_group_tag(const godot::StringName &p_tag);
    godot::StringName get_player_group_tag() const;

    /**
     * Updates tracked 6DoF state for a VR hand controller (0 = Left, 1 = Right) or HMD (2 = Head),
     * evaluating physical swing gestures, thumbstick locomotion/snap-turn, and analog triggers/grips.
     */
    void feed_vr_controller_state(
        int p_tracker_index,
        const godot::Transform3D &p_pose,
        const godot::Vector3 &p_linear_velocity,
        const godot::Vector3 &p_angular_velocity,
        const godot::Vector2 &p_thumbstick,
        float p_trigger,
        float p_grip,
        uint32_t p_button_mask = 0
    );

    godot::Transform3D get_vr_tracker_pose(int p_tracker_index) const;
    godot::Vector3 get_vr_tracker_linear_velocity(int p_tracker_index) const;
    godot::Vector3 get_vr_tracker_angular_velocity(int p_tracker_index) const;

    // --- Unified Input Ingestion, Queries & Mode Adaptation ---

    /**
     * Ingests a standard Godot `InputEvent` (Keyboard, Mouse, Joypad, Touch), automatically
     * updating `active_device_family` (when set to `DEVICE_AUTO_DETECT`) and unified axes/actions.
     */
    void feed_input_event(const godot::Ref<godot::InputEvent> &p_event);

    /**
     * Polls active input state, advances sequence/hold timeouts and input buffer timers,
     * and synthesizes unified movement and look vectors for the frame.
     */
    virtual void process_input(double p_delta);

    void begin_sequence();
    void complete_sequence();
    void fail_sequence();
    void reset_runtime_state();

    godot::Vector2 get_unified_move_vector() const;
    godot::Vector2 get_unified_look_vector() const;

    /**
     * Projects the 2D unified movement vector into a 3D world-space direction vector
     * adapted to the active `GameplayPerspective` (Camera-Relative, Character/Tank-Relative,
     * World-Axis-Aligned for Isometric/Top-Down/Side-Scroller, or VR HMD/Hand-Oriented).
     */
    godot::Vector3 get_world_move_direction(
        const godot::Transform3D &p_camera_or_head_transform,
        const godot::Transform3D &p_agent_transform,
        const godot::Ref<GameplayPerspective> &p_perspective = godot::Ref<GameplayPerspective>()
    ) const;

    float get_action_strength(const godot::StringName &p_action) const;
    bool is_action_pressed(const godot::StringName &p_action) const;
    bool is_action_just_pressed(const godot::StringName &p_action, bool p_consume_buffer = true);
    void buffer_action_press(const godot::StringName &p_action);

    /**
     * Smoothly interpolates sensitivities, deadzones, and analog state between this control map
     * and `p_target` by `p_weight` (0.0 .. 1.0) during `GameplayMode` transitions.
     */
    godot::Ref<GameplayControlMap> blend_with_control_map(
        const godot::Ref<GameplayControlMap> &p_target,
        float p_weight
    ) const;

    /**
     * Applies this control map's active actions and movement/look state to a controlled `GameAgent`,
     * invoking mapped `GameAgentAction`s (`start_action` / `end_action` / `interact`).
     * Returns the number of agent actions triggered or updated.
     */
    int apply_to_agent(GameAgent *p_agent);

    /**
     * Synchronizes Godot's `Input::MouseMode` (captured vs. visible) with this control map's context.
     */
    void apply_mouse_mode_to_engine() const;

    // --- Resource & Cascade Overrides ---
    void apply_control_map(const godot::Ref<GameplayControlMap> &p_other);

    virtual godot::TypedArray<GameplayRule> collect_rules() const override;
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::ControlDeviceFamily);
VARIANT_ENUM_CAST(ideam::godot_ext::ControlContextMode);
VARIANT_ENUM_CAST(ideam::godot_ext::DeadzoneCurveShape);
VARIANT_ENUM_CAST(ideam::godot_ext::VRMovementLocomotionMode);
VARIANT_ENUM_CAST(ideam::godot_ext::VRTurnMode);
VARIANT_ENUM_CAST(ideam::godot_ext::CustomUSBProtocolFormat);