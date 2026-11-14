#include "container_3d.h"
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// Container3D
// ============================================================================

void Container3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_cell_spacing", "spacing"), &Container3D::set_cell_spacing);
    ClassDB::bind_method(D_METHOD("get_cell_spacing"), &Container3D::get_cell_spacing);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "cell_spacing"), "set_cell_spacing", "get_cell_spacing");

    ClassDB::bind_method(D_METHOD("set_auto_refit_children", "auto_refit"), &Container3D::set_auto_refit_children);
    ClassDB::bind_method(D_METHOD("get_auto_refit_children"), &Container3D::get_auto_refit_children);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_refit_children"), "set_auto_refit_children", "get_auto_refit_children");

    ClassDB::bind_method(D_METHOD("refit_children_3d"), &Container3D::refit_children_3d);
}

void Container3D::set_cell_spacing(const godot::Vector3 &p_spacing) {
    cell_spacing_ = p_spacing;
    if (auto_refit_children_) {
        refit_children_3d();
    }
}

void Container3D::set_auto_refit_children(bool p_auto) {
    auto_refit_children_ = p_auto;
}

void Container3D::refit_children_3d() {
    const int n = get_child_count();
    for (int i = 0; i < n; ++i) {
        if (auto *child = godot::Object::cast_to<godot::Node3D>(get_child(i))) {
            child->set_position(cell_spacing_ * static_cast<float>(i));
        }
    }
}

// ============================================================================
// BoxContainer3D
// ============================================================================

void BoxContainer3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_primary_axis", "axis"), &BoxContainer3D::set_primary_axis);
    ClassDB::bind_method(D_METHOD("get_primary_axis"), &BoxContainer3D::get_primary_axis);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "primary_axis", PROPERTY_HINT_ENUM, "X Axis:0,Y Axis (Vertical):1,Z Axis:2"),
        "set_primary_axis",
        "get_primary_axis"
    );

    ClassDB::bind_method(D_METHOD("set_center_align", "center"), &BoxContainer3D::set_center_align);
    ClassDB::bind_method(D_METHOD("get_center_align"), &BoxContainer3D::get_center_align);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "center_align"), "set_center_align", "get_center_align");
}

void BoxContainer3D::set_primary_axis(int p_axis) {
    primary_axis_ = std::clamp(p_axis, 0, 2);
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void BoxContainer3D::set_center_align(bool p_center) {
    center_align_ = p_center;
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void BoxContainer3D::refit_children_3d() {
    const int n = get_child_count();
    if (n <= 0) return;

    const godot::Vector3 spacing = get_cell_spacing();
    const float gap = (primary_axis_ == 0) ? spacing.x : ((primary_axis_ == 1) ? spacing.y : spacing.z);

    float cursor = 0.0f;
    for (int i = 0; i < n; ++i) {
        auto *child = godot::Object::cast_to<godot::Node3D>(get_child(i));
        if (!child) continue;

        float half_span = 0.5f;
        if (auto *sem = godot::Object::cast_to<SemanticStructure3D>(child)) {
            const godot::Vector3 ext = sem->get_rest_obb_extents() + sem->get_margin_extents();
            half_span = (primary_axis_ == 0) ? ext.x : ((primary_axis_ == 1) ? ext.y : ext.z);
        }

        cursor += half_span;
        godot::Vector3 pos(0.0f, 0.0f, 0.0f);
        if (primary_axis_ == 0)      pos.x = cursor;
        else if (primary_axis_ == 1) pos.y = cursor;
        else                         pos.z = cursor;
        child->set_position(pos);
        cursor += half_span + gap;
    }
}

// ============================================================================
// GridContainer3D
// ============================================================================

void GridContainer3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_columns_x", "columns"), &GridContainer3D::set_columns_x);
    ClassDB::bind_method(D_METHOD("get_columns_x"), &GridContainer3D::get_columns_x);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "columns_x", PROPERTY_HINT_RANGE, "1,64,1"), "set_columns_x", "get_columns_x");

    ClassDB::bind_method(D_METHOD("set_rows_y", "rows"), &GridContainer3D::set_rows_y);
    ClassDB::bind_method(D_METHOD("get_rows_y"), &GridContainer3D::get_rows_y);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "rows_y", PROPERTY_HINT_RANGE, "1,64,1"), "set_rows_y", "get_rows_y");
}

