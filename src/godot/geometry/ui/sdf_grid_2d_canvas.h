#pragma once

/**
 * sdf_grid_2d_canvas.h
 * Interactive 2D Signed Distance Field & Chladni Standing-Wave Canvas (`SDFGrid2DCanvas`, Tier 2 Specific UI)
 * and Composite Authoring Control (`SDFGrid2DControl`, Tier 3 UI Tool) in `src/godot/geometry/ui/`.
 *
 * Implements Section 3.2 of the Ideam Core Extension Master Plan:
 *   - Extends `GeometryCanvas2DBase` AND bridges directly with `HeatmapGrid` (`src/godot/controls/heatmap_grid.h`)
 *     to visualize 2D signed distance fields, scrub iso-contour thresholds (`iso_value`), preview
 *     Marching Squares contour extraction (`extract_iso_contour_segments`), render gradient vector
 *     arrows (`sample_gradient`), and test boolean CSG operations (`smooth_union`, `smooth_subtract`,
 *     `smooth_intersect`) interactively with `IdeamUndoRedo`.
 *   - Provides a built-in Chladni modal standing-wave evaluator (`evaluate_chladni_field`) to bridge
 *     `IdeamGeometry` directly with `IdeamCymatics` (`CymaticMap`).
 */

#include "geometry_canvas_2d_base.h"
#include "../resources/sdf_grid_2d_resource.h"
#include "../../controls/heatmap_grid.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

enum SDFCanvasBrushTool : int32_t {
    SDF_TOOL_PROBE_INSPECT = 0, // Hover/click to probe exact signed distance & gradient
    SDF_TOOL_STAMP_CIRCLE  = 1, // Click/drag to stamp circle primitive with active combine mode
    SDF_TOOL_STAMP_BOX     = 2, // Click/drag to stamp box primitive with active combine mode
    SDF_TOOL_STAMP_SEGMENT = 3, // Drag from A to B to stamp capsule segment with active combine mode
    SDF_TOOL_CARVE_ERASE   = 4  // Click/drag to smooth-subtract circle brush
};

class SDFGrid2DCanvas : public GeometryCanvas2DBase {
    GDCLASS(SDFGrid2DCanvas, GeometryCanvas2DBase)

private:
    SDFCanvasBrushTool brush_tool_         = SDF_TOOL_STAMP_CIRCLE;
    GeometrySDFCombineMode combine_mode_   = SDF_COMBINE_SMOOTH_UNION;
    float brush_radius_                    = 1.25f;
    float brush_smooth_k_                  = 0.45f;

    bool show_distance_field_ = true;
    bool show_distance_rings_ = true;
    bool show_iso_contours_   = true;
    bool show_gradients_      = false;

    bool has_probe_point_             = false;
    godot::Vector2 probe_world_pos_   = godot::Vector2();
    godot::Vector2 segment_drag_start_world_ = godot::Vector2();

    void _apply_brush_at_world(const godot::Vector2 &p_world_pos, bool p_is_segment_drag = false);
    [[nodiscard]] godot::Color _evaluate_sdf_color(float p_d, float p_iso) const noexcept;

protected:
    static void _bind_methods();

    [[nodiscard]] int _get_canvas_point_count() const override { return 0; }
    [[nodiscard]] bool _has_non_point_content() const override;

    [[nodiscard]] godot::Vector2 _get_point_world_pos(int p_index) const override {
        (void)p_index;
        return godot::Vector2();
    }
    void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) override {
        (void)p_index;
        (void)p_world_pos;
    }

    [[nodiscard]] godot::PackedVector2Array _sample_curve_screen_points(
        const godot::Rect2 &p_plot
    ) const override;
    [[nodiscard]] bool _compute_world_bounds(godot::Rect2 &r_bounds) const override;

    [[nodiscard]] bool _try_begin_custom_drag(
        const godot::Ref<godot::InputEvent> &p_event,
        const godot::Rect2 &p_plot
    ) override;
    [[nodiscard]] bool _update_custom_drag(
        const godot::Ref<godot::InputEvent> &p_event,
        const godot::Rect2 &p_plot
    ) override;
    void _update_custom_hover(
        const godot::Vector2 &p_mouse_pos,
        const godot::Rect2 &p_plot
    ) override;

    void _draw_plot_background_overlay(const godot::Rect2 &p_plot) override;
    void _draw_plot_foreground_overlay(const godot::Rect2 &p_plot) override;

