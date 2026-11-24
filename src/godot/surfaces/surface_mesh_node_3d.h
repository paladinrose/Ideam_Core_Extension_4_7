#pragma once

/**
 * surface_mesh_node_3d.h
 * 3D Parametric Surface SceneTree Node (`SurfaceMeshNode3D`, `godot::Node3D`) and
 * 3D Viewport Gizmo Plugin (`SurfacePatchGizmo`, `godot::EditorNode3DGizmoPlugin`)
 * in `src/godot/surfaces/`.
 *
 * Capabilities:
 *   - Renders either a standalone `SurfacePatchResource` (`NurbsPatch3DResource` /
 *     `CoonsPatch3DResource`) or a full watertight `SurfaceNetworkResource` via Godot's
 *     `RenderingServer` + `ArrayMesh` (mirroring `SplineExtrudeMesh3D`).
 *   - Supports live `lod_level` (`0..3`), `material_override`, 1-click
 *     `ConcavePolygonShape3D` collision baking, and world-space curvilinear frame queries
 *     (`sample_world_frame(patch_id, u, v)`).
 *   - `SurfacePatchGizmo` renders UV isoparm curves, NURBS control cages, optional surface
 *     normal needles, and interactive 3D control-point handles with `IdeamUndoRedo` snapshots.
 */

#include "coons_patch_3d_resource.h"
#include "nurbs_patch_3d_resource.h"
#include "surface_network_resource.h"
#include "surface_patch_resource.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include "../gizmos/ideam_gizmo.h"
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class SurfaceMeshNode3D : public godot::Node3D {
    GDCLASS(SurfaceMeshNode3D, godot::Node3D)

private:
    godot::Ref<SurfacePatchResource> surface_patch_;
    godot::Ref<SurfaceNetworkResource> surface_network_;
    int lod_level_ = 0;
    bool show_control_cage_ = true;
    bool show_normal_needles_ = false;

    godot::Ref<godot::Material> material_override_;
    godot::Ref<godot::ArrayMesh> built_mesh_;
    godot::RID mesh_instance_rid_;

    void _on_surface_changed();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    SurfaceMeshNode3D();
    ~SurfaceMeshNode3D() override;

    void set_surface_patch(const godot::Ref<SurfacePatchResource> &p_patch);
    [[nodiscard]] godot::Ref<SurfacePatchResource> get_surface_patch() const { return surface_patch_; }

    void set_surface_network(const godot::Ref<SurfaceNetworkResource> &p_network);
    [[nodiscard]] godot::Ref<SurfaceNetworkResource> get_surface_network() const { return surface_network_; }

    void set_lod_level(int p_lod);
    [[nodiscard]] int get_lod_level() const noexcept { return lod_level_; }

    void set_show_control_cage(bool p_show);
    [[nodiscard]] bool get_show_control_cage() const noexcept { return show_control_cage_; }

    void set_show_normal_needles(bool p_show);
    [[nodiscard]] bool get_show_normal_needles() const noexcept { return show_normal_needles_; }

    void set_material_override(const godot::Ref<godot::Material> &p_material);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_built_mesh() const { return built_mesh_; }
    [[nodiscard]] godot::Ref<godot::ConcavePolygonShape3D> bake_concave_collision_shape() const;
    [[nodiscard]] godot::Transform3D sample_world_frame(int p_patch_id, real_t p_u, real_t p_v) const;

    void rebuild_mesh();
};

class SurfacePatchGizmo : public IdeamGizmo {
    GDCLASS(SurfacePatchGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_CP_BASE = 0;

private:
    struct PatchDragState {
        godot::Dictionary initial_snapshot;
        godot::Vector3 initial_local_pos;
        bool active = false;
    };

    mutable godot::HashMap<uint64_t, PatchDragState> active_drags_;

    [[nodiscard]] static godot::Ref<NurbsPatch3DResource> _resolve_editable_nurbs(const SurfaceMeshNode3D *p_node);

protected:
    static void _bind_methods() {}

public:
    SurfacePatchGizmo() = default;
    ~SurfacePatchGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override;
    [[nodiscard]] int32_t get_priority() const override;

    void build_draw_list(
        godot::Node3D *p_target,
        IdeamGizmoDrawList &r_out,
        const GizmoViewState &p_view
    ) const override;
    [[nodiscard]] godot::String get_handle_name(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        bool p_secondary
    ) const override;
    [[nodiscard]] godot::Variant get_handle_value(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        bool p_secondary
    ) const override;
    void on_handle_drag_begin(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        const GizmoRay &p_ray
    ) override;
    void on_handle_drag_update(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        const GizmoRay &p_ray,
        godot::BitField<godot::KeyModifierMask> p_modifiers
    ) override;
    void on_handle_drag_commit(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        bool p_cancel
    ) override;
};

} // namespace ideam::godot_ext
