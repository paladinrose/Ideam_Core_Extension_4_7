class_name XR_Eye_FX
extends XR_FX

#region SIGNALS #################################################

#endregion


#region ENUMS ###################################################

enum EffectedFloatResolutions {
	PRIORITY,
	ADD,
	AVERAGE,
	PRIORITY_AVERAGE,
}


enum EffectedColorResolutions {
	PRIORITY,
	ADD,
	AVERAGE,
	PRIORITY_AVERAGE,
}


#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var both_eyes_overlay : MeshInstance3D

@export var left_eye_overlay : MeshInstance3D

@export var right_eye_overlay : MeshInstance3D

@export var effects : Array[XR_Eye_Effect]

#endregion


#region VARIABLES ###############################################

var _effected_floats_left : Dictionary
var _effected_colors_left : Dictionary

var _effected_floats_right : Dictionary
var _effected_colors_right : Dictionary

var _effected_floats_both : Dictionary
var _effected_colors_both : Dictionary

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	if _is_ready:
		return
		
	_effected_colors_left = {}
	_effected_floats_left = {}
	
	_effected_colors_right = {}
	_effected_floats_right = {}
	
	_effected_colors_both = {}
	_effected_floats_both = {}
	
	_is_ready = true

#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################
func add_effect(effect : XR_Eye_Effect)->int:
	var id = effects.find(effect)
	if id < 0:
		id = effects.size()
		
		effects.append(effect)
		
		if _initialized:
			_connect_eye_effect(effect)
		
		effect_added.emit(effect)
	
	return id


func remove_effect(effect : XR_Eye_Effect):
	var id = effects.find(effect)
	if id < 0:
		return
	
	remove_effect_at(id)


func remove_effect_at(id : int):
	if id < 0 or id >= effects.size():
		return
	
	var effect : XR_Eye_Effect = effects[id]
	
	_disconnect_eye_effect(effect)
	
	effect.material = null
	
	effects.remove_at(id)
	
	effect_removed.emit(effect)


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

func initialize_fx(player : XR_Player):
	if _initialized or not _is_ready:
		return
	
	_initialized = true
	
	for i in range(effects.size()):
		_connect_eye_effect(effects[i])
	
	initialized.emit()


func _connect_eye_effect(effect : XR_Eye_Effect):
	if not effect.current_value_updated.is_connected(_effect_value_update):
		effect.current_value_updated.connect(_effect_value_update)
	
	for f in range(effect.target_floats.size()):
		var ft : StringName = effect.target_floats[f]
		
		match effect.effect_target:
			XR_Eye_Effect.EyeEffectTarget.LEFT:
				if not _effected_floats_left.has(ft):
					_create_effected_float(ft, effect.effect_target)
		
				var effected_float : Array = _effected_floats_left[ft]
		
				if not effected_float.has(effect):
					effected_float.append(effect)
					_update_effected_float(ft, effect.effect_target)
			
			XR_Eye_Effect.EyeEffectTarget.RIGHT:
				if not _effected_floats_right.has(ft):
					_create_effected_float(ft, effect.effect_target)
		
				var effected_float : Array = _effected_floats_right[ft]
		
				if not effected_float.has(effect):
					effected_float.append(effect)
					_update_effected_float(ft, effect.effect_target)
			
			XR_Eye_Effect.EyeEffectTarget.BOTH:
				if not _effected_floats_both.has(ft):
					_create_effected_float(ft, effect.effect_target)
		
				var effected_float : Array = _effected_floats_both[ft]
		
				if not effected_float.has(effect):
					effected_float.append(effect)
					_update_effected_float(ft, effect.effect_target)
	
	for f in range(effect.target_colors.size()):
		var ft : StringName = effect.target_colors[f]
		
		match effect.effect_target:
			XR_Eye_Effect.EyeEffectTarget.LEFT:
				if not _effected_colors_left.has(ft):
					_create_effected_color(ft, effect.effect_target)
		
				var effected_color : Array = _effected_colors_left[ft]
		
				if not effected_color.has(effect):
					effected_color.append(effect)
					_update_effected_color(ft, effect.effect_target)
			
			XR_Eye_Effect.EyeEffectTarget.RIGHT:
				if not _effected_colors_right.has(ft):
					_create_effected_color(ft, effect.effect_target)
		
				var effected_color : Array = _effected_colors_right[ft]
		
				if not effected_color.has(effect):
					effected_color.append(effect)
					_update_effected_color(ft, effect.effect_target)
			
			XR_Eye_Effect.EyeEffectTarget.BOTH:
				if not _effected_colors_both.has(ft):
					_create_effected_color(ft, effect.effect_target)
		
				var effected_color : Array = _effected_colors_both[ft]
		
				if not effected_color.has(effect):
					effected_color.append(effect)
					_update_effected_color(ft, effect.effect_target)


