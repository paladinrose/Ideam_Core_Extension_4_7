#include "project_blueprint_resource.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

namespace ideam::godot_ext {

// ============================================================================
// 1. ProjectScriptSpec
// ============================================================================

void ProjectScriptSpec::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_target_folder", "folder"), &ProjectScriptSpec::set_target_folder);
    ClassDB::bind_method(D_METHOD("get_target_folder"), &ProjectScriptSpec::get_target_folder);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "target_folder"), "set_target_folder", "get_target_folder");

    ClassDB::bind_method(D_METHOD("set_script_name", "name"), &ProjectScriptSpec::set_script_name);
    ClassDB::bind_method(D_METHOD("get_script_name"), &ProjectScriptSpec::get_script_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "script_name"), "set_script_name", "get_script_name");

    ClassDB::bind_method(D_METHOD("set_base_class", "base_class"), &ProjectScriptSpec::set_base_class);
    ClassDB::bind_method(D_METHOD("get_base_class"), &ProjectScriptSpec::get_base_class);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "base_class"), "set_base_class", "get_base_class");

    ClassDB::bind_method(D_METHOD("set_class_name_override", "override"), &ProjectScriptSpec::set_class_name_override);
    ClassDB::bind_method(D_METHOD("get_class_name_override"), &ProjectScriptSpec::get_class_name_override);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "class_name_override"), "set_class_name_override", "get_class_name_override");

    ClassDB::bind_method(D_METHOD("set_template_name", "template_name"), &ProjectScriptSpec::set_template_name);
    ClassDB::bind_method(D_METHOD("get_template_name"), &ProjectScriptSpec::get_template_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "template_name"), "set_template_name", "get_template_name");

    ClassDB::bind_method(D_METHOD("set_template_tokens", "tokens"), &ProjectScriptSpec::set_template_tokens);
    ClassDB::bind_method(D_METHOD("get_template_tokens"), &ProjectScriptSpec::get_template_tokens);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "template_tokens"), "set_template_tokens", "get_template_tokens");

    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &ProjectScriptSpec::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &ProjectScriptSpec::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ClassDB::bind_method(D_METHOD("get_effective_class_name"), &ProjectScriptSpec::get_effective_class_name);
    ClassDB::bind_method(D_METHOD("to_plan_dictionary"), &ProjectScriptSpec::to_plan_dictionary);
    ClassDB::bind_static_method("ProjectScriptSpec", D_METHOD("from_plan_dictionary", "dict"), &ProjectScriptSpec::from_plan_dictionary);
}

String ProjectScriptSpec::get_effective_class_name() const {
    if (!class_name_override_.is_empty()) {
        return class_name_override_.strip_edges().replace(" ", "_");
    }
    if (!script_name_.is_empty()) {
        return script_name_.strip_edges().replace(" ", "_").to_pascal_case();
    }
    return String("Custom") + base_class_;
}

Dictionary ProjectScriptSpec::to_plan_dictionary() const {
    Dictionary d;
    d["target_folder"]       = target_folder_;
    d["script_name"]         = script_name_;
    d["base_class"]          = base_class_;
    d["class_name_override"] = class_name_override_;
    d["template_name"]       = template_name_;
    d["template_tokens"]     = template_tokens_.duplicate(true);
    d["enabled"]             = enabled_;
    return d;
}

Ref<ProjectScriptSpec> ProjectScriptSpec::from_plan_dictionary(const Dictionary &p_dict) {
    Ref<ProjectScriptSpec> spec;
    spec.instantiate();
    if (p_dict.has("target_folder")) spec->set_target_folder(p_dict["target_folder"]);
    if (p_dict.has("script_name")) spec->set_script_name(p_dict["script_name"]);
    if (p_dict.has("base_class")) spec->set_base_class(p_dict["base_class"]);
    if (p_dict.has("class_name_override")) spec->set_class_name_override(p_dict["class_name_override"]);
    if (p_dict.has("template_name")) spec->set_template_name(p_dict["template_name"]);
    if (p_dict.has("template_tokens")) spec->set_template_tokens(p_dict["template_tokens"]);
    if (p_dict.has("enabled")) spec->set_enabled(bool(p_dict["enabled"]));
    return spec;
}

