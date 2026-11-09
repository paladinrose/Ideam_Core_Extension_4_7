#include "spline_point_field.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>

namespace ideam::godot_ext {

void SplinePointField::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "point_selected",
        PropertyInfo(Variant::INT, "point_index")
    ));
    ADD_SIGNAL(MethodInfo(
        "position_changed",
        PropertyInfo(Variant::INT, "point_index"),
        PropertyInfo(Variant::NIL, "position", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT)
    ));
    ADD_SIGNAL(MethodInfo(
        "back_handle_changed",
        PropertyInfo(Variant::INT, "point_index"),
        PropertyInfo(Variant::NIL, "handle", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT),
        PropertyInfo(Variant::BOOL, "enabled")
    ));
    ADD_SIGNAL(MethodInfo(
        "fore_handle_changed",
        PropertyInfo(Variant::INT, "point_index"),
        PropertyInfo(Variant::NIL, "handle", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT),
        PropertyInfo(Variant::BOOL, "enabled")
    ));
    ADD_SIGNAL(MethodInfo(
        "remove_point_requested",
        PropertyInfo(Variant::INT, "point_index")
    ));

    ClassDB::bind_method(D_METHOD("set_dimension", "dimension"), &SplinePointField::set_dimension);
    ClassDB::bind_method(D_METHOD("get_dimension"), &SplinePointField::get_dimension);
    ClassDB::bind_method(D_METHOD("set_point_index", "index"), &SplinePointField::set_point_index);
    ClassDB::bind_method(D_METHOD("get_point_index"), &SplinePointField::get_point_index);
    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &SplinePointField::set_selected);
    ClassDB::bind_method(D_METHOD("is_selected"), &SplinePointField::is_selected);
    ClassDB::bind_method(D_METHOD("set_handles_expanded", "expanded"), &SplinePointField::set_handles_expanded);
    ClassDB::bind_method(D_METHOD("is_handles_expanded"), &SplinePointField::is_handles_expanded);

    ClassDB::bind_method(D_METHOD("_on_select_badge_pressed"), &SplinePointField::_on_select_badge_pressed);
    ClassDB::bind_method(D_METHOD("_on_handles_toggle_pressed"), &SplinePointField::_on_handles_toggle_pressed);
    ClassDB::bind_method(D_METHOD("_on_delete_pressed"), &SplinePointField::_on_delete_pressed);
    ClassDB::bind_method(D_METHOD("_on_pos_spin_changed", "value"), &SplinePointField::_on_pos_spin_changed);
    ClassDB::bind_method(D_METHOD("_on_back_handle_toggled", "enabled"), &SplinePointField::_on_back_handle_toggled);
    ClassDB::bind_method(D_METHOD("_on_back_spin_changed", "value"), &SplinePointField::_on_back_spin_changed);
    ClassDB::bind_method(D_METHOD("_on_fore_handle_toggled", "enabled"), &SplinePointField::_on_fore_handle_toggled);
    ClassDB::bind_method(D_METHOD("_on_fore_spin_changed", "value"), &SplinePointField::_on_fore_spin_changed);
}

SplinePointField::SplinePointField() {
    _build_ui();
}

SplinePointField::~SplinePointField() = default;

