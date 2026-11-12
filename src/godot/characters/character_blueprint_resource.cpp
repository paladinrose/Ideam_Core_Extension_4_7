#include "character_blueprint_resource.h"
#include <algorithm>

namespace ideam::godot_ext {

CharacterBlueprintResource::CharacterBlueprintResource() {
    core_mesh_.build_archetype_preset(core::CharacterBodyArchetype::BIPED);
}

void CharacterBlueprintResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_body_archetype", "archetype"), &CharacterBlueprintResource::set_body_archetype);
    ClassDB::bind_method(D_METHOD("get_body_archetype"), &CharacterBlueprintResource::get_body_archetype);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "body_archetype", PROPERTY_HINT_ENUM, "Biped,Quadruped,Avian,FlyingEye,Serpentine,Custom"),
        "set_body_archetype",
        "get_body_archetype"
    );

    ClassDB::bind_method(D_METHOD("set_subdivision_lod_level", "lod_level"), &CharacterBlueprintResource::set_subdivision_lod_level);
    ClassDB::bind_method(D_METHOD("get_subdivision_lod_level"), &CharacterBlueprintResource::get_subdivision_lod_level);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "subdivision_lod_level", PROPERTY_HINT_RANGE, "0,3,1"),
        "set_subdivision_lod_level",
        "get_subdivision_lod_level"
    );

    ClassDB::bind_method(D_METHOD("set_morph_weights", "weights"), &CharacterBlueprintResource::set_morph_weights);
    ClassDB::bind_method(D_METHOD("get_morph_weights"), &CharacterBlueprintResource::get_morph_weights);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "morph_weights"),
        "set_morph_weights",
        "get_morph_weights"
    );

    ClassDB::bind_method(D_METHOD("get_base_vertex_count"), &CharacterBlueprintResource::get_base_vertex_count);
    ClassDB::bind_method(D_METHOD("get_base_quad_count"), &CharacterBlueprintResource::get_base_quad_count);
}

void CharacterBlueprintResource::set_body_archetype(int p_archetype) {
    const auto arch = static_cast<core::CharacterBodyArchetype>(std::clamp(p_archetype, 0, 5));
    core_mesh_.build_archetype_preset(arch);
    emit_changed();
}

void CharacterBlueprintResource::set_subdivision_lod_level(int p_lod) {
    subdivision_lod_level_ = std::clamp(p_lod, 0, 3);
    emit_changed();
}

void CharacterBlueprintResource::set_morph_weights(const godot::PackedFloat32Array &p_weights) {
    morph_weights_ = p_weights;
    emit_changed();
}

void CharacterApparelResource::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_normal_offset", "offset"), &CharacterApparelResource::set_normal_offset);
    ClassDB::bind_method(D_METHOD("get_normal_offset"), &CharacterApparelResource::get_normal_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_offset", PROPERTY_HINT_RANGE, "0.0,0.5,0.001"), "set_normal_offset", "get_normal_offset");

    ClassDB::bind_method(D_METHOD("set_xpbd_relax_iterations", "iterations"), &CharacterApparelResource::set_xpbd_relax_iterations);
    ClassDB::bind_method(D_METHOD("get_xpbd_relax_iterations"), &CharacterApparelResource::get_xpbd_relax_iterations);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "xpbd_relax_iterations", PROPERTY_HINT_RANGE, "0,16,1"), "set_xpbd_relax_iterations", "get_xpbd_relax_iterations");
}

void CharacterApparelResource::set_normal_offset(real_t p_offset) {
    normal_offset_ = std::max(static_cast<real_t>(0.0), p_offset);
    emit_changed();
}

void CharacterApparelResource::set_xpbd_relax_iterations(int p_iters) {
    xpbd_relax_iterations_ = std::clamp(p_iters, 0, 16);
    emit_changed();
}

void ParametricCharacterNode3D::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_blueprint", "blueprint"), &ParametricCharacterNode3D::set_blueprint);
    ClassDB::bind_method(D_METHOD("get_blueprint"), &ParametricCharacterNode3D::get_blueprint);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "blueprint", PROPERTY_HINT_RESOURCE_TYPE, "CharacterBlueprintResource"),
        "set_blueprint",
        "get_blueprint"
    );

    ClassDB::bind_method(D_METHOD("set_primary_apparel", "apparel"), &ParametricCharacterNode3D::set_primary_apparel);
    ClassDB::bind_method(D_METHOD("get_primary_apparel"), &ParametricCharacterNode3D::get_primary_apparel);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "primary_apparel", PROPERTY_HINT_RESOURCE_TYPE, "CharacterApparelResource"),
        "set_primary_apparel",
        "get_primary_apparel"
    );
}

void ParametricCharacterNode3D::set_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint) {
    blueprint_ = p_blueprint;
}

void ParametricCharacterNode3D::set_primary_apparel(const godot::Ref<CharacterApparelResource> &p_apparel) {
    primary_apparel_ = p_apparel;
}

} // namespace ideam::godot_ext
