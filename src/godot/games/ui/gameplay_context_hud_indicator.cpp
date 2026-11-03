#include "gameplay_context_hud_indicator.h"

#include <cmath>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

namespace ideam::godot_ext {

void GameplayContextHUDIndicator::_bind_methods() {
    // --- Signals ---
    ADD_SIGNAL(godot::MethodInfo(
        "hud_device_family_switched",
        godot::PropertyInfo(godot::Variant::INT, "new_family"),
        godot::PropertyInfo(godot::Variant::STRING, "family_label")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "hud_mode_toast_displayed",
        godot::PropertyInfo(godot::Variant::STRING, "headline"),
        godot::PropertyInfo(godot::Variant::STRING, "subtext")
    ));

    // --- Context & Resources Group ---
    ADD_GROUP("Context & Resources", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game", "game"), &GameplayContextHUDIndicator::set_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game"), &GameplayContextHUDIndicator::get_game);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game"),
        "set_game",
        "get_game"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_active_board", "board"), &GameplayContextHUDIndicator::set_active_board);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_board"), &GameplayContextHUDIndicator::get_active_board);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "active_board", godot::PROPERTY_HINT_NODE_TYPE, "GameBoard"),
        "set_active_board",
        "get_active_board"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_possessed_agent", "agent"), &GameplayContextHUDIndicator::set_possessed_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("get_possessed_agent"), &GameplayContextHUDIndicator::get_possessed_agent);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "possessed_agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent"),
        "set_possessed_agent",
        "get_possessed_agent"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_control_map", "control_map"), &GameplayContextHUDIndicator::set_control_map);
    godot::ClassDB::bind_method(godot::D_METHOD("get_control_map"), &GameplayContextHUDIndicator::get_control_map);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "control_map", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayControlMap"),
        "set_control_map",
        "get_control_map"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_active_style", "style"), &GameplayContextHUDIndicator::set_active_style);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_style"), &GameplayContextHUDIndicator::get_active_style);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "active_style", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayStyle"),
        "set_active_style",
        "get_active_style"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_active_mode", "mode"), &GameplayContextHUDIndicator::set_active_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_mode"), &GameplayContextHUDIndicator::get_active_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "active_mode", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayMode"),
        "set_active_mode",
        "get_active_mode"
    );

    // --- HUD Presentation Group ---
    ADD_GROUP("HUD Presentation", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_device_badge", "show"), &GameplayContextHUDIndicator::set_show_device_badge);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_device_badge"), &GameplayContextHUDIndicator::get_show_device_badge);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_device_badge"), "set_show_device_badge", "get_show_device_badge");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_action_prompts", "show"), &GameplayContextHUDIndicator::set_show_action_prompts);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_action_prompts"), &GameplayContextHUDIndicator::get_show_action_prompts);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_action_prompts"), "set_show_action_prompts", "get_show_action_prompts");

    godot::ClassDB::bind_method(godot::D_METHOD("set_show_transition_toast", "show"), &GameplayContextHUDIndicator::set_show_transition_toast);
    godot::ClassDB::bind_method(godot::D_METHOD("get_show_transition_toast"), &GameplayContextHUDIndicator::get_show_transition_toast);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "show_transition_toast"), "set_show_transition_toast", "get_show_transition_toast");

    godot::ClassDB::bind_method(godot::D_METHOD("set_enable_comfort_vignette", "enable"), &GameplayContextHUDIndicator::set_enable_comfort_vignette);
    godot::ClassDB::bind_method(godot::D_METHOD("get_enable_comfort_vignette"), &GameplayContextHUDIndicator::get_enable_comfort_vignette);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "enable_comfort_vignette"), "set_enable_comfort_vignette", "get_enable_comfort_vignette");

    godot::ClassDB::bind_method(godot::D_METHOD("set_toast_duration", "duration"), &GameplayContextHUDIndicator::set_toast_duration);
    godot::ClassDB::bind_method(godot::D_METHOD("get_toast_duration"), &GameplayContextHUDIndicator::get_toast_duration);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "toast_duration", godot::PROPERTY_HINT_RANGE, "0.5,10.0,0.1"),
        "set_toast_duration",
        "get_toast_duration"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vignette_max_intensity", "intensity"), &GameplayContextHUDIndicator::set_vignette_max_intensity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vignette_max_intensity"), &GameplayContextHUDIndicator::get_vignette_max_intensity);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "vignette_max_intensity", godot::PROPERTY_HINT_RANGE, "0.0,0.95,0.05"),
        "set_vignette_max_intensity",
        "get_vignette_max_intensity"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_vignette_smoothing", "smoothing"), &GameplayContextHUDIndicator::set_vignette_smoothing);
    godot::ClassDB::bind_method(godot::D_METHOD("get_vignette_smoothing"), &GameplayContextHUDIndicator::get_vignette_smoothing);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::FLOAT, "vignette_smoothing", godot::PROPERTY_HINT_RANGE, "1.0,30.0,0.5"),
        "set_vignette_smoothing",
        "get_vignette_smoothing"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_tracked_actions", "actions"), &GameplayContextHUDIndicator::set_tracked_actions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_tracked_actions"), &GameplayContextHUDIndicator::get_tracked_actions);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "tracked_actions", godot::PROPERTY_HINT_TYPE_STRING, "StringName"),
        "set_tracked_actions",
        "get_tracked_actions"
    );

    // --- Methods ---
    godot::ClassDB::bind_method(
        godot::D_METHOD("show_mode_transition_toast", "headline", "subtext"),
        &GameplayContextHUDIndicator::show_mode_transition_toast,
        DEFVAL("")
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("pulse_action_prompt", "action", "strength"),
        &GameplayContextHUDIndicator::pulse_action_prompt,
        DEFVAL(1.0f)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("set_manual_vignette_intensity", "weight"), &GameplayContextHUDIndicator::set_manual_vignette_intensity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_current_vignette_weight"), &GameplayContextHUDIndicator::get_current_vignette_weight);
    godot::ClassDB::bind_method(godot::D_METHOD("get_displayed_device_family"), &GameplayContextHUDIndicator::get_displayed_device_family);
    godot::ClassDB::bind_method(godot::D_METHOD("get_binding_glyph_for_action", "action"), &GameplayContextHUDIndicator::get_binding_glyph_for_action);

    // Internal Signal Callbacks
    godot::ClassDB::bind_method(godot::D_METHOD("_on_device_family_changed", "new_family", "prev_family"), &GameplayContextHUDIndicator::_on_device_family_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_action_triggered", "action", "strength"), &GameplayContextHUDIndicator::_on_action_triggered);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_action_released", "action"), &GameplayContextHUDIndicator::_on_action_released);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_vr_gesture_detected", "gesture", "hand", "speed"), &GameplayContextHUDIndicator::_on_vr_gesture_detected);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_agent_style_changed", "style"), &GameplayContextHUDIndicator::_on_agent_style_changed);
}

