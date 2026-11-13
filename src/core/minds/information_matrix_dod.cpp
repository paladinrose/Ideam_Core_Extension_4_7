#include "information_matrix_dod.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

InformationMatrixDOD::InformationMatrixDOD() {
    _ensure_default_tome();
}

void InformationMatrixDOD::_ensure_default_tome() {
    if (akashic_tomes_.empty()) {
        AkashicTomeHeaderPod default_tome{};
        default_tome.tome_id          = static_cast<uint16_t>(TOME_PHYSICAL_REALITY);
        default_tome.domain_flags     = FACT_FLAG_PUBLIC;
        default_tome.fact_slice_offset= 0u;
        default_tome.fact_slice_count = 0u;
        default_tome.authority_weight = 1.0f;
        akashic_tomes_.push_back(default_tome);
    }
}

void InformationMatrixDOD::clear() noexcept {
    akashic_tomes_.clear();
    ground_truth_facts_.clear();
    agent_belief_overlays_.clear();
    agent_catalogue_entries_.clear();
    hypotheses_.clear();
    syllogisms_.clear();
    explanations_.clear();
    _ensure_default_tome();
}

uint16_t InformationMatrixDOD::register_akashic_tome(
    uint16_t p_tome_id,
    float p_authority_weight,
    uint16_t p_domain_flags
) {
    for (auto &tome : akashic_tomes_) {
        if (tome.tome_id == p_tome_id) {
            tome.authority_weight = p_authority_weight;
            tome.domain_flags     = p_domain_flags;
            return tome.tome_id;
        }
    }
    AkashicTomeHeaderPod header{};
    header.tome_id          = p_tome_id;
    header.domain_flags     = p_domain_flags;
    header.fact_slice_offset= static_cast<uint32_t>(ground_truth_facts_.size());
    header.fact_slice_count = 0u;
    header.authority_weight = p_authority_weight;
    akashic_tomes_.push_back(header);
    return p_tome_id;
}

const AkashicTomeHeaderPod *InformationMatrixDOD::find_akashic_tome(uint16_t p_tome_id) const noexcept {
    for (const auto &tome : akashic_tomes_) {
        if (tome.tome_id == p_tome_id) {
            return &tome;
        }
    }
    return nullptr;
}

uint32_t InformationMatrixDOD::upsert_ground_truth_fact(const FactPod &p_fact, uint16_t p_tome_id) {
    _ensure_default_tome();
    const uint16_t effective_tome = (p_tome_id != static_cast<uint16_t>(TOME_PHYSICAL_REALITY))
        ? p_tome_id
        : p_fact.get_tome_id();
    register_akashic_tome(effective_tome);

    for (auto &existing : ground_truth_facts_) {
        if (existing.fact_id == p_fact.fact_id &&
            p_fact.fact_id != 0u &&
            existing.get_tome_id() == effective_tome) {
            existing = p_fact;
            existing.set_tome_id(effective_tome);
            return existing.fact_id;
        }
    }

    FactPod copy = p_fact;
    if (copy.fact_id == 0u) {
        copy.fact_id = static_cast<uint32_t>(ground_truth_facts_.size() + 1u);
    }
    copy.set_tome_id(effective_tome);
    ground_truth_facts_.push_back(copy);

    for (auto &tome : akashic_tomes_) {
        if (tome.tome_id == effective_tome) {
            ++tome.fact_slice_count;
            break;
        }
    }
    return copy.fact_id;
}

const FactPod *InformationMatrixDOD::find_ground_truth_fact(uint32_t p_fact_id, uint16_t p_tome_id) const noexcept {
    for (const auto &f : ground_truth_facts_) {
        if (f.fact_id == p_fact_id && f.get_tome_id() == p_tome_id) {
            return &f;
        }
    }
    // Fallback if caller did not specify tome_id and fact_id is globally unique
    if (p_tome_id == static_cast<uint16_t>(TOME_PHYSICAL_REALITY)) {
        for (const auto &f : ground_truth_facts_) {
            if (f.fact_id == p_fact_id) {
                return &f;
            }
        }
    }
    return nullptr;
}

