class_name XRController3D_Animated_Parameters
extends Node


#region SIGNALS #################################################

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################
@export var controller : XRController3D:
	set(nc):
		set_controller(nc)
	get:
		return _controller

@export var animationTree : AnimationTree
@export var parameter_controls : Array[String] = []
@export var parameters : Array[String] = []


#endregion


#region VARIABLES ###############################################

var _controller : XRController3D

var _parameter_functions : Array[Callable]

var _is_ready : bool


#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region STATIC VARIABLES ########################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	if _is_ready:
		return
	
	_is_ready = true
	
	if controller:
		_connect_controller()



#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################

func set_float_parameter(value : float, parameter_id : int = 0):
	if parameter_id < 0 or parameter_id >= parameters.size():
		return
	var parm : String = "parameters/" + parameters[parameter_id]
	
	animationTree.set(parm, value)

func set_controller(new_controller : XRController3D):
	if new_controller == _controller:
		return
		
	if not _is_ready:
		_controller = new_controller
		return
	
	if controller:
		_disconnect_controller()
	
	_controller = new_controller
	
	_connect_controller()


func _connect_controller():
	_parameter_functions = []
	_parameter_functions.resize(parameters.size())
	
	var cons = controller.find_children("*", "XR_Control")
	for c in cons:
		var xr_control : XR_Control = c as XR_Control
		
		for i in range(parameter_controls.size()):
			var control : String = parameter_controls[i]
			
			if xr_control.control_name == control:
				var analogue = func(val : float):
					set_float_parameter(val, i)
				_parameter_functions[i] = analogue
				xr_control.analogue_changed.connect(analogue)


func _disconnect_controller():
	var cons = controller.find_children("*", "XR_Control")
	for c in cons:
		var xr_control : XR_Control = c as XR_Control
		
		for i in range(parameter_controls.size()):
			var control : String = parameter_controls[i]
			
			if xr_control.control_name == control and _parameter_functions[i]:
				xr_control.analogue_changed.disconnect(_parameter_functions[i])


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
