#include "information_matrix_dod.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

void InformationMatrixDOD::clear() noexcept {
    ground_truth_facts_.clear();
    agent_belief_overlays_.clear();
}

uint32_t InformationMatrixDOD::upsert_ground_truth_fact(const FactPod &p_fact) {
    for (auto &existing : ground_truth_facts_) {
        if (existing.fact_id == p_fact.fact_id && p_fact.fact_id != 0) {
            existing = p_fact;
            return existing.fact_id;
        }
    }
    FactPod copy = p_fact;
    if (copy.fact_id == 0) {
        copy.fact_id = static_cast<uint32_t>(ground_truth_facts_.size() + 1);
    }
    ground_truth_facts_.push_back(copy);
    return copy.fact_id;
}

const FactPod *InformationMatrixDOD::find_ground_truth_fact(uint32_t p_fact_id) const noexcept {
    for (const auto &f : ground_truth_facts_) {
        if (f.fact_id == p_fact_id) {
            return &f;
        }
    }
    return nullptr;
}

void InformationMatrixDOD::upsert_agent_belief(uint32_t p_agent_id, const BeliefPod &p_belief) {
    auto &overlay = agent_belief_overlays_[p_agent_id];
    for (auto &b : overlay) {
        if (b.fact_id == p_belief.fact_id) {
            b = p_belief;
            return;
        }
    }
    overlay.push_back(p_belief);
}

const BeliefPod *InformationMatrixDOD::find_agent_belief(uint32_t p_agent_id, uint32_t p_fact_id) const noexcept {
    const auto it = agent_belief_overlays_.find(p_agent_id);
    if (it == agent_belief_overlays_.end()) {
        return nullptr;
    }
    for (const auto &b : it->second) {
        if (b.fact_id == p_fact_id) {
            return &b;
        }
    }
    return nullptr;
}

const std::vector<BeliefPod> *InformationMatrixDOD::get_agent_beliefs(uint32_t p_agent_id) const noexcept {
    const auto it = agent_belief_overlays_.find(p_agent_id);
    return (it != agent_belief_overlays_.end()) ? &it->second : nullptr;
}

ClaimPod InformationMatrixDOD::synthesize_claim(
    uint32_t p_speaker_agent_id,
    uint32_t p_listener_agent_id,
    uint32_t p_fact_id,
    int64_t p_fabricated_alibi_value,
    const InstinctiveTraitWeightsPod &p_traits
) {
    ClaimPod claim{};
    claim.speaker_agent_id  = p_speaker_agent_id;
    claim.listener_agent_id = p_listener_agent_id;
    claim.fact_id           = p_fact_id;

    const BeliefPod *belief = find_agent_belief(p_speaker_agent_id, p_fact_id);
    const int64_t believed_val = belief ? belief->believed_value : 0;
    const float conf           = belief ? belief->confidence : 0.0f;

    // Stage 4 deception rule: if Self_Preservation + Guilt outweighs Trust_In_Listener + Loyalty, emit a lie
    const float lie_drive   = p_traits.self_preservation * 0.65f + p_traits.guilt * 0.35f;
    const float truth_drive = p_traits.trust_in_listener * 0.60f + p_traits.loyalty * 0.40f;

    if (lie_drive > truth_drive) {
        claim.claimed_value      = p_fabricated_alibi_value;
        claim.claimed_confidence = 0.9f;
        claim.is_intentional_lie = true;

        // Record alibi self-fact so the liar remembers their own fabricated statement
        FactPod alibi_fact{};
        alibi_fact.subject_id    = p_speaker_agent_id;
        alibi_fact.predicate_id  = static_cast<uint16_t>(p_fact_id & 0xFFFFu);
        alibi_fact.flags         = FACT_FLAG_ALIBI_RECORD;
        alibi_fact.truth_value   = p_fabricated_alibi_value;
        alibi_fact.location_id   = p_listener_agent_id;
        alibi_fact.base_salience = 1.0f;
        const uint32_t alibi_id  = upsert_ground_truth_fact(alibi_fact);

        BeliefPod self_alibi{};
        self_alibi.fact_id          = alibi_id;
        self_alibi.source_agent_id  = p_speaker_agent_id;
        self_alibi.believed_value   = p_fabricated_alibi_value;
        self_alibi.confidence       = 1.0f;
        self_alibi.emotional_charge = p_traits.guilt;
        upsert_agent_belief(p_speaker_agent_id, self_alibi);
    } else {
        claim.claimed_value      = believed_val;
        claim.claimed_confidence = conf;
        claim.is_intentional_lie = false;
    }
    return claim;
}

bool InformationMatrixDOD::ingest_heard_claim(
    uint32_t p_listener_agent_id,
    const ClaimPod &p_claim,
    float p_trust_in_speaker
) {
    const BeliefPod *existing = find_agent_belief(p_listener_agent_id, p_claim.fact_id);
    if (existing != nullptr && existing->believed_value != p_claim.claimed_value && existing->confidence > 0.7f) {
        // Stage 5 Contradiction detected!
        return false;
    }
    BeliefPod updated{};
    updated.fact_id          = p_claim.fact_id;
    updated.source_agent_id  = p_claim.speaker_agent_id;
    updated.believed_value   = p_claim.claimed_value;
    updated.confidence       = std::clamp(p_claim.claimed_confidence * p_trust_in_speaker, 0.0f, 1.0f);
    updated.emotional_charge = 0.1f;
    upsert_agent_belief(p_listener_agent_id, updated);
    return true;
}

} // namespace ideam::core
