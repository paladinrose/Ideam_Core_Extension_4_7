#include "spline_extrude_mesh_3d.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// SplineExtrudeMesh3D Implementation
// ============================================================================

void SplineExtrudeMesh3D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(EXTRUDE_W_RADIUS_SCALE);
    BIND_ENUM_CONSTANT(EXTRUDE_W_ROLL_RADIANS);
    BIND_ENUM_CONSTANT(EXTRUDE_W_IGNORE);

    ADD_GROUP("Spline Extrude Paths & Profile", "");

    ClassDB::bind_method(D_METHOD("set_path_spline_3d", "spline"), &SplineExtrudeMesh3D::set_path_spline_3d);
    ClassDB::bind_method(D_METHOD("get_path_spline_3d"), &SplineExtrudeMesh3D::get_path_spline_3d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "path_spline_3d", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"),
        "set_path_spline_3d",
        "get_path_spline_3d"
    );

    ClassDB::bind_method(D_METHOD("set_path_spline_4d", "spline"), &SplineExtrudeMesh3D::set_path_spline_4d);
    ClassDB::bind_method(D_METHOD("get_path_spline_4d"), &SplineExtrudeMesh3D::get_path_spline_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "path_spline_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_path_spline_4d",
        "get_path_spline_4d"
    );

    ClassDB::bind_method(D_METHOD("set_profile_spline_2d", "profile"), &SplineExtrudeMesh3D::set_profile_spline_2d);
    ClassDB::bind_method(D_METHOD("get_profile_spline_2d"), &SplineExtrudeMesh3D::get_profile_spline_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "profile_spline_2d", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_profile_spline_2d",
        "get_profile_spline_2d"
    );

    ClassDB::bind_method(D_METHOD("set_taper_curve_1d", "taper"), &SplineExtrudeMesh3D::set_taper_curve_1d);
    ClassDB::bind_method(D_METHOD("get_taper_curve_1d"), &SplineExtrudeMesh3D::get_taper_curve_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "taper_curve_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_taper_curve_1d",
        "get_taper_curve_1d"
    );

    ClassDB::bind_method(D_METHOD("set_color_gradient_4d", "gradient"), &SplineExtrudeMesh3D::set_color_gradient_4d);
    ClassDB::bind_method(D_METHOD("get_color_gradient_4d"), &SplineExtrudeMesh3D::get_color_gradient_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "color_gradient_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_color_gradient_4d",
        "get_color_gradient_4d"
    );

    ADD_GROUP("Extrusion Parameters", "");

    ClassDB::bind_method(D_METHOD("set_w_channel_mode", "mode"), &SplineExtrudeMesh3D::set_w_channel_mode);
    ClassDB::bind_method(D_METHOD("get_w_channel_mode"), &SplineExtrudeMesh3D::get_w_channel_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "w_channel_mode", PROPERTY_HINT_ENUM, "W = Radius Scale:0,W = Roll (Radians):1,Ignore W:2"),
        "set_w_channel_mode",
        "get_w_channel_mode"
    );

    ClassDB::bind_method(D_METHOD("set_profile_scale", "scale"), &SplineExtrudeMesh3D::set_profile_scale);
    ClassDB::bind_method(D_METHOD("get_profile_scale"), &SplineExtrudeMesh3D::get_profile_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "profile_scale"), "set_profile_scale", "get_profile_scale");

    ClassDB::bind_method(D_METHOD("set_total_twist_radians", "twist"), &SplineExtrudeMesh3D::set_total_twist_radians);
    ClassDB::bind_method(D_METHOD("get_total_twist_radians"), &SplineExtrudeMesh3D::get_total_twist_radians);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "total_twist_radians"), "set_total_twist_radians", "get_total_twist_radians");

    ClassDB::bind_method(D_METHOD("set_path_subdivisions", "subdivs"), &SplineExtrudeMesh3D::set_path_subdivisions);
    ClassDB::bind_method(D_METHOD("get_path_subdivisions"), &SplineExtrudeMesh3D::get_path_subdivisions);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "path_subdivisions"), "set_path_subdivisions", "get_path_subdivisions");

    ClassDB::bind_method(D_METHOD("set_profile_subdivisions", "subdivs"), &SplineExtrudeMesh3D::set_profile_subdivisions);
    ClassDB::bind_method(D_METHOD("get_profile_subdivisions"), &SplineExtrudeMesh3D::get_profile_subdivisions);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "profile_subdivisions"), "set_profile_subdivisions", "get_profile_subdivisions");

    ClassDB::bind_method(D_METHOD("set_cap_ends", "cap"), &SplineExtrudeMesh3D::set_cap_ends);
    ClassDB::bind_method(D_METHOD("get_cap_ends"), &SplineExtrudeMesh3D::get_cap_ends);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cap_ends"), "set_cap_ends", "get_cap_ends");

    ClassDB::bind_method(D_METHOD("set_uv_scale", "uv_scale"), &SplineExtrudeMesh3D::set_uv_scale);
    ClassDB::bind_method(D_METHOD("get_uv_scale"), &SplineExtrudeMesh3D::get_uv_scale);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "uv_scale"), "set_uv_scale", "get_uv_scale");

    ClassDB::bind_method(D_METHOD("set_base_color", "color"), &SplineExtrudeMesh3D::set_base_color);
    ClassDB::bind_method(D_METHOD("get_base_color"), &SplineExtrudeMesh3D::get_base_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "base_color"), "set_base_color", "get_base_color");

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &SplineExtrudeMesh3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &SplineExtrudeMesh3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("get_extruded_mesh"), &SplineExtrudeMesh3D::get_extruded_mesh);
    ClassDB::bind_method(D_METHOD("bake_concave_collision_shape"), &SplineExtrudeMesh3D::bake_concave_collision_shape);
    ClassDB::bind_method(D_METHOD("rebuild_mesh"), &SplineExtrudeMesh3D::rebuild_mesh);
    ClassDB::bind_method(D_METHOD("_on_dependency_changed"), &SplineExtrudeMesh3D::_on_dependency_changed);
}

