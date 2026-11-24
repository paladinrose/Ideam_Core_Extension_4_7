#include "geometry_projector_gizmo.h"

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

bool GeometryProjectorGizmo::can_handle(godot::Node3D *p_target) const {
    return godot::Object::cast_to<Geometry2DProjector3D>(p_target) != nullptr;
}

void GeometryProjectorGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    (void)p_view;
    auto *proj = godot::Object::cast_to<Geometry2DProjector3D>(p_target);
    if (!proj) return;

    const float radius = static_cast<float>(std::max(static_cast<real_t>(0.25), proj->get_projection_radius()));
    const float height = static_cast<float>(std::max(static_cast<real_t>(0.25), proj->get_ray_origin_height()));

    // Ground ring & ray-origin ring
    r_out.add_circle(godot::Vector3(0.0f, 0.0f, 0.0f), godot::Vector3(0.0f, 1.0f, 0.0f), radius, 32, GIZMO_LINE_AXIS_NORMAL);
    r_out.add_circle(godot::Vector3(0.0f, height, 0.0f), godot::Vector3(0.0f, 1.0f, 0.0f), radius * 0.45f, 24, GIZMO_LINE_CLEARANCE);
    r_out.add_cone_arrow(godot::Vector3(0.0f, height, 0.0f), godot::Vector3(0.0f, -1.0f, 0.0f), std::min(height, 2.0f), 0.12f, GIZMO_SNAP_PREVIEW);

    r_out.add_handle(
        HANDLE_PROJECTION_RADIUS,
        godot::Vector3(radius, 0.0f, 0.0f),
        GIZMO_LINE_RULER,
        HANDLE_SHAPE_RING,
        static_cast<real_t>(0.08),
        GIZMO_STATE_NONE,
        "Projection Radius"
    );
    r_out.add_handle(
        HANDLE_RAY_ORIGIN_HEIGHT,
        godot::Vector3(0.0f, height, 0.0f),
        GIZMO_HANDLE_SCAFFOLD,
        HANDLE_SHAPE_CONE,
        static_cast<real_t>(0.09),
        GIZMO_STATE_NONE,
        "Ray Origin Height"
    );
}

godot::String GeometryProjectorGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_target;
    (void)p_secondary;
    if (p_handle_id == HANDLE_RAY_ORIGIN_HEIGHT) return "Ray Origin Height";
    return "Projection Radius";
}

godot::Variant GeometryProjectorGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *proj = godot::Object::cast_to<Geometry2DProjector3D>(p_target);
    if (!proj) return 0.0f;
    return (p_handle_id == HANDLE_RAY_ORIGIN_HEIGHT) ? proj->get_ray_origin_height() : proj->get_projection_radius();
}

void GeometryProjectorGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    if (!p_target) return;
    drag_restore_[p_target->get_instance_id()] = static_cast<real_t>(get_handle_value(p_target, p_handle_id, false));
}

void GeometryProjectorGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_modifiers;
    auto *proj = godot::Object::cast_to<Geometry2DProjector3D>(p_target);
    if (!proj || !p_ray.is_valid()) return;
    if (!drag_restore_.has(proj->get_instance_id())) {
        on_handle_drag_begin(p_target, p_handle_id, p_ray);
    }

    const godot::Transform3D inv_gt = proj->get_global_transform().affine_inverse();
    const godot::Vector3 ray_from   = inv_gt.xform(p_ray.origin);
    const godot::Vector3 ray_dir    = inv_gt.basis.xform(p_ray.normal).normalized();

    const godot::Vector3 axis = (p_handle_id == HANDLE_RAY_ORIGIN_HEIGHT)
        ? godot::Vector3(0.0f, 1.0f, 0.0f)
        : godot::Vector3(1.0f, 0.0f, 0.0f);

    const real_t d_dot_a = ray_dir.dot(axis);
    const real_t denom   = static_cast<real_t>(1.0) - d_dot_a * d_dot_a;
    if (std::abs(denom) > static_cast<real_t>(1e-5)) {
        const real_t s = (ray_from.dot(axis) - ray_from.dot(ray_dir) * d_dot_a) / denom;
        if (p_handle_id == HANDLE_RAY_ORIGIN_HEIGHT) {
            proj->set_ray_origin_height(std::clamp(s, static_cast<real_t>(0.25), static_cast<real_t>(500.0)));
        } else {
            proj->set_projection_radius(std::clamp(s, static_cast<real_t>(0.25), static_cast<real_t>(500.0)));
        }
        proj->update_gizmos();
    }
}

void GeometryProjectorGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    auto *proj = godot::Object::cast_to<Geometry2DProjector3D>(p_target);
    if (!proj) return;
    const uint64_t nid = proj->get_instance_id();
    if (!drag_restore_.has(nid)) return;
    const real_t prev = drag_restore_[nid];
    drag_restore_.erase(nid);

    const bool is_height = (p_handle_id == HANDLE_RAY_ORIGIN_HEIGHT);
    if (p_cancel) {
        if (is_height) proj->set_ray_origin_height(prev);
        else proj->set_projection_radius(prev);
        proj->update_gizmos();
        return;
    }

    const real_t cur = is_height ? proj->get_ray_origin_height() : proj->get_projection_radius();
    if (auto ur = resolve_undo_redo(); ur.is_valid()) {
        ur->create_action(is_height ? "Adjust Projector Ray Height" : "Adjust Projection Radius");
        ur->add_do_method(proj, is_height ? "set_ray_origin_height" : "set_projection_radius", cur);
        ur->add_undo_method(proj, is_height ? "set_ray_origin_height" : "set_projection_radius", prev);
        ur->commit_action(false);
    }
    proj->update_gizmos();
}

} // namespace ideam::godot_ext
