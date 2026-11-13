#pragma once

/**
 * wfc_dod_common.h
 * Foundational pure C++26 DOD buffer slots, stage flags, and capacities for `IdeamWaveFunctionCollapse`.
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

#include "../domain_dod_common.h"
#include "../../../procedural_generation/procedural_environment.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

namespace ideam::core {

enum class WaveFunctionCollapseBufferSlot : uint8_t {
    TOKEN_DESCRIPTOR_POD = 0,
    ENVIRONMENT_SLOT_POD,
    CASCADE_RULE_POD,
    CHILD_SLOT_CSR,
    POINTER_OR_CONFLICT_MASK,
    COUNT
};

} // namespace ideam::core
