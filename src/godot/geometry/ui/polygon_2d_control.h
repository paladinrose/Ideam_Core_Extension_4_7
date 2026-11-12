#pragma once

/**
 * polygon_2d_control.h
 * Composite Reflective Editor & Runtime UI Control (`VBoxContainer`, Tier 3 UI Tool)
 * for inspecting and authoring a `Polygon2DResource` (`src/godot/geometry/ui/`).
 *
 * Elevates `Polygon2DResource` to match `Spline2DControl` and `WallpaperTilingControl`:
 *   1. Viewport & Topology Toolbar (`Closed Loop`, `Triangulation`, `Centroid`, `Convex Hull`,
 *      `Ensure CCW / CW`, `Bake Convex Hull`, `CSG & Bridge Workbench` toggle, `Fit View`).
 *   2. Interactive `Polygon2DCanvas` with live offset contour preview (`compute_offset_polygon`).
 *   3. Parametric Shape Generator Bar (`Rectangle`, `Regular N-Gon`, `Star` with configurable
 *      sides and outer/inner radii, wired to `IdeamUndoRedo`).
 *   4. Polygon Offset / Buffer & Miter Join Bar (`Offset Delta`, `Join Style`, `Miter Limit`,
 *      `Live Preview` toggle, and `Commit Offset` button — previewing `GEO3` & `IdeamDesign` wall offsets).
 *   5. Collapsible `GeometryCSGBridgeControl` Workbench drawer for Wave-2 boundary classification,
 *      Sutherland-Hodgman clipping, topological seam welding, and Voronoi/SDF cross-plugin bridging.
 *   6. Selected Vertex Coordinate Editor & Telemetry Footer (Signed Area, Perimeter, Winding Order,
 *      Ear-Clipped Triangle Count).
 */

#include "geometry_canvas_2d_base.h"
#include "geometry_csg_bridge_control.h"
#include "../resources/polygon_2d_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

enum PolygonShapePreset : int32_t {
    POLY_PRESET_RECTANGLE = 0,
    POLY_PRESET_REGULAR   = 1,
    POLY_PRESET_STAR      = 2
};

class Polygon2DControl : public godot::VBoxContainer {
    GDCLASS(Polygon2DControl, godot::VBoxContainer)

private:
    godot::Ref<Polygon2DResource> polygon_resource_;
    bool is_updating_ui_ = false;

    // 1. Top Viewport & Winding Toolbar
    godot::CheckBox *closed_check_        = nullptr;
    godot::CheckBox *triangulation_check_ = nullptr;
    godot::CheckBox *centroid_check_      = nullptr;
    godot::CheckBox *hull_overlay_check_  = nullptr;
    godot::CheckBox *csg_workbench_check_ = nullptr;

    // 2. Embedded 2D Polygon Canvas
    Polygon2DCanvas *polygon_canvas_ = nullptr;

    // 3. Parametric Shape Generator Controls
    godot::OptionButton *preset_opt_   = nullptr;
    godot::SpinBox *sides_spin_        = nullptr;
    godot::SpinBox *outer_radius_spin_ = nullptr;
    godot::SpinBox *inner_radius_spin_ = nullptr;

    // 4. Polygon Offset / Buffer Controls
    godot::CheckBox *offset_preview_check_ = nullptr;
    godot::SpinBox *offset_delta_spin_     = nullptr;
    godot::OptionButton *join_style_opt_   = nullptr;
    godot::SpinBox *miter_limit_spin_      = nullptr;

    // 5. Collapsible Cross-Plugin CSG & Seam Bridge Workbench
    GeometryCSGBridgeControl *csg_bridge_control_ = nullptr;

    // 6. Selected Vertex & Telemetry Footer
    godot::SpinBox *sel_vertex_x_spin_ = nullptr;
    godot::SpinBox *sel_vertex_y_spin_ = nullptr;
    godot::Label *telemetry_label_     = nullptr;

    void _build_ui();
    void _sync_offset_preview();
    void _update_telemetry_label();

    // --- Callbacks ---
    void _on_resource_changed();
    void _on_closed_toggled(bool p_closed);
    void _on_triangulation_toggled(bool p_show);
    void _on_centroid_toggled(bool p_show);
    void _on_hull_overlay_toggled(bool p_show);
    void _on_csg_workbench_toggled(bool p_show);
    void _on_ensure_ccw_pressed();
    void _on_ensure_cw_pressed();
    void _on_bake_convex_hull_pressed();
    void _on_fit_view_pressed();

    void _on_add_vertex_pressed();
    void _on_remove_selected_vertex_pressed();
    void _on_clear_pressed();
    void _on_generate_shape_pressed();

    void _on_offset_preview_toggled(bool p_show);
    void _on_offset_params_changed(double p_unused);
    void _on_join_style_selected(int p_idx);
    void _on_commit_offset_pressed();

    void _on_canvas_point_selected(int p_vertex_index);
    void _on_selected_vertex_coords_edited(double p_unused);

protected:
    static void _bind_methods();

public:
    Polygon2DControl();
    ~Polygon2DControl() override;

    void set_polygon_resource(const godot::Ref<Polygon2DResource> &p_polygon);
    [[nodiscard]] godot::Ref<Polygon2DResource> get_polygon_resource() const { return polygon_resource_; }

    [[nodiscard]] Polygon2DCanvas *get_polygon_canvas() const noexcept { return polygon_canvas_; }
    [[nodiscard]] GeometryCSGBridgeControl *get_csg_bridge_control() const noexcept { return csg_bridge_control_; }

    void refresh_ui();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::PolygonShapePreset);