#include "character_blueprint_resource.h"
#include "ideam_characters_plugin.h"

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <algorithm>
#include <cmath>
#include <span>

namespace ideam::godot_ext {

// ============================================================================
// CharacterBlueprintResource
// ============================================================================

CharacterBlueprintResource::CharacterBlueprintResource() {
    core_mesh_.build_archetype_preset(core::CharacterBodyArchetype::BIPED);
    _ensure_morph_weights_size();
}

void CharacterBlueprintResource::_ensure_morph_weights_size() {
    const int min_channels = static_cast<int>(core::MORPH_DEFAULT_CHANNEL_COUNT);
    if (morph_weights_.size() < min_channels) {
        const int old_size = static_cast<int>(morph_weights_.size());
        morph_weights_.resize(min_channels);
        for (int i = old_size; i < min_channels; ++i) {
            morph_weights_.set(i, 0.0f);
        }
    }
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

    ClassDB::bind_method(D_METHOD("set_active_partition_mask", "mask"), &CharacterBlueprintResource::set_active_partition_mask);
    ClassDB::bind_method(D_METHOD("get_active_partition_mask"), &CharacterBlueprintResource::get_active_partition_mask);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "active_partition_mask"),
        "set_active_partition_mask",
        "get_active_partition_mask"
    );

    ClassDB::bind_method(D_METHOD("set_morph_weights", "weights"), &CharacterBlueprintResource::set_morph_weights);
    ClassDB::bind_method(D_METHOD("get_morph_weights"), &CharacterBlueprintResource::get_morph_weights);
    ADD_PROPERTY(
        PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "morph_weights"),
        "set_morph_weights",
        "get_morph_weights"
    );

    ClassDB::bind_method(D_METHOD("set_semantic_tags", "tags"), &CharacterBlueprintResource::set_semantic_tags);
    ClassDB::bind_method(D_METHOD("get_semantic_tags"), &CharacterBlueprintResource::get_semantic_tags);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "semantic_tags", PROPERTY_HINT_RESOURCE_TYPE, "SemanticTagSetResource"),
        "set_semantic_tags",
        "get_semantic_tags"
    );

    ClassDB::bind_method(D_METHOD("get_morph_channel_count"), &CharacterBlueprintResource::get_morph_channel_count);
    ClassDB::bind_method(D_METHOD("get_morph_channel_name", "channel"), &CharacterBlueprintResource::get_morph_channel_name);
    ClassDB::bind_method(D_METHOD("get_all_morph_channel_names"), &CharacterBlueprintResource::get_all_morph_channel_names);
    ClassDB::bind_method(D_METHOD("set_morph_channel_weight", "channel", "weight"), &CharacterBlueprintResource::set_morph_channel_weight);
    ClassDB::bind_method(D_METHOD("get_morph_channel_weight", "channel"), &CharacterBlueprintResource::get_morph_channel_weight);
    ClassDB::bind_method(D_METHOD("reset_all_morph_weights"), &CharacterBlueprintResource::reset_all_morph_weights);

    ClassDB::bind_method(D_METHOD("get_base_vertex_count"), &CharacterBlueprintResource::get_base_vertex_count);
    ClassDB::bind_method(D_METHOD("get_base_quad_count"), &CharacterBlueprintResource::get_base_quad_count);
    ClassDB::bind_method(D_METHOD("get_sparse_morph_delta_count"), &CharacterBlueprintResource::get_sparse_morph_delta_count);
    ClassDB::bind_method(D_METHOD("get_deformed_vertex_positions"), &CharacterBlueprintResource::get_deformed_vertex_positions);
    ClassDB::bind_method(D_METHOD("get_deformed_vertex_normals"), &CharacterBlueprintResource::get_deformed_vertex_normals);
    ClassDB::bind_method(
        D_METHOD("evaluate_garment_point", "quad_id", "u", "v", "d_normal"),
        &CharacterBlueprintResource::evaluate_garment_point,
        DEFVAL(0.015f)
    );
    ClassDB::bind_method(D_METHOD("build_evaluated_array_mesh"), &CharacterBlueprintResource::build_evaluated_array_mesh);

    ClassDB::bind_method(
        D_METHOD("commit_set_body_archetype", "archetype", "undo_redo"),
        &CharacterBlueprintResource::commit_set_body_archetype,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_subdivision_lod_level", "lod_level", "undo_redo"),
        &CharacterBlueprintResource::commit_set_subdivision_lod_level,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_active_partition_mask", "mask", "undo_redo"),
        &CharacterBlueprintResource::commit_set_active_partition_mask,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_morph_weights", "weights", "undo_redo"),
        &CharacterBlueprintResource::commit_set_morph_weights,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_morph_channel_weight", "channel", "weight", "undo_redo"),
        &CharacterBlueprintResource::commit_set_morph_channel_weight,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_semantic_tags", "tags", "undo_redo"),
        &CharacterBlueprintResource::commit_set_semantic_tags,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
}

