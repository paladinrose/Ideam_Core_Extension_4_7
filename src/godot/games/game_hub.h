#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class SceneTransition;
class GamePlayer;
class Game;
class GameplayGraphicsOption;

class GameHub : public godot::Node {
    GDCLASS(GameHub, godot::Node)

protected:
    static void _bind_methods();
    void _notification(int p_what);

private:
    godot::TypedArray<godot::String> game_paths;
    godot::TypedArray<godot::String> game_titles;
    
    int startup_game = -1;

    SceneTransition* _game_loader = nullptr;
    SceneTransition* _default_game_loader = nullptr;

    godot::Node* hub_scene = nullptr;
    bool disable_hub_scene_on_game_load = false;

    godot::Dictionary loaded_games;
    
    godot::Node* hub_scene_parent = nullptr;
    bool hub_scene_disabled = false;
    godot::TypedArray<godot::String> loading_games;

    bool _is_ready = false;

    // --- Section 2.4 XR_Session_Manager Merger: OpenXR Session Lifecycle & Focus Routing ---
    godot::Ref<GameplayGraphicsOption> graphics_option;
    godot::TypedArray<GamePlayer> connected_players;
    bool xr_is_enabled = false;
    bool xr_is_started = false;
    bool xr_is_focussed = false;
    bool pause_games_on_xr_focus_lost = true;

    // Internal deferred startup & OpenXR signal callbacks
    void _on_process_frame_startup();
    void _on_openxr_session_begun();
    void _on_openxr_visible_state();
    void _on_openxr_focused_state();
    void _on_openxr_session_stopping();
    void _on_openxr_pose_recentered();

public:
    GameHub();
    ~GameHub();

    virtual void _ready() override;

    // Setters / Getters
    godot::Dictionary get_loaded_games() const;

    void set_game_paths(const godot::TypedArray<godot::String>& p_paths);
    godot::TypedArray<godot::String> get_game_paths() const;

    void set_game_titles(const godot::TypedArray<godot::String>& p_titles);
    godot::TypedArray<godot::String> get_game_titles() const;

    void set_startup_game(int p_startup);
    int get_startup_game() const;

    void set_game_loader(SceneTransition* p_loader);
    SceneTransition* get_game_loader() const;

    void set_hub_scene(godot::Node* p_scene);
    godot::Node* get_hub_scene() const;

    void set_disable_hub_scene_on_game_load(bool p_disable);
    bool get_disable_hub_scene_on_game_load() const;

    // --- Section 2.4 OpenXR Session Lifecycle API (ported from `xr_session_manager.gd`) ---
    void set_graphics_option(const godot::Ref<GameplayGraphicsOption>& p_option);
    godot::Ref<GameplayGraphicsOption> get_graphics_option() const;

    void set_xr_is_enabled(bool p_enabled);
    bool get_xr_is_enabled() const;
    bool get_xr_is_started() const;
    bool get_xr_is_focussed() const;

    void set_pause_games_on_xr_focus_lost(bool p_pause);
    bool get_pause_games_on_xr_focus_lost() const;

    bool start_xr();
    void end_xr();
    bool switch_to_immersive_mode();
    bool switch_to_windowed_mode();

    // Class Functions
    void connect_to_player(GamePlayer* player);
    void load_startup_game();
    void load_game(int game_id);
    void game_load_complete(godot::Node* game_node);
    void game_load_fail();
    void unload_game(int game_id);
    
    void disable_hub_scene();
    void enable_hub_scene();
    
    int add_game(const godot::String& gamePath);
    void remove_game(int game_id);

    // Internal Helpers
    void _create_default_game_loader();
    void _connect_to_game_loader();
    void _disconnect_from_game_loader();
};

} // namespace ideam::godot_ext