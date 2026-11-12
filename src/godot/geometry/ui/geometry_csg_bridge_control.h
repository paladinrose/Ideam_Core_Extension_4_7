#pragma once

/**
 * geometry_csg_bridge_control.h
 * Cross-Plugin 2D CSG, Wave-2 Boundary Classification, Topological Seam Welding, and
 * Multi-Resource Bridge Workbench (`GeometryCSGPreviewCanvas` & `GeometryCSGBridgeControl`)
 * in `src/godot/geometry/ui/`.
 *
 * Designed specifically for cross-plugin bridging across `IdeamGeometry`, `IdeamTiles`,
 * `IdeamSplines`, `IdeamGames` (`TiledGameBoardArchitectSmartTool`), `IdeamNarratives`
 * (`SpatialNarrativeTerritorySmartTool`), and `IdeamDesign` (Aperture CSG cutouts):
 *   1. Visualizes `CSGMath2D::classify_polygon_against_boundary` & `classify_polygon_half_plane`
 *      (`FULLY_INSIDE` vs. `STRADDLING_BOUNDARY` vs. `FULLY_OUTSIDE`) and exact edge-edge
 *      intersections (`find_polygon_intersections`).
 *   2. Interactive draggable Half-Plane / Seam Line handle (`plane_origin`, `plane_angle_deg`)
 *      with live preview of Sutherland-Hodgman `clip_polygon_half_plane` and `clip_polygon_convex`.
 *   3. Previews watertight Topological Seam Welding (`CSGMath2D::weld_polygons_along_seam`)
 *      across left/right adjacent polygons with configurable `weld_tolerance`.
 *   4. One-click Cross-Resource Bridge actions (`Commit Clipped -> Subject Polygon2D`,
 *      `Bind Boundary -> VoronoiDiagram2DResource`, `Stamp Subject -> SDFGrid2DResource`).
 */

#include "geometry_canvas_2d_base.h"
#include "../resources/polygon_2d_resource.h"
#include "../resources/voronoi_diagram_2d_resource.h"
#include "../resources/sdf_grid_2d_resource.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_box.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_box_container.hpp>

namespace ideam::godot_ext {

enum GeometryCSGPreviewMode : int32_t {
    CSG_PREVIEW_CONVEX_CLIP       = 0, // Sutherland-Hodgman Subject A clipped against Boundary B
    CSG_PREVIEW_HALF_PLANE_CLIP   = 1, // Subject A clipped against interactive Half-Plane
    CSG_PREVIEW_SEAM_WELD_SPLIT   = 2, // Left/Right split & watertight seam weld along seam polyline
    CSG_PREVIEW_WAVE2_CLASSIFY    = 3  // TileDOD Wave 2 classification + intersection event markers
};

class GeometryCSGPreviewCanvas : public GeometryCanvas2DBase {
    GDCLASS(GeometryCSGPreviewCanvas, GeometryCanvas2DBase)

private:
    godot::Ref<Polygon2DResource> boundary_polygon_;
    GeometryCSGPreviewMode preview_mode_ = CSG_PREVIEW_CONVEX_CLIP;

    godot::Vector2 half_plane_origin_ = godot::Vector2(0.0f, 0.0f);
    float half_plane_angle_deg_       = 25.0f;
    float weld_tolerance_             = 0.02f;
    bool show_intersections_          = true;

    void _on_boundary_changed();

protected:
    static void _bind_methods();

    [[nodiscard]] int _get_canvas_point_count() const override;
    [[nodiscard]] int _get_canvas_section_count() const override;
    [[nodiscard]] bool _is_canvas_wrapped() const override;
    [[nodiscard]] bool _has_non_point_content() const override;

    [[nodiscard]] godot::Vector2 _get_point_world_pos(int p_index) const override;
    void _set_point_world_pos(int p_index, const godot::Vector2 &p_world_pos) override;

    [[nodiscard]] godot::PackedVector2Array _sample_curve_screen_points(
        const godot::Rect2 &p_plot
    ) const override;

    void _draw_plot_background_overlay(const godot::Rect2 &p_plot) override;
    void _draw_plot_foreground_overlay(const godot::Rect2 &p_plot) override;

public:
    GeometryCSGPreviewCanvas();
    ~GeometryCSGPreviewCanvas() override;

    void set_subject_polygon(const godot::Ref<Polygon2DResource> &p_subject);
    [[nodiscard]] godot::Ref<Polygon2DResource> get_subject_polygon() const;