void InformationMatrixDOD::upsert_agent_belief(
    uint32_t p_agent_id,
    const BeliefPod &p_belief,
    uint32_t p_true_fact_id,
    uint16_t p_believed_tome_id,
    uint16_t p_true_tome_id,
    StimulusPriority p_priority
) {
    const uint32_t resolved_true_id = (p_true_fact_id != 0u) ? p_true_fact_id : p_belief.fact_id;
    auto &overlay   = agent_belief_overlays_[p_agent_id];
    auto &catalogue = agent_catalogue_entries_[p_agent_id];

    BeliefCatalogueEntryPod cat_entry{};
    cat_entry.catalogued_fact_id = p_belief.fact_id;
    cat_entry.true_fact_id       = resolved_true_id;
    cat_entry.believed_tome_id   = p_believed_tome_id;
    cat_entry.true_tome_id       = p_true_tome_id;
    cat_entry.stimulus_priority  = static_cast<uint8_t>(p_priority);
    cat_entry.catalogue_flags    = cat_entry.is_misreferenced() ? static_cast<uint8_t>(FACT_FLAG_MISREFERENCED) : static_cast<uint8_t>(FACT_FLAG_NONE);

    for (size_t i = 0; i < overlay.size(); ++i) {
        if (overlay[i].fact_id == p_belief.fact_id) {
            const bool value_changed = (overlay[i].believed_value != p_belief.believed_value);
            overlay[i] = p_belief;
            if (i < catalogue.size()) {
                if (p_priority == StimulusPriority::NEW_NEUTRAL) {
                    cat_entry.stimulus_priority = static_cast<uint8_t>(
                        classify_stimulus_priority(false, value_changed, StimulusValence::NEUTRAL)
                    );
                }
                catalogue[i] = cat_entry;
            }
            return;
        }
    }

    overlay.push_back(p_belief);
    catalogue.push_back(cat_entry);
}

void InformationMatrixDOD::upsert_misreferenced_belief(
    uint32_t p_agent_id,
    uint32_t p_catalogued_fact_id,
    uint32_t p_true_fact_id,
    int64_t p_believed_value,
    float p_confidence,
    float p_acceptance,
    uint16_t p_believed_tome_id,
    uint16_t p_true_tome_id,
    uint32_t p_source_agent_id
) {
    BeliefPod belief{};
    belief.fact_id          = p_catalogued_fact_id;
    belief.source_agent_id  = p_source_agent_id;
    belief.believed_value   = p_believed_value;
    belief.confidence       = std::clamp(p_confidence, 0.0f, 1.0f);
    belief.acceptance       = std::clamp(p_acceptance, 0.0f, 1.0f);
    belief.emotional_charge = 0.0f;
    belief.intuition_charge = 0.0f;

    upsert_agent_belief(
        p_agent_id,
        belief,
        p_true_fact_id,
        p_believed_tome_id,
        p_true_tome_id,
        StimulusPriority::NEW_UNKNOWN
    );
}

const BeliefPod *InformationMatrixDOD::find_agent_belief(uint32_t p_agent_id, uint32_t p_catalogued_fact_id) const noexcept {
    const auto it = agent_belief_overlays_.find(p_agent_id);
    if (it == agent_belief_overlays_.end()) {
        return nullptr;
    }
    for (const auto &b : it->second) {
        if (b.fact_id == p_catalogued_fact_id) {
            return &b;
        }
    }
    return nullptr;
}

BeliefPod *InformationMatrixDOD::find_agent_belief_mut(uint32_t p_agent_id, uint32_t p_catalogued_fact_id) noexcept {
    auto it = agent_belief_overlays_.find(p_agent_id);
    if (it == agent_belief_overlays_.end()) {
        return nullptr;
    }
    for (auto &b : it->second) {
        if (b.fact_id == p_catalogued_fact_id) {
            return &b;
        }
    }
    return nullptr;
}

const BeliefCatalogueEntryPod *InformationMatrixDOD::find_agent_catalogue_entry(
    uint32_t p_agent_id,
    uint32_t p_catalogued_fact_id
) const noexcept {
    const auto it = agent_catalogue_entries_.find(p_agent_id);
    if (it == agent_catalogue_entries_.end()) {
        return nullptr;
    }
    for (const auto &c : it->second) {
        if (c.catalogued_fact_id == p_catalogued_fact_id) {
            return &c;
        }
    }
    return nullptr;
}

const std::vector<BeliefPod> *InformationMatrixDOD::get_agent_beliefs(uint32_t p_agent_id) const noexcept {
    const auto it = agent_belief_overlays_.find(p_agent_id);
    return (it != agent_belief_overlays_.end()) ? &it->second : nullptr;
}

