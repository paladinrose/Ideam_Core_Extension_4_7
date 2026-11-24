#include "character_apparel_resource.h"

#include "ideam_characters_plugin.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

#include <algorithm>
#include <span>

namespace ideam::godot_ext {

void CharacterApparelResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_normal_offset", "offset"), &CharacterApparelResource::set_normal_offset);
    ClassDB::bind_method(D_METHOD("get_normal_offset"), &CharacterApparelResource::get_normal_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_offset", PROPERTY_HINT_RANGE, "0.0,0.25,0.001"), "set_normal_offset", "get_normal_offset");

    ClassDB::bind_method(D_METHOD("set_xpbd_relax_iterations", "iterations"), &CharacterApparelResource::set_xpbd_relax_iterations);
    ClassDB::bind_method(D_METHOD("get_xpbd_relax_iterations"), &CharacterApparelResource::get_xpbd_relax_iterations);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "xpbd_relax_iterations", PROPERTY_HINT_RANGE, "0,16,1"), "set_xpbd_relax_iterations", "get_xpbd_relax_iterations");

    ClassDB::bind_method(D_METHOD("set_xpbd_compliance", "compliance"), &CharacterApparelResource::set_xpbd_compliance);
    ClassDB::bind_method(D_METHOD("get_xpbd_compliance"), &CharacterApparelResource::get_xpbd_compliance);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "xpbd_compliance", PROPERTY_HINT_RANGE, "0.0,0.1,0.0001"), "set_xpbd_compliance", "get_xpbd_compliance");

    ClassDB::bind_method(D_METHOD("set_target_partition_mask", "mask"), &CharacterApparelResource::set_target_partition_mask);
    ClassDB::bind_method(D_METHOD("get_target_partition_mask"), &CharacterApparelResource::get_target_partition_mask);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "target_partition_mask"), "set_target_partition_mask", "get_target_partition_mask");

    ClassDB::bind_method(D_METHOD("set_substeps_per_quad_axis", "steps"), &CharacterApparelResource::set_substeps_per_quad_axis);
    ClassDB::bind_method(D_METHOD("get_substeps_per_quad_axis"), &CharacterApparelResource::get_substeps_per_quad_axis);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "substeps_per_quad_axis", PROPERTY_HINT_RANGE, "1,8,1"), "set_substeps_per_quad_axis", "get_substeps_per_quad_axis");

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &CharacterApparelResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &CharacterApparelResource::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );

    ClassDB::bind_method(D_METHOD("get_garment_vertex_count"), &CharacterApparelResource::get_garment_vertex_count);
    ClassDB::bind_method(D_METHOD("generate_default_bindings_for_character", "character"), &CharacterApparelResource::generate_default_bindings_for_character);
    ClassDB::bind_method(D_METHOD("evaluate_refitted_positions", "character"), &CharacterApparelResource::evaluate_refitted_positions);
    ClassDB::bind_method(D_METHOD("build_refitted_apparel_mesh", "character"), &CharacterApparelResource::build_refitted_apparel_mesh);

    ClassDB::bind_method(
        D_METHOD("commit_set_normal_offset", "offset", "undo_redo"),
        &CharacterApparelResource::commit_set_normal_offset,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_xpbd_relax_iterations", "iterations", "undo_redo"),
        &CharacterApparelResource::commit_set_xpbd_relax_iterations,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_target_partition_mask", "mask", "undo_redo"),
        &CharacterApparelResource::commit_set_target_partition_mask,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
}

void CharacterApparelResource::set_normal_offset(real_t p_offset) {
    normal_offset_ = std::max(static_cast<real_t>(0.0), p_offset);
    emit_changed();
}

void CharacterApparelResource::set_xpbd_relax_iterations(int p_iters) {
    xpbd_relax_iterations_ = std::clamp(p_iters, 0, 16);
    emit_changed();
}

void CharacterApparelResource::set_xpbd_compliance(float p_compliance) {
    xpbd_compliance_ = std::max(0.0f, p_compliance);
    emit_changed();
}

void CharacterApparelResource::set_target_partition_mask(int p_mask) {
    target_partition_mask_ = static_cast<uint32_t>(p_mask) & core::PARTITION_MASK_ALL;
    emit_changed();
}

void CharacterApparelResource::set_substeps_per_quad_axis(int p_steps) {
    substeps_per_quad_axis_ = std::clamp(p_steps, 1, 8);
    emit_changed();
}

void CharacterApparelResource::set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

void CharacterApparelResource::generate_default_bindings_for_character(
    const godot::Ref<LoomisCharacter3DResource> &p_character
) {
    if (p_character.is_null()) return;
    p_character->get_core_mesh().generate_default_garment_bindings(
        garment_bindings_,
        target_partition_mask_,
        static_cast<float>(normal_offset_),
        static_cast<uint32_t>(substeps_per_quad_axis_)
    );
    emit_changed();
}

