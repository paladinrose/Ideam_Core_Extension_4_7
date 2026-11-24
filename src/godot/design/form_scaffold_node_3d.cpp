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

    skel->set_name(scaffold_->get_scaffold_name().replace(" ", "_") + godot::String("_Skeleton3D"));
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
// FormScaffoldGizmo Implementation (Trans-Environment `IdeamGizmo`)
// ============================================================================

bool FormScaffoldGizmo::can_handle(godot::Node3D *p_target) const {
    return godot::Object::cast_to<FormScaffoldNode3D>(p_target) != nullptr;
}

godot::String FormScaffoldGizmo::get_gizmo_name() const {
    return "IdeamFormScaffoldGizmo";
}

int32_t FormScaffoldGizmo::get_priority() const {
    return 0;
}

void FormScaffoldGizmo::build_draw_list(
    godot::Node3D *p_target,
    IdeamGizmoDrawList &r_out,
    const GizmoViewState &p_view
) const {
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_target);
    if (!node || !node->get_scaffold().is_valid()) return;

    const auto &sc = node->get_scaffold()->get_core_scaffold();
    const auto &nodes = sc.nodes();
    const uint64_t inst_id = node->get_instance_id();

    // 1. Socket / Bone Hierarchy Links + Interactive Handles
    if (node->get_show_bone_hierarchy() && !nodes.empty()) {
        godot::PackedVector3Array bone_lines;

        for (const auto &sn : nodes) {
            const godot::Vector3 wp(sn.world_transform[3][0], sn.world_transform[3][1], sn.world_transform[3][2]);
            const int32_t hid = HANDLE_NODE_BASE + static_cast<int32_t>(sn.node_id);

            uint32_t hstate = GIZMO_STATE_NONE;
            if (p_view.active_handle_id == hid) {
                hstate |= GIZMO_STATE_DRAGGING;
                if (active_drags_.has(inst_id) && active_drags_[inst_id].limit_reached) {
                    hstate |= GIZMO_STATE_LIMIT_REACHED;
                }
            } else if (p_view.hovered_handle_id == hid) {
                hstate |= GIZMO_STATE_HOVER;
            }

            r_out.add_handle(
                hid,
                wp,
                GIZMO_HANDLE_SCAFFOLD,
                HANDLE_SHAPE_SPHERE,
                static_cast<real_t>(0.038),
                hstate,
                godot::String::utf8(sn.name.c_str()),
                false
            );

            if (sn.parent_id != 0xFFFFFFFFu && sn.parent_id < nodes.size()) {
                const auto &pn = nodes[sn.parent_id];
                const godot::Vector3 pp(pn.world_transform[3][0], pn.world_transform[3][1], pn.world_transform[3][2]);
                bone_lines.append(pp);
                bone_lines.append(wp);
            }
        }
        if (!bone_lines.is_empty()) {
            r_out.add_lines(bone_lines, GIZMO_LINE_BONE_LINK);
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

        const float unit = std::max(0.05f, rg.get_base_unit_meters());
        for (float y = 0.0f; y <= max_h + 1e-3f; y += unit) {
            ruler_lines.append(godot::Vector3(staff_x - 0.08f, y, 0.0f));
            ruler_lines.append(godot::Vector3(staff_x + 0.08f, y, 0.0f));
        }
        for (const auto &rn : rg.nodes()) {
            if (!rn.is_landmark) continue;
            ruler_lines.append(godot::Vector3(staff_x, rn.computed_value, 0.0f));
            ruler_lines.append(godot::Vector3(staff_x + 0.22f, rn.computed_value, 0.0f));
        }
        r_out.add_lines(ruler_lines, GIZMO_LINE_RULER);
    }

    // 3. Loomis Center-of-Gravity Plumb Line & Support Ring
    if (node->get_show_balance_plumb_line()) {
        const auto bal = sc.compute_balance_report();
        const GizmoSemanticRole plumb_role = bal.is_balanced ? GIZMO_PLUMB_BALANCED : GIZMO_PLUMB_UNBALANCED;
        const godot::Vector3 cog(bal.center_of_gravity.x, bal.center_of_gravity.y, bal.center_of_gravity.z);
        const godot::Vector3 gnd(bal.plumb_ground_point.x, 0.0f, bal.plumb_ground_point.z);
        r_out.add_segment(cog, gnd, plumb_role);
        r_out.add_circle(
            godot::Vector3(bal.support_center.x, 0.005f, bal.support_center.z),
            godot::Vector3(0.0f, 1.0f, 0.0f),
            std::max(0.08f, bal.support_radius),
            20,
            plumb_role
        );
    }
}

