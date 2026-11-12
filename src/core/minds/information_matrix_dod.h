#pragma once

/**
 * information_matrix_dod.h
 * Pure C++26 Epistemic Information Matrix & 5-Stage Cognitive Pipeline (`src/core/minds/`).
 */

#include "mind_common.h"
#include <unordered_map>
#include <vector>

namespace ideam::core {

class InformationMatrixDOD {
private:
    std::vector<FactPod> ground_truth_facts_;
    std::unordered_map<uint32_t, std::vector<BeliefPod>> agent_belief_overlays_;

public:
    InformationMatrixDOD() = default;
    ~InformationMatrixDOD() = default;

    void clear() noexcept;

    // --- Ground-Truth Fact Table (`SOA` / Flat POD Store) ---
    uint32_t upsert_ground_truth_fact(const FactPod &p_fact);
    [[nodiscard]] const FactPod *find_ground_truth_fact(uint32_t p_fact_id) const noexcept;
    [[nodiscard]] const std::vector<FactPod> &ground_truth_facts() const noexcept { return ground_truth_facts_; }

    // --- Per-Agent `SPARSE_SET` Epistemic Belief Overlay ---
    void upsert_agent_belief(uint32_t p_agent_id, const BeliefPod &p_belief);
    [[nodiscard]] const BeliefPod *find_agent_belief(uint32_t p_agent_id, uint32_t p_fact_id) const noexcept;
    [[nodiscard]] const std::vector<BeliefPod> *get_agent_beliefs(uint32_t p_agent_id) const noexcept;

    // --- Stage 4 & Stage 5 Epistemic Operations ---
    [[nodiscard]] ClaimPod synthesize_claim(
        uint32_t p_speaker_agent_id,
        uint32_t p_listener_agent_id,
        uint32_t p_fact_id,
        int64_t p_fabricated_alibi_value,
        const InstinctiveTraitWeightsPod &p_traits
    );

    bool ingest_heard_claim(
        uint32_t p_listener_agent_id,
        const ClaimPod &p_claim,
        float p_trust_in_speaker
    );
};

} // namespace ideam::core
