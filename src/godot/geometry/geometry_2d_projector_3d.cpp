#include "geometry_2d_projector_3d.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void Geometry2DProjector3D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(PROJ_PLANE_XZ_GROUND);
    BIND_ENUM_CONSTANT(PROJ_PLANE_XY_WALL);
    BIND_ENUM_CONSTANT(PROJ_CYLINDRICAL_WRAP);
    BIND_ENUM_CONSTANT(PROJ_SPHERICAL_WRAP);
    BIND_ENUM_CONSTANT(PROJ_PHYSICS_RAYCAST_DRAPE);
    BIND_ENUM_CONSTANT(PROJ_TARGET_MESH_SURFACE);

    ADD_SIGNAL(MethodInfo("projection_rebuilt"));

    ADD_GROUP("Projection Topology", "");

    ClassDB::bind_method(D_METHOD("set_projection_mode", "mode"), &Geometry2DProjector3D::set_projection_mode);
    ClassDB::bind_method(D_METHOD("get_projection_mode"), &Geometry2DProjector3D::get_projection_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "projection_mode",
            PROPERTY_HINT_ENUM,
            "Plane XZ (Ground):0,Plane XY (Wall):1,Cylindrical Wrap:2,Spherical Wrap:3,Physics Raycast Drape:4,Target Mesh Surface:5"
        ),
        "set_projection_mode",
        "get_projection_mode"
    );

    ClassDB::bind_method(D_METHOD("set_input_uv_scale", "scale"), &Geometry2DProjector3D::set_input_uv_scale);
    ClassDB::bind_method(D_METHOD("get_input_uv_scale"), &Geometry2DProjector3D::get_input_uv_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "input_uv_scale"), "set_input_uv_scale", "get_input_uv_scale");

    ClassDB::bind_method(D_METHOD("set_input_uv_offset", "offset"), &Geometry2DProjector3D::set_input_uv_offset);
    ClassDB::bind_method(D_METHOD("get_input_uv_offset"), &Geometry2DProjector3D::get_input_uv_offset);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "input_uv_offset"), "set_input_uv_offset", "get_input_uv_offset");

    ClassDB::bind_method(D_METHOD("set_normal_offset", "offset"), &Geometry2DProjector3D::set_normal_offset);
    ClassDB::bind_method(D_METHOD("get_normal_offset"), &Geometry2DProjector3D::get_normal_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_offset"), "set_normal_offset", "get_normal_offset");

    ClassDB::bind_method(D_METHOD("set_projection_radius", "radius"), &Geometry2DProjector3D::set_projection_radius);
    ClassDB::bind_method(D_METHOD("get_projection_radius"), &Geometry2DProjector3D::get_projection_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "projection_radius"), "set_projection_radius", "get_projection_radius");

    ClassDB::bind_method(D_METHOD("set_align_to_surface_normal", "align"), &Geometry2DProjector3D::set_align_to_surface_normal);
    ClassDB::bind_method(D_METHOD("get_align_to_surface_normal"), &Geometry2DProjector3D::get_align_to_surface_normal);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "align_to_surface_normal"), "set_align_to_surface_normal", "get_align_to_surface_normal");

    ClassDB::bind_method(D_METHOD("set_extrude_thickness", "thickness"), &Geometry2DProjector3D::set_extrude_thickness);
    ClassDB::bind_method(D_METHOD("get_extrude_thickness"), &Geometry2DProjector3D::get_extrude_thickness);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "extrude_thickness"), "set_extrude_thickness", "get_extrude_thickness");

    ADD_GROUP("Raycast & Target Mesh", "");

    ClassDB::bind_method(D_METHOD("set_target_mesh_instance_path", "path"), &Geometry2DProjector3D::set_target_mesh_instance_path);
    ClassDB::bind_method(D_METHOD("get_target_mesh_instance_path"), &Geometry2DProjector3D::get_target_mesh_instance_path);
    ADD_PROPERTY(
        PropertyInfo(Variant::NODE_PATH, "target_mesh_instance_path"),
        "set_target_mesh_instance_path",
        "get_target_mesh_instance_path"
    );

    ClassDB::bind_method(D_METHOD("set_ray_origin_height", "height"), &Geometry2DProjector3D::set_ray_origin_height);
    ClassDB::bind_method(D_METHOD("get_ray_origin_height"), &Geometry2DProjector3D::get_ray_origin_height);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ray_origin_height"), "set_ray_origin_height", "get_ray_origin_height");

    ClassDB::bind_method(D_METHOD("set_max_ray_distance", "distance"), &Geometry2DProjector3D::set_max_ray_distance);
    ClassDB::bind_method(D_METHOD("get_max_ray_distance"), &Geometry2DProjector3D::get_max_ray_distance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_ray_distance"), "set_max_ray_distance", "get_max_ray_distance");

    ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &Geometry2DProjector3D::set_collision_mask);
    ClassDB::bind_method(D_METHOD("get_collision_mask"), &Geometry2DProjector3D::get_collision_mask);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS),
        "set_collision_mask",
        "get_collision_mask"
    );

    ADD_GROUP("2D Source & Rendering", "");

    ClassDB::bind_method(D_METHOD("set_source_polygon_2d", "polygon"), &Geometry2DProjector3D::set_source_polygon_2d);
    ClassDB::bind_method(D_METHOD("get_source_polygon_2d"), &Geometry2DProjector3D::get_source_polygon_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "source_polygon_2d", PROPERTY_HINT_RESOURCE_TYPE, "Polygon2DResource"),
        "set_source_polygon_2d",
        "get_source_polygon_2d"
    );

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &Geometry2DProjector3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &Geometry2DProjector3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("set_surface_albedo", "color"), &Geometry2DProjector3D::set_surface_albedo);
    ClassDB::bind_method(D_METHOD("get_surface_albedo"), &Geometry2DProjector3D::get_surface_albedo);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "surface_albedo"), "set_surface_albedo", "get_surface_albedo");

    ClassDB::bind_method(D_METHOD("get_projected_mesh"), &Geometry2DProjector3D::get_projected_mesh);

    // --- Projection Queries & Builders ---
    ClassDB::bind_method(D_METHOD("project_point_2d", "uv"), &Geometry2DProjector3D::project_point_2d);
    ClassDB::bind_method(
        D_METHOD("project_transform_2d", "uv", "tangent_2d"),
        &Geometry2DProjector3D::project_transform_2d,
        DEFVAL(Vector2(1.0f, 0.0f))
    );
    ClassDB::bind_method(
        D_METHOD("project_polyline_2d", "points_2d", "max_segment_length_2d"),
        &Geometry2DProjector3D::project_polyline_2d,
        DEFVAL(0.5)
    );
    ClassDB::bind_method(
        D_METHOD("build_projected_polygon_mesh", "vertices_2d", "triangles_2d", "extrude_thickness", "vertex_color"),
        &Geometry2DProjector3D::build_projected_polygon_mesh,
        DEFVAL(0.0),
        DEFVAL(Color(1.0f, 1.0f, 1.0f, 1.0f))
    );
    ClassDB::bind_method(D_METHOD("invalidate_target_mesh_cache"), &Geometry2DProjector3D::invalidate_target_mesh_cache);
    ClassDB::bind_method(D_METHOD("rebuild_projection"), &Geometry2DProjector3D::rebuild_projection);

    ClassDB::bind_method(D_METHOD("_on_source_polygon_changed"), &Geometry2DProjector3D::_on_source_polygon_changed);
}

