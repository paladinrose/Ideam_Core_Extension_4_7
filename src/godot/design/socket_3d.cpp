#include "socket_3d.h"
#include "../../core/design/structure_layout_solver.h"
#include <algorithm>

namespace ideam::godot_ext {

void Socket3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_compatibility_mask", "mask"), &Socket3D::set_compatibility_mask);
    ClassDB::bind_method(D_METHOD("get_compatibility_mask"), &Socket3D::get_compatibility_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "compatibility_mask"), "set_compatibility_mask", "get_compatibility_mask");

    ClassDB::bind_method(D_METHOD("set_clearance_extents", "extents"), &Socket3D::set_clearance_extents);
    ClassDB::bind_method(D_METHOD("get_clearance_extents"), &Socket3D::get_clearance_extents);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "clearance_extents"), "set_clearance_extents", "get_clearance_extents");

    ClassDB::bind_method(D_METHOD("set_socket_normal", "normal"), &Socket3D::set_socket_normal);
    ClassDB::bind_method(D_METHOD("get_socket_normal"), &Socket3D::get_socket_normal);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "socket_normal"), "set_socket_normal", "get_socket_normal");

    ClassDB::bind_method(D_METHOD("set_socket_tangent", "tangent"), &Socket3D::set_socket_tangent);
    ClassDB::bind_method(D_METHOD("get_socket_tangent"), &Socket3D::get_socket_tangent);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "socket_tangent"), "set_socket_tangent", "get_socket_tangent");

    ClassDB::bind_method(D_METHOD("set_physics_binding_mode", "mode"), &Socket3D::set_physics_binding_mode);
    ClassDB::bind_method(D_METHOD("get_physics_binding_mode"), &Socket3D::get_physics_binding_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "physics_binding_mode",
            PROPERTY_HINT_ENUM,
            "None:0,StaticWeld:1,PinJoint3D:2,HingeJoint3D:3,SliderJoint3D:4,Generic6DOFJoint3D:5"
        ),
        "set_physics_binding_mode",
        "get_physics_binding_mode"
    );

    ClassDB::bind_method(D_METHOD("set_is_male_plug", "is_male"), &Socket3D::set_is_male_plug);
    ClassDB::bind_method(D_METHOD("get_is_male_plug"), &Socket3D::get_is_male_plug);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_male_plug"), "set_is_male_plug", "get_is_male_plug");

    ClassDB::bind_method(D_METHOD("set_is_occupied", "occupied"), &Socket3D::set_is_occupied);
    ClassDB::bind_method(D_METHOD("get_is_occupied"), &Socket3D::get_is_occupied);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_occupied"), "set_is_occupied", "get_is_occupied");

    ClassDB::bind_method(D_METHOD("set_joint_limit_lower", "lower"), &Socket3D::set_joint_limit_lower);
    ClassDB::bind_method(D_METHOD("get_joint_limit_lower"), &Socket3D::get_joint_limit_lower);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "joint_limit_lower"), "set_joint_limit_lower", "get_joint_limit_lower");

    ClassDB::bind_method(D_METHOD("set_joint_limit_upper", "upper"), &Socket3D::set_joint_limit_upper);
    ClassDB::bind_method(D_METHOD("get_joint_limit_upper"), &Socket3D::get_joint_limit_upper);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "joint_limit_upper"), "set_joint_limit_upper", "get_joint_limit_upper");

    ClassDB::bind_method(D_METHOD("set_mated_socket_path", "path"), &Socket3D::set_mated_socket_path);
    ClassDB::bind_method(D_METHOD("get_mated_socket_path"), &Socket3D::get_mated_socket_path);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "mated_socket_path"), "set_mated_socket_path", "get_mated_socket_path");

    ClassDB::bind_method(D_METHOD("can_mate_with", "other_socket"), &Socket3D::can_mate_with);
    ClassDB::bind_method(D_METHOD("mate_with_socket", "other_socket"), &Socket3D::mate_with_socket);
}

void Socket3D::set_compatibility_mask(int64_t p_mask) {
    compatibility_mask_ = static_cast<uint64_t>(p_mask);
}

void Socket3D::set_clearance_extents(const godot::Vector3 &p_extents) {
    clearance_extents_ = p_extents;
}

void Socket3D::set_socket_normal(const godot::Vector3 &p_normal) {
    socket_normal_ = p_normal.length_squared() > 1e-6f ? p_normal.normalized() : godot::Vector3(0.0f, 1.0f, 0.0f);
}

void Socket3D::set_socket_tangent(const godot::Vector3 &p_tangent) {
    socket_tangent_ = p_tangent.length_squared() > 1e-6f ? p_tangent.normalized() : godot::Vector3(1.0f, 0.0f, 0.0f);
}

void Socket3D::set_physics_binding_mode(int p_mode) {
    physics_mode_ = static_cast<core::SocketPhysicsBindingMode>(std::clamp(p_mode, 0, 5));
}

bool Socket3D::can_mate_with(const Socket3D *p_other) const noexcept {
    if (!p_other || p_other == this) {
        return false;
    }
    const core::SocketPod a = to_socket_pod(1u);
    const core::SocketPod b = p_other->to_socket_pod(2u);
    return core::StructureLayoutSolver::are_sockets_compatible(a, b);
}

bool Socket3D::mate_with_socket(Socket3D *p_other) {
    if (!can_mate_with(p_other)) {
        return false;
    }
    is_occupied_ = true;
    p_other->set_is_occupied(true);
    if (auto *parent_node = godot::Object::cast_to<godot::Node3D>(get_parent())) {
        if (auto *other_parent = godot::Object::cast_to<godot::Node3D>(p_other->get_parent())) {
            const godot::Vector3 target_pos = other_parent->get_position() + p_other->get_position() - get_position();
            parent_node->set_position(target_pos);
        }
    }
    return true;
}

core::SocketPod Socket3D::to_socket_pod(uint32_t p_owner_node_id) const {
    core::SocketPod pod{};
    const godot::Vector3 pos   = get_position();
    pod.owner_node_id          = p_owner_node_id;
    pod.compatibility_bitmask  = compatibility_mask_;
    pod.position               = glm::vec3(pos.x, pos.y, pos.z);
    pod.normal                 = glm::vec3(socket_normal_.x, socket_normal_.y, socket_normal_.z);
    pod.tangent                = glm::vec3(socket_tangent_.x, socket_tangent_.y, socket_tangent_.z);
    pod.clearance_half_extents = glm::vec3(clearance_extents_.x, clearance_extents_.y, clearance_extents_.z);
    pod.joint_limit_lower      = joint_limit_lower_;
    pod.joint_limit_upper      = joint_limit_upper_;
    pod.physics_binding_mode   = physics_mode_;
    pod.is_male_plug           = is_male_plug_ ? 1u : 0u;
    pod.is_occupied            = is_occupied_ ? 1u : 0u;
    return pod;
}

} // namespace ideam::godot_ext
