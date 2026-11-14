#pragma once

/**
 * wfc_dod.h
 * DOD Orchestrator for `IdeamWaveFunctionCollapse` (`src/core/tasks/domains/wave_function_collapse/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Operates on the shared `ProceduralEnvironment` populated by `IdeamProceduralGeneration`.
 * - Supports zero-copy buffer sharing with `ProceduralGenerationDOD` (`share_buffers_from_manifest`),
 *   as well as standalone buffer allocation and staging.
 * - Runs minimum-entropy slot collapse (`WFC1` -> `WFC2` -> `WFC3`) and propagates both
 *   spatial port adjacency and cascading `GenerationRule` hierarchy constraints (`CastleGrounds -> CastleTower`,
 *   child cardinality `[min..max]`, and sibling dependencies like `TowerLanding requires TowerStairs`).
 * - Flags any contradiction slots in `POINTER_OR_CONFLICT_MASK` and populates
 *   `WFC_CONTRADICTION_REPORT_POD` so `IdeamQuantumDarwinism` TaskGraph nodes can stabilize
 *   the local environment horizon box without full-grid backtracking.
 */

#include "wfc_dod_common.h"
#include "metadata_logic/wfc_entropy_partition_metadata_logic.h"
#include "transform_logic/wfc_forward_collapse_and_propagate_transform_logic.h"
#include "query_logic/wfc_contradiction_detect_query_logic.h"
#include <span>
#include <vector>

namespace ideam::core {

class WaveFunctionCollapseDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

    WaveFunctionCollapseBufferCapacities capacities_{};
    WaveFunctionCollapseBufferManifest   manifest_{};
    WaveFunctionCollapsePipelineTopology topology_{};
    WaveFunctionCollapseNodeExecutionConfig exec_config_{};

    ProceduralEnvironment environment_{};
    WFCSolverCore solver_core_{};

    uint32_t max_collapse_steps_        = 64u;
    uint32_t master_seed_lo_            = 1337u;
    float pointer_redundancy_threshold_ = 0.75f;
    uint32_t horizon_radius_            = 4u;

public:
    WaveFunctionCollapseDOD() {
        solver_core_.bind_environment(&environment_);
    }
    ~WaveFunctionCollapseDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }

    [[nodiscard]] ProceduralEnvironment &environment() noexcept { return environment_; }
    [[nodiscard]] const ProceduralEnvironment &environment() const noexcept { return environment_; }

    [[nodiscard]] WFCSolverCore &solver_core() noexcept { return solver_core_; }
    [[nodiscard]] const WFCSolverCore &solver_core() const noexcept { return solver_core_; }

    void set_capacities(const WaveFunctionCollapseBufferCapacities &p_caps) noexcept { capacities_ = p_caps; }
    [[nodiscard]] const WaveFunctionCollapseBufferCapacities &get_capacities() const noexcept { return capacities_; }

    [[nodiscard]] const WaveFunctionCollapseBufferManifest &get_manifest() const noexcept { return manifest_; }
    [[nodiscard]] WaveFunctionCollapseBufferManifest &get_manifest_mut() noexcept { return manifest_; }

    void set_max_collapse_steps(uint32_t p_steps) noexcept { max_collapse_steps_ = p_steps; }
    [[nodiscard]] uint32_t get_max_collapse_steps() const noexcept { return max_collapse_steps_; }

    void set_master_seed(uint32_t p_seed) noexcept { master_seed_lo_ = p_seed; }
    [[nodiscard]] uint32_t get_master_seed() const noexcept { return master_seed_lo_; }

    void set_pointer_redundancy_threshold(float p_thresh) noexcept { pointer_redundancy_threshold_ = p_thresh; }
    [[nodiscard]] float get_pointer_redundancy_threshold() const noexcept { return pointer_redundancy_threshold_; }

    void set_horizon_radius(uint32_t p_radius) noexcept { horizon_radius_ = p_radius; }
    [[nodiscard]] uint32_t get_horizon_radius() const noexcept { return horizon_radius_; }

    // --- Buffer Initialization, Sharing & Staging ---
    bool initialize_wfc_memory(MemoryManagerDOD *p_mem);
    [[nodiscard]] uint32_t get_buffer_id(WaveFunctionCollapseBufferSlot p_slot) const noexcept {
        return manifest_.get(p_slot);
    }

    bool stage_environment_and_rules_to_dod(
        const ProceduralEnvironment &p_env,
        std::span<const WFCRuleEntryPOD> p_wfc_rules = {}
    );
    bool stage_environment_and_rules_to_dod() {
        return stage_environment_and_rules_to_dod(environment_, solver_core_.rule_tensor());
    }

    // --- Direct Synchronous Execution (`WFC1` -> `WFC2` -> `WFC3`) ---
    bool execute_full_pipeline_direct(
        uint32_t p_stage_mask = WFC_STAGE_DEFAULT_PIPELINE_MASK,
        SelectionGrantPOD *r_conflict_selection = nullptr
    );

    // --- Readback & Sync Helpers ---
    void read_environment_slots(std::vector<EnvironmentSlotPOD> &r_out) const;
    void read_pointer_or_conflict_masks(std::vector<uint32_t> &r_out) const;
    void read_contradiction_reports(std::vector<WFCContradictionReportPOD> &r_out) const;
    void sync_dod_slots_back_to_environment();
};

} // namespace ideam::core
