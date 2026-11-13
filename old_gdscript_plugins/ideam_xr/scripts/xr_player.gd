class_name XR_Player
extends Game_Player


#region SIGNALS #################################################
signal joined_session

signal left_hand_control_added (control : XR_Control)
signal left_hand_control_removed (control : XR_Control)

signal right_hand_control_added (control : XR_Control)
signal right_hand_control_removed (control : XR_Control)

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var session_manager : XR_Session_Manager

@export var origin : XROrigin3D


@export_group("XR Head")
## Required for this XR_Player to be valid and working. The head is 
## also a key component for possessed XR_Game_Agents.
@export var head : XRCamera3D

@export var head_assignments : Node_Retargeter

## Optional. Allows for rendering screen shaders and effects to
## each eye, separately, and both eyes together. XR_Game_Agents can
## apply effects or overrides to effects, when they're possessed.
## These effects are removed when the avatar is released.
@export var eyes : XR_Eye_FX

@export var eye_assignments : Node_Retargeter

## Optional. Allows for audio bus manipulation and effects with
## a special emphasis on XR uses. XR_Game_Agents can
## apply effects or overrides to effects, when they're possessed.
## These effects are removed when the avatar is released.
@export var ears : XR_Ear_FX

@export var ear_assignments : Node_Retargeter

@export_group("XR Hands")

## Optional The controller used for the left hand. XR_Game_Agents can
## connect XR_Controls to it when possessed. These controls are
## released when the XR_Game_Agent is released.
@export var left_hand : XRController3D

@export var left_hand_assignments : Node_Retargeter

## Optional The controller used for the right hand. XR_Game_Agents can
## connect XR_Controls to it when possessed. These controls are
## released when the XR_Game_Agent is released.
@export var right_hand : XRController3D

@export var right_hand_assignments : Node_Retargeter

#endregion


#region VARIABLES ###############################################

var height : float = 0.0

var current_height : float = 0.0

var _initial_scale : Vector3

var _possessed_avatars : Array[XR_Game_Agent] = []

var _root3D : Node3D

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	_original_parent = get_parent()
	
	_find_origin_head_and_hands()
	
	_find_session_manager()
	
	

#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################

func recenter_player():
	var current_forward : Vector3 = _estimate_body_forward_dir()
	#This is where I am: I need to figure out what my desired forward is,
	#and how to rotate from current to that. 
	
	var xz_pos : Vector3 = head.global_position - origin.global_position
	xz_pos.y = 0
	
	_root3D.global_position -= xz_pos 


func set_height():
	_initial_scale = _root3D.scale
	height = head.position.y
	current_height = height


## Adapted, with gratitude, from the Godot XRTools player_body.gd script
func _estimate_body_forward_dir() -> Vector3:
	var forward = Vector3()
	var camera_basis : Basis = head.global_transform.basis
	var camera_forward : Vector3 = -camera_basis.z;
	
	var up_player : Vector3 = _root3D.to_global(Vector3.UP)
	
	var camera_elevation := camera_forward.dot(up_player)
	if camera_elevation > 0.75:
		# User is looking up
		forward = -camera_basis.y.slide(up_player).normalized()
		
	elif camera_elevation < -0.75:
		# User is looking down
		forward = camera_basis.y.slide(up_player).normalized()
	
	else:
		forward = camera_forward.slide(up_player).normalized()
	
	return forward


func _connect_to_agent(agent: Game_Agent):
	#print(name + " is connecting to agent " + agent.name)
	if agent.player_parent_target:
		reparent_to_agent(agent)
	
	if agent.agent_targets_anchor:
		var ap = Node3D.new()
		ap.name = agent.name + "_Agent_Targets"
	
		add_child(ap)
		ap.transform = agent.agent_targets_anchor.transform
	
		for at in agent.agent_targets:
			var local_t = at.transform
			at.reparent(ap)
			at.transform = local_t
		
	if agent_node_assignments:
		agent_node_assignments.retarget(self, agent)
	
	var lcon = left_hand.get_node("Controls")
	
	if agent.left_hand_control_anchor:
		var lp = Node3D.new()
		lp.name = agent.name + "_Left_Hand_Controls"
	
		lcon.add_child(lp)
		
		lp.transform = agent.left_hand_control_anchor.transform
		
		for lc in agent.left_hand_controls:
			#print("Connecting Left Control: " + lc.name)
			
			lc.controller = left_hand
			if lc.reparent_to_controller and lc.get_parent() != lp:
				var local_t = lc.transform
				lc.reparent(lp)
				lc.transform = local_t
			
		if left_hand_assignments:
			left_hand_assignments.retarget(left_hand, agent.left_hand_control_anchor)
		
	
	var rcon = right_hand.get_node("Controls")
	
	if agent.right_hand_control_anchor:
		var rp = Node3D.new()
		rp.name = agent.name+ "_Right_Hand_Controls"
		rcon.add_child(rp)
		rp.transform = agent.right_hand_control_anchor.transform
		
		for rc in agent.right_hand_controls:
			#print("Connecting Right Control: " + rc.name)
			
			rc.controller = right_hand
			if rc.reparent_to_controller and rc.get_parent() != rp:
				var local_t = rc.transform
				rc.reparent(rp)
				rc.transform = local_t
			
		if right_hand_assignments:
			right_hand_assignments.retarget(right_hand, agent.right_hand_control_anchor)
	
	if agent.head_control_anchor:
		var hp = Node3D.new()
		hp.name = agent.name + "_Head_Targets"
	
		head.add_child(hp)
		hp.transform = agent.head_control_anchor.transform
	
		for ht in agent.head_controls:
			var local_t = ht.transform
			ht.reparent(hp)
			ht.transform = local_t
	
	if head_assignments:
		head_assignments.retarget(head, agent)
	
	for eye_e in agent.eye_effects:
		eyes.add_effect(eye_e)
	
	if eye_assignments:
		eye_assignments.retarget(eyes, agent)
	
	for ear_e in agent.ear_effects:
		ears.add_effect(ear_e)
		
	if ear_assignments:
		ear_assignments.retarget(ears, agent)
	
	agent.player = self


