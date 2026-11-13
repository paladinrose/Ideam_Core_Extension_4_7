#pragma once

/**
 * information_matrix_dod.h
 * Pure C++26 Multi-Tome Akashic Ground-Truth Registry & Per-Agent Epistemic Reference Catalogue
 * (`src/core/minds/`).
 *
 * Reimagines the legacy `InformationMatrix.cs` in pure Data-Oriented Design:
 *   - Supports **Multiple Akashic Tomes** (`tome_id`: physical reality, `IdeamGames` board rules,
 *     lore archives, diegetic sub-simulations) so agents can inhabit worlds with multiple
 *     distinct sources of objective facts.
 *   - Models each agent's internal mind as a lightweight **Reference Catalogue** (`BeliefPod` +
 *     `BeliefCatalogueEntryPod`) that indexes into those Akashic Tomes, including first-class
 *     support for **misreferencing** (`catalogued_fact_id != true_fact_id` or wrong `tome_id`),
 *     inductive `confidence`, abductive `acceptance`, near-miss `intuition_charge`, and
 *     `KnowledgeThreshold` verification.
 */

#include "mind_common.h"
#include <unordered_map>
#include <vector>

namespace ideam::core {

class InformationMatrixDOD {
private:
    std::vector<AkashicTomeHeaderPod> akashic_tomes_;
    std::vector<FactPod> ground_truth_facts_;
    std::unordered_map<uint32_t, std::vector<BeliefPod>> agent_belief_overlays_;
    std::unordered_map<uint32_t, std::vector<BeliefCatalogueEntryPod>> agent_catalogue_entries_;
    std::vector<HypothesisPod> hypotheses_;
    std::vector<SyllogisticPropositionPod> syllogisms_;
    std::vector<ExplanationPod> explanations_;

    void _ensure_default_tome();

public:
    InformationMatrixDOD();
    ~InformationMatrixDOD() = default;

    void clear() noexcept;

    // ========================================================================
    // 1. Multi-Source Akashic Tomes (`AkashicTomeHeaderPod` & `FactPod`)
    // ========================================================================

    uint16_t register_akashic_tome(uint16_t p_tome_id = TOME_PHYSICAL_REALITY, float p_authority_weight = 1.0f, uint16_t p_domain_flags = 0u);
    [[nodiscard]] const AkashicTomeHeaderPod *find_akashic_tome(uint16_t p_tome_id) const noexcept;
    [[nodiscard]] const std::vector<AkashicTomeHeaderPod> &akashic_tomes() const noexcept { return akashic_tomes_; }

    uint32_t upsert_ground_truth_fact(const FactPod &p_fact, uint16_t p_tome_id = TOME_PHYSICAL_REALITY);
    [[nodiscard]] const FactPod *find_ground_truth_fact(uint32_t p_fact_id, uint16_t p_tome_id = TOME_PHYSICAL_REALITY) const noexcept;
    [[nodiscard]] const std::vector<FactPod> &ground_truth_facts() const noexcept { return ground_truth_facts_; }

    // ========================================================================
    // 2. Per-Agent Epistemic Reference Catalogue (Supports Misreferencing!)
    // ========================================================================

    void upsert_agent_belief(
        uint32_t p_agent_id,
        const BeliefPod &p_belief,
        uint32_t p_true_fact_id = 0u,
        uint16_t p_believed_tome_id = TOME_PHYSICAL_REALITY,
        uint16_t p_true_tome_id = TOME_PHYSICAL_REALITY,
        StimulusPriority p_priority = StimulusPriority::NEW_NEUTRAL
    );

    /**
     * Explicitly records a misreferenced belief in the agent's Reference Catalogue:
     * the agent observes or infers data originating from `(p_true_tome_id, p_true_fact_id)`
     * in the Akashic Tome, but files it under `(p_believed_tome_id, p_catalogued_fact_id)`.
     */
    void upsert_misreferenced_belief(
        uint32_t p_agent_id,
        uint32_t p_catalogued_fact_id,
        uint32_t p_true_fact_id,
        int64_t p_believed_value,
        float p_confidence,
        float p_acceptance = 0.0f,
        uint16_t p_believed_tome_id = TOME_PHYSICAL_REALITY,
        uint16_t p_true_tome_id = TOME_PHYSICAL_REALITY,
        uint32_t p_source_agent_id = 0u
    );

    [[nodiscard]] const BeliefPod *find_agent_belief(uint32_t p_agent_id, uint32_t p_catalogued_fact_id) const noexcept;
    [[nodiscard]] BeliefPod *find_agent_belief_mut(uint32_t p_agent_id, uint32_t p_catalogued_fact_id) noexcept;
    [[nodiscard]] const BeliefCatalogueEntryPod *find_agent_catalogue_entry(uint32_t p_agent_id, uint32_t p_catalogued_fact_id) const noexcept;

    [[nodiscard]] const std::vector<BeliefPod> *get_agent_beliefs(uint32_t p_agent_id) const noexcept;
    [[nodiscard]] const std::vector<BeliefCatalogueEntryPod> *get_agent_catalogue(uint32_t p_agent_id) const noexcept;

    /**
     * Checks whether the agent's belief about `p_catalogued_fact_id` qualifies as true **Knowledge**:
     *   1. The catalogue reference is NOT misreferenced (`catalogued_fact_id == true_fact_id` and `believed_tome_id == true_tome_id`).
     *   2. The believed value matches the Akashic Tome's objective `truth_value` (`Fact.Sound`).
     *   3. `(confidence + acceptance) >= p_knowledge_threshold`.
     */
    [[nodiscard]] bool is_agent_belief_knowledge(
        uint32_t p_agent_id,
        uint32_t p_catalogued_fact_id,
        float p_knowledge_threshold = 1.0f
    ) const noexcept;

    [[nodiscard]] bool is_agent_belief_misreferenced(
        uint32_t p_agent_id,
        uint32_t p_catalogued_fact_id
    ) const noexcept;

    // ========================================================================
    // 3. Reasoning Registry (Hypotheses, Syllogisms & Explanations)
    // ========================================================================

    uint32_t register_hypothesis(const HypothesisPod &p_hypothesis);
    [[nodiscard]] const std::vector<HypothesisPod> &hypotheses() const noexcept { return hypotheses_; }

    uint32_t register_syllogism(const SyllogisticPropositionPod &p_syllogism);
    [[nodiscard]] const std::vector<SyllogisticPropositionPod> &syllogisms() const noexcept { return syllogisms_; }

    uint32_t register_explanation(const ExplanationPod &p_explanation);
    [[nodiscard]] const std::vector<ExplanationPod> &explanations() const noexcept { return explanations_; }

    // ========================================================================
    // 4. Wave 1–5 Epistemic Operations (Sensory Ingest, Reasoning, Deception & Contradiction)
    // ========================================================================

    /**
     * Wave 1 + Wave 3 helper: Decodes a `SensoryEventPod` (applying modality noise and
     * tracking any sensory misreference between `perceived_fact_id` and `target_fact_id`).
     */
    bool ingest_sensory_event(
        const SensoryEventPod &p_event,
        float p_attention_threshold = 0.2f
    );

    /**
     * Wave 4 Deliberation & Deception: Synthesizes a truthful or fabricated `ClaimPod`
     * speech act and records a self-alibi fact in the Akashic Tome if lying.
     */
    [[nodiscard]] ClaimPod synthesize_claim(
        uint32_t p_speaker_agent_id,
        uint32_t p_listener_agent_id,
        uint32_t p_fact_id,
        int64_t p_fabricated_alibi_value,
        const InstinctiveTraitWeightsPod &p_traits,
        uint16_t p_tome_id = TOME_PHYSICAL_REALITY
    );

    /**
     * Wave 5 Contradiction & Trust Reconciliation: Ingests a heard `ClaimPod` into
     * `p_listener_agent_id`'s reference catalogue, returning `false` and flagging
     * `FACT_FLAG_CONTRADICTED` if it conflicts with a high-conviction existing belief.
     */
    bool ingest_heard_claim(
        uint32_t p_listener_agent_id,
        const ClaimPod &p_claim,
        float p_trust_in_speaker,
        float p_contradiction_threshold = 0.7f
    );
};

} // namespace ideam::core
