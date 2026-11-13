class_name Ideam_Sensor3D
extends Ideam_Sensor


#region SIGNALS #################################################

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var target : Node3D:
	set(new_target):
		set_target(new_target)
	get:
		return _target


#endregion


#region VARIABLES ###############################################

var _target : Node3D

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region STATIC VARIABLES ########################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	super()

#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################

func set_target(new_target : Node3D):
	if new_target == _target:
		return
	
	if not _is_ready:
		_target = new_target
		return
		
	_target = new_target


func clear_target():
	target = null


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
