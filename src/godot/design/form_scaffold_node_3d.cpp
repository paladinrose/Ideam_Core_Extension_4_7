#include "form_scaffold_node_3d.h"

#include "ideam_design_plugin.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/plane.hpp>

#include <algorithm>
#include <cmath>

namespace ideam::godot_ext {

// ============================================================================
// FormScaffoldNode3D Implementation
// ============================================================================

void FormScaffoldNode3D::_bind_methods() {
    using namespace godot;

    ADD_GROUP("Loomis Form Scaffold", "");

    ClassDB::bind_method(D_METHOD("set_scaffold", "scaffold"), &FormScaffoldNode3D::set_scaffold);
    ClassDB::bind_method(D_METHOD("get_scaffold"), &FormScaffoldNode3D::get_scaffold);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "scaffold", PROPERTY_HINT_RESOURCE_TYPE, "FormScaffoldResource"),
        "set_scaffold",
        "get_scaffold"
    );

    ClassDB::bind_method(D_METHOD("set_preview_subdivs", "subdivs"), &FormScaffoldNode3D::set_preview_subdivs);
    ClassDB::bind_method(D_METHOD("get_preview_subdivs"), &FormScaffoldNode3D::get_preview_subdivs);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "preview_subdivs", PROPERTY_HINT_RANGE, "4,24,2"), "set_preview_subdivs", "get_preview_subdivs");

    ADD_GROUP("Loomis Construction Guides", "");

    ClassDB::bind_method(D_METHOD("set_show_proportion_ruler", "show"), &FormScaffoldNode3D::set_show_proportion_ruler);
    ClassDB::bind_method(D_METHOD("get_show_proportion_ruler"), &FormScaffoldNode3D::get_show_proportion_ruler);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_proportion_ruler"), "set_show_proportion_ruler", "get_show_proportion_ruler");

    ClassDB::bind_method(D_METHOD("set_show_balance_plumb_line", "show"), &FormScaffoldNode3D::set_show_balance_plumb_line);
    ClassDB::bind_method(D_METHOD("get_show_balance_plumb_line"), &FormScaffoldNode3D::get_show_balance_plumb_line);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_balance_plumb_line"), "set_show_balance_plumb_line", "get_show_balance_plumb_line");

    ClassDB::bind_method(D_METHOD("set_show_bone_hierarchy", "show"), &FormScaffoldNode3D::set_show_bone_hierarchy);
    ClassDB::bind_method(D_METHOD("get_show_bone_hierarchy"), &FormScaffoldNode3D::get_show_bone_hierarchy);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_bone_hierarchy"), "set_show_bone_hierarchy", "get_show_bone_hierarchy");

    ClassDB::bind_method(D_METHOD("set_material_override", "material"), &FormScaffoldNode3D::set_material_override);
    ClassDB::bind_method(D_METHOD("get_material_override"), &FormScaffoldNode3D::get_material_override);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "material_override", PROPERTY_HINT_RESOURCE_TYPE, "Material"),
        "set_material_override",
        "get_material_override"
    );

    ClassDB::bind_method(D_METHOD("get_preview_mesh"), &FormScaffoldNode3D::get_preview_mesh);
    ClassDB::bind_method(D_METHOD("sample_world_scaffold_frame", "node_id", "u", "v"), &FormScaffoldNode3D::sample_world_scaffold_frame);
    ClassDB::bind_method(D_METHOD("build_skeleton_3d"), &FormScaffoldNode3D::build_skeleton_3d);
    ClassDB::bind_method(D_METHOD("build_ragdoll_collider_descriptors"), &FormScaffoldNode3D::build_ragdoll_collider_descriptors);
    ClassDB::bind_method(D_METHOD("rebuild_preview"), &FormScaffoldNode3D::rebuild_preview);
    ClassDB::bind_method(D_METHOD("_on_scaffold_changed"), &FormScaffoldNode3D::_on_scaffold_changed);
}

FormScaffoldNode3D::FormScaffoldNode3D() {
    set_notify_transform(true);
    mesh_instance_rid_ = godot::RenderingServer::get_singleton()->instance_create();
}

FormScaffoldNode3D::~FormScaffoldNode3D() {
    if (scaffold_.is_valid()) {
        const godot::Callable cb(this, "_on_scaffold_changed");
        if (scaffold_->is_connected("changed", cb)) scaffold_->disconnect("changed", cb);
    }
    if (mesh_instance_rid_.is_valid()) {
        godot::RenderingServer::get_singleton()->free_rid(mesh_instance_rid_);
        mesh_instance_rid_ = godot::RID();
    }
}

