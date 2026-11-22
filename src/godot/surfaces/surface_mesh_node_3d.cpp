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

SurfacePatchGizmo::SurfacePatchGizmo() {
    _init_materials();
}

void SurfacePatchGizmo::_init_materials() {
    create_material("isoparm_u", godot::Color(0.32f, 0.82f, 0.98f, 0.85f), false, true, false);
    create_material("isoparm_v", godot::Color(0.38f, 0.92f, 0.58f, 0.85f), false, true, false);
    create_material("control_cage", godot::Color(0.98f, 0.72f, 0.25f, 0.78f), false, true, false);
    create_material("normal_needle", godot::Color(0.96f, 0.36f, 0.42f, 0.85f), false, true, false);
    create_handle_material("handles");
}

godot::Ref<NurbsPatch3DResource> SurfacePatchGizmo::_resolve_editable_nurbs(const SurfaceMeshNode3D *p_node) {
    if (!p_node) return godot::Ref<NurbsPatch3DResource>();
    if (auto *nurbs = godot::Object::cast_to<NurbsPatch3DResource>(p_node->get_surface_patch().ptr())) {
        return godot::Ref<NurbsPatch3DResource>(nurbs);
    }
    return godot::Ref<NurbsPatch3DResource>();
}

bool SurfacePatchGizmo::_has_gizmo(godot::Node3D *p_for_node_3d) const {
    return godot::Object::cast_to<SurfaceMeshNode3D>(p_for_node_3d) != nullptr;
}

godot::String SurfacePatchGizmo::_get_gizmo_name() const {
    return "IdeamSurfacePatchGizmo";
}

int32_t SurfacePatchGizmo::_get_priority() const {
    return 0;
}

void SurfacePatchGizmo::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) {
    p_gizmo->clear();
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_gizmo->get_node_3d());
    if (!node) return;

    godot::Ref<SurfacePatchResource> patch = node->get_surface_patch();
    if (!patch.is_valid() && node->get_surface_network().is_valid() && node->get_surface_network()->get_patch_count() > 0) {
        patch = node->get_surface_network()->get_patch(0);
    }
    if (!patch.is_valid() || !patch->is_patch_valid()) return;

    // 1. Draw U & V isoparm curves
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
    p_gizmo->add_lines(u_lines, get_material("isoparm_u", p_gizmo));
    p_gizmo->add_lines(v_lines, get_material("isoparm_v", p_gizmo));

    // 2. Optional normal needles
    if (node->get_show_normal_needles()) {
        godot::PackedVector3Array needles;
        for (int j = 0; j < kIsoLines; ++j) {
            const real_t v = static_cast<real_t>(j) / static_cast<real_t>(kIsoLines - 1);
            for (int i = 0; i < kIsoLines; ++i) {
                const real_t u = static_cast<real_t>(i) / static_cast<real_t>(kIsoLines - 1);
                const godot::Vector3 p = patch->sample_position(u, v);
                const godot::Vector3 n = patch->sample_normal(u, v);
                needles.append(p);
                needles.append(p + n * 0.25f);
            }
        }
        p_gizmo->add_lines(needles, get_material("normal_needle", p_gizmo));
    }

    // 3. NURBS control cage & interactive 3D drag handles
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (nurbs.is_valid() && node->get_show_control_cage()) {
        const int nu = nurbs->get_count_u();
        const int nv = nurbs->get_count_v();
        godot::PackedVector3Array cage;
        godot::PackedVector3Array handles;
        godot::PackedInt32Array handle_ids;

        for (int j = 0; j < nv; ++j) {
            for (int i = 0; i < nu; ++i) {
                const godot::Vector3 p = nurbs->get_control_point(i, j);
                handles.append(p);
                handle_ids.append(HANDLE_CP_BASE + j * nu + i);

                if (i + 1 < nu) {
                    cage.append(p);
                    cage.append(nurbs->get_control_point(i + 1, j));
                }
                if (j + 1 < nv) {
                    cage.append(p);
                    cage.append(nurbs->get_control_point(i, j + 1));
                }
            }
        }
        if (!cage.is_empty()) {
            p_gizmo->add_lines(cage, get_material("control_cage", p_gizmo));
        }
        if (!handles.is_empty()) {
            p_gizmo->add_handles(handles, get_material("handles", p_gizmo), handle_ids);
        }
    }
}

