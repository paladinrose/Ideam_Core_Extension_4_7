#include "project_blueprint_editor.h"

#include "../../tools/tool_registry.h"

#include <godot_cpp/classes/tree_item.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void ProjectBlueprintEditor::_bind_methods() {
    BIND_ENUM_CONSTANT(MODE_WIZARD);
    BIND_ENUM_CONSTANT(MODE_AUTHOR);

    ADD_SIGNAL(MethodInfo("spec_selected", PropertyInfo(Variant::OBJECT, "spec")));
    ADD_SIGNAL(MethodInfo("blueprint_modified"));

    ClassDB::bind_method(D_METHOD("set_blueprint", "blueprint"), &ProjectBlueprintEditor::set_blueprint);
    ClassDB::bind_method(D_METHOD("get_blueprint"), &ProjectBlueprintEditor::get_blueprint);
    ClassDB::bind_method(D_METHOD("set_manifest", "manifest"), &ProjectBlueprintEditor::set_manifest);
    ClassDB::bind_method(D_METHOD("get_manifest"), &ProjectBlueprintEditor::get_manifest);
    ClassDB::bind_method(D_METHOD("set_editor_mode", "mode"), &ProjectBlueprintEditor::set_editor_mode);
    ClassDB::bind_method(D_METHOD("get_editor_mode"), &ProjectBlueprintEditor::get_editor_mode);
    ClassDB::bind_method(D_METHOD("apply_validation_result", "validation"), &ProjectBlueprintEditor::apply_validation_result);
    ClassDB::bind_method(D_METHOD("refresh"), &ProjectBlueprintEditor::refresh);

    ClassDB::bind_method(D_METHOD("_on_root_path_changed", "new_path"), &ProjectBlueprintEditor::_on_root_path_changed);
    ClassDB::bind_method(D_METHOD("_on_tree_item_selected"), &ProjectBlueprintEditor::_on_tree_item_selected);
    ClassDB::bind_method(D_METHOD("_on_tree_item_edited"), &ProjectBlueprintEditor::_on_tree_item_edited);
    ClassDB::bind_method(D_METHOD("_on_add_folder_pressed"), &ProjectBlueprintEditor::_on_add_folder_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_script_pressed"), &ProjectBlueprintEditor::_on_add_script_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_resource_pressed"), &ProjectBlueprintEditor::_on_add_resource_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_scene_pressed"), &ProjectBlueprintEditor::_on_add_scene_pressed);
}

