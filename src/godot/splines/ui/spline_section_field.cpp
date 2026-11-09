#include "spline_section_field.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

void SplineSectionField::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "section_selected",
        PropertyInfo(Variant::INT, "section_index")
    ));
    ADD_SIGNAL(MethodInfo(
        "curve_type_changed",
        PropertyInfo(Variant::INT, "section_index"),
        PropertyInfo(Variant::INT, "curve_type")
    ));
    ADD_SIGNAL(MethodInfo(
        "weight_changed",
        PropertyInfo(Variant::INT, "section_index"),
        PropertyInfo(Variant::FLOAT, "weight")
    ));
    ADD_SIGNAL(MethodInfo(
        "split_section_requested",
        PropertyInfo(Variant::INT, "section_index")
    ));

    ClassDB::bind_method(D_METHOD("set_section_index", "index"), &SplineSectionField::set_section_index);
    ClassDB::bind_method(D_METHOD("get_section_index"), &SplineSectionField::get_section_index);
    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &SplineSectionField::set_selected);
    ClassDB::bind_method(D_METHOD("is_selected"), &SplineSectionField::is_selected);

    ClassDB::bind_method(D_METHOD("_on_select_badge_pressed"), &SplineSectionField::_on_select_badge_pressed);
    ClassDB::bind_method(D_METHOD("_on_curve_type_selected", "item_idx"), &SplineSectionField::_on_curve_type_selected);
    ClassDB::bind_method(D_METHOD("_on_weight_value_changed", "value"), &SplineSectionField::_on_weight_value_changed);
    ClassDB::bind_method(D_METHOD("_on_split_pressed"), &SplineSectionField::_on_split_pressed);
}

SplineSectionField::SplineSectionField() {
    _build_ui();
}

SplineSectionField::~SplineSectionField() = default;

void SplineSectionField::_build_ui() {
    using namespace godot;

    set_h_size_flags(Control::SIZE_EXPAND_FILL);

    // Subtle indented connector styling to distinguish sections from anchor points
    normal_style.instantiate();
    normal_style->set_bg_color(Color(0.14f, 0.16f, 0.20f, 0.85f));
    normal_style->set_border_width_left(3);
    normal_style->set_border_color(Color(0.32f, 0.52f, 0.78f, 0.75f));
    normal_style->set_corner_radius_all(3);
    normal_style->set_content_margin_left(10.0f);
    normal_style->set_content_margin_right(6.0f);
    normal_style->set_content_margin_top(3.0f);
    normal_style->set_content_margin_bottom(3.0f);

    selected_style.instantiate();
    selected_style->set_bg_color(Color(0.18f, 0.25f, 0.34f, 0.95f));
    selected_style->set_border_width_left(4);
    selected_style->set_border_color(Color(0.45f, 0.78f, 1.0f, 1.0f));
    selected_style->set_corner_radius_all(3);
    selected_style->set_content_margin_left(10.0f);
    selected_style->set_content_margin_right(6.0f);
    selected_style->set_content_margin_top(3.0f);
    selected_style->set_content_margin_bottom(3.0f);

    _apply_selection_style();

    HBoxContainer *row = memnew(HBoxContainer);
    row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    row->add_theme_constant_override("separation", 6);
    add_child(row);

    // 1. Section topological badge button ("S0 (P0 → P1)")
    select_badge_btn = memnew(Button);
    select_badge_btn->set_flat(true);
    select_badge_btn->set_text("S0 (P0 -> P1)");
    select_badge_btn->set_custom_minimum_size(Vector2(108.0f, 0.0f));
    select_badge_btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
    select_badge_btn->set_tooltip_text("Click to highlight this section in the visual curve editor.");
    select_badge_btn->connect("pressed", Callable(this, "_on_select_badge_pressed"));
    row->add_child(select_badge_btn);

    // 2. Curve Type Selector
    curve_dropdown = memnew(OptionButton);
    curve_dropdown->add_item("Linear", CURVE_LINEAR);
    curve_dropdown->add_item("Catmull-Rom", CURVE_CATMULL_ROM);
    curve_dropdown->add_item("Cubic Bezier", CURVE_CUBIC_BEZIER);
    curve_dropdown->add_item("Step", CURVE_STEP);
    curve_dropdown->set_custom_minimum_size(Vector2(118.0f, 0.0f));
    curve_dropdown->set_tooltip_text("Interpolation mode for this spline section.");
    curve_dropdown->connect("item_selected", Callable(this, "_on_curve_type_selected"));
    row->add_child(curve_dropdown);

    // 3. Section Weight Label + SpinBox
    Label *weight_lbl = memnew(Label);
    weight_lbl->set_text("W:");
    weight_lbl->set_tooltip_text("Relative temporal/parametric weight of this section.");
    row->add_child(weight_lbl);

    weight_spin = memnew(SpinBox);
    weight_spin->set_min(0.0001);
    weight_spin->set_max(1000.0);
    weight_spin->set_step(0.05);
    weight_spin->set_allow_greater(true);
    weight_spin->set_value(1.0);
    weight_spin->set_custom_minimum_size(Vector2(76.0f, 0.0f));
    weight_spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    weight_spin->set_tooltip_text("Relative section weight (controls horizontal width in weighted mode).");
    weight_spin->connect("value_changed", Callable(this, "_on_weight_value_changed"));
    row->add_child(weight_spin);

    // 4. Cached Arc-Length Readout
    length_label = memnew(Label);
    length_label->set_text("Len: 0.00");
    length_label->set_custom_minimum_size(Vector2(74.0f, 0.0f));
    length_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
    length_label->add_theme_color_override("font_color", Color(0.70f, 0.76f, 0.84f, 0.9f));
    length_label->set_tooltip_text("Approximate geometric arc-length of this section.");
    row->add_child(length_label);

    // 5. Split Section Button (inserts midpoint between P[i] and P[i+1])
    split_btn = memnew(Button);
    split_btn->set_text("+ Split");
    split_btn->set_tooltip_text("Insert a new point at the midpoint (t = 0.5) of this section.");
    split_btn->connect("pressed", Callable(this, "_on_split_pressed"));
    row->add_child(split_btn);
}

