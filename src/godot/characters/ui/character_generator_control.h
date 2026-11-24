#pragma once

/**
 * character_generator_control.h
 * Tier 1 DomainTool (`src/godot/characters/ui/`): Character Generator Mode view for
 * `LoomisCharacter3DResource`.
 *
 * Features:
 *   - Dynamically built bounded parameter sliders (`[min..max]`) from the linked `BodyPlanResource`
 *     with `IdeamUndoRedo` commits.
 *   - 1-click Randomize Within Bounds + Seed SpinBox + Reset to Defaults.
 *   - Live LOD 0..3 toggle (emits `lod_changed(int)` for live preview sync),
 *     Topological Bridge Regime telemetry (`SMOOTH` / `CREASE` / `DEEP_FOLD`),
 *     and Loomis Center-of-Gravity balance readout.
 *   - Non-destructive `Details` toggle + Quick Layer 4 Asymmetric Detail Bank apply buttons + Clear Details.
 *   - `IdeamTutorials` selectors & `?` Help hook.
 */

#include "../character_gallery_bank_resource.h"
#include "../loomis_character_3d_resource.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

class CharacterGeneratorControl : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(CharacterGeneratorControl, godot::VBoxContainer)

private:
    godot::Ref<LoomisCharacter3DResource> instance_;
    godot::Ref<CharacterGalleryBankResource> gallery_bank_;
    godot::Ref<ToolContext> tool_context_;
    int active_lod_ = 0;
    bool rebuilding_sliders_ = false;
    bool syncing_details_toggle_ = false;

    godot::SpinBox *seed_spin_           = nullptr;
    godot::OptionButton *lod_opt_        = nullptr;
    godot::CheckButton *details_toggle_  = nullptr;
    godot::VBoxContainer *sliders_vbox_  = nullptr;
    godot::Label *bridge_telemetry_lbl_  = nullptr;
    godot::Label *balance_telemetry_lbl_ = nullptr;

    void _build_ui();
    void _rebuild_parameter_sliders();
    void _refresh_telemetry();

protected:
    static void _bind_methods();

public:
    CharacterGeneratorControl();
    ~CharacterGeneratorControl() override = default;

    void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;

    [[nodiscard]] godot::StringName get_domain_tool_id() const override {
        return godot::StringName("CharacterGeneratorControl");
    }

    [[nodiscard]] godot::PackedStringArray get_domain_semantic_tags() const override;

    void set_character_instance(const godot::Ref<LoomisCharacter3DResource> &p_instance);
    [[nodiscard]] godot::Ref<LoomisCharacter3DResource> get_character_instance() const { return instance_; }

    void set_gallery_bank(const godot::Ref<CharacterGalleryBankResource> &p_bank) { gallery_bank_ = p_bank; }
    [[nodiscard]] godot::Ref<CharacterGalleryBankResource> get_gallery_bank() const { return gallery_bank_; }

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    [[nodiscard]] int get_active_lod() const noexcept { return active_lod_; }

    void _on_slider_changed(double p_val, const godot::String &p_param_name);
    void _on_randomize_pressed();
    void _on_reset_defaults_pressed();
    void _on_lod_selected(int p_index);
    void _on_details_toggled(bool p_pressed);
    void _on_clear_details_pressed();
    void _on_apply_detail_preset_pressed(const godot::String &p_preset_id);
    void _on_help_pressed();
    void _on_instance_changed();
};

} // namespace ideam::godot_ext
