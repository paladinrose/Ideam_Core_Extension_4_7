#include "game_options_menu.h"

#include <cmath>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/foldable_container.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../game.h"
#include "../game_entity.h"
#include "../gameplay/gameplay_rule.h"
#include "game_menu.h"

namespace ideam::godot_ext {

void GameOptionsMenu::_bind_methods() {
    // --- Signals (preserving existing lifecycle signals + domain change notifications) ---
    ADD_SIGNAL(godot::MethodInfo("applied"));
    ADD_SIGNAL(godot::MethodInfo("canceled"));
    ADD_SIGNAL(godot::MethodInfo("opened"));
    ADD_SIGNAL(godot::MethodInfo("closed"));
    ADD_SIGNAL(godot::MethodInfo(
        "option_changed",
        godot::PropertyInfo(godot::Variant::STRING_NAME, "domain"),
        godot::PropertyInfo(godot::Variant::STRING_NAME, "property"),
        godot::PropertyInfo(godot::Variant::NIL, "value")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "mode_selected",
        godot::PropertyInfo(godot::Variant::INT, "mode_index"),
        godot::PropertyInfo(godot::Variant::OBJECT, "mode", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayMode")
    ));
    ADD_SIGNAL(godot::MethodInfo("defaults_restored"));
    ADD_SIGNAL(godot::MethodInfo("display_changes_reverted"));

    // --- Core Lifecycle & Action Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("validate_game"), &GameOptionsMenu::validate_game);
    godot::ClassDB::bind_method(godot::D_METHOD("build_options_menu"), &GameOptionsMenu::build_options_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("refresh_ui_from_staged"), &GameOptionsMenu::refresh_ui_from_staged);

    godot::ClassDB::bind_method(godot::D_METHOD("open_options"), &GameOptionsMenu::open_options);
    godot::ClassDB::bind_method(godot::D_METHOD("open_options_for_entity", "entity"), &GameOptionsMenu::open_options_for_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("close_options"), &GameOptionsMenu::close_options);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_options"), &GameOptionsMenu::apply_options);
    godot::ClassDB::bind_method(godot::D_METHOD("cancel_options"), &GameOptionsMenu::cancel_options);
    godot::ClassDB::bind_method(godot::D_METHOD("reset_to_defaults"), &GameOptionsMenu::reset_to_defaults);

    godot::ClassDB::bind_method(godot::D_METHOD("save_options_to_config", "path"), &GameOptionsMenu::save_options_to_config, DEFVAL(""));
    godot::ClassDB::bind_method(godot::D_METHOD("load_options_from_config", "path"), &GameOptionsMenu::load_options_from_config, DEFVAL(""));

    // --- Internal Signal Callbacks ---
    godot::ClassDB::bind_method(godot::D_METHOD("_on_mode_selected", "index"), &GameOptionsMenu::_on_mode_selected);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_enum_changed", "index", "domain", "prop", "option_btn"), &GameOptionsMenu::_on_enum_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_scalar_changed", "value", "domain", "prop"), &GameOptionsMenu::_on_scalar_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_bool_changed", "pressed", "domain", "prop"), &GameOptionsMenu::_on_bool_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_display_confirmed"), &GameOptionsMenu::_on_display_confirmed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_display_reverted"), &GameOptionsMenu::_on_display_reverted);

    // --- 1. Context & Target Scope Properties ---
    ADD_GROUP("Context & Target Scope", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game", "game"), &GameOptionsMenu::set_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game"), &GameOptionsMenu::get_game);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game"), "set_game", "get_game");

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_entity", "target_entity"), &GameOptionsMenu::set_target_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_entity"), &GameOptionsMenu::get_target_entity);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "target_entity", godot::PROPERTY_HINT_NODE_TYPE, "GameEntity"), "set_target_entity", "get_target_entity");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_menu", "game_menu"), &GameOptionsMenu::set_game_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_menu"), &GameOptionsMenu::get_game_menu);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_menu", godot::PROPERTY_HINT_NODE_TYPE, "GameMenu"), "set_game_menu", "get_game_menu");

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_style", "target_style"), &GameOptionsMenu::set_target_style);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_style"), &GameOptionsMenu::get_target_style);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "target_style", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayStyle"), "set_target_style", "get_target_style");

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_mode", "target_mode"), &GameOptionsMenu::set_target_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_mode"), &GameOptionsMenu::get_target_mode);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "target_mode", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayMode"), "set_target_mode", "get_target_mode");

    godot::ClassDB::bind_method(godot::D_METHOD("set_active_mode_index", "active_mode_index"), &GameOptionsMenu::set_active_mode_index);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_mode_index"), &GameOptionsMenu::get_active_mode_index);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "active_mode_index", godot::PROPERTY_HINT_RANGE, "0,32,1"), "set_active_mode_index", "get_active_mode_index");

    // --- Direct Domain Resource Overrides ---
    ADD_GROUP("Domain Resource Overrides", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_graphics_option", "graphics_option"), &GameOptionsMenu::set_graphics_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_graphics_option"), &GameOptionsMenu::get_graphics_option);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "graphics_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayGraphicsOption"), "set_graphics_option", "get_graphics_option");

    godot::ClassDB::bind_method(godot::D_METHOD("set_audio_option", "audio_option"), &GameOptionsMenu::set_audio_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_audio_option"), &GameOptionsMenu::get_audio_option);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "audio_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayAudioOption"), "set_audio_option", "get_audio_option");

    godot::ClassDB::bind_method(godot::D_METHOD("set_control_map", "control_map"), &GameOptionsMenu::set_control_map);
    godot::ClassDB::bind_method(godot::D_METHOD("get_control_map"), &GameOptionsMenu::get_control_map);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "control_map", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayControlMap"), "set_control_map", "get_control_map");

    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulty_option", "difficulty_option"), &GameOptionsMenu::set_difficulty_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulty_option"), &GameOptionsMenu::get_difficulty_option);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "difficulty_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayDifficulty"), "set_difficulty_option", "get_difficulty_option");

    godot::ClassDB::bind_method(godot::D_METHOD("set_perspective_option", "perspective_option"), &GameOptionsMenu::set_perspective_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspective_option"), &GameOptionsMenu::get_perspective_option);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "perspective_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayPerspective"), "set_perspective_option", "get_perspective_option");

    godot::ClassDB::bind_method(godot::D_METHOD("set_accessibility_option", "accessibility_option"), &GameOptionsMenu::set_accessibility_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_accessibility_option"), &GameOptionsMenu::get_accessibility_option);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "accessibility_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayAccessibilityOption"), "set_accessibility_option", "get_accessibility_option");

    // --- 2. Behavior & Persistence ---
    ADD_GROUP("Behavior & Persistence", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_build_fallback_ui", "auto_build"), &GameOptionsMenu::set_auto_build_fallback_ui);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_build_fallback_ui"), &GameOptionsMenu::get_auto_build_fallback_ui);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_build_fallback_ui"), "set_auto_build_fallback_ui", "get_auto_build_fallback_ui");

    godot::ClassDB::bind_method(godot::D_METHOD("set_live_preview_enabled", "enabled"), &GameOptionsMenu::set_live_preview_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_live_preview_enabled"), &GameOptionsMenu::get_live_preview_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "live_preview_enabled"), "set_live_preview_enabled", "get_live_preview_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_load_config_on_ready", "auto_load"), &GameOptionsMenu::set_auto_load_config_on_ready);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_load_config_on_ready"), &GameOptionsMenu::get_auto_load_config_on_ready);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_load_config_on_ready"), "set_auto_load_config_on_ready", "get_auto_load_config_on_ready");

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_save_config_on_apply", "auto_save"), &GameOptionsMenu::set_auto_save_config_on_apply);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_save_config_on_apply"), &GameOptionsMenu::get_auto_save_config_on_apply);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_save_config_on_apply"), "set_auto_save_config_on_apply", "get_auto_save_config_on_apply");

    godot::ClassDB::bind_method(godot::D_METHOD("set_config_file_path", "path"), &GameOptionsMenu::set_config_file_path);
    godot::ClassDB::bind_method(godot::D_METHOD("get_config_file_path"), &GameOptionsMenu::get_config_file_path);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING, "config_file_path"), "set_config_file_path", "get_config_file_path");

    godot::ClassDB::bind_method(godot::D_METHOD("set_display_confirm_timeout", "seconds"), &GameOptionsMenu::set_display_confirm_timeout);
    godot::ClassDB::bind_method(godot::D_METHOD("get_display_confirm_timeout"), &GameOptionsMenu::get_display_confirm_timeout);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "display_confirm_timeout", godot::PROPERTY_HINT_RANGE, "3.0,30.0,0.5"), "set_display_confirm_timeout", "get_display_confirm_timeout");

    // --- 3. Slot-Injected UI Controls ---
    ADD_GROUP("UI Node Slots", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_context_label", "context_label"), &GameOptionsMenu::set_context_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_context_label"), &GameOptionsMenu::get_context_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "context_label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_context_label", "get_context_label");

    godot::ClassDB::bind_method(godot::D_METHOD("set_mode_selector", "mode_selector"), &GameOptionsMenu::set_mode_selector);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mode_selector"), &GameOptionsMenu::get_mode_selector);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "mode_selector", godot::PROPERTY_HINT_NODE_TYPE, "OptionButton"), "set_mode_selector", "get_mode_selector");

    godot::ClassDB::bind_method(godot::D_METHOD("set_tabs", "tabs"), &GameOptionsMenu::set_tabs);
    godot::ClassDB::bind_method(godot::D_METHOD("get_tabs"), &GameOptionsMenu::get_tabs);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "tabs", godot::PROPERTY_HINT_NODE_TYPE, "TabContainer"), "set_tabs", "get_tabs");

    godot::ClassDB::bind_method(godot::D_METHOD("set_graphics_tab", "graphics_tab"), &GameOptionsMenu::set_graphics_tab);
    godot::ClassDB::bind_method(godot::D_METHOD("get_graphics_tab"), &GameOptionsMenu::get_graphics_tab);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "graphics_tab", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_graphics_tab", "get_graphics_tab");

    godot::ClassDB::bind_method(godot::D_METHOD("set_audio_tab", "audio_tab"), &GameOptionsMenu::set_audio_tab);
    godot::ClassDB::bind_method(godot::D_METHOD("get_audio_tab"), &GameOptionsMenu::get_audio_tab);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "audio_tab", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_audio_tab", "get_audio_tab");

    godot::ClassDB::bind_method(godot::D_METHOD("set_controls_tab", "controls_tab"), &GameOptionsMenu::set_controls_tab);
    godot::ClassDB::bind_method(godot::D_METHOD("get_controls_tab"), &GameOptionsMenu::get_controls_tab);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "controls_tab", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_controls_tab", "get_controls_tab");

    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulty_tab", "difficulty_tab"), &GameOptionsMenu::set_difficulty_tab);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulty_tab"), &GameOptionsMenu::get_difficulty_tab);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "difficulty_tab", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_difficulty_tab", "get_difficulty_tab");

    godot::ClassDB::bind_method(godot::D_METHOD("set_perspective_tab", "perspective_tab"), &GameOptionsMenu::set_perspective_tab);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspective_tab"), &GameOptionsMenu::get_perspective_tab);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "perspective_tab", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_perspective_tab", "get_perspective_tab");

    godot::ClassDB::bind_method(godot::D_METHOD("set_accessibility_tab", "accessibility_tab"), &GameOptionsMenu::set_accessibility_tab);
    godot::ClassDB::bind_method(godot::D_METHOD("get_accessibility_tab"), &GameOptionsMenu::get_accessibility_tab);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "accessibility_tab", godot::PROPERTY_HINT_NODE_TYPE, "Control"), "set_accessibility_tab", "get_accessibility_tab");

    godot::ClassDB::bind_method(godot::D_METHOD("set_reset_defaults_button", "reset_defaults_button"), &GameOptionsMenu::set_reset_defaults_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_reset_defaults_button"), &GameOptionsMenu::get_reset_defaults_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "reset_defaults_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_reset_defaults_button", "get_reset_defaults_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_apply_button", "apply_button"), &GameOptionsMenu::set_apply_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_apply_button"), &GameOptionsMenu::get_apply_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "apply_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_apply_button", "get_apply_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_cancel_button", "cancel_button"), &GameOptionsMenu::set_cancel_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_cancel_button"), &GameOptionsMenu::get_cancel_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "cancel_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_cancel_button", "get_cancel_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_display_confirm_dialog", "display_confirm_dialog"), &GameOptionsMenu::set_display_confirm_dialog);
    godot::ClassDB::bind_method(godot::D_METHOD("get_display_confirm_dialog"), &GameOptionsMenu::get_display_confirm_dialog);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "display_confirm_dialog", godot::PROPERTY_HINT_NODE_TYPE, "ConfirmationDialog"), "set_display_confirm_dialog", "get_display_confirm_dialog");
}