void SplineSectionField::_apply_selection_style() {
    add_theme_stylebox_override("panel", selected ? selected_style : normal_style);
}

void SplineSectionField::configure_section(
    int p_section_index,
    int p_from_point_index,
    int p_to_point_index,
    bool p_is_wrap_section,
    SplineCurveType p_curve_type,
    real_t p_weight,
    real_t p_length
) {
    is_updating_ui   = true;
    section_index    = p_section_index;
    from_point_index = p_from_point_index;
    to_point_index   = p_to_point_index;
    is_wrap_section  = p_is_wrap_section;

    if (select_badge_btn) {
        godot::String badge_text = godot::String("S") + godot::String::num_int64(section_index) +
            godot::String(" (P") + godot::String::num_int64(from_point_index) +
            (is_wrap_section ? godot::String(" ↻ P") : godot::String(" → P")) +
            godot::String::num_int64(to_point_index) + godot::String(")");
        select_badge_btn->set_text(badge_text);
    }

    if (curve_dropdown) {
        const int idx = curve_dropdown->get_item_index(static_cast<int32_t>(p_curve_type));
        if (idx >= 0) {
            curve_dropdown->select(idx);
        }
    }

    if (weight_spin) {
        weight_spin->set_value_no_signal(static_cast<double>(p_weight));
    }

    if (length_label) {
        length_label->set_text(
            godot::String("Len: ") + godot::String::num(static_cast<double>(p_length), 2)
        );
    }

    is_updating_ui = false;
}

void SplineSectionField::set_section_index(int p_index) {
    section_index = p_index;
}

void SplineSectionField::set_selected(bool p_selected) {
    if (selected == p_selected) return;
    selected = p_selected;
    _apply_selection_style();
}

void SplineSectionField::_on_select_badge_pressed() {
    emit_signal("section_selected", section_index);
}

void SplineSectionField::_on_curve_type_selected(int p_item_idx) {
    if (is_updating_ui || !curve_dropdown) return;
    const int32_t id = curve_dropdown->get_item_id(p_item_idx);
    emit_signal("curve_type_changed", section_index, id);
}

void SplineSectionField::_on_weight_value_changed(double p_value) {
    if (is_updating_ui) return;
    emit_signal("weight_changed", section_index, static_cast<real_t>(p_value));
}

void SplineSectionField::_on_split_pressed() {
    if (is_updating_ui) return;
    emit_signal("split_section_requested", section_index);
}

} // namespace ideam::godot_ext