#pragma once

/**
 * mind_common.h
 * Pure C++26 POD types, epistemic reference-catalogue primitives, and compile-time
 * deductive validity tables for `IdeamMinds` (`src/core/minds/`).
 *
 * Architectural Paradigm — Multi-Source Akashic Tomes & Reference Catalogues:
 *   1. **Akashic Tomes (`AkashicTomeHeaderPod` & `FactPod`)**:
 *      Rather than duplicating heavy fact structures inside every agent's head or assuming
 *      a single monolithic world dictionary, objective truth is partitioned into one or more
 *      **Akashic Tomes** (`tome_id`: e.g., `TOME_PHYSICAL_REALITY = 0`, `TOME_GAME_BOARD_RULES = 1`,
 *      `TOME_FACTION_OR_CULTURE = 2`, `TOME_SIMULATION_OR_DREAM = 3`). Each `FactPod` (32 bytes)
 *      is an immutable or versioned ground-truth entry within a specific Akashic Tome.
 *   2. **Epistemic Reference Catalogue (`BeliefPod`, 32 bytes + `BeliefCatalogueEntryPod`, 16 bytes)**:
 *      Every agent's internal knowledge base is a lightweight **Reference Catalogue** (`SPARSE_SET` /
 *      CSR overlay) indexing into those Akashic Tomes. Critically, a catalogue entry stores both:
 *        - `tome_id` (`believed_tome_id` vs `true_tome_id`),
 *        - `catalogued_fact_id` (the slot/concept the agent *thinks* they are filing this under), and
 *        - `true_fact_id` (the actual Akashic Tome fact row that generated or anchors the memory).
 *      When `catalogued_fact_id != true_fact_id` (or `believed_tome_id != true_tome_id`), the agent
 *      has **misreferenced** or **conflated** the fact (e.g., mistaking twin A for twin B in the dark,
 *      attributing a sound from the Study to the Conservatory, or filing a metaphorical clue under a
 *      literal rule).
 *   3. **Unification of Induction, Abduction, Deduction & Intuition (`Reason.cs` + `InformationMatrix.cs`)**:
 *      - `confidence` (`[0.0, 1.0]`): Accumulated via **Induction** (`ApplyEvidence` / `EvidencePod`).
 *      - `acceptance` (`[0.0, 1.0]`): Established via **Abduction** (`ApplyObservation` / `HypothesisPod`).
 *      - `intuition_charge` (`[0.0, 1.0]`): Near-miss abductive accumulator (`InformationMatrix.cs`)
 *        that triggers a stochastic **Intuitive Leap** when crossing threshold.
 *      - `is_knowledge(ground_truth, threshold)`: True iff `!is_misreferenced() &&
 *        (believed_value == ground_truth.truth_value) && ((confidence + acceptance) >= threshold)`.
 *   4. **1-Cycle Deductive Syllogism LUT (`Deduction.cs`)**:
 *      Packs `(SyllogisticFigure << 6) | (MajorQuantifier << 4) | (MinorQuantifier << 2) | ConclusionQuantifier`
 *      into a single `uint8_t` (`0..255`) and evaluates Aristotelian validity in 1 CPU cycle via
 *      `consteval std::array<uint64_t, 4> SYLLOGISM_VALIDITY_LUT`.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace ideam::core {

// ============================================================================
// 1. Cognitive Wave Stages & Epistemic Fact Flags
// ============================================================================

enum class CognitiveWaveStage : uint8_t {
    WAVE_1_SENSORY_PERCEPTION         = 0, ///< Spatial/Event Ring -> wake observable fact_id bits
    WAVE_2_EMOTIONAL_APPRAISAL        = 1, ///< SIMD affect appraisal against instinctive traits
    WAVE_3_COGNITIVE_LATENCY_TRIAGE   = 2, ///< StimulusPriority (1..12), Abduction, Intuition & Induction
    WAVE_4_DELIBERATION_AND_DECEPTION = 3, ///< Deductive syllogisms, truth/withhold/lie synthesis & Discourse
    WAVE_5_CONTRADICTION_REFLECTION   = 4  ///< Epistemic reconciliation, trust mutation & Sememe morphology
};

enum FactFlagBits : uint16_t {
    FACT_FLAG_NONE            = 0u,
    FACT_FLAG_PUBLIC          = 1u << 0, ///< Widely observable / common-sense baseline fact
    FACT_FLAG_SECRET          = 1u << 1, ///< Restricted / covert fact
    FACT_FLAG_ALIBI_RECORD    = 1u << 2, ///< Self-fact recorded when an agent emits a fabricated claim
    FACT_FLAG_CONTRADICTED    = 1u << 3, ///< Flagged during Wave 5 when a claim contradicts high-confidence belief
    FACT_FLAG_MISREFERENCED   = 1u << 4, ///< Catalogue entry points to `catalogued_fact_id != true_fact_id`
    FACT_FLAG_INTUITIVE_LEAP  = 1u << 5, ///< Woken via near-miss abductive intuition (`intuition_charge`)
    FACT_FLAG_DEDUCTIVE_CHAIN = 1u << 6, ///< Derived via valid categorical or transitive deduction
    FACT_FLAG_IMMUTABLE_RULE  = 1u << 7  ///< Invariant law/rule of a `GameBoard` or physical tome
};

/**
 * Built-in Akashic Tome domains (`uint16_t` `tome_id` supports up to 65,535 simultaneous
 * sources of ground truth across `IdeamGames`, `IdeamNarratives`, and nested simulations).
 */
enum AkashicTomeDomain : uint16_t {
    TOME_PHYSICAL_REALITY    = 0u, ///< Primary physical/narrative world ground truth
    TOME_GAME_BOARD_RULES    = 1u, ///< Canonical `IdeamGames` / `GameBoard` rule & state truth
    TOME_FACTION_OR_CULTURE  = 2u, ///< Shared institutional / cultural canon or codex
    TOME_SIMULATION_OR_DREAM = 3u, ///< Diegetic sub-game, illusion, or hypothetical simulation tome
    TOME_USER_CUSTOM_START   = 16u
};

