#include "plot_outliner.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_split_container.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void PlotOutliner::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_narrative", "narrative"), &PlotOutliner::set_narrative);
    ClassDB::bind_method(D_METHOD("get_narrative"), &PlotOutliner::get_narrative);
    ClassDB::bind_method(D_METHOD("set_plot", "plot"), &PlotOutliner::set_plot);
    ClassDB::bind_method(D_METHOD("get_plot"), &PlotOutliner::get_plot);
    ClassDB::bind_method(D_METHOD("set_incident", "incident"), &PlotOutliner::set_incident);
    ClassDB::bind_method(D_METHOD("get_incident"), &PlotOutliner::get_incident);
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &PlotOutliner::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &PlotOutliner::get_tool_context);
    ClassDB::bind_method(D_METHOD("refresh_all"), &PlotOutliner::refresh_all);

    ClassDB::bind_method(D_METHOD("_on_plot_picker_selected", "index"), &PlotOutliner::_on_plot_picker_selected);
    ClassDB::bind_method(D_METHOD("_on_evaluate_plot_pressed"), &PlotOutliner::_on_evaluate_plot_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_plot_event_pressed"), &PlotOutliner::_on_add_plot_event_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_plot_event_pressed"), &PlotOutliner::_on_remove_plot_event_pressed);
    ClassDB::bind_method(D_METHOD("_on_new_incident_pressed"), &PlotOutliner::_on_new_incident_pressed);
    ClassDB::bind_method(D_METHOD("_on_event_selected", "index"), &PlotOutliner::_on_event_selected);
    ClassDB::bind_method(D_METHOD("_on_apply_event_pressed"), &PlotOutliner::_on_apply_event_pressed);
    ClassDB::bind_method(D_METHOD("_on_evaluate_incident_pressed"), &PlotOutliner::_on_evaluate_incident_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_narrative_cond_pressed"), &PlotOutliner::_on_add_narrative_cond_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_gameplay_cond_pressed"), &PlotOutliner::_on_add_gameplay_cond_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_causal_cond_pressed"), &PlotOutliner::_on_add_causal_cond_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_condition_pressed", "cond_index"), &PlotOutliner::_on_remove_condition_pressed);
    ClassDB::bind_method(D_METHOD("_on_gameplay_status_selected", "option_idx", "cond_index"), &PlotOutliner::_on_gameplay_status_selected);
    ClassDB::bind_method(D_METHOD("_on_add_causal_prereq_pressed", "cond_index"), &PlotOutliner::_on_add_causal_prereq_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &PlotOutliner::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_narrative_changed"), &PlotOutliner::_on_narrative_changed);

    ADD_SIGNAL(MethodInfo("incident_selected", PropertyInfo(Variant::OBJECT, "incident", PROPERTY_HINT_RESOURCE_TYPE, "Incident")));
}

PlotOutliner::PlotOutliner() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);
    domain_tool::stamp_ui_selector(this, "narratives.plot_outliner");
    domain_tool::attach_help_topic(this, StringName("PlotOutliner"));
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());
    _build_ui();
}

PackedStringArray PlotOutliner::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("narrative");
    tags.append("plot");
    tags.append("incident");
    tags.append("condition");
    tags.append("outliner");
    tags.append("quest");
    return tags;
}

String PlotOutliner::_plot_status_to_string(PlotStatus p_status) {
    switch (p_status) {
        case PLOT_NOT_BEGUN:       return "NOT BEGUN";
        case PLOT_IN_PROGRESS:     return "IN PROGRESS";
        case PLOT_COMPLETE:        return "COMPLETE";
        case PLOT_CANNOT_COMPLETE: return "CANNOT COMPLETE";
        case PLOT_FAILED:          return "FAILED";
    }
    return "UNKNOWN";
}

String PlotOutliner::_incident_status_to_string(IncidentStatus p_status) {
    switch (p_status) {
        case HAS_NOT_HAPPENED: return "PENDING";
        case HAS_HAPPENED:     return "HAPPENED";
        case CANNOT_HAPPEN:    return "IMPOSSIBLE";
    }
    return "UNKNOWN";
}

