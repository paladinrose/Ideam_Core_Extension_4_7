#include "spline_control_base.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

void SplineControlBase::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(LIST_VIEW_INTERLEAVED);
    BIND_ENUM_CONSTANT(LIST_VIEW_POINTS_ONLY);
    BIND_ENUM_CONSTANT(LIST_VIEW_SECTIONS_ONLY);

    ADD_SIGNAL(MethodInfo(
        "point_selected",
        PropertyInfo(Variant::INT, "point_index")
    ));
    ADD_SIGNAL(MethodInfo(
        "section_selected",
        PropertyInfo(Variant::INT, "section_index")
    ));

    ClassDB::bind_method(D_METHOD("set_spline_resource", "resource"), &SplineControlBase::set_spline_resource);
    ClassDB::bind_method(D_METHOD("get_spline_resource"), &SplineControlBase::get_spline_resource);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline_resource", PROPERTY_HINT_RESOURCE_TYPE, "SplineResource"),
        "set_spline_resource",
        "get_spline_resource"
    );

    ClassDB::bind_method(D_METHOD("set_list_view_mode", "mode"), &SplineControlBase::set_list_view_mode);
    ClassDB::bind_method(D_METHOD("get_list_view_mode"), &SplineControlBase::get_list_view_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "list_view_mode", PROPERTY_HINT_ENUM, "Interleaved (P ⇄ S):0,Points Only:1,Sections Only:2"),
        "set_list_view_mode",
        "get_list_view_mode"
    );

    ClassDB::bind_method(D_METHOD("set_minimum_list_height", "height"), &SplineControlBase::set_minimum_list_height);
    ClassDB::bind_method(D_METHOD("get_minimum_list_height"), &SplineControlBase::get_minimum_list_height);

    ClassDB::bind_method(D_METHOD("select_point", "point_index"), &SplineControlBase::select_point);
    ClassDB::bind_method(D_METHOD("get_selected_point_index"), &SplineControlBase::get_selected_point_index);
    ClassDB::bind_method(D_METHOD("select_section", "section_index"), &SplineControlBase::select_section);
    ClassDB::bind_method(D_METHOD("get_selected_section_index"), &SplineControlBase::get_selected_section_index);
    ClassDB::bind_method(D_METHOD("refresh_ui"), &SplineControlBase::refresh_ui);

    // Internal callbacks
    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &SplineControlBase::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_wrap_toggled", "wrap"), &SplineControlBase::_on_wrap_toggled);
    ClassDB::bind_method(D_METHOD("_on_default_curve_selected", "item_idx"), &SplineControlBase::_on_default_curve_selected);
    ClassDB::bind_method(D_METHOD("_on_default_weight_changed", "weight"), &SplineControlBase::_on_default_weight_changed);
    ClassDB::bind_method(D_METHOD("_on_view_mode_selected", "item_idx"), &SplineControlBase::_on_view_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_add_point_pressed"), &SplineControlBase::_on_add_point_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_pressed"), &SplineControlBase::_on_clear_pressed);

    ClassDB::bind_method(D_METHOD("_on_row_point_selected", "point_index"), &SplineControlBase::_on_row_point_selected);
    ClassDB::bind_method(D_METHOD("_on_row_point_position_changed", "point_index", "pos"), &SplineControlBase::_on_row_point_position_changed);
    ClassDB::bind_method(D_METHOD("_on_row_point_back_handle_changed", "point_index", "handle", "enabled"), &SplineControlBase::_on_row_point_back_handle_changed);
    ClassDB::bind_method(D_METHOD("_on_row_point_fore_handle_changed", "point_index", "handle", "enabled"), &SplineControlBase::_on_row_point_fore_handle_changed);
    ClassDB::bind_method(D_METHOD("_on_row_remove_point_requested", "point_index"), &SplineControlBase::_on_row_remove_point_requested);

    ClassDB::bind_method(D_METHOD("_on_row_section_selected", "section_index"), &SplineControlBase::_on_row_section_selected);
    ClassDB::bind_method(D_METHOD("_on_row_section_curve_changed", "section_index", "curve_type"), &SplineControlBase::_on_row_section_curve_changed);
    ClassDB::bind_method(D_METHOD("_on_row_section_weight_changed", "section_index", "weight"), &SplineControlBase::_on_row_section_weight_changed);
    ClassDB::bind_method(D_METHOD("_on_row_split_section_requested", "section_index"), &SplineControlBase::_on_row_split_section_requested);
}

SplineControlBase::SplineControlBase() {
    _build_base_ui();
}