void FormScaffoldNode3D::_notification(int p_what) {
    auto *rs = godot::RenderingServer::get_singleton();
    switch (p_what) {
        case NOTIFICATION_ENTER_WORLD: {
            if (mesh_instance_rid_.is_valid() && get_world_3d().is_valid()) {
                rs->instance_set_scenario(mesh_instance_rid_, get_world_3d()->get_scenario());
                rs->instance_set_transform(mesh_instance_rid_, get_global_transform());
                rs->instance_set_visible(mesh_instance_rid_, is_visible_in_tree());
            }
            rebuild_preview();
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

void FormScaffoldNode3D::_on_scaffold_changed() {
    rebuild_preview();
    update_gizmos();
}

void FormScaffoldNode3D::set_scaffold(const godot::Ref<FormScaffoldResource> &p_scaffold) {
    const godot::Callable cb(this, "_on_scaffold_changed");
    if (scaffold_.is_valid() && scaffold_->is_connected("changed", cb)) {
        scaffold_->disconnect("changed", cb);
    }
    scaffold_ = p_scaffold;
    if (scaffold_.is_valid() && !scaffold_->is_connected("changed", cb)) {
        scaffold_->connect("changed", cb);
    }
    rebuild_preview();
    update_gizmos();
}

void FormScaffoldNode3D::set_preview_subdivs(int p_subdivs) {
    preview_subdivs_ = std::clamp(p_subdivs, 4, 24);
    rebuild_preview();
}

void FormScaffoldNode3D::set_show_proportion_ruler(bool p_show) { show_proportion_ruler_ = p_show; update_gizmos(); }
void FormScaffoldNode3D::set_show_balance_plumb_line(bool p_show) { show_balance_plumb_line_ = p_show; update_gizmos(); }
void FormScaffoldNode3D::set_show_bone_hierarchy(bool p_show) { show_bone_hierarchy_ = p_show; update_gizmos(); }

void FormScaffoldNode3D::set_material_override(const godot::Ref<godot::Material> &p_mat) {
    material_override_ = p_mat;
    if (mesh_instance_rid_.is_valid()) {
        godot::RenderingServer::get_singleton()->instance_geometry_set_material_override(
            mesh_instance_rid_,
            material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
        );
    }
}

void FormScaffoldNode3D::rebuild_preview() {
    if (scaffold_.is_valid()) {
        preview_mesh_ = scaffold_->build_preview_array_mesh(preview_subdivs_);
    } else {
        preview_mesh_.unref();
    }

    if (mesh_instance_rid_.is_valid()) {
        auto *rs = godot::RenderingServer::get_singleton();
        rs->instance_set_base(mesh_instance_rid_, preview_mesh_.is_valid() ? preview_mesh_->get_rid() : godot::RID());
        rs->instance_geometry_set_material_override(
            mesh_instance_rid_,
            material_override_.is_valid() ? material_override_->get_rid() : godot::RID()
        );
    }
}

godot::Transform3D FormScaffoldNode3D::sample_world_scaffold_frame(int p_node_id, real_t p_u, real_t p_v) const {
    if (!scaffold_.is_valid()) return get_global_transform();
    return get_global_transform() * scaffold_->sample_scaffold_frame(p_node_id, p_u, p_v);
}

godot::Skeleton3D *FormScaffoldNode3D::build_skeleton_3d() const {
    auto *skel = memnew(godot::Skeleton3D);
    if (!scaffold_.is_valid()) return skel;

    skel->set_name(scaffold_->get_scaffold_name().replace(" ", "_") + "_Skeleton3D");
    const godot::TypedArray<godot::Dictionary> plan = scaffold_->build_skeleton_export_plan();

    for (int64_t i = 0; i < plan.size(); ++i) {
        const godot::Dictionary b = plan[i];
        const int32_t bone_idx = skel->add_bone(godot::String(b.get("bone_name", "Bone")));
        const int32_t parent_idx = int32_t(b.get("parent_bone_index", -1));
        if (parent_idx >= 0 && parent_idx < bone_idx) {
            skel->set_bone_parent(bone_idx, parent_idx);
        }
        const godot::Transform3D rest = b.get("rest_local_transform", godot::Transform3D());
        skel->set_bone_rest(bone_idx, rest);
        skel->set_bone_pose_position(bone_idx, rest.origin);
        skel->set_bone_pose_rotation(bone_idx, rest.basis.get_rotation_quaternion());
    }
    return skel;
}

godot::TypedArray<godot::Dictionary> FormScaffoldNode3D::build_ragdoll_collider_descriptors() const {
    if (!scaffold_.is_valid()) return godot::TypedArray<godot::Dictionary>();
    return scaffold_->build_skeleton_export_plan();
}

// ============================================================================
// FormScaffoldGizmo Implementation
// ============================================================================

FormScaffoldGizmo::FormScaffoldGizmo() {
    _init_materials();
}

void FormScaffoldGizmo::_init_materials() {
    create_material("ruler_staff", godot::Color(0.32f, 0.85f, 0.98f, 0.92f), false, true, false);
    create_material("bone_links", godot::Color(0.98f, 0.80f, 0.28f, 0.90f), false, true, false);
    create_material("plumb_balanced", godot::Color(0.34f, 0.94f, 0.48f, 0.92f), false, true, false);
    create_material("plumb_offbalance", godot::Color(0.98f, 0.42f, 0.25f, 0.95f), false, true, false);
    create_handle_material("handles");
}

bool FormScaffoldGizmo::_has_gizmo(godot::Node3D *p_for_node_3d) const {
    return godot::Object::cast_to<FormScaffoldNode3D>(p_for_node_3d) != nullptr;
}

godot::String FormScaffoldGizmo::_get_gizmo_name() const {
    return "IdeamFormScaffoldGizmo";
}

int32_t FormScaffoldGizmo::_get_priority() const {
    return 0;
}

void FormScaffoldGizmo::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) {
    p_gizmo->clear();
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_gizmo->get_node_3d());
    if (!node || !node->get_scaffold().is_valid()) return;

    const auto &sc = node->get_scaffold()->get_core_scaffold();
    const auto &nodes = sc.nodes();

    // 1. Socket / Bone Hierarchy Links + Interactive Handles
    if (node->get_show_bone_hierarchy() && !nodes.empty()) {
        godot::PackedVector3Array bone_lines;
        godot::PackedVector3Array handles;
        godot::PackedInt32Array handle_ids;

        for (const auto &sn : nodes) {
            const godot::Vector3 wp(sn.world_transform[3][0], sn.world_transform[3][1], sn.world_transform[3][2]);
            handles.append(wp);
            handle_ids.append(HANDLE_NODE_BASE + static_cast<int32_t>(sn.node_id));

            if (sn.parent_id != 0xFFFFFFFFu && sn.parent_id < nodes.size()) {
                const auto &pn = nodes[sn.parent_id];
                const godot::Vector3 pp(pn.world_transform[3][0], pn.world_transform[3][1], pn.world_transform[3][2]);
                bone_lines.append(pp);
                bone_lines.append(wp);
            }
        }
        if (!bone_lines.is_empty()) {
            p_gizmo->add_lines(bone_lines, get_material("bone_links", p_gizmo));
        }
        if (!handles.is_empty()) {
            p_gizmo->add_handles(handles, get_material("handles", p_gizmo), handle_ids);
        }
    }

    // 2. Loomis Canonical-Unit Proportion Ruler Staff
    if (node->get_show_proportion_ruler()) {
        const auto &rg = sc.get_ruler_graph();
        float max_h = rg.get_base_unit_meters();
        for (const auto &rn : rg.nodes()) {
            if (rn.is_landmark) max_h = std::max(max_h, rn.computed_value);
        }

        const float staff_x = -0.85f;
        godot::PackedVector3Array ruler_lines;
        ruler_lines.append(godot::Vector3(staff_x, 0.0f, 0.0f));
        ruler_lines.append(godot::Vector3(staff_x, max_h, 0.0f));

        // Unit subdivisions along staff
        const float unit = std::max(0.05f, rg.get_base_unit_meters());
        for (float y = 0.0f; y <= max_h + 1e-3f; y += unit) {
            ruler_lines.append(godot::Vector3(staff_x - 0.08f, y, 0.0f));
            ruler_lines.append(godot::Vector3(staff_x + 0.08f, y, 0.0f));
        }
        // Landmark ticks
        for (const auto &rn : rg.nodes()) {
            if (!rn.is_landmark) continue;
            ruler_lines.append(godot::Vector3(staff_x, rn.computed_value, 0.0f));
            ruler_lines.append(godot::Vector3(staff_x + 0.22f, rn.computed_value, 0.0f));
        }
        p_gizmo->add_lines(ruler_lines, get_material("ruler_staff", p_gizmo));
    }

    // 3. Loomis Center-of-Gravity Plumb Line & Support Ring
    if (node->get_show_balance_plumb_line()) {
        const auto bal = sc.compute_balance_report();
        godot::PackedVector3Array plumb_lines;
        const godot::Vector3 cog(bal.center_of_gravity.x, bal.center_of_gravity.y, bal.center_of_gravity.z);
        const godot::Vector3 gnd(bal.plumb_ground_point.x, 0.0f, bal.plumb_ground_point.z);
        plumb_lines.append(cog);
        plumb_lines.append(gnd);

        // Draw ground support circle
        const int segs = 16;
        const float r = std::max(0.08f, bal.support_radius);
        for (int i = 0; i < segs; ++i) {
            const float a0 = static_cast<float>(i) / static_cast<float>(segs) * 6.2831853f;
            const float a1 = static_cast<float>(i + 1) / static_cast<float>(segs) * 6.2831853f;
            plumb_lines.append(godot::Vector3(bal.support_center.x + std::cos(a0) * r, 0.005f, bal.support_center.z + std::sin(a0) * r));
            plumb_lines.append(godot::Vector3(bal.support_center.x + std::cos(a1) * r, 0.005f, bal.support_center.z + std::sin(a1) * r));
        }
        p_gizmo->add_lines(plumb_lines, get_material(bal.is_balanced ? "plumb_balanced" : "plumb_offbalance", p_gizmo));
    }
}

godot::String FormScaffoldGizmo::_get_handle_name(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_gizmo->get_node_3d());
    if (node && node->get_scaffold().is_valid()) {
        return node->get_scaffold()->get_node_name(p_handle_id - HANDLE_NODE_BASE);
    }
    return "Scaffold Node";
}

