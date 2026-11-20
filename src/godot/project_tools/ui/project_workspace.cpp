#include "project_workspace.h"

#include "../ideam_project_plugin.h"
#include "../../controls/ideam_tier1_control_registry.h"

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

namespace ideam::godot_ext {

ProjectWorkspace *ProjectWorkspace::shared_instance_ = nullptr;
Window *ProjectWorkspace::shared_window_             = nullptr;

void ProjectWorkspace::_bind_methods() {
    ADD_SIGNAL(MethodInfo("project_generated", PropertyInfo(Variant::DICTIONARY, "receipt")));
    ADD_SIGNAL(MethodInfo("validation_completed", PropertyInfo(Variant::DICTIONARY, "result")));
    ADD_SIGNAL(MethodInfo("workspace_handoff_launched", PropertyInfo(Variant::STRING, "workspace_id")));

    ClassDB::bind_method(D_METHOD("discover_and_open_active_blueprint"), &ProjectWorkspace::discover_and_open_active_blueprint);
    ClassDB::bind_method(D_METHOD("run_validation"), &ProjectWorkspace::run_validation);
    ClassDB::bind_method(D_METHOD("run_generation"), &ProjectWorkspace::run_generation);
    ClassDB::bind_method(D_METHOD("launch_handoff_workspace", "workspace_id"), &ProjectWorkspace::launch_handoff_workspace);

    ClassDB::bind_method(D_METHOD("_on_discover_pressed"), &ProjectWorkspace::_on_discover_pressed);
    ClassDB::bind_method(D_METHOD("_on_mode_selected", "index"), &ProjectWorkspace::_on_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_validate_pressed"), &ProjectWorkspace::_on_validate_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_pressed"), &ProjectWorkspace::_on_generate_pressed);
    ClassDB::bind_method(D_METHOD("_on_handoff_workspace_pressed", "workspace_id"), &ProjectWorkspace::_on_handoff_workspace_pressed);
    ClassDB::bind_method(D_METHOD("_on_editor_spec_selected", "spec"), &ProjectWorkspace::_on_editor_spec_selected);
    ClassDB::bind_method(D_METHOD("_on_editor_blueprint_modified"), &ProjectWorkspace::_on_editor_blueprint_modified);
    ClassDB::bind_method(D_METHOD("_on_spec_text_changed", "text", "prop_name"), &ProjectWorkspace::_on_spec_text_changed);
    ClassDB::bind_method(D_METHOD("_on_spec_bool_toggled", "pressed", "prop_name"), &ProjectWorkspace::_on_spec_bool_toggled);
    ClassDB::bind_method(D_METHOD("_on_pick_class_pressed", "target", "base_filter"), &ProjectWorkspace::_on_pick_class_pressed);
    ClassDB::bind_method(D_METHOD("_on_class_picked", "class_name"), &ProjectWorkspace::_on_class_picked);

    ClassDB::bind_static_method(
        "ProjectWorkspace",
        D_METHOD("open_blueprint", "blueprint", "owner"),
        &ProjectWorkspace::open_blueprint,
        DEFVAL(nullptr)
    );
    ClassDB::bind_static_method(
        "ProjectWorkspace",
        D_METHOD("open_manifest", "manifest", "owner"),
        &ProjectWorkspace::open_manifest,
        DEFVAL(nullptr)
    );
}

ProjectWorkspace::ProjectWorkspace() {
    engine_.instantiate();
    set_workspace_id(StringName(WORKSPACE_ID));

    // Pre-create canonical tabs; active plugin domains are added dynamically during discovery
    ensure_sidebar_tab("Blueprints");
    ensure_sidebar_tab("Modules");

    configure_theme_registry("res://addons/ideam_project_tools/resources/project_workspace_theme_registry.tres");

    if (tool_context_.is_valid()) {
        tool_context_->set_update_commit_policy(UPDATE_EXPLICIT_TRIGGER);
#ifdef TOOLS_ENABLED
        if (EditorUndoRedoManager *ed_ur = IdeamProjectPlugin::get_static_undo_redo()) {
            tool_context_->set_editor_undo_redo(ed_ur);
        }
#endif
    }

    // Phase 3: Post-generation Tier 2 Handoff Strip (sits right beneath the dirty banner)
    handoff_panel_ = memnew(PanelContainer);
    handoff_panel_->set_h_size_flags(SIZE_EXPAND_FILL);
    handoff_panel_->set_visible(false);
    add_child(handoff_panel_);
    if (workspace_split_) {
        move_child(handoff_panel_, workspace_split_->get_index());
    }

    handoff_bar_ = memnew(HBoxContainer);
    handoff_bar_->set_h_size_flags(SIZE_EXPAND_FILL);
    handoff_panel_->add_child(handoff_bar_);

    // Mount typed spec editor fields container in the inherited right-hand inspector pane
    if (inspector_vbox_) {
        spec_fields_box_ = memnew(VBoxContainer);
        spec_fields_box_->set_h_size_flags(SIZE_EXPAND_FILL);
        inspector_vbox_->add_child(spec_fields_box_);
        inspector_vbox_->move_child(spec_fields_box_, 1);
    }

    class_picker_ = memnew(ClassPickerDialog);
    class_picker_->connect("class_selected", Callable(this, "_on_class_picked"));
    add_child(class_picker_);

    _populate_command_bar(custom_actions_box_);
}

