#pragma once

/**
 * socket_3d.h
 * 3D connection anchor (`Socket3D`) with clearance AABB, compatibility bitmask,
 * and automatic `Joint3D` physics handshake (`src/godot/design/`).
 */

#include "../../core/design/design_common.h"
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

    void set_is_male_plug(bool p_male) { is_male_plug_ = p_male; }
    [[nodiscard]] bool get_is_male_plug() const noexcept { return is_male_plug_; }

    void set_is_occupied(bool p_occupied) { is_occupied_ = p_occupied; }
    [[nodiscard]] bool get_is_occupied() const noexcept { return is_occupied_; }

    void set_joint_limit_lower(float p_lower) { joint_limit_lower_ = p_lower; }
    [[nodiscard]] float get_joint_limit_lower() const noexcept { return joint_limit_lower_; }

    void set_joint_limit_upper(float p_upper) { joint_limit_upper_ = p_upper; }
    [[nodiscard]] float get_joint_limit_upper() const noexcept { return joint_limit_upper_; }

    void set_mated_socket_path(const godot::NodePath &p_path) { mated_socket_path_ = p_path; }
    [[nodiscard]] godot::NodePath get_mated_socket_path() const { return mated_socket_path_; }

    [[nodiscard]] bool can_mate_with(const Socket3D *p_other) const noexcept;
    bool mate_with_socket(Socket3D *p_other);

    [[nodiscard]] core::SocketPod to_socket_pod(uint32_t p_owner_node_id = 0u) const;
};

} // namespace ideam::godot_ext
