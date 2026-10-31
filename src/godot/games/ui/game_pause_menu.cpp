#include "game_pause_menu.h"

#include <godot_cpp/classes/box_container.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/h_separator.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/margin_container.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "../game.h"
#include "../game_entities/game_agent.h"
#include "../game_entities/game_board.h"
#include "../gameplay/gameplay_control_map.h"
#include "../gameplay/gameplay_difficulty.h"
#include "../gameplay/gameplay_mode.h"
#include "../gameplay/gameplay_perspective.h"
#include "../gameplay/gameplay_style.h"
#include "game_menu.h"
#include "game_options_menu.h"

namespace ideam::godot_ext {

void GamePauseMenu::_bind_methods() {
    // --- Signals ---
    ADD_SIGNAL(godot::MethodInfo("opened"));
    ADD_SIGNAL(godot::MethodInfo("closed"));
    ADD_SIGNAL(godot::MethodInfo("resumed"));
    ADD_SIGNAL(godot::MethodInfo("options_requested"));
    ADD_SIGNAL(godot::MethodInfo("restart_board_requested"));
    ADD_SIGNAL(godot::MethodInfo("return_to_menu_requested"));
    ADD_SIGNAL(godot::MethodInfo("quit_requested"));
    ADD_SIGNAL(godot::MethodInfo(
        "mode_switched",
        godot::PropertyInfo(godot::Variant::INT, "mode_index"),
        godot::PropertyInfo(godot::Variant::OBJECT, "mode", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayMode")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "perspective_switched",
        godot::PropertyInfo(godot::Variant::INT, "perspective_type")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "difficulty_switched",
        godot::PropertyInfo(godot::Variant::INT, "difficulty_level")
    ));
    ADD_SIGNAL(godot::MethodInfo("shoulder_swapped"));

    // --- Lifecycle & Action Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("validate_pause_context"), &GamePauseMenu::validate_pause_context);
    godot::ClassDB::bind_method(godot::D_METHOD("build_pause_menu"), &GamePauseMenu::build_pause_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("refresh_pause_telemetry"), &GamePauseMenu::refresh_pause_telemetry);

    godot::ClassDB::bind_method(godot::D_METHOD("open_pause_menu"), &GamePauseMenu::open_pause_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("close_pause_menu"), &GamePauseMenu::close_pause_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("toggle_pause_menu"), &GamePauseMenu::toggle_pause_menu);

    godot::ClassDB::bind_method(godot::D_METHOD("resume_pressed"), &GamePauseMenu::resume_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("save_game_pressed"), &GamePauseMenu::save_game_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game_pressed"), &GamePauseMenu::load_game_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("restart_board_pressed"), &GamePauseMenu::restart_board_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("options_pressed"), &GamePauseMenu::options_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("swap_shoulder_pressed"), &GamePauseMenu::swap_shoulder_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("main_menu_pressed"), &GamePauseMenu::main_menu_pressed);
    godot::ClassDB::bind_method(godot::D_METHOD("quit_pressed"), &GamePauseMenu::quit_pressed);

    // --- Internal Signal Callbacks ---
    godot::ClassDB::bind_method(godot::D_METHOD("_on_game_paused"), &GamePauseMenu::_on_game_paused);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_game_continued"), &GamePauseMenu::_on_game_continued);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_options_menu_closed"), &GamePauseMenu::_on_options_menu_closed);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_quick_mode_selected", "index"), &GamePauseMenu::_on_quick_mode_selected);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_quick_perspective_selected", "index"), &GamePauseMenu::_on_quick_perspective_selected);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_quick_difficulty_selected", "index"), &GamePauseMenu::_on_quick_difficulty_selected);

    // --- 1. Context & Linked Menus Properties ---
    ADD_GROUP("Context & Linked Menus", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game", "game"), &GamePauseMenu::set_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game"), &GamePauseMenu::get_game);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_NODE_TYPE, "Game"), "set_game", "get_game");

    godot::ClassDB::bind_method(godot::D_METHOD("set_active_board", "active_board"), &GamePauseMenu::set_active_board);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_board"), &GamePauseMenu::get_active_board);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "active_board", godot::PROPERTY_HINT_NODE_TYPE, "GameBoard"), "set_active_board", "get_active_board");

    godot::ClassDB::bind_method(godot::D_METHOD("set_controlled_agent", "controlled_agent"), &GamePauseMenu::set_controlled_agent);
    godot::ClassDB::bind_method(godot::D_METHOD("get_controlled_agent"), &GamePauseMenu::get_controlled_agent);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "controlled_agent", godot::PROPERTY_HINT_NODE_TYPE, "GameAgent"), "set_controlled_agent", "get_controlled_agent");

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_menu", "game_menu"), &GamePauseMenu::set_game_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_menu"), &GamePauseMenu::get_game_menu);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_menu", godot::PROPERTY_HINT_NODE_TYPE, "GameMenu"), "set_game_menu", "get_game_menu");

    godot::ClassDB::bind_method(godot::D_METHOD("set_options_menu", "options_menu"), &GamePauseMenu::set_options_menu);
    godot::ClassDB::bind_method(godot::D_METHOD("get_options_menu"), &GamePauseMenu::get_options_menu);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "options_menu", godot::PROPERTY_HINT_NODE_TYPE, "GameOptionsMenu"), "set_options_menu", "get_options_menu");

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_build_fallback_ui", "auto_build"), &GamePauseMenu::set_auto_build_fallback_ui);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_build_fallback_ui"), &GamePauseMenu::get_auto_build_fallback_ui);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_build_fallback_ui"), "set_auto_build_fallback_ui", "get_auto_build_fallback_ui");

    godot::ClassDB::bind_method(godot::D_METHOD("set_handle_pause_input", "handle_pause_input"), &GamePauseMenu::set_handle_pause_input);
    godot::ClassDB::bind_method(godot::D_METHOD("get_handle_pause_input"), &GamePauseMenu::get_handle_pause_input);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "handle_pause_input"), "set_handle_pause_input", "get_handle_pause_input");

    godot::ClassDB::bind_method(godot::D_METHOD("set_fallback_pause_action", "action"), &GamePauseMenu::set_fallback_pause_action);
    godot::ClassDB::bind_method(godot::D_METHOD("get_fallback_pause_action"), &GamePauseMenu::get_fallback_pause_action);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::STRING_NAME, "fallback_pause_action"), "set_fallback_pause_action", "get_fallback_pause_action");

    // --- 2. Slot-Injected UI Controls ---
    ADD_GROUP("UI Node Slots", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_pause_title_label", "label"), &GamePauseMenu::set_pause_title_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_pause_title_label"), &GamePauseMenu::get_pause_title_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "pause_title_label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_pause_title_label", "get_pause_title_label");

    godot::ClassDB::bind_method(godot::D_METHOD("set_context_status_label", "label"), &GamePauseMenu::set_context_status_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_context_status_label"), &GamePauseMenu::get_context_status_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "context_status_label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_context_status_label", "get_context_status_label");

    godot::ClassDB::bind_method(godot::D_METHOD("set_active_tags_label", "label"), &GamePauseMenu::set_active_tags_label);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_tags_label"), &GamePauseMenu::get_active_tags_label);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "active_tags_label", godot::PROPERTY_HINT_NODE_TYPE, "Label"), "set_active_tags_label", "get_active_tags_label");

    godot::ClassDB::bind_method(godot::D_METHOD("set_mode_selector", "selector"), &GamePauseMenu::set_mode_selector);
    godot::ClassDB::bind_method(godot::D_METHOD("get_mode_selector"), &GamePauseMenu::get_mode_selector);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "mode_selector", godot::PROPERTY_HINT_NODE_TYPE, "OptionButton"), "set_mode_selector", "get_mode_selector");

    godot::ClassDB::bind_method(godot::D_METHOD("set_perspective_selector", "selector"), &GamePauseMenu::set_perspective_selector);
    godot::ClassDB::bind_method(godot::D_METHOD("get_perspective_selector"), &GamePauseMenu::get_perspective_selector);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "perspective_selector", godot::PROPERTY_HINT_NODE_TYPE, "OptionButton"), "set_perspective_selector", "get_perspective_selector");

    godot::ClassDB::bind_method(godot::D_METHOD("set_swap_shoulder_button", "button"), &GamePauseMenu::set_swap_shoulder_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_swap_shoulder_button"), &GamePauseMenu::get_swap_shoulder_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "swap_shoulder_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_swap_shoulder_button", "get_swap_shoulder_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_difficulty_selector", "selector"), &GamePauseMenu::set_difficulty_selector);
    godot::ClassDB::bind_method(godot::D_METHOD("get_difficulty_selector"), &GamePauseMenu::get_difficulty_selector);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "difficulty_selector", godot::PROPERTY_HINT_NODE_TYPE, "OptionButton"), "set_difficulty_selector", "get_difficulty_selector");

    godot::ClassDB::bind_method(godot::D_METHOD("set_resume_button", "button"), &GamePauseMenu::set_resume_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_resume_button"), &GamePauseMenu::get_resume_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "resume_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_resume_button", "get_resume_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_save_game_button", "button"), &GamePauseMenu::set_save_game_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_save_game_button"), &GamePauseMenu::get_save_game_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "save_game_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_save_game_button", "get_save_game_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_load_game_button", "button"), &GamePauseMenu::set_load_game_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_load_game_button"), &GamePauseMenu::get_load_game_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "load_game_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_load_game_button", "get_load_game_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_restart_board_button", "button"), &GamePauseMenu::set_restart_board_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_restart_board_button"), &GamePauseMenu::get_restart_board_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "restart_board_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_restart_board_button", "get_restart_board_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_options_button", "button"), &GamePauseMenu::set_options_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_options_button"), &GamePauseMenu::get_options_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "options_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_options_button", "get_options_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_main_menu_button", "button"), &GamePauseMenu::set_main_menu_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_main_menu_button"), &GamePauseMenu::get_main_menu_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "main_menu_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_main_menu_button", "get_main_menu_button");

    godot::ClassDB::bind_method(godot::D_METHOD("set_quit_button", "button"), &GamePauseMenu::set_quit_button);
    godot::ClassDB::bind_method(godot::D_METHOD("get_quit_button"), &GamePauseMenu::get_quit_button);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "quit_button", godot::PROPERTY_HINT_NODE_TYPE, "Button"), "set_quit_button", "get_quit_button");
}