Geometry2DProjector3D::Geometry2DProjector3D() {
    projected_mesh_.instantiate();
    if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
        mesh_instance_rid_ = rs->instance_create();
        rs->instance_set_base(mesh_instance_rid_, projected_mesh_->get_rid());
    }
    set_notify_transform(true);
}

Geometry2DProjector3D::~Geometry2DProjector3D() {
    if (source_polygon_2d_.is_valid()) {
        const godot::Callable cb(this, "_on_source_polygon_changed");
        if (source_polygon_2d_->is_connected("changed", cb)) {
            source_polygon_2d_->disconnect("changed", cb);
        }
    }
    if (mesh_instance_rid_.is_valid()) {
        if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
            rs->free_rid(mesh_instance_rid_);
        }
    }
}

void Geometry2DProjector3D::_notification(int p_what) {
    using namespace godot;
    RenderingServer *rs = RenderingServer::get_singleton();
    if (!rs || !mesh_instance_rid_.is_valid()) return;

    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            Ref<World3D> w3d = get_world_3d();
            if (w3d.is_valid()) {
                rs->instance_set_scenario(mesh_instance_rid_, w3d->get_scenario());
            }
            rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
            rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            _rebuild_projected_geometry();
            break;
        }
        case NOTIFICATION_EXIT_WORLD: {
            rs->instance_set_scenario(mesh_instance_rid_, RID());
            break;
        }
        case NOTIFICATION_TRANSFORM_CHANGED: {
            rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
            if (projection_mode_ == PROJ_PHYSICS_RAYCAST_DRAPE || projection_mode_ == PROJ_TARGET_MESH_SURFACE) {
                target_mesh_cache_dirty_ = true;
                _rebuild_projected_geometry();
            }
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            break;
        }
        default:
            break;
    }
}

