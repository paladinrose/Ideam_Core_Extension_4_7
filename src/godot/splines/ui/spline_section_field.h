#pragma once

/**
 * spline_section_field.h
 * Reusable runtime & editor UI row representing a single Spline Section (0 .. M-1).
 *
 * Because section metadata (`curve_type`, `weight`, and cached `length`) is 100% identical
 * across 1D, 2D, 3D, and 4D splines, this single control is shared by all dimensional
 * SplineControls (`Spline1DControl` .. `Spline4DControl`).
 */

#include "../spline_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

namespace ideam::godot_ext {

class SplineSectionField : public godot::PanelContainer {
    GDCLASS(SplineSectionField, godot::PanelContainer)

private:
    int section_index    = 0;
    int from_point_index = 0;
    int to_point_index   = 1;
    bool is_wrap_section = false;
    bool selected        = false;
    bool is_updating_ui  = false;

    godot::Button *select_badge_btn     = nullptr;
    godot::OptionButton *curve_dropdown = nullptr;
    godot::SpinBox *weight_spin         = nullptr;
    godot::Label *length_label          = nullptr;
    godot::Button *split_btn            = nullptr;

    godot::Ref<godot::StyleBoxFlat> normal_style;
    godot::Ref<godot::StyleBoxFlat> selected_style;

    void _build_ui();
    void _apply_selection_style();

    void _on_select_badge_pressed();
    void _on_curve_type_selected(int p_item_idx);
    void _on_weight_value_changed(double p_value);
    void _on_split_pressed();

protected:
    static void _bind_methods();

public:
    SplineSectionField();
    ~SplineSectionField() override;

    void configure_section(
        int p_section_index,
        int p_from_point_index,
        int p_to_point_index,
        bool p_is_wrap_section,
        SplineCurveType p_curve_type,
        real_t p_weight,
        real_t p_length
    );

    void set_section_index(int p_index);
    [[nodiscard]] int get_section_index() const noexcept { return section_index; }

    void set_selected(bool p_selected);
    [[nodiscard]] bool is_selected() const noexcept { return selected; }
};

} // namespace ideam::godot_ext