SplineExtrudeMesh3D::SplineExtrudeMesh3D() {
    extruded_mesh_.instantiate();
    if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
        mesh_instance_rid_ = rs->instance_create();
        rs->instance_set_base(mesh_instance_rid_, extruded_mesh_->get_rid());
    }
    set_notify_transform(true);
}

SplineExtrudeMesh3D::~SplineExtrudeMesh3D() {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (path_spline_3d_.is_valid() && path_spline_3d_->is_connected("changed", cb)) path_spline_3d_->disconnect("changed", cb);
    if (path_spline_4d_.is_valid() && path_spline_4d_->is_connected("changed", cb)) path_spline_4d_->disconnect("changed", cb);
    if (profile_spline_2d_.is_valid() && profile_spline_2d_->is_connected("changed", cb)) profile_spline_2d_->disconnect("changed", cb);
    if (taper_curve_1d_.is_valid() && taper_curve_1d_->is_connected("changed", cb)) taper_curve_1d_->disconnect("changed", cb);
    if (color_gradient_4d_.is_valid() && color_gradient_4d_->is_connected("changed", cb)) color_gradient_4d_->disconnect("changed", cb);

    if (mesh_instance_rid_.is_valid()) {
        if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
            rs->free_rid(mesh_instance_rid_);
        }
    }
}

