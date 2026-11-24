#pragma once

/**
 * loomis_character_3d_resource.h
 * Unified 3D Character Resource (`LoomisCharacter3DResource`) for `IdeamCharacters`.
 *
 * Consolidates the 4-layer Andrew Loomis character stack (`BodyPlanResource` ->
 * `Scaffold` -> `Topology` -> `Features` -> non-destructive `Details` override stack)
 * with the parametric quad-loop morph & garment refitting core (`ParametricCharacterMesh`):
 *   - Auto-initializes a default Loomis Humanoid `BodyPlanResource` on creation.
 *   - Bounded generator parameters (`parameter_values`), randomization, blending,
 *     cross-layer `CharacterDetailOverrideStack`, and separable per-part `ArrayMesh` outputs.
 *   - 12 canonical sparse morph channels (including `MORPH_DIGIT_CURL_0..4` for OpenXR
 *     trigger/grip binding), anatomical partition masks, curvilinear garment evaluation,
 *     and `IdeamLinguistics` `SemanticTagSetResource` tagging.
 */

#include "body_plan_resource.h"
#include "../../core/characters/parametric_character_mesh.h"
#include "../linguistics/sememe_resource.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <vector>

namespace ideam::godot_ext {

class LoomisCharacter3DResource : public godot::Resource {
    GDCLASS(LoomisCharacter3DResource, godot::Resource)

private:
    // --- 4-Layer Loomis Stack ---
    godot::Ref<BodyPlanResource> body_plan_;
    core::CharacterInstanceCore core_instance_;
    godot::Color base_color_ = godot::Color(0.86f, 0.78f, 0.70f, 1.0f);

    // --- Parametric Quad-Loop Morph & Garment Core (DOD / XR / Apparel Bridge) ---
    core::ParametricCharacterMesh core_mesh_;
    int subdivision_lod_level_      = 1;
    uint32_t active_partition_mask_ = core::PARTITION_MASK_ALL;
    godot::PackedFloat32Array morph_weights_;
    godot::Ref<SemanticTagSetResource> semantic_tag_set_;

    // --- Shared Tooling & Tutorials Metadata ---
    godot::Ref<IdeamUndoRedo> undo_redo_;
    godot::StringName help_topic_id_;
    godot::PackedStringArray semantic_tags_;

    void _on_body_plan_changed();
    void _ensure_morph_weights_size();
    [[nodiscard]] godot::Ref<godot::ArrayMesh> _quad_mesh_to_array_mesh(
        const core::SurfaceQuadMesh3D<float> &p_qmesh
    ) const;

protected:
    static void _bind_methods();

public:
    LoomisCharacter3DResource();
    ~LoomisCharacter3DResource() override;

    // --- Direct C++ Core Accessors ---
    [[nodiscard]] const core::CharacterInstanceCore &get_core_instance() const noexcept { return core_instance_; }
    [[nodiscard]] core::CharacterInstanceCore &get_core_instance_mut() noexcept { return core_instance_; }
    [[nodiscard]] const core::ParametricCharacterMesh &get_core_mesh() const noexcept { return core_mesh_; }
    [[nodiscard]] core::ParametricCharacterMesh &get_core_mesh_mut() noexcept { return core_mesh_; }

    // --- 4-Layer Loomis Plan & Generator Properties ---
    void ensure_default_body_plan();

    void set_body_plan(const godot::Ref<BodyPlanResource> &p_plan);
    [[nodiscard]] godot::Ref<BodyPlanResource> get_body_plan() const { return body_plan_; }

    void set_character_name(const godot::String &p_name);
    [[nodiscard]] godot::String get_character_name() const;

    void set_parameter_values(const godot::Dictionary &p_values);
    [[nodiscard]] godot::Dictionary get_parameter_values() const;

    void set_detail_deltas(const godot::TypedArray<godot::Dictionary> &p_deltas);
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_detail_deltas() const;

    void set_details_enabled(bool p_enabled);
    [[nodiscard]] bool get_details_enabled() const noexcept { return core_instance_.get_details_enabled(); }

    void set_base_color(const godot::Color &p_color);
    [[nodiscard]] godot::Color get_base_color() const noexcept { return base_color_; }

    // --- Parametric Archetype, LOD, Partition Mask & Sparse Morph Channels ---
    void set_body_archetype(int p_archetype);
    [[nodiscard]] int get_body_archetype() const noexcept { return static_cast<int>(core_mesh_.archetype()); }

    void set_subdivision_lod_level(int p_lod);
    [[nodiscard]] int get_subdivision_lod_level() const noexcept { return subdivision_lod_level_; }

