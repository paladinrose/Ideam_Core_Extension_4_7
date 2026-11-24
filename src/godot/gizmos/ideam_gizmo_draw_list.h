#pragma once

/**
 * ideam_gizmo_draw_list.h
 * Data-only 3D gizmo command buffer (`IdeamGizmoDrawList`), semantic roles,
 * handle shapes, view state, and ray descriptors (`src/godot/gizmos/`).
 *
 * Domain gizmos emit purely semantic draw commands into `IdeamGizmoDrawList`
 * without knowing whether they are running in:
 *   1. Godot Editor 3D Scene View (`IdeamEditorGizmoAdapter`)
 *   2. Custom Editor 3D SubViewport (`IdeamViewportGizmoBackend`)
 *   3. Runtime 3D Gameplay World (`IdeamViewportGizmoBackend` + diegetic `Theme3DResource`)
 *   4. Runtime Dedicated Avatar Creator / Modeler (`IdeamViewport3DHost`)
 */

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <vector>

namespace ideam::godot_ext {

/**
 * Semantic role identifiers resolved by `IdeamGizmoStyleResolver` against the active
 * `Theme3DResource` (or default editor/studio palette).
 */
enum GizmoSemanticRole : int32_t {
    GIZMO_ROLE_DEFAULT              = 0,
    GIZMO_HANDLE_SCAFFOLD           = 1,
    GIZMO_HANDLE_SPLINE_ANCHOR      = 2,
    GIZMO_HANDLE_SPLINE_TANGENT     = 3,
    GIZMO_HANDLE_FEATURE_MAJOR      = 4,
    GIZMO_HANDLE_FEATURE_DETAIL     = 5,
    GIZMO_HANDLE_SOCKET_MALE        = 6,
    GIZMO_HANDLE_SOCKET_FEMALE      = 7,
    GIZMO_LINE_BONE_LINK            = 8,
    GIZMO_LINE_RULER                = 9,
    GIZMO_LINE_SEAM_HOLE            = 10,
    GIZMO_LINE_CLEARANCE            = 11,
    GIZMO_LINE_AXIS_NORMAL          = 12,
    GIZMO_LINE_AXIS_TANGENT         = 13,
    GIZMO_PLUMB_BALANCED            = 14,
    GIZMO_PLUMB_UNBALANCED          = 15,
    GIZMO_SNAP_PREVIEW              = 16,
    GIZMO_LIMIT_REACHED             = 17,
    GIZMO_LABEL_ANNOTATION          = 18
};

enum GizmoHandleShapeHint : int32_t {
    HANDLE_SHAPE_SPHERE  = 0,
    HANDLE_SHAPE_CUBE    = 1,
    HANDLE_SHAPE_DIAMOND = 2,
    HANDLE_SHAPE_RING    = 3,
    HANDLE_SHAPE_CONE    = 4,
    HANDLE_SHAPE_DISC    = 5
};

enum GizmoStateFlags : uint32_t {
    GIZMO_STATE_NONE              = 0u,
    GIZMO_STATE_HOVER             = 1u << 0,
    GIZMO_STATE_DRAGGING          = 1u << 1,
    GIZMO_STATE_LOCKED            = 1u << 2,
    GIZMO_STATE_ASYMMETRIC_DETAIL = 1u << 3,
    GIZMO_STATE_LIMIT_REACHED     = 1u << 4
};

enum GizmoDepthTestMode : int32_t {
    DEPTH_TEST_OVERLAY        = 0, ///< X-ray / on-top editor overlay (no depth test)
    DEPTH_TEST_DIEGETIC_WORLD = 1  ///< World-integrated depth-tested diegetic rendering
};

struct GizmoRay {
    godot::Vector3 origin;
    godot::Vector3 normal = godot::Vector3(0.0f, 0.0f, -1.0f);
    godot::Vector2 screen_pos;
    godot::Camera3D *camera = nullptr;

