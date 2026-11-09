#pragma once

/**
 * tiling_surface_node_3d.h
 * 3D Scene Node that projects a 2D `TilingResource` onto a 3D plane (`XZ`, `XY`, `YZ`)
 * or along a `Spline3DResource` ribbon using Rotation-Minimizing Bishop Frames, with
 * optional 3D prism extrusion (`src/godot/tiles/`).
 */

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "tiling_resource.h"
#include "wallpaper_tiling_resource.h"
#include "composite_tiling_resource.h"
#include "../splines/spline_2d/spline_2d_resource.h"
#include "../splines/spline_3d/spline_3d_resource.h"
#include "../../core/tiles/tiling_region_2d.h"

namespace ideam::godot_ext {

class TilingSurfaceNode3D : public godot::Node3D {
    GDCLASS(TilingSurfaceNode3D, godot::Node3D)

public:
    enum Tiling3DProjectionMode {
        PROJECTION_XZ_GROUND        = 0,
        PROJECTION_XY_WALL          = 1,
        PROJECTION_YZ_SIDE          = 2,
        PROJECTION_SPLINE_3D_RIBBON = 3
    };

private:
    using CoreRegion = core::TilingRegion2D<real_t>;
    using CoreSpline = core::Spline2D<real_t>;
    using Vec2       = typename CoreRegion::Vec2;

    godot::Ref<TilingResource> tiling_;
    godot::Ref<Spline2DResource> boundary_spline_2d_;
    godot::Ref<Spline3DResource> path_spline_3d_;
    godot::Rect2 fallback_region_rect_ = godot::Rect2(-5.0f, -5.0f, 10.0f, 10.0f);

    Tiling3DProjectionMode projection_mode_ = PROJECTION_XZ_GROUND;
    real_t extrude_depth_                   = static_cast<real_t>(0.0);
    int edge_subdivisions_                  = 12;

    godot::Color color_0_           = godot::Color(0.22f, 0.45f, 0.72f, 1.0f);
    godot::Color color_1_           = godot::Color(0.32f, 0.65f, 0.58f, 1.0f);
    godot::Color color_2_           = godot::Color(0.82f, 0.62f, 0.28f, 1.0f);
    godot::Color bridge_quad_color_ = godot::Color(0.76f, 0.35f, 0.48f, 1.0f);

    godot::Ref<godot::Material> material_override_;
    godot::Ref<godot::ArrayMesh> mesh_;
    godot::RID mesh_instance_rid_;

    mutable CoreRegion local_region_cache_;
    mutable bool local_cache_dirty_ = true;

    void _on_dependency_changed();
    [[nodiscard]] const CoreRegion *_resolve_active_region() const;
    [[nodiscard]] godot::Vector3 _project_point_2d_to_3d(
        const Vec2 &p_pt,
        real_t p_normal_offset,
        godot::Vector3 &r_normal
    ) const;

    [[nodiscard]] static inline Vec2 _to_glm(const godot::Vector2 &p_v) noexcept {
        return Vec2(static_cast<real_t>(p_v.x), static_cast<real_t>(p_v.y));
    }

protected:
    static void _bind_methods();

public:
    TilingSurfaceNode3D();
    ~TilingSurfaceNode3D() override;

    void _notification(int p_what);

    void set_tiling(const godot::Ref<TilingResource> &p_tiling);
    [[nodiscard]] godot::Ref<TilingResource> get_tiling() const { return tiling_; }

    void set_boundary_spline_2d(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_boundary_spline_2d() const { return boundary_spline_2d_; }

    void set_path_spline_3d(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_path_spline_3d() const { return path_spline_3d_; }

    void set_fallback_region_rect(const godot::Rect2 &p_rect);
    [[nodiscard]] godot::Rect2 get_fallback_region_rect() const { return fallback_region_rect_; }

    void set_projection_mode(int p_mode);
    [[nodiscard]] int get_projection_mode() const { return static_cast<int>(projection_mode_); }

    void set_extrude_depth(real_t p_depth);
    [[nodiscard]] real_t get_extrude_depth() const { return extrude_depth_; }

    void set_edge_subdivisions(int p_subdivs);
    [[nodiscard]] int get_edge_subdivisions() const { return edge_subdivisions_; }

    void set_color_0(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_0() const { return color_0_; }

    void set_color_1(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_1() const { return color_1_; }

    void set_color_2(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_color_2() const { return color_2_; }

    void set_bridge_quad_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_bridge_quad_color() const { return bridge_quad_color_; }

    void set_material_override(const godot::Ref<godot::Material> &p_material);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_generated_mesh() const { return mesh_; }
    void rebuild_mesh();
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::TilingSurfaceNode3D::Tiling3DProjectionMode);