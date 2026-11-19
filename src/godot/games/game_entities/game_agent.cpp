#include "game_agent.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include "game_piece.h"
#include "actions/game_agent_action.h"
#include "actions/game_interaction.h"
#include "../game.h"
#include "../game_player.h"
#include "../game_property.h"
#include "../gameplay/gameplay_style.h"
#include "../gameplay/xr_eye_effect_resource.h"
#include "../../utilities/node_retargeter.h"
#include "../../utilities/signal_connector.h"


namespace ideam::godot_ext {

void GameAgent::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("action_added", godot::PropertyInfo(godot::Variant::OBJECT, "action", godot::PROPERTY_HINT_NODE_TYPE, "GameAgentAction")));
    ADD_SIGNAL(godot::MethodInfo("action_removed", godot::PropertyInfo(godot::Variant::OBJECT, "action", godot::PROPERTY_HINT_NODE_TYPE, "GameAgentAction")));
    ADD_SIGNAL(godot::MethodInfo("game_piece_added", godot::PropertyInfo(godot::Variant::OBJECT, "new_piece", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));
    ADD_SIGNAL(godot::MethodInfo("game_piece_removed", godot::PropertyInfo(godot::Variant::OBJECT, "removed", godot::PROPERTY_HINT_NODE_TYPE, "GamePiece")));
    
    ADD_SIGNAL(godot::MethodInfo("player_control_started", godot::PropertyInfo(godot::Variant::OBJECT, "by", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));
    ADD_SIGNAL(godot::MethodInfo("player_controlled", godot::PropertyInfo(godot::Variant::OBJECT, "by", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));
    ADD_SIGNAL(godot::MethodInfo("player_control_release_started", godot::PropertyInfo(godot::Variant::OBJECT, "by", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));
    ADD_SIGNAL(godot::MethodInfo("player_released", godot::PropertyInfo(godot::Variant::OBJECT, "by", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer")));

    ADD_SIGNAL(godot::MethodInfo("match_position_started"));
    ADD_SIGNAL(godot::MethodInfo("match_position_completed"));
    ADD_SIGNAL(godot::MethodInfo("match_rotation_started"));
    ADD_SIGNAL(godot::MethodInfo("match_rotation_completed"));
    ADD_SIGNAL(godot::MethodInfo("match_scale_started"));
    ADD_SIGNAL(godot::MethodInfo("match_scale_completed"));
    ADD_SIGNAL(godot::MethodInfo("match_height_started"));
    ADD_SIGNAL(godot::MethodInfo("match_height_completed"));

    ADD_SIGNAL(godot::MethodInfo("head_controlled", godot::PropertyInfo(godot::Variant::OBJECT, "head", godot::PROPERTY_HINT_NODE_TYPE, "Node3D")));
    ADD_SIGNAL(godot::MethodInfo("eyes_controlled", godot::PropertyInfo(godot::Variant::OBJECT, "eyes", godot::PROPERTY_HINT_NODE_TYPE, "XREyeFXNode3D")));
    ADD_SIGNAL(godot::MethodInfo("left_hand_controlled", godot::PropertyInfo(godot::Variant::OBJECT, "left_hand", godot::PROPERTY_HINT_NODE_TYPE, "Node3D")));
    ADD_SIGNAL(godot::MethodInfo("right_hand_controlled", godot::PropertyInfo(godot::Variant::OBJECT, "right_hand", godot::PROPERTY_HINT_NODE_TYPE, "Node3D")));

    // Enums
    BIND_ENUM_CONSTANT(AgentExclusivity::NONE);
    BIND_ENUM_CONSTANT(AgentExclusivity::EXCLUSIVE);
    BIND_ENUM_CONSTANT(AgentExclusivity::GROUP_INCLUSIVE);
    BIND_ENUM_CONSTANT(AgentExclusivity::GROUP_EXCLUSIVE);

    BIND_ENUM_CONSTANT(AgentMatching::MATCH_NONE);
    BIND_ENUM_CONSTANT(AgentMatching::PLAYER_TO_AGENT);
    BIND_ENUM_CONSTANT(AgentMatching::AGENT_TO_PLAYER);
    BIND_ENUM_CONSTANT(AgentMatching::AVERAGE);

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_player", "new_player"), &GameAgent::set_player);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player"), &GameAgent::get_player);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "player", godot::PROPERTY_HINT_NODE_TYPE, "GamePlayer"), "set_player", "get_player");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_parent_target", "target"), &GameAgent::set_player_parent_target);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_parent_target"), &GameAgent::get_player_parent_target);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "player_parent_target", godot::PROPERTY_HINT_NODE_TYPE, "Node"), "set_player_parent_target", "get_player_parent_target");

    godot::ClassDB::bind_method(godot::D_METHOD("set_exclusivity", "exclusivity"), &GameAgent::set_exclusivity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_exclusivity"), &GameAgent::get_exclusivity);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "exclusivity", godot::PROPERTY_HINT_ENUM, "None,Exclusive,Group Inclusive,Group Exclusive"), "set_exclusivity", "get_exclusivity");

    godot::ClassDB::bind_method(godot::D_METHOD("set_match_position", "match_mode"), &GameAgent::set_match_position);
    godot::ClassDB::bind_method(godot::D_METHOD("get_match_position"), &GameAgent::get_match_position);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "match_position", godot::PROPERTY_HINT_ENUM, "Match None,Player To Agent,Agent To Player,Average"), "set_match_position", "get_match_position");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_reposition_time", "time"), &GameAgent::set_player_reposition_time);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_reposition_time"), &GameAgent::get_player_reposition_time);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "player_reposition_time"), "set_player_reposition_time", "get_player_reposition_time");

    godot::ClassDB::bind_method(godot::D_METHOD("set_match_rotation", "match_mode"), &GameAgent::set_match_rotation);
    godot::ClassDB::bind_method(godot::D_METHOD("get_match_rotation"), &GameAgent::get_match_rotation);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "match_rotation", godot::PROPERTY_HINT_ENUM, "Match None,Player To Agent,Agent To Player,Average"), "set_match_rotation", "get_match_rotation");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_reorient_time", "time"), &GameAgent::set_player_reorient_time);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_reorient_time"), &GameAgent::get_player_reorient_time);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "player_reorient_time"), "set_player_reorient_time", "get_player_reorient_time");

    godot::ClassDB::bind_method(godot::D_METHOD("set_match_scale", "match_mode"), &GameAgent::set_match_scale);
    godot::ClassDB::bind_method(godot::D_METHOD("get_match_scale"), &GameAgent::get_match_scale);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "match_scale", godot::PROPERTY_HINT_ENUM, "Match None,Player To Agent,Agent To Player,Average"), "set_match_scale", "get_match_scale");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_rescale_time", "time"), &GameAgent::set_player_rescale_time);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_rescale_time"), &GameAgent::get_player_rescale_time);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "player_rescale_time"), "set_player_rescale_time", "get_player_rescale_time");

    // --- Section 2.4 XR_Game_Agent Properties ---
    ADD_GROUP("XR Embodiment & Height Matching", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_agent_height", "height"), &GameAgent::set_agent_height);
    godot::ClassDB::bind_method(godot::D_METHOD("get_agent_height"), &GameAgent::get_agent_height);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "agent_height", godot::PROPERTY_HINT_RANGE, "0.2,4.0,0.01,suffix:m"), "set_agent_height", "get_agent_height");

    godot::ClassDB::bind_method(godot::D_METHOD("set_match_height", "match_mode"), &GameAgent::set_match_height);
    godot::ClassDB::bind_method(godot::D_METHOD("get_match_height"), &GameAgent::get_match_height);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "match_height", godot::PROPERTY_HINT_ENUM, "Match None,Player To Agent,Agent To Player,Average"), "set_match_height", "get_match_height");

    godot::ClassDB::bind_method(godot::D_METHOD("set_player_height_change_time", "time"), &GameAgent::set_player_height_change_time);
    godot::ClassDB::bind_method(godot::D_METHOD("get_player_height_change_time"), &GameAgent::get_player_height_change_time);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "player_height_change_time", godot::PROPERTY_HINT_RANGE, "0.0,10.0,0.01,suffix:s"), "set_player_height_change_time", "get_player_height_change_time");

    godot::ClassDB::bind_method(godot::D_METHOD("set_head_assignments", "assignments"), &GameAgent::set_head_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_head_assignments"), &GameAgent::get_head_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "head_assignments", godot::PROPERTY_HINT_RESOURCE_TYPE, "NodeRetargeter"), "set_head_assignments", "get_head_assignments");

    godot::ClassDB::bind_method(godot::D_METHOD("set_head_control_anchor", "anchor"), &GameAgent::set_head_control_anchor);
    godot::ClassDB::bind_method(godot::D_METHOD("get_head_control_anchor"), &GameAgent::get_head_control_anchor);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "head_control_anchor", godot::PROPERTY_HINT_NODE_TYPE, "Node3D"), "set_head_control_anchor", "get_head_control_anchor");

    godot::ClassDB::bind_method(godot::D_METHOD("set_eye_effects", "effects"), &GameAgent::set_eye_effects);
    godot::ClassDB::bind_method(godot::D_METHOD("get_eye_effects"), &GameAgent::get_eye_effects);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "eye_effects", godot::PROPERTY_HINT_ARRAY_TYPE, "XREyeEffectResource"), "set_eye_effects", "get_eye_effects");

    godot::ClassDB::bind_method(godot::D_METHOD("set_left_hand_assignments", "assignments"), &GameAgent::set_left_hand_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_left_hand_assignments"), &GameAgent::get_left_hand_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "left_hand_assignments", godot::PROPERTY_HINT_RESOURCE_TYPE, "NodeRetargeter"), "set_left_hand_assignments", "get_left_hand_assignments");

    godot::ClassDB::bind_method(godot::D_METHOD("set_left_hand_control_anchor", "anchor"), &GameAgent::set_left_hand_control_anchor);
    godot::ClassDB::bind_method(godot::D_METHOD("get_left_hand_control_anchor"), &GameAgent::get_left_hand_control_anchor);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "left_hand_control_anchor", godot::PROPERTY_HINT_NODE_TYPE, "Node3D"), "set_left_hand_control_anchor", "get_left_hand_control_anchor");

    godot::ClassDB::bind_method(godot::D_METHOD("set_right_hand_assignments", "assignments"), &GameAgent::set_right_hand_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_right_hand_assignments"), &GameAgent::get_right_hand_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "right_hand_assignments", godot::PROPERTY_HINT_RESOURCE_TYPE, "NodeRetargeter"), "set_right_hand_assignments", "get_right_hand_assignments");

    godot::ClassDB::bind_method(godot::D_METHOD("set_right_hand_control_anchor", "anchor"), &GameAgent::set_right_hand_control_anchor);
    godot::ClassDB::bind_method(godot::D_METHOD("get_right_hand_control_anchor"), &GameAgent::get_right_hand_control_anchor);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "right_hand_control_anchor", godot::PROPERTY_HINT_NODE_TYPE, "Node3D"), "set_right_hand_control_anchor", "get_right_hand_control_anchor");

    // Standard methods mapped from header
    godot::ClassDB::bind_method(godot::D_METHOD("start_position_match", "player_to_match"), &GameAgent::start_position_match, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("_position_match", "percent"), &GameAgent::_position_match);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_position_match"), &GameAgent::complete_position_match);

    godot::ClassDB::bind_method(godot::D_METHOD("start_rotation_match", "player_to_match"), &GameAgent::start_rotation_match, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("_rotation_match", "percent"), &GameAgent::_rotation_match);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_rotation_match"), &GameAgent::complete_rotation_match);

    godot::ClassDB::bind_method(godot::D_METHOD("start_scale_match", "player_to_match"), &GameAgent::start_scale_match, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("_scale_match", "percent"), &GameAgent::_scale_match);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_scale_match"), &GameAgent::complete_scale_match);

    godot::ClassDB::bind_method(godot::D_METHOD("start_height_match", "player_to_match"), &GameAgent::start_height_match, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("_height_match", "percent"), &GameAgent::_height_match);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_height_match"), &GameAgent::complete_height_match);

    godot::ClassDB::bind_method(godot::D_METHOD("find_best_action", "instigating_action"), &GameAgent::find_best_action);
    godot::ClassDB::bind_method(godot::D_METHOD("handle_agent_request", "game_piece"), &GameAgent::handle_agent_request);
    godot::ClassDB::bind_method(godot::D_METHOD("targeted_action", "action_id", "target"), &GameAgent::targeted_action);
}

