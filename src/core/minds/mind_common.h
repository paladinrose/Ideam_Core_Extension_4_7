#pragma once

/**
 * mind_common.h
 * Pure C++26 POD types and enums for `IdeamMinds` (`src/core/minds/`).
 *
 * Implements the Section 8 DOD epistemic AI & cognitive flow structures:
 *   - `FactPod` (32 bytes): Objective ground-truth world fact entry in `InformationMatrixDOD`.
 *   - `BeliefPod` (24 bytes): Subjective per-agent `SPARSE_SET` belief overlay entry
 *     supporting partial confidence, misinformation (`believed_value != truth_value`),
 *     and source attribution (`source_agent_id`).
 *   - `ClaimPod`: Speech-act / disclosure / intentional-lie payload emitted during
 *     Stage 4 (Deliberation, Utility & Deception Synthesis).
 *   - `SensoryEventPod`: Ring-buffer sensory observation entry for Stage 1 & Stage 3.
 */

#include <cstdint>

namespace ideam::core {

enum class CognitiveWaveStage : uint8_t {
    WAVE_1_SENSORY_PERCEPTION        = 0, ///< Spatial/Event Ring -> wake observable fact_id bits
    WAVE_2_EMOTIONAL_APPRAISAL       = 1, ///< SIMD affect appraisal against instinctive traits
    WAVE_3_COGNITIVE_LATENCY_TRIAGE  = 2, ///< MetadataLogic on lod_levels & cognitive RING queue
    WAVE_4_DELIBERATION_AND_DECEPTION= 3, ///< Belief-space utility & truth/withhold/lie synthesis
    WAVE_5_CONTRADICTION_REFLECTION  = 4  ///< Epistemic reconciliation, trust & drive mutation
};

enum FactFlagBits : uint16_t {
    FACT_FLAG_NONE          = 0u,
    FACT_FLAG_PUBLIC        = 1u << 0,
    FACT_FLAG_SECRET        = 1u << 1,
    FACT_FLAG_ALIBI_RECORD  = 1u << 2, ///< Self-fact recorded when an agent emits a fabricated claim
    FACT_FLAG_CONTRADICTED  = 1u << 3
};

/**
 * Compact 32-byte Objective Ground-Truth Fact record (`InformationMatrixDOD`).
 * Example: (Fact #104: Subject = Butler, Predicate = WAS_AT_LOCATION, Value = Study, Time = 22:15).
 */
struct alignas(8) FactPod {
    uint32_t fact_id       = 0;
    uint32_t subject_id    = 0;
    uint16_t predicate_id  = 0;
    uint16_t flags         = FACT_FLAG_NONE;
    int64_t  truth_value   = 0;
    float    timestamp     = 0.0f;
    uint32_t location_id   = 0;
    float    base_salience = 1.0f;
};
static_assert(sizeof(FactPod) == 32, "FactPod must remain exactly 32 bytes.");

/**
 * Compact 24-byte Subjective Per-Agent Belief Overlay record (`SPARSE_SET` buffer).
 */
struct alignas(8) BeliefPod {
    uint32_t fact_id          = 0;
    uint32_t source_agent_id  = 0;
    int64_t  believed_value   = 0;
    float    confidence       = 1.0f;
    float    emotional_charge = 0.0f;
};
static_assert(sizeof(BeliefPod) == 24, "BeliefPod must remain exactly 24 bytes.");

/**
 * Speech-act claim emitted during Stage 4 (Truth, Withholding, or Intentional Lie).
 */
struct ClaimPod {
    uint32_t speaker_agent_id  = 0;
    uint32_t listener_agent_id = 0;
    uint32_t fact_id           = 0;
    int64_t  claimed_value     = 0;
    float    claimed_confidence= 1.0f;
    bool     is_intentional_lie= false;
};

/**
 * Per-agent instinctive trait & drive weights used in Stages 2 and 4.
 */
struct InstinctiveTraitWeightsPod {
    float fear              = 0.5f;
    float curiosity         = 0.5f;
    float guilt             = 0.5f;
    float loyalty           = 0.5f;
    float self_preservation = 0.5f;
    float trust_in_listener = 0.5f;
};

} // namespace ideam::core