// ============================================================================
// 2. ProjectResourceSpec
// ============================================================================

void ProjectResourceSpec::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_target_folder", "folder"), &ProjectResourceSpec::set_target_folder);
    ClassDB::bind_method(D_METHOD("get_target_folder"), &ProjectResourceSpec::get_target_folder);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "target_folder"), "set_target_folder", "get_target_folder");

    ClassDB::bind_method(D_METHOD("set_resource_name", "name"), &ProjectResourceSpec::set_resource_name);
    ClassDB::bind_method(D_METHOD("get_resource_name"), &ProjectResourceSpec::get_resource_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "resource_name"), "set_resource_name", "get_resource_name");

    ClassDB::bind_method(D_METHOD("set_resource_class", "resource_class"), &ProjectResourceSpec::set_resource_class);
    ClassDB::bind_method(D_METHOD("get_resource_class"), &ProjectResourceSpec::get_resource_class);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "resource_class"), "set_resource_class", "get_resource_class");

    ClassDB::bind_method(D_METHOD("set_file_extension", "ext"), &ProjectResourceSpec::set_file_extension);
    ClassDB::bind_method(D_METHOD("get_file_extension"), &ProjectResourceSpec::get_file_extension);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "file_extension"), "set_file_extension", "get_file_extension");

    ClassDB::bind_method(D_METHOD("set_use_generated_script", "use"), &ProjectResourceSpec::set_use_generated_script);
    ClassDB::bind_method(D_METHOD("get_use_generated_script"), &ProjectResourceSpec::get_use_generated_script);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_generated_script"), "set_use_generated_script", "get_use_generated_script");

    ClassDB::bind_method(D_METHOD("set_generated_script_key", "key"), &ProjectResourceSpec::set_generated_script_key);
    ClassDB::bind_method(D_METHOD("get_generated_script_key"), &ProjectResourceSpec::get_generated_script_key);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "generated_script_key"), "set_generated_script_key", "get_generated_script_key");

    ClassDB::bind_method(D_METHOD("set_properties", "properties"), &ProjectResourceSpec::set_properties);
    ClassDB::bind_method(D_METHOD("get_properties"), &ProjectResourceSpec::get_properties);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "properties"), "set_properties", "get_properties");

    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &ProjectResourceSpec::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &ProjectResourceSpec::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ClassDB::bind_method(D_METHOD("to_plan_dictionary"), &ProjectResourceSpec::to_plan_dictionary);
    ClassDB::bind_static_method("ProjectResourceSpec", D_METHOD("from_plan_dictionary", "dict"), &ProjectResourceSpec::from_plan_dictionary);
}

Dictionary ProjectResourceSpec::to_plan_dictionary() const {
    Dictionary d;
    d["target_folder"]        = target_folder_;
    d["resource_name"]        = resource_name_;
    d["resource_class"]       = resource_class_;
    d["file_extension"]       = file_extension_;
    d["use_generated_script"] = use_generated_script_;
    d["generated_script_key"] = generated_script_key_;
    d["properties"]           = properties_.duplicate(true);
    d["enabled"]              = enabled_;
    return d;
}

Ref<ProjectResourceSpec> ProjectResourceSpec::from_plan_dictionary(const Dictionary &p_dict) {
    Ref<ProjectResourceSpec> spec;
    spec.instantiate();
    if (p_dict.has("target_folder")) spec->set_target_folder(p_dict["target_folder"]);
    if (p_dict.has("resource_name")) spec->set_resource_name(p_dict["resource_name"]);
    if (p_dict.has("resource_class")) spec->set_resource_class(p_dict["resource_class"]);
    if (p_dict.has("file_extension")) spec->set_file_extension(p_dict["file_extension"]);
    if (p_dict.has("use_generated_script")) spec->set_use_generated_script(bool(p_dict["use_generated_script"]));
    if (p_dict.has("generated_script_key")) spec->set_generated_script_key(p_dict["generated_script_key"]);
    if (p_dict.has("properties")) spec->set_properties(p_dict["properties"]);
    if (p_dict.has("enabled")) spec->set_enabled(bool(p_dict["enabled"]));
    return spec;
}

