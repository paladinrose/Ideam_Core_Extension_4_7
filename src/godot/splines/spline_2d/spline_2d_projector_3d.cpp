#include "spline_2d_projector_3d.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

void Spline2DProjector3D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SPLINE_PROJ_RIBBON_STRIP);
    BIND_ENUM_CONSTANT(SPLINE_PROJ_FILLED_POLYGON);
    BIND_ENUM_CONSTANT(SPLINE_PROJ_EXTRUDED_WALL);

    ADD_GROUP("Spline 2D Projector", "");

    ClassDB::bind_method(D_METHOD("set_spline_2d", "spline"), &Spline2DProjector3D::set_spline_2d);
    ClassDB::bind_method(D_METHOD("get_spline_2d"), &Spline2DProjector3D::get_spline_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "spline_2d", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_spline_2d",
        "get_spline_2d"
    );

    ClassDB::bind_method(D_METHOD("set_output_mode", "mode"), &Spline2DProjector3D::set_output_mode);
    ClassDB::bind_method(D_METHOD("get_output_mode"), &Spline2DProjector3D::get_output_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "output_mode",
            PROPERTY_HINT_ENUM,
            "Conformal Ribbon Strip:0,Filled Draped Polygon:1,Extruded Normal Wall:2"
        ),
        "set_output_mode",
        "get_output_mode"
    );

    ClassDB::bind_method(D_METHOD("set_ribbon_width", "width"), &Spline2DProjector3D::set_ribbon_width);
    ClassDB::bind_method(D_METHOD("get_ribbon_width"), &Spline2DProjector3D::get_ribbon_width);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ribbon_width"), "set_ribbon_width", "get_ribbon_width");

    ClassDB::bind_method(D_METHOD("set_curve_subdivisions", "subdivs"), &Spline2DProjector3D::set_curve_subdivisions);
    ClassDB::bind_method(D_METHOD("get_curve_subdivisions"), &Spline2DProjector3D::get_curve_subdivisions);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "curve_subdivisions"), "set_curve_subdivisions", "get_curve_subdivisions");

    ClassDB::bind_method(D_METHOD("set_width_profile_1d", "profile"), &Spline2DProjector3D::set_width_profile_1d);
    ClassDB::bind_method(D_METHOD("get_width_profile_1d"), &Spline2DProjector3D::get_width_profile_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "width_profile_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_width_profile_1d",
        "get_width_profile_1d"
    );

    ClassDB::bind_method(D_METHOD("set_color_gradient_4d", "gradient"), &Spline2DProjector3D::set_color_gradient_4d);
    ClassDB::bind_method(D_METHOD("get_color_gradient_4d"), &Spline2DProjector3D::get_color_gradient_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "color_gradient_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_color_gradient_4d",
        "get_color_gradient_4d"
    );

    ClassDB::bind_method(
        D_METHOD("sample_projected_point_3d", "percent", "weighted"),
        &Spline2DProjector3D::sample_projected_point_3d,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("sample_projected_transform_3d", "percent", "weighted"),
        &Spline2DProjector3D::sample_projected_transform_3d,
        DEFVAL(true)
    );
    ClassDB::bind_method(
        D_METHOD("bake_to_spline_3d_resource", "bake_subdivided_points"),
        &Spline2DProjector3D::bake_to_spline_3d_resource,
        DEFVAL(false)
    );

    ClassDB::bind_method(D_METHOD("_on_spline_dependency_changed"), &Spline2DProjector3D::_on_spline_dependency_changed);
}

Spline2DProjector3D::Spline2DProjector3D() = default;

Spline2DProjector3D::~Spline2DProjector3D() {
    const godot::Callable cb(this, "_on_spline_dependency_changed");
    if (spline_2d_.is_valid() && spline_2d_->is_connected("changed", cb)) {
        spline_2d_->disconnect("changed", cb);
    }
    if (width_profile_1d_.is_valid() && width_profile_1d_->is_connected("changed", cb)) {
        width_profile_1d_->disconnect("changed", cb);
    }
    if (color_gradient_4d_.is_valid() && color_gradient_4d_->is_connected("changed", cb)) {
        color_gradient_4d_->disconnect("changed", cb);
    }
}

void Spline2DProjector3D::set_spline_2d(const godot::Ref<Spline2DResource> &p_spline) {
    if (spline_2d_ == p_spline) return;
    const godot::Callable cb(this, "_on_spline_dependency_changed");
    if (spline_2d_.is_valid() && spline_2d_->is_connected("changed", cb)) {
        spline_2d_->disconnect("changed", cb);
    }
    spline_2d_ = p_spline;
    if (spline_2d_.is_valid() && !spline_2d_->is_connected("changed", cb)) {
        spline_2d_->connect("changed", cb);
    }
    _rebuild_projected_geometry();
}

