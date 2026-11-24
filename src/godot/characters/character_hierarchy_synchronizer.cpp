#include "character_hierarchy_synchronizer.h"

#include "loomis_character_node_3d.h"
#include "../design/container_3d.h"
#include "../design/socket_3d.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ideam::godot_ext {

namespace {

inline godot::Transform3D glm4_to_gd(const glm::mat4 &m) noexcept {
    godot::Basis b;
    b.set_column(0, godot::Vector3(m[0][0], m[0][1], m[0][2]));
    b.set_column(1, godot::Vector3(m[1][0], m[1][1], m[1][2]));
    b.set_column(2, godot::Vector3(m[2][0], m[2][1], m[2][2]));
    return godot::Transform3D(b, godot::Vector3(m[3][0], m[3][1], m[3][2]));
}

inline void prune_orphaned_managed_children(
    godot::Node *p_parent,
    const std::unordered_set<std::string> &p_active_ids
) {
    if (!p_parent) return;
    std::vector<godot::Node *> to_remove;
    for (int i = 0; i < p_parent->get_child_count(); ++i) {
        godot::Node *ch = p_parent->get_child(i);
        if (ch && ch->has_meta(CharacterHierarchySynchronizer::META_MANAGED_ID)) {
            const godot::String mid = ch->get_meta(CharacterHierarchySynchronizer::META_MANAGED_ID);
            const std::string key(mid.utf8().get_data());
            if (!p_active_ids.contains(key)) {
                to_remove.push_back(ch);
            }
        }
    }
    for (godot::Node *orphan : to_remove) {
        p_parent->remove_child(orphan);
        memdelete(orphan);
    }
}

} // namespace

void CharacterHierarchySynchronizer::_assign_scene_owner(godot::Node *p_owner_root, godot::Node *p_child) {
    if (!p_owner_root || !p_child) return;
    if (godot::Engine::get_singleton()->is_editor_hint() && p_owner_root->is_inside_tree()) {
        if (godot::SceneTree *tree = p_owner_root->get_tree()) {
            if (godot::Node *edited_root = tree->get_edited_scene_root()) {
                p_child->set_owner(edited_root);
            }
        }
    }
}

godot::Node *CharacterHierarchySynchronizer::_find_managed_child(
    godot::Node *p_parent,
    const godot::String &p_managed_id
) {
    if (!p_parent) return nullptr;
    const int n = p_parent->get_child_count();
    for (int i = 0; i < n; ++i) {
        godot::Node *ch = p_parent->get_child(i);
        if (ch && ch->has_meta(META_MANAGED_ID)) {
            if (godot::String(ch->get_meta(META_MANAGED_ID)) == p_managed_id) {
                return ch;
            }
        }
    }
    return nullptr;
}