SplineControlBase::~SplineControlBase() {
    if (spline_resource.is_valid()) {
        const godot::Callable cb(this, "_on_resource_changed");
        if (spline_resource->is_connected("changed", cb)) {
            spline_resource->disconnect("changed", cb);
        }
    }
}

void SplineControlBase::_build_base_ui() {
    using namespace godot;

    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);

    // 1. Top Global Toolbar (Wrap, Default Curve, Default Weight, View Filter, + Add, Clear)
    global_toolbar = memnew(HBoxContainer);
    global_toolbar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    global_toolbar->add_theme_constant_override("separation", 6);
    add_child(global_toolbar);

    wrap_check = memnew(CheckBox);
    wrap_check->set_text("Wrap");
    wrap_check->set_tooltip_text("Close the spline loop by adding a wrapping section from the last point back to P0.");
    wrap_check->connect("toggled", Callable(this, "_on_wrap_toggled"));
    global_toolbar->add_child(wrap_check);

    Label *def_curve_lbl = memnew(Label);
    def_curve_lbl->set_text("Def Curve:");
    global_toolbar->add_child(def_curve_lbl);

    default_curve_opt = memnew(OptionButton);
    default_curve_opt->add_item("Linear", CURVE_LINEAR);
    default_curve_opt->add_item("Catmull-Rom", CURVE_CATMULL_ROM);
    default_curve_opt->add_item("Cubic Bezier", CURVE_CUBIC_BEZIER);
    default_curve_opt->add_item("Step", CURVE_STEP);
    default_curve_opt->set_tooltip_text("Default interpolation mode assigned to newly added sections.");
    default_curve_opt->connect("item_selected", Callable(this, "_on_default_curve_selected"));
    global_toolbar->add_child(default_curve_opt);

    Label *def_w_lbl = memnew(Label);
    def_w_lbl->set_text("Def W:");
    global_toolbar->add_child(def_w_lbl);

    default_weight_spin = memnew(SpinBox);
    default_weight_spin->set_min(0.0001);
    default_weight_spin->set_max(1000.0);
    default_weight_spin->set_step(0.05);
    default_weight_spin->set_value(1.0);
    default_weight_spin->set_allow_greater(true);
    default_weight_spin->set_custom_minimum_size(Vector2(68.0f, 0.0f));
    default_weight_spin->set_tooltip_text("Default weight assigned to newly added sections.");
    default_weight_spin->connect("value_changed", Callable(this, "_on_default_weight_changed"));
    global_toolbar->add_child(default_weight_spin);

    view_mode_opt = memnew(OptionButton);
    view_mode_opt->add_item("Interleaved (P ⇄ S)", LIST_VIEW_INTERLEAVED);
    view_mode_opt->add_item("Points Only", LIST_VIEW_POINTS_ONLY);
    view_mode_opt->add_item("Sections Only", LIST_VIEW_SECTIONS_ONLY);
    view_mode_opt->set_tooltip_text("Switch between an alternating Point->Section list, Points only, or Sections only.");
    view_mode_opt->connect("item_selected", Callable(this, "_on_view_mode_selected"));
    global_toolbar->add_child(view_mode_opt);

    Control *spacer = memnew(Control);
    spacer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    global_toolbar->add_child(spacer);

    add_point_btn = memnew(Button);
    add_point_btn->set_text("+ Add Point");
    add_point_btn->set_tooltip_text("Append a new point to the end of the spline.");
    add_point_btn->connect("pressed", Callable(this, "_on_add_point_pressed"));
    global_toolbar->add_child(add_point_btn);

    clear_btn = memnew(Button);
    clear_btn->set_text("Clear");
    clear_btn->set_tooltip_text("Remove all points and sections from the spline.");
    clear_btn->connect("pressed", Callable(this, "_on_clear_pressed"));
    global_toolbar->add_child(clear_btn);

    // 2. Subclass Custom Header Slot (e.g., 1D Presets & Equalizer toggles)
    custom_header_slot = memnew(VBoxContainer);
    custom_header_slot->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(custom_header_slot);

    // 3. Subclass Visual Editor Slot (e.g., Spline1DEqualizerCanvas, Spline2DCanvas)
    visual_editor_slot = memnew(VBoxContainer);
    visual_editor_slot->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(visual_editor_slot);

    // 4. Summary Telemetry Bar
    telemetry_bar = memnew(HBoxContainer);
    telemetry_bar->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(telemetry_bar);

    telemetry_label = memnew(Label);
    telemetry_label->set_text("No SplineResource bound");
    telemetry_label->add_theme_color_override("font_color", Color(0.68f, 0.74f, 0.82f, 0.95f));
    telemetry_bar->add_child(telemetry_label);

    // 5. Scrollable Point & Section Chain List
    chain_scroll = memnew(ScrollContainer);
    chain_scroll->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    chain_scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    chain_scroll->set_custom_minimum_size(Vector2(0.0f, 220.0f));
    add_child(chain_scroll);

    chain_list_vbox = memnew(VBoxContainer);
    chain_list_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    chain_list_vbox->add_theme_constant_override("separation", 4);
    chain_scroll->add_child(chain_list_vbox);
}

