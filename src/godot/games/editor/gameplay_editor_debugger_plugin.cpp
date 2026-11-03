#include "gameplay_editor_debugger_plugin.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include "../game.h"
#include "../game_entity.h"
#include "../gameplay/gameplay_mode.h"

namespace ideam::godot_ext {

void GameplayEditorDebuggerPlugin::_bind_methods() {
    godot::ClassDB::bind_method(
        godot::D_METHOD("set_target_control_map", "control_map"),
        &GameplayEditorDebuggerPlugin::set_target_control_map
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_target_control_map"),
        &GameplayEditorDebuggerPlugin::get_target_control_map
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("set_target_style", "style"),
        &GameplayEditorDebuggerPlugin::set_target_style
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_target_style"),
        &GameplayEditorDebuggerPlugin::get_target_style
    );

    godot::ClassDB::bind_method(
        godot::D_METHOD("broadcast_hot_reload_style", "style"),
        &GameplayEditorDebuggerPlugin::broadcast_hot_reload_style
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("broadcast_hot_reload_rule", "rule"),
        &GameplayEditorDebuggerPlugin::broadcast_hot_reload_rule
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("inject_serial_line", "line"),
        &GameplayEditorDebuggerPlugin::inject_serial_line
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("inject_hid_bytes", "bytes"),
        &GameplayEditorDebuggerPlugin::inject_hid_bytes
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD(
            "inject_vr_controller_state",
            "tracker_hand",
            "pose",
            "linear_velocity",
            "thumbstick",
            "trigger",
            "grip",
            "button_mask"
        ),
        &GameplayEditorDebuggerPlugin::inject_vr_controller_state
    );

    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_hot_reload_cascade_pressed"),
        &GameplayEditorDebuggerPlugin::_on_hot_reload_cascade_pressed
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_send_serial_line_pressed"),
        &GameplayEditorDebuggerPlugin::_on_send_serial_line_pressed
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_send_serial_preset_pressed", "preset"),
        &GameplayEditorDebuggerPlugin::_on_send_serial_preset_pressed
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_send_hid_bytes_pressed"),
        &GameplayEditorDebuggerPlugin::_on_send_hid_bytes_pressed
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("_on_send_vr_state_pressed"),
        &GameplayEditorDebuggerPlugin::_on_send_vr_state_pressed
    );
}

GameplayEditorDebuggerPlugin::GameplayEditorDebuggerPlugin() {}

GameplayEditorDebuggerPlugin::~GameplayEditorDebuggerPlugin() {}

bool GameplayEditorDebuggerPlugin::_has_capture(const godot::String &p_capture) const {
    return p_capture == "ideam_games";
}

bool GameplayEditorDebuggerPlugin::_capture(
    const godot::String &p_message,
    const godot::Array &p_data,
    int32_t p_session_id
) {
    if (p_message == "ideam_games:telemetry_ack") {
        godot::String ack_info = p_data.size() > 0 ? godot::String(p_data[0]) : "OK";
        _set_status_message(
            "Session #" + godot::String::num_int64(p_session_id) + " ACK: " + ack_info
        );
        return true;
    }
    return false;
}

void GameplayEditorDebuggerPlugin::_setup_session(int32_t p_session_id) {
    godot::Ref<godot::EditorDebuggerSession> session = get_session(p_session_id);
    if (session.is_null()) {
        return;
    }

    godot::Control *tab = _build_simulator_tab(p_session_id);
    session->add_session_tab(tab);
}

void GameplayEditorDebuggerPlugin::set_target_control_map(
    const godot::Ref<GameplayControlMap> &p_map
) {
    _target_control_map = p_map;
}

godot::Ref<GameplayControlMap> GameplayEditorDebuggerPlugin::get_target_control_map() const {
    return _target_control_map;
}

void GameplayEditorDebuggerPlugin::set_target_style(const godot::Ref<GameplayStyle> &p_style) {
    _target_style = p_style;
}

godot::Ref<GameplayStyle> GameplayEditorDebuggerPlugin::get_target_style() const {
    return _target_style;
}

