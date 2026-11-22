#pragma once

/**
 * form_scaffold_node_3d.h
 * 3D Volumetric Loomis Scaffold SceneTree Node (`FormScaffoldNode3D`, `godot::Node3D`)
 * and 3D Viewport Gizmo Plugin (`FormScaffoldGizmo`, `godot::EditorNode3DGizmoPlugin`)
 * in `src/godot/design/`.
 *
 * Capabilities:
 *   - Renders the live `FormScaffoldResource` volumetric forms via `RenderingServer` + `ArrayMesh`.
 *   - 1-click `build_skeleton_3d()` generating a Godot `Skeleton3D` bone tree directly from
 *     the scaffold's `SocketPod` hierarchy (with exact rest poses and parent indices).
 *   - `build_ragdoll_collider_descriptors()` emitting rough collider hulls, masses, and
 *     joint limits ready for `PhysicalBoneSimulator3D` / `OmniRotationConstraintPod`.
 *   - `FormScaffoldGizmo` renders the Loomis Canonical-Unit Proportion Ruler Staff,
 *     Center-of-Gravity Plumb Line (green = balanced, amber = off-balance),
 *     socket bone hierarchy links, and interactive 3D node offset handles with `IdeamUndoRedo`.
 */

#include "form_scaffold_resource.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class FormScaffoldNode3D : public godot::Node3D {
    GDCLASS(FormScaffoldNode3D, godot::Node3D)

private:
    godot::Ref<FormScaffoldResource> scaffold_;
    int preview_subdivs_ = 10;
    bool show_proportion_ruler_ = true;
    bool show_balance_plumb_line_ = true;
    bool show_bone_hierarchy_ = true;

    godot::Ref<godot::Material> material_override_;
    godot::Ref<godot::ArrayMesh> preview_mesh_;
    godot::RID mesh_instance_rid_;

    void _on_scaffold_changed();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    FormScaffoldNode3D();
    ~FormScaffoldNode3D() override;

    void set_scaffold(const godot::Ref<FormScaffoldResource> &p_scaffold);
    [[nodiscard]] godot::Ref<FormScaffoldResource> get_scaffold() const { return scaffold_; }

    void set_preview_subdivs(int p_subdivs);
    [[nodiscard]] int get_preview_subdivs() const noexcept { return preview_subdivs_; }

    void set_show_proportion_ruler(bool p_show);
    [[nodiscard]] bool get_show_proportion_ruler() const noexcept { return show_proportion_ruler_; }

    void set_show_balance_plumb_line(bool p_show);
    [[nodiscard]] bool get_show_balance_plumb_line() const noexcept { return show_balance_plumb_line_; }

    void set_show_bone_hierarchy(bool p_show);
    [[nodiscard]] bool get_show_bone_hierarchy() const noexcept { return show_bone_hierarchy_; }

    void set_material_override(const godot::Ref<godot::Material> &p_mat);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_preview_mesh() const { return preview_mesh_; }
    [[nodiscard]] godot::Transform3D sample_world_scaffold_frame(int p_node_id, real_t p_u, real_t p_v) const;

    /**
     * Generates a ready-to-use Godot `Skeleton3D` node with bone hierarchy and rest poses
     * derived from the scaffold's `SocketPod` tree.
     */
    [[nodiscard]] godot::Skeleton3D *build_skeleton_3d() const;

    /**
     * Emits ragdoll/collider/joint-limit descriptors for downstream physics & `IdeamCharacters`.
     */
    [[nodiscard]] godot::TypedArray<godot::Dictionary> build_ragdoll_collider_descriptors() const;

    void rebuild_preview();
};

class FormScaffoldGizmo : public godot::EditorNode3DGizmoPlugin {
    GDCLASS(FormScaffoldGizmo, godot::EditorNode3DGizmoPlugin)

public:
    static constexpr int32_t HANDLE_NODE_BASE = 0;

private:
    struct ScaffoldDragState {
        int node_id = -1;
        godot::Vector3 initial_local_offset;
        bool active = false;
    };

    mutable godot::HashMap<uint64_t, ScaffoldDragState> active_drags_;

    void _init_materials();

protected:
    static void _bind_methods() {}

public:
    FormScaffoldGizmo();
    ~FormScaffoldGizmo() override = default;

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
