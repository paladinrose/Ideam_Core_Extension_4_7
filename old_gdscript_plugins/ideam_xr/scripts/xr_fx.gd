class_name XR_FX
extends Node3D




#region SIGNALS #################################################

signal initialized

signal effect_added (effect)
signal effect_removed (effect)

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

#endregion


#region VARIABLES ###############################################

var _is_ready : bool = false
var _initialized : bool = false

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region GODOT FUNCTIONS #########################################

#func _ready() -> void:
#	pass 

#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################
func initialize_fx(player : XR_Player):
	pass
#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
