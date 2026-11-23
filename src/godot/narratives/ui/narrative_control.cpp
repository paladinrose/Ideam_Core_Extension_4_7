#include "narrative_control.h"

#include "../narremes/character.h"
#include "../narremes/incident.h"
#include "../narremes/location.h"
#include "../narremes/plot.h"
#include "../narremes/prop.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

namespace ideam::godot_ext {

void NarrativeControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_narrative", "narrative"), &NarrativeControl::set_narrative);
    ClassDB::bind_method(D_METHOD("get_narrative"), &NarrativeControl::get_narrative);
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &NarrativeControl::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &NarrativeControl::get_tool_context);
    ClassDB::bind_method(D_METHOD("set_active_family", "family"), &NarrativeControl::set_active_family);
    ClassDB::bind_method(D_METHOD("get_active_family"), &NarrativeControl::get_active_family);
    ClassDB::bind_method(D_METHOD("refresh_lists"), &NarrativeControl::refresh_lists);

    ClassDB::bind_method(D_METHOD("_on_family_tab_changed", "tab"), &NarrativeControl::_on_family_tab_changed);
    ClassDB::bind_method(D_METHOD("_on_add_literal_pressed"), &NarrativeControl::_on_add_literal_pressed);
    ClassDB::bind_method(D_METHOD("_on_add_symbolic_pressed"), &NarrativeControl::_on_add_symbolic_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_literal_pressed"), &NarrativeControl::_on_remove_literal_pressed);
    ClassDB::bind_method(D_METHOD("_on_remove_symbolic_pressed"), &NarrativeControl::_on_remove_symbolic_pressed);
    ClassDB::bind_method(D_METHOD("_on_convert_to_symbolic_pressed"), &NarrativeControl::_on_convert_to_symbolic_pressed);
    ClassDB::bind_method(D_METHOD("_on_convert_to_literal_pressed"), &NarrativeControl::_on_convert_to_literal_pressed);
    ClassDB::bind_method(D_METHOD("_on_instantiate_from_symbolic_pressed"), &NarrativeControl::_on_instantiate_from_symbolic_pressed);
    ClassDB::bind_method(D_METHOD("_on_assign_selected_pressed"), &NarrativeControl::_on_assign_selected_pressed);
    ClassDB::bind_method(D_METHOD("_on_auto_fit_pressed"), &NarrativeControl::_on_auto_fit_pressed);
    ClassDB::bind_method(D_METHOD("_on_vacate_symbolic_pressed"), &NarrativeControl::_on_vacate_symbolic_pressed);
    ClassDB::bind_method(D_METHOD("_on_literal_selected", "index"), &NarrativeControl::_on_literal_selected);
    ClassDB::bind_method(D_METHOD("_on_symbolic_selected", "index"), &NarrativeControl::_on_symbolic_selected);
    ClassDB::bind_method(D_METHOD("_on_literal_activated", "index"), &NarrativeControl::_on_literal_activated);
    ClassDB::bind_method(D_METHOD("_on_symbolic_activated", "index"), &NarrativeControl::_on_symbolic_activated);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &NarrativeControl::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_narrative_changed"), &NarrativeControl::_on_narrative_changed);

    ADD_SIGNAL(MethodInfo("narreme_selected", PropertyInfo(Variant::OBJECT, "narreme", PROPERTY_HINT_RESOURCE_TYPE, "Narreme")));
    ADD_SIGNAL(MethodInfo("open_narreme_requested", PropertyInfo(Variant::OBJECT, "narreme", PROPERTY_HINT_RESOURCE_TYPE, "Narreme")));

    BIND_ENUM_CONSTANT(FAMILY_CHARACTERS);
    BIND_ENUM_CONSTANT(FAMILY_LOCATIONS);
    BIND_ENUM_CONSTANT(FAMILY_PROPS);
    BIND_ENUM_CONSTANT(FAMILY_INCIDENTS);
    BIND_ENUM_CONSTANT(FAMILY_PLOTS);
}

NarrativeControl::NarrativeControl() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);
    domain_tool::stamp_ui_selector(this, "narratives.narrative_control");
    domain_tool::attach_help_topic(this, StringName("NarrativeControl"));
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());
    _build_ui();
}

