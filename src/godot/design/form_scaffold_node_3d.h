#pragma once

/**
 * form_scaffold_node_3d.h
 * 3D Volumetric Loomis Scaffold SceneTree Node (`FormScaffoldNode3D`, `godot::Node3D`)
 * and Trans-Environment 3D Gizmo (`FormScaffoldGizmo`, `IdeamGizmo`)
 * in `src/godot/design/`.
 *
 * Capabilities:
 *   - Renders the live `FormScaffoldResource` volumetric forms via `RenderingServer` + `ArrayMesh`.
 *   - 1-click `build_skeleton_3d()` generating a Godot `Skeleton3D` bone tree directly from
 *     the scaffold's `SocketPod` hierarchy (with exact rest poses and parent indices).
 *   - `build_ragdoll_collider_descriptors()` emitting rough collider hulls, masses, and
 *     joint limits ready for `PhysicalBoneSimulator3D` / `OmniRotationConstraintPod`.
 *   - `FormScaffoldGizmo` (migrated onto `IdeamGizmo`) renders the Loomis Canonical-Unit
 *     Proportion Ruler Staff, Center-of-Gravity Plumb Line (green = balanced, amber = off-balance),
 *     socket bone hierarchy links, and interactive 3D node offset handles with `IdeamUndoRedo`
 *     across all 4 execution contexts (Godot Editor, SubViewport, Runtime World, Avatar Creator).
 */

#include "form_scaffold_resource.h"
#include "../gizmos/ideam_gizmo.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <functional>

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

    [[nodiscard]] godot::Skeleton3D *build_skeleton_3d() const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> build_ragdoll_collider_descriptors() const;

    void rebuild_preview();
};

/**
 * Trans-environment Loomis Form Scaffold Gizmo (`IdeamGizmo` subclass).
 * Supports optional offset clamp callback (used by `BodyPlanResource` in `IdeamCharacters`).
 */
class FormScaffoldGizmo : public IdeamGizmo {
    GDCLASS(FormScaffoldGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_NODE_BASE = 0;

    using OffsetClampCallback = std::function<godot::Vector3(int p_node_id, const godot::Vector3 &p_proposed, bool &r_clamped)>;

private:
    struct ScaffoldDragState {
        int node_id = -1;
        godot::Vector3 initial_local_offset;
        bool active = false;
        bool limit_reached = false;
    };

    mutable godot::HashMap<uint64_t, ScaffoldDragState> active_drags_;
    OffsetClampCallback offset_clamp_cb_;

protected:
    static void _bind_methods() {}

public:
    FormScaffoldGizmo() = default;
    ~FormScaffoldGizmo() override = default;

    void set_offset_clamp_callback(OffsetClampCallback p_cb) { offset_clamp_cb_ = std::move(p_cb); }

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