GameplayContextHUDIndicator::GameplayContextHUDIndicator() {
    set_clip_contents(false);
    set_mouse_filter(MOUSE_FILTER_IGNORE);
    set_process(true);

    tracked_actions.append(godot::StringName("primary_action"));
    tracked_actions.append(godot::StringName("secondary_action"));
    tracked_actions.append(godot::StringName("interact"));
    tracked_actions.append(godot::StringName("jump"));
    tracked_actions.append(godot::StringName("sprint"));
    _rebuild_action_prompts();
}

GameplayContextHUDIndicator::~GameplayContextHUDIndicator() {
    _disconnect_control_map_signals();
    _disconnect_agent_signals();
}

void GameplayContextHUDIndicator::_ready() {
    _rebuild_action_prompts();
    queue_redraw();
}

void GameplayContextHUDIndicator::_process(double p_delta) {
    float dt = static_cast<float>(p_delta);
    bool needs_redraw = false;

    if (_toast_timer > 0.0f) {
        _toast_timer = godot::Math::max(0.0f, _toast_timer - dt);
        needs_redraw = true;
    }

    for (HUDActionPromptPOD &p : _prompts) {
        if (control_map.is_valid()) {
            float live_str = control_map->get_action_strength(p.semantic_action);
            if (!godot::Math::is_equal_approx(live_str, p.active_strength)) {
                p.active_strength = live_str;
                needs_redraw = true;
            }
        }
        if (p.pulse_timer > 0.0f) {
            p.pulse_timer = godot::Math::max(0.0f, p.pulse_timer - dt * 2.5f);
            needs_redraw = true;
        }
    }

    if (enable_comfort_vignette && control_map.is_valid()) {
        bool vr_vignette_active = (control_map->get_active_device_family() == ControlDeviceFamily::DEVICE_VR_XR ||
                                   control_map->get_vr_input_enabled()) &&
                                  control_map->get_vr_comfort_vignette_on_move();
        if (vr_vignette_active) {
            float move_mag = control_map->get_unified_move_vector().length();
            float look_mag = control_map->get_unified_look_vector().length();
            _target_vignette_weight = godot::Math::clamp(move_mag * 0.85f + look_mag * 0.45f, 0.0f, 1.0f);
        }
    }

    if (!godot::Math::is_equal_approx(_current_vignette_weight, _target_vignette_weight)) {
        float alpha = godot::Math::clamp(dt * vignette_smoothing, 0.0f, 1.0f);
        _current_vignette_weight = godot::Math::lerp(_current_vignette_weight, _target_vignette_weight, alpha);
        needs_redraw = true;
    }

    if (needs_redraw) {
        queue_redraw();
    }
}

godot::Vector2 GameplayContextHUDIndicator::_get_minimum_size() const {
    return godot::Vector2(360.0f, 48.0f);
}

// --- Context & Resource Setters / Getters ---

void GameplayContextHUDIndicator::set_game(Game *p_game) {
    _game_id = p_game ? godot::ObjectID(p_game->get_instance_id()) : godot::ObjectID();
    queue_redraw();
}

Game *GameplayContextHUDIndicator::get_game() const {
    if (!_game_id.is_valid()) return nullptr;
    return godot::Object::cast_to<Game>(godot::ObjectDB::get_instance(_game_id));
}

