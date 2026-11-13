#pragma once

/**
 * linguistics_dod_common.h
 * Foundational pure C++26 DOD buffer slots, stage flags, and capacities for `IdeamLinguistics`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Bridges `SemeDescriptorPOD`, `SememeDescriptorPOD`, `PhonemePOD` (16-bit distinctive
 *   feature bitvectors), `MorphemePOD`, and `LexemePOD` onto `TaskGraphDOD`.
 * - `SemeComponentialAffinityQueryLogic`: Matches assets, `GameplayRule`s, and `ProceduralPattern`
 *   rules against natural-language Wizard prompts (e.g. `"top down, medieval, fantasy RPG"`)
 *   via Nuclear & Peripheral `Seme` bitmask intersection.
 * - `PhoneticHammingLexemeDecodeTransformLogic`: Reconstructs the most likely `Lexeme` and
 *   `Sememe` from noise-degraded `uint16_t` phoneme streams (biased by listener `AgentMindResource`
 *   expectations) and compiles grammar/vocabulary logit masks for optional local SLMs.
 */

#include "../domain_dod_common.h"
#include "../../../linguistics/semantic_lexicon_core.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

namespace ideam::core {

enum class LinguisticsBufferSlot : uint8_t {
    PRIMARY_DESCRIPTOR_POD = 0,
    SECONDARY_SLICE_CSR,
    FEATURE_BITMASK_STREAM,
    DECODED_SEMEME_STREAM,
    COUNT
};

} // namespace ideam::core
