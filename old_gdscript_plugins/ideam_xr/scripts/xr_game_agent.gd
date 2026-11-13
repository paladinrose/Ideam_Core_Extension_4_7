@tool
class_name XR_Game_Agent
extends Game_Agent

#region SIGNALS #################################################

signal match_height_started
signal match_height_completed

signal left_hand_controlled (left_hand : XRController3D)
signal right_hand_controlled (right_hand : XRController3D)

signal head_controlled (head : XRCamera3D)
signal eyes_controlled (eyes : XR_Eye_FX)
signal ears_controlled (ears : XR_Ear_FX)

#endregion

#region ENUMS ###################################################

#endregion

#region CONSTANTS ###############################################

#endregion

#region EXPORTS #################################################
@export var agent_root3D : Node3D

@export var agent_targets_anchor : Node3D
@export var agent_targets : Array[Node3D]
@export var agent_height : float = 1.7

@export var match_height : AgentMatching = AgentMatching.PLAYER_TO_AGENT:
	set(new_match):
		if new_match == match_height:
			return
		
		match_height = new_match
		
		_check_height_match()

@export var player_height_change_time : float = 0.1


@export_group("XR Head")
@export var head_assignments : Node_Retargeter
@export var head_control_anchor : Node3D
@export var head_controls : Array[Node3D]

@export var eye_effects : Array[XR_Eye_Effect]
@export var ear_effects : Array[XR_Ear_Effect]

@export_group("XR Left Hand")
@export var left_hand_assignments : Node_Retargeter
@export var left_hand_control_anchor : Node3D
@export var left_hand_controls : Array[XR_Control]

@export_group("XR Right Hand")
@export var right_hand_assignments : Node_Retargeter
@export var right_hand_control_anchor : Node3D
@export var right_hand_controls : Array[XR_Control]

#endregion

#region VARIABLES ###############################################

var _starting_position : Vector3
var _move_to : Vector3
var _average_position : Vector3
var _position_tween : Tween

var _starting_rotation : Quaternion
var _rotate_to : Quaternion
var _average_rotation : Quaternion
var _rotate_tween : Tween

var _starting_scale : float
var _scale_to : float
var _average_scale : float
var _scale_tween : Tween

var _starting_height : float
var _height_to : float
var _average_height : float
var _height_tween : Tween
var _height_match_complete : bool

#endregion

#region ONREADY VARIABLES #######################################

#endregion

#region GODOT FUNCTIONS #########################################

#func _process(delta : float) -> void:
#	pass


#endregion

#region CLASS FUNCTIONS #########################################

func start_height_match(player_to_match : Game_Player):
	_height_match_complete = false
	
	_starting_height = _started_to_control.height
	_height_to = _started_to_control.height / _started_to_control._initial_height.y * agent_height
	_average_height = (_starting_height + _height_to) * 0.5
	
	if match_height_started.get_connections().size() > 0:
		match_height_started.emit()
		
		return
	
	if player_rescale_time > 0:
		tween_started_to_match_height()
	else:
		player_height(1.0)


func tween_started_to_match_height():
	if _height_tween:
		_height_tween.kill()
	
	if _starting_height == _height_to:
		complete_height_match()
		return
	
	_height_tween = create_tween()
	_height_tween.tween_method(player_height,0.0,1.0,player_rescale_time)


func player_height(percent : float):
	match match_height:
		AgentMatching.PLAYER_TO_AGENT:
			var l : float = lerpf(_starting_height, _height_to, percent)
			_started_to_control.height = l
			
			var ps : Vector3 = Vector3.ONE * l
			_started_to_control.scale = ps
		
		AgentMatching.AGENT_TO_PLAYER:
			var l : float = lerpf(_height_to,_starting_height,  percent)
			var s : Vector3 = Vector3.ONE * l
			agent_root3D.scale = s
			
		AgentMatching.AVERAGE:
			var pl : float = lerpf(_starting_height,_average_height, percent)
			_started_to_control.height = pl
			
			var ps : Vector3 = Vector3.ONE * pl
			_started_to_control.scale = ps
			
			var l : float = lerpf(_height_to, _average_height,  percent)
			var s : Vector3 = Vector3.ONE * l
			agent_root3D.scale = s
	
	if percent >= 1.0:
		complete_height_match()


