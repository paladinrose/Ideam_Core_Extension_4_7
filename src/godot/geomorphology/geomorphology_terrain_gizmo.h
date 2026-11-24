#pragma once

/**
 * geomorphology_terrain_gizmo.h
 * Trans-Environment `IdeamGizmo` for `GeomorphologyTerrainNode3D` (`src/godot/geomorphology/`).
 *
 * Renders terrain grid footprint bounds, sampled elevation contour gridlines,
 * and an interactive cell-size scale handle with `IdeamUndoRedo` support.
 */

#include "../gizmos/ideam_gizmo.h"
#include "geomorphology_terrain_resource.h"

#include <godot_cpp/templates/hash_map.hpp>

namespace ideam::godot_ext {

class GeomorphologyTerrainGizmo : public IdeamGizmo {
    GDCLASS(GeomorphologyTerrainGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_CELL_SIZE_ID = 0;

private:
    mutable godot::HashMap<uint64_t, float> drag_initial_cell_size_;

protected:
    static void _bind_methods() {}

public:
    GeomorphologyTerrainGizmo() = default;
    ~GeomorphologyTerrainGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "GeomorphologyTerrainGizmo"; }
    [[nodiscard]] int32_t get_priority() const override { return 0; }

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

    void on_handle_drag_begin(godot::Node3D *p_target, int32_t p_handle_id, const GizmoRay &p_ray) override;
    void on_handle_drag_update(
        godot::Node3D *p_target,
        int32_t p_handle_id,
        const GizmoRay &p_ray,
        godot::BitField<godot::KeyModifierMask> p_modifiers
    ) override;
    void on_handle_drag_commit(godot::Node3D *p_target, int32_t p_handle_id, bool p_cancel) override;
};

} // namespace ideam::godot_ext