// ============================================================================
// 3. ProjectSceneNodeSpec & ProjectSceneSpec
// ============================================================================

void ProjectSceneNodeSpec::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_node_name", "name"), &ProjectSceneNodeSpec::set_node_name);
    ClassDB::bind_method(D_METHOD("get_node_name"), &ProjectSceneNodeSpec::get_node_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "node_name"), "set_node_name", "get_node_name");

    ClassDB::bind_method(D_METHOD("set_node_class", "node_class"), &ProjectSceneNodeSpec::set_node_class);
    ClassDB::bind_method(D_METHOD("get_node_class"), &ProjectSceneNodeSpec::get_node_class);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "node_class"), "set_node_class", "get_node_class");

    ClassDB::bind_method(D_METHOD("set_use_generated_script", "use"), &ProjectSceneNodeSpec::set_use_generated_script);
    ClassDB::bind_method(D_METHOD("get_use_generated_script"), &ProjectSceneNodeSpec::get_use_generated_script);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_generated_script"), "set_use_generated_script", "get_use_generated_script");

    ClassDB::bind_method(D_METHOD("set_generated_script_key", "key"), &ProjectSceneNodeSpec::set_generated_script_key);
    ClassDB::bind_method(D_METHOD("get_generated_script_key"), &ProjectSceneNodeSpec::get_generated_script_key);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "generated_script_key"), "set_generated_script_key", "get_generated_script_key");

    ClassDB::bind_method(D_METHOD("set_properties", "properties"), &ProjectSceneNodeSpec::set_properties);
    ClassDB::bind_method(D_METHOD("get_properties"), &ProjectSceneNodeSpec::get_properties);
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "properties"), "set_properties", "get_properties");

    ClassDB::bind_method(D_METHOD("set_sub_nodes", "sub_nodes"), &ProjectSceneNodeSpec::set_sub_nodes);
    ClassDB::bind_method(D_METHOD("get_sub_nodes"), &ProjectSceneNodeSpec::get_sub_nodes);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "sub_nodes", PROPERTY_HINT_ARRAY_TYPE, "ProjectSceneNodeSpec"),
        "set_sub_nodes",
        "get_sub_nodes"
    );

    ClassDB::bind_method(D_METHOD("add_sub_node", "node"), &ProjectSceneNodeSpec::add_sub_node);
    ClassDB::bind_method(D_METHOD("to_plan_dictionary"), &ProjectSceneNodeSpec::to_plan_dictionary);
    ClassDB::bind_static_method("ProjectSceneNodeSpec", D_METHOD("from_plan_dictionary", "dict"), &ProjectSceneNodeSpec::from_plan_dictionary);
}

void ProjectSceneNodeSpec::add_sub_node(const Ref<ProjectSceneNodeSpec> &p_node) {
    if (p_node.is_valid()) {
        sub_nodes_.append(p_node);
        emit_changed();
    }
}

Dictionary ProjectSceneNodeSpec::to_plan_dictionary() const {
    Dictionary d;
    d["node_name"]            = node_name_;
    d["node_class"]           = node_class_;
    d["use_generated_script"] = use_generated_script_;
    d["generated_script_key"] = generated_script_key_;
    d["properties"]           = properties_.duplicate(true);
    Array subs;
    for (int i = 0; i < sub_nodes_.size(); ++i) {
        Ref<ProjectSceneNodeSpec> sub = sub_nodes_[i];
        if (sub.is_valid()) {
            subs.append(sub->to_plan_dictionary());
        }
    }
    d["sub_nodes"] = subs;
    return d;
}

