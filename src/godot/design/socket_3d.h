#pragma once

/**
 * socket_3d.h
 * 3D connection anchor (`Socket3D`) with clearance AABB and compatibility bitmask
 * (`src/godot/design/`).
 */

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include "../../core/design/design_common.h"

namespace ideam::godot_ext {

class Socket3D : public godot::Node3D {
    GDCLASS(Socket3D, godot::Node3D)

private:
    uint64_t compatibility_mask_     = 0xFFFFFFFFFFFFFFFFULL;
    godot::Vector3 clearance_extents_= godot::Vector3(0.25f, 0.25f, 0.25f);

protected:
    static void _bind_methods();

public:
    Socket3D() = default;
    ~Socket3D() override = default;

    void set_compatibility_mask(int64_t p_mask);
    [[nodiscard]] int64_t get_compatibility_mask() const { return static_cast<int64_t>(compatibility_mask_); }

    void set_clearance_extents(const godot::Vector3 &p_extents);
    [[nodiscard]] godot::Vector3 get_clearance_extents() const { return clearance_extents_; }

    [[nodiscard]] core::SocketPod to_socket_pod() const;
};

} // namespace ideam::godot_ext
