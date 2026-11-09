#pragma once

/**
 * spline_point_field.h
 * Dimension-adaptive (1D float, 2D Vector2, 3D Vector3, 4D Vector4) UI control
 * for inspecting and editing a single Spline Anchor Point and its Back/Fore Handles.
 *
 * Uses runtime-safe `godot::SpinBox` fields (rather than editor-exclusive `EditorSpinSlider`)
 * so the exact same control functions in both the Godot Editor Inspector and in-game UIs.
 */

#include "../spline_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <array>

namespace ideam::godot_ext {

class SplinePointField : public godot::PanelContainer {
    GDCLASS(SplinePointField, godot::PanelContainer)

public:
    static constexpr int MAX_DIMENSIONS = 4;

private:
    int point_index       = 0;
    int dimension         = 1;
    bool selected         = false;
    bool handles_expanded = false;
    bool is_updating_ui   = false;

    godot::Button *select_badge_btn   = nullptr;
    godot::Button *handles_toggle_btn = nullptr;
    godot::Button *delete_btn         = nullptr;

    godot::HBoxContainer *pos_axes_box       = nullptr;
    godot::VBoxContainer *handles_drawer     = nullptr;
    godot::CheckBox *use_back_check          = nullptr;
    godot::CheckBox *use_fore_check          = nullptr;
    godot::HBoxContainer *back_axes_box      = nullptr;
    godot::HBoxContainer *fore_axes_box      = nullptr;

    std::array<godot::Label *, MAX_DIMENSIONS> pos_axis_labels  = { nullptr, nullptr, nullptr, nullptr };
    std::array<godot::SpinBox *, MAX_DIMENSIONS> pos_spins      = { nullptr, nullptr, nullptr, nullptr };
    std::array<godot::SpinBox *, MAX_DIMENSIONS> back_spins     = { nullptr, nullptr, nullptr, nullptr };
    std::array<godot::SpinBox *, MAX_DIMENSIONS> fore_spins     = { nullptr, nullptr, nullptr, nullptr };

    godot::Ref<godot::StyleBoxFlat> normal_style;
    godot::Ref<godot::StyleBoxFlat> selected_style;

    void _build_ui();
    void _update_dimension_visibility();
    void _apply_selection_style();
    [[nodiscard]] godot::Variant _pack_spin_values(
        const std::array<godot::SpinBox *, MAX_DIMENSIONS> &p_spins
    ) const;
    void _unpack_variant_to_spins(
        const godot::Variant &p_val,
        std::array<godot::SpinBox *, MAX_DIMENSIONS> &r_spins
    );

    void _on_select_badge_pressed();
    void _on_handles_toggle_pressed();
    void _on_delete_pressed();
    void _on_pos_spin_changed(double p_unused);
    void _on_back_handle_toggled(bool p_enabled);
    void _on_back_spin_changed(double p_unused);
    void _on_fore_handle_toggled(bool p_enabled);
    void _on_fore_spin_changed(double p_unused);

protected:
    static void _bind_methods();

public:
    SplinePointField();
    ~SplinePointField() override;

    void set_dimension(int p_dimension);
    [[nodiscard]] int get_dimension() const noexcept { return dimension; }

    void set_point_index(int p_index);
    [[nodiscard]] int get_point_index() const noexcept { return point_index; }

    void set_selected(bool p_selected);
    [[nodiscard]] bool is_selected() const noexcept { return selected; }

    void set_handles_expanded(bool p_expanded);
    [[nodiscard]] bool is_handles_expanded() const noexcept { return handles_expanded; }

    // --- Polymorphic Variant Configuration (1D float, 2D Vector2, 3D Vector3, 4D Vector4) ---
    void configure_point_variant(
        int p_point_index,
        const godot::Variant &p_position,
        const godot::Variant &p_back_handle,
        bool p_use_back,
        const godot::Variant &p_fore_handle,
        bool p_use_fore,
        bool p_can_delete = true
    );

    // --- Typed Convenience Helpers ---
    void configure_point_1d(
        int p_point_index,
        real_t p_position,
        real_t p_back_handle,
        bool p_use_back,
        real_t p_fore_handle,
        bool p_use_fore,
        bool p_can_delete = true
    );
};

} // namespace ideam::godot_ext