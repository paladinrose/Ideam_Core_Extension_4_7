#pragma once

/**
 * layered_character_node_3d.h
 * 3D SceneTree Node (`LayeredCharacterNode3D`, `godot::Node3D`) rendering a live
 * `CharacterInstanceResource` (4-layer Loomis stack: Scaffold -> Topology -> Features -> Details)
 * via `RenderingServer` + `ArrayMesh`, with live LOD 0..3 switching and 1-click `Skeleton3D` export.
 */

#include "character_instance_resource.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace ideam::godot_ext {

class LayeredCharacterNode3D : public godot::Node3D {
    GDCLASS(LayeredCharacterNode3D, godot::Node3D)

private:
    godot::Ref<CharacterInstanceResource> character_instance_;
    int lod_level_ = 0;
    godot::Ref<godot::Material> material_override_;
    godot::Ref<godot::ArrayMesh> built_mesh_;
    godot::RID mesh_instance_rid_;

    void _on_instance_changed();

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    LayeredCharacterNode3D();
    ~LayeredCharacterNode3D() override;

    void set_character_instance(const godot::Ref<CharacterInstanceResource> &p_instance);
    [[nodiscard]] godot::Ref<CharacterInstanceResource> get_character_instance() const { return character_instance_; }

    void set_lod_level(int p_lod);
    [[nodiscard]] int get_lod_level() const noexcept { return lod_level_; }

    void set_material_override(const godot::Ref<godot::Material> &p_material);
    [[nodiscard]] godot::Ref<godot::Material> get_material_override() const { return material_override_; }

    [[nodiscard]] godot::Ref<godot::ArrayMesh> get_built_mesh() const { return built_mesh_; }
    [[nodiscard]] godot::Skeleton3D *build_skeleton_3d() const;

    void rebuild_character();
};

} // namespace ideam::godot_ext
