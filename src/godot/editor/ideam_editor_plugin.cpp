#include "ideam_editor_plugin.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

using namespace godot;

namespace ideam::godot_ext {

void IdeamEditorPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("validate_script_templates"), &IdeamEditorPlugin::validate_script_templates);
	ClassDB::bind_method(D_METHOD("get_or_create_plugin_tool_context"), &IdeamEditorPlugin::get_or_create_plugin_tool_context);
	ClassDB::bind_method(D_METHOD("register_ideam_gizmo", "gizmo"), &IdeamEditorPlugin::register_ideam_gizmo);
	ClassDB::bind_method(D_METHOD("unregister_ideam_gizmo", "gizmo"), &IdeamEditorPlugin::unregister_ideam_gizmo);
	ClassDB::bind_method(D_METHOD("register_spatial_toolbar", "toolbar"), &IdeamEditorPlugin::register_spatial_toolbar);
	ClassDB::bind_method(D_METHOD("unregister_spatial_toolbar", "toolbar"), &IdeamEditorPlugin::unregister_spatial_toolbar);

	// Handshake API
	ClassDB::bind_method(D_METHOD("set_plugin_active", "plugin_name", "active"), &IdeamEditorPlugin::set_plugin_active);
	ClassDB::bind_method(D_METHOD("is_plugin_active", "plugin_name"), &IdeamEditorPlugin::is_plugin_active);
	ClassDB::bind_method(D_METHOD("get_active_plugins"), &IdeamEditorPlugin::get_active_plugins);

	// Expose the universal registry to GDScript
	ClassDB::bind_method(D_METHOD("register_to_ecosystem", "section", "key", "value"), &IdeamEditorPlugin::register_to_ecosystem);
	ClassDB::bind_method(D_METHOD("unregister_from_ecosystem", "section", "key"), &IdeamEditorPlugin::unregister_from_ecosystem);
	ClassDB::bind_method(D_METHOD("get_from_ecosystem", "section", "key", "default_value"), &IdeamEditorPlugin::get_from_ecosystem, DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("get_ecosystem_keys", "section"), &IdeamEditorPlugin::get_ecosystem_keys);
	ClassDB::bind_method(D_METHOD("get_ecosystem_section", "section"), &IdeamEditorPlugin::get_ecosystem_section);

	// Project Tools ecosystem helpers
	ClassDB::bind_method(D_METHOD("register_wizard_manifest", "plugin_name", "manifest_path"), &IdeamEditorPlugin::register_wizard_manifest);
	ClassDB::bind_method(
		D_METHOD("register_script_template", "base_class", "template_name", "source_path"),
		&IdeamEditorPlugin::register_script_template
	);
}

void IdeamEditorPlugin::_exit_tree() {
	cleanup_ideam_spatial_registrations();
}

Ref<ToolContext> IdeamEditorPlugin::get_or_create_plugin_tool_context() {
	if (plugin_tool_context_.is_null()) {
		plugin_tool_context_.instantiate();
	}
#ifdef TOOLS_ENABLED
	if (EditorUndoRedoManager *ed_ur = get_undo_redo()) {
		plugin_tool_context_->set_editor_undo_redo(ed_ur);
	}
#endif
	return plugin_tool_context_;
}

void IdeamEditorPlugin::register_ideam_gizmo(const Ref<IdeamGizmo> &p_gizmo) {
	if (p_gizmo.is_null()) {
		return;
	}
	if (p_gizmo->get_tool_context().is_null()) {
		p_gizmo->set_tool_context(get_or_create_plugin_tool_context());
	}
#ifdef TOOLS_ENABLED
	for (const auto &adapter : registered_gizmo_adapters_) {
		if (adapter.is_valid() && adapter->get_ideam_gizmo() == p_gizmo) {
			return;
		}
	}
	Ref<IdeamEditorGizmoAdapter> adapter;
	adapter.instantiate();
	adapter->set_ideam_gizmo(p_gizmo);
	add_node_3d_gizmo_plugin(adapter);
	registered_gizmo_adapters_.push_back(adapter);
#endif
}

