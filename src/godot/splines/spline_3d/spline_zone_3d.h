#pragma once

/**
 * spline_zone_3d.h
 * 3D Capture Corridor, AI Patrol Lane, Camera Rail & Vector-Field Swirl Zone Node
 * (`SplineZone3D`, `godot::Node3D`) and 3D Viewport Corridor Gizmo (`SplineCorridorGizmo`,
 * `godot::EditorNode3DGizmoPlugin`) in `src/godot/splines/spline_3d/`.
 *
 * Implements Section 3.3 of the Ideam Core Extension Master Plan:
 *   - Visualizes and queries the `get_closest_point_on_spline` capture radius along a
 *     `Spline3DResource` or `Spline4DResource` (`W` = local corridor radius), modulated by
 *     an optional `Spline1DResource` (`radius_envelope_1d`) and `falloff_curve_1d`.
 *   - Computes 3D flow/swirl velocity fields (`sample_flow_velocity_at_world_pos`) combining
 *     axial tangent speed, radial attraction toward the spline centerline, and azimuthal vortex
 *     swirl around the tangent (`SPL11` `SplineVectorFieldSwirlTransformLogic`).
 *   - `SplineCorridorGizmo` renders 3D corridor tube rings, directional flow chevrons, and
 *     interactive corridor radius / anchor handles wired to `IdeamUndoRedo`.
 */

#include "../spline_1d/spline_1d_resource.h"
#include "../spline_4d/spline_4d_resource.h"
#include "spline_3d_resource.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class SplineZone3D : public godot::Node3D {
    GDCLASS(SplineZone3D, godot::Node3D)

private:
    godot::Ref<Spline3DResource> spline_3d_;
    godot::Ref<Spline4DResource> spline_4d_;
    godot::Ref<Spline1DResource> radius_envelope_1d_;
    godot::Ref<Spline1DResource> falloff_curve_1d_;

    real_t base_corridor_radius_ = static_cast<real_t>(2.0);
    real_t axial_flow_speed_     = static_cast<real_t>(5.0);
    real_t radial_pull_speed_    = static_cast<real_t>(2.0);
    real_t swirl_angular_speed_  = static_cast<real_t>(0.0);
    int search_samples_          = 16;

    void _on_dependency_changed();

protected:
    static void _bind_methods();

public:
    SplineZone3D() = default;
    ~SplineZone3D() override;

    void set_spline_3d(const godot::Ref<Spline3DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline3DResource> get_spline_3d() const { return spline_3d_; }

    void set_spline_4d(const godot::Ref<Spline4DResource> &p_spline);
    [[nodiscard]] godot::Ref<Spline4DResource> get_spline_4d() const { return spline_4d_; }

    void set_radius_envelope_1d(const godot::Ref<Spline1DResource> &p_envelope);
    [[nodiscard]] godot::Ref<Spline1DResource> get_radius_envelope_1d() const { return radius_envelope_1d_; }

    void set_falloff_curve_1d(const godot::Ref<Spline1DResource> &p_falloff);
    [[nodiscard]] godot::Ref<Spline1DResource> get_falloff_curve_1d() const { return falloff_curve_1d_; }

    void set_base_corridor_radius(real_t p_radius);
    [[nodiscard]] real_t get_base_corridor_radius() const noexcept { return base_corridor_radius_; }

    void set_axial_flow_speed(real_t p_speed);
    [[nodiscard]] real_t get_axial_flow_speed() const noexcept { return axial_flow_speed_; }

    void set_radial_pull_speed(real_t p_speed);
    [[nodiscard]] real_t get_radial_pull_speed() const noexcept { return radial_pull_speed_; }

    void set_swirl_angular_speed(real_t p_speed);
    [[nodiscard]] real_t get_swirl_angular_speed() const noexcept { return swirl_angular_speed_; }

    void set_search_samples(int p_samples);
    [[nodiscard]] int get_search_samples() const noexcept { return search_samples_; }

    // ========================================================================
    // 3D Corridor Containment, Projection & Flow-Field Queries
    // ========================================================================

    [[nodiscard]] real_t sample_corridor_radius_at_percent(real_t p_percent) const;
    [[nodiscard]] bool is_world_point_inside(const godot::Vector3 &p_world_pos) const;

    /**
     * Queries the closest point on the corridor spline to `p_world_pos`, returning a Dictionary:
     *   - `"is_inside"` (`bool`)
     *   - `"closest_percent"` (`float`)
     *   - `"closest_world_point"` (`Vector3`)
     *   - `"world_tangent"` (`Vector3`)
     *   - `"distance"` (`float`)
     *   - `"corridor_radius"` (`float`)
     *   - `"falloff_weight"` (`float` in `[0, 1]`)
     */
    [[nodiscard]] godot::Dictionary query_zone_at_world_pos(const godot::Vector3 &p_world_pos) const;

    /**
     * Evaluates the 3D vector-field flow/swirl velocity at `p_world_pos` (`SPL11` bridge).
     */
    [[nodiscard]] godot::Vector3 sample_flow_velocity_at_world_pos(const godot::Vector3 &p_world_pos) const;
};

/**
 * SplineCorridorGizmo (`godot::EditorNode3DGizmoPlugin`)
 * 3D Viewport Gizmo for `SplineZone3D` rendering corridor capture rings, flow chevrons,
 * and interactive radius / anchor handles.
 */
class SplineCorridorGizmo : public godot::EditorNode3DGizmoPlugin {
    GDCLASS(SplineCorridorGizmo, godot::EditorNode3DGizmoPlugin)

public:
    static constexpr int32_t HANDLE_ANCHOR_BASE     = 0;
    static constexpr int32_t HANDLE_CORRIDOR_RADIUS = 50000;

protected:
    static void _bind_methods() {}

public:
    SplineCorridorGizmo();
    ~SplineCorridorGizmo() override = default;

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