PackedStringArray NarrativeControl::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("narrative");
    tags.append("roster");
    tags.append("symbolic");
    tags.append("literal");
    tags.append("casting");
    return tags;
}

void NarrativeControl::_build_ui() {
    // Row 1: Family Tabs + Help Button
    HBoxContainer *top_row = memnew(HBoxContainer);
    add_child(top_row);

    family_tabs_ = memnew(TabBar);
    family_tabs_->set_h_size_flags(SIZE_EXPAND_FILL);
    family_tabs_->add_tab("Characters / Roles");
    family_tabs_->add_tab("Locations / Places");
    family_tabs_->add_tab("Props / Symbols");
    family_tabs_->add_tab("Incidents / Moments");
    family_tabs_->add_tab("Plots / Stories");
    domain_tool::stamp_ui_selector(family_tabs_, "narratives.narrative_control.family_tabs");
    family_tabs_->connect("tab_changed", Callable(this, "_on_family_tab_changed"));
    top_row->add_child(family_tabs_);

    Button *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Help: Real vs Symbolic Narreme Roster");
    domain_tool::stamp_ui_selector(help_btn, "narratives.narrative_control.help");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed"));
    top_row->add_child(help_btn);

    // Row 2: Creation / Utility Bar
    HBoxContainer *create_row = memnew(HBoxContainer);
    create_row->add_theme_constant_override("separation", 6);
    add_child(create_row);

    new_name_edit_ = memnew(LineEdit);
    new_name_edit_->set_placeholder("New narreme name (optional)...");
    new_name_edit_->set_h_size_flags(SIZE_EXPAND_FILL);
    domain_tool::stamp_ui_selector(new_name_edit_, "narratives.narrative_control.new_name");
    create_row->add_child(new_name_edit_);

    Button *add_lit_btn = memnew(Button);
    add_lit_btn->set_text("+ Real");
    domain_tool::stamp_ui_selector(add_lit_btn, "narratives.narrative_control.add_literal");
    add_lit_btn->connect("pressed", Callable(this, "_on_add_literal_pressed"));
    create_row->add_child(add_lit_btn);

    Button *add_sym_btn = memnew(Button);
    add_sym_btn->set_text("+ Symbolic");
    domain_tool::stamp_ui_selector(add_sym_btn, "narratives.narrative_control.add_symbolic");
    add_sym_btn->connect("pressed", Callable(this, "_on_add_symbolic_pressed"));
    create_row->add_child(add_sym_btn);

    fit_mode_opt_ = memnew(OptionButton);
    fit_mode_opt_->add_item("Fit: Basic", FIT_BASIC);
    fit_mode_opt_->add_item("Fit: Symbolic", FIT_SYMBOLIC);
    fit_mode_opt_->add_item("Fit: Literal", FIT_LITERAL);
    fit_mode_opt_->add_item("Fit: Thorough", FIT_THOROUGH);
    fit_mode_opt_->select(3);
    create_row->add_child(fit_mode_opt_);

    // Row 3: Two-Column Split (Real | Center Utility Actions | Symbolic)
    HBoxContainer *columns = memnew(HBoxContainer);
    columns->set_h_size_flags(SIZE_EXPAND_FILL);
    columns->set_v_size_flags(SIZE_EXPAND_FILL);
    columns->add_theme_constant_override("separation", 8);
    add_child(columns);

    // Left Column: Real (Literal)
    VBoxContainer *left_col = memnew(VBoxContainer);
    left_col->set_h_size_flags(SIZE_EXPAND_FILL);
    left_col->set_v_size_flags(SIZE_EXPAND_FILL);
    columns->add_child(left_col);

    literal_header_lbl_ = memnew(Label);
    literal_header_lbl_->add_theme_color_override("font_color", Color(0.75f, 0.92f, 0.78f));
    left_col->add_child(literal_header_lbl_);

    literal_list_ = memnew(ItemList);
    literal_list_->set_h_size_flags(SIZE_EXPAND_FILL);
    literal_list_->set_v_size_flags(SIZE_EXPAND_FILL);
    literal_list_->set_custom_minimum_size(Vector2(200, 220));
    domain_tool::stamp_ui_selector(literal_list_, "narratives.narrative_control.literal_list");
    literal_list_->connect("item_selected", Callable(this, "_on_literal_selected"));
    literal_list_->connect("item_activated", Callable(this, "_on_literal_activated"));
    left_col->add_child(literal_list_);

    Button *del_lit_btn = memnew(Button);
    del_lit_btn->set_text("Remove Real");
    del_lit_btn->connect("pressed", Callable(this, "_on_remove_literal_pressed"));
    left_col->add_child(del_lit_btn);

    // Center Action Strip
    VBoxContainer *mid_col = memnew(VBoxContainer);
    mid_col->set_v_size_flags(SIZE_SHRINK_CENTER);
    mid_col->add_theme_constant_override("separation", 6);
    columns->add_child(mid_col);

    auto add_mid_btn = [&](const char *p_label, const char *p_tip, const char *p_sel, const char *p_slot) {
        Button *b = memnew(Button);
        b->set_text(p_label);
        b->set_tooltip_text(p_tip);
        domain_tool::stamp_ui_selector(b, p_sel);
        b->connect("pressed", Callable(this, p_slot));
        mid_col->add_child(b);
    };

    add_mid_btn("Assign ->", "Fill selected Symbolic form with selected Real narreme", "narratives.narrative_control.assign", "_on_assign_selected_pressed");
    add_mid_btn("Auto-Fit", "Find best-fit Real narreme for selected Symbolic form and fill it", "narratives.narrative_control.auto_fit", "_on_auto_fit_pressed");
    add_mid_btn("Vacate", "Vacate selected Symbolic form's assignment", "narratives.narrative_control.vacate", "_on_vacate_symbolic_pressed");
    add_mid_btn("Instantiate <-", "Generate a new Real instance cloned from selected Symbolic form", "narratives.narrative_control.instantiate", "_on_instantiate_from_symbolic_pressed");
    add_mid_btn("To Symbolic >>", "Convert selected Real narreme into a Symbolic form", "narratives.narrative_control.to_symbolic", "_on_convert_to_symbolic_pressed");
    add_mid_btn("<< To Real", "Convert selected Symbolic form into a Real narreme", "narratives.narrative_control.to_literal", "_on_convert_to_literal_pressed");

    // Right Column: Symbolic
    VBoxContainer *right_col = memnew(VBoxContainer);
    right_col->set_h_size_flags(SIZE_EXPAND_FILL);
    right_col->set_v_size_flags(SIZE_EXPAND_FILL);
    columns->add_child(right_col);

    symbolic_header_lbl_ = memnew(Label);
    symbolic_header_lbl_->add_theme_color_override("font_color", Color(0.86f, 0.78f, 1.0f));
    right_col->add_child(symbolic_header_lbl_);

    symbolic_list_ = memnew(ItemList);
    symbolic_list_->set_h_size_flags(SIZE_EXPAND_FILL);
    symbolic_list_->set_v_size_flags(SIZE_EXPAND_FILL);
    symbolic_list_->set_custom_minimum_size(Vector2(200, 220));
    domain_tool::stamp_ui_selector(symbolic_list_, "narratives.narrative_control.symbolic_list");
    symbolic_list_->connect("item_selected", Callable(this, "_on_symbolic_selected"));
    symbolic_list_->connect("item_activated", Callable(this, "_on_symbolic_activated"));
    right_col->add_child(symbolic_list_);

    Button *del_sym_btn = memnew(Button);
    del_sym_btn->set_text("Remove Symbolic");
    del_sym_btn->connect("pressed", Callable(this, "_on_remove_symbolic_pressed"));
    right_col->add_child(del_sym_btn);

    // Status Footer
    status_lbl_ = memnew(Label);
    status_lbl_->add_theme_color_override("font_color", Color(0.74f, 0.78f, 0.84f));
    add_child(status_lbl_);

    _update_column_headers();
}

