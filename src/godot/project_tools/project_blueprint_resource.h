#pragma once

/**
 * project_blueprint_resource.h
 * Typed C++26 Resource schema for Ideam Project Tools (Phase 1).
 *
 * Replaces untyped PackedDataContainer dictionaries with inspectable, serializable Godot
 * Resources that support:
 *   - Folder hierarchies
 *   - Script generation from templates (with custom token substitution)
 *   - Typed Resource instantiation & property wiring (including @ref: cross-references)
 *   - Recursive Scene trees (proper owner assignment & optional generated-script attachment)
 *   - ProjectSettings / Autoload patches
 *   - Dictionary/JSON schema round-tripping for Tier 3 overlays, Tier 4 workflows, and Tier 5 AI tools.
 */

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

// ============================================================================
// 1. ProjectScriptSpec
// ============================================================================

class ProjectScriptSpec : public godot::Resource {
    GDCLASS(ProjectScriptSpec, godot::Resource)

private:
    godot::String target_folder_;
    godot::String script_name_;
    godot::String base_class_ = "Node";
    godot::String class_name_override_;
    godot::String template_name_ = "default";
    godot::Dictionary template_tokens_;
    bool enabled_ = true;

protected:
    static void _bind_methods();

public:
    ProjectScriptSpec() = default;
    ~ProjectScriptSpec() override = default;

    void set_target_folder(const godot::String &p_folder) { target_folder_ = p_folder; emit_changed(); }
    [[nodiscard]] godot::String get_target_folder() const { return target_folder_; }

    void set_script_name(const godot::String &p_name) { script_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_script_name() const { return script_name_; }

    void set_base_class(const godot::String &p_class) { base_class_ = p_class; emit_changed(); }
    [[nodiscard]] godot::String get_base_class() const { return base_class_; }

    void set_class_name_override(const godot::String &p_override) { class_name_override_ = p_override; emit_changed(); }
    [[nodiscard]] godot::String get_class_name_override() const { return class_name_override_; }

    void set_template_name(const godot::String &p_template) { template_name_ = p_template; emit_changed(); }
    [[nodiscard]] godot::String get_template_name() const { return template_name_; }

    void set_template_tokens(const godot::Dictionary &p_tokens) { template_tokens_ = p_tokens; emit_changed(); }
    [[nodiscard]] godot::Dictionary get_template_tokens() const { return template_tokens_; }

    void set_enabled(bool p_enabled) { enabled_ = p_enabled; emit_changed(); }
    [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }

    [[nodiscard]] godot::String get_effective_class_name() const;
    [[nodiscard]] godot::Dictionary to_plan_dictionary() const;
    static godot::Ref<ProjectScriptSpec> from_plan_dictionary(const godot::Dictionary &p_dict);
};

// ============================================================================
// 2. ProjectResourceSpec
// ============================================================================

class ProjectResourceSpec : public godot::Resource {
    GDCLASS(ProjectResourceSpec, godot::Resource)

private:
    godot::String target_folder_;
    godot::String resource_name_;
    godot::String resource_class_ = "Resource";
    godot::String file_extension_ = "tres";
    bool use_generated_script_    = false;
    godot::String generated_script_key_;
    godot::Dictionary properties_;
    bool enabled_ = true;

protected:
    static void _bind_methods();

public:
    ProjectResourceSpec() = default;
    ~ProjectResourceSpec() override = default;

    void set_target_folder(const godot::String &p_folder) { target_folder_ = p_folder; emit_changed(); }
    [[nodiscard]] godot::String get_target_folder() const { return target_folder_; }

    void set_resource_name(const godot::String &p_name) { resource_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_resource_name() const { return resource_name_; }

    void set_resource_class(const godot::String &p_class) { resource_class_ = p_class; emit_changed(); }
    [[nodiscard]] godot::String get_resource_class() const { return resource_class_; }

    void set_file_extension(const godot::String &p_ext) { file_extension_ = p_ext; emit_changed(); }
    [[nodiscard]] godot::String get_file_extension() const { return file_extension_; }

    void set_use_generated_script(bool p_use) { use_generated_script_ = p_use; emit_changed(); }
    [[nodiscard]] bool get_use_generated_script() const noexcept { return use_generated_script_; }

    void set_generated_script_key(const godot::String &p_key) { generated_script_key_ = p_key; emit_changed(); }
    [[nodiscard]] godot::String get_generated_script_key() const { return generated_script_key_; }

    void set_properties(const godot::Dictionary &p_props) { properties_ = p_props; emit_changed(); }
    [[nodiscard]] godot::Dictionary get_properties() const { return properties_; }

    void set_enabled(bool p_enabled) { enabled_ = p_enabled; emit_changed(); }
    [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }

    [[nodiscard]] godot::Dictionary to_plan_dictionary() const;
    static godot::Ref<ProjectResourceSpec> from_plan_dictionary(const godot::Dictionary &p_dict);
};

// ============================================================================
// 3. ProjectSceneNodeSpec & ProjectSceneSpec
// ============================================================================

class ProjectSceneNodeSpec : public godot::Resource {
    GDCLASS(ProjectSceneNodeSpec, godot::Resource)

private:
    godot::String node_name_  = "Root";
    godot::String node_class_ = "Node";
    bool use_generated_script_ = false;
    godot::String generated_script_key_;
    godot::Dictionary properties_;
    godot::TypedArray<ProjectSceneNodeSpec> sub_nodes_;

protected:
    static void _bind_methods();

public:
    ProjectSceneNodeSpec() = default;
    ~ProjectSceneNodeSpec() override = default;

