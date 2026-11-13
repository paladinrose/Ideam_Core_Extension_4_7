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
 *     1. `ConflictHorizonBoxPartitionMetadataLogic`: Draws the bounded environment box (E_box)
 *        around a WFC contradiction or cardinality conflict slot.
 *     2. `PointerStateEinselectionQueryLogic`: Evaluates `GenerationRule` scores and neighbor
 *        mutual-information redundancy (R_delta) to select settled Pointer States.
 *     3. `CoherenceCascadeRelaxAndPropagateTransformLogic`: Evaluates which lower-precedence
 *        CSS cascade properties/siblings on intermediate cells between each Pointer State and
 *        the problem cell can be altered without losing coherence, then propagates pointer
 *        properties inward to resolve the conflict.
 */

#include "quantum_darwinism_dod_common.h"

namespace ideam::core {

class QuantumDarwinismDOD {
private:
    MemoryManagerDOD *memory_manager_ = nullptr;
    TaskGraphDOD *task_graph_         = nullptr;

public:
    QuantumDarwinismDOD() = default;
    ~QuantumDarwinismDOD() = default;

    void bind_execution_context(MemoryManagerDOD *p_mem, TaskGraphDOD *p_graph) noexcept {
        memory_manager_ = p_mem;
        task_graph_     = p_graph;
    }

    [[nodiscard]] MemoryManagerDOD *memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] TaskGraphDOD *task_graph() const noexcept { return task_graph_; }
};

} // namespace ideam::core