void SplineControlBase::set_spline_resource(const godot::Ref<SplineResource> &p_resource) {
    if (spline_resource == p_resource) return;

    const godot::Callable cb(this, "_on_resource_changed");
    if (spline_resource.is_valid() && spline_resource->is_connected("changed", cb)) {
        spline_resource->disconnect("changed", cb);
    }

    spline_resource = p_resource;
    selected_point_index   = -1;
    selected_section_index = -1;
    last_built_point_count   = -1;
    last_built_section_count = -1;

    if (spline_resource.is_valid() && !spline_resource->is_connected("changed", cb)) {
        spline_resource->connect("changed", cb);
    }

    _on_resource_rebound();
    refresh_ui();
}

void SplineControlBase::set_list_view_mode(SplineListViewMode p_mode) {
    if (list_view_mode == p_mode) return;
    list_view_mode = p_mode;
    if (view_mode_opt) {
        const int idx = view_mode_opt->get_item_index(static_cast<int32_t>(p_mode));
        if (idx >= 0) view_mode_opt->select(idx);
    }
    refresh_ui();
}

void SplineControlBase::set_minimum_list_height(float p_height) {
    if (chain_scroll) {
        chain_scroll->set_custom_minimum_size(godot::Vector2(0.0f, p_height));
    }
}

float SplineControlBase::get_minimum_list_height() const {
    return chain_scroll ? chain_scroll->get_custom_minimum_size().y : 0.0f;
}

void SplineControlBase::select_point(int p_point_index) {
    if (selected_point_index == p_point_index && selected_section_index == -1) return;
    selected_point_index   = p_point_index;
    selected_section_index = -1;

    for (size_t i = 0; i < point_fields.size(); ++i) {
        if (point_fields[i]) {
            point_fields[i]->set_selected(static_cast<int>(i) == selected_point_index);
        }
    }
    for (size_t i = 0; i < section_fields.size(); ++i) {
        if (section_fields[i]) {
            section_fields[i]->set_selected(false);
        }
    }

    _on_selection_updated();
    emit_signal("point_selected", selected_point_index);
}

void SplineControlBase::select_section(int p_section_index) {
    if (selected_section_index == p_section_index && selected_point_index == -1) return;
    selected_section_index = p_section_index;
    selected_point_index   = -1;

    for (size_t i = 0; i < point_fields.size(); ++i) {
        if (point_fields[i]) {
            point_fields[i]->set_selected(false);
        }
    }
    for (size_t i = 0; i < section_fields.size(); ++i) {
        if (section_fields[i]) {
            section_fields[i]->set_selected(static_cast<int>(i) == selected_section_index);
        }
    }

    _on_selection_updated();
    emit_signal("section_selected", selected_section_index);
}

void SplineControlBase::_clear_chain_rows() {
    if (!chain_list_vbox) return;
    for (int i = chain_list_vbox->get_child_count() - 1; i >= 0; --i) {
        godot::Node *child = chain_list_vbox->get_child(i);
        chain_list_vbox->remove_child(child);
        memdelete(child);
    }
    point_fields.clear();
    section_fields.clear();
    last_built_point_count   = -1;
    last_built_section_count = -1;
}