/**
 * Packed 64-bit composite key uniquely identifying a fact across multiple Akashic Tomes:
 * Upper 32 bits = `tome_id` (or domain/version), Lower 32 bits = `fact_id`.
 */
struct alignas(8) AkashicFactKey {
    uint32_t fact_id = 0u;
    uint16_t tome_id = static_cast<uint16_t>(TOME_PHYSICAL_REALITY);
    uint16_t flags   = 0u;

    [[nodiscard]] constexpr uint64_t packed() const noexcept {
        return (static_cast<uint64_t>(tome_id) << 32u) | static_cast<uint64_t>(fact_id);
    }

    [[nodiscard]] constexpr bool operator==(const AkashicFactKey &p_other) const noexcept {
        return fact_id == p_other.fact_id && tome_id == p_other.tome_id;
    }
};
static_assert(sizeof(AkashicFactKey) == 8, "AkashicFactKey must be 8 bytes.");

/**
 * AkashicTomeHeaderPod (16 bytes)
 * Metadata descriptor for an individual Akashic Tome (source of objective truth) registered
 * in `InformationMatrixDOD` / `MindDOD`.
 */
struct alignas(8) AkashicTomeHeaderPod {
    uint16_t tome_id           = static_cast<uint16_t>(TOME_PHYSICAL_REALITY);
    uint16_t domain_flags      = 0u;
    uint32_t fact_slice_offset = 0u;   ///< Start row in `WorldGroundTruthFactTable` when flattened
    uint32_t fact_slice_count  = 0u;   ///< Active fact count in this tome
    float    authority_weight  = 1.0f; ///< Epistemic authority when multiple tomes conflict
};
static_assert(sizeof(AkashicTomeHeaderPod) == 16, "AkashicTomeHeaderPod must be 16 bytes.");

// ============================================================================
// 2. Objective Ground Truth (`FactPod`, 32B) & Subjective Reference Catalogue (`BeliefPod`, 32B)
// ============================================================================

/**
 * Compact 32-byte Objective Ground-Truth Fact record stored inside an Akashic Tome
 * (`InformationMatrixDOD` / `WorldGroundTruthFactTable`).
 *
 * Member ordering is strictly arranged (`int64_t truth_value` at offset 8) so there is
 * zero internal compiler padding (32 bytes on both MSVC and GCC/Clang).
 */
struct alignas(8) FactPod {
    uint32_t fact_id       = 0u;             ///< Offset 0  (4B)
    uint32_t subject_id    = 0u;             ///< Offset 4  (4B)
    int64_t  truth_value   = 0;              ///< Offset 8  (8B)
    uint32_t location_id   = 0u;             ///< Offset 16 (4B)
    float    timestamp     = 0.0f;           ///< Offset 20 (4B)
    float    base_salience = 1.0f;           ///< Offset 24 (4B)
    uint16_t predicate_id  = 0u;             ///< Offset 28 (2B)
    uint16_t flags         = FACT_FLAG_NONE; ///< Offset 30 (2B: Low 8 bits = FactFlagBits; High 8 bits = inline tome_id)

    [[nodiscard]] constexpr uint16_t get_tome_id() const noexcept {
        return static_cast<uint16_t>((flags >> 8u) & 0x00FFu);
    }

    constexpr void set_tome_id(uint16_t p_tome_id) noexcept {
        flags = static_cast<uint16_t>((flags & 0x00FFu) | ((p_tome_id & 0x00FFu) << 8u));
    }

    [[nodiscard]] constexpr uint16_t get_flag_bits() const noexcept {
        return static_cast<uint16_t>(flags & 0x00FFu);
    }

    constexpr void set_flag_bits(uint16_t p_flag_bits) noexcept {
        flags = static_cast<uint16_t>((flags & 0xFF00u) | (p_flag_bits & 0x00FFu));
    }

    [[nodiscard]] constexpr bool is_sound(int64_t p_sememe_or_value) const noexcept {
        return truth_value == p_sememe_or_value;
    }
};
static_assert(sizeof(FactPod) == 32, "FactPod must remain exactly 32 bytes.");

/**
 * Compact 32-byte Subjective Per-Agent Reference Catalogue record (`BeliefPod`).
 *
 * Unifies `Reason.cs` (`Belief`: `Fact`, `Confidence`, `Acceptance`, `KnowledgeThreshold`),
 * `InformationMatrix.cs` (`Intuition` near-miss accumulator), `UnityMind.cs` (`StimulusPriority`),
 * and the **Akashic Tome Reference Catalogue** model:
 *   - `fact_id` (`catalogued_fact_id`): The fact slot the agent indexes this belief under.
 *   - `source_agent_id`: Source attribution (`0` = direct sensory observation, `>0` = speaker agent ID).
 *   - `believed_value`: Subjective value or misattributed target truth value.
 *   - `confidence`: Inductive evidence strength `[0.0, 1.0]` (`ApplyEvidence`).
 *   - `acceptance`: Abductive hypothesis explanatory fit `[0.0, 1.0]` (`ApplyObservation`).
 *   - `emotional_charge`: Affective salience from Wave 2 appraisal.
 *   - `intuition_charge`: Near-miss abductive accumulator triggering an **Intuitive Leap**.
 */
struct alignas(8) BeliefPod {
    uint32_t fact_id          = 0u;   ///< Offset 0  (4B): Catalogued fact ID in the agent's mental reference table
    uint32_t source_agent_id  = 0u;   ///< Offset 4  (4B): Source agent ID (0 = direct sensory observation)
    int64_t  believed_value   = 0;    ///< Offset 8  (8B): Subjective value or misattributed target truth value
    float    confidence       = 1.0f; ///< Offset 16 (4B): Inductive evidence strength [0.0, 1.0] (`ApplyEvidence`)
    float    acceptance       = 0.0f; ///< Offset 20 (4B): Abductive hypothesis explanatory fit [0.0, 1.0] (`ApplyObservation`)
    float    emotional_charge = 0.0f; ///< Offset 24 (4B): Affective salience from Wave 2 appraisal
    float    intuition_charge = 0.0f; ///< Offset 28 (4B): Near-miss abductive accumulator triggering Intuitive Leap

