class_name ExpandedLabel3D
extends Label3D

# SIGNALS #################################################

# ENUMS ###################################################

# CONSTANTS ###############################################

# EXPORTS #################################################

@export var visible_ratio : float:
	get:
		return visible_ratio
	set(new_ratio):
		if new_ratio != visible_ratio:
			visible_ratio = new_ratio
			update_visible_ratio()


# VARIABLES ###############################################
var full_text : String


# ONREADY VARIABLES #######################################

# GODOT FUNCTIONS #########################################

#func _ready():
#	pass 

#func _process(delta ):
#   pass


# CLASS FUNCTIONS #########################################

func update_visible_ratio():
	pass


# STATIC FUNCTIONS ########################################

# FUNCTION OVERRIDES ######################################

func set_text(new_text:String):
	if new_text != full_text:
		full_text = new_text
		update_visible_ratio()
		
		

# INTERNAL CLASSES ########################################