void SplineExtrudeMesh3D::_notification(int p_what) {
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
            rebuild_mesh();
            break;
        }
        case NOTIFICATION_EXIT_WORLD: {
            rs->instance_set_scenario(mesh_instance_rid_, RID());
            break;
        }
        case NOTIFICATION_TRANSFORM_CHANGED: {
            rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
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

void SplineExtrudeMesh3D::_on_dependency_changed() {
    rebuild_mesh();
    update_gizmos();
}

void SplineExtrudeMesh3D::set_path_spline_3d(const godot::Ref<Spline3DResource> &p_spline) {
    if (path_spline_3d_ == p_spline) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (path_spline_3d_.is_valid() && path_spline_3d_->is_connected("changed", cb)) path_spline_3d_->disconnect("changed", cb);
    path_spline_3d_ = p_spline;
    if (path_spline_3d_.is_valid() && !path_spline_3d_->is_connected("changed", cb)) path_spline_3d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_path_spline_4d(const godot::Ref<Spline4DResource> &p_spline) {
    if (path_spline_4d_ == p_spline) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (path_spline_4d_.is_valid() && path_spline_4d_->is_connected("changed", cb)) path_spline_4d_->disconnect("changed", cb);
    path_spline_4d_ = p_spline;
    if (path_spline_4d_.is_valid() && !path_spline_4d_->is_connected("changed", cb)) path_spline_4d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_profile_spline_2d(const godot::Ref<Spline2DResource> &p_profile) {
    if (profile_spline_2d_ == p_profile) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (profile_spline_2d_.is_valid() && profile_spline_2d_->is_connected("changed", cb)) profile_spline_2d_->disconnect("changed", cb);
    profile_spline_2d_ = p_profile;
    if (profile_spline_2d_.is_valid() && !profile_spline_2d_->is_connected("changed", cb)) profile_spline_2d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_taper_curve_1d(const godot::Ref<Spline1DResource> &p_taper) {
    if (taper_curve_1d_ == p_taper) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (taper_curve_1d_.is_valid() && taper_curve_1d_->is_connected("changed", cb)) taper_curve_1d_->disconnect("changed", cb);
    taper_curve_1d_ = p_taper;
    if (taper_curve_1d_.is_valid() && !taper_curve_1d_->is_connected("changed", cb)) taper_curve_1d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_color_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient) {
    if (color_gradient_4d_ == p_gradient) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (color_gradient_4d_.is_valid() && color_gradient_4d_->is_connected("changed", cb)) color_gradient_4d_->disconnect("changed", cb);
    color_gradient_4d_ = p_gradient;
    if (color_gradient_4d_.is_valid() && !color_gradient_4d_->is_connected("changed", cb)) color_gradient_4d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_w_channel_mode(SplineExtrudeWMode p_mode) {
    w_channel_mode_ = p_mode;
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_profile_scale(const godot::Vector2 &p_scale) {
    profile_scale_ = godot::Vector2(std::max(0.01f, p_scale.x), std::max(0.01f, p_scale.y));
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_total_twist_radians(real_t p_twist) {
    total_twist_radians_ = p_twist;
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_path_subdivisions(int p_subdivs) {
    path_subdivisions_ = std::clamp(p_subdivs, 2, 128);
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_profile_subdivisions(int p_subdivs) {
    profile_subdivisions_ = std::clamp(p_subdivs, 3, 64);
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_cap_ends(bool p_cap) {
    cap_ends_ = p_cap;
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_uv_scale(const godot::Vector2 &p_uv_scale) {
    uv_scale_ = p_uv_scale;
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_base_color(const godot::Color &p_color) {
    base_color_ = p_color;
    _on_dependency_changed();
}

void SplineExtrudeMesh3D::set_material_override(const godot::Ref<godot::Material> &p_material) {
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

godot::PackedVector2Array SplineExtrudeMesh3D::_resolve_profile_ring_2d() const {
    using namespace godot;
    if (profile_spline_2d_.is_valid() && profile_spline_2d_->get_point_count() >= 2) {
        return profile_spline_2d_->get_smoothed_points(profile_subdivisions_);
    }

    // Default unit circle cross-section with `profile_subdivisions_ * 2` vertices
    const int ring_verts = std::max(6, profile_subdivisions_ * 2);
    PackedVector2Array ring;
    ring.resize(ring_verts);
    for (int i = 0; i < ring_verts; ++i) {
        const float angle = (static_cast<float>(i) / static_cast<float>(ring_verts)) * Math_TAU;
        ring.set(i, Vector2(std::cos(angle), std::sin(angle)));
    }
    return ring;
}

void SplineExtrudeMesh3D::rebuild_mesh() {
    using namespace godot;
    if (extruded_mesh_.is_null()) extruded_mesh_.instantiate();
    extruded_mesh_->clear_surfaces();

    const bool use_4d = path_spline_4d_.is_valid() && path_spline_4d_->get_point_count() >= 2;
    const bool use_3d = !use_4d && path_spline_3d_.is_valid() && path_spline_3d_->get_point_count() >= 2;
    if (!use_4d && !use_3d) return;

    const TypedArray<Transform3D> bishop_frames = use_4d
        ? path_spline_4d_->compute_bishop_transforms_3d(path_subdivisions_)
        : path_spline_3d_->compute_bishop_transforms(path_subdivisions_);
    const int num_rings = static_cast<int>(bishop_frames.size());
    if (num_rings < 2) return;

    const PackedVector2Array profile_2d = _resolve_profile_ring_2d();
    const int ring_cols                 = static_cast<int>(profile_2d.size());
    if (ring_cols < 2) return;

    const bool closed_profile = profile_spline_2d_.is_null() || profile_spline_2d_->get_wrap();
    const int stride_cols     = closed_profile ? (ring_cols + 1) : ring_cols;

    PackedVector3Array vertices;
    PackedVector3Array normals;
    PackedVector2Array uvs;
    PackedColorArray colors;
    PackedInt32Array indices;

    vertices.resize(num_rings * stride_cols);
    normals.resize(num_rings * stride_cols);
    uvs.resize(num_rings * stride_cols);
    colors.resize(num_rings * stride_cols);

    for (int r = 0; r < num_rings; ++r) {
        const real_t t         = static_cast<real_t>(r) / static_cast<real_t>(num_rings - 1);
        const Transform3D b_xf = bishop_frames[r];

        real_t w_scale = static_cast<real_t>(1.0);
        real_t w_roll  = static_cast<real_t>(0.0);
        if (use_4d) {
            const real_t w_val = path_spline_4d_->sample_w(t, false);
            if (w_channel_mode_ == EXTRUDE_W_RADIUS_SCALE) {
                w_scale = std::max(static_cast<real_t>(0.01), w_val);
            } else if (w_channel_mode_ == EXTRUDE_W_ROLL_RADIANS) {
                w_roll = w_val;
            }
        }
        if (taper_curve_1d_.is_valid() && taper_curve_1d_->get_point_count() > 0) {
            w_scale *= std::max(static_cast<real_t>(0.0), taper_curve_1d_->sample(t, true));
        }

        const float roll_angle = static_cast<float>(w_roll + total_twist_radians_ * t);
        const Vector3 fwd      = -b_xf.basis.get_column(2).normalized();
        const Basis rolled_b   = (std::abs(roll_angle) > 1e-5f)
            ? (Basis(fwd, roll_angle) * b_xf.basis)
            : b_xf.basis;

        const Color ring_col = (color_gradient_4d_.is_valid() && color_gradient_4d_->get_point_count() > 0)
            ? color_gradient_4d_->sample_color(t, true)
            : ((use_4d && path_spline_4d_->is_color_gradient_mode())
                ? path_spline_4d_->sample_color(t, true)
                : base_color_);

        for (int c = 0; c < stride_cols; ++c) {
            const int src_c    = c % ring_cols;
            const Vector2 p2   = profile_2d[src_c];
            const Vector3 l_pt = Vector3(
                p2.x * profile_scale_.x * static_cast<float>(w_scale),
                p2.y * profile_scale_.y * static_cast<float>(w_scale),
                0.0f
            );
            const Vector3 w_pt   = b_xf.origin + rolled_b.xform(l_pt);
            const Vector3 w_norm = rolled_b.xform(Vector3(p2.x, p2.y, 0.0f)).normalized();
            const float u_coord  = (static_cast<float>(c) / static_cast<float>(std::max(1, stride_cols - 1))) * uv_scale_.x;
            const float v_coord  = static_cast<float>(t) * uv_scale_.y;

            const int idx = r * stride_cols + c;
            vertices.set(idx, w_pt);
            normals.set(idx, w_norm);
            uvs.set(idx, Vector2(u_coord, v_coord));
            colors.set(idx, ring_col);
        }

        if (r + 1 < num_rings) {
            for (int c = 0; c + 1 < stride_cols; ++c) {
                const int32_t i00 = r * stride_cols + c;
                const int32_t i10 = r * stride_cols + (c + 1);
                const int32_t i01 = (r + 1) * stride_cols + c;
                const int32_t i11 = (r + 1) * stride_cols + (c + 1);

                indices.push_back(i00);
                indices.push_back(i10);
                indices.push_back(i01);

                indices.push_back(i01);
                indices.push_back(i10);
                indices.push_back(i11);
            }
        }
    }

    // End caps when closed_profile && cap_ends_ && path is not wrapped
    const bool path_wrapped = use_4d ? path_spline_4d_->get_wrap() : path_spline_3d_->get_wrap();
    if (cap_ends_ && closed_profile && !path_wrapped && ring_cols >= 3) {
        const Transform3D start_xf = bishop_frames[0];
        const Transform3D end_xf   = bishop_frames[num_rings - 1];
        const int32_t start_c_idx  = static_cast<int32_t>(vertices.size());
        vertices.push_back(start_xf.origin);
        normals.push_back(start_xf.basis.get_column(2).normalized());
        uvs.push_back(Vector2(0.5f, 0.5f));
        colors.push_back(colors[0]);

        for (int c = 0; c < ring_cols; ++c) {
            indices.push_back(start_c_idx);
            indices.push_back((c + 1) % ring_cols);
            indices.push_back(c);
        }

        const int32_t end_c_idx = static_cast<int32_t>(vertices.size());
        const int32_t last_base = (num_rings - 1) * stride_cols;
        vertices.push_back(end_xf.origin);
        normals.push_back(-end_xf.basis.get_column(2).normalized());
        uvs.push_back(Vector2(0.5f, 0.5f));
        colors.push_back(colors[last_base]);

        for (int c = 0; c < ring_cols; ++c) {
            indices.push_back(end_c_idx);
            indices.push_back(last_base + c);
            indices.push_back(last_base + ((c + 1) % ring_cols));
        }
    }

    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = vertices;
    arrays[Mesh::ARRAY_NORMAL] = normals;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    arrays[Mesh::ARRAY_COLOR]  = colors;
    arrays[Mesh::ARRAY_INDEX]  = indices;
    extruded_mesh_->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

    if (RenderingServer *rs = RenderingServer::get_singleton()) {
        if (mesh_instance_rid_.is_valid()) {
            rs->instance_set_base(mesh_instance_rid_, extruded_mesh_->get_rid());
        }
    }
}

godot::Ref<godot::ConcavePolygonShape3D> SplineExtrudeMesh3D::bake_concave_collision_shape() const {
    godot::Ref<godot::ConcavePolygonShape3D> shape;
    shape.instantiate();
    if (extruded_mesh_.is_valid() && extruded_mesh_->get_surface_count() > 0) {
        shape->set_faces(extruded_mesh_->get_faces());
    }
    return shape;
}

// ============================================================================
// SplineExtrudeGizmo Implementation
// ============================================================================

SplineExtrudeGizmo::SplineExtrudeGizmo() {
    create_material("extrude_wire", godot::Color(0.38f, 0.86f, 1.0f, 0.85f), false, false, true);
    create_handle_material("extrude_handles");
}

bool SplineExtrudeGizmo::_has_gizmo(godot::Node3D *p_for_node_3d) const {
    return p_for_node_3d != nullptr && godot::Object::cast_to<SplineExtrudeMesh3D>(p_for_node_3d) != nullptr;
}

godot::String SplineExtrudeGizmo::_get_gizmo_name() const {
    return "SplineExtrudeGizmo";
}

int32_t SplineExtrudeGizmo::_get_priority() const {
    return 10;
}

void SplineExtrudeGizmo::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) {
    using namespace godot;
    if (p_gizmo.is_null()) return;
    p_gizmo->clear();

    SplineExtrudeMesh3D *node = Object::cast_to<SplineExtrudeMesh3D>(p_gizmo->get_node_3d());
    if (!node) return;

    const Ref<Spline4DResource> s4 = node->get_path_spline_4d();
    const Ref<Spline3DResource> s3 = node->get_path_spline_3d();
    const bool use_4d              = s4.is_valid() && s4->get_point_count() >= 2;
    const bool use_3d              = !use_4d && s3.is_valid() && s3->get_point_count() >= 2;
    if (!use_4d && !use_3d) return;

    PackedVector3Array lines;
    const TypedArray<Transform3D> frames = use_4d
        ? s4->compute_bishop_transforms_3d(6)
        : s3->compute_bishop_transforms(6);

    for (int64_t i = 0; i < frames.size(); ++i) {
        const Transform3D xf = frames[i];
        if (i + 1 < frames.size()) {
            const Transform3D next_xf = frames[i + 1];
            lines.push_back(xf.origin);
            lines.push_back(next_xf.origin);
        }
        // Draw cross-section profile ring every 2 samples
        if (i % 2 == 0) {
            constexpr int RING_SEG = 16;
            const Vector2 sc       = node->get_profile_scale();
            for (int k = 0; k < RING_SEG; ++k) {
                const float a0 = (static_cast<float>(k) / static_cast<float>(RING_SEG)) * Math_TAU;
                const float a1 = (static_cast<float>(k + 1) / static_cast<float>(RING_SEG)) * Math_TAU;
                const Vector3 p0 = xf.xform(Vector3(std::cos(a0) * sc.x, std::sin(a0) * sc.y, 0.0f));
                const Vector3 p1 = xf.xform(Vector3(std::cos(a1) * sc.x, std::sin(a1) * sc.y, 0.0f));
                lines.push_back(p0);
                lines.push_back(p1);
            }
        }
    }

    if (!lines.is_empty()) {
        p_gizmo->add_lines(lines, get_material("extrude_wire", p_gizmo));
    }
    if (node->get_extruded_mesh().is_valid() && node->get_extruded_mesh()->get_surface_count() > 0) {
        p_gizmo->add_collision_triangles(node->get_extruded_mesh()->generate_triangle_mesh());
    }

    PackedVector3Array handles;
    PackedInt32Array handle_ids;
    const int n_pts = use_4d ? s4->get_point_count() : s3->get_point_count();
    for (int i = 0; i < n_pts; ++i) {
        handles.push_back(use_4d ? s4->get_point_position_3d(i) : s3->get_point_position(i));
        handle_ids.push_back(HANDLE_ANCHOR_BASE + i);
    }

    if (!frames.is_empty()) {
        const Transform3D start_xf = frames[0];
        handles.push_back(start_xf.xform(Vector3(node->get_profile_scale().x, 0.0f, 0.0f)));
        handle_ids.push_back(HANDLE_PROFILE_RADIUS);
    }

    if (!handles.is_empty()) {
        p_gizmo->add_handles(handles, get_material("extrude_handles", p_gizmo), handle_ids);
    }
}

godot::String SplineExtrudeGizmo::_get_handle_name(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_gizmo;
    (void)p_secondary;
    if (p_handle_id == HANDLE_PROFILE_RADIUS) return "Extrude Profile Scale";
    if (p_handle_id == HANDLE_TOTAL_TWIST) return "Extrude Total Twist";
    return "Extrude Path Point " + godot::String::num_int64(p_handle_id);
}

godot::Variant SplineExtrudeGizmo::_get_handle_value(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    if (p_gizmo.is_null()) return godot::Variant();
    SplineExtrudeMesh3D *node = godot::Object::cast_to<SplineExtrudeMesh3D>(p_gizmo->get_node_3d());
    if (!node) return godot::Variant();
    if (p_handle_id == HANDLE_PROFILE_RADIUS) return node->get_profile_scale();
    if (node->get_path_spline_4d().is_valid() && p_handle_id < node->get_path_spline_4d()->get_point_count()) {
        return node->get_path_spline_4d()->get_point_position_3d(p_handle_id);
    }
    if (node->get_path_spline_3d().is_valid() && p_handle_id < node->get_path_spline_3d()->get_point_count()) {
        return node->get_path_spline_3d()->get_point_position(p_handle_id);
    }
    return godot::Variant();
}

void SplineExtrudeGizmo::_set_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos
) {
    using namespace godot;
    (void)p_secondary;
    if (p_gizmo.is_null() || !p_camera) return;
    SplineExtrudeMesh3D *node = Object::cast_to<SplineExtrudeMesh3D>(p_gizmo->get_node_3d());
    if (!node) return;

    const Transform3D inv_gxf = node->get_global_transform().affine_inverse();
    const Vector3 ray_from    = inv_gxf.xform(p_camera->project_ray_origin(p_screen_pos));
    const Vector3 ray_dir     = inv_gxf.basis.xform(p_camera->project_ray_normal(p_screen_pos)).normalized();

    if (p_handle_id == HANDLE_PROFILE_RADIUS) {
        const Vector3 origin = (node->get_path_spline_3d().is_valid() && node->get_path_spline_3d()->get_point_count() > 0)
            ? node->get_path_spline_3d()->get_point_position(0)
            : Vector3();
        const float denom = ray_dir.dot(-ray_dir);
        (void)denom;
        const Vector3 hit = ray_from + ray_dir * std::max(0.1f, (origin - ray_from).length());
        const float r     = std::max(0.02f, hit.distance_to(origin));
        node->set_profile_scale(Vector2(r, r));
        node->update_gizmos();
        return;
    }

    if (node->get_path_spline_4d().is_valid() && p_handle_id >= 0 && p_handle_id < node->get_path_spline_4d()->get_point_count()) {
        const Vector3 cur = node->get_path_spline_4d()->get_point_position_3d(p_handle_id);
        const float dist  = std::max(0.1f, (cur - ray_from).dot(ray_dir));
        node->get_path_spline_4d()->set_point_position_3d(p_handle_id, ray_from + ray_dir * dist);
    } else if (node->get_path_spline_3d().is_valid() && p_handle_id >= 0 && p_handle_id < node->get_path_spline_3d()->get_point_count()) {
        const Vector3 cur = node->get_path_spline_3d()->get_point_position(p_handle_id);
        const float dist  = std::max(0.1f, (cur - ray_from).dot(ray_dir));
        node->get_path_spline_3d()->set_point_position(p_handle_id, ray_from + ray_dir * dist);
    }
}

void SplineExtrudeGizmo::_commit_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    const godot::Variant &p_restore,
    bool p_cancel
) {
    using namespace godot;
    (void)p_secondary;
    if (p_gizmo.is_null()) return;
    SplineExtrudeMesh3D *node = Object::cast_to<SplineExtrudeMesh3D>(p_gizmo->get_node_3d());
    if (!node) return;

    if (p_handle_id == HANDLE_PROFILE_RADIUS) {
        if (p_cancel) {
            node->set_profile_scale(p_restore);
            return;
        }
        Ref<IdeamUndoRedo> ur = IdeamSplinesPlugin::create_undo_redo_wrapper();
        ur->create_action("Scale Extrude Profile", UndoRedo::MERGE_DISABLE, node);
        ur->add_do_method(Callable(node, "set_profile_scale").bind(node->get_profile_scale()));
        ur->add_undo_method(Callable(node, "set_profile_scale").bind(p_restore));
        ur->commit_action(false);
        return;
    }

    if (node->get_path_spline_3d().is_valid() && p_handle_id >= 0 && p_handle_id < node->get_path_spline_3d()->get_point_count()) {
        const Vector3 final_pos = node->get_path_spline_3d()->get_point_position(p_handle_id);
        node->get_path_spline_3d()->set_point_position(p_handle_id, p_restore);
        if (!p_cancel) {
            node->get_path_spline_3d()->commit_set_point_position(p_handle_id, final_pos);
        }
    }
}

} // namespace ideam::godot_ext