Ref<ProjectSceneNodeSpec> ProjectSceneNodeSpec::from_plan_dictionary(const Dictionary &p_dict) {
    Ref<ProjectSceneNodeSpec> spec;
    spec.instantiate();
    if (p_dict.has("node_name")) spec->set_node_name(p_dict["node_name"]);
    if (p_dict.has("node_class")) spec->set_node_class(p_dict["node_class"]);
    if (p_dict.has("use_generated_script")) spec->set_use_generated_script(bool(p_dict["use_generated_script"]));
    if (p_dict.has("generated_script_key")) spec->set_generated_script_key(p_dict["generated_script_key"]);
    if (p_dict.has("properties")) spec->set_properties(p_dict["properties"]);
    if (p_dict.has("sub_nodes")) {
        Array subs = p_dict["sub_nodes"];
        for (int i = 0; i < subs.size(); ++i) {
            if (subs[i].get_type() == Variant::DICTIONARY) {
                spec->add_sub_node(ProjectSceneNodeSpec::from_plan_dictionary(subs[i]));
            }
        }
    }
    return spec;
}

ProjectSceneSpec::ProjectSceneSpec() {
    root_node_.instantiate();
}

void ProjectSceneSpec::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_target_folder", "folder"), &ProjectSceneSpec::set_target_folder);
    ClassDB::bind_method(D_METHOD("get_target_folder"), &ProjectSceneSpec::get_target_folder);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "target_folder"), "set_target_folder", "get_target_folder");

    ClassDB::bind_method(D_METHOD("set_scene_name", "name"), &ProjectSceneSpec::set_scene_name);
    ClassDB::bind_method(D_METHOD("get_scene_name"), &ProjectSceneSpec::get_scene_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "scene_name"), "set_scene_name", "get_scene_name");

    ClassDB::bind_method(D_METHOD("set_root_node", "root_node"), &ProjectSceneSpec::set_root_node);
    ClassDB::bind_method(D_METHOD("get_root_node"), &ProjectSceneSpec::get_root_node);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "root_node", PROPERTY_HINT_RESOURCE_TYPE, "ProjectSceneNodeSpec"),
        "set_root_node",
        "get_root_node"
    );

    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &ProjectSceneSpec::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &ProjectSceneSpec::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ClassDB::bind_method(D_METHOD("to_plan_dictionary"), &ProjectSceneSpec::to_plan_dictionary);
    ClassDB::bind_static_method("ProjectSceneSpec", D_METHOD("from_plan_dictionary", "dict"), &ProjectSceneSpec::from_plan_dictionary);
}

Dictionary ProjectSceneSpec::to_plan_dictionary() const {
    Dictionary d;
    d["target_folder"] = target_folder_;
    d["scene_name"]    = scene_name_;
    d["enabled"]       = enabled_;
    if (root_node_.is_valid()) {
        d["root_node"] = root_node_->to_plan_dictionary();
    }
    return d;
}

Ref<ProjectSceneSpec> ProjectSceneSpec::from_plan_dictionary(const Dictionary &p_dict) {
    Ref<ProjectSceneSpec> spec;
    spec.instantiate();
    if (p_dict.has("target_folder")) spec->set_target_folder(p_dict["target_folder"]);
    if (p_dict.has("scene_name")) spec->set_scene_name(p_dict["scene_name"]);
    if (p_dict.has("enabled")) spec->set_enabled(bool(p_dict["enabled"]));
    if (p_dict.has("root_node") && p_dict["root_node"].get_type() == Variant::DICTIONARY) {
        spec->set_root_node(ProjectSceneNodeSpec::from_plan_dictionary(p_dict["root_node"]));
    }
    return spec;
}

// ============================================================================
// 4. ProjectSettingPatch
// ============================================================================

