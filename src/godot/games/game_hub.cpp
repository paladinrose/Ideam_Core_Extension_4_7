#include "game_hub.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/xr_interface.hpp>
#include <godot_cpp/classes/xr_server.hpp>

#include "scene_transition.h"
#include "game_player.h"
#include "game.h"
#include "gameplay/gameplay_graphics_option.h"

namespace ideam::godot_ext {

void GameHub::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("game_not_found", godot::PropertyInfo(godot::Variant::INT, "game_id")));
    ADD_SIGNAL(godot::MethodInfo("load_game_started", godot::PropertyInfo(godot::Variant::INT, "game_id")));
    ADD_SIGNAL(godot::MethodInfo("load_game_canceled", godot::PropertyInfo(godot::Variant::INT, "game_id")));
    ADD_SIGNAL(godot::MethodInfo("load_game_completed", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_RESOURCE_TYPE, "Game")));
    ADD_SIGNAL(godot::MethodInfo("game_already_loaded", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_RESOURCE_TYPE, "Game")));
    ADD_SIGNAL(godot::MethodInfo("game_unloaded", godot::PropertyInfo(godot::Variant::OBJECT, "game", godot::PROPERTY_HINT_RESOURCE_TYPE, "Game")));

    // Section 2.4 XR_Session_Manager Lifecycle Signals
    ADD_SIGNAL(godot::MethodInfo("xr_started"));
    ADD_SIGNAL(godot::MethodInfo("xr_enabled"));
    ADD_SIGNAL(godot::MethodInfo("xr_failed"));
    ADD_SIGNAL(godot::MethodInfo("xr_disabled"));
    ADD_SIGNAL(godot::MethodInfo("xr_stopped"));
    ADD_SIGNAL(godot::MethodInfo("entered_immersive_mode"));
    ADD_SIGNAL(godot::MethodInfo("entered_windowed_mode"));
    ADD_SIGNAL(godot::MethodInfo("focus_lost"));
    ADD_SIGNAL(godot::MethodInfo("focus_gained"));
    ADD_SIGNAL(godot::MethodInfo("pose_recentered"));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_game_paths", "paths"), &GameHub::set_game_paths);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_paths"), &GameHub::get_game_paths);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_paths", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_game_paths", "get_game_paths");
    ADD_SIGNAL(godot::MethodInfo("game_paths_changed", godot::PropertyInfo(godot::Variant::ARRAY, "game_paths", godot::PROPERTY_HINT_ARRAY_TYPE, "String")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_titles", "titles"), &GameHub::set_game_titles);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_titles"), &GameHub::get_game_titles);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "game_titles", godot::PROPERTY_HINT_ARRAY_TYPE, "String"), "set_game_titles", "get_game_titles");
    ADD_SIGNAL(godot::MethodInfo("game_titles_changed", godot::PropertyInfo(godot::Variant::ARRAY, "game_titles", godot::PROPERTY_HINT_ARRAY_TYPE, "String")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_startup_game", "startup_game"), &GameHub::set_startup_game);
    godot::ClassDB::bind_method(godot::D_METHOD("get_startup_game"), &GameHub::get_startup_game);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "startup_game"), "set_startup_game", "get_startup_game");
    ADD_SIGNAL(godot::MethodInfo("startup_game_changed", godot::PropertyInfo(godot::Variant::INT, "startup_game")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_game_loader", "loader"), &GameHub::set_game_loader);
    godot::ClassDB::bind_method(godot::D_METHOD("get_game_loader"), &GameHub::get_game_loader);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "game_loader", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition"), "set_game_loader", "get_game_loader");
    ADD_SIGNAL(godot::MethodInfo("game_loader_changed", godot::PropertyInfo(godot::Variant::OBJECT, "game_loader", godot::PROPERTY_HINT_NODE_TYPE, "SceneTransition")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_hub_scene", "scene"), &GameHub::set_hub_scene);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hub_scene"), &GameHub::get_hub_scene);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "hub_scene", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_hub_scene", "get_hub_scene");
    ADD_SIGNAL(godot::MethodInfo("hub_scene_changed", godot::PropertyInfo(godot::Variant::OBJECT, "hub_scene", godot::PROPERTY_HINT_NODE_TYPE, "Node")));

    godot::ClassDB::bind_method(godot::D_METHOD("set_disable_hub_scene_on_game_load", "disable"), &GameHub::set_disable_hub_scene_on_game_load);
    godot::ClassDB::bind_method(godot::D_METHOD("get_disable_hub_scene_on_game_load"), &GameHub::get_disable_hub_scene_on_game_load);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "disable_hub_scene_on_game_load"), "set_disable_hub_scene_on_game_load", "get_disable_hub_scene_on_game_load");

    // Section 2.4 OpenXR Session Properties
    ADD_GROUP("OpenXR Session Management", "xr_");
    godot::ClassDB::bind_method(godot::D_METHOD("set_graphics_option", "option"), &GameHub::set_graphics_option);
    godot::ClassDB::bind_method(godot::D_METHOD("get_graphics_option"), &GameHub::get_graphics_option);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "graphics_option", godot::PROPERTY_HINT_RESOURCE_TYPE, "GameplayGraphicsOption"), "set_graphics_option", "get_graphics_option");

    godot::ClassDB::bind_method(godot::D_METHOD("set_xr_is_enabled", "enabled"), &GameHub::set_xr_is_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_xr_is_enabled"), &GameHub::get_xr_is_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "xr_is_enabled"), "set_xr_is_enabled", "get_xr_is_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("get_xr_is_started"), &GameHub::get_xr_is_started);
    godot::ClassDB::bind_method(godot::D_METHOD("get_xr_is_focussed"), &GameHub::get_xr_is_focussed);

    godot::ClassDB::bind_method(godot::D_METHOD("set_pause_games_on_xr_focus_lost", "pause"), &GameHub::set_pause_games_on_xr_focus_lost);
    godot::ClassDB::bind_method(godot::D_METHOD("get_pause_games_on_xr_focus_lost"), &GameHub::get_pause_games_on_xr_focus_lost);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "pause_games_on_xr_focus_lost"), "set_pause_games_on_xr_focus_lost", "get_pause_games_on_xr_focus_lost");

    godot::ClassDB::bind_method(godot::D_METHOD("get_loaded_games"), &GameHub::get_loaded_games);

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("start_xr"), &GameHub::start_xr);
    godot::ClassDB::bind_method(godot::D_METHOD("end_xr"), &GameHub::end_xr);
    godot::ClassDB::bind_method(godot::D_METHOD("switch_to_immersive_mode"), &GameHub::switch_to_immersive_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("switch_to_windowed_mode"), &GameHub::switch_to_windowed_mode);

    godot::ClassDB::bind_method(godot::D_METHOD("connect_to_player", "player"), &GameHub::connect_to_player);
    godot::ClassDB::bind_method(godot::D_METHOD("load_startup_game"), &GameHub::load_startup_game);
    godot::ClassDB::bind_method(godot::D_METHOD("load_game", "game_id"), &GameHub::load_game);
    godot::ClassDB::bind_method(godot::D_METHOD("game_load_complete", "game_node"), &GameHub::game_load_complete);
    godot::ClassDB::bind_method(godot::D_METHOD("game_load_fail"), &GameHub::game_load_fail);
    godot::ClassDB::bind_method(godot::D_METHOD("unload_game", "game_id"), &GameHub::unload_game);
    godot::ClassDB::bind_method(godot::D_METHOD("disable_hub_scene"), &GameHub::disable_hub_scene);
    godot::ClassDB::bind_method(godot::D_METHOD("enable_hub_scene"), &GameHub::enable_hub_scene);
    godot::ClassDB::bind_method(godot::D_METHOD("add_game", "gamePath"), &GameHub::add_game);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_game", "game_id"), &GameHub::remove_game);
    
    // Internal callback bindings
    godot::ClassDB::bind_method(godot::D_METHOD("_on_process_frame_startup"), &GameHub::_on_process_frame_startup);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_openxr_session_begun"), &GameHub::_on_openxr_session_begun);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_openxr_visible_state"), &GameHub::_on_openxr_visible_state);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_openxr_focused_state"), &GameHub::_on_openxr_focused_state);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_openxr_session_stopping"), &GameHub::_on_openxr_session_stopping);
    godot::ClassDB::bind_method(godot::D_METHOD("_on_openxr_pose_recentered"), &GameHub::_on_openxr_pose_recentered);
}

