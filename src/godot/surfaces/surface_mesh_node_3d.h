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
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/material.hpp>
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

class SurfacePatchGizmo : public godot::EditorNode3DGizmoPlugin {
    GDCLASS(SurfacePatchGizmo, godot::EditorNode3DGizmoPlugin)

public:
    static constexpr int32_t HANDLE_CP_BASE = 0;

private:
    struct PatchDragState {
        godot::Dictionary initial_snapshot;
        godot::Vector3 initial_local_pos;
        bool active = false;
    };

    mutable godot::HashMap<uint64_t, PatchDragState> active_drags_;

    void _init_materials();
    [[nodiscard]] static godot::Ref<NurbsPatch3DResource> _resolve_editable_nurbs(const SurfaceMeshNode3D *p_node);

protected:
    static void _bind_methods() {}

public:
    SurfacePatchGizmo();
    ~SurfacePatchGizmo() override = default;

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
