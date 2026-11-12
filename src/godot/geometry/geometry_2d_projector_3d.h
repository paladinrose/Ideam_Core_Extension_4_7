#pragma once

/**
 * geometry_2d_projector_3d.h
 * Reusable 2D-to-3D Topology Projector Base Node (`Geometry2DProjector3D`, `godot::Node3D`)
 * in `src/godot/geometry/`.
 *
 * Serves as the shared 2D -> 3D topological projection substrate for both `IdeamSplines`
 * (`Spline2DProjector3D`) and `IdeamTiles` / `IdeamGeometry` (draping 2D curves, `Polygon2DResource`
 * regions, `VoronoiDiagram2DResource` territories, or 2D tile meshes onto 3D topology):
 *   1. Supports 6 projection modes (`Geometry3DProjectionMode`):
 *      - `PROJ_PLANE_XZ_GROUND`: Planar projection onto local XZ ground `(u, offset, v)`.
 *      - `PROJ_PLANE_XY_WALL`: Planar projection onto local XY wall `(u, v, offset)`.
 *      - `PROJ_CYLINDRICAL_WRAP`: Wraps `u` around cylinder circumference of `projection_radius` and maps `v` to Y.
 *      - `PROJ_SPHERICAL_WRAP`: Wraps `(u, v)` onto a 3D sphere of `projection_radius` (azimuth/elevation).
 *      - `PROJ_PHYSICS_RAYCAST_DRAPE`: Casts 3D physics rays (`PhysicsDirectSpaceState3D`) along the
 *        local projection axis to drape 2D geometry onto arbitrary 3D terrain/colliders.
 *      - `PROJ_TARGET_MESH_SURFACE`: Raycasts in pure C++ against a target `MeshInstance3D`'s triangle
 *        faces so 2D splines and polygons snap onto 3D meshes in the Editor even without physics bodies.
 *   2. Provides point, orthonormal basis (`Transform3D`), subdivided polyline, and triangulated
 *      ribbon/polygon/extruded-wall `ArrayMesh` builders, plus an optional bound `Polygon2DResource`
 *      for direct 2D polygon draping in 3D scenes.
 */

#include "resources/polygon_2d_resource.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <vector>

namespace ideam::godot_ext {

enum Geometry3DProjectionMode : int32_t {
    PROJ_PLANE_XZ_GROUND       = 0,
    PROJ_PLANE_XY_WALL         = 1,
    PROJ_CYLINDRICAL_WRAP      = 2,
    PROJ_SPHERICAL_WRAP        = 3,
    PROJ_PHYSICS_RAYCAST_DRAPE = 4,
    PROJ_TARGET_MESH_SURFACE   = 5
};

class Geometry2DProjector3D : public godot::Node3D {
    GDCLASS(Geometry2DProjector3D, godot::Node3D)

private:
    struct CachedMeshTriangle {
        godot::Vector3 v0;
        godot::Vector3 v1;
        godot::Vector3 v2;
        godot::Vector3 normal;
    };

    Geometry3DProjectionMode projection_mode_ = PROJ_PLANE_XZ_GROUND;
    godot::Vector2 input_uv_scale_            = godot::Vector2(1.0f, 1.0f);
    godot::Vector2 input_uv_offset_           = godot::Vector2(0.0f, 0.0f);

    real_t normal_offset_          = static_cast<real_t>(0.02);
    real_t projection_radius_      = static_cast<real_t>(5.0);
    real_t max_ray_distance_       = static_cast<real_t>(100.0);
    real_t ray_origin_height_      = static_cast<real_t>(25.0);
    uint32_t collision_mask_       = 1u;
    bool align_to_surface_normal_  = true;
    real_t extrude_thickness_      = static_cast<real_t>(0.0);

    godot::NodePath target_mesh_instance_path_;
    godot::Ref<Polygon2DResource> source_polygon_2d_;
    godot::Ref<godot::Material> material_override_;
    godot::Color surface_albedo_ = godot::Color(0.28f, 0.76f, 0.96f, 0.85f);

    godot::Ref<godot::ArrayMesh> projected_mesh_;
    godot::RID mesh_instance_rid_;

    mutable std::vector<CachedMeshTriangle> cached_target_triangles_;
    mutable uint64_t cached_target_mesh_id_ = 0ULL;
    mutable bool target_mesh_cache_dirty_   = true;

    void _on_source_polygon_changed();
    void _rebuild_target_mesh_triangle_cache() const;
    [[nodiscard]] bool _raycast_target_mesh_local(
        const godot::Vector3 &p_ray_origin_local,
        const godot::Vector3 &p_ray_dir_local,
        real_t p_max_dist,
        godot::Vector3 &r_hit_local,
        godot::Vector3 &r_normal_local
    ) const;

protected:
    static void _bind_methods();
    void _notification(int p_what);

    /**
     * Subclasses (`Spline2DProjector3D`, etc.) override this to rebuild their projected
     * 3D geometry whenever projection parameters or bound 2D resources change.
     */
    virtual void _rebuild_projected_geometry();

