#pragma once

/**
 * loomis_character_node_3d.h
 * Unified 3D Character SceneTree Node (`LoomisCharacterNode3D`, `godot::Node3D`).
 *
 * Unifies the 4-layer Loomis character stack (`LoomisCharacter3DResource`) with
 * transparent Scene Tree hierarchy synchronization (`Scaffold/`, `Topology/`,
 * `Features/`, `Rig/`), optional baked RID rendering (`HIERARCHY_BAKED_COMBINED`),
 * live LOD 0..3 switching, 1-click `Skeleton3D` export, refitted `CharacterApparelResource`
 * attachment, and OpenXR trigger/grip morph channel bridging.
 */

#include "character_apparel_resource.h"
#include "loomis_character_3d_resource.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace ideam::godot_ext {

enum CharacterHierarchyMode : int32_t {
    HIERARCHY_TRANSPARENT_LIVE = 0, ///< Default: scene-owned Scaffold/, Topology/, Features/, Rig/ child nodes
    HIERARCHY_BAKED_COMBINED   = 1  ///< Optional runtime optimization: single RenderingServer RID mesh instance
};

class LoomisCharacterNode3D : public godot::Node3D {
    GDCLASS(LoomisCharacterNode3D, godot::Node3D)

private:
    godot::Ref<LoomisCharacter3DResource> character_resource_;
    godot::Ref<CharacterApparelResource> primary_apparel_;
    int lod_level_                         = 0;
    CharacterHierarchyMode hierarchy_mode_ = HIERARCHY_TRANSPARENT_LIVE;
    bool auto_rebuild_meshes_              = true;
    godot::Ref<godot::Material> material_override_;
    godot::Ref<godot::ArrayMesh> built_mesh_;
    godot::Ref<godot::ArrayMesh> built_apparel_mesh_;
    godot::RID mesh_instance_rid_;

    void _on_character_changed();
    void _update_rid_visibility();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    LoomisCharacterNode3D();
    ~LoomisCharacterNode3D() override;

    void set_character_resource(const godot::Ref<LoomisCharacter3DResource> &p_resource);
    [[nodiscard]] godot::Ref<LoomisCharacter3DResource> get_character_resource() const { return character_resource_; }

    void set_primary_apparel(const godot::Ref<CharacterApparelResource> &p_apparel);
    [[nodiscard]] godot::Ref<CharacterApparelResource> get_primary_apparel() const { return primary_apparel_; }

    void set_lod_level(int p_lod);
    [[nodiscard]] int get_lod_level() const noexcept { return lod_level_; }

    void set_hierarchy_mode(CharacterHierarchyMode p_mode);
    [[nodiscard]] CharacterHierarchyMode get_hierarchy_mode() const noexcept { return hierarchy_mode_; }

    void set_auto_rebuild_meshes(bool p_auto);
    [[nodiscard]] bool get_auto_rebuild_meshes() const noexcept { return auto_rebuild_meshes_; }

    void set_material_override(const godot::Ref<godot::Material> &p_material);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_built_mesh() const { return built_mesh_; }
    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_built_apparel_mesh() const { return built_apparel_mesh_; }
    [[nodiscard]] godot::Skeleton3D *build_skeleton_3d() const;

    /**
     * Bridges OpenXR controller analog trigger & grip floats (`GameplayControlMap`)
     * directly onto `MORPH_DIGIT_CURL_0..4` on `character_resource_`.
     */
    void apply_xr_trigger_grip_morphs(float p_trigger, float p_grip, float p_thumb = -1.0f);

    void rebuild_character();
    [[nodiscard]] godot::Dictionary get_character_telemetry_summary() const;

    // --- Tier 1 `IdeamUndoRedo` Reversible Mutation API ---
    void commit_set_character_resource(
        const godot::Ref<LoomisCharacter3DResource> &p_resource,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_primary_apparel(
        const godot::Ref<CharacterApparelResource> &p_apparel,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::CharacterHierarchyMode);
