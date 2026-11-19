#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/xr_camera3d.hpp>
#include <godot_cpp/classes/xr_controller3d.hpp>
#include <godot_cpp/classes/xr_origin3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "game_player_manager.h"
#include "game_player_profile.h"
#include "game.h"
#include "game_entities/game_board.h"
#include "game_entities/game_agent.h"
#include "gameplay/xr_eye_effect_resource.h"

#include "ui/game_menu.h"

#include "scene_transition.h"

#include "../utilities/node_retargeter.h"
#include "../utilities/signal_connector.h"

namespace ideam::godot_ext {

class GamePlayer : public godot::Node {
    GDCLASS(GamePlayer, godot::Node)

protected:
    static void _bind_methods();

private:
    godot::Ref<GamePlayerProfile> player_profile;
    
    godot::Node* _root = nullptr;
    godot::String player_agent_name = "Player_Agent";
    bool allow_agent_reparenting = true;
    bool join_game_on_load = true;

    SceneTransition* game_transition = nullptr;
    SceneTransition* board_transition = nullptr;
    GameAgent* default_agent = nullptr;
    
    godot::Ref<NodeRetargeter> agent_node_assignments;
    godot::Ref<SignalConnector> agent_signal_assignments;

    // --- Section 2.4 XR_Player Merger: Optional 6DoF XR Rig & Per-Limb Retargeters ---
    godot::XROrigin3D* xr_origin = nullptr;
    godot::XRCamera3D* xr_head = nullptr;
    XREyeFXNode3D* xr_eyes = nullptr;
    godot::Node3D* xr_ears = nullptr;
    godot::XRController3D* xr_left_hand = nullptr;
    godot::XRController3D* xr_right_hand = nullptr;

    godot::Ref<NodeRetargeter> head_assignments;
    godot::Ref<NodeRetargeter> eye_assignments;
    godot::Ref<NodeRetargeter> ear_assignments;
    godot::Ref<NodeRetargeter> left_hand_assignments;
    godot::Ref<NodeRetargeter> right_hand_assignments;

    float player_height = 1.7f;
    float current_height = 1.7f;

    // DOD NOTE: Storing Node pointers in TypedArray causes severe cache thrashing when iterating 
    // over these collections to evaluate exclusivity rules. In a DOD paradigm, `controlled_agents` 
    // should be replaced by a dense `std::vector<uint32_t>` containing purely structural IDs. 
    godot::TypedArray<GameAgent> controlled_agents;
    godot::TypedArray<Game> games;
    godot::TypedArray<GameMenu> gameMenus;

    godot::String profile_path;
    godot::Node* _original_parent = nullptr;

    // Internal async callback
    void _on_process_frame_find_game();

public:
    GamePlayer();
    ~GamePlayer();

    virtual void _ready() override;

    // Setters / Getters
    void set_player_profile(const godot::Ref<GamePlayerProfile>& p_profile);
    godot::Ref<GamePlayerProfile> get_player_profile() const;

    void set_player_root(godot::Node* p_root);
    godot::Node* get_player_root() const;

    void set_player_agent_name(const godot::String& p_name);
    godot::String get_player_agent_name() const;

    void set_allow_agent_reparenting(bool p_allow);
    bool get_allow_agent_reparenting() const;

    void set_join_game_on_load(bool p_join);
    bool get_join_game_on_load() const;

    void set_game_transition(SceneTransition* p_transition);
    SceneTransition* get_game_transition() const;

    void set_board_transition(SceneTransition* p_transition);
    SceneTransition* get_board_transition() const;

    void set_default_agent(GameAgent* p_agent);
    GameAgent* get_default_agent() const;

    void set_agent_node_assignments(const godot::Ref<NodeRetargeter>& p_assignments);
    godot::Ref<NodeRetargeter> get_agent_node_assignments() const;

    void set_agent_signal_assignments(const godot::Ref<SignalConnector>& p_assignments);
    godot::Ref<SignalConnector> get_agent_signal_assignments() const;

    // --- Section 2.4 XR Embodiment Setters / Getters & Helpers ---
    void set_xr_origin(godot::XROrigin3D* p_origin);
    godot::XROrigin3D* get_xr_origin() const;

    void set_xr_head(godot::XRCamera3D* p_head);
    godot::XRCamera3D* get_xr_head() const;

    void set_xr_eyes(XREyeFXNode3D* p_eyes);
    XREyeFXNode3D* get_xr_eyes() const;

    void set_xr_ears(godot::Node3D* p_ears);
    godot::Node3D* get_xr_ears() const;

    void set_xr_left_hand(godot::XRController3D* p_hand);
    godot::XRController3D* get_xr_left_hand() const;

    void set_xr_right_hand(godot::XRController3D* p_hand);
    godot::XRController3D* get_xr_right_hand() const;

    void set_head_assignments(const godot::Ref<NodeRetargeter>& p_assignments);
    godot::Ref<NodeRetargeter> get_head_assignments() const;

    void set_eye_assignments(const godot::Ref<NodeRetargeter>& p_assignments);
    godot::Ref<NodeRetargeter> get_eye_assignments() const;

    void set_ear_assignments(const godot::Ref<NodeRetargeter>& p_assignments);
    godot::Ref<NodeRetargeter> get_ear_assignments() const;

    void set_left_hand_assignments(const godot::Ref<NodeRetargeter>& p_assignments);
    godot::Ref<NodeRetargeter> get_left_hand_assignments() const;

    void set_right_hand_assignments(const godot::Ref<NodeRetargeter>& p_assignments);
    godot::Ref<NodeRetargeter> get_right_hand_assignments() const;

    void set_player_height(float p_height);
    float get_player_height() const;

    void set_current_height(float p_height);
    float get_current_height() const;

    /**
     * Pitch-compensated horizontal body forward estimation (ported from `xr_player.gd::_estimate_body_forward_dir()`).
     * Slides `camera_basis.y` onto the horizontal player plane when HMD elevation exceeds `abs(dot) > 0.75`.
     */
    godot::Vector3 estimate_body_forward_dir() const;

    /**
     * Recenters the 3D player root horizontally onto the current HMD XZ offset relative to `xr_origin`.
     */
    void recenter_player();

    // Class Functions
    void find_game_player_manager(godot::Node* on = nullptr);
    void set_game_player_manager(GamePlayerManager* gpm);
    void find_current_game();
    
    void login();
    void logout();
    
    void join_game(Game* game);
    int get_game_id(Game* game) const;
    void leave_game(int gameID);
    
    void open_game_menu();
    void close_game_menu();
    
    void load_game_board_started(Game* game, int game_board_id);
    void load_game_board_completed(Game* game, GameBoard* game_board);
    
    void load_game_started(int game_id);
    void load_game_completed(Game* game);

    // Agent Functions
    void find_and_control_player_agent(godot::Node* on);
    void find_and_control_game_agents(godot::Node* on);
    void find_and_release_game_agents(godot::Node* on);
    
    void control_game_agent(GameAgent* new_agent);
    void release_game_agent(GameAgent* toRelease);
    void release_game_agent_at(int id);
    void reparent_to_agent(GameAgent* agent);
    void validate_agents();

    void _connect_to_agent(GameAgent* agent);
    void _disconnect_from_agent(GameAgent* agent);
    
    godot::Dictionary save_data() const;
    void load_data(const godot::Dictionary& data);
};

} // namespace ideam::godot_ext