    // --- Epistemic Reference & Knowledge Evaluation (`Reason.cs` Port) ---

    /**
     * Evaluates `Reason.cs` `Belief.IsKnowledge`:
     *   `fact.Sound && (Confidence + Acceptance) >= KnowledgeThreshold`
     * In the Reference Catalogue model, true knowledge requires that the agent's believed value
     * matches the Akashic Tome's `p_ground_truth` AND their combined inductive `confidence` +
     * abductive `acceptance` meets or exceeds `p_knowledge_threshold`.
     */
    [[nodiscard]] constexpr bool is_knowledge(
        int64_t p_ground_truth,
        float p_knowledge_threshold = 1.0f
    ) const noexcept {
        return (believed_value == p_ground_truth) &&
               ((confidence + acceptance) >= p_knowledge_threshold);
    }

    /**
     * Overload validating both ground-truth value soundness AND reference fidelity
     * (`fact_id == p_true_fact_id`), so a right guess filed under a misreferenced Akashic Tome
     * entry (Gettier case / misattribution) is not false-positived as true knowledge.
     */
    [[nodiscard]] constexpr bool is_knowledge(
        const FactPod &p_true_fact,
        uint32_t p_true_fact_id,
        float p_knowledge_threshold = 1.0f
    ) const noexcept {
        return (fact_id == p_true_fact_id) &&
               (believed_value == p_true_fact.truth_value) &&
               ((confidence + acceptance) >= p_knowledge_threshold);
    }

    /**
     * Applies inductive evidence strength (`Induction.cs` / `Reason.cs::ApplyEvidence`)
     * using bounded asymptotic accumulation so repeated evidence approaches 1.0 smoothly.
     */
    constexpr void apply_inductive_evidence(float p_evidence_strength, float p_cogence_weight = 1.0f) noexcept {
        const float delta = std::clamp(p_evidence_strength * p_cogence_weight, -1.0f, 1.0f);
        if (delta >= 0.0f) {
            confidence = std::clamp(confidence + (1.0f - confidence) * delta, 0.0f, 1.0f);
        } else {
            confidence = std::clamp(confidence * (1.0f + delta), 0.0f, 1.0f);
        }
    }

    /**
     * Applies an abductive observation check (`Abduction.cs` / `InformationMatrix.cs`):
     * If `p_clarity * p_hypothesis_consistency` meets `p_acceptance_floor`, boosts `acceptance`.
     * Otherwise, if it falls within `p_intuition_margin` (near-miss), increments `intuition_charge`!
     */
    constexpr bool apply_abductive_observation(
        float p_clarity,
        float p_hypothesis_consistency,
        float p_acceptance_floor = 0.5f,
        float p_intuition_margin = 0.25f
    ) noexcept {
        const float score = std::clamp(p_clarity * p_hypothesis_consistency, 0.0f, 1.0f);
        if (score >= p_acceptance_floor) {
            acceptance = std::clamp(std::max(acceptance, score), 0.0f, 1.0f);
            return true;
        }
        if (score + p_intuition_margin >= p_acceptance_floor) {
            const float proximity = 1.0f - ((p_acceptance_floor - score) / std::max(0.0001f, p_intuition_margin));
            intuition_charge = std::clamp(intuition_charge + 0.25f * proximity, 0.0f, 1.0f);
        }
        return false;
    }
};
static_assert(sizeof(BeliefPod) == 32, "BeliefPod must remain exactly 32 bytes.");

/**
 * BeliefCatalogueEntryPod (16 bytes)
 * Companion reference-routing header stored alongside `BeliefPod` in `InformationMatrixDOD`
 * and `MindDOD::AgentBeliefSparseTable` to explicitly track multi-tome provenance and
 * misreferencing (`catalogued_fact_id != true_fact_id` or `believed_tome_id != true_tome_id`).
 */
struct alignas(8) BeliefCatalogueEntryPod {
    uint32_t catalogued_fact_id = 0u; ///< Fact ID in the agent's subjective mental index (4B)
    uint32_t true_fact_id       = 0u; ///< Actual fact ID in the Akashic Tome (differs when misreferenced!) (4B)
    uint16_t believed_tome_id   = static_cast<uint16_t>(TOME_PHYSICAL_REALITY); ///< Which tome the agent thinks this belongs to (2B)
    uint16_t true_tome_id       = static_cast<uint16_t>(TOME_PHYSICAL_REALITY); ///< Which tome actually sourced the observation (2B)
    uint8_t  stimulus_priority  = 1u; ///< `StimulusPriority` (1..12) from Wave 3 Latency Triage (1B)
    uint8_t  catalogue_flags    = 0u; ///< `FactFlagBits` (e.g., `FACT_FLAG_MISREFERENCED`) (1B)
    uint16_t sememe_id          = 0u; ///< Primary semantic concept tag (`Sememe.cs`) (2B)

    [[nodiscard]] constexpr bool is_misreferenced() const noexcept {
        return (catalogued_fact_id != true_fact_id) || (believed_tome_id != true_tome_id);
    }
};
static_assert(sizeof(BeliefCatalogueEntryPod) == 16, "BeliefCatalogueEntryPod must be 16 bytes.");

// ============================================================================
// 3. Deductive Reasoning: `LogicalQuantifier`, `SyllogisticFigure` & 256-Bit LUT (`Deduction.cs`)
// ============================================================================

/**
 * Port of `Ideam.Reason.Quantifier` (`Reason.cs` / `Deduction.cs`).
 * Represents categorical scope in Aristotelian syllogisms:
 *   - `ALL` (Universal Affirmative, A: "All M are P")
 *   - `NONE` (Universal Negative, E: "No M are P")
 *   - `SOME` (Particular Affirmative, I: "Some M are P")
 *   - `NOT_SOME` (Particular Negative, O: "Some M are not P")
 */