void CharacterBlueprintResource::set_body_archetype(int p_archetype) {
    const auto arch = static_cast<core::CharacterBodyArchetype>(std::clamp(p_archetype, 0, 5));
    core_mesh_.build_archetype_preset(arch);
    _ensure_morph_weights_size();
    emit_changed();
}

void CharacterBlueprintResource::set_subdivision_lod_level(int p_lod) {
    subdivision_lod_level_ = std::clamp(p_lod, 0, 3);
    emit_changed();
}

void CharacterBlueprintResource::set_active_partition_mask(int p_mask) {
    active_partition_mask_ = static_cast<uint32_t>(p_mask) & core::PARTITION_MASK_ALL;
    emit_changed();
}

void CharacterBlueprintResource::set_morph_weights(const godot::PackedFloat32Array &p_weights) {
    morph_weights_ = p_weights;
    _ensure_morph_weights_size();
    emit_changed();
}

void CharacterBlueprintResource::set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags) {
    semantic_tags_ = p_tags;
    emit_changed();
}

godot::String CharacterBlueprintResource::get_morph_channel_name(int p_channel) const {
    const std::string_view sv = core::morph_channel_to_string(static_cast<uint32_t>(std::max(0, p_channel)));
    return godot::String::utf8(sv.data(), static_cast<int>(sv.size()));
}

godot::PackedStringArray CharacterBlueprintResource::get_all_morph_channel_names() const {
    godot::PackedStringArray names;
    for (int i = 0; i < get_morph_channel_count(); ++i) {
        names.push_back(get_morph_channel_name(i));
    }
    return names;
}

void CharacterBlueprintResource::set_morph_channel_weight(int p_channel, float p_weight) {
    if (p_channel < 0) return;
    _ensure_morph_weights_size();
    if (p_channel >= morph_weights_.size()) {
        morph_weights_.resize(p_channel + 1);
    }
    morph_weights_.set(p_channel, std::clamp(p_weight, -2.0f, 2.0f));
    emit_changed();
}

float CharacterBlueprintResource::get_morph_channel_weight(int p_channel) const {
    if (p_channel < 0 || p_channel >= morph_weights_.size()) {
        return 0.0f;
    }
    return morph_weights_[p_channel];
}

void CharacterBlueprintResource::reset_all_morph_weights() {
    _ensure_morph_weights_size();
    for (int i = 0; i < morph_weights_.size(); ++i) {
        morph_weights_.set(i, 0.0f);
    }
    emit_changed();
}

void CharacterBlueprintResource::evaluate_deformed_buffers(
    std::vector<glm::vec3> &r_positions,
    std::vector<glm::vec3> &r_normals,
    std::vector<uint32_t> *r_active_masks
) const {
    const size_t n_verts = core_mesh_.vertex_count();
    r_positions.resize(n_verts);
    r_normals.resize(n_verts);
    if (r_active_masks) {
        r_active_masks->resize(n_verts);
    }
    if (n_verts == 0u) return;

    core::ParametricCharacterMesh::evaluate_quad_loop_lod(
        std::span<const core::QuadLoopVertexPod>(core_mesh_.base_vertices()),
        std::span<const core::QuadFacePod>(core_mesh_.base_quads()),
        static_cast<uint8_t>(std::clamp(subdivision_lod_level_, 0, 3)),
        0ULL,
        r_positions.data(),
        r_normals.data(),
        r_active_masks ? r_active_masks->data() : nullptr,
        n_verts
    );

    std::vector<float> weights(static_cast<size_t>(morph_weights_.size()), 0.0f);
    for (int i = 0; i < morph_weights_.size(); ++i) {
        weights[static_cast<size_t>(i)] = morph_weights_[i];
    }

    core::ParametricCharacterMesh::evaluate_sparse_morph_sliders(
        std::span<const core::QuadLoopVertexPod>(core_mesh_.base_vertices()),
        std::span<const core::MorphDeltaPod>(core_mesh_.sparse_morph_deltas()),
        std::span<const float>(weights),
        active_partition_mask_,
        r_positions.data(),
        r_normals.data(),
        n_verts
    );
}

