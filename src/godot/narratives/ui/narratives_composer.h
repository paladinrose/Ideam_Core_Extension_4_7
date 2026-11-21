#pragma once

/**
 * narratives_composer.h
 * Context-aware `ToolWorkspace` window (`src/godot/narratives/ui/`) hosting all
 * "I don't fit in an Inspector" DomainTools for the `IdeamNarratives` domain:
 *   - `NarrativeControl` (dual-column Literal/Symbolic roster)
 *   - `LocationMap` (directed Location topology & BFS pathfinding)
 *   - `RelationshipMap` (multi-character relationship graph)
 *   - `PlotOutliner` (Master-Detail Quest/Rule-Engine outliner with `NarrativeConditionField`)
 *   - `NarrativeConditionField` (standalone condition sentence editor)
 */

#include "../../tools/tool_workspace.h"
#include "../narremes/narrative.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/window.hpp>

namespace ideam::godot_ext {

class NarrativesComposer : public ToolWorkspace {
    GDCLASS(NarrativesComposer, ToolWorkspace)

public:
    enum NarrativeViewMode : int32_t {
        VIEW_ROSTER           = 0,
        VIEW_LOCATION_MAP     = 1,
        VIEW_RELATIONSHIP_MAP = 2,
        VIEW_PLOT_OUTLINER    = 3
    };

private:
    static NarrativesComposer *shared_instance_;
    static godot::Window *shared_window_;

    NarrativeViewMode active_view_mode_ = VIEW_ROSTER;
    godot::Ref<Narrative> active_narrative_;

    godot::Button *btn_view_roster_  = nullptr;
    godot::Button *btn_view_locs_    = nullptr;
    godot::Button *btn_view_rels_    = nullptr;
    godot::Button *btn_view_plots_   = nullptr;

    void _rebuild_active_narrative_view();

protected:
    static void _bind_methods();

    virtual void _populate_command_bar(godot::HBoxContainer *p_custom_actions) override;
    virtual godot::Control *_create_domain_control_for_asset(const godot::Ref<godot::Resource> &p_asset) override;
    virtual void _on_active_asset_switched(const godot::Ref<godot::Resource> &p_asset) override;

public:
    static constexpr const char *WORKSPACE_ID = "NarrativesComposer";

    NarrativesComposer();
    ~NarrativesComposer() override;

    void set_narrative_view_mode(NarrativeViewMode p_mode);
    [[nodiscard]] NarrativeViewMode get_narrative_view_mode() const noexcept { return active_view_mode_; }

    void _on_view_mode_pressed(int p_mode);
    void _on_subtool_narreme_selected(const godot::Ref<Narreme> &p_narreme);
    void _on_subtool_open_narreme_requested(const godot::Ref<Narreme> &p_narreme);

    static NarrativesComposer *get_or_create_shared_composer(godot::Control *p_owner = nullptr);
    static void open_narrative_resource(
        const godot::Ref<godot::Resource> &p_resource,
        godot::Control *p_owner = nullptr
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::NarrativesComposer::NarrativeViewMode);