GameAgent::GameAgent() {}

GameAgent::~GameAgent() {}

// Setters / Getters
void GameAgent::set_player(GamePlayer* p_player) {
    if (p_player == _player) return;
    if (_player) {
        release_and_control(p_player);
    } else {
        start_control(p_player);
    }
}
GamePlayer* GameAgent::get_player() const { return _player; }

void GameAgent::set_player_parent_target(godot::Node* p_target) { 
    if (p_target == player_parent_target) return;
    player_parent_target = p_target; 
}
godot::Node* GameAgent::get_player_parent_target() const { return player_parent_target; }

void GameAgent::set_exclusivity(AgentExclusivity p_exclusivity) { 
    if (p_exclusivity == exclusivity) return;
    exclusivity = p_exclusivity; 
}
AgentExclusivity GameAgent::get_exclusivity() const { return exclusivity; }

void GameAgent::set_match_position(AgentMatching p_match) {
    if (p_match == match_position) return;
    match_position = p_match;
    _check_position_match();
}
AgentMatching GameAgent::get_match_position() const { return match_position; }

void GameAgent::set_player_reposition_time(float p_time) { 
    if (p_time == player_reposition_time) return;
    player_reposition_time = p_time; 
}
float GameAgent::get_player_reposition_time() const { return player_reposition_time; }