public:
    SDFGrid2DCanvas();
    ~SDFGrid2DCanvas() override = default;

    void set_sdf_grid_resource(const godot::Ref<SDFGrid2DResource> &p_sdf);
    [[nodiscard]] godot::Ref<SDFGrid2DResource> get_sdf_grid_resource() const;

    void set_brush_tool(SDFCanvasBrushTool p_tool);
    [[nodiscard]] SDFCanvasBrushTool get_brush_tool() const noexcept { return brush_tool_; }

    void set_combine_mode(GeometrySDFCombineMode p_mode);
    [[nodiscard]] GeometrySDFCombineMode get_combine_mode() const noexcept { return combine_mode_; }

    void set_brush_radius(float p_radius);
    [[nodiscard]] float get_brush_radius() const noexcept { return brush_radius_; }

    void set_brush_smooth_k(float p_smooth_k);
    [[nodiscard]] float get_brush_smooth_k() const noexcept { return brush_smooth_k_; }

    void set_show_distance_field(bool p_show);
    [[nodiscard]] bool get_show_distance_field() const noexcept { return show_distance_field_; }

    void set_show_distance_rings(bool p_show);
    [[nodiscard]] bool get_show_distance_rings() const noexcept { return show_distance_rings_; }

    void set_show_iso_contours(bool p_show);
    [[nodiscard]] bool get_show_iso_contours() const noexcept { return show_iso_contours_; }

    void set_show_gradients(bool p_show);
    [[nodiscard]] bool get_show_gradients() const noexcept { return show_gradients_; }

    [[nodiscard]] godot::Vector2 get_probe_world_pos() const noexcept { return probe_world_pos_; }
};

/**
 * SDFGrid2DControl (Tier 3 UI Tool)
 * Composite Editor & Runtime Control wrapping `SDFGrid2DCanvas` AND an optional collapsible
 * `HeatmapGrid` companion view, with CSG brush controls, Marching Squares iso-contour scrubbing,
 * and a Chladni standing-wave modal synthesizer (`IdeamCymatics` bridge).
 */
class SDFGrid2DControl : public godot::VBoxContainer {
    GDCLASS(SDFGrid2DControl, godot::VBoxContainer)

private:
    godot::Ref<SDFGrid2DResource> sdf_resource_;
    bool is_updating_ui_ = false;

    // 1. Top Overlay & Iso-Contour Threshold Bar
    godot::CheckBox *field_check_        = nullptr;
    godot::CheckBox *rings_check_        = nullptr;
    godot::CheckBox *contours_check_     = nullptr;
    godot::CheckBox *gradients_check_    = nullptr;
    godot::CheckBox *heatmap_view_check_ = nullptr;
    godot::HSlider *iso_slider_          = nullptr;
    godot::SpinBox *iso_spin_            = nullptr;

    // 2. Embedded 2D SDF Canvas + Companion HeatmapGrid
    SDFGrid2DCanvas *sdf_canvas_ = nullptr;
    HeatmapGrid *heatmap_grid_   = nullptr;

    // 3. Interactive CSG Brush & Smooth Boolean Toolbar
    godot::OptionButton *brush_tool_opt_   = nullptr;
    godot::OptionButton *combine_mode_opt_ = nullptr;
    godot::SpinBox *brush_radius_spin_     = nullptr;
    godot::SpinBox *smooth_k_spin_         = nullptr;

    // 4. Grid Resolution & Chladni Modal Standing-Wave Bar
    godot::SpinBox *res_x_spin_     = nullptr;
    godot::SpinBox *res_y_spin_     = nullptr;
    godot::SpinBox *chladni_n_spin_ = nullptr;
    godot::SpinBox *chladni_m_spin_ = nullptr;
    godot::SpinBox *chladni_a_spin_ = nullptr;
    godot::SpinBox *chladni_b_spin_ = nullptr;

    // 5. Telemetry & Probe Footer
    godot::Label *telemetry_label_ = nullptr;

    void _build_ui();
    void _sync_heatmap_and_telemetry();

    // --- Callbacks ---
    void _on_resource_changed();
    void _on_fit_view_pressed();
    void _on_field_toggled(bool p_show);
    void _on_rings_toggled(bool p_show);
    void _on_contours_toggled(bool p_show);
    void _on_gradients_toggled(bool p_show);
    void _on_heatmap_view_toggled(bool p_show);
    void _on_iso_value_changed(double p_val);

    void _on_brush_tool_selected(int p_idx);
    void _on_combine_mode_selected(int p_idx);
    void _on_brush_params_changed(double p_unused);
    void _on_stamp_preset_ring_pressed();
    void _on_clear_field_pressed();

    void _on_apply_resolution_pressed();
    void _on_evaluate_chladni_pressed();
    void _on_canvas_probed(const godot::Vector2 &p_world_pos, double p_distance, const godot::Vector2 &p_gradient);

protected:
    static void _bind_methods();

public:
    SDFGrid2DControl();
    ~SDFGrid2DControl() override;

    void set_sdf_grid_resource(const godot::Ref<SDFGrid2DResource> &p_sdf);
    [[nodiscard]] godot::Ref<SDFGrid2DResource> get_sdf_grid_resource() const { return sdf_resource_; }

    [[nodiscard]] SDFGrid2DCanvas *get_sdf_canvas() const noexcept { return sdf_canvas_; }
    [[nodiscard]] HeatmapGrid *get_heatmap_grid() const noexcept { return heatmap_grid_; }

    void refresh_ui();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SDFCanvasBrushTool);