#include "spline_zone_3d.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// SplineZone3D Implementation
// ============================================================================

void SplineZone3D::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Spline Corridor Zone 3D", "");

    ClassDB::bind_method(D_METHOD("set_spline_3d", "spline"), &SplineZone3D::set_spline_3d);
    ClassDB::bind_method(D_METHOD("get_spline_3d"), &SplineZone3D::get_spline_3d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline_3d", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"),
        "set_spline_3d",
        "get_spline_3d"
    );

    ClassDB::bind_method(D_METHOD("set_spline_4d", "spline"), &SplineZone3D::set_spline_4d);
    ClassDB::bind_method(D_METHOD("get_spline_4d"), &SplineZone3D::get_spline_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_spline_4d",
        "get_spline_4d"
    );

    ClassDB::bind_method(D_METHOD("set_radius_envelope_1d", "envelope"), &SplineZone3D::set_radius_envelope_1d);
    ClassDB::bind_method(D_METHOD("get_radius_envelope_1d"), &SplineZone3D::get_radius_envelope_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "radius_envelope_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_radius_envelope_1d",
        "get_radius_envelope_1d"
    );

    ClassDB::bind_method(D_METHOD("set_falloff_curve_1d", "falloff"), &SplineZone3D::set_falloff_curve_1d);
    ClassDB::bind_method(D_METHOD("get_falloff_curve_1d"), &SplineZone3D::get_falloff_curve_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "falloff_curve_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_falloff_curve_1d",
        "get_falloff_curve_1d"
    );

    ClassDB::bind_method(D_METHOD("set_base_corridor_radius", "radius"), &SplineZone3D::set_base_corridor_radius);
    ClassDB::bind_method(D_METHOD("get_base_corridor_radius"), &SplineZone3D::get_base_corridor_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_corridor_radius"), "set_base_corridor_radius", "get_base_corridor_radius");

    ClassDB::bind_method(D_METHOD("set_axial_flow_speed", "speed"), &SplineZone3D::set_axial_flow_speed);
    ClassDB::bind_method(D_METHOD("get_axial_flow_speed"), &SplineZone3D::get_axial_flow_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "axial_flow_speed"), "set_axial_flow_speed", "get_axial_flow_speed");

    ClassDB::bind_method(D_METHOD("set_radial_pull_speed", "speed"), &SplineZone3D::set_radial_pull_speed);
    ClassDB::bind_method(D_METHOD("get_radial_pull_speed"), &SplineZone3D::get_radial_pull_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radial_pull_speed"), "set_radial_pull_speed", "get_radial_pull_speed");

    ClassDB::bind_method(D_METHOD("set_swirl_angular_speed", "speed"), &SplineZone3D::set_swirl_angular_speed);
    ClassDB::bind_method(D_METHOD("get_swirl_angular_speed"), &SplineZone3D::get_swirl_angular_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "swirl_angular_speed"), "set_swirl_angular_speed", "get_swirl_angular_speed");

    ClassDB::bind_method(D_METHOD("set_search_samples", "samples"), &SplineZone3D::set_search_samples);
    ClassDB::bind_method(D_METHOD("get_search_samples"), &SplineZone3D::get_search_samples);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "search_samples"), "set_search_samples", "get_search_samples");

    // --- Queries ---
    ClassDB::bind_method(
        D_METHOD("sample_corridor_radius_at_percent", "percent"),
        &SplineZone3D::sample_corridor_radius_at_percent
    );
    ClassDB::bind_method(D_METHOD("is_world_point_inside", "world_pos"), &SplineZone3D::is_world_point_inside);
    ClassDB::bind_method(D_METHOD("query_zone_at_world_pos", "world_pos"), &SplineZone3D::query_zone_at_world_pos);
    ClassDB::bind_method(
        D_METHOD("sample_flow_velocity_at_world_pos", "world_pos"),
        &SplineZone3D::sample_flow_velocity_at_world_pos
    );

    ClassDB::bind_method(D_METHOD("_on_dependency_changed"), &SplineZone3D::_on_dependency_changed);
}