    void set_node_name(const godot::String &p_name) { node_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_node_name() const { return node_name_; }

    void set_node_class(const godot::String &p_class) { node_class_ = p_class; emit_changed(); }
    [[nodiscard]] godot::String get_node_class() const { return node_class_; }

    void set_use_generated_script(bool p_use) { use_generated_script_ = p_use; emit_changed(); }
    [[nodiscard]] bool get_use_generated_script() const noexcept { return use_generated_script_; }

    void set_generated_script_key(const godot::String &p_key) { generated_script_key_ = p_key; emit_changed(); }
    [[nodiscard]] godot::String get_generated_script_key() const { return generated_script_key_; }

    void set_properties(const godot::Dictionary &p_props) { properties_ = p_props; emit_changed(); }
    [[nodiscard]] godot::Dictionary get_properties() const { return properties_; }

    void set_sub_nodes(const godot::TypedArray<ProjectSceneNodeSpec> &p_nodes) { sub_nodes_ = p_nodes; emit_changed(); }
    [[nodiscard]] godot::TypedArray<ProjectSceneNodeSpec> get_sub_nodes() const { return sub_nodes_; }

    void add_sub_node(const godot::Ref<ProjectSceneNodeSpec> &p_node);

    [[nodiscard]] godot::Dictionary to_plan_dictionary() const;
    static godot::Ref<ProjectSceneNodeSpec> from_plan_dictionary(const godot::Dictionary &p_dict);
};

class ProjectSceneSpec : public godot::Resource {
    GDCLASS(ProjectSceneSpec, godot::Resource)

private:
    godot::String target_folder_;
    godot::String scene_name_;
    godot::Ref<ProjectSceneNodeSpec> root_node_;
    bool enabled_ = true;

protected:
    static void _bind_methods();

public:
    ProjectSceneSpec();
    ~ProjectSceneSpec() override = default;

    void set_target_folder(const godot::String &p_folder) { target_folder_ = p_folder; emit_changed(); }
    [[nodiscard]] godot::String get_target_folder() const { return target_folder_; }

    void set_scene_name(const godot::String &p_name) { scene_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_scene_name() const { return scene_name_; }

    void set_root_node(const godot::Ref<ProjectSceneNodeSpec> &p_root) { root_node_ = p_root; emit_changed(); }
    [[nodiscard]] godot::Ref<ProjectSceneNodeSpec> get_root_node() const { return root_node_; }

    void set_enabled(bool p_enabled) { enabled_ = p_enabled; emit_changed(); }
    [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }

    [[nodiscard]] godot::Dictionary to_plan_dictionary() const;
    static godot::Ref<ProjectSceneSpec> from_plan_dictionary(const godot::Dictionary &p_dict);
};

// ============================================================================
// 4. ProjectSettingPatch
// ============================================================================

class ProjectSettingPatch : public godot::Resource {
    GDCLASS(ProjectSettingPatch, godot::Resource)

private:
    godot::String setting_path_;
    godot::Variant value_;
    bool is_autoload_ = false;
    bool enabled_     = true;

protected:
    static void _bind_methods();

public:
    ProjectSettingPatch() = default;
    ~ProjectSettingPatch() override = default;

    void set_setting_path(const godot::String &p_path) { setting_path_ = p_path; emit_changed(); }
    [[nodiscard]] godot::String get_setting_path() const { return setting_path_; }

    void set_value(const godot::Variant &p_value) { value_ = p_value; emit_changed(); }
    [[nodiscard]] godot::Variant get_value() const { return value_; }

    void set_is_autoload(bool p_autoload) { is_autoload_ = p_autoload; emit_changed(); }
    [[nodiscard]] bool get_is_autoload() const noexcept { return is_autoload_; }

    void set_enabled(bool p_enabled) { enabled_ = p_enabled; emit_changed(); }
    [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }

    [[nodiscard]] godot::Dictionary to_plan_dictionary() const;
    static godot::Ref<ProjectSettingPatch> from_plan_dictionary(const godot::Dictionary &p_dict);
};

// ============================================================================
// 5. ProjectModuleManifest (Per-Plugin Contribution Unit)
// ============================================================================

class ProjectModuleManifest : public godot::Resource {
    GDCLASS(ProjectModuleManifest, godot::Resource)

private:
    godot::String module_id_;
    godot::String display_name_;
    godot::String plugin_domain_;
    godot::String description_;
    godot::PackedStringArray semantic_tags_;
    bool enabled_ = true;