ProjectWorkspace::~ProjectWorkspace() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

void ProjectWorkspace::_populate_command_bar(HBoxContainer *p_custom_actions) {
    if (!p_custom_actions) {
        return;
    }

    p_custom_actions->add_child(memnew(VSeparator));

    discover_btn_ = memnew(Button);
    discover_btn_->set_text("Discover Plugins");
    discover_btn_->set_tooltip_text("Scan active Ideam plugins and C++ contributions to assemble a fresh Project Blueprint.");
    discover_btn_->connect("pressed", Callable(this, "_on_discover_pressed"));
    p_custom_actions->add_child(discover_btn_);

    mode_btn_ = memnew(OptionButton);
    mode_btn_->add_item("Mode: Wizard", static_cast<int>(ProjectBlueprintEditor::MODE_WIZARD));
    mode_btn_->add_item("Mode: Author", static_cast<int>(ProjectBlueprintEditor::MODE_AUTHOR));
    mode_btn_->select(static_cast<int>(ProjectBlueprintEditor::MODE_WIZARD));
    mode_btn_->connect("item_selected", Callable(this, "_on_mode_selected"));
    p_custom_actions->add_child(mode_btn_);

    overwrite_check_ = memnew(CheckBox);
    overwrite_check_->set_text("Overwrite");
    overwrite_check_->set_tooltip_text("Allow overwriting existing scripts, resources, and scenes during generation.");
    p_custom_actions->add_child(overwrite_check_);

    validate_btn_ = memnew(Button);
    validate_btn_->set_text("Validate");
    validate_btn_->set_tooltip_text("Run a non-destructive dry-run validation on the active Project Blueprint.");
    validate_btn_->connect("pressed", Callable(this, "_on_validate_pressed"));
    p_custom_actions->add_child(validate_btn_);

    generate_btn_ = memnew(Button);
    generate_btn_->set_text("Generate Project");
    generate_btn_->set_tooltip_text("Create folders, templated scripts, resources, scenes, and settings patches.");
    generate_btn_->connect("pressed", Callable(this, "_on_generate_pressed"));
    p_custom_actions->add_child(generate_btn_);
}

ProjectBlueprintEditor *ProjectWorkspace::_get_active_blueprint_editor() const {
    for (const auto &entry : open_assets_) {
        if (entry.asset == active_asset_ && entry.tier1_control) {
            return Object::cast_to<ProjectBlueprintEditor>(entry.tier1_control);
        }
    }
    return nullptr;
}

Ref<ProjectBlueprintResource> ProjectWorkspace::_get_active_blueprint() const {
    if (Ref<ProjectBlueprintResource> bp = active_asset_; bp.is_valid()) {
        return bp;
    }
    if (Ref<ProjectModuleManifest> mod = active_asset_; mod.is_valid()) {
        Ref<ProjectBlueprintResource> wrapper;
        wrapper.instantiate();
        wrapper->set_blueprint_name(mod->get_display_name().is_empty() ? mod->get_module_id() : mod->get_display_name());
        wrapper->add_module(mod);
        return wrapper;
    }
    return Ref<ProjectBlueprintResource>();
}

void ProjectWorkspace::_on_active_asset_switched(const Ref<Resource> &p_asset) {
    (void)p_asset;
    if (ProjectBlueprintEditor *ed = _get_active_blueprint_editor()) {
        const Callable sel_cb(this, "_on_editor_spec_selected");
        if (!ed->is_connected("spec_selected", sel_cb)) {
            ed->connect("spec_selected", sel_cb);
        }
        const Callable mod_cb(this, "_on_editor_blueprint_modified");
        if (!ed->is_connected("blueprint_modified", mod_cb)) {
            ed->connect("blueprint_modified", mod_cb);
        }
        if (mode_btn_) {
            ed->set_editor_mode(static_cast<ProjectBlueprintEditor::EditorMode>(mode_btn_->get_selected()));
        }
    }
    _rebuild_spec_inspector(Ref<Resource>());
}

