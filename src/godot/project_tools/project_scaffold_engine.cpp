#include "project_scaffold_engine.h"

#include "ideam_project_contribution_registry.h"
#include "../tools/tool_registry.h"

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#endif

#include <unordered_set>

using namespace godot;

namespace ideam::godot_ext {

void ProjectScaffoldEngine::_bind_methods() {
    BIND_ENUM_CONSTANT(PROJECT_DIAG_INFO);
    BIND_ENUM_CONSTANT(PROJECT_DIAG_WARNING);
    BIND_ENUM_CONSTANT(PROJECT_DIAG_ERROR);

    ClassDB::bind_static_method(
        "ProjectScaffoldEngine",
        D_METHOD("normalize_res_path", "root", "relative"),
        &ProjectScaffoldEngine::normalize_res_path
    );
    ClassDB::bind_static_method(
        "ProjectScaffoldEngine",
        D_METHOD("sanitize_filename", "name"),
        &ProjectScaffoldEngine::sanitize_filename
    );
    ClassDB::bind_static_method(
        "ProjectScaffoldEngine",
        D_METHOD("is_class_instantiable", "class_name", "required_base"),
        &ProjectScaffoldEngine::is_class_instantiable,
        DEFVAL(String())
    );

    ClassDB::bind_method(
        D_METHOD("discover_active_blueprint", "project_root"),
        &ProjectScaffoldEngine::discover_active_blueprint,
        DEFVAL(String("res://"))
    );
    ClassDB::bind_method(
        D_METHOD("compile_plan", "blueprint", "overlay"),
        &ProjectScaffoldEngine::compile_plan,
        DEFVAL(Ref<ProjectModuleManifest>())
    );
    ClassDB::bind_method(
        D_METHOD("validate_blueprint", "blueprint", "allow_overwrite"),
        &ProjectScaffoldEngine::validate_blueprint,
        DEFVAL(false)
    );
    ClassDB::bind_method(
        D_METHOD("execute_blueprint", "blueprint", "allow_overwrite"),
        &ProjectScaffoldEngine::execute_blueprint,
        DEFVAL(false)
    );
}

// ============================================================================
// Static Utilities
// ============================================================================

String ProjectScaffoldEngine::normalize_res_path(const String &p_root, const String &p_relative) {
    String root = p_root.strip_edges();
    if (root.is_empty()) {
        root = "res://";
    }
    if (!root.begins_with("res://") && !root.begins_with("user://")) {
        root = String("res://") + root.trim_prefix("/");
    }

    String rel = p_relative.strip_edges();
    if (rel.is_empty() || rel == "." || rel == "/") {
        return root.ends_with("/") ? root.left(root.length() - 1) : root;
    }
    if (rel.begins_with("res://") || rel.begins_with("user://")) {
        return rel.simplify_path();
    }

    while (rel.begins_with("/")) {
        rel = rel.substr(1);
    }
    return root.path_join(rel).simplify_path();
}

String ProjectScaffoldEngine::sanitize_filename(const String &p_name) {
    String cleaned = p_name.strip_edges().replace(" ", "_");
    cleaned = cleaned.replace("\\", "_").replace("/", "_").replace(":", "_");
    return cleaned;
}

String ProjectScaffoldEngine::find_global_script_path(const String &p_class_name) {
    if (p_class_name.is_empty()) {
        return String();
    }
    ProjectSettings *ps = ProjectSettings::get_singleton();
    if (!ps) {
        return String();
    }
    TypedArray<Dictionary> global_classes = ps->get_global_class_list();
    for (int i = 0; i < global_classes.size(); ++i) {
        Dictionary entry = global_classes[i];
        if (String(entry.get("class", "")) == p_class_name) {
            return String(entry.get("path", ""));
        }
    }
    return String();
}

bool ProjectScaffoldEngine::is_class_instantiable(const String &p_class_name, const String &p_required_base) {
    if (p_class_name.is_empty()) {
        return false;
    }
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    if (cdb && cdb->class_exists(p_class_name)) {
        if (!cdb->can_instantiate(p_class_name)) {
            return false;
        }
        if (!p_required_base.is_empty() && !cdb->is_parent_class(p_class_name, p_required_base) && p_class_name != p_required_base) {
            return false;
        }
        return true;
    }
    const String script_path = find_global_script_path(p_class_name);
    if (!script_path.is_empty() && ResourceLoader::get_singleton()->exists(script_path)) {
        return true;
    }
    return false;
}

Dictionary ProjectScaffoldEngine::_make_diagnostic(
    ProjectDiagnosticSeverity p_severity,
    const String &p_code,
    const String &p_target_path,
    const String &p_message,
    const String &p_suggested_fix
) {
    Dictionary d;
    d["severity"]      = static_cast<int>(p_severity);
    d["code"]          = p_code;
    d["target_path"]   = p_target_path;
    d["message"]       = p_message;
    d["suggested_fix"] = p_suggested_fix;
    return d;
}

// ============================================================================
// Script Template Resolution
// ============================================================================

String ProjectScaffoldEngine::_synthesize_default_gdscript(const String &p_base_class, const String &p_class_name) {
    String out;
    out += String("class_name ") + p_class_name + String("\n");
    out += String("extends ") + p_base_class + String("\n\n");
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();
    if ((cdb && cdb->is_parent_class(p_base_class, "Node")) || p_base_class == "Node") {
        out += "func _ready() -> void:\n";
        out += "\tpass\n";
    }
    return out;
}

String ProjectScaffoldEngine::_resolve_script_template_text(
    const String &p_base_class,
    const String &p_template_name
) {
    const String tpl_name = p_template_name.is_empty() ? String("default") : p_template_name;

    // 1. Check C++ IdeamProjectContributionRegistry
    if (const auto *entry = IdeamProjectContributionRegistry::get_singleton().find_script_template(
            StringName(p_base_class), tpl_name)) {
        if (!entry->source_path.is_empty() && FileAccess::file_exists(entry->source_path)) {
            Ref<FileAccess> f = FileAccess::open(entry->source_path, FileAccess::READ);
            if (f.is_valid()) {
                return f->get_as_text();
            }
        }
        if (!entry->inline_content.is_empty()) {
            return entry->inline_content;
        }
    }

    // 2. Check res://script_templates/<base_class>/<template_name>.gd
    String clean_tpl = tpl_name.ends_with(".gd") ? tpl_name : (tpl_name + String(".gd"));
    const String direct_path = String("res://script_templates/").path_join(p_base_class).path_join(clean_tpl);
    if (FileAccess::file_exists(direct_path)) {
        Ref<FileAccess> f = FileAccess::open(direct_path, FileAccess::READ);
        if (f.is_valid()) {
            return f->get_as_text();
        }
    }

    const String default_class_path = String("res://script_templates/").path_join(p_base_class).path_join("default_template.gd");
    if (FileAccess::file_exists(default_class_path)) {
        Ref<FileAccess> f = FileAccess::open(default_class_path, FileAccess::READ);
        if (f.is_valid()) {
            return f->get_as_text();
        }
    }

    // 3. Check .ideam_registry.cfg [ScriptTemplates]
    Ref<ConfigFile> cfg;
    cfg.instantiate();
    if (cfg->load("res://.ideam_registry.cfg") == OK && cfg->has_section("ScriptTemplates")) {
        const String reg_key = p_base_class + String("/") + tpl_name.get_basename();
        if (cfg->has_section_key("ScriptTemplates", reg_key)) {
            const String reg_path = cfg->get_value("ScriptTemplates", reg_key, "");
            if (!reg_path.is_empty() && FileAccess::file_exists(reg_path)) {
                Ref<FileAccess> f = FileAccess::open(reg_path, FileAccess::READ);
                if (f.is_valid()) {
                    return f->get_as_text();
                }
            }
        }
    }

    return String();
}

// ============================================================================
// Hybrid Push-Pull Ecosystem Discovery
// ============================================================================

Ref<ProjectBlueprintResource> ProjectScaffoldEngine::discover_active_blueprint(const String &p_project_root) const {
    Ref<ProjectBlueprintResource> blueprint;
    blueprint.instantiate();
    blueprint->set_blueprint_name("ActiveEcosystemBlueprint");
    blueprint->set_project_root(p_project_root.is_empty() ? String("res://") : p_project_root);

    // 1. Collect C++ programmatic contributions from IdeamProjectContributionRegistry
    const auto &cpp_contribs = IdeamProjectContributionRegistry::get_singleton().get_manifest_contributions();
    for (const auto &contrib : cpp_contribs) {
        if (contrib.build_manifest) {
            Ref<ProjectModuleManifest> manifest = contrib.build_manifest();
            if (manifest.is_valid()) {
                if (manifest->get_module_id().is_empty()) {
                    manifest->set_module_id(String(contrib.plugin_id));
                }
                if (manifest->get_plugin_domain().is_empty()) {
                    manifest->set_plugin_domain(contrib.display_tab.is_empty() ? String(contrib.plugin_id) : contrib.display_tab);
                }
                blueprint->add_module(manifest);
            }
        }
    }

    // 2. Collect typed ProjectModuleManifest (.tres) assets registered by active plugins in .ideam_registry.cfg
    Ref<ConfigFile> cfg;
    cfg.instantiate();
    if (cfg->load("res://.ideam_registry.cfg") == OK) {
        if (cfg->has_section("ActivePlugins") && cfg->has_section("WizardSettings")) {
            PackedStringArray active_keys = cfg->get_section_keys("ActivePlugins");
            for (int i = 0; i < active_keys.size(); ++i) {
                const String plugin_name = active_keys[i];
                if (!bool(cfg->get_value("ActivePlugins", plugin_name, false))) {
                    continue;
                }
                if (!cfg->has_section_key("WizardSettings", plugin_name)) {
                    continue;
                }
                const String manifest_path = cfg->get_value("WizardSettings", plugin_name, "");
                if (manifest_path.is_empty() || !ResourceLoader::get_singleton()->exists(manifest_path)) {
                    continue;
                }
                Ref<Resource> loaded = ResourceLoader::get_singleton()->load(manifest_path);
                Ref<ProjectModuleManifest> typed_manifest = loaded;
                if (typed_manifest.is_valid()) {
                    if (typed_manifest->get_module_id().is_empty()) {
                        typed_manifest->set_module_id(plugin_name);
                    }
                    if (typed_manifest->get_plugin_domain().is_empty()) {
                        typed_manifest->set_plugin_domain(plugin_name);
                    }
                    blueprint->add_module(typed_manifest);
                }
            }
        }
    }

    return blueprint;
}

// ============================================================================
// Compilation & Dry-Run Validation
// ============================================================================

Dictionary ProjectScaffoldEngine::compile_plan(
    const Ref<ProjectBlueprintResource> &p_blueprint,
    const Ref<ProjectModuleManifest> &p_overlay
) const {
    Dictionary plan;
    PackedStringArray folders;
    Array scripts;
    Array resources;
    Array scenes;
    Array patches;

    if (p_blueprint.is_null()) {
        plan["folders"]         = folders;
        plan["scripts"]         = scripts;
        plan["resources"]       = resources;
        plan["scenes"]          = scenes;
        plan["setting_patches"] = patches;
        return plan;
    }

    const String root = p_blueprint->get_project_root();
    std::unordered_set<std::string> seen_folders;

    auto append_folder = [&](const String &p_rel) {
        const String full = normalize_res_path(root, p_rel);
        const std::string key(full.utf8().get_data());
        if (seen_folders.insert(key).second) {
            folders.push_back(full);
        }
    };

    auto ingest_module = [&](const Ref<ProjectModuleManifest> &p_mod) {
        if (p_mod.is_null() || !p_mod->is_enabled()) {
            return;
        }
        const PackedStringArray mod_folders = p_mod->get_folders();
        for (int i = 0; i < mod_folders.size(); ++i) {
            append_folder(mod_folders[i]);
        }

        const TypedArray<ProjectScriptSpec> mod_scripts = p_mod->get_scripts();
        for (int i = 0; i < mod_scripts.size(); ++i) {
            Ref<ProjectScriptSpec> s = mod_scripts[i];
            if (s.is_valid() && s->is_enabled()) {
                append_folder(s->get_target_folder());
                scripts.append(s);
            }
        }

        const TypedArray<ProjectResourceSpec> mod_resources = p_mod->get_resources();
        for (int i = 0; i < mod_resources.size(); ++i) {
            Ref<ProjectResourceSpec> r = mod_resources[i];
            if (r.is_valid() && r->is_enabled()) {
                append_folder(r->get_target_folder());
                resources.append(r);
            }
        }

        const TypedArray<ProjectSceneSpec> mod_scenes = p_mod->get_scenes();
        for (int i = 0; i < mod_scenes.size(); ++i) {
            Ref<ProjectSceneSpec> sc = mod_scenes[i];
            if (sc.is_valid() && sc->is_enabled()) {
                append_folder(sc->get_target_folder());
                scenes.append(sc);
            }
        }

        const TypedArray<ProjectSettingPatch> mod_patches = p_mod->get_setting_patches();
        for (int i = 0; i < mod_patches.size(); ++i) {
            Ref<ProjectSettingPatch> p = mod_patches[i];
            if (p.is_valid() && p->is_enabled()) {
                patches.append(p);
            }
        }
    };

    const TypedArray<ProjectModuleManifest> mods = p_blueprint->get_modules();
    for (int i = 0; i < mods.size(); ++i) {
        Ref<ProjectModuleManifest> mod = mods[i];
        ingest_module(mod);
    }
    if (p_overlay.is_valid()) {
        ingest_module(p_overlay);
    }

    plan["project_root"]    = normalize_res_path(root, "");
    plan["folders"]         = folders;
    plan["scripts"]         = scripts;
    plan["resources"]       = resources;
    plan["scenes"]          = scenes;
    plan["setting_patches"] = patches;
    return plan;
}

Dictionary ProjectScaffoldEngine::validate_blueprint(
    const Ref<ProjectBlueprintResource> &p_blueprint,
    bool p_allow_overwrite
) const {
    Array diagnostics;
    int error_count   = 0;
    int warning_count = 0;

    auto push_diag = [&](ProjectDiagnosticSeverity sev, const String &code, const String &path, const String &msg, const String &fix = "") {
        diagnostics.append(_make_diagnostic(sev, code, path, msg, fix));
        if (sev == PROJECT_DIAG_ERROR) {
            ++error_count;
        } else if (sev == PROJECT_DIAG_WARNING) {
            ++warning_count;
        }
    };

    if (p_blueprint.is_null()) {
        push_diag(PROJECT_DIAG_ERROR, "NULL_BLUEPRINT", "", "Blueprint resource is null.", "Provide a valid ProjectBlueprintResource.");
        Dictionary result;
        result["valid"]         = false;
        result["error_count"]   = error_count;
        result["warning_count"] = warning_count;
        result["diagnostics"]   = diagnostics;
        return result;
    }

    const Dictionary plan = compile_plan(p_blueprint);
    const String root     = plan["project_root"];

    const PackedStringArray folders = plan["folders"];
    for (int i = 0; i < folders.size(); ++i) {
        if (DirAccess::dir_exists_absolute(folders[i])) {
            push_diag(PROJECT_DIAG_INFO, "FOLDER_EXISTS", folders[i], "Folder already exists and will be reused.");
        }
    }

    // Track script keys that will exist after phase 2
    std::unordered_set<std::string> planned_script_keys;
    const Array scripts = plan["scripts"];
    for (int i = 0; i < scripts.size(); ++i) {
        Ref<ProjectScriptSpec> s = scripts[i];
        const String fname = sanitize_filename(s->get_script_name()).to_lower();
        if (fname.is_empty()) {
            push_diag(PROJECT_DIAG_ERROR, "EMPTY_SCRIPT_NAME", s->get_target_folder(), "Script spec has an empty script_name.", "Set a valid filename.");
            continue;
        }
        const String full_path = normalize_res_path(root, s->get_target_folder()).path_join(fname + String(".gd"));
        if (!is_class_instantiable(s->get_base_class())) {
            push_diag(
                PROJECT_DIAG_ERROR,
                "INVALID_BASE_CLASS",
                full_path,
                String("Base class '") + s->get_base_class() + String("' is not a valid instantiable class."),
                "Choose a registered ClassDB or global script class."
            );
        }
        if (FileAccess::file_exists(full_path) && !p_allow_overwrite) {
            push_diag(PROJECT_DIAG_WARNING, "SCRIPT_EXISTS", full_path, "Script already exists and will be skipped unless overwrite is enabled.");
        }
        planned_script_keys.insert(std::string(s->get_base_class().utf8().get_data()));
        planned_script_keys.insert(std::string(s->get_script_name().utf8().get_data()));
        planned_script_keys.insert(std::string(s->get_effective_class_name().utf8().get_data()));
    }

    const Array resources = plan["resources"];
    for (int i = 0; i < resources.size(); ++i) {
        Ref<ProjectResourceSpec> r = resources[i];
        const String fname = sanitize_filename(r->get_resource_name());
        const String ext   = r->get_file_extension().is_empty() ? String("tres") : r->get_file_extension().trim_prefix(".");
        if (fname.is_empty()) {
            push_diag(PROJECT_DIAG_ERROR, "EMPTY_RESOURCE_NAME", r->get_target_folder(), "Resource spec has an empty resource_name.", "Set a valid filename.");
            continue;
        }
        const String full_path = normalize_res_path(root, r->get_target_folder()).path_join(fname + String(".") + ext);
        if (!is_class_instantiable(r->get_resource_class(), "Resource")) {
            push_diag(
                PROJECT_DIAG_ERROR,
                "INVALID_RESOURCE_CLASS",
                full_path,
                String("Resource class '") + r->get_resource_class() + String("' is not an instantiable Resource."),
                "Choose a class deriving from Resource."
            );
        }
        if (r->get_use_generated_script()) {
            const String key = r->get_generated_script_key().is_empty() ? r->get_resource_class() : r->get_generated_script_key();
            if (!planned_script_keys.contains(std::string(key.utf8().get_data()))) {
                push_diag(
                    PROJECT_DIAG_WARNING,
                    "MISSING_GENERATED_SCRIPT",
                    full_path,
                    String("Resource requests generated script '") + key + String("', which is not enabled in this blueprint."),
                    "Enable the matching ProjectScriptSpec or disable use_generated_script."
                );
            }
        }
        if (FileAccess::file_exists(full_path) && !p_allow_overwrite) {
            push_diag(PROJECT_DIAG_WARNING, "RESOURCE_EXISTS", full_path, "Resource file already exists and will be skipped unless overwrite is enabled.");
        }
    }

    const Array scenes = plan["scenes"];
    for (int i = 0; i < scenes.size(); ++i) {
        Ref<ProjectSceneSpec> sc = scenes[i];
        const String fname = sanitize_filename(sc->get_scene_name());
        if (fname.is_empty()) {
            push_diag(PROJECT_DIAG_ERROR, "EMPTY_SCENE_NAME", sc->get_target_folder(), "Scene spec has an empty scene_name.", "Set a valid filename.");
            continue;
        }
        const String full_path = normalize_res_path(root, sc->get_target_folder()).path_join(fname + String(".tscn"));
        if (sc->get_root_node().is_null()) {
            push_diag(PROJECT_DIAG_ERROR, "NULL_SCENE_ROOT", full_path, "Scene spec has no root_node.", "Assign a ProjectSceneNodeSpec.");
        } else if (!is_class_instantiable(sc->get_root_node()->get_node_class(), "Node")) {
            push_diag(
                PROJECT_DIAG_ERROR,
                "INVALID_NODE_CLASS",
                full_path,
                String("Root node class '") + sc->get_root_node()->get_node_class() + String("' is not an instantiable Node."),
                "Choose a class deriving from Node."
            );
        }
        if (FileAccess::file_exists(full_path) && !p_allow_overwrite) {
            push_diag(PROJECT_DIAG_WARNING, "SCENE_EXISTS", full_path, "Scene file already exists and will be skipped unless overwrite is enabled.");
        }
    }

    Dictionary summary;
    summary["folders"]   = folders.size();
    summary["scripts"]   = scripts.size();
    summary["resources"] = resources.size();
    summary["scenes"]    = scenes.size();
    summary["patches"]   = Array(plan["setting_patches"]).size();

    Dictionary result;
    result["valid"]         = (error_count == 0);
    result["error_count"]   = error_count;
    result["warning_count"] = warning_count;
    result["diagnostics"]   = diagnostics;
    result["summary"]       = summary;
    return result;
}

// ============================================================================
// Execution Helpers
// ============================================================================

Object *ProjectScaffoldEngine::_instantiate_class_or_script(
    const String &p_class_name,
    bool p_use_generated_script,
    const String &p_generated_script_key,
    const Dictionary &p_generated_scripts
) {
    Object *instance = nullptr;
    ClassDBSingleton *cdb = ClassDBSingleton::get_singleton();

    if (cdb && cdb->class_exists(p_class_name) && cdb->can_instantiate(p_class_name)) {
        Variant v = cdb->instantiate(p_class_name);
        instance  = v;
    } else {
        const String script_path = find_global_script_path(p_class_name);
        if (!script_path.is_empty() && ResourceLoader::get_singleton()->exists(script_path)) {
            Ref<Script> scr = ResourceLoader::get_singleton()->load(script_path);
            if (scr.is_valid() && scr->has_method("new")) {
                Variant v = scr->call("new");
                instance  = v;
            }
        }
    }

    if (instance && p_use_generated_script) {
        const String key = p_generated_script_key.is_empty() ? p_class_name : p_generated_script_key;
        if (p_generated_scripts.has(key)) {
            Ref<Script> gen_script = p_generated_scripts[key];
            if (gen_script.is_valid()) {
                instance->set_script(gen_script);
            }
        }
    }

    return instance;
}

void ProjectScaffoldEngine::_apply_properties(
    Object *p_target,
    const Dictionary &p_properties,
    const Dictionary &p_generated_resources
) {
    if (!p_target || p_properties.is_empty()) {
        return;
    }
    Array keys = p_properties.keys();
    for (int i = 0; i < keys.size(); ++i) {
        const String prop_name = keys[i];
        Variant val            = p_properties[prop_name];

        // Resolve @ref:<resource_name> cross-references to newly scaffolded resources
        if (val.get_type() == Variant::STRING) {
            const String str_val = val;
            if (str_val.begins_with("@ref:")) {
                const String ref_key = str_val.substr(5);
                if (p_generated_resources.has(ref_key)) {
                    val = p_generated_resources[ref_key];
                }
            } else if (str_val.begins_with("res://") && ResourceLoader::get_singleton()->exists(str_val)) {
                // Only auto-load if target property expects an Object/Resource
                Variant existing = p_target->get(prop_name);
                if (existing.get_type() == Variant::OBJECT || existing.get_type() == Variant::NIL) {
                    Ref<Resource> loaded = ResourceLoader::get_singleton()->load(str_val);
                    if (loaded.is_valid()) {
                        val = loaded;
                    }
                }
            }
        }

        p_target->set(prop_name, val);
    }
}

Node *ProjectScaffoldEngine::_build_scene_node_recursive(
    const Ref<ProjectSceneNodeSpec> &p_spec,
    Node *p_owner_root,
    const Dictionary &p_generated_scripts,
    const Dictionary &p_generated_resources,
    Array &r_diagnostics
) {
    if (p_spec.is_null()) {
        return nullptr;
    }

    Object *obj = _instantiate_class_or_script(
        p_spec->get_node_class(),
        p_spec->get_use_generated_script(),
        p_spec->get_generated_script_key(),
        p_generated_scripts
    );
    Node *node = Object::cast_to<Node>(obj);
    if (!node) {
        if (obj) {
            memdelete(obj);
        }
        r_diagnostics.append(_make_diagnostic(
            PROJECT_DIAG_ERROR,
            "SCENE_NODE_INSTANTIATE_FAILED",
            p_spec->get_node_name(),
            String("Could not instantiate Node of class '") + p_spec->get_node_class() + String("'.")
        ));
        return nullptr;
    }

    node->set_name(p_spec->get_node_name().is_empty() ? p_spec->get_node_class() : p_spec->get_node_name());
    _apply_properties(node, p_spec->get_properties(), p_generated_resources);

    Node *effective_owner = p_owner_root ? p_owner_root : node;
    const TypedArray<ProjectSceneNodeSpec> subs = p_spec->get_sub_nodes();
    for (int i = 0; i < subs.size(); ++i) {
        Ref<ProjectSceneNodeSpec> sub_spec = subs[i];
        if (Node *child = _build_scene_node_recursive(sub_spec, effective_owner, p_generated_scripts, p_generated_resources, r_diagnostics)) {
            node->add_child(child);
            child->set_owner(effective_owner);
        }
    }

    return node;
}

// ============================================================================
// Atomic Topological Execution
// ============================================================================

Dictionary ProjectScaffoldEngine::execute_blueprint(
    const Ref<ProjectBlueprintResource> &p_blueprint,
    bool p_allow_overwrite
) {
    Dictionary receipt;
    receipt["success"] = false;

    const Dictionary validation = validate_blueprint(p_blueprint, p_allow_overwrite);
    Array diagnostics           = validation["diagnostics"];
    if (!bool(validation["valid"])) {
        receipt["diagnostics"] = diagnostics;
        return receipt;
    }

    const Dictionary plan = compile_plan(p_blueprint);
    const String root     = plan["project_root"];

    PackedStringArray created_folders;
    PackedStringArray created_scripts;
    PackedStringArray created_resources;
    PackedStringArray created_scenes;
    Dictionary generated_scripts;
    Dictionary generated_resources;
    Dictionary owner_workspaces; // resource_path -> Tier 2 workspace_id (for handoff)

    // Phase 1: Create folders
    const PackedStringArray folders = plan["folders"];
    for (int i = 0; i < folders.size(); ++i) {
        if (!DirAccess::dir_exists_absolute(folders[i])) {
            if (DirAccess::make_dir_recursive_absolute(folders[i]) == OK) {
                created_folders.push_back(folders[i]);
            }
        }
    }

    // Phase 2: Generate scripts from templates
    const Array scripts = plan["scripts"];
    for (int i = 0; i < scripts.size(); ++i) {
        Ref<ProjectScriptSpec> s = scripts[i];
        const String fname       = sanitize_filename(s->get_script_name()).to_lower();
        const String full_path   = normalize_res_path(root, s->get_target_folder()).path_join(fname + String(".gd"));

        if (!FileAccess::file_exists(full_path) || p_allow_overwrite) {
            const String class_ident = s->get_effective_class_name();
            String code              = _resolve_script_template_text(s->get_base_class(), s->get_template_name());
            if (code.is_empty()) {
                code = _synthesize_default_gdscript(s->get_base_class(), class_ident);
            } else {
                code = code.replace("_BASE_", s->get_base_class());
                code = code.replace("_CLASS_", class_ident);
                Dictionary tokens = s->get_template_tokens();
                Array tkeys       = tokens.keys();
                for (int t = 0; t < tkeys.size(); ++t) {
                    const String tk = tkeys[t];
                    code            = code.replace(tk, String(tokens[tk]));
                }
                if (!code.contains("class_name ")) {
                    code = String("class_name ") + class_ident + String("\n") + code;
                }
            }

            Ref<FileAccess> f = FileAccess::open(full_path, FileAccess::WRITE);
            if (f.is_valid()) {
                f->store_string(code);
                f->close();
                created_scripts.push_back(full_path);
            }
        }

        if (ResourceLoader::get_singleton()->exists(full_path)) {
            Ref<Script> loaded_script = ResourceLoader::get_singleton()->load(full_path);
            if (loaded_script.is_valid()) {
                generated_scripts[s->get_base_class()]          = loaded_script;
                generated_scripts[s->get_script_name()]         = loaded_script;
                generated_scripts[s->get_effective_class_name()] = loaded_script;
            }
        }
    }

    // Phase 3: Instantiate & save resources
    const Array resources = plan["resources"];
    for (int i = 0; i < resources.size(); ++i) {
        Ref<ProjectResourceSpec> r = resources[i];
        const String fname         = sanitize_filename(r->get_resource_name());
        const String ext           = r->get_file_extension().is_empty() ? String("tres") : r->get_file_extension().trim_prefix(".");
        const String full_path     = normalize_res_path(root, r->get_target_folder()).path_join(fname + String(".") + ext);

        if (FileAccess::file_exists(full_path) && !p_allow_overwrite) {
            if (Ref<Resource> existing = ResourceLoader::get_singleton()->load(full_path); existing.is_valid()) {
                generated_resources[r->get_resource_name()] = existing;
            }
            continue;
        }

        Object *obj = _instantiate_class_or_script(
            r->get_resource_class(),
            r->get_use_generated_script(),
            r->get_generated_script_key(),
            generated_scripts
        );
        Ref<Resource> res = Object::cast_to<Resource>(obj);
        if (res.is_null()) {
            if (obj) {
                memdelete(obj);
            }
            diagnostics.append(_make_diagnostic(
                PROJECT_DIAG_ERROR,
                "RESOURCE_INSTANTIATE_FAILED",
                full_path,
                String("Could not instantiate Resource '") + r->get_resource_class() + String("'.")
            ));
            continue;
        }

        res->set_name(r->get_resource_name());
        _apply_properties(res.ptr(), r->get_properties(), generated_resources);

        if (ResourceSaver::get_singleton()->save(res, full_path) == OK) {
            created_resources.push_back(full_path);
            generated_resources[r->get_resource_name()] = res;
            if (const auto *desc = ToolRegistry::get_singleton().find_any_descriptor_for_resource(res)) {
                owner_workspaces[full_path] = String(desc->workspace_id);
            }
        }
    }

    // Phase 4: Build & pack scenes (with proper owner assignment!)
    const Array scenes = plan["scenes"];
    for (int i = 0; i < scenes.size(); ++i) {
        Ref<ProjectSceneSpec> sc = scenes[i];
        const String fname       = sanitize_filename(sc->get_scene_name());
        const String full_path   = normalize_res_path(root, sc->get_target_folder()).path_join(fname + String(".tscn"));

        if (FileAccess::file_exists(full_path) && !p_allow_overwrite) {
            continue;
        }

        Node *root_node = _build_scene_node_recursive(
            sc->get_root_node(),
            nullptr,
            generated_scripts,
            generated_resources,
            diagnostics
        );
        if (!root_node) {
            continue;
        }

        Ref<PackedScene> packed;
        packed.instantiate();
        if (packed->pack(root_node) == OK) {
            if (ResourceSaver::get_singleton()->save(packed, full_path) == OK) {
                created_scenes.push_back(full_path);
            }
        }
        memdelete(root_node);
    }

    // Phase 5: Apply ProjectSettings / Autoload patches
    const Array patches = plan["setting_patches"];
    if (!patches.is_empty()) {
        if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
            bool touched = false;
            for (int i = 0; i < patches.size(); ++i) {
                Ref<ProjectSettingPatch> p = patches[i];
                if (p.is_null() || p->get_setting_path().is_empty()) {
                    continue;
                }
                String key = p->get_setting_path();
                Variant val = p->get_value();
                if (p->get_is_autoload() && !key.begins_with("autoload/")) {
                    key = String("autoload/") + key;
                }
                if (p->get_is_autoload() && val.get_type() == Variant::STRING) {
                    String s = val;
                    if (!s.begins_with("*")) {
                        val = String("*") + normalize_res_path(root, s);
                    }
                }
                ps->set_setting(key, val);
                touched = true;
            }
            if (touched) {
                ps->save();
            }
        }
    }

    // Phase 6: Trigger EditorFileSystem scan
#ifdef TOOLS_ENABLED
    if (EditorInterface *ei = EditorInterface::get_singleton()) {
        if (EditorFileSystem *efs = ei->get_resource_filesystem()) {
            efs->scan();
        }
    }
#endif

    receipt["success"]           = true;
    receipt["project_root"]      = root;
    receipt["created_folders"]   = created_folders;
    receipt["created_scripts"]   = created_scripts;
    receipt["created_resources"] = created_resources;
    receipt["created_scenes"]    = created_scenes;
    receipt["owner_workspaces"]  = owner_workspaces;
    receipt["diagnostics"]       = diagnostics;

    // Phase 7: Dispatch C++ plugin post-generation hooks
    IdeamProjectContributionRegistry::get_singleton().invoke_post_generate_hooks(receipt);

    return receipt;
}

} // namespace ideam::godot_ext