SplineZone3D::~SplineZone3D() {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (spline_3d_.is_valid() && spline_3d_->is_connected("changed", cb)) spline_3d_->disconnect("changed", cb);
    if (spline_4d_.is_valid() && spline_4d_->is_connected("changed", cb)) spline_4d_->disconnect("changed", cb);
    if (radius_envelope_1d_.is_valid() && radius_envelope_1d_->is_connected("changed", cb)) radius_envelope_1d_->disconnect("changed", cb);
    if (falloff_curve_1d_.is_valid() && falloff_curve_1d_->is_connected("changed", cb)) falloff_curve_1d_->disconnect("changed", cb);
}

void SplineZone3D::_on_dependency_changed() {
    update_gizmos();
}

void SplineZone3D::set_spline_3d(const godot::Ref<Spline3DResource> &p_spline) {
    if (spline_3d_ == p_spline) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (spline_3d_.is_valid() && spline_3d_->is_connected("changed", cb)) spline_3d_->disconnect("changed", cb);
    spline_3d_ = p_spline;
    if (spline_3d_.is_valid() && !spline_3d_->is_connected("changed", cb)) spline_3d_->connect("changed", cb);
    update_gizmos();
}

void SplineZone3D::set_spline_4d(const godot::Ref<Spline4DResource> &p_spline) {
    if (spline_4d_ == p_spline) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (spline_4d_.is_valid() && spline_4d_->is_connected("changed", cb)) spline_4d_->disconnect("changed", cb);
    spline_4d_ = p_spline;
    if (spline_4d_.is_valid() && !spline_4d_->is_connected("changed", cb)) spline_4d_->connect("changed", cb);
    update_gizmos();
}

void SplineZone3D::set_radius_envelope_1d(const godot::Ref<Spline1DResource> &p_envelope) {
    if (radius_envelope_1d_ == p_envelope) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (radius_envelope_1d_.is_valid() && radius_envelope_1d_->is_connected("changed", cb)) radius_envelope_1d_->disconnect("changed", cb);
    radius_envelope_1d_ = p_envelope;
    if (radius_envelope_1d_.is_valid() && !radius_envelope_1d_->is_connected("changed", cb)) radius_envelope_1d_->connect("changed", cb);
    update_gizmos();
}

void SplineZone3D::set_falloff_curve_1d(const godot::Ref<Spline1DResource> &p_falloff) {
    if (falloff_curve_1d_ == p_falloff) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (falloff_curve_1d_.is_valid() && falloff_curve_1d_->is_connected("changed", cb)) falloff_curve_1d_->disconnect("changed", cb);
    falloff_curve_1d_ = p_falloff;
    if (falloff_curve_1d_.is_valid() && !falloff_curve_1d_->is_connected("changed", cb)) falloff_curve_1d_->connect("changed", cb);
}

void SplineZone3D::set_base_corridor_radius(real_t p_radius) {
    base_corridor_radius_ = std::max(static_cast<real_t>(0.05), p_radius);
    update_gizmos();
}

void SplineZone3D::set_axial_flow_speed(real_t p_speed) {
    axial_flow_speed_ = p_speed;
    update_gizmos();
}

void SplineZone3D::set_radial_pull_speed(real_t p_speed) {
    radial_pull_speed_ = p_speed;
}

void SplineZone3D::set_swirl_angular_speed(real_t p_speed) {
    swirl_angular_speed_ = p_speed;
}

void SplineZone3D::set_search_samples(int p_samples) {
    search_samples_ = std::clamp(p_samples, 4, 128);
}

real_t SplineZone3D::sample_corridor_radius_at_percent(real_t p_percent) const {
    real_t r = base_corridor_radius_;
    if (spline_4d_.is_valid() && spline_4d_->get_point_count() >= 2) {
        r = std::max(static_cast<real_t>(0.05), spline_4d_->sample_w(p_percent, false) * base_corridor_radius_);
    }
    if (radius_envelope_1d_.is_valid() && radius_envelope_1d_->get_point_count() > 0) {
        r *= std::max(static_cast<real_t>(0.01), radius_envelope_1d_->sample(p_percent, true));
    }
    return r;
}

