#include "control_action_rebind_matrix.h"

#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

void ControlActionRebindMatrix::_bind_methods() {
    ADD_SIGNAL(godot::MethodInfo(
        "action_rebound",
        godot::PropertyInfo(godot::Variant::STRING_NAME, "semantic_action"),
        godot::PropertyInfo(godot::Variant::STRING, "binding_description")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "binding_conflict_detected",
        godot::PropertyInfo(godot::Variant::STRING_NAME, "action_a"),
        godot::PropertyInfo(godot::Variant::STRING_NAME, "action_b")
    ));

    godot::ClassDB::bind_method(godot::D_METHOD("set_control_map", "control_map"), &ControlActionRebindMatrix::set_control_map);
    godot::ClassDB::bind_method(godot::D_METHOD("get_control_map"), &ControlActionRebindMatrix::get_control_map);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "control_map", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayControlMap"), "set_control_map", "get_control_map");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_agent_action_column", "show"), &ControlActionRebindMatrix::set_show_agent_action_column);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_agent_action_column"), &ControlActionRebindMatrix::get_show_agent_action_column);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_agent_action_column"), "set_show_agent_action_column", "get_show_agent_action_column");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_usb_calibrator", "show"), &ControlActionRebindMatrix::set_show_usb_calibrator);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_usb_calibrator"), &ControlActionRebindMatrix::get_show_usb_calibrator);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_usb_calibrator"), "set_show_usb_calibrator", "get_show_usb_calibrator");

    godot::ClassDB::bind_method(godot::D_METHOD("begin_listening_for_action", "semantic_action"), &ControlActionRebindMatrix::begin_listening_for_action);
    godot::ClassDB::bind_method(godot::D_METHOD("cancel_listening"), &ControlActionRebindMatrix::cancel_listening);
    godot::ClassDB::bind_method(godot::D_METHOD("is_listening_for_input"), &ControlActionRebindMatrix::is_listening_for_input);
    godot::ClassDB::bind_method(godot::D_METHOD("reset_all_remaps"), &ControlActionRebindMatrix::reset_all_remaps);

    godot::ClassDB::bind_method(godot::D_METHOD("_on_control_map_changed"), &ControlActionRebindMatrix::_on_control_map_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_custom_packet_received", "channel_values"), &ControlActionRebindMatrix::_on_custom_packet_received);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_rebind_button_pressed", "semantic_action", "btn"), &ControlActionRebindMatrix::_on_rebind_button_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_agent_action_text_submitted", "text", "semantic_action"), &ControlActionRebindMatrix::_on_agent_action_text_submitted);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_calibrate_channel_center", "channel"), &ControlActionRebindMatrix::_on_calibrate_channel_center);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_send_test_serial_line"), &ControlActionRebindMatrix::_on_send_test_serial_line);
}

ControlActionRebindMatrix::ControlActionRebindMatrix() {
    set_process_input(false);
}

ControlActionRebindMatrix::~ControlActionRebindMatrix() {
}

void ControlActionRebindMatrix::_ready() {
    _rebuild_matrix();
}

void ControlActionRebindMatrix::set_control_map(const godot::Ref<GameplayControlMap> &p_map) {
    if (control_map == p_map) return;

    if (control_map.is_valid()) {
        if (control_map->is_connected("changed", godot::Callable(this, "_on_control_map_changed"))) {
            control_map->disconnect("changed", godot::Callable(this, "_on_control_map_changed"));
        }
        if (control_map->is_connected("custom_packet_received", godot::Callable(this, "_on_custom_packet_received"))) {
            control_map->disconnect("custom_packet_received", godot::Callable(this, "_on_custom_packet_received"));
        }
    }

    control_map = p_map;

    if (control_map.is_valid()) {
        if (!control_map->is_connected("changed", godot::Callable(this, "_on_control_map_changed"))) {
            control_map->connect("changed", godot::Callable(this, "_on_control_map_changed"));
        }
        if (!control_map->is_connected("custom_packet_received", godot::Callable(this, "_on_custom_packet_received"))) {
            control_map->connect("custom_packet_received", godot::Callable(this, "_on_custom_packet_received"));
        }
    }

    if (is_inside_tree()) {
        _rebuild_matrix();
    }
}