void SplineControlBase::_rebuild_chain_rows(int p_point_count, int p_section_count) {
    using namespace godot;
    if (!chain_list_vbox) return;

    _clear_chain_rows();

    point_fields.resize(static_cast<size_t>(p_point_count), nullptr);
    section_fields.resize(static_cast<size_t>(p_section_count), nullptr);

    const int dim = _get_expected_dimension();

    auto create_point_widget = [&](int p_idx) -> SplinePointField * {
        SplinePointField *pf = memnew(SplinePointField);
        pf->set_dimension(dim);
        pf->set_point_index(p_idx);
        pf->connect("point_selected", Callable(this, "_on_row_point_selected"));
        pf->connect("position_changed", Callable(this, "_on_row_point_position_changed"));
        pf->connect("back_handle_changed", Callable(this, "_on_row_point_back_handle_changed"));
        pf->connect("fore_handle_changed", Callable(this, "_on_row_point_fore_handle_changed"));
        pf->connect("remove_point_requested", Callable(this, "_on_row_remove_point_requested"));
        point_fields[static_cast<size_t>(p_idx)] = pf;
        return pf;
    };

    auto create_section_widget = [&](int p_sec_idx) -> SplineSectionField * {
        SplineSectionField *sf = memnew(SplineSectionField);
        sf->set_section_index(p_sec_idx);
        sf->connect("section_selected", Callable(this, "_on_row_section_selected"));
        sf->connect("curve_type_changed", Callable(this, "_on_row_section_curve_changed"));
        sf->connect("weight_changed", Callable(this, "_on_row_section_weight_changed"));
        sf->connect("split_section_requested", Callable(this, "_on_row_split_section_requested"));
        section_fields[static_cast<size_t>(p_sec_idx)] = sf;
        return sf;
    };

    switch (list_view_mode) {
        case LIST_VIEW_INTERLEAVED: {
            // Alternating topological chain: Point[0] -> Section[0] -> Point[1] -> Section[1] ...
            for (int i = 0; i < p_point_count; ++i) {
                chain_list_vbox->add_child(create_point_widget(i));
                if (i < p_section_count) {
                    chain_list_vbox->add_child(create_section_widget(i));
                }
            }
            break;
        }
        case LIST_VIEW_POINTS_ONLY: {
            for (int i = 0; i < p_point_count; ++i) {
                chain_list_vbox->add_child(create_point_widget(i));
            }
            break;
        }
        case LIST_VIEW_SECTIONS_ONLY: {
            for (int s = 0; s < p_section_count; ++s) {
                chain_list_vbox->add_child(create_section_widget(s));
            }
            break;
        }
    }

    last_built_point_count   = p_point_count;
    last_built_section_count = p_section_count;
    last_built_view_mode     = list_view_mode;
}

void SplineControlBase::_populate_point_field(int p_index, SplinePointField *p_field) {
    if (!p_field || spline_resource.is_null()) return;
    p_field->configure_point_variant(
        p_index,
        0.0f,
        0.0f,
        spline_resource->get_point_use_back_handle(p_index),
        0.0f,
        spline_resource->get_point_use_fore_handle(p_index),
        spline_resource->get_point_count() > 0
    );
}

void SplineControlBase::refresh_ui() {
    using namespace godot;

    is_updating_ui = true;

    const bool has_res = spline_resource.is_valid();
    if (wrap_check) wrap_check->set_disabled(!has_res);
    if (default_curve_opt) default_curve_opt->set_disabled(!has_res);
    if (default_weight_spin) default_weight_spin->set_editable(has_res);
    if (add_point_btn) add_point_btn->set_disabled(!has_res);
    if (clear_btn) clear_btn->set_disabled(!has_res);

    if (!has_res) {
        _clear_chain_rows();
        if (telemetry_label) {
            telemetry_label->set_text("No SplineResource assigned.");
        }
        _on_ui_refreshed();
        is_updating_ui = false;
        return;
    }

    // Update global toolbar controls
    if (wrap_check) {
        wrap_check->set_pressed_no_signal(spline_resource->get_wrap());
    }
    if (default_curve_opt) {
        const int idx = default_curve_opt->get_item_index(
            static_cast<int32_t>(spline_resource->get_default_curve_type())
        );
        if (idx >= 0) default_curve_opt->select(idx);
    }
    if (default_weight_spin) {
        default_weight_spin->set_value_no_signal(static_cast<double>(spline_resource->get_default_weight()));
    }

    const int pt_count  = spline_resource->get_point_count();
    const int sec_count = spline_resource->get_section_count();
    const bool is_wrap  = spline_resource->get_wrap();

    // Update summary telemetry readout
    if (telemetry_label) {
        const double total_w   = static_cast<double>(spline_resource->get_total_weight());
        const double total_len = static_cast<double>(spline_resource->get_total_spline_length());
        telemetry_label->set_text(
            String("Points: ") + String::num_int64(pt_count) +
            String("   |   Sections: ") + String::num_int64(sec_count) +
            String("   |   Total Weight: ") + String::num(total_w, 2) +
            String("   |   Arc Length: ") + String::num(total_len, 2)
        );
    }

    // Rebuild row controls only if topology or filter mode changed
    if (pt_count != last_built_point_count ||
        sec_count != last_built_section_count ||
        list_view_mode != last_built_view_mode) {
        _rebuild_chain_rows(pt_count, sec_count);
    }

    // Update Point rows in-place
    for (int i = 0; i < pt_count; ++i) {
        if (SplinePointField *pf = point_fields[static_cast<size_t>(i)]) {
            _populate_point_field(i, pf);
            pf->set_selected(i == selected_point_index);
        }
    }

    // Update Section rows in-place
    for (int s = 0; s < sec_count; ++s) {
        if (SplineSectionField *sf = section_fields[static_cast<size_t>(s)]) {
            const bool wrap_sec = is_wrap && (s == pt_count - 1);
            const int from_pt   = s;
            const int to_pt     = wrap_sec ? 0 : (s + 1);
            sf->configure_section(
                s,
                from_pt,
                to_pt,
                wrap_sec,
                spline_resource->get_section_curve_type(s),
                spline_resource->get_section_weight(s),
                spline_resource->get_section_length(s)
            );
            sf->set_selected(s == selected_section_index);
        }
    }

    _on_ui_refreshed();
    is_updating_ui = false;
}

