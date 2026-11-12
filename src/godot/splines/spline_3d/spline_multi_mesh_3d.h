#pragma once

/**
 * spline_multi_mesh_3d.h
 * Spline-Driven 3D MultiMesh Populator (`SplineMultiMesh3D`, `godot::Node3D`) and
 * 3D Viewport Scatter Gizmo (`SplineScatterGizmo`, `godot::EditorNode3DGizmoPlugin`)
 * in `src/godot/splines/spline_3d/`.
 *
 * Implements Section 3.3 of the Ideam Core Extension Master Plan:
 *   - Populates a `MultiMesh` (`RenderingServer` instance) either:
 *     1. **Along a `Spline3DResource` / `Spline4DResource` curve** (`SCATTER_ALONG_PATH_3D`)
 *        with distance-even or section-weighted placement, lateral ribbon jitter (`scatter_width`),
 *        twist-free Bishop frame orientation, and `Spline1DResource` scale envelope (`scale_curve_1d`).
 *     2. **Inside a closed `Spline2DResource` loop** (`SCATTER_INSIDE_REGION_2D`) projected onto
 *        the local XZ plane using exact `Spline2DResource::contains_point()` filtering.
 *   - `SplineScatterGizmo` visualizes instance placement markers, lateral scatter corridor rails,
 *     and interactive width/anchor handles wired to `IdeamUndoRedo`.
 */

#include "../spline_1d/spline_1d_resource.h"
#include "../spline_2d/spline_2d_resource.h"
#include "../spline_4d/spline_4d_resource.h"
#include "spline_3d_resource.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

enum SplineMultiMeshPlacementMode : int32_t {
    SCATTER_ALONG_PATH_3D    = 0,
    SCATTER_INSIDE_REGION_2D = 1
};

class SplineMultiMesh3D : public godot::Node3D {
    GDCLASS(SplineMultiMesh3D, godot::Node3D)

private:
    godot::Ref<Spline3DResource> path_spline_3d_;
    godot::Ref<Spline2DResource> region_spline_2d_;
    godot::Ref<Spline1DResource> scale_curve_1d_;
    godot::Ref<Spline4DResource> color_gradient_4d_;

    godot::Ref<godot::Mesh> instance_mesh_;
    godot::Ref<godot::Material> material_override_;
    godot::Ref<godot::MultiMesh> multimesh_;
    godot::RID multimesh_instance_rid_;

    SplineMultiMeshPlacementMode placement_mode_ = SCATTER_ALONG_PATH_3D;
    int instance_count_                          = 24;
    real_t scatter_width_                        = static_cast<real_t>(0.0);
    real_t random_yaw_radians_                   = static_cast<real_t>(0.0);
    real_t random_scale_jitter_                  = static_cast<real_t>(0.0);
    bool align_to_spline_frame_                  = true;
    bool use_weighted_distribution_              = false;
    int64_t random_seed_                         = 1337;

    void _on_dependency_changed();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    SplineMultiMesh3D();
    ~SplineMultiMesh3D() override;

    void set_path_spline_3d(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_path_spline_3d() const { return path_spline_3d_; }

    void set_region_spline_2d(const godot::Ref<Spline2DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline2DResource> get_region_spline_2d() const { return region_spline_2d_; }

    void set_scale_curve_1d(const godot::Ref<Spline1DResource> &p_curve);
    [[nodiscard]] godot::Ref<Spline1DResource> get_scale_curve_1d() const { return scale_curve_1d_; }

    void set_color_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient);
    [[nodiscard]] godot::Ref<Spline4DResource> get_color_gradient_4d() const { return color_gradient_4d_; }

    void set_instance_mesh(const godot::Ref<godot::Mesh> &p_mesh);
    [[nodiscard]] godot::Ref<godot::Mesh> get_instance_mesh() const { return instance_mesh_; }

    void set_material_override(const godot::Ref<godot::Material> &p_material);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    void set_placement_mode(SplineMultiMeshPlacementMode p_mode);
    [[nodiscard]] SplineMultiMeshPlacementMode get_placement_mode() const noexcept { return placement_mode_; }

    void set_instance_count(int p_count);
    [[nodiscard]] int get_instance_count() const noexcept { return instance_count_; }

    void set_scatter_width(real_t p_width);
    [[nodiscard]] real_t get_scatter_width() const noexcept { return scatter_width_; }

    void set_random_yaw_radians(real_t p_yaw);
    [[nodiscard]] real_t get_random_yaw_radians() const noexcept { return random_yaw_radians_; }

    void set_random_scale_jitter(real_t p_jitter);
    [[nodiscard]] real_t get_random_scale_jitter() const noexcept { return random_scale_jitter_; }

    void set_align_to_spline_frame(bool p_align);
    [[nodiscard]] bool get_align_to_spline_frame() const noexcept { return align_to_spline_frame_; }

    void set_use_weighted_distribution(bool p_weighted);
    [[nodiscard]] bool get_use_weighted_distribution() const noexcept { return use_weighted_distribution_; }

    void set_random_seed(int64_t p_seed);
    [[nodiscard]] int64_t get_random_seed() const noexcept { return random_seed_; }

    [[nodiscard]] godot::Ref<godot::MultiMesh> get_multimesh() const { return multimesh_; }
    void rebuild_instances();
};

/**
 * SplineScatterGizmo (`godot::EditorNode3DGizmoPlugin`)
 * 3D Viewport Gizmo for `SplineMultiMesh3D`.
 */
class SplineScatterGizmo : public godot::EditorNode3DGizmoPlugin {
    GDCLASS(SplineScatterGizmo, godot::EditorNode3DGizmoPlugin)

public:
    static constexpr int32_t HANDLE_ANCHOR_BASE   = 0;
    static constexpr int32_t HANDLE_SCATTER_WIDTH = 50000;

protected:
    static void _bind_methods() {}

public:
    SplineScatterGizmo();
    ~SplineScatterGizmo() override = default;

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

VARIANT_ENUM_CAST(ideam::godot_ext::SplineMultiMeshPlacementMode);