void NarrativeControl::bind_domain_asset(
    const Ref<Resource> &p_asset,
    const Ref<ToolContext> &p_context
) {
    set_tool_context(p_context);
    if (Ref<Narrative> nar = p_asset; nar.is_valid()) {
        set_narrative(nar);
    }
}

void NarrativeControl::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
    if (narrative_.is_valid() && tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        narrative_->propagate_undo_redo(tool_context_->get_undo_redo());
    }
}

void NarrativeControl::set_narrative(const Ref<Narrative> &p_narrative) {
    if (narrative_.is_valid()) {
        if (narrative_->is_connected("roster_changed", Callable(this, "_on_narrative_changed"))) {
            narrative_->disconnect("roster_changed", Callable(this, "_on_narrative_changed"));
        }
        if (narrative_->is_connected("changed", Callable(this, "_on_narrative_changed"))) {
            narrative_->disconnect("changed", Callable(this, "_on_narrative_changed"));
        }
    }
    narrative_ = p_narrative;
    if (narrative_.is_valid()) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
            narrative_->propagate_undo_redo(tool_context_->get_undo_redo());
        }
        narrative_->connect("roster_changed", Callable(this, "_on_narrative_changed"));
        narrative_->connect("changed", Callable(this, "_on_narrative_changed"));
    }
    refresh_lists();
}

