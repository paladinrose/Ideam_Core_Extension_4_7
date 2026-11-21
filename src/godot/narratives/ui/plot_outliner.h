#pragma once

/**
 * plot_outliner.h
 * Tier 1 DomainTool (`src/godot/narratives/ui/`): Master-Detail Quest / Rule-Engine
 * Outliner (Trigger-Condition-Outcome / StateTree pattern) for authoring and simulating
 * `Plot` -> `Plot_Event` -> `Incident` -> `Incident_Condition` hierarchies.
 *
 * Layout:
 *   - Top Bar: Plot picker (from Narrative), live PlotStatus badge, Evaluate Plot,
 *     + Add Plot Event, + New Incident, and `?` Help button.
 *   - Left Pane (Plot Event Lanes): Lists Plot_Events with outcome lane tags
 *     ([REQ], [WIN!], [FAIL!], [BLOCK], [OPEN]) plus an inline Plot_Event editor
 *     routed through `Plot_Event::commit_edit` (`IdeamUndoRedo`).
 *   - Right Pane (Incident Trigger & Condition Clauses): Displays the linked Incident,
 *     optional FrameNet frame badge, live Evaluate Incident button, and a scrollable
 *     clause builder embedding live `NarrativeConditionField`s for `Narrative_Condition`,
 *     status selectors for `Gameplay_Condition`, and prerequisite cards for `Causal_Condition`.
 */

#include "narrative_condition_field.h"
#include "../helpers/causal_condition.h"
#include "../helpers/gameplay_condition.h"
#include "../helpers/incident_condition.h"
#include "../helpers/narrative_condition.h"
#include "../helpers/plot_event.h"
#include "../narremes/incident.h"
#include "../narremes/narrative.h"
#include "../narremes/plot.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include <vector>

namespace ideam::godot_ext {

class PlotOutliner : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(PlotOutliner, godot::VBoxContainer)

private:
    godot::Ref<Narrative> narrative_;
    godot::Ref<Plot> active_plot_;
    godot::Ref<Incident> standalone_incident_;
    godot::Ref<ToolContext> tool_context_;

    std::vector<godot::Ref<Plot>> plot_choices_;
    std::vector<godot::Ref<Incident>> incident_choices_;
    int selected_event_idx_ = -1;

    godot::OptionButton *plot_picker_opt_    = nullptr;
    godot::Label *plot_status_lbl_           = nullptr;
    godot::ItemList *event_list_             = nullptr;
    godot::LineEdit *event_title_edit_       = nullptr;
    godot::OptionButton *event_incident_opt_ = nullptr;
    godot::OptionButton *event_req_opt_      = nullptr;

    godot::Label *incident_header_lbl_       = nullptr;
    godot::Label *incident_frame_lbl_        = nullptr;
    godot::ScrollContainer *cond_scroll_     = nullptr;
    godot::VBoxContainer *cond_list_box_     = nullptr;

    void _build_ui();
    void _refresh_plot_picker();
    void _refresh_event_list();
    void _refresh_event_editor();
    void _refresh_incident_pane();
    [[nodiscard]] godot::Ref<Plot_Event> _get_selected_plot_event() const;
    [[nodiscard]] godot::Ref<Incident> _get_active_incident() const;

    static godot::String _plot_status_to_string(PlotStatus p_status);
    static godot::String _incident_status_to_string(IncidentStatus p_status);
    static godot::String _requirement_lane_tag(IncidentRequirement p_req);

protected:
    static void _bind_methods();

public:
    PlotOutliner();
    ~PlotOutliner() override = default;

    // --- DomainToolInterface ---
    virtual void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;
    [[nodiscard]] virtual godot::StringName get_domain_tool_id() const override {
        return godot::StringName("PlotOutliner");
    }
    [[nodiscard]] virtual godot::PackedStringArray get_domain_semantic_tags() const override;

    void set_narrative(const godot::Ref<Narrative> &p_narrative);
    [[nodiscard]] godot::Ref<Narrative> get_narrative() const { return narrative_; }

    void set_plot(const godot::Ref<Plot> &p_plot);
    [[nodiscard]] godot::Ref<Plot> get_plot() const { return active_plot_; }

    void set_incident(const godot::Ref<Incident> &p_incident);
    [[nodiscard]] godot::Ref<Incident> get_incident() const { return _get_active_incident(); }

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void refresh_all();

    // --- Signal Handlers ---
    void _on_plot_picker_selected(int p_index);
    void _on_evaluate_plot_pressed();
    void _on_add_plot_event_pressed();
    void _on_remove_plot_event_pressed();
    void _on_new_incident_pressed();
    void _on_event_selected(int p_index);
    void _on_apply_event_pressed();
    void _on_evaluate_incident_pressed();
    void _on_add_narrative_cond_pressed();
    void _on_add_gameplay_cond_pressed();
    void _on_add_causal_cond_pressed();
    void _on_remove_condition_pressed(int p_cond_index);
    void _on_gameplay_status_selected(int p_option_idx, int p_cond_index);
    void _on_add_causal_prereq_pressed(int p_cond_index);
    void _on_help_pressed();
    void _on_narrative_changed();
};

} // namespace ideam::godot_ext
