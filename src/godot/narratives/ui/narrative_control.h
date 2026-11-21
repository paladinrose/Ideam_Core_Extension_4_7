#pragma once

/**
 * narrative_control.h
 * Tier 1 DomainTool (`src/godot/narratives/ui/`): Dual-column Literal vs. Symbolic roster
 * manager for a `Narrative` resource.
 *
 * Features:
 *   - Family tabs: Characters/Roles, Locations/Places, Props/Symbols, Incidents/Moments, Plots/Stories
 *   - Two-column layout: Real (Literal) Narremes on the left, Symbolic Forms + current assignments on the right
 *   - Utility actions routed through `Narrative`'s `IdeamUndoRedo`-backed methods:
 *     Add Literal/Symbolic, Delete, Convert Real <-> Symbolic, Instantiate Real from Symbolic,
 *     Assign Selected Literal to Symbolic Form, Auto-Fit Best Literal, and Vacate Symbolic Form
 */

#include "../narreme.h"
#include "../narremes/narrative.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/tab_bar.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

class NarrativeControl : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(NarrativeControl, godot::VBoxContainer)

public:
    enum NarremeFamily : int32_t {
        FAMILY_CHARACTERS = 0,
        FAMILY_LOCATIONS  = 1,
        FAMILY_PROPS      = 2,
        FAMILY_INCIDENTS  = 3,
        FAMILY_PLOTS      = 4
    };

private:
    godot::Ref<Narrative> narrative_;
    godot::Ref<ToolContext> tool_context_;
    NarremeFamily active_family_ = FAMILY_CHARACTERS;

    godot::TabBar *family_tabs_           = nullptr;
    godot::LineEdit *new_name_edit_       = nullptr;
    godot::Label *literal_header_lbl_     = nullptr;
    godot::Label *symbolic_header_lbl_    = nullptr;
    godot::ItemList *literal_list_        = nullptr;
    godot::ItemList *symbolic_list_       = nullptr;
    godot::OptionButton *fit_mode_opt_    = nullptr;
    godot::Label *status_lbl_             = nullptr;

    void _build_ui();
    void _update_column_headers();
    [[nodiscard]] int _get_selected_literal_index() const;
    [[nodiscard]] int _get_selected_symbolic_index() const;

protected:
    static void _bind_methods();

public:
    NarrativeControl();
    ~NarrativeControl() override = default;

    // --- DomainToolInterface ---
    virtual void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;
    [[nodiscard]] virtual godot::StringName get_domain_tool_id() const override {
        return godot::StringName("NarrativeControl");
    }
    [[nodiscard]] virtual godot::PackedStringArray get_domain_semantic_tags() const override;

    void set_narrative(const godot::Ref<Narrative> &p_narrative);
    [[nodiscard]] godot::Ref<Narrative> get_narrative() const { return narrative_; }

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void set_active_family(NarremeFamily p_family);
    [[nodiscard]] NarremeFamily get_active_family() const noexcept { return active_family_; }

    void refresh_lists();

    // --- Signal Handlers ---
    void _on_family_tab_changed(int p_tab);
    void _on_add_literal_pressed();
    void _on_add_symbolic_pressed();
    void _on_remove_literal_pressed();
    void _on_remove_symbolic_pressed();
    void _on_convert_to_symbolic_pressed();
    void _on_convert_to_literal_pressed();
    void _on_instantiate_from_symbolic_pressed();
    void _on_assign_selected_pressed();
    void _on_auto_fit_pressed();
    void _on_vacate_symbolic_pressed();
    void _on_literal_selected(int p_index);
    void _on_symbolic_selected(int p_index);
    void _on_literal_activated(int p_index);
    void _on_symbolic_activated(int p_index);
    void _on_help_pressed();
    void _on_narrative_changed();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::NarrativeControl::NarremeFamily);
