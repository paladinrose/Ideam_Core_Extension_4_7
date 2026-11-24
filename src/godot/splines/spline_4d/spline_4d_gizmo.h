#pragma once

/**
 * spline_4d_gizmo.h
 * Trans-Environment `IdeamGizmo` for `SplineNode4D` (`src/godot/splines/spline_4d/`).
 *
 * Capabilities (Editor + Runtime):
 *   - Attaches to `SplineNode4D` (and any `Node3D` exposing a `Spline4DResource`).
 *   - Renders 3D spatial `(X, Y, Z)` curve sections + radial W-ring handles oriented
 *     perpendicular to the local Bishop frame so artists can sculpt the 4th dimension (`W`
 *     scale/radius or banking roll angle in radians) in any viewport.
 *   - Includes primary 3D anchor handles, tangent stalks, section midpoint splits,
 *     and 3D AABB rotate/scale handles wired to `IdeamUndoRedo`.
 */

#include "../../gizmos/ideam_gizmo.h"
#include "spline_4d_resource.h"
#include "spline_node4d.h"

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

namespace ideam::godot_ext {

class Spline4DGizmo : public IdeamGizmo {
    GDCLASS(Spline4DGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_ANCHOR_BASE          = 0;
    static constexpr int32_t HANDLE_BACK_OFFSET          = 10000;
    static constexpr int32_t HANDLE_FORE_OFFSET          = 20000;
    static constexpr int32_t HANDLE_SECTION_SPLIT_OFFSET = 30000;
    static constexpr int32_t HANDLE_W_RING_OFFSET        = 40000;
    static constexpr int32_t HANDLE_AABB_SCALE_ID        = 50000;
    static constexpr int32_t HANDLE_AABB_ROTATE_Y_ID     = 50001;

private:
    struct DragState {
        godot::Dictionary initial_snapshot;
        godot::Variant initial_value;
        godot::Vector3 initial_handle_pos = godot::Vector3(0.0f, 0.0f, 0.0f);
        godot::AABB initial_aabb;
        bool active = false;
    };

    mutable godot::HashMap<uint64_t, DragState> active_drags_;

    [[nodiscard]] static godot::Ref<Spline4DResource> _resolve_spline_4d(godot::Node3D *p_node);
    [[nodiscard]] static bool _is_w_banking_roll(godot::Node3D *p_node);

    [[nodiscard]] static godot::Basis _compute_anchor_frame_basis(
        const godot::Ref<Spline4DResource> &p_spline,
        int p_point_index,
        const godot::Vector3 &p_up_hint
    );

    [[nodiscard]] static godot::Vector3 _compute_w_ring_handle_pos(
        const godot::Ref<Spline4DResource> &p_spline,
        int p_point_index,
        bool p_w_is_roll,
        const godot::Vector3 &p_up_hint
    );

    [[nodiscard]] static godot::Vector3 _project_ray_to_local_plane(
        const godot::Node3D *p_node,
        const GizmoRay &p_ray,
        const godot::Vector3 &p_plane_origin_local,
        const godot::Vector3 &p_custom_normal_local = godot::Vector3(0.0f, 0.0f, 0.0f)
    );

    [[nodiscard]] static GizmoSemanticRole _role_for_curve_type(
        SplineCurveType p_type,
        bool p_selected
    ) noexcept;

protected:
    static void _bind_methods();

public:
    Spline4DGizmo() = default;
    ~Spline4DGizmo() override = default;

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