String PlotOutliner::_requirement_lane_tag(IncidentRequirement p_req) {
    switch (p_req) {
        case INCIDENT_REQ_HAS_HAPPENED:     return "[REQ]";
        case INCIDENT_REQ_INSTANT_SUCCESS:  return "[WIN!]";
        case INCIDENT_REQ_INSTANT_FAILURE:  return "[FAIL!]";
        case INCIDENT_REQ_CANNOT_HAPPEN:    return "[BLOCK]";
        case INCIDENT_REQ_HAS_NOT_HAPPENED: return "[OPEN]";
    }
    return "[EVENT]";
}

void PlotOutliner::_build_ui() {
    // Row 1: Plot Header Bar
    HBoxContainer *top_bar = memnew(HBoxContainer);
    top_bar->add_theme_constant_override("separation", 6);
    add_child(top_bar);

    Label *plot_lbl = memnew(Label);
    plot_lbl->set_text("Plot:");
    top_bar->add_child(plot_lbl);

    plot_picker_opt_ = memnew(OptionButton);
    plot_picker_opt_->set_custom_minimum_size(Vector2(170, 0));
    domain_tool::stamp_ui_selector(plot_picker_opt_, "narratives.plot_outliner.plot_picker");
    plot_picker_opt_->connect("item_selected", Callable(this, "_on_plot_picker_selected"));
    top_bar->add_child(plot_picker_opt_);

    plot_status_lbl_ = memnew(Label);
    plot_status_lbl_->set_text("[NO PLOT]");
    plot_status_lbl_->add_theme_color_override("font_color", Color(0.75f, 0.90f, 1.0f));
    top_bar->add_child(plot_status_lbl_);

    Button *eval_plot_btn = memnew(Button);
    eval_plot_btn->set_text("Evaluate Plot");
    domain_tool::stamp_ui_selector(eval_plot_btn, "narratives.plot_outliner.evaluate_plot");
    eval_plot_btn->connect("pressed", Callable(this, "_on_evaluate_plot_pressed"));
    top_bar->add_child(eval_plot_btn);

    Button *add_ev_btn = memnew(Button);
    add_ev_btn->set_text("+ Plot Event");
    domain_tool::stamp_ui_selector(add_ev_btn, "narratives.plot_outliner.add_plot_event");
    add_ev_btn->connect("pressed", Callable(this, "_on_add_plot_event_pressed"));
    top_bar->add_child(add_ev_btn);

    Button *new_inc_btn = memnew(Button);
    new_inc_btn->set_text("+ New Incident");
    domain_tool::stamp_ui_selector(new_inc_btn, "narratives.plot_outliner.new_incident");
    new_inc_btn->connect("pressed", Callable(this, "_on_new_incident_pressed"));
    top_bar->add_child(new_inc_btn);

    Button *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Help: Plot & Incident Outliner");
    domain_tool::stamp_ui_selector(help_btn, "narratives.plot_outliner.help");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed"));
    top_bar->add_child(help_btn);

    // Row 2: Master-Detail Split (Left: Plot Event Lanes | Right: Incident Condition Clauses)
    HSplitContainer *split = memnew(HSplitContainer);
    split->set_h_size_flags(SIZE_EXPAND_FILL);
    split->set_v_size_flags(SIZE_EXPAND_FILL);
    split->set_split_offset(290);
    add_child(split);

    // Left Pane: Plot Events
    VBoxContainer *left_pane = memnew(VBoxContainer);
    left_pane->set_custom_minimum_size(Vector2(260, 0));
    left_pane->add_theme_constant_override("separation", 6);
    split->add_child(left_pane);

    Label *ev_hdr = memnew(Label);
    ev_hdr->set_text("Plot Events (Outcome Lanes)");
    ev_hdr->add_theme_color_override("font_color", Color(0.86f, 0.80f, 1.0f));
    left_pane->add_child(ev_hdr);

    event_list_ = memnew(ItemList);
    event_list_->set_h_size_flags(SIZE_EXPAND_FILL);
    event_list_->set_v_size_flags(SIZE_EXPAND_FILL);
    domain_tool::stamp_ui_selector(event_list_, "narratives.plot_outliner.event_list");
    event_list_->connect("item_selected", Callable(this, "_on_event_selected"));
    left_pane->add_child(event_list_);

    event_title_edit_ = memnew(LineEdit);
    event_title_edit_->set_placeholder("Plot Event title...");
    left_pane->add_child(event_title_edit_);

    event_incident_opt_ = memnew(OptionButton);
    domain_tool::stamp_ui_selector(event_incident_opt_, "narratives.plot_outliner.event_incident");
    left_pane->add_child(event_incident_opt_);

    event_req_opt_ = memnew(OptionButton);
    event_req_opt_->add_item("[OPEN] Has Not Happened", INCIDENT_REQ_HAS_NOT_HAPPENED);
    event_req_opt_->add_item("[REQ] Has Happened", INCIDENT_REQ_HAS_HAPPENED);
    event_req_opt_->add_item("[BLOCK] Cannot Happen", INCIDENT_REQ_CANNOT_HAPPEN);
    event_req_opt_->add_item("[WIN!] Instant Success", INCIDENT_REQ_INSTANT_SUCCESS);
    event_req_opt_->add_item("[FAIL!] Instant Failure", INCIDENT_REQ_INSTANT_FAILURE);
    domain_tool::stamp_ui_selector(event_req_opt_, "narratives.plot_outliner.event_requirement");
    left_pane->add_child(event_req_opt_);

    HBoxContainer *ev_btns = memnew(HBoxContainer);
    ev_btns->add_theme_constant_override("separation", 6);
    left_pane->add_child(ev_btns);

    Button *apply_ev_btn = memnew(Button);
    apply_ev_btn->set_text("Apply Event");
    apply_ev_btn->set_h_size_flags(SIZE_EXPAND_FILL);
    apply_ev_btn->connect("pressed", Callable(this, "_on_apply_event_pressed"));
    ev_btns->add_child(apply_ev_btn);

    Button *rm_ev_btn = memnew(Button);
    rm_ev_btn->set_text("Remove");
    rm_ev_btn->connect("pressed", Callable(this, "_on_remove_plot_event_pressed"));
    ev_btns->add_child(rm_ev_btn);

    // Right Pane: Active Incident & Condition Clause Builder
    VBoxContainer *right_pane = memnew(VBoxContainer);
    right_pane->set_h_size_flags(SIZE_EXPAND_FILL);
    right_pane->set_v_size_flags(SIZE_EXPAND_FILL);
    right_pane->add_theme_constant_override("separation", 6);
    split->add_child(right_pane);

    HBoxContainer *inc_hdr_row = memnew(HBoxContainer);
    inc_hdr_row->add_theme_constant_override("separation", 6);
    right_pane->add_child(inc_hdr_row);

    incident_header_lbl_ = memnew(Label);
    incident_header_lbl_->set_text("Incident: [Select a Plot Event]");
    incident_header_lbl_->set_h_size_flags(SIZE_EXPAND_FILL);
    incident_header_lbl_->add_theme_color_override("font_color", Color(0.96f, 0.84f, 0.66f));
    inc_hdr_row->add_child(incident_header_lbl_);

    incident_frame_lbl_ = memnew(Label);
    incident_frame_lbl_->add_theme_color_override("font_color", Color(0.62f, 0.85f, 0.96f));
    inc_hdr_row->add_child(incident_frame_lbl_);

    Button *eval_inc_btn = memnew(Button);
    eval_inc_btn->set_text("Evaluate Incident");
    domain_tool::stamp_ui_selector(eval_inc_btn, "narratives.plot_outliner.evaluate_incident");
    eval_inc_btn->connect("pressed", Callable(this, "_on_evaluate_incident_pressed"));
    inc_hdr_row->add_child(eval_inc_btn);

    HBoxContainer *cond_add_row = memnew(HBoxContainer);
    cond_add_row->add_theme_constant_override("separation", 6);
    right_pane->add_child(cond_add_row);

    Label *when_lbl = memnew(Label);
    when_lbl->set_text("WHEN ALL MET:");
    when_lbl->add_theme_color_override("font_color", Color(0.74f, 0.80f, 0.88f));
    cond_add_row->add_child(when_lbl);

    Button *add_nc_btn = memnew(Button);
    add_nc_btn->set_text("+ Narrative Condition");
    domain_tool::stamp_ui_selector(add_nc_btn, "narratives.plot_outliner.add_narrative_cond");
    add_nc_btn->connect("pressed", Callable(this, "_on_add_narrative_cond_pressed"));
    cond_add_row->add_child(add_nc_btn);

    Button *add_gc_btn = memnew(Button);
    add_gc_btn->set_text("+ Gameplay Flag");
    domain_tool::stamp_ui_selector(add_gc_btn, "narratives.plot_outliner.add_gameplay_cond");
    add_gc_btn->connect("pressed", Callable(this, "_on_add_gameplay_cond_pressed"));
    cond_add_row->add_child(add_gc_btn);

    Button *add_cc_btn = memnew(Button);
    add_cc_btn->set_text("+ Causal Gate");
    domain_tool::stamp_ui_selector(add_cc_btn, "narratives.plot_outliner.add_causal_cond");
    add_cc_btn->connect("pressed", Callable(this, "_on_add_causal_cond_pressed"));
    cond_add_row->add_child(add_cc_btn);

    cond_scroll_ = memnew(ScrollContainer);
    cond_scroll_->set_h_size_flags(SIZE_EXPAND_FILL);
    cond_scroll_->set_v_size_flags(SIZE_EXPAND_FILL);
    right_pane->add_child(cond_scroll_);

    cond_list_box_ = memnew(VBoxContainer);
    cond_list_box_->set_h_size_flags(SIZE_EXPAND_FILL);
    cond_list_box_->add_theme_constant_override("separation", 6);
    domain_tool::stamp_ui_selector(cond_list_box_, "narratives.plot_outliner.conditions");
    cond_scroll_->add_child(cond_list_box_);
}