void GameplayContextHUDIndicator::set_active_board(GameBoard *p_board) {
    godot::ObjectID new_id = p_board ? godot::ObjectID(p_board->get_instance_id()) : godot::ObjectID();
    if (_active_board_id == new_id) return;
    _active_board_id = new_id;
    if (p_board) {
        show_mode_transition_toast("Board: " + p_board->get_title(), _active_mode_title);
    }
    queue_redraw();
}

GameBoard *GameplayContextHUDIndicator::get_active_board() const {
    if (!_active_board_id.is_valid()) return nullptr;
    return godot::Object::cast_to<GameBoard>(godot::ObjectDB::get_instance(_active_board_id));
}

void GameplayContextHUDIndicator::set_possessed_agent(GameAgent *p_agent) {
    godot::ObjectID new_id = p_agent ? godot::ObjectID(p_agent->get_instance_id()) : godot::ObjectID();
    if (_possessed_agent_id == new_id) return;

    _disconnect_agent_signals();
    _possessed_agent_id = new_id;

    if (p_agent) {
        _possessed_agent_title = p_agent->get_title().is_empty() ? godot::String(p_agent->get_name()) : p_agent->get_title();
        _connect_agent_signals(p_agent);
        if (p_agent->get_resolved_gameplay_style().is_valid()) {
            set_active_style(p_agent->get_resolved_gameplay_style());
        } else if (p_agent->get_gameplay_style().is_valid()) {
            set_active_style(p_agent->get_gameplay_style());
        }
        show_mode_transition_toast("Possessed: " + _possessed_agent_title, _active_mode_title);
    } else {
        _possessed_agent_title = "";
    }

    _rebuild_action_prompts();
    queue_redraw();
}

GameAgent *GameplayContextHUDIndicator::get_possessed_agent() const {
    if (!_possessed_agent_id.is_valid()) return nullptr;
    return godot::Object::cast_to<GameAgent>(godot::ObjectDB::get_instance(_possessed_agent_id));
}

void GameplayContextHUDIndicator::set_control_map(const godot::Ref<GameplayControlMap> &p_map) {
    if (control_map == p_map) return;
    _disconnect_control_map_signals();
    control_map = p_map;
    if (control_map.is_valid()) {
        _active_device = control_map->get_active_device_family();
        _active_context_mode = control_map->get_context_mode();
        _connect_control_map_signals();
    }
    _rebuild_action_prompts();
    queue_redraw();
}

godot::Ref<GameplayControlMap> GameplayContextHUDIndicator::get_control_map() const {
    return control_map;
}

void GameplayContextHUDIndicator::set_active_style(const godot::Ref<GameplayStyle> &p_style) {
    if (active_style == p_style) return;
    active_style = p_style;
    if (active_style.is_valid() && !active_style->get_modes().is_empty()) {
        godot::Ref<GameplayMode> first_mode(godot::Object::cast_to<GameplayMode>(active_style->get_modes()[0]));
        if (first_mode.is_valid()) {
            set_active_mode(first_mode);
        }
    }
    queue_redraw();
}

godot::Ref<GameplayStyle> GameplayContextHUDIndicator::get_active_style() const {
    return active_style;
}

void GameplayContextHUDIndicator::set_active_mode(const godot::Ref<GameplayMode> &p_mode) {
    if (active_mode == p_mode) return;
    active_mode = p_mode;

    if (active_mode.is_valid()) {
        _active_mode_title = active_mode->get_title();
        if (!active_mode->get_control_maps().is_empty()) {
            godot::Ref<GameplayControlMap> cmap(godot::Object::cast_to<GameplayControlMap>(active_mode->get_control_maps()[0]));
            if (cmap.is_valid()) {
                set_control_map(cmap);
            }
        }
        show_mode_transition_toast(
            "Gameplay Mode: " + (_active_mode_title.is_empty() ? godot::String("Active") : _active_mode_title),
            _context_mode_short_name(_active_context_mode)
        );
    }
    queue_redraw();
}

godot::Ref<GameplayMode> GameplayContextHUDIndicator::get_active_mode() const {
    return active_mode;
}

// --- Display & Behavior Setters / Getters ---

void GameplayContextHUDIndicator::set_show_device_badge(bool p_show) {
    if (show_device_badge == p_show) return;
    show_device_badge = p_show;
    queue_redraw();
}
bool GameplayContextHUDIndicator::get_show_device_badge() const { return show_device_badge; }

void GameplayContextHUDIndicator::set_show_action_prompts(bool p_show) {
    if (show_action_prompts == p_show) return;
    show_action_prompts = p_show;
    queue_redraw();
}
bool GameplayContextHUDIndicator::get_show_action_prompts() const { return show_action_prompts; }

void GameplayContextHUDIndicator::set_show_transition_toast(bool p_show) {
    if (show_transition_toast == p_show) return;
    show_transition_toast = p_show;
    queue_redraw();
}
bool GameplayContextHUDIndicator::get_show_transition_toast() const { return show_transition_toast; }