godot::PackedVector3Array CharacterApparelResource::evaluate_refitted_positions(
    const godot::Ref<LoomisCharacter3DResource> &p_character
) const {
    godot::PackedVector3Array out;
    if (p_character.is_null()) return out;

    std::vector<core::SurfaceGarmentBindPod> active_binds = garment_bindings_;
    if (active_binds.empty()) {
        p_character->get_core_mesh().generate_default_garment_bindings(
            active_binds,
            target_partition_mask_,
            static_cast<float>(normal_offset_),
            static_cast<uint32_t>(substeps_per_quad_axis_)
        );
    }
    if (active_binds.empty()) return out;

    std::vector<glm::vec3> body_pos, body_nrm;
    p_character->evaluate_deformed_buffers(body_pos, body_nrm);

    std::vector<glm::vec3> g_pos(active_binds.size(), glm::vec3(0.0f));
    core::ParametricCharacterMesh::evaluate_garment_surface_batch(
        std::span<const core::SurfaceGarmentBindPod>(active_binds),
        std::span<const core::QuadFacePod>(p_character->get_core_mesh().base_quads()),
        std::span<const glm::vec3>(body_pos),
        std::span<const glm::vec3>(body_nrm),
        0.0f,
        g_pos.data(),
        g_pos.size()
    );

    out.resize(static_cast<int>(g_pos.size()));
    for (size_t i = 0; i < g_pos.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector3(g_pos[i].x, g_pos[i].y, g_pos[i].z));
    }
    return out;
}

godot::Ref<godot::ArrayMesh> CharacterApparelResource::build_refitted_apparel_mesh(
    const godot::Ref<LoomisCharacter3DResource> &p_character
) const {
    using namespace godot;
    Ref<ArrayMesh> mesh;
    mesh.instantiate();
    if (p_character.is_null()) return mesh;

    std::vector<glm::vec3> body_pos, body_nrm;
    p_character->evaluate_deformed_buffers(body_pos, body_nrm);
    const auto &quads = p_character->get_core_mesh().base_quads();
    const auto &verts = p_character->get_core_mesh().base_vertices();
    if (body_pos.empty() || quads.empty()) return mesh;

    PackedVector3Array gd_verts;
    PackedVector3Array gd_norms;
    PackedInt32Array gd_indices;

    const float d_off = static_cast<float>(normal_offset_);
    for (size_t i = 0; i < body_pos.size(); ++i) {
        const glm::vec3 p = body_pos[i] + body_nrm[i] * d_off;
        gd_verts.push_back(Vector3(p.x, p.y, p.z));
        gd_norms.push_back(Vector3(body_nrm[i].x, body_nrm[i].y, body_nrm[i].z));
    }

    for (const auto &q : quads) {
        if (q[0] >= verts.size()) continue;
        const uint16_t part_id = verts[q[0]].partition_id;
        if (part_id < 32u && ((target_partition_mask_ & (1u << part_id)) == 0u)) continue;

        gd_indices.push_back(static_cast<int32_t>(q[0]));
        gd_indices.push_back(static_cast<int32_t>(q[1]));
        gd_indices.push_back(static_cast<int32_t>(q[2]));

        gd_indices.push_back(static_cast<int32_t>(q[0]));
        gd_indices.push_back(static_cast<int32_t>(q[2]));
        gd_indices.push_back(static_cast<int32_t>(q[3]));
    }

    if (gd_indices.size() >= 3) {
        Array surface_arrays;
        surface_arrays.resize(Mesh::ARRAY_MAX);
        surface_arrays[Mesh::ARRAY_VERTEX] = gd_verts;
        surface_arrays[Mesh::ARRAY_NORMAL] = gd_norms;
        surface_arrays[Mesh::ARRAY_INDEX]  = gd_indices;
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, surface_arrays);
    }

    return mesh;
}

void CharacterApparelResource::commit_set_normal_offset(
    real_t p_offset,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const real_t old_val = normal_offset_;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Set Character Apparel Normal Offset");
        ur->add_do_property(this, "normal_offset", p_offset);
        ur->add_undo_property(this, "normal_offset", old_val);
        ur->commit_action();
    } else {
        set_normal_offset(p_offset);
    }
}

void CharacterApparelResource::commit_set_xpbd_relax_iterations(
    int p_iters,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const int old_val = xpbd_relax_iterations_;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Set Character Apparel XPBD Relax Iterations");
        ur->add_do_property(this, "xpbd_relax_iterations", p_iters);
        ur->add_undo_property(this, "xpbd_relax_iterations", old_val);
        ur->commit_action();
    } else {
        set_xpbd_relax_iterations(p_iters);
    }
}

void CharacterApparelResource::commit_set_target_partition_mask(
    int p_mask,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const int old_val = static_cast<int>(target_partition_mask_);
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Set Character Apparel Partition Mask");
        ur->add_do_property(this, "target_partition_mask", p_mask);
        ur->add_undo_property(this, "target_partition_mask", old_val);
        ur->commit_action();
    } else {
        set_target_partition_mask(p_mask);
    }
}

} // namespace ideam::godot_ext