    void _commit_generated_mesh(const godot::Ref<godot::ArrayMesh> &p_mesh);

public:
    Geometry2DProjector3D();
    ~Geometry2DProjector3D() override;

    // ========================================================================
    // Projection Mode & Topology Configuration
    // ========================================================================

    void set_projection_mode(Geometry3DProjectionMode p_mode);
    [[nodiscard]] Geometry3DProjectionMode get_projection_mode() const noexcept { return projection_mode_; }

    void set_input_uv_scale(const godot::Vector2 &p_scale);
    [[nodiscard]] godot::Vector2 get_input_uv_scale() const noexcept { return input_uv_scale_; }

    void set_input_uv_offset(const godot::Vector2 &p_offset);
    [[nodiscard]] godot::Vector2 get_input_uv_offset() const noexcept { return input_uv_offset_; }

    void set_normal_offset(real_t p_offset);
    [[nodiscard]] real_t get_normal_offset() const noexcept { return normal_offset_; }

    void set_projection_radius(real_t p_radius);
    [[nodiscard]] real_t get_projection_radius() const noexcept { return projection_radius_; }

    void set_max_ray_distance(real_t p_distance);
    [[nodiscard]] real_t get_max_ray_distance() const noexcept { return max_ray_distance_; }

    void set_ray_origin_height(real_t p_height);
    [[nodiscard]] real_t get_ray_origin_height() const noexcept { return ray_origin_height_; }

    void set_collision_mask(uint32_t p_mask);
    [[nodiscard]] uint32_t get_collision_mask() const noexcept { return collision_mask_; }

    void set_align_to_surface_normal(bool p_align);
    [[nodiscard]] bool get_align_to_surface_normal() const noexcept { return align_to_surface_normal_; }

    void set_extrude_thickness(real_t p_thickness);
    [[nodiscard]] real_t get_extrude_thickness() const noexcept { return extrude_thickness_; }

    void set_target_mesh_instance_path(const godot::NodePath &p_path);
    [[nodiscard]] godot::NodePath get_target_mesh_instance_path() const { return target_mesh_instance_path_; }

    void set_source_polygon_2d(const godot::Ref<Polygon2DResource> &p_polygon);
    [[nodiscard]] godot::Ref<Polygon2DResource> get_source_polygon_2d() const { return source_polygon_2d_; }

    void set_material_override(const godot::Ref<godot::Material> &p_material);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    void set_surface_albedo(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_surface_albedo() const noexcept { return surface_albedo_; }

    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_projected_mesh() const { return projected_mesh_; }

    // ========================================================================
    // Core 2D -> 3D Projection Queries & Mesh Builders (Used by Splines & Tiles)
    // ========================================================================

    /**
     * Projects a 2D coordinate `p_uv` into the projector's local 3D space, outputting
     * the projected local 3D point `r_local_pos` and local surface normal `r_local_normal`.
     * Returns true if a surface hit occurred (always true for analytic plane/cylinder/sphere modes).
     */
    bool project_point_2d_with_normal(
        const godot::Vector2 &p_uv,
        godot::Vector3 &r_local_pos,
        godot::Vector3 &r_local_normal
    ) const;

    [[nodiscard]] godot::Vector3 project_point_2d(const godot::Vector2 &p_uv) const;

    /**
     * Projects a 2D point and 2D tangent vector into an orthonormal local 3D `Transform3D`
     * whose `Y` axis aligns with the 3D surface normal and `-Z` aligns with the projected tangent.
     */
    [[nodiscard]] godot::Transform3D project_transform_2d(
        const godot::Vector2 &p_uv,
        const godot::Vector2 &p_tangent_2d = godot::Vector2(1.0f, 0.0f)
    ) const;

    /**
     * Projects an entire 2D polyline onto 3D topology, optionally subdividing segments longer
     * than `p_max_segment_length_2d` so the 3D polyline conforms smoothly to curved/uneven terrain.
     */
    [[nodiscard]] godot::PackedVector3Array project_polyline_2d(
        const godot::PackedVector2Array &p_points_2d,
        real_t p_max_segment_length_2d = static_cast<real_t>(0.5)
    ) const;

    /**
     * Builds a 3D `ArrayMesh` by projecting a 2D triangulated polygon (`p_vertices_2d`, `p_triangles_2d`)
     * onto the 3D topology, with optional normal extrusion (`p_extrude_thickness`).
     */
    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_projected_polygon_mesh(
        const godot::PackedVector2Array &p_vertices_2d,
        const godot::PackedInt32Array &p_triangles_2d,
        real_t p_extrude_thickness = static_cast<real_t>(0.0),
        const godot::Color &p_vertex_color = godot::Color(1.0f, 1.0f, 1.0f, 1.0f)
    ) const;

    void invalidate_target_mesh_cache();
    void rebuild_projection();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::Geometry3DProjectionMode);