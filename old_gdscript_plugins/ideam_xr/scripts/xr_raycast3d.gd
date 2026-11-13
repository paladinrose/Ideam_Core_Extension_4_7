@tool
class_name XR_RayCast3D
extends RayCast3D


#region SIGNALS #################################################

signal target_hit (target : XR_RayCast3D_Target)

signal target_lost (target : XR_RayCast3D_Target)

signal target_action_used(target)

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

#endregion


#region VARIABLES ###############################################

var _last_col : Object
var _last_target : XR_RayCast3D_Target

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region STATIC VARIABLES ########################################

#endregion


#region GODOT FUNCTIONS #########################################

#func _ready() -> void:
#	pass 

func _process(delta : float) -> void:
	var col : Object = get_collider()
	var now_hit : XR_RayCast3D_Target = null
	
	if col:
		if col == _last_col:
			if _last_target:
				_last_target.raycast_over(self, delta)

			return
		
		_last_col = col
		
		if col is XR_RayCast3D_Target:
			now_hit = col as XR_RayCast3D_Target
		
		else:
			var hits = col.find_children("*", "XR_RayCast3D_Target")
			
			if hits.size() > 0:
				now_hit = hits[0]
	
	if not now_hit:
		if not _last_target:
			return
		
		target_lost.emit(_last_target)
		
		_last_target.raycast_exit(self)
		
		_last_target = null
		
	else:
		if _last_target:
			#if _last_target == now_hit:
				#_last_target.raycast_over(self, delta)
				#return
			
			target_lost.emit(_last_target)
			
			_last_target.raycast_exit(self)
		
		target_hit.emit(now_hit)
		
		now_hit.raycast_enter(self)
		
		_last_target = now_hit


#endregion


#region CLASS FUNCTIONS #########################################

func use_target_action():
	if not _last_target:
		return
	
	if _last_target.target_source:
		target_action_used.emit(_last_target.target_source)
	
	else:
		target_action_used.emit(_last_target)
	
	_last_target.use_action(self)


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