void GameAgent::set_match_rotation(AgentMatching p_match) {
    if (p_match == match_rotation) return;
    match_rotation = p_match;
    _check_rotation_match();
}
AgentMatching GameAgent::get_match_rotation() const { return match_rotation; }

void GameAgent::set_player_reorient_time(float p_time) { 
    if (p_time == player_reorient_time) return;
    player_reorient_time = p_time; 
}
float GameAgent::get_player_reorient_time() const { return player_reorient_time; }

void GameAgent::set_match_scale(AgentMatching p_match) {
    if (p_match == match_scale) return;
    match_scale = p_match;
    _check_scale_match();
}
AgentMatching GameAgent::get_match_scale() const { return match_scale; }

void GameAgent::set_player_rescale_time(float p_time) { 
    if (p_time == player_rescale_time) return;
    player_rescale_time = p_time; 
}
float GameAgent::get_player_rescale_time() const { return player_rescale_time; }

// --- Section 2.4 XR_Game_Agent Setters / Getters ---

void GameAgent::set_agent_height(float p_height) {
    float clamped = godot::Math::clamp(p_height, 0.2f, 4.0f);
    if (godot::Math::is_equal_approx(agent_height, clamped)) return;
    agent_height = clamped;
}
float GameAgent::get_agent_height() const { return agent_height; }

void GameAgent::set_match_height(AgentMatching p_match) {
    if (p_match == match_height) return;
    match_height = p_match;
    _check_height_match();
}
AgentMatching GameAgent::get_match_height() const { return match_height; }

void GameAgent::set_player_height_change_time(float p_time) {
    float clamped = godot::Math::max(0.0f, p_time);
    if (godot::Math::is_equal_approx(player_height_change_time, clamped)) return;
    player_height_change_time = clamped;
}
float GameAgent::get_player_height_change_time() const { return player_height_change_time; }

