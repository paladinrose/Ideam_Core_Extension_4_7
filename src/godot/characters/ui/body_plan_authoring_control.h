#pragma once

/**
 * body_plan_authoring_control.h
 * Tier 1 DomainTool (`src/godot/characters/ui/`): Plan Authoring Mode workspace view
 * for `BodyPlanResource` paired with tabbed `CharacterGalleryBankResource` galleries
 * (`ScaffoldBank`, `TopologyBank`, `FeatureBank`, `DetailBank`).
 */

#include "../body_plan_resource.h"
#include "../character_gallery_bank_resource.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/tab_bar.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

class BodyPlanAuthoringControl : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(BodyPlanAuthoringControl, godot::VBoxContainer)

private:
    godot::Ref<BodyPlanResource> body_plan_;
    godot::Ref<CharacterGalleryBankResource> gallery_bank_;
    godot::Ref<ToolContext> tool_context_;
    CharacterBankCategory active_bank_ = BANK_SCAFFOLD;

    godot::TabBar *bank_tabs_       = nullptr;
    godot::LineEdit *search_edit_   = nullptr;
    godot::ItemList *preset_list_   = nullptr;
    godot::Label *preset_desc_lbl_  = nullptr;
    godot::Button *apply_btn_       = nullptr;
    godot::Label *plan_summary_lbl_ = nullptr;

    godot::PackedStringArray displayed_preset_ids_;

    void _build_ui();
    void _refresh_bank_list();
    void _refresh_plan_summary();

protected:
    static void _bind_methods();

public:
    BodyPlanAuthoringControl();
    ~BodyPlanAuthoringControl() override = default;

    void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;

    [[nodiscard]] godot::StringName get_domain_tool_id() const override {
        return godot::StringName("BodyPlanAuthoringControl");
    }

    [[nodiscard]] godot::PackedStringArray get_domain_semantic_tags() const override;

    void set_body_plan(const godot::Ref<BodyPlanResource> &p_plan);
    [[nodiscard]] godot::Ref<BodyPlanResource> get_body_plan() const { return body_plan_; }

    void set_gallery_bank(const godot::Ref<CharacterGalleryBankResource> &p_bank);
    [[nodiscard]] godot::Ref<CharacterGalleryBankResource> get_gallery_bank() const { return gallery_bank_; }

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void _on_bank_tab_changed(int p_tab);
    void _on_search_changed(const godot::String &p_text);
    void _on_preset_selected(int p_index);
    void _on_apply_preset_pressed();
    void _on_starter_humanoid_pressed();
    void _on_starter_quadruped_pressed();
    void _on_starter_avian_pressed();
    void _on_help_pressed();
    void _on_plan_changed();
};

} // namespace ideam::godot_ext