GameOptionsMenu::GameOptionsMenu() {
}

GameOptionsMenu::~GameOptionsMenu() {
}

void GameOptionsMenu::_ready() {
    validate_game();
    _resolve_context_resources();

    if (auto_load_config_on_ready && !godot::Engine::get_singleton()->is_editor_hint()) {
        load_options_from_config(config_file_path);
    }

    build_options_menu();
}

void GameOptionsMenu::_process(double p_delta) {
    if (!_awaiting_display_confirmation) {
        return;
    }

    _display_confirm_remaining -= static_cast<float>(p_delta);
    if (_display_confirm_remaining <= 0.0f) {
        _on_display_reverted();
        return;
    }

    if (display_confirm_dialog && display_confirm_dialog->is_visible()) {
        int secs = static_cast<int>( std::ceil(_display_confirm_remaining));
        display_confirm_dialog->set_text(
            "Keep these display settings?\nReverting automatically in " +
            godot::String::num_int64(secs) + " seconds..."
        );
    }
}

// --- 1. Context & Target Scope Setters / Getters ---

void GameOptionsMenu::set_game(Game *p_game) { game = p_game; }
Game *GameOptionsMenu::get_game() const { return game; }

void GameOptionsMenu::set_target_entity(GameEntity *p_entity) { target_entity = p_entity; }
GameEntity *GameOptionsMenu::get_target_entity() const { return target_entity; }

void GameOptionsMenu::set_game_menu(GameMenu *p_menu) { game_menu = p_menu; }
GameMenu *GameOptionsMenu::get_game_menu() const { return game_menu; }

void GameOptionsMenu::set_target_style(const godot::Ref<GameplayStyle> &p_style) { target_style = p_style; }
godot::Ref<GameplayStyle> GameOptionsMenu::get_target_style() const { return target_style; }

void GameOptionsMenu::set_target_mode(const godot::Ref<GameplayMode> &p_mode) { target_mode = p_mode; }
godot::Ref<GameplayMode> GameOptionsMenu::get_target_mode() const { return target_mode; }

void GameOptionsMenu::set_active_mode_index(int p_index) {
    active_mode_index = p_index < 0 ? 0 : p_index;
}
int GameOptionsMenu::get_active_mode_index() const { return active_mode_index; }

void GameOptionsMenu::set_graphics_option(const godot::Ref<GameplayGraphicsOption> &p_opt) { graphics_option = p_opt; }
godot::Ref<GameplayGraphicsOption> GameOptionsMenu::get_graphics_option() const { return graphics_option; }

void GameOptionsMenu::set_audio_option(const godot::Ref<GameplayAudioOption> &p_opt) { audio_option = p_opt; }
godot::Ref<GameplayAudioOption> GameOptionsMenu::get_audio_option() const { return audio_option; }

void GameOptionsMenu::set_control_map(const godot::Ref<GameplayControlMap> &p_map) { control_map = p_map; }
godot::Ref<GameplayControlMap> GameOptionsMenu::get_control_map() const { return control_map; }

void GameOptionsMenu::set_difficulty_option(const godot::Ref<GameplayDifficulty> &p_opt) { difficulty_option = p_opt; }
godot::Ref<GameplayDifficulty> GameOptionsMenu::get_difficulty_option() const { return difficulty_option; }

void GameOptionsMenu::set_perspective_option(const godot::Ref<GameplayPerspective> &p_opt) { perspective_option = p_opt; }
godot::Ref<GameplayPerspective> GameOptionsMenu::get_perspective_option() const { return perspective_option; }

void GameOptionsMenu::set_accessibility_option(const godot::Ref<GameplayAccessibilityOption> &p_opt) { accessibility_option = p_opt; }
godot::Ref<GameplayAccessibilityOption> GameOptionsMenu::get_accessibility_option() const { return accessibility_option; }

// --- 2. Behavior & Persistence Setters / Getters ---

void GameOptionsMenu::set_auto_build_fallback_ui(bool p_auto_build) { auto_build_fallback_ui = p_auto_build; }
bool GameOptionsMenu::get_auto_build_fallback_ui() const { return auto_build_fallback_ui; }

void GameOptionsMenu::set_live_preview_enabled(bool p_enabled) { live_preview_enabled = p_enabled; }
bool GameOptionsMenu::get_live_preview_enabled() const { return live_preview_enabled; }

void GameOptionsMenu::set_auto_load_config_on_ready(bool p_auto_load) { auto_load_config_on_ready = p_auto_load; }
bool GameOptionsMenu::get_auto_load_config_on_ready() const { return auto_load_config_on_ready; }

void GameOptionsMenu::set_auto_save_config_on_apply(bool p_auto_save) { auto_save_config_on_apply = p_auto_save; }
bool GameOptionsMenu::get_auto_save_config_on_apply() const { return auto_save_config_on_apply; }

void GameOptionsMenu::set_config_file_path(const godot::String &p_path) {
    if (!p_path.is_empty()) {
        config_file_path = p_path;
    }
}
godot::String GameOptionsMenu::get_config_file_path() const { return config_file_path; }

void GameOptionsMenu::set_display_confirm_timeout(float p_seconds) {
    display_confirm_timeout = godot::Math::clamp(p_seconds, 3.0f, 30.0f);
}
float GameOptionsMenu::get_display_confirm_timeout() const { return display_confirm_timeout; }

// --- 3. Slot-Injected Control Setters / Getters ---

void GameOptionsMenu::set_context_label(godot::Label *p_label) { context_label = p_label; }
godot::Label *GameOptionsMenu::get_context_label() const { return context_label; }

void GameOptionsMenu::set_mode_selector(godot::OptionButton *p_selector) { mode_selector = p_selector; }
godot::OptionButton *GameOptionsMenu::get_mode_selector() const { return mode_selector; }

void GameOptionsMenu::set_tabs(godot::TabContainer *p_tabs) { tabs = p_tabs; }
godot::TabContainer *GameOptionsMenu::get_tabs() const { return tabs; }

void GameOptionsMenu::set_graphics_tab(godot::Control *p_tab) { graphics_tab = p_tab; }
godot::Control *GameOptionsMenu::get_graphics_tab() const { return graphics_tab; }

void GameOptionsMenu::set_audio_tab(godot::Control *p_tab) { audio_tab = p_tab; }
godot::Control *GameOptionsMenu::get_audio_tab() const { return audio_tab; }

void GameOptionsMenu::set_controls_tab(godot::Control *p_tab) { controls_tab = p_tab; }
godot::Control *GameOptionsMenu::get_controls_tab() const { return controls_tab; }

void GameOptionsMenu::set_difficulty_tab(godot::Control *p_tab) { difficulty_tab = p_tab; }
godot::Control *GameOptionsMenu::get_difficulty_tab() const { return difficulty_tab; }

void GameOptionsMenu::set_perspective_tab(godot::Control *p_tab) { perspective_tab = p_tab; }
godot::Control *GameOptionsMenu::get_perspective_tab() const { return perspective_tab; }

void GameOptionsMenu::set_accessibility_tab(godot::Control *p_tab) { accessibility_tab = p_tab; }
godot::Control *GameOptionsMenu::get_accessibility_tab() const { return accessibility_tab; }

void GameOptionsMenu::set_reset_defaults_button(godot::Button *p_button) { reset_defaults_button = p_button; }
godot::Button *GameOptionsMenu::get_reset_defaults_button() const { return reset_defaults_button; }

void GameOptionsMenu::set_apply_button(godot::Button *p_button) { apply_button = p_button; }
godot::Button *GameOptionsMenu::get_apply_button() const { return apply_button; }

void GameOptionsMenu::set_cancel_button(godot::Button *p_button) { cancel_button = p_button; }
godot::Button *GameOptionsMenu::get_cancel_button() const { return cancel_button; }

void GameOptionsMenu::set_display_confirm_dialog(godot::ConfirmationDialog *p_dialog) { display_confirm_dialog = p_dialog; }
godot::ConfirmationDialog *GameOptionsMenu::get_display_confirm_dialog() const { return display_confirm_dialog; }

// --- 4. Context Resolution & Transactional Staging ---

void GameOptionsMenu::validate_game() {
    if (game != nullptr && target_entity != nullptr) {
        return;
    }

    godot::Node *cursor = get_parent();
    while (cursor != nullptr) {
        if (target_entity == nullptr) {
            if (GameEntity *ent = godot::Object::cast_to<GameEntity>(cursor)) {
                target_entity = ent;
            }
        }
        if (game == nullptr) {
            if (Game *found_game = godot::Object::cast_to<Game>(cursor)) {
                game = found_game;
                break;
            }
        }
        cursor = cursor->get_parent();
    }

    if (game == nullptr && game_menu != nullptr) {
        game = game_menu->get_game();
    }
}

