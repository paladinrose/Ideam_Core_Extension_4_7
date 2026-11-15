#pragma once

/**
 * character_blueprint_resource.h
 * `CharacterBlueprintResource`, `CharacterApparelResource`, and `ParametricCharacterNode3D`
 * (`src/godot/characters/`).
 *
 * Implements Tier 1 Specific Features for `IdeamCharacters`:
 *   - Quad-loop parametric character topology (`Biped`, `Quadruped`, `Avian`, `FlyingEye`,
 *     `Serpentine`, `Custom`) with progressive LOD (`0..3`) and anatomical partition masks.
 *   - 12 canonical sparse morph slider channels (including `MORPH_DIGIT_CURL_0..4` for
 *     direct OpenXR trigger/grip binding).
 *   - Curvilinear `(quad_id, u, v, d_normal)` surface garment refitting & XPBD relaxation.
 *   - `IdeamLinguistics` `SemanticTagSetResource` attachment and `IdeamUndoRedo` `commit_*`
 *     reversible mutation wrappers.
 */

#include "../../core/characters/parametric_character_mesh.h"
#include "../linguistics/sememe_resource.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <vector>

namespace ideam::godot_ext {

class CharacterBlueprintResource : public godot::Resource {
    GDCLASS(CharacterBlueprintResource, godot::Resource)

private:
    core::ParametricCharacterMesh core_mesh_;
    int subdivision_lod_level_   = 1;
    uint32_t active_partition_mask_ = core::PARTITION_MASK_ALL;
    godot::PackedFloat32Array morph_weights_;
    godot::Ref<SemanticTagSetResource> semantic_tags_;

    void _ensure_morph_weights_size();

protected:
    static void _bind_methods();

public:
    CharacterBlueprintResource();
    ~CharacterBlueprintResource() override = default;

    [[nodiscard]] const core::ParametricCharacterMesh &get_core_mesh() const noexcept { return core_mesh_; }
    [[nodiscard]] core::ParametricCharacterMesh &get_core_mesh_mut() noexcept { return core_mesh_; }

    void set_body_archetype(int p_archetype);
    [[nodiscard]] int get_body_archetype() const noexcept { return static_cast<int>(core_mesh_.archetype()); }

    void set_subdivision_lod_level(int p_lod);
    [[nodiscard]] int get_subdivision_lod_level() const noexcept { return subdivision_lod_level_; }

    void set_active_partition_mask(int p_mask);
    [[nodiscard]] int get_active_partition_mask() const noexcept { return static_cast<int>(active_partition_mask_); }

