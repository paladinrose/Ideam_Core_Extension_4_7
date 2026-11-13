#pragma once

/**
 * minds_dod_common.h
 * Foundational pure C++26 data structures, table-driven buffer manifest descriptors,
 * capacity configurations, and composable TaskGraph pipeline stage flags for the
 * `IdeamMinds <-> IdeamTasks` Data-Oriented Design (DOD) substrate (`src/core/tasks/domains/minds/`).
 *
 * Implements the 5 core contiguous memory table schemas and 3 reactive event rings
 * defined in Section 2.2 of the Master Plan, unified with the **Multi-Tome Akashic Ground Truth**
 * and **Per-Agent Epistemic Reference Catalogue** paradigm:
 *   1. `WorldGroundTruthFactTable` (`AkashicTomeHeaderPod` + `FactPod` AoS & SOA columns:
 *      `subject_id`, `predicate_id`, `truth_value`, `location_id`, `timestamp`, `base_salience`,
 *      `fact_flags`, `tome_id`).
 *   2. `AgentTraitAndMentalStateTable` (Per-agent `InstinctiveTraitWeightsPod`, `excitation_level`
 *      from `IMentalState`, `attention_threshold`, `knowledge_threshold`, `active_emotion_mask`,
 *      `primary_tome_id`, and CSR belief-catalogue row offsets/counts).
 *   3. `AgentBeliefSparseTable` (`SPARSE_SET` / CSR per-agent **Reference Catalogue** overlay:
 *      `BeliefPod` + `BeliefCatalogueEntryPod` & SOA columns `catalogued_fact_id`, `true_fact_id`
 *      for misreferencing, `believed_tome_id`, `true_tome_id`, `believed_value`, `prev_believed_value`
 *      for `SwapView` latency triage, `confidence`, `acceptance`, `emotional_charge`,
 *      `intuition_charge`, `source_agent_id`, `stimulus_priority`).
 *   4. `ReasoningAndTheoryTable` (`ObservationPod` rows, `HypothesisPod` rows, `EvidencePod` rows,
 *      `SyllogisticPropositionPod` rows, `ExplanationPod` CSR headers, and flattened explanation
 *      fact/cause CSR index slices).
 *   5. `DiscourseAndProsodyTable` (`DiscourseTurnPod` rows, `TurnConstructionUnitType`,
 *      `TurnResolutionType`, active speaker/listener bitmasks, `Spline2D` prosody handle IDs,
 *      and sememe/lexeme token streams).
 *   6. **3 Event Rings**:
 *      - `SensoryObservationEventRing` (`SensoryEventPod` Wave 1 input)
 *      - `SpeechClaimEventRing` (`ClaimPod` Wave 4 -> Wave 5)
 *      - `MindIntuitionAndContradictionRing` (`MindEpistemicEventPod` notifications to `NarrativeDOD` / `GameDOD`)
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#include "../domain_dod_common.h"
#include "../../../minds/mind_common.h"
#include "../../../minds/information_matrix_dod.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

// Include the 3 Minds task registry headers
#include "registration/minds_metadata_task_registry.h"
#include "registration/minds_query_task_registry.h"
#include "registration/minds_transform_task_registry.h"

// Include the 12 IdeamMinds Task Logic Structs (MND1 - MND12)
#include "metadata_logic/stimulus_latency_triage_metadata_logic.h"
#include "query_logic/intuitive_leap_stochastic_query_logic.h"
#include "query_logic/sensory_observation_wake_query_logic.h"
#include "transform_logic/abductive_hypothesis_transform_logic.h"
#include "transform_logic/contradiction_and_trust_reconcile_transform_logic.h"
#include "transform_logic/deductive_syllogism_propagate_transform_logic.h"
#include "transform_logic/deliberation_and_deception_transform_logic.h"
#include "transform_logic/discourse_turn_and_prosody_transform_logic.h"
#include "transform_logic/inductive_evidence_accumulate_transform_logic.h"
#include "transform_logic/instinctive_affect_appraisal_transform_logic.h"
#include "transform_logic/sememe_and_lexicon_morphology_transform_logic.h"
#include "transform_logic/sensory_signal_decode_transform_logic.h"

namespace ideam::core {

/**
 * MindsPipelineStageFlags
 * Composable bitmask flags representing the 5 cognitive waves in `MindDOD`.
 */
enum MindsPipelineStageFlags : uint32_t {
    MINDS_STAGE_NONE                          = 0u,

    // Wave 1: MND1 SensorySignalDecodeTransformLogic + MND2 SensoryObservationWakeQueryLogic
    MINDS_STAGE_WAVE_1_SENSORY_PERCEPTION     = 1u << 0,