void GameplayEditorDebuggerPlugin::_resolve_active_resources_from_editor() {
#ifdef TOOLS_ENABLED
    if (!godot::EditorInterface::get_singleton()) {
        return;
    }
    godot::Node *scene_root = godot::EditorInterface::get_singleton()->get_edited_scene_root();
    if (!scene_root) {
        return;
    }

    if (_target_style.is_null()) {
        if (Game *game = godot::Object::cast_to<Game>(scene_root)) {
            _target_style = game->get_gameplay_style();
            if (_target_style.is_null()) {
                _target_style = game->get_resolved_gameplay_style();
            }
        } else if (GameEntity *entity = godot::Object::cast_to<GameEntity>(scene_root)) {
            _target_style = entity->get_gameplay_style();
            if (_target_style.is_null()) {
                _target_style = entity->get_resolved_gameplay_style();
            }
        }
    }

    if (_target_style.is_valid() && _target_control_map.is_null()) {
        godot::TypedArray<GameplayMode> modes = _target_style->get_modes();
        for (int i = 0; i < modes.size(); ++i) {
            GameplayMode *mode = godot::Object::cast_to<GameplayMode>(modes[i]);
            if (!mode || !mode->get_enabled()) {
                continue;
            }
            godot::TypedArray<GameplayControlMap> control_maps = mode->get_control_maps();
            for (int j = 0; j < control_maps.size(); ++j) {
                GameplayControlMap *cmap = godot::Object::cast_to<GameplayControlMap>(control_maps[j]);
                if (cmap && cmap->get_enabled()) {
                    _target_control_map = godot::Ref<GameplayControlMap>(cmap);
                    break;
                }
            }
            if (_target_control_map.is_valid()) {
                break;
            }
        }
    }
#endif
}

void GameplayEditorDebuggerPlugin::_set_status_message(const godot::String &p_msg) {
    if (_status_label_id.is_null()) {
        return;
    }
    if (godot::Label *lbl = godot::Object::cast_to<godot::Label>(
            godot::ObjectDB::get_instance(_status_label_id)
        )) {
        lbl->set_text(p_msg);
    }
}

