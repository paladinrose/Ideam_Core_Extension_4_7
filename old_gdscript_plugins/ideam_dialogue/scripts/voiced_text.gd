class_name Voiced_Text
extends Resource

# SIGNALS #################################################

# ENUMS ###################################################


# CONSTANTS ###############################################

# EXPORTS #################################################

@export var text : String

@export var pitch : float

@export_group("Voice")

@export var voice_name : String

@export_subgroup("Prosody and Style")

@export var gender : int
@export var age: int
@export var variant : int
@export var desired_languages : Array[String]
@export var volume : int
@export var utterance_id: int
@export var rate : float
@export var interrupt : bool


# VARIABLES ###############################################

# ONREADY VARIABLES #######################################

# GODOT FUNCTIONS #########################################

# CLASS FUNCTIONS #########################################

func speak():
	DisplayServer.tts_speak(text, voice_name, volume, pitch, rate, utterance_id, interrupt)

	
# STATIC FUNCTIONS ########################################

# FUNCTION OVERRIDES ######################################

# INTERNAL CLASSES ########################################