void PlotOutliner::bind_domain_asset(
    const Ref<Resource> &p_asset,
    const Ref<ToolContext> &p_context
) {
    set_tool_context(p_context);
    if (Ref<Narrative> nar = p_asset; nar.is_valid()) {
        set_narrative(nar);
    } else if (Ref<Plot> pl = p_asset; pl.is_valid()) {
        set_plot(pl);
    } else if (Ref<Incident> inc = p_asset; inc.is_valid()) {
        set_incident(inc);
    }
}

void PlotOutliner::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
}

void PlotOutliner::set_narrative(const Ref<Narrative> &p_narrative) {
    if (narrative_.is_valid() && narrative_->is_connected("roster_changed", Callable(this, "_on_narrative_changed"))) {
        narrative_->disconnect("roster_changed", Callable(this, "_on_narrative_changed"));
    }
    narrative_ = p_narrative;
    if (narrative_.is_valid()) {
        narrative_->connect("roster_changed", Callable(this, "_on_narrative_changed"));
        if (!active_plot_.is_valid() && narrative_->get_plots().size() > 0) {
            active_plot_ = narrative_->get_plots()[0];
        }
    }
    refresh_all();
}

void PlotOutliner::set_plot(const Ref<Plot> &p_plot) {
    active_plot_ = p_plot;
    standalone_incident_.unref();
    selected_event_idx_ = 0;
    if (active_plot_.is_valid() && tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        active_plot_->set_undo_redo(tool_context_->get_undo_redo());
    }
    refresh_all();
}