void GameAgent::set_head_assignments(const godot::Ref<NodeRetargeter>& p_assignments) { head_assignments = p_assignments; }
godot::Ref<NodeRetargeter> GameAgent::get_head_assignments() const { return head_assignments; }

void GameAgent::set_head_control_anchor(godot::Node3D* p_anchor) { head_control_anchor = p_anchor; }
godot::Node3D* GameAgent::get_head_control_anchor() const { return head_control_anchor; }

void GameAgent::set_eye_effects(const godot::TypedArray<XREyeEffectResource>& p_effects) { eye_effects = p_effects; }
godot::TypedArray<XREyeEffectResource> GameAgent::get_eye_effects() const { return eye_effects; }

void GameAgent::set_left_hand_assignments(const godot::Ref<NodeRetargeter>& p_assignments) { left_hand_assignments = p_assignments; }
godot::Ref<NodeRetargeter> GameAgent::get_left_hand_assignments() const { return left_hand_assignments; }

void GameAgent::set_left_hand_control_anchor(godot::Node3D* p_anchor) { left_hand_control_anchor = p_anchor; }
godot::Node3D* GameAgent::get_left_hand_control_anchor() const { return left_hand_control_anchor; }

void GameAgent::set_right_hand_assignments(const godot::Ref<NodeRetargeter>& p_assignments) { right_hand_assignments = p_assignments; }
godot::Ref<NodeRetargeter> GameAgent::get_right_hand_assignments() const { return right_hand_assignments; }

void GameAgent::set_right_hand_control_anchor(godot::Node3D* p_anchor) { right_hand_control_anchor = p_anchor; }
godot::Node3D* GameAgent::get_right_hand_control_anchor() const { return right_hand_control_anchor; }

void GameAgent::set_actions(const godot::TypedArray<GameAgentAction>& p_actions) { 
    if (p_actions == actions) return;
    actions = p_actions; 
}
godot::TypedArray<GameAgentAction> GameAgent::get_actions() const { return actions; }

void GameAgent::set_game_pieces(const godot::TypedArray<GamePiece>& p_pieces) { 
    if (p_pieces == game_pieces) return;
    game_pieces = p_pieces; 
}
godot::TypedArray<GamePiece> GameAgent::get_game_pieces() const { return game_pieces; }

void GameAgent::set_player_node_assignments(const godot::Ref<NodeRetargeter>& p_assignments) { 
    if (p_assignments == player_node_assignments) return;
    player_node_assignments = p_assignments; 
}
godot::Ref<NodeRetargeter> GameAgent::get_player_node_assignments() const { return player_node_assignments; }

void GameAgent::set_player_signal_assignments(const godot::Ref<SignalConnector>& p_assignments) { 
    if (p_assignments == player_signal_assignments) return;
    player_signal_assignments = p_assignments; 
}
godot::Ref<SignalConnector> GameAgent::get_player_signal_assignments() const { return player_signal_assignments; }


// Player Functions
void GameAgent::find_and_possess_agent(godot::Variant on) {
    if (!_player) return;
    
    godot::Object* obj = on.operator godot::Object*();
    GameAgent* other = godot::Object::cast_to<GameAgent>(obj);
    
    if (other && other != this) {
        _player->control_game_agent(other);
    }
}

void GameAgent::find_and_swap_agent(godot::Variant on) {
    if (!_player) return;
    
    godot::Object* obj = on.operator godot::Object*();
    GameAgent* other = godot::Object::cast_to<GameAgent>(obj);
    
    if (other && other != this) {
        _player->control_game_agent(other);
        _player->release_game_agent(this);
    }
}

void GameAgent::start_control(GamePlayer* new_player) {
    if (_player == new_player) return;

    _started_to_control = new_player;

    _check_position_match();
    _check_rotation_match();
    _check_scale_match();
    _check_height_match();

    if (has_connections("player_control_started")) {
        emit_signal("player_control_started", new_player);
        return;
    }

    control(new_player);
}

void GameAgent::continue_control() {
    control(_started_to_control);
}

void GameAgent::control(GamePlayer* new_player) {
    if (_player == new_player) return;
    _player = new_player;

    if (player_node_assignments.is_valid()) player_node_assignments->retarget(this, _player);
    if (player_signal_assignments.is_valid()) player_signal_assignments->connect_signals(this, _player);

    if (_player) {
        if (head_assignments.is_valid() && _player->get_xr_head()) {
            head_assignments->retarget(this, _player->get_xr_head());
            emit_signal("head_controlled", _player->get_xr_head());
        }
        if (_player->get_xr_eyes()) {
            for (int i = 0; i < eye_effects.size(); ++i) {
                godot::Ref<XREyeEffectResource> fx = eye_effects[i];
                if (fx.is_valid()) {
                    _player->get_xr_eyes()->add_effect(fx);
                }
            }
            emit_signal("eyes_controlled", _player->get_xr_eyes());
        }
        if (left_hand_assignments.is_valid() && _player->get_xr_left_hand()) {
            left_hand_assignments->retarget(this, _player->get_xr_left_hand());
            emit_signal("left_hand_controlled", _player->get_xr_left_hand());
        }
        if (right_hand_assignments.is_valid() && _player->get_xr_right_hand()) {
            right_hand_assignments->retarget(this, _player->get_xr_right_hand());
            emit_signal("right_hand_controlled", _player->get_xr_right_hand());
        }
    }

    emit_signal("player_controlled", _player);
}

