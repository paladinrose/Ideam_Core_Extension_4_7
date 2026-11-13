class_name XR_Basic_Movement
extends Node


#region SIGNALS #################################################

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var agent : XR_Game_Agent

@export var move_speed : float = 3
@export var turn_speed : float = 3

@export var jump_strength : float = 0.5

@export var can_fly : bool
@export var fly_strength  : float = 1.0
@export var fly_raycast : RayCast3D

@export var allow_air_move : bool
@export var allow_air_turn : bool

@export var ground_detector : RayCast3D
@export var fall_direction : Vector3


#endregion


#region VARIABLES ###############################################

var _move_vector : Vector3
var _fly_vector : Vector3
var _rotate_vector : Vector3

var _rising : bool
var _rising_timer : Timer
var _grounded : bool = true
var _flying : bool


#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region STATIC VARIABLES ########################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	_rising_timer = Timer.new()
	add_child(_rising_timer)
	_rising_timer.timeout.connect(_start_falling)


func _physics_process(delta : float) -> void:
	if not agent:
		print("No agent to move!")
		return
	
	if not agent.player:
		print("agent has no player!")
		return
	
	if not _grounded:
		if _flying:
			_handle_fly(delta)
			
		elif not _rising:
			_fall()
			
			if ground_detector and ground_detector.is_colliding():
				_land()
		else:
			_rise()
			
	if _rotate_vector != Vector3.ZERO:
		agent.player.rotation_degrees += _rotate_vector * delta
		
	
	if _move_vector != Vector3.ZERO:
		agent.player.translate(_move_vector*delta)
		#agent.player.position += _move_vector * delta
		


#endregion


#region CLASS FUNCTIONS #########################################

func move(move_vec : Vector2):
	
	if _grounded:
		_move_vector = Vector3(move_vec.x*move_speed, 0, -move_vec.y*move_speed)
		return
	
	if allow_air_move:
		_move_vector = Vector3(move_vec.x*move_speed, 0, -move_vec.y*move_speed)
	

func move_forward():
	if _grounded:
		_move_vector += Vector3(0, 0, -move_speed)
		return
	
	if allow_air_move or _flying:
		_move_vector += Vector3(0, 0, -move_speed)


func move_backward():
	if _grounded:
		_move_vector += Vector3(0, 0, move_speed)
		return
	
	if allow_air_move or _flying:
		_move_vector += Vector3(0, 0, move_speed)


func strafe_left():
	if _grounded:
		_move_vector += Vector3(-move_speed, 0, 0)
		return
	
	if allow_air_move or _flying:
		_move_vector += Vector3(-move_speed, 0, 0)


func strafe_right():
	if _grounded:
		_move_vector += Vector3(move_speed, 0, 0)
		return
	
	if allow_air_move or _flying:
		_move_vector += Vector3(move_speed, 0, 0)



func turn(turn_vect : Vector2):
	_rotate_vector = Vector3(0, -turn_vect.x*turn_speed, 0)


func turn_left():
	_rotate_vector += Vector3(0, -turn_speed, 0)


func turn_right():
	_rotate_vector += Vector3(0, turn_speed, 0)


func jump():
	if not _grounded: 
		if not _flying and can_fly:
			_start_flying()
			
		elif _flying:
			_stop_flying()
			
		return
	
	if _rising:
		return
	
	_grounded = false
	_rising = true
	
	_rising_timer.start(0.5)


func _start_falling():
	_rising = false


func _rise():
	_move_vector -= fall_direction * jump_strength


func _fall():
	_move_vector += fall_direction


func _land():
	_grounded = true


func _start_flying():
	_flying = true


func fly(value : float):
	if not _flying:
		return
		
	var direction : Vector3 = fly_raycast.to_global(fly_raycast.target_position)
	direction = direction.normalized() * fly_strength * value
	_fly_vector = direction


func _handle_fly(_delta : float):
	_move_vector += _fly_vector


func _stop_flying():
	_flying = false
	_fly_vector = Vector3.ZERO
	
#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