void PlotOutliner::set_incident(const Ref<Incident> &p_incident) {
    standalone_incident_ = p_incident;
    if (standalone_incident_.is_valid() && tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        standalone_incident_->set_undo_redo(tool_context_->get_undo_redo());
    }
    refresh_all();
}

Ref<Plot_Event> PlotOutliner::_get_selected_plot_event() const {
    if (!active_plot_.is_valid()) return nullptr;
    const TypedArray<Plot_Event> evs = active_plot_->get_plot_events();
    if (selected_event_idx_ < 0 || selected_event_idx_ >= evs.size()) return nullptr;
    return evs[selected_event_idx_];
}

Ref<Incident> PlotOutliner::_get_active_incident() const {
    if (Ref<Plot_Event> ev = _get_selected_plot_event(); ev.is_valid() && ev->get_incident().is_valid()) {
        return ev->get_incident();
    }
    return standalone_incident_;
}

void PlotOutliner::_refresh_plot_picker() {
    plot_picker_opt_->clear();
    plot_choices_.clear();
    int sel = -1;

    if (narrative_.is_valid()) {
        const TypedArray<Plot> plots = narrative_->get_plots();
        for (int i = 0; i < plots.size(); ++i) {
            Ref<Plot> p = plots[i];
            if (!p.is_valid()) continue;
            const int pid = static_cast<int>(plot_choices_.size());
            plot_choices_.push_back(p);
            const String lbl = p->get_official_name().is_empty() ? (String("Plot #") + String::num_int64(i)) : p->get_official_name();
            plot_picker_opt_->add_item(lbl, pid);
            if (p == active_plot_) sel = pid;
        }
        const TypedArray<Plot> stories = narrative_->get_stories();
        for (int i = 0; i < stories.size(); ++i) {
            Ref<Plot> st = stories[i];
            if (!st.is_valid()) continue;
            const int pid = static_cast<int>(plot_choices_.size());
            plot_choices_.push_back(st);
            const String lbl = String("[Story] ") + (st->get_official_name().is_empty() ? (String("Story #") + String::num_int64(i)) : st->get_official_name());
            plot_picker_opt_->add_item(lbl, pid);
            if (st == active_plot_) sel = pid;
        }
    } else if (active_plot_.is_valid()) {
        plot_choices_.push_back(active_plot_);
        plot_picker_opt_->add_item(active_plot_->get_official_name().is_empty() ? String("Plot") : active_plot_->get_official_name(), 0);
        sel = 0;
    }

    if (sel >= 0) {
        plot_picker_opt_->select(sel);
    } else if (!plot_choices_.empty()) {
        active_plot_ = plot_choices_[0];
        plot_picker_opt_->select(0);
    }

    plot_status_lbl_->set_text(
        active_plot_.is_valid()
            ? (String("[") + _plot_status_to_string(active_plot_->get_status()) + String("]"))
            : String("[NO PLOT]")
    );
}