godot::PackedVector3Array CharacterBlueprintResource::get_deformed_vertex_positions() const {
    std::vector<glm::vec3> pos, nrm;
    evaluate_deformed_buffers(pos, nrm);
    godot::PackedVector3Array out;
    out.resize(static_cast<int>(pos.size()));
    for (size_t i = 0; i < pos.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector3(pos[i].x, pos[i].y, pos[i].z));
    }
    return out;
}

godot::PackedVector3Array CharacterBlueprintResource::get_deformed_vertex_normals() const {
    std::vector<glm::vec3> pos, nrm;
    evaluate_deformed_buffers(pos, nrm);
    godot::PackedVector3Array out;
    out.resize(static_cast<int>(nrm.size()));
    for (size_t i = 0; i < nrm.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector3(nrm[i].x, nrm[i].y, nrm[i].z));
    }
    return out;
}

godot::Vector3 CharacterBlueprintResource::evaluate_garment_point(
    int p_quad_id,
    float p_u,
    float p_v,
    float p_d_normal
) const {
    std::vector<glm::vec3> pos, nrm;
    evaluate_deformed_buffers(pos, nrm);
    core::SurfaceGarmentBindPod bind{};
    bind.quad_id  = static_cast<uint32_t>(std::max(0, p_quad_id));
    bind.u        = p_u;
    bind.v        = p_v;
    bind.d_normal = p_d_normal;
    const glm::vec3 p = core::ParametricCharacterMesh::evaluate_garment_surface_point_on_buffers(
        bind,
        std::span<const core::QuadFacePod>(core_mesh_.base_quads()),
        std::span<const glm::vec3>(pos),
        std::span<const glm::vec3>(nrm)
    );
    return godot::Vector3(p.x, p.y, p.z);
}

godot::Ref<godot::ArrayMesh> CharacterBlueprintResource::build_evaluated_array_mesh() const {
    using namespace godot;
    Ref<ArrayMesh> mesh;
    mesh.instantiate();

    std::vector<glm::vec3> pos, nrm;
    std::vector<uint32_t> active_masks;
    evaluate_deformed_buffers(pos, nrm, &active_masks);
    if (pos.empty() || core_mesh_.base_quads().empty()) {
        return mesh;
    }

    PackedVector3Array gd_verts;
    PackedVector3Array gd_norms;
    PackedVector2Array gd_uvs;
    PackedInt32Array gd_indices;

    gd_verts.resize(static_cast<int>(pos.size()));
    gd_norms.resize(static_cast<int>(nrm.size()));
    gd_uvs.resize(static_cast<int>(pos.size()));

    for (size_t i = 0; i < pos.size(); ++i) {
        gd_verts.set(static_cast<int>(i), Vector3(pos[i].x, pos[i].y, pos[i].z));
        gd_norms.set(static_cast<int>(i), Vector3(nrm[i].x, nrm[i].y, nrm[i].z));
        const float u = static_cast<float>(i & 3u) / 3.0f;
        const float v = static_cast<float>(core_mesh_.base_vertices()[i].ring_index) * 0.1f;
        gd_uvs.set(static_cast<int>(i), Vector2(u, v));
    }

    for (const auto &q : core_mesh_.base_quads()) {
        if (q[0] >= pos.size() || q[1] >= pos.size() || q[2] >= pos.size() || q[3] >= pos.size()) {
            continue;
        }
        const uint8_t q_lod = std::max({
            core_mesh_.base_vertices()[q[0]].min_lod_level,
            core_mesh_.base_vertices()[q[1]].min_lod_level,
            core_mesh_.base_vertices()[q[2]].min_lod_level,
            core_mesh_.base_vertices()[q[3]].min_lod_level
        });
        if (q_lod > static_cast<uint8_t>(subdivision_lod_level_)) {
            continue;
        }
        const uint16_t part_id = core_mesh_.base_vertices()[q[0]].partition_id;
        if (part_id < 32u && ((active_partition_mask_ & (1u << part_id)) == 0u)) {
            continue;
        }

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
        surface_arrays[Mesh::ARRAY_TEX_UV] = gd_uvs;
        surface_arrays[Mesh::ARRAY_INDEX]  = gd_indices;
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, surface_arrays);
    }

    return mesh;
}