godot::Control *GameplayEditorDebuggerPlugin::_build_simulator_tab(int32_t p_session_id) {
    godot::ScrollContainer *scroll = memnew(godot::ScrollContainer);
    scroll->set_name("Ideam Gameplay & Hardware Sim");
    scroll->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    scroll->set_v_size_flags(godot::Control::SIZE_EXPAND_FILL);

    godot::VBoxContainer *root = memnew(godot::VBoxContainer);
    root->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    root->add_theme_constant_override("separation", 8);
    scroll->add_child(root);

    // --- Section 1: Live Cascade Hot-Reload ---
    godot::HBoxContainer *reload_row = memnew(godot::HBoxContainer);
    reload_row->add_theme_constant_override("separation", 8);

    godot::Label *reload_title = memnew(godot::Label);
    reload_title->set_text("1. Live Cascade Hot-Reload (Session #" + godot::String::num_int64(p_session_id) + ")");
    reload_title->add_theme_color_override("font_color", godot::Color(0.55f, 0.85f, 1.0f));
    reload_title->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    reload_row->add_child(reload_title);

    godot::Button *hot_reload_btn = memnew(godot::Button);
    hot_reload_btn->set_text("Broadcast Active GameplayStyle / Cascade Now");
    hot_reload_btn->connect(
        "pressed",
        godot::Callable(this, "_on_hot_reload_cascade_pressed")
    );
    reload_row->add_child(hot_reload_btn);
    root->add_child(reload_row);

    root->add_child(memnew(godot::HSeparator));

    // --- Section 2: Custom USB / Arduino Serial & HID Injector ---
    godot::Label *serial_title = memnew(godot::Label);
    serial_title->set_text("2. Custom USB / Arduino Serial & Raw HID Packet Injector");
    serial_title->add_theme_color_override("font_color", godot::Color(0.60f, 0.92f, 0.68f));
    root->add_child(serial_title);

    godot::HBoxContainer *serial_row = memnew(godot::HBoxContainer);
    serial_row->add_theme_constant_override("separation", 6);

    godot::Label *serial_lbl = memnew(godot::Label);
    serial_lbl->set_text("Serial Line (KEY:VAL):");
    serial_row->add_child(serial_lbl);

    godot::LineEdit *serial_edit = memnew(godot::LineEdit);
    serial_edit->set_text("AX:1023,AY:512,BTN1:1,ENC1:4");
    serial_edit->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    _serial_line_edit_id = godot::ObjectID(serial_edit->get_instance_id());
    serial_row->add_child(serial_edit);

    godot::Button *send_serial_btn = memnew(godot::Button);
    send_serial_btn->set_text("Feed Serial Line");
    send_serial_btn->connect(
        "pressed",
        godot::Callable(this, "_on_send_serial_line_pressed")
    );
    serial_row->add_child(send_serial_btn);

    godot::Button *preset_neutral_btn = memnew(godot::Button);
    preset_neutral_btn->set_text("Neutral (512,512)");
    preset_neutral_btn->connect(
        "pressed",
        godot::Callable(this, "_on_send_serial_preset_pressed")
            .bind(godot::String("AX:512,AY:512,BTN1:0,BTN2:0"))
    );
    serial_row->add_child(preset_neutral_btn);

    godot::Button *preset_burst_btn = memnew(godot::Button);
    preset_burst_btn->set_text("Full Tilt + Fire");
    preset_burst_btn->connect(
        "pressed",
        godot::Callable(this, "_on_send_serial_preset_pressed")
            .bind(godot::String("AX:1023,AY:1023,BTN1:1,BTN2:1"))
    );
    serial_row->add_child(preset_burst_btn);
    root->add_child(serial_row);

    // Raw HID Hex Row
    godot::HBoxContainer *hid_row = memnew(godot::HBoxContainer);
    hid_row->add_theme_constant_override("separation", 6);

    godot::Label *hid_lbl = memnew(godot::Label);
    hid_lbl->set_text("Raw HID Packet (Hex Bytes):");
    hid_row->add_child(hid_lbl);

    godot::LineEdit *hid_edit = memnew(godot::LineEdit);
    hid_edit->set_text("FF 03 00 02 01 00 80 7F");
    hid_edit->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    _hid_hex_edit_id = godot::ObjectID(hid_edit->get_instance_id());
    hid_row->add_child(hid_edit);

    godot::Button *send_hid_btn = memnew(godot::Button);
    send_hid_btn->set_text("Feed HID Packet");
    send_hid_btn->connect(
        "pressed",
        godot::Callable(this, "_on_send_hid_bytes_pressed")
    );
    hid_row->add_child(send_hid_btn);
    root->add_child(hid_row);

    root->add_child(memnew(godot::HSeparator));

    // --- Section 3: Simulated VR / OpenXR 6DoF Controller Injector ---
    godot::Label *vr_title = memnew(godot::Label);
    vr_title->set_text("3. Simulated VR / OpenXR 6DoF Controller & Swing Injector");
    vr_title->add_theme_color_override("font_color", godot::Color(0.95f, 0.78f, 0.45f));
    root->add_child(vr_title);

    godot::HBoxContainer *vr_row1 = memnew(godot::HBoxContainer);
    vr_row1->add_theme_constant_override("separation", 8);

    godot::Label *hand_lbl = memnew(godot::Label);
    hand_lbl->set_text("Tracker:");
    vr_row1->add_child(hand_lbl);

    godot::OptionButton *tracker_opt = memnew(godot::OptionButton);
    tracker_opt->add_item("left_hand");
    tracker_opt->add_item("right_hand");
    tracker_opt->select(1);
    _vr_tracker_option_id = godot::ObjectID(tracker_opt->get_instance_id());
    vr_row1->add_child(tracker_opt);

    godot::Label *trig_lbl = memnew(godot::Label);
    trig_lbl->set_text("Trigger:");
    vr_row1->add_child(trig_lbl);

    godot::HSlider *trig_slider = memnew(godot::HSlider);
    trig_slider->set_min(0.0);
    trig_slider->set_max(1.0);
    trig_slider->set_step(0.05);
    trig_slider->set_value(0.85);
    trig_slider->set_custom_minimum_size(godot::Vector2(90.0f, 0.0f));
    _vr_trigger_slider_id = godot::ObjectID(trig_slider->get_instance_id());
    vr_row1->add_child(trig_slider);

    godot::Label *grip_lbl = memnew(godot::Label);
    grip_lbl->set_text("Grip:");
    vr_row1->add_child(grip_lbl);

    godot::HSlider *grip_slider = memnew(godot::HSlider);
    grip_slider->set_min(0.0);
    grip_slider->set_max(1.0);
    grip_slider->set_step(0.05);
    grip_slider->set_value(0.50);
    grip_slider->set_custom_minimum_size(godot::Vector2(90.0f, 0.0f));
    _vr_grip_slider_id = godot::ObjectID(grip_slider->get_instance_id());
    vr_row1->add_child(grip_slider);

    godot::CheckBox *btn_check = memnew(godot::CheckBox);
    btn_check->set_text("Primary Button Pressed");
    btn_check->set_pressed(true);
    _vr_primary_btn_check_id = godot::ObjectID(btn_check->get_instance_id());
    vr_row1->add_child(btn_check);

    root->add_child(vr_row1);

    godot::HBoxContainer *vr_row2 = memnew(godot::HBoxContainer);
    vr_row2->add_theme_constant_override("separation", 8);

    godot::Label *stick_lbl = memnew(godot::Label);
    stick_lbl->set_text("Thumbstick (X/Y):");
    vr_row2->add_child(stick_lbl);

    godot::HSlider *sx_slider = memnew(godot::HSlider);
    sx_slider->set_min(-1.0);
    sx_slider->set_max(1.0);
    sx_slider->set_step(0.05);
    sx_slider->set_value(0.0);
    sx_slider->set_custom_minimum_size(godot::Vector2(80.0f, 0.0f));
    _vr_stick_x_slider_id = godot::ObjectID(sx_slider->get_instance_id());
    vr_row2->add_child(sx_slider);

    godot::HSlider *sy_slider = memnew(godot::HSlider);
    sy_slider->set_min(-1.0);
    sy_slider->set_max(1.0);
    sy_slider->set_step(0.05);
    sy_slider->set_value(0.65);
    sy_slider->set_custom_minimum_size(godot::Vector2(80.0f, 0.0f));
    _vr_stick_y_slider_id = godot::ObjectID(sy_slider->get_instance_id());
    vr_row2->add_child(sy_slider);

    godot::Label *vel_lbl = memnew(godot::Label);
    vel_lbl->set_text("Swing Velocity (m/s X/Y/Z):");
    vr_row2->add_child(vel_lbl);

    godot::SpinBox *vx_spin = memnew(godot::SpinBox);
    vx_spin->set_min(-20.0);
    vx_spin->set_max(20.0);
    vx_spin->set_step(0.25);
    vx_spin->set_value(2.5);
    _vr_vel_x_spin_id = godot::ObjectID(vx_spin->get_instance_id());
    vr_row2->add_child(vx_spin);

    godot::SpinBox *vy_spin = memnew(godot::SpinBox);
    vy_spin->set_min(-20.0);
    vy_spin->set_max(20.0);
    vy_spin->set_step(0.25);
    vy_spin->set_value(-1.5);
    _vr_vel_y_spin_id = godot::ObjectID(vy_spin->get_instance_id());
    vr_row2->add_child(vy_spin);

    godot::SpinBox *vz_spin = memnew(godot::SpinBox);
    vz_spin->set_min(-20.0);
    vz_spin->set_max(20.0);
    vz_spin->set_step(0.25);
    vz_spin->set_value(-3.0);
    _vr_vel_z_spin_id = godot::ObjectID(vz_spin->get_instance_id());
    vr_row2->add_child(vz_spin);

    godot::Button *send_vr_btn = memnew(godot::Button);
    send_vr_btn->set_text("Inject 6DoF VR State");
    send_vr_btn->connect(
        "pressed",
        godot::Callable(this, "_on_send_vr_state_pressed")
    );
    vr_row2->add_child(send_vr_btn);

    root->add_child(vr_row2);

    // Status Footer
    godot::Label *status_lbl = memnew(godot::Label);
    status_lbl->set_text("Ready. Select or edit a Game / GameplayControlMap to test live telemetry.");
    status_lbl->add_theme_color_override("font_color", godot::Color(0.62f, 0.68f, 0.78f));
    _status_label_id = godot::ObjectID(status_lbl->get_instance_id());
    root->add_child(status_lbl);

    return scroll;
}