void NarrativeControl::set_active_family(NarremeFamily p_family) {
    active_family_ = p_family;
    if (family_tabs_) {
        family_tabs_->set_current_tab(static_cast<int>(p_family));
    }
    _update_column_headers();
    refresh_lists();
}

void NarrativeControl::_update_column_headers() {
    switch (active_family_) {
        case FAMILY_CHARACTERS:
            literal_header_lbl_->set_text("Real: Characters");
            symbolic_header_lbl_->set_text("Symbolic: Roles (Assignments)");
            break;
        case FAMILY_LOCATIONS:
            literal_header_lbl_->set_text("Real: Locations");
            symbolic_header_lbl_->set_text("Symbolic: Places (Assignments)");
            break;
        case FAMILY_PROPS:
            literal_header_lbl_->set_text("Real: Props");
            symbolic_header_lbl_->set_text("Symbolic: Symbols (Assignments)");
            break;
        case FAMILY_INCIDENTS:
            literal_header_lbl_->set_text("Real: Incidents");
            symbolic_header_lbl_->set_text("Symbolic: Moments (Assignments)");
            break;
        case FAMILY_PLOTS:
            literal_header_lbl_->set_text("Real: Plots");
            symbolic_header_lbl_->set_text("Symbolic: Stories (Assignments)");
            break;
    }
}

int NarrativeControl::_get_selected_literal_index() const {
    const PackedInt32Array sel = literal_list_->get_selected_items();
    return sel.is_empty() ? -1 : sel[0];
}

int NarrativeControl::_get_selected_symbolic_index() const {
    const PackedInt32Array sel = symbolic_list_->get_selected_items();
    return sel.is_empty() ? -1 : sel[0];
}