void GameplayContextHUDIndicator::set_enable_comfort_vignette(bool p_enable) {
    if (enable_comfort_vignette == p_enable) return;
    enable_comfort_vignette = p_enable;
    if (!enable_comfort_vignette) {
        _target_vignette_weight = 0.0f;
        _current_vignette_weight = 0.0f;
    }
    queue_redraw();
}
bool GameplayContextHUDIndicator::get_enable_comfort_vignette() const { return enable_comfort_vignette; }

void GameplayContextHUDIndicator::set_toast_duration(float p_duration) {
    toast_duration = godot::Math::clamp(p_duration, 0.25f, 15.0f);
}
float GameplayContextHUDIndicator::get_toast_duration() const { return toast_duration; }

void GameplayContextHUDIndicator::set_vignette_max_intensity(float p_intensity) {
    vignette_max_intensity = godot::Math::clamp(p_intensity, 0.0f, 0.95f);
    queue_redraw();
}
float GameplayContextHUDIndicator::get_vignette_max_intensity() const { return vignette_max_intensity; }

void GameplayContextHUDIndicator::set_vignette_smoothing(float p_smoothing) {
    vignette_smoothing = godot::Math::clamp(p_smoothing, 0.5f, 60.0f);
}
float GameplayContextHUDIndicator::get_vignette_smoothing() const { return vignette_smoothing; }

void GameplayContextHUDIndicator::set_tracked_actions(const godot::TypedArray<godot::StringName> &p_actions) {
    if (tracked_actions == p_actions) return;
    tracked_actions = p_actions;
    _rebuild_action_prompts();
    queue_redraw();
}
godot::TypedArray<godot::StringName> GameplayContextHUDIndicator::get_tracked_actions() const { return tracked_actions; }

// --- Imperative HUD Actions & Queries ---

void GameplayContextHUDIndicator::show_mode_transition_toast(const godot::String &p_headline, const godot::String &p_subtext) {
    if (!show_transition_toast || p_headline.is_empty()) {
        return;
    }
    _toast_headline = p_headline;
    _toast_subtext = p_subtext;
    _toast_timer = toast_duration;
    emit_signal("hud_mode_toast_displayed", _toast_headline, _toast_subtext);
    queue_redraw();
}

void GameplayContextHUDIndicator::pulse_action_prompt(const godot::StringName &p_action, float p_strength) {
    for (HUDActionPromptPOD &p : _prompts) {
        if (p.semantic_action == p_action) {
            p.active_strength = godot::Math::clamp(p_strength, 0.0f, 1.0f);
            p.pulse_timer = 1.0f;
            queue_redraw();
            return;
        }
    }
}

void GameplayContextHUDIndicator::set_manual_vignette_intensity(float p_weight) {
    _target_vignette_weight = godot::Math::clamp(p_weight, 0.0f, 1.0f);
    queue_redraw();
}

float GameplayContextHUDIndicator::get_current_vignette_weight() const {
    return _current_vignette_weight;
}

ControlDeviceFamily GameplayContextHUDIndicator::get_displayed_device_family() const {
    return _active_device;
}

godot::String GameplayContextHUDIndicator::get_binding_glyph_for_action(const godot::StringName &p_action) const {
    return _resolve_binding_glyph(p_action, _active_device);
}

// --- Signal Wiring & Callbacks ---

void GameplayContextHUDIndicator::_connect_control_map_signals() {
    if (!control_map.is_valid()) return;

    godot::Callable dev_cb(this, "_on_device_family_changed");
    if (!control_map->is_connected("device_family_changed", dev_cb)) {
        control_map->connect("device_family_changed", dev_cb);
    }

    godot::Callable trig_cb(this, "_on_action_triggered");
    if (!control_map->is_connected("action_triggered", trig_cb)) {
        control_map->connect("action_triggered", trig_cb);
    }

    godot::Callable rel_cb(this, "_on_action_released");
    if (!control_map->is_connected("action_released", rel_cb)) {
        control_map->connect("action_released", rel_cb);
    }

    godot::Callable vr_cb(this, "_on_vr_gesture_detected");
    if (!control_map->is_connected("vr_gesture_detected", vr_cb)) {
        control_map->connect("vr_gesture_detected", vr_cb);
    }
}

void GameplayContextHUDIndicator::_disconnect_control_map_signals() {
    if (!control_map.is_valid()) return;

    godot::Callable dev_cb(this, "_on_device_family_changed");
    if (control_map->is_connected("device_family_changed", dev_cb)) {
        control_map->disconnect("device_family_changed", dev_cb);
    }

    godot::Callable trig_cb(this, "_on_action_triggered");
    if (control_map->is_connected("action_triggered", trig_cb)) {
        control_map->disconnect("action_triggered", trig_cb);
    }

    godot::Callable rel_cb(this, "_on_action_released");
    if (control_map->is_connected("action_released", rel_cb)) {
        control_map->disconnect("action_released", rel_cb);
    }

    godot::Callable vr_cb(this, "_on_vr_gesture_detected");
    if (control_map->is_connected("vr_gesture_detected", vr_cb)) {
        control_map->disconnect("vr_gesture_detected", vr_cb);
    }
}

