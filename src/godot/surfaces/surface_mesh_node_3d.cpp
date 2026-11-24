#include "surface_mesh_node_3d.h"

#include "ideam_surfaces_plugin.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>

#include <algorithm>

namespace ideam::godot_ext {

// ============================================================================
// SurfaceMeshNode3D Implementation
// ============================================================================

void SurfaceMeshNode3D::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Surface Source", "");

    ClassDB::bind_method(D_METHOD("set_surface_patch", "patch"), &SurfaceMeshNode3D::set_surface_patch);
    ClassDB::bind_method(D_METHOD("get_surface_patch"), &SurfaceMeshNode3D::get_surface_patch);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "surface_patch", PROPERTY_HINT_RESOURCE_TYPE, "SurfacePatchResource"),
        "set_surface_patch",
        "get_surface_patch"
    );

    ClassDB::bind_method(D_METHOD("set_surface_network", "network"), &SurfaceMeshNode3D::set_surface_network);
    ClassDB::bind_method(D_METHOD("get_surface_network"), &SurfaceMeshNode3D::get_surface_network);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "surface_network", PROPERTY_HINT_RESOURCE_TYPE, "SurfaceNetworkResource"),
        "set_surface_network",
        "get_surface_network"
    );

    ADD_GROUP("Tessellation & Display", "");

    ClassDB::bind_method(D_METHOD("set_lod_level", "lod_level"), &SurfaceMeshNode3D::set_lod_level);
    ClassDB::bind_method(D_METHOD("get_lod_level"), &SurfaceMeshNode3D::get_lod_level);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "lod_level", PROPERTY_HINT_RANGE, "0,3,1"), "set_lod_level", "get_lod_level");

    ClassDB::bind_method(D_METHOD("set_show_control_cage", "show"), &SurfaceMeshNode3D::set_show_control_cage);
    ClassDB::bind_method(D_METHOD("get_show_control_cage"), &SurfaceMeshNode3D::get_show_control_cage);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_control_cage"), "set_show_control_cage", "get_show_control_cage");

    ClassDB::bind_method(D_METHOD("set_show_normal_needles", "show"), &SurfaceMeshNode3D::set_show_normal_needles);
    ClassDB::bind_method(D_METHOD("get_show_normal_needles"), &SurfaceMeshNode3D::get_show_normal_needles);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_normal_needles"), "set_show_normal_needles", "get_show_normal_needles");

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &SurfaceMeshNode3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &SurfaceMeshNode3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("get_built_mesh"), &SurfaceMeshNode3D::get_built_mesh);
    ClassDB::bind_method(D_METHOD("bake_concave_collision_shape"), &SurfaceMeshNode3D::bake_concave_collision_shape);
    ClassDB::bind_method(D_METHOD("sample_world_frame", "patch_id", "u", "v"), &SurfaceMeshNode3D::sample_world_frame);
    ClassDB::bind_method(D_METHOD("rebuild_mesh"), &SurfaceMeshNode3D::rebuild_mesh);
    ClassDB::bind_method(D_METHOD("_on_surface_changed"), &SurfaceMeshNode3D::_on_surface_changed);
}

SurfaceMeshNode3D::SurfaceMeshNode3D() {
    set_notify_transform(true);
    mesh_instance_rid_ = godot::RenderingServer::get_singleton()->instance_create();
}

SurfaceMeshNode3D::~SurfaceMeshNode3D() {
    if (surface_patch_.is_valid()) {
        const godot::Callable cb(this, "_on_surface_changed");
        if (surface_patch_->is_connected("changed", cb)) surface_patch_->disconnect("changed", cb);
    }
    if (surface_network_.is_valid()) {
        const godot::Callable cb(this, "_on_surface_changed");
        if (surface_network_->is_connected("changed", cb)) surface_network_->disconnect("changed", cb);
    }
    if (mesh_instance_rid_.is_valid()) {
        godot::RenderingServer::get_singleton()->free_rid(mesh_instance_rid_);
        mesh_instance_rid_ = godot::RID();
    }
}