    void set_morph_weights(const godot::PackedFloat32Array &p_weights);
    [[nodiscard]] godot::PackedFloat32Array get_morph_weights() const { return morph_weights_; }

    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags);
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }

    // --- Anatomical Morph Channel Helpers ---
    [[nodiscard]] int get_morph_channel_count() const noexcept {
        return static_cast<int>(core::MORPH_DEFAULT_CHANNEL_COUNT);
    }
    [[nodiscard]] godot::String get_morph_channel_name(int p_channel) const;
    [[nodiscard]] godot::PackedStringArray get_all_morph_channel_names() const;

    void set_morph_channel_weight(int p_channel, float p_weight);
    [[nodiscard]] float get_morph_channel_weight(int p_channel) const;
    void reset_all_morph_weights();

    // --- Telemetry & Mesh / Garment Evaluation ---
    [[nodiscard]] int get_base_vertex_count() const noexcept { return static_cast<int>(core_mesh_.vertex_count()); }
    [[nodiscard]] int get_base_quad_count() const noexcept { return static_cast<int>(core_mesh_.quad_count()); }
    [[nodiscard]] int get_sparse_morph_delta_count() const noexcept { return static_cast<int>(core_mesh_.morph_delta_count()); }

    void evaluate_deformed_buffers(
        std::vector<glm::vec3> &r_positions,
        std::vector<glm::vec3> &r_normals,
        std::vector<uint32_t> *r_active_masks = nullptr
    ) const;

    [[nodiscard]] godot::PackedVector3Array get_deformed_vertex_positions() const;
    [[nodiscard]] godot::PackedVector3Array get_deformed_vertex_normals() const;
    [[nodiscard]] godot::Vector3 evaluate_garment_point(int p_quad_id, float p_u, float p_v, float p_d_normal = 0.015f) const;
    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_evaluated_array_mesh() const;

    // --- Tier 1 `IdeamUndoRedo` Reversible Mutation API ---
    void commit_set_body_archetype(
        int p_archetype,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_subdivision_lod_level(
        int p_lod,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_active_partition_mask(
        int p_mask,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_morph_weights(
        const godot::PackedFloat32Array &p_weights,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_morph_channel_weight(
        int p_channel,
        float p_weight,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_semantic_tags(
        const godot::Ref<SemanticTagSetResource> &p_tags,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
};

class CharacterApparelResource : public godot::Resource {
    GDCLASS(CharacterApparelResource, godot::Resource)

private:
    real_t normal_offset_         = static_cast<real_t>(0.015);
    int xpbd_relax_iterations_    = 3;
    float xpbd_compliance_        = 0.0005f;
    uint32_t target_partition_mask_ = (1u << core::PARTITION_TORSO) | (1u << core::PARTITION_PELVIS);
    int substeps_per_quad_axis_   = 2;
    godot::Ref<SemanticTagSetResource> semantic_tags_;
    std::vector<core::SurfaceGarmentBindPod> garment_bindings_;

protected:
    static void _bind_methods();

public:
    CharacterApparelResource() = default;
    ~CharacterApparelResource() override = default;

    void set_normal_offset(real_t p_offset);
    [[nodiscard]] real_t get_normal_offset() const noexcept { return normal_offset_; }

    void set_xpbd_relax_iterations(int p_iters);
    [[nodiscard]] int get_xpbd_relax_iterations() const noexcept { return xpbd_relax_iterations_; }

    void set_xpbd_compliance(float p_compliance);
    [[nodiscard]] float get_xpbd_compliance() const noexcept { return xpbd_compliance_; }

    void set_target_partition_mask(int p_mask);
    [[nodiscard]] int get_target_partition_mask() const noexcept { return static_cast<int>(target_partition_mask_); }

    void set_substeps_per_quad_axis(int p_steps);
    [[nodiscard]] int get_substeps_per_quad_axis() const noexcept { return substeps_per_quad_axis_; }

    void set_semantic_tags(const godot::Ref<SemanticTagSetResource> &p_tags);
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tags() const { return semantic_tags_; }

    [[nodiscard]] const std::vector<core::SurfaceGarmentBindPod> &get_garment_bindings() const noexcept { return garment_bindings_; }
    [[nodiscard]] int get_garment_vertex_count() const noexcept { return static_cast<int>(garment_bindings_.size()); }

    void generate_default_bindings_for_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint);
    [[nodiscard]] godot::PackedVector3Array evaluate_refitted_positions(
        const godot::Ref<CharacterBlueprintResource> &p_blueprint
    ) const;
    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_refitted_apparel_mesh(
        const godot::Ref<CharacterBlueprintResource> &p_blueprint
    ) const;

    // --- Tier 1 `IdeamUndoRedo` Reversible Mutation API ---
    void commit_set_normal_offset(
        real_t p_offset,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_xpbd_relax_iterations(
        int p_iters,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_target_partition_mask(
        int p_mask,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
};

class ParametricCharacterNode3D : public godot::Node3D {
    GDCLASS(ParametricCharacterNode3D, godot::Node3D)

private:
    godot::Ref<CharacterBlueprintResource> blueprint_;
    godot::Ref<CharacterApparelResource> primary_apparel_;
    bool auto_rebuild_meshes_ = true;

protected:
    static void _bind_methods();
    void _notification(int p_what);

public:
    ParametricCharacterNode3D() = default;
    ~ParametricCharacterNode3D() override = default;

    void set_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint);
    [[nodiscard]] godot::Ref<CharacterBlueprintResource> get_blueprint() const { return blueprint_; }

    void set_primary_apparel(const godot::Ref<CharacterApparelResource> &p_apparel);
    [[nodiscard]] godot::Ref<CharacterApparelResource> get_primary_apparel() const { return primary_apparel_; }

    void set_auto_rebuild_meshes(bool p_auto);
    [[nodiscard]] bool get_auto_rebuild_meshes() const noexcept { return auto_rebuild_meshes_; }

    /**
     * Bridges OpenXR controller analog trigger & grip floats (`GameplayControlMap` / Section 2.4)
     * directly to `MORPH_DIGIT_CURL_0..4` on the active `CharacterBlueprintResource`.
     */
    void apply_xr_trigger_grip_morphs(float p_trigger, float p_grip, float p_thumb = -1.0f);

    void rebuild_character_meshes();
    [[nodiscard]] godot::Dictionary get_character_telemetry_summary() const;

    // --- Tier 1 `IdeamUndoRedo` Reversible Mutation API ---
    void commit_set_blueprint(
        const godot::Ref<CharacterBlueprintResource> &p_blueprint,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
    void commit_set_primary_apparel(
        const godot::Ref<CharacterApparelResource> &p_apparel,
        const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>()
    );
};

} // namespace ideam::godot_ext
