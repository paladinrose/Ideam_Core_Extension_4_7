#pragma once

/**
 * project_blueprint_editor.h
 * Reusable Tier 1 Control (`godot::VBoxContainer`) for viewing and editing a
 * `ProjectBlueprintResource` or single `ProjectModuleManifest` (Phase 2).
 *
 * Registered in `ToolRegistry` for `ProjectWorkspace::WORKSPACE_ID` so it can be
 * hosted inside `ProjectWorkspace` OR embedded inside future Tier 3–5 meta-tools.
 *
 * Capabilities:
 *   - Interactive hierarchical Tree of Modules -> Folders / Scripts / Resources / Scenes / Patches
 *   - Checkbox toggling in Wizard mode & Add/Remove authoring controls in Author mode
 *   - Live dry-run validation status badges per item
 *   - Embedded Tier 1 Preview slot: when a `ProjectResourceSpec` is selected whose `resource_class`
 *     has a registered Tier 1 control in `ToolRegistry`, allows live visual previewing!
 */

#include "../project_blueprint_resource.h"
#include "../project_scaffold_engine.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/v_split_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <unordered_map>

namespace ideam::godot_ext {

class ProjectBlueprintEditor : public godot::VBoxContainer {
    GDCLASS(ProjectBlueprintEditor, godot::VBoxContainer)

public:
    enum EditorMode : int32_t {
        MODE_WIZARD = 0,
        MODE_AUTHOR = 1
    };

private:
    godot::Ref<ProjectBlueprintResource> blueprint_;
    godot::Ref<ProjectModuleManifest> single_manifest_;
    godot::Ref<ToolContext> tool_context_;
    godot::Ref<ProjectScaffoldEngine> engine_;
    EditorMode mode_ = MODE_WIZARD;

    // Header / Root Path Bar
    godot::HBoxContainer *header_bar_ = nullptr;
    godot::LineEdit *root_path_edit_  = nullptr;
    godot::HBoxContainer *author_bar_ = nullptr;
    godot::Button *add_folder_btn_    = nullptr;
    godot::Button *add_script_btn_    = nullptr;
    godot::Button *add_resource_btn_  = nullptr;
    godot::Button *add_scene_btn_     = nullptr;

    // Main Split: Scaffold Tree (top) + Optional Embedded Tier 1 Preview (bottom)
    godot::VSplitContainer *main_split_    = nullptr;
    godot::Tree *scaffold_tree_            = nullptr;
    godot::PanelContainer *preview_panel_  = nullptr;
    godot::VBoxContainer *preview_vbox_    = nullptr;
    godot::Label *preview_header_label_    = nullptr;
    godot::Control *active_tier1_preview_  = nullptr;
    godot::Ref<godot::Resource> preview_resource_;

    // Diagnostics status map (target_path -> severity/message)
    godot::Dictionary last_validation_map_;

    void _rebuild_tree();
    void _populate_module_branch(godot::TreeItem *p_parent, const godot::Ref<ProjectModuleManifest> &p_mod);
    void _clear_tier1_preview();
    void _try_mount_tier1_preview(const godot::Ref<ProjectResourceSpec> &p_spec);
    [[nodiscard]] godot::Ref<ProjectModuleManifest> _get_target_module_for_authoring() const;

protected:
    static void _bind_methods();

public:
    ProjectBlueprintEditor();
    ~ProjectBlueprintEditor() override = default;

    void set_tool_context(const godot::Ref<ToolContext> &p_context) { tool_context_ = p_context; }
    void set_blueprint(const godot::Ref<ProjectBlueprintResource> &p_blueprint);
    [[nodiscard]] godot::Ref<ProjectBlueprintResource> get_blueprint() const { return blueprint_; }

    void set_manifest(const godot::Ref<ProjectModuleManifest> &p_manifest);
    [[nodiscard]] godot::Ref<ProjectModuleManifest> get_manifest() const { return single_manifest_; }

    void set_editor_mode(EditorMode p_mode);
    [[nodiscard]] EditorMode get_editor_mode() const noexcept { return mode_; }

    void apply_validation_result(const godot::Dictionary &p_validation_result);
    void refresh();

    // Signal handlers
    void _on_root_path_changed(const godot::String &p_new_path);
    void _on_tree_item_selected();
    void _on_tree_item_edited();
    void _on_add_folder_pressed();
    void _on_add_script_pressed();
    void _on_add_resource_pressed();
    void _on_add_scene_pressed();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::ProjectBlueprintEditor::EditorMode);
