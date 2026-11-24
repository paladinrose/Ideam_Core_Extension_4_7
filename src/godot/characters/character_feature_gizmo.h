#pragma once

/**
 * character_feature_gizmo.h
 * Tool 3 — Shift+Click / Hold-Shift Focus Feature Gizmo (`CharacterFeatureGizmo`, `IdeamGizmo`)
 * in `src/godot/characters/`.
 *
 * Implements the 2-state focus controller:
 *   - State A (Feature Overview — default when entering Tool 3, OR whenever Shift is held):
 *     Hides fine detail gizmos and renders high-visibility major feature anchor handles
 *     (`GIZMO_HANDLE_FEATURE_MAJOR`) for Eyes, Ears, Nose, Mouth, Fingers, Toes, etc.
 *   - Action (Shift + Click on a Major Feature):
 *     Sets `active_feature_id_` and emits `feature_focused(feature_id)`.
 *   - State B (Isolated Feature Detail — when Shift is released with a valid `active_feature_id_`):
 *     Hides all other major feature anchors and renders ONLY fine detail handles
 *     (`GIZMO_HANDLE_FEATURE_DETAIL`: Width/Height Scale, Outward Projection, Secondary Param
 *     such as eyelid droop, ear point, lip parting, or knuckle curl).
 */

#include "../gizmos/ideam_gizmo.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class CharacterFeatureGizmo : public IdeamGizmo {
    GDCLASS(CharacterFeatureGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_MAJOR_FEATURE_BASE = 300;
    static constexpr int32_t HANDLE_DETAIL_SCALE       = 400;
    static constexpr int32_t HANDLE_DETAIL_SECONDARY   = 401;
    static constexpr int32_t HANDLE_DETAIL_OFFSET      = 402;

private:
    struct DragState {
        int32_t feature_id = -1;
        int32_t detail_handle = -1;
        godot::Vector3 initial_extents;
        godot::Vector3 initial_offset;
        real_t initial_secondary = static_cast<real_t>(0.0);
        godot::Vector3 anchor_world_pos;
        godot::Vector3 anchor_world_u;
        godot::Vector3 anchor_world_v;
        godot::Vector3 anchor_world_n;
        bool active = false;
    };

    int32_t active_feature_id_ = -1;
    bool shift_overview_held_  = false;
    mutable godot::HashMap<uint64_t, DragState> active_drags_;

protected:
    static void _bind_methods();

public:
    CharacterFeatureGizmo();
    ~CharacterFeatureGizmo() override = default;

    void set_active_feature_id(int32_t p_feature_id);
    [[nodiscard]] int32_t get_active_feature_id() const noexcept { return active_feature_id_; }
    [[nodiscard]] bool is_in_overview_state() const noexcept {
        return shift_overview_held_ || (active_feature_id_ < 0);
    }

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "IdeamCharacterFeatureGizmo"; }
    [[nodiscard]] int32_t get_priority() const override { return 7; }

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

    bool on_modifier_state_changed(
        godot::Node3D *p_target,
        godot::BitField<godot::KeyModifierMask> p_modifiers
    ) override;

    bool on_handle_clicked(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        godot::BitField<godot::KeyModifierMask> p_modifiers
    ) override;

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