void GameOptionsMenu::_resolve_context_resources() {
    validate_game();

    // 1. Resolve GameplayStyle from explicit override -> target_entity -> game
    if (!target_style.is_valid()) {
        if (target_entity != nullptr) {
            target_style = target_entity->get_gameplay_style();
            if (!target_style.is_valid()) {
                target_style = target_entity->get_resolved_gameplay_style();
            }
        }
        if (!target_style.is_valid() && game != nullptr) {
            target_style = game->get_gameplay_style();
            if (!target_style.is_valid()) {
                target_style = game->get_resolved_gameplay_style();
            }
        }
    }

    // 2. Resolve GameplayMode from target_style modes array
    if (target_style.is_valid()) {
        godot::TypedArray<GameplayMode> modes = target_style->get_modes();
        if (modes.size() > 0) {
            int clamped_idx = godot::Math::clamp(active_mode_index, 0, static_cast<int>(modes.size()) - 1);
            active_mode_index = clamped_idx;
            target_mode = godot::Ref<GameplayMode>(godot::Object::cast_to<GameplayMode>(modes[clamped_idx]));
        }
    }

    // 3. Extract active domain option resources from target_mode if not explicitly overridden
    if (target_mode.is_valid()) {
        if (!graphics_option.is_valid() && target_mode->get_graphics_options().size() > 0) {
            graphics_option = godot::Ref<GameplayGraphicsOption>(
                godot::Object::cast_to<GameplayGraphicsOption>(target_mode->get_graphics_options()[0])
            );
        }
        if (!audio_option.is_valid() && target_mode->get_audio_options().size() > 0) {
            audio_option = godot::Ref<GameplayAudioOption>(
                godot::Object::cast_to<GameplayAudioOption>(target_mode->get_audio_options()[0])
            );
        }
        if (!control_map.is_valid() && target_mode->get_control_maps().size() > 0) {
            control_map = godot::Ref<GameplayControlMap>(
                godot::Object::cast_to<GameplayControlMap>(target_mode->get_control_maps()[0])
            );
        }
        if (!difficulty_option.is_valid() && target_mode->get_difficulties().size() > 0) {
            difficulty_option = godot::Ref<GameplayDifficulty>(
                godot::Object::cast_to<GameplayDifficulty>(target_mode->get_difficulties()[0])
            );
        }
        if (!perspective_option.is_valid() && target_mode->get_perspectives().size() > 0) {
            perspective_option = godot::Ref<GameplayPerspective>(
                godot::Object::cast_to<GameplayPerspective>(target_mode->get_perspectives()[0])
            );
        }
        if (!accessibility_option.is_valid() && target_mode->get_accessibility_options().size() > 0) {
            accessibility_option = godot::Ref<GameplayAccessibilityOption>(
                godot::Object::cast_to<GameplayAccessibilityOption>(target_mode->get_accessibility_options()[0])
            );
        }
    }

    // 4. Ensure fallback instances exist so the Options Menu is always functional out of the box
    if (!graphics_option.is_valid()) {
        graphics_option.instantiate();
        graphics_option->set_title("DefaultGraphics");
    }
    if (!audio_option.is_valid()) {
        audio_option.instantiate();
        audio_option->set_title("DefaultAudio");
    }
    if (!control_map.is_valid()) {
        control_map.instantiate();
        control_map->set_title("DefaultControls");
    }
    if (!difficulty_option.is_valid()) {
        difficulty_option.instantiate();
        difficulty_option->set_title("DefaultDifficulty");
    }
    if (!perspective_option.is_valid()) {
        perspective_option.instantiate();
        perspective_option->set_title("DefaultPerspective");
    }
    if (!accessibility_option.is_valid()) {
        accessibility_option.instantiate();
        accessibility_option->set_title("DefaultAccessibility");
    }
}

void GameOptionsMenu::_create_staging_clones() {
    _resolve_context_resources();

    // Snapshot serialized dictionaries for clean rollback on Cancel or Display Timeout
    _backup_graphics_data = graphics_option->save_resource();
    _backup_audio_data = audio_option->save_resource();
    _backup_controls_data = control_map->save_resource();
    _backup_difficulty_data = difficulty_option->save_resource();
    _backup_perspective_data = perspective_option->save_resource();
    _backup_accessibility_data = accessibility_option->save_resource();

    // Instantiate isolated staging clones
    _staged_graphics.instantiate();
    _staged_graphics->apply_graphics_option(graphics_option);

    _staged_audio.instantiate();
    _staged_audio->apply_audio_option(audio_option);

    _staged_controls.instantiate();
    _staged_controls->apply_control_map(control_map);

    _staged_difficulty.instantiate();
    _staged_difficulty->apply_difficulty(difficulty_option);

    _staged_perspective.instantiate();
    _staged_perspective->apply_perspective(perspective_option);

    _staged_accessibility.instantiate();
    _staged_accessibility->apply_accessibility_option(accessibility_option);
}

void GameOptionsMenu::_restore_from_backups() {
    if (graphics_option.is_valid() && !_backup_graphics_data.is_empty()) {
        graphics_option->load_resource(_backup_graphics_data);
    }
    if (audio_option.is_valid() && !_backup_audio_data.is_empty()) {
        audio_option->load_resource(_backup_audio_data);
    }
    if (control_map.is_valid() && !_backup_controls_data.is_empty()) {
        control_map->load_resource(_backup_controls_data);
    }
    if (difficulty_option.is_valid() && !_backup_difficulty_data.is_empty()) {
        difficulty_option->load_resource(_backup_difficulty_data);
    }
    if (perspective_option.is_valid() && !_backup_perspective_data.is_empty()) {
        perspective_option->load_resource(_backup_perspective_data);
    }
    if (accessibility_option.is_valid() && !_backup_accessibility_data.is_empty()) {
        accessibility_option->load_resource(_backup_accessibility_data);
    }

    // Re-sync staged copies with restored authoritative options
    if (_staged_graphics.is_valid()) _staged_graphics->apply_graphics_option(graphics_option);
    if (_staged_audio.is_valid()) _staged_audio->apply_audio_option(audio_option);
    if (_staged_controls.is_valid()) _staged_controls->apply_control_map(control_map);
    if (_staged_difficulty.is_valid()) _staged_difficulty->apply_difficulty(difficulty_option);
    if (_staged_perspective.is_valid()) _staged_perspective->apply_perspective(perspective_option);
    if (_staged_accessibility.is_valid()) _staged_accessibility->apply_accessibility_option(accessibility_option);

    _apply_staged_to_runtime(true);
}

void GameOptionsMenu::_commit_staged_to_authoritative() {
    if (_staged_graphics.is_valid() && graphics_option.is_valid()) {
        graphics_option->apply_graphics_option(_staged_graphics);
    }
    if (_staged_audio.is_valid() && audio_option.is_valid()) {
        audio_option->apply_audio_option(_staged_audio);
    }
    if (_staged_controls.is_valid() && control_map.is_valid()) {
        control_map->apply_control_map(_staged_controls);
    }
    if (_staged_difficulty.is_valid() && difficulty_option.is_valid()) {
        difficulty_option->apply_difficulty(_staged_difficulty);
    }
    if (_staged_perspective.is_valid() && perspective_option.is_valid()) {
        perspective_option->apply_perspective(_staged_perspective);
    }
    if (_staged_accessibility.is_valid() && accessibility_option.is_valid()) {
        accessibility_option->apply_accessibility_option(_staged_accessibility);
    }

    _apply_staged_to_runtime(true);
}

godot::Environment *GameOptionsMenu::_find_active_environment() const {
    godot::Viewport *vp = get_viewport();
    if (vp != nullptr && vp->find_world_3d().is_valid()) {
        godot::Ref<godot::Environment> env = vp->find_world_3d()->get_environment();
        if (env.is_valid()) {
            return env.ptr();
        }
    }
    return nullptr;
}

godot::Node *GameOptionsMenu::_find_scene_root() const {
    if (target_entity != nullptr) {
        return target_entity;
    }
    if (game != nullptr) {
        return game->get_game_root() ? game->get_game_root() : game;
    }
    if (get_tree() != nullptr) {
        return get_tree()->get_current_scene();
    }
    return nullptr;
}

void GameOptionsMenu::_apply_staged_to_runtime(bool p_cascade_rules) {
    if (godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    godot::Window *win = get_window();
    godot::Environment *env = _find_active_environment();
    godot::Node *scene_root = _find_scene_root();

    if (_staged_audio.is_valid()) {
        _staged_audio->apply_to_audio_server();
    }
    if (_staged_graphics.is_valid()) {
        _staged_graphics->apply_to_scene(win, env, scene_root);
    }
    if (_staged_accessibility.is_valid()) {
        _staged_accessibility->apply_to_scene(win, env, scene_root);
    }

    if (!p_cascade_rules) {
        return;
    }

    // Synthesize GameplayRules from all active domain options and cascade through target_style
    if (target_style.is_valid()) {
        auto merge_domain_rules = [this](const godot::Ref<GameplayResource> &p_res) {
            if (!p_res.is_valid()) return;
            godot::TypedArray<GameplayRule> rules = p_res->collect_rules();
            for (int i = 0; i < rules.size(); ++i) {
                godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(rules[i]));
                if (rule.is_valid()) {
                    target_style->add_rule(rule);
                }
            }
        };

        merge_domain_rules(_staged_difficulty);
        merge_domain_rules(_staged_perspective);
        merge_domain_rules(_staged_controls);
        merge_domain_rules(_staged_audio);
        merge_domain_rules(_staged_graphics);
        merge_domain_rules(_staged_accessibility);
    }

    if (target_entity != nullptr) {
        target_entity->apply_gameplay_style(target_style);
    } else if (game != nullptr) {
        game->apply_gameplay_style(target_style);
    }
}

// --- 5. Modular UI Construction & Slot Validation ---