func complete_height_match():
	match_height_completed.emit()
	_height_match_complete = true
	_check_all_matches()


func _check_height_match():
	if match_height != AgentMatching.NONE:
		if not player_control_started.is_connected(start_height_match):
			player_control_started.connect(start_height_match)
	else:
		if player_control_started.is_connected(start_height_match):
			player_control_started.disconnect(start_height_match)


#endregion

#region STATIC FUNCTIONS ########################################

#endregion

#region FUNCTION OVERRIDES ######################################

func set_player(new_player):
	if new_player == _player:
		return
	if not new_player is XR_Player:
		return
	
	if _player:
		release_and_control(new_player)
	else:
		start_control(new_player)


func control(new_player : Game_Player):
	if _player == new_player:
		return
	
	if not new_player is XR_Player:
		return
		
	_player = new_player
	
	if player_node_assignments:
		player_node_assignments.retarget(self, _player)
	
	if head_assignments:
		head_assignments.retarget(self, _player.head)
	
	head_controlled.emit(_player.head)
	
	if _player.eyes:
		eyes_controlled.emit(_player.eyes)
		
	if _player.ears:
		ears_controlled.emit(_player.ears)
	
	if _player.left_hand:
		if left_hand_assignments:
			left_hand_assignments.retarget(left_hand_control_anchor, _player.left_hand)
		
		left_hand_controlled.emit(_player.left_hand)
	
	if _player.right_hand:
		if right_hand_assignments:
			right_hand_assignments.retarget(right_hand_control_anchor, _player.right_hand)
		
		right_hand_controlled.emit(_player.right_hand)
	
	player_controlled.emit(_player)


func control_end():
	var rel = _player
	
	if player_node_assignments:
		player_node_assignments.clear_set_targets(self, _player)
	
	if head_assignments:
		head_assignments.clear_set_targets(self, _player.head)
	
	if _player.left_hand and left_hand_assignments:
		left_hand_assignments.clear_set_targets(left_hand_control_anchor, _player.left_hand)
	
	if _player.right_hand and right_hand_assignments:
		right_hand_assignments.clear_set_targets(right_hand_control_anchor, _player.right_hand)
	
	
	_player = null
	
	player_released.emit(rel)
	
	rel.release_agent(self)
	
	if _waiting_to_control:
		start_control(_waiting_to_control)
		
		_waiting_to_control = null


##Begins the process of matching the player's position to the 
##agent's position.
func start_position_match(player_to_match : Game_Player):
	_position_match_complete = false
	
	_starting_position = _started_to_control.global_position
	_move_to = agent_root3D.global_position
	_average_position = (_starting_position + _move_to) * 0.5
	
	if match_position_started.get_connections().size() > 0:
		match_position_started.emit()
		
		return
	
	if player_reposition_time > 0:
		tween_started_to_match_position()
		
	else:
		player_position(1.0)


func tween_started_to_match_position():
	if _position_tween:
		_position_tween.kill()
	
	if _starting_position == _move_to:
		complete_position_match()
		return
	
	_position_tween = create_tween()
	_position_tween.tween_method(player_position, 0.0, 1.0, player_reposition_time)


func player_position(percent : float):
	match match_position:
		AgentMatching.PLAYER_TO_AGENT:
			print("Matching Player to Avatar")
			var pos : Vector3 = lerp(_starting_position, _move_to, percent)
			_started_to_control.global_position = pos
			
		AgentMatching.AGENT_TO_PLAYER:
			var pos : Vector3 = lerp(_move_to, _starting_position, percent)
			agent_root3D.global_position = pos
		
		AgentMatching.AVERAGE:
			var player_pos : Vector3 = lerp(_starting_position, _average_position, percent)
			var new_pos : Vector3 = lerp(_move_to, _average_position, percent)
			_started_to_control.global_position = player_pos
			agent_root3D.global_position = new_pos
			
	if percent >= 1.0:
		complete_position_match()


func complete_position_match():
	match_position_completed.emit()
	
	_position_match_complete = true
	_check_all_matches()