void GameplayContextHUDIndicator::_connect_agent_signals(GameAgent *p_agent) {
    if (!p_agent) return;
    godot::Callable style_cb(this, "_on_agent_style_changed");
    if (!p_agent->is_connected("gameplay_style_changed", style_cb)) {
        p_agent->connect("gameplay_style_changed", style_cb);
    }
}

void GameplayContextHUDIndicator::_disconnect_agent_signals() {
    if (!_possessed_agent_id.is_valid()) return;
    if (GameAgent *agent = godot::Object::cast_to<GameAgent>(godot::ObjectDB::get_instance(_possessed_agent_id))) {
        godot::Callable style_cb(this, "_on_agent_style_changed");
        if (agent->is_connected("gameplay_style_changed", style_cb)) {
            agent->disconnect("gameplay_style_changed", style_cb);
        }
    }
}

void GameplayContextHUDIndicator::_on_device_family_changed(int p_new_family, int p_prev_family) {
    _active_device = static_cast<ControlDeviceFamily>(p_new_family);
    _rebuild_action_prompts();
    godot::String label = _device_family_short_name(_active_device);
    emit_signal("hud_device_family_switched", p_new_family, label);
    queue_redraw();
}

void GameplayContextHUDIndicator::_on_action_triggered(const godot::StringName &p_action, float p_strength) {
    pulse_action_prompt(p_action, p_strength);
}

void GameplayContextHUDIndicator::_on_action_released(const godot::StringName &p_action) {
    for (HUDActionPromptPOD &p : _prompts) {
        if (p.semantic_action == p_action) {
            p.active_strength = 0.0f;
            queue_redraw();
            return;
        }
    }
}

void GameplayContextHUDIndicator::_on_vr_gesture_detected(const godot::StringName &p_gesture, int p_hand, float p_speed) {
    show_mode_transition_toast(
        "VR Gesture: " + godot::String(p_gesture),
        (p_hand == 0 ? godot::String("Left Hand • ") : godot::String("Right Hand • ")) + godot::String::num(p_speed, 1) + " m/s"
    );
}

void GameplayContextHUDIndicator::_on_agent_style_changed(const godot::Ref<GameplayStyle> &p_style) {
    set_active_style(p_style);
}

// --- Prompt Resolution Helpers ---

void GameplayContextHUDIndicator::_rebuild_action_prompts() {
    _prompts.clear();
    if (control_map.is_valid()) {
        _active_device = control_map->get_active_device_family();
        _active_context_mode = control_map->get_context_mode();
    }

    for (int i = 0; i < tracked_actions.size(); ++i) {
        godot::StringName act = tracked_actions[i];
        if (act.is_empty()) continue;

        HUDActionPromptPOD item;
        item.semantic_action = act;
        item.display_label = _resolve_action_display_label(act);
        item.binding_glyph = _resolve_binding_glyph(act, _active_device);
        item.active_strength = control_map.is_valid() ? control_map->get_action_strength(act) : 0.0f;
        item.pulse_timer = 0.0f;
        _prompts.push_back(item);
    }
}

godot::String GameplayContextHUDIndicator::_resolve_action_display_label(const godot::StringName &p_action) const {
    godot::String act_str(p_action);
    if (control_map.is_valid()) {
        godot::Dictionary agent_map = control_map->get_action_to_agent_action_map();
        if (agent_map.has(act_str)) {
            godot::Variant mapped = agent_map[act_str];
            if (mapped.get_type() == godot::Variant::STRING || mapped.get_type() == godot::Variant::STRING_NAME) {
                return godot::String(mapped);
            }
            if (mapped.get_type() == godot::Variant::INT) {
                if (GameAgent *agent = get_possessed_agent()) {
                    int idx = static_cast<int>(mapped);
                    godot::TypedArray<GameAgentAction> actions = agent->get_actions();
                    if (idx >= 0 && idx < actions.size()) {
                        if (GameAgentAction *gaa = godot::Object::cast_to<GameAgentAction>(actions[idx])) {
                            if (!gaa->get_title().is_empty()) {
                                return gaa->get_title();
                            }
                        }
                    }
                }
            }
        }
    }

    if (act_str == "primary_action") return "Primary";
    if (act_str == "secondary_action") return "Aim / Alt";
    if (act_str == "interact") return "Interact";
    if (act_str == "jump") return "Jump";
    if (act_str == "sprint") return "Sprint";
    if (act_str == "crouch") return "Crouch";
    return act_str.capitalize();
}

