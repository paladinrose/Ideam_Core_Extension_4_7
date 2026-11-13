class_name Conversation
extends Node

# SIGNALS #################################################

signal conversation_begun
signal conversation_concluded
#signal conversation_suspended

signal interlocutor_joined (interlocutor : Interlocutor)
signal interlocutor_left (interlocutor : Interlocutor)


# ENUMS ###################################################

# CONSTANTS ###############################################

# EXPORTS #################################################

@export var interlocutors : Array[Interlocutor]


# VARIABLES ###############################################

# ONREADY VARIABLES #######################################

# GODOT FUNCTIONS #########################################

#func _ready():
#	pass 

#func _process(delta ):
#   pass


# CLASS FUNCTIONS #########################################

func begin():
	conversation_begun.emit()


func introduce_interlocutor(new_interlocutor : Interlocutor):
	if interlocutors.has(new_interlocutor):
		return
	
	interlocutors.append(new_interlocutor)
	interlocutor_joined.emit(new_interlocutor)


func dismiss_interlocutor(interlocutor : Interlocutor):
	var id : int = interlocutors.find(interlocutor)
	if id < 0:
		return
	
	dismiss_interlocutor_at(id)


func dismiss_interlocutor_at(id : int):
	if id < 0 or id >= interlocutors.size():
		return
	
	var interlocutor = interlocutors[id]
	interlocutors.remove_at(id)
	
	interlocutor_left.emit(interlocutor)


func conclude():
	conversation_concluded.emit()


# STATIC FUNCTIONS ########################################

# FUNCTION OVERRIDES ######################################

# INTERNAL CLASSES ########################################
