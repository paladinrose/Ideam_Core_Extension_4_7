#pragma once

/**
 * characters_dod.h
 * DOD Orchestrator for `IdeamCharacters` (`src/core/tasks/domains/characters/`).
 */

#include "characters_dod_common.h"
#include "transform_logic/quad_loop_subdivision_and_lod_transform_logic.h"
#include "transform_logic/sparse_morph_slider_evaluate_transform_logic.h"
#include "transform_logic/surface_garment_refit_transform_logic.h"
#include <vector>

namespace ideam::core {

class CharactersDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

    CharactersBufferCapacities capacities_{};
    CharactersBufferManifest   manifest_{};

    ParametricCharacterMesh character_mesh_{};
    std::vector<float> morph_weights_;
    std::vector<SurfaceGarmentBindPod> garment_bindings_;

    uint32_t target_lod_level_        = 1u;
    uint32_t active_partition_mask_   = PARTITION_MASK_ALL;
    float    extra_garment_offset_    = 0.0f;
    uint32_t xpbd_relax_iterations_   = 3u;
    float    xpbd_compliance_         = 0.0005f;

public:
    CharactersDOD();
    ~CharactersDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }

    [[nodiscard]] ParametricCharacterMesh &character_mesh() noexcept { return character_mesh_; }
    [[nodiscard]] const ParametricCharacterMesh &character_mesh() const noexcept { return character_mesh_; }

    void set_capacities(const CharactersBufferCapacities &p_caps) noexcept { capacities_ = p_caps; }
    [[nodiscard]] const CharactersBufferCapacities &get_capacities() const noexcept { return capacities_; }

    [[nodiscard]] const CharactersBufferManifest &get_manifest() const noexcept { return manifest_; }
    [[nodiscard]] CharactersBufferManifest &get_manifest_mut() noexcept { return manifest_; }

    void set_target_lod_level(uint32_t p_lod) noexcept { target_lod_level_ = std::min<uint32_t>(p_lod, 3u); }
    [[nodiscard]] uint32_t get_target_lod_level() const noexcept { return target_lod_level_; }

    void set_active_partition_mask(uint32_t p_mask) noexcept { active_partition_mask_ = p_mask; }
    [[nodiscard]] uint32_t get_active_partition_mask() const noexcept { return active_partition_mask_; }

    void set_extra_garment_offset(float p_offset) noexcept { extra_garment_offset_ = p_offset; }
    [[nodiscard]] float get_extra_garment_offset() const noexcept { return extra_garment_offset_; }

    void set_xpbd_relax_iterations(uint32_t p_iters) noexcept { xpbd_relax_iterations_ = std::min<uint32_t>(p_iters, 16u); }
    [[nodiscard]] uint32_t get_xpbd_relax_iterations() const noexcept { return xpbd_relax_iterations_; }

    void set_xpbd_compliance(float p_comp) noexcept { xpbd_compliance_ = std::max(0.0f, p_comp); }
    [[nodiscard]] float get_xpbd_compliance() const noexcept { return xpbd_compliance_; }

    void set_morph_weights(std::span<const float> p_weights);
    void set_morph_channel_weight(uint32_t p_channel, float p_weight);
    [[nodiscard]] const std::vector<float> &morph_weights() const noexcept { return morph_weights_; }

    void set_garment_bindings(std::span<const SurfaceGarmentBindPod> p_binds);
    [[nodiscard]] const std::vector<SurfaceGarmentBindPod> &garment_bindings() const noexcept { return garment_bindings_; }

    // --- Buffer Initialization & Staging ---
    bool initialize_characters_memory(MemoryManagerDOD *p_mem);
    [[nodiscard]] uint32_t get_buffer_id(CharactersBufferSlot p_slot) const noexcept {
        return manifest_.get(p_slot);
    }

    bool stage_character_to_dod(
        const ParametricCharacterMesh &p_mesh,
        std::span<const float> p_morph_weights,
        std::span<const SurfaceGarmentBindPod> p_garment_binds
    );
    bool stage_character_to_dod();

    // --- Direct Synchronous Execution (`CHR1 -> CHR2 -> CHR3`) ---
    bool execute_full_pipeline_direct(uint32_t p_stage_mask = CHARACTERS_STAGE_DEFAULT_PIPELINE_MASK);

    // --- Readback Helpers ---
    void read_deformed_positions(std::vector<glm::vec3> &r_out) const;
    void read_deformed_normals(std::vector<glm::vec3> &r_out) const;
    void read_garment_positions(std::vector<glm::vec3> &r_out) const;
    void read_active_lod_masks(std::vector<uint32_t> &r_out) const;
};

} // namespace ideam::core
