#include "socket_3d.h"

#include "../../core/design/structure_layout_solver.h"

#include <godot_cpp/classes/generic6_dof_joint3d.hpp>
#include <godot_cpp/classes/hinge_joint3d.hpp>
#include <godot_cpp/classes/physics_body3d.hpp>
#include <godot_cpp/classes/pin_joint3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/slider_joint3d.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/plane.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace ideam::godot_ext {

namespace {

godot::Basis build_socket_frame_basis(const godot::Vector3 &p_normal, const godot::Vector3 &p_tangent) {
    const godot::Vector3 y_axis = p_normal.length_squared() > 1e-6f
        ? p_normal.normalized()
        : godot::Vector3(0.0f, 1.0f, 0.0f);

    godot::Vector3 t = p_tangent - y_axis * p_tangent.dot(y_axis);
    if (t.length_squared() <= 1e-6f) {
        const godot::Vector3 ref = (std::abs(y_axis.x) < 0.9f)
            ? godot::Vector3(1.0f, 0.0f, 0.0f)
            : godot::Vector3(0.0f, 0.0f, 1.0f);
        t = ref - y_axis * ref.dot(y_axis);
    }
    const godot::Vector3 x_axis = t.normalized();
    const godot::Vector3 z_axis = x_axis.cross(y_axis).normalized();
    return godot::Basis(x_axis, y_axis, z_axis);
}

godot::PhysicsBody3D *find_enclosing_physics_body(godot::Node *p_start) {
    godot::Node *cur = p_start;
    while (cur) {
        if (auto *body = godot::Object::cast_to<godot::PhysicsBody3D>(cur)) {
            return body;
        }
        cur = cur->get_parent();
    }
    return nullptr;
}

void collect_sockets_recursive(const godot::Node *p_root, std::vector<Socket3D *> &r_out) {
    if (!p_root) return;
    if (auto *sock = godot::Object::cast_to<Socket3D>(const_cast<godot::Node *>(p_root))) {
        r_out.push_back(sock);
    }
    const int n = p_root->get_child_count();
    for (int i = 0; i < n; ++i) {
        collect_sockets_recursive(p_root->get_child(i), r_out);
    }
}

} // namespace

// ============================================================================
// Socket3D Implementation
// ============================================================================

void Socket3D::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo("socket_mated", PropertyInfo(Variant::OBJECT, "other_socket", PROPERTY_HINT_NODE_TYPE, "Socket3D")));
    ADD_SIGNAL(MethodInfo("socket_unmated"));

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

    ClassDB::bind_method(D_METHOD("get_spawned_joint"), &Socket3D::get_spawned_joint);
    ClassDB::bind_method(D_METHOD("can_mate_with", "other_socket"), &Socket3D::can_mate_with);
    ClassDB::bind_method(D_METHOD("mate_with_socket", "other_socket"), &Socket3D::mate_with_socket);
    ClassDB::bind_method(D_METHOD("unmate_socket"), &Socket3D::unmate_socket);
    ClassDB::bind_method(
        D_METHOD("find_nearest_compatible_socket", "max_distance"),
        &Socket3D::find_nearest_compatible_socket,
        DEFVAL(0.65)
    );
}

void Socket3D::set_compatibility_mask(int64_t p_mask) {
    compatibility_mask_ = static_cast<uint64_t>(p_mask);
}

void Socket3D::set_clearance_extents(const godot::Vector3 &p_extents) {
    clearance_extents_ = godot::Vector3(
        std::max(static_cast<real_t>(0.01), std::abs(p_extents.x)),
        std::max(static_cast<real_t>(0.01), std::abs(p_extents.y)),
        std::max(static_cast<real_t>(0.01), std::abs(p_extents.z))
    );
    update_gizmos();
}

void Socket3D::set_socket_normal(const godot::Vector3 &p_normal) {
    socket_normal_ = p_normal.length_squared() > 1e-6f ? p_normal.normalized() : godot::Vector3(0.0f, 1.0f, 0.0f);
    update_gizmos();
}

void Socket3D::set_socket_tangent(const godot::Vector3 &p_tangent) {
    socket_tangent_ = p_tangent.length_squared() > 1e-6f ? p_tangent.normalized() : godot::Vector3(1.0f, 0.0f, 0.0f);
    update_gizmos();
}

void Socket3D::set_physics_binding_mode(int p_mode) {
    physics_mode_ = static_cast<core::SocketPhysicsBindingMode>(std::clamp(p_mode, 0, 5));
}

