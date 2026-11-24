#pragma once

#include "../gizmos/ideam_gizmo.h"
#include "../gizmos/ideam_gizmo_backend.h"
#include "../tools/tool_context.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <string_view>
#include <vector>

namespace ideam::godot_ext {

/**
 * @class IdeamEditorPlugin
 * @brief Abstract base class for the Ideam plugin ecosystem.
 * Handles filesystem standardization, script template provisioning, cross-plugin registry discovery,
 * shared `ToolContext` lifecycle, trans-environment `IdeamGizmo` registration, and
 * `CONTAINER_SPATIAL_EDITOR_MENU` toolbar mounting with automatic cleanup.
 */
class IdeamEditorPlugin : public godot::EditorPlugin {
	GDCLASS(IdeamEditorPlugin, godot::EditorPlugin)

private:
	godot::Ref<ToolContext> plugin_tool_context_;
#ifdef TOOLS_ENABLED
	std::vector<godot::Ref<IdeamEditorGizmoAdapter>> registered_gizmo_adapters_;
#endif
	std::vector<godot::Control *> registered_spatial_toolbars_;

protected:
	static void _bind_methods();

	// C++26 Static Constants
	static constexpr std::string_view SCRIPT_TEMPLATE_PATH = "res://script_templates/";
	static constexpr std::string_view GDIGNORE_FILENAME = ".gdignore";
	static constexpr std::string_view REGISTRY_PATH = "res://.ideam_registry.cfg";

	/**
	 * @brief Ensures the project has a valid, ignored script_templates directory.
	 */
	void validate_script_templates();

	/**
	 * @brief Unregisters all `IdeamGizmo` adapters and spatial toolbars registered by this plugin.
	 * Automatically invoked in `_exit_tree()` (subclasses overriding `_exit_tree()` should call
	 * `IdeamEditorPlugin::_exit_tree()` or `cleanup_ideam_spatial_registrations()`).
	 */
	void cleanup_ideam_spatial_registrations();

public:
	IdeamEditorPlugin() = default;
	~IdeamEditorPlugin() override = default;

	void _exit_tree() override;

	// --- Shared Plugin ToolContext ---
	[[nodiscard]] godot::Ref<ToolContext> get_or_create_plugin_tool_context();

	// --- Trans-Environment Gizmo & Spatial Toolbar Registration ---
	void register_ideam_gizmo(const godot::Ref<IdeamGizmo> &p_gizmo);
	void unregister_ideam_gizmo(const godot::Ref<IdeamGizmo> &p_gizmo);

	void register_spatial_toolbar(godot::Control *p_toolbar);
	void unregister_spatial_toolbar(godot::Control *p_toolbar);

	int32_t forward_gizmos_3d_gui_input(
		godot::Camera3D *p_camera,
		const godot::Ref<godot::InputEvent> &p_event,
		godot::Node3D *p_selected_node
	);

	// --- Plugin Lifecycle & Lateral Handshake ---
	void set_plugin_active(const godot::String &p_plugin_name, bool p_active);
	bool is_plugin_active(const godot::String &p_plugin_name) const;
	godot::TypedArray<godot::String> get_active_plugins() const;

	// --- Universal Ecosystem Registry API ---
	void register_to_ecosystem(const godot::String &p_section, const godot::String &p_key, const godot::Variant &p_value);
	void unregister_from_ecosystem(const godot::String &p_section, const godot::String &p_key);
	godot::Variant get_from_ecosystem(const godot::String &p_section, const godot::String &p_key, const godot::Variant &p_default = godot::Variant()) const;
	godot::TypedArray<godot::String> get_ecosystem_keys(const godot::String &p_section) const;
	godot::Dictionary get_ecosystem_section(const godot::String &p_section) const;

	// --- Project Tools Ecosystem Helpers (Tier 2+ Bridge) ---
	void register_wizard_manifest(const godot::String &p_plugin_name, const godot::String &p_manifest_path);
	bool register_script_template(
		const godot::String &p_base_class,
		const godot::String &p_template_name,
		const godot::String &p_source_path
	);
};

} // namespace ideam::godot_ext