void IdeamEditorPlugin::unregister_ideam_gizmo(const Ref<IdeamGizmo> &p_gizmo) {
	if (p_gizmo.is_null()) {
		return;
	}
#ifdef TOOLS_ENABLED
	std::erase_if(registered_gizmo_adapters_, [&](const Ref<IdeamEditorGizmoAdapter> &p_adapter) {
		if (p_adapter.is_valid() && p_adapter->get_ideam_gizmo() == p_gizmo) {
			remove_node_3d_gizmo_plugin(p_adapter);
			return true;
		}
		return false;
	});
#endif
}

void IdeamEditorPlugin::register_spatial_toolbar(Control *p_toolbar) {
	if (!p_toolbar) {
		return;
	}
	for (Control *existing : registered_spatial_toolbars_) {
		if (existing == p_toolbar) {
			return;
		}
	}
	add_control_to_container(EditorPlugin::CONTAINER_SPATIAL_EDITOR_MENU, p_toolbar);
	registered_spatial_toolbars_.push_back(p_toolbar);
}

void IdeamEditorPlugin::unregister_spatial_toolbar(Control *p_toolbar) {
	if (!p_toolbar) {
		return;
	}
	auto it = std::find(registered_spatial_toolbars_.begin(), registered_spatial_toolbars_.end(), p_toolbar);
	if (it != registered_spatial_toolbars_.end()) {
		remove_control_from_container(EditorPlugin::CONTAINER_SPATIAL_EDITOR_MENU, p_toolbar);
		registered_spatial_toolbars_.erase(it);
	}
}

void IdeamEditorPlugin::cleanup_ideam_spatial_registrations() {
#ifdef TOOLS_ENABLED
	for (auto &adapter : registered_gizmo_adapters_) {
		if (adapter.is_valid()) {
			remove_node_3d_gizmo_plugin(adapter);
		}
	}
	registered_gizmo_adapters_.clear();
#endif
	for (Control *toolbar : registered_spatial_toolbars_) {
		if (toolbar) {
			remove_control_from_container(EditorPlugin::CONTAINER_SPATIAL_EDITOR_MENU, toolbar);
			memdelete(toolbar);
		}
	}
	registered_spatial_toolbars_.clear();
}

int32_t IdeamEditorPlugin::forward_gizmos_3d_gui_input(
	Camera3D *p_camera,
	const Ref<InputEvent> &p_event,
	Node3D *p_selected_node
) {
#ifdef TOOLS_ENABLED
	for (auto &adapter : registered_gizmo_adapters_) {
		if (adapter.is_valid()) {
			const int32_t res = adapter->forward_3d_gui_input(p_camera, p_event, p_selected_node);
			if (res != EditorPlugin::AFTER_GUI_INPUT_PASS) {
				return res;
			}
		}
	}
#else
	(void)p_camera;
	(void)p_event;
	(void)p_selected_node;
#endif
	return EditorPlugin::AFTER_GUI_INPUT_PASS;
}

void IdeamEditorPlugin::validate_script_templates() {
	Ref<DirAccess> dir = DirAccess::open("res://");
	if (dir.is_null()) {
		return;
	}

	String template_path = String(SCRIPT_TEMPLATE_PATH.data());
	if (!dir->dir_exists(template_path)) {
		Error err = dir->make_dir_recursive(template_path);
		if (err == OK) {
			UtilityFunctions::print("Ideam: Created script templates directory at ", template_path);
		}
	}

	String ignore_path = template_path.path_join(String(GDIGNORE_FILENAME.data()));
	if (!FileAccess::file_exists(ignore_path)) {
		Ref<FileAccess> f = FileAccess::open(ignore_path, FileAccess::WRITE);
		if (f.is_valid()) {
			f->close();
			UtilityFunctions::print("Ideam: Created .gdignore in templates directory.");
		}
	}
}

// --- Plugin Lifecycle & Lateral Handshake ---

void IdeamEditorPlugin::set_plugin_active(const String &p_plugin_name, bool p_active) {
	register_to_ecosystem("ActivePlugins", p_plugin_name, p_active);
}

bool IdeamEditorPlugin::is_plugin_active(const String &p_plugin_name) const {
	return get_from_ecosystem("ActivePlugins", p_plugin_name, false);
}

TypedArray<String> IdeamEditorPlugin::get_active_plugins() const {
	TypedArray<String> active;
	Dictionary section = get_ecosystem_section("ActivePlugins");
	Array keys = section.keys();
	for (int i = 0; i < keys.size(); ++i) {
		const String key = keys[i];
		if (bool(section[key])) {
			active.append(key);
		}
	}
	return active;
}