void SplinePointField::_build_ui() {
    using namespace godot;

    set_h_size_flags(Control::SIZE_EXPAND_FILL);

    normal_style.instantiate();
    normal_style->set_bg_color(Color(0.20f, 0.22f, 0.26f, 0.95f));
    normal_style->set_border_width_left(3);
    normal_style->set_border_color(Color(0.88f, 0.64f, 0.24f, 0.90f));
    normal_style->set_corner_radius_all(4);
    normal_style->set_content_margin_left(6.0f);
    normal_style->set_content_margin_right(6.0f);
    normal_style->set_content_margin_top(4.0f);
    normal_style->set_content_margin_bottom(4.0f);

    selected_style.instantiate();
    selected_style->set_bg_color(Color(0.27f, 0.25f, 0.19f, 0.98f));
    selected_style->set_border_width_left(4);
    selected_style->set_border_color(Color(1.0f, 0.80f, 0.30f, 1.0f));
    selected_style->set_corner_radius_all(4);
    selected_style->set_content_margin_left(6.0f);
    selected_style->set_content_margin_right(6.0f);
    selected_style->set_content_margin_top(4.0f);
    selected_style->set_content_margin_bottom(4.0f);

    _apply_selection_style();

    VBoxContainer *root_vbox = memnew(VBoxContainer);
    root_vbox->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    root_vbox->add_theme_constant_override("separation", 4);
    add_child(root_vbox);

    // --- Main Anchor Row ---
    HBoxContainer *main_row = memnew(HBoxContainer);
    main_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    main_row->add_theme_constant_override("separation", 6);
    root_vbox->add_child(main_row);

    select_badge_btn = memnew(Button);
    select_badge_btn->set_text("P0");
    select_badge_btn->set_custom_minimum_size(Vector2(44.0f, 0.0f));
    select_badge_btn->set_tooltip_text("Click to select this anchor point in the visual curve editor.");
    select_badge_btn->connect("pressed", Callable(this, "_on_select_badge_pressed"));
    main_row->add_child(select_badge_btn);

    pos_axes_box = memnew(HBoxContainer);
    pos_axes_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    pos_axes_box->add_theme_constant_override("separation", 4);
    main_row->add_child(pos_axes_box);

    static const char *AXIS_NAMES[MAX_DIMENSIONS] = { "X:", "Y:", "Z:", "W:" };
    static const Color AXIS_COLORS[MAX_DIMENSIONS] = {
        Color(0.95f, 0.45f, 0.45f),
        Color(0.45f, 0.90f, 0.50f),
        Color(0.45f, 0.68f, 0.98f),
        Color(0.88f, 0.55f, 0.95f)
    };

    for (int i = 0; i < MAX_DIMENSIONS; ++i) {
        Label *lbl = memnew(Label);
        lbl->set_text(AXIS_NAMES[i]);
        lbl->add_theme_color_override("font_color", AXIS_COLORS[i]);
        pos_axes_box->add_child(lbl);
        pos_axis_labels[i] = lbl;

        SpinBox *spin = memnew(SpinBox);
        spin->set_min(-100000.0);
        spin->set_max(100000.0);
        spin->set_step(0.01);
        spin->set_allow_lesser(true);
        spin->set_allow_greater(true);
        spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        spin->set_custom_minimum_size(Vector2(68.0f, 0.0f));
        spin->connect("value_changed", Callable(this, "_on_pos_spin_changed"));
        pos_axes_box->add_child(spin);
        pos_spins[i] = spin;
    }

    handles_toggle_btn = memnew(Button);
    handles_toggle_btn->set_text("Handles ▸");
    handles_toggle_btn->set_tooltip_text("Show or hide custom Back and Fore tangent handles for this point.");
    handles_toggle_btn->connect("pressed", Callable(this, "_on_handles_toggle_pressed"));
    main_row->add_child(handles_toggle_btn);

    delete_btn = memnew(Button);
    delete_btn->set_text("✕");
    delete_btn->set_custom_minimum_size(Vector2(28.0f, 0.0f));
    delete_btn->set_tooltip_text("Remove this point from the spline.");
    delete_btn->connect("pressed", Callable(this, "_on_delete_pressed"));
    main_row->add_child(delete_btn);

    // --- Collapsible Back/Fore Handles Drawer ---
    handles_drawer = memnew(VBoxContainer);
    handles_drawer->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    handles_drawer->add_theme_constant_override("separation", 3);
    handles_drawer->set_visible(false);
    root_vbox->add_child(handles_drawer);

    // Back Handle Row
    HBoxContainer *back_row = memnew(HBoxContainer);
    back_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    back_row->add_theme_constant_override("separation", 6);
    handles_drawer->add_child(back_row);

    use_back_check = memnew(CheckBox);
    use_back_check->set_text("Back Handle");
    use_back_check->set_custom_minimum_size(Vector2(115.0f, 0.0f));
    use_back_check->connect("toggled", Callable(this, "_on_back_handle_toggled"));
    back_row->add_child(use_back_check);

    back_axes_box = memnew(HBoxContainer);
    back_axes_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    back_axes_box->add_theme_constant_override("separation", 4);
    back_row->add_child(back_axes_box);

    for (int i = 0; i < MAX_DIMENSIONS; ++i) {
        SpinBox *spin = memnew(SpinBox);
        spin->set_min(-100000.0);
        spin->set_max(100000.0);
        spin->set_step(0.01);
        spin->set_allow_lesser(true);
        spin->set_allow_greater(true);
        spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        spin->set_editable(false);
        spin->connect("value_changed", Callable(this, "_on_back_spin_changed"));
        back_axes_box->add_child(spin);
        back_spins[i] = spin;
    }

    // Fore Handle Row
    HBoxContainer *fore_row = memnew(HBoxContainer);
    fore_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    fore_row->add_theme_constant_override("separation", 6);
    handles_drawer->add_child(fore_row);

    use_fore_check = memnew(CheckBox);
    use_fore_check->set_text("Fore Handle");
    use_fore_check->set_custom_minimum_size(Vector2(115.0f, 0.0f));
    use_fore_check->connect("toggled", Callable(this, "_on_fore_handle_toggled"));
    fore_row->add_child(use_fore_check);

    fore_axes_box = memnew(HBoxContainer);
    fore_axes_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    fore_axes_box->add_theme_constant_override("separation", 4);
    fore_row->add_child(fore_axes_box);

    for (int i = 0; i < MAX_DIMENSIONS; ++i) {
        SpinBox *spin = memnew(SpinBox);
        spin->set_min(-100000.0);
        spin->set_max(100000.0);
        spin->set_step(0.01);
        spin->set_allow_lesser(true);
        spin->set_allow_greater(true);
        spin->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        spin->set_editable(false);
        spin->connect("value_changed", Callable(this, "_on_fore_spin_changed"));
        fore_axes_box->add_child(spin);
        fore_spins[i] = spin;
    }

    _update_dimension_visibility();
}

