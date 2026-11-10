#pragma once

/**
 * spline_canvas_base.h
 * Spline-specific 2D interactive curve, fader, and vertex canvas (`src/godot/splines/ui/`),
 * inheriting the shared 2D viewport projection, pan/zoom, grid/axis rendering, hit-testing,
 * and Undo/Redo snapshot drag engine from `GeometryCanvas2DBase` (`src/godot/geometry/ui/`).
 *
 * Preserves 100% compatibility with `Spline1DEqualizerCanvas`, `Spline2DCanvas`, and
 * `Spline4DWChannelCanvas`.
 */

#include "../../geometry/ui/geometry_canvas_2d_base.h"
#include "../spline_resource.h"

namespace ideam::godot_ext {

/**
 * Constrains how anchor points move when dragged in the 2D spline canvas
 * (1:1 values with `GeometryCanvasAxisConstraint`).
 */
enum SplineCanvasAxisConstraint : int32_t {
    DRAG_AXIS_FREE_2D = static_cast<int32_t>(GEOM_DRAG_AXIS_FREE_2D),
    DRAG_AXIS_X_ONLY  = static_cast<int32_t>(GEOM_DRAG_AXIS_X_ONLY),
    DRAG_AXIS_Y_ONLY  = static_cast<int32_t>(GEOM_DRAG_AXIS_Y_ONLY)
};

/**
 * Visual rendering style for anchor points on the spline canvas
 * (1:1 values with `GeometryCanvasPointStyle`).
 */
enum SplineCanvasPointStyle : int32_t {
    POINT_STYLE_CIRCLE    = static_cast<int32_t>(GEOM_POINT_STYLE_CIRCLE),
    POINT_STYLE_SQUARE    = static_cast<int32_t>(GEOM_POINT_STYLE_SQUARE),
    POINT_STYLE_FADER_CAP = static_cast<int32_t>(GEOM_POINT_STYLE_FADER_CAP)
};

class SplineCanvasBase : public GeometryCanvas2DBase {
    GDCLASS(SplineCanvasBase, GeometryCanvas2DBase)

protected:
    static void _bind_methods();

    godot::Ref<SplineResource> spline_resource;

    // --- GeometryCanvas2DBase Overrides for SplineResource ---
    [[nodiscard]] int _get_canvas_point_count() const override;
    [[nodiscard]] int _get_canvas_section_count() const override;
    [[nodiscard]] bool _is_canvas_wrapped() const override;

    [[nodiscard]] bool _has_back_handle(int p_index) const override;
    [[nodiscard]] bool _has_fore_handle(int p_index) const override;
    void _on_right_click_point(int p_index) override;

public:
    SplineCanvasBase() = default;
    ~SplineCanvasBase() override = default;

    // --- Typed SplineResource Binding (Syncs with GeometryCanvas2DBase::geometry_resource) ---
    void set_spline_resource(const godot::Ref<SplineResource> &p_resource);
    [[nodiscard]] godot::Ref<SplineResource> get_spline_resource() const { return spline_resource; }

    // --- Spline Enum Convenience Overloads ---
    void set_anchor_drag_constraint(SplineCanvasAxisConstraint p_constraint) {
        GeometryCanvas2DBase::set_anchor_drag_constraint(
            static_cast<GeometryCanvasAxisConstraint>(p_constraint)
        );
    }
    [[nodiscard]] SplineCanvasAxisConstraint get_anchor_drag_constraint() const noexcept {
        return static_cast<SplineCanvasAxisConstraint>(anchor_drag_constraint);
    }

    void set_point_style(SplineCanvasPointStyle p_style) {
        GeometryCanvas2DBase::set_point_style(static_cast<GeometryCanvasPointStyle>(p_style));
    }
    [[nodiscard]] SplineCanvasPointStyle get_point_style() const noexcept {
        return static_cast<SplineCanvasPointStyle>(point_style);
    }
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplineCanvasAxisConstraint);
VARIANT_ENUM_CAST(ideam::godot_ext::SplineCanvasPointStyle);