void ProjectSettingPatch::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_setting_path", "path"), &ProjectSettingPatch::set_setting_path);
    ClassDB::bind_method(D_METHOD("get_setting_path"), &ProjectSettingPatch::get_setting_path);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "setting_path"), "set_setting_path", "get_setting_path");

    ClassDB::bind_method(D_METHOD("set_value", "value"), &ProjectSettingPatch::set_value);
    ClassDB::bind_method(D_METHOD("get_value"), &ProjectSettingPatch::get_value);
    ADD_PROPERTY(PropertyInfo(Variant::NIL, "value", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT), "set_value", "get_value");

    ClassDB::bind_method(D_METHOD("set_is_autoload", "is_autoload"), &ProjectSettingPatch::set_is_autoload);
    ClassDB::bind_method(D_METHOD("get_is_autoload"), &ProjectSettingPatch::get_is_autoload);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_autoload"), "set_is_autoload", "get_is_autoload");

    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &ProjectSettingPatch::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &ProjectSettingPatch::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ClassDB::bind_method(D_METHOD("to_plan_dictionary"), &ProjectSettingPatch::to_plan_dictionary);
    ClassDB::bind_static_method("ProjectSettingPatch", D_METHOD("from_plan_dictionary", "dict"), &ProjectSettingPatch::from_plan_dictionary);
}

Dictionary ProjectSettingPatch::to_plan_dictionary() const {
    Dictionary d;
    d["setting_path"] = setting_path_;
    d["value"]        = value_;
    d["is_autoload"]  = is_autoload_;
    d["enabled"]      = enabled_;
    return d;
}

Ref<ProjectSettingPatch> ProjectSettingPatch::from_plan_dictionary(const Dictionary &p_dict) {
    Ref<ProjectSettingPatch> patch;
    patch.instantiate();
    if (p_dict.has("setting_path")) patch->set_setting_path(p_dict["setting_path"]);
    if (p_dict.has("value")) patch->set_value(p_dict["value"]);
    if (p_dict.has("is_autoload")) patch->set_is_autoload(bool(p_dict["is_autoload"]));
    if (p_dict.has("enabled")) patch->set_enabled(bool(p_dict["enabled"]));
    return patch;
}

// ============================================================================
// 5. ProjectModuleManifest
// ============================================================================

