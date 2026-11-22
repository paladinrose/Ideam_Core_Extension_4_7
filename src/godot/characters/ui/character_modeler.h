#pragma once

/**
 * character_modeler.h
 * Dual-Mode Tier 2 Orchestration Workspace (`ToolWorkspace`) for `IdeamCharacters`
 * (`src/godot/characters/ui/`), mirroring `NarrativesComposer`:
 *
 * Modes:
 *   - `MODE_PLAN_AUTHORING`: Assemble `CharacterGalleryBankResource` galleries
 *     (`ScaffoldBank`, `TopologyBank`, `FeatureBank`, `DetailBank`), configure
 *     `ProportionRulerGraph` canons, and author `BodyPlanResource` schemas.
 *   - `MODE_CHARACTER_GENERATOR`: Bounded sliders from the active `BodyPlanResource`,
 *     1-click randomization, live bridge regime & balance telemetry, LOD 0..3 toggle,
 *     and Layer 4 asymmetric detail deltas on `CharacterInstanceResource`.
 *
 * Sidebar Tabs: `Body Plans`, `Characters`, `Gallery Banks`, `Blueprints`, `Apparel`.
 */

#include "../body_plan_resource.h"
#include "../character_gallery_bank_resource.h"
#include "../character_instance_resource.h"
#include "../../tools/tool_workspace.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
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
    godot::Ref<CharacterInstanceResource> active_character_;
    godot::Ref<CharacterGalleryBankResource> active_gallery_bank_;

    godot::Button *btn_mode_authoring_ = nullptr;
    godot::Button *btn_mode_generator_ = nullptr;
    godot::Button *btn_new_plan_       = nullptr;
    godot::Button *btn_new_character_  = nullptr;
    godot::Button *btn_new_bank_       = nullptr;

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

    void _on_mode_pressed(int p_mode);
    void _on_new_body_plan_pressed();
    void _on_new_character_pressed();
    void _on_new_gallery_bank_pressed();

    static CharacterModeler *get_or_create_shared_modeler(godot::Control *p_owner = nullptr);
    static void open_character_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::CharacterModeler::ModelerMode);
