class_name XR_Eye_Effect
extends Node


#region SIGNALS #################################################

signal current_value_updated (on : XR_Eye_Effect)

signal effect_tween_started
signal effect_tween_ended

#endregion


#region ENUMS ###################################################

enum EyeEffectTarget{
	BOTH,
	LEFT,
	RIGHT,

}


#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var priority : int
@export var effect_target : EyeEffectTarget

@export var target_colors : Array[StringName]
@export var target_color_gradients : Array[Gradient]

@export var target_floats : Array[StringName]
@export var target_float_curves : Array[Curve]

@export_range(0.0, 1.0) var default_effect_value : float
@export var default_tween_time : float = 0.1

@export var current_value : float :
	set(new_v):
		if new_v == _current_value:
			return
		
		if not _is_ready:
			_current_value = new_v
			return
		
		set_current_value(new_v)
		
	get:
		return _current_value

#endregion


#region VARIABLES ###############################################
var _is_ready : bool
var _current_value : float
var _value_tween : Tween

var _priority : int

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	if _is_ready:
		return
	_is_ready = true
	
	_priority = priority

#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################

func set_to_default():
	priority = _priority
	current_value = default_effect_value


func tween_to_default(tween_priority : int = -1):
	#print(name + " tweening to default. From " + str(current_value) + " to " + str(default_effect_value))
	tween_value_to(default_effect_value, tween_priority)


func set_current_value(new_value : float):
	_current_value = new_value
	current_value_updated.emit(self)


func tween_value_to(new_value : float,  tween_priority : int = -1, duration : float = default_tween_time):
	if tween_priority >=0:
		priority = tween_priority
	
	if _value_tween:
		_value_tween.kill()
	
	_value_tween = create_tween()
	_value_tween.tween_property(self, "current_value", new_value, duration)
	
	effect_tween_started.emit()
	_value_tween.finished.connect(finish_tween)


func finish_tween():
	priority = _priority
	effect_tween_ended.emit()


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