void SplinePointField::_update_dimension_visibility() {
    if (pos_axis_labels[0]) {
        pos_axis_labels[0]->set_text(dimension == 1 ? "Val:" : "X:");
    }
    for (int i = 0; i < MAX_DIMENSIONS; ++i) {
        const bool show = (i < dimension);
        if (pos_axis_labels[i]) pos_axis_labels[i]->set_visible(show);
        if (pos_spins[i]) pos_spins[i]->set_visible(show);
        if (back_spins[i]) back_spins[i]->set_visible(show);
        if (fore_spins[i]) fore_spins[i]->set_visible(show);
    }
}

void SplinePointField::_apply_selection_style() {
    add_theme_stylebox_override("panel", selected ? selected_style : normal_style);
}

godot::Variant SplinePointField::_pack_spin_values(
    const std::array<godot::SpinBox *, MAX_DIMENSIONS> &p_spins
) const {
    const real_t v0 = p_spins[0] ? static_cast<real_t>(p_spins[0]->get_value()) : static_cast<real_t>(0.0);
    const real_t v1 = p_spins[1] ? static_cast<real_t>(p_spins[1]->get_value()) : static_cast<real_t>(0.0);
    const real_t v2 = p_spins[2] ? static_cast<real_t>(p_spins[2]->get_value()) : static_cast<real_t>(0.0);
    const real_t v3 = p_spins[3] ? static_cast<real_t>(p_spins[3]->get_value()) : static_cast<real_t>(0.0);

    switch (dimension) {
        case 1: return v0;
        case 2: return godot::Vector2(v0, v1);
        case 3: return godot::Vector3(v0, v1, v2);
        case 4: return godot::Vector4(v0, v1, v2, v3);
        default: return v0;
    }
}

void SplinePointField::_unpack_variant_to_spins(
    const godot::Variant &p_val,
    std::array<godot::SpinBox *, MAX_DIMENSIONS> &r_spins
) {
    double vals[MAX_DIMENSIONS] = { 0.0, 0.0, 0.0, 0.0 };
    switch (p_val.get_type()) {
        case godot::Variant::FLOAT:
        case godot::Variant::INT: {
            vals[0] = static_cast<double>(p_val);
            break;
        }
        case godot::Variant::VECTOR2: {
            const godot::Vector2 v = p_val;
            vals[0] = v.x;
            vals[1] = v.y;
            break;
        }
        case godot::Variant::VECTOR3: {
            const godot::Vector3 v = p_val;
            vals[0] = v.x;
            vals[1] = v.y;
            vals[2] = v.z;
            break;
        }
        case godot::Variant::VECTOR4: {
            const godot::Vector4 v = p_val;
            vals[0] = v.x;
            vals[1] = v.y;
            vals[2] = v.z;
            vals[3] = v.w;
            break;
        }
        default:
            break;
    }

    for (int i = 0; i < MAX_DIMENSIONS; ++i) {
        if (r_spins[i]) {
            r_spins[i]->set_value_no_signal(vals[i]);
        }
    }
}

void SplinePointField::set_dimension(int p_dimension) {
    const int clamped = std::clamp(p_dimension, 1, MAX_DIMENSIONS);
    if (dimension == clamped) return;
    dimension = clamped;
    _update_dimension_visibility();
}