void SurfaceMeshNode3D::_notification(int p_what) {
    auto *rs = godot::RenderingServer::get_singleton();
    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            if (mesh_instance_rid_.is_valid() && get_world_3d().is_valid()) {
                rs->instance_set_scenario(mesh_instance_rid_, get_world_3d()->get_scenario());
                rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
                rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            }
            rebuild_mesh();
            break;
        }
        case NOTIFICATION_EXIT_WORLD: {
            if (mesh_instance_rid_.is_valid()) {
                rs->instance_set_scenario(mesh_instance_rid_, godot::RID());
            }
            break;
        }
        case NOTIFICATION_TRANSFORM_CHANGED: {
            if (mesh_instance_rid_.is_valid()) {
                rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
            }
            break;
        }
        case NOTIFICATION_VISIBILITY_CHANGED: {
            if (mesh_instance_rid_.is_valid()) {
                rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            }
            break;
        }
        default:
            break;
    }
}

void SurfaceMeshNode3D::_on_surface_changed() {
    rebuild_mesh();
    update_gizmos();
}

void SurfaceMeshNode3D::set_surface_patch(const godot::Ref<SurfacePatchResource> &p_patch) {
    const godot::Callable cb(this, "_on_surface_changed");
    if (surface_patch_.is_valid() && surface_patch_->is_connected("changed", cb)) {
        surface_patch_->disconnect("changed", cb);
    }
    surface_patch_ = p_patch;
    if (surface_patch_.is_valid() && !surface_patch_->is_connected("changed", cb)) {
        surface_patch_->connect("changed", cb);
    }
    rebuild_mesh();
    update_gizmos();
}

void SurfaceMeshNode3D::set_surface_network(const godot::Ref<SurfaceNetworkResource> &p_network) {
    const godot::Callable cb(this, "_on_surface_changed");
    if (surface_network_.is_valid() && surface_network_->is_connected("changed", cb)) {
        surface_network_->disconnect("changed", cb);
    }
    surface_network_ = p_network;
    if (surface_network_.is_valid() && !surface_network_->is_connected("changed", cb)) {
        surface_network_->connect("changed", cb);
    }
    rebuild_mesh();
    update_gizmos();
}

void SurfaceMeshNode3D::set_lod_level(int p_lod) {
    lod_level_ = std::clamp(p_lod, 0, 3);
    rebuild_mesh();
    update_gizmos();
}

void SurfaceMeshNode3D::set_show_control_cage(bool p_show) {
    show_control_cage_ = p_show;
    update_gizmos();
}

void SurfaceMeshNode3D::set_show_normal_needles(bool p_show) {
    show_normal_needles_ = p_show;
    update_gizmos();
}

void SurfaceMeshNode3D::set_material_override(const godot::Ref<godot::Material> &p_material) {
    material_override_ = p_material;
    if (mesh_instance_rid_.is_valid()) {
        godot::RenderingServer::get_singleton()->instance_geometry_set_material_override(
            mesh_instance_rid_,
            material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
        );
    }
}

void SurfaceMeshNode3D::rebuild_mesh() {
    if (surface_network_.is_valid() && surface_network_->get_patch_count() > 0) {
        built_mesh_ = surface_network_->build_array_mesh(lod_level_);
    } else if (surface_patch_.is_valid() && surface_patch_->is_patch_valid()) {
        built_mesh_ = surface_patch_->build_array_mesh(lod_level_);
    } else {
        built_mesh_.unref();
    }

    if (mesh_instance_rid_.is_valid()) {
        auto *rs = godot::RenderingServer::get_singleton();
        rs->instance_set_base(mesh_instance_rid_, built_mesh_.is_valid() ? built_mesh_->get_rid() : godot::RID());
        rs->instance_geometry_set_material_override(
            mesh_instance_rid_,
            material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
        );
    }
}

godot::Ref<godot::ConcavePolygonShape3D> SurfaceMeshNode3D::bake_concave_collision_shape() const {
    godot::Ref<godot::ConcavePolygonShape3D> shape;
    shape.instantiate();
    if (!built_mesh_.is_valid() || built_mesh_->get_surface_count() == 0) {
        return shape;
    }
    const godot::PackedVector3Array faces = built_mesh_->get_faces();
    if (!faces.is_empty()) {
        shape->set_faces(faces);
    }
    return shape;
}