void Geometry2DProjector3D::set_projection_mode(Geometry3DProjectionMode p_mode) {
    if (projection_mode_ == p_mode) return;
    projection_mode_ = p_mode;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_input_uv_scale(const godot::Vector2 &p_scale) {
    if (input_uv_scale_ == p_scale) return;
    input_uv_scale_ = p_scale;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_input_uv_offset(const godot::Vector2 &p_offset) {
    if (input_uv_offset_ == p_offset) return;
    input_uv_offset_ = p_offset;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_normal_offset(real_t p_offset) {
    if (std::abs(normal_offset_ - p_offset) <= 1e-6f) return;
    normal_offset_ = p_offset;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_projection_radius(real_t p_radius) {
    const real_t clamped = std::max(static_cast<real_t>(0.05), p_radius);
    if (std::abs(projection_radius_ - clamped) <= 1e-6f) return;
    projection_radius_ = clamped;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_max_ray_distance(real_t p_distance) {
    max_ray_distance_ = std::max(static_cast<real_t>(0.1), p_distance);
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_ray_origin_height(real_t p_height) {
    ray_origin_height_ = p_height;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_collision_mask(uint32_t p_mask) {
    collision_mask_ = p_mask;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_align_to_surface_normal(bool p_align) {
    align_to_surface_normal_ = p_align;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_extrude_thickness(real_t p_thickness) {
    extrude_thickness_ = std::max(static_cast<real_t>(0.0), p_thickness);
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_target_mesh_instance_path(const godot::NodePath &p_path) {
    if (target_mesh_instance_path_ == p_path) return;
    target_mesh_instance_path_ = p_path;
    target_mesh_cache_dirty_   = true;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_source_polygon_2d(const godot::Ref<Polygon2DResource> &p_polygon) {
    if (source_polygon_2d_ == p_polygon) return;
    const godot::Callable cb(this, "_on_source_polygon_changed");
    if (source_polygon_2d_.is_valid() && source_polygon_2d_->is_connected("changed", cb)) {
        source_polygon_2d_->disconnect("changed", cb);
    }
    source_polygon_2d_ = p_polygon;
    if (source_polygon_2d_.is_valid() && !source_polygon_2d_->is_connected("changed", cb)) {
        source_polygon_2d_->connect("changed", cb);
    }
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::set_material_override(const godot::Ref<godot::Material> &p_material) {
    material_override_ = p_material;
    if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
        if (mesh_instance_rid_.is_valid()) {
            rs->instance_geometry_set_material_override(
                mesh_instance_rid_,
                material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
            );
        }
    }
}

void Geometry2DProjector3D::set_surface_albedo(const godot::Color &p_color) {
    surface_albedo_ = p_color;
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::_on_source_polygon_changed() {
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::invalidate_target_mesh_cache() {
    target_mesh_cache_dirty_ = true;
}

void Geometry2DProjector3D::rebuild_projection() {
    _rebuild_projected_geometry();
}

void Geometry2DProjector3D::_rebuild_target_mesh_triangle_cache() const {
    using namespace godot;
    if (!target_mesh_cache_dirty_) return;
    cached_target_triangles_.clear();
    target_mesh_cache_dirty_ = false;

    if (target_mesh_instance_path_.is_empty() || !is_inside_tree()) return;
    MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(get_node_or_null(target_mesh_instance_path_));
    if (!mi || mi->get_mesh().is_null()) return;

    Ref<Mesh> m = mi->get_mesh();
    const PackedVector3Array faces = m->get_faces();
    if (faces.size() < 3) return;

    // Transform target mesh vertices into this projector's local coordinate space
    const Transform3D to_local_xf = get_global_transform().affine_inverse() * mi->get_global_transform();
    cached_target_triangles_.reserve(static_cast<size_t>(faces.size() / 3));

    for (int64_t i = 0; i + 2 < faces.size(); i += 3) {
        const Vector3 a = to_local_xf.xform(faces[i]);
        const Vector3 b = to_local_xf.xform(faces[i + 1]);
        const Vector3 c = to_local_xf.xform(faces[i + 2]);
        Vector3 n       = (b - a).cross(c - a);
        if (n.length_squared() < 1e-10f) continue;
        n.normalize();
        if (n.y < 0.0f) n = -n;
        cached_target_triangles_.push_back(CachedMeshTriangle{a, b, c, n});
    }
}

bool Geometry2DProjector3D::_raycast_target_mesh_local(
    const godot::Vector3 &p_ray_origin_local,
    const godot::Vector3 &p_ray_dir_local,
    real_t p_max_dist,
    godot::Vector3 &r_hit_local,
    godot::Vector3 &r_normal_local
) const {
    _rebuild_target_mesh_triangle_cache();
    if (cached_target_triangles_.empty()) return false;

    bool found    = false;
    real_t best_t = p_max_dist;

    // Möller-Trumbore ray-triangle intersection
    for (const CachedMeshTriangle &tri : cached_target_triangles_) {
        const godot::Vector3 e1 = tri.v1 - tri.v0;
        const godot::Vector3 e2 = tri.v2 - tri.v0;
        const godot::Vector3 h  = p_ray_dir_local.cross(e2);
        const real_t a          = e1.dot(h);
        if (std::abs(a) < static_cast<real_t>(1e-7)) continue;

        const real_t f          = static_cast<real_t>(1.0) / a;
        const godot::Vector3 s  = p_ray_origin_local - tri.v0;
        const real_t u          = f * s.dot(h);
        if (u < static_cast<real_t>(0.0) || u > static_cast<real_t>(1.0)) continue;

        const godot::Vector3 q = s.cross(e1);
        const real_t v         = f * p_ray_dir_local.dot(q);
        if (v < static_cast<real_t>(0.0) || u + v > static_cast<real_t>(1.0)) continue;

        const real_t t = f * e2.dot(q);
        if (t > static_cast<real_t>(1e-5) && t < best_t) {
            best_t         = t;
            r_hit_local    = p_ray_origin_local + p_ray_dir_local * t;
            r_normal_local = tri.normal;
            found          = true;
        }
    }
    return found;
}

bool Geometry2DProjector3D::project_point_2d_with_normal(
    const godot::Vector2 &p_uv,
    godot::Vector3 &r_local_pos,
    godot::Vector3 &r_local_normal
) const {
    using namespace godot;
    const Vector2 uv(
        p_uv.x * input_uv_scale_.x + input_uv_offset_.x,
        p_uv.y * input_uv_scale_.y + input_uv_offset_.y
    );

    switch (projection_mode_) {
        case PROJ_PLANE_XY_WALL: {
            r_local_normal = Vector3(0.0f, 0.0f, 1.0f);
            r_local_pos    = Vector3(uv.x, uv.y, static_cast<float>(normal_offset_));
            return true;
        }
        case PROJ_CYLINDRICAL_WRAP: {
            const float r     = std::max(0.05f, static_cast<float>(projection_radius_));
            const float theta = uv.x / r;
            r_local_normal    = Vector3(std::sin(theta), 0.0f, std::cos(theta)).normalized();
            const float eff_r = r + static_cast<float>(normal_offset_);
            r_local_pos       = Vector3(eff_r * std::sin(theta), uv.y, eff_r * std::cos(theta));
            return true;
        }
        case PROJ_SPHERICAL_WRAP: {
            const float r   = std::max(0.05f, static_cast<float>(projection_radius_));
            const float lon = uv.x / r;
            const float lat = std::clamp(uv.y / r, -1.55f, 1.55f);
            r_local_normal  = Vector3(
                std::cos(lat) * std::sin(lon),
                std::sin(lat),
                std::cos(lat) * std::cos(lon)
            ).normalized();
            r_local_pos = r_local_normal * (r + static_cast<float>(normal_offset_));
            return true;
        }
        case PROJ_TARGET_MESH_SURFACE: {
            const Vector3 ray_start(uv.x, static_cast<float>(ray_origin_height_), uv.y);
            const Vector3 ray_dir(0.0f, -1.0f, 0.0f);
            Vector3 hit_pt, hit_n;
            if (_raycast_target_mesh_local(ray_start, ray_dir, max_ray_distance_, hit_pt, hit_n)) {
                r_local_normal = align_to_surface_normal_ ? hit_n : Vector3(0.0f, 1.0f, 0.0f);
                r_local_pos    = hit_pt + r_local_normal * static_cast<float>(normal_offset_);
                return true;
            }
            r_local_normal = Vector3(0.0f, 1.0f, 0.0f);
            r_local_pos    = Vector3(uv.x, static_cast<float>(normal_offset_), uv.y);
            return false;
        }
        case PROJ_PHYSICS_RAYCAST_DRAPE: {
            if (is_inside_tree() && get_world_3d().is_valid()) {
                PhysicsDirectSpaceState3D *space = get_world_3d()->get_direct_space_state();
                if (space) {
                    const Transform3D gxf    = get_global_transform();
                    const Vector3 local_from = Vector3(uv.x, static_cast<float>(ray_origin_height_), uv.y);
                    const Vector3 local_to   = Vector3(uv.x, static_cast<float>(ray_origin_height_ - max_ray_distance_), uv.y);
                    Ref<PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(
                        gxf.xform(local_from),
                        gxf.xform(local_to),
                        collision_mask_
                    );
                    const Dictionary hit = space->intersect_ray(query);
                    if (!hit.is_empty()) {
                        const Transform3D inv_gxf = gxf.affine_inverse();
                        const Vector3 hit_w       = hit["position"];
                        const Vector3 norm_w      = hit["normal"];
                        r_local_normal = align_to_surface_normal_
                            ? inv_gxf.basis.xform(norm_w).normalized()
                            : Vector3(0.0f, 1.0f, 0.0f);
                        r_local_pos = inv_gxf.xform(hit_w) + r_local_normal * static_cast<float>(normal_offset_);
                        return true;
                    }
                }
            }
            r_local_normal = Vector3(0.0f, 1.0f, 0.0f);
            r_local_pos    = Vector3(uv.x, static_cast<float>(normal_offset_), uv.y);
            return false;
        }
        case PROJ_PLANE_XZ_GROUND:
        default: {
            r_local_normal = Vector3(0.0f, 1.0f, 0.0f);
            r_local_pos    = Vector3(uv.x, static_cast<float>(normal_offset_), uv.y);
            return true;
        }
    }
}

godot::Vector3 Geometry2DProjector3D::project_point_2d(const godot::Vector2 &p_uv) const {
    godot::Vector3 pos, norm;
    (void)project_point_2d_with_normal(p_uv, pos, norm);
    return pos;
}

godot::Transform3D Geometry2DProjector3D::project_transform_2d(
    const godot::Vector2 &p_uv,
    const godot::Vector2 &p_tangent_2d
) const {
    using namespace godot;
    Vector3 pos, up_n;
    (void)project_point_2d_with_normal(p_uv, pos, up_n);

    const Vector2 t2 = (p_tangent_2d.length_squared() > 1e-6f) ? p_tangent_2d.normalized() : Vector2(1.0f, 0.0f);
    const Vector3 p_ahead = project_point_2d(p_uv + t2 * 0.05f);
    Vector3 forward = (p_ahead - pos);
    // Orthogonalize forward against surface normal up_n
    forward = forward - up_n * forward.dot(up_n);
    if (forward.length_squared() < 1e-6f) {
        forward = (std::abs(up_n.y) < 0.9f) ? Vector3(0.0f, 1.0f, 0.0f).cross(up_n) : Vector3(1.0f, 0.0f, 0.0f);
    }
    forward.normalize();
    const Vector3 right = forward.cross(up_n).normalized();
    const Vector3 ortho_up = right.cross(forward).normalized();

    // Godot convention: -Z is forward, X is right, Y is up
    return Transform3D(Basis(right, ortho_up, -forward), pos);
}

godot::PackedVector3Array Geometry2DProjector3D::project_polyline_2d(
    const godot::PackedVector2Array &p_points_2d,
    real_t p_max_segment_length_2d
) const {
    using namespace godot;
    PackedVector3Array out;
    if (p_points_2d.is_empty()) return out;

    const float max_seg = std::max(0.05f, static_cast<float>(p_max_segment_length_2d));
    out.push_back(project_point_2d(p_points_2d[0]));

    for (int64_t i = 1; i < p_points_2d.size(); ++i) {
        const Vector2 a = p_points_2d[i - 1];
        const Vector2 b = p_points_2d[i];
        const float len = a.distance_to(b);
        const int steps = std::clamp(static_cast<int>(std::ceil(len / max_seg)), 1, 64);
        for (int s = 1; s <= steps; ++s) {
            const float t = static_cast<float>(s) / static_cast<float>(steps);
            out.push_back(project_point_2d(a.lerp(b, t)));
        }
    }
    return out;
}

godot::Ref<godot::ArrayMesh> Geometry2DProjector3D::build_projected_polygon_mesh(
    const godot::PackedVector2Array &p_vertices_2d,
    const godot::PackedInt32Array &p_triangles_2d,
    real_t p_extrude_thickness,
    const godot::Color &p_vertex_color
) const {
    using namespace godot;
    Ref<ArrayMesh> mesh;
    mesh.instantiate();
    if (p_vertices_2d.size() < 3 || p_triangles_2d.size() < 3) return mesh;

    const int64_t nv = p_vertices_2d.size();
    PackedVector3Array top_pos;
    PackedVector3Array top_norm;
    PackedVector2Array top_uv;
    top_pos.resize(nv);
    top_norm.resize(nv);
    top_uv.resize(nv);

    for (int64_t i = 0; i < nv; ++i) {
        Vector3 p, n;
        (void)project_point_2d_with_normal(p_vertices_2d[i], p, n);
        top_pos.set(i, p + n * static_cast<float>(p_extrude_thickness));
        top_norm.set(i, n);
        top_uv.set(i, p_vertices_2d[i]);
    }

    PackedColorArray colors;
    colors.resize(nv);
    for (int64_t i = 0; i < nv; ++i) colors.set(i, p_vertex_color);

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = top_pos;
    arrays[Mesh::ARRAY_NORMAL] = top_norm;
    arrays[Mesh::ARRAY_TEX_UV] = top_uv;
    arrays[Mesh::ARRAY_COLOR]  = colors;
    arrays[Mesh::ARRAY_INDEX]  = p_triangles_2d;

    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
    return mesh;
}

void Geometry2DProjector3D::_commit_generated_mesh(const godot::Ref<godot::ArrayMesh> &p_mesh) {
    if (projected_mesh_.is_null()) {
        projected_mesh_.instantiate();
    }
    projected_mesh_->clear_surfaces();

    if (p_mesh.is_valid()) {
        for (int s = 0; s < p_mesh->get_surface_count(); ++s) {
            projected_mesh_->add_surface_from_arrays(
                p_mesh->surface_get_primitive_type(s),
                p_mesh->surface_get_arrays(s)
            );
        }
    }

    if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
        if (mesh_instance_rid_.is_valid()) {
            rs->instance_set_base(mesh_instance_rid_, projected_mesh_->get_rid());
            if (material_override_.is_valid()) {
                rs->instance_geometry_set_material_override(mesh_instance_rid_, material_override_->get_rid());
            }
        }
    }
    emit_signal("projection_rebuilt");
}

void Geometry2DProjector3D::_rebuild_projected_geometry() {
    if (source_polygon_2d_.is_valid() && source_polygon_2d_->get_vertex_count() >= 3) {
        const godot::Ref<godot::ArrayMesh> built = build_projected_polygon_mesh(
            source_polygon_2d_->get_vertices(),
            source_polygon_2d_->get_triangles(),
            extrude_thickness_,
            surface_albedo_
        );
        _commit_generated_mesh(built);
    } else if (projected_mesh_.is_valid()) {
        projected_mesh_->clear_surfaces();
    }
}

} // namespace ideam::godot_ext