void Spline2DProjector3D::set_width_profile_1d(const godot::Ref<Spline1DResource> &p_profile) {
    if (width_profile_1d_ == p_profile) return;
    const godot::Callable cb(this, "_on_spline_dependency_changed");
    if (width_profile_1d_.is_valid() && width_profile_1d_->is_connected("changed", cb)) {
        width_profile_1d_->disconnect("changed", cb);
    }
    width_profile_1d_ = p_profile;
    if (width_profile_1d_.is_valid() && !width_profile_1d_->is_connected("changed", cb)) {
        width_profile_1d_->connect("changed", cb);
    }
    _rebuild_projected_geometry();
}

void Spline2DProjector3D::set_color_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient) {
    if (color_gradient_4d_ == p_gradient) return;
    const godot::Callable cb(this, "_on_spline_dependency_changed");
    if (color_gradient_4d_.is_valid() && color_gradient_4d_->is_connected("changed", cb)) {
        color_gradient_4d_->disconnect("changed", cb);
    }
    color_gradient_4d_ = p_gradient;
    if (color_gradient_4d_.is_valid() && !color_gradient_4d_->is_connected("changed", cb)) {
        color_gradient_4d_->connect("changed", cb);
    }
    _rebuild_projected_geometry();
}

void Spline2DProjector3D::set_output_mode(Spline2DProjectionOutputMode p_mode) {
    if (output_mode_ == p_mode) return;
    output_mode_ = p_mode;
    _rebuild_projected_geometry();
}

void Spline2DProjector3D::set_ribbon_width(real_t p_width) {
    ribbon_width_ = std::max(static_cast<real_t>(0.01), p_width);
    _rebuild_projected_geometry();
}

void Spline2DProjector3D::set_curve_subdivisions(int p_subdivs) {
    curve_subdivisions_ = std::clamp(p_subdivs, 2, 128);
    _rebuild_projected_geometry();
}

void Spline2DProjector3D::_on_spline_dependency_changed() {
    _rebuild_projected_geometry();
}

godot::Vector3 Spline2DProjector3D::sample_projected_point_3d(real_t p_percent, bool p_weighted) const {
    if (spline_2d_.is_null() || spline_2d_->get_point_count() <= 0) return godot::Vector3();
    return project_point_2d(spline_2d_->sample(p_percent, p_weighted));
}

godot::Transform3D Spline2DProjector3D::sample_projected_transform_3d(real_t p_percent, bool p_weighted) const {
    if (spline_2d_.is_null() || spline_2d_->get_point_count() <= 0) return godot::Transform3D();
    return project_transform_2d(
        spline_2d_->sample(p_percent, p_weighted),
        spline_2d_->get_tangent(p_percent, p_weighted)
    );
}

godot::Ref<Spline3DResource> Spline2DProjector3D::bake_to_spline_3d_resource(bool p_bake_subdivided_points) const {
    godot::Ref<Spline3DResource> out;
    out.instantiate();
    if (spline_2d_.is_null() || spline_2d_->get_point_count() <= 0) return out;

    out->set_wrap(spline_2d_->get_wrap());
    out->set_default_curve_type(p_bake_subdivided_points ? CURVE_CATMULL_ROM : spline_2d_->get_default_curve_type());

    if (p_bake_subdivided_points) {
        const godot::PackedVector2Array smoothed = spline_2d_->get_smoothed_points(curve_subdivisions_);
        for (int64_t i = 0; i < smoothed.size(); ++i) {
            out->add_point(project_point_2d(smoothed[i]), CURVE_CATMULL_ROM);
        }
        return out;
    }

    const int n = spline_2d_->get_point_count();
    for (int i = 0; i < n; ++i) {
        const godot::Vector2 p2 = spline_2d_->get_point_position(i);
        const SplineCurveType ctype = (i < spline_2d_->get_section_count())
            ? spline_2d_->get_section_curve_type(i)
            : spline_2d_->get_default_curve_type();
        const real_t w = (i < spline_2d_->get_section_count())
            ? spline_2d_->get_section_weight(i)
            : spline_2d_->get_default_weight();

        out->add_point(project_point_2d(p2), ctype, w);

        if (spline_2d_->get_point_use_back_handle(i)) {
            const godot::Vector2 bh2 = p2 + spline_2d_->get_point_back_handle(i);
            out->set_point_back_handle(i, project_point_2d(bh2) - out->get_point_position(i), true);
        }
        if (spline_2d_->get_point_use_fore_handle(i)) {
            const godot::Vector2 fh2 = p2 + spline_2d_->get_point_fore_handle(i);
            out->set_point_fore_handle(i, project_point_2d(fh2) - out->get_point_position(i), true);
        }
    }

    return out;
}