void ProjectWorkspace::_on_dirty_banner_action_pressed() {
    run_validation();
}

void ProjectWorkspace::discover_and_open_active_blueprint() {
    if (engine_.is_null()) {
        engine_.instantiate();
    }
    Ref<ProjectBlueprintResource> bp = engine_->discover_active_blueprint("res://");
    if (bp.is_null()) {
        return;
    }
    open_asset(bp, "Blueprints");

    // Also populate per-domain sidebar entries for each discovered module
    const TypedArray<ProjectModuleManifest> mods = bp->get_modules();
    for (int i = 0; i < mods.size(); ++i) {
        Ref<ProjectModuleManifest> mod = mods[i];
        if (mod.is_valid()) {
            const String tab = mod->get_plugin_domain().is_empty() ? String("Modules") : mod->get_plugin_domain();
            ensure_sidebar_tab(tab);
            open_asset(mod, tab);
        }
    }
    select_asset(bp);
    run_validation();
}

Dictionary ProjectWorkspace::run_validation() {
    Ref<ProjectBlueprintResource> bp = _get_active_blueprint();
    if (bp.is_null() || engine_.is_null()) {
        return Dictionary();
    }
    const bool allow_overwrite = overwrite_check_ && overwrite_check_->is_pressed();
    const Dictionary result    = engine_->validate_blueprint(bp, allow_overwrite);

    if (ProjectBlueprintEditor *ed = _get_active_blueprint_editor()) {
        ed->apply_validation_result(result);
    }

    const int errors   = result.get("error_count", 0);
    const int warnings = result.get("warning_count", 0);
    Dictionary summary = result.get("summary", Dictionary());
    if (errors > 0) {
        set_dirty_banner(
            true,
            String("Validation failed: ") + String::num_int64(errors) + " error(s), " + String::num_int64(warnings) + " warning(s).",
            "Re-Validate"
        );
    } else {
        set_dirty_banner(
            true,
            String("Ready: ") + String::num_int64(int(summary.get("folders", 0))) + " folders, "
                + String::num_int64(int(summary.get("scripts", 0))) + " scripts, "
                + String::num_int64(int(summary.get("resources", 0))) + " resources, "
                + String::num_int64(int(summary.get("scenes", 0))) + " scenes ("
                + String::num_int64(warnings) + " warning(s)).",
            "Re-Validate"
        );
    }

    emit_signal("validation_completed", result);
    return result;
}

Dictionary ProjectWorkspace::run_generation() {
    Ref<ProjectBlueprintResource> bp = _get_active_blueprint();
    if (bp.is_null() || engine_.is_null()) {
        return Dictionary();
    }
    const bool allow_overwrite = overwrite_check_ && overwrite_check_->is_pressed();
    const Dictionary receipt   = engine_->execute_blueprint(bp, allow_overwrite);

    if (bool(receipt.get("success", false))) {
        const PackedStringArray folders   = receipt.get("created_folders", PackedStringArray());
        const PackedStringArray scripts   = receipt.get("created_scripts", PackedStringArray());
        const PackedStringArray resources = receipt.get("created_resources", PackedStringArray());
        const PackedStringArray scenes    = receipt.get("created_scenes", PackedStringArray());
        set_dirty_banner(
            true,
            String("Generated ") + String::num_int64(folders.size()) + " folders, "
                + String::num_int64(scripts.size()) + " scripts, "
                + String::num_int64(resources.size()) + " resources, "
                + String::num_int64(scenes.size()) + " scenes.",
            "Re-Validate"
        );
        _rebuild_handoff_strip(receipt.get("owner_workspaces", Dictionary()));
    } else {
        set_dirty_banner(true, "Generation blocked by validation errors. Check tree status column.", "Re-Validate");
        _clear_handoff_strip();
    }

    if (ProjectBlueprintEditor *ed = _get_active_blueprint_editor()) {
        ed->apply_validation_result(engine_->validate_blueprint(bp, allow_overwrite));
    }

    emit_signal("project_generated", receipt);
    return receipt;
}

// ============================================================================
// Phase 3: Cross-Tier-2 Handoff Strip
// ============================================================================

