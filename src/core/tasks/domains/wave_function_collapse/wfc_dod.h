#pragma once

/**
 * wfc_dod.h
 * DOD Orchestrator for `IdeamWaveFunctionCollapse` (`src/core/tasks/domains/wave_function_collapse/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Operates on the shared `ProceduralEnvironment` populated by `IdeamProceduralGeneration`.
 * - Runs minimum-entropy slot collapse and propagates both spatial port adjacency and
 *   cascading `GenerationRule` hierarchy constraints (`CastleGrounds -> CastleTower`,
 *   child cardinality `[min..max]`, and sibling dependencies like `TowerLanding requires TowerStairs`).
 * - Flags any contradiction slots in `POINTER_OR_CONFLICT_MASK` so `IdeamQuantumDarwinism`
 *   TaskGraph nodes can stabilize the local environment horizon box without full-grid backtracking.
 */

#include "wfc_dod_common.h"

namespace ideam::core {

class WaveFunctionCollapseDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

public:
    WaveFunctionCollapseDOD() = default;
    ~WaveFunctionCollapseDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }
};

} // namespace ideam::core