void Spline2DProjector3D::_rebuild_projected_geometry() {
    using namespace godot;

    if (spline_2d_.is_null() || spline_2d_->get_point_count() < 2) {
        Geometry2DProjector3D::_rebuild_projected_geometry();
        return;
    }

    if (output_mode_ == SPLINE_PROJ_FILLED_POLYGON && spline_2d_->get_point_count() >= 3) {
        const PackedVector2Array pts_2d = spline_2d_->get_smoothed_points(curve_subdivisions_);
        const PackedInt32Array tris_2d  = spline_2d_->triangulate(curve_subdivisions_);
        const Ref<ArrayMesh> poly_mesh  = build_projected_polygon_mesh(
            pts_2d,
            tris_2d,
            get_extrude_thickness(),
            get_surface_albedo()
        );
        _commit_generated_mesh(poly_mesh);
        return;
    }

    // Sample spline along [0, 1] to build either a conformal 3D ribbon strip or extruded normal wall
    const int total_samples = std::max(4, spline_2d_->get_section_count() * curve_subdivisions_ + 1);
    PackedVector3Array vertices;
    PackedVector3Array normals;
    PackedVector2Array uvs;
    PackedColorArray colors;
    PackedInt32Array indices;

    vertices.resize(total_samples * 2);
    normals.resize(total_samples * 2);
    uvs.resize(total_samples * 2);
    colors.resize(total_samples * 2);

    const float half_w_base = static_cast<float>(ribbon_width_ * 0.5);
    const float wall_h      = std::max(0.05f, static_cast<float>(get_extrude_thickness() > 1e-4 ? get_extrude_thickness() : ribbon_width_));

    for (int i = 0; i < total_samples; ++i) {
        const real_t t = static_cast<real_t>(i) / static_cast<real_t>(total_samples - 1);
        const Vector2 p2 = spline_2d_->sample(t, true);
        const Vector2 n2 = spline_2d_->get_normal(t, true);

        const float w_scale = (width_profile_1d_.is_valid() && width_profile_1d_->get_point_count() > 0)
            ? static_cast<float>(width_profile_1d_->sample(t, true))
            : 1.0f;
        const Color col = (color_gradient_4d_.is_valid() && color_gradient_4d_->get_point_count() > 0)
            ? color_gradient_4d_->sample_color(t, true)
            : get_surface_albedo();

        if (output_mode_ == SPLINE_PROJ_EXTRUDED_WALL) {
            Vector3 base_p, surf_n;
            (void)project_point_2d_with_normal(p2, base_p, surf_n);
            const Vector3 top_p = base_p + surf_n * (wall_h * w_scale);
            const Transform3D xf = project_transform_2d(p2, spline_2d_->get_tangent(t, true));
            const Vector3 wall_n = xf.basis.get_column(0);

            vertices.set(i * 2, base_p);
            vertices.set(i * 2 + 1, top_p);
            normals.set(i * 2, wall_n);
            normals.set(i * 2 + 1, wall_n);
        } else {
            // Conformal Ribbon Strip: project left and right 2D offset points directly onto 3D topology!
            const float hw = half_w_base * w_scale;
            Vector3 left_p, left_n, right_p, right_n;
            (void)project_point_2d_with_normal(p2 - n2 * hw, left_p, left_n);
            (void)project_point_2d_with_normal(p2 + n2 * hw, right_p, right_n);

            vertices.set(i * 2, left_p);
            vertices.set(i * 2 + 1, right_p);
            normals.set(i * 2, left_n);
            normals.set(i * 2 + 1, right_n);
        }

        uvs.set(i * 2, Vector2(0.0f, static_cast<float>(t)));
        uvs.set(i * 2 + 1, Vector2(1.0f, static_cast<float>(t)));
        colors.set(i * 2, col);
        colors.set(i * 2 + 1, col);

        if (i + 1 < total_samples) {
            const int32_t v0 = i * 2;
            const int32_t v1 = i * 2 + 1;
            const int32_t v2 = (i + 1) * 2;
            const int32_t v3 = (i + 1) * 2 + 1;
            indices.push_back(v0);
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v2);
            indices.push_back(v1);
            indices.push_back(v3);
        }
    }

    Ref<ArrayMesh> mesh;
    mesh.instantiate();
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = vertices;
    arrays[Mesh::ARRAY_NORMAL] = normals;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    arrays[Mesh::ARRAY_COLOR]  = colors;
    arrays[Mesh::ARRAY_INDEX]  = indices;
    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

    _commit_generated_mesh(mesh);
}

} // namespace ideam::godot_ext