godot::Dictionary SplineZone3D::query_zone_at_world_pos(const godot::Vector3 &p_world_pos) const {
    using namespace godot;
    Dictionary out;
    const bool use_4d = spline_4d_.is_valid() && spline_4d_->get_point_count() >= 2;
    const bool use_3d = !use_4d && spline_3d_.is_valid() && spline_3d_->get_point_count() >= 2;
    if (!use_4d && !use_3d) {
        out["is_inside"] = false;
        return out;
    }

    const Transform3D gxf     = get_global_transform();
    const Transform3D inv_gxf = gxf.affine_inverse();
    const Vector3 local_pos   = inv_gxf.xform(p_world_pos);

    const real_t closest_t = use_4d
        ? spline_4d_->get_closest_percent_on_spline_3d(local_pos, search_samples_, false)
        : spline_3d_->get_closest_percent_on_spline(local_pos, search_samples_, false);

    const Vector3 closest_local = use_4d
        ? spline_4d_->sample_3d(closest_t, false)
        : spline_3d_->sample(closest_t, false);
    const Vector3 local_tan = use_4d
        ? spline_4d_->get_tangent_3d(closest_t, false)
        : spline_3d_->get_tangent(closest_t, false);

    const Vector3 closest_world = gxf.xform(closest_local);
    const Vector3 world_tan     = gxf.basis.xform(local_tan).normalized();
    const real_t dist           = static_cast<real_t>(p_world_pos.distance_to(closest_world));
    const real_t radius         = sample_corridor_radius_at_percent(closest_t);
    const bool inside           = dist <= radius;

    real_t falloff = inside ? (static_cast<real_t>(1.0) - (dist / std::max(static_cast<real_t>(1e-4), radius))) : static_cast<real_t>(0.0);
    if (inside && falloff_curve_1d_.is_valid() && falloff_curve_1d_->get_point_count() > 0) {
        falloff = std::clamp(falloff_curve_1d_->sample(dist / std::max(static_cast<real_t>(1e-4), radius), true), static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    }

    out["is_inside"]           = inside;
    out["closest_percent"]     = closest_t;
    out["closest_world_point"] = closest_world;
    out["world_tangent"]       = world_tan;
    out["distance"]            = dist;
    out["corridor_radius"]     = radius;
    out["falloff_weight"]      = falloff;
    return out;
}

bool SplineZone3D::is_world_point_inside(const godot::Vector3 &p_world_pos) const {
    const godot::Dictionary q = query_zone_at_world_pos(p_world_pos);
    return q.has("is_inside") && static_cast<bool>(q["is_inside"]);
}

godot::Vector3 SplineZone3D::sample_flow_velocity_at_world_pos(const godot::Vector3 &p_world_pos) const {
    using namespace godot;
    const Dictionary q = query_zone_at_world_pos(p_world_pos);
    if (!q.has("is_inside") || !static_cast<bool>(q["is_inside"])) {
        return Vector3();
    }

    const Vector3 closest_w = q["closest_world_point"];
    const Vector3 tan_w     = q["world_tangent"];
    const float weight      = static_cast<float>(q["falloff_weight"]);
    const Vector3 radial_vec = p_world_pos - closest_w;
    const Vector3 pull_dir   = (radial_vec.length_squared() > 1e-6f) ? (-radial_vec.normalized()) : Vector3();
    const Vector3 swirl_dir  = tan_w.cross(radial_vec);

    return (tan_w * static_cast<float>(axial_flow_speed_) +
            pull_dir * static_cast<float>(radial_pull_speed_) +
            swirl_dir * static_cast<float>(swirl_angular_speed_)) * weight;
}

// ============================================================================
// SplineCorridorGizmo Implementation
// ============================================================================

bool SplineCorridorGizmo::can_handle(godot::Node3D *p_target) const {
    return p_target != nullptr && godot::Object::cast_to<SplineZone3D>(p_target) != nullptr;
}

godot::String SplineCorridorGizmo::get_gizmo_name() const {
    return "SplineCorridorGizmo";
}

int32_t SplineCorridorGizmo::get_priority() const {
    return 10;
}

void SplineCorridorGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    using namespace godot;
    (void)p_view;
    auto *node = Object::cast_to<SplineZone3D>(p_target);
    if (!node) return;

    const Ref<Spline4DResource> s4 = node->get_spline_4d();
    const Ref<Spline3DResource> s3 = node->get_spline_3d();
    const bool use_4d = s4.is_valid() && s4->get_point_count() >= 2;
    const bool use_3d = !use_4d && s3.is_valid() && s3->get_point_count() >= 2;
    if (!use_4d && !use_3d) return;

    PackedVector3Array lines;
    const TypedArray<Transform3D> frames = use_4d
        ? s4->compute_bishop_transforms_3d(6)
        : s3->compute_bishop_transforms(6);

    const int n_frames = static_cast<int>(frames.size());
    for (int i = 0; i < n_frames; ++i) {
        const real_t t       = (n_frames > 1) ? (static_cast<real_t>(i) / static_cast<real_t>(n_frames - 1)) : static_cast<real_t>(0.0);
        const Transform3D xf = frames[i];
        const float r        = static_cast<float>(node->sample_corridor_radius_at_percent(t));

        if (i + 1 < n_frames) {
            const real_t t_next       = static_cast<real_t>(i + 1) / static_cast<real_t>(n_frames - 1);
            const Transform3D next_xf = frames[i + 1];
            const float r_next        = static_cast<float>(node->sample_corridor_radius_at_percent(t_next));
            lines.push_back(xf.origin);
            lines.push_back(next_xf.origin);

            for (int rail = 0; rail < 4; ++rail) {
                const float ang = static_cast<float>(rail) * (3.141592653589793f * 0.5f);
                lines.push_back(xf.xform(Vector3(std::cos(ang) * r, std::sin(ang) * r, 0.0f)));
                lines.push_back(next_xf.xform(Vector3(std::cos(ang) * r_next, std::sin(ang) * r_next, 0.0f)));
            }
        }
        if (i % 2 == 0) {
            constexpr int RING_SEG = 16;
            for (int k = 0; k < RING_SEG; ++k) {
                const float a0 = (static_cast<float>(k) / static_cast<float>(RING_SEG)) * 6.283185307179586f;
                const float a1 = (static_cast<float>(k + 1) / static_cast<float>(RING_SEG)) * 6.283185307179586f;
                lines.push_back(xf.xform(Vector3(std::cos(a0) * r, std::sin(a0) * r, 0.0f)));
                lines.push_back(xf.xform(Vector3(std::cos(a1) * r, std::sin(a1) * r, 0.0f)));
            }
        }
    }
    if (!lines.is_empty()) r_out.add_lines(lines, GIZMO_HANDLE_SPLINE_TANGENT);

    const int n_pts = use_4d ? s4->get_point_count() : s3->get_point_count();
    for (int i = 0; i < n_pts; ++i) {
        r_out.add_handle(
            HANDLE_ANCHOR_BASE + i,
            use_4d ? s4->get_point_position_3d(i) : s3->get_point_position(i),
            GIZMO_HANDLE_SPLINE_ANCHOR,
            HANDLE_SHAPE_SPHERE,
            static_cast<real_t>(0.05),
            GIZMO_STATE_NONE,
            String("Corridor Path Point ") + String::num_int64(i)
        );
    }
    if (!frames.is_empty()) {
        const Transform3D xf0 = frames[0];
        r_out.add_handle(
            HANDLE_CORRIDOR_RADIUS,
            xf0.xform(Vector3(static_cast<float>(node->get_base_corridor_radius()), 0.0f, 0.0f)),
            GIZMO_LINE_RULER,
            HANDLE_SHAPE_RING,
            static_cast<real_t>(0.05),
            GIZMO_STATE_NONE,
            "Base Corridor Radius"
        );
    }
}