GameHub::GameHub() {}

GameHub::~GameHub() {}

void GameHub::_ready() {
    if (_is_ready || godot::Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    _is_ready = true;

    if (hub_scene) {
        hub_scene_parent = hub_scene->get_parent();
    }

    if (xr_is_enabled) {
        start_xr();
    }

    if (get_tree()) {
        get_tree()->connect("process_frame", godot::Callable(this, "_on_process_frame_startup"), godot::Object::CONNECT_ONE_SHOT);
    }
}

void GameHub::_on_process_frame_startup() {
    if (_game_loader) {
        _connect_to_game_loader();
    }
    load_startup_game();
}

void GameHub::_notification(int p_what) {
    if (p_what == NOTIFICATION_WM_CLOSE_REQUEST) {
        if (xr_is_started) {
            end_xr();
        }
        if (get_tree()) {
            get_tree()->quit();
        }
    }
}

// --- Section 2.4 OpenXR Session Management Implementation ---

void GameHub::set_graphics_option(const godot::Ref<GameplayGraphicsOption>& p_option) {
    graphics_option = p_option;
}
godot::Ref<GameplayGraphicsOption> GameHub::get_graphics_option() const { return graphics_option; }

void GameHub::set_xr_is_enabled(bool p_enabled) {
    if (xr_is_enabled == p_enabled) return;
    xr_is_enabled = p_enabled;
    if (!_is_ready) return;
    if (xr_is_enabled) {
        start_xr();
    } else {
        end_xr();
    }
}
bool GameHub::get_xr_is_enabled() const { return xr_is_enabled; }
bool GameHub::get_xr_is_started() const { return xr_is_started; }
bool GameHub::get_xr_is_focussed() const { return xr_is_focussed; }

void GameHub::set_pause_games_on_xr_focus_lost(bool p_pause) { pause_games_on_xr_focus_lost = p_pause; }
bool GameHub::get_pause_games_on_xr_focus_lost() const { return pause_games_on_xr_focus_lost; }

bool GameHub::start_xr() {
    if (!graphics_option.is_valid()) {
        graphics_option.instantiate();
        graphics_option->set_xr_enabled(true);
    }

    godot::Viewport* vp = get_viewport();
    bool ok = graphics_option->start_xr_session(vp);
    if (!ok) {
        xr_is_enabled = false;
        emit_signal("xr_failed");
        return false;
    }

    if (godot::XRServer* xrs = godot::XRServer::get_singleton()) {
        godot::Ref<godot::XRInterface> iface = xrs->find_interface("OpenXR");
        if (iface.is_valid() && !xr_is_started) {
            xr_is_started = true;
            if (!iface->is_connected("session_begun", godot::Callable(this, "_on_openxr_session_begun"))) {
                iface->connect("session_begun", godot::Callable(this, "_on_openxr_session_begun"));
            }
            if (!iface->is_connected("session_visible", godot::Callable(this, "_on_openxr_visible_state"))) {
                iface->connect("session_visible", godot::Callable(this, "_on_openxr_visible_state"));
            }
            if (!iface->is_connected("session_focussed", godot::Callable(this, "_on_openxr_focused_state"))) {
                iface->connect("session_focussed", godot::Callable(this, "_on_openxr_focused_state"));
            }
            if (!iface->is_connected("session_stopping", godot::Callable(this, "_on_openxr_session_stopping"))) {
                iface->connect("session_stopping", godot::Callable(this, "_on_openxr_session_stopping"));
            }
            if (!iface->is_connected("pose_recentered", godot::Callable(this, "_on_openxr_pose_recentered"))) {
                iface->connect("pose_recentered", godot::Callable(this, "_on_openxr_pose_recentered"));
            }
        }
    }

    xr_is_enabled = true;
    emit_signal("xr_enabled");
    return true;
}

void GameHub::end_xr() {
    if (graphics_option.is_valid()) {
        graphics_option->end_xr_session(get_viewport());
    }
    xr_is_enabled = false;
    xr_is_started = false;
    xr_is_focussed = false;
    emit_signal("xr_disabled");
}

bool GameHub::switch_to_immersive_mode() {
    bool ok = start_xr();
    if (ok) {
        emit_signal("entered_immersive_mode");
    }
    return ok;
}

bool GameHub::switch_to_windowed_mode() {
    end_xr();
    emit_signal("entered_windowed_mode");
    return true;
}

void GameHub::_on_openxr_session_begun() {
    if (graphics_option.is_valid()) {
        graphics_option->start_xr_session(get_viewport());
    }
    emit_signal("xr_started");
}

void GameHub::_on_openxr_visible_state() {
    if (xr_is_focussed) {
        xr_is_focussed = false;
        if (pause_games_on_xr_focus_lost) {
            godot::Array keys = loaded_games.keys();
            for (int i = 0; i < keys.size(); ++i) {
                if (Game* g = godot::Object::cast_to<Game>(loaded_games[keys[i]])) {
                    g->pause_game();
                }
            }
        }
        emit_signal("focus_lost");
    }
}

void GameHub::_on_openxr_focused_state() {
    if (!xr_is_focussed) {
        xr_is_focussed = true;
        if (pause_games_on_xr_focus_lost) {
            godot::Array keys = loaded_games.keys();
            for (int i = 0; i < keys.size(); ++i) {
                if (Game* g = godot::Object::cast_to<Game>(loaded_games[keys[i]])) {
                    g->pause_game();
                }
            }
        }
        emit_signal("focus_gained");
    }
}

void GameHub::_on_openxr_session_stopping() {
    xr_is_started = false;
    xr_is_focussed = false;
    emit_signal("xr_stopped");
}

void GameHub::_on_openxr_pose_recentered() {
    for (int i = 0; i < connected_players.size(); ++i) {
        if (GamePlayer* p = godot::Object::cast_to<GamePlayer>(connected_players[i])) {
            p->recenter_player();
        }
    }
    emit_signal("pose_recentered");
}

// Property Implementations
void GameHub::set_game_paths(const godot::TypedArray<godot::String>& p_paths) { 
    if (game_paths == p_paths) return;
    game_paths = p_paths; 
    emit_signal("game_paths_changed", game_paths);
}
godot::TypedArray<godot::String> GameHub::get_game_paths() const { return game_paths; }

void GameHub::set_game_titles(const godot::TypedArray<godot::String>& p_titles) { 
    if (game_titles == p_titles) return;
    game_titles = p_titles; 
    emit_signal("game_titles_changed", game_titles);
}
godot::TypedArray<godot::String> GameHub::get_game_titles() const { return game_titles; }

void GameHub::set_startup_game(int p_startup) { 
    if (startup_game == p_startup) return;
    startup_game = p_startup; 
    emit_signal("startup_game_changed", startup_game);
}
int GameHub::get_startup_game() const { return startup_game; }

void GameHub::set_game_loader(SceneTransition* p_loader) {
    if (p_loader == _game_loader) return;

    if (!_is_ready) {
        _game_loader = p_loader;
        return;
    }

    if (_game_loader) {
        _disconnect_from_game_loader();
    }

    _game_loader = p_loader;

    if (_game_loader) {
        _connect_to_game_loader();
    }
    emit_signal("game_loader_changed", _game_loader);
}
SceneTransition* GameHub::get_game_loader() const { return _game_loader; }

void GameHub::set_hub_scene(godot::Node* p_scene) { 
    if (hub_scene == p_scene) return;
    hub_scene = p_scene; 
    emit_signal("hub_scene_changed", hub_scene);
}
godot::Node* GameHub::get_hub_scene() const { return hub_scene; }

void GameHub::set_disable_hub_scene_on_game_load(bool p_disable) { 
    if (disable_hub_scene_on_game_load == p_disable) return;
    disable_hub_scene_on_game_load = p_disable; 
}
bool GameHub::get_disable_hub_scene_on_game_load() const { return disable_hub_scene_on_game_load; }

godot::Dictionary GameHub::get_loaded_games() const { 
    return loaded_games; 
}

// Class Methods
void GameHub::connect_to_player(GamePlayer* player) {
    if (!player) return;

    if (!connected_players.has(player)) {
        connected_players.append(player);
    }
    
    SceneTransition* transition = player->get_game_transition();
    if (transition) {
        if (_game_loader) {
            _disconnect_from_game_loader();
        }
        
        _game_loader = transition;
        _connect_to_game_loader();
    }
}

void GameHub::load_startup_game() {
    if (startup_game < 0 || startup_game >= game_paths.size()) {
        return;
    }
    load_game(startup_game);
}

void GameHub::load_game(int game_id) {
    if (loaded_games.has(game_id)) {
        godot::Object* loadedGameObj = loaded_games[game_id];
        if (loadedGameObj) {
            emit_signal("game_already_loaded", loadedGameObj);
            return;
        }
    }

    if (game_id >= game_paths.size()) {
        emit_signal("game_not_found", game_id);
        return;
    }

    if (!_game_loader) {
        if (!_default_game_loader) {
            _create_default_game_loader();
        }
        set_game_loader(_default_game_loader);
    }

    godot::String gamePath = game_paths[game_id];
    
    _game_loader->start_transition(gamePath);
    
    loading_games.append(gamePath);
    emit_signal("load_game_started", game_id);
}

void GameHub::game_load_complete(godot::Node* game_node) {
    Game* loadedGame = godot::Object::cast_to<Game>(game_node);
    if (!loadedGame) return;
    
    if (!_game_loader) {
        if (!_default_game_loader) {
            _create_default_game_loader();
        }
        set_game_loader(_default_game_loader);
    }

    godot::String gamePath = _game_loader->get_to_path();
    
    int path_index = loading_games.find(gamePath);
    if (path_index != -1) {
        loading_games.remove_at(path_index);
    }

    int game_id = game_paths.find(gamePath);
    loaded_games[game_id] = loadedGame;
    
    add_child(loadedGame);
    
    if (get_tree() && get_tree()->get_edited_scene_root()) {
        loadedGame->set_owner(get_tree()->get_edited_scene_root());
    }
    
    loadedGame->set_game_hub_ID(game_id);
    
    if (!loadedGame->is_connected("game_ended", godot::Callable(this, "unload_game"))) {
        loadedGame->connect("game_ended", godot::Callable(this, "unload_game"));
    }
    
    if (disable_hub_scene_on_game_load && hub_scene) {
        disable_hub_scene();
    }
        
    loadedGame->game_loaded_from_game_hub();
    emit_signal("load_game_completed", loadedGame);
    
    _game_loader->complete_transition();
}

void GameHub::game_load_fail() {}

void GameHub::unload_game(int game_id) {
    if (!loaded_games.has(game_id)) return;
    
    Game* loadedGame = godot::Object::cast_to<Game>(loaded_games[game_id]);
    if (!loadedGame) return;
    
    if (loadedGame->get_close_hub_on_quit()) {
        if (get_tree() && get_tree()->get_root()) {
            get_tree()->get_root()->propagate_notification(NOTIFICATION_WM_CLOSE_REQUEST);
        }
    } else {
        emit_signal("game_unloaded", loadedGame);
        
        godot::Node* target_scene = nullptr;
        if (get_tree()) {
            target_scene = get_tree()->get_current_scene();
        }
        
        if (target_scene) {
            target_scene->remove_child(loadedGame);
        }
        
        loaded_games.erase(game_id);
        loadedGame->queue_free();
    }
}

void GameHub::disable_hub_scene() {
    if (hub_scene_disabled || !hub_scene_parent || !hub_scene) return;

    hub_scene_parent->remove_child(hub_scene);
    hub_scene_disabled = true;
}

void GameHub::enable_hub_scene() {
    if (!hub_scene_disabled || !hub_scene_parent || !hub_scene) return;

    hub_scene_parent->add_child(hub_scene);
    hub_scene_disabled = false;
}

int GameHub::add_game(const godot::String& gamePath) {
    if (godot::ResourceLoader::get_singleton()->exists(gamePath)) {
        if (!game_paths.has(gamePath)) {
            game_paths.append(gamePath);
            return game_paths.size() - 1;
        }
    }
    return -1;
}

void GameHub::remove_game(int game_id) {
    if (game_id >= 0 && game_id < game_paths.size()) {
        game_paths.remove_at(game_id);
    }
}

void GameHub::_create_default_game_loader() {
    _default_game_loader = memnew(SceneTransition);
    add_child(_default_game_loader);
}

void GameHub::_connect_to_game_loader() {
    if (!_game_loader) return;
    
    if (!_game_loader->is_connected("load_to_scene_completed", godot::Callable(this, "game_load_complete"))) {
        _game_loader->connect("load_to_scene_completed", godot::Callable(this, "game_load_complete"));
    }
    if (!_game_loader->is_connected("load_to_scene_failed", godot::Callable(this, "game_load_fail"))) {
        _game_loader->connect("load_to_scene_failed", godot::Callable(this, "game_load_fail"));
    }
}

void GameHub::_disconnect_from_game_loader() {
    if (!_game_loader) return;
    
    if (_game_loader->is_connected("load_to_scene_completed", godot::Callable(this, "game_load_complete"))) {
        _game_loader->disconnect("load_to_scene_completed", godot::Callable(this, "game_load_complete"));
    }
    if (_game_loader->is_connected("load_to_scene_failed", godot::Callable(this, "game_load_fail"))) {
        _game_loader->disconnect("load_to_scene_failed", godot::Callable(this, "game_load_fail"));
    }
}

} // namespace ideam::godot_ext