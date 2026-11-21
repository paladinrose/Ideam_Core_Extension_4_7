#include "narratives_composer.h"

#include "location_map.h"
#include "narrative_condition_field.h"
#include "narrative_control.h"
#include "plot_outliner.h"
#include "relationship_map.h"
#include "../ideam_narratives_plugin.h"
#include "../../tools/domain/domain_tool_interface.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

#ifdef TOOLS_ENABLED
#include <godot_cpp/classes/editor_interface.hpp>
#endif

using namespace godot;

namespace ideam::godot_ext {

NarrativesComposer *NarrativesComposer::shared_instance_ = nullptr;
Window *NarrativesComposer::shared_window_               = nullptr;

void NarrativesComposer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_narrative_view_mode", "mode"), &NarrativesComposer::set_narrative_view_mode);
    ClassDB::bind_method(D_METHOD("get_narrative_view_mode"), &NarrativesComposer::get_narrative_view_mode);
    ClassDB::bind_method(D_METHOD("_on_view_mode_pressed", "mode"), &NarrativesComposer::_on_view_mode_pressed);
    ClassDB::bind_method(D_METHOD("_on_subtool_narreme_selected", "narreme"), &NarrativesComposer::_on_subtool_narreme_selected);
    ClassDB::bind_method(D_METHOD("_on_subtool_open_narreme_requested", "narreme"), &NarrativesComposer::_on_subtool_open_narreme_requested);

    BIND_ENUM_CONSTANT(VIEW_ROSTER);
    BIND_ENUM_CONSTANT(VIEW_LOCATION_MAP);
    BIND_ENUM_CONSTANT(VIEW_RELATIONSHIP_MAP);
    BIND_ENUM_CONSTANT(VIEW_PLOT_OUTLINER);
}

NarrativesComposer::NarrativesComposer() {
    set_workspace_id(StringName(WORKSPACE_ID));
    set_help_topic_id(StringName("IdeamNarratives"));

    ensure_sidebar_tab("Narratives");
    ensure_sidebar_tab("Locations");
    ensure_sidebar_tab("Characters");
    ensure_sidebar_tab("Plots");
    ensure_sidebar_tab("Incidents");
    ensure_sidebar_tab("Conditions");

    if (tool_context_.is_valid()) {
        tool_context_->set_undo_redo(IdeamNarrativesPlugin::create_undo_redo_wrapper());
    }

    _populate_command_bar(custom_actions_box_);
}

NarrativesComposer::~NarrativesComposer() {
    if (shared_instance_ == this) {
        shared_instance_ = nullptr;
        shared_window_   = nullptr;
    }
}

void NarrativesComposer::_populate_command_bar(HBoxContainer *p_custom_actions) {
    if (!p_custom_actions || btn_view_roster_ != nullptr) return;

    auto add_mode_btn = [&](const char *p_label, const char *p_sel, NarrativeViewMode p_mode) -> Button * {
        Button *b = memnew(Button);
        b->set_text(p_label);
        domain_tool::stamp_ui_selector(b, p_sel);
        b->connect("pressed", Callable(this, "_on_view_mode_pressed").bind(static_cast<int>(p_mode)));
        p_custom_actions->add_child(b);
        return b;
    };

    btn_view_roster_ = add_mode_btn("Roster",           "narratives.composer.mode_roster",        VIEW_ROSTER);
    btn_view_locs_   = add_mode_btn("Location Map",     "narratives.composer.mode_location_map",  VIEW_LOCATION_MAP);
    btn_view_rels_   = add_mode_btn("Relationship Map", "narratives.composer.mode_rel_map",       VIEW_RELATIONSHIP_MAP);
    btn_view_plots_  = add_mode_btn("Plot Outliner",    "narratives.composer.mode_plot_outliner", VIEW_PLOT_OUTLINER);
}