godot::String SurfacePatchGizmo::_get_handle_name(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_gizmo->get_node_3d());
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (nurbs.is_valid() && nurbs->get_count_u() > 0) {
        const int idx = p_handle_id - HANDLE_CP_BASE;
        const int u = idx % nurbs->get_count_u();
        const int v = idx / nurbs->get_count_u();
        return "Control Point (" + godot::String::num_int64(u) + ", " + godot::String::num_int64(v) + ")";
    }
    return "Surface Handle";
}

godot::Variant SurfacePatchGizmo::_get_handle_value(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_gizmo->get_node_3d());
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (!nurbs.is_valid() || nurbs->get_count_u() <= 0) return godot::Vector3();

    const int idx = p_handle_id - HANDLE_CP_BASE;
    const int u = idx % nurbs->get_count_u();
    const int v = idx / nurbs->get_count_u();
    const godot::Vector3 pt = nurbs->get_control_point(u, v);

    PatchDragState st{};
    st.initial_snapshot = nurbs->get_snapshot_state();
    st.initial_local_pos = pt;
    st.active = true;
    active_drags_[node->get_instance_id()] = st;
    return pt;
}

void SurfacePatchGizmo::_set_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos
) {
    (void)p_secondary;
    if (!p_camera) return;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_gizmo->get_node_3d());
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (!nurbs.is_valid() || nurbs->get_count_u() <= 0) return;

    const int idx = p_handle_id - HANDLE_CP_BASE;
    const int u = idx % nurbs->get_count_u();
    const int v = idx / nurbs->get_count_u();

    const godot::Transform3D gt = node->get_global_transform();
    const godot::Vector3 world_pt = gt.xform(nurbs->get_control_point(u, v));
    const godot::Vector3 cam_fwd = -p_camera->get_global_transform().basis.get_column(2);
    const godot::Plane drag_plane(cam_fwd, world_pt);

    const godot::Vector3 ray_origin = p_camera->project_ray_origin(p_screen_pos);
    const godot::Vector3 ray_dir    = p_camera->project_ray_normal(p_screen_pos);
    const godot::Variant hit = drag_plane.intersects_ray(ray_origin, ray_dir);
    if (hit.get_type() == godot::Variant::VECTOR3) {
        const godot::Vector3 local_hit = gt.affine_inverse().xform(godot::Vector3(hit));
        nurbs->set_control_point(u, v, local_hit, nurbs->get_control_weight(u, v));
    }
}

void SurfacePatchGizmo::_commit_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    const godot::Variant &p_restore,
    bool p_cancel
) {
    (void)p_handle_id;
    (void)p_secondary;
    (void)p_restore;
    auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(p_gizmo->get_node_3d());
    godot::Ref<NurbsPatch3DResource> nurbs = _resolve_editable_nurbs(node);
    if (!node || !nurbs.is_valid()) return;

    const uint64_t nid = node->get_instance_id();
    if (!active_drags_.has(nid)) return;
    const PatchDragState st = active_drags_[nid];
    active_drags_.erase(nid);
    if (!st.active) return;

    if (p_cancel) {
        nurbs->apply_snapshot_state(st.initial_snapshot);
        return;
    }

    if (!nurbs->get_undo_redo().is_valid()) {
        nurbs->set_undo_redo(IdeamSurfacesPlugin::create_undo_redo_wrapper());
    }
    nurbs->commit_apply_snapshot(st.initial_snapshot, "Move Surface Control Point");
}

} // namespace ideam::godot_ext