void CharacterBlueprintResource::commit_set_body_archetype(
    int p_archetype,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const int old_val = get_body_archetype();
    if (old_val == p_archetype) return;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Set Character Body Archetype");
        ur->add_do_property(this, "body_archetype", p_archetype);
        ur->add_undo_property(this, "body_archetype", old_val);
        ur->commit_action();
    } else {
        set_body_archetype(p_archetype);
    }
}

void CharacterBlueprintResource::commit_set_subdivision_lod_level(
    int p_lod,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const int old_val = subdivision_lod_level_;
    if (old_val == p_lod) return;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Set Character Subdivision LOD Level");
        ur->add_do_property(this, "subdivision_lod_level", p_lod);
        ur->add_undo_property(this, "subdivision_lod_level", old_val);
        ur->commit_action();
    } else {
        set_subdivision_lod_level(p_lod);
    }
}

void CharacterBlueprintResource::commit_set_active_partition_mask(
    int p_mask,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    const int old_val = static_cast<int>(active_partition_mask_);
    if (old_val == p_mask) return;
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Set Character Active Partition Mask");
        ur->add_do_property(this, "active_partition_mask", p_mask);
        ur->add_undo_property(this, "active_partition_mask", old_val);
        ur->commit_action();
    } else {
        set_active_partition_mask(p_mask);
    }
}

void CharacterBlueprintResource::commit_set_morph_weights(
    const godot::PackedFloat32Array &p_weights,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const godot::PackedFloat32Array old_weights = morph_weights_.duplicate();
        ur->create_action("Set Character Morph Weights");
        ur->add_do_property(this, "morph_weights", p_weights);
        ur->add_undo_property(this, "morph_weights", old_weights);
        ur->commit_action();
    } else {
        set_morph_weights(p_weights);
    }
}

void CharacterBlueprintResource::commit_set_morph_channel_weight(
    int p_channel,
    float p_weight,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    if (p_channel < 0) return;
    _ensure_morph_weights_size();
    const godot::PackedFloat32Array old_weights = morph_weights_.duplicate();
    godot::PackedFloat32Array next_weights      = morph_weights_.duplicate();
    if (p_channel >= next_weights.size()) {
        next_weights.resize(p_channel + 1);
    }
    next_weights.set(p_channel, std::clamp(p_weight, -2.0f, 2.0f));

    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        ur->create_action("Adjust Character Morph Slider: " + get_morph_channel_name(p_channel));
        ur->add_do_property(this, "morph_weights", next_weights);
        ur->add_undo_property(this, "morph_weights", old_weights);
        ur->commit_action();
    } else {
        set_morph_weights(next_weights);
    }
}

void CharacterBlueprintResource::commit_set_semantic_tags(
    const godot::Ref<SemanticTagSetResource> &p_tags,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const godot::Ref<SemanticTagSetResource> old_tags = semantic_tags_;
        ur->create_action("Set Character Semantic Tags");
        ur->add_do_property(this, "semantic_tags", p_tags);
        ur->add_undo_property(this, "semantic_tags", old_tags);
        ur->commit_action();
    } else {
        set_semantic_tags(p_tags);
    }
}

// ============================================================================
// CharacterApparelResource
// ============================================================================

void CharacterApparelResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_normal_offset", "offset"), &CharacterApparelResource::set_normal_offset);
    ClassDB::bind_method(D_METHOD("get_normal_offset"), &CharacterApparelResource::get_normal_offset);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "normal_offset", PROPERTY_HINT_RANGE, "0.0,0.5,0.001"), "set_normal_offset", "get_normal_offset");

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
    ClassDB::bind_method(D_METHOD("generate_default_bindings_for_blueprint", "blueprint"), &CharacterApparelResource::generate_default_bindings_for_blueprint);
    ClassDB::bind_method(D_METHOD("evaluate_refitted_positions", "blueprint"), &CharacterApparelResource::evaluate_refitted_positions);
    ClassDB::bind_method(D_METHOD("build_refitted_apparel_mesh", "blueprint"), &CharacterApparelResource::build_refitted_apparel_mesh);

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

