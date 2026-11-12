#include "spline_multi_mesh_3d.h"

#include "../ideam_splines_plugin.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ideam::godot_ext {

namespace {

[[nodiscard]] inline uint64_t scatter_splitmix64(uint64_t &r_state) noexcept {
    uint64_t z = (r_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31U);
}

[[nodiscard]] inline float scatter_rand_01(uint64_t &r_state) noexcept {
    return static_cast<float>((scatter_splitmix64(r_state) >> 11U) * (1.0 / 9007199254740992.0));
}

} // namespace

// ============================================================================
// SplineMultiMesh3D Implementation
// ============================================================================

void SplineMultiMesh3D::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(SCATTER_ALONG_PATH_3D);
    BIND_ENUM_CONSTANT(SCATTER_INSIDE_REGION_2D);

    ADD_GROUP("Spline MultiMesh Scatter", "");

    ClassDB::bind_method(D_METHOD("set_placement_mode", "mode"), &SplineMultiMesh3D::set_placement_mode);
    ClassDB::bind_method(D_METHOD("get_placement_mode"), &SplineMultiMesh3D::get_placement_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "placement_mode", PROPERTY_HINT_ENUM, "Along 3D Spline Path:0,Inside Closed 2D Spline Region:1"),
        "set_placement_mode",
        "get_placement_mode"
    );

    ClassDB::bind_method(D_METHOD("set_path_spline_3d", "spline"), &SplineMultiMesh3D::set_path_spline_3d);
    ClassDB::bind_method(D_METHOD("get_path_spline_3d"), &SplineMultiMesh3D::get_path_spline_3d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "path_spline_3d", PROPERTY_HINT_RESOURCE_TYPE, "Spline3DResource"),
        "set_path_spline_3d",
        "get_path_spline_3d"
    );

    ClassDB::bind_method(D_METHOD("set_region_spline_2d", "spline"), &SplineMultiMesh3D::set_region_spline_2d);
    ClassDB::bind_method(D_METHOD("get_region_spline_2d"), &SplineMultiMesh3D::get_region_spline_2d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "region_spline_2d", PROPERTY_HINT_RESOURCE_TYPE, "Spline2DResource"),
        "set_region_spline_2d",
        "get_region_spline_2d"
    );

    ClassDB::bind_method(D_METHOD("set_scale_curve_1d", "curve"), &SplineMultiMesh3D::set_scale_curve_1d);
    ClassDB::bind_method(D_METHOD("get_scale_curve_1d"), &SplineMultiMesh3D::get_scale_curve_1d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "scale_curve_1d", PROPERTY_HINT_RESOURCE_TYPE, "Spline1DResource"),
        "set_scale_curve_1d",
        "get_scale_curve_1d"
    );

    ClassDB::bind_method(D_METHOD("set_color_gradient_4d", "gradient"), &SplineMultiMesh3D::set_color_gradient_4d);
    ClassDB::bind_method(D_METHOD("get_color_gradient_4d"), &SplineMultiMesh3D::get_color_gradient_4d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "color_gradient_4d", PROPERTY_HINT_RESOURCE_TYPE, "Spline4DResource"),
        "set_color_gradient_4d",
        "get_color_gradient_4d"
    );

    ClassDB::bind_method(D_METHOD("set_instance_mesh", "mesh"), &SplineMultiMesh3D::set_instance_mesh);
    ClassDB::bind_method(D_METHOD("get_instance_mesh"), &SplineMultiMesh3D::get_instance_mesh);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "instance_mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"),
        "set_instance_mesh",
        "get_instance_mesh"
    );

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &SplineMultiMesh3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &SplineMultiMesh3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("set_instance_count", "count"), &SplineMultiMesh3D::set_instance_count);
    ClassDB::bind_method(D_METHOD("get_instance_count"), &SplineMultiMesh3D::get_instance_count);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "instance_count"), "set_instance_count", "get_instance_count");

    ClassDB::bind_method(D_METHOD("set_scatter_width", "width"), &SplineMultiMesh3D::set_scatter_width);
    ClassDB::bind_method(D_METHOD("get_scatter_width"), &SplineMultiMesh3D::get_scatter_width);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scatter_width"), "set_scatter_width", "get_scatter_width");

    ClassDB::bind_method(D_METHOD("set_random_yaw_radians", "yaw"), &SplineMultiMesh3D::set_random_yaw_radians);
    ClassDB::bind_method(D_METHOD("get_random_yaw_radians"), &SplineMultiMesh3D::get_random_yaw_radians);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_yaw_radians"), "set_random_yaw_radians", "get_random_yaw_radians");

    ClassDB::bind_method(D_METHOD("set_random_scale_jitter", "jitter"), &SplineMultiMesh3D::set_random_scale_jitter);
    ClassDB::bind_method(D_METHOD("get_random_scale_jitter"), &SplineMultiMesh3D::get_random_scale_jitter);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_scale_jitter"), "set_random_scale_jitter", "get_random_scale_jitter");

    ClassDB::bind_method(D_METHOD("set_align_to_spline_frame", "align"), &SplineMultiMesh3D::set_align_to_spline_frame);
    ClassDB::bind_method(D_METHOD("get_align_to_spline_frame"), &SplineMultiMesh3D::get_align_to_spline_frame);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "align_to_spline_frame"), "set_align_to_spline_frame", "get_align_to_spline_frame");

    ClassDB::bind_method(D_METHOD("set_use_weighted_distribution", "weighted"), &SplineMultiMesh3D::set_use_weighted_distribution);
    ClassDB::bind_method(D_METHOD("get_use_weighted_distribution"), &SplineMultiMesh3D::get_use_weighted_distribution);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_weighted_distribution"), "set_use_weighted_distribution", "get_use_weighted_distribution");

    ClassDB::bind_method(D_METHOD("set_random_seed", "seed"), &SplineMultiMesh3D::set_random_seed);
    ClassDB::bind_method(D_METHOD("get_random_seed"), &SplineMultiMesh3D::get_random_seed);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "random_seed"), "set_random_seed", "get_random_seed");

    ClassDB::bind_method(D_METHOD("get_multimesh"), &SplineMultiMesh3D::get_multimesh);
    ClassDB::bind_method(D_METHOD("rebuild_instances"), &SplineMultiMesh3D::rebuild_instances);
    ClassDB::bind_method(D_METHOD("_on_dependency_changed"), &SplineMultiMesh3D::_on_dependency_changed);
}