    // Wave 2: MND3 InstinctiveAffectAppraisalTransformLogic
    MINDS_STAGE_WAVE_2_EMOTIONAL_APPRAISAL    = 1u << 1,

    // Wave 3: MND4 StimulusLatencyTriage + MND5 AbductiveHypothesis + MND6 IntuitiveLeap + MND7 InductiveEvidence
    MINDS_STAGE_WAVE_3_LATENCY_AND_REASONING  = 1u << 2,

    // Wave 4: MND8 DeductiveSyllogism + MND9 DeliberationAndDeception + MND10 DiscourseTurnAndProsody
    MINDS_STAGE_WAVE_4_DEDUCTION_AND_DISCOURSE= 1u << 3,

    // Wave 5: MND11 ContradictionAndTrustReconcile + MND12 SememeAndLexiconMorphology
    MINDS_STAGE_WAVE_5_REFLECTION_AND_SEMEMES = 1u << 4,

    // Preset masks:
    MINDS_STAGE_DEFAULT_PIPELINE_MASK         =
        MINDS_STAGE_WAVE_1_SENSORY_PERCEPTION      |
        MINDS_STAGE_WAVE_2_EMOTIONAL_APPRAISAL     |
        MINDS_STAGE_WAVE_3_LATENCY_AND_REASONING   |
        MINDS_STAGE_WAVE_4_DEDUCTION_AND_DISCOURSE |
        MINDS_STAGE_WAVE_5_REFLECTION_AND_SEMEMES,

    MINDS_STAGE_ALL_MASK                      = 0x001Fu
};

/**
 * MindsNodeExecutionConfig
 * Optional overrides when instantiating any Minds task node in a `TaskGraphDOD`.
 */
struct MindsNodeExecutionConfig {
    MemoryView view            = MemoryView::SingleElementView;
    MemoryStrategy strategy    = MemoryStrategy::FlatStrategy;
    MemoryTypes element_type   = MemoryTypes::INT32;
    QueryOp query_op           = QueryOp::CULL;
    int32_t execution_priority = 0;
    bool use_registry_factory  = false;
    std::span<const GrantPartPOD> custom_requirements{};
};

/**
 * MindsStageAnchor
 * Topological handle returned for each injected cognitive wave stage in a `TaskGraphDOD`.
 */
struct MindsStageAnchor {
    MindsPipelineStageFlags stage = MINDS_STAGE_NONE;
    NodeID entry_node             = INVALID_ID;
    NodeID exit_node              = INVALID_ID;
    std::vector<NodeID> nodes;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return entry_node != INVALID_ID && exit_node != INVALID_ID;
    }
};

/**
 * MindsPipelineTopology
 * Describes the set of `MindsStageAnchor`s injected into a `TaskGraphDOD`.
 */
struct MindsPipelineTopology {
    NodeID first_node = INVALID_ID;
    NodeID last_node  = INVALID_ID;
    std::vector<MindsStageAnchor> stages;

