@tool
class_name Dialogue_Box
extends Node

# SIGNALS #################################################

signal box_opened
signal box_closed

signal dialogue_started (id : int)
signal dialogue_updated (id : int)
signal dialogue_completed (id : int)


# ENUMS ###################################################

#I'm making the decision to leave this enum capitalized as is.
#This is because each option is ALSO the class name of the
#label type in question. I want to directly reflect that.
enum Text_Class {
	Label,
	RichTextLabel,
	DynamicRichText,
	Label3D, 
	
}


# CONSTANTS ###############################################

# EXPORTS #################################################

@export var label_type : Text_Class:
	get:
		return label_type
	set(new_type):
		if new_type == label_type: return
		
		label_type = new_type
		if label: label.queue_free()
		await get_tree().process_frame
		create_label()


@export var label_parent : Node


@export var main_text_type : Text_Class:
	get:
		return main_text_type
	set(new_type):
		if new_type == main_text_type: return
		
		main_text_type = new_type
		if label: label.queue_free()
		await get_tree().process_frame
		create_main_text()


@export var main_text_parent : Node


# VARIABLES ###############################################

var label 
var main_text 
var current_dialogue : Dialogue
var dialogue_tween : Tween

var _interlocutor_id : int

# ONREADY VARIABLES #######################################

# GODOT FUNCTIONS #########################################

func _ready():
	validate_label()
	validate_main_text()


# CLASS FUNCTIONS #########################################

func validate_label():
	var l_p : Node = self
	if label_parent : 
		l_p = label_parent
		
	if l_p.has_node("Label"):
		label = l_p.get_node("Label")
		
		if label is Label:
			label_type = Text_Class.Label
			
		elif label is RichTextLabel:
			label_type = Text_Class.RichTextLabel
			
		elif label is DynamicRichText:
			label_type = Text_Class.DynamicRichText
			
		elif label is ExpandedLabel3D:
			label_type = Text_Class.Label3D
			
		return
		
	create_label()


func validate_main_text():
	var mt_p : Node = self
	
	if main_text_parent : 
		mt_p = main_text_parent
		
	if mt_p.has_node("Main_Text"):
		main_text = mt_p.get_node("Main_Text")
		
		if main_text is Label:
			main_text_type = Text_Class.Label
			
		elif main_text is RichTextLabel:
			main_text_type = Text_Class.RichTextLabel
			
		elif main_text is DynamicRichText:
			main_text_type = Text_Class.DynamicRichText
			
		elif main_text is ExpandedLabel3D:
			main_text_type = Text_Class.Label3D
			
		return
		
	create_main_text()


func create_label():
	match label_type:
		Text_Class.Label:
			label = Label.new()
			
		Text_Class.RichTextLabel:
			label = RichTextLabel.new()
			
		Text_Class.DynamicRichText:
			label = DynamicRichText.new()
			
		Text_Class.Label3D:
			label = ExpandedLabel3D.new()
			
	
	if label_parent:
		label_parent.add_child(label)
		
	else:
		add_child(label)
		
	label.owner = get_tree().edited_scene_root
	label.name = "Label"


func create_main_text():
	match main_text_type:
		Text_Class.Label:
			main_text = Label.new()
			
		Text_Class.RichTextLabel:
			main_text = RichTextLabel.new()
			
		Text_Class.DynamicRichText:
			main_text = DynamicRichText.new()
			
		Text_Class.Label3D:
			main_text = ExpandedLabel3D.new()
			
	
	if main_text_parent:
		main_text_parent.add_child(main_text)
		
	else:
		add_child(main_text)
		
	main_text.owner = get_tree().edited_scene_root
	main_text.name = "Main_Text"


func open_dialogue_box():
	if not label:
		validate_label()
		
	if label is Control:
		label.show()
		
	else:
		label.visible = true
	
	if not main_text:
		validate_main_text()
		
	if main_text is Control:
		main_text.show()
		
	else:
		main_text.visible = true
	
	box_opened.emit()


func close_dialogue_box():
	label.text = ""
	
	if label is Control:
		label.hide()
		
	else:
		label.visible = false
	
	main_text.text = ""
	main_text.visible_ratio = 0
	
	if main_text is Control:
		main_text.hide()
		
	else:
		main_text.visible = false
		
	box_closed.emit()


func display_dialogue(dialogue : Dialogue):
	
	main_text.visible_ratio = 0.0
	main_text.text = dialogue.get_text()
	
	var time : float = main_text.text.length() / dialogue.rate
	
	dialogue_tween = create_tween() \
	.set_ease(dialogue.ease) \
	.set_trans(dialogue.transition)
	
	current_dialogue = dialogue
	
	dialogue_tween.tween_method(update_dialogue, 0.0, 1.0, time)
	 
	dialogue_started.emit(_interlocutor_id)


func update_dialogue(percent : float):
	if percent == 1.0:
		complete_dialogue()
		return
		
	main_text.visible_ratio = percent
	
	dialogue_updated.emit(_interlocutor_id)


func stop_dialogue():
	dialogue_tween.kill()


func complete_dialogue():
	main_text.visible_ratio = 1.0
	
	dialogue_completed.emit(_interlocutor_id)


func fast_track_dialogue():
	stop_dialogue()
	
	complete_dialogue()


# STATIC FUNCTIONS ########################################

# FUNCTION OVERRIDES ######################################

# INTERNAL CLASSES ########################################
