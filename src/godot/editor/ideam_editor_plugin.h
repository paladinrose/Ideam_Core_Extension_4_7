#pragma once

#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <string_view>

namespace ideam::godot_ext {

/**
 * @class IdeamEditorPlugin
 * @brief Abstract base class for the Ideam plugin ecosystem.
 * Handles filesystem standardization, script template provisioning, and cross-plugin registry discovery.
 */
class IdeamEditorPlugin : public godot::EditorPlugin {
	GDCLASS(IdeamEditorPlugin, godot::EditorPlugin)

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

public:
	IdeamEditorPlugin() = default;
	~IdeamEditorPlugin() = default;

	// --- Plugin Lifecycle & Lateral Handshake ---

	/**
	 * @brief Broadcasts the active state of this plugin to the ecosystem.
	 */
	void set_plugin_active(const godot::String &p_plugin_name, bool p_active);

	/**
	 * @brief Checks if another plugin in the Ideam ecosystem is currently active.
	 */
	bool is_plugin_active(const godot::String &p_plugin_name) const;

	/**
	 * @brief Returns all plugin names currently marked active (`true`) in `[ActivePlugins]`.
	 */
	godot::TypedArray<godot::String> get_active_plugins() const;

	// --- Universal Ecosystem Registry API ---
	// Exposed to GDScript so standalone plugins can inject their tools and manifests.

	/**
	 * @brief Writes a value to the universal .ideam_registry.cfg
	 * Use this to inject tools into the Project Workspace or register file paths.
	 */
	void register_to_ecosystem(const godot::String &p_section, const godot::String &p_key, const godot::Variant &p_value);

	/**
	 * @brief Removes a key from a section in .ideam_registry.cfg (if present).
	 */
	void unregister_from_ecosystem(const godot::String &p_section, const godot::String &p_key);

	/**
	 * @brief Retrieves a value from the registry.
	 */
	godot::Variant get_from_ecosystem(const godot::String &p_section, const godot::String &p_key, const godot::Variant &p_default = godot::Variant()) const;

	/**
	 * @brief Retrieves all keys registered under a specific section (e.g., "WizardSettings").
	 */
	godot::TypedArray<godot::String> get_ecosystem_keys(const godot::String &p_section) const;

	/**
	 * @brief Retrieves a dictionary of all Key-Value pairs in a section. Useful for building UI lists.
	 */
	godot::Dictionary get_ecosystem_section(const godot::String &p_section) const;

	// --- Project Tools Ecosystem Helpers (Tier 2+ Bridge) ---

	/**
	 * @brief Registers a typed ProjectModuleManifest (.tres) path for this plugin under `[WizardSettings]`.
	 */
	void register_wizard_manifest(const godot::String &p_plugin_name, const godot::String &p_manifest_path);

	/**
	 * @brief Registers a script template for `p_base_class` under `[ScriptTemplates]` and ensures
	 * `res://script_templates/<p_base_class>/<p_template_name>.gd` exists when `p_source_path` is readable.
	 */
	bool register_script_template(
		const godot::String &p_base_class,
		const godot::String &p_template_name,
		const godot::String &p_source_path
	);
};

} // namespace ideam::godot_ext
