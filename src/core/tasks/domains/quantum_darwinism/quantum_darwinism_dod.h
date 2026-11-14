#pragma once

/**
 * quantum_darwinism_dod.h
 * DOD Orchestrator for `IdeamQuantumDarwinism` (`src/core/tasks/domains/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Implements Quantum Darwinism conflict stabilization on `TaskGraphDOD` over the shared
 *   `ProceduralEnvironment`:
 *     1. `ConflictHorizonBoxPartitionMetadataLogic` (`QD1`): Draws the bounded environment
 *        box (`E_box`) around a WFC contradiction or cardinality conflict slot.
 *     2. `PointerStateEinselectionQueryLogic` (`QD2`): Evaluates `GenerationRule` scores
 *        and neighbor mutual-information redundancy ($R_\delta$) to select settled Pointer States.
 *     3. `CoherenceCascadeRelaxAndPropagateTransformLogic` (`QD3`): Evaluates which
 *        lower-precedence CSS cascade properties/siblings on intermediate cells between
 *        each Pointer State and the problem cell can be altered without losing coherence,
 *        then propagates pointer properties inward to resolve the conflict.
 */

#include "quantum_darwinism_dod_common.h"
#include "metadata_logic/conflict_horizon_box_partition_metadata_logic.h"
#include "query_logic/pointer_state_einselection_query_logic.h"
#include "transform_logic/coherence_cascade_relax_and_propagate_transform_logic.h"
#include <span>
#include <vector>

namespace ideam::core {

class QuantumDarwinismDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

    QuantumDarwinismBufferCapacities capacities_{};
    QuantumDarwinismBufferManifest   manifest_{};
    QuantumDarwinismPipelineTopology topology_{};
    QuantumDarwinismNodeExecutionConfig exec_config_{};

    ProceduralEnvironment environment_{};
    QuantumDarwinismSolver solver_{};

public:
    QuantumDarwinismDOD() = default;
    ~QuantumDarwinismDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }

    [[nodiscard]] ProceduralEnvironment &environment() noexcept { return environment_; }
    [[nodiscard]] const ProceduralEnvironment &environment() const noexcept { return environment_; }

    [[nodiscard]] QuantumDarwinismSolver &solver() noexcept { return solver_; }
    [[nodiscard]] const QuantumDarwinismSolver &solver() const noexcept { return solver_; }

    void set_capacities(const QuantumDarwinismBufferCapacities &p_caps) noexcept { capacities_ = p_caps; }
    [[nodiscard]] const QuantumDarwinismBufferCapacities &get_capacities() const noexcept { return capacities_; }

    [[nodiscard]] const QuantumDarwinismBufferManifest &get_manifest() const noexcept { return manifest_; }
    [[nodiscard]] QuantumDarwinismBufferManifest &get_manifest_mut() noexcept { return manifest_; }

    void set_horizon_config(const HorizonBoxConfigPOD &p_cfg) noexcept { solver_.set_horizon_config(p_cfg); }
    [[nodiscard]] const HorizonBoxConfigPOD &get_horizon_config() const noexcept { return solver_.horizon_config(); }

    // --- Buffer Initialization & Staging ---
    bool initialize_quantum_darwinism_memory(MemoryManagerDOD *p_mem);
    [[nodiscard]] uint32_t get_buffer_id(QuantumDarwinismBufferSlot p_slot) const noexcept {
        return manifest_.get(p_slot);
    }

    bool stage_environment_to_dod(const ProceduralEnvironment &p_env);
    bool stage_environment_to_dod() { return stage_environment_to_dod(environment_); }

    // --- Direct Synchronous Execution (`QD1` -> `QD2` -> `QD3`) ---
    bool execute_full_pipeline_direct(uint32_t p_stage_mask = QD_STAGE_DEFAULT_PIPELINE_MASK);

    // --- Readback & Sync Helpers ---
    void read_environment_slots(std::vector<EnvironmentSlotPOD> &r_out) const;
    void read_horizon_partitions(std::vector<HorizonSlotPartitionPOD> &r_out) const;
    void read_pointer_state_records(std::vector<PointerStateRecordPOD> &r_out) const;
    void read_coherence_bridge_slots(std::vector<CoherenceBridgeSlotPOD> &r_out) const;
    void read_pointer_or_conflict_masks(std::vector<uint32_t> &r_out) const;
    void sync_dod_slots_back_to_environment();
};

} // namespace ideam::core
