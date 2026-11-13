#pragma once

/**
 * procedural_generation_dod.h
 * DOD Orchestrator for `IdeamProceduralGeneration` (`src/core/tasks/domains/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Bridges `AnalysisRule`, `GenerationRule`, `ProceduralPattern`, and `ProceduralPatterner`
 *   into the shared `ProceduralEnvironment` DOD tables.
 * - Evaluates Godot reflection signatures (`ClassDB` inheritance, node names, suffixes like
 *   `"_cell"`, properties, methods, signals) and compiles cascading CSS-like `GenerationRule`s
 *   into flat bitmask domains and parent/child/sibling cardinality constraints.
 */

#include "procedural_generation_dod_common.h"

namespace ideam::core {

class ProceduralGenerationDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

public:
    ProceduralGenerationDOD() = default;
    ~ProceduralGenerationDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }
};

} // namespace ideam::core
