@tool
class_name Ideam_Dialogue
extends EditorPlugin

const DIALOGUE_SETTINGS_PATH = "res://addons/ideam_dialogue/resources/project_dialogue_settings.res"
const SETTINGS_PATHS =  "res://addons/ideam_project_tools/resources/project_wizard_settings_paths.txt"

func _enter_tree():
	validate_script_templates()
	
	check_for_project_tools()


func _exit_tree():
	pass


func check_for_project_tools():
	if ResourceLoader.exists(DIALOGUE_SETTINGS_PATH) and DirAccess.dir_exists_absolute("res://addons/ideam_project_tools"):
		var settings_paths : Array[String] = []
		
		var paths_file = FileAccess.open(SETTINGS_PATHS, FileAccess.READ)
		
		var has_settings : bool = false
		
		while paths_file.get_position() < paths_file.get_length():
			var path : String = paths_file.get_line()
			
			if path == DIALOGUE_SETTINGS_PATH: 
				has_settings = true
				
			settings_paths.append(path)
			
		paths_file.close()
		
		if not has_settings: 
			settings_paths.append(DIALOGUE_SETTINGS_PATH)
			
			paths_file = FileAccess.open(SETTINGS_PATHS, FileAccess.WRITE)
			
			for path in settings_paths:
				paths_file.store_line(path)
				
			paths_file.close()


func validate_script_templates():
	var path = "res://script_templates"
	
	if not DirAccess.dir_exists_absolute(path):
		print("Creating Script Templates directory.")
		DirAccess.make_dir_absolute(path)
	
	var dir = DirAccess.open(path)
	
	if not dir.file_exists(".gdignore"):
		
		var file_path = path + "/.gdignore"
		print("Creating " + file_path)
		var gdignore = FileAccess.open(file_path, FileAccess.WRITE)
		
		gdignore.close()