    void set_active_partition_mask(int p_mask);
    [[nodiscard]] int get_active_partition_mask() const noexcept { return static_cast<int>(active_partition_mask_); }

    void set_morph_weights(const godot::PackedFloat32Array &p_weights);
    [[nodiscard]] godot::PackedFloat32Array get_morph_weights() const { return morph_weights_; }

    void set_semantic_tag_set(const godot::Ref<SemanticTagSetResource> &p_tags);
    [[nodiscard]] godot::Ref<SemanticTagSetResource> get_semantic_tag_set() const { return semantic_tag_set_; }

    [[nodiscard]] int get_morph_channel_count() const noexcept {
        return static_cast<int>(core::MORPH_DEFAULT_CHANNEL_COUNT);
    }
    [[nodiscard]] godot::String get_morph_channel_name(int p_channel) const;
    [[nodiscard]] godot::PackedStringArray get_all_morph_channel_names() const;

    void set_morph_channel_weight(int p_channel, float p_weight);
    [[nodiscard]] float get_morph_channel_weight(int p_channel) const;
    void reset_all_morph_weights();

    // --- Tooling, Undo/Redo & Tutorials Metadata ---
    void set_undo_redo(const godot::Ref<IdeamUndoRedo> &p_ur) { undo_redo_ = p_ur; }
    [[nodiscard]] godot::Ref<IdeamUndoRedo> get_undo_redo() const { return undo_redo_; }

    void set_help_topic_id(const godot::StringName &p_topic) { help_topic_id_ = p_topic; }
    [[nodiscard]] godot::StringName get_help_topic_id() const;

    void set_semantic_tags(const godot::PackedStringArray &p_tags);
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }

    // --- Direct & Undoable Generator API ---
    void set_parameter_value(const godot::String &p_name, real_t p_value);
    void commit_set_parameter_value(const godot::String &p_name, real_t p_value);
    [[nodiscard]] real_t get_parameter_value(const godot::String &p_name) const;

    // --- Reversible Morph/Archetype Commit API ---
    void commit_set_body_archetype(int p_archetype, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());
    void commit_set_subdivision_lod_level(int p_lod, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());
    void commit_set_active_partition_mask(int p_mask, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());
    void commit_set_morph_weights(const godot::PackedFloat32Array &p_weights, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());
    void commit_set_morph_channel_weight(int p_channel, float p_weight, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());
    void commit_set_semantic_tag_set(const godot::Ref<SemanticTagSetResource> &p_tags, const godot::Ref<IdeamUndoRedo> &p_undo_redo = godot::Ref<IdeamUndoRedo>());

    // --- Cross-Layer Non-Destructive Detail Override API (Tool 4) ---
    void set_scaffold_override(
        int p_scaffold_node_id,
        const godot::Vector3 &p_position_offset,
        const godot::Vector3 &p_scale_multiplier = godot::Vector3(1.0f, 1.0f, 1.0f)
    );
    void set_topology_override(int p_landmark_id, const godot::Vector3 &p_offset);
    void set_feature_override(
        int p_feature_id,
        const godot::Vector3 &p_offset,
        real_t p_scale_multiplier = static_cast<real_t>(1.0)
    );
    int add_detail_delta(
        const godot::String &p_label,
        int p_scaffold_node_id,
        real_t p_u,
        real_t p_v,
        const godot::Vector3 &p_local_delta_ntuv,
        real_t p_radius = 0.05f,
        bool p_is_crease = false
    );
    void clear_detail_deltas();
    void clear_all_detail_overrides();
    [[nodiscard]] int get_detail_override_count() const;

    void randomize_within_bounds(int64_t p_seed = 0);
    [[nodiscard]] godot::Ref<LoomisCharacter3DResource> blend_with(
        const godot::Ref<LoomisCharacter3DResource> &p_other,
        real_t p_alpha = 0.5f
    ) const;

    // --- 4-Layer Loomis Stack Evaluation Outputs ---
    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_array_mesh(int p_lod_level = 0) const;
    [[nodiscard]] godot::Dictionary build_part_array_meshes(int p_lod_level = 0) const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> get_evaluated_bridge_regimes() const;
    [[nodiscard]] godot::TypedArray<godot::Dictionary> build_skeleton_export_plan() const;
    [[nodiscard]] godot::Dictionary get_balance_report() const;

    // --- Parametric Morph & Garment Evaluation (DOD / Apparel Bridge) ---
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
};

} // namespace ideam::godot_ext
