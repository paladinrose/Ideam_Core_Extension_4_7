class_name Position3D_Sensor
extends Ideam_Sensor3D

#region SIGNALS #################################################

signal position_range_entered
signal position_range_exited

signal position_changed


#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var target_x : bool
@export var target_y : bool
@export var target_z : bool

@export var report_position_change : bool

@export var use_range : bool

@export var range_minimum : Vector3
@export var range_maximum : Vector3

#endregion


#region VARIABLES ###############################################

var position_diff: Vector3

var _last_position : Vector3
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
		_last_position = _target.position


func _process(delta : float) -> void:
	if enabled:
		sense()


#endregion


#region CLASS FUNCTIONS #########################################

#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

func sense():
	position_diff = _last_position - _target.position
	
	if not position_diff.is_zero_approx():
		_last_position = _target.position
		
		if report_position_change:
			position_changed.emit()
	
	var is_within : bool = target_x or target_y or target_z

	if target_x:
		if _target.position.x < range_minimum.x or _target.position.x > range_maximum.x:
			is_within = false
	
	if target_y:
		if _target.position.y < range_minimum.y or _target.position.y > range_maximum.y:
			is_within = false
	
	if target_x:
		if _target.position.z < range_minimum.z or _target.position.z > range_maximum.z:
			is_within = false
	
	if is_within != _target_within_range:
		_target_within_range = is_within
		
		if _target_within_range:
			position_range_entered.emit()
		else:
			position_range_exited.emit()


#endregion


#region INTERNAL CLASSES ########################################

#endregion