    [[nodiscard]] const MindsStageAnchor *get_stage_anchor(MindsPipelineStageFlags p_stage) const noexcept {
        for (const auto &anchor : stages) {
            if (anchor.stage == p_stage) {
                return &anchor;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool has_stage(MindsPipelineStageFlags p_stage) const noexcept {
        const MindsStageAnchor *anchor = get_stage_anchor(p_stage);
        return anchor != nullptr && anchor->is_valid();
    }
};

/**
 * MindCapacityKind
 * Identifies which configurable capacity field in `MindsBufferCapacities` governs a slot.
 */
enum class MindCapacityKind : uint8_t {
    AKASHIC_TOMES = 0,
    FACTS,
    AGENTS,
    AGENT_CSR_OFFSETS,
    BELIEFS,
    OBSERVATIONS,
    HYPOTHESES,
    EVIDENCE_ROWS,
    SYLLOGISMS,
    EXPLANATIONS,
    EXPLANATION_CSR_INDICES,
    DISCOURSE_TURNS,
    SEMEME_TOKENS,
    SENSORY_EVENT_RING,
    SPEECH_CLAIM_RING,
    EPISTEMIC_EVENT_RING,
    RING_CURSOR_OR_COUNTER
};

/**
 * MindTableGroup
 * Logical table ownership tag for each buffer slot, enabling `MindDOD` to synchronize
 * active selection counts in a single loop over `MIND_BUFFER_SPECS`.
 */
enum class MindTableGroup : uint8_t {
    AKASHIC_TOME_TABLE = 0,
    WORLD_GROUND_TRUTH_FACT_TABLE,
    AGENT_TRAIT_AND_STATE_TABLE,
    AGENT_BELIEF_CSR_ROW,
    AGENT_BELIEF_SPARSE_TABLE,
    OBSERVATION_TABLE,
    HYPOTHESIS_TABLE,
    EVIDENCE_TABLE,
    SYLLOGISM_TABLE,
    EXPLANATION_TABLE,
    EXPLANATION_CSR_SLICE,
    DISCOURSE_AND_PROSODY_TABLE,
    SEMEME_TOKEN_SLICE,
    EVENT_RING_OR_COUNTER
};

/**
 * MindsBufferCapacities
 * Configurable row capacities for the 5 Section 2.2 buffer tables and 3 event rings.
 */
struct MindsBufferCapacities {
    uint32_t max_akashic_tomes           = 64;
    uint32_t max_facts                   = 8192;
    uint32_t max_agents                  = 1024;
    uint32_t max_beliefs                 = 32768;
    uint32_t max_observations            = 8192;
    uint32_t max_hypotheses              = 2048;
    uint32_t max_evidence_rows           = 4096;
    uint32_t max_syllogisms              = 2048;
    uint32_t max_explanations            = 1024;
    uint32_t max_explanation_csr_indices = 8192;
    uint32_t max_discourse_turns         = 512;
    uint32_t max_sememe_tokens           = 8192;
    uint32_t max_sensory_events          = 2048;
    uint32_t max_speech_claims           = 1024;
    uint32_t max_epistemic_events        = 1024;
    uint32_t max_ring_cursors            = 4;

    [[nodiscard]] constexpr uint32_t get_capacity(MindCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case MindCapacityKind::AKASHIC_TOMES:           return max_akashic_tomes;
            case MindCapacityKind::FACTS:                   return max_facts;
            case MindCapacityKind::AGENTS:                  return max_agents;
            case MindCapacityKind::AGENT_CSR_OFFSETS:       return max_agents + 1u;
            case MindCapacityKind::BELIEFS:                 return max_beliefs;
            case MindCapacityKind::OBSERVATIONS:            return max_observations;
            case MindCapacityKind::HYPOTHESES:              return max_hypotheses;
            case MindCapacityKind::EVIDENCE_ROWS:           return max_evidence_rows;
            case MindCapacityKind::SYLLOGISMS:              return max_syllogisms;
            case MindCapacityKind::EXPLANATIONS:            return max_explanations;
            case MindCapacityKind::EXPLANATION_CSR_INDICES: return max_explanation_csr_indices;
            case MindCapacityKind::DISCOURSE_TURNS:         return max_discourse_turns;
            case MindCapacityKind::SEMEME_TOKENS:           return max_sememe_tokens;
            case MindCapacityKind::SENSORY_EVENT_RING:      return max_sensory_events;
            case MindCapacityKind::SPEECH_CLAIM_RING:       return max_speech_claims;
            case MindCapacityKind::EPISTEMIC_EVENT_RING:    return max_epistemic_events;
            case MindCapacityKind::RING_CURSOR_OR_COUNTER:  return max_ring_cursors;
            default:                                        return max_facts;
        }
    }
};

/**
 * MindBufferSlot
 * Enumerates every DOD buffer column and event ring across the 5 Section 2.2 tables
 * and 3 event rings. Indexed 1:1 into `MIND_BUFFER_SPECS` and `MindsBufferManifest`.
 */
enum class MindBufferSlot : uint8_t {
    // --- Table 1: WorldGroundTruthFactTable (Multi-Source Akashic Tomes & Ground-Truth Facts) ---
    MIND_AKASHIC_TOME_HEADER_POD = 0,     // AkashicTomeHeaderPod (16B AoS)
    MIND_FACT_POD,                        // FactPod (32B AoS)
    MIND_FACT_SUBJECT_ID,                 // uint32_t (4B SOA)
    MIND_FACT_PREDICATE_ID,               // uint16_t (2B SOA)
    MIND_FACT_TRUTH_VALUE,                // int64_t  (8B SOA)
    MIND_FACT_LOCATION_ID,                // uint32_t (4B SOA)
    MIND_FACT_TIMESTAMP,                  // float    (4B SOA)
    MIND_FACT_BASE_SALIENCE,              // float    (4B SOA)
    MIND_FACT_FLAGS,                      // uint16_t (2B SOA: FactFlagBits)
    MIND_FACT_TOME_ID,                    // uint16_t (2B SOA: AkashicTomeDomain / tome_id)

    // --- Table 2: AgentTraitAndMentalStateTable (Per-Agent Traits, Mental State & CSR Offsets) ---
    MIND_AGENT_TRAIT_WEIGHTS_POD,         // InstinctiveTraitWeightsPod (24B AoS)
    MIND_AGENT_EXCITATION_LEVEL,          // float    (4B SOA: IMentalState.ExcitationLevel)
    MIND_AGENT_ATTENTION_THRESHOLD,       // float    (4B SOA)
    MIND_AGENT_KNOWLEDGE_THRESHOLD,       // float    (4B SOA: Reason.cs KnowledgeThreshold)
    MIND_AGENT_ACTIVE_EMOTION_MASK,       // uint32_t (4B SOA: UnityEmotion active mask)
    MIND_AGENT_PRIMARY_TOME_ID,           // uint16_t (2B SOA: Default Akashic Tome consulted by agent)
    MIND_AGENT_BELIEF_CSR_OFFSETS,        // uint32_t (4B Flat: N+1 CSR offsets into AgentBeliefSparseTable)
    MIND_AGENT_BELIEF_CSR_COUNTS,         // uint16_t (2B Flat: per-agent belief entry count)

    // --- Table 3: AgentBeliefSparseTable (Per-Agent Epistemic Reference Catalogue Overlay) ---
    MIND_BELIEF_POD,                      // BeliefPod (32B AoS)
    MIND_BELIEF_CATALOGUE_ENTRY_POD,      // BeliefCatalogueEntryPod (16B AoS: catalogued vs true fact & tome IDs)
    MIND_BELIEF_CATALOGUED_FACT_ID,       // uint32_t (4B SOA: fact ID the agent thinks they are referencing)
    MIND_BELIEF_TRUE_FACT_ID,             // uint32_t (4B SOA: actual Akashic Tome fact ID — differs on misreference!)
    MIND_BELIEF_BELIEVED_TOME_ID,         // uint16_t (2B SOA: tome ID the agent thinks this belongs to)
    MIND_BELIEF_TRUE_TOME_ID,             // uint16_t (2B SOA: actual tome ID in the Akashic registry)
    MIND_BELIEF_BELIEVED_VALUE,           // int64_t  (8B SOA: current subjective believed value)
    MIND_BELIEF_PREV_BELIEVED_VALUE,      // int64_t  (8B SOA: previous tick believed value for SwapView MND4 latency triage)
    MIND_BELIEF_CONFIDENCE,               // float    (4B SOA: Inductive evidence strength [0.0, 1.0])
    MIND_BELIEF_ACCEPTANCE,               // float    (4B SOA: Abductive explanatory fit [0.0, 1.0])
    MIND_BELIEF_EMOTIONAL_CHARGE,         // float    (4B SOA: Wave 2 affective appraisal charge)
    MIND_BELIEF_INTUITION_CHARGE,         // float    (4B SOA: Wave 3 near-miss abductive intuition accumulator)
    MIND_BELIEF_SOURCE_AGENT_ID,          // uint32_t (4B SOA: 0 = direct sensory observation, >0 = speaker ID)
    MIND_BELIEF_STIMULUS_PRIORITY,        // uint8_t  (1B SOA: StimulusPriority 1..12)
    MIND_BELIEF_FLAGS,                    // uint8_t  (1B SOA: FactFlagBits including FACT_FLAG_MISREFERENCED)

    // --- Table 4: ReasoningAndTheoryTable (Abduction, Induction, Deduction & Explanations) ---
    MIND_OBSERVATION_POD,                 // ObservationPod (16B AoS)
    MIND_HYPOTHESIS_POD,                  // HypothesisPod (16B AoS)
    MIND_EVIDENCE_POD,                    // EvidencePod (16B AoS)
    MIND_SYLLOGISM_PROPOSITION_POD,       // SyllogisticPropositionPod (16B AoS)
    MIND_EXPLANATION_POD,                 // ExplanationPod (16B AoS CSR header)
    MIND_EXPLANATION_FACT_CSR_IDS,        // uint32_t (4B Flat: CSR slice of Fact IDs in an Explanation)
    MIND_EXPLANATION_CAUSE_CSR_IDS,       // uint32_t (4B Flat: CSR slice of Cause Statement/Condition IDs)

    // --- Table 5: DiscourseAndProsodyTable (Spoken Dialogue Turn-Taking, Prosody & Sememes) ---
    MIND_DISCOURSE_TURN_POD,              // DiscourseTurnPod (32B AoS)
    MIND_DISCOURSE_SPEAKER_MASK,          // uint64_t (8B SOA: bitmask of active speakers per discourse)
    MIND_DISCOURSE_LISTENER_MASK,         // uint64_t (8B SOA: bitmask of active listeners per discourse)
    MIND_DISCOURSE_PROSODY_SPLINE_ID,     // uint32_t (4B SOA: Spline2D handle ID for X=Volume, Y=Pitch)
    MIND_SEMEME_TOKEN_STREAM,             // uint32_t (4B Flat: resolved Sememe/Lexeme/Morpheme token IDs)

    // --- 3 Event Rings & Cursors ---
    MIND_SENSORY_OBSERVATION_EVENT_RING,  // SensoryEventPod (32B Ring: Wave 1 input)
    MIND_SENSORY_RING_CURSOR,             // uint32_t[2]: [read_idx, write_idx]
    MIND_SPEECH_CLAIM_EVENT_RING,         // ClaimPod (32B Ring: Wave 4 -> Wave 5 speech acts)
    MIND_SPEECH_CLAIM_RING_CURSOR,        // uint32_t[2]: [read_idx, write_idx]
    MIND_INTUITION_CONTRADICTION_RING,    // MindEpistemicEventPod (16B Ring: notifications to NarrativeDOD / GameDOD)
    MIND_INTUITION_RING_CURSOR,           // uint32_t[2]: [read_idx, write_idx]

    COUNT
};

static constexpr size_t MIND_BUFFER_SLOT_COUNT = static_cast<size_t>(MindBufferSlot::COUNT);

/**
 * MindBufferSpec
 * Compile-time descriptor row in `MIND_BUFFER_SPECS`.
 */
struct MindBufferSpec {
    MindBufferSlot slot                     = MindBufferSlot::COUNT;
    std::string_view canonical_name         = {};
    std::array<std::string_view, 3> aliases = {};
    uint32_t element_stride                 = sizeof(uint32_t);
    BufferLayoutType layout                 = BufferLayoutType::FLAT;
    SelectionMode selection_mode            = SelectionMode::RANGE;
    MindCapacityKind capacity_kind          = MindCapacityKind::FACTS;
    MindTableGroup table_group              = MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE;

    [[nodiscard]] constexpr bool matches_name(std::string_view p_query) const noexcept {
        if (p_query.empty()) return false;
        if (canonical_name == p_query) return true;
        for (const auto &alias : aliases) {
            if (!alias.empty() && alias == p_query) {
                return true;
            }
        }
        return false;
    }
};

/**
 * MIND_BUFFER_SPECS
 * Compile-time manifest covering all 51 `MindDOD` buffer slots and property aliases.
 */
inline constexpr std::array<MindBufferSpec, MIND_BUFFER_SLOT_COUNT> MIND_BUFFER_SPECS = {{
    // ========================================================================
    // Table 1: WorldGroundTruthFactTable (Akashic Tomes & Ground-Truth Facts)
    // ========================================================================
    {
        MindBufferSlot::MIND_AKASHIC_TOME_HEADER_POD,
        "akashic_tome_header_buffer_id",
        {"mind_akashic_tome_header_pod_buffer_id", "tome_header_buffer_id", ""},
        sizeof(AkashicTomeHeaderPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::AKASHIC_TOMES, MindTableGroup::AKASHIC_TOME_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_POD,
        "world_ground_truth_fact_buffer_id",
        {"mind_fact_pod_buffer_id", "fact_pod_buffer_id", "akashic_fact_buffer_id"},
        sizeof(FactPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_SUBJECT_ID,
        "fact_subject_id_buffer_id",
        {"mind_fact_subject_id_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_PREDICATE_ID,
        "fact_predicate_id_buffer_id",
        {"mind_fact_predicate_id_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_TRUTH_VALUE,
        "fact_truth_value_buffer_id",
        {"mind_fact_truth_value_buffer_id", "ground_truth_value_buffer_id", ""},
        sizeof(int64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_LOCATION_ID,
        "fact_location_id_buffer_id",
        {"mind_fact_location_id_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_TIMESTAMP,
        "fact_timestamp_buffer_id",
        {"mind_fact_timestamp_buffer_id", "", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_BASE_SALIENCE,
        "fact_base_salience_buffer_id",
        {"mind_fact_base_salience_buffer_id", "", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_FLAGS,
        "fact_flags_buffer_id",
        {"mind_fact_flags_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },
    {
        MindBufferSlot::MIND_FACT_TOME_ID,
        "fact_tome_id_buffer_id",
        {"mind_fact_tome_id_buffer_id", "akashic_tome_id_buffer_id", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::FACTS, MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE
    },

    // ========================================================================
    // Table 2: AgentTraitAndMentalStateTable
    // ========================================================================
    {
        MindBufferSlot::MIND_AGENT_TRAIT_WEIGHTS_POD,
        "agent_trait_weights_buffer_id",
        {"mind_agent_trait_weights_pod_buffer_id", "instinctive_traits_buffer_id", ""},
        sizeof(InstinctiveTraitWeightsPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::AGENTS, MindTableGroup::AGENT_TRAIT_AND_STATE_TABLE
    },
    {
        MindBufferSlot::MIND_AGENT_EXCITATION_LEVEL,
        "agent_excitation_level_buffer_id",
        {"mind_agent_excitation_level_buffer_id", "excitation_level_buffer_id", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::AGENTS, MindTableGroup::AGENT_TRAIT_AND_STATE_TABLE
    },
    {
        MindBufferSlot::MIND_AGENT_ATTENTION_THRESHOLD,
        "agent_attention_threshold_buffer_id",
        {"mind_agent_attention_threshold_buffer_id", "attention_threshold_buffer_id", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::AGENTS, MindTableGroup::AGENT_TRAIT_AND_STATE_TABLE
    },
    {
        MindBufferSlot::MIND_AGENT_KNOWLEDGE_THRESHOLD,
        "agent_knowledge_threshold_buffer_id",
        {"mind_agent_knowledge_threshold_buffer_id", "knowledge_threshold_buffer_id", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::AGENTS, MindTableGroup::AGENT_TRAIT_AND_STATE_TABLE
    },
    {
        MindBufferSlot::MIND_AGENT_ACTIVE_EMOTION_MASK,
        "agent_active_emotion_mask_buffer_id",
        {"mind_agent_active_emotion_mask_buffer_id", "active_emotion_mask_buffer_id", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::AGENTS, MindTableGroup::AGENT_TRAIT_AND_STATE_TABLE
    },
    {
        MindBufferSlot::MIND_AGENT_PRIMARY_TOME_ID,
        "agent_primary_tome_id_buffer_id",
        {"mind_agent_primary_tome_id_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::AGENTS, MindTableGroup::AGENT_TRAIT_AND_STATE_TABLE
    },
    {
        MindBufferSlot::MIND_AGENT_BELIEF_CSR_OFFSETS,
        "agent_belief_csr_offsets_buffer_id",
        {"mind_agent_belief_csr_offsets_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::AGENT_CSR_OFFSETS, MindTableGroup::AGENT_BELIEF_CSR_ROW
    },
    {
        MindBufferSlot::MIND_AGENT_BELIEF_CSR_COUNTS,
        "agent_belief_csr_counts_buffer_id",
        {"mind_agent_belief_csr_counts_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::AGENTS, MindTableGroup::AGENT_TRAIT_AND_STATE_TABLE
    },

    // ========================================================================
    // Table 3: AgentBeliefSparseTable (Reference Catalogue Overlay)
    // ========================================================================
    {
        MindBufferSlot::MIND_BELIEF_POD,
        "agent_belief_pod_buffer_id",
        {"mind_belief_pod_buffer_id", "belief_pod_buffer_id", ""},
        sizeof(BeliefPod), BufferLayoutType::AOS, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_CATALOGUE_ENTRY_POD,
        "agent_belief_catalogue_entry_buffer_id",
        {"mind_belief_catalogue_entry_pod_buffer_id", "reference_catalogue_buffer_id", ""},
        sizeof(BeliefCatalogueEntryPod), BufferLayoutType::AOS, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_CATALOGUED_FACT_ID,
        "belief_catalogued_fact_id_buffer_id",
        {"mind_belief_catalogued_fact_id_buffer_id", "belief_fact_id_buffer_id", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_TRUE_FACT_ID,
        "belief_true_fact_id_buffer_id",
        {"mind_belief_true_fact_id_buffer_id", "akashic_referenced_fact_id_buffer_id", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_BELIEVED_TOME_ID,
        "belief_believed_tome_id_buffer_id",
        {"mind_belief_believed_tome_id_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_TRUE_TOME_ID,
        "belief_true_tome_id_buffer_id",
        {"mind_belief_true_tome_id_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_BELIEVED_VALUE,
        "belief_believed_value_buffer_id",
        {"mind_belief_believed_value_buffer_id", "curr_believed_value_buffer_id", ""},
        sizeof(int64_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_PREV_BELIEVED_VALUE,
        "belief_prev_believed_value_buffer_id",
        {"mind_belief_prev_believed_value_buffer_id", "prev_believed_value_buffer_id", ""},
        sizeof(int64_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_CONFIDENCE,
        "belief_confidence_buffer_id",
        {"mind_belief_confidence_buffer_id", "inductive_confidence_buffer_id", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_ACCEPTANCE,
        "belief_acceptance_buffer_id",
        {"mind_belief_acceptance_buffer_id", "abductive_acceptance_buffer_id", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_EMOTIONAL_CHARGE,
        "belief_emotional_charge_buffer_id",
        {"mind_belief_emotional_charge_buffer_id", "", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_INTUITION_CHARGE,
        "belief_intuition_charge_buffer_id",
        {"mind_belief_intuition_charge_buffer_id", "intuition_charge_buffer_id", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_SOURCE_AGENT_ID,
        "belief_source_agent_id_buffer_id",
        {"mind_belief_source_agent_id_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_STIMULUS_PRIORITY,
        "belief_stimulus_priority_buffer_id",
        {"mind_belief_stimulus_priority_buffer_id", "stimulus_priority_buffer_id", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },
    {
        MindBufferSlot::MIND_BELIEF_FLAGS,
        "belief_flags_buffer_id",
        {"mind_belief_flags_buffer_id", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        MindCapacityKind::BELIEFS, MindTableGroup::AGENT_BELIEF_SPARSE_TABLE
    },

    // ========================================================================
    // Table 4: ReasoningAndTheoryTable
    // ========================================================================
    {
        MindBufferSlot::MIND_OBSERVATION_POD,
        "observation_pod_buffer_id",
        {"mind_observation_pod_buffer_id", "", ""},
        sizeof(ObservationPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::OBSERVATIONS, MindTableGroup::OBSERVATION_TABLE
    },
    {
        MindBufferSlot::MIND_HYPOTHESIS_POD,
        "hypothesis_pod_buffer_id",
        {"mind_hypothesis_pod_buffer_id", "", ""},
        sizeof(HypothesisPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::HYPOTHESES, MindTableGroup::HYPOTHESIS_TABLE
    },
    {
        MindBufferSlot::MIND_EVIDENCE_POD,
        "evidence_pod_buffer_id",
        {"mind_evidence_pod_buffer_id", "", ""},
        sizeof(EvidencePod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::EVIDENCE_ROWS, MindTableGroup::EVIDENCE_TABLE
    },
    {
        MindBufferSlot::MIND_SYLLOGISM_PROPOSITION_POD,
        "syllogism_proposition_pod_buffer_id",
        {"mind_syllogism_proposition_pod_buffer_id", "syllogism_buffer_id", ""},
        sizeof(SyllogisticPropositionPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::SYLLOGISMS, MindTableGroup::SYLLOGISM_TABLE
    },
    {
        MindBufferSlot::MIND_EXPLANATION_POD,
        "explanation_pod_buffer_id",
        {"mind_explanation_pod_buffer_id", "", ""},
        sizeof(ExplanationPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::EXPLANATIONS, MindTableGroup::EXPLANATION_TABLE
    },
    {
        MindBufferSlot::MIND_EXPLANATION_FACT_CSR_IDS,
        "explanation_fact_csr_ids_buffer_id",
        {"mind_explanation_fact_csr_ids_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::EXPLANATION_CSR_INDICES, MindTableGroup::EXPLANATION_CSR_SLICE
    },
    {
        MindBufferSlot::MIND_EXPLANATION_CAUSE_CSR_IDS,
        "explanation_cause_csr_ids_buffer_id",
        {"mind_explanation_cause_csr_ids_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::EXPLANATION_CSR_INDICES, MindTableGroup::EXPLANATION_CSR_SLICE
    },

    // ========================================================================
    // Table 5: DiscourseAndProsodyTable
    // ========================================================================
    {
        MindBufferSlot::MIND_DISCOURSE_TURN_POD,
        "discourse_turn_pod_buffer_id",
        {"mind_discourse_turn_pod_buffer_id", "", ""},
        sizeof(DiscourseTurnPod), BufferLayoutType::AOS, SelectionMode::RANGE,
        MindCapacityKind::DISCOURSE_TURNS, MindTableGroup::DISCOURSE_AND_PROSODY_TABLE
    },
    {
        MindBufferSlot::MIND_DISCOURSE_SPEAKER_MASK,
        "discourse_speaker_mask_buffer_id",
        {"mind_discourse_speaker_mask_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::DISCOURSE_TURNS, MindTableGroup::DISCOURSE_AND_PROSODY_TABLE
    },
    {
        MindBufferSlot::MIND_DISCOURSE_LISTENER_MASK,
        "discourse_listener_mask_buffer_id",
        {"mind_discourse_listener_mask_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::DISCOURSE_TURNS, MindTableGroup::DISCOURSE_AND_PROSODY_TABLE
    },
    {
        MindBufferSlot::MIND_DISCOURSE_PROSODY_SPLINE_ID,
        "discourse_prosody_spline_id_buffer_id",
        {"mind_discourse_prosody_spline_id_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::DISCOURSE_TURNS, MindTableGroup::DISCOURSE_AND_PROSODY_TABLE
    },
    {
        MindBufferSlot::MIND_SEMEME_TOKEN_STREAM,
        "sememe_token_stream_buffer_id",
        {"mind_sememe_token_stream_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::SEMEME_TOKENS, MindTableGroup::SEMEME_TOKEN_SLICE
    },

    // ========================================================================
    // 3 Event Rings & Cursors
    // ========================================================================
    {
        MindBufferSlot::MIND_SENSORY_OBSERVATION_EVENT_RING,
        "sensory_observation_event_ring_buffer_id",
        {"mind_sensory_observation_event_ring_buffer_id", "sensory_event_ring_buffer_id", ""},
        sizeof(SensoryEventPod), BufferLayoutType::RING, SelectionMode::RANGE,
        MindCapacityKind::SENSORY_EVENT_RING, MindTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        MindBufferSlot::MIND_SENSORY_RING_CURSOR,
        "sensory_ring_cursor_buffer_id",
        {"mind_sensory_ring_cursor_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::RING_CURSOR_OR_COUNTER, MindTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        MindBufferSlot::MIND_SPEECH_CLAIM_EVENT_RING,
        "speech_claim_event_ring_buffer_id",
        {"mind_speech_claim_event_ring_buffer_id", "claim_event_ring_buffer_id", ""},
        sizeof(ClaimPod), BufferLayoutType::RING, SelectionMode::RANGE,
        MindCapacityKind::SPEECH_CLAIM_RING, MindTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        MindBufferSlot::MIND_SPEECH_CLAIM_RING_CURSOR,
        "speech_claim_ring_cursor_buffer_id",
        {"mind_speech_claim_ring_cursor_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::RING_CURSOR_OR_COUNTER, MindTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        MindBufferSlot::MIND_INTUITION_CONTRADICTION_RING,
        "mind_intuition_and_contradiction_ring_buffer_id",
        {"mind_epistemic_event_ring_buffer_id", "contradiction_ring_buffer_id", ""},
        sizeof(MindEpistemicEventPod), BufferLayoutType::RING, SelectionMode::RANGE,
        MindCapacityKind::EPISTEMIC_EVENT_RING, MindTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        MindBufferSlot::MIND_INTUITION_RING_CURSOR,
        "mind_intuition_ring_cursor_buffer_id",
        {"epistemic_event_ring_cursor_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        MindCapacityKind::RING_CURSOR_OR_COUNTER, MindTableGroup::EVENT_RING_OR_COUNTER
    }
}};

[[nodiscard]] consteval bool validate_mind_buffer_specs() noexcept {
    for (size_t i = 0; i < MIND_BUFFER_SLOT_COUNT; ++i) {
        if (static_cast<size_t>(MIND_BUFFER_SPECS[i].slot) != i) {
            return false;
        }
        if (MIND_BUFFER_SPECS[i].canonical_name.empty() || MIND_BUFFER_SPECS[i].element_stride == 0u) {
            return false;
        }
    }
    return true;
}

static_assert(validate_mind_buffer_specs(), "MIND_BUFFER_SPECS indices must match MindBufferSlot enum order!");

/**
 * MindsBufferManifest
 * Table-driven directory of all `MemoryManagerDOD` buffer IDs backing `MindDOD`.
 */
struct MindsBufferManifest {
    std::array<uint32_t, MIND_BUFFER_SLOT_COUNT> buffer_ids = []() consteval {
        std::array<uint32_t, MIND_BUFFER_SLOT_COUNT> arr{};
        arr.fill(INVALID_ID);
        return arr;
    }();

    [[nodiscard]] constexpr uint32_t &operator[](MindBufferSlot p_slot) noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr const uint32_t &operator[](MindBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr uint32_t get(MindBufferSlot p_slot) const noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        return (idx < MIND_BUFFER_SLOT_COUNT) ? buffer_ids[idx] : INVALID_ID;
    }

    constexpr void set(MindBufferSlot p_slot, uint32_t p_buffer_id) noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        if (idx < MIND_BUFFER_SLOT_COUNT) {
            buffer_ids[idx] = p_buffer_id;
        }
    }

    constexpr void reset() noexcept {
        buffer_ids.fill(INVALID_ID);
    }

    [[nodiscard]] constexpr uint32_t get_by_name(std::string_view p_name) const noexcept {
        for (size_t i = 0; i < MIND_BUFFER_SLOT_COUNT; ++i) {
            if (MIND_BUFFER_SPECS[i].matches_name(p_name)) {
                return buffer_ids[i];
            }
        }
        return INVALID_ID;
    }

    [[nodiscard]] constexpr bool is_initialized() const noexcept {
        return get(MindBufferSlot::MIND_AKASHIC_TOME_HEADER_POD) != INVALID_ID &&
               get(MindBufferSlot::MIND_FACT_POD) != INVALID_ID &&
               get(MindBufferSlot::MIND_AGENT_TRAIT_WEIGHTS_POD) != INVALID_ID &&
               get(MindBufferSlot::MIND_BELIEF_POD) != INVALID_ID;
    }
};

using MindBufferManifest = MindsBufferManifest;

} // namespace ideam::core