void GameOptionsMenu::_validate_layout_shell() {
    // Discover existing named nodes if designer placed them in the scene without assigning @export slots
    if (context_label == nullptr) {
        context_label = godot::Object::cast_to<godot::Label>(find_child("Context_Label", true, false));
    }
    if (mode_selector == nullptr) {
        mode_selector = godot::Object::cast_to<godot::OptionButton>(find_child("Mode_Selector", true, false));
    }
    if (tabs == nullptr) {
        tabs = godot::Object::cast_to<godot::TabContainer>(find_child("Options_Tabs", true, false));
    }
    if (reset_defaults_button == nullptr) {
        reset_defaults_button = godot::Object::cast_to<godot::Button>(find_child("Reset_Defaults_Button", true, false));
    }
    if (apply_button == nullptr) {
        apply_button = godot::Object::cast_to<godot::Button>(find_child("Apply_Button", true, false));
    }
    if (cancel_button == nullptr) {
        cancel_button = godot::Object::cast_to<godot::Button>(find_child("Cancel_Button", true, false));
    }
    if (display_confirm_dialog == nullptr) {
        display_confirm_dialog = godot::Object::cast_to<godot::ConfirmationDialog>(find_child("Display_Confirm_Dialog", true, false));
    }

    if (auto_build_fallback_ui && tabs == nullptr) {
        godot::VBoxContainer *main_vbox = memnew(godot::VBoxContainer);
        main_vbox->set_name("Options_Main_VBox");
        main_vbox->set_anchors_and_offsets_preset(godot::Control::PRESET_FULL_RECT);
        add_child(main_vbox);

        // Header Bar: Context Label + GameplayMode Selector
        godot::HBoxContainer *header_bar = memnew(godot::HBoxContainer);
        header_bar->set_name("Options_Header_Bar");
        main_vbox->add_child(header_bar);

        if (context_label == nullptr) {
            context_label = memnew(godot::Label);
            context_label->set_name("Context_Label");
            context_label->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
            context_label->set_text("Gameplay Options");
            header_bar->add_child(context_label);
        }

        if (mode_selector == nullptr) {
            godot::Label *mode_lbl = memnew(godot::Label);
            mode_lbl->set_text("Gameplay Mode:");
            header_bar->add_child(mode_lbl);

            mode_selector = memnew(godot::OptionButton);
            mode_selector->set_name("Mode_Selector");
            mode_selector->set_custom_minimum_size(godot::Vector2(180.0f, 0.0f));
            header_bar->add_child(mode_selector);
        }

        // TabContainer for the 6 GameplayResource domains
        tabs = memnew(godot::TabContainer);
        tabs->set_name("Options_Tabs");
        tabs->set_v_size_flags(godot::Control::SIZE_EXPAND_FILL);
        tabs->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        main_vbox->add_child(tabs);

        // Footer Bar: Reset Defaults, Cancel, Apply
        godot::HBoxContainer *footer_bar = memnew(godot::HBoxContainer);
        footer_bar->set_name("Options_Footer_Bar");
        footer_bar->set_alignment(godot::BoxContainer::ALIGNMENT_END);
        main_vbox->add_child(footer_bar);

        if (reset_defaults_button == nullptr) {
            reset_defaults_button = memnew(godot::Button);
            reset_defaults_button->set_name("Reset_Defaults_Button");
            reset_defaults_button->set_text("Reset to Defaults");
            footer_bar->add_child(reset_defaults_button);
        }

        if (cancel_button == nullptr) {
            cancel_button = memnew(godot::Button);
            cancel_button->set_name("Cancel_Button");
            cancel_button->set_text("Cancel");
            footer_bar->add_child(cancel_button);
        }

        if (apply_button == nullptr) {
            apply_button = memnew(godot::Button);
            apply_button->set_name("Apply_Button");
            apply_button->set_text("Apply");
            footer_bar->add_child(apply_button);
        }
    }

    if (display_confirm_dialog == nullptr) {
        display_confirm_dialog = memnew(godot::ConfirmationDialog);
        display_confirm_dialog->set_name("Display_Confirm_Dialog");
        display_confirm_dialog->set_title("Confirm Display Settings");
        display_confirm_dialog->set_ok_button_text("Keep Changes");
        display_confirm_dialog->set_cancel_button_text("Revert");
        add_child(display_confirm_dialog);
    }

    // Wire action signals idempotently
    if (mode_selector && !mode_selector->is_connected("item_selected", godot::Callable(this, "_on_mode_selected"))) {
        mode_selector->connect("item_selected", godot::Callable(this, "_on_mode_selected"));
    }
    if (reset_defaults_button && !reset_defaults_button->is_connected("pressed", godot::Callable(this, "reset_to_defaults"))) {
        reset_defaults_button->connect("pressed", godot::Callable(this, "reset_to_defaults"));
    }
    if (apply_button && !apply_button->is_connected("pressed", godot::Callable(this, "apply_options"))) {
        apply_button->connect("pressed", godot::Callable(this, "apply_options"));
    }
    if (cancel_button && !cancel_button->is_connected("pressed", godot::Callable(this, "cancel_options"))) {
        cancel_button->connect("pressed", godot::Callable(this, "cancel_options"));
    }
    if (display_confirm_dialog) {
        if (!display_confirm_dialog->is_connected("confirmed", godot::Callable(this, "_on_display_confirmed"))) {
            display_confirm_dialog->connect("confirmed", godot::Callable(this, "_on_display_confirmed"));
        }
        if (!display_confirm_dialog->is_connected("canceled", godot::Callable(this, "_on_display_reverted"))) {
            display_confirm_dialog->connect("canceled", godot::Callable(this, "_on_display_reverted"));
        }
    }
}

godot::VBoxContainer *GameOptionsMenu::_ensure_tab_scroll_vbox(godot::Control *&r_tab_slot, const godot::String &p_tab_name) {
    if (r_tab_slot == nullptr && tabs != nullptr) {
        godot::Node *existing = tabs->get_node_or_null(p_tab_name);
        if (existing != nullptr) {
            r_tab_slot = godot::Object::cast_to<godot::Control>(existing);
        } else if (auto_build_fallback_ui) {
            godot::ScrollContainer *scroll = memnew(godot::ScrollContainer);
            scroll->set_name(p_tab_name);
            scroll->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
            scroll->set_v_size_flags(godot::Control::SIZE_EXPAND_FILL);
            tabs->add_child(scroll);
            r_tab_slot = scroll;
        }
    }

    if (r_tab_slot == nullptr) {
        return nullptr;
    }

    if (godot::VBoxContainer *direct_vbox = godot::Object::cast_to<godot::VBoxContainer>(r_tab_slot)) {
        return direct_vbox;
    }

    godot::VBoxContainer *inner_vbox = godot::Object::cast_to<godot::VBoxContainer>(
        r_tab_slot->find_child("Content_VBox", true, false)
    );
    if (inner_vbox == nullptr && auto_build_fallback_ui) {
        inner_vbox = memnew(godot::VBoxContainer);
        inner_vbox->set_name("Content_VBox");
        inner_vbox->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
        r_tab_slot->add_child(inner_vbox);
    }
    return inner_vbox;
}

void GameOptionsMenu::build_options_menu() {
    _validate_layout_shell();
    _create_staging_clones();
    _populate_mode_selector();
    _update_context_header();

    _graphics_vbox = _ensure_tab_scroll_vbox(graphics_tab, "Graphics");
    _audio_vbox = _ensure_tab_scroll_vbox(audio_tab, "Audio");
    _controls_vbox = _ensure_tab_scroll_vbox(controls_tab, "Controls");
    _difficulty_vbox = _ensure_tab_scroll_vbox(difficulty_tab, "Difficulty");
    _perspective_vbox = _ensure_tab_scroll_vbox(perspective_tab, "Perspective");
    _accessibility_vbox = _ensure_tab_scroll_vbox(accessibility_tab, "Accessibility");

    if (auto_build_fallback_ui) {
        _rebuild_all_domain_tabs();
    }
}

void GameOptionsMenu::_populate_mode_selector() {
    if (mode_selector == nullptr) {
        return;
    }

    _updating_ui = true;
    mode_selector->clear();

    if (target_style.is_valid() && target_style->get_modes().size() > 0) {
        godot::TypedArray<GameplayMode> modes = target_style->get_modes();
        for (int i = 0; i < modes.size(); ++i) {
            GameplayMode *m = godot::Object::cast_to<GameplayMode>(modes[i]);
            godot::String label = (m && !m->get_title().is_empty())
                ? m->get_title()
                : ("Mode " + godot::String::num_int64(i + 1));
            mode_selector->add_item(label, i);
        }
        int sel = godot::Math::clamp(active_mode_index, 0, static_cast<int>(modes.size()) - 1);
        mode_selector->select(sel);
        mode_selector->set_disabled(false);
    } else {
        godot::String fallback_title = (target_mode.is_valid() && !target_mode->get_title().is_empty())
            ? target_mode->get_title()
            : "Default Mode";
        mode_selector->add_item(fallback_title, 0);
        mode_selector->select(0);
        mode_selector->set_disabled(true);
    }
    _updating_ui = false;
}

void GameOptionsMenu::_update_context_header() {
    if (context_label == nullptr) {
        return;
    }

    godot::String scope_str = "Global Game Scope";
    if (target_entity != nullptr) {
        godot::String ent_title = target_entity->get_title();
        if (ent_title.is_empty()) {
            ent_title = target_entity->get_name();
        }
        scope_str = target_entity->get_class() + godot::String(": ") + ent_title;
    } else if (game != nullptr && !game->get_title().is_empty()) {
        scope_str = game->get_title();
    }

    context_label->set_text("Options — " + scope_str);
}

void GameOptionsMenu::_rebuild_all_domain_tabs() {
    _updating_ui = true;

    auto clear_vbox = [](godot::VBoxContainer *p_vbox) {
        if (!p_vbox) return;
        for (int i = p_vbox->get_child_count() - 1; i >= 0; --i) {
            godot::Node *child = p_vbox->get_child(i);
            p_vbox->remove_child(child);
            child->queue_free();
        }
    };

    if (_graphics_vbox) {
        clear_vbox(_graphics_vbox);
        _build_graphics_tab_content(_graphics_vbox);
    }
    if (_audio_vbox) {
        clear_vbox(_audio_vbox);
        _build_audio_tab_content(_audio_vbox);
    }
    if (_controls_vbox) {
        clear_vbox(_controls_vbox);
        _build_controls_tab_content(_controls_vbox);
    }
    if (_difficulty_vbox) {
        clear_vbox(_difficulty_vbox);
        _build_difficulty_tab_content(_difficulty_vbox);
    }
    if (_perspective_vbox) {
        clear_vbox(_perspective_vbox);
        _build_perspective_tab_content(_perspective_vbox);
    }
    if (_accessibility_vbox) {
        clear_vbox(_accessibility_vbox);
        _build_accessibility_tab_content(_accessibility_vbox);
    }

    _updating_ui = false;
}

void GameOptionsMenu::refresh_ui_from_staged() {
    _populate_mode_selector();
    _update_context_header();
    if (auto_build_fallback_ui) {
        _rebuild_all_domain_tabs();
    }
}

// --- 6. Row Factory Helpers ---

void GameOptionsMenu::_add_section_header(godot::VBoxContainer *p_parent, const godot::String &p_title) {
    if (!p_parent) return;

    if (p_parent->get_child_count() > 0) {
        godot::HSeparator *sep = memnew(godot::HSeparator);
        p_parent->add_child(sep);
    }

    godot::Label *header = memnew(godot::Label);
    header->set_text(p_title);
    p_parent->add_child(header);
}

godot::OptionButton *GameOptionsMenu::_add_enum_row(
    godot::VBoxContainer *p_parent,
    const godot::String &p_label,
    const godot::PackedStringArray &p_items,
    const godot::PackedInt32Array &p_ids,
    int p_selected_id,
    const godot::StringName &p_domain,
    const godot::StringName &p_prop
) {
    godot::HBoxContainer *row = memnew(godot::HBoxContainer);
    p_parent->add_child(row);

    godot::Label *lbl = memnew(godot::Label);
    lbl->set_text(p_label);
    lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    row->add_child(lbl);

    godot::OptionButton *opt = memnew(godot::OptionButton);
    opt->set_custom_minimum_size(godot::Vector2(210.0f, 0.0f));
    int selected_idx = 0;
    for (int i = 0; i < p_items.size(); ++i) {
        int id = (i < p_ids.size()) ? p_ids[i] : i;
        opt->add_item(p_items[i], id);
        if (id == p_selected_id) {
            selected_idx = i;
        }
    }
    opt->select(selected_idx);
    opt->connect("item_selected", godot::Callable(this, "_on_enum_changed").bind(p_domain, p_prop, opt));
    row->add_child(opt);
    return opt;
}

