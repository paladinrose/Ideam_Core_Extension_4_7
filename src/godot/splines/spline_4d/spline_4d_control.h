#pragma once

/**
 * spline_4d_control.h
 * 4D W-Channel & Color Gradient Visual Canvas (`Spline4DWChannelCanvas`, extending `SplineCanvasBase`)
 * and Composite 4D Resource Editor (`Spline4DControl`, extending `SplineControlBase`).
 *
 * Design Highlights:
 *   1. `Spline4DControl` sets `_get_expected_dimension() == 4`, so `SplineControlBase`
 *      automatically configures every `SplinePointField` in the alternating Point/Section
 *      chain with four color-coded `SpinBox` fields (`X:`, `Y:`, `Z:`, `W:`) for the
 *      4D anchor and its collapsible 4D Back/Fore tangent handles.
 *   2. Includes a collapsible **`Spline4DWChannelCanvas`** (built on our generalized
 *      `SplineCanvasBase`!) that lets the user:
 *        - Visually edit the 4th component (`W`, e.g., scale, banking roll, or alpha)
 *          or any chosen channel (`X=R`, `Y=G`, `Z=B`, `W=A`) using graphic-equalizer faders.
 *        - Preview the evaluated 4D RGBA / HSVA **Color Gradient Strip** along the bottom
 *          of the canvas when editing a 4D color spline!
 *   3. Provides 4D presets (`Tapered Line`, `Banked Circle`, `Color Gradient`), 3D XYZ
 *      Euler rotations (`↻ Y 45°`, `↻ X 45°`) that preserve `W`, live `3D AABB + W Range`
 *      telemetry, and bidirectional **4D Gizmo Selection Hooks** (`set_bound_gizmo_node`).
 */

#include "../ui/spline_canvas_base.h"
#include "../ui/spline_control_base.h"
#include "spline_4d_resource.h"
#include "spline_node4d.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/core/object_id.hpp>

#include <vector>

namespace ideam::godot_ext {

/**
 * Visualizes and edits a selected channel (`W` by default, or `X`/`Y`/`Z`) of a `Spline4DResource`
 * over normalized cumulative section weights, with a live RGBA/HSVA color gradient strip at the bottom.
 */
class Spline4DWChannelCanvas : public SplineCanvasBase {
    GDCLASS(Spline4DWChannelCanvas, SplineCanvasBase)

private:
    int active_channel         = 3; // 0 = X (R/H), 1 = Y (G/S), 2 = Z (B/V), 3 = W (Scale/Roll/A)
    bool show_color_gradient   = true;
    bool hsva_gradient_preview = false;

    [[nodiscard]] godot::Rect2 _get_gradient_strip_rect(const godot::Rect2 &p_plot) const;
    [[nodiscard]] std::vector<real_t> _compute_fader_norms() const;
    [[nodiscard]] float _extract_channel(const godot::Vector4 &p_v) const noexcept;
    void _inject_channel(godot::Vector4 &r_v, float p_val) const noexcept;

protected:
    static void _bind_methods();

    // --- SplineCanvasBase Overrides ---
    [[nodiscard]] godot::Vector2 _get_point_world_pos(int p_index) const override;
    void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) override;
    [[nodiscard]] bool _has_wrap_ghost_endpoint(godot::Vector2 &r_ghost_world_pos) const override;

    [[nodiscard]] godot::Vector2 _get_back_handle_screen_pos(
        int p_index,
        const godot::Rect2 &p_plot
    ) const override;
    void _set_back_handle_from_screen_pos(
        int p_index,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) override;

    [[nodiscard]] godot::Vector2 _get_fore_handle_screen_pos(
        int p_index,
        const godot::Rect2 &p_plot
    ) const override;
    void _set_fore_handle_from_screen_pos(
        int p_index,
        const godot::Vector2 &p_screen_pos,
        const godot::Rect2 &p_plot
    ) override;

    [[nodiscard]] godot::PackedVector2Array _sample_curve_screen_points(
        const godot::Rect2 &p_plot
    ) const override;
    [[nodiscard]] bool _compute_world_bounds(godot::Rect2 &r_bounds) const override;

    void _draw_plot_foreground_overlay(const godot::Rect2 &p_plot) override;

public:
    Spline4DWChannelCanvas();
    ~Spline4DWChannelCanvas() override;

    void set_spline(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_spline() const;

    void set_active_channel(int p_channel);
    [[nodiscard]] int get_active_channel() const noexcept { return active_channel; }

    void set_show_color_gradient(bool p_show);
    [[nodiscard]] bool get_show_color_gradient() const noexcept { return show_color_gradient; }

    void set_hsva_gradient_preview(bool p_hsva);
    [[nodiscard]] bool get_hsva_gradient_preview() const noexcept { return hsva_gradient_preview; }
};

/**
 * Full Editor & Runtime composite UI control for `Spline4DResource`.
 */
class Spline4DControl : public SplineControlBase {
    GDCLASS(Spline4DControl, SplineControlBase)

private:
    Spline4DWChannelCanvas *w_canvas       = nullptr;
    godot::SpinBox *preset_points_spin     = nullptr;
    godot::OptionButton *channel_select    = nullptr;
    godot::CheckBox *show_w_canvas_check   = nullptr;
    godot::CheckBox *hsva_preview_check    = nullptr;
    godot::Label *bounds_telemetry_label   = nullptr;

    godot::ObjectID bound_gizmo_node_id;

    void _build_4d_extensions();
    [[nodiscard]] SplineNode4D *_get_bound_gizmo_node() const;

    // 4D Preset & Toolbar Callbacks
    void _on_preset_tapered_line();
    void _on_preset_banked_circle();
    void _on_preset_color_gradient();
    void _on_rotate_y_45_pressed();
    void _on_rotate_x_45_pressed();
    void _on_channel_selected(int p_item_idx);
    void _on_fit_w_pressed();
    void _on_hsva_preview_toggled(bool p_hsva);
    void _on_show_w_canvas_toggled(bool p_show);
    void _on_canvas_point_selected(int p_point_index);
    void _on_canvas_section_selected(int p_section_index);

    // 4D Gizmo Sync Callbacks
    void _on_gizmo_node_point_selected(int p_point_index);
    void _on_gizmo_node_section_selected(int p_section_index);

protected:
    static void _bind_methods();

    // --- SplineControlBase Overrides ---
    [[nodiscard]] int _get_expected_dimension() const override { return 4; }
    void _on_resource_rebound() override;
    void _on_ui_refreshed() override;
    void _on_selection_updated() override;

    void _populate_point_field(int p_index, SplinePointField *p_field) override;
    void _commit_point_position(int p_index, const godot::Variant &p_position) override;
    void _commit_point_back_handle(int p_index, const godot::Variant &p_handle, bool p_enabled) override;
    void _commit_point_fore_handle(int p_index, const godot::Variant &p_handle, bool p_enabled) override;
    void _commit_add_default_point() override;
    void _commit_split_section(int p_section_index) override;

public:
    Spline4DControl();
    ~Spline4DControl() override;

    void set_spline_4d(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_spline_4d() const;

    [[nodiscard]] Spline4DWChannelCanvas *get_w_channel_canvas() const noexcept { return w_canvas; }

    void set_bound_gizmo_node(SplineNode4D *p_node);
    [[nodiscard]] SplineNode4D *get_bound_gizmo_node() const { return _get_bound_gizmo_node(); }
};

using Spline4DResourceControl = Spline4DControl;

} // namespace ideam::godot_ext