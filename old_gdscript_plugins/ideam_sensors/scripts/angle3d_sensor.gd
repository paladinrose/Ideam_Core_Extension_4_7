class_name Angle3D_Sensor
extends Ideam_Sensor3D


#region SIGNALS #################################################

signal angle_range_entered
signal angle_range_exited

signal angle_changed

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var target_x : bool
@export var target_y : bool
@export var target_z : bool

@export var report_angle_change : bool

@export var use_range : bool

@export var range_minimum : Vector3
@export var range_maximum : Vector3


#endregion


#region VARIABLES ###############################################
var angle_diff: Vector3
var rounded_angle : Vector3

var _last_angle : Vector3
var _unrounded_angle : Vector3

var _target_within_range : bool = false

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region STATIC VARIABLES ########################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	super()
	
	if target:
		_last_angle = _target.rotation_degrees


func _process(delta : float) -> void:
	if enabled:
		sense()


#endregion


#region CLASS FUNCTIONS #########################################

func get_unrounded_angle()->Vector3:
	return Vector3(_unrounded_angle)


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

func sense():
	angle_diff = _last_angle - _target.rotation_degrees
	
	_unrounded_angle += angle_diff
	
	if not angle_diff.is_zero_approx():
		_last_angle = _target.rotation_degrees
		angle_changed.emit()
	
	var is_within : bool = target_x or target_y or target_z

	rounded_angle = Vector3(_unrounded_angle)
	
	if target_x:
		rounded_angle.x = fmod(rounded_angle.x, 360)
		
		if rounded_angle.x < 0: 
			rounded_angle.x += 360.0
		
		if rounded_angle.x < range_minimum.x or rounded_angle.x > range_maximum.x:
			is_within = false
	
	if target_y:
		rounded_angle.y = fmod(rounded_angle.y, 360)
		
		if rounded_angle.y < 0: 
			rounded_angle.y += 360.0
		
		if rounded_angle.y < range_minimum.y or rounded_angle.y > range_maximum.y:
			is_within = false
	
	if target_z:
		rounded_angle.z = fmod(rounded_angle.z, 360)
		
		if rounded_angle.z < 0: 
			rounded_angle.z += 360.0
		
		if rounded_angle.z < range_minimum.z or rounded_angle.z > range_maximum.z:
			is_within = false
	
	if is_within != _target_within_range:
		_target_within_range = is_within
		
		if _target_within_range:
			angle_range_entered.emit()
		else:
			angle_range_exited.emit()


#endregion


#region INTERNAL CLASSES ########################################

#endregion
