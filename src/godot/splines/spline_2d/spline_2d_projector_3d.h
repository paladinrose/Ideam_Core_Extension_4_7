#pragma once

/**
 * spline_2d_projector_3d.h
 * Concrete 2D-Spline-to-3D-Topology Projector Node (`Spline2DProjector3D`, extends `Geometry2DProjector3D`)
 * in `src/godot/splines/spline_2d/`.
 *
 * Projects a `Spline2DResource` onto 3D topology (Terrain Physics Raycast, Target `MeshInstance3D`
 * triangle surface, Cylinder, Sphere, or XZ/XY Plane) via `Geometry2DProjector3D`:
 *   1. Supports 3 3D Mesh Generation Modes (`Spline2DProjectionOutputMode`):
 *      - `SPLINE_PROJ_RIBBON_STRIP`: Generates a conformal 3D road/decal ribbon along the projected
 *        2D curve whose width is modulated by `ribbon_width` and an optional `Spline1DResource`
 *        (`width_profile_1d`), and whose vertex colors are optionally driven by a `Spline4DResource`
 *        (`color_gradient_4d`).
 *      - `SPLINE_PROJ_FILLED_POLYGON`: Triangulates a closed `Spline2DResource` and drapes the
 *        curved region onto 3D topology (with optional `extrude_thickness`).
 *      - `SPLINE_PROJ_EXTRUDED_WALL`: Sweeps a vertical wall/curb of `extrude_thickness` along the
 *        projected 3D surface normals.
 *   2. Provides `bake_to_spline_3d_resource()` to convert the projected 2D spline directly into
 *      a `Spline3DResource` (preserving anchor count or baking dense subdivided samples) for
 *      `SplineNode3D`, `SplineExtrudeMesh3D`, `SplineMultiMesh3D`, or `SplineZone3D`.
 */

#include "../../geometry/geometry_2d_projector_3d.h"
#include "../spline_1d/spline_1d_resource.h"
#include "../spline_3d/spline_3d_resource.h"
#include "../spline_4d/spline_4d_resource.h"
#include "spline_2d_resource.h"

namespace ideam::godot_ext {

enum Spline2DProjectionOutputMode : int32_t {
    SPLINE_PROJ_RIBBON_STRIP   = 0,
    SPLINE_PROJ_FILLED_POLYGON = 1,
    SPLINE_PROJ_EXTRUDED_WALL  = 2
};

class Spline2DProjector3D : public Geometry2DProjector3D {
    GDCLASS(Spline2DProjector3D, Geometry2DProjector3D)

private:
    godot::Ref<Spline2DResource> spline_2d_;
    godot::Ref<Spline1DResource> width_profile_1d_;
    godot::Ref<Spline4DResource> color_gradient_4d_;

    Spline2DProjectionOutputMode output_mode_ = SPLINE_PROJ_RIBBON_STRIP;
    real_t ribbon_width_                      = static_cast<real_t>(0.6);
    int curve_subdivisions_                   = 16;

    void _on_spline_dependency_changed();

protected:
    static void _bind_methods();
    void _rebuild_projected_geometry() override;

public:
    Spline2DProjector3D();
    ~Spline2DProjector3D() override;

    void set_spline_2d(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_spline_2d() const { return spline_2d_; }

    void set_width_profile_1d(const godot::Ref<Spline1DResource> &p_profile);
    [[nodiscard]] godot::Ref<Spline1DResource> get_width_profile_1d() const { return width_profile_1d_; }

    void set_color_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient);
    [[nodiscard]] godot::Ref<Spline4DResource> get_color_gradient_4d() const { return color_gradient_4d_; }

    void set_output_mode(Spline2DProjectionOutputMode p_mode);
    [[nodiscard]] Spline2DProjectionOutputMode get_output_mode() const noexcept { return output_mode_; }

    void set_ribbon_width(real_t p_width);
    [[nodiscard]] real_t get_ribbon_width() const noexcept { return ribbon_width_; }

    void set_curve_subdivisions(int p_subdivs);
    [[nodiscard]] int get_curve_subdivisions() const noexcept { return curve_subdivisions_; }

    /**
     * Evaluates the projected 3D point and surface-aligned `Transform3D` at normalized `p_percent` in `[0, 1]`.
     */
    [[nodiscard]] godot::Vector3 sample_projected_point_3d(real_t p_percent, bool p_weighted = true) const;
    [[nodiscard]] godot::Transform3D sample_projected_transform_3d(real_t p_percent, bool p_weighted = true) const;

    /**
     * Bakes the projected 2D spline into a new `Spline3DResource` whose 3D control points
     * lie on the target 3D topology.
     */
    [[nodiscard]] godot::Ref<Spline3DResource> bake_to_spline_3d_resource(
        bool p_bake_subdivided_points = false
    ) const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::Spline2DProjectionOutputMode);