// Helper in anonymous namespace or static helper in gameplay_editor_debugger_plugin.cpp
static void _apply_rule_to_scene_tree_recursive(
    godot::Node *p_node,
    const godot::Ref<GameplayRule> &p_rule,
    bool p_is_root
) {
    if (!p_node || p_rule.is_null()) {
        return;
    }
    p_rule->apply_to_object(p_node, p_is_root);

    int32_t child_count = p_node->get_child_count();
    for (int32_t i = 0; i < child_count; ++i) {
        _apply_rule_to_scene_tree_recursive(p_node->get_child(i), p_rule, false);
    }
}

void GameplayEditorDebuggerPlugin::broadcast_hot_reload_style(
    const godot::Ref<GameplayStyle> &p_style
) {
    godot::Ref<GameplayStyle> style = p_style.is_valid() ? p_style : _target_style;
    if (style.is_null()) {
        _resolve_active_resources_from_editor();
        style = _target_style;
    }

    // 1. Apply in-editor to edited scene root if it's a Game / GameEntity (or generic Node)
#ifdef TOOLS_ENABLED
    if (style.is_valid() && godot::EditorInterface::get_singleton()) {
        godot::Node *scene_root = godot::EditorInterface::get_singleton()->get_edited_scene_root();
        if (scene_root) {
            if (Game *game = godot::Object::cast_to<Game>(scene_root)) {
                game->apply_gameplay_style(style);
            } else if (GameEntity *entity = godot::Object::cast_to<GameEntity>(scene_root)) {
                entity->apply_gameplay_style(style);
            } else {
                style->apply_rules_to_object(scene_root, true, true);
            }
        }
    }
#endif

    // 2. Broadcast over all active debugger sessions
    godot::Array payload;
    if (style.is_valid()) {
        payload.append(style->get_path());
        payload.append(style->collect_rules().size());
    }
    godot::Array sessions = get_sessions();
    int active_sent = 0;
    for (int i = 0; i < sessions.size(); ++i) {
        godot::Ref<godot::EditorDebuggerSession> sess = sessions[i];
        if (sess.is_valid() && sess->is_active()) {
            sess->send_message("ideam_games:apply_style", payload);
            ++active_sent;
        }
    }

    _set_status_message(
        "Hot-reloaded GameplayStyle cascade (applied to scene + sent to " +
        godot::String::num_int64(active_sent) + " active debug session(s))."
    );
}