void ProjectModuleManifest::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_module_id", "id"), &ProjectModuleManifest::set_module_id);
    ClassDB::bind_method(D_METHOD("get_module_id"), &ProjectModuleManifest::get_module_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "module_id"), "set_module_id", "get_module_id");

    ClassDB::bind_method(D_METHOD("set_display_name", "name"), &ProjectModuleManifest::set_display_name);
    ClassDB::bind_method(D_METHOD("get_display_name"), &ProjectModuleManifest::get_display_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");

    ClassDB::bind_method(D_METHOD("set_plugin_domain", "domain"), &ProjectModuleManifest::set_plugin_domain);
    ClassDB::bind_method(D_METHOD("get_plugin_domain"), &ProjectModuleManifest::get_plugin_domain);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "plugin_domain"), "set_plugin_domain", "get_plugin_domain");

    ClassDB::bind_method(D_METHOD("set_description", "description"), &ProjectModuleManifest::set_description);
    ClassDB::bind_method(D_METHOD("get_description"), &ProjectModuleManifest::get_description);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &ProjectModuleManifest::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &ProjectModuleManifest::get_semantic_tags);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "semantic_tags"), "set_semantic_tags", "get_semantic_tags");

    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &ProjectModuleManifest::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &ProjectModuleManifest::is_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");

    ClassDB::bind_method(D_METHOD("set_folders", "folders"), &ProjectModuleManifest::set_folders);
    ClassDB::bind_method(D_METHOD("get_folders"), &ProjectModuleManifest::get_folders);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "folders"), "set_folders", "get_folders");

    ClassDB::bind_method(D_METHOD("set_scripts", "scripts"), &ProjectModuleManifest::set_scripts);
    ClassDB::bind_method(D_METHOD("get_scripts"), &ProjectModuleManifest::get_scripts);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "scripts", PROPERTY_HINT_ARRAY_TYPE, "ProjectScriptSpec"), "set_scripts", "get_scripts");

    ClassDB::bind_method(D_METHOD("set_resources", "resources"), &ProjectModuleManifest::set_resources);
    ClassDB::bind_method(D_METHOD("get_resources"), &ProjectModuleManifest::get_resources);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "resources", PROPERTY_HINT_ARRAY_TYPE, "ProjectResourceSpec"), "set_resources", "get_resources");

    ClassDB::bind_method(D_METHOD("set_scenes", "scenes"), &ProjectModuleManifest::set_scenes);
    ClassDB::bind_method(D_METHOD("get_scenes"), &ProjectModuleManifest::get_scenes);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "scenes", PROPERTY_HINT_ARRAY_TYPE, "ProjectSceneSpec"), "set_scenes", "get_scenes");

    ClassDB::bind_method(D_METHOD("set_setting_patches", "patches"), &ProjectModuleManifest::set_setting_patches);
    ClassDB::bind_method(D_METHOD("get_setting_patches"), &ProjectModuleManifest::get_setting_patches);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "setting_patches", PROPERTY_HINT_ARRAY_TYPE, "ProjectSettingPatch"), "set_setting_patches", "get_setting_patches");

    ClassDB::bind_method(D_METHOD("add_folder", "folder"), &ProjectModuleManifest::add_folder);
    ClassDB::bind_method(D_METHOD("add_script_spec", "spec"), &ProjectModuleManifest::add_script_spec);
    ClassDB::bind_method(D_METHOD("add_resource_spec", "spec"), &ProjectModuleManifest::add_resource_spec);
    ClassDB::bind_method(D_METHOD("add_scene_spec", "spec"), &ProjectModuleManifest::add_scene_spec);
    ClassDB::bind_method(D_METHOD("add_setting_patch", "patch"), &ProjectModuleManifest::add_setting_patch);

    ClassDB::bind_method(D_METHOD("to_plan_dictionary"), &ProjectModuleManifest::to_plan_dictionary);
    ClassDB::bind_static_method("ProjectModuleManifest", D_METHOD("from_plan_dictionary", "dict"), &ProjectModuleManifest::from_plan_dictionary);
}

void ProjectModuleManifest::add_folder(const String &p_folder) {
    if (p_folder.is_empty()) {
        return;
    }
    if (!folders_.has(p_folder)) {
        folders_.push_back(p_folder);
        emit_changed();
    }
}

void ProjectModuleManifest::add_script_spec(const Ref<ProjectScriptSpec> &p_spec) {
    if (p_spec.is_valid()) {
        scripts_.append(p_spec);
        emit_changed();
    }
}

void ProjectModuleManifest::add_resource_spec(const Ref<ProjectResourceSpec> &p_spec) {
    if (p_spec.is_valid()) {
        resources_.append(p_spec);
        emit_changed();
    }
}

void ProjectModuleManifest::add_scene_spec(const Ref<ProjectSceneSpec> &p_spec) {
    if (p_spec.is_valid()) {
        scenes_.append(p_spec);
        emit_changed();
    }
}

void ProjectModuleManifest::add_setting_patch(const Ref<ProjectSettingPatch> &p_patch) {
    if (p_patch.is_valid()) {
        setting_patches_.append(p_patch);
        emit_changed();
    }
}

