#pragma once

/**
 * character_hierarchy_synchronizer.h
 * Transparent Scene Tree Reconciliation Engine (`CharacterHierarchySynchronizer`, Phase 1)
 * in `src/godot/characters/`.
 *
 * Reconciles visible, scene-owned (`set_owner(get_tree()->get_edited_scene_root())`)
 * child subtrees under `LoomisCharacterNode3D` in-place with deterministic names and
 * `ideam_managed_id` metadata tags so user-attached scripts, collision shapes, VFX, or
 * custom children are NEVER destroyed on rebuild:
 *
 *   LoomisCharacterNode3D (Root)
 *     ├── Scaffold (ProportionContainer3D bound to BodyPlanResource.base_scaffold.ruler_graph)
 *     │     ├── Pelvis_Socket, Thorax_Socket, Shoulder_L_Socket, ... (Socket3D nodes)
 *     └── Rig (Node3D)
 *           ├── CharacterSkeleton (Skeleton3D synced in-place)
 *           └── BodyMesh (MeshInstance3D bound to evaluated ArrayMesh & material_override)
 */

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>

namespace ideam::godot_ext {

class LoomisCharacterNode3D;

class CharacterHierarchySynchronizer {
public:
    static constexpr const char *META_MANAGED_ID = "ideam_managed_id";

    /**
     * Synchronizes Phase 1 transparent hierarchy (`Scaffold/` + `Rig/Skeleton3D` + `Rig/BodyMesh`)
     * in-place under `p_owner_node`.
     */
    static void synchronize(
        LoomisCharacterNode3D *p_owner_node,
        const godot::Ref<godot::ArrayMesh> &p_mesh,
        const godot::Ref<godot::Material> &p_material_override
    );

    /**
     * Removes only `ideam_managed_id`-tagged children created by the synchronizer
     * (used when switching from `HIERARCHY_TRANSPARENT_LIVE` to `HIERARCHY_BAKED_COMBINED`).
     */
    static void clear_managed_nodes(LoomisCharacterNode3D *p_owner_node);

private:
    static void _assign_scene_owner(godot::Node *p_owner_root, godot::Node *p_child);
    static godot::Node *_find_managed_child(godot::Node *p_parent, const godot::String &p_managed_id);
};

} // namespace ideam::godot_ext