enum class LogicalQuantifier : uint8_t {
    ALL      = 0u,
    NONE     = 1u,
    SOME     = 2u,
    NOT_SOME = 3u
};

/**
 * Port of `Ideam.Reason.SyllogisticFigure` (`Reason.cs` / `Deduction.cs`).
 * Encodes the placement of the Middle term (M) relative to Major (P) and Minor (S) terms:
 *   - `MP_SM = 0` (Figure 1: Major = M-P, Minor = S-M)
 *   - `PM_SM = 1` (Figure 2: Major = P-M, Minor = S-M)
 *   - `MP_MS = 2` (Figure 3: Major = M-P, Minor = M-S)
 *   - `PM_MS = 3` (Figure 4: Major = P-M, Minor = M-S)
 */
enum class SyllogisticFigure : uint8_t {
    MP_SM = 0u,
    PM_SM = 1u,
    MP_MS = 2u,
    PM_MS = 3u
};

/**
 * Packs `(figure, major_quantifier, minor_quantifier, conclusion_quantifier)` into a
 * single 8-bit lookup index (`0..255`).
 */
[[nodiscard]] constexpr uint8_t pack_syllogistic_form(
    SyllogisticFigure p_figure,
    LogicalQuantifier p_major_q,
    LogicalQuantifier p_minor_q,
    LogicalQuantifier p_conclusion_q
) noexcept {
    return static_cast<uint8_t>(
        ((static_cast<uint8_t>(p_figure) & 0x03u) << 6u) |
        ((static_cast<uint8_t>(p_major_q) & 0x03u) << 4u) |
        ((static_cast<uint8_t>(p_minor_q) & 0x03u) << 2u) |
        (static_cast<uint8_t>(p_conclusion_q) & 0x03u)
    );
}

/**
 * Exact C++26 `consteval` port of `SyllogisticProposition.IsValid` from `Deduction.cs`
 * (lines 265–388), covering all 24 unconditionally + conditionally valid Aristotelian moods
 * across all 4 figures (Barbara, Barbari, Celarent, Celaront, Darii, Ferio, Cesare, Cesaro,
 * Camestres, Camestrop, Festino, Baroco, Darapti, Disamis, Datisi, Felapton, Bocardo, Ferison,
 * Bramantip, Camenes, Camenop, Dimaris, Fesapo, Fresison).
 */
[[nodiscard]] consteval bool evaluate_syllogism_validity_reference(
    SyllogisticFigure p_figure,
    LogicalQuantifier p_major,
    LogicalQuantifier p_minor,
    LogicalQuantifier p_conclusion
) noexcept {
    switch (p_figure) {
        case SyllogisticFigure::MP_SM:
            if (p_major == LogicalQuantifier::ALL) {
                if (p_minor == LogicalQuantifier::ALL) {
                    if (p_conclusion == LogicalQuantifier::ALL ||
                        p_conclusion == LogicalQuantifier::SOME) {
                        return true; // Barbara (AAA-1), Barbari (AAI-1)
                    }
                }
                if (p_minor == LogicalQuantifier::SOME &&
                    p_conclusion == LogicalQuantifier::SOME) {
                    return true; // Darii (AII-1)
                }
            } else if (p_major == LogicalQuantifier::NONE) {
                if (p_minor == LogicalQuantifier::ALL) {
                    if (p_conclusion == LogicalQuantifier::NONE ||
                        p_conclusion == LogicalQuantifier::NOT_SOME) {
                        return true; // Celarent (EAE-1), Celaront (EAO-1)
                    }
                } else if (p_minor == LogicalQuantifier::SOME &&
                           p_conclusion == LogicalQuantifier::NOT_SOME) {
                    return true; // Ferio (EIO-1)
                }
            }
            break;

        case SyllogisticFigure::PM_SM:
            if (p_major == LogicalQuantifier::ALL) {
                if (p_minor == LogicalQuantifier::NONE) {
                    if (p_conclusion == LogicalQuantifier::NONE ||
                        p_conclusion == LogicalQuantifier::NOT_SOME) {
                        return true; // Camestres (AEE-2), Camestrop (AEO-2)
                    }
                } else if (p_minor == LogicalQuantifier::NOT_SOME &&
                           p_conclusion == LogicalQuantifier::NOT_SOME) {
                    return true; // Baroco (AOO-2)
                }
            } else if (p_major == LogicalQuantifier::NONE) {
                if (p_minor == LogicalQuantifier::ALL) {
                    if (p_conclusion == LogicalQuantifier::NONE ||
                        p_conclusion == LogicalQuantifier::NOT_SOME) {
                        return true; // Cesare (EAE-2), Cesaro (EAO-2)
                    }
                } else if (p_minor == LogicalQuantifier::SOME &&
                           p_conclusion == LogicalQuantifier::NOT_SOME) {
                    return true; // Festino (EIO-2)
                }
            }
            break;

        case SyllogisticFigure::MP_MS:
            if (p_major == LogicalQuantifier::ALL) {
                if (p_conclusion == LogicalQuantifier::SOME) {
                    if (p_minor == LogicalQuantifier::ALL ||
                        p_minor == LogicalQuantifier::SOME) {
                        return true; // Darapti (AAI-3), Datisi (AII-3)
                    }
                }
            } else if (p_major == LogicalQuantifier::NONE) {
                if (p_conclusion == LogicalQuantifier::NOT_SOME) {
                    if (p_minor == LogicalQuantifier::ALL ||
                        p_minor == LogicalQuantifier::SOME) {
                        return true; // Felapton (EAO-3), Ferison (EIO-3)
                    }
                }
            } else if (p_major == LogicalQuantifier::SOME &&
                       p_minor == LogicalQuantifier::ALL &&
                       p_conclusion == LogicalQuantifier::SOME) {
                return true; // Disamis (IAI-3)
            } else if (p_major == LogicalQuantifier::NOT_SOME &&
                       p_minor == LogicalQuantifier::ALL &&
                       p_conclusion == LogicalQuantifier::NOT_SOME) {
                return true; // Bocardo (OAO-3)
            }
            break;

        case SyllogisticFigure::PM_MS:
            if (p_major == LogicalQuantifier::ALL) {
                if (p_minor == LogicalQuantifier::NONE) {
                    if (p_conclusion == LogicalQuantifier::NONE ||
                        p_conclusion == LogicalQuantifier::NOT_SOME) {
                        return true; // Camenes (AEE-4), Camenop (AEO-4)
                    }
                } else if (p_minor == LogicalQuantifier::ALL &&
                           p_conclusion == LogicalQuantifier::SOME) {
                    return true; // Bramantip (AAI-4)
                }
            } else if (p_major == LogicalQuantifier::NONE) {
                if (p_conclusion == LogicalQuantifier::NOT_SOME) {
                    if (p_minor == LogicalQuantifier::SOME ||
                        p_minor == LogicalQuantifier::ALL) {
                        return true; // Fesapo (EAO-4), Fresison (EIO-4)
                    }
                }
            } else if (p_major == LogicalQuantifier::SOME &&
                       p_minor == LogicalQuantifier::ALL &&
                       p_conclusion == LogicalQuantifier::SOME) {
                return true; // Dimaris (IAI-4)
            }
            break;
    }
    return false;
}