godot::Variant FormScaffoldGizmo::_get_handle_value(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_gizmo->get_node_3d());
    if (!node || !node->get_scaffold().is_valid()) return godot::Vector3();

    const int nid = p_handle_id - HANDLE_NODE_BASE;
    const godot::Vector3 off = node->get_scaffold()->get_node_local_offset(nid);
    ScaffoldDragState st{nid, off, true};
    active_drags_[node->get_instance_id()] = st;
    return off;
}

void FormScaffoldGizmo::_set_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos
) {
    (void)p_secondary;
    if (!p_camera) return;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_gizmo->get_node_3d());
    if (!node || !node->get_scaffold().is_valid()) return;

    const int nid = p_handle_id - HANDLE_NODE_BASE;
    const godot::Transform3D gt = node->get_global_transform();
    const godot::Vector3 cur_world = gt.xform(node->get_scaffold()->get_node_world_position(nid));
    const godot::Vector3 cam_fwd = -p_camera->get_global_transform().basis.get_column(2);
    const godot::Plane drag_plane(cam_fwd, cur_world);

    godot::Vector3 hit;
    if (drag_plane.intersects_ray(
            p_camera->project_ray_origin(p_screen_pos),
            p_camera->project_ray_normal(p_screen_pos),
            &hit)) {
        const godot::Vector3 delta_world = hit - cur_world;
        const godot::Vector3 delta_local = gt.basis.inverse().xform(delta_world);
        node->get_scaffold()->set_node_local_offset(nid, node->get_scaffold()->get_node_local_offset(nid) + delta_local);
    }
}

void FormScaffoldGizmo::_commit_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    const godot::Variant &p_restore,
    bool p_cancel
) {
    (void)p_handle_id;
    (void)p_secondary;
    (void)p_restore;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_gizmo->get_node_3d());
    if (!node || !node->get_scaffold().is_valid()) return;

    const uint64_t id = node->get_instance_id();
    if (!active_drags_.has(id)) return;
    const ScaffoldDragState st = active_drags_[id];
    active_drags_.erase(id);
    if (!st.active) return;

    auto scaffold = node->get_scaffold();
    if (p_cancel) {
        scaffold->set_node_local_offset(st.node_id, st.initial_local_offset);
        return;
    }

    const godot::Vector3 final_off = scaffold->get_node_local_offset(st.node_id);
    scaffold->set_node_local_offset(st.node_id, st.initial_local_offset);
    if (!scaffold->get_undo_redo().is_valid()) {
        scaffold->set_undo_redo(IdeamDesignPlugin::create_undo_redo_wrapper());
    }
    scaffold->commit_set_node_local_offset(st.node_id, final_off);
}

} // namespace ideam::godot_ext