void GameplayEditorDebuggerPlugin::broadcast_hot_reload_rule(
    const godot::Ref<GameplayRule> &p_rule
) {
    if (p_rule.is_null()) {
        return;
    }

#ifdef TOOLS_ENABLED
    if (godot::EditorInterface::get_singleton()) {
        godot::Node *scene_root = godot::EditorInterface::get_singleton()->get_edited_scene_root();
        if (scene_root) {
            _apply_rule_to_scene_tree_recursive(scene_root, p_rule, true);
        }
    }
#endif

    godot::Array payload;
    payload.append(p_rule->get_target_class());
    payload.append(p_rule->get_target_title_or_group());
    payload.append(p_rule->get_target_property());
    payload.append(p_rule->get_value());

    godot::Array sessions = get_sessions();
    for (int i = 0; i < sessions.size(); ++i) {
        godot::Ref<godot::EditorDebuggerSession> sess = sessions[i];
        if (sess.is_valid() && sess->is_active()) {
            sess->send_message("ideam_games:apply_rule", payload);
        }
    }
}

void GameplayEditorDebuggerPlugin::inject_serial_line(const godot::String &p_line) {
    _resolve_active_resources_from_editor();
    if (_target_control_map.is_valid()) {
        _target_control_map->feed_custom_serial_line(p_line);
    }

    godot::Array payload;
    payload.append(p_line);
    godot::Array sessions = get_sessions();
    for (int i = 0; i < sessions.size(); ++i) {
        godot::Ref<godot::EditorDebuggerSession> sess = sessions[i];
        if (sess.is_valid() && sess->is_active()) {
            sess->send_message("ideam_games:feed_serial_line", payload);
        }
    }

    _set_status_message("Injected USB/Arduino serial line: " + p_line);
}