SplineMultiMesh3D::SplineMultiMesh3D() {
    multimesh_.instantiate();
    multimesh_->set_transform_format(godot::MultiMesh::TRANSFORM_3D);
    multimesh_->set_use_colors(true);
    if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
        multimesh_instance_rid_ = rs->instance_create();
        rs->instance_set_base(multimesh_instance_rid_, multimesh_->get_rid());
    }
    set_notify_transform(true);
}

SplineMultiMesh3D::~SplineMultiMesh3D() {
    const godot::Callable cb(this, "_on_dependency_changed");
    if (path_spline_3d_.is_valid() && path_spline_3d_->is_connected("changed", cb)) path_spline_3d_->disconnect("changed", cb);
    if (region_spline_2d_.is_valid() && region_spline_2d_->is_connected("changed", cb)) region_spline_2d_->disconnect("changed", cb);
    if (scale_curve_1d_.is_valid() && scale_curve_1d_->is_connected("changed", cb)) scale_curve_1d_->disconnect("changed", cb);
    if (color_gradient_4d_.is_valid() && color_gradient_4d_->is_connected("changed", cb)) color_gradient_4d_->disconnect("changed", cb);

    if (multimesh_instance_rid_.is_valid()) {
        if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
            rs->free_rid(multimesh_instance_rid_);
        }
    }
}