void Socket3D::set_is_male_plug(bool p_male) {
    is_male_plug_ = p_male;
    update_gizmos();
}

void Socket3D::set_is_occupied(bool p_occupied) {
    is_occupied_ = p_occupied;
    update_gizmos();
}

godot::Joint3D *Socket3D::get_spawned_joint() const {
    if (spawned_joint_id_ == 0u) return nullptr;
    return godot::Object::cast_to<godot::Joint3D>(godot::ObjectDB::get_instance(spawned_joint_id_));
}

bool Socket3D::can_mate_with(const Socket3D *p_other) const noexcept {
    if (!p_other || p_other == this) {
        return false;
    }
    if (is_occupied_ || p_other->get_is_occupied()) {
        return false;
    }
    // Enforce opposite polarity (male plug <-> female receptacle)
    if (is_male_plug_ == p_other->get_is_male_plug()) {
        return false;
    }
    // Validate positive clearance extents on both sockets
    if (clearance_extents_.x <= 0.0f || clearance_extents_.y <= 0.0f || clearance_extents_.z <= 0.0f ||
        p_other->clearance_extents_.x <= 0.0f || p_other->clearance_extents_.y <= 0.0f || p_other->clearance_extents_.z <= 0.0f) {
        return false;
    }
    const core::SocketPod a = to_socket_pod(1u);
    const core::SocketPod b = p_other->to_socket_pod(2u);
    return core::StructureLayoutSolver::are_sockets_compatible(a, b);
}

godot::Joint3D *Socket3D::_spawn_physics_joint(Socket3D *p_other) {
    if (!p_other || physics_mode_ == core::SocketPhysicsBindingMode::PHYSICS_BIND_NONE) {
        return nullptr;
    }

    godot::Joint3D *joint = nullptr;
    switch (physics_mode_) {
        case core::SocketPhysicsBindingMode::PHYSICS_BIND_PIN_JOINT_3D: {
            joint = memnew(godot::PinJoint3D);
            joint->set_name(godot::String(get_name()) + godot::String("_PinJoint3D"));
            break;
        }
        case core::SocketPhysicsBindingMode::PHYSICS_BIND_HINGE_JOINT_3D: {
            auto *hinge = memnew(godot::HingeJoint3D);
            hinge->set_name(godot::String(get_name()) + godot::String("_HingeJoint3D"));
            if (joint_limit_upper_ > joint_limit_lower_) {
                hinge->set_flag(godot::HingeJoint3D::FLAG_USE_LIMIT, true);
                hinge->set_param(godot::HingeJoint3D::PARAM_LIMIT_LOWER, godot::Math::deg_to_rad(joint_limit_lower_));
                hinge->set_param(godot::HingeJoint3D::PARAM_LIMIT_UPPER, godot::Math::deg_to_rad(joint_limit_upper_));
            }
            joint = hinge;
            break;
        }
        case core::SocketPhysicsBindingMode::PHYSICS_BIND_SLIDER_JOINT_3D: {
            auto *slider = memnew(godot::SliderJoint3D);
            slider->set_name(godot::String(get_name()) + godot::String("_SliderJoint3D"));
            if (joint_limit_upper_ > joint_limit_lower_) {
                slider->set_param(godot::SliderJoint3D::PARAM_LINEAR_LIMIT_LOWER, joint_limit_lower_);
                slider->set_param(godot::SliderJoint3D::PARAM_LINEAR_LIMIT_UPPER, joint_limit_upper_);
            }
            joint = slider;
            break;
        }
        case core::SocketPhysicsBindingMode::PHYSICS_BIND_STATIC_WELD:
        case core::SocketPhysicsBindingMode::PHYSICS_BIND_GENERIC_6DOF_JOINT_3D: {
            auto *j6 = memnew(godot::Generic6DOFJoint3D);
            j6->set_name(godot::String(get_name()) + godot::String("_Generic6DOFJoint3D"));
            const bool is_weld = (physics_mode_ == core::SocketPhysicsBindingMode::PHYSICS_BIND_STATIC_WELD);
            const float lo_rad = is_weld ? 0.0f : godot::Math::deg_to_rad(joint_limit_lower_);
            const float hi_rad = is_weld ? 0.0f : godot::Math::deg_to_rad(joint_limit_upper_);
            if (is_weld || joint_limit_upper_ > joint_limit_lower_) {
                j6->set_flag_x(godot::Generic6DOFJoint3D::FLAG_ENABLE_ANGULAR_LIMIT, true);
                j6->set_flag_y(godot::Generic6DOFJoint3D::FLAG_ENABLE_ANGULAR_LIMIT, true);
                j6->set_flag_z(godot::Generic6DOFJoint3D::FLAG_ENABLE_ANGULAR_LIMIT, true);
                j6->set_param_x(godot::Generic6DOFJoint3D::PARAM_ANGULAR_LOWER_LIMIT, lo_rad);
                j6->set_param_x(godot::Generic6DOFJoint3D::PARAM_ANGULAR_UPPER_LIMIT, hi_rad);
                j6->set_param_y(godot::Generic6DOFJoint3D::PARAM_ANGULAR_LOWER_LIMIT, lo_rad);
                j6->set_param_y(godot::Generic6DOFJoint3D::PARAM_ANGULAR_UPPER_LIMIT, hi_rad);
                j6->set_param_z(godot::Generic6DOFJoint3D::PARAM_ANGULAR_LOWER_LIMIT, lo_rad);
                j6->set_param_z(godot::Generic6DOFJoint3D::PARAM_ANGULAR_UPPER_LIMIT, hi_rad);
            }
            joint = j6;
            break;
        }
        case core::SocketPhysicsBindingMode::PHYSICS_BIND_NONE:
        default:
            return nullptr;
    }

    if (!joint) return nullptr;
    add_child(joint);
    if (is_inside_tree() && get_tree() && get_tree()->get_edited_scene_root()) {
        joint->set_owner(get_tree()->get_edited_scene_root());
    }

    if (godot::PhysicsBody3D *body_a = find_enclosing_physics_body(get_parent())) {
        if (joint->is_inside_tree() && body_a->is_inside_tree()) {
            joint->set_node_a(joint->get_path_to(body_a));
        }
    }
    if (godot::PhysicsBody3D *body_b = find_enclosing_physics_body(p_other->get_parent())) {
        if (joint->is_inside_tree() && body_b->is_inside_tree()) {
            joint->set_node_b(joint->get_path_to(body_b));
        }
    }

    spawned_joint_id_ = joint->get_instance_id();
    return joint;
}