godot::SpinBox *GameOptionsMenu::_add_scalar_row(
    godot::VBoxContainer *p_parent,
    const godot::String &p_label,
    double p_value,
    double p_min,
    double p_max,
    double p_step,
    const godot::StringName &p_domain,
    const godot::StringName &p_prop,
    const godot::String &p_suffix
) {
    godot::HBoxContainer *row = memnew(godot::HBoxContainer);
    p_parent->add_child(row);

    godot::Label *lbl = memnew(godot::Label);
    lbl->set_text(p_label);
    lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    row->add_child(lbl);

    godot::SpinBox *spin = memnew(godot::SpinBox);
    spin->set_custom_minimum_size(godot::Vector2(160.0f, 0.0f));
    spin->set_min(p_min);
    spin->set_max(p_max);
    spin->set_step(p_step);
    spin->set_value_no_signal(p_value);
    if (!p_suffix.is_empty()) {
        spin->set_suffix(p_suffix);
    }
    spin->connect("value_changed", godot::Callable(this, "_on_scalar_changed").bind(p_domain, p_prop));
    row->add_child(spin);
    return spin;
}

godot::CheckBox *GameOptionsMenu::_add_bool_row(
    godot::VBoxContainer *p_parent,
    const godot::String &p_label,
    bool p_value,
    const godot::StringName &p_domain,
    const godot::StringName &p_prop
) {
    godot::HBoxContainer *row = memnew(godot::HBoxContainer);
    p_parent->add_child(row);

    godot::Label *lbl = memnew(godot::Label);
    lbl->set_text(p_label);
    lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    row->add_child(lbl);

    godot::CheckBox *chk = memnew(godot::CheckBox);
    chk->set_pressed_no_signal(p_value);
    chk->connect("toggled", godot::Callable(this, "_on_bool_changed").bind(p_domain, p_prop));
    row->add_child(chk);
    return chk;
}

// --- 7. Per-Domain Tab Builders ---

void GameOptionsMenu::_build_graphics_tab_content(godot::VBoxContainer *p_vbox) {
    if (!p_vbox || !_staged_graphics.is_valid()) return;
    const godot::StringName dom("graphics");

    _add_section_header(p_vbox, "Quality & Display");
    {
        godot::PackedStringArray items;
        items.push_back("Custom");
        items.push_back("Low");
        items.push_back("Medium");
        items.push_back("High");
        items.push_back("Ultra");
        godot::PackedInt32Array ids = {0, 1, 2, 3, 4};
        _add_enum_row(p_vbox, "Overall Quality Preset", items, ids, static_cast<int>(_staged_graphics->get_quality_preset()), dom, godot::StringName("quality_preset"));
    }
    {
        godot::PackedStringArray items;
        items.push_back("Windowed");
        items.push_back("Fullscreen");
        items.push_back("Exclusive Fullscreen");
        godot::PackedInt32Array ids = {
            static_cast<int>(godot::Window::MODE_WINDOWED),
            static_cast<int>(godot::Window::MODE_FULLSCREEN),
            static_cast<int>(godot::Window::MODE_EXCLUSIVE_FULLSCREEN)
        };
        _add_enum_row(p_vbox, "Display Mode", items, ids, static_cast<int>(_staged_graphics->get_display_mode()), dom, godot::StringName("display_mode"));
    }
    {
        godot::PackedStringArray items;
        items.push_back("Disabled");
        items.push_back("Enabled");
        items.push_back("Adaptive");
        items.push_back("Mailbox");
        godot::PackedInt32Array ids = {
            static_cast<int>(godot::DisplayServer::VSYNC_DISABLED),
            static_cast<int>(godot::DisplayServer::VSYNC_ENABLED),
            static_cast<int>(godot::DisplayServer::VSYNC_ADAPTIVE),
            static_cast<int>(godot::DisplayServer::VSYNC_MAILBOX)
        };
        _add_enum_row(p_vbox, "V-Sync Mode", items, ids, static_cast<int>(_staged_graphics->get_vsync_mode()), dom, godot::StringName("vsync_mode"));
    }
    _add_scalar_row(p_vbox, "Max Framerate (0 = Unlimited)", _staged_graphics->get_max_fps(), 0.0, 360.0, 1.0, dom, godot::StringName("max_fps"), " FPS");
    _add_scalar_row(p_vbox, "3D Resolution Scale", _staged_graphics->get_resolution_scale(), 0.25, 2.0, 0.05, dom, godot::StringName("resolution_scale"), "x");
    _add_scalar_row(p_vbox, "Upscaler Sharpness (FSR2)", _staged_graphics->get_fsr_sharpness(), 0.0, 2.0, 0.05, dom, godot::StringName("fsr_sharpness"));

    _add_section_header(p_vbox, "Anti-Aliasing & Lighting");
    _add_bool_row(p_vbox, "Temporal Anti-Aliasing (TAA)", _staged_graphics->get_taa_enabled(), dom, godot::StringName("taa_enabled"));
    {
        godot::PackedStringArray items;
        items.push_back("Disabled");
        items.push_back("2x MSAA");
        items.push_back("4x MSAA");
        items.push_back("8x MSAA");
        godot::PackedInt32Array ids = {0, 1, 2, 3};
        _add_enum_row(p_vbox, "3D MSAA", items, ids, static_cast<int>(_staged_graphics->get_msaa_3d()), dom, godot::StringName("msaa_3d"));
    }
    _add_bool_row(p_vbox, "Shadow Mapping Enabled", _staged_graphics->get_shadow_mapping_enabled(), dom, godot::StringName("shadow_mapping_enabled"));
    {
        godot::PackedStringArray items;
        items.push_back("SDFGI");
        items.push_back("VoxelGI");
        items.push_back("LightmapGI");
        godot::PackedInt32Array ids = {0, 1, 2};
        _add_enum_row(p_vbox, "Global Illumination Type", items, ids, static_cast<int>(_staged_graphics->get_gi_type()), dom, godot::StringName("gi_type"));
    }
    {
        godot::PackedStringArray items;
        items.push_back("Disabled");
        items.push_back("Low");
        items.push_back("High");
        godot::PackedInt32Array ids = {0, 1, 2};
        _add_enum_row(p_vbox, "Global Illumination Quality", items, ids, static_cast<int>(_staged_graphics->get_gi_quality()), dom, godot::StringName("gi_quality"));
    }

    _add_section_header(p_vbox, "Post-Processing & LOD Budgets");
    {
        godot::PackedStringArray items;
        items.push_back("Disabled");
        items.push_back("Very Low");
        items.push_back("Low");
        items.push_back("Medium");
        items.push_back("High");
        items.push_back("Ultra");
        godot::PackedInt32Array ids = {-1, 0, 1, 2, 3, 4};
        _add_enum_row(p_vbox, "Ambient Occlusion (SSAO)", items, ids, static_cast<int>(_staged_graphics->get_ssao_quality()), dom, godot::StringName("ssao_quality"));
        _add_enum_row(p_vbox, "Indirect Lighting (SSIL)", items, ids, static_cast<int>(_staged_graphics->get_ssil_quality()), dom, godot::StringName("ssil_quality"));
        _add_enum_row(p_vbox, "Screen-Space Reflections (SSR)", items, ids, static_cast<int>(_staged_graphics->get_ssr_quality()), dom, godot::StringName("ssr_quality"));
    }
    _add_bool_row(p_vbox, "Glow / Bloom Enabled", _staged_graphics->get_bloom_enabled(), dom, godot::StringName("bloom_enabled"));
    _add_bool_row(p_vbox, "Volumetric Fog Enabled", _staged_graphics->get_volumetric_fog_enabled(), dom, godot::StringName("volumetric_fog_enabled"));
    _add_scalar_row(p_vbox, "Mesh LOD Threshold", _staged_graphics->get_mesh_lod_threshold(), 0.1, 8.0, 0.1, dom, godot::StringName("mesh_lod_threshold"));
    _add_scalar_row(p_vbox, "Debris Budget Multiplier", _staged_graphics->get_debris_budget_multiplier(), 0.0, 4.0, 0.1, dom, godot::StringName("debris_budget_multiplier"), "x");
}

void GameOptionsMenu::_build_audio_tab_content(godot::VBoxContainer *p_vbox) {
    if (!p_vbox || !_staged_audio.is_valid()) return;
    const godot::StringName dom("audio");

    _add_section_header(p_vbox, "Bus Volumes & Mute States");
    _add_scalar_row(p_vbox, "Master Volume", _staged_audio->get_master_volume(), 0.0, 1.0, 0.01, dom, godot::StringName("master_volume"));
    _add_bool_row(p_vbox, "Mute Master", _staged_audio->get_master_muted(), dom, godot::StringName("master_muted"));

    _add_scalar_row(p_vbox, "Music Volume", _staged_audio->get_music_volume(), 0.0, 1.0, 0.01, dom, godot::StringName("music_volume"));
    _add_bool_row(p_vbox, "Mute Music", _staged_audio->get_music_muted(), dom, godot::StringName("music_muted"));

    _add_scalar_row(p_vbox, "SFX Volume", _staged_audio->get_sfx_volume(), 0.0, 1.0, 0.01, dom, godot::StringName("sfx_volume"));
    _add_bool_row(p_vbox, "Mute SFX", _staged_audio->get_sfx_muted(), dom, godot::StringName("sfx_muted"));

    _add_scalar_row(p_vbox, "Voice / Dialogue Volume", _staged_audio->get_voice_volume(), 0.0, 1.0, 0.01, dom, godot::StringName("voice_volume"));
    _add_bool_row(p_vbox, "Mute Voice", _staged_audio->get_voice_muted(), dom, godot::StringName("voice_muted"));

    _add_scalar_row(p_vbox, "Ambient Volume", _staged_audio->get_ambient_volume(), 0.0, 1.0, 0.01, dom, godot::StringName("ambient_volume"));
    _add_bool_row(p_vbox, "Mute Ambient", _staged_audio->get_ambient_muted(), dom, godot::StringName("ambient_muted"));

    _add_scalar_row(p_vbox, "UI Volume", _staged_audio->get_ui_volume(), 0.0, 1.0, 0.01, dom, godot::StringName("ui_volume"));
    _add_bool_row(p_vbox, "Mute UI", _staged_audio->get_ui_muted(), dom, godot::StringName("ui_muted"));

    _add_section_header(p_vbox, "Output, Dynamic Range & Spatial Cymatics");
    {
        godot::PackedStringArray items;
        items.push_back("Stereo");
        items.push_back("Headphones / Binaural");
        items.push_back("Surround 5.1");
        items.push_back("Surround 7.1");
        items.push_back("Mono");
        godot::PackedInt32Array ids = {0, 1, 2, 3, 4};
        _add_enum_row(p_vbox, "Speaker Mode", items, ids, static_cast<int>(_staged_audio->get_speaker_mode()), dom, godot::StringName("speaker_mode"));
    }
    {
        godot::PackedStringArray items;
        items.push_back("Full (Studio)");
        items.push_back("Standard");
        items.push_back("Medium Compressed");
        items.push_back("Night Mode (Quiet Boost)");
        godot::PackedInt32Array ids = {0, 1, 2, 3};
        _add_enum_row(p_vbox, "Dynamic Range Preset", items, ids, static_cast<int>(_staged_audio->get_dynamic_range_preset()), dom, godot::StringName("dynamic_range_preset"));
    }
    _add_bool_row(p_vbox, "Mute Audio When Window Unfocused", _staged_audio->get_mute_when_unfocused(), dom, godot::StringName("mute_when_unfocused"));
    _add_bool_row(p_vbox, "Dialogue Voice Ducking", _staged_audio->get_voice_ducking_enabled(), dom, godot::StringName("voice_ducking_enabled"));
    {
        godot::PackedStringArray items;
        items.push_back("Disabled");
        items.push_back("Basic Raycast");
        items.push_back("2D Cymatic Wave");
        items.push_back("3D Cymatic Wave");
        godot::PackedInt32Array ids = {0, 1, 2, 3};
        _add_enum_row(p_vbox, "Acoustic Propagation Quality", items, ids, static_cast<int>(_staged_audio->get_acoustic_quality()), dom, godot::StringName("acoustic_quality"));
    }
    _add_bool_row(p_vbox, "Acoustic Occlusion Enabled", _staged_audio->get_acoustic_occlusion_enabled(), dom, godot::StringName("acoustic_occlusion_enabled"));
    _add_bool_row(p_vbox, "Dynamic Reverb Enabled", _staged_audio->get_dynamic_reverb_enabled(), dom, godot::StringName("dynamic_reverb_enabled"));
    _add_scalar_row(p_vbox, "AI Hearing Sensitivity Multiplier", _staged_audio->get_ai_hearing_sensitivity_multiplier(), 0.1, 4.0, 0.05, dom, godot::StringName("ai_hearing_sensitivity_multiplier"), "x");
}