ProjectBlueprintEditor::ProjectBlueprintEditor() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    engine_.instantiate();

    // Header bar: target project root
    header_bar_ = memnew(HBoxContainer);
    header_bar_->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(header_bar_);

    Label *root_lbl = memnew(Label);
    root_lbl->set_text("Project Root:");
    header_bar_->add_child(root_lbl);

    root_path_edit_ = memnew(LineEdit);
    root_path_edit_->set_h_size_flags(SIZE_EXPAND_FILL);
    root_path_edit_->set_text("res://");
    root_path_edit_->set_placeholder("res://");
    root_path_edit_->connect("text_changed", Callable(this, "_on_root_path_changed"));
    header_bar_->add_child(root_path_edit_);

    // Authoring bar (shown in MODE_AUTHOR)
    author_bar_ = memnew(HBoxContainer);
    author_bar_->set_h_size_flags(SIZE_EXPAND_FILL);
    author_bar_->set_visible(false);
    add_child(author_bar_);

    add_folder_btn_ = memnew(Button);
    add_folder_btn_->set_text("+ Folder");
    add_folder_btn_->connect("pressed", Callable(this, "_on_add_folder_pressed"));
    author_bar_->add_child(add_folder_btn_);

    add_script_btn_ = memnew(Button);
    add_script_btn_->set_text("+ Script");
    add_script_btn_->connect("pressed", Callable(this, "_on_add_script_pressed"));
    author_bar_->add_child(add_script_btn_);

    add_resource_btn_ = memnew(Button);
    add_resource_btn_->set_text("+ Resource");
    add_resource_btn_->connect("pressed", Callable(this, "_on_add_resource_pressed"));
    author_bar_->add_child(add_resource_btn_);

    add_scene_btn_ = memnew(Button);
    add_scene_btn_->set_text("+ Scene");
    add_scene_btn_->connect("pressed", Callable(this, "_on_add_scene_pressed"));
    author_bar_->add_child(add_scene_btn_);

    // Main Vertical Split: Scaffold Tree + Embedded Tier 1 Preview
    main_split_ = memnew(VSplitContainer);
    main_split_->set_h_size_flags(SIZE_EXPAND_FILL);
    main_split_->set_v_size_flags(SIZE_EXPAND_FILL);
    add_child(main_split_);

    scaffold_tree_ = memnew(Tree);
    scaffold_tree_->set_h_size_flags(SIZE_EXPAND_FILL);
    scaffold_tree_->set_v_size_flags(SIZE_EXPAND_FILL);
    scaffold_tree_->set_hide_root(true);
    scaffold_tree_->set_columns(3);
    scaffold_tree_->set_column_title(0, "Item");
    scaffold_tree_->set_column_title(1, "Type / Target");
    scaffold_tree_->set_column_title(2, "Status");
    scaffold_tree_->set_column_titles_visible(true);
    scaffold_tree_->connect("item_selected", Callable(this, "_on_tree_item_selected"));
    scaffold_tree_->connect("item_edited", Callable(this, "_on_tree_item_edited"));
    main_split_->add_child(scaffold_tree_);

    preview_panel_ = memnew(PanelContainer);
    preview_panel_->set_h_size_flags(SIZE_EXPAND_FILL);
    preview_panel_->set_v_size_flags(SIZE_EXPAND_FILL);
    preview_panel_->set_custom_minimum_size(Vector2(0.0f, 220.0f));
    preview_panel_->set_visible(false);
    main_split_->add_child(preview_panel_);

    preview_vbox_ = memnew(VBoxContainer);
    preview_vbox_->set_h_size_flags(SIZE_EXPAND_FILL);
    preview_vbox_->set_v_size_flags(SIZE_EXPAND_FILL);
    preview_panel_->add_child(preview_vbox_);

    preview_header_label_ = memnew(Label);
    preview_header_label_->set_text("Tier 1 Starter Asset Preview");
    preview_vbox_->add_child(preview_header_label_);
}

void ProjectBlueprintEditor::set_blueprint(const Ref<ProjectBlueprintResource> &p_blueprint) {
    blueprint_       = p_blueprint;
    single_manifest_.unref();
    if (header_bar_) {
        header_bar_->set_visible(true);
    }
    if (blueprint_.is_valid() && root_path_edit_) {
        root_path_edit_->set_text(blueprint_->get_project_root());
    }
    _clear_tier1_preview();
    _rebuild_tree();
}

void ProjectBlueprintEditor::set_manifest(const Ref<ProjectModuleManifest> &p_manifest) {
    single_manifest_ = p_manifest;
    blueprint_.unref();
    if (header_bar_) {
        header_bar_->set_visible(false);
    }
    _clear_tier1_preview();
    _rebuild_tree();
}

void ProjectBlueprintEditor::set_editor_mode(EditorMode p_mode) {
    mode_ = p_mode;
    if (author_bar_) {
        author_bar_->set_visible(mode_ == MODE_AUTHOR);
    }
    _rebuild_tree();
}

void ProjectBlueprintEditor::apply_validation_result(const Dictionary &p_validation_result) {
    last_validation_map_.clear();
    if (p_validation_result.has("diagnostics")) {
        Array diags = p_validation_result["diagnostics"];
        for (int i = 0; i < diags.size(); ++i) {
            Dictionary d        = diags[i];
            const String target = d.get("target_path", "");
            if (!target.is_empty()) {
                last_validation_map_[target] = d;
            }
        }
    }
    _rebuild_tree();
}

void ProjectBlueprintEditor::refresh() {
    _rebuild_tree();
}