void GameAgent::release_and_control(GamePlayer* to_control) {
    _waiting_to_control = to_control;
    start_release();
}

void GameAgent::start_release() {
    if (has_connections("player_control_release_started")) {
        emit_signal("player_control_release_started");
        return;
    }
    control_end();
}

void GameAgent::control_end() {
    GamePlayer* rel = _player;
    
    if (player_node_assignments.is_valid()) player_node_assignments->clear_set_targets(this, _player);
    if (player_signal_assignments.is_valid()) player_signal_assignments->disconnect_signals(this, _player);

    if (rel) {
        if (head_assignments.is_valid() && rel->get_xr_head()) {
            head_assignments->clear_set_targets(this, rel->get_xr_head());
        }
        if (rel->get_xr_eyes()) {
            for (int i = 0; i < eye_effects.size(); ++i) {
                godot::Ref<XREyeEffectResource> fx = eye_effects[i];
                if (fx.is_valid()) {
                    rel->get_xr_eyes()->remove_effect(fx);
                }
            }
        }
        if (left_hand_assignments.is_valid() && rel->get_xr_left_hand()) {
            left_hand_assignments->clear_set_targets(this, rel->get_xr_left_hand());
        }
        if (right_hand_assignments.is_valid() && rel->get_xr_right_hand()) {
            right_hand_assignments->clear_set_targets(this, rel->get_xr_right_hand());
        }
    }
    
    _player = nullptr;
    emit_signal("player_released", rel);
    
    if (rel) rel->release_game_agent(this);
    
    if (_waiting_to_control) {
        start_control(_waiting_to_control);
        _waiting_to_control = nullptr;
    }
}

// Matching Functions
void GameAgent::start_position_match(GamePlayer* player_to_match) {
    _position_match_complete = false;
    if (has_connections("match_position_started")) {
        emit_signal("match_position_started");
        return;
    }
    _position_match(1.0f);
    complete_position_match();
}
void GameAgent::_position_match(float percent) { }
void GameAgent::complete_position_match() {
    _position_match_complete = true;
    emit_signal("match_position_completed");
    _check_all_matches();
}

void GameAgent::start_rotation_match(GamePlayer* player_to_match) {
    _rotation_match_complete = false;
    if (has_connections("match_rotation_started")) {
        emit_signal("match_rotation_started");
        return;
    }
    _rotation_match(1.0f);
    complete_rotation_match();
}
void GameAgent::_rotation_match(float percent) { }
void GameAgent::complete_rotation_match() {
    _rotation_match_complete = true;
    emit_signal("match_rotation_completed");
    _check_all_matches();
}

void GameAgent::start_scale_match(GamePlayer* player_to_match) {
    _scale_match_complete = false;
    if (has_connections("match_scale_started")) {
        emit_signal("match_scale_started");
        return;
    }
    _scale_match(1.0f);
    complete_scale_match();
}
void GameAgent::_scale_match(float percent) { }
void GameAgent::complete_scale_match() {
    _scale_match_complete = true;
    emit_signal("match_scale_completed");
    _check_all_matches();
}

void GameAgent::start_height_match(GamePlayer* player_to_match) {
    _height_match_complete = false;
    if (has_connections("match_height_started")) {
        emit_signal("match_height_started");
        return;
    }
    _height_match(1.0f);
    complete_height_match();
}

void GameAgent::_height_match(float percent) {
    GamePlayer* target_player = _started_to_control ? _started_to_control : _player;
    if (!target_player) return;

    const float alpha = godot::Math::clamp(percent, 0.0f, 1.0f);
    const float p_h = target_player->get_current_height();
    const float a_h = agent_height;

    if (match_height == AgentMatching::PLAYER_TO_AGENT) {
        target_player->set_current_height(p_h + (a_h - p_h) * alpha);
    } else if (match_height == AgentMatching::AGENT_TO_PLAYER) {
        set_agent_height(a_h + (p_h - a_h) * alpha);
    } else if (match_height == AgentMatching::AVERAGE) {
        const float mid = 0.5f * (p_h + a_h);
        target_player->set_current_height(p_h + (mid - p_h) * alpha);
        set_agent_height(a_h + (mid - a_h) * alpha);
    }
}

void GameAgent::complete_height_match() {
    _height_match_complete = true;
    emit_signal("match_height_completed");
    _check_all_matches();
}

void GameAgent::_check_position_match() {
    if (match_position != AgentMatching::MATCH_NONE) {
        if (!is_connected("player_control_started", godot::Callable(this, "start_position_match"))) {
            connect("player_control_started", godot::Callable(this, "start_position_match"));
        }
    } else {
        if (is_connected("player_control_started", godot::Callable(this, "start_position_match"))) {
            disconnect("player_control_started", godot::Callable(this, "start_position_match"));
        }
    }
}

void GameAgent::_check_rotation_match() {
    if (match_rotation != AgentMatching::MATCH_NONE) {
        if (!is_connected("player_control_started", godot::Callable(this, "start_rotation_match"))) {
            connect("player_control_started", godot::Callable(this, "start_rotation_match"));
        }
    } else {
        if (is_connected("player_control_started", godot::Callable(this, "start_rotation_match"))) {
            disconnect("player_control_started", godot::Callable(this, "start_rotation_match"));
        }
    }
}