void GameOptionsMenu::_build_controls_tab_content(godot::VBoxContainer *p_vbox) {
    if (!p_vbox || !_staged_controls.is_valid()) return;
    const godot::StringName dom("controls");

    _add_section_header(p_vbox, "Device Routing & Context Mode");
    {
        godot::PackedStringArray items;
        items.push_back("Exploration");
        items.push_back("Combat");
        items.push_back("Vehicle / Mount");
        items.push_back("Puzzle / Inspect");
        items.push_back("UI / Menu");
        items.push_back("VR Immersive");
        items.push_back("Custom");
        godot::PackedInt32Array ids = {0, 1, 2, 3, 4, 5, 6};
        _add_enum_row(p_vbox, "Control Context Preset", items, ids, static_cast<int>(_staged_controls->get_context_mode()), dom, godot::StringName("context_mode"));
    }
    {
        godot::PackedStringArray items;
        items.push_back("Auto-Detect");
        items.push_back("Keyboard & Mouse");
        items.push_back("Console Gamepad");
        items.push_back("Custom USB / Arduino HID");
        items.push_back("VR / OpenXR");
        items.push_back("Touch / Virtual");
        godot::PackedInt32Array ids = {0, 1, 2, 3, 4, 5};
        _add_enum_row(p_vbox, "Preferred Device Family", items, ids, static_cast<int>(_staged_controls->get_preferred_device_family()), dom, godot::StringName("preferred_device_family"));
    }
    _add_scalar_row(p_vbox, "Input Buffer Window", _staged_controls->get_input_buffer_window(), 0.0, 0.5, 0.01, dom, godot::StringName("input_buffer_window"), " s");

    _add_section_header(p_vbox, "Mouse & Keyboard");
    godot::Vector2 msens = _staged_controls->get_mouse_sensitivity();
    _add_scalar_row(p_vbox, "Mouse Sensitivity X", msens.x, 0.01, 5.0, 0.01, dom, godot::StringName("mouse_sensitivity_x"));
    _add_scalar_row(p_vbox, "Mouse Sensitivity Y", msens.y, 0.01, 5.0, 0.01, dom, godot::StringName("mouse_sensitivity_y"));
    _add_bool_row(p_vbox, "Raw Mouse Input", _staged_controls->get_mouse_raw_input(), dom, godot::StringName("mouse_raw_input"));
    _add_bool_row(p_vbox, "Invert Mouse X", _staged_controls->get_invert_mouse_x(), dom, godot::StringName("invert_mouse_x"));
    _add_bool_row(p_vbox, "Invert Mouse Y", _staged_controls->get_invert_mouse_y(), dom, godot::StringName("invert_mouse_y"));

    _add_section_header(p_vbox, "Controller / Gamepad Deadzones & Haptics");
    {
        godot::PackedStringArray items;
        items.push_back("Radial Scaled");
        items.push_back("Axial Independent");
        items.push_back("Cubic Precision");
        godot::PackedInt32Array ids = {0, 1, 2};
        _add_enum_row(p_vbox, "Deadzone Response Curve", items, ids, static_cast<int>(_staged_controls->get_deadzone_curve_shape()), dom, godot::StringName("deadzone_curve_shape"));
    }
    _add_scalar_row(p_vbox, "Left Stick Inner Deadzone", _staged_controls->get_left_stick_deadzone_inner(), 0.0, 0.85, 0.01, dom, godot::StringName("left_stick_deadzone_inner"));
    _add_scalar_row(p_vbox, "Right Stick Inner Deadzone", _staged_controls->get_right_stick_deadzone_inner(), 0.0, 0.85, 0.01, dom, godot::StringName("right_stick_deadzone_inner"));
    _add_scalar_row(p_vbox, "Trigger Deadzone", _staged_controls->get_trigger_deadzone(), 0.0, 0.85, 0.01, dom, godot::StringName("trigger_deadzone"));
    _add_bool_row(p_vbox, "Swap Left / Right Thumbsticks", _staged_controls->get_swap_thumbsticks(), dom, godot::StringName("swap_thumbsticks"));
    _add_bool_row(p_vbox, "Controller Vibration Enabled", _staged_controls->get_vibration_enabled(), dom, godot::StringName("vibration_enabled"));
    _add_scalar_row(p_vbox, "Vibration Strength", _staged_controls->get_vibration_strength(), 0.0, 1.0, 0.05, dom, godot::StringName("vibration_strength"));

    _add_section_header(p_vbox, "Custom USB / Arduino & VR Locomotion");
    _add_bool_row(p_vbox, "Enable Custom USB / Serial HID", _staged_controls->get_custom_usb_enabled(), dom, godot::StringName("custom_usb_enabled"));
    _add_bool_row(p_vbox, "Enable VR Tracked Input", _staged_controls->get_vr_input_enabled(), dom, godot::StringName("vr_input_enabled"));
    {
        godot::PackedStringArray items;
        items.push_back("Continuous (HMD-Oriented)");
        items.push_back("Continuous (Hand-Oriented)");
        items.push_back("Teleport / Blink");
        items.push_back("Physical Arm Swing");
        godot::PackedInt32Array ids = {0, 1, 2, 3};
        _add_enum_row(p_vbox, "VR Locomotion Mode", items, ids, static_cast<int>(_staged_controls->get_vr_locomotion_mode()), dom, godot::StringName("vr_locomotion_mode"));
    }
    {
        godot::PackedStringArray items;
        items.push_back("Snap Turn");
        items.push_back("Smooth Turn");
        items.push_back("Physical Turn Only");
        godot::PackedInt32Array ids = {0, 1, 2};
        _add_enum_row(p_vbox, "VR Turn Mode", items, ids, static_cast<int>(_staged_controls->get_vr_turn_mode()), dom, godot::StringName("vr_turn_mode"));
    }
    _add_scalar_row(p_vbox, "VR Snap Turn Angle", _staged_controls->get_vr_snap_turn_degrees(), 15.0, 90.0, 5.0, dom, godot::StringName("vr_snap_turn_degrees"), " deg");
    _add_bool_row(p_vbox, "VR Comfort Vignette on Move", _staged_controls->get_vr_comfort_vignette_on_move(), dom, godot::StringName("vr_comfort_vignette_on_move"));
}

void GameOptionsMenu::_build_difficulty_tab_content(godot::VBoxContainer *p_vbox) {
    if (!p_vbox || !_staged_difficulty.is_valid()) return;
    const godot::StringName dom("difficulty");

    bool mid_game_locked = (game != nullptr &&
                            (game->get_game_state() == GameState::PLAYING || game->get_game_state() == GameState::PAUSED) &&
                            !_staged_difficulty->get_allow_mid_game_change());

    if (mid_game_locked) {
        godot::Label *lock_banner = memnew(godot::Label);
        lock_banner->set_text("Note: Difficulty changes are locked while a session is in progress.");
        p_vbox->add_child(lock_banner);
    }

    _add_section_header(p_vbox, "Difficulty Scale Mode & Presets");
    {
        godot::PackedStringArray mode_items;
        mode_items.push_back("Unified Scale");
        mode_items.push_back("Split Aspects (Combat / Puzzle / Stealth / Survival)");
        godot::PackedInt32Array mode_ids = {0, 1};
        godot::OptionButton *mode_btn = _add_enum_row(p_vbox, "Difficulty Architecture", mode_items, mode_ids, static_cast<int>(_staged_difficulty->get_scale_mode()), dom, godot::StringName("scale_mode"));
        if (mid_game_locked && mode_btn) mode_btn->set_disabled(true);
    }

    godot::PackedStringArray tier_items;
    tier_items.push_back("Story");
    tier_items.push_back("Easy");
    tier_items.push_back("Normal");
    tier_items.push_back("Hard");
    tier_items.push_back("Nightmare");
    tier_items.push_back("Custom");
    godot::PackedInt32Array tier_ids = {0, 1, 2, 3, 4, 5};

    godot::OptionButton *ov_btn = _add_enum_row(p_vbox, "Overall Difficulty Level", tier_items, tier_ids, static_cast<int>(_staged_difficulty->get_overall_level()), dom, godot::StringName("overall_level"));
    if (mid_game_locked && ov_btn) ov_btn->set_disabled(true);

    if (_staged_difficulty->get_scale_mode() == DifficultyScaleMode::SCALE_MODE_SPLIT_ASPECTS) {
        godot::OptionButton *c_btn = _add_enum_row(p_vbox, "Action / Combat Level", tier_items, tier_ids, static_cast<int>(_staged_difficulty->get_combat_level()), dom, godot::StringName("combat_level"));
        godot::OptionButton *p_btn = _add_enum_row(p_vbox, "Riddle / Puzzle Level", tier_items, tier_ids, static_cast<int>(_staged_difficulty->get_puzzle_level()), dom, godot::StringName("puzzle_level"));
        godot::OptionButton *s_btn = _add_enum_row(p_vbox, "Stealth / AI Perception Level", tier_items, tier_ids, static_cast<int>(_staged_difficulty->get_stealth_level()), dom, godot::StringName("stealth_level"));
        godot::OptionButton *v_btn = _add_enum_row(p_vbox, "Survival / Economy Level", tier_items, tier_ids, static_cast<int>(_staged_difficulty->get_survival_level()), dom, godot::StringName("survival_level"));
        if (mid_game_locked) {
            if (c_btn) c_btn->set_disabled(true);
            if (p_btn) p_btn->set_disabled(true);
            if (s_btn) s_btn->set_disabled(true);
            if (v_btn) v_btn->set_disabled(true);
        }
    }

    _add_bool_row(p_vbox, "Dynamic Difficulty Adjustment (DDA)", _staged_difficulty->get_dynamic_difficulty_adjustment(), dom, godot::StringName("dynamic_difficulty_adjustment"));

    _add_section_header(p_vbox, "Fine-Tuned Domain Multipliers");
    _add_scalar_row(p_vbox, "Enemy Health Multiplier", _staged_difficulty->get_enemy_health_multiplier(), 0.1, 5.0, 0.05, dom, godot::StringName("enemy_health_multiplier"), "x");
    _add_scalar_row(p_vbox, "Enemy Damage Multiplier", _staged_difficulty->get_enemy_damage_multiplier(), 0.1, 5.0, 0.05, dom, godot::StringName("enemy_damage_multiplier"), "x");
    _add_scalar_row(p_vbox, "Player Damage Dealt Multiplier", _staged_difficulty->get_player_damage_dealt_multiplier(), 0.1, 5.0, 0.05, dom, godot::StringName("player_damage_dealt_multiplier"), "x");
    _add_scalar_row(p_vbox, "AI Vision Range Multiplier", _staged_difficulty->get_ai_vision_range_multiplier(), 0.1, 3.0, 0.05, dom, godot::StringName("ai_vision_range_multiplier"), "x");
    _add_scalar_row(p_vbox, "Loot Abundance Multiplier", _staged_difficulty->get_loot_abundance_multiplier(), 0.1, 4.0, 0.05, dom, godot::StringName("loot_abundance_multiplier"), "x");
    _add_bool_row(p_vbox, "Waypoint Guidance Enabled", _staged_difficulty->get_waypoint_guidance_enabled(), dom, godot::StringName("waypoint_guidance_enabled"));
    _add_bool_row(p_vbox, "Friendly Fire Enabled", _staged_difficulty->get_friendly_fire_enabled(), dom, godot::StringName("friendly_fire_enabled"));
}

