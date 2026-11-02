#pragma once

#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_debugger_plugin.hpp>
#include <godot_cpp/classes/editor_debugger_session.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "../gameplay/gameplay_control_map.h"
#include "../gameplay/gameplay_rule.h"
#include "../gameplay/gameplay_style.h"

namespace ideam::godot_ext {

/**
 * @brief GameplayEditorDebuggerPlugin — Phase 4 Live Editor Debugger & Hardware/VR Simulator.
 *
 * Responsibilities:
 *   1. Live Cascade Hot-Reload:
 *      Broadcasts `ideam_games:apply_style` and `ideam_games:apply_rule` packets to active
 *      EditorDebuggerSession instances and applies directly to edited scene nodes so tuning
 *      sliders or rules in the Inspector updates running/edited games in real time.
 *   2. Hardware & VR Input Injector:
 *      Interactive test bench for sending synthetic Arduino/USB serial lines
 *      (`feed_custom_serial_line("AX:1023,AY:512,BTN1:1")`), 8/16-bit HID byte packets
 *      (`feed_custom_hid_bytes()`), and simulated VR 6DoF controller poses/velocities
 *      (`feed_vr_controller_state()`) without requiring physical hardware attached.
 */
class GameplayEditorDebuggerPlugin : public godot::EditorDebuggerPlugin {
    GDCLASS(GameplayEditorDebuggerPlugin, godot::EditorDebuggerPlugin)

private:
    godot::Ref<GameplayControlMap> _target_control_map;
    godot::Ref<GameplayStyle> _target_style;

    // UI widget handles per session tab (tracked via ObjectID for DOD safety)
    godot::ObjectID _serial_line_edit_id;
    godot::ObjectID _hid_hex_edit_id;
    godot::ObjectID _vr_tracker_option_id;
    godot::ObjectID _vr_trigger_slider_id;
    godot::ObjectID _vr_grip_slider_id;
    godot::ObjectID _vr_stick_x_slider_id;
    godot::ObjectID _vr_stick_y_slider_id;
    godot::ObjectID _vr_vel_x_spin_id;
    godot::ObjectID _vr_vel_y_spin_id;
    godot::ObjectID _vr_vel_z_spin_id;
    godot::ObjectID _vr_primary_btn_check_id;
    godot::ObjectID _status_label_id;

    godot::Control *_build_simulator_tab(int32_t p_session_id);
    void _resolve_active_resources_from_editor();
    void _set_status_message(const godot::String &p_msg);

    // UI Callbacks
    void _on_hot_reload_cascade_pressed();
    void _on_send_serial_line_pressed();
    void _on_send_serial_preset_pressed(const godot::String &p_preset);
    void _on_send_hid_bytes_pressed();
    void _on_send_vr_state_pressed();

protected:
    static void _bind_methods();

public:
    GameplayEditorDebuggerPlugin();
    ~GameplayEditorDebuggerPlugin() override;

    virtual bool _has_capture(const godot::String &p_capture) const override;
    virtual bool _capture(
        const godot::String &p_message,
        const godot::Array &p_data,
        int32_t p_session_id
    ) override;
    virtual void _setup_session(int32_t p_session_id) override;

    // Target resource bindings
    void set_target_control_map(const godot::Ref<GameplayControlMap> &p_map);
    godot::Ref<GameplayControlMap> get_target_control_map() const;

    void set_target_style(const godot::Ref<GameplayStyle> &p_style);
    godot::Ref<GameplayStyle> get_target_style() const;

    // Programmatic broadcast & injection API
    void broadcast_hot_reload_style(const godot::Ref<GameplayStyle> &p_style);
    void broadcast_hot_reload_rule(const godot::Ref<GameplayRule> &p_rule);
    void inject_serial_line(const godot::String &p_line);
    void inject_hid_bytes(const godot::PackedByteArray &p_bytes);
    void inject_vr_controller_state(
        const godot::StringName &p_tracker_hand,
        const godot::Transform3D &p_pose,
        const godot::Vector3 &p_linear_velocity,
        const godot::Vector2 &p_thumbstick,
        float p_trigger,
        float p_grip,
        int64_t p_button_mask
    );
};

} // namespace ideam::godot_ext