/**
 * Branchless 256-bit (`4 x uint64_t`) compile-time lookup table encoding all 256
 * `(SyllogisticFigure x MajorQuantifier x MinorQuantifier x ConclusionQuantifier)` combinations.
 */
inline constexpr std::array<uint64_t, 4> SYLLOGISM_VALIDITY_LUT = []() consteval {
    std::array<uint64_t, 4> lut{0ULL, 0ULL, 0ULL, 0ULL};
    for (uint32_t idx = 0; idx < 256u; ++idx) {
        const auto fig   = static_cast<SyllogisticFigure>((idx >> 6u) & 0x03u);
        const auto major = static_cast<LogicalQuantifier>((idx >> 4u) & 0x03u);
        const auto minor = static_cast<LogicalQuantifier>((idx >> 2u) & 0x03u);
        const auto concl = static_cast<LogicalQuantifier>(idx & 0x03u);
        if (evaluate_syllogism_validity_reference(fig, major, minor, concl)) {
            lut[idx >> 6u] |= (1ULL << (idx & 63u));
        }
    }
    return lut;
}();

/**
 * Evaluates Aristotelian syllogism validity in 1 CPU cycle via bit-test into `SYLLOGISM_VALIDITY_LUT`.
 */
[[nodiscard]] constexpr bool is_syllogism_valid(uint8_t p_packed_index) noexcept {
    return (SYLLOGISM_VALIDITY_LUT[p_packed_index >> 6u] & (1ULL << (p_packed_index & 63u))) != 0ULL;
}

[[nodiscard]] constexpr bool is_syllogism_valid(
    SyllogisticFigure p_figure,
    LogicalQuantifier p_major_q,
    LogicalQuantifier p_minor_q,
    LogicalQuantifier p_conclusion_q
) noexcept {
    return is_syllogism_valid(pack_syllogistic_form(p_figure, p_major_q, p_minor_q, p_conclusion_q));
}

// Compile-time verification of Aristotelian validity LUT
static_assert(is_syllogism_valid(SyllogisticFigure::MP_SM, LogicalQuantifier::ALL, LogicalQuantifier::ALL, LogicalQuantifier::ALL), "Barbara (AAA-1) must be valid.");
static_assert(is_syllogism_valid(SyllogisticFigure::MP_SM, LogicalQuantifier::NONE, LogicalQuantifier::ALL, LogicalQuantifier::NONE), "Celarent (EAE-1) must be valid.");
static_assert(!is_syllogism_valid(SyllogisticFigure::MP_SM, LogicalQuantifier::SOME, LogicalQuantifier::SOME, LogicalQuantifier::SOME), "Two particular premises (III-1) must be invalid.");

// ============================================================================
// 4. `StimulusPriority` 12-Tier Latency Triage Lattice (`UnityMind.cs`)
// ============================================================================

/**
 * Port of `Ideam.Unity.Metaphysics.Mind.StimulusPriority` (`UnityMind.cs`, lines 307–325).
 * Categorizes incoming sensory observations across the 2x2x4 lattice of
 * `Familiarity (Old/New) x Delta (Unchanged/Changed) x Valence (Neutral/Beneficial/Unknown/Threat)`.
 */
enum class StimulusPriority : uint8_t {
    NONE                     = 0u,
    OLD_NEUTRAL_UNCHANGED    = 1u,
    OLD_BENEFICIAL_UNCHANGED = 2u,
    OLD_UNKNOWN_UNCHANGED    = 3u,

    OLD_NEUTRAL_CHANGED      = 4u,
    OLD_BENEFICIAL_CHANGED   = 5u,
    
    NEW_NEUTRAL              = 6u,
    NEW_BENEFICIAL           = 7u,

    OLD_THREAT_UNCHANGED     = 8u,
    OLD_UNKNOWN_CHANGED      = 9u,
    OLD_THREAT_CHANGED       = 10u,

    NEW_UNKNOWN              = 11u,
    NEW_THREAT               = 12u
};

enum class StimulusValence : uint8_t {
    NEUTRAL    = 0u,
    BENEFICIAL = 1u,
    UNKNOWN    = 2u,
    THREAT     = 3u
};

/**
 * Branchless classification mapping `(is_new, has_changed, valence)`
 * directly to the 12 `StimulusPriority` tiers from `UnityMind.cs`.
 */
