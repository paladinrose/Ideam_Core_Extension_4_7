class_name SSML_Parser
extends XMLParser

# SIGNALS #################################################

# ENUMS ###################################################

# CONSTANTS ###############################################

# EXPORTS #################################################

# VARIABLES ###############################################

# ONREADY VARIABLES #######################################

# GODOT FUNCTIONS #########################################

#func _ready():
#	pass 

#func _process(delta ):
#   pass


# CLASS FUNCTIONS #########################################

func parse_to_voiced_text(ssml : String)->Voiced_Text:
	var voiced_text : Voiced_Text = Voiced_Text.new()
	#parse
	return voiced_text

func save_to_ssml(voiced_text : Voiced_Text) -> String:
	var ssml : String = ""
	#save
	return ssml

# STATIC FUNCTIONS ########################################

# FUNCTION OVERRIDES ######################################

# INTERNAL CLASSES ########################################