const std::vector<BeliefCatalogueEntryPod> *InformationMatrixDOD::get_agent_catalogue(uint32_t p_agent_id) const noexcept {
    const auto it = agent_catalogue_entries_.find(p_agent_id);
    return (it != agent_catalogue_entries_.end()) ? &it->second : nullptr;
}

bool InformationMatrixDOD::is_agent_belief_knowledge(
    uint32_t p_agent_id,
    uint32_t p_catalogued_fact_id,
    float p_knowledge_threshold
) const noexcept {
    const BeliefPod *belief = find_agent_belief(p_agent_id, p_catalogued_fact_id);
    if (!belief) return false;

    const BeliefCatalogueEntryPod *cat = find_agent_catalogue_entry(p_agent_id, p_catalogued_fact_id);
    if (cat && cat->is_misreferenced()) {
        return false;
    }

    const uint16_t tome_id   = cat ? cat->true_tome_id : static_cast<uint16_t>(TOME_PHYSICAL_REALITY);
    const uint32_t target_id = cat ? cat->true_fact_id : p_catalogued_fact_id;
    const FactPod *true_fact = find_ground_truth_fact(target_id, tome_id);
    if (!true_fact) return false;

    return belief->is_knowledge(*true_fact, target_id, p_knowledge_threshold);
}

bool InformationMatrixDOD::is_agent_belief_misreferenced(
    uint32_t p_agent_id,
    uint32_t p_catalogued_fact_id
) const noexcept {
    const BeliefCatalogueEntryPod *cat = find_agent_catalogue_entry(p_agent_id, p_catalogued_fact_id);
    return cat ? cat->is_misreferenced() : false;
}

uint32_t InformationMatrixDOD::register_hypothesis(const HypothesisPod &p_hypothesis) {
    hypotheses_.push_back(p_hypothesis);
    return static_cast<uint32_t>(hypotheses_.size() - 1u);
}

uint32_t InformationMatrixDOD::register_syllogism(const SyllogisticPropositionPod &p_syllogism) {
    syllogisms_.push_back(p_syllogism);
    return static_cast<uint32_t>(syllogisms_.size() - 1u);
}

uint32_t InformationMatrixDOD::register_explanation(const ExplanationPod &p_explanation) {
    explanations_.push_back(p_explanation);
    return static_cast<uint32_t>(explanations_.size() - 1u);
}

bool InformationMatrixDOD::ingest_sensory_event(
    const SensoryEventPod &p_event,
    float p_attention_threshold
) {
    const float clarity = p_event.effective_clarity();
    if (clarity < p_attention_threshold) {
        return false;
    }

    const uint32_t cat_id  = p_event.resolved_catalogued_fact_id();
    const uint32_t true_id = (p_event.target_fact_id != 0u) ? p_event.target_fact_id : cat_id;

    const BeliefPod *existing = find_agent_belief(p_event.sensor_agent_id, cat_id);
    const bool is_new         = (existing == nullptr);
    const bool has_changed    = (!is_new && existing->believed_value != p_event.sensed_value);

    BeliefPod updated = existing ? *existing : BeliefPod{};
    updated.fact_id         = cat_id;
    updated.source_agent_id = 0u; // Direct sensory observation
    updated.believed_value  = p_event.sensed_value;
    updated.apply_inductive_evidence(clarity);
    (void)updated.apply_abductive_observation(clarity, 1.0f, 0.5f, 0.25f);

    const StimulusValence valence = (cat_id != true_id) ? StimulusValence::UNKNOWN : StimulusValence::NEUTRAL;
    const StimulusPriority prio   = classify_stimulus_priority(is_new, has_changed, valence);

    upsert_agent_belief(
        p_event.sensor_agent_id,
        updated,
        true_id,
        p_event.tome_id,
        p_event.tome_id,
        prio
    );
    return true;
}