void CharacterApparelResource::generate_default_bindings_for_blueprint(
    const godot::Ref<CharacterBlueprintResource> &p_blueprint
) {
    if (p_blueprint.is_null()) return;
    p_blueprint->get_core_mesh().generate_default_garment_bindings(
        garment_bindings_,
        target_partition_mask_,
        static_cast<float>(normal_offset_),
        static_cast<uint32_t>(substeps_per_quad_axis_)
    );
    emit_changed();
}

godot::PackedVector3Array CharacterApparelResource::evaluate_refitted_positions(
    const godot::Ref<CharacterBlueprintResource> &p_blueprint
) const {
    godot::PackedVector3Array out;
    if (p_blueprint.is_null()) return out;

    std::vector<core::SurfaceGarmentBindPod> active_binds = garment_bindings_;
    if (active_binds.empty()) {
        p_blueprint->get_core_mesh().generate_default_garment_bindings(
            active_binds,
            target_partition_mask_,
            static_cast<float>(normal_offset_),
            static_cast<uint32_t>(substeps_per_quad_axis_)
        );
    }
    if (active_binds.empty()) return out;

    std::vector<glm::vec3> body_pos, body_nrm;
    p_blueprint->evaluate_deformed_buffers(body_pos, body_nrm);

    std::vector<glm::vec3> g_pos(active_binds.size(), glm::vec3(0.0f));
    core::ParametricCharacterMesh::evaluate_garment_surface_batch(
        std::span<const core::SurfaceGarmentBindPod>(active_binds),
        std::span<const core::QuadFacePod>(p_blueprint->get_core_mesh().base_quads()),
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
    const godot::Ref<CharacterBlueprintResource> &p_blueprint
) const {
    using namespace godot;
    Ref<ArrayMesh> mesh;
    mesh.instantiate();
    if (p_blueprint.is_null()) return mesh;

    std::vector<glm::vec3> body_pos, body_nrm;
    p_blueprint->evaluate_deformed_buffers(body_pos, body_nrm);
    const auto &quads = p_blueprint->get_core_mesh().base_quads();
    const auto &verts = p_blueprint->get_core_mesh().base_vertices();
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
        if (part_id < 32u && ((target_partition_mask_ & (1u << part_id)) == 0u)) {
            continue;
        }
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

// ============================================================================
// ParametricCharacterNode3D
// ============================================================================

void ParametricCharacterNode3D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        if (auto_rebuild_meshes_ && blueprint_.is_valid()) {
            rebuild_character_meshes();
        }
    }
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

    ClassDB::bind_method(D_METHOD("set_auto_rebuild_meshes", "auto_rebuild"), &ParametricCharacterNode3D::set_auto_rebuild_meshes);
    ClassDB::bind_method(D_METHOD("get_auto_rebuild_meshes"), &ParametricCharacterNode3D::get_auto_rebuild_meshes);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_rebuild_meshes"), "set_auto_rebuild_meshes", "get_auto_rebuild_meshes");

    ClassDB::bind_method(
        D_METHOD("apply_xr_trigger_grip_morphs", "trigger", "grip", "thumb"),
        &ParametricCharacterNode3D::apply_xr_trigger_grip_morphs,
        DEFVAL(-1.0f)
    );
    ClassDB::bind_method(D_METHOD("rebuild_character_meshes"), &ParametricCharacterNode3D::rebuild_character_meshes);
    ClassDB::bind_method(D_METHOD("get_character_telemetry_summary"), &ParametricCharacterNode3D::get_character_telemetry_summary);

    ClassDB::bind_method(
        D_METHOD("commit_set_blueprint", "blueprint", "undo_redo"),
        &ParametricCharacterNode3D::commit_set_blueprint,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
    ClassDB::bind_method(
        D_METHOD("commit_set_primary_apparel", "apparel", "undo_redo"),
        &ParametricCharacterNode3D::commit_set_primary_apparel,
        DEFVAL(Ref<IdeamUndoRedo>())
    );
}

void ParametricCharacterNode3D::set_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint) {
    blueprint_ = p_blueprint;
    if (auto_rebuild_meshes_ && is_inside_tree()) {
        rebuild_character_meshes();
    }
}

