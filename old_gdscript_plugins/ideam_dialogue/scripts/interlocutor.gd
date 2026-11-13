class_name Interlocutor
extends Node

# SIGNALS #################################################

signal started_communicating
signal communication_updated
signal stopped_communicating
signal interrupted


# ENUMS ###################################################

# CONSTANTS ###############################################

# EXPORTS #################################################

@export var active_conversations : Array[Conversation]
@export var dialogue_boxes : Array[Dialogue_Box]

@export var default_speaking_rate : int = 10


# VARIABLES ###############################################

# ONREADY VARIABLES #######################################

# GODOT FUNCTIONS #########################################

#func _ready():
#	pass 

#func _process(delta ):
#   pass


# CLASS FUNCTIONS #########################################

func begin_conversation(with : Interlocutor, opening : String):
	var new_conversation : Conversation = Conversation.new()
	add_child(new_conversation)
	join_conversation(new_conversation)
	
	var convo_id : int = _get_conversation_id(new_conversation)
	var box_id : int = _get_available_dialogue_box(convo_id)
	
	start_communicating(convo_id, box_id, opening)
	with.join_conversation(new_conversation)


func join_conversation(new_conversation : Conversation):
	if active_conversations.has(new_conversation):
		return
	
	new_conversation.introduce_interlocutor(self)
	active_conversations.append(new_conversation)


func leave_conversation(conversation : Conversation):
	var id : int = _get_conversation_id(conversation)
	if id < 0:
		return
	
	conversation.dismiss_interlocutor(self)


func start_communicating(conversation_id : int, dialogue_box_id : int, text : String):
	var conversation : Conversation = active_conversations[conversation_id]
	
	var dialogue : Dialogue = Dialogue.new()
	dialogue.rate = default_speaking_rate
	dialogue.plain_text = text
	
	var box : Dialogue_Box = dialogue_boxes[dialogue_box_id]
	box._interlocutor_id = dialogue_box_id
	box.open_dialogue_box()
	box.display_dialogue(dialogue)
	
	started_communicating.emit()


func update_communication(dialogue_box_id : int):
	communication_updated.emit()


func stop_communicating(dialogue_box_id : int):
	var box : Dialogue_Box = dialogue_boxes[dialogue_box_id]
	
	stopped_communicating.emit()



func _get_conversation_id(conversation : Conversation) -> int:
	return active_conversations.find(conversation)


func _get_available_dialogue_box(conversation_id : int) -> int:
	for i in range(dialogue_boxes.size()):
		if dialogue_boxes[i].current_dialogue == null:
			var box  : Dialogue_Box = dialogue_boxes[i]
			if not box.dialogue_updated.is_connected(update_communication):
				box.dialogue_updated.connect(update_communication)
				
			return i
	
	return -1


# STATIC FUNCTIONS ########################################

# FUNCTION OVERRIDES ######################################

# INTERNAL CLASSES ########################################