[[nodiscard]] constexpr StimulusPriority classify_stimulus_priority(
    bool p_is_new,
    bool p_has_changed,
    StimulusValence p_valence
) noexcept {
    if (p_is_new) {
        switch (p_valence) {
            case StimulusValence::NEUTRAL:    return StimulusPriority::NEW_NEUTRAL;
            case StimulusValence::BENEFICIAL: return StimulusPriority::NEW_BENEFICIAL;
            case StimulusValence::UNKNOWN:    return StimulusPriority::NEW_UNKNOWN;
            case StimulusValence::THREAT:     return StimulusPriority::NEW_THREAT;
        }
    } else if (p_has_changed) {
        switch (p_valence) {
            case StimulusValence::NEUTRAL:    return StimulusPriority::OLD_NEUTRAL_CHANGED;
            case StimulusValence::BENEFICIAL: return StimulusPriority::OLD_BENEFICIAL_CHANGED;
            case StimulusValence::UNKNOWN:    return StimulusPriority::OLD_UNKNOWN_CHANGED;
            case StimulusValence::THREAT:     return StimulusPriority::OLD_THREAT_CHANGED;
        }
    } else {
        switch (p_valence) {
            case StimulusValence::NEUTRAL:    return StimulusPriority::OLD_NEUTRAL_UNCHANGED;
            case StimulusValence::BENEFICIAL: return StimulusPriority::OLD_BENEFICIAL_UNCHANGED;
            case StimulusValence::UNKNOWN:    return StimulusPriority::OLD_UNKNOWN_UNCHANGED;
            case StimulusValence::THREAT:     return StimulusPriority::OLD_THREAT_UNCHANGED;
        }
    }
    return StimulusPriority::OLD_NEUTRAL_UNCHANGED;
}

// ============================================================================
// 5. Sensory Perception, Abduction, Induction, Explanation & Discourse PODs
// ============================================================================

/**
 * Sensory modalities ported from `UnityMind.cs` (`UnitySight`, `UnityHearing`,
 * `UnityTouch`, `UnityInteroception` / `UnityNumbness`).
 */
enum class SensoryModality : uint8_t {
    SIGHT         = 0u, ///< Visual ray/frustum perception (`RaySensor.seenDuration`, `UnityVisualNoise`)
    HEARING       = 1u, ///< Acoustic signal reception (`AudioSensor`, `UnityAudioNoise`, `CymaticMap`)
    TOUCH         = 2u, ///< Haptic/collision contact (`CollisionSensor`, `UnityTouchNoise`)
    INTEROCEPTION = 3u  ///< Internal sense-of-self / proprioception (`UnityInteroception`, `UnityNumbness`)
};

/**
 * SensoryEventPod (32 bytes, `alignas(8)`)
 * Wave 1 input ring payload (`SensoryObservationEventRing`) representing a raw sensory
 * signal before noise attenuation and reference-catalogue indexing.
 *
 * Supports Akashic Tome misreferencing right at the sensory boundary:
 *   If `perceived_fact_id != 0 && perceived_fact_id != target_fact_id` (or when clarity
 *   is degraded by `environmental_noise`), the agent files the observation under
 *   `perceived_fact_id` while `target_fact_id` records the true Akashic Tome source!
 */
struct alignas(8) SensoryEventPod {
    uint32_t sensor_agent_id     = 0u;   ///< Offset 0  (4B): Observing agent slot ID
    uint32_t target_fact_id      = 0u;   ///< Offset 4  (4B): True Akashic Tome `fact_id` being emitted
    int64_t  sensed_value        = 0;    ///< Offset 8  (8B): Raw value or perceived value
    float    raw_clarity         = 1.0f; ///< Offset 16 (4B): Continuous `seenDuration * percentTransmitted` [0.0, 1.0]
    float    environmental_noise = 0.0f; ///< Offset 20 (4B): `INoise` attenuation (`Visual`, `Audio`, `Touch`, `Numbness`)
    uint32_t perceived_fact_id   = 0u;   ///< Offset 24 (4B): Catalogued fact ID (0 = same as `target_fact_id`; != means misreferenced)
    uint16_t tome_id             = static_cast<uint16_t>(TOME_PHYSICAL_REALITY); ///< Offset 28 (2B): Akashic Tome ID
    SensoryModality modality     = SensoryModality::SIGHT; ///< Offset 30 (1B): Modality channel
    uint8_t  flags               = 0u;   ///< Offset 31 (1B): Optional sensory flags

    [[nodiscard]] constexpr float effective_clarity() const noexcept {
        return std::clamp(raw_clarity * (1.0f - std::clamp(environmental_noise, 0.0f, 1.0f)), 0.0f, 1.0f);
    }

    [[nodiscard]] constexpr uint32_t resolved_catalogued_fact_id() const noexcept {
        return (perceived_fact_id != 0u) ? perceived_fact_id : target_fact_id;
    }
};
static_assert(sizeof(SensoryEventPod) == 32, "SensoryEventPod must be 32 bytes.");

/**
 * ObservationPod (16 bytes, `alignas(8)`)
 * Decoded epistemic premise used in Abductive Reasoning (`Abduction.cs`: `Observation { Int sememe, clarity; }`).
 */
struct alignas(8) ObservationPod {
    uint32_t fact_id         = 0u;   ///< Catalogued fact ID (4B)
    uint32_t sememe_id       = 0u;   ///< Semantic concept ID (`Observation.Sememe()`) (4B)
    float    clarity         = 1.0f; ///< Post-noise decoded clarity [0.0, 1.0] (`Observation.Clarity()`) (4B)
    uint16_t tome_id         = static_cast<uint16_t>(TOME_PHYSICAL_REALITY); ///< Akashic Tome ID (2B)
    SensoryModality modality = SensoryModality::SIGHT; ///< Originating modality (1B)
    uint8_t  priority        = static_cast<uint8_t>(StimulusPriority::NEW_NEUTRAL); ///< `StimulusPriority` (1B)
};
static_assert(sizeof(ObservationPod) == 16, "ObservationPod must be 16 bytes.");