Control *NarrativesComposer::_create_domain_control_for_asset(const Ref<Resource> &p_asset) {
    if (!p_asset.is_valid()) return nullptr;

    if (Ref<Narrative> nar = p_asset; nar.is_valid()) {
        active_narrative_ = nar;
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
            nar->propagate_undo_redo(tool_context_->get_undo_redo());
        }

        switch (active_view_mode_) {
            case VIEW_LOCATION_MAP: {
                auto *lm = memnew(LocationMap);
                lm->bind_domain_asset(nar, tool_context_);
                lm->connect("location_selected", Callable(this, "_on_subtool_narreme_selected"));
                return lm;
            }
            case VIEW_RELATIONSHIP_MAP: {
                auto *rm = memnew(RelationshipMap);
                rm->bind_domain_asset(nar, tool_context_);
                rm->connect("narreme_selected", Callable(this, "_on_subtool_narreme_selected"));
                return rm;
            }
            case VIEW_PLOT_OUTLINER: {
                auto *po = memnew(PlotOutliner);
                po->bind_domain_asset(nar, tool_context_);
                po->connect("incident_selected", Callable(this, "_on_subtool_narreme_selected"));
                return po;
            }
            case VIEW_ROSTER:
            default: {
                auto *nc = memnew(NarrativeControl);
                nc->bind_domain_asset(nar, tool_context_);
                nc->connect("narreme_selected", Callable(this, "_on_subtool_narreme_selected"));
                nc->connect("open_narreme_requested", Callable(this, "_on_subtool_open_narreme_requested"));
                return nc;
            }
        }
    }

    // Fall back to ToolRegistry descriptors for individual Location / Character / Plot / Incident / Narrative_Condition assets
    return ToolWorkspace::_create_domain_control_for_asset(p_asset);
}

void NarrativesComposer::_on_active_asset_switched(const Ref<Resource> &p_asset) {
    if (Ref<Narrative> nar = p_asset; nar.is_valid()) {
        active_narrative_ = nar;
    }
}

void NarrativesComposer::set_narrative_view_mode(NarrativeViewMode p_mode) {
    if (active_view_mode_ == p_mode) return;
    active_view_mode_ = p_mode;
    _rebuild_active_narrative_view();
}

void NarrativesComposer::_rebuild_active_narrative_view() {
    if (!active_narrative_.is_valid()) return;
    for (auto &entry : open_assets_) {
        if (entry.asset == active_narrative_) {
            if (entry.domain_control) {
                if (entry.domain_control->get_parent()) {
                    entry.domain_control->get_parent()->remove_child(entry.domain_control);
                }
                entry.domain_control->queue_free();
                entry.domain_control = nullptr;
            }
            _show_asset_in_center(entry);
            break;
        }
    }
}

void NarrativesComposer::_on_view_mode_pressed(int p_mode) {
    set_narrative_view_mode(static_cast<NarrativeViewMode>(p_mode));
}

void NarrativesComposer::_on_subtool_narreme_selected(const Ref<Narreme> &p_narreme) {
    if (p_narreme.is_valid()) {
        _refresh_inspector_for_asset(p_narreme);
    }
}

void NarrativesComposer::_on_subtool_open_narreme_requested(const Ref<Narreme> &p_narreme) {
    if (p_narreme.is_valid()) {
        open_asset(p_narreme);
    }
}

NarrativesComposer *NarrativesComposer::get_or_create_shared_composer(Control *p_owner) {
    if (shared_instance_ && shared_window_) {
        shared_window_->popup_centered();
        shared_window_->grab_focus();
        return shared_instance_;
    }

    shared_instance_ = memnew(NarrativesComposer);
    shared_window_   = create_workspace_window(shared_instance_, "Ideam Narratives Composer", Vector2i(1180, 740));

    Node *host_parent = p_owner;
#ifdef TOOLS_ENABLED
    if (!host_parent && Engine::get_singleton()->is_editor_hint()) {
        if (auto *ei = EditorInterface::get_singleton()) {
            host_parent = ei->get_base_control();
        }
    }
#endif
    if (host_parent) {
        host_parent->add_child(shared_window_);
    }
    shared_window_->popup_centered();
    return shared_instance_;
}

void NarrativesComposer::open_narrative_resource(
    const Ref<Resource> &p_resource,
    Control *p_owner
) {
    if (NarrativesComposer *composer = get_or_create_shared_composer(p_owner)) {
        if (p_resource.is_valid()) {
            composer->open_asset(p_resource);
        }
    }
}

} // namespace ideam::godot_ext
