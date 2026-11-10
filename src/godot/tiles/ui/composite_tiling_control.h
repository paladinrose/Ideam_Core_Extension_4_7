#pragma once

/**
 * composite_tiling_control.h
 * Composite Reflective Editor & Runtime UI Control (`VBoxContainer`) for inspecting and
 * authoring a `CompositeTilingResource` (`src/godot/tiles/ui/`).
 *
 * Responsibilities:
 *   1. Provides top-level controls for `Coverage Mode` (`Strict Interior`, `Centroid Inside`,
 *      `Include Straddling`, `Clip Exact to Spline`), `Stitch Mode` (`Quad Bridge`,
 *      `Topological Seam Weld`, `Curvilinear Metric Morph`), `Max Bridge Span`,
 *      `Edge Subdivisions`, `Auto Hexize`, `Auto Rebuild`, and `Rebuild Now`.
 *   2. Embeds `TilingRegionPreviewCanvas` with color-mode selector (`3-Coloring`,
 *      `Layer Group`, `Generation Ring`, `Cell Flags`), overlay toggles (`Hex Super-Cells`,
 *      `Border Splines`, `Flow-Field Vectors`), and interactive sandbox mode selector
 *      (`Inspect Cell`, `Carve/Remove Cell`, `A* Pathfinding Sandbox`, `Cycle Edge Port`).
 *   3. Provides quick-action layer & boundary contour builders (`+ Add Wallpaper Layer`,
 *      `+ Add Boundary Spline2D Contour`) and a live Cell & Pathfinding Telemetry footer.
 */

#include "tiling_region_preview_canvas.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

class CompositeTilingControl : public godot::VBoxContainer {
    GDCLASS(CompositeTilingControl, godot::VBoxContainer)

private:
    godot::Ref<CompositeTilingResource> composite_tiling_;
    bool is_updating_ui_ = false;

    // 1. Top Coverage, Seam Stitch & Rebuild Toolbar
    godot::OptionButton *coverage_mode_opt_ = nullptr;
    godot::OptionButton *stitch_mode_opt_   = nullptr;
    godot::SpinBox *bridge_span_spin_       = nullptr;
    godot::SpinBox *edge_subdivs_spin_      = nullptr;
    godot::CheckBox *auto_hexize_check_     = nullptr;
    godot::CheckBox *auto_rebuild_check_    = nullptr;

    // 2. Canvas Viewport & Sandbox Mode Bar
    godot::OptionButton *color_mode_opt_    = nullptr;
    godot::OptionButton *interact_mode_opt_ = nullptr;
    godot::CheckBox *hex_overlay_check_     = nullptr;
    godot::CheckBox *border_overlay_check_  = nullptr;
    godot::CheckBox *flow_field_check_      = nullptr;
    godot::CheckBox *manhattan_check_       = nullptr;

    // 3. Embedded 2D Region Preview Canvas
    TilingRegionPreviewCanvas *region_canvas_ = nullptr;

    // 4. Layer Stack & Selected Cell Telemetry Footer
    godot::Label *layers_summary_label_     = nullptr;
    godot::Label *cell_inspector_label_     = nullptr;

    void _build_ui();
    void _update_cell_inspector_label(int p_cell_id);

    // --- Callbacks ---
    void _on_resource_changed();
    void _on_rebuild_pressed();
    void _on_fit_view_pressed();
    void _on_coverage_mode_selected(int p_idx);
    void _on_stitch_mode_selected(int p_idx);
    void _on_bridge_span_changed(double p_val);
    void _on_edge_subdivs_changed(double p_val);
    void _on_auto_hexize_toggled(bool p_enabled);
    void _on_auto_rebuild_toggled(bool p_enabled);

    void _on_color_mode_selected(int p_idx);
    void _on_interact_mode_selected(int p_idx);
    void _on_hex_overlay_toggled(bool p_show);
    void _on_border_overlay_toggled(bool p_show);
    void _on_flow_field_toggled(bool p_show);
    void _on_manhattan_toggled(bool p_manhattan);

    void _on_add_layer_pressed();
    void _on_add_boundary_spline_pressed();
    void _on_canvas_cell_selected(int p_cell_id);
    void _on_canvas_path_updated(int p_start_cell, int p_goal_cell, int p_path_len);

protected:
    static void _bind_methods();

public:
    CompositeTilingControl();
    ~CompositeTilingControl() override;

    void set_composite_tiling(const godot::Ref<CompositeTilingResource> &p_composite);
    [[nodiscard]] godot::Ref<CompositeTilingResource> get_composite_tiling() const { return composite_tiling_; }

    [[nodiscard]] TilingRegionPreviewCanvas *get_region_canvas() const noexcept { return region_canvas_; }

    void refresh_ui();
};

} // namespace ideam::godot_ext