/**
 * HypothesisPod (16 bytes, `alignas(8)`)
 * Abductive conclusion candidate (`Abduction.cs`: `Hypothesis { Int sememe; int theoryGroup; }`
 * + `AbductiveArgument.Consistency`).
 */
struct alignas(8) HypothesisPod {
    uint32_t hypothesis_fact_id        = 0u;   ///< Target fact ID in the Akashic Tome / belief table (4B)
    uint32_t theory_group_id           = 0u;   ///< Competing hypothesis cluster (`IHypothesis.TheoryGroup`) (4B)
    uint32_t required_observation_mask = 0u;   ///< Bitmask of supporting observation slots (4B)
    float    consistency_score         = 1.0f; ///< Explanatory fit [0.0, 1.0] (`IAbduction.Consistency`) (4B)
};
static_assert(sizeof(HypothesisPod) == 16, "HypothesisPod must be 16 bytes.");

/**
 * EvidencePod (16 bytes, `alignas(8)`)
 * Inductive premise & probable conclusion accumulator (`Induction.cs`: `Evidence`, `ProbableConclusion`, `Cogence`).
 */
struct alignas(8) EvidencePod {
    uint32_t target_fact_id = 0u;   ///< Target generalization / probable conclusion fact ID (4B)
    uint32_t sample_count   = 0u;   ///< Number of supporting observations accumulated across ticks (4B)
    float    strength       = 0.0f; ///< Inductive strength (`IEvidence.Strength` / `IProbability.Strength`) (4B)
    float    cogence        = 1.0f; ///< Soundness * strength (`IInduction.Cogence`) (4B)
};
static_assert(sizeof(EvidencePod) == 16, "EvidencePod must be 16 bytes.");

/**
 * SyllogisticPropositionPod (16 bytes, `alignas(8)`)
 * Flattened DOD representation of `Deduction.cs` `SyllogisticProposition` & `TransitiveInference`.
 */
struct alignas(8) SyllogisticPropositionPod {
    uint32_t major_premise_fact_id = 0u; ///< Antecedent categorical statement fact ID (4B)
    uint32_t minor_premise_fact_id = 0u; ///< Consequent categorical statement fact ID (4B)
    uint32_t conclusion_fact_id    = 0u; ///< Inferred categorical consequence fact ID (4B)
    uint8_t  packed_syllogism_form = 0u; ///< `pack_syllogistic_form(figure, major_q, minor_q, conclusion_q)` (1B)
    uint8_t  is_transitive         = 0u; ///< Non-zero if using `TransitiveInference` instead of categorical mood (1B)
    uint16_t tome_id               = static_cast<uint16_t>(TOME_PHYSICAL_REALITY); ///< Target Akashic Tome ID (2B)

    [[nodiscard]] constexpr bool is_valid_deduction() const noexcept {
        return (is_transitive != 0u) ? true : is_syllogism_valid(packed_syllogism_form);
    }
};
static_assert(sizeof(SyllogisticPropositionPod) == 16, "SyllogisticPropositionPod must be 16 bytes.");

/**
 * ExplanationPod (16 bytes, `alignas(8)`)
 * Flattened CSR header equivalent of `Reason.cs` `Explanation`
 * (`Fact[] facts; Statement[] causes; Consequence[] consequences;`), indexing into
 * `ConditionEvaluationTableBuffer` or `FactPod` slices to represent causal explanations and alibis.
 */
struct alignas(8) ExplanationPod {
    uint32_t fact_slice_offset       = 0u; ///< Start offset in explanation fact ID CSR array (4B)
    uint16_t fact_slice_count        = 0u; ///< Number of facts in this explanation (2B)
    uint16_t cause_slice_count       = 0u; ///< Number of causal statements (2B)
    uint32_t cause_slice_offset      = 0u; ///< Start offset in cause condition/fact CSR array (4B)
    uint16_t consequence_slice_count = 0u; ///< Number of consequences (2B)
    uint16_t owner_agent_id          = 0u; ///< Agent authoring or holding this explanation/alibi (2B)
};
static_assert(sizeof(ExplanationPod) == 16, "ExplanationPod must be 16 bytes.");

// ============================================================================
// 6. Spoken Dialogue, Prosody & Sememe Morphology (`Discourse.cs` & `Sememe.cs`)
// ============================================================================

enum class TurnConstructionUnitType : uint8_t {
    UNDEFINED  = 0u,
    LEXICAL    = 1u,
    PHRASAL    = 2u,
    CLAUSAL    = 3u,
    SENTENTIAL = 4u
};

enum class TurnResolutionType : uint8_t {
    UNRESOLVED   = 0u,
    INTONATIONAL = 1u, ///< Resolved through a shift in prosody (pitch or volume)
    GRAMMATICAL  = 2u, ///< Resolved through syntactic completion
    PRAGMATICAL  = 3u  ///< Resolved practically / situationally (`Discourse.cs`)
};

enum class DiscourseTransitionType : uint8_t {
    UNDEFINED   = 0u,
    GRAMMATICAL = 1u,
    PROSODIC    = 2u,
    PRAGMATIC   = 3u,
    NON_VERBAL  = 4u
};

/**
 * Sacks-Schegloff-Jefferson turn-allocation rules ported from `Discourse.TurnAllocation()` (`Discourse.cs`).
 */
enum class TurnAllocationRule : uint8_t {
    SPEAKER_SELECTS_NEXT = 0u, ///< Option 1a: Current speaker selects next speaker by name/gaze/context
    SELF_SELECT_FIRST    = 1u, ///< Option 1b: First starter among listeners claims the turn
    SPEAKER_CONTINUES    = 2u  ///< Option 1c: Current speaker continues with another TCU
};

/**
 * Port of `Ideam.Epistemology.Semantics.SememicForm` (`Semantics.cs` / `Sememe.cs`).
 */
