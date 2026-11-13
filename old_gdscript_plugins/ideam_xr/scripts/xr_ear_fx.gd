class_name XR_Ear_FX
extends XR_FX

#region SIGNALS #################################################

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var audio_sources : Array[AudioStreamPlayer3D]
@export var effects : Array[XR_Ear_Effect]

#endregion


#region VARIABLES ###############################################

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
func add_effect(effect : XR_Ear_Effect)->int:
	var id = effects.find(effect)
	if id < 0:
		id = effects.size()
		
		effects.append(effect)
		
		effect_added.emit(effect)
	
	return id


func remove_effect(effect : XR_Ear_Effect):
	var id = effects.find(effect)
	if id < 0:
		return
	
	remove_effect_at(id)


func remove_effect_at(id : int):
	if id < 0 or id >= effects.size():
		return
	
	var effect : XR_Ear_Effect = effects[id]
	
	effects.remove_at(id)
	
	effect_removed.emit(effect)


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
