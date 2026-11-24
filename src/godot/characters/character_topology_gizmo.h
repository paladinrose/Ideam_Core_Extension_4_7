#pragma once

/**
 * character_topology_gizmo.h
 * Tool 2 — Character Topology & Seamless Part-Seam Gizmo (`CharacterTopologyGizmo`, `IdeamGizmo`)
 * in `src/godot/characters/`.
 *
 * Active when `active_tool_mode == "topology"`:
 *   - Displays scaffold-anchored topology landmark handles (`GIZMO_HANDLE_SPLINE_ANCHOR`)
 *     with planar-sharpness rings and normal-offset drag lines.
 *   - Highlights separable `PartSeamContract` boundary rings (`GIZMO_LINE_SEAM_HOLE`) between
 *     adjacent parts (Chest <-> Shoulders, Chest <-> Neck, Pelvis <-> Thighs).
 *   - Dragging a landmark handle adjusts its normal offset (or records an asymmetric
 *     `TopologyDetailOverride` when Details mode is active) with `IdeamUndoRedo`.
 */

#include "../gizmos/ideam_gizmo.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class CharacterTopologyGizmo : public IdeamGizmo {
    GDCLASS(CharacterTopologyGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_LANDMARK_BASE = 200;

private:
    struct DragState {
        int32_t landmark_id = -1;
        real_t initial_normal_offset = static_cast<real_t>(0.0);
        godot::Vector3 anchor_world_pos;
        godot::Vector3 anchor_world_normal;
        bool active = false;
    };

    mutable godot::HashMap<uint64_t, DragState> active_drags_;

protected:
    static void _bind_methods() {}

public:
    CharacterTopologyGizmo();
    ~CharacterTopologyGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "IdeamCharacterTopologyGizmo"; }
    [[nodiscard]] int32_t get_priority() const override { return 6; }

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