    godot::PackedStringArray folders_;
    godot::TypedArray<ProjectScriptSpec> scripts_;
    godot::TypedArray<ProjectResourceSpec> resources_;
    godot::TypedArray<ProjectSceneSpec> scenes_;
    godot::TypedArray<ProjectSettingPatch> setting_patches_;

protected:
    static void _bind_methods();

public:
    ProjectModuleManifest() = default;
    ~ProjectModuleManifest() override = default;

    void set_module_id(const godot::String &p_id) { module_id_ = p_id; emit_changed(); }
    [[nodiscard]] godot::String get_module_id() const { return module_id_; }

    void set_display_name(const godot::String &p_name) { display_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_display_name() const { return display_name_; }

    void set_plugin_domain(const godot::String &p_domain) { plugin_domain_ = p_domain; emit_changed(); }
    [[nodiscard]] godot::String get_plugin_domain() const { return plugin_domain_; }

    void set_description(const godot::String &p_desc) { description_ = p_desc; emit_changed(); }
    [[nodiscard]] godot::String get_description() const { return description_; }

    void set_semantic_tags(const godot::PackedStringArray &p_tags) { semantic_tags_ = p_tags; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    void set_enabled(bool p_enabled) { enabled_ = p_enabled; emit_changed(); }
    [[nodiscard]] bool is_enabled() const noexcept { return enabled_; }

    void set_folders(const godot::PackedStringArray &p_folders) { folders_ = p_folders; emit_changed(); }
    [[nodiscard]] godot::PackedStringArray get_folders() const { return folders_; }

    void set_scripts(const godot::TypedArray<ProjectScriptSpec> &p_scripts) { scripts_ = p_scripts; emit_changed(); }
    [[nodiscard]] godot::TypedArray<ProjectScriptSpec> get_scripts() const { return scripts_; }

    void set_resources(const godot::TypedArray<ProjectResourceSpec> &p_resources) { resources_ = p_resources; emit_changed(); }
    [[nodiscard]] godot::TypedArray<ProjectResourceSpec> get_resources() const { return resources_; }

    void set_scenes(const godot::TypedArray<ProjectSceneSpec> &p_scenes) { scenes_ = p_scenes; emit_changed(); }
    [[nodiscard]] godot::TypedArray<ProjectSceneSpec> get_scenes() const { return scenes_; }

    void set_setting_patches(const godot::TypedArray<ProjectSettingPatch> &p_patches) { setting_patches_ = p_patches; emit_changed(); }
    [[nodiscard]] godot::TypedArray<ProjectSettingPatch> get_setting_patches() const { return setting_patches_; }

    // Convenience mutators
    void add_folder(const godot::String &p_folder);
    void add_script_spec(const godot::Ref<ProjectScriptSpec> &p_spec);
    void add_resource_spec(const godot::Ref<ProjectResourceSpec> &p_spec);
    void add_scene_spec(const godot::Ref<ProjectSceneSpec> &p_spec);
    void add_setting_patch(const godot::Ref<ProjectSettingPatch> &p_patch);

    [[nodiscard]] godot::Dictionary to_plan_dictionary() const;
    static godot::Ref<ProjectModuleManifest> from_plan_dictionary(const godot::Dictionary &p_dict);
};

// ============================================================================
// 6. ProjectBlueprintResource (Root Composite Project Definition)
// ============================================================================

class ProjectBlueprintResource : public godot::Resource {
    GDCLASS(ProjectBlueprintResource, godot::Resource)

private:
    godot::String blueprint_name_ = "NewIdeamProject";
    godot::String project_root_   = "res://";
    godot::String profile_overlay_id_; // Tier 3 user profile hook
    godot::TypedArray<ProjectModuleManifest> modules_;

protected:
    static void _bind_methods();

public:
    ProjectBlueprintResource() = default;
    ~ProjectBlueprintResource() override = default;

    void set_blueprint_name(const godot::String &p_name) { blueprint_name_ = p_name; emit_changed(); }
    [[nodiscard]] godot::String get_blueprint_name() const { return blueprint_name_; }

    void set_project_root(const godot::String &p_root) { project_root_ = p_root; emit_changed(); }
    [[nodiscard]] godot::String get_project_root() const { return project_root_; }

    void set_profile_overlay_id(const godot::String &p_id) { profile_overlay_id_ = p_id; emit_changed(); }
    [[nodiscard]] godot::String get_profile_overlay_id() const { return profile_overlay_id_; }

    void set_modules(const godot::TypedArray<ProjectModuleManifest> &p_modules) { modules_ = p_modules; emit_changed(); }
    [[nodiscard]] godot::TypedArray<ProjectModuleManifest> get_modules() const { return modules_; }

    void add_module(const godot::Ref<ProjectModuleManifest> &p_module);
    void remove_module_by_id(const godot::String &p_module_id);
    [[nodiscard]] godot::Ref<ProjectModuleManifest> find_module_by_id(const godot::String &p_module_id) const;

    [[nodiscard]] godot::Dictionary to_plan_dictionary() const;
    static godot::Ref<ProjectBlueprintResource> from_plan_dictionary(const godot::Dictionary &p_dict);
};

} // namespace ideam::godot_ext