void NarrativeControl::refresh_lists() {
    literal_list_->clear();
    symbolic_list_->clear();

    if (!narrative_.is_valid()) {
        status_lbl_->set_text("No Narrative bound.");
        return;
    }

    auto populate_pair = [&](const Array &p_lits, const Array &p_syms, const TypedArray<int> &p_assigns, const char *p_lit_fallback, const char *p_sym_fallback) {
        for (int i = 0; i < p_lits.size(); ++i) {
            Ref<Narreme> n = p_lits[i];
            String label = (n.is_valid() && !n->get_official_name().is_empty())
                ? n->get_official_name()
                : (String(p_lit_fallback) + String(" #") + String::num_int64(i));
            literal_list_->add_item(label);
        }
        for (int i = 0; i < p_syms.size(); ++i) {
            Ref<Narreme> s = p_syms[i];
            String label = (s.is_valid() && !s->get_official_name().is_empty())
                ? s->get_official_name()
                : (String(p_sym_fallback) + String(" #") + String::num_int64(i));

            int assigned = (i < p_assigns.size()) ? static_cast<int>(p_assigns[i]) : -1;
            if (assigned >= 0 && assigned < p_lits.size()) {
                Ref<Narreme> lit = p_lits[assigned];
                const String lit_name = (lit.is_valid() && !lit->get_official_name().is_empty())
                    ? lit->get_official_name()
                    : (String(p_lit_fallback) + String(" #") + String::num_int64(assigned));
                label += String("   ->   ") + lit_name;
            } else {
                label += "   ->   [Unassigned]";
            }
            symbolic_list_->add_item(label);
        }
        status_lbl_->set_text(
            String::num_int64(p_lits.size()) + String(" real  |  ") + String::num_int64(p_syms.size()) + String(" symbolic")
        );
    };

    switch (active_family_) {
        case FAMILY_CHARACTERS:
            populate_pair(narrative_->get_characters(), narrative_->get_roles(), narrative_->get_role_assignments(), "Character", "Role");
            break;
        case FAMILY_LOCATIONS:
            populate_pair(narrative_->get_locations(), narrative_->get_places(), narrative_->get_place_assignments(), "Location", "Place");
            break;
        case FAMILY_PROPS:
            populate_pair(narrative_->get_props(), narrative_->get_symbols(), narrative_->get_symbol_assignments(), "Prop", "Symbol");
            break;
        case FAMILY_INCIDENTS:
            populate_pair(narrative_->get_incidents(), narrative_->get_moments(), narrative_->get_moment_assignments(), "Incident", "Moment");
            break;
        case FAMILY_PLOTS:
            populate_pair(narrative_->get_plots(), narrative_->get_stories(), narrative_->get_story_assignments(), "Plot", "Story");
            break;
    }
}

void NarrativeControl::_on_family_tab_changed(int p_tab) {
    active_family_ = static_cast<NarremeFamily>(p_tab);
    _update_column_headers();
    refresh_lists();
}

void NarrativeControl::_on_add_literal_pressed() {
    if (!narrative_.is_valid()) return;
    const String name = new_name_edit_->get_text().strip_edges();

    switch (active_family_) {
        case FAMILY_CHARACTERS: {
            Ref<Character> c; c.instantiate();
            c->set_official_name(name.is_empty() ? (String("Character ") + String::num_int64(narrative_->get_characters().size() + 1)) : name);
            narrative_->add_character(c);
            break;
        }
        case FAMILY_LOCATIONS: {
            Ref<Location> l; l.instantiate();
            l->set_official_name(name.is_empty() ? (String("Location ") + String::num_int64(narrative_->get_locations().size() + 1)) : name);
            narrative_->add_location(l);
            break;
        }
        case FAMILY_PROPS: {
            Ref<Prop> p; p.instantiate();
            p->set_official_name(name.is_empty() ? (String("Prop ") + String::num_int64(narrative_->get_props().size() + 1)) : name);
            narrative_->add_prop(p);
            break;
        }
        case FAMILY_INCIDENTS: {
            Ref<Incident> inc; inc.instantiate();
            inc->set_official_name(name.is_empty() ? (String("Incident ") + String::num_int64(narrative_->get_incidents().size() + 1)) : name);
            narrative_->add_incident(inc);
            break;
        }
        case FAMILY_PLOTS: {
            Ref<Plot> pl; pl.instantiate();
            pl->set_official_name(name.is_empty() ? (String("Plot ") + String::num_int64(narrative_->get_plots().size() + 1)) : name);
            narrative_->add_plot(pl);
            break;
        }
    }
    new_name_edit_->clear();
}