// ============================================================================
// Signal Handlers
// ============================================================================

void SplineControlBase::_on_resource_changed() {
    if (is_updating_ui) return;
    refresh_ui();
}

void SplineControlBase::_on_wrap_toggled(bool p_wrap) {
    if (is_updating_ui || spline_resource.is_null()) return;
    spline_resource->commit_set_wrap(p_wrap);
}

void SplineControlBase::_on_default_curve_selected(int p_item_idx) {
    if (is_updating_ui || spline_resource.is_null() || !default_curve_opt) return;
    const int32_t id = default_curve_opt->get_item_id(p_item_idx);
    spline_resource->set_default_curve_type(static_cast<SplineCurveType>(id));
}

void SplineControlBase::_on_default_weight_changed(double p_weight) {
    if (is_updating_ui || spline_resource.is_null()) return;
    spline_resource->set_default_weight(static_cast<real_t>(p_weight));
}

void SplineControlBase::_on_view_mode_selected(int p_item_idx) {
    if (is_updating_ui || !view_mode_opt) return;
    const int32_t id = view_mode_opt->get_item_id(p_item_idx);
    set_list_view_mode(static_cast<SplineListViewMode>(id));
}

void SplineControlBase::_on_add_point_pressed() {
    if (is_updating_ui || spline_resource.is_null()) return;
    _commit_add_default_point();
}

void SplineControlBase::_on_clear_pressed() {
    if (is_updating_ui || spline_resource.is_null()) return;
    spline_resource->commit_clear();
}

void SplineControlBase::_on_row_point_selected(int p_point_index) {
    select_point(p_point_index);
}

void SplineControlBase::_on_row_point_position_changed(int p_point_index, const godot::Variant &p_pos) {
    if (is_updating_ui || spline_resource.is_null()) return;
    _commit_point_position(p_point_index, p_pos);
}

void SplineControlBase::_on_row_point_back_handle_changed(
    int p_point_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    if (is_updating_ui || spline_resource.is_null()) return;
    _commit_point_back_handle(p_point_index, p_handle, p_enabled);
}

void SplineControlBase::_on_row_point_fore_handle_changed(
    int p_point_index,
    const godot::Variant &p_handle,
    bool p_enabled
) {
    if (is_updating_ui || spline_resource.is_null()) return;
    _commit_point_fore_handle(p_point_index, p_handle, p_enabled);
}

void SplineControlBase::_on_row_remove_point_requested(int p_point_index) {
    if (is_updating_ui || spline_resource.is_null()) return;
    spline_resource->commit_remove_point(p_point_index);
}

void SplineControlBase::_on_row_section_selected(int p_section_index) {
    select_section(p_section_index);
}

void SplineControlBase::_on_row_section_curve_changed(int p_section_index, int p_curve_type) {
    if (is_updating_ui || spline_resource.is_null()) return;
    spline_resource->commit_set_section_curve_type(
        p_section_index,
        static_cast<SplineCurveType>(p_curve_type)
    );
}

void SplineControlBase::_on_row_section_weight_changed(int p_section_index, double p_weight) {
    if (is_updating_ui || spline_resource.is_null()) return;
    spline_resource->commit_set_section_weight(p_section_index, static_cast<real_t>(p_weight));
}

void SplineControlBase::_on_row_split_section_requested(int p_section_index) {
    if (is_updating_ui || spline_resource.is_null()) return;
    _commit_split_section(p_section_index);
}

} // namespace ideam::godot_ext