void ProjectBlueprintEditor::_rebuild_tree() {
    if (!scaffold_tree_) {
        return;
    }
    scaffold_tree_->clear();
    TreeItem *root = scaffold_tree_->create_item();

    if (blueprint_.is_valid()) {
        const TypedArray<ProjectModuleManifest> mods = blueprint_->get_modules();
        for (int i = 0; i < mods.size(); ++i) {
            Ref<ProjectModuleManifest> mod = mods[i];
            if (mod.is_valid()) {
                _populate_module_branch(root, mod);
            }
        }
    } else if (single_manifest_.is_valid()) {
        _populate_module_branch(root, single_manifest_);
    }
}

void ProjectBlueprintEditor::_populate_module_branch(TreeItem *p_parent, const Ref<ProjectModuleManifest> &p_mod) {
    const String proj_root = blueprint_.is_valid() ? blueprint_->get_project_root() : String("res://");

    TreeItem *mod_item = scaffold_tree_->create_item(p_parent);
    mod_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
    mod_item->set_checked(0, p_mod->is_enabled());
    mod_item->set_editable(0, true);
    mod_item->set_text(0, p_mod->get_display_name().is_empty() ? p_mod->get_module_id() : p_mod->get_display_name());
    mod_item->set_text(1, p_mod->get_plugin_domain().is_empty() ? String("Module") : p_mod->get_plugin_domain());
    mod_item->set_metadata(0, p_mod);

    auto apply_status = [&](TreeItem *p_item, const String &p_full_path) {
        if (last_validation_map_.has(p_full_path)) {
            Dictionary diag = last_validation_map_[p_full_path];
            const int sev   = diag.get("severity", 0);
            const String msg = diag.get("message", "");
            if (sev == PROJECT_DIAG_ERROR) {
                p_item->set_text(2, String("ERROR: ") + msg);
            } else if (sev == PROJECT_DIAG_WARNING) {
                p_item->set_text(2, String("WARN: ") + msg);
            } else {
                p_item->set_text(2, "EXISTS");
            }
        } else {
            p_item->set_text(2, "OK");
        }
    };

    // Folders
    const PackedStringArray folders = p_mod->get_folders();
    for (int i = 0; i < folders.size(); ++i) {
        const String full = ProjectScaffoldEngine::normalize_res_path(proj_root, folders[i]);
        TreeItem *f_item  = scaffold_tree_->create_item(mod_item);
        f_item->set_text(0, String("[Folder] ") + folders[i]);
        f_item->set_text(1, full);
        apply_status(f_item, full);
    }

    // Scripts
    const TypedArray<ProjectScriptSpec> scripts = p_mod->get_scripts();
    for (int i = 0; i < scripts.size(); ++i) {
        Ref<ProjectScriptSpec> s = scripts[i];
        if (s.is_null()) continue;
        const String fname = ProjectScaffoldEngine::sanitize_filename(s->get_script_name()).to_lower();
        const String full  = ProjectScaffoldEngine::normalize_res_path(proj_root, s->get_target_folder()).path_join(fname + ".gd");

        TreeItem *s_item = scaffold_tree_->create_item(mod_item);
        s_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
        s_item->set_checked(0, s->is_enabled());
        s_item->set_editable(0, true);
        s_item->set_text(0, String("[Script] ") + s->get_script_name());
        s_item->set_text(1, String("extends ") + s->get_base_class());
        s_item->set_metadata(0, s);
        apply_status(s_item, full);
    }

    // Resources
    const TypedArray<ProjectResourceSpec> resources = p_mod->get_resources();
    for (int i = 0; i < resources.size(); ++i) {
        Ref<ProjectResourceSpec> r = resources[i];
        if (r.is_null()) continue;
        const String fname = ProjectScaffoldEngine::sanitize_filename(r->get_resource_name());
        const String ext   = r->get_file_extension().is_empty() ? String("tres") : r->get_file_extension().trim_prefix(".");
        const String full  = ProjectScaffoldEngine::normalize_res_path(proj_root, r->get_target_folder()).path_join(fname + "." + ext);

        TreeItem *r_item = scaffold_tree_->create_item(mod_item);
        r_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
        r_item->set_checked(0, r->is_enabled());
        r_item->set_editable(0, true);
        r_item->set_text(0, String("[Resource] ") + r->get_resource_name());
        r_item->set_text(1, r->get_resource_class());
        r_item->set_metadata(0, r);
        apply_status(r_item, full);
    }

    // Scenes
    const TypedArray<ProjectSceneSpec> scenes = p_mod->get_scenes();
    for (int i = 0; i < scenes.size(); ++i) {
        Ref<ProjectSceneSpec> sc = scenes[i];
        if (sc.is_null()) continue;
        const String fname = ProjectScaffoldEngine::sanitize_filename(sc->get_scene_name());
        const String full  = ProjectScaffoldEngine::normalize_res_path(proj_root, sc->get_target_folder()).path_join(fname + ".tscn");

        TreeItem *sc_item = scaffold_tree_->create_item(mod_item);
        sc_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
        sc_item->set_checked(0, sc->is_enabled());
        sc_item->set_editable(0, true);
        sc_item->set_text(0, String("[Scene] ") + sc->get_scene_name());
        const String root_cls = sc->get_root_node().is_valid() ? sc->get_root_node()->get_node_class() : String("Node");
        sc_item->set_text(1, String("Root: ") + root_cls);
        sc_item->set_metadata(0, sc);
        apply_status(sc_item, full);
    }
}

