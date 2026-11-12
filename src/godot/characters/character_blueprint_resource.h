#pragma once

/**
 * character_blueprint_resource.h
 * `CharacterBlueprintResource`, `CharacterApparelResource`, and `ParametricCharacterNode3D`
 * (`src/godot/characters/`).
 */

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include "../../core/characters/parametric_character_mesh.h"

namespace ideam::godot_ext {

class CharacterBlueprintResource : public godot::Resource {
    GDCLASS(CharacterBlueprintResource, godot::Resource)

private:
    core::ParametricCharacterMesh core_mesh_;
    int subdivision_lod_level_ = 1;
    godot::PackedFloat32Array morph_weights_;

protected:
    static void _bind_methods();

public:
    CharacterBlueprintResource();
    ~CharacterBlueprintResource() override = default;

    void set_body_archetype(int p_archetype);
    [[nodiscard]] int get_body_archetype() const { return static_cast<int>(core_mesh_.archetype()); }

    void set_subdivision_lod_level(int p_lod);
    [[nodiscard]] int get_subdivision_lod_level() const { return subdivision_lod_level_; }

    void set_morph_weights(const godot::PackedFloat32Array &p_weights);
    [[nodiscard]] godot::PackedFloat32Array get_morph_weights() const { return morph_weights_; }

    [[nodiscard]] int get_base_vertex_count() const { return static_cast<int>(core_mesh_.vertex_count()); }
    [[nodiscard]] int get_base_quad_count() const { return static_cast<int>(core_mesh_.quad_count()); }
};

class CharacterApparelResource : public godot::Resource {
    GDCLASS(CharacterApparelResource, godot::Resource)

private:
    real_t normal_offset_      = static_cast<real_t>(0.015);
    int xpbd_relax_iterations_ = 3;

protected:
    static void _bind_methods();

public:
    CharacterApparelResource() = default;
    ~CharacterApparelResource() override = default;

    void set_normal_offset(real_t p_offset);
    [[nodiscard]] real_t get_normal_offset() const { return normal_offset_; }

    void set_xpbd_relax_iterations(int p_iters);
    [[nodiscard]] int get_xpbd_relax_iterations() const { return xpbd_relax_iterations_; }
};

class ParametricCharacterNode3D : public godot::Node3D {
    GDCLASS(ParametricCharacterNode3D, godot::Node3D)

private:
    godot::Ref<CharacterBlueprintResource> blueprint_;
    godot::Ref<CharacterApparelResource> primary_apparel_;

protected:
    static void _bind_methods();

public:
    ParametricCharacterNode3D() = default;
    ~ParametricCharacterNode3D() override = default;

    void set_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint);
    [[nodiscard]] godot::Ref<CharacterBlueprintResource> get_blueprint() const { return blueprint_; }

    void set_primary_apparel(const godot::Ref<CharacterApparelResource> &p_apparel);
    [[nodiscard]] godot::Ref<CharacterApparelResource> get_primary_apparel() const { return primary_apparel_; }
};

} // namespace ideam::godot_ext
