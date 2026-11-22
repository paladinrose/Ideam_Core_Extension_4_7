#pragma once

/**
 * form_scaffold_control.h
 * Tier 1 DomainTool (`src/godot/design/ui/`): Interactive Andrew Loomis Proportion Ruler,
 * Archetype Preset Strip, Contrapposto Slider, and 2D Orthographic Silhouette/Plumb-Line
 * Canvas for `FormScaffoldResource` and `ProportionRulerGraphResource`.
 *
 * Follows the `IdeamNarratives` & `IdeamSurfaces` Tools/Tutorials blueprint:
 *   - Implements `DomainToolInterface`
 *   - Stamps stable `ideam_ui_selector` metadata keys for `IdeamTutorials`
 *   - Built-in `?` Help hook via `domain_tool::request_control_help(...)`
 */

#include "../form_scaffold_resource.h"
#include "../proportion_ruler_resource.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

class FormScaffoldControl : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(FormScaffoldControl, godot::VBoxContainer)

private:
    godot::Ref<FormScaffoldResource> scaffold_;
    godot::Ref<ProportionRulerGraphResource> ruler_;
    godot::Ref<ToolContext> tool_context_;
    bool updating_ui_ = false;

    godot::OptionButton *archetype_opt_ = nullptr;
    godot::SpinBox *unit_spin_          = nullptr;
    godot::HSlider *contrapposto_sld_   = nullptr;
    godot::Control *ortho_canvas_       = nullptr;
    godot::Label *telemetry_lbl_        = nullptr;

    void _build_ui();
    void _sync_ui();

protected:
    static void _bind_methods();

public:
    FormScaffoldControl();
    ~FormScaffoldControl() override = default;

    // --- DomainToolInterface ---
    void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;

    [[nodiscard]] godot::StringName get_domain_tool_id() const override {
        return godot::StringName("FormScaffoldControl");
    }

    [[nodiscard]] godot::PackedStringArray get_domain_semantic_tags() const override;

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void set_scaffold(const godot::Ref<FormScaffoldResource> &p_scaffold);
    [[nodiscard]] godot::Ref<FormScaffoldResource> get_scaffold() const { return scaffold_; }

    void set_ruler_graph(const godot::Ref<ProportionRulerGraphResource> &p_ruler);
    [[nodiscard]] godot::Ref<ProportionRulerGraphResource> get_ruler_graph() const { return ruler_; }

    void _on_archetype_selected(int p_index);
    void _on_unit_changed(double p_val);
    void _on_contrapposto_changed(double p_val);
    void _on_help_pressed();
    void _on_ortho_canvas_draw();
    void _on_asset_changed();
};

} // namespace ideam::godot_ext
