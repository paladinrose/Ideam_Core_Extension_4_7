#pragma once

/**
 * spline_control_base.h
 * Shared editor & runtime base UI control (`VBoxContainer`) for inspecting and editing
 * any `SplineResource` (1D, 2D, 3D, or 4D).
 *
 * Architectural Highlights:
 *   1. Identical execution in the Godot Editor Inspector and in-game runtime UIs.
 *   2. Unified top toolbar for `wrap`, `default_curve_type`, `default_weight`,
 *      `+ Add Point`, `Clear`, and a `SplineListViewMode` filter that lets the user switch
 *      seamlessly between:
 *        - `LIST_VIEW_INTERLEAVED`: `Point[0] -> Section[0] -> Point[1] -> Section[1] -> ...`
 *        - `LIST_VIEW_POINTS_ONLY`: `Point[0], Point[1], ...`
 *        - `LIST_VIEW_SECTIONS_ONLY`: `Section[0], Section[1], ...`
 *   3. Dedicated `custom_header_slot` and `visual_editor_slot` containers so dimensional
 *      subclasses (`Spline1DControl`, `Spline2DControl`) can mount preset buttons and
 *      embedded visual curve canvases (`Spline1DEqualizerCanvas`, `Spline2DCanvas`)
 *      while keeping those visual canvases decoupled and reusable on their own.
 *   4. Incremental in-place row updates: when dragging spinboxes or visual handles without
 *      changing point/section counts, existing `SplinePointField` and `SplineSectionField`
 *      instances are updated in-place so focus and scroll state are never lost.
 */

#include "../spline_resource.h"
#include "spline_point_field.h"
#include "spline_section_field.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

#include <vector>

namespace ideam::godot_ext {

enum SplineListViewMode : int32_t {
    LIST_VIEW_INTERLEAVED   = 0,
    LIST_VIEW_POINTS_ONLY   = 1,
    LIST_VIEW_SECTIONS_ONLY = 2
};

class SplineControlBase : public godot::VBoxContainer {
    GDCLASS(SplineControlBase, godot::VBoxContainer)

protected:
    static void _bind_methods();

    godot::Ref<SplineResource> spline_resource;

    SplineListViewMode list_view_mode = LIST_VIEW_INTERLEAVED;
    int selected_point_index          = -1;
    int selected_section_index        = -1;
    bool is_updating_ui               = false;

    // Built-in structural containers
    godot::HBoxContainer *global_toolbar      = nullptr;
    godot::VBoxContainer *custom_header_slot  = nullptr;
    godot::VBoxContainer *visual_editor_slot  = nullptr;
    godot::HBoxContainer *telemetry_bar       = nullptr;
    godot::ScrollContainer *chain_scroll      = nullptr;
    godot::VBoxContainer *chain_list_vbox     = nullptr;

    // Toolbar widgets
    godot::CheckBox *wrap_check               = nullptr;
    godot::OptionButton *default_curve_opt    = nullptr;
    godot::SpinBox *default_weight_spin       = nullptr;
    godot::OptionButton *view_mode_opt        = nullptr;
    godot::Button *add_point_btn              = nullptr;
    godot::Button *clear_btn                  = nullptr;
    godot::Label *telemetry_label             = nullptr;

    // Active row pools (indexed by point 0..N-1 and section 0..M-1)
    std::vector<SplinePointField *> point_fields;
    std::vector<SplineSectionField *> section_fields;
    int last_built_point_count                = -1;
    int last_built_section_count              = -1;
    SplineListViewMode last_built_view_mode   = LIST_VIEW_INTERLEAVED;

    void _build_base_ui();
    void _rebuild_chain_rows(int p_point_count, int p_section_count);
    void _clear_chain_rows();

    // --- Internal Signal Callbacks ---
    void _on_resource_changed();
    void _on_wrap_toggled(bool p_wrap);
    void _on_default_curve_selected(int p_item_idx);
    void _on_default_weight_changed(double p_weight);
    void _on_view_mode_selected(int p_item_idx);
    void _on_add_point_pressed();
    void _on_clear_pressed();

    // Row callbacks
    void _on_row_point_selected(int p_point_index);
    void _on_row_point_position_changed(int p_point_index, const godot::Variant &p_pos);
    void _on_row_point_back_handle_changed(int p_point_index, const godot::Variant &p_handle, bool p_enabled);
    void _on_row_point_fore_handle_changed(int p_point_index, const godot::Variant &p_handle, bool p_enabled);
    void _on_row_remove_point_requested(int p_point_index);

    void _on_row_section_selected(int p_section_index);
    void _on_row_section_curve_changed(int p_section_index, int p_curve_type);
    void _on_row_section_weight_changed(int p_section_index, double p_weight);
    void _on_row_split_section_requested(int p_section_index);

    // ========================================================================
    // Dimensional Customization Hooks (Overridden by Spline1DControl..4DControl)
    // ========================================================================

    [[nodiscard]] virtual int _get_expected_dimension() const { return 1; }
    virtual void _on_resource_rebound() {}
    virtual void _on_ui_refreshed() {}
    virtual void _on_selection_updated() {}

    virtual void _populate_point_field(int p_index, SplinePointField *p_field);
    virtual void _commit_point_position(int p_index, const godot::Variant &p_position) {}
    virtual void _commit_point_back_handle(int p_index, const godot::Variant &p_handle, bool p_enabled) {}
    virtual void _commit_point_fore_handle(int p_index, const godot::Variant &p_handle, bool p_enabled) {}
    virtual void _commit_add_default_point() {}
    virtual void _commit_split_section(int p_section_index) {}

public:
    SplineControlBase();
    ~SplineControlBase() override;

    // --- Resource Binding ---
    void set_spline_resource(const godot::Ref<SplineResource> &p_resource);
    [[nodiscard]] godot::Ref<SplineResource> get_spline_resource() const { return spline_resource; }

    // --- View Mode & Layout Configuration ---
    void set_list_view_mode(SplineListViewMode p_mode);
    [[nodiscard]] SplineListViewMode get_list_view_mode() const noexcept { return list_view_mode; }

    void set_minimum_list_height(float p_height);
    [[nodiscard]] float get_minimum_list_height() const;

    // --- Selection Sync (Between Visual Canvas and Point/Section List) ---
    void select_point(int p_point_index);
    [[nodiscard]] int get_selected_point_index() const noexcept { return selected_point_index; }

    void select_section(int p_section_index);
    [[nodiscard]] int get_selected_section_index() const noexcept { return selected_section_index; }

    void refresh_ui();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplineListViewMode);