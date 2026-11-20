#pragma once

/**
 * ideam_project_contribution_registry.h
 * Lightweight C++26 singleton registry decoupling `ProjectWorkspace` & `ProjectScaffoldExecutor`
 * from domain plugins (`IdeamGamesPlugin`, `IdeamGeometryPlugin`, `IdeamTasksPlugin`, etc.).
 *
 * Mirrors `ToolRegistry`: plugins register programmatic `ProjectModuleManifest`
 * providers, custom script templates, and post-generation hooks in `_enter_tree()` and
 * unregister them in `_exit_tree()` without compile-time coupling.
 */

#include "project_blueprint_resource.h"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <functional>
#include <vector>

namespace ideam::godot_ext {

struct ProjectManifestContribution {
    godot::StringName plugin_id;     // e.g. "IdeamGames", "IdeamGeometry"
    godot::String display_tab;       // Sidebar tab label (e.g. "Games", "Geometry")
    int32_t sort_order = 100;

    // Lazily synthesizes or loads a typed ProjectModuleManifest for this plugin.
    std::function<godot::Ref<ProjectModuleManifest>()> build_manifest;
};

struct ProjectPostGenerateHook {
    godot::StringName plugin_id;
    godot::String hook_name;
    int32_t priority = 100; // Lower runs earlier

    // Invoked after ProjectScaffoldExecutor finishes writing files and scanning the filesystem.
    std::function<void(const godot::Dictionary &p_receipt)> on_project_generated;
};

struct ProjectScriptTemplateEntry {
    godot::StringName plugin_id;
    godot::StringName base_class;
    godot::String template_name;  // e.g. "default"
    godot::String source_path;    // Optional res:// path to template file
    godot::String inline_content; // Optional inline fallback template text
};

class IdeamProjectContributionRegistry {
private:
    std::vector<ProjectManifestContribution> manifest_contributions_;
    std::vector<ProjectPostGenerateHook> post_generate_hooks_;
    std::vector<ProjectScriptTemplateEntry> script_templates_;

    IdeamProjectContributionRegistry() = default;

public:
    static IdeamProjectContributionRegistry &get_singleton();

    void register_manifest_contribution(ProjectManifestContribution p_contribution);
    void register_post_generate_hook(ProjectPostGenerateHook p_hook);
    void register_script_template(ProjectScriptTemplateEntry p_entry);

    void unregister_plugin(const godot::StringName &p_plugin_id);
    void clear();

    [[nodiscard]] const std::vector<ProjectManifestContribution> &get_manifest_contributions() const noexcept {
        return manifest_contributions_;
    }

    [[nodiscard]] const std::vector<ProjectPostGenerateHook> &get_post_generate_hooks() const noexcept {
        return post_generate_hooks_;
    }

    [[nodiscard]] const ProjectScriptTemplateEntry *find_script_template(
        const godot::StringName &p_base_class,
        const godot::String &p_template_name = "default"
    ) const;

    void invoke_post_generate_hooks(const godot::Dictionary &p_receipt) const;
};

} // namespace ideam::godot_ext