void PlotOutliner::_refresh_event_list() {
    event_list_->clear();
    if (!active_plot_.is_valid()) return;

    const TypedArray<Plot_Event> evs = active_plot_->get_plot_events();
    for (int i = 0; i < evs.size(); ++i) {
        Ref<Plot_Event> ev = evs[i];
        if (!ev.is_valid()) continue;
        const String lane = _requirement_lane_tag(ev->get_requirement());
        String label      = lane + String(" ") + (ev->get_title().is_empty() ? (String("Event #") + String::num_int64(i + 1)) : ev->get_title());
        if (ev->get_incident().is_valid()) {
            const Ref<Incident> inc = ev->get_incident();
            label += String(" -> ") + (inc->get_official_name().is_empty() ? String("Incident") : inc->get_official_name()) +
                     String(" (") + _incident_status_to_string(inc->get_status()) + String(")");
        } else {
            label += " -> [Unlinked]";
        }
        event_list_->add_item(label);
    }

    if (evs.is_empty()) {
        selected_event_idx_ = -1;
    } else {
        if (selected_event_idx_ < 0 || selected_event_idx_ >= evs.size()) {
            selected_event_idx_ = 0;
        }
        event_list_->select(selected_event_idx_);
    }
}

void PlotOutliner::_refresh_event_editor() {
    event_incident_opt_->clear();
    incident_choices_.clear();
    event_incident_opt_->add_item("(No Incident Linked)", -1);

    if (narrative_.is_valid()) {
        const TypedArray<Incident> incs = narrative_->get_incidents();
        for (int i = 0; i < incs.size(); ++i) {
            Ref<Incident> inc = incs[i];
            if (!inc.is_valid()) continue;
            const int iid = static_cast<int>(incident_choices_.size());
            incident_choices_.push_back(inc);
            event_incident_opt_->add_item(inc->get_official_name().is_empty() ? (String("Incident #") + String::num_int64(i)) : inc->get_official_name(), iid);
        }
        const TypedArray<Incident> moms = narrative_->get_moments();
        for (int i = 0; i < moms.size(); ++i) {
            Ref<Incident> m = moms[i];
            if (!m.is_valid()) continue;
            const int iid = static_cast<int>(incident_choices_.size());
            incident_choices_.push_back(m);
            event_incident_opt_->add_item(String("[Moment] ") + (m->get_official_name().is_empty() ? (String("Moment #") + String::num_int64(i)) : m->get_official_name()), iid);
        }
    }

    Ref<Plot_Event> ev = _get_selected_plot_event();
    if (!ev.is_valid()) {
        event_title_edit_->set_text("");
        event_incident_opt_->select(0);
        return;
    }

    event_title_edit_->set_text(ev->get_title());
    const int req_idx = event_req_opt_->get_item_index(static_cast<int>(ev->get_requirement()));
    if (req_idx >= 0) event_req_opt_->select(req_idx);

    int inc_sel = 0;
    if (ev->get_incident().is_valid()) {
        for (size_t i = 0; i < incident_choices_.size(); ++i) {
            if (incident_choices_[i] == ev->get_incident()) {
                inc_sel = event_incident_opt_->get_item_index(static_cast<int>(i));
                break;
            }
        }
    }
    event_incident_opt_->select(inc_sel >= 0 ? inc_sel : 0);
}

