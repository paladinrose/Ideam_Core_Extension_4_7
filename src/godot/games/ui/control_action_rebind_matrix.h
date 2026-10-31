#pragma once

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include "../gameplay/gameplay_control_map.h"

namespace ideam::godot_ext {

/**
 * DOD NOTE: ControlActionRebindMatrix provides a modular, interactive action-rebinding
 * and Custom USB / Arduino ADC channel calibration matrix for `GameplayControlMap`.
 *
 * It supports:
 * 1. Semantic Action Rebinding: Displays all 15 standard semantic actions (`move_forward`,
 *    `primary_action`, `interact`, `jump`, etc.) plus custom entries in `action_remap_table`
 *    and `action_to_agent_action_map`, with a one-click "Listen for Input..." capture mode
 *    that intercepts Keyboard keys, Mouse buttons, Joypad buttons/axes, or Custom USB channels.
 * 2. binding Conflict Detection: Highlights duplicate input bindings across actions.
 * 3. Custom USB / Arduino Serial & HID Channel Monitor & Calibrator: Subscribes to
 *    `custom_packet_received` on `GameplayControlMap`, displaying live normalized channel
 *    strengths (`AX`, `AY`, `LX`, `LY`, `BTN1`..`BTN4`) and one-click Min/Center/Max ADC
 *    calibration (`custom_channel_calibration`).
 */
class ControlActionRebindMatrix : public godot::VBoxContainer {
    GDCLASS(ControlActionRebindMatrix, godot::VBoxContainer)

protected:
    static void _bind_methods();

private:
    godot::Ref<GameplayControlMap> control_map;

    bool show_agent_action_column = true;
    bool show_usb_calibrator = true;

    bool _listening_for_input = false;
    godot::StringName _listening_semantic_action;
    godot::Button *_active_listen_button = nullptr;

    godot::Label *status_banner = nullptr;
    godot::VBoxContainer *rows_container = nullptr;
    godot::VBoxContainer *usb_channels_container = nullptr;
    godot::LineEdit *serial_test_input = nullptr;

    void _on_control_map_changed();
    void _on_custom_packet_received(const godot::Dictionary &p_channel_values);
    void _rebuild_matrix();
    void _rebuild_usb_calibrator();

    void _on_rebind_button_pressed(const godot::StringName &p_semantic_action, godot::Object *p_btn);
    void _on_agent_action_text_submitted(const godot::String &p_text, const godot::StringName &p_semantic_action);
    void _on_calibrate_channel_center(const godot::String &p_channel);
    void _on_send_test_serial_line();

public:
    ControlActionRebindMatrix();
    ~ControlActionRebindMatrix() override;

    virtual void _ready() override;
    virtual void _input(const godot::Ref<godot::InputEvent> &p_event) override;

    // --- Setters / Getters ---
    void set_control_map(const godot::Ref<GameplayControlMap> &p_map);
    godot::Ref<GameplayControlMap> get_control_map() const;

    void set_show_agent_action_column(bool p_show);
    bool get_show_agent_action_column() const;

    void set_show_usb_calibrator(bool p_show);
    bool get_show_usb_calibrator() const;

    // --- Public Operations ---
    void begin_listening_for_action(const godot::StringName &p_semantic_action);
    void cancel_listening();
    bool is_listening_for_input() const;
    void reset_all_remaps();
};

} // namespace ideam::godot_ext