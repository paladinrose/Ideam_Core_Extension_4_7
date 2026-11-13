#pragma once

/**
 * quantum_darwinism_dod_common.h
 * Foundational pure C++26 DOD buffer slots, stage flags, and capacities for `IdeamQuantumDarwinism`.
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

#include "../domain_dod_common.h"
#include "../../../procedural_generation/procedural_environment.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

namespace ideam::core {

enum class QuantumDarwinismBufferSlot : uint8_t {
    TOKEN_DESCRIPTOR_POD = 0,
    ENVIRONMENT_SLOT_POD,
    CASCADE_RULE_POD,
    CHILD_SLOT_CSR,
    POINTER_OR_CONFLICT_MASK,
    COUNT
};

} // namespace ideam::core
