class_name XR_Control
extends Node

#region SIGNALS #################################################

signal controller_connected (controller : XRController3D)
signal controller_released (controller : XRController3D)

signal button_pressed
signal button_released
signal button_clicked

signal analogue_pressed
signal analogue_changed(value : float)
signal analogue_released
signal analogue_clicked

signal two_axis_pressed
signal two_axis_changed(value : Vector2)
signal two_axis_released
signal two_axis_clicked

signal three_axis_pressed
signal three_axis_changed(value : Vector3)
signal three_axis_released
signal three_axis_clicked

signal toggled
signal toggled_on
signal toggled_off

#endregion


#region ENUMS ###################################################

enum XRControlType {
	BUTTON,
	ANALOGUE,
	TWO_AXIS,
	THREE_AXIS,
}


#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var control_name : String
@export var control_type : XRControlType

@export var controller : XRController3D:
	set(new_controller):
		if new_controller == controller:
			return
		
		if not _is_ready:
			controller = new_controller
			return
			
		if controller:
			_disconnect_from_controller()
		
		controller = new_controller
		
		if controller:
			_connect_to_controller()
	get:
		return controller

@export var analogue_deadzone : float = 0.09

@export var reparent_to_controller : bool

@export var click_time : float = 0.15

@export var toggle_on_click : bool

#endregion


#region VARIABLES ###############################################
var _is_ready : bool = false

var _last_value : Vector3 = Vector3.ZERO

var _dead_zone_scalar : float = 1.0

var _click_timer : Timer
var _click_check : bool = false

var _toggled : bool = false

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	_is_ready = true
	
	_dead_zone_scalar = 1.0 / (1.0 - analogue_deadzone)
	
	_click_timer = Timer.new()
	add_child(_click_timer)
	
	_click_timer.timeout.connect(_end_click_check)
	
	if controller:
		_connect_to_controller()



#func _process(delta : float) -> void:
#	pass


#endregion


#region CLASS FUNCTIONS #########################################

func _connect_to_controller():
	match control_type:
		XRControlType.BUTTON:
			if not controller.button_pressed.is_connected(_button_press):
				controller.button_pressed.connect(_button_press)
			
			if not controller.button_released.is_connected(_button_release):
				controller.button_released.connect(_button_release)
				
		XRControlType.ANALOGUE:
			if not controller.input_float_changed.is_connected(_analogue_change):
				controller.input_float_changed.connect(_analogue_change)
			
		XRControlType.TWO_AXIS:
			if not controller.input_vector2_changed.is_connected(_two_axis_change):
				controller.input_vector2_changed.connect(_two_axis_change)
	
	controller_connected.emit(controller)


func _disconnect_from_controller():
	match control_type:
		XRControlType.BUTTON:
			if controller.button_pressed.is_connected(_button_press):
				controller.button_pressed.disconnect(_button_press)
			
			if controller.button_released.is_connected(_button_release):
				controller.button_released.disconnect(_button_release)
				
		XRControlType.ANALOGUE:
			if controller.input_float_changed.is_connected(_analogue_change):
				controller.input_float_changed.disconnect(_analogue_change)
			
		XRControlType.TWO_AXIS:
			if controller.input_vector2_changed.is_connected(_two_axis_change):
				controller.input_vector2_changed.disconnect(_two_axis_change)
	
	controller_released.emit(controller)


func _end_click_check():
	_click_check = false


func _toggle_click():
	_toggled = !_toggled
	
	toggled.emit()
	
	if _toggled:
		toggled_on.emit()
		
	else:
		toggled_off.emit()


func _button_press(button_name : String):
	if button_name != control_name:
		return
	
	button_pressed.emit()
	if _click_check:
		_end_click_check()
		
	_click_check = true
	_click_timer.start()


func _button_release(button_name : String):
	if button_name != control_name:
		return
	
	if _click_check:
		button_clicked.emit()
		_click_check = false
		_click_timer.stop()
		
		if toggle_on_click:
			_toggle_click()
			
	
	button_released.emit()