godot::String GameplayContextHUDIndicator::_resolve_binding_glyph(const godot::StringName &p_action, ControlDeviceFamily p_family) const {
    godot::String act_str(p_action);

    // 1. Check Custom USB / Arduino channel mapping
    if (p_family == ControlDeviceFamily::DEVICE_CUSTOM_USB_HID && control_map.is_valid()) {
        godot::Dictionary ch_map = control_map->get_custom_channel_to_action_map();
        godot::Array keys = ch_map.keys();
        for (int i = 0; i < keys.size(); ++i) {
            godot::String k = keys[i];
            if (godot::String(ch_map[k]) == act_str) {
                return "[USB:" + k + "]";
            }
        }
        return "[USB]";
    }

    // 2. Check VR gesture mapping
    if (p_family == ControlDeviceFamily::DEVICE_VR_XR && control_map.is_valid()) {
        godot::Dictionary vr_map = control_map->get_vr_gesture_to_action_map();
        godot::Array keys = vr_map.keys();
        for (int i = 0; i < keys.size(); ++i) {
            godot::String k = keys[i];
            if (godot::String(vr_map[k]) == act_str) {
                return "[VR:" + k + "]";
            }
        }
        if (act_str == "primary_action") return "[R-Trig]";
        if (act_str == "secondary_action") return "[L-Trig]";
        if (act_str == "interact") return "[R-Grip]";
        if (act_str == "jump") return "[VR-A]";
        return "[VR]";
    }

    // 3. Query Godot InputMap for the bound InputEvent matching the active device family
    godot::StringName target_input_action = p_action;
    if (control_map.is_valid()) {
        godot::Dictionary remap = control_map->get_action_remap_table();
        if (remap.has(act_str)) {
            godot::String remapped_str = remap[act_str];
            target_input_action = godot::StringName(remapped_str);
        }
    }

    if (godot::InputMap *imap = godot::InputMap::get_singleton()) {
        if (imap->has_action(target_input_action)) {
            godot::TypedArray<godot::InputEvent> events = imap->action_get_events(target_input_action);
            for (int i = 0; i < events.size(); ++i) {
                godot::Ref<godot::InputEvent> ev = events[i];
                if (!ev.is_valid()) continue;

                if (p_family == ControlDeviceFamily::DEVICE_GAMEPAD) {
                    if (godot::Object::cast_to<godot::InputEventJoypadButton>(ev.ptr()) ||
                        godot::Object::cast_to<godot::InputEventJoypadMotion>(ev.ptr())) {
                        return "(" + ev->as_text().substr(0, 6) + ")";
                    }
                } else {
                    if (godot::Object::cast_to<godot::InputEventKey>(ev.ptr()) ||
                        godot::Object::cast_to<godot::InputEventMouseButton>(ev.ptr())) {
                        return "[" + ev->as_text().substr(0, 7) + "]";
                    }
                }
            }
        }
    }

    // 4. Clean human-readable defaults by Device Family
    if (p_family == ControlDeviceFamily::DEVICE_GAMEPAD) {
        if (act_str == "primary_action") return "(RT)";
        if (act_str == "secondary_action") return "(LT)";
        if (act_str == "interact") return "(X)";
        if (act_str == "jump") return "(A)";
        if (act_str == "sprint") return "(L3)";
        if (act_str == "crouch") return "(B)";
        return "(Pad)";
    }

    if (p_family == ControlDeviceFamily::DEVICE_TOUCH_VIRTUAL) {
        return "[Tap]";
    }

    if (act_str == "primary_action") return "[LMB]";
    if (act_str == "secondary_action") return "[RMB]";
    if (act_str == "interact") return "[E]";
    if (act_str == "jump") return "[Space]";
    if (act_str == "sprint") return "[Shift]";
    if (act_str == "crouch") return "[Ctrl]";
    return "[Key]";
}

godot::String GameplayContextHUDIndicator::_device_family_short_name(ControlDeviceFamily p_family) {
    switch (p_family) {
        case ControlDeviceFamily::DEVICE_KEYBOARD_MOUSE: return "KB & Mouse";
        case ControlDeviceFamily::DEVICE_GAMEPAD: return "Gamepad";
        case ControlDeviceFamily::DEVICE_CUSTOM_USB_HID: return "Custom USB";
        case ControlDeviceFamily::DEVICE_VR_XR: return "VR / OpenXR";
        case ControlDeviceFamily::DEVICE_TOUCH_VIRTUAL: return "Touch";
        default: return "Auto Input";
    }
}

godot::String GameplayContextHUDIndicator::_context_mode_short_name(ControlContextMode p_mode) {
    switch (p_mode) {
        case ControlContextMode::CONTROL_CONTEXT_EXPLORATION: return "Exploration";
        case ControlContextMode::CONTROL_CONTEXT_COMBAT: return "Combat";
        case ControlContextMode::CONTROL_CONTEXT_VEHICLE_MOUNT: return "Vehicle";
        case ControlContextMode::CONTROL_CONTEXT_PUZZLE_INSPECT: return "Puzzle";
        case ControlContextMode::CONTROL_CONTEXT_UI_MENU: return "Menu";
        case ControlContextMode::CONTROL_CONTEXT_VR_IMMERSIVE: return "VR Immersive";
        default: return "Custom";
    }
}

// --- Custom Drawing ---

void GameplayContextHUDIndicator::_notification(int p_what) {
    if (p_what == NOTIFICATION_RESIZED || p_what == NOTIFICATION_THEME_CHANGED) {
        queue_redraw();
    } else if (p_what == NOTIFICATION_DRAW) {
        godot::Vector2 sz = get_size();
        if (sz.x <= 4.0f || sz.y <= 4.0f) {
            return;
        }

        if (enable_comfort_vignette && _current_vignette_weight > 0.01f) {
            _draw_comfort_vignette(sz);
        }

        if (show_transition_toast && _toast_timer > 0.0f) {
            _draw_transition_toast(sz);
        }

        if (show_device_badge || show_action_prompts) {
            _draw_device_and_prompt_bar(sz);
        }
    }
}

