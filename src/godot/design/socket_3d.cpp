#include "socket_3d.h"

namespace ideam::godot_ext {

void Socket3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_compatibility_mask", "mask"), &Socket3D::set_compatibility_mask);
    ClassDB::bind_method(D_METHOD("get_compatibility_mask"), &Socket3D::get_compatibility_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "compatibility_mask"), "set_compatibility_mask", "get_compatibility_mask");

    ClassDB::bind_method(D_METHOD("set_clearance_extents", "extents"), &Socket3D::set_clearance_extents);
    ClassDB::bind_method(D_METHOD("get_clearance_extents"), &Socket3D::get_clearance_extents);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "clearance_extents"), "set_clearance_extents", "get_clearance_extents");
}

void Socket3D::set_compatibility_mask(int64_t p_mask) {
    compatibility_mask_ = static_cast<uint64_t>(p_mask);
}

void Socket3D::set_clearance_extents(const godot::Vector3 &p_extents) {
    clearance_extents_ = p_extents;
}

core::SocketPod Socket3D::to_socket_pod() const {
    core::SocketPod pod{};
    const godot::Vector3 pos = get_position();
    pod.compatibility_bitmask  = compatibility_mask_;
    pod.position               = glm::vec3(pos.x, pos.y, pos.z);
    pod.clearance_half_extents = glm::vec3(clearance_extents_.x, clearance_extents_.y, clearance_extents_.z);
    return pod;
}

} // namespace ideam::godot_ext