void CharacterHierarchySynchronizer::synchronize(
    LoomisCharacterNode3D *p_owner_node,
    const godot::Ref<godot::ArrayMesh> &p_mesh,
    const godot::Ref<godot::Material> &p_material_override
) {
    if (!p_owner_node) return;
    auto inst = p_owner_node->get_character_resource();
    if (inst.is_null() || inst->get_body_plan().is_null()) {
        clear_managed_nodes(p_owner_node);
        return;
    }

    auto plan = inst->get_body_plan();
    const auto evaluated = inst->get_core_instance().evaluate_stack(plan->get_core_plan(), 3u);

    std::unordered_map<uint32_t, const core::EvaluatedScaffoldNode3D *> scaffold_map;
    for (const auto &sn : evaluated.evaluated_scaffold.nodes()) {
        scaffold_map[sn.node_id] = &sn;
    }

    // ------------------------------------------------------------------------
    // 1. Scaffold/ (ProportionContainer3D + per-joint Socket3D children)
    // ------------------------------------------------------------------------
    auto *scaffold_root = godot::Object::cast_to<ProportionContainer3D>(
        _find_managed_child(p_owner_node, "scaffold_root")
    );
    if (!scaffold_root) {
        scaffold_root = memnew(ProportionContainer3D);
        scaffold_root->set_name("Scaffold");
        scaffold_root->set_meta(META_MANAGED_ID, godot::String("scaffold_root"));
        scaffold_root->set_auto_refit_children(false);
        p_owner_node->add_child(scaffold_root);
    }
    _assign_scene_owner(p_owner_node, scaffold_root);

    if (plan->get_base_scaffold().is_valid() && plan->get_base_scaffold()->get_ruler_graph().is_valid()) {
        scaffold_root->set_ruler_graph(plan->get_base_scaffold()->get_ruler_graph());
    }

    std::unordered_set<std::string> active_socket_ids;
    for (const auto &sn : evaluated.evaluated_scaffold.nodes()) {
        const std::string id_str = "scaffold_socket_" + std::to_string(sn.node_id);
        active_socket_ids.insert(id_str);
        const godot::String managed_id = godot::String::utf8(id_str.c_str());

        auto *sock = godot::Object::cast_to<Socket3D>(_find_managed_child(scaffold_root, managed_id));
        if (!sock) {
            sock = memnew(Socket3D);
            const godot::String node_name = sn.name.empty()
                ? (godot::String("Joint_") + godot::String::num_int64(sn.node_id))
                : (godot::String::utf8(sn.name.c_str()) + godot::String("_Socket"));
            sock->set_name(node_name);
            sock->set_meta(META_MANAGED_ID, managed_id);
            scaffold_root->add_child(sock);
        }
        _assign_scene_owner(p_owner_node, sock);

        sock->set_transform(glm4_to_gd(sn.world_transform));
        sock->set_clearance_extents(godot::Vector3(
            sn.evaluated_half_extents.x,
            sn.evaluated_half_extents.y,
            sn.evaluated_half_extents.z
        ));
        sock->set_physics_binding_mode(static_cast<int>(sn.socket.physics_binding_mode));
        sock->set_joint_limit_lower(sn.socket.joint_limit_lower);
        sock->set_joint_limit_upper(sn.socket.joint_limit_upper);
    }
    prune_orphaned_managed_children(scaffold_root, active_socket_ids);

    // ------------------------------------------------------------------------
    // 2. Topology/ (Separable per-part MeshInstance3D nodes with seamless seam normals)
    // ------------------------------------------------------------------------
    auto *topology_root = godot::Object::cast_to<godot::Node3D>(_find_managed_child(p_owner_node, "topology_root"));
    if (!topology_root) {
        topology_root = memnew(godot::Node3D);
        topology_root->set_name("Topology");
        topology_root->set_meta(META_MANAGED_ID, godot::String("topology_root"));
        // Hidden by default so BodyMesh renders unified unless the artist toggles per-part visibility
        topology_root->set_visible(false);
        p_owner_node->add_child(topology_root);
    }
    _assign_scene_owner(p_owner_node, topology_root);

    const godot::Dictionary part_meshes = inst->build_part_array_meshes(p_owner_node->get_lod_level());
    const godot::Array part_keys = part_meshes.keys();
    std::unordered_set<std::string> active_part_ids;
    for (int64_t i = 0; i < part_keys.size(); ++i) {
        const uint32_t part_id = static_cast<uint32_t>(int64_t(part_keys[i]));
        const std::string id_str = "topology_part_" + std::to_string(part_id);
        active_part_ids.insert(id_str);
        const godot::String managed_id = godot::String::utf8(id_str.c_str());

        auto *part_mi = godot::Object::cast_to<godot::MeshInstance3D>(_find_managed_child(topology_root, managed_id));
        if (!part_mi) {
            part_mi = memnew(godot::MeshInstance3D);
            godot::String part_name = godot::String("Part_") + godot::String::num_int64(part_id);
            if (auto it = scaffold_map.find(part_id); it != scaffold_map.end() && !it->second->name.empty()) {
                part_name = godot::String::utf8(it->second->name.c_str()) + godot::String("_PartMesh");
            }
            part_mi->set_name(part_name);
            part_mi->set_meta(META_MANAGED_ID, managed_id);
            topology_root->add_child(part_mi);
        }
        _assign_scene_owner(p_owner_node, part_mi);

        godot::Ref<godot::ArrayMesh> pm = part_meshes[part_keys[i]];
        part_mi->set_mesh(pm);
        part_mi->set_material_override(p_material_override);
    }
    prune_orphaned_managed_children(topology_root, active_part_ids);

    // ------------------------------------------------------------------------
    // 3. Features/ (Per-feature anatomical Node3D anchors for accessories/eyeballs)
    // ------------------------------------------------------------------------
    auto *features_root = godot::Object::cast_to<godot::Node3D>(_find_managed_child(p_owner_node, "features_root"));
    if (!features_root) {
        features_root = memnew(godot::Node3D);
        features_root->set_name("Features");
        features_root->set_meta(META_MANAGED_ID, godot::String("features_root"));
        p_owner_node->add_child(features_root);
    }
    _assign_scene_owner(p_owner_node, features_root);

    std::unordered_set<std::string> active_feature_ids;
    for (const auto &feat : plan->get_core_plan().features()) {
        const std::string id_str = "feature_anchor_" + std::to_string(feat.id);
        active_feature_ids.insert(id_str);
        const godot::String managed_id = godot::String::utf8(id_str.c_str());

        auto *anchor = godot::Object::cast_to<godot::Node3D>(_find_managed_child(features_root, managed_id));
        if (!anchor) {
            anchor = memnew(godot::Node3D);
            const godot::String fname = feat.name.empty()
                ? (godot::String("Feature_") + godot::String::num_int64(feat.id))
                : (godot::String::utf8(feat.name.c_str()) + godot::String("_Anchor"));
            anchor->set_name(fname);
            anchor->set_meta(META_MANAGED_ID, managed_id);
            features_root->add_child(anchor);
        }
        _assign_scene_owner(p_owner_node, anchor);

        if (auto it = scaffold_map.find(feat.host_scaffold_node_id); it != scaffold_map.end()) {
            const auto *host = it->second;
            const glm::vec3 ext = host->evaluated_half_extents;
            const glm::vec3 local_pos(
                (feat.host_u - 0.5f) * 2.0f * ext.x,
                (feat.host_v - 0.5f) * 2.0f * ext.y,
                ext.z
            );
            const glm::vec3 world_pos = glm::vec3(host->world_transform * glm::vec4(local_pos, 1.0f));
            anchor->set_position(godot::Vector3(world_pos.x, world_pos.y, world_pos.z));
        }
    }
    prune_orphaned_managed_children(features_root, active_feature_ids);

    // ------------------------------------------------------------------------
    // 4. Rig/ (CharacterSkeleton Skeleton3D + BodyMesh MeshInstance3D)
    // ------------------------------------------------------------------------
    auto *rig_root = godot::Object::cast_to<godot::Node3D>(_find_managed_child(p_owner_node, "rig_root"));
    if (!rig_root) {
        rig_root = memnew(godot::Node3D);
        rig_root->set_name("Rig");
        rig_root->set_meta(META_MANAGED_ID, godot::String("rig_root"));
        p_owner_node->add_child(rig_root);
    }
    _assign_scene_owner(p_owner_node, rig_root);

    auto *skel = godot::Object::cast_to<godot::Skeleton3D>(_find_managed_child(rig_root, "rig_skeleton"));
    if (!skel) {
        skel = memnew(godot::Skeleton3D);
        skel->set_name("CharacterSkeleton");
        skel->set_meta(META_MANAGED_ID, godot::String("rig_skeleton"));
        rig_root->add_child(skel);
    }
    _assign_scene_owner(p_owner_node, skel);

    const auto &bones = evaluated.skeleton_plan;
    const bool bone_count_changed = (skel->get_bone_count() != static_cast<int32_t>(bones.size()));
    if (bone_count_changed) {
        skel->clear_bones();
        for (const auto &b : bones) {
            const int32_t idx = skel->add_bone(godot::String::utf8(b.bone_name.c_str()));
            if (b.parent_bone_index >= 0 && b.parent_bone_index < idx) {
                skel->set_bone_parent(idx, b.parent_bone_index);
            }
        }
    }
    for (size_t i = 0; i < bones.size(); ++i) {
        const int32_t idx = static_cast<int32_t>(i);
        const godot::Transform3D rest = glm4_to_gd(bones[i].rest_local_transform);
        skel->set_bone_rest(idx, rest);
        skel->set_bone_pose_position(idx, rest.origin);
        skel->set_bone_pose_rotation(idx, rest.basis.get_rotation_quaternion());
    }

    auto *body_mesh_inst = godot::Object::cast_to<godot::MeshInstance3D>(_find_managed_child(rig_root, "rig_body_mesh"));
    if (!body_mesh_inst) {
        body_mesh_inst = memnew(godot::MeshInstance3D);
        body_mesh_inst->set_name("BodyMesh");
        body_mesh_inst->set_meta(META_MANAGED_ID, godot::String("rig_body_mesh"));
        rig_root->add_child(body_mesh_inst);
    }
    _assign_scene_owner(p_owner_node, body_mesh_inst);

    body_mesh_inst->set_mesh(p_mesh);
    body_mesh_inst->set_material_override(p_material_override);
    if (body_mesh_inst->is_inside_tree() && skel->is_inside_tree()) {
        body_mesh_inst->set_skeleton_path(body_mesh_inst->get_path_to(skel));
    }

    // Optional refitted CharacterApparelResource mesh under Rig/ApparelMesh
    const godot::Ref<godot::ArrayMesh> apparel_mesh = p_owner_node->get_built_apparel_mesh();
    auto *apparel_inst = godot::Object::cast_to<godot::MeshInstance3D>(_find_managed_child(rig_root, "rig_apparel_mesh"));
    if (apparel_mesh.is_valid() && apparel_mesh->get_surface_count() > 0) {
        if (!apparel_inst) {
            apparel_inst = memnew(godot::MeshInstance3D);
            apparel_inst->set_name("ApparelMesh");
            apparel_inst->set_meta(META_MANAGED_ID, godot::String("rig_apparel_mesh"));
            rig_root->add_child(apparel_inst);
        }
        _assign_scene_owner(p_owner_node, apparel_inst);
        apparel_inst->set_mesh(apparel_mesh);
        if (apparel_inst->is_inside_tree() && skel->is_inside_tree()) {
            apparel_inst->set_skeleton_path(apparel_inst->get_path_to(skel));
        }
    } else if (apparel_inst) {
        rig_root->remove_child(apparel_inst);
        memdelete(apparel_inst);
    }
}

void CharacterHierarchySynchronizer::clear_managed_nodes(LoomisCharacterNode3D *p_owner_node) {
    if (!p_owner_node) return;
    std::vector<godot::Node *> managed;
    for (int i = 0; i < p_owner_node->get_child_count(); ++i) {
        godot::Node *ch = p_owner_node->get_child(i);
        if (ch && ch->has_meta(META_MANAGED_ID)) {
            managed.push_back(ch);
        }
    }
    for (godot::Node *m : managed) {
        p_owner_node->remove_child(m);
        memdelete(m);
    }
}

} // namespace ideam::godot_ext
