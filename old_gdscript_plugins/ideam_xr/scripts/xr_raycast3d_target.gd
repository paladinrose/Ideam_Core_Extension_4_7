@tool
class_name XR_RayCast3D_Target
extends Node


#region SIGNALS #################################################

signal ray_entered (by : XR_RayCast3D)

signal ray_over (ray : XR_RayCast3D, delta : float)

signal ray_exited (gone : XR_RayCast3D)

signal action_used


#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var target_source : Node

#endregion


#region VARIABLES ###############################################

var rays_over : Array[XR_RayCast3D]
var times_over : Array[float]

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region STATIC VARIABLES ########################################

#endregion


#region GODOT FUNCTIONS #########################################

#func _ready() -> void:
#	pass 

#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################

func raycast_enter(raycast : XR_RayCast3D):
	rays_over.append(raycast)
	times_over.append(0.0)
	ray_entered.emit(raycast)


func raycast_exit(raycast : XR_RayCast3D):
	var id = rays_over.find(raycast)
	if id < 0:
		return
		
	rays_over.remove_at(id)
	times_over.remove_at(id)
	
	ray_exited.emit(raycast)


func raycast_over(raycast : XR_RayCast3D, delta : float):
	var id = rays_over.find(raycast)
	if id < 0:
		return
	
	times_over[id] += delta
	
	ray_over.emit(raycast, delta)


func use_action(raycast : XR_RayCast3D):
	action_used.emit()


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
