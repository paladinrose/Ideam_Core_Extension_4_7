#pragma once

/**
 * character_scaffold_gizmo.h
 * Tool 1 — BodyPlan-Clamped Character Scaffolding Gizmo (`CharacterScaffoldGizmo`, `IdeamGizmo`)
 * in `src/godot/characters/`.
 *
 * Composes Loomis scaffolding guides (Proportion Ruler Staff, Center-of-Gravity Plumb Line,
 * bone/socket hierarchy) with interactive BodyPlan-clamped handles on `LoomisCharacterNode3D`:
 *   - Ruler Base-Unit Handle (`HANDLE_RULER_BASE_UNIT`): drags canonical stature/head height
 *     clamped to `BodyPlanResource::find_parameter_bound()`, applying `BodyPlanCouplingRule`s live.
 *   - Scaffold Volume Handles (`HANDLE_SCAFFOLD_NODE_BASE + node_id`): drags volumetric scaffold
 *     offsets clamped to BodyPlan limits, tinting handles with `GIZMO_STATE_LIMIT_REACHED` when
 *     hitting min/max bounds, and committing via `IdeamUndoRedo`.
 */

#include "../gizmos/ideam_gizmo.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class CharacterScaffoldGizmo : public IdeamGizmo {
    GDCLASS(CharacterScaffoldGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_RULER_BASE_UNIT    = 0;
    static constexpr int32_t HANDLE_SCAFFOLD_NODE_BASE = 100;

private:
    struct DragState {
        int32_t handle_id = -1;
        godot::String param_name;
        real_t initial_param_value = static_cast<real_t>(0.0);
        godot::Vector3 initial_offset;
        bool limit_reached = false;
        bool active        = false;
    };

    mutable godot::HashMap<uint64_t, DragState> active_drags_;

protected:
    static void _bind_methods() {}

public:
    CharacterScaffoldGizmo();
    ~CharacterScaffoldGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "IdeamCharacterScaffoldGizmo"; }
    [[nodiscard]] int32_t get_priority() const override { return 5; }

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