void SplinePointField::set_point_index(int p_index) {
    point_index = p_index;
    if (select_badge_btn) {
        select_badge_btn->set_text(godot::String("P") + godot::String::num_int64(point_index));
    }
}

void SplinePointField::set_selected(bool p_selected) {
    if (selected == p_selected) return;
    selected = p_selected;
    _apply_selection_style();
}

void SplinePointField::set_handles_expanded(bool p_expanded) {
    handles_expanded = p_expanded;
    if (handles_drawer) {
        handles_drawer->set_visible(handles_expanded);
    }
    if (handles_toggle_btn) {
        const bool any_active = (use_back_check && use_back_check->is_pressed()) ||
                                (use_fore_check && use_fore_check->is_pressed());
        godot::String prefix = any_active ? godot::String("Handles* ") : godot::String("Handles ");
        handles_toggle_btn->set_text(prefix + (handles_expanded ? godot::String("▾") : godot::String("▸")));
    }
}

void SplinePointField::configure_point_variant(
    int p_point_index,
    const godot::Variant &p_position,
    const godot::Variant &p_back_handle,
    bool p_use_back,
    const godot::Variant &p_fore_handle,
    bool p_use_fore,
    bool p_can_delete
) {
    is_updating_ui = true;
    set_point_index(p_point_index);

    _unpack_variant_to_spins(p_position, pos_spins);
    _unpack_variant_to_spins(p_back_handle, back_spins);
    _unpack_variant_to_spins(p_fore_handle, fore_spins);

    if (use_back_check) {
        use_back_check->set_pressed_no_signal(p_use_back);
    }
    if (use_fore_check) {
        use_fore_check->set_pressed_no_signal(p_use_fore);
    }
    for (int i = 0; i < MAX_DIMENSIONS; ++i) {
        if (back_spins[i]) back_spins[i]->set_editable(p_use_back);
        if (fore_spins[i]) fore_spins[i]->set_editable(p_use_fore);
    }
    if (delete_btn) {
        delete_btn->set_disabled(!p_can_delete);
    }

    set_handles_expanded(handles_expanded);
    is_updating_ui = false;
}

void SplinePointField::configure_point_1d(
    int p_point_index,
    real_t p_position,
    real_t p_back_handle,
    bool p_use_back,
    real_t p_fore_handle,
    bool p_use_fore,
    bool p_can_delete
) {
    set_dimension(1);
    configure_point_variant(
        p_point_index,
        p_position,
        p_back_handle,
        p_use_back,
        p_fore_handle,
        p_use_fore,
        p_can_delete
    );
}

void SplinePointField::_on_select_badge_pressed() {
    emit_signal("point_selected", point_index);
}

void SplinePointField::_on_handles_toggle_pressed() {
    set_handles_expanded(!handles_expanded);
}

void SplinePointField::_on_delete_pressed() {
    if (is_updating_ui) return;
    emit_signal("remove_point_requested", point_index);
}

void SplinePointField::_on_pos_spin_changed(double) {
    if (is_updating_ui) return;
    emit_signal("position_changed", point_index, _pack_spin_values(pos_spins));
}

void SplinePointField::_on_back_handle_toggled(bool p_enabled) {
    if (is_updating_ui) return;
    for (int i = 0; i < MAX_DIMENSIONS; ++i) {
        if (back_spins[i]) back_spins[i]->set_editable(p_enabled);
    }
    set_handles_expanded(handles_expanded);
    emit_signal("back_handle_changed", point_index, _pack_spin_values(back_spins), p_enabled);
}

void SplinePointField::_on_back_spin_changed(double) {
    if (is_updating_ui) return;
    const bool enabled = use_back_check ? use_back_check->is_pressed() : true;
    emit_signal("back_handle_changed", point_index, _pack_spin_values(back_spins), enabled);
}

void SplinePointField::_on_fore_handle_toggled(bool p_enabled) {
    if (is_updating_ui) return;
    for (int i = 0; i < MAX_DIMENSIONS; ++i) {
        if (fore_spins[i]) fore_spins[i]->set_editable(p_enabled);
    }
    set_handles_expanded(handles_expanded);
    emit_signal("fore_handle_changed", point_index, _pack_spin_values(fore_spins), p_enabled);
}

void SplinePointField::_on_fore_spin_changed(double) {
    if (is_updating_ui) return;
    const bool enabled = use_fore_check ? use_fore_check->is_pressed() : true;
    emit_signal("fore_handle_changed", point_index, _pack_spin_values(fore_spins), enabled);
}

} // namespace ideam::godot_ext