void GameOptionsMenu::_build_perspective_tab_content(godot::VBoxContainer *p_vbox) {
    if (!p_vbox || !_staged_perspective.is_valid()) return;
    const godot::StringName dom("perspective");

    _add_section_header(p_vbox, "Camera Archetype & Projection");
    {
        godot::PackedStringArray items;
        items.push_back("Custom");
        items.push_back("First Person");
        items.push_back("Third Person Over-the-Shoulder");
        items.push_back("Third Person Orbit");
        items.push_back("Top-Down");
        items.push_back("3/4 Overhead");
        items.push_back("Isometric");
        items.push_back("Dimetric (2:1)");
        items.push_back("Side-Scroller / 2.5D");
        items.push_back("Fixed Cinematic");
        godot::PackedInt32Array ids = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        _add_enum_row(p_vbox, "Perspective Preset", items, ids, static_cast<int>(_staged_perspective->get_perspective_type()), dom, godot::StringName("perspective_type"));
    }
    {
        godot::PackedStringArray items;
        items.push_back("Perspective");
        items.push_back("Orthogonal");
        items.push_back("Frustum");
        godot::PackedInt32Array ids = {0, 1, 2};
        _add_enum_row(p_vbox, "Projection Mode", items, ids, static_cast<int>(_staged_perspective->get_projection_mode()), dom, godot::StringName("projection_mode"));
    }
    _add_scalar_row(p_vbox, "Field of View (FOV)", _staged_perspective->get_fov(), 30.0, 130.0, 1.0, dom, godot::StringName("fov"), " deg");
    _add_scalar_row(p_vbox, "Aiming Field of View", _staged_perspective->get_aim_fov(), 20.0, 110.0, 1.0, dom, godot::StringName("aim_fov"), " deg");

    _add_section_header(p_vbox, "Rig Offsets & Shoulder Alignment");
    {
        godot::PackedStringArray items;
        items.push_back("Center");
        items.push_back("Right Shoulder");
        items.push_back("Left Shoulder");
        godot::PackedInt32Array ids = {0, 1, 2};
        _add_enum_row(p_vbox, "Shoulder Side", items, ids, static_cast<int>(_staged_perspective->get_shoulder_side()), dom, godot::StringName("shoulder_side"));
    }
    _add_bool_row(p_vbox, "Allow Runtime Shoulder Swap", _staged_perspective->get_allow_shoulder_swap(), dom, godot::StringName("allow_shoulder_swap"));
    _add_scalar_row(p_vbox, "Spring Arm Length", _staged_perspective->get_arm_length(), 0.0, 25.0, 0.1, dom, godot::StringName("arm_length"), " m");
    _add_scalar_row(p_vbox, "Look Sensitivity X", _staged_perspective->get_look_sensitivity_x(), 0.1, 5.0, 0.05, dom, godot::StringName("look_sensitivity_x"));
    _add_scalar_row(p_vbox, "Look Sensitivity Y", _staged_perspective->get_look_sensitivity_y(), 0.1, 5.0, 0.05, dom, godot::StringName("look_sensitivity_y"));
    _add_bool_row(p_vbox, "Invert Look Y", _staged_perspective->get_invert_look_y(), dom, godot::StringName("invert_look_y"));
    _add_bool_row(p_vbox, "Show Aim Reticle", _staged_perspective->get_show_aim_reticle(), dom, godot::StringName("show_aim_reticle"));
}

void GameOptionsMenu::_build_accessibility_tab_content(godot::VBoxContainer *p_vbox) {
    if (!p_vbox || !_staged_accessibility.is_valid()) return;
    const godot::StringName dom("accessibility");

    _add_section_header(p_vbox, "UI Theme, Scaling & Readability");
    {
        godot::PackedStringArray items;
        items.push_back("Default Theme");
        items.push_back("High Contrast Dark");
        items.push_back("High Contrast Light");
        items.push_back("Yellow on Black");
        items.push_back("Custom Override");
        godot::PackedInt32Array ids = {0, 1, 2, 3, 4};
        _add_enum_row(p_vbox, "High Contrast Preset", items, ids, static_cast<int>(_staged_accessibility->get_high_contrast_preset()), dom, godot::StringName("high_contrast_preset"));
    }
    _add_scalar_row(p_vbox, "UI Scale Factor", _staged_accessibility->get_ui_scale(), 0.75, 2.0, 0.05, dom, godot::StringName("ui_scale"), "x");
    _add_scalar_row(p_vbox, "Font Scale Factor", _staged_accessibility->get_font_scale(), 0.75, 2.5, 0.05, dom, godot::StringName("font_scale"), "x");
    _add_scalar_row(p_vbox, "UI Background Opacity", _staged_accessibility->get_ui_background_opacity(), 0.2, 1.0, 0.05, dom, godot::StringName("ui_background_opacity"));

    _add_section_header(p_vbox, "Color Vision & Entity Highlights");
    {
        godot::PackedStringArray items;
        items.push_back("None (Standard Vision)");
        items.push_back("Protanopia (Red-Blind)");
        items.push_back("Deuteranopia (Green-Blind)");
        items.push_back("Tritanopia (Blue-Blind)");
        items.push_back("Achromatopsia (Monochrome)");
        items.push_back("Custom LUT");
        godot::PackedInt32Array ids = {0, 1, 2, 3, 4, 5};
        _add_enum_row(p_vbox, "Color Vision Mode", items, ids, static_cast<int>(_staged_accessibility->get_color_vision_mode()), dom, godot::StringName("color_vision_mode"));
    }
    _add_scalar_row(p_vbox, "Filter Strength", _staged_accessibility->get_color_vision_strength(), 0.0, 1.0, 0.05, dom, godot::StringName("color_vision_strength"));
    _add_bool_row(p_vbox, "High-Contrast 3D Entity Outlines", _staged_accessibility->get_high_contrast_entity_outlines(), dom, godot::StringName("high_contrast_entity_outlines"));
    _add_bool_row(p_vbox, "Disable Volumetric Fog for Clarity", _staged_accessibility->get_disable_volumetric_fog_for_clarity(), dom, godot::StringName("disable_volumetric_fog_for_clarity"));

    _add_section_header(p_vbox, "Motion, Photosensitivity & Captions");
    _add_bool_row(p_vbox, "Reduce Flashing Lights", _staged_accessibility->get_reduce_flashing_lights(), dom, godot::StringName("reduce_flashing_lights"));
    _add_scalar_row(p_vbox, "Screen Shake Scale", _staged_accessibility->get_screen_shake_scale(), 0.0, 2.0, 0.05, dom, godot::StringName("screen_shake_scale"), "x");
    _add_bool_row(p_vbox, "Reduce Camera Bob", _staged_accessibility->get_reduce_camera_bob(), dom, godot::StringName("reduce_camera_bob"));
    _add_bool_row(p_vbox, "Closed Captions Enabled", _staged_accessibility->get_closed_captions_enabled(), dom, godot::StringName("closed_captions_enabled"));
    _add_scalar_row(p_vbox, "Caption Text Scale", _staged_accessibility->get_caption_text_scale(), 0.75, 2.5, 0.05, dom, godot::StringName("caption_text_scale"), "x");
    _add_bool_row(p_vbox, "Visual Sound / Cymatic Indicators", _staged_accessibility->get_visual_sound_indicators(), dom, godot::StringName("visual_sound_indicators"));

    _add_section_header(p_vbox, "Motor & Cognitive Assists (!important Cascade Rules)");
    _add_scalar_row(p_vbox, "Overall Game Speed Scale", _staged_accessibility->get_game_speed_scale(), 0.25, 1.5, 0.05, dom, godot::StringName("game_speed_scale"), "x");
    _add_scalar_row(p_vbox, "Enemy Time Scale", _staged_accessibility->get_enemy_time_scale(), 0.25, 1.5, 0.05, dom, godot::StringName("enemy_time_scale"), "x");
    _add_scalar_row(p_vbox, "Player Damage Taken Multiplier", _staged_accessibility->get_player_damage_taken_multiplier(), 0.0, 2.0, 0.05, dom, godot::StringName("player_damage_taken_multiplier"), "x");
    {
        godot::PackedStringArray items;
        items.push_back("None");
        items.push_back("Reticle Friction");
        items.push_back("Bullet / Aim Magnetism");
        items.push_back("Target Snap Lock");
        godot::PackedInt32Array ids = {0, 1, 2, 3};
        _add_enum_row(p_vbox, "Aim Assist Mode", items, ids, static_cast<int>(_staged_accessibility->get_aim_assist_mode()), dom, godot::StringName("aim_assist_mode"));
    }
    _add_scalar_row(p_vbox, "Aim Assist Strength", _staged_accessibility->get_aim_assist_strength(), 0.0, 1.0, 0.05, dom, godot::StringName("aim_assist_strength"));
    _add_bool_row(p_vbox, "Hold-to-Toggle Aim", _staged_accessibility->get_hold_to_toggle_aim(), dom, godot::StringName("hold_to_toggle_aim"));
    _add_bool_row(p_vbox, "Auto-Fire on Hold", _staged_accessibility->get_auto_fire_on_hold(), dom, godot::StringName("auto_fire_on_hold"));
}

// --- 8. UI Signal Handlers ---

void GameOptionsMenu::_on_mode_selected(int p_index) {
    if (_updating_ui) return;

    active_mode_index = p_index;
    graphics_option.unref();
    audio_option.unref();
    control_map.unref();
    difficulty_option.unref();
    perspective_option.unref();
    accessibility_option.unref();

    _create_staging_clones();
    refresh_ui_from_staged();

    emit_signal("mode_selected", active_mode_index, target_mode);
}