Dictionary ProjectModuleManifest::to_plan_dictionary() const {
    Dictionary d;
    d["module_id"]     = module_id_;
    d["display_name"]  = display_name_;
    d["plugin_domain"] = plugin_domain_;
    d["description"]   = description_;
    d["semantic_tags"] = semantic_tags_;
    d["enabled"]       = enabled_;
    d["folders"]       = folders_;

    Array scripts_arr;
    for (int i = 0; i < scripts_.size(); ++i) {
        Ref<ProjectScriptSpec> s = scripts_[i];
        if (s.is_valid()) scripts_arr.append(s->to_plan_dictionary());
    }
    d["scripts"] = scripts_arr;

    Array resources_arr;
    for (int i = 0; i < resources_.size(); ++i) {
        Ref<ProjectResourceSpec> r = resources_[i];
        if (r.is_valid()) resources_arr.append(r->to_plan_dictionary());
    }
    d["resources"] = resources_arr;

    Array scenes_arr;
    for (int i = 0; i < scenes_.size(); ++i) {
        Ref<ProjectSceneSpec> sc = scenes_[i];
        if (sc.is_valid()) scenes_arr.append(sc->to_plan_dictionary());
    }
    d["scenes"] = scenes_arr;

    Array patches_arr;
    for (int i = 0; i < setting_patches_.size(); ++i) {
        Ref<ProjectSettingPatch> p = setting_patches_[i];
        if (p.is_valid()) patches_arr.append(p->to_plan_dictionary());
    }
    d["setting_patches"] = patches_arr;
    return d;
}

Ref<ProjectModuleManifest> ProjectModuleManifest::from_plan_dictionary(const Dictionary &p_dict) {
    Ref<ProjectModuleManifest> m;
    m.instantiate();
    if (p_dict.has("module_id")) m->set_module_id(p_dict["module_id"]);
    if (p_dict.has("display_name")) m->set_display_name(p_dict["display_name"]);
    if (p_dict.has("plugin_domain")) m->set_plugin_domain(p_dict["plugin_domain"]);
    if (p_dict.has("description")) m->set_description(p_dict["description"]);
    if (p_dict.has("semantic_tags")) m->set_semantic_tags(p_dict["semantic_tags"]);
    if (p_dict.has("enabled")) m->set_enabled(bool(p_dict["enabled"]));
    if (p_dict.has("folders")) m->set_folders(p_dict["folders"]);

    if (p_dict.has("scripts")) {
        Array arr = p_dict["scripts"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == Variant::DICTIONARY) {
                m->add_script_spec(ProjectScriptSpec::from_plan_dictionary(arr[i]));
            }
        }
    }
    if (p_dict.has("resources")) {
        Array arr = p_dict["resources"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == Variant::DICTIONARY) {
                m->add_resource_spec(ProjectResourceSpec::from_plan_dictionary(arr[i]));
            }
        }
    }
    if (p_dict.has("scenes")) {
        Array arr = p_dict["scenes"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == Variant::DICTIONARY) {
                m->add_scene_spec(ProjectSceneSpec::from_plan_dictionary(arr[i]));
            }
        }
    }
    if (p_dict.has("setting_patches")) {
        Array arr = p_dict["setting_patches"];
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].get_type() == Variant::DICTIONARY) {
                m->add_setting_patch(ProjectSettingPatch::from_plan_dictionary(arr[i]));
            }
        }
    }
    return m;
}

// ============================================================================
// 6. ProjectBlueprintResource
// ============================================================================

