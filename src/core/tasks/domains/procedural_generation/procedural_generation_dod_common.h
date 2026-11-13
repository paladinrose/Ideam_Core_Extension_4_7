#pragma once

/**
 * procedural_generation_dod_common.h
 * Foundational pure C++26 DOD buffer slots, stage flags, and capacities for `IdeamProceduralGeneration`.
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

#include "../domain_dod_common.h"
#include "../../../procedural_generation/procedural_environment.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

namespace ideam::core {

enum class ProceduralGenerationBufferSlot : uint8_t {
    TOKEN_DESCRIPTOR_POD = 0,
    ENVIRONMENT_SLOT_POD,
    CASCADE_RULE_POD,
    CHILD_SLOT_CSR,
    POINTER_OR_CONFLICT_MASK,
    COUNT
};

} // namespace ideam::core