godot::Ref<GameplayControlMap> ControlActionRebindMatrix::get_control_map() const {
    return control_map;
}

void ControlActionRebindMatrix::set_show_agent_action_column(bool p_show) {
    show_agent_action_column = p_show;
    if (is_inside_tree()) _rebuild_matrix();
}
bool ControlActionRebindMatrix::get_show_agent_action_column() const { return show_agent_action_column; }

void ControlActionRebindMatrix::set_show_usb_calibrator(bool p_show) {
    show_usb_calibrator = p_show;
    if (is_inside_tree()) _rebuild_matrix();
}
bool ControlActionRebindMatrix::get_show_usb_calibrator() const { return show_usb_calibrator; }

void ControlActionRebindMatrix::_on_control_map_changed() {
    if (!_listening_for_input && is_inside_tree()) {
        _rebuild_matrix();
    }
}

void ControlActionRebindMatrix::_on_custom_packet_received(const godot::Dictionary &p_channel_values) {
    if (show_usb_calibrator && is_inside_tree()) {
        _rebuild_usb_calibrator();
    }
}

void ControlActionRebindMatrix::_rebuild_matrix() {
    for (int i = get_child_count() - 1; i >= 0; --i) {
        godot::Node *ch = get_child(i);
        remove_child(ch);
        ch->queue_free();
    }

    status_banner = memnew(godot::Label);
    status_banner->set_text("Click any action binding below to capture a new Key, Mouse, Gamepad, or USB input.");
    add_child(status_banner);

    rows_container = memnew(godot::VBoxContainer);
    rows_container->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    add_child(rows_container);

    if (!control_map.is_valid()) {
        control_map.instantiate();
    }

    struct ActionRowSpec {
        const char *label;
        godot::StringName semantic_name;
    };

    ActionRowSpec specs[] = {
        {"Move Forward", control_map->get_move_forward_action()},
        {"Move Backward", control_map->get_move_backward_action()},
        {"Move Left", control_map->get_move_left_action()},
        {"Move Right", control_map->get_move_right_action()},
        {"Look Up", control_map->get_look_up_action()},
        {"Look Down", control_map->get_look_down_action()},
        {"Look Left", control_map->get_look_left_action()},
        {"Look Right", control_map->get_look_right_action()},
        {"Primary Action / Fire", control_map->get_primary_action()},
        {"Secondary Action / Aim", control_map->get_secondary_action()},
        {"Interact / Use", control_map->get_interact_action()},
        {"Jump", control_map->get_jump_action()},
        {"Sprint", control_map->get_sprint_action()},
        {"Crouch", control_map->get_crouch_action()},
        {"Pause", control_map->get_pause_action()}
    };

    godot::Dictionary remap_table = control_map->get_action_remap_table();
    godot::Dictionary agent_map = control_map->get_action_to_agent_action_map();

    for (int i = 0; i < 15; ++i) {
        godot::String sem_str(specs[i].semantic_name);
        godot::String mapped_target = remap_table.has(sem_str) ? godot::String(remap_table[sem_str]) : sem_str;

        godot::HBoxContainer *row = memnew(godot::HBoxContainer);
        rows_container->add_child(row);

        godot::Label *lbl = memnew(godot::Label);
        lbl->set_text(specs[i].label);
        lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        row->add_child(lbl);

        godot::Button *bind_btn = memnew(godot::Button);
        bind_btn->set_custom_minimum_size(godot::Vector2(170.0f, 0.0f));
        bind_btn->set_text(mapped_target);
        bind_btn->connect("pressed", godot::Callable(this, "_on_rebind_button_pressed").bind(specs[i].semantic_name, bind_btn));
        row->add_child(bind_btn);

        if (show_agent_action_column) {
            godot::LineEdit *agent_edit = memnew(godot::LineEdit);
            agent_edit->set_custom_minimum_size(godot::Vector2(140.0f, 0.0f));
            agent_edit->set_placeholder("GameAgentAction...");
            if (agent_map.has(sem_str)) {
                agent_edit->set_text(godot::String(agent_map[sem_str]));
            }
            agent_edit->connect("text_submitted", godot::Callable(this, "_on_agent_action_text_submitted").bind(specs[i].semantic_name));
            row->add_child(agent_edit);
        }
    }

    if (show_usb_calibrator) {
        godot::HSeparator *sep = memnew(godot::HSeparator);
        add_child(sep);

        godot::Label *usb_header = memnew(godot::Label);
        usb_header->set_text("Custom USB / Arduino Serial Telemetry & Calibration");
        add_child(usb_header);

        godot::HBoxContainer *test_bar = memnew(godot::HBoxContainer);
        add_child(test_bar);

        serial_test_input = memnew(godot::LineEdit);
        serial_test_input->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        serial_test_input->set_placeholder("Test Serial Packet (e.g. AX:768,AY:512,BTN1:1)");
        serial_test_input->set_text("AX:512,AY:512,BTN1:0");
        test_bar->add_child(serial_test_input);

        godot::Button *send_btn = memnew(godot::Button);
        send_btn->set_text("Feed Serial Line");
        send_btn->connect("pressed", godot::Callable(this, "_on_send_test_serial_line"));
        test_bar->add_child(send_btn);

        usb_channels_container = memnew(godot::VBoxContainer);
        add_child(usb_channels_container);
        _rebuild_usb_calibrator();
    }
}