void PlotOutliner::_refresh_incident_pane() {
    while (cond_list_box_->get_child_count() > 0) {
        Node *child = cond_list_box_->get_child(0);
        cond_list_box_->remove_child(child);
        child->queue_free();
    }

    Ref<Incident> inc = _get_active_incident();
    if (!inc.is_valid()) {
        incident_header_lbl_->set_text("Incident: [Select or link an Incident]");
        incident_frame_lbl_->set_text("");
        return;
    }

    if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid() && !inc->get_undo_redo().is_valid()) {
        inc->set_undo_redo(tool_context_->get_undo_redo());
    }

    incident_header_lbl_->set_text(
        String("Incident: ") + (inc->get_official_name().is_empty() ? String("<Unnamed>") : inc->get_official_name()) +
        String("  [") + _incident_status_to_string(inc->get_status()) + String("]")
    );
    incident_frame_lbl_->set_text(
        inc->get_semantic_frame().is_valid()
            ? (String("FrameNet: [") + String(inc->get_semantic_frame()->get_frame_name()) + String("]"))
            : String("")
    );

    const TypedArray<Incident_Condition> conds = inc->get_conditions();
    if (conds.is_empty()) {
        Label *empty = memnew(Label);
        empty->set_text("No conditions yet — add a Narrative Condition, Gameplay Flag, or Causal Gate above.");
        empty->add_theme_color_override("font_color", Color(0.65f, 0.70f, 0.78f));
        cond_list_box_->add_child(empty);
        return;
    }

    for (int i = 0; i < conds.size(); ++i) {
        Ref<Incident_Condition> cond = conds[i];
        if (!cond.is_valid()) continue;

        PanelContainer *card = memnew(PanelContainer);
        Ref<StyleBoxFlat> sb;
        sb.instantiate();
        sb->set_bg_color(Color(0.13f, 0.15f, 0.19f, 0.95f));
        sb->set_border_color(Color(0.30f, 0.36f, 0.46f, 0.80f));
        sb->set_border_width_all(1);
        sb->set_corner_radius_all(4);
        sb->set_content_margin_all(6);
        card->add_theme_stylebox_override("panel", sb);
        cond_list_box_->add_child(card);

        HBoxContainer *row = memnew(HBoxContainer);
        row->add_theme_constant_override("separation", 6);
        card->add_child(row);

        if (Ref<Narrative_Condition> nc = cond; nc.is_valid()) {
            NarrativeConditionField *field = memnew(NarrativeConditionField);
            field->set_tool_context(tool_context_);
            field->set_narrative(narrative_);
            field->set_condition(nc);
            row->add_child(field);
        } else if (Ref<Gameplay_Condition> gc = cond; gc.is_valid()) {
            Label *tag = memnew(Label);
            tag->set_text(String("[Gameplay] ") + (gc->get_title().is_empty() ? String("Gameplay Flag") : gc->get_title()));
            tag->set_h_size_flags(SIZE_EXPAND_FILL);
            row->add_child(tag);

            OptionButton *status_opt = memnew(OptionButton);
            status_opt->add_item("UNKNOWN", NarremeConditionStatus::UNKNOWN);
            status_opt->add_item("MET", NarremeConditionStatus::MET);
            status_opt->add_item("NOT MET", NarremeConditionStatus::NOT_MET);
            status_opt->add_item("PERMANENT", NarremeConditionStatus::PERMANENT);
            status_opt->add_item("CANNOT MEET", NarremeConditionStatus::CANNOT_MEET);
            const int sidx = status_opt->get_item_index(static_cast<int>(gc->get_gameplay_status()));
            if (sidx >= 0) status_opt->select(sidx);
            status_opt->connect("item_selected", Callable(this, "_on_gameplay_status_selected").bind(i));
            row->add_child(status_opt);
        } else if (Ref<Causal_Condition> cc = cond; cc.is_valid()) {
            VBoxContainer *cbox = memnew(VBoxContainer);
            cbox->set_h_size_flags(SIZE_EXPAND_FILL);
            row->add_child(cbox);

            Label *ctitle = memnew(Label);
            ctitle->set_text(
                String("[Causal Gate] ") + (cc->get_title().is_empty() ? String("Causal Dependency") : cc->get_title()) +
                String(" (") + String::num_int64(cc->get_conditions().size()) + String(" prerequisites)")
            );
            ctitle->add_theme_color_override("font_color", Color(0.94f, 0.78f, 0.42f));
            cbox->add_child(ctitle);

            Button *add_prereq = memnew(Button);
            add_prereq->set_text("+ Link Previous Condition as Prerequisite");
            add_prereq->connect("pressed", Callable(this, "_on_add_causal_prereq_pressed").bind(i));
            cbox->add_child(add_prereq);
        } else {
            Label *generic = memnew(Label);
            generic->set_text(String("[Condition] ") + cond->get_title());
            generic->set_h_size_flags(SIZE_EXPAND_FILL);
            row->add_child(generic);
        }

        Button *rm_btn = memnew(Button);
        rm_btn->set_text("x");
        rm_btn->set_tooltip_text("Remove this condition from Incident");
        rm_btn->connect("pressed", Callable(this, "_on_remove_condition_pressed").bind(i));
        row->add_child(rm_btn);
    }
}