ClaimPod InformationMatrixDOD::synthesize_claim(
    uint32_t p_speaker_agent_id,
    uint32_t p_listener_agent_id,
    uint32_t p_fact_id,
    int64_t p_fabricated_alibi_value,
    const InstinctiveTraitWeightsPod &p_traits,
    uint16_t p_tome_id
) {
    ClaimPod claim{};
    claim.speaker_agent_id   = p_speaker_agent_id;
    claim.listener_agent_id  = p_listener_agent_id;
    claim.fact_id            = p_fact_id;
    claim.referenced_fact_id = p_fact_id;
    claim.tome_id            = p_tome_id;

    const BeliefPod *belief = find_agent_belief(p_speaker_agent_id, p_fact_id);
    const BeliefCatalogueEntryPod *cat = find_agent_catalogue_entry(p_speaker_agent_id, p_fact_id);
    if (cat) {
        claim.referenced_fact_id = cat->true_fact_id;
        claim.tome_id            = cat->believed_tome_id;
    }

    const int64_t believed_val = belief ? belief->believed_value : 0;
    const float conf           = belief ? std::clamp(belief->confidence + belief->acceptance * 0.5f, 0.0f, 1.0f) : 0.0f;

    // Stage 4 deception rule: if Self_Preservation + Guilt outweighs Trust_In_Listener + Loyalty, emit a lie
    const float lie_drive   = p_traits.self_preservation * 0.65f + p_traits.guilt * 0.35f;
    const float truth_drive = p_traits.trust_in_listener * 0.60f + p_traits.loyalty * 0.40f;

    if (lie_drive > truth_drive) {
        claim.claimed_value      = p_fabricated_alibi_value;
        claim.claimed_confidence = 0.9f;
        claim.is_intentional_lie = true;
        claim.sememic_form       = static_cast<uint8_t>(SememicForm::EVALUATIVE_CONNOTATION);

        // Record alibi self-fact in the Akashic Tome so the liar remembers their own fabricated statement
        FactPod alibi_fact{};
        alibi_fact.subject_id    = p_speaker_agent_id;
        alibi_fact.predicate_id  = static_cast<uint16_t>(p_fact_id & 0xFFFFu);
        alibi_fact.flags         = FACT_FLAG_ALIBI_RECORD;
        alibi_fact.truth_value   = p_fabricated_alibi_value;
        alibi_fact.location_id   = p_listener_agent_id;
        alibi_fact.base_salience = 1.0f;
        const uint32_t alibi_id  = upsert_ground_truth_fact(alibi_fact, p_tome_id);
        claim.referenced_fact_id = alibi_id;

        BeliefPod self_alibi{};
        self_alibi.fact_id          = alibi_id;
        self_alibi.source_agent_id  = p_speaker_agent_id;
        self_alibi.believed_value   = p_fabricated_alibi_value;
        self_alibi.confidence       = 1.0f;
        self_alibi.acceptance       = 1.0f;
        self_alibi.emotional_charge = p_traits.guilt;
        upsert_agent_belief(p_speaker_agent_id, self_alibi, alibi_id, p_tome_id, p_tome_id, StimulusPriority::NEW_THREAT);
    } else {
        claim.claimed_value      = believed_val;
        claim.claimed_confidence = conf;
        claim.is_intentional_lie = false;
        claim.sememic_form       = static_cast<uint8_t>(SememicForm::PRIMARY_DENOTATION);
    }
    return claim;
}

bool InformationMatrixDOD::ingest_heard_claim(
    uint32_t p_listener_agent_id,
    const ClaimPod &p_claim,
    float p_trust_in_speaker,
    float p_contradiction_threshold
) {
    const BeliefPod *existing = find_agent_belief(p_listener_agent_id, p_claim.fact_id);
    if (existing != nullptr &&
        existing->believed_value != p_claim.claimed_value &&
        (existing->confidence + existing->acceptance) > p_contradiction_threshold) {
        // Stage 5 Contradiction detected!
        return false;
    }

    BeliefPod updated = existing ? *existing : BeliefPod{};
    updated.fact_id          = p_claim.fact_id;
    updated.source_agent_id  = p_claim.speaker_agent_id;
    updated.believed_value   = p_claim.claimed_value;
    updated.confidence       = std::clamp(p_claim.claimed_confidence * p_trust_in_speaker, 0.0f, 1.0f);
    updated.acceptance       = std::clamp(updated.acceptance + 0.35f * p_trust_in_speaker, 0.0f, 1.0f);
    updated.emotional_charge = 0.1f;

    const uint32_t true_id = (p_claim.referenced_fact_id != 0u) ? p_claim.referenced_fact_id : p_claim.fact_id;
    upsert_agent_belief(
        p_listener_agent_id,
        updated,
        true_id,
        p_claim.tome_id,
        p_claim.tome_id,
        StimulusPriority::NEW_NEUTRAL
    );
    return true;
}

} // namespace ideam::core