void ParametricCharacterNode3D::set_primary_apparel(const godot::Ref<CharacterApparelResource> &p_apparel) {
    primary_apparel_ = p_apparel;
    if (auto_rebuild_meshes_ && is_inside_tree()) {
        rebuild_character_meshes();
    }
}

void ParametricCharacterNode3D::set_auto_rebuild_meshes(bool p_auto) {
    auto_rebuild_meshes_ = p_auto;
}

void ParametricCharacterNode3D::apply_xr_trigger_grip_morphs(float p_trigger, float p_grip, float p_thumb) {
    if (blueprint_.is_null()) return;
    const float trig  = std::clamp(p_trigger, 0.0f, 1.0f);
    const float grip  = std::clamp(p_grip, 0.0f, 1.0f);
    const float thumb = (p_thumb >= 0.0f) ? std::clamp(p_thumb, 0.0f, 1.0f) : grip;

    blueprint_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_0), thumb);
    blueprint_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_1), trig);
    blueprint_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_2), grip);
    blueprint_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_3), grip);
    blueprint_->set_morph_channel_weight(static_cast<int>(core::MORPH_DIGIT_CURL_4), grip);

    if (auto_rebuild_meshes_ && is_inside_tree()) {
        rebuild_character_meshes();
    }
}

void ParametricCharacterNode3D::rebuild_character_meshes() {
    using namespace godot;
    for (int i = get_child_count() - 1; i >= 0; --i) {
        Node *child = get_child(i);
        if (child && child->has_meta("_ideam_generated_character_mesh")) {
            remove_child(child);
            memdelete(child);
        }
    }
    if (blueprint_.is_null()) return;

    MeshInstance3D *body_inst = memnew(MeshInstance3D);
    body_inst->set_meta("_ideam_generated_character_mesh", true);
    body_inst->set_name("_IdeamCharacterBodyMesh");
    body_inst->set_mesh(blueprint_->build_evaluated_array_mesh());
    add_child(body_inst);

    if (primary_apparel_.is_valid()) {
        MeshInstance3D *app_inst = memnew(MeshInstance3D);
        app_inst->set_meta("_ideam_generated_character_mesh", true);
        app_inst->set_name("_IdeamCharacterApparelMesh");
        app_inst->set_mesh(primary_apparel_->build_refitted_apparel_mesh(blueprint_));
        add_child(app_inst);
    }
}

godot::Dictionary ParametricCharacterNode3D::get_character_telemetry_summary() const {
    godot::Dictionary d;
    if (blueprint_.is_valid()) {
        d["body_archetype"]        = blueprint_->get_body_archetype();
        d["subdivision_lod_level"] = blueprint_->get_subdivision_lod_level();
        d["base_vertex_count"]     = blueprint_->get_base_vertex_count();
        d["base_quad_count"]       = blueprint_->get_base_quad_count();
        d["morph_delta_count"]     = blueprint_->get_sparse_morph_delta_count();
    }
    if (primary_apparel_.is_valid()) {
        d["apparel_normal_offset"]  = primary_apparel_->get_normal_offset();
        d["apparel_xpbd_iters"]     = primary_apparel_->get_xpbd_relax_iterations();
    }
    return d;
}

void ParametricCharacterNode3D::commit_set_blueprint(
    const godot::Ref<CharacterBlueprintResource> &p_blueprint,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const godot::Ref<CharacterBlueprintResource> old_bp = blueprint_;
        ur->create_action("Set ParametricCharacterNode3D Blueprint");
        ur->add_do_property(this, "blueprint", p_blueprint);
        ur->add_undo_property(this, "blueprint", old_bp);
        ur->commit_action();
    } else {
        set_blueprint(p_blueprint);
    }
}

void ParametricCharacterNode3D::commit_set_primary_apparel(
    const godot::Ref<CharacterApparelResource> &p_apparel,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamCharactersPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const godot::Ref<CharacterApparelResource> old_app = primary_apparel_;
        ur->create_action("Set ParametricCharacterNode3D Primary Apparel");
        ur->add_do_property(this, "primary_apparel", p_apparel);
        ur->add_undo_property(this, "primary_apparel", old_app);
        ur->commit_action();
    } else {
        set_primary_apparel(p_apparel);
    }
}

} // namespace ideam::godot_ext