void PlotOutliner::refresh_all() {
    _refresh_plot_picker();
    _refresh_event_list();
    _refresh_event_editor();
    _refresh_incident_pane();
}

void PlotOutliner::_on_plot_picker_selected(int p_index) {
    if (p_index < 0) return;
    const int pid = plot_picker_opt_->get_item_id(p_index);
    if (pid >= 0 && pid < static_cast<int>(plot_choices_.size())) {
        active_plot_        = plot_choices_[static_cast<size_t>(pid)];
        selected_event_idx_ = 0;
        refresh_all();
    }
}

void PlotOutliner::_on_evaluate_plot_pressed() {
    if (!active_plot_.is_valid()) return;
    active_plot_->evaluate_plot_status();
    refresh_all();
}

void PlotOutliner::_on_add_plot_event_pressed() {
    if (!active_plot_.is_valid()) return;
    Ref<Plot_Event> ev;
    ev.instantiate();
    ev->set_title(String("Beat #") + String::num_int64(active_plot_->get_plot_events().size() + 1));
    ev->set_requirement(INCIDENT_REQ_HAS_HAPPENED);
    active_plot_->add_plot_event(ev);
    selected_event_idx_ = static_cast<int>(active_plot_->get_plot_events().size()) - 1;
    refresh_all();
}

void PlotOutliner::_on_remove_plot_event_pressed() {
    if (!active_plot_.is_valid() || selected_event_idx_ < 0) return;
    active_plot_->remove_plot_event_at(selected_event_idx_);
    refresh_all();
}

void PlotOutliner::_on_new_incident_pressed() {
    Ref<Incident> inc;
    inc.instantiate();
    const int num = narrative_.is_valid() ? static_cast<int>(narrative_->get_incidents().size()) + 1 : 1;
    inc->set_official_name(String("Incident ") + String::num_int64(num));
    if (narrative_.is_valid()) {
        narrative_->add_incident(inc);
    } else {
        standalone_incident_ = inc;
    }

    if (Ref<Plot_Event> ev = _get_selected_plot_event(); ev.is_valid()) {
        ev->commit_edit(ev->get_title(), inc, ev->get_requirement(), "Link New Incident to Plot Event");
    }
    refresh_all();
}

void PlotOutliner::_on_event_selected(int p_index) {
    selected_event_idx_ = p_index;
    _refresh_event_editor();
    _refresh_incident_pane();
    if (Ref<Incident> inc = _get_active_incident(); inc.is_valid()) {
        emit_signal("incident_selected", inc);
    }
}