godot::String FormScaffoldGizmo::get_handle_name(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_target);
    if (node && node->get_scaffold().is_valid()) {
        return node->get_scaffold()->get_node_name(p_handle_id - HANDLE_NODE_BASE);
    }
    return "Scaffold Node";
}

godot::Variant FormScaffoldGizmo::get_handle_value(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_secondary
) const {
    (void)p_secondary;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_target);
    if (!node || !node->get_scaffold().is_valid()) return godot::Vector3();

    const int nid = p_handle_id - HANDLE_NODE_BASE;
    const godot::Vector3 off = node->get_scaffold()->get_node_local_offset(nid);
    ScaffoldDragState st{nid, off, true, false};
    active_drags_[node->get_instance_id()] = st;
    return off;
}

void FormScaffoldGizmo::on_handle_drag_begin(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray
) {
    (void)p_ray;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_target);
    if (!node || !node->get_scaffold().is_valid()) return;

    const int nid = p_handle_id - HANDLE_NODE_BASE;
    const godot::Vector3 off = node->get_scaffold()->get_node_local_offset(nid);
    ScaffoldDragState st{nid, off, true, false};
    active_drags_[node->get_instance_id()] = st;
}

void FormScaffoldGizmo::on_handle_drag_update(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    const GizmoRay &p_ray,
    godot::BitField<godot::KeyModifierMask> p_modifiers
) {
    (void)p_modifiers;
    if (!p_ray.is_valid() || !p_ray.camera) return;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_target);
    if (!node || !node->get_scaffold().is_valid()) return;

    const int nid = p_handle_id - HANDLE_NODE_BASE;
    const godot::Transform3D gt = node->get_global_transform();
    const godot::Vector3 cur_world = gt.xform(node->get_scaffold()->get_node_world_position(nid));
    const godot::Vector3 cam_fwd = -p_ray.camera->get_global_transform().basis.get_column(2);
    const godot::Plane drag_plane(cam_fwd, cur_world);

    godot::Vector3 hit;
    if (drag_plane.intersects_ray(p_ray.origin, p_ray.normal, &hit)) {
        const godot::Vector3 delta_world = hit - cur_world;
        const godot::Vector3 delta_local = gt.basis.inverse().xform(delta_world);
        godot::Vector3 proposed = node->get_scaffold()->get_node_local_offset(nid) + delta_local;
        bool clamped = false;
        if (offset_clamp_cb_) {
            proposed = offset_clamp_cb_(nid, proposed, clamped);
        }
        const uint64_t inst_id = node->get_instance_id();
        if (active_drags_.has(inst_id)) {
            active_drags_[inst_id].limit_reached = clamped;
        }
        node->get_scaffold()->set_node_local_offset(nid, proposed);
    }
}

void FormScaffoldGizmo::on_handle_drag_commit(
    godot::Node3D *p_target,
    int32_t p_handle_id,
    bool p_cancel
) {
    (void)p_handle_id;
    auto *node = godot::Object::cast_to<FormScaffoldNode3D>(p_target);
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
    godot::Ref<IdeamUndoRedo> ur = resolve_undo_redo();
    if (ur.is_valid()) {
        scaffold->set_undo_redo(ur);
    } else if (!scaffold->get_undo_redo().is_valid()) {
        scaffold->set_undo_redo(IdeamDesignPlugin::create_undo_redo_wrapper());
    }
    scaffold->commit_set_node_local_offset(st.node_id, final_off);
}

} // namespace ideam::godot_ext