void NarrativeControl::_on_add_symbolic_pressed() {
    if (!narrative_.is_valid()) return;
    const String name = new_name_edit_->get_text().strip_edges();

    switch (active_family_) {
        case FAMILY_CHARACTERS: {
            Ref<Character> r; r.instantiate();
            r->set_official_name(name.is_empty() ? (String("Role ") + String::num_int64(narrative_->get_roles().size() + 1)) : name);
            narrative_->add_role(r);
            break;
        }
        case FAMILY_LOCATIONS: {
            Ref<Location> p; p.instantiate();
            p->set_official_name(name.is_empty() ? (String("Place ") + String::num_int64(narrative_->get_places().size() + 1)) : name);
            narrative_->add_place(p);
            break;
        }
        case FAMILY_PROPS: {
            Ref<Prop> s; s.instantiate();
            s->set_official_name(name.is_empty() ? (String("Symbol ") + String::num_int64(narrative_->get_symbols().size() + 1)) : name);
            narrative_->add_symbol(s);
            break;
        }
        case FAMILY_INCIDENTS: {
            Ref<Incident> m; m.instantiate();
            m->set_official_name(name.is_empty() ? (String("Moment ") + String::num_int64(narrative_->get_moments().size() + 1)) : name);
            narrative_->add_moment(m);
            break;
        }
        case FAMILY_PLOTS: {
            Ref<Plot> st; st.instantiate();
            st->set_official_name(name.is_empty() ? (String("Story ") + String::num_int64(narrative_->get_stories().size() + 1)) : name);
            narrative_->add_story(st);
            break;
        }
    }
    new_name_edit_->clear();
}

void NarrativeControl::_on_remove_literal_pressed() {
    if (!narrative_.is_valid()) return;
    const int idx = _get_selected_literal_index();
    if (idx < 0) return;

    switch (active_family_) {
        case FAMILY_CHARACTERS: narrative_->remove_character_at(idx); break;
        case FAMILY_LOCATIONS:  narrative_->remove_location_at(idx); break;
        case FAMILY_PROPS:      narrative_->remove_prop_at(idx); break;
        case FAMILY_INCIDENTS:  narrative_->remove_incident_at(idx); break;
        case FAMILY_PLOTS:      narrative_->remove_plot_at(idx); break;
    }
}

void NarrativeControl::_on_remove_symbolic_pressed() {
    if (!narrative_.is_valid()) return;
    const int idx = _get_selected_symbolic_index();
    if (idx < 0) return;

    switch (active_family_) {
        case FAMILY_CHARACTERS: narrative_->remove_role_at(idx); break;
        case FAMILY_LOCATIONS:  narrative_->remove_place_at(idx); break;
        case FAMILY_PROPS:      narrative_->remove_symbol_at(idx); break;
        case FAMILY_INCIDENTS:  narrative_->remove_moment_at(idx); break;
        case FAMILY_PLOTS:      narrative_->remove_story_at(idx); break;
    }
}

void NarrativeControl::_on_convert_to_symbolic_pressed() {
    if (!narrative_.is_valid()) return;
    const int idx = _get_selected_literal_index();
    if (idx < 0) return;

    switch (active_family_) {
        case FAMILY_CHARACTERS: narrative_->convert_character_to_role(idx); break;
        case FAMILY_LOCATIONS:  narrative_->convert_location_to_place(idx); break;
        case FAMILY_PROPS:      narrative_->convert_prop_to_symbol(idx); break;
        case FAMILY_INCIDENTS:  narrative_->convert_incident_to_moment(idx); break;
        case FAMILY_PLOTS:      narrative_->convert_plot_to_story(idx); break;
    }
}

void NarrativeControl::_on_convert_to_literal_pressed() {
    if (!narrative_.is_valid()) return;
    const int idx = _get_selected_symbolic_index();
    if (idx < 0) return;

    switch (active_family_) {
        case FAMILY_CHARACTERS: narrative_->convert_role_to_character(idx); break;
        case FAMILY_LOCATIONS:  narrative_->convert_place_to_location(idx); break;
        case FAMILY_PROPS:      narrative_->convert_symbol_to_prop(idx); break;
        case FAMILY_INCIDENTS:  narrative_->convert_moment_to_incident(idx); break;
        case FAMILY_PLOTS:      narrative_->convert_story_to_plot(idx); break;
    }
}

void NarrativeControl::_on_instantiate_from_symbolic_pressed() {
    if (!narrative_.is_valid()) return;
    const int idx = _get_selected_symbolic_index();
    if (idx < 0) return;
    const String name = new_name_edit_->get_text().strip_edges();

    switch (active_family_) {
        case FAMILY_CHARACTERS: narrative_->instantiate_character_from_role(idx, name, true); break;
        case FAMILY_LOCATIONS:  narrative_->instantiate_location_from_place(idx, name, true); break;
        case FAMILY_PROPS:      narrative_->instantiate_prop_from_symbol(idx, name, true); break;
        case FAMILY_INCIDENTS:  narrative_->instantiate_incident_from_moment(idx, name, true); break;
        case FAMILY_PLOTS:      narrative_->instantiate_plot_from_story(idx, name, true); break;
    }
    new_name_edit_->clear();
}