void GameAgent::_check_scale_match() {
    if (match_scale != AgentMatching::MATCH_NONE) {
        if (!is_connected("player_control_started", godot::Callable(this, "start_scale_match"))) {
            connect("player_control_started", godot::Callable(this, "start_scale_match"));
        }
    } else {
        if (is_connected("player_control_started", godot::Callable(this, "start_scale_match"))) {
            disconnect("player_control_started", godot::Callable(this, "start_scale_match"));
        }
    }
}

void GameAgent::_check_height_match() {
    if (match_height != AgentMatching::MATCH_NONE) {
        if (!is_connected("player_control_started", godot::Callable(this, "start_height_match"))) {
            connect("player_control_started", godot::Callable(this, "start_height_match"));
        }
    } else {
        if (is_connected("player_control_started", godot::Callable(this, "start_height_match"))) {
            disconnect("player_control_started", godot::Callable(this, "start_height_match"));
        }
    }
}

void GameAgent::_check_all_matches() {
    bool p = true;
    if (match_position != AgentMatching::MATCH_NONE && !_position_match_complete) p = false;
    
    bool r = true;
    if (match_rotation != AgentMatching::MATCH_NONE && !_rotation_match_complete) r = false;
    
    bool s = true;
    if (match_scale != AgentMatching::MATCH_NONE && !_scale_match_complete) s = false;

    bool h = true;
    if (match_height != AgentMatching::MATCH_NONE && !_height_match_complete) h = false;
    
    if (p && r && s && h) {
        continue_control();
    }
}

// Action Functions
godot::TypedArray<godot::String> GameAgent::gather_action_titles() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* act = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (act) names.append(act->get_name());
    }
    return names;
}

int GameAgent::add_action(GameAgentAction* new_action) {
    if (!new_action) return -1;
    int id = get_action_id(new_action);
    
    if (id < 0) {
        id = actions.size();
        actions.append(new_action);
        if (_resolved_gameplay_style.is_valid()) {
            new_action->apply_gameplay_style(_resolved_gameplay_style);
        }
        emit_signal("action_added", new_action);
    }
    return id;
}

int GameAgent::get_action_id_from_title(const godot::String& title) const {
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* act = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (act && act->get_name() == title) return i;
    }
    return -1;
}

int GameAgent::get_action_id(GameAgentAction* action) const {
    return actions.find(action);
}

bool GameAgent::has_action(GameAgentAction* action) const {
    return get_action_id(action) >= 0;
}

bool GameAgent::remove_action(GameAgentAction* action) {
    int id = get_action_id(action);
    if (id >= 0) return remove_action_at(id);
    return false;
}

bool GameAgent::remove_action_at(int action_ID) {
    if (action_ID < 0 || action_ID >= actions.size()) return false;
    
    GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_ID]);
    actions.remove_at(action_ID);
    
    if (action) emit_signal("action_removed", action);
    return true;
}

godot::Array GameAgent::find_best_action(GameAgentAction* instigating_action) const {
    godot::Array result;
    int action_id = -1;
    int highest_score = -1;
    bool competing = false;

    if (!instigating_action) {
        result.push_back(action_id);
        result.push_back(competing);
        return result;
    }

    godot::TypedArray<godot::StringName> inst_groups = instigating_action->get_groups();

    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* act = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (!act || !act->get_enabled()) continue;

        bool skip = false;
        int act_score = 0;
        bool act_competing = false;

        godot::TypedArray<GamePiece> act_pieces;
        for (int j = 0; j < act_pieces.size(); ++j) {
            GamePiece* p = godot::Object::cast_to<GamePiece>(act_pieces[j]);
            if (!p || !p->get_enabled()) {
                skip = true;
                break;
            }
            
            godot::TypedArray<GameProperty> props;
            for(int k = 0; k < props.size(); ++k) {
                GameProperty* prop = godot::Object::cast_to<GameProperty>(props[k]);
                if (prop) {
                    if (prop->get_locked()) act_score -= 1;
                }
            }
        }

        if (skip) continue;

        for (int g = 0; g < inst_groups.size(); ++g) {
            if (act->is_in_group(inst_groups[g])) {
                act_score += 1;
            }
        }

        if (act_score > highest_score) {
            action_id = i;
            highest_score = act_score;
            competing = act_competing;
        }
    }

    result.push_back(action_id);
    result.push_back(competing);
    return result;
}

void GameAgent::start_action(int action_id, int target_id) {
    if (!get_enabled() || action_id < 0 || action_id >= actions.size()) return;
    
    if (target_id >= 0 && target_id < game_pieces.size()) {
        GamePiece* target = godot::Object::cast_to<GamePiece>(game_pieces[target_id]);
        targeted_action(action_id, target);
        return;
    }
    
    GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
    if (action && !action->get_in_progress()) {
        action->start_action();
    }
}

void GameAgent::targeted_action(int action_id, GamePiece* target) {
    if (!get_enabled() || action_id < 0 || action_id >= actions.size() || !target) return;
    
    GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
    if (action) {
        if (!current_action_targets.has(target)) {
            current_action_targets.append(target);
        }
        if (!action->get_in_progress()) {
            action->start_action();
        }
    }
}

