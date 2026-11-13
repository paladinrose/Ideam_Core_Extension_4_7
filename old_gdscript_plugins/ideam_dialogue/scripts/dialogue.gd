class_name Dialogue
extends Node

# SIGNALS #################################################

# ENUMS ###################################################

# CONSTANTS ###############################################

# EXPORTS #################################################

@export var plain_text : String
@export var voiced_text : Voiced_Text

@export var ease : Tween.EaseType
@export var transition : Tween.TransitionType

## The number of characters, per second, to reveal
@export var rate : float = 10


# VARIABLES ###############################################

# ONREADY VARIABLES #######################################

# GODOT FUNCTIONS #########################################

# CLASS FUNCTIONS #########################################

func get_text() -> String:
	if voiced_text:
		return voiced_text.text
	else:
		return plain_text


# STATIC FUNCTIONS ########################################

# FUNCTION OVERRIDES ######################################

# INTERNAL CLASSES ########################################