void NarrativeControl::_on_assign_selected_pressed() {
    if (!narrative_.is_valid()) return;
    const int lit_idx = _get_selected_literal_index();
    const int sym_idx = _get_selected_symbolic_index();
    if (lit_idx < 0 || sym_idx < 0) return;

    switch (active_family_) {
        case FAMILY_CHARACTERS: narrative_->fill_role(sym_idx, lit_idx, REPLACE_THOROUGH); break;
        case FAMILY_LOCATIONS:  narrative_->fill_place(sym_idx, lit_idx, REPLACE_THOROUGH); break;
        case FAMILY_PROPS:      narrative_->fill_symbol(sym_idx, lit_idx, REPLACE_THOROUGH); break;
        case FAMILY_INCIDENTS:  narrative_->fill_moment(sym_idx, lit_idx, REPLACE_THOROUGH); break;
        case FAMILY_PLOTS:      narrative_->fill_story(sym_idx, lit_idx, REPLACE_THOROUGH); break;
    }
}

void NarrativeControl::_on_auto_fit_pressed() {
    if (!narrative_.is_valid()) return;
    const int sym_idx = _get_selected_symbolic_index();
    if (sym_idx < 0) return;

    const BestNarrativeFit fit = static_cast<BestNarrativeFit>(fit_mode_opt_->get_selected_id());
    const TypedArray<int> empty_mask;
    int best_id = -1;

    switch (active_family_) {
        case FAMILY_CHARACTERS:
            best_id = narrative_->get_best_fit_character_for_role(sym_idx, empty_mask, fit);
            if (best_id >= 0) narrative_->fill_role(sym_idx, best_id, REPLACE_THOROUGH);
            break;
        case FAMILY_LOCATIONS:
            best_id = narrative_->get_best_fit_location_for_place(sym_idx, empty_mask, fit);
            if (best_id >= 0) narrative_->fill_place(sym_idx, best_id, REPLACE_THOROUGH);
            break;
        case FAMILY_PROPS:
            best_id = narrative_->get_best_fit_prop_for_symbol(sym_idx, empty_mask, fit);
            if (best_id >= 0) narrative_->fill_symbol(sym_idx, best_id, REPLACE_THOROUGH);
            break;
        case FAMILY_INCIDENTS:
            best_id = narrative_->get_best_fit_incident_for_moment(sym_idx, empty_mask, fit);
            if (best_id >= 0) narrative_->fill_moment(sym_idx, best_id, REPLACE_THOROUGH);
            break;
        case FAMILY_PLOTS:
            best_id = narrative_->get_best_fit_plot_for_story(sym_idx, empty_mask, fit);
            if (best_id >= 0) narrative_->fill_story(sym_idx, best_id, REPLACE_THOROUGH);
            break;
    }
}

void NarrativeControl::_on_vacate_symbolic_pressed() {
    if (!narrative_.is_valid()) return;
    const int sym_idx = _get_selected_symbolic_index();
    if (sym_idx < 0) return;

    switch (active_family_) {
        case FAMILY_CHARACTERS: narrative_->vacate_role(sym_idx); break;
        case FAMILY_LOCATIONS:  narrative_->vacate_place(sym_idx); break;
        case FAMILY_PROPS:      narrative_->vacate_symbol(sym_idx); break;
        case FAMILY_INCIDENTS:  narrative_->vacate_moment(sym_idx); break;
        case FAMILY_PLOTS:      narrative_->vacate_story(sym_idx); break;
    }
}