void GameAgent::end_action(int action_id) {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) action->end_action();
    }
}

void GameAgent::interrupt_action(int action_id) {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) action->interrupt_action();
    }
}

void GameAgent::fail_action(int action_id, int margin_of_failure) {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) action->fail_action(margin_of_failure);
    }
}

void GameAgent::complete_action(int action_id, int margin_of_victory) {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) action->complete_action(margin_of_victory);
    }
}

int GameAgent::get_action_value(int action_id) const {
    if (action_id >= 0 && action_id < actions.size()) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[action_id]);
        if (action && action->get_in_progress()) {
            // return action->get_current_value();
        }
    }
    return 0;
}

void GameAgent::apply_action_value_property(const godot::NodePath& to, int action_id, const godot::String& property_path) {
    if (has_node(to)) {
        godot::Node* target = get_node<godot::Node>(to);
        int v = get_action_value(action_id);
        if (target) target->set(property_path, v);
    }
}

void GameAgent::apply_action_value_method(const godot::NodePath& to, int action_id, const godot::String& method_name) {
    if (has_node(to)) {
        godot::Node* target = get_node<godot::Node>(to);
        int v = get_action_value(action_id);
        if (target) target->call(method_name, v);
    }
}

GameInteraction* GameAgent::interact(int action_id) {
    for (int i = 0; i < interactions.size(); ++i) {
        GameInteraction* inter = godot::Object::cast_to<GameInteraction>(interactions[i]);
        if (inter) {
            godot::Array instigator = inter->get_instigator();
            if (instigator.size() > 1 && static_cast<int>(instigator[1]) == action_id) {
                return inter;
            }
        }
    }
    return nullptr;
}

void GameAgent::join_interaction(GameInteraction* interaction) {
    if (!interaction) return;
    
    godot::Array instigator = interaction->get_instigator();
    if (instigator.size() < 2) return;
    
    GameAgent* inst_agent = godot::Object::cast_to<GameAgent>(instigator[0]);
    if (!inst_agent) return;
    
    godot::TypedArray<GameAgentAction> inst_actions = inst_agent->get_actions();
    int inst_action_id = instigator[1];
    
    if (inst_action_id >= 0 && inst_action_id < inst_actions.size()) {
        GameAgentAction* inst_action = godot::Object::cast_to<GameAgentAction>(inst_actions[inst_action_id]);
        
        godot::Array do_act = find_best_action(inst_action);
        int action_id = do_act[0];
        
        if (action_id >= 0) {
            godot::Array participation;
            participation.push_back(this);
            participation.push_back(action_id);
        }
    }
}

void GameAgent::leave_interaction(GameInteraction* interaction) {}

void GameAgent::stop_interacting(int action_id) {
    for (int i = interactions.size() - 1; i >= 0; --i) {
        GameInteraction* inter = godot::Object::cast_to<GameInteraction>(interactions[i]);
        if (inter) {
            godot::Array instigator = inter->get_instigator();
            if (instigator.size() > 1 && static_cast<int>(instigator[1]) == action_id) {
                interactions.remove_at(i);
                return;
            }
        }
    }
}

void GameAgent::action_consequences(int score, const godot::Dictionary& consequences) {
    GameEntity::action_consequences(score, consequences);
    
    if (consequences.has("current_action_targets")) {
        godot::Dictionary targets_dict = consequences["current_action_targets"];
        for (int i = 0; i < current_action_targets.size(); ++i) {
            GameEntity* a = godot::Object::cast_to<GameEntity>(current_action_targets[i]);
            if (a) a->action_consequences(score, targets_dict);
        }
    }
    
    if (consequences.has("add_action")) {
        godot::TypedArray<GameAgentAction> actions_to_add = consequences["add_action"];
        for (int i = 0; i < actions_to_add.size(); ++i) {
            add_action(godot::Object::cast_to<GameAgentAction>(actions_to_add[i]));
        }
    }
    
    if (consequences.has("remove_action")) {
        godot::TypedArray<GameAgentAction> actions_to_remove = consequences["remove_action"];
        for (int i = 0; i < actions_to_remove.size(); ++i) {
            remove_action(godot::Object::cast_to<GameAgentAction>(actions_to_remove[i]));
        }
    }
    
    if (consequences.has("actions")) {
        godot::Dictionary actions_dict = consequences["actions"];
        for (int i = 0; i < actions.size(); ++i) {
            GameAgentAction* act = godot::Object::cast_to<GameAgentAction>(actions[i]);
            if (act) act->action_consequences(score, actions_dict);
        }
    }
    
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) {
            godot::String action_setting = "action_" + action->get_name();
            if (consequences.has(action_setting)) {
                action->action_consequences(score, consequences[action_setting]);
            }
        }
    }
    
    if (consequences.has("add_game_piece")) {
        godot::TypedArray<GamePiece> pieces_to_add = consequences["add_game_piece"];
        for (int i = 0; i < pieces_to_add.size(); ++i) {
            add_game_piece(godot::Object::cast_to<GamePiece>(pieces_to_add[i]));
        }
    }
    
    if (consequences.has("remove_game_piece")) {
        godot::TypedArray<GamePiece> pieces_to_remove = consequences["remove_game_piece"];
        for (int i = 0; i < pieces_to_remove.size(); ++i) {
            remove_game_piece(godot::Object::cast_to<GamePiece>(pieces_to_remove[i]));
        }
    }
    
    if (consequences.has("game_pieces")) {
        godot::Dictionary pieces_dict = consequences["game_pieces"];
        for (int i = 0; i < game_pieces.size(); ++i) {
            GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
            if (piece) piece->action_consequences(score, pieces_dict);
        }
    }
    
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) {
            godot::String piece_setting = "game_piece_" + piece->get_name();
            if (consequences.has(piece_setting)) {
                piece->action_consequences(score, consequences[piece_setting]);
            }
        }
    }
}

