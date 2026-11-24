#pragma once

/**
 * geometry_projector_gizmo.h
 * Trans-Environment `IdeamGizmo` for `Geometry2DProjector3D` (`src/godot/geometry/`).
 *
 * Renders projection radius rings, raycast height rays, and interactive handles
 * for `projection_radius` and `ray_origin_height` with `IdeamUndoRedo` support.
 */

#include "../gizmos/ideam_gizmo.h"
#include "geometry_2d_projector_3d.h"

#include <godot_cpp/templates/hash_map.hpp>

namespace ideam::godot_ext {

class GeometryProjectorGizmo : public IdeamGizmo {
    GDCLASS(GeometryProjectorGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_PROJECTION_RADIUS = 0;
    static constexpr int32_t HANDLE_RAY_ORIGIN_HEIGHT = 1;

private:
    mutable godot::HashMap<uint64_t, real_t> drag_restore_;

protected:
    static void _bind_methods() {}

public:
    GeometryProjectorGizmo() = default;
    ~GeometryProjectorGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "GeometryProjectorGizmo"; }
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
