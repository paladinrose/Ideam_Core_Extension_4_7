#pragma once

/**
 * character_modeler.h
 * Dual-Mode Tier 2 Orchestration Workspace (`ToolWorkspace`) for `IdeamCharacters`
 * (`src/godot/characters/ui/`), working identically in Editor (Context 2) and Runtime
 * Avatar Creator / In-Game Modeler (Context 4):
 *
 *   - Top Ribbon (`ribbon_slot_`): 4-tool `IdeamSpatialToolbar` (`Scaffolding`, `Topology`,
 *     `Features`, persistent `Details` toggle).
 *   - Embedded 3D Studio Viewport (`IdeamViewport3DHost`) hosting a live `LoomisCharacterNode3D`
 *     with all 3 trans-environment `IdeamGizmo`s (`CharacterScaffoldGizmo`,
 *     `CharacterTopologyGizmo`, `CharacterFeatureGizmo`) and studio orbit camera.
 *   - Modes:
 *     - `MODE_PLAN_AUTHORING`: Assemble `CharacterGalleryBankResource` galleries and author `BodyPlanResource`.
 *     - `MODE_CHARACTER_GENERATOR`: Bounded sliders, seeded randomization, live LOD 0..3 preview sync,
 *       and non-destructive Layer 4 details on `LoomisCharacter3DResource`.
 */

#include "../body_plan_resource.h"
#include "../character_feature_gizmo.h"
#include "../character_gallery_bank_resource.h"
#include "../loomis_character_3d_resource.h"
#include "../character_scaffold_gizmo.h"
#include "../character_topology_gizmo.h"
#include "../loomis_character_node_3d.h"
#include "../../gizmos/ideam_spatial_toolbar.h"
#include "../../gizmos/ideam_viewport_3d_host.h"
#include "../../tools/tool_workspace.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

class CharacterModeler : public ToolWorkspace {
    GDCLASS(CharacterModeler, ToolWorkspace)

public:
    enum ModelerMode : int32_t {
        MODE_PLAN_AUTHORING      = 0,
        MODE_CHARACTER_GENERATOR = 1
    };

private:
    static CharacterModeler *shared_instance_;
    static godot::Window *shared_window_;

    ModelerMode active_mode_ = MODE_PLAN_AUTHORING;
    godot::Ref<BodyPlanResource> active_body_plan_;
    godot::Ref<LoomisCharacter3DResource> active_character_;
    godot::Ref<CharacterGalleryBankResource> active_gallery_bank_;

    godot::Button *btn_mode_authoring_ = nullptr;
    godot::Button *btn_mode_generator_ = nullptr;
    godot::Button *btn_new_plan_       = nullptr;
    godot::Button *btn_new_character_  = nullptr;
    godot::Button *btn_new_bank_       = nullptr;

    // Trans-environment 3D studio stage & spatial toolbar
    IdeamSpatialToolbar *spatial_toolbar_   = nullptr;
    IdeamViewport3DHost *viewport_3d_host_  = nullptr;
    LoomisCharacterNode3D *preview_node_   = nullptr;
    godot::Ref<CharacterScaffoldGizmo> gizmo_scaffold_;
    godot::Ref<CharacterTopologyGizmo> gizmo_topology_;
    godot::Ref<CharacterFeatureGizmo> gizmo_feature_;

    void _setup_spatial_studio();
    void _sync_preview_character();
    void _rebuild_active_view();

protected:
    static void _bind_methods();

    void _populate_command_bar(godot::HBoxContainer *p_custom_actions) override;
    godot::Control *_create_domain_control_for_asset(const godot::Ref<godot::Resource> &p_asset) override;
    void _on_active_asset_switched(const godot::Ref<godot::Resource> &p_asset) override;

public:
    static constexpr const char *WORKSPACE_ID = "CharacterModeler";

    CharacterModeler();
    ~CharacterModeler() override;

    void set_modeler_mode(ModelerMode p_mode);
    [[nodiscard]] ModelerMode get_modeler_mode() const noexcept { return active_mode_; }

    void set_active_gallery_bank(const godot::Ref<CharacterGalleryBankResource> &p_bank);
    [[nodiscard]] godot::Ref<CharacterGalleryBankResource> get_active_gallery_bank() const { return active_gallery_bank_; }

    [[nodiscard]] IdeamViewport3DHost *get_viewport_3d_host() const noexcept { return viewport_3d_host_; }
    [[nodiscard]] LoomisCharacterNode3D *get_preview_node() const noexcept { return preview_node_; }

    void _on_mode_pressed(int p_mode);
    void _on_new_body_plan_pressed();
    void _on_new_character_pressed();
    void _on_new_gallery_bank_pressed();
    void _on_spatial_mode_changed(const godot::StringName &p_mode_id);
    void _on_spatial_toggle_changed(const godot::StringName &p_toggle_id, bool p_pressed);
    void _on_preview_lod_changed(int p_lod_level);

    static CharacterModeler *get_or_create_shared_modeler(godot::Control *p_owner = nullptr);
    static void open_character_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::CharacterModeler::ModelerMode);