func _disconnect_eye_effect(effect : XR_Eye_Effect):
	if effect.current_value_updated.is_connected(_effect_value_update):
		effect.current_value_updated.disconnect(_effect_value_update)
		
	
	for f in range(effect.target_floats.size()):
		var ft : StringName = effect.target_floats[f]
		match effect.effect_target:
			XR_Eye_Effect.EyeEffectTarget.LEFT:
				if not _effected_floats_left.has(ft):
					continue
				
				var effected_float : Array = _effected_floats_left[ft]
				
				var eid : int = effected_float.find(effect)
				if eid >=0:
					effected_float.remove_at(eid)
					
					_update_effected_float(ft, effect.effect_target)
			
			XR_Eye_Effect.EyeEffectTarget.RIGHT:
				if not _effected_floats_right.has(ft):
					continue
				
				var effected_float : Array = _effected_floats_right[ft]
				
				var eid : int = effected_float.find(effect)
				if eid >=0:
					effected_float.remove_at(eid)
					
					_update_effected_float(ft, effect.effect_target)
			
			XR_Eye_Effect.EyeEffectTarget.BOTH:
				if not _effected_floats_both.has(ft):
					continue
				
				var effected_float : Array = _effected_floats_both[ft]
				
				var eid : int = effected_float.find(effect)
				if eid >=0:
					effected_float.remove_at(eid)
					
					_update_effected_float(ft, effect.effect_target)
	
	for f in range(effect.target_colors.size()):
		var ft : StringName = effect.target_colors[f]
		match effect.effect_target:
			XR_Eye_Effect.EyeEffectTarget.LEFT:
				if not _effected_colors_left.has(ft):
					continue
				
				var effected_color : Array = _effected_colors_left[ft]
				
				var eid : int = effected_color.find(effect)
				if eid >=0:
					effected_color.remove_at(eid)
					
					_update_effected_color(ft, effect.effect_target)
			
			XR_Eye_Effect.EyeEffectTarget.RIGHT:
				if not _effected_colors_right.has(ft):
					continue
				
				var effected_color : Array = _effected_colors_right[ft]
				
				var eid : int = effected_color.find(effect)
				if eid >=0:
					effected_color.remove_at(eid)
					
					_update_effected_color(ft, effect.effect_target)
			
			XR_Eye_Effect.EyeEffectTarget.BOTH:
				if not _effected_colors_both.has(ft):
					continue
				
				var effected_color : Array = _effected_colors_both[ft]
				
				var eid : int = effected_color.find(effect)
				if eid >=0:
					effected_color.remove_at(eid)
					
					_update_effected_color(ft, effect.effect_target)


func _create_effected_float(float_target : StringName, effect_target : XR_Eye_Effect.EyeEffectTarget):
	var effected_float : Array = []
	effected_float.append(EffectedFloatResolutions.PRIORITY)
	
	var mat : ShaderMaterial
	match effect_target:
		XR_Eye_Effect.EyeEffectTarget.LEFT:
			mat = left_eye_overlay.get_surface_override_material(0)
			effected_float.append(mat.get_shader_parameter(float_target))
			_effected_floats_left[float_target] = effected_float
		
		XR_Eye_Effect.EyeEffectTarget.RIGHT:
			mat = right_eye_overlay.get_surface_override_material(0)
			effected_float.append(mat.get_shader_parameter(float_target))
			_effected_floats_right[float_target] = effected_float
		
		XR_Eye_Effect.EyeEffectTarget.BOTH:
			mat = both_eyes_overlay.get_surface_override_material(0)
			effected_float.append(mat.get_shader_parameter(float_target))
			_effected_floats_both[float_target] = effected_float