// --- Universal Ecosystem Registry Implementation ---

void IdeamEditorPlugin::register_to_ecosystem(const String &p_section, const String &p_key, const Variant &p_value) {
	Ref<ConfigFile> config;
	config.instantiate();

	String path = String(REGISTRY_PATH.data());
	if (FileAccess::file_exists(path)) {
		config->load(path);
	}

	config->set_value(p_section, p_key, p_value);

	Error err = config->save(path);
	if (err != OK) {
		UtilityFunctions::printerr("Ideam: Failed to save to ecosystem registry at ", path);
	}
}

void IdeamEditorPlugin::unregister_from_ecosystem(const String &p_section, const String &p_key) {
	Ref<ConfigFile> config;
	config.instantiate();

	String path = String(REGISTRY_PATH.data());
	if (config->load(path) != OK) {
		return;
	}
	if (!config->has_section_key(p_section, p_key)) {
		return;
	}

	config->erase_section_key(p_section, p_key);
	Error err = config->save(path);
	if (err != OK) {
		UtilityFunctions::printerr("Ideam: Failed to update ecosystem registry at ", path);
	}
}

Variant IdeamEditorPlugin::get_from_ecosystem(const String &p_section, const String &p_key, const Variant &p_default) const {
	Ref<ConfigFile> config;
	config.instantiate();

	String path = String(REGISTRY_PATH.data());
	if (config->load(path) == OK) {
		return config->get_value(p_section, p_key, p_default);
	}

	return p_default;
}

TypedArray<String> IdeamEditorPlugin::get_ecosystem_keys(const String &p_section) const {
	Ref<ConfigFile> config;
	config.instantiate();
	TypedArray<String> keys;

	String path = String(REGISTRY_PATH.data());
	if (config->load(path) == OK && config->has_section(p_section)) {
		PackedStringArray section_keys = config->get_section_keys(p_section);
		for (int i = 0; i < section_keys.size(); ++i) {
			keys.append(section_keys[i]);
		}
	}
	return keys;
}

Dictionary IdeamEditorPlugin::get_ecosystem_section(const String &p_section) const {
	Ref<ConfigFile> config;
	config.instantiate();
	Dictionary section_data;

	String path = String(REGISTRY_PATH.data());
	if (config->load(path) == OK && config->has_section(p_section)) {
		PackedStringArray section_keys = config->get_section_keys(p_section);
		for (int i = 0; i < section_keys.size(); ++i) {
			String key = section_keys[i];
			section_data[key] = config->get_value(p_section, key);
		}
	}
	return section_data;
}

// --- Project Tools Ecosystem Helpers ---

void IdeamEditorPlugin::register_wizard_manifest(const String &p_plugin_name, const String &p_manifest_path) {
	if (p_plugin_name.is_empty() || p_manifest_path.is_empty()) {
		return;
	}
	register_to_ecosystem("WizardSettings", p_plugin_name, p_manifest_path);
}

bool IdeamEditorPlugin::register_script_template(
	const String &p_base_class,
	const String &p_template_name,
	const String &p_source_path
) {
	if (p_base_class.is_empty() || p_template_name.is_empty()) {
		return false;
	}

	validate_script_templates();

	String clean_name = p_template_name;
	if (!clean_name.ends_with(".gd")) {
		clean_name += ".gd";
	}

	const String base_dir = String(SCRIPT_TEMPLATE_PATH.data()).path_join(p_base_class);
	if (!DirAccess::dir_exists_absolute(base_dir)) {
		DirAccess::make_dir_recursive_absolute(base_dir);
	}

	const String target_path = base_dir.path_join(clean_name);
	if (!p_source_path.is_empty() && FileAccess::file_exists(p_source_path) && !FileAccess::file_exists(target_path)) {
		Ref<FileAccess> src = FileAccess::open(p_source_path, FileAccess::READ);
		if (src.is_valid()) {
			Ref<FileAccess> dst = FileAccess::open(target_path, FileAccess::WRITE);
			if (dst.is_valid()) {
				dst->store_string(src->get_as_text());
				dst->close();
			}
			src->close();
		}
	}

	const String reg_key = p_base_class + String("/") + clean_name.get_basename();
	register_to_ecosystem("ScriptTemplates", reg_key, target_path);
	return true;
}

} // namespace ideam::godot_ext