void GameOptionsMenu::_on_enum_changed(int p_index, const godot::StringName &p_domain, const godot::StringName &p_prop, godot::Object *p_option_btn) {
    if (_updating_ui) return;

    godot::OptionButton *btn = godot::Object::cast_to<godot::OptionButton>(p_option_btn);
    int id = btn ? btn->get_item_id(p_index) : p_index;

    if (p_domain == godot::StringName("graphics") && _staged_graphics.is_valid()) {
        if (p_prop == godot::StringName("quality_preset")) {
            _staged_graphics->set_quality_preset(static_cast<GraphicsQualityPreset>(id));
            refresh_ui_from_staged();
        } else {
            _staged_graphics->set(p_prop, id);
        }
    } else if (p_domain == godot::StringName("audio") && _staged_audio.is_valid()) {
        _staged_audio->set(p_prop, id);
    } else if (p_domain == godot::StringName("controls") && _staged_controls.is_valid()) {
        if (p_prop == godot::StringName("context_mode")) {
            _staged_controls->apply_context_preset(static_cast<ControlContextMode>(id));
            refresh_ui_from_staged();
        } else {
            _staged_controls->set(p_prop, id);
        }
    } else if (p_domain == godot::StringName("difficulty") && _staged_difficulty.is_valid()) {
        if (p_prop == godot::StringName("scale_mode")) {
            _staged_difficulty->set_scale_mode(static_cast<DifficultyScaleMode>(id));
            refresh_ui_from_staged();
        } else if (p_prop == godot::StringName("overall_level")) {
            _staged_difficulty->apply_overall_preset(static_cast<DifficultyLevel>(id));
            refresh_ui_from_staged();
        } else {
            _staged_difficulty->set(p_prop, id);
            refresh_ui_from_staged();
        }
    } else if (p_domain == godot::StringName("perspective") && _staged_perspective.is_valid()) {
        if (p_prop == godot::StringName("perspective_type")) {
            _staged_perspective->apply_perspective_preset(static_cast<PerspectiveType>(id));
            refresh_ui_from_staged();
        } else {
            _staged_perspective->set(p_prop, id);
        }
    } else if (p_domain == godot::StringName("accessibility") && _staged_accessibility.is_valid()) {
        _staged_accessibility->set(p_prop, id);
    }

    if (live_preview_enabled) {
        _apply_staged_to_runtime(false);
    }
    emit_signal("option_changed", p_domain, p_prop, id);
}

void GameOptionsMenu::_on_scalar_changed(double p_value, const godot::StringName &p_domain, const godot::StringName &p_prop) {
    if (_updating_ui) return;

    if (p_domain == godot::StringName("graphics") && _staged_graphics.is_valid()) {
        _staged_graphics->set(p_prop, p_value);
    } else if (p_domain == godot::StringName("audio") && _staged_audio.is_valid()) {
        _staged_audio->set(p_prop, p_value);
    } else if (p_domain == godot::StringName("controls") && _staged_controls.is_valid()) {
        if (p_prop == godot::StringName("mouse_sensitivity_x")) {
            godot::Vector2 s = _staged_controls->get_mouse_sensitivity();
            s.x = static_cast<float>(p_value);
            _staged_controls->set_mouse_sensitivity(s);
        } else if (p_prop == godot::StringName("mouse_sensitivity_y")) {
            godot::Vector2 s = _staged_controls->get_mouse_sensitivity();
            s.y = static_cast<float>(p_value);
            _staged_controls->set_mouse_sensitivity(s);
        } else {
            _staged_controls->set(p_prop, p_value);
        }
    } else if (p_domain == godot::StringName("difficulty") && _staged_difficulty.is_valid()) {
        _staged_difficulty->set(p_prop, p_value);
    } else if (p_domain == godot::StringName("perspective") && _staged_perspective.is_valid()) {
        _staged_perspective->set(p_prop, p_value);
    } else if (p_domain == godot::StringName("accessibility") && _staged_accessibility.is_valid()) {
        _staged_accessibility->set(p_prop, p_value);
    }

    if (live_preview_enabled) {
        _apply_staged_to_runtime(false);
    }
    emit_signal("option_changed", p_domain, p_prop, p_value);
}

void GameOptionsMenu::_on_bool_changed(bool p_pressed, const godot::StringName &p_domain, const godot::StringName &p_prop) {
    if (_updating_ui) return;

    if (p_domain == godot::StringName("graphics") && _staged_graphics.is_valid()) {
        _staged_graphics->set(p_prop, p_pressed);
    } else if (p_domain == godot::StringName("audio") && _staged_audio.is_valid()) {
        _staged_audio->set(p_prop, p_pressed);
    } else if (p_domain == godot::StringName("controls") && _staged_controls.is_valid()) {
        _staged_controls->set(p_prop, p_pressed);
    } else if (p_domain == godot::StringName("difficulty") && _staged_difficulty.is_valid()) {
        _staged_difficulty->set(p_prop, p_pressed);
    } else if (p_domain == godot::StringName("perspective") && _staged_perspective.is_valid()) {
        _staged_perspective->set(p_prop, p_pressed);
    } else if (p_domain == godot::StringName("accessibility") && _staged_accessibility.is_valid()) {
        _staged_accessibility->set(p_prop, p_pressed);
    }

    if (live_preview_enabled) {
        _apply_staged_to_runtime(false);
    }
    emit_signal("option_changed", p_domain, p_prop, p_pressed);
}

void GameOptionsMenu::_on_display_confirmed() {
    _awaiting_display_confirmation = false;
    _display_confirm_remaining = 0.0f;
    if (_staged_graphics.is_valid()) {
        _backup_graphics_data = _staged_graphics->save_resource();
    }
    if (auto_save_config_on_apply) {
        save_options_to_config(config_file_path);
    }
    close_options();
}

void GameOptionsMenu::_on_display_reverted() {
    _awaiting_display_confirmation = false;
    _display_confirm_remaining = 0.0f;
    if (display_confirm_dialog && display_confirm_dialog->is_visible()) {
        display_confirm_dialog->hide();
    }

    if (graphics_option.is_valid() && !_backup_graphics_data.is_empty()) {
        graphics_option->load_resource(_backup_graphics_data);
        if (_staged_graphics.is_valid()) {
            _staged_graphics->apply_graphics_option(graphics_option);
        }
        _apply_staged_to_runtime(false);
        refresh_ui_from_staged();
    }

    emit_signal("display_changes_reverted");
}

// --- 9. Public Lifecycle & Persistence Operations ---

void GameOptionsMenu::open_options() {
    _create_staging_clones();
    refresh_ui_from_staged();
    show();
    emit_signal("opened");
}

void GameOptionsMenu::open_options_for_entity(GameEntity *p_entity) {
    target_entity = p_entity;
    target_style.unref();
    target_mode.unref();
    graphics_option.unref();
    audio_option.unref();
    control_map.unref();
    difficulty_option.unref();
    perspective_option.unref();
    accessibility_option.unref();

    open_options();
}

void GameOptionsMenu::close_options() {
    hide();
    emit_signal("closed");
}

void GameOptionsMenu::apply_options() {
    bool display_mode_changed = false;
    if (_staged_graphics.is_valid() && !_backup_graphics_data.is_empty()) {
        int old_mode = _backup_graphics_data.has("display_mode")
            ? static_cast<int>(_backup_graphics_data["display_mode"])
            : static_cast<int>(_staged_graphics->get_display_mode());
        float old_scale = _backup_graphics_data.has("resolution_scale")
            ? static_cast<float>(_backup_graphics_data["resolution_scale"])
            : _staged_graphics->get_resolution_scale();

        display_mode_changed =
            (old_mode != static_cast<int>(_staged_graphics->get_display_mode())) ||
            !godot::Math::is_equal_approx(old_scale, _staged_graphics->get_resolution_scale());
    }

    _commit_staged_to_authoritative();

    if (display_mode_changed && display_confirm_dialog != nullptr && !godot::Engine::get_singleton()->is_editor_hint()) {
        _awaiting_display_confirmation = true;
        _display_confirm_remaining = display_confirm_timeout;
        display_confirm_dialog->set_text(
            "Keep these display settings?\nReverting automatically in " +
            godot::String::num_int64(static_cast<int>(std::ceil(_display_confirm_remaining))) + " seconds..."
        );
        display_confirm_dialog->popup_centered();
        emit_signal("applied");
        return;
    }

    if (auto_save_config_on_apply) {
        save_options_to_config(config_file_path);
    }

    emit_signal("applied");
    close_options();
}

void GameOptionsMenu::cancel_options() {
    _awaiting_display_confirmation = false;
    _restore_from_backups();
    emit_signal("canceled");
    close_options();
}

void GameOptionsMenu::reset_to_defaults() {
    _staged_graphics.instantiate();
    _staged_audio.instantiate();
    _staged_controls.instantiate();
    _staged_difficulty.instantiate();
    _staged_perspective.instantiate();
    _staged_accessibility.instantiate();

    refresh_ui_from_staged();
    if (live_preview_enabled) {
        _apply_staged_to_runtime(false);
    }
    emit_signal("defaults_restored");
}

bool GameOptionsMenu::save_options_to_config(const godot::String &p_path) {
    godot::String path = p_path.is_empty() ? config_file_path : p_path;
    if (path.is_empty()) {
        return false;
    }

    _resolve_context_resources();
    godot::Ref<godot::ConfigFile> cfg;
    cfg.instantiate();
    cfg->load(path);

    if (graphics_option.is_valid()) {
        graphics_option->save_to_config(cfg, "rendering");
        graphics_option->save_to_tps_config(cfg);
    }
    if (audio_option.is_valid()) {
        audio_option->save_to_config(cfg, "audio");
    }
    if (control_map.is_valid()) {
        control_map->save_to_config(cfg, "controls");
    }
    if (difficulty_option.is_valid()) {
        difficulty_option->save_to_config(cfg, "difficulty");
    }
    if (perspective_option.is_valid()) {
        perspective_option->save_to_config(cfg, "perspective");
    }
    if (accessibility_option.is_valid()) {
        accessibility_option->save_to_config(cfg, "accessibility");
    }

    return cfg->save(path) == godot::OK;
}

bool GameOptionsMenu::load_options_from_config(const godot::String &p_path) {
    godot::String path = p_path.is_empty() ? config_file_path : p_path;
    if (path.is_empty()) {
        return false;
    }

    _resolve_context_resources();
    godot::Ref<godot::ConfigFile> cfg;
    cfg.instantiate();
    if (cfg->load(path) != godot::OK) {
        return false;
    }

    if (graphics_option.is_valid()) {
        graphics_option->load_from_config(cfg, "rendering");
        graphics_option->load_from_tps_config(cfg);
    }
    if (audio_option.is_valid()) {
        audio_option->load_from_config(cfg, "audio");
    }
    if (control_map.is_valid()) {
        control_map->load_from_config(cfg, "controls");
    }
    if (difficulty_option.is_valid()) {
        difficulty_option->load_from_config(cfg, "difficulty");
    }
    if (perspective_option.is_valid()) {
        perspective_option->load_from_config(cfg, "perspective");
    }
    if (accessibility_option.is_valid()) {
        accessibility_option->load_from_config(cfg, "accessibility");
    }

    _create_staging_clones();
    _apply_staged_to_runtime(true);
    refresh_ui_from_staged();
    return true;
}

} // namespace ideam::godot_ext