void ProjectWorkspace::_clear_handoff_strip() {
    handoff_assets_by_workspace_.clear();
    if (handoff_bar_) {
        for (int i = handoff_bar_->get_child_count() - 1; i >= 0; --i) {
            Node *child = handoff_bar_->get_child(i);
            handoff_bar_->remove_child(child);
            child->queue_free();
        }
    }
    if (handoff_panel_) {
        handoff_panel_->set_visible(false);
    }
}

void ProjectWorkspace::_rebuild_handoff_strip(const Dictionary &p_owner_workspaces) {
    _clear_handoff_strip();
    if (!handoff_bar_ || !handoff_panel_ || p_owner_workspaces.is_empty()) {
        return;
    }

    // Group generated resource paths by owning sibling Tier 2 workspace_id
    Array res_paths = p_owner_workspaces.keys();
    for (int i = 0; i < res_paths.size(); ++i) {
        const String res_path = res_paths[i];
        const String ws_id    = p_owner_workspaces[res_path];
        if (ws_id.is_empty() || ws_id == String(WORKSPACE_ID)) {
            continue;
        }
        PackedStringArray list = handoff_assets_by_workspace_.has(ws_id)
            ? PackedStringArray(handoff_assets_by_workspace_[ws_id])
            : PackedStringArray();
        list.push_back(res_path);
        handoff_assets_by_workspace_[ws_id] = list;
    }

    Array ws_keys = handoff_assets_by_workspace_.keys();
    if (ws_keys.is_empty()) {
        return;
    }

    auto &reg = IdeamTier1ControlRegistry::get_singleton();
    int added = 0;

    Label *lbl = memnew(Label);
    lbl->set_text("Open generated assets in:");
    handoff_bar_->add_child(lbl);

    for (int i = 0; i < ws_keys.size(); ++i) {
        const String ws_id     = ws_keys[i];
        const auto *launcher   = reg.find_workspace_launcher(StringName(ws_id));
        if (!launcher) {
            continue;
        }
        const PackedStringArray paths = handoff_assets_by_workspace_[ws_id];
        const String title = launcher->display_name.is_empty() ? ws_id : launcher->display_name;

        Button *btn = memnew(Button);
        btn->set_text(title + " (" + String::num_int64(paths.size()) + ")");
        btn->set_tooltip_text(String("Open generated assets in ") + title + " with shared IdeamToolContext.");
        btn->connect("pressed", Callable(this, "_on_handoff_workspace_pressed").bind(ws_id));
        handoff_bar_->add_child(btn);
        ++added;
    }

    handoff_panel_->set_visible(added > 0);
}

bool ProjectWorkspace::launch_handoff_workspace(const String &p_workspace_id) {
    if (!handoff_assets_by_workspace_.has(p_workspace_id)) {
        return false;
    }
    const PackedStringArray paths = handoff_assets_by_workspace_[p_workspace_id];
    auto &reg                     = IdeamTier1ControlRegistry::get_singleton();
    bool launched_any             = false;

    for (int i = 0; i < paths.size(); ++i) {
        if (!ResourceLoader::get_singleton()->exists(paths[i])) {
            continue;
        }
        Ref<Resource> asset = ResourceLoader::get_singleton()->load(paths[i]);
        if (asset.is_valid()) {
            if (reg.launch_asset_in_workspace(StringName(p_workspace_id), asset, tool_context_)) {
                launched_any = true;
            }
        }
    }

    if (launched_any) {
        emit_signal("workspace_handoff_launched", p_workspace_id);
    }
    return launched_any;
}

void ProjectWorkspace::_on_handoff_workspace_pressed(const String &p_workspace_id) {
    launch_handoff_workspace(p_workspace_id);
}

// ============================================================================
// Right-Hand Spec Inspector
// ============================================================================