bool Socket3D::mate_with_socket(Socket3D *p_other) {
    if (!can_mate_with(p_other)) {
        return false;
    }

    // 1. Align parent transform so opposing socket normals face each other and tangents match
    if (auto *parent_node = godot::Object::cast_to<godot::Node3D>(get_parent())) {
        if (p_other->is_inside_tree() && parent_node->is_inside_tree()) {
            const godot::Transform3D other_world = p_other->get_global_transform();
            const godot::Vector3 other_n_world   = other_world.basis.xform(p_other->get_socket_normal()).normalized();
            const godot::Vector3 other_t_world   = other_world.basis.xform(p_other->get_socket_tangent()).normalized();

            // Target socket frame in world space: normal opposes other_n_world, tangent aligns with other_t_world
            const godot::Basis target_socket_world_basis = build_socket_frame_basis(-other_n_world, other_t_world);
            const godot::Basis local_socket_frame        = get_transform().basis * build_socket_frame_basis(socket_normal_, socket_tangent_);
            const godot::Basis new_parent_world_basis    = target_socket_world_basis * local_socket_frame.inverse();

            const godot::Vector3 new_parent_world_pos =
                other_world.origin - new_parent_world_basis.xform(get_position());

            parent_node->set_global_transform(godot::Transform3D(new_parent_world_basis, new_parent_world_pos));
        } else if (auto *other_parent = godot::Object::cast_to<godot::Node3D>(p_other->get_parent())) {
            const godot::Vector3 target_pos = other_parent->get_position() + p_other->get_position() - get_position();
            parent_node->set_position(target_pos);
        }
    }

    // 2. Mark occupied and record mutual mated_socket_path
    is_occupied_ = true;
    p_other->set_is_occupied(true);
    if (is_inside_tree() && p_other->is_inside_tree()) {
        mated_socket_path_ = get_path_to(p_other);
        p_other->set_mated_socket_path(p_other->get_path_to(this));
    }

    // 3. Optionally spawn & configure matching Joint3D
    _spawn_physics_joint(p_other);

    update_gizmos();
    p_other->update_gizmos();
    emit_signal("socket_mated", p_other);
    return true;
}

