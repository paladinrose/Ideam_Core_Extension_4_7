#pragma once

/**
 * procedural_generation_dod.h
 * DOD Orchestrator for `IdeamProceduralGeneration` (`src/core/tasks/domains/procedural_generation/`).
 */

#include "procedural_generation_dod_common.h"
#include "metadata_logic/cascade_rule_compile_metadata_logic.h"
#include "query_logic/reflection_selector_match_query_logic.h"
#include "transform_logic/pattern_domain_initialize_transform_logic.h"
#include "transform_logic/hierarchy_cardinality_propagate_transform_logic.h"
#include <span>
#include <vector>

namespace ideam::core {

class ProceduralGenerationDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

    ProceduralGenerationBufferCapacities capacities_{};
    ProceduralGenerationBufferManifest   manifest_{};
    ProceduralGenerationPipelineTopology topology_{};
    ProceduralGenerationNodeExecutionConfig exec_config_{};

    ProceduralEnvironment environment_{};
    float pointer_redundancy_threshold_ = 0.75f;
    uint32_t max_propagation_sweeps_    = 16u;

public:
    ProceduralGenerationDOD() = default;
    ~ProceduralGenerationDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }

    [[nodiscard]] ProceduralEnvironment &environment() noexcept { return environment_; }
    [[nodiscard]] const ProceduralEnvironment &environment() const noexcept { return environment_; }

    void set_capacities(const ProceduralGenerationBufferCapacities &p_caps) noexcept { capacities_ = p_caps; }
    [[nodiscard]] const ProceduralGenerationBufferCapacities &get_capacities() const noexcept { return capacities_; }

    [[nodiscard]] const ProceduralGenerationBufferManifest &get_manifest() const noexcept { return manifest_; }
    [[nodiscard]] ProceduralGenerationBufferManifest &get_manifest_mut() noexcept { return manifest_; }

    void set_pointer_redundancy_threshold(float p_thresh) noexcept { pointer_redundancy_threshold_ = p_thresh; }
    [[nodiscard]] float get_pointer_redundancy_threshold() const noexcept { return pointer_redundancy_threshold_; }

    void set_max_propagation_sweeps(uint32_t p_sweeps) noexcept { max_propagation_sweeps_ = p_sweeps; }
    [[nodiscard]] uint32_t get_max_propagation_sweeps() const noexcept { return max_propagation_sweeps_; }

    // --- Buffer Initialization & Staging ---
    bool initialize_procedural_generation_memory(MemoryManagerDOD *p_mem);
    [[nodiscard]] uint32_t get_buffer_id(ProceduralGenerationBufferSlot p_slot) const noexcept {
        return manifest_.get(p_slot);
    }

    bool stage_environment_to_dod(const ProceduralEnvironment &p_env);
    bool stage_environment_to_dod() { return stage_environment_to_dod(environment_); }

    // --- Direct Synchronous Execution ---
    bool execute_full_pipeline_direct(uint32_t p_stage_mask = PROCGEN_STAGE_DEFAULT_PIPELINE_MASK);

    // --- Readback Helpers ---
    void read_environment_slots(std::vector<EnvironmentSlotPOD> &r_out) const;
    void read_pointer_or_conflict_masks(std::vector<uint32_t> &r_out) const;
    void read_slot_entropies(std::vector<float> &r_out) const;
    void sync_dod_slots_back_to_environment();
};

} // namespace ideam::core
