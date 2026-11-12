#pragma once

/**
 * spline_extrude_mesh_3d.h
 * 3D Bishop-Frame Swept Tube/Ribbon Mesh Node (`SplineExtrudeMesh3D`, `godot::Node3D`)
 * and 3D Viewport Gizmo Plugin (`SplineExtrudeGizmo`, `godot::EditorNode3DGizmoPlugin`)
 * in `src/godot/splines/spline_3d/`.
 *
 * Implements Section 3.3 of the Ideam Core Extension Master Plan:
 *   - Sweeps a `Spline2DResource` cross-section (or built-in circular/rectangular profile)
 *     along a `Spline3DResource` or `Spline4DResource` (`W` = cross-section scale or banking
 *     roll angle in radians) using twist-free Wang Double-Reflection `ParallelTransport3D`
 *     Bishop frames (`SPL7` / `Spline3DResource::compute_bishop_transforms`).
 *   - Supports an optional `Spline1DResource` longitudinal taper profile (`taper_curve_1d`),
 *     optional `Spline4DResource` vertex color gradient, end-cap triangulation (`cap_ends`),
 *     UV tiling, and 1-click `ConcavePolygonShape3D` collision baking.
 *   - `SplineExtrudeGizmo` renders cross-section profile rings along the path and provides
 *     interactive 3D handles for profile scale, total twist, and spline anchors with `IdeamUndoRedo`.
 */

#include "../spline_1d/spline_1d_resource.h"
#include "../spline_2d/spline_2d_resource.h"
#include "../spline_4d/spline_4d_resource.h"
#include "spline_3d_resource.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

enum SplineExtrudeWMode : int32_t {
    EXTRUDE_W_RADIUS_SCALE = 0, // Spline4D W channel scales cross-section radius
    EXTRUDE_W_ROLL_RADIANS = 1, // Spline4D W channel rotates cross-section in radians
    EXTRUDE_W_IGNORE       = 2  // Ignore Spline4D W channel
};

class SplineExtrudeMesh3D : public godot::Node3D {
    GDCLASS(SplineExtrudeMesh3D, godot::Node3D)

private:
    godot::Ref<Spline3DResource> path_spline_3d_;
    godot::Ref<Spline4DResource> path_spline_4d_;
    godot::Ref<Spline2DResource> profile_spline_2d_;
    godot::Ref<Spline1DResource> taper_curve_1d_;
    godot::Ref<Spline4DResource> color_gradient_4d_;

    SplineExtrudeWMode w_channel_mode_ = EXTRUDE_W_RADIUS_SCALE;
    godot::Vector2 profile_scale_      = godot::Vector2(0.5f, 0.5f);
    real_t total_twist_radians_        = static_cast<real_t>(0.0);
    int path_subdivisions_             = 12;
    int profile_subdivisions_          = 8;
    bool cap_ends_                     = true;
    godot::Vector2 uv_scale_           = godot::Vector2(1.0f, 1.0f);
    godot::Color base_color_           = godot::Color(0.85f, 0.90f, 0.98f, 1.0f);

    godot::Ref<godot::Material> material_override_;
    godot::Ref<godot::ArrayMesh> extruded_mesh_;
    godot::RID mesh_instance_rid_;

    void _on_dependency_changed();
    [[nodiscard]] godot::PackedVector2Array _resolve_profile_ring_2d() const;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    SplineExtrudeMesh3D();
    ~SplineExtrudeMesh3D() override;

    void set_path_spline_3d(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_path_spline_3d() const { return path_spline_3d_; }

    void set_path_spline_4d(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_path_spline_4d() const { return path_spline_4d_; }

    void set_profile_spline_2d(const godot::Ref<Spline2DResource> &p_profile);
    [[nodiscard]] godot::Ref<Spline2DResource> get_profile_spline_2d() const { return profile_spline_2d_; }

    void set_taper_curve_1d(const godot::Ref<Spline1DResource> &p_taper);
    [[nodiscard]] godot::Ref<Spline1DResource> get_taper_curve_1d() const { return taper_curve_1d_; }

    void set_color_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient);
    [[nodiscard]] godot::Ref<Spline4DResource> get_color_gradient_4d() const { return color_gradient_4d_; }

    void set_w_channel_mode(SplineExtrudeWMode p_mode);
    [[nodiscard]] SplineExtrudeWMode get_w_channel_mode() const noexcept { return w_channel_mode_; }

    void set_profile_scale(const godot::Vector2 &p_scale);
    [[nodiscard]] godot::Vector2 get_profile_scale() const noexcept { return profile_scale_; }

    void set_total_twist_radians(real_t p_twist);
    [[nodiscard]] real_t get_total_twist_radians() const noexcept { return total_twist_radians_; }

    void set_path_subdivisions(int p_subdivs);
    [[nodiscard]] int get_path_subdivisions() const noexcept { return path_subdivisions_; }

    void set_profile_subdivisions(int p_subdivs);
    [[nodiscard]] int get_profile_subdivisions() const noexcept { return profile_subdivisions_; }

    void set_cap_ends(bool p_cap);
    [[nodiscard]] bool get_cap_ends() const noexcept { return cap_ends_; }

    void set_uv_scale(const godot::Vector2 &p_uv_scale);
    [[nodiscard]] godot::Vector2 get_uv_scale() const noexcept { return uv_scale_; }

    void set_base_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_base_color() const noexcept { return base_color_; }

    void set_material_override(const godot::Ref<godot::Material> &p_material);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_extruded_mesh() const { return extruded_mesh_; }
    [[nodiscard]] godot::Ref<godot::ConcavePolygonShape3D> bake_concave_collision_shape() const;

    void rebuild_mesh();
};

/**
 * SplineExtrudeGizmo (`godot::EditorNode3DGizmoPlugin`)
 * 3D Viewport Gizmo for `SplineExtrudeMesh3D` rendering swept cross-section rings,
 * anchor handles, and interactive profile radius / twist grips.
 */
class SplineExtrudeGizmo : public godot::EditorNode3DGizmoPlugin {
    GDCLASS(SplineExtrudeGizmo, godot::EditorNode3DGizmoPlugin)

public:
    static constexpr int32_t HANDLE_ANCHOR_BASE    = 0;
    static constexpr int32_t HANDLE_PROFILE_RADIUS = 50000;
    static constexpr int32_t HANDLE_TOTAL_TWIST    = 50001;

private:
    struct ExtrudeDragState {
        godot::Dictionary initial_snapshot;
        godot::Vector2 initial_profile_scale;
        real_t initial_twist = static_cast<real_t>(0.0);
        bool active          = false;
    };

    mutable godot::HashMap<uint64_t, ExtrudeDragState> active_drags_;

protected:
    static void _bind_methods() {}

public:
    SplineExtrudeGizmo();
    ~SplineExtrudeGizmo() override = default;

    [[nodiscard]] bool _has_gizmo(godot::Node3D *p_for_node_3d) const override;
    [[nodiscard]] godot::String _get_gizmo_name() const override;
    [[nodiscard]] int32_t _get_priority() const override;

    void _redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) override;
    [[nodiscard]] godot::String _get_handle_name(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary
    ) const override;
    [[nodiscard]] godot::Variant _get_handle_value(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary
    ) const override;
    void _set_handle(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary,
        godot::Camera3D *p_camera,
        const godot::Vector2 &p_screen_pos
    ) override;
    void _commit_handle(
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
        int32_t p_handle_id,
        bool p_secondary,
        const godot::Variant &p_restore,
        bool p_cancel
    ) override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::SplineExtrudeWMode);