void ProjectBlueprintResource::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_blueprint_name", "name"), &ProjectBlueprintResource::set_blueprint_name);
    ClassDB::bind_method(D_METHOD("get_blueprint_name"), &ProjectBlueprintResource::get_blueprint_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "blueprint_name"), "set_blueprint_name", "get_blueprint_name");

    ClassDB::bind_method(D_METHOD("set_project_root", "root"), &ProjectBlueprintResource::set_project_root);
    ClassDB::bind_method(D_METHOD("get_project_root"), &ProjectBlueprintResource::get_project_root);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "project_root"), "set_project_root", "get_project_root");

    ClassDB::bind_method(D_METHOD("set_profile_overlay_id", "overlay_id"), &ProjectBlueprintResource::set_profile_overlay_id);
    ClassDB::bind_method(D_METHOD("get_profile_overlay_id"), &ProjectBlueprintResource::get_profile_overlay_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "profile_overlay_id"), "set_profile_overlay_id", "get_profile_overlay_id");

    ClassDB::bind_method(D_METHOD("set_modules", "modules"), &ProjectBlueprintResource::set_modules);
    ClassDB::bind_method(D_METHOD("get_modules"), &ProjectBlueprintResource::get_modules);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "modules", PROPERTY_HINT_ARRAY_TYPE, "ProjectModuleManifest"),
        "set_modules",
        "get_modules"
    );

    ClassDB::bind_method(D_METHOD("add_module", "module"), &ProjectBlueprintResource::add_module);
    ClassDB::bind_method(D_METHOD("remove_module_by_id", "module_id"), &ProjectBlueprintResource::remove_module_by_id);
    ClassDB::bind_method(D_METHOD("find_module_by_id", "module_id"), &ProjectBlueprintResource::find_module_by_id);

    ClassDB::bind_method(D_METHOD("to_plan_dictionary"), &ProjectBlueprintResource::to_plan_dictionary);
    ClassDB::bind_static_method("ProjectBlueprintResource", D_METHOD("from_plan_dictionary", "dict"), &ProjectBlueprintResource::from_plan_dictionary);
}

void ProjectBlueprintResource::add_module(const Ref<ProjectModuleManifest> &p_module) {
    if (p_module.is_null()) {
        return;
    }
    if (!p_module->get_module_id().is_empty()) {
        for (int i = 0; i < modules_.size(); ++i) {
            Ref<ProjectModuleManifest> existing = modules_[i];
            if (existing.is_valid() && existing->get_module_id() == p_module->get_module_id()) {
                modules_[i] = p_module;
                emit_changed();
                return;
            }
        }
    }
    modules_.append(p_module);
    emit_changed();
}

void ProjectBlueprintResource::remove_module_by_id(const String &p_module_id) {
    for (int i = modules_.size() - 1; i >= 0; --i) {
        Ref<ProjectModuleManifest> existing = modules_[i];
        if (existing.is_valid() && existing->get_module_id() == p_module_id) {
            modules_.remove_at(i);
            emit_changed();
        }
    }
}

Ref<ProjectModuleManifest> ProjectBlueprintResource::find_module_by_id(const String &p_module_id) const {
    for (int i = 0; i < modules_.size(); ++i) {
        Ref<ProjectModuleManifest> existing = modules_[i];
        if (existing.is_valid() && existing->get_module_id() == p_module_id) {
            return existing;
        }
    }
    return Ref<ProjectModuleManifest>();
}

Dictionary ProjectBlueprintResource::to_plan_dictionary() const {
    Dictionary d;
    d["blueprint_name"]     = blueprint_name_;
    d["project_root"]       = project_root_;
    d["profile_overlay_id"] = profile_overlay_id_;
    Array mods;
    for (int i = 0; i < modules_.size(); ++i) {
        Ref<ProjectModuleManifest> m = modules_[i];
        if (m.is_valid()) {
            mods.append(m->to_plan_dictionary());
        }
    }
    d["modules"] = mods;
    return d;
}

Ref<ProjectBlueprintResource> ProjectBlueprintResource::from_plan_dictionary(const Dictionary &p_dict) {
    Ref<ProjectBlueprintResource> bp;
    bp.instantiate();
    if (p_dict.has("blueprint_name")) bp->set_blueprint_name(p_dict["blueprint_name"]);
    if (p_dict.has("project_root")) bp->set_project_root(p_dict["project_root"]);
    if (p_dict.has("profile_overlay_id")) bp->set_profile_overlay_id(p_dict["profile_overlay_id"]);
    if (p_dict.has("modules")) {
        Array mods = p_dict["modules"];
        for (int i = 0; i < mods.size(); ++i) {
            if (mods[i].get_type() == Variant::DICTIONARY) {
                bp->add_module(ProjectModuleManifest::from_plan_dictionary(mods[i]));
            }
        }
    }
    return bp;
}

} // namespace ideam::godot_ext
