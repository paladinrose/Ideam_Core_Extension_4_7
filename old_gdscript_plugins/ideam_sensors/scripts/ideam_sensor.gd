class_name Ideam_Sensor
extends Node


#region SIGNALS #################################################
signal sensor_enabled
signal sensor_disabled

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var enabled : bool:
	set(new_enabled):
		if new_enabled == enabled:
			return
		
		if not _is_ready:
			enabled = new_enabled
			return
		
		enabled = new_enabled
		
		if enabled:
			sensor_enabled.emit()
		else:
			sensor_disabled.emit()


#endregion


#region VARIABLES ###############################################
var _is_ready : bool

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region STATIC VARIABLES ########################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	_is_ready = true
	
	if enabled:
		sensor_enabled.emit()
	else:
		sensor_disabled.emit()


#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################

func sense():
	pass

func enable_sensor():
	enabled = true

func disable_sensor():
	enabled = false


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