// Game Piece Functions
godot::TypedArray<godot::String> GameAgent::gather_game_piece_titles() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) names.append(piece->get_name());
    }
    return names;
}

int GameAgent::add_game_piece(GamePiece* new_game_piece) {
    if (!new_game_piece) return -1;
    int id = get_game_piece_id(new_game_piece);
    
    if (id < 0) {
        id = game_pieces.size();
        game_pieces.append(new_game_piece);
        if (_resolved_gameplay_style.is_valid()) {
            new_game_piece->apply_gameplay_style(_resolved_gameplay_style);
        }
        emit_signal("game_piece_added", new_game_piece);
        
        if (!new_game_piece->is_connected("agent_requested", godot::Callable(this, "handle_agent_request"))) {
            new_game_piece->connect("agent_requested", godot::Callable(this, "handle_agent_request"));
        }
    }
    return id;
}

int GameAgent::get_game_piece_id(GamePiece* game_piece) const {
    return game_pieces.find(game_piece);
}

bool GameAgent::has_game_piece(GamePiece* game_piece) const {
    return get_game_piece_id(game_piece) >= 0;
}

bool GameAgent::remove_game_piece(GamePiece* game_piece) {
    int id = get_game_piece_id(game_piece);
    if (id >= 0) return remove_game_piece_at(id);
    return false;
}

bool GameAgent::remove_game_piece_at(int game_piece_ID) {
    if (game_piece_ID < 0 || game_piece_ID >= game_pieces.size()) return false;
    
    GamePiece* game_piece = godot::Object::cast_to<GamePiece>(game_pieces[game_piece_ID]);
    if (game_piece) {
        if (game_piece->is_connected("agent_requested", godot::Callable(this, "handle_agent_request"))) {
            game_piece->disconnect("agent_requested", godot::Callable(this, "handle_agent_request"));
        }
    }
    
    game_pieces.remove_at(game_piece_ID);
    if (game_piece) emit_signal("game_piece_removed", game_piece);
    
    return true;
}

void GameAgent::handle_agent_request(GamePiece* game_piece) {
    (void)game_piece;
}

// GameEntity Overrides
void GameAgent::enter_game() {
    GameEntity::enter_game();
}

void GameAgent::game_start() {
    GameEntity::game_start();
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_start();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_start();
    }
}

void GameAgent::game_pause() {
    if (entity_is_paused) return;
    GameEntity::game_pause();
    
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_pause();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_pause();
    }
}

void GameAgent::game_continue() {
    if (!entity_is_paused) return;
    GameEntity::game_continue();
    
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_continue();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_continue();
    }
}

void GameAgent::game_end() {
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_end();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_end();
    }
    GameEntity::game_end();
}

void GameAgent::exit_game() {
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->exit_game();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->exit_game();
    }
    GameEntity::exit_game();
}

void GameAgent::apply_gameplay_style(const godot::Ref<GameplayStyle>& newStyle) {
    GameEntity::apply_gameplay_style(newStyle);

    if (_resolved_gameplay_style.is_valid()) {
        for (int i = 0; i < actions.size(); ++i) {
            if (GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i])) {
                action->apply_gameplay_style(_resolved_gameplay_style);
            }
        }
        for (int i = 0; i < game_pieces.size(); ++i) {
            if (GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i])) {
                piece->apply_gameplay_style(_resolved_gameplay_style);
            }
        }
    }
}

void GameAgent::game_process(double delta) {
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_process(delta);
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_process(delta);
    }
}

void GameAgent::game_process_clear() {
    for (int i = 0; i < actions.size(); ++i) {
        GameAgentAction* action = godot::Object::cast_to<GameAgentAction>(actions[i]);
        if (action) action->game_process_clear();
    }
    for (int i = 0; i < game_pieces.size(); ++i) {
        GamePiece* piece = godot::Object::cast_to<GamePiece>(game_pieces[i]);
        if (piece) piece->game_process_clear();
    }
}

godot::Dictionary GameAgent::save_data() const {
    godot::Dictionary data = GameEntity::save_data();
    data["agent_height"] = agent_height;
    data["match_height"] = static_cast<int>(match_height);
    data["player_height_change_time"] = player_height_change_time;
    return data;
}

void GameAgent::load_data(const godot::Dictionary& data) {
    GameEntity::load_data(data);
    if (data.has("agent_height")) set_agent_height(static_cast<float>(data["agent_height"]));
    if (data.has("match_height")) set_match_height(static_cast<AgentMatching>(static_cast<int>(data["match_height"])));
    if (data.has("player_height_change_time")) set_player_height_change_time(static_cast<float>(data["player_height_change_time"]));
}

} // namespace ideam::godot_ext