void ProjectBlueprintEditor::_clear_tier1_preview() {
    if (active_tier1_preview_) {
        if (active_tier1_preview_->get_parent()) {
            active_tier1_preview_->get_parent()->remove_child(active_tier1_preview_);
        }
        active_tier1_preview_->queue_free();
        active_tier1_preview_ = nullptr;
    }
    preview_resource_.unref();
    if (preview_panel_) {
        preview_panel_->set_visible(false);
    }
}

void ProjectBlueprintEditor::_try_mount_tier1_preview(const Ref<ProjectResourceSpec> &p_spec) {
    _clear_tier1_preview();
    if (p_spec.is_null() || !preview_panel_ || !preview_vbox_) {
        return;
    }

    const auto *desc = ToolRegistry::get_singleton().find_any_descriptor_for_class(
        StringName(p_spec->get_resource_class())
    );
    if (!desc || !desc->create_control) {
        return;
    }

    ClassDB *cdb = ClassDB::get_singleton();
    if (!cdb || !cdb->class_exists(p_spec->get_resource_class()) || !cdb->can_instantiate(p_spec->get_resource_class())) {
        return;
    }

    Variant inst = cdb->instantiate(p_spec->get_resource_class());
    Ref<Resource> res = Object::cast_to<Resource>(static_cast<Object *>(inst));
    if (res.is_null()) {
        return;
    }

    Control *ctrl = desc->create_control(res, tool_context_);
    if (!ctrl) {
        return;
    }

    preview_resource_     = res;
    active_tier1_preview_ = ctrl;
    active_tier1_preview_->set_h_size_flags(SIZE_EXPAND_FILL);
    active_tier1_preview_->set_v_size_flags(SIZE_EXPAND_FILL);
    preview_vbox_->add_child(active_tier1_preview_);
    if (preview_header_label_) {
        preview_header_label_->set_text(
            String("Tier 1 Preview: ") + desc->display_label + " (Owned by " + String(desc->workspace_id) + ")"
        );
    }
    preview_panel_->set_visible(true);
}

Ref<ProjectModuleManifest> ProjectBlueprintEditor::_get_target_module_for_authoring() const {
    if (single_manifest_.is_valid()) {
        return single_manifest_;
    }
    if (scaffold_tree_) {
        TreeItem *sel = scaffold_tree_->get_selected();
        while (sel) {
            Variant meta = sel->get_metadata(0);
            if (meta.get_type() == Variant::OBJECT) {
                if (auto *mod = Object::cast_to<ProjectModuleManifest>(static_cast<Object *>(meta))) {
                    return Ref<ProjectModuleManifest>(mod);
                }
            }
            sel = sel->get_parent();
        }
    }
    if (blueprint_.is_valid()) {
        TypedArray<ProjectModuleManifest> mods = blueprint_->get_modules();
        if (!mods.is_empty()) {
            return mods[0];
        }
        Ref<ProjectModuleManifest> created;
        created.instantiate();
        created->set_module_id("custom_module");
        created->set_display_name("Custom Module");
        created->set_plugin_domain("Core");
        blueprint_->add_module(created);
        return created;
    }
    return Ref<ProjectModuleManifest>();
}