func start_rotation_match(player_to_match : Game_Player):
	_rotation_match_complete = false
	
	_starting_rotation = Quaternion(_started_to_control.global_basis)
	_rotate_to = Quaternion(agent_root3D.global_basis)
	_average_rotation = _starting_rotation.slerp(_rotate_to, 0.5)
	
	if match_rotation_started.get_connections().size() > 0:
		match_rotation_started.emit()
		
		return
	
	if player_reorient_time > 0:
		tween_started_to_match_rotation()
	else:
		player_rotate(1.0)


func tween_started_to_match_rotation():
	if _rotate_tween:
		_rotate_tween.kill()
	
	if _starting_rotation == _rotate_to:
		complete_rotation_match()
		return
	
	_rotate_tween = create_tween()
	_rotate_tween.tween_method(player_rotate, 0.0, 1.0, player_reorient_time)


func player_rotate(percent : float):
	match match_rotation:
		AgentMatching.PLAYER_TO_AGENT:
			var r = _starting_rotation.slerp(_rotate_to, percent)
			_started_to_control.rotation = r.get_euler()
			
		AgentMatching.AGENT_TO_PLAYER:
			var r = _rotate_to.slerp(_starting_rotation, percent)
			agent_root3D.global_basis = Basis(r)
		
		AgentMatching.AVERAGE:
			var pr = _starting_rotation.slerp(_average_rotation, percent)
			_started_to_control.global_basis = Basis(pr)
			
			var r = _rotate_to.slerp(_average_rotation, percent)
			agent_root3D.global_basis = Basis(r)
	
	if percent >= 1.0:
		complete_rotation_match()


func complete_rotation_match():
	match_rotation_completed.emit()
	_rotation_match_complete = true
	_check_all_matches()


func start_scale_match(player_to_match : Game_Player):
	_scale_match_complete = false
	
	_starting_scale = _started_to_control.scale.y
	_scale_to = agent_root3D.scale.y
	_average_scale = (_starting_scale + _scale_to) * 0.5
	
	if match_scale_started.get_connections().size() > 0:
		match_scale_started.emit()
		
		return
	
	if player_rescale_time > 0:
		tween_started_to_match_scale()
	else:
		player_scale(1.0)


func tween_started_to_match_scale():
	if _scale_tween:
		_scale_tween.kill()
	
	if _starting_scale == _scale_to:
		complete_scale_match()
		return
	
	_scale_tween = create_tween()
	_scale_tween.tween_method(player_scale,0.0,1.0,player_rescale_time)


func player_scale(percent : float):
	match match_scale:
		AgentMatching.PLAYER_TO_AGENT:
			var l : float = lerpf(_starting_scale, _scale_to, percent)
			var ps : Vector3 = Vector3.ONE * l
			_started_to_control.scale = ps
		
		AgentMatching.AGENT_TO_PLAYER:
			var l : float = lerpf(_scale_to,_starting_scale,  percent)
			var s : Vector3 = Vector3.ONE * l
			agent_root3D.scale = s
			
		AgentMatching.AVERAGE:
			var pl : float = lerpf(_starting_scale,_average_scale, percent)
			var ps : Vector3 = Vector3.ONE * pl
			_started_to_control.scale = ps
			
			var l : float = lerpf(_scale_to, _average_scale,  percent)
			var s : Vector3 = Vector3.ONE * l
			agent_root3D.scale = s
	
	if percent >= 1.0:
		complete_scale_match()


func complete_scale_match():
	match_scale_completed.emit()
	_scale_match_complete = true
	_check_all_matches()


func _check_all_matches():
	var p : bool = true
	if match_position != AgentMatching.NONE and not _position_match_complete:
		p = false
	
	var r : bool = true
	if match_rotation != AgentMatching.NONE and not _rotation_match_complete:
		r = false
	
	var s : bool = true
	if match_scale != AgentMatching.NONE and not _scale_match_complete:
		s = false
	
	var h : bool = true
	if match_height != AgentMatching.NONE and not _height_match_complete:
		h = false
	
	if p and r and s and h:
		continue_control()

#endregion

#region INTERNAL CLASSES ########################################

#endregion