void GameplayEditorDebuggerPlugin::inject_hid_bytes(const godot::PackedByteArray &p_bytes) {
    _resolve_active_resources_from_editor();
    if (_target_control_map.is_valid()) {
        _target_control_map->feed_custom_hid_bytes(p_bytes);
    }

    godot::Array payload;
    payload.append(p_bytes);
    godot::Array sessions = get_sessions();
    for (int i = 0; i < sessions.size(); ++i) {
        godot::Ref<godot::EditorDebuggerSession> sess = sessions[i];
        if (sess.is_valid() && sess->is_active()) {
            sess->send_message("ideam_games:feed_hid_bytes", payload);
        }
    }

    _set_status_message(
        "Injected raw HID packet (" + godot::String::num_int64(p_bytes.size()) + " bytes)."
    );
}

void GameplayEditorDebuggerPlugin::inject_vr_controller_state(
    const godot::StringName &p_tracker_hand,
    const godot::Transform3D &p_pose,
    const godot::Vector3 &p_linear_velocity,
    const godot::Vector2 &p_thumbstick,
    float p_trigger,
    float p_grip,
    int64_t p_button_mask
) {
    _resolve_active_resources_from_editor();
    if (_target_control_map.is_valid()) {
        int tracker_index = 1; // Default: 1 = Right Hand
        if (p_tracker_hand == godot::StringName("left_hand") ||
            p_tracker_hand == godot::StringName("left")) {
            tracker_index = 0; // 0 = Left Hand
        } else if (p_tracker_hand == godot::StringName("head") ||
                   p_tracker_hand == godot::StringName("hmd")) {
            tracker_index = 2; // 2 = Head / HMD
        }

        _target_control_map->feed_vr_controller_state(
            tracker_index,
            p_pose,
            p_linear_velocity,
            godot::Vector3(), // p_angular_velocity
            p_thumbstick,
            p_trigger,
            p_grip,
            static_cast<uint32_t>(p_button_mask)
        );
    }

    godot::Array payload;
    payload.append(p_tracker_hand);
    payload.append(p_pose);
    payload.append(p_linear_velocity);
    payload.append(p_thumbstick);
    payload.append(p_trigger);
    payload.append(p_grip);
    payload.append(p_button_mask);

    godot::Array sessions = get_sessions();
    for (int i = 0; i < sessions.size(); ++i) {
        godot::Ref<godot::EditorDebuggerSession> sess = sessions[i];
        if (sess.is_valid() && sess->is_active()) {
            sess->send_message("ideam_games:feed_vr_state", payload);
        }
    }

    _set_status_message(
        "Injected VR 6DoF state for [" + godot::String(p_tracker_hand) +
        "] | speed=" + godot::String::num(p_linear_velocity.length(), 2) + " m/s"
    );
}

void GameplayEditorDebuggerPlugin::_on_hot_reload_cascade_pressed() {
    broadcast_hot_reload_style(_target_style);
}

void GameplayEditorDebuggerPlugin::_on_send_serial_line_pressed() {
    if (_serial_line_edit_id.is_null()) {
        return;
    }
    if (godot::LineEdit *edit = godot::Object::cast_to<godot::LineEdit>(
            godot::ObjectDB::get_instance(_serial_line_edit_id)
        )) {
        inject_serial_line(edit->get_text());
    }
}

void GameplayEditorDebuggerPlugin::_on_send_serial_preset_pressed(const godot::String &p_preset) {
    if (!_serial_line_edit_id.is_null()) {
        if (godot::LineEdit *edit = godot::Object::cast_to<godot::LineEdit>(
                godot::ObjectDB::get_instance(_serial_line_edit_id)
            )) {
            edit->set_text(p_preset);
        }
    }
    inject_serial_line(p_preset);
}

