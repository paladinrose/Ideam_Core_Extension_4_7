class_name Alignment3D_Sensor
extends Ideam_Sensor3D


#region SIGNALS #################################################

signal entered_alignment
signal lost_alignment

#endregion


#region ENUMS ###################################################

#endregion


#region CONSTANTS ###############################################

#endregion


#region EXPORTS #################################################

@export var targets : Array[Node3D]

@export var alignment_tolerance : float = 0.1

#endregion


#region VARIABLES ###############################################
var _square_tolerance : float

var _in_alignment : bool = false

var _last_positions : Array[Vector3]

#endregion


#region ONREADY VARIABLES #######################################

#endregion


#region STATIC VARIABLES ########################################

#endregion


#region GODOT FUNCTIONS #########################################

func _ready() -> void:
	super()
	_square_tolerance = alignment_tolerance * alignment_tolerance
	
	var s = targets.size()
	if s < 3:
		return
	
	_last_positions = []
	_last_positions.resize(s)
	for i in range(s):
		_last_positions[i] = targets[i].global_position


func _process(delta : float) -> void:
	if enabled:
		sense()


#endregion


#region CLASS FUNCTIONS #########################################

#endregion


#region STATIC FUNCTIONS ########################################
func sense():
	var s : int = targets.size()
	if s < 3:
		#enabled = false
		return
	
	var do_update : bool = false
	
	for i in range(s):
		if targets[i].global_position != _last_positions[i]:
			do_update = true
			break
	
	if not do_update:
		return
	
	var farthest_distance : float = 0.0
	
	var farthest_a : int = -1
	var farthest_b : int = -1
	
	for i in range(s-1):
		var a : Node3D = targets[i]
		_last_positions[i] = a.global_position
		
		for j in range(i+1, s):
			var b : Node3D = targets[j]
			
			var distance = a.global_position.distance_squared_to(b.global_position)
			if distance > farthest_distance:
				farthest_distance = distance
				farthest_a = i
				farthest_b = j
	
	_last_positions[s-1] = targets[s-1].global_position
	
	var a : Node3D = targets[farthest_a]
	var b : Node3D = targets[farthest_b]
	
	var line_a : Vector3 = a.global_position
	var line_b : Vector3 = b.global_position
	var line_mid : Vector3 = (line_a + line_b) * 0.5
	
	var in_alignment : bool = true
	
	for i in range(s):
		if i == farthest_a or i == farthest_b:
			continue
		
		var pi : Vector3 = targets[i].global_position
		
		var pa : Vector3 = Vector3(line_a)
		var pa_dist : float = pa.distance_squared_to(pi)
		
		var pb : Vector3 = Vector3(line_b)
		var pb_dist : float = pb.distance_squared_to(pi)
		
		var pm : Vector3 = Vector3(line_mid)
		var pm_dist : float = pm.distance_squared_to(pi)
		
		
		if pa_dist <= pb_dist:
			pb_dist = pm_dist
			pb = pm
		
		else:
			pa_dist = pm_dist
			pa = pm
			
		for j in range(5):
			pm = (pa + pb) * 0.5
			pm_dist = pm.distance_squared_to(pi)
			
			if pa_dist <= pb_dist:
				pb_dist = pm_dist
				pb = pm
				
			else:
				pa_dist = pm_dist
				pa = pm
				
		var cp : Vector3
		
		if pa_dist < pm_dist:
			if pb_dist < pa_dist:
				cp = pb
			else:
				cp = pa
		elif pb_dist < pm_dist:
			cp = pb
		
		else:
			cp = pm
		
		if cp.distance_squared_to(pi) > _square_tolerance:
			in_alignment = false
		
		if in_alignment != _in_alignment:
			_in_alignment = in_alignment
			
			if _in_alignment:
				entered_alignment.emit()
			else:
				lost_alignment.emit()
		
#endregion


#region FUNCTION OVERRIDES ######################################

#endregion


#region INTERNAL CLASSES ########################################

#endregion
