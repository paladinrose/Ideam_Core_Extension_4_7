#pragma once

/**
 * quantum_darwinism_dod.h
 * DOD Orchestrator for `IdeamQuantumDarwinism` (`src/core/tasks/domains/quantum_darwinism/`).
 *
 * Implements both:
 *   - Conflict Stabilization (`QD1` -> `QD2` -> `QD3`)
 *   - Standalone Outward Multi-Channel Generative Einselection (`QD4` -> `QD5`)
 *   - Hybrid Generative + Inward Coherence Repair (`QD4` -> `QD5` -> `QD1` -> `QD2` -> `QD3`)
 */

#include "quantum_darwinism_dod_common.h"
#include "metadata_logic/conflict_horizon_box_partition_metadata_logic.h"
#include "query_logic/pointer_state_einselection_query_logic.h"
#include "transform_logic/coherence_cascade_relax_and_propagate_transform_logic.h"
#include "transform_logic/outward_wavefront_einselection_transform_logic.h"
#include "transform_logic/channel_to_token_projection_transform_logic.h"
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

    void set_generative_config(const GenerativeWavefrontConfigPOD &p_cfg) noexcept { solver_.set_generative_config(p_cfg); }
    [[nodiscard]] const GenerativeWavefrontConfigPOD &get_generative_config() const noexcept { return solver_.generative_config(); }

    // --- Buffer Initialization & Staging ---
    bool initialize_quantum_darwinism_memory(MemoryManagerDOD *p_mem);
    [[nodiscard]] uint32_t get_buffer_id(QuantumDarwinismBufferSlot p_slot) const noexcept {
        return manifest_.get(p_slot);
    }

    bool stage_environment_to_dod(const ProceduralEnvironment &p_env);
    bool stage_environment_to_dod() { return stage_environment_to_dod(environment_); }
    bool stage_channels_and_couplings_to_dod();
    bool stage_perturbation_field_to_dod(std::span<const float> p_field);

    // --- Direct Synchronous Execution (`QD1`–`QD5`) ---
    bool execute_full_pipeline_direct(uint32_t p_stage_mask = QD_STAGE_DEFAULT_PIPELINE_MASK);
    bool execute_generative_pipeline_direct();

    // --- Readback & Sync Helpers ---
    void read_environment_slots(std::vector<EnvironmentSlotPOD> &r_out) const;
    void read_horizon_partitions(std::vector<HorizonSlotPartitionPOD> &r_out) const;
    void read_pointer_state_records(std::vector<PointerStateRecordPOD> &r_out) const;
    void read_coherence_bridge_slots(std::vector<CoherenceBridgeSlotPOD> &r_out) const;
    void read_slot_channel_states(std::vector<SlotChannelStatePOD> &r_out) const;
    void read_pointer_or_conflict_masks(std::vector<uint32_t> &r_out) const;
    void sync_dod_slots_back_to_environment();
};

} // namespace ideam::core