void NarrativeControl::_on_literal_selected(int p_index) {
    if (!narrative_.is_valid() || p_index < 0) return;
    Ref<Narreme> chosen;
    switch (active_family_) {
        case FAMILY_CHARACTERS: if (p_index < narrative_->get_characters().size()) chosen = narrative_->get_characters()[p_index]; break;
        case FAMILY_LOCATIONS:  if (p_index < narrative_->get_locations().size())  chosen = narrative_->get_locations()[p_index]; break;
        case FAMILY_PROPS:      if (p_index < narrative_->get_props().size())      chosen = narrative_->get_props()[p_index]; break;
        case FAMILY_INCIDENTS:  if (p_index < narrative_->get_incidents().size())  chosen = narrative_->get_incidents()[p_index]; break;
        case FAMILY_PLOTS:      if (p_index < narrative_->get_plots().size())      chosen = narrative_->get_plots()[p_index]; break;
    }
    if (chosen.is_valid()) {
        emit_signal("narreme_selected", chosen);
    }
}

void NarrativeControl::_on_symbolic_selected(int p_index) {
    if (!narrative_.is_valid() || p_index < 0) return;
    Ref<Narreme> chosen;
    switch (active_family_) {
        case FAMILY_CHARACTERS: if (p_index < narrative_->get_roles().size())   chosen = narrative_->get_roles()[p_index]; break;
        case FAMILY_LOCATIONS:  if (p_index < narrative_->get_places().size())  chosen = narrative_->get_places()[p_index]; break;
        case FAMILY_PROPS:      if (p_index < narrative_->get_symbols().size()) chosen = narrative_->get_symbols()[p_index]; break;
        case FAMILY_INCIDENTS:  if (p_index < narrative_->get_moments().size()) chosen = narrative_->get_moments()[p_index]; break;
        case FAMILY_PLOTS:      if (p_index < narrative_->get_stories().size()) chosen = narrative_->get_stories()[p_index]; break;
    }
    if (chosen.is_valid()) {
        emit_signal("narreme_selected", chosen);
    }
}

void NarrativeControl::_on_literal_activated(int p_index) {
    _on_literal_selected(p_index);
    if (!narrative_.is_valid() || p_index < 0) return;
    Ref<Narreme> chosen;
    switch (active_family_) {
        case FAMILY_CHARACTERS: if (p_index < narrative_->get_characters().size()) chosen = narrative_->get_characters()[p_index]; break;
        case FAMILY_LOCATIONS:  if (p_index < narrative_->get_locations().size())  chosen = narrative_->get_locations()[p_index]; break;
        case FAMILY_PROPS:      if (p_index < narrative_->get_props().size())      chosen = narrative_->get_props()[p_index]; break;
        case FAMILY_INCIDENTS:  if (p_index < narrative_->get_incidents().size())  chosen = narrative_->get_incidents()[p_index]; break;
        case FAMILY_PLOTS:      if (p_index < narrative_->get_plots().size())      chosen = narrative_->get_plots()[p_index]; break;
    }
    if (chosen.is_valid()) {
        emit_signal("open_narreme_requested", chosen);
    }
}

void NarrativeControl::_on_symbolic_activated(int p_index) {
    _on_symbolic_selected(p_index);
    if (!narrative_.is_valid() || p_index < 0) return;
    Ref<Narreme> chosen;
    switch (active_family_) {
        case FAMILY_CHARACTERS: if (p_index < narrative_->get_roles().size())   chosen = narrative_->get_roles()[p_index]; break;
        case FAMILY_LOCATIONS:  if (p_index < narrative_->get_places().size())  chosen = narrative_->get_places()[p_index]; break;
        case FAMILY_PROPS:      if (p_index < narrative_->get_symbols().size()) chosen = narrative_->get_symbols()[p_index]; break;
        case FAMILY_INCIDENTS:  if (p_index < narrative_->get_moments().size()) chosen = narrative_->get_moments()[p_index]; break;
        case FAMILY_PLOTS:      if (p_index < narrative_->get_stories().size()) chosen = narrative_->get_stories()[p_index]; break;
    }
    if (chosen.is_valid()) {
        emit_signal("open_narreme_requested", chosen);
    }
}

void NarrativeControl::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void NarrativeControl::_on_narrative_changed() {
    refresh_lists();
}

} // namespace ideam::godot_ext