void ControlActionRebindMatrix::_rebuild_usb_calibrator() {
    if (!usb_channels_container || !control_map.is_valid()) return;

    for (int i = usb_channels_container->get_child_count() - 1; i >= 0; --i) {
        godot::Node *ch = usb_channels_container->get_child(i);
        usb_channels_container->remove_child(ch);
        ch->queue_free();
    }

    godot::Dictionary ch_map = control_map->get_custom_channel_to_action_map();
    godot::Array keys = ch_map.keys();
    for (int i = 0; i < keys.size(); ++i) {
        godot::String ch_name = keys[i];
        godot::String target_act = ch_map[keys[i]];
        float val = control_map->get_custom_channel_value(godot::StringName(ch_name));

        godot::HBoxContainer *row = memnew(godot::HBoxContainer);
        usb_channels_container->add_child(row);

        godot::Label *ch_lbl = memnew(godot::Label);
        ch_lbl->set_text(ch_name + " -> " + target_act + "  [" + godot::String::num(val, 2) + "]");
        ch_lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        row->add_child(ch_lbl);

        godot::Button *cal_btn = memnew(godot::Button);
        cal_btn->set_text("Zero / Calibrate Center");
        cal_btn->connect("pressed", godot::Callable(this, "_on_calibrate_channel_center").bind(ch_name));
        row->add_child(cal_btn);
    }
}

void ControlActionRebindMatrix::begin_listening_for_action(const godot::StringName &p_semantic_action) {
    _listening_for_input = true;
    _listening_semantic_action = p_semantic_action;
    set_process_input(true);
    if (status_banner) {
        status_banner->set_text("Listening for [" + godot::String(p_semantic_action) + "]... Press any Key, Mouse Button, or Gamepad Button (ESC to cancel).");
    }
}

void ControlActionRebindMatrix::cancel_listening() {
    _listening_for_input = false;
    _listening_semantic_action = godot::StringName();
    _active_listen_button = nullptr;
    set_process_input(false);
    if (status_banner) {
        status_banner->set_text("Binding capture canceled.");
    }
    _rebuild_matrix();
}

bool ControlActionRebindMatrix::is_listening_for_input() const {
    return _listening_for_input;
}

void ControlActionRebindMatrix::reset_all_remaps() {
    if (control_map.is_valid()) {
        control_map->set_action_remap_table(godot::Dictionary());
        _rebuild_matrix();
    }
}

void ControlActionRebindMatrix::_on_rebind_button_pressed(const godot::StringName &p_semantic_action, godot::Object *p_btn) {
    _active_listen_button = godot::Object::cast_to<godot::Button>(p_btn);
    if (_active_listen_button) {
        _active_listen_button->set_text("[Press Input...]");
    }
    begin_listening_for_action(p_semantic_action);
}