void SplineMultiMesh3D::_notification(int p_what) {
    using namespace godot;
    RenderingServer *rs = RenderingServer::get_singleton();
    if (!rs || !multimesh_instance_rid_.is_valid()) return;

    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            Ref<World3D> w3d = get_world_3d();
            if (w3d.is_valid()) {
                rs->instance_set_scenario(multimesh_instance_rid_, w3d->get_scenario());
            }
            rs->instance_set_transform(multimesh_instance_rid_, get_global_transform());
            rs->instance_set_visible(multimesh_instance_rid_, is_visible_in_tree());
            rebuild_instances();
            break;
        }
        case NOTIFICATION_EXIT_WORLD: {
            rs->instance_set_scenario(multimesh_instance_rid_, RID());
            break;
        }
        case NOTIFICATION_TRANSFORM_CHANGED: {
            rs->instance_set_transform(multimesh_instance_rid_, get_global_transform());
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            rs->instance_set_visible(multimesh_instance_rid_, is_visible_in_tree());
            break;
        }
        default:
            break;
    }
}

void SplineMultiMesh3D::_on_dependency_changed() {
    rebuild_instances();
    update_gizmos();
}

void SplineMultiMesh3D::set_path_spline_3d(const godot::Ref<Spline3DResource> &p_spline) {
    if (path_spline_3d_ == p_spline) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (path_spline_3d_.is_valid() && path_spline_3d_->is_connected("changed", cb)) path_spline_3d_->disconnect("changed", cb);
    path_spline_3d_ = p_spline;
    if (path_spline_3d_.is_valid() && !path_spline_3d_->is_connected("changed", cb)) path_spline_3d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_region_spline_2d(const godot::Ref<Spline2DResource> &p_spline) {
    if (region_spline_2d_ == p_spline) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (region_spline_2d_.is_valid() && region_spline_2d_->is_connected("changed", cb)) region_spline_2d_->disconnect("changed", cb);
    region_spline_2d_ = p_spline;
    if (region_spline_2d_.is_valid() && !region_spline_2d_->is_connected("changed", cb)) region_spline_2d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_scale_curve_1d(const godot::Ref<Spline1DResource> &p_curve) {
    if (scale_curve_1d_ == p_curve) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (scale_curve_1d_.is_valid() && scale_curve_1d_->is_connected("changed", cb)) scale_curve_1d_->disconnect("changed", cb);
    scale_curve_1d_ = p_curve;
    if (scale_curve_1d_.is_valid() && !scale_curve_1d_->is_connected("changed", cb)) scale_curve_1d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_color_gradient_4d(const godot::Ref<Spline4DResource> &p_gradient) {
    if (color_gradient_4d_ == p_gradient) return;
    const godot::Callable cb(this, "_on_dependency_changed");
    if (color_gradient_4d_.is_valid() && color_gradient_4d_->is_connected("changed", cb)) color_gradient_4d_->disconnect("changed", cb);
    color_gradient_4d_ = p_gradient;
    if (color_gradient_4d_.is_valid() && !color_gradient_4d_->is_connected("changed", cb)) color_gradient_4d_->connect("changed", cb);
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_instance_mesh(const godot::Ref<godot::Mesh> &p_mesh) {
    instance_mesh_ = p_mesh;
    if (multimesh_.is_valid()) {
        multimesh_->set_mesh(instance_mesh_);
    }
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_material_override(const godot::Ref<godot::Material> &p_material) {
    material_override_ = p_material;
    if (godot::RenderingServer *rs = godot::RenderingServer::get_singleton()) {
        if (multimesh_instance_rid_.is_valid()) {
            rs->instance_geometry_set_material_override(
                multimesh_instance_rid_,
                material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
            );
        }
    }
}

void SplineMultiMesh3D::set_placement_mode(SplineMultiMeshPlacementMode p_mode) {
    placement_mode_ = p_mode;
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_instance_count(int p_count) {
    instance_count_ = std::clamp(p_count, 0, 16384);
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_scatter_width(real_t p_width) {
    scatter_width_ = std::max(static_cast<real_t>(0.0), p_width);
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_random_yaw_radians(real_t p_yaw) {
    random_yaw_radians_ = std::max(static_cast<real_t>(0.0), p_yaw);
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_random_scale_jitter(real_t p_jitter) {
    random_scale_jitter_ = std::clamp(p_jitter, static_cast<real_t>(0.0), static_cast<real_t>(0.95));
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_align_to_spline_frame(bool p_align) {
    align_to_spline_frame_ = p_align;
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_use_weighted_distribution(bool p_weighted) {
    use_weighted_distribution_ = p_weighted;
    _on_dependency_changed();
}

void SplineMultiMesh3D::set_random_seed(int64_t p_seed) {
    random_seed_ = p_seed;
    _on_dependency_changed();
}

void SplineMultiMesh3D::rebuild_instances() {
    using namespace godot;
    if (multimesh_.is_null()) return;
    multimesh_->set_mesh(instance_mesh_);

    if (instance_count_ <= 0) {
        multimesh_->set_instance_count(0);
        return;
    }

    uint64_t rng = static_cast<uint64_t>(random_seed_) ^ 0xCAFEF00D12345678ULL;

    if (placement_mode_ == SCATTER_INSIDE_REGION_2D &&
        region_spline_2d_.is_valid() && region_spline_2d_->get_point_count() >= 3) {
        const Rect2 bounds = region_spline_2d_->get_bounding_rect(true);
        std::vector<Transform3D> valid_xforms;
        std::vector<Color> valid_cols;
        valid_xforms.reserve(static_cast<size_t>(instance_count_));
        valid_cols.reserve(static_cast<size_t>(instance_count_));

        const int max_attempts = instance_count_ * 24;
        for (int a = 0; a < max_attempts && static_cast<int>(valid_xforms.size()) < instance_count_; ++a) {
            const Vector2 pt2(
                bounds.position.x + scatter_rand_01(rng) * bounds.size.x,
                bounds.position.y + scatter_rand_01(rng) * bounds.size.y
            );
            if (!region_spline_2d_->contains_point(pt2, 8)) continue;

            const float norm_u = (bounds.size.x > 1e-4f) ? ((pt2.x - bounds.position.x) / bounds.size.x) : 0.5f;
            const float yaw    = (scatter_rand_01(rng) * 2.0f - 1.0f) * static_cast<float>(random_yaw_radians_);
            float sc           = 1.0f + (scatter_rand_01(rng) * 2.0f - 1.0f) * static_cast<float>(random_scale_jitter_);
            if (scale_curve_1d_.is_valid() && scale_curve_1d_->get_point_count() > 0) {
                sc *= std::max(0.05f, static_cast<float>(scale_curve_1d_->sample(norm_u, true)));
            }

            Basis b(Vector3(0.0f, 1.0f, 0.0f), yaw);
            b.scale(Vector3(sc, sc, sc));
            valid_xforms.emplace_back(b, Vector3(pt2.x, 0.0f, pt2.y));

            const Color col = (color_gradient_4d_.is_valid() && color_gradient_4d_->get_point_count() > 0)
                ? color_gradient_4d_->sample_color(norm_u, true)
                : Color(1.0f, 1.0f, 1.0f, 1.0f);
            valid_cols.push_back(col);
        }

        const int count = static_cast<int>(valid_xforms.size());
        multimesh_->set_instance_count(count);
        for (int i = 0; i < count; ++i) {
            multimesh_->set_instance_transform(i, valid_xforms[static_cast<size_t>(i)]);
            multimesh_->set_instance_color(i, valid_cols[static_cast<size_t>(i)]);
        }
        return;
    }

    if (path_spline_3d_.is_null() || path_spline_3d_->get_point_count() < 2) {
        multimesh_->set_instance_count(0);
        return;
    }

    multimesh_->set_instance_count(instance_count_);
    for (int i = 0; i < instance_count_; ++i) {
        const real_t t = (instance_count_ == 1)
            ? static_cast<real_t>(0.5)
            : (static_cast<real_t>(i) / static_cast<real_t>(instance_count_ - 1));

        Transform3D xf = align_to_spline_frame_
            ? path_spline_3d_->get_transform_at_percent(t, use_weighted_distribution_)
            : Transform3D(Basis(), path_spline_3d_->sample(t, use_weighted_distribution_));

        if (scatter_width_ > static_cast<real_t>(1e-4)) {
            const float lat = (scatter_rand_01(rng) - 0.5f) * static_cast<float>(scatter_width_);
            xf.origin += xf.basis.get_column(0) * lat;
        }

        if (std::abs(random_yaw_radians_) > static_cast<real_t>(1e-4)) {
            const float yaw = (scatter_rand_01(rng) * 2.0f - 1.0f) * static_cast<float>(random_yaw_radians_);
            xf.basis = xf.basis * Basis(Vector3(0.0f, 1.0f, 0.0f), yaw);
        }

        float sc = 1.0f + (scatter_rand_01(rng) * 2.0f - 1.0f) * static_cast<float>(random_scale_jitter_);
        if (scale_curve_1d_.is_valid() && scale_curve_1d_->get_point_count() > 0) {
            sc *= std::max(0.05f, static_cast<float>(scale_curve_1d_->sample(t, true)));
        }
        xf.basis = xf.basis.scaled(Vector3(sc, sc, sc));

        const Color col = (color_gradient_4d_.is_valid() && color_gradient_4d_->get_point_count() > 0)
            ? color_gradient_4d_->sample_color(t, true)
            : Color(1.0f, 1.0f, 1.0f, 1.0f);

        multimesh_->set_instance_transform(i, xf);
        multimesh_->set_instance_color(i, col);
    }
}

// ============================================================================
// SplineScatterGizmo Implementation
// ============================================================================

SplineScatterGizmo::SplineScatterGizmo() {
    create_material("scatter_wire", godot::Color(0.46f, 0.94f, 0.58f, 0.85f), false, false, true);
    create_handle_material("scatter_handles");
}

bool SplineScatterGizmo::_has_gizmo(godot::Node3D *p_for_node_3d) const {
    return p_for_node_3d != nullptr && godot::Object::cast_to<SplineMultiMesh3D>(p_for_node_3d) != nullptr;
}

godot::String SplineScatterGizmo::_get_gizmo_name() const {
    return "SplineScatterGizmo";
}

int32_t SplineScatterGizmo::_get_priority() const {
    return 10;
}

void SplineScatterGizmo::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) {
    using namespace godot;
    if (p_gizmo.is_null()) return;
    p_gizmo->clear();

    SplineMultiMesh3D *node = Object::cast_to<SplineMultiMesh3D>(p_gizmo->get_node_3d());
    if (!node) return;

    PackedVector3Array lines;
    PackedVector3Array handles;
    PackedInt32Array handle_ids;

    const Ref<Spline3DResource> s3 = node->get_path_spline_3d();
    if (s3.is_valid() && s3->get_point_count() >= 2) {
        const PackedVector3Array smoothed = s3->get_smoothed_points(10);
        for (int64_t i = 0; i + 1 < smoothed.size(); ++i) {
            lines.push_back(smoothed[i]);
            lines.push_back(smoothed[i + 1]);
        }

        const float hw = static_cast<float>(node->get_scatter_width() * 0.5);
        if (hw > 0.01f) {
            const TypedArray<Transform3D> frames = s3->compute_bishop_transforms(6);
            for (int64_t i = 0; i + 1 < frames.size(); ++i) {
                const Transform3D f0 = frames[i];
                const Transform3D f1 = frames[i + 1];
                const Vector3 r0     = f0.basis.get_column(0) * hw;
                const Vector3 r1     = f1.basis.get_column(0) * hw;
                lines.push_back(f0.origin - r0);
                lines.push_back(f1.origin - r1);
                lines.push_back(f0.origin + r0);
                lines.push_back(f1.origin + r1);
            }
        }

        for (int i = 0; i < s3->get_point_count(); ++i) {
            handles.push_back(s3->get_point_position(i));
            handle_ids.push_back(HANDLE_ANCHOR_BASE + i);
        }
        const Transform3D xf0 = s3->get_transform_at_percent(0.0f);
        handles.push_back(xf0.origin + xf0.basis.get_column(0) * std::max(0.25f, hw));
        handle_ids.push_back(HANDLE_SCATTER_WIDTH);
    }

    if (!lines.is_empty()) {
        p_gizmo->add_lines(lines, get_material("scatter_wire", p_gizmo));
    }
    if (!handles.is_empty()) {
        p_gizmo->add_handles(handles, get_material("scatter_handles", p_gizmo), handle_ids);
    }
}

godot::String SplineScatterGizmo::_get_handle_name(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_gizmo;
    (void)p_secondary;
    if (p_handle_id == HANDLE_SCATTER_WIDTH) return "Scatter Ribbon Width";
    return "Scatter Path Point " + godot::String::num_int64(p_handle_id);
}

godot::Variant SplineScatterGizmo::_get_handle_value(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    if (p_gizmo.is_null()) return godot::Variant();
    SplineMultiMesh3D *node = godot::Object::cast_to<SplineMultiMesh3D>(p_gizmo->get_node_3d());
    if (!node) return godot::Variant();
    if (p_handle_id == HANDLE_SCATTER_WIDTH) return node->get_scatter_width();
    if (node->get_path_spline_3d().is_valid() && p_handle_id < node->get_path_spline_3d()->get_point_count()) {
        return node->get_path_spline_3d()->get_point_position(p_handle_id);
    }
    return godot::Variant();
}

void SplineScatterGizmo::_set_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos
) {
    using namespace godot;
    (void)p_secondary;
    if (p_gizmo.is_null() || !p_camera) return;
    SplineMultiMesh3D *node = Object::cast_to<SplineMultiMesh3D>(p_gizmo->get_node_3d());
    if (!node || node->get_path_spline_3d().is_null()) return;

    const Transform3D inv_gxf = node->get_global_transform().affine_inverse();
    const Vector3 ray_from    = inv_gxf.xform(p_camera->project_ray_origin(p_screen_pos));
    const Vector3 ray_dir     = inv_gxf.basis.xform(p_camera->project_ray_normal(p_screen_pos)).normalized();

    if (p_handle_id == HANDLE_SCATTER_WIDTH) {
        const Vector3 p0  = node->get_path_spline_3d()->get_point_position(0);
        const Vector3 hit = ray_from + ray_dir * std::max(0.1f, (p0 - ray_from).dot(ray_dir));
        node->set_scatter_width(hit.distance_to(p0) * 2.0f);
        node->update_gizmos();
        return;
    }

    if (p_handle_id >= 0 && p_handle_id < node->get_path_spline_3d()->get_point_count()) {
        const Vector3 cur  = node->get_path_spline_3d()->get_point_position(p_handle_id);
        const float dist   = std::max(0.1f, (cur - ray_from).dot(ray_dir));
        node->get_path_spline_3d()->set_point_position(p_handle_id, ray_from + ray_dir * dist);
    }
}

void SplineScatterGizmo::_commit_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    const godot::Variant &p_restore,
    bool p_cancel
) {
    using namespace godot;
    (void)p_secondary;
    if (p_gizmo.is_null()) return;
    SplineMultiMesh3D *node = Object::cast_to<SplineMultiMesh3D>(p_gizmo->get_node_3d());
    if (!node) return;

    if (p_handle_id == HANDLE_SCATTER_WIDTH) {
        if (p_cancel) {
            node->set_scatter_width(p_restore);
            return;
        }
        Ref<IdeamUndoRedo> ur = IdeamSplinesPlugin::create_undo_redo_wrapper();
        ur->create_action("Adjust Scatter Ribbon Width", UndoRedo::MERGE_DISABLE, node);
        ur->add_do_method(Callable(node, "set_scatter_width").bind(node->get_scatter_width()));
        ur->add_undo_method(Callable(node, "set_scatter_width").bind(p_restore));
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