godot::String SplineCorridorGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_target;
    (void)p_secondary;
    if (p_handle_id == HANDLE_CORRIDOR_RADIUS) return "Base Corridor Radius";
    return godot::String("Corridor Path Point ") + godot::String::num_int64(p_handle_id);
}

godot::Variant SplineCorridorGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<SplineZone3D>(p_target);
    if (!node) return godot::Variant();
    if (p_handle_id == HANDLE_CORRIDOR_RADIUS) return node->get_base_corridor_radius();
    if (node->get_spline_4d().is_valid() && p_handle_id >= 0 && p_handle_id < node->get_spline_4d()->get_point_count()) {
        return node->get_spline_4d()->get_point_position_3d(p_handle_id);
    }
    if (node->get_spline_3d().is_valid() && p_handle_id >= 0 && p_handle_id < node->get_spline_3d()->get_point_count()) {
        return node->get_spline_3d()->get_point_position(p_handle_id);
    }
    return godot::Variant();
}

void SplineCorridorGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    if (!p_target) return;
    drag_restore_[p_target->get_instance_id()] = get_handle_value(p_target, p_handle_id, false);
}

void SplineCorridorGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    using namespace godot;
    (void)p_modifiers;
    auto *node = Object::cast_to<SplineZone3D>(p_target);
    if (!node || !p_ray.is_valid()) return;

    if (!drag_restore_.has(node->get_instance_id())) {
        on_handle_drag_begin(p_target, p_handle_id, p_ray);
    }

    const Transform3D inv_gxf = node->get_global_transform().affine_inverse();
    const Vector3 ray_from    = inv_gxf.xform(p_ray.origin);
    const Vector3 ray_dir     = inv_gxf.basis.xform(p_ray.normal).normalized();

    if (p_handle_id == HANDLE_CORRIDOR_RADIUS) {
        const Vector3 p0 = (node->get_spline_3d().is_valid() && node->get_spline_3d()->get_point_count() > 0)
            ? node->get_spline_3d()->get_point_position(0)
            : Vector3();
        const Vector3 hit = ray_from + ray_dir * std::max(0.1f, (p0 - ray_from).dot(ray_dir));
        node->set_base_corridor_radius(hit.distance_to(p0));
        node->update_gizmos();
        return;
    }

    if (node->get_spline_4d().is_valid() && p_handle_id >= 0 && p_handle_id < node->get_spline_4d()->get_point_count()) {
        const Vector3 cur = node->get_spline_4d()->get_point_position_3d(p_handle_id);
        const float dist  = std::max(0.1f, (cur - ray_from).dot(ray_dir));
        node->get_spline_4d()->set_point_position_3d(p_handle_id, ray_from + ray_dir * dist);
    } else if (node->get_spline_3d().is_valid() && p_handle_id >= 0 && p_handle_id < node->get_spline_3d()->get_point_count()) {
        const Vector3 cur = node->get_spline_3d()->get_point_position(p_handle_id);
        const float dist  = std::max(0.1f, (cur - ray_from).dot(ray_dir));
        node->get_spline_3d()->set_point_position(p_handle_id, ray_from + ray_dir * dist);
    }
    node->update_gizmos();
}

void SplineCorridorGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    using namespace godot;
    auto *node = Object::cast_to<SplineZone3D>(p_target);
    if (!node) return;
    const uint64_t nid = node->get_instance_id();
    if (!drag_restore_.has(nid)) return;
    const Variant restore = drag_restore_[nid];
    drag_restore_.erase(nid);

    if (p_handle_id == HANDLE_CORRIDOR_RADIUS) {
        if (p_cancel) {
            node->set_base_corridor_radius(restore);
            return;
        }
        Ref<IdeamUndoRedo> ur = resolve_undo_redo();
        if (ur.is_null()) ur = IdeamSplinesPlugin::create_undo_redo_wrapper();
        ur->create_action("Adjust Corridor Radius", UndoRedo::MERGE_DISABLE, node);
        ur->add_do_method(Callable(node, "set_base_corridor_radius").bind(node->get_base_corridor_radius()));
        ur->add_undo_method(Callable(node, "set_base_corridor_radius").bind(restore));
        ur->commit_action(false);
        return;
    }

    if (node->get_spline_3d().is_valid() && p_handle_id >= 0 && p_handle_id < node->get_spline_3d()->get_point_count()) {
        const Vector3 final_pos = node->get_spline_3d()->get_point_position(p_handle_id);
        node->get_spline_3d()->set_point_position(p_handle_id, restore);
        if (!p_cancel) {
            node->get_spline_3d()->commit_set_point_position(p_handle_id, final_pos);
        }
    }
}

} // namespace ideam::godot_ext