void Socket3D::unmate_socket() {
    if (godot::Joint3D *j = get_spawned_joint()) {
        if (j->get_parent()) {
            j->get_parent()->remove_child(j);
        }
        memdelete(j);
    }
    spawned_joint_id_ = 0u;

    if (!mated_socket_path_.is_empty() && is_inside_tree()) {
        if (auto *other = godot::Object::cast_to<Socket3D>(get_node_or_null(mated_socket_path_))) {
            other->set_is_occupied(false);
            other->set_mated_socket_path(godot::NodePath());
        }
    }
    mated_socket_path_ = godot::NodePath();
    is_occupied_       = false;
    update_gizmos();
    emit_signal("socket_unmated");
}

Socket3D *Socket3D::find_nearest_compatible_socket(real_t p_max_distance) const {
    if (!is_inside_tree() || !get_tree()) {
        return nullptr;
    }
    godot::Node *search_root = get_tree()->get_edited_scene_root();
    if (!search_root) {
        search_root = get_parent() ? get_parent()->get_parent() : get_parent();
    }
    if (!search_root) {
        return nullptr;
    }

    std::vector<Socket3D *> candidates;
    collect_sockets_recursive(search_root, candidates);

    const godot::Vector3 my_pos = get_global_position();
    real_t best_dist_sq = p_max_distance * p_max_distance;
    Socket3D *best = nullptr;

    for (Socket3D *cand : candidates) {
        if (!cand || cand == this || !can_mate_with(cand)) continue;
        const real_t dsq = my_pos.distance_squared_to(cand->get_global_position());
        if (dsq <= best_dist_sq) {
            best_dist_sq = dsq;
            best = cand;
        }
    }
    return best;
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

// ============================================================================
// Socket3DGizmo Implementation (Trans-Environment `IdeamGizmo`)
// ============================================================================

bool Socket3DGizmo::can_handle(godot::Node3D *p_target) const {
    return godot::Object::cast_to<Socket3D>(p_target) != nullptr;
}

void Socket3DGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    auto *sock = godot::Object::cast_to<Socket3D>(p_target);
    if (!sock) return;

    const GizmoSemanticRole polarity_role = sock->get_is_male_plug()
        ? GIZMO_HANDLE_SOCKET_MALE
        : GIZMO_HANDLE_SOCKET_FEMALE;
    const uint32_t lock_state = sock->get_is_occupied() ? GIZMO_STATE_LOCKED : GIZMO_STATE_NONE;

    const godot::Vector3 n = sock->get_socket_normal();
    const godot::Vector3 t = sock->get_socket_tangent();
    const godot::Vector3 ext = sock->get_clearance_extents();
    const real_t ring_r = std::max(static_cast<real_t>(0.05), std::min(ext.x, ext.z) * static_cast<real_t>(0.55));

    // 1. Male / Female polarity ring(s)
    r_out.add_circle(godot::Vector3(), n, ring_r, 20, polarity_role, lock_state);
    if (sock->get_is_male_plug()) {
        // Inner plug ring
        r_out.add_circle(n * static_cast<real_t>(0.03), n, ring_r * static_cast<real_t>(0.55), 14, polarity_role, lock_state);
    } else {
        // Outer receptacle ring
        r_out.add_circle(godot::Vector3(), n, ring_r * static_cast<real_t>(1.25), 20, polarity_role, lock_state);
    }

    // 2. Clearance extents wireframe AABB
    r_out.add_wire_box(godot::Vector3(), ext, godot::Basis(), GIZMO_LINE_CLEARANCE, lock_state);

    // 3. Normal & tangent orientation arrows
    const real_t normal_len  = std::max(static_cast<real_t>(0.18), ext.y * static_cast<real_t>(1.1));
    const real_t tangent_len = std::max(static_cast<real_t>(0.14), ext.x * static_cast<real_t>(0.9));
    r_out.add_cone_arrow(godot::Vector3(), n, normal_len, static_cast<real_t>(0.025), GIZMO_LINE_AXIS_NORMAL, lock_state);
    r_out.add_cone_arrow(godot::Vector3(), t, tangent_len, static_cast<real_t>(0.020), GIZMO_LINE_AXIS_TANGENT, lock_state);

    // 4. Interactive handles (origin + normal tip)
    uint32_t origin_state = lock_state;
    if (p_view.active_handle_id == HANDLE_SOCKET_ORIGIN) origin_state |= GIZMO_STATE_DRAGGING;
    else if (p_view.hovered_handle_id == HANDLE_SOCKET_ORIGIN) origin_state |= GIZMO_STATE_HOVER;

    r_out.add_handle(
        HANDLE_SOCKET_ORIGIN,
        godot::Vector3(),
        polarity_role,
        sock->get_is_male_plug() ? HANDLE_SHAPE_CONE : HANDLE_SHAPE_RING,
        static_cast<real_t>(0.032),
        origin_state,
        sock->get_is_male_plug() ? "Male Socket" : "Female Socket",
        false
    );

    uint32_t tip_state = lock_state;
    if (p_view.active_handle_id == HANDLE_NORMAL_TIP) tip_state |= GIZMO_STATE_DRAGGING;
    else if (p_view.hovered_handle_id == HANDLE_NORMAL_TIP) tip_state |= GIZMO_STATE_HOVER;

    r_out.add_handle(
        HANDLE_NORMAL_TIP,
        n * normal_len,
        GIZMO_LINE_AXIS_NORMAL,
        HANDLE_SHAPE_DIAMOND,
        static_cast<real_t>(0.024),
        tip_state,
        "Socket Normal",
        true
    );

    // 5. Snap-preview highlight to nearest compatible unoccupied Socket3D
    if (!sock->get_is_occupied() && sock->is_inside_tree()) {
        if (Socket3D *snap = sock->find_nearest_compatible_socket(static_cast<real_t>(0.65))) {
            const godot::Vector3 local_snap = sock->to_local(snap->get_global_position());
            r_out.add_segment(godot::Vector3(), local_snap, GIZMO_SNAP_PREVIEW);
            r_out.add_circle(local_snap, n, ring_r * static_cast<real_t>(1.15), 16, GIZMO_SNAP_PREVIEW);
        }
    }
}