godot::Transform3D SurfaceMeshNode3D::sample_world_frame(int p_patch_id, real_t p_u, real_t p_v) const {
    godot::Transform3D local_frame;
    if (surface_network_.is_valid() && surface_network_->get_patch_count() > 0) {
        local_frame = surface_network_->sample_frame(p_patch_id, p_u, p_v);
    } else if (surface_patch_.is_valid()) {
        local_frame = surface_patch_->sample_frame(p_u, p_v);
    }
    return get_global_transform() * local_frame;
}

// ============================================================================
// SurfacePatchGizmo Implementation
// ============================================================================

godot::Ref<NurbsPatch3DResource> SurfacePatchGizmo::_resolve_editable_nurbs(const SurfaceMeshNode3D *p_node) {
    if (!p_node) return godot::Ref<NurbsPatch3DResource>();
    if (auto *nurbs = godot::Object::cast_to<NurbsPatch3DResource>(p_node->get_surface_patch().ptr())) {
        return godot::Ref<NurbsPatch3DResource>(nurbs);
    }
    return godot::Ref<NurbsPatch3DResource>();
}

bool SurfacePatchGizmo::can_handle(godot::Node3D *p_target) const {
    return godot::Object::cast_to<SurfaceMeshNode3D>(p_target) != nullptr;
}

godot::String SurfacePatchGizmo::get_gizmo_name() const {
    return "IdeamSurfacePatchGizmo";
}

int32_t SurfacePatchGizmo::get_priority() const {
    return 0;
}

void SurfacePatchGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    (void)p_view;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_target);
    if (!node) return;

    godot::Ref<SurfacePatchResource> patch = node->get_surface_patch();
    if (!patch.is_valid() && node->get_surface_network().is_valid() && node->get_surface_network()->get_patch_count() > 0) {
        patch = node->get_surface_network()->get_patch(0);
    }
    if (!patch.is_valid() || !patch->is_patch_valid()) return;

    const godot::StringName mode = get_active_tool_mode();

    // 1. U & V isoparm curves
    constexpr int kIsoLines = 5;
    constexpr int kSamples  = 16;
    godot::PackedVector3Array u_lines;
    godot::PackedVector3Array v_lines;

    for (int line = 0; line < kIsoLines; ++line) {
        const real_t fixed = static_cast<real_t>(line) / static_cast<real_t>(kIsoLines - 1);
        for (int s = 0; s < kSamples; ++s) {
            const real_t t0 = static_cast<real_t>(s) / static_cast<real_t>(kSamples);
            const real_t t1 = static_cast<real_t>(s + 1) / static_cast<real_t>(kSamples);
            u_lines.append(patch->sample_position(t0, fixed));
            u_lines.append(patch->sample_position(t1, fixed));
            v_lines.append(patch->sample_position(fixed, t0));
            v_lines.append(patch->sample_position(fixed, t1));
        }
    }
    r_out.add_lines(u_lines, GIZMO_LINE_AXIS_NORMAL);
    r_out.add_lines(v_lines, GIZMO_PLUMB_BALANCED);

    // 2. Normal needles
    if (node->get_show_normal_needles() || mode == godot::StringName("normals")) {
        godot::PackedVector3Array needles;
        for (int j = 0; j < kIsoLines; ++j) {
            const real_t v = static_cast<real_t>(j) / static_cast<real_t>(kIsoLines - 1);
            for (int i = 0; i < kIsoLines; ++i) {
                const real_t u = static_cast<real_t>(i) / static_cast<real_t>(kIsoLines - 1);
                const godot::Vector3 pt = patch->sample_position(u, v);
                const godot::Vector3 n  = patch->sample_normal(u, v);
                needles.append(pt);
                needles.append(pt + n * 0.25f);
            }
        }
        r_out.add_lines(needles, GIZMO_LINE_AXIS_TANGENT);
    }

    // 3. NURBS control cage & interactive 3D drag handles
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (nurbs.is_valid() && (node->get_show_control_cage() || mode == godot::StringName("cage"))) {
        const int nu = nurbs->get_count_u();
        const int nv = nurbs->get_count_v();
        godot::PackedVector3Array cage;

        for (int j = 0; j < nv; ++j) {
            for (int i = 0; i < nu; ++i) {
                const godot::Vector3 pt = nurbs->get_control_point(i, j);
                r_out.add_handle(
                    HANDLE_CP_BASE + j * nu + i,
                    pt,
                    GIZMO_HANDLE_SPLINE_ANCHOR,
                    HANDLE_SHAPE_SPHERE,
                    static_cast<real_t>(0.045),
                    GIZMO_STATE_NONE,
                    godot::String("Control Point (") + godot::String::num_int64(i) + godot::String(", ") + godot::String::num_int64(j) + godot::String(")")
                );
                if (i + 1 < nu) {
                    cage.append(pt);
                    cage.append(nurbs->get_control_point(i + 1, j));
                }
                if (j + 1 < nv) {
                    cage.append(pt);
                    cage.append(nurbs->get_control_point(i, j + 1));
                }
            }
        }
        if (!cage.is_empty()) r_out.add_lines(cage, GIZMO_HANDLE_SPLINE_TANGENT);
    }
}