void GameplayContextHUDIndicator::_draw_comfort_vignette(const godot::Vector2 &p_size) {
    float alpha = godot::Math::clamp(_current_vignette_weight * vignette_max_intensity, 0.0f, 0.92f);
    if (alpha <= 0.01f) return;

    float border_w = p_size.x * (0.08f + 0.14f * _current_vignette_weight);
    float border_h = p_size.y * (0.08f + 0.14f * _current_vignette_weight);
    godot::Color vig_col(0.02f, 0.03f, 0.05f, alpha);

    draw_rect(godot::Rect2(0.0f, 0.0f, p_size.x, border_h), vig_col, true);
    draw_rect(godot::Rect2(0.0f, p_size.y - border_h, p_size.x, border_h), vig_col, true);
    draw_rect(godot::Rect2(0.0f, border_h, border_w, p_size.y - border_h * 2.0f), vig_col, true);
    draw_rect(godot::Rect2(p_size.x - border_w, border_h, border_w, p_size.y - border_h * 2.0f), vig_col, true);
}

void GameplayContextHUDIndicator::_draw_transition_toast(const godot::Vector2 &p_size) {
    godot::Ref<godot::Font> font = godot::ThemeDB::get_singleton()->get_fallback_font();
    if (!font.is_valid()) return;

    float fade = godot::Math::clamp(_toast_timer / 0.4f, 0.0f, 1.0f);
    float toast_w = godot::Math::min(p_size.x - 16.0f, 320.0f);
    float toast_h = _toast_subtext.is_empty() ? 28.0f : 44.0f;
    godot::Rect2 toast_rect(
        godot::Vector2((p_size.x - toast_w) * 0.5f, 4.0f),
        godot::Vector2(toast_w, toast_h)
    );

    draw_rect(toast_rect, godot::Color(0.08f, 0.11f, 0.16f, 0.90f * fade), true);
    draw_rect(toast_rect, godot::Color(0.24f, 0.78f, 0.96f, 0.85f * fade), false, 1.5f);

    draw_string(
        font,
        godot::Vector2(toast_rect.position.x + 10.0f, toast_rect.position.y + 18.0f),
        _toast_headline,
        godot::HORIZONTAL_ALIGNMENT_CENTER,
        toast_w - 20.0f,
        12,
        godot::Color(0.95f, 0.98f, 1.0f, fade)
    );

    if (!_toast_subtext.is_empty()) {
        draw_string(
            font,
            godot::Vector2(toast_rect.position.x + 10.0f, toast_rect.position.y + 34.0f),
            _toast_subtext,
            godot::HORIZONTAL_ALIGNMENT_CENTER,
            toast_w - 20.0f,
            10,
            godot::Color(0.68f, 0.82f, 0.94f, 0.90f * fade)
        );
    }
}

void GameplayContextHUDIndicator::_draw_device_icon(
    const godot::Vector2 &p_center,
    ControlDeviceFamily p_family,
    const godot::Color &p_color
) {
    switch (p_family) {
        case ControlDeviceFamily::DEVICE_GAMEPAD: {
            draw_rect(godot::Rect2(p_center - godot::Vector2(9.0f, 5.0f), godot::Vector2(18.0f, 10.0f)), p_color, false, 1.5f);
            draw_circle(p_center + godot::Vector2(-4.0f, 0.0f), 2.0f, p_color);
            draw_circle(p_center + godot::Vector2(4.0f, 0.0f), 2.0f, p_color);
            break;
        }
        case ControlDeviceFamily::DEVICE_CUSTOM_USB_HID: {
            draw_rect(godot::Rect2(p_center - godot::Vector2(6.0f, 6.0f), godot::Vector2(12.0f, 12.0f)), p_color, false, 1.5f);
            draw_line(p_center + godot::Vector2(-9.0f, -3.0f), p_center + godot::Vector2(-6.0f, -3.0f), p_color, 1.5f);
            draw_line(p_center + godot::Vector2(-9.0f, 3.0f), p_center + godot::Vector2(-6.0f, 3.0f), p_color, 1.5f);
            draw_line(p_center + godot::Vector2(6.0f, -3.0f), p_center + godot::Vector2(9.0f, -3.0f), p_color, 1.5f);
            draw_line(p_center + godot::Vector2(6.0f, 3.0f), p_center + godot::Vector2(9.0f, 3.0f), p_color, 1.5f);
            break;
        }
        case ControlDeviceFamily::DEVICE_VR_XR: {
            draw_rect(godot::Rect2(p_center - godot::Vector2(10.0f, 5.0f), godot::Vector2(20.0f, 10.0f)), p_color, false, 1.5f);
            draw_circle(p_center + godot::Vector2(-4.0f, 0.0f), 2.2f, p_color);
            draw_circle(p_center + godot::Vector2(4.0f, 0.0f), 2.2f, p_color);
            break;
        }
        default: {
            draw_rect(godot::Rect2(p_center - godot::Vector2(9.0f, 5.0f), godot::Vector2(11.0f, 10.0f)), p_color, false, 1.5f);
            draw_rect(godot::Rect2(p_center + godot::Vector2(4.0f, -5.0f), godot::Vector2(6.0f, 10.0f)), p_color, false, 1.5f);
            break;
        }
    }
}

