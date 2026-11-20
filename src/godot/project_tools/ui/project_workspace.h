#pragma once

/**
 * project_workspace.h
 * Tier 2 Orchestration Workspace (`IdeamToolWorkspace`) for Ideam Project Tools (Phases 2 & 3).
 *
 * Unifies:
 *   - New Project Wizard (scaffolding folders, templated .gd scripts, starter Resources, Scenes,
 *     and ProjectSettings patches across all active Ideam plugins).
 *   - Blueprint / Manifest Authoring (creating and saving typed `ProjectBlueprintResource` &
 *     `ProjectModuleManifest` `.tres` files).
 *   - Phase 3 Cross-Tier-2 Handoff Strip: launches newly generated assets directly inside their
 *     owning sibling Tier 2 workspaces (`GeometryComposer`, `PlanetarySystemsModeler`,
 *     `DesignComposer`, `CharacterModeler`, `ProceduralComposer`) sharing `IdeamToolContext`.
 */

#include "class_picker_dialog.h"
#include "project_blueprint_editor.h"
#include "../project_blueprint_resource.h"
#include "../project_scaffold_engine.h"
#include "../../controls/ideam_tool_workspace.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace ideam::godot_ext {

class ProjectWorkspace : public IdeamToolWorkspace {
    GDCLASS(ProjectWorkspace, IdeamToolWorkspace)

private:
    static ProjectWorkspace *shared_instance_;
    static godot::Window *shared_window_;

    godot::Ref<ProjectScaffoldEngine> engine_;
    godot::Ref<godot::Resource> selected_spec_;

    // Custom Command Bar Controls
    godot::Button *discover_btn_          = nullptr;
    godot::OptionButton *mode_btn_        = nullptr;
    godot::CheckBox *overwrite_check_     = nullptr;
    godot::Button *validate_btn_          = nullptr;
    godot::Button *generate_btn_          = nullptr;

    // Phase 3: Post-Generation Cross-Tier-2 Handoff Strip
    godot::PanelContainer *handoff_panel_ = nullptr;
    godot::HBoxContainer *handoff_bar_    = nullptr;
    godot::Dictionary handoff_assets_by_workspace_; // workspace_id (String) -> PackedStringArray of res:// paths

    // Right-hand Spec Inspector (sits inside inherited inspector_vbox_)
    godot::VBoxContainer *spec_fields_box_ = nullptr;
    ClassPickerDialog *class_picker_       = nullptr;
    godot::LineEdit *class_picker_target_  = nullptr;

    [[nodiscard]] ProjectBlueprintEditor *_get_active_blueprint_editor() const;
    [[nodiscard]] godot::Ref<ProjectBlueprintResource> _get_active_blueprint() const;
    void _rebuild_spec_inspector(const godot::Ref<godot::Resource> &p_spec);
    void _rebuild_handoff_strip(const godot::Dictionary &p_owner_workspaces);
    void _clear_handoff_strip();
    void _add_text_field(
        const godot::String &p_label,
        const godot::String &p_value,
        const godot::String &p_prop_name,
        bool p_with_class_picker = false,
        const godot::String &p_base_filter = ""
    );
    void _add_bool_field(
        const godot::String &p_label,
        bool p_value,
        const godot::String &p_prop_name
    );

protected:
    static void _bind_methods();

    void _populate_command_bar(godot::HBoxContainer *p_custom_actions) override;
    void _on_active_asset_switched(const godot::Ref<godot::Resource> &p_asset) override;
    void _on_dirty_banner_action_pressed() override;

public:
    static constexpr const char *WORKSPACE_ID = "ProjectWorkspace";

    ProjectWorkspace();
    ~ProjectWorkspace() override;

    // --- Ecosystem Discovery & Execution ---
    void discover_and_open_active_blueprint();
    godot::Dictionary run_validation();
    godot::Dictionary run_generation();
    bool launch_handoff_workspace(const godot::String &p_workspace_id);

    // --- Signal Handlers ---
    void _on_discover_pressed();
    void _on_mode_selected(int p_index);
    void _on_validate_pressed();
    void _on_generate_pressed();
    void _on_handoff_workspace_pressed(const godot::String &p_workspace_id);
    void _on_editor_spec_selected(godot::Object *p_spec);
    void _on_editor_blueprint_modified();
    void _on_spec_text_changed(const godot::String &p_text, const godot::String &p_prop_name);
    void _on_spec_bool_toggled(bool p_pressed, const godot::String &p_prop_name);
    void _on_pick_class_pressed(godot::LineEdit *p_target, const godot::String &p_base_filter);
    void _on_class_picked(const godot::String &p_class_name);

    // --- Unified Static Launcher API ---
    static ProjectWorkspace *get_or_create_shared_workspace(godot::Control *p_owner = nullptr);
    static void open_blueprint(
        const godot::Ref<ProjectBlueprintResource> &p_blueprint,
        godot::Control *p_owner = nullptr
    );
    static void open_manifest(
        const godot::Ref<ProjectModuleManifest> &p_manifest,
        godot::Control *p_owner = nullptr
    );
    static void close_shared_window();
};

} // namespace ideam::godot_ext