void GridContainer3D::set_columns_x(int p_cols) {
    columns_x_ = std::max(1, p_cols);
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void GridContainer3D::set_rows_y(int p_rows) {
    rows_y_ = std::max(1, p_rows);
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void GridContainer3D::refit_children_3d() {
    const int n = get_child_count();
    if (n <= 0) return;

    const int cols = std::max(1, columns_x_);
    const int rows = std::max(1, rows_y_);
    const int layer_stride = cols * rows;
    const godot::Vector3 step = get_cell_spacing();

    int placed = 0;
    for (int i = 0; i < n; ++i) {
        auto *child = godot::Object::cast_to<godot::Node3D>(get_child(i));
        if (!child) continue;
        const int gx = placed % cols;
        const int gy = (placed / cols) % rows;
        const int gz = placed / layer_stride;
        child->set_position(godot::Vector3(
            static_cast<float>(gx) * step.x,
            static_cast<float>(gy) * step.y,
            static_cast<float>(gz) * step.z
        ));
        ++placed;
    }
}

// ============================================================================
// RadialContainer3D
// ============================================================================

void RadialContainer3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_radius", "radius"), &RadialContainer3D::set_radius);
    ClassDB::bind_method(D_METHOD("get_radius"), &RadialContainer3D::get_radius);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0.0,500.0,0.05"), "set_radius", "get_radius");

    ClassDB::bind_method(D_METHOD("set_arc_angle_degrees", "degrees"), &RadialContainer3D::set_arc_angle_degrees);
    ClassDB::bind_method(D_METHOD("get_arc_angle_degrees"), &RadialContainer3D::get_arc_angle_degrees);
    ADD_PROPERTY(
        PropertyInfo(Variant::FLOAT, "arc_angle_degrees", PROPERTY_HINT_RANGE, "1.0,360.0,1.0"),
        "set_arc_angle_degrees",
        "get_arc_angle_degrees"
    );

    ClassDB::bind_method(D_METHOD("set_helix_height_step", "step"), &RadialContainer3D::set_helix_height_step);
    ClassDB::bind_method(D_METHOD("get_helix_height_step"), &RadialContainer3D::get_helix_height_step);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "helix_height_step"), "set_helix_height_step", "get_helix_height_step");
}

void RadialContainer3D::set_radius(real_t p_radius) {
    radius_ = p_radius;
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void RadialContainer3D::set_arc_angle_degrees(real_t p_degrees) {
    arc_angle_degrees_ = std::clamp<real_t>(p_degrees, static_cast<real_t>(1.0), static_cast<real_t>(360.0));
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void RadialContainer3D::set_helix_height_step(real_t p_step) {
    helix_height_step_ = p_step;
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void RadialContainer3D::refit_children_3d() {
    const int n = get_child_count();
    if (n <= 0) {
        return;
    }
    const float arc_rad = static_cast<float>(arc_angle_degrees_) * (godot::Math::TAU / 360.0f);
    for (int i = 0; i < n; ++i) {
        if (auto *child = godot::Object::cast_to<godot::Node3D>(get_child(i))) {
            const float angle = (arc_rad * static_cast<float>(i)) / static_cast<float>(n);
            child->set_position(godot::Vector3(
                std::cos(angle) * static_cast<float>(radius_),
                static_cast<float>(helix_height_step_) * static_cast<float>(i),
                std::sin(angle) * static_cast<float>(radius_)
            ));
        }
    }
}

// ============================================================================
// SplineContainer3D
// ============================================================================

void SplineContainer3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_curve_points_3d", "points"), &SplineContainer3D::set_curve_points_3d);
    ClassDB::bind_method(D_METHOD("get_curve_points_3d"), &SplineContainer3D::get_curve_points_3d);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "curve_points_3d"), "set_curve_points_3d", "get_curve_points_3d");

    ClassDB::bind_method(D_METHOD("set_taper_scale_start", "scale"), &SplineContainer3D::set_taper_scale_start);
    ClassDB::bind_method(D_METHOD("get_taper_scale_start"), &SplineContainer3D::get_taper_scale_start);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "taper_scale_start"), "set_taper_scale_start", "get_taper_scale_start");

    ClassDB::bind_method(D_METHOD("set_taper_scale_end", "scale"), &SplineContainer3D::set_taper_scale_end);
    ClassDB::bind_method(D_METHOD("get_taper_scale_end"), &SplineContainer3D::get_taper_scale_end);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "taper_scale_end"), "set_taper_scale_end", "get_taper_scale_end");
}

