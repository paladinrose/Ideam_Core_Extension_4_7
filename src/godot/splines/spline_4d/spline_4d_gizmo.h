#pragma once

/**
 * spline_4d_gizmo.h
 * 3D+W Viewport EditorNode3DGizmoPlugin for `SplineNode4D` (`src/godot/splines/spline_4d/`).
 *
 * Capabilities:
 *   - Attaches to `SplineNode4D` (and any `Node3D` exposing a `Spline4DResource`).
 *   - Renders the 3D spatial `(X, Y, Z)` path with either curve-type color coding or
 *     per-segment RGBA / HSVA color gradient tinting when `hsva_color_mode` or
 *     `SPLINE_4D_W_COLOR_RGBA` is active.
 *   - Adds **radial $W$-ring handles** around each 3D anchor (oriented perpendicular to the
 *     local 3D curve tangent via Bishop frames) so artists can sculpt the 4th dimension (`W`
 *     scale/radius or banking roll angle in radians) directly in the 3D viewport!
 *   - Includes primary 3D anchor handles, `back_handle`/`fore_handle` 3D tangent stalks,
 *     section midpoint split handles, and 3D AABB rotate/scale handles wired to `IdeamUndoRedo`.
 */

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include "spline_4d_resource.h"
#include "spline_node4d.h"

namespace ideam::godot_ext {

class Spline4DGizmo : public godot::EditorNode3DGizmoPlugin {
    GDCLASS(Spline4DGizmo, godot::EditorNode3DGizmoPlugin)

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
        godot::Vector3 initial_handle_pos = godot::Vector3(0.0f, 0.0f, 0.0f);
        godot::AABB initial_aabb;
        bool active = false;
    };

    mutable godot::HashMap<uint64_t, DragState> active_drags;

    void _init_gizmo_materials();

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

    [[nodiscard]] static godot::Vector3 _project_screen_to_local_plane(
        const godot::Node3D *p_node,
        godot::Camera3D *p_camera,
        const godot::Vector2 &p_screen_pos,
        const godot::Vector3 &p_plane_origin_local,
        const godot::Vector3 &p_custom_normal_local = godot::Vector3(0.0f, 0.0f, 0.0f)
    );

    [[nodiscard]] godot::Ref<godot::StandardMaterial3D> _get_curve_material(
        SplineCurveType p_type,
        bool p_selected,
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo
    );

protected:
    static void _bind_methods();

public:
    Spline4DGizmo();
    ~Spline4DGizmo() override;

    // --- EditorNode3DGizmoPlugin Virtual Overrides ---
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