func _create_effected_color(color_target : StringName, effect_target : XR_Eye_Effect.EyeEffectTarget):
	var effected_color : Array = []
	effected_color.append(EffectedColorResolutions.PRIORITY)
	
	var mat : ShaderMaterial
	match effect_target:
		XR_Eye_Effect.EyeEffectTarget.LEFT:
			mat = left_eye_overlay.get_surface_override_material(0)
			effected_color.append(mat.get_shader_parameter(color_target))
			_effected_colors_left[color_target] = effected_color
		
		XR_Eye_Effect.EyeEffectTarget.RIGHT:
			mat = right_eye_overlay.get_surface_override_material(0)
			effected_color.append(mat.get_shader_parameter(color_target))
			_effected_colors_right[color_target] = effected_color
		
		XR_Eye_Effect.EyeEffectTarget.BOTH:
			mat = both_eyes_overlay.get_surface_override_material(0)
			effected_color.append(mat.get_shader_parameter(color_target))
			_effected_colors_both[color_target] = effected_color


func _update_effected_float(float_target : StringName, effect_target : XR_Eye_Effect.EyeEffectTarget):
	var mat : ShaderMaterial
	var ef : Array 
	match effect_target:
		XR_Eye_Effect.EyeEffectTarget.LEFT:
			if not _effected_floats_left.has(float_target):
				return
			mat = left_eye_overlay.get_surface_override_material(0)
			
			ef = _effected_floats_left[float_target]
		
		XR_Eye_Effect.EyeEffectTarget.RIGHT:
			if not _effected_floats_right.has(float_target):
				return
			mat = right_eye_overlay.get_surface_override_material(0)
			
			ef = _effected_floats_right[float_target]
		
		XR_Eye_Effect.EyeEffectTarget.BOTH:
			if not _effected_floats_both.has(float_target):
				return
			mat = both_eyes_overlay.get_surface_override_material(0)
			
			ef = _effected_floats_both[float_target]
	
	if ef.size() == 2:
		mat.set_shader_parameter(float_target, ef[1])
		return
	
	var resolution : EffectedFloatResolutions = ef[0]
	
	match resolution:
		EffectedFloatResolutions.PRIORITY:
			var highest_priority : int = -1
			var highest_id : int = -1
			
			for i in range(2, ef.size()):
				if ef[i].priority > highest_priority:
					highest_priority = ef[i].priority
					highest_id = i
			
			if highest_id < 0:
				return
			
			var h_ef : XR_Eye_Effect = ef[highest_id]
			var ef_id : int = h_ef.target_floats.find(float_target)
			var ef_v : float = h_ef.target_float_curves[ef_id].sample_baked(h_ef.current_value)
			
			mat.set_shader_parameter(float_target, ef_v)
		
		EffectedFloatResolutions.ADD:
			var sum : float = 0
			for i in range(2, ef.size()):
				var h_ef : XR_Eye_Effect = ef[i]
				var ef_id : int = h_ef.target_floats.find(float_target)
				var ef_v : float = h_ef.target_float_curves[ef_id].sample_baked(h_ef.current_value)
				
				sum += ef_v
			
			mat.set_shader_parameter(float_target, sum)
		
		EffectedFloatResolutions.AVERAGE:
			var av : float = 0
			var av_count : int = 0
			for i in range(2, ef.size()):
				var h_ef : XR_Eye_Effect = ef[i]
				var ef_id : int = h_ef.target_floats.find(float_target)
				var ef_v : float = h_ef.target_float_curves[ef_id].sample_baked(h_ef.current_value)
				
				av += ef_v
				av_count += 1
			
			av /= av_count
			mat.set_shader_parameter(float_target, av)
		
		EffectedFloatResolutions.PRIORITY_AVERAGE:
			var av : float = 0
			var av_count : int = 0
			for i in range(2, ef.size()):
				var h_ef : XR_Eye_Effect = ef[i]
				var ef_id : int = h_ef.target_floats.find(float_target)
				var ef_v : float = h_ef.target_float_curves[ef_id].sample_baked(h_ef.current_value)
				
				av += ef_v * h_ef.priority
				av_count += h_ef.priority
			
			av /= av_count
			mat.set_shader_parameter(float_target, av)