godot::String Socket3DGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_target;
    (void)p_secondary;
    return (p_handle_id == HANDLE_NORMAL_TIP) ? "Socket Normal" : "Socket Origin";
}

godot::Variant Socket3DGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *sock = godot::Object::cast_to<Socket3D>(p_target);
    if (!sock) return godot::Vector3();
    return (p_handle_id == HANDLE_NORMAL_TIP) ? godot::Variant(sock->get_socket_normal()) : godot::Variant(sock->get_position());
}

void Socket3DGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_handle_id;
    (void)p_ray;
    auto *sock = godot::Object::cast_to<Socket3D>(p_target);
    if (!sock) return;
    drag_initial_pos_    = sock->get_position();
    drag_initial_normal_ = sock->get_socket_normal();
    dragging_            = true;
}

void Socket3DGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_modifiers;
    auto *sock = godot::Object::cast_to<Socket3D>(p_target);
    if (!sock || !p_ray.is_valid() || !p_ray.camera) return;

    const godot::Vector3 world_origin = sock->get_global_position();
    const godot::Vector3 cam_fwd      = -p_ray.camera->get_global_transform().basis.get_column(2);
    const godot::Plane drag_plane(cam_fwd, world_origin);

    godot::Vector3 hit;
    if (!drag_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit)) {
        return;
    }

    if (p_handle_id == HANDLE_NORMAL_TIP) {
        const godot::Vector3 local_dir = sock->to_local(hit);
        if (local_dir.length_squared() > 1e-4f) {
            sock->set_socket_normal(local_dir.normalized());
        }
    } else if (auto *parent_3d = godot::Object::cast_to<godot::Node3D>(sock->get_parent())) {
        sock->set_position(parent_3d->to_local(hit));
        sock->update_gizmos();
    }
}

void Socket3DGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    auto *sock = godot::Object::cast_to<Socket3D>(p_target);
    if (!sock || !dragging_) return;
    dragging_ = false;

    if (p_cancel) {
        if (p_handle_id == HANDLE_NORMAL_TIP) {
            sock->set_socket_normal(drag_initial_normal_);
        } else {
            sock->set_position(drag_initial_pos_);
        }
        return;
    }

    godot::Ref<IdeamUndoRedo> ur = resolve_undo_redo();
    if (ur.is_valid()) {
        if (p_handle_id == HANDLE_NORMAL_TIP) {
            const godot::Vector3 final_n = sock->get_socket_normal();
            ur->create_action("Edit Socket3D Normal");
            ur->add_do_property(sock, "socket_normal", final_n);
            ur->add_undo_property(sock, "socket_normal", drag_initial_normal_);
            ur->commit_action(false);
        } else {
            const godot::Vector3 final_p = sock->get_position();
            ur->create_action("Move Socket3D");
            ur->add_do_property(sock, "position", final_p);
            ur->add_undo_property(sock, "position", drag_initial_pos_);
            ur->commit_action(false);
        }
    }
}

} // namespace ideam::godot_ext