void ProjectWorkspace::_rebuild_spec_inspector(const Ref<Resource> &p_spec) {
    selected_spec_ = p_spec;
    if (!spec_fields_box_) {
        return;
    }
    for (int i = spec_fields_box_->get_child_count() - 1; i >= 0; --i) {
        Node *child = spec_fields_box_->get_child(i);
        spec_fields_box_->remove_child(child);
        child->queue_free();
    }

    if (p_spec.is_null()) {
        if (inspector_title_) {
            inspector_title_->set_text("Inspector");
        }
        return;
    }

    if (inspector_title_) {
        inspector_title_->set_text(String("Spec: ") + String(p_spec->get_class()));
    }

    if (Ref<ProjectModuleManifest> mod = p_spec; mod.is_valid()) {
        _add_text_field("Module ID", mod->get_module_id(), "module_id");
        _add_text_field("Display Name", mod->get_display_name(), "display_name");
        _add_text_field("Plugin Domain", mod->get_plugin_domain(), "plugin_domain");
        _add_bool_field("Enabled", mod->is_enabled(), "enabled");
    } else if (Ref<ProjectScriptSpec> s = p_spec; s.is_valid()) {
        _add_text_field("Target Folder", s->get_target_folder(), "target_folder");
        _add_text_field("Script Name", s->get_script_name(), "script_name");
        _add_text_field("Base Class", s->get_base_class(), "base_class", true, "");
        _add_text_field("class_name", s->get_class_name_override(), "class_name_override");
        _add_text_field("Template", s->get_template_name(), "template_name");
        _add_bool_field("Enabled", s->is_enabled(), "enabled");
    } else if (Ref<ProjectResourceSpec> r = p_spec; r.is_valid()) {
        _add_text_field("Target Folder", r->get_target_folder(), "target_folder");
        _add_text_field("Resource Name", r->get_resource_name(), "resource_name");
        _add_text_field("Resource Class", r->get_resource_class(), "resource_class", true, "Resource");
        _add_text_field("Extension", r->get_file_extension(), "file_extension");
        _add_bool_field("Use Generated Script", r->get_use_generated_script(), "use_generated_script");
        _add_text_field("Script Key", r->get_generated_script_key(), "generated_script_key");
        _add_bool_field("Enabled", r->is_enabled(), "enabled");
    } else if (Ref<ProjectSceneSpec> sc = p_spec; sc.is_valid()) {
        _add_text_field("Target Folder", sc->get_target_folder(), "target_folder");
        _add_text_field("Scene Name", sc->get_scene_name(), "scene_name");
        _add_bool_field("Enabled", sc->is_enabled(), "enabled");
        if (sc->get_root_node().is_valid()) {
            _add_text_field("Root Node Name", sc->get_root_node()->get_node_name(), "root_node.node_name");
            _add_text_field("Root Node Class", sc->get_root_node()->get_node_class(), "root_node.node_class", true, "Node");
            _add_bool_field("Use Generated Script", sc->get_root_node()->get_use_generated_script(), "root_node.use_generated_script");
        }
    }
}

void ProjectWorkspace::_add_text_field(
    const String &p_label,
    const String &p_value,
    const String &p_prop_name,
    bool p_with_class_picker,
    const String &p_base_filter
) {
    HBoxContainer *row = memnew(HBoxContainer);
    row->set_h_size_flags(SIZE_EXPAND_FILL);

    Label *lbl = memnew(Label);
    lbl->set_text(p_label);
    lbl->set_custom_minimum_size(Vector2(108.0f, 0.0f));
    row->add_child(lbl);

    LineEdit *edit = memnew(LineEdit);
    edit->set_text(p_value);
    edit->set_h_size_flags(SIZE_EXPAND_FILL);
    edit->connect("text_changed", Callable(this, "_on_spec_text_changed").bind(p_prop_name));
    row->add_child(edit);

    if (p_with_class_picker) {
        Button *pick = memnew(Button);
        pick->set_text("...");
        pick->set_tooltip_text("Pick class from Ideam / ClassDB / Global Scripts");
        pick->connect("pressed", Callable(this, "_on_pick_class_pressed").bind(edit, p_base_filter));
        row->add_child(pick);
    }

    spec_fields_box_->add_child(row);
}

void ProjectWorkspace::_add_bool_field(const String &p_label, bool p_value, const String &p_prop_name) {
    CheckBox *cb = memnew(CheckBox);
    cb->set_text(p_label);
    cb->set_pressed_no_signal(p_value);
    cb->connect("toggled", Callable(this, "_on_spec_bool_toggled").bind(p_prop_name));
    spec_fields_box_->add_child(cb);
}

// ============================================================================
// Signal Handlers
// ============================================================================

void ProjectWorkspace::_on_discover_pressed() {
    discover_and_open_active_blueprint();
}

void ProjectWorkspace::_on_mode_selected(int p_index) {
    if (ProjectBlueprintEditor *ed = _get_active_blueprint_editor()) {
        ed->set_editor_mode(static_cast<ProjectBlueprintEditor::EditorMode>(p_index));
    }
}

void ProjectWorkspace::_on_validate_pressed() {
    run_validation();
}