    void set_boundary_polygon(const godot::Ref<Polygon2DResource> &p_boundary);
    [[nodiscard]] godot::Ref<Polygon2DResource> get_boundary_polygon() const { return boundary_polygon_; }

    void set_preview_mode(GeometryCSGPreviewMode p_mode);
    [[nodiscard]] GeometryCSGPreviewMode get_preview_mode() const noexcept { return preview_mode_; }

    void set_half_plane_origin(const godot::Vector2 &p_origin);
    [[nodiscard]] godot::Vector2 get_half_plane_origin() const noexcept { return half_plane_origin_; }

    void set_half_plane_angle_deg(float p_angle_deg);
    [[nodiscard]] float get_half_plane_angle_deg() const noexcept { return half_plane_angle_deg_; }

    [[nodiscard]] godot::Vector2 get_half_plane_normal() const noexcept;

    void set_weld_tolerance(float p_tol);
    [[nodiscard]] float get_weld_tolerance() const noexcept { return weld_tolerance_; }

    void set_show_intersections(bool p_show);
    [[nodiscard]] bool get_show_intersections() const noexcept { return show_intersections_; }
};

/**
 * GeometryCSGBridgeControl (Tier 3/4 Cross-Plugin Geometry Workbench)
 * Provides interactive CSG clipping, Wave-2 boundary classification, seam welding, and
 * 1-click bridging between `Polygon2DResource`, `VoronoiDiagram2DResource`, and `SDFGrid2DResource`.
 */
class GeometryCSGBridgeControl : public godot::VBoxContainer {
    GDCLASS(GeometryCSGBridgeControl, godot::VBoxContainer)

private:
    godot::Ref<Polygon2DResource> subject_polygon_;
    godot::Ref<Polygon2DResource> boundary_polygon_;
    godot::Ref<VoronoiDiagram2DResource> target_voronoi_;
    godot::Ref<SDFGrid2DResource> target_sdf_grid_;
    bool is_updating_ui_ = false;

    // 1. Mode & Overlay Toolbar
    godot::OptionButton *mode_opt_          = nullptr;
    godot::CheckBox *intersections_check_   = nullptr;
    godot::SpinBox *plane_x_spin_           = nullptr;
    godot::SpinBox *plane_y_spin_           = nullptr;
    godot::SpinBox *plane_angle_spin_       = nullptr;
    godot::SpinBox *weld_tol_spin_          = nullptr;

    // 2. Embedded CSG Preview Canvas
    GeometryCSGPreviewCanvas *csg_canvas_ = nullptr;

    // 3. Telemetry & Classification Status Label
    godot::Label *classification_label_ = nullptr;

    void _build_ui();
    void _ensure_default_operands();
    void _update_classification_telemetry();

    // --- Callbacks ---
    void _on_operand_changed();
    void _on_mode_selected(int p_idx);
    void _on_intersections_toggled(bool p_show);
    void _on_plane_params_changed(double p_unused);
    void _on_fit_view_pressed();

    void _on_commit_clip_to_subject_pressed();
    void _on_swap_subject_and_boundary_pressed();
    void _on_bridge_to_voronoi_clip_pressed();
    void _on_bridge_stamp_to_sdf_pressed();

protected:
    static void _bind_methods();

public:
    GeometryCSGBridgeControl();
    ~GeometryCSGBridgeControl() override;

    void set_subject_polygon(const godot::Ref<Polygon2DResource> &p_subject);
    [[nodiscard]] godot::Ref<Polygon2DResource> get_subject_polygon() const { return subject_polygon_; }

    void set_boundary_polygon(const godot::Ref<Polygon2DResource> &p_boundary);
    [[nodiscard]] godot::Ref<Polygon2DResource> get_boundary_polygon() const { return boundary_polygon_; }

    void set_target_voronoi(const godot::Ref<VoronoiDiagram2DResource> &p_voronoi);
    [[nodiscard]] godot::Ref<VoronoiDiagram2DResource> get_target_voronoi() const { return target_voronoi_; }

    void set_target_sdf_grid(const godot::Ref<SDFGrid2DResource> &p_sdf);
    [[nodiscard]] godot::Ref<SDFGrid2DResource> get_target_sdf_grid() const { return target_sdf_grid_; }

    [[nodiscard]] GeometryCSGPreviewCanvas *get_csg_canvas() const noexcept { return csg_canvas_; }

    void refresh_ui();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::GeometryCSGPreviewMode);