void PlotOutliner::_on_apply_event_pressed() {
    Ref<Plot_Event> ev = _get_selected_plot_event();
    if (!ev.is_valid()) return;

    Ref<Incident> chosen_inc;
    const int isel = event_incident_opt_->get_selected();
    if (isel >= 0) {
        const int iid = event_incident_opt_->get_item_id(isel);
        if (iid >= 0 && iid < static_cast<int>(incident_choices_.size())) {
            chosen_inc = incident_choices_[static_cast<size_t>(iid)];
        }
    }
    const IncidentRequirement req = static_cast<IncidentRequirement>(event_req_opt_->get_selected_id());
    ev->commit_edit(event_title_edit_->get_text().strip_edges(), chosen_inc, req);
    refresh_all();
}

void PlotOutliner::_on_evaluate_incident_pressed() {
    if (Ref<Incident> inc = _get_active_incident(); inc.is_valid()) {
        inc->evaluate_incident();
        refresh_all();
    }
}

void PlotOutliner::_on_add_narrative_cond_pressed() {
    Ref<Incident> inc = _get_active_incident();
    if (!inc.is_valid()) return;
    Ref<Narrative_Condition> nc;
    nc.instantiate();
    nc->set_title("Narrative Condition");
    inc->add_condition(nc);
    _refresh_incident_pane();
}

void PlotOutliner::_on_add_gameplay_cond_pressed() {
    Ref<Incident> inc = _get_active_incident();
    if (!inc.is_valid()) return;
    Ref<Gameplay_Condition> gc;
    gc.instantiate();
    gc->set_title(String("Gameplay Flag #") + String::num_int64(inc->get_conditions().size() + 1));
    inc->add_condition(gc);
    _refresh_incident_pane();
}

void PlotOutliner::_on_add_causal_cond_pressed() {
    Ref<Incident> inc = _get_active_incident();
    if (!inc.is_valid()) return;
    Ref<Causal_Condition> cc;
    cc.instantiate();
    cc->set_title(String("Causal Gate #") + String::num_int64(inc->get_conditions().size() + 1));
    inc->add_condition(cc);
    _refresh_incident_pane();
}

void PlotOutliner::_on_remove_condition_pressed(int p_cond_index) {
    Ref<Incident> inc = _get_active_incident();
    if (!inc.is_valid()) return;
    inc->remove_condition_at(p_cond_index);
    _refresh_incident_pane();
}

void PlotOutliner::_on_gameplay_status_selected(int p_option_idx, int p_cond_index) {
    Ref<Incident> inc = _get_active_incident();
    if (!inc.is_valid()) return;
    const TypedArray<Incident_Condition> conds = inc->get_conditions();
    if (p_cond_index < 0 || p_cond_index >= conds.size()) return;
    Ref<Gameplay_Condition> gc = conds[p_cond_index];
    if (!gc.is_valid()) return;

    const int status_vals[] = {
        NarremeConditionStatus::UNKNOWN,
        NarremeConditionStatus::MET,
        NarremeConditionStatus::NOT_MET,
        NarremeConditionStatus::PERMANENT,
        NarremeConditionStatus::CANNOT_MEET
    };
    if (p_option_idx >= 0 && p_option_idx < 5) {
        gc->set_gameplay_status(static_cast<NarremeConditionStatus>(status_vals[p_option_idx]));
        gc->evaluate_condition();
    }
}

void PlotOutliner::_on_add_causal_prereq_pressed(int p_cond_index) {
    Ref<Incident> inc = _get_active_incident();
    if (!inc.is_valid()) return;
    const TypedArray<Incident_Condition> conds = inc->get_conditions();
    if (p_cond_index <= 0 || p_cond_index >= conds.size()) return;
    Ref<Causal_Condition> cc = conds[p_cond_index];
    if (!cc.is_valid()) return;

    Ref<Incident_Condition> prev = conds[p_cond_index - 1];
    if (prev.is_valid()) {
        cc->add_condition(prev, NarremeConditionStatus::MET);
        _refresh_incident_pane();
    }
}

void PlotOutliner::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void PlotOutliner::_on_narrative_changed() {
    refresh_all();
}

} // namespace ideam::godot_ext