void ProjectWorkspace::_on_generate_pressed() {
    run_generation();
}

void ProjectWorkspace::_on_editor_spec_selected(Object *p_spec) {
    Ref<Resource> res = Object::cast_to<Resource>(p_spec);
    _rebuild_spec_inspector(res);
}

void ProjectWorkspace::_on_editor_blueprint_modified() {
    if (tool_context_.is_valid() && active_asset_.is_valid()) {
        tool_context_->mark_asset_dirty(active_asset_, true);
    }
    set_dirty_banner(true, "Blueprint modified — click Validate to refresh status.", "Validate");
}

void ProjectWorkspace::_on_spec_text_changed(const String &p_text, const String &p_prop_name) {
    if (selected_spec_.is_null()) {
        return;
    }
    if (p_prop_name.begins_with("root_node.")) {
        if (Ref<ProjectSceneSpec> sc = selected_spec_; sc.is_valid() && sc->get_root_node().is_valid()) {
            sc->get_root_node()->set(p_prop_name.substr(10), p_text);
        }
    } else {
        selected_spec_->set(p_prop_name, p_text);
    }
    if (ProjectBlueprintEditor *ed = _get_active_blueprint_editor()) {
        ed->refresh();
    }
    _on_editor_blueprint_modified();
}

void ProjectWorkspace::_on_spec_bool_toggled(bool p_pressed, const String &p_prop_name) {
    if (selected_spec_.is_null()) {
        return;
    }
    if (p_prop_name.begins_with("root_node.")) {
        if (Ref<ProjectSceneSpec> sc = selected_spec_; sc.is_valid() && sc->get_root_node().is_valid()) {
            sc->get_root_node()->set(p_prop_name.substr(10), p_pressed);
        }
    } else {
        selected_spec_->set(p_prop_name, p_pressed);
    }
    if (ProjectBlueprintEditor *ed = _get_active_blueprint_editor()) {
        ed->refresh();
    }
    _on_editor_blueprint_modified();
}

void ProjectWorkspace::_on_pick_class_pressed(LineEdit *p_target, const String &p_base_filter) {
    class_picker_target_ = p_target;
    if (class_picker_) {
        class_picker_->popup_for_base(p_base_filter);
    }
}

void ProjectWorkspace::_on_class_picked(const String &p_class_name) {
    if (class_picker_target_ && !p_class_name.is_empty()) {
        class_picker_target_->set_text(p_class_name);
        class_picker_target_->emit_signal("text_changed", p_class_name);
    }
    class_picker_target_ = nullptr;
}

// ============================================================================
// Static Launcher API
// ============================================================================

ProjectWorkspace *ProjectWorkspace::get_or_create_shared_workspace(Control *p_owner) {
    if (p_owner) {
        for (int i = 0; i < p_owner->get_child_count(); ++i) {
            if (auto *existing = Object::cast_to<ProjectWorkspace>(p_owner->get_child(i))) {
                return existing;
            }
        }
        ProjectWorkspace *embedded = memnew(ProjectWorkspace);
        p_owner->add_child(embedded);
        return embedded;
    }

    if (shared_instance_ && shared_window_) {
        shared_window_->show();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(ProjectWorkspace);
    shared_window_   = IdeamToolWorkspace::create_workspace_window(
        shared_instance_,
        "Ideam Project Workspace",
        Vector2i(1180, 740)
    );
    return shared_instance_;
}

void ProjectWorkspace::open_blueprint(const Ref<ProjectBlueprintResource> &p_blueprint, Control *p_owner) {
    if (p_blueprint.is_null()) {
        return;
    }
    if (ProjectWorkspace *ws = get_or_create_shared_workspace(p_owner)) {
        ws->open_asset(p_blueprint, "Blueprints");
    }
}

void ProjectWorkspace::open_manifest(const Ref<ProjectModuleManifest> &p_manifest, Control *p_owner) {
    if (p_manifest.is_null()) {
        return;
    }
    if (ProjectWorkspace *ws = get_or_create_shared_workspace(p_owner)) {
        const String tab = p_manifest->get_plugin_domain().is_empty() ? String("Modules") : p_manifest->get_plugin_domain();
        ws->open_asset(p_manifest, tab);
    }
}

void ProjectWorkspace::close_shared_window() {
    if (shared_window_) {
        shared_window_->hide();
        shared_window_->queue_free();
        shared_window_   = nullptr;
        shared_instance_ = nullptr;
    }
}

} // namespace ideam::godot_ext