void SplineContainer3D::set_curve_points_3d(const godot::PackedVector3Array &p_points) {
    curve_points_3d_ = p_points;
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void SplineContainer3D::set_taper_scale_start(float p_start) {
    taper_scale_start_ = std::max(0.01f, p_start);
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void SplineContainer3D::set_taper_scale_end(float p_end) {
    taper_scale_end_ = std::max(0.01f, p_end);
    if (get_auto_refit_children()) {
        refit_children_3d();
    }
}

void SplineContainer3D::refit_children_3d() {
    const int n = get_child_count();
    if (n <= 0 || curve_points_3d_.is_empty()) {
        return;
    }
    const int pts_cnt = static_cast<int>(curve_points_3d_.size());
    for (int i = 0; i < n; ++i) {
        auto *child = godot::Object::cast_to<godot::Node3D>(get_child(i));
        if (!child) continue;
        const float t = (n > 1) ? (static_cast<float>(i) / static_cast<float>(n - 1)) : 0.0f;
        const float f_idx = t * static_cast<float>(pts_cnt - 1);
        const int idx0    = std::clamp(static_cast<int>(std::floor(f_idx)), 0, pts_cnt - 1);
        const int idx1    = std::clamp(idx0 + 1, 0, pts_cnt - 1);
        const float frac  = f_idx - static_cast<float>(idx0);

        const godot::Vector3 pos = curve_points_3d_[idx0] * (1.0f - frac) + curve_points_3d_[idx1] * frac;
        const float taper        = taper_scale_start_ * (1.0f - t) + taper_scale_end_ * t;
        child->set_position(pos);
        child->set_scale(godot::Vector3(taper, taper, taper));
    }
}

// ============================================================================
// SemanticStructure3D
// ============================================================================

void SemanticStructure3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_semantic_role_id", "role_id"), &SemanticStructure3D::set_semantic_role_id);
    ClassDB::bind_method(D_METHOD("get_semantic_role_id"), &SemanticStructure3D::get_semantic_role_id);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "semantic_role_id"), "set_semantic_role_id", "get_semantic_role_id");

    ClassDB::bind_method(D_METHOD("set_rest_obb_extents", "extents"), &SemanticStructure3D::set_rest_obb_extents);
    ClassDB::bind_method(D_METHOD("get_rest_obb_extents"), &SemanticStructure3D::get_rest_obb_extents);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "rest_obb_extents"), "set_rest_obb_extents", "get_rest_obb_extents");

    ClassDB::bind_method(D_METHOD("set_margin_extents", "margin"), &SemanticStructure3D::set_margin_extents);
    ClassDB::bind_method(D_METHOD("get_margin_extents"), &SemanticStructure3D::get_margin_extents);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "margin_extents"), "set_margin_extents", "get_margin_extents");

    ClassDB::bind_method(D_METHOD("set_stretch_ratio", "ratio"), &SemanticStructure3D::set_stretch_ratio);
    ClassDB::bind_method(D_METHOD("get_stretch_ratio"), &SemanticStructure3D::get_stretch_ratio);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stretch_ratio"), "set_stretch_ratio", "get_stretch_ratio");

    ClassDB::bind_method(D_METHOD("set_scale_mode", "mode"), &SemanticStructure3D::set_scale_mode);
    ClassDB::bind_method(D_METHOD("get_scale_mode"), &SemanticStructure3D::get_scale_mode);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "scale_mode",
            PROPERTY_HINT_ENUM,
            "Uniform Fit,Stretch Middle,Repeat Tiles,Spline Loft Deform,Proportional Axis"
        ),
        "set_scale_mode",
        "get_scale_mode"
    );

    ClassDB::bind_method(D_METHOD("set_edge_snap_policy", "policy"), &SemanticStructure3D::set_edge_snap_policy);
    ClassDB::bind_method(D_METHOD("get_edge_snap_policy"), &SemanticStructure3D::get_edge_snap_policy);
    ADD_PROPERTY(
        PropertyInfo(
            Variant::INT,
            "edge_snap_policy",
            PROPERTY_HINT_ENUM,
            "None,Watertight,Miter Corner,Socket Anchor,Surface Normal Drape"
        ),
        "set_edge_snap_policy",
        "get_edge_snap_policy"
    );

    ClassDB::bind_method(D_METHOD("set_active_theme_3d", "theme"), &SemanticStructure3D::set_active_theme_3d);
    ClassDB::bind_method(D_METHOD("get_active_theme_3d"), &SemanticStructure3D::get_active_theme_3d);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "active_theme_3d", PROPERTY_HINT_RESOURCE_TYPE, "Theme3DResource"),
        "set_active_theme_3d",
        "get_active_theme_3d"
    );

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &SemanticStructure3D::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &SemanticStructure3D::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );

    ClassDB::bind_method(
        D_METHOD("apply_theme_and_bounds_to_children"),
        &SemanticStructure3D::apply_theme_and_bounds_to_children
    );
}

void SemanticStructure3D::set_semantic_role_id(int p_role_id) {
    semantic_role_id_ = static_cast<uint32_t>(p_role_id);
}