void ControlActionRebindMatrix::_on_agent_action_text_submitted(const godot::String &p_text, const godot::StringName &p_semantic_action) {
    if (!control_map.is_valid()) return;
    control_map->map_input_to_agent_action(p_semantic_action, p_text);
}

void ControlActionRebindMatrix::_on_calibrate_channel_center(const godot::String &p_channel) {
    if (!control_map.is_valid()) return;
    godot::Dictionary cal = control_map->get_custom_channel_calibration().duplicate();
    float max_adc = static_cast<float>(control_map->get_custom_adc_resolution_max());
    float center_adc = static_cast<float>(control_map->get_custom_adc_center_value());
    cal[p_channel] = godot::Vector3(0.0f, center_adc, max_adc);
    control_map->set_custom_channel_calibration(cal);
    if (status_banner) {
        status_banner->set_text("Calibrated ADC center for channel " + p_channel + ".");
    }
}

void ControlActionRebindMatrix::_on_send_test_serial_line() {
    if (!control_map.is_valid() || !serial_test_input) return;
    control_map->set_custom_usb_enabled(true);
    control_map->feed_custom_serial_line(serial_test_input->get_text());
    _rebuild_usb_calibrator();
}

void ControlActionRebindMatrix::_input(const godot::Ref<godot::InputEvent> &p_event) {
    if (!_listening_for_input || !p_event.is_valid() || !p_event->is_pressed() || p_event->is_echo()) {
        return;
    }

    godot::String desc;
    if (godot::InputEventKey *key_ev = godot::Object::cast_to<godot::InputEventKey>(p_event.ptr())) {
        if (key_ev->get_keycode() == godot::KEY_ESCAPE) {
            cancel_listening();
            if (get_viewport()) get_viewport()->set_input_as_handled();
            return;
        }
        desc = godot::OS::get_singleton()->get_keycode_string(key_ev->get_keycode());
    } else if (godot::InputEventMouseButton *mb_ev = godot::Object::cast_to<godot::InputEventMouseButton>(p_event.ptr())) {
        desc = "Mouse Button " + godot::String::num_int64(static_cast<int>(mb_ev->get_button_index()));
    } else if (godot::InputEventJoypadButton *jb_ev = godot::Object::cast_to<godot::InputEventJoypadButton>(p_event.ptr())) {
        desc = "Joypad Button " + godot::String::num_int64(static_cast<int>(jb_ev->get_button_index()));
    } else if (godot::InputEventJoypadMotion *jm_ev = godot::Object::cast_to<godot::InputEventJoypadMotion>(p_event.ptr())) {
        if (godot::Math::abs(jm_ev->get_axis_value()) < 0.6f) {
            return;
        }
        desc = "Joypad Axis " + godot::String::num_int64(static_cast<int>(jm_ev->get_axis())) +
               (jm_ev->get_axis_value() >= 0.0f ? "+" : "-");
    } else {
        return;
    }

    if (control_map.is_valid() && !_listening_semantic_action.is_empty()) {
        // Register event in Godot InputMap under the semantic action name and record in remap table
        godot::InputMap *imap = godot::InputMap::get_singleton();
        if (imap) {
            if (!imap->has_action(_listening_semantic_action)) {
                imap->add_action(_listening_semantic_action);
            }
            imap->action_erase_events(_listening_semantic_action);
            imap->action_add_event(_listening_semantic_action, p_event);
        }

        // Check for duplicate bindings in action_remap_table
        godot::Dictionary table = control_map->get_action_remap_table();
        godot::Array keys = table.keys();
        for (int i = 0; i < keys.size(); ++i) {
            godot::String k = keys[i];
            if (k != godot::String(_listening_semantic_action) && godot::String(table[keys[i]]) == desc) {
                emit_signal("binding_conflict_detected", _listening_semantic_action, godot::StringName(k));
            }
        }

        control_map->remap_action(_listening_semantic_action, godot::StringName(desc));
        emit_signal("action_rebound", _listening_semantic_action, desc);
    }

    _listening_for_input = false;
    _active_listen_button = nullptr;
    set_process_input(false);
    if (get_viewport()) {
        get_viewport()->set_input_as_handled();
    }
    _rebuild_matrix();
}

} // namespace ideam::godot_ext