func _analogue_press(analogue_name : String):
	if analogue_name != control_name:
		return
	
	analogue_pressed.emit()
	if _click_check:
		_end_click_check()
		
	_click_check = true
	_click_timer.start()


func _apply_deadzone(new_value : float)->float:
	if absf(new_value) <= analogue_deadzone:
		new_value = 0.0
	
	elif new_value < 0:
		new_value += analogue_deadzone
	
	elif new_value > 0:
		new_value -= analogue_deadzone
	
	new_value *= _dead_zone_scalar
	
	return new_value


func _analogue_change(analogue_name : String, new_value : float):
	if analogue_name != control_name:
		return
	
	new_value = _apply_deadzone(new_value)
	
	if _last_value.x == 0:
		if new_value == 0:
			return
			
		_last_value.x = new_value
		
		_analogue_press(analogue_name)
		
		analogue_changed.emit(new_value)
		return
		
	elif new_value == 0:
		_last_value.x = 0
		_analogue_release(analogue_name)
		return
		
	_last_value.x = new_value
		
	analogue_changed.emit(new_value)


func _analogue_release(analogue_name : String):
	if analogue_name != control_name:
		return
	
	if _click_check:
		analogue_clicked.emit()
		_click_check = false
		_click_timer.stop()
		
		if toggle_on_click:
			_toggle_click()
	
	analogue_released.emit()


func _two_axis_press(two_axis_name : String):
	if two_axis_name != control_name:
		return
	
	two_axis_pressed.emit()
	
	if _click_check:
		_end_click_check()
		
	_click_check = true
	_click_timer.start()


func _two_axis_change(two_axis_name : String, new_value : Vector2):
	if two_axis_name != control_name:
		return
	
	new_value.x = _apply_deadzone(new_value.x)
	new_value.y = _apply_deadzone(new_value.y)
	
	if _last_value.x == 0 and _last_value.y == 0:
		if new_value == Vector2.ZERO:
			return
		
		_last_value.x = new_value.x
		_last_value.y = new_value.y
		
		_two_axis_press(two_axis_name)
		
		two_axis_changed.emit(new_value)
		
		return
	
	elif new_value == Vector2.ZERO:
		_last_value = Vector3.ZERO
		
		two_axis_changed.emit(new_value)
		
		_two_axis_release(two_axis_name)
		
		return
		
	_last_value.x = new_value.x
	_last_value.y = new_value.y
			
	two_axis_changed.emit(new_value)


func _two_axis_release(two_axis_name : String):
	if two_axis_name != control_name:
		return
	
	if _click_check:
		two_axis_clicked.emit()
		_click_check = false
		_click_timer.stop()
		
		if toggle_on_click:
			_toggle_click()
			
	two_axis_released.emit()


func _three_axis_press(three_axis_name : String):
	if three_axis_name != control_name:
		return
	
	three_axis_pressed.emit()
	if _click_check:
		_end_click_check()
		
	_click_check = true
	_click_timer.start()


func _three_axis_change(three_axis_name : String, new_value : Vector3):
	if three_axis_name != control_name:
		return
	
	new_value.x = _apply_deadzone(new_value.x)
	new_value.y = _apply_deadzone(new_value.y)
	new_value.z = _apply_deadzone(new_value.z)
	
	if _last_value == Vector3.ZERO:
		if new_value == Vector3.ZERO:
			return
			
		_last_value = new_value
		
		_three_axis_press(three_axis_name)
		
		three_axis_changed.emit(new_value)
		return
	
	elif new_value == Vector3.ZERO:
		_last_value = Vector3.ZERO
		
		_three_axis_release(three_axis_name)
		return
		
	_last_value = new_value
		
	three_axis_changed.emit(new_value)


func _three_axis_release(three_axis_name : String):
	if three_axis_name != control_name:
		return
	
	if _click_check:
		three_axis_clicked.emit()
		_click_check = false
		_click_timer.stop()
		
		if toggle_on_click:
			_toggle_click()
			
	three_axis_released.emit()


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