godot::String SurfacePatchGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_target);
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (nurbs.is_valid() && nurbs->get_count_u() > 0) {
        const int idx = p_handle_id - HANDLE_CP_BASE;
        const int u = idx % nurbs->get_count_u();
        const int v = idx / nurbs->get_count_u();
        return godot::String("Control Point (") + godot::String::num_int64(u) + godot::String(", ") + godot::String::num_int64(v) + godot::String(")");
    }
    return "Surface Handle";
}

godot::Variant SurfacePatchGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_target);
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (!nurbs.is_valid() || nurbs->get_count_u() <= 0) return godot::Vector3();
    const int idx = p_handle_id - HANDLE_CP_BASE;
    return nurbs->get_control_point(idx % nurbs->get_count_u(), idx / nurbs->get_count_u());
}

void SurfacePatchGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_target);
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (!node || !nurbs.is_valid() || nurbs->get_count_u() <= 0) return;

    const int idx = p_handle_id - HANDLE_CP_BASE;
    PatchDragState st{};
    st.initial_snapshot  = nurbs->get_snapshot_state();
    st.initial_local_pos = nurbs->get_control_point(idx % nurbs->get_count_u(), idx / nurbs->get_count_u());
    st.active = true;
    active_drags_[node->get_instance_id()] = st;
}

void SurfacePatchGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_modifiers;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_target);
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (!node || !nurbs.is_valid() || nurbs->get_count_u() <= 0 || !p_ray.is_valid()) return;

    if (!active_drags_.has(node->get_instance_id())) {
        on_handle_drag_begin(p_target, p_handle_id, p_ray);
    }

    const int idx = p_handle_id - HANDLE_CP_BASE;
    const int u = idx % nurbs->get_count_u();
    const int v = idx / nurbs->get_count_u();

    const godot::Transform3D gt   = node->get_global_transform();
    const godot::Vector3 world_pt = gt.xform(nurbs->get_control_point(u, v));
    godot::Vector3 plane_n = -p_ray.normal.normalized();
    if (p_ray.camera) {
        plane_n = -p_ray.camera->get_global_transform().basis.get_column(2).normalized();
    }
    const godot::Plane drag_plane(plane_n, world_pt);
    godot::Vector3 hit;
    if (drag_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit)) {
        const godot::Vector3 local_hit = gt.affine_inverse().xform(hit);
        nurbs->set_control_point(u, v, local_hit, nurbs->get_control_weight(u, v));
        node->update_gizmos();
    }
}

void SurfacePatchGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    (void)p_handle_id;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_target);
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (!node || !nurbs.is_valid()) return;

    const uint64_t nid = node->get_instance_id();
    if (!active_drags_.has(nid)) return;
    const PatchDragState st = active_drags_[nid];
    active_drags_.erase(nid);
    if (!st.active) return;

    if (p_cancel) {
        nurbs->apply_snapshot_state(st.initial_snapshot);
        node->update_gizmos();
        return;
    }

    if (!nurbs->get_undo_redo().is_valid()) {
        godot::Ref<IdeamUndoRedo> ur = resolve_undo_redo();
        if (ur.is_null()) ur = IdeamSurfacesPlugin::create_undo_redo_wrapper();
        nurbs->set_undo_redo(ur);
    }
    nurbs->commit_apply_snapshot(st.initial_snapshot, "Move Surface Control Point");
    node->update_gizmos();
}

} // namespace ideam::godot_ext