void GameplayContextHUDIndicator::_draw_device_and_prompt_bar(const godot::Vector2 &p_size) {
    godot::Ref<godot::Font> font = godot::ThemeDB::get_singleton()->get_fallback_font();
    if (!font.is_valid()) return;

    float bar_h = 34.0f;
    float bar_y = p_size.y - bar_h - 2.0f;
    float cursor_x = 4.0f;

    // 1. Device Family & Context Pill
    if (show_device_badge) {
        godot::String dev_str = _device_family_short_name(_active_device);
        if (!_possessed_agent_title.is_empty()) {
            dev_str += " • " + _possessed_agent_title;
        }
        float badge_w = 135.0f;
        godot::Rect2 badge_rect(godot::Vector2(cursor_x, bar_y), godot::Vector2(badge_w, bar_h));
        draw_rect(badge_rect, godot::Color(0.09f, 0.12f, 0.17f, 0.90f), true);
        draw_rect(badge_rect, godot::Color(0.22f, 0.68f, 0.92f, 0.85f), false, 1.5f);

        _draw_device_icon(
            godot::Vector2(badge_rect.position.x + 16.0f, badge_rect.position.y + bar_h * 0.5f),
            _active_device,
            godot::Color(0.28f, 0.82f, 0.98f, 1.0f)
        );

        draw_string(
            font,
            godot::Vector2(badge_rect.position.x + 32.0f, badge_rect.position.y + 15.0f),
            dev_str,
            godot::HORIZONTAL_ALIGNMENT_LEFT,
            badge_w - 36.0f,
            11,
            godot::Color(0.94f, 0.97f, 1.0f, 1.0f)
        );
        draw_string(
            font,
            godot::Vector2(badge_rect.position.x + 32.0f, badge_rect.position.y + 28.0f),
            _context_mode_short_name(_active_context_mode),
            godot::HORIZONTAL_ALIGNMENT_LEFT,
            badge_w - 36.0f,
            9,
            godot::Color(0.65f, 0.78f, 0.90f, 0.88f)
        );

        cursor_x += badge_w + 8.0f;
    }

    // 2. Action Prompt Cards
    if (show_action_prompts && !_prompts.empty()) {
        float avail_w = godot::Math::max(60.0f, p_size.x - cursor_x - 4.0f);
        int count = static_cast<int>(_prompts.size());
        float card_w = godot::Math::clamp((avail_w - static_cast<float>(count - 1) * 6.0f) / static_cast<float>(count), 54.0f, 96.0f);

        for (int i = 0; i < count; ++i) {
            const HUDActionPromptPOD &p = _prompts[i];
            if (cursor_x + card_w > p_size.x) break;

            godot::Rect2 card_rect(godot::Vector2(cursor_x, bar_y), godot::Vector2(card_w, bar_h));
            float highlight = godot::Math::max(p.active_strength, p.pulse_timer);

            godot::Color bg_col = godot::Color(0.11f, 0.13f, 0.17f, 0.85f).lerp(
                godot::Color(0.18f, 0.52f, 0.76f, 0.95f),
                highlight * 0.75f
            );
            godot::Color border_col = godot::Color(0.28f, 0.33f, 0.40f, 0.85f).lerp(
                godot::Color(0.35f, 0.92f, 0.68f, 1.0f),
                highlight
            );

            draw_rect(card_rect, bg_col, true);
            if (p.active_strength > 0.01f) {
                draw_rect(
                    godot::Rect2(
                        card_rect.position,
                        godot::Vector2(card_rect.size.x * godot::Math::clamp(p.active_strength, 0.0f, 1.0f), 3.0f)
                    ),
                    godot::Color(0.35f, 0.92f, 0.68f, 1.0f),
                    true
                );
            }
            draw_rect(card_rect, border_col, false, 1.25f);

            draw_string(
                font,
                godot::Vector2(card_rect.position.x + 2.0f, card_rect.position.y + 14.0f),
                p.binding_glyph,
                godot::HORIZONTAL_ALIGNMENT_CENTER,
                card_w - 4.0f,
                11,
                godot::Color(0.98f, 0.86f, 0.28f, 1.0f)
            );
            draw_string(
                font,
                godot::Vector2(card_rect.position.x + 2.0f, card_rect.position.y + 28.0f),
                p.display_label,
                godot::HORIZONTAL_ALIGNMENT_CENTER,
                card_w - 4.0f,
                10,
                godot::Color(0.88f, 0.92f, 0.96f, 0.95f)
            );

            cursor_x += card_w + 6.0f;
        }
    }
}

} // namespace ideam::godot_ext