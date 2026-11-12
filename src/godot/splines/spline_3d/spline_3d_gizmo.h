#pragma once

/**
 * spline_3d_gizmo.h
 * 3D Viewport EditorNode3DGizmoPlugin for `SplineNode3D` (`src/godot/splines/spline_3d/`).
 *
 * Capabilities:
 *   - Curve-type color-coded 3D path rendering (`Linear`, `Catmull-Rom`, `Cubic Bezier`, `Step`)
 *     with highlighted active section and optional twist-free Bishop / Up-Vector coordinate frames.
 *   - Primary 3D anchor handles (`0 .. N-1`) for interactive 3D point translation.
 *   - Secondary tangent stalk handles (`back_handle` and `fore_handle`) for Bezier/Catmull-Rom sculpting.
 *   - Section midpoint split handles (`+`) that subdivide a section and insert a new 3D anchor on commit.
 *   - Wireframe 3D AABB bounding box with dedicated Uniform/Axis Scale and Y-Axis Rotation handles.
 *   - Full `IdeamUndoRedo` snapshot and method-commit integration via `IdeamSplinesPlugin`.
 */

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo.hpp>
#include <godot_cpp/classes/editor_node3d_gizmo_plugin.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include "spline_3d_resource.h"
#include "spline_node3d.h"

namespace ideam::godot_ext {

class Spline3DGizmo : public godot::EditorNode3DGizmoPlugin {
    GDCLASS(Spline3DGizmo, godot::EditorNode3DGizmoPlugin)

public:
    // Handle ID ranges encoded in a single 32-bit integer for O(1) decoding
    static constexpr int32_t HANDLE_ANCHOR_BASE          = 0;
    static constexpr int32_t HANDLE_BACK_OFFSET          = 10000;
    static constexpr int32_t HANDLE_FORE_OFFSET          = 20000;
    static constexpr int32_t HANDLE_SECTION_SPLIT_OFFSET = 30000;
    static constexpr int32_t HANDLE_AABB_SCALE_ID        = 40000;
    static constexpr int32_t HANDLE_AABB_ROTATE_Y_ID     = 40001;

private:
    struct DragState {
        godot::Dictionary initial_snapshot;
        godot::Vector3 initial_handle_pos = godot::Vector3(0.0f, 0.0f, 0.0f);
        godot::AABB initial_aabb;
        bool active = false;
    };

    mutable godot::HashMap<uint64_t, DragState> active_drags;

    void _init_gizmo_materials();

    [[nodiscard]] static godot::Vector3 _project_screen_to_local_plane(
        const SplineNode3D *p_node,
        godot::Camera3D *p_camera,
        const godot::Vector2 &p_screen_pos,
        const godot::Vector3 &p_plane_origin_local
    );

    [[nodiscard]] godot::Ref<godot::StandardMaterial3D> _get_curve_material(
        SplineCurveType p_type,
        bool p_selected,
        const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo
    );

protected:
    static void _bind_methods();

public:
    Spline3DGizmo();
    ~Spline3DGizmo() override;

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