GamePauseMenu::GamePauseMenu() {
    set_process_unhandled_input(true);
}

GamePauseMenu::~GamePauseMenu() {
}

void GamePauseMenu::_ready() {
    validate_pause_context();
    build_pause_menu();
    hide();
}

void GamePauseMenu::_unhandled_input(const godot::Ref<godot::InputEvent> &p_event) {
    if (!handle_pause_input || !p_event.is_valid() || godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    // Determine pause action name from the active GameplayControlMap, falling back to `fallback_pause_action`
    godot::StringName pause_act = fallback_pause_action;
    godot::Ref<GameplayControlMap> cmap = _resolve_active_control_map();
    if (cmap.is_valid() && !cmap->get_pause_action().is_empty()) {
        pause_act = cmap->get_pause_action();
    }

    if (p_event->is_action_pressed(pause_act) ||
        (!fallback_pause_action.is_empty() && p_event->is_action_pressed(fallback_pause_action))) {
        if (options_menu != nullptr && options_menu->is_visible()) {
            options_menu->cancel_options();
        } else {
            toggle_pause_menu();
        }
        if (get_viewport() != nullptr) {
            get_viewport()->set_input_as_handled();
        }
    }
}

// --- 1. Context & Configuration Setters / Getters ---

void GamePauseMenu::set_game(Game *p_game) {
    if (game == p_game) return;
    game = p_game;
    _connect_game_signals();
}
Game *GamePauseMenu::get_game() const { return game; }

void GamePauseMenu::set_active_board(GameBoard *p_board) { active_board = p_board; }
GameBoard *GamePauseMenu::get_active_board() const { return active_board; }

void GamePauseMenu::set_controlled_agent(GameAgent *p_agent) { controlled_agent = p_agent; }
GameAgent *GamePauseMenu::get_controlled_agent() const { return controlled_agent; }

void GamePauseMenu::set_game_menu(GameMenu *p_menu) { game_menu = p_menu; }
GameMenu *GamePauseMenu::get_game_menu() const { return game_menu; }

void GamePauseMenu::set_options_menu(GameOptionsMenu *p_menu) {
    options_menu = p_menu;
    if (options_menu != nullptr && !options_menu->is_connected("closed", godot::Callable(this, "_on_options_menu_closed"))) {
        options_menu->connect("closed", godot::Callable(this, "_on_options_menu_closed"));
    }
}
GameOptionsMenu *GamePauseMenu::get_options_menu() const { return options_menu; }

void GamePauseMenu::set_auto_build_fallback_ui(bool p_auto_build) { auto_build_fallback_ui = p_auto_build; }
bool GamePauseMenu::get_auto_build_fallback_ui() const { return auto_build_fallback_ui; }

void GamePauseMenu::set_handle_pause_input(bool p_handle) { handle_pause_input = p_handle; }
bool GamePauseMenu::get_handle_pause_input() const { return handle_pause_input; }

void GamePauseMenu::set_fallback_pause_action(const godot::StringName &p_action) { fallback_pause_action = p_action; }
godot::StringName GamePauseMenu::get_fallback_pause_action() const { return fallback_pause_action; }

// --- 2. Slot-Injected Control Setters / Getters ---

void GamePauseMenu::set_pause_title_label(godot::Label *p_label) { pause_title_label = p_label; }
godot::Label *GamePauseMenu::get_pause_title_label() const { return pause_title_label; }

void GamePauseMenu::set_context_status_label(godot::Label *p_label) { context_status_label = p_label; }
godot::Label *GamePauseMenu::get_context_status_label() const { return context_status_label; }

void GamePauseMenu::set_active_tags_label(godot::Label *p_label) { active_tags_label = p_label; }
godot::Label *GamePauseMenu::get_active_tags_label() const { return active_tags_label; }

void GamePauseMenu::set_mode_selector(godot::OptionButton *p_selector) { mode_selector = p_selector; }
godot::OptionButton *GamePauseMenu::get_mode_selector() const { return mode_selector; }

void GamePauseMenu::set_perspective_selector(godot::OptionButton *p_selector) { perspective_selector = p_selector; }
godot::OptionButton *GamePauseMenu::get_perspective_selector() const { return perspective_selector; }

void GamePauseMenu::set_swap_shoulder_button(godot::Button *p_button) { swap_shoulder_button = p_button; }
godot::Button *GamePauseMenu::get_swap_shoulder_button() const { return swap_shoulder_button; }

void GamePauseMenu::set_difficulty_selector(godot::OptionButton *p_selector) { difficulty_selector = p_selector; }
godot::OptionButton *GamePauseMenu::get_difficulty_selector() const { return difficulty_selector; }

void GamePauseMenu::set_resume_button(godot::Button *p_button) { resume_button = p_button; }
godot::Button *GamePauseMenu::get_resume_button() const { return resume_button; }

void GamePauseMenu::set_save_game_button(godot::Button *p_button) { save_game_button = p_button; }
godot::Button *GamePauseMenu::get_save_game_button() const { return save_game_button; }

void GamePauseMenu::set_load_game_button(godot::Button *p_button) { load_game_button = p_button; }
godot::Button *GamePauseMenu::get_load_game_button() const { return load_game_button; }

void GamePauseMenu::set_restart_board_button(godot::Button *p_button) { restart_board_button = p_button; }
godot::Button *GamePauseMenu::get_restart_board_button() const { return restart_board_button; }

void GamePauseMenu::set_options_button(godot::Button *p_button) { options_button = p_button; }
godot::Button *GamePauseMenu::get_options_button() const { return options_button; }

void GamePauseMenu::set_main_menu_button(godot::Button *p_button) { main_menu_button = p_button; }
godot::Button *GamePauseMenu::get_main_menu_button() const { return main_menu_button; }

void GamePauseMenu::set_quit_button(godot::Button *p_button) { quit_button = p_button; }
godot::Button *GamePauseMenu::get_quit_button() const { return quit_button; }

// --- 3. Context Discovery & GameplayStyle Cascade Resolution ---

void GamePauseMenu::validate_pause_context() {
    godot::Node *cursor = get_parent();
    while (cursor != nullptr) {
        if (controlled_agent == nullptr) {
            if (GameAgent *ag = godot::Object::cast_to<GameAgent>(cursor)) {
                controlled_agent = ag;
            }
        }
        if (active_board == nullptr) {
            if (GameBoard *bd = godot::Object::cast_to<GameBoard>(cursor)) {
                active_board = bd;
            }
        }
        if (game == nullptr) {
            if (Game *gm = godot::Object::cast_to<Game>(cursor)) {
                game = gm;
                break;
            }
        }
        cursor = cursor->get_parent();
    }

    // Discover active player-controlled GameAgent on active_board if not directly parented
    if (controlled_agent == nullptr && active_board != nullptr) {
        godot::TypedArray<GameAgent> agents = active_board->get_game_agents();
        for (int i = 0; i < agents.size(); ++i) {
            GameAgent *ag = godot::Object::cast_to<GameAgent>(agents[i]);
            if (ag != nullptr && ag->get_player() != nullptr) {
                controlled_agent = ag;
                break;
            }
        }
        if (controlled_agent == nullptr && agents.size() > 0) {
            controlled_agent = godot::Object::cast_to<GameAgent>(agents[0]);
        }
    }

    _connect_game_signals();
}

void GamePauseMenu::_connect_game_signals() {
    if (game == nullptr) return;

    if (!game->is_connected("game_paused", godot::Callable(this, "_on_game_paused"))) {
        game->connect("game_paused", godot::Callable(this, "_on_game_paused"));
    }
    if (!game->is_connected("game_continued", godot::Callable(this, "_on_game_continued"))) {
        game->connect("game_continued", godot::Callable(this, "_on_game_continued"));
    }
}

godot::Ref<GameplayStyle> GamePauseMenu::_resolve_active_style() const {
    if (controlled_agent != nullptr) {
        if (controlled_agent->get_resolved_gameplay_style().is_valid()) {
            return controlled_agent->get_resolved_gameplay_style();
        }
        if (controlled_agent->get_gameplay_style().is_valid()) {
            return controlled_agent->get_gameplay_style();
        }
    }
    if (active_board != nullptr) {
        if (active_board->get_resolved_gameplay_style().is_valid()) {
            return active_board->get_resolved_gameplay_style();
        }
        if (active_board->get_gameplay_style().is_valid()) {
            return active_board->get_gameplay_style();
        }
    }
    if (game != nullptr) {
        if (game->get_resolved_gameplay_style().is_valid()) {
            return game->get_resolved_gameplay_style();
        }
        if (game->get_gameplay_style().is_valid()) {
            return game->get_gameplay_style();
        }
    }
    return godot::Ref<GameplayStyle>();
}

godot::Ref<GameplayMode> GamePauseMenu::_resolve_active_mode() const {
    godot::Ref<GameplayStyle> style = _resolve_active_style();
    if (style.is_valid() && style->get_modes().size() > 0) {
        int idx = 0;
        if (mode_selector != nullptr && mode_selector->get_selected() >= 0 && mode_selector->get_selected() < style->get_modes().size()) {
            idx = mode_selector->get_selected();
        }
        return godot::Ref<GameplayMode>(godot::Object::cast_to<GameplayMode>(style->get_modes()[idx]));
    }
    return godot::Ref<GameplayMode>();
}

godot::Ref<GameplayPerspective> GamePauseMenu::_resolve_active_perspective() const {
    godot::Ref<GameplayMode> mode = _resolve_active_mode();
    if (mode.is_valid() && mode->get_perspectives().size() > 0) {
        return godot::Ref<GameplayPerspective>(godot::Object::cast_to<GameplayPerspective>(mode->get_perspectives()[0]));
    }
    return godot::Ref<GameplayPerspective>();
}

godot::Ref<GameplayDifficulty> GamePauseMenu::_resolve_active_difficulty() const {
    godot::Ref<GameplayMode> mode = _resolve_active_mode();
    if (mode.is_valid() && mode->get_difficulties().size() > 0) {
        return godot::Ref<GameplayDifficulty>(godot::Object::cast_to<GameplayDifficulty>(mode->get_difficulties()[0]));
    }
    return godot::Ref<GameplayDifficulty>();
}

godot::Ref<GameplayControlMap> GamePauseMenu::_resolve_active_control_map() const {
    godot::Ref<GameplayMode> mode = _resolve_active_mode();
    if (mode.is_valid() && mode->get_control_maps().size() > 0) {
        return godot::Ref<GameplayControlMap>(godot::Object::cast_to<GameplayControlMap>(mode->get_control_maps()[0]));
    }
    return godot::Ref<GameplayControlMap>();
}

void GamePauseMenu::_reapply_active_style_cascade() {
    if (game != nullptr) {
        game->apply_gameplay_style(game->get_gameplay_style());
    } else if (active_board != nullptr) {
        active_board->apply_gameplay_style(active_board->get_gameplay_style());
    } else if (controlled_agent != nullptr) {
        controlled_agent->apply_gameplay_style(controlled_agent->get_gameplay_style());
    }
    refresh_pause_telemetry();
}

// --- 4. UI Building & Validation ---

void GamePauseMenu::build_pause_menu() {
    if (_root_vbox == nullptr) {
        _root_vbox = godot::Object::cast_to<godot::VBoxContainer>(find_child("Pause_Root_VBox", true, false));
        if (_root_vbox == nullptr && auto_build_fallback_ui) {
            godot::PanelContainer *panel = memnew(godot::PanelContainer);
            panel->set_name("Pause_Panel");
            panel->set_anchors_and_offsets_preset(godot::Control::PRESET_CENTER);
            panel->set_custom_minimum_size(godot::Vector2(360.0f, 0.0f));
            add_child(panel);

            _root_vbox = memnew(godot::VBoxContainer);
            _root_vbox->set_name("Pause_Root_VBox");
            panel->add_child(_root_vbox);
        }
    }

    if (pause_title_label == nullptr) {
        pause_title_label = godot::Object::cast_to<godot::Label>(find_child("Pause_Title", true, false));
        if (pause_title_label == nullptr && _root_vbox != nullptr && auto_build_fallback_ui) {
            pause_title_label = memnew(godot::Label);
            pause_title_label->set_name("Pause_Title");
            pause_title_label->set_text("PAUSED");
            pause_title_label->set_horizontal_alignment(godot::HORIZONTAL_ALIGNMENT_CENTER);
            _root_vbox->add_child(pause_title_label);
        }
    }

    if (context_status_label == nullptr) {
        context_status_label = godot::Object::cast_to<godot::Label>(find_child("Context_Status_Label", true, false));
        if (context_status_label == nullptr && _root_vbox != nullptr && auto_build_fallback_ui) {
            context_status_label = memnew(godot::Label);
            context_status_label->set_name("Context_Status_Label");
            context_status_label->set_horizontal_alignment(godot::HORIZONTAL_ALIGNMENT_CENTER);
            _root_vbox->add_child(context_status_label);
        }
    }

    if (active_tags_label == nullptr) {
        active_tags_label = godot::Object::cast_to<godot::Label>(find_child("Active_Tags_Label", true, false));
        if (active_tags_label == nullptr && _root_vbox != nullptr && auto_build_fallback_ui) {
            active_tags_label = memnew(godot::Label);
            active_tags_label->set_name("Active_Tags_Label");
            active_tags_label->set_horizontal_alignment(godot::HORIZONTAL_ALIGNMENT_CENTER);
            _root_vbox->add_child(active_tags_label);
        }
    }

    _validate_quick_switch_bar();
    _validate_buttons();
    refresh_pause_telemetry();
}

void GamePauseMenu::_validate_quick_switch_bar() {
    if (mode_selector == nullptr) {
        mode_selector = godot::Object::cast_to<godot::OptionButton>(find_child("Quick_Mode_Selector", true, false));
    }
    if (perspective_selector == nullptr) {
        perspective_selector = godot::Object::cast_to<godot::OptionButton>(find_child("Quick_Perspective_Selector", true, false));
    }
    if (swap_shoulder_button == nullptr) {
        swap_shoulder_button = godot::Object::cast_to<godot::Button>(find_child("Swap_Shoulder_Button", true, false));
    }
    if (difficulty_selector == nullptr) {
        difficulty_selector = godot::Object::cast_to<godot::OptionButton>(find_child("Quick_Difficulty_Selector", true, false));
    }

    if (auto_build_fallback_ui && _root_vbox != nullptr) {
        if (mode_selector == nullptr) {
            godot::HBoxContainer *row = memnew(godot::HBoxContainer);
            _root_vbox->add_child(row);
            godot::Label *lbl = memnew(godot::Label);
            lbl->set_text("Mode:");
            lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
            row->add_child(lbl);
            mode_selector = memnew(godot::OptionButton);
            mode_selector->set_name("Quick_Mode_Selector");
            row->add_child(mode_selector);
        }

        if (perspective_selector == nullptr) {
            godot::HBoxContainer *row = memnew(godot::HBoxContainer);
            _root_vbox->add_child(row);
            godot::Label *lbl = memnew(godot::Label);
            lbl->set_text("Perspective:");
            lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
            row->add_child(lbl);
            perspective_selector = memnew(godot::OptionButton);
            perspective_selector->set_name("Quick_Perspective_Selector");
            row->add_child(perspective_selector);

            if (swap_shoulder_button == nullptr) {
                swap_shoulder_button = memnew(godot::Button);
                swap_shoulder_button->set_name("Swap_Shoulder_Button");
                swap_shoulder_button->set_text("Swap Shoulder");
                row->add_child(swap_shoulder_button);
            }
        }

        if (difficulty_selector == nullptr) {
            godot::HBoxContainer *row = memnew(godot::HBoxContainer);
            _root_vbox->add_child(row);
            godot::Label *lbl = memnew(godot::Label);
            lbl->set_text("Difficulty:");
            lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
            row->add_child(lbl);
            difficulty_selector = memnew(godot::OptionButton);
            difficulty_selector->set_name("Quick_Difficulty_Selector");
            row->add_child(difficulty_selector);
        }
    }

    if (mode_selector && !mode_selector->is_connected("item_selected", godot::Callable(this, "_on_quick_mode_selected"))) {
        mode_selector->connect("item_selected", godot::Callable(this, "_on_quick_mode_selected"));
    }
    if (perspective_selector && !perspective_selector->is_connected("item_selected", godot::Callable(this, "_on_quick_perspective_selected"))) {
        perspective_selector->connect("item_selected", godot::Callable(this, "_on_quick_perspective_selected"));
    }
    if (swap_shoulder_button && !swap_shoulder_button->is_connected("pressed", godot::Callable(this, "swap_shoulder_pressed"))) {
        swap_shoulder_button->connect("pressed", godot::Callable(this, "swap_shoulder_pressed"));
    }
    if (difficulty_selector && !difficulty_selector->is_connected("item_selected", godot::Callable(this, "_on_quick_difficulty_selected"))) {
        difficulty_selector->connect("item_selected", godot::Callable(this, "_on_quick_difficulty_selected"));
    }
}

void GamePauseMenu::_validate_buttons() {
    auto ensure_btn = [this](godot::Button *&r_btn, const godot::String &p_node_name, const godot::String &p_label, const char *p_method) {
        if (r_btn == nullptr) {
            r_btn = godot::Object::cast_to<godot::Button>(find_child(p_node_name, true, false));
            if (r_btn == nullptr && auto_build_fallback_ui && _root_vbox != nullptr) {
                r_btn = memnew(godot::Button);
                r_btn->set_name(p_node_name);
                r_btn->set_text(p_label);
                _root_vbox->add_child(r_btn);
            }
        }
        if (r_btn != nullptr && !r_btn->is_connected("pressed", godot::Callable(this, p_method))) {
            r_btn->connect("pressed", godot::Callable(this, p_method));
        }
    };

    ensure_btn(resume_button, "Resume_Button", "Resume Game", "resume_pressed");
    ensure_btn(save_game_button, "Save_Game_Button", "Save Game", "save_game_pressed");
    ensure_btn(load_game_button, "Load_Game_Button", "Load Game", "load_game_pressed");
    ensure_btn(restart_board_button, "Restart_Board_Button", "Restart Board", "restart_board_pressed");
    ensure_btn(options_button, "Options_Button", "Gameplay Options", "options_pressed");
    ensure_btn(main_menu_button, "Main_Menu_Button", "Return to Main Menu", "main_menu_pressed");
    ensure_btn(quit_button, "Quit_Button", "Quit Game", "quit_pressed");

    // Adapt button visibility to Game configuration
    if (game != nullptr) {
        if (save_game_button != nullptr) {
            save_game_button->set_visible(game->get_save_options() != SaveOptions::NO_SAVE);
        }
        if (load_game_button != nullptr) {
            load_game_button->set_visible(game->get_load_options() != LoadOptions::NO_LOAD);
        }
    }
    if (restart_board_button != nullptr) {
        restart_board_button->set_visible(active_board != nullptr);
    }
    if (options_button != nullptr) {
        options_button->set_visible(options_menu != nullptr);
    }
}

void GamePauseMenu::refresh_pause_telemetry() {
    validate_pause_context();
    _updating_quick_switchers = true;

    // 1. Update Context Status Label (Board + Controlled Agent)
    if (context_status_label != nullptr) {
        godot::String board_str = (active_board && !active_board->get_title().is_empty())
            ? active_board->get_title()
            : (active_board ? active_board->get_name() : godot::String("No Active Board"));
        godot::String agent_str = (controlled_agent && !controlled_agent->get_title().is_empty())
            ? controlled_agent->get_title()
            : (controlled_agent ? controlled_agent->get_name() : godot::String("Unassigned Agent"));
        context_status_label->set_text("Board: " + board_str + "  |  Agent: " + agent_str);
    }

    // 2. Populate Quick Mode Selector
    godot::Ref<GameplayStyle> style = _resolve_active_style();
    if (mode_selector != nullptr) {
        int prev_sel = mode_selector->get_selected() >= 0 ? mode_selector->get_selected() : 0;
        mode_selector->clear();
        if (style.is_valid() && style->get_modes().size() > 0) {
            godot::TypedArray<GameplayMode> modes = style->get_modes();
            for (int i = 0; i < modes.size(); ++i) {
                GameplayMode *m = godot::Object::cast_to<GameplayMode>(modes[i]);
                godot::String title = (m && !m->get_title().is_empty())
                    ? m->get_title()
                    : ("Mode " + godot::String::num_int64(i + 1));
                mode_selector->add_item(title, i);
            }
            mode_selector->select(prev_sel < modes.size() ? prev_sel : 0);
            mode_selector->set_disabled(modes.size() <= 1);
        } else {
            mode_selector->add_item("Default Mode", 0);
            mode_selector->select(0);
            mode_selector->set_disabled(true);
        }
    }

    // 3. Populate Quick Perspective Selector & Swap Shoulder State
    godot::Ref<GameplayPerspective> persp = _resolve_active_perspective();
    if (perspective_selector != nullptr) {
        perspective_selector->clear();
        const char *names[] = {
            "Custom", "First Person", "Third Person OTS", "Third Person Orbit",
            "Top-Down", "3/4 View", "Isometric", "Dimetric", "Side-Scroller", "Fixed Cinematic"
        };
        for (int i = 0; i < 10; ++i) {
            perspective_selector->add_item(names[i], i);
        }
        if (persp.is_valid()) {
            perspective_selector->select(static_cast<int>(persp->get_perspective_type()));
            perspective_selector->set_disabled(false);
        } else {
            perspective_selector->select(2);
            perspective_selector->set_disabled(true);
        }
    }
    if (swap_shoulder_button != nullptr) {
        bool can_swap = persp.is_valid() && persp->get_allow_shoulder_swap() &&
                        persp->get_perspective_type() == PerspectiveType::PERSPECTIVE_THIRD_PERSON_OTS;
        swap_shoulder_button->set_disabled(!can_swap);
    }

    // 4. Populate Quick Difficulty Selector (respecting allow_mid_game_change)
    godot::Ref<GameplayDifficulty> diff = _resolve_active_difficulty();
    if (difficulty_selector != nullptr) {
        difficulty_selector->clear();
        const char *tiers[] = {"Story", "Easy", "Normal", "Hard", "Nightmare", "Custom"};
        for (int i = 0; i < 6; ++i) {
            difficulty_selector->add_item(tiers[i], i);
        }
        if (diff.is_valid()) {
            difficulty_selector->select(static_cast<int>(diff->get_overall_level()));
            difficulty_selector->set_disabled(!diff->get_allow_mid_game_change());
        } else {
            difficulty_selector->select(2);
            difficulty_selector->set_disabled(true);
        }
    }

    // 5. Summarize Active Tags & Cascade Rules Count
    if (active_tags_label != nullptr) {
        godot::String tags_text = "";
        auto append_tags = [&tags_text](const godot::Ref<GameplayResource> &p_res) {
            if (!p_res.is_valid()) return;
            godot::TypedArray<godot::StringName> tags = p_res->get_option_tags();
            for (int i = 0; i < tags.size(); ++i) {
                if (!tags_text.is_empty()) tags_text += " • ";
                tags_text += godot::String(tags[i]);
            }
        };
        append_tags(persp);
        append_tags(diff);
        append_tags(_resolve_active_control_map());

        int rule_count = style.is_valid() ? style->get_rule_count() : 0;
        if (tags_text.is_empty()) {
            tags_text = "Active Cascade Rules: " + godot::String::num_int64(rule_count);
        } else {
            tags_text += "  (" + godot::String::num_int64(rule_count) + " rules)";
        }
        active_tags_label->set_text(tags_text);
    }

    _updating_quick_switchers = false;
}

// --- 5. Pause Lifecycle & Button Actions ---

void GamePauseMenu::open_pause_menu() {
    validate_pause_context();
    refresh_pause_telemetry();

    if (game != nullptr && game->get_game_state() == GameState::PLAYING) {
        game->pause_game();
    }

    if (godot::Input *inp = godot::Input::get_singleton()) {
        inp->set_mouse_mode(godot::Input::MOUSE_MODE_VISIBLE);
    }

    show();
    emit_signal("opened");
}

void GamePauseMenu::close_pause_menu() {
    hide();

    godot::Ref<GameplayControlMap> cmap = _resolve_active_control_map();
    if (cmap.is_valid()) {
        cmap->apply_mouse_mode_to_engine();
    }

    emit_signal("closed");
}

void GamePauseMenu::toggle_pause_menu() {
    if (is_visible()) {
        resume_pressed();
    } else {
        open_pause_menu();
    }
}

void GamePauseMenu::_on_game_paused() {
    if (!is_visible()) {
        refresh_pause_telemetry();
        show();
        emit_signal("opened");
    }
}

void GamePauseMenu::_on_game_continued() {
    if (is_visible()) {
        close_pause_menu();
    }
}

void GamePauseMenu::_on_options_menu_closed() {
    refresh_pause_telemetry();
    show();
}

void GamePauseMenu::_on_quick_mode_selected(int p_index) {
    if (_updating_quick_switchers) return;

    godot::Ref<GameplayStyle> style = _resolve_active_style();
    if (!style.is_valid() || p_index < 0 || p_index >= style->get_modes().size()) {
        return;
    }

    godot::Ref<GameplayMode> selected_mode(godot::Object::cast_to<GameplayMode>(style->get_modes()[p_index]));
    if (options_menu != nullptr) {
        options_menu->set_active_mode_index(p_index);
    }
    _reapply_active_style_cascade();
    emit_signal("mode_switched", p_index, selected_mode);
}

void GamePauseMenu::_on_quick_perspective_selected(int p_index) {
    if (_updating_quick_switchers) return;

    godot::Ref<GameplayPerspective> persp = _resolve_active_perspective();
    if (!persp.is_valid()) return;

    persp->apply_perspective_preset(static_cast<PerspectiveType>(p_index));
    _reapply_active_style_cascade();
    emit_signal("perspective_switched", p_index);
}

void GamePauseMenu::_on_quick_difficulty_selected(int p_index) {
    if (_updating_quick_switchers) return;

    godot::Ref<GameplayDifficulty> diff = _resolve_active_difficulty();
    if (!diff.is_valid() || !diff->get_allow_mid_game_change()) return;

    diff->apply_overall_preset(static_cast<DifficultyLevel>(p_index));
    _reapply_active_style_cascade();
    emit_signal("difficulty_switched", p_index);
}

void GamePauseMenu::swap_shoulder_pressed() {
    godot::Ref<GameplayPerspective> persp = _resolve_active_perspective();
    if (!persp.is_valid() || !persp->get_allow_shoulder_swap()) return;

    persp->swap_shoulder();
    _reapply_active_style_cascade();
    emit_signal("shoulder_swapped");
}

void GamePauseMenu::resume_pressed() {
    if (game != nullptr) {
        game->pause_game();
    }
    close_pause_menu();
    emit_signal("resumed");
}

void GamePauseMenu::save_game_pressed() {
    if (game != nullptr) {
        game->save_game();
    }
}

void GamePauseMenu::load_game_pressed() {
    if (game != nullptr) {
        game->load_game(0);
        close_pause_menu();
    }
}

void GamePauseMenu::restart_board_pressed() {
    if (active_board != nullptr) {
        active_board->reset_game_board();
    }
    emit_signal("restart_board_requested");
    resume_pressed();
}

void GamePauseMenu::options_pressed() {
    emit_signal("options_requested");
    if (options_menu == nullptr) {
        return;
    }

    if (!options_menu->is_connected("closed", godot::Callable(this, "_on_options_menu_closed"))) {
        options_menu->connect("closed", godot::Callable(this, "_on_options_menu_closed"));
    }

    GameEntity *target_ent = nullptr;
    if (controlled_agent != nullptr) {
        target_ent = controlled_agent;
    } else if (active_board != nullptr) {
        target_ent = active_board;
    }

    if (target_ent != nullptr) {
        options_menu->open_options_for_entity(target_ent);
    } else {
        options_menu->open_options();
    }
}

void GamePauseMenu::main_menu_pressed() {
    emit_signal("return_to_menu_requested");
    close_pause_menu();
    if (game != nullptr) {
        game->reset_to_menu();
    }
    if (game_menu != nullptr) {
        game_menu->open_menu();
    }
}

void GamePauseMenu::quit_pressed() {
    emit_signal("quit_requested");
    if (game != nullptr) {
        game->quit_game();
    }
}

} // namespace ideam::godot_ext