    [[nodiscard]] bool is_valid() const noexcept {
        return normal.length_squared() > 1e-8f;
    }
};

struct GizmoViewState {
    godot::Camera3D *camera = nullptr;
    godot::Vector3 camera_position;
    godot::Vector3 camera_forward = godot::Vector3(0.0f, 0.0f, -1.0f);
    godot::Vector2 viewport_size  = godot::Vector2(1280.0f, 720.0f);
    int32_t hovered_handle_id     = -1;
    int32_t active_handle_id      = -1;
    godot::BitField<godot::KeyModifierMask> modifiers = 0;
    bool is_editor_context        = false;
    godot::StringName active_tool_mode;
};

struct GizmoLineBatchCommand {
    godot::PackedVector3Array points; ///< Pairs of vertices (PRIMITIVE_LINES)
    GizmoSemanticRole role = GIZMO_ROLE_DEFAULT;
    uint32_t state_flags   = GIZMO_STATE_NONE;
    GizmoDepthTestMode depth_test = DEPTH_TEST_OVERLAY;
};

struct GizmoMeshCommand {
    godot::Ref<godot::Mesh> mesh;
    godot::Transform3D transform;
    GizmoSemanticRole role = GIZMO_ROLE_DEFAULT;
    uint32_t state_flags   = GIZMO_STATE_NONE;
    GizmoDepthTestMode depth_test = DEPTH_TEST_OVERLAY;
};

struct GizmoHandleCommand {
    int32_t id = 0;
    godot::Vector3 position;
    GizmoSemanticRole role         = GIZMO_HANDLE_SCAFFOLD;
    GizmoHandleShapeHint shape     = HANDLE_SHAPE_SPHERE;
    real_t radius                  = static_cast<real_t>(0.045);
    uint32_t state_flags           = GIZMO_STATE_NONE;
    bool secondary                 = false;
    godot::String label;
    GizmoDepthTestMode depth_test  = DEPTH_TEST_OVERLAY;
};

struct GizmoLabel3DCommand {
    godot::Vector3 position;
    godot::String text;
    GizmoSemanticRole role = GIZMO_LABEL_ANNOTATION;
    uint32_t state_flags   = GIZMO_STATE_NONE;
};

/**
 * Pure command buffer filled by `IdeamGizmo::build_draw_list()` in target-local coordinates.
 */
class IdeamGizmoDrawList {
private:
    GizmoDepthTestMode current_depth_mode_ = DEPTH_TEST_OVERLAY;
    std::vector<GizmoLineBatchCommand> line_batches_;
    std::vector<GizmoMeshCommand> meshes_;
    std::vector<GizmoHandleCommand> handles_;
    std::vector<GizmoLabel3DCommand> labels_;

public:
    IdeamGizmoDrawList() = default;
    ~IdeamGizmoDrawList() = default;

    void clear();

    void set_depth_test_mode(GizmoDepthTestMode p_mode) noexcept { current_depth_mode_ = p_mode; }
    [[nodiscard]] GizmoDepthTestMode get_depth_test_mode() const noexcept { return current_depth_mode_; }

    void add_lines(
        const godot::PackedVector3Array &p_pairs,
        GizmoSemanticRole p_role = GIZMO_ROLE_DEFAULT,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    );

    void add_segment(
        const godot::Vector3 &p_a,
        const godot::Vector3 &p_b,
        GizmoSemanticRole p_role = GIZMO_ROLE_DEFAULT,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    );

    void add_polyline(
        const godot::PackedVector3Array &p_points,
        bool p_closed = false,
        GizmoSemanticRole p_role = GIZMO_ROLE_DEFAULT,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    );

    void add_circle(
        const godot::Vector3 &p_center,
        const godot::Vector3 &p_normal,
        real_t p_radius,
        int p_segments = 24,
        GizmoSemanticRole p_role = GIZMO_ROLE_DEFAULT,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    );

    void add_wire_box(
        const godot::Vector3 &p_center,
        const godot::Vector3 &p_half_extents,
        const godot::Basis &p_basis = godot::Basis(),
        GizmoSemanticRole p_role = GIZMO_LINE_CLEARANCE,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    );

    void add_cone_arrow(
        const godot::Vector3 &p_origin,
        const godot::Vector3 &p_direction,
        real_t p_length = static_cast<real_t>(0.25),
        real_t p_head_radius = static_cast<real_t>(0.03),
        GizmoSemanticRole p_role = GIZMO_LINE_AXIS_NORMAL,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    );

    void add_mesh(
        const godot::Ref<godot::Mesh> &p_mesh,
        const godot::Transform3D &p_transform = godot::Transform3D(),
        GizmoSemanticRole p_role = GIZMO_ROLE_DEFAULT,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    );

    void add_handle(
        int32_t p_id,
        const godot::Vector3 &p_position,
        GizmoSemanticRole p_role = GIZMO_HANDLE_SCAFFOLD,
        GizmoHandleShapeHint p_shape = HANDLE_SHAPE_SPHERE,
        real_t p_radius = static_cast<real_t>(0.045),
        uint32_t p_state_flags = GIZMO_STATE_NONE,
        const godot::String &p_label = godot::String(),
        bool p_secondary = false
    );

    void add_label_3d(
        const godot::Vector3 &p_position,
        const godot::String &p_text,
        GizmoSemanticRole p_role = GIZMO_LABEL_ANNOTATION,
        uint32_t p_state_flags = GIZMO_STATE_NONE
    );

    [[nodiscard]] const std::vector<GizmoLineBatchCommand> &line_batches() const noexcept { return line_batches_; }
    [[nodiscard]] const std::vector<GizmoMeshCommand> &meshes() const noexcept { return meshes_; }
    [[nodiscard]] const std::vector<GizmoHandleCommand> &handles() const noexcept { return handles_; }
    [[nodiscard]] const std::vector<GizmoLabel3DCommand> &labels() const noexcept { return labels_; }
    [[nodiscard]] bool is_empty() const noexcept {
        return line_batches_.empty() && meshes_.empty() && handles_.empty() && labels_.empty();
    }
};

} // namespace ideam::godot_ext