void GameplayEditorDebuggerPlugin::_on_send_hid_bytes_pressed() {
    if (_hid_hex_edit_id.is_null()) {
        return;
    }
    godot::LineEdit *edit = godot::Object::cast_to<godot::LineEdit>(
        godot::ObjectDB::get_instance(_hid_hex_edit_id)
    );
    if (!edit) {
        return;
    }

    godot::PackedByteArray bytes;
    godot::PackedStringArray tokens = edit->get_text().split(" ", false);
    for (int i = 0; i < tokens.size(); ++i) {
        godot::String tok = tokens[i].strip_edges();
        if (!tok.is_empty()) {
            int64_t val = tok.hex_to_int();
            bytes.append(static_cast<uint8_t>(val & 0xFF));
        }
    }
    inject_hid_bytes(bytes);
}

void GameplayEditorDebuggerPlugin::_on_send_vr_state_pressed() {
    godot::StringName hand("right_hand");
    if (!_vr_tracker_option_id.is_null()) {
        if (godot::OptionButton *opt = godot::Object::cast_to<godot::OptionButton>(
                godot::ObjectDB::get_instance(_vr_tracker_option_id)
            )) {
            godot::String item_txt = opt->get_item_text(opt->get_selected());
            hand = godot::StringName(item_txt);
        }
    }

    float trigger = 0.0f;
    if (!_vr_trigger_slider_id.is_null()) {
        if (godot::HSlider *s = godot::Object::cast_to<godot::HSlider>(
                godot::ObjectDB::get_instance(_vr_trigger_slider_id)
            )) {
            trigger = static_cast<float>(s->get_value());
        }
    }

    float grip = 0.0f;
    if (!_vr_grip_slider_id.is_null()) {
        if (godot::HSlider *s = godot::Object::cast_to<godot::HSlider>(
                godot::ObjectDB::get_instance(_vr_grip_slider_id)
            )) {
            grip = static_cast<float>(s->get_value());
        }
    }

    godot::Vector2 stick;
    if (!_vr_stick_x_slider_id.is_null()) {
        if (godot::HSlider *s = godot::Object::cast_to<godot::HSlider>(
                godot::ObjectDB::get_instance(_vr_stick_x_slider_id)
            )) {
            stick.x = static_cast<float>(s->get_value());
        }
    }
    if (!_vr_stick_y_slider_id.is_null()) {
        if (godot::HSlider *s = godot::Object::cast_to<godot::HSlider>(
                godot::ObjectDB::get_instance(_vr_stick_y_slider_id)
            )) {
            stick.y = static_cast<float>(s->get_value());
        }
    }

    godot::Vector3 vel;
    if (!_vr_vel_x_spin_id.is_null()) {
        if (godot::SpinBox *sp = godot::Object::cast_to<godot::SpinBox>(
                godot::ObjectDB::get_instance(_vr_vel_x_spin_id)
            )) {
            vel.x = static_cast<float>(sp->get_value());
        }
    }
    if (!_vr_vel_y_spin_id.is_null()) {
        if (godot::SpinBox *sp = godot::Object::cast_to<godot::SpinBox>(
                godot::ObjectDB::get_instance(_vr_vel_y_spin_id)
            )) {
            vel.y = static_cast<float>(sp->get_value());
        }
    }
    if (!_vr_vel_z_spin_id.is_null()) {
        if (godot::SpinBox *sp = godot::Object::cast_to<godot::SpinBox>(
                godot::ObjectDB::get_instance(_vr_vel_z_spin_id)
            )) {
            vel.z = static_cast<float>(sp->get_value());
        }
    }

    int64_t btn_mask = 0;
    if (!_vr_primary_btn_check_id.is_null()) {
        if (godot::CheckBox *cb = godot::Object::cast_to<godot::CheckBox>(
                godot::ObjectDB::get_instance(_vr_primary_btn_check_id)
            )) {
            if (cb->is_pressed()) {
                btn_mask = 1;
            }
        }
    }

    inject_vr_controller_state(
        hand,
        godot::Transform3D(),
        vel,
        stick,
        trigger,
        grip,
        btn_mask
    );
}

} // namespace ideam::godot_ext