enum class SememicForm : uint8_t {
    NOT_SET                = 0u,
    UNKNOWN                = 1u,
    PRIMARY_DENOTATION     = 2u,
    SECONDARY_DENOTATION   = 3u,
    ANALOGOUS_CONNOTATION  = 4u,
    EMOTIVE_CONNOTATION    = 5u,
    EVALUATIVE_CONNOTATION = 6u
};

/**
 * DiscourseTurnPod (32 bytes, `alignas(8)`)
 * Flattened DOD row for `Discourse.cs` (`Turn`, `TurnConstructionUnit`, `Utterance.prosody` `Spline2D`).
 */
struct alignas(8) DiscourseTurnPod {
    uint32_t discourse_id             = 0u;   ///< Conversation / Discourse instance ID (4B)
    uint32_t active_speaker_id        = 0u;   ///< Current speaking agent ID (4B)
    uint32_t selected_next_id         = 0u;   ///< Next speaker selected under Rule 1a (`INVALID_ID` if none) (4B)
    uint32_t prosody_spline_2d_id     = 0u;   ///< Handle to `Spline2D` (`X=Volume, Y=Pitch`) prosody curve (4B)
    float    percent_transmitted      = 0.0f; ///< Utterance transmission progress [0.0, 1.0] (4B)
    float    current_volume           = 1.0f; ///< Sampled `Spline2D.x` prosody volume (4B)
    float    current_pitch            = 1.0f; ///< Sampled `Spline2D.y` prosody pitch (4B)
    TurnConstructionUnitType tcu_type = TurnConstructionUnitType::SENTENTIAL; ///< 1B
    TurnResolutionType resolution     = TurnResolutionType::UNRESOLVED;       ///< 1B
    TurnAllocationRule allocation_rule= TurnAllocationRule::SPEAKER_SELECTS_NEXT; ///< 1B
    SememicForm sememic_form          = SememicForm::PRIMARY_DENOTATION;      ///< 1B
};
static_assert(sizeof(DiscourseTurnPod) == 32, "DiscourseTurnPod must be 32 bytes.");

// ============================================================================
// 7. Speech-Act Claims, Contradiction Events & Instinctive Trait Profiles
// ============================================================================

/**
 * Speech-act claim emitted during Wave 4 (Truth, Withholding, or Intentional Lie)
 * and consumed in Wave 5 (`SpeechClaimEventRing`).
 */
struct alignas(8) ClaimPod {
    uint32_t speaker_agent_id   = 0u;   ///< Offset 0  (4B): Speaking agent ID
    uint32_t listener_agent_id  = 0u;   ///< Offset 4  (4B): Target listener agent ID
    int64_t  claimed_value      = 0;    ///< Offset 8  (8B): Stated value (truthful belief or fabricated alibi)
    uint32_t fact_id            = 0u;   ///< Offset 16 (4B): Catalogued fact ID being discussed
    uint32_t referenced_fact_id = 0u;   ///< Offset 20 (4B): Underlying Akashic Tome fact ID (or alibi fact ID)
    float    claimed_confidence = 1.0f; ///< Offset 24 (4B): Stated confidence / rhetorical certainty
    uint16_t tome_id            = static_cast<uint16_t>(TOME_PHYSICAL_REALITY); ///< Offset 28 (2B): Referenced Akashic Tome ID
    uint8_t  sememic_form       = static_cast<uint8_t>(SememicForm::PRIMARY_DENOTATION); ///< Offset 30 (1B): Connotation
    bool     is_intentional_lie = false;///< Offset 31 (1B): True when speaker deliberately fabricated the claim
};
static_assert(sizeof(ClaimPod) == 32, "ClaimPod must be 32 bytes.");

/**
 * MindEpistemicEventPod (16 bytes, `alignas(8)`)
 * Emitted into `MindIntuitionAndContradictionRing` during Wave 3 (Intuitive Leap)
 * or Wave 5 (Contradiction / Lie Detection / Misreference Discovery) to notify
 * `NarrativeDOD` and `GameDOD`.
 */
enum MindEpistemicEventKind : uint8_t {
    MIND_EVENT_INTUITIVE_LEAP         = 0u, ///< Agent crossed `intuition_charge` threshold on a hypothesis
    MIND_EVENT_CONTRADICTION_DETECTED = 1u, ///< Heard claim contradicted high-confidence belief
    MIND_EVENT_MISREFERENCE_CORRECTED = 2u, ///< Agent realized `catalogued_fact_id != true_fact_id`
    MIND_EVENT_KNOWLEDGE_ATTAINED     = 3u  ///< Belief crossed `KnowledgeThreshold` and matches Akashic Tome
};

struct alignas(8) MindEpistemicEventPod {
    uint32_t agent_id       = 0u;   ///< Agent experiencing the insight or contradiction (4B)
    uint32_t fact_id        = 0u;   ///< Relevant fact ID (4B)
    uint32_t other_agent_id = 0u;   ///< Speaker or source agent ID (if contradiction) (4B)
    uint16_t tome_id        = static_cast<uint16_t>(TOME_PHYSICAL_REALITY); ///< Akashic Tome ID (2B)
    uint8_t  event_kind     = MIND_EVENT_INTUITIVE_LEAP; ///< `MindEpistemicEventKind` (1B)
    uint8_t  priority       = static_cast<uint8_t>(StimulusPriority::NEW_UNKNOWN); ///< (1B)
};
static_assert(sizeof(MindEpistemicEventPod) == 16, "MindEpistemicEventPod must be 16 bytes.");

/**
 * Per-agent instinctive trait & drive weights used in Waves 2, 3, and 4.
 */
struct alignas(4) InstinctiveTraitWeightsPod {
    float fear              = 0.5f;
    float curiosity         = 0.5f;
    float guilt             = 0.5f;
    float loyalty           = 0.5f;
    float self_preservation = 0.5f;
    float trust_in_listener = 0.5f;
};
static_assert(sizeof(InstinctiveTraitWeightsPod) == 24, "InstinctiveTraitWeightsPod must be 24 bytes.");

} // namespace ideam::core