void ProjectBlueprintEditor::_on_root_path_changed(const String &p_new_path) {
    if (blueprint_.is_valid()) {
        blueprint_->set_project_root(p_new_path);
        emit_signal("blueprint_modified");
    }
}

void ProjectBlueprintEditor::_on_tree_item_selected() {
    if (!scaffold_tree_) {
        return;
    }
    TreeItem *sel = scaffold_tree_->get_selected();
    if (!sel) {
        _clear_tier1_preview();
        emit_signal("spec_selected", Variant());
        return;
    }
    Variant meta = sel->get_metadata(0);
    if (meta.get_type() == Variant::OBJECT) {
        Object *obj = meta;
        emit_signal("spec_selected", obj);
        if (auto *res_spec = Object::cast_to<ProjectResourceSpec>(obj)) {
            _try_mount_tier1_preview(Ref<ProjectResourceSpec>(res_spec));
        } else {
            _clear_tier1_preview();
        }
    } else {
        _clear_tier1_preview();
        emit_signal("spec_selected", Variant());
    }
}

void ProjectBlueprintEditor::_on_tree_item_edited() {
    if (!scaffold_tree_) {
        return;
    }
    TreeItem *edited = scaffold_tree_->get_edited();
    if (!edited) {
        return;
    }
    const bool checked = edited->is_checked(0);
    Variant meta       = edited->get_metadata(0);
    if (meta.get_type() == Variant::OBJECT) {
        Object *obj = meta;
        if (auto *mod = Object::cast_to<ProjectModuleManifest>(obj)) {
            mod->set_enabled(checked);
        } else if (auto *s = Object::cast_to<ProjectScriptSpec>(obj)) {
            s->set_enabled(checked);
        } else if (auto *r = Object::cast_to<ProjectResourceSpec>(obj)) {
            r->set_enabled(checked);
        } else if (auto *sc = Object::cast_to<ProjectSceneSpec>(obj)) {
            sc->set_enabled(checked);
        }
        emit_signal("blueprint_modified");
    }
}

void ProjectBlueprintEditor::_on_add_folder_pressed() {
    Ref<ProjectModuleManifest> mod = _get_target_module_for_authoring();
    if (mod.is_null()) return;
    mod->add_folder(String("new_folder_") + String::num_int64(mod->get_folders().size() + 1));
    emit_signal("blueprint_modified");
    _rebuild_tree();
}

void ProjectBlueprintEditor::_on_add_script_pressed() {
    Ref<ProjectModuleManifest> mod = _get_target_module_for_authoring();
    if (mod.is_null()) return;
    Ref<ProjectScriptSpec> spec;
    spec.instantiate();
    spec->set_target_folder("scripts");
    spec->set_script_name(String("new_script_") + String::num_int64(mod->get_scripts().size() + 1));
    spec->set_base_class("Node");
    mod->add_script_spec(spec);
    emit_signal("blueprint_modified");
    _rebuild_tree();
}

void ProjectBlueprintEditor::_on_add_resource_pressed() {
    Ref<ProjectModuleManifest> mod = _get_target_module_for_authoring();
    if (mod.is_null()) return;
    Ref<ProjectResourceSpec> spec;
    spec.instantiate();
    spec->set_target_folder("resources");
    spec->set_resource_name(String("new_resource_") + String::num_int64(mod->get_resources().size() + 1));
    spec->set_resource_class("Resource");
    mod->add_resource_spec(spec);
    emit_signal("blueprint_modified");
    _rebuild_tree();
}

void ProjectBlueprintEditor::_on_add_scene_pressed() {
    Ref<ProjectModuleManifest> mod = _get_target_module_for_authoring();
    if (mod.is_null()) return;
    Ref<ProjectSceneSpec> spec;
    spec.instantiate();
    spec->set_target_folder("scenes");
    spec->set_scene_name(String("new_scene_") + String::num_int64(mod->get_scenes().size() + 1));
    mod->add_scene_spec(spec);
    emit_signal("blueprint_modified");
    _rebuild_tree();
}

} // namespace ideam::godot_ext
