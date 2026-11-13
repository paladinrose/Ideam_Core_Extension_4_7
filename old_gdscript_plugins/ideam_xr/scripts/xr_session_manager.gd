class_name XR_Session_Manager
extends Node3D

#region SIGNALS #################################################

signal xr_started
signal xr_enabled
signal xr_failed
signal xr_disabled
signal xr_stopped

signal entered_immersive_mode
signal entered_windowed_mode
 
signal focus_lost
signal focus_gained

signal pose_recentered


#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var maximum_refresh_rate : int = 90

@export var xr_is_enabled : bool:
	set(enabled):
		if not initialized:
			xr_is_enabled = enabled
			return
		
		if enabled == xr_is_enabled:
			return
		
		xr_is_enabled = enabled
		
		if enabled: 
			enable_xr()
		else:
			disable_xr()


#endregion


#region VARIABLES ###############################################

var initialized : bool

var xr_interface: XRInterface

var original_vsync_mode : DisplayServer.VSyncMode

var xr_is_focussed : bool = false

var xr_is_started : bool = false

var data : Dictionary = {}

var hybrid_app_mode := OpenXRHybridApp.HYBRID_MODE_NONE


#endregion


#region ONREADY VARIABLES #######################################

#endregion

#region Static VARIABLES #######################################

static var default_session_manager : XR_Session_Manager

#endregion

#region GODOT FUNCTIONS #########################################

func _ready():
	initialized = true
	xr_interface = XRServer.find_interface("OpenXR")
	
	hybrid_app_mode = OpenXRHybridApp.get_mode()
	
	if xr_is_enabled:
		enable_xr()
		entered_immersive_mode.emit()
	
	else:
		entered_windowed_mode.emit()
		
		#var data_string = JSON.stringify(data)
		#var success = OpenXRHybridApp.switch_mode(OpenXRHybridApp.HYBRID_MODE_PANEL, data_string)

		#if not success:
		#	print("Unable to toggle hybrid app mode.")



#endregion


#region CLASS FUNCTIONS #########################################

func toggle_xr(toggled : bool):
	xr_is_enabled = toggled


func enable_xr():
	var data_string: String = OpenXRHybridApp.get_launch_data()
	if data_string != "":
		var data: Dictionary = JSON.parse_string(data_string)
		print("Hybrid App Launch Data: ", data)


	if xr_interface and xr_interface.is_initialized():
		
		# Enable passthrough
		xr_interface.environment_blend_mode = XRInterface.XR_ENV_BLEND_MODE_ALPHA_BLEND
		get_viewport().transparent_bg = true

		var viewport : Viewport = get_viewport()
		
		original_vsync_mode = DisplayServer.window_get_vsync_mode()
		
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
		
		viewport.use_xr = true
		
		if RenderingServer.get_rendering_device():
			viewport.vrs_mode = Viewport.VRS_XR
		elif int(ProjectSettings.get_setting("xr/openxr/foveation_level")) == 0:
			push_warning("OpenXR: Recommend setting Foveation level to High in Project Settings")

		# Connect the OpenXR events
		xr_interface.session_begun.connect(_on_openxr_session_begun)
		xr_interface.session_visible.connect(_on_openxr_visible_state)
		xr_interface.session_focussed.connect(_on_openxr_focused_state)
		xr_interface.session_stopping.connect(_on_openxr_stopping)
		xr_interface.pose_recentered.connect(_on_openxr_pose_recentered)
		
		xr_enabled.emit()
		
		#print("XR enabled")
		
	else:
		xr_failed.emit()
		
		print("XR failed")


func disable_xr():
	DisplayServer.window_set_vsync_mode(original_vsync_mode)
	get_viewport().use_xr = false
		
	xr_disabled.emit()
	
	#print("XR disabled")

func _switch_hybrid_mode()-> void:
	var data_string := JSON.stringify(data)

	var success := false

	print("Attempting to toggle hybrid app mode with data: ", data_string)

	if hybrid_app_mode == OpenXRHybridApp.HYBRID_MODE_IMMERSIVE:
		success = OpenXRHybridApp.switch_mode(OpenXRHybridApp.HYBRID_MODE_PANEL, data_string)
		
	elif hybrid_app_mode == OpenXRHybridApp.HYBRID_MODE_PANEL:
		success = OpenXRHybridApp.switch_mode(OpenXRHybridApp.HYBRID_MODE_IMMERSIVE, data_string)

	if not success:
		print("Unable to toggle hybrid app mode.")


func _on_openxr_session_begun() -> void:
	# Get the reported refresh rate
	var current_refresh_rate = xr_interface.get_display_refresh_rate()
	if current_refresh_rate > 0:
		print("OpenXR: Refresh rate reported as ", str(current_refresh_rate))
	else:
		print("OpenXR: No refresh rate given by XR runtime")
	
	# See if we have a better refresh rate available
	var new_rate = current_refresh_rate
	var available_rates : Array = xr_interface.get_available_display_refresh_rates()
	if available_rates.size() == 0:
		print("OpenXR: Target does not support refresh rate extension")
	elif available_rates.size() == 1:
		# Only one available, so use it
		new_rate = available_rates[0]
	else:
		for rate in available_rates:
			if rate > new_rate and rate <= maximum_refresh_rate:
				new_rate = rate
	
	# Did we find a better rate?
	if current_refresh_rate != new_rate:
		print("OpenXR: Setting refresh rate to ", str(new_rate))
		xr_interface.set_display_refresh_rate(new_rate)
		current_refresh_rate = new_rate
	
	# Now match our physics rate
	Engine.physics_ticks_per_second = current_refresh_rate
	
	xr_is_started = true
	if not default_session_manager:
		default_session_manager = self
		
	xr_started.emit()


func _on_openxr_visible_state() -> void:
	# We always pass this state at startup,
	# but the second time we get this it means our player took off their headset
	if xr_is_focussed:
		#print("OpenXR lost focus")
		
		xr_is_focussed = false
		
		# pause our game
		#get_tree().paused = true
		
		focus_lost.emit()


func _on_openxr_focused_state() -> void:
	#print("OpenXR gained focus")
	xr_is_focussed = true

	# unpause our game
	#get_tree().paused = false

	focus_gained.emit()


func _on_openxr_stopping() -> void:
	# Our session is being stopped.
	print("OpenXR is stopping")
	xr_is_started = false
	xr_stopped.emit()


func _on_openxr_pose_recentered() -> void:
	# User recentered view, we have to react to this by recentering the view.
	# This is game implementation dependent.
	pose_recentered.emit()


#endregion


#region STATIC FUNCTIONS ########################################

#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
