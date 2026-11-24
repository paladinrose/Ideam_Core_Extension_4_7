#pragma once

/**
 * spline_3d_gizmo.h
 * Trans-Environment `IdeamGizmo` for `SplineNode3D` (`src/godot/splines/spline_3d/`).
 *
 * Works identically in Godot Editor 3D Viewport, Custom SubViewports, and Runtime 3D Worlds:
 *   - Semantic curve path rendering with highlighted active section and optional twist-free
 *     Bishop / Up-Vector coordinate frames.
 *   - Primary 3D anchor handles (`0 .. N-1`) for interactive 3D point translation.
 *   - Secondary tangent stalk handles (`back_handle` and `fore_handle`) for Bezier/Catmull-Rom sculpting.
 *   - Section midpoint split handles (`+`) that subdivide a section and insert a new 3D anchor on commit.
 *   - Wireframe 3D AABB bounding box with dedicated Scale and Y-Axis Rotation handles.
 *   - Full `IdeamUndoRedo` snapshot and method-commit integration via `ToolContext` / `IdeamSplinesPlugin`.
 */

#include "../../gizmos/ideam_gizmo.h"
#include "spline_3d_resource.h"
#include "spline_node3d.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class Spline3DGizmo : public IdeamGizmo {
    GDCLASS(Spline3DGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_ANCHOR_BASE          = 0;
    static constexpr int32_t HANDLE_BACK_OFFSET          = 10000;
    static constexpr int32_t HANDLE_FORE_OFFSET          = 20000;
    static constexpr int32_t HANDLE_SECTION_SPLIT_OFFSET = 30000;
    static constexpr int32_t HANDLE_AABB_SCALE_ID        = 40000;
    static constexpr int32_t HANDLE_AABB_ROTATE_Y_ID     = 40001;

private:
    struct DragState {
        godot::Dictionary initial_snapshot;
        godot::Variant initial_value;
        godot::Vector3 initial_handle_pos = godot::Vector3(0.0f, 0.0f, 0.0f);
        godot::AABB initial_aabb;
        bool active = false;
    };

    mutable godot::HashMap<uint64_t, DragState> active_drags_;

    [[nodiscard]] static godot::Vector3 _project_ray_to_local_plane(
        const SplineNode3D *p_node,
        const GizmoRay &p_ray,
        const godot::Vector3 &p_plane_origin_local
    );

    [[nodiscard]] static GizmoSemanticRole _role_for_curve_type(
        SplineCurveType p_type,
        bool p_selected
    ) noexcept;

protected:
    static void _bind_methods();

public:
    Spline3DGizmo() = default;
    ~Spline3DGizmo() override = default;

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