#pragma once

/**
 * project_scaffold_engine.h
 * Headless C++26 Compilation, Validation, and Execution Engine for Ideam Project Tools (Phase 1).
 *
 * Completely UI-agnostic (`godot::RefCounted`), enabling:
 *   - Tier 2 (`ProjectWorkspace`): Live dry-run validation & one-click project generation.
 *   - Tier 3 (User Profiles): Overlaying user preferences/assets onto plugin manifests.
 *   - Tier 4 (Workflows): Headless invocation producing a structured `receipt` Dictionary.
 *   - Tier 5 (AI Super-Tools): Deterministic validate -> fix -> execute tool-calling loop.
 */

#include "project_blueprint_resource.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <unordered_map>

namespace ideam::godot_ext {

enum ProjectDiagnosticSeverity : int32_t {
    PROJECT_DIAG_INFO    = 0,
    PROJECT_DIAG_WARNING = 1,
    PROJECT_DIAG_ERROR   = 2
};

class ProjectScaffoldEngine : public godot::RefCounted {
    GDCLASS(ProjectScaffoldEngine, godot::RefCounted)

public:
    static godot::String normalize_res_path(const godot::String &p_root, const godot::String &p_relative);
    static godot::String sanitize_filename(const godot::String &p_name);
    static bool is_class_instantiable(const godot::String &p_class_name, const godot::String &p_required_base = "");
    static godot::String find_global_script_path(const godot::String &p_class_name);

private:
    static godot::Dictionary _make_diagnostic(
        ProjectDiagnosticSeverity p_severity,
        const godot::String &p_code,
        const godot::String &p_target_path,
        const godot::String &p_message,
        const godot::String &p_suggested_fix = ""
    );

    static godot::String _resolve_script_template_text(
        const godot::String &p_base_class,
        const godot::String &p_template_name
    );

    static godot::String _synthesize_default_gdscript(
        const godot::String &p_base_class,
        const godot::String &p_class_name
    );

    static godot::Object *_instantiate_class_or_script(
        const godot::String &p_class_name,
        bool p_use_generated_script,
        const godot::String &p_generated_script_key,
        const godot::Dictionary &p_generated_scripts
    );

    static void _apply_properties(
        godot::Object *p_target,
        const godot::Dictionary &p_properties,
        const godot::Dictionary &p_generated_resources
    );

    static godot::Node *_build_scene_node_recursive(
        const godot::Ref<ProjectSceneNodeSpec> &p_spec,
        godot::Node *p_owner_root,
        const godot::Dictionary &p_generated_scripts,
        const godot::Dictionary &p_generated_resources,
        godot::Array &r_diagnostics
    );

protected:
    static void _bind_methods();

public:
    ProjectScaffoldEngine() = default;
    ~ProjectScaffoldEngine() override = default;

    /**
     * @brief Discovers all active Ideam plugins from `.ideam_registry.cfg` (`[ActivePlugins]` ∩ `[WizardSettings]`)
     * AND C++ contributions in `IdeamProjectContributionRegistry`, returning a ready-to-use Blueprint.
     */
    [[nodiscard]] godot::Ref<ProjectBlueprintResource> discover_active_blueprint(
        const godot::String &p_project_root = "res://"
    ) const;

    /**
     * @brief Merges enabled modules (and optional Tier 3 overlay) into a deduplicated plan Dictionary.
     */
    [[nodiscard]] godot::Dictionary compile_plan(
        const godot::Ref<ProjectBlueprintResource> &p_blueprint,
        const godot::Ref<ProjectModuleManifest> &p_overlay = godot::Ref<ProjectModuleManifest>()
    ) const;

    /**
     * @brief Runs a full dry-run validation without writing to disk.
     * Returns `{ "valid": bool, "error_count": int, "warning_count": int, "diagnostics": Array[Dictionary], "summary": Dictionary }`.
     */
    [[nodiscard]] godot::Dictionary validate_blueprint(
        const godot::Ref<ProjectBlueprintResource> &p_blueprint,
        bool p_allow_overwrite = false
    ) const;

    /**
     * @brief Executes the blueprint in topological order:
     *   1. Folders -> 2. Scripts -> 3. Resources -> 4. Scenes (with proper owner) -> 5. ProjectSettings -> 6. Editor scan -> 7. Post-gen hooks.
     * Returns a structured ProjectGenerationReceipt Dictionary suitable for Tier 2 handoff and Tier 4/5 pipelines.
     */
    [[nodiscard]] godot::Dictionary execute_blueprint(
        const godot::Ref<ProjectBlueprintResource> &p_blueprint,
        bool p_allow_overwrite = false
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::ProjectDiagnosticSeverity);