void SemanticStructure3D::set_rest_obb_extents(const godot::Vector3 &p_extents) {
    rest_obb_extents_ = p_extents;
}

void SemanticStructure3D::set_margin_extents(const godot::Vector3 &p_margin) {
    margin_extents_ = p_margin;
}

void SemanticStructure3D::set_stretch_ratio(float p_ratio) {
    stretch_ratio_ = std::max(0.01f, p_ratio);
}

void SemanticStructure3D::set_scale_mode(int p_mode) {
    scale_mode_ = static_cast<core::DesignScaleMode>(std::clamp(p_mode, 0, 4));
}

void SemanticStructure3D::set_edge_snap_policy(int p_policy) {
    edge_snap_policy_ = static_cast<core::EdgeSnapPolicy>(std::clamp(p_policy, 0, 4));
}

void SemanticStructure3D::set_active_theme_3d(const godot::Ref<Theme3DResource> &p_theme) {
    active_theme_3d_ = p_theme;
    apply_theme_and_bounds_to_children();
}

void SemanticStructure3D::set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) {
    semantic_tags_ = p_tags;
}

void SemanticStructure3D::apply_theme_and_bounds_to_children() {
    int parent_role = 0;
    if (auto *parent_sem = godot::Object::cast_to<SemanticStructure3D>(get_parent())) {
        parent_role = parent_sem->get_semantic_role_id();
    }

    godot::Ref<Style3DResource> resolved;
    if (active_theme_3d_.is_valid()) {
        resolved = active_theme_3d_->resolve_cascaded_style(
            static_cast<int>(semantic_role_id_),
            parent_role,
            semantic_tags_
        );
    }

    if (resolved.is_valid()) {
        scale_mode_       = static_cast<core::DesignScaleMode>(resolved->get_scale_mode());
        edge_snap_policy_ = static_cast<core::EdgeSnapPolicy>(resolved->get_edge_snap_policy());
        const godot::Vector3 s = resolved->get_target_extents_scale();
        rest_obb_extents_ = godot::Vector3(
            rest_obb_extents_.x * s.x,
            rest_obb_extents_.y * s.y,
            rest_obb_extents_.z * s.z
        );
    }

    const int n = get_child_count();
    for (int i = 0; i < n; ++i) {
        if (auto *mesh_inst = godot::Object::cast_to<godot::MeshInstance3D>(get_child(i))) {
            if (resolved.is_valid() && resolved->get_mesh_asset().is_valid()) {
                mesh_inst->set_mesh(resolved->get_mesh_asset());
            }
            if (resolved.is_valid() && resolved->get_material_override().is_valid()) {
                mesh_inst->set_material_override(resolved->get_material_override());
            }
            mesh_inst->set_scale(rest_obb_extents_ * 2.0f);
        } else if (auto *child_sem = godot::Object::cast_to<SemanticStructure3D>(get_child(i))) {
            if (child_sem->get_active_theme_3d().is_null() && active_theme_3d_.is_valid()) {
                child_sem->set_active_theme_3d(active_theme_3d_);
            } else {
                child_sem->apply_theme_and_bounds_to_children();
            }
        }
    }
}

core::StructureNodePod SemanticStructure3D::to_structure_node_pod(
    uint32_t p_node_id,
    uint32_t p_parent_id
) const {
    core::StructureNodePod pod{};
    pod.node_id          = p_node_id;
    pod.parent_id        = p_parent_id;
    pod.semantic_tag_id  = semantic_role_id_;
    if (semantic_tags_.is_valid()) {
        const auto desc               = semantic_tags_->compile_combined_descriptor();
        pod.nuclear_seme_mask_lo32    = static_cast<uint32_t>(desc.nuclear_seme_words[0] & 0xFFFFFFFFULL);
        pod.peripheral_seme_mask_lo32 = static_cast<uint32_t>(desc.peripheral_seme_words[0] & 0xFFFFFFFFULL);
    }
    pod.scale_mode       = scale_mode_;
    pod.edge_snap_policy = edge_snap_policy_;
    pod.size_flags_x     = size_flags_x_;
    pod.size_flags_y     = size_flags_y_;
    pod.size_flags_z     = size_flags_z_;
    pod.stretch_ratio    = stretch_ratio_;
    const godot::Vector3 pos = get_position();
    pod.rest_obb_center  = glm::vec3(pos.x, pos.y, pos.z);
    pod.rest_obb_extents = glm::vec3(rest_obb_extents_.x, rest_obb_extents_.y, rest_obb_extents_.z);
    pod.margin_uniform   = margin_extents_.x;
    return pod;
}

} // namespace ideam::godot_ext
