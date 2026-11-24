#pragma once

/**
 * socket_3d.h
 * 3D connection anchor (`Socket3D`) with clearance AABB, compatibility bitmask,
 * polarity & normal/tangent alignment, automatic `Joint3D` physics handshake,
 * and trans-environment `Socket3DGizmo` (`IdeamGizmo`) in `src/godot/design/`.
 */

#include "../../core/design/design_common.h"
#include "../gizmos/ideam_gizmo.h"

#include <godot_cpp/classes/joint3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ideam::godot_ext {

class Socket3D : public godot::Node3D {
    GDCLASS(Socket3D, godot::Node3D)

private:
    uint64_t compatibility_mask_                   = 0xFFFFFFFFFFFFFFFFULL;
    godot::Vector3 clearance_extents_              = godot::Vector3(0.25f, 0.25f, 0.25f);
    godot::Vector3 socket_normal_                  = godot::Vector3(0.0f, 1.0f, 0.0f);
    godot::Vector3 socket_tangent_                 = godot::Vector3(1.0f, 0.0f, 0.0f);
    core::SocketPhysicsBindingMode physics_mode_   = core::SocketPhysicsBindingMode::PHYSICS_BIND_NONE;
    bool is_male_plug_                             = false;
    bool is_occupied_                              = false;
    float joint_limit_lower_                       = 0.0f;
    float joint_limit_upper_                       = 0.0f;
    godot::NodePath mated_socket_path_;
    uint64_t spawned_joint_id_                     = 0u;

    godot::Joint3D *_spawn_physics_joint(Socket3D *p_other);

protected:
    static void _bind_methods();

public:
    Socket3D() = default;
    ~Socket3D() override = default;

    void set_compatibility_mask(int64_t p_mask);
    [[nodiscard]] int64_t get_compatibility_mask() const noexcept { return static_cast<int64_t>(compatibility_mask_); }

    void set_clearance_extents(const godot::Vector3 &p_extents);
    [[nodiscard]] godot::Vector3 get_clearance_extents() const noexcept { return clearance_extents_; }

    void set_socket_normal(const godot::Vector3 &p_normal);
    [[nodiscard]] godot::Vector3 get_socket_normal() const noexcept { return socket_normal_; }

    void set_socket_tangent(const godot::Vector3 &p_tangent);
    [[nodiscard]] godot::Vector3 get_socket_tangent() const noexcept { return socket_tangent_; }

    void set_physics_binding_mode(int p_mode);
    [[nodiscard]] int get_physics_binding_mode() const noexcept { return static_cast<int>(physics_mode_); }

    void set_is_male_plug(bool p_male);
    [[nodiscard]] bool get_is_male_plug() const noexcept { return is_male_plug_; }

    void set_is_occupied(bool p_occupied);
    [[nodiscard]] bool get_is_occupied() const noexcept { return is_occupied_; }

    void set_joint_limit_lower(float p_lower) { joint_limit_lower_ = p_lower; }
    [[nodiscard]] float get_joint_limit_lower() const noexcept { return joint_limit_lower_; }

    void set_joint_limit_upper(float p_upper) { joint_limit_upper_ = p_upper; }
    [[nodiscard]] float get_joint_limit_upper() const noexcept { return joint_limit_upper_; }

    void set_mated_socket_path(const godot::NodePath &p_path) { mated_socket_path_ = p_path; }
    [[nodiscard]] godot::NodePath get_mated_socket_path() const { return mated_socket_path_; }

    [[nodiscard]] godot::Joint3D *get_spawned_joint() const;

    /**
     * Checks polarity (`is_male_plug`), occupancy, compatibility bitmask, and clearance bounds.
     */
    [[nodiscard]] bool can_mate_with(const Socket3D *p_other) const noexcept;

    /**
     * Validates compatibility & clearance, aligns parent transform so opposing normals and
     * tangents meet flush, records `mated_socket_path`, and spawns the configured `Joint3D`.
     */
    bool mate_with_socket(Socket3D *p_other);

    /**
     * Releases occupancy, clears `mated_socket_path`, and removes any spawned `Joint3D`.
     */
    void unmate_socket();

    /**
     * Searches sibling/scene `Socket3D` nodes within `p_max_distance` for a compatible snap target.
     */
    [[nodiscard]] Socket3D *find_nearest_compatible_socket(real_t p_max_distance = static_cast<real_t>(0.65)) const;

    [[nodiscard]] core::SocketPod to_socket_pod(uint32_t p_owner_node_id = 0u) const;
};

/**
 * Trans-Environment `Socket3DGizmo` (`IdeamGizmo` subclass):
 * Renders male/female polarity rings, `clearance_extents` wireframe bounds,
 * `socket_normal` / `socket_tangent` orientation arrows, and snap-preview links
 * when a compatible `Socket3D` is within mating range.
 */
class Socket3DGizmo : public IdeamGizmo {
    GDCLASS(Socket3DGizmo, IdeamGizmo)

public:
    static constexpr int32_t HANDLE_SOCKET_ORIGIN = 0;
    static constexpr int32_t HANDLE_NORMAL_TIP    = 1;

private:
    godot::Vector3 drag_initial_pos_;
    godot::Vector3 drag_initial_normal_;
    bool dragging_ = false;

protected:
    static void _bind_methods() {}

public:
    Socket3DGizmo() = default;
    ~Socket3DGizmo() override = default;

    [[nodiscard]] bool can_handle(godot::Node3D *p_target) const override;
    [[nodiscard]] godot::String get_gizmo_name() const override { return "IdeamSocket3DGizmo"; }
    [[nodiscard]] int32_t get_priority() const override { return 1; }

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