func _disconnect_from_agent(agent : Game_Agent):
	var current_parent = get_parent()
	#if current_parent == agent.player_parent_target:
		
	
	if agent.agent_targets_anchor:
		var a2r = get_node(agent.name + "_Agent_Targets")
		
		var ap = agent.agent_targets_anchor
		
		for at in agent.agent_targets:
			var local_t = at.transform
			at.reparent(ap)
			at.transform = local_t
		
		a2r.queue_free()
	
	if agent_node_assignments:
		agent_node_assignments.clear_set_targets(self, agent)
	
	if agent.left_hand_control_anchor:
		var l2r = left_hand.get_node("Controls/" + agent.name + "_Left_Hand_Controls")
		
		var lp = agent.left_hand_control_anchor
		
		for lc in agent.left_hand_controls:
			lc.controller = null
			
			if lc.get_parent() != lp:
				var local_t = lc.transform
				lc.reparent(lp)
				lc.transform = local_t
			
		if left_hand_assignments:
			left_hand_assignments.clear_set_targets(left_hand, agent.left_hand_control_anchor)
		
		l2r.queue_free()
	
	if agent.right_hand_control_anchor:
		var r2r = right_hand.get_node("Controls/" + agent.name + "_Right_Hand_Controls")
		
		var rp = agent.right_hand_control_anchor
		
		for rc in agent.right_hand_controls:
			rc.controller = null
			
			if rc.get_parent() != rp:
				var local_t = rc.transform
				
				rc.reparent(rp)
				rc.transform = local_t
		
		if right_hand_assignments:
			right_hand_assignments.clear_set_targets(right_hand, agent.right_hand_control_anchor)
		
		r2r.queue_free()
	
	if agent.head_control_anchor:
		var h2r = get_node(agent.name + "_Head_Targets")
		
		var hp = agent.head_control_anchor
		
		for ht in agent.head_targets:
			var local_t = ht.transform
			ht.reparent(hp)
			ht.transform = local_t
		
		h2r.queue_free()
	
	if head_assignments:
		head_assignments.clear_set_targets(head, agent)
	
	for eye_e in agent.eye_effects:
		eyes.remove_effect(eye_e)
		
	if eye_assignments:
		eye_assignments.clear_set_targets(eyes, agent)
	
	for ear_e in agent.ear_effects:
		ears.remove_effect(ear_e)
		
	if ear_assignments:
		ear_assignments.clear_set_targets(ears, agent)


func _find_origin_head_and_hands():
	var deep_search : bool = false
	if not origin:
		if has_node("XROrigin3D"):
			_connect_origin($XROrigin3D)
		else:
			deep_search = true
			
	if not head:
		if has_node("XR_Head"):
			_connect_head($XR_Head)
		else:
			deep_search = true
	
	if not left_hand:
		if has_node("XR_Hand_Left"):
			_connect_left_hand($XR_Hand_Left)
		else:
			deep_search = true
	
	if not right_hand:
		if has_node("XR_Hand_Right"):
			_connect_right_hand($XR_Hand_Right)
		else:
			deep_search = true
	
	if deep_search:
		for c in get_children(true):
			if not origin and c is XROrigin3D:
				_connect_origin(c)
				
			elif not head and c is XRCamera3D:
				_connect_head(c)
			
			elif c is XRController3D:
				if c.tracker == "left_hand" and not left_hand:
					_connect_left_hand(c)
				
				elif c.tracker == "right_hand" and not right_hand:
					_connect_right_hand(c)


func _connect_origin(new_origin : XROrigin3D):
	origin = new_origin


func _connect_head(new_head : XRCamera3D):
	head = new_head
	if not eyes and head.has_node("XR_Eye_FX"):
		eyes = head.get_node("XR_Eye_FX")
	
	if not ears and head.has_node("XR_Ear_FX"):
		ears = head.get_node("XR_Ear_FX")
	
	recenter_player()
	set_height()


func _connect_left_hand(new_left_hand : XRController3D):
	left_hand = new_left_hand


func _connect_right_hand(new_right_hand : XRController3D):
	right_hand = new_right_hand


func _find_session_manager():
	if not session_manager:
		session_manager = XR_Session_Manager.default_session_manager
		if not session_manager:
			print(name + " has no XR_Session_Manager!")
			return
			
	if not session_manager.xr_stopped.is_connected(_session_stop):
		session_manager.xr_stopped.connect(_session_stop)
	
	if not session_manager.xr_is_started:
		session_manager.xr_started.connect(_session_start)
	else:
		_session_start()


func _session_start():
	if session_manager.xr_started.is_connected(_session_start):
		session_manager.xr_started.disconnect(_session_start)
	
	if eyes:
		eyes.initialize_fx(self)
	
	if ears:
		ears.initialize_fx(self)
	
	validate_agents()
	
	joined_session.emit()


func _session_stop():
	pass


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################
func _set_player_root(new_root : Node):
	if new_root == _root:
		return
	if not new_root is Node3D:
		return
		
	_root = new_root
	_root3D = new_root as Node3D


#endregion


#region INTERNAL CLASSES ########################################

#endregion