func _update_effected_color(color_target : StringName, effect_target : XR_Eye_Effect.EyeEffectTarget):
	var mat : ShaderMaterial
	var ef : Array 
	match effect_target:
		XR_Eye_Effect.EyeEffectTarget.LEFT:
			if not _effected_colors_left.has(color_target):
				return
			mat = left_eye_overlay.get_surface_override_material(0)
			
			ef = _effected_colors_left[color_target]
		
		XR_Eye_Effect.EyeEffectTarget.RIGHT:
			if not _effected_colors_right.has(color_target):
				return
			mat = right_eye_overlay.get_surface_override_material(0)
			
			ef = _effected_colors_right[color_target]
		
		XR_Eye_Effect.EyeEffectTarget.BOTH:
			if not _effected_colors_both.has(color_target):
				return
			mat = both_eyes_overlay.get_surface_override_material(0)
			
			ef = _effected_colors_both[color_target]
	
	if ef.size() == 2:
		mat.set_shader_parameter(color_target, ef[1])
		return
	
	var resolution : EffectedColorResolutions = ef[0]
	
	match resolution:
		EffectedColorResolutions.PRIORITY:
			var highest_priority : int = -1
			var highest_id : int = -1
			
			for i in range(2, ef.size()):
				if ef[i].priority > highest_priority:
					highest_priority = ef[i].priority
					highest_id = i
			
			if highest_id < 0:
				return
			
			var h_ef : XR_Eye_Effect = ef[highest_id]
			var ef_id : int = h_ef.target_colors.find(color_target)
			var ef_v : Color = h_ef.target_color_gradients[ef_id].sample(h_ef.current_value)
			
			mat.set_shader_parameter(color_target, ef_v)
		
		EffectedColorResolutions.ADD:
			var sum : Color = Color(0,0,0,0)
			for i in range(2, ef.size()):
				var h_ef : XR_Eye_Effect = ef[i]
				var ef_id : int = h_ef.target_colors.find(color_target)
				var ef_v : Color = h_ef.target_color_gradients[ef_id].sample(h_ef.current_value)
				
				sum += ef_v
			
			mat.set_shader_parameter(color_target, sum)
		
		EffectedColorResolutions.AVERAGE:
			var av : Color = Color(0,0,0,0)
			var av_count : int = 0
			for i in range(2, ef.size()):
				var h_ef : XR_Eye_Effect = ef[i]
				var ef_id : int = h_ef.target_colors.find(color_target)
				var ef_v : Color = h_ef.target_color_gradients[ef_id].sample(h_ef.current_value)
				
				av += ef_v
				av_count += 1
			
			av /= av_count
			mat.set_shader_parameter(color_target, av)
		
		EffectedColorResolutions.PRIORITY_AVERAGE:
			var av : Color = Color(0,0,0,0)
			
			var av_count : int = 0
			for i in range(2, ef.size()):
				var h_ef : XR_Eye_Effect = ef[i]
				var ef_id : int = h_ef.target_colors.find(color_target)
				var ef_v : Color = h_ef.target_color_gradients[ef_id].sample(h_ef.current_value)
				
				av += ef_v * h_ef.priority
				av_count += h_ef.priority
			
			av /= av_count
			mat.set_shader_parameter(color_target, av)


func _effect_value_update(effect:XR_Eye_Effect):
	if not effects.has(effect):
		return
		
	for i in range(effect.target_floats.size()):
		_update_effected_float(effect.target_floats[i], effect.effect_target)
	
	for i in range(effect.target_colors.size()):
		_update_effected_color(effect.target_colors[i], effect.effect_target)


#endregion


#region INTERNAL CLASSES ########################################

#endregion
