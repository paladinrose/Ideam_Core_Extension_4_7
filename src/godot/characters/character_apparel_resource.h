#pragma once

/**
 * character_apparel_resource.h
 * Curvilinear surface-bound garment refitting resource (`CharacterApparelResource`)
 * wired to `LoomisCharacter3DResource` (`src/godot/characters/`).
 */

#include "loomis_character_3d_resource.h"
#include "../../core/characters/parametric_character_mesh.h"
#include "../linguistics/sememe_resource.h"
#include "../utilities/ideam_undo_redo.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <vector>

namespace ideam::godot_ext {

class CharacterApparelResource : public godot::Resource {
    GDCLASS(CharacterApparelResource, godot::Resource)

private:
    real_t normal_offset_           = static_cast<real_t>(0.015);
    int xpbd_relax_iterations_      = 3;
    float xpbd_compliance_          = 0.0005f;
    uint32_t target_partition_mask_ = (1u << core::PARTITION_TORSO) | (1u << core::PARTITION_PELVIS);
    int substeps_per_quad_axis_     = 2;
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

    void generate_default_bindings_for_character(const godot::Ref<LoomisCharacter3DResource> &p_character);
    [[nodiscard]] godot::PackedVector3Array evaluate_refitted_positions(
        const godot::Ref<LoomisCharacter3DResource> &p_character
    ) const;
    [[nodiscard]] godot::Ref<godot::ArrayMesh> build_refitted_apparel_mesh(
        const godot::Ref<LoomisCharacter3DResource> &p_character
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

} // namespace ideam::godot_ext
