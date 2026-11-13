#include "mind_dod.h"

#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

constexpr size_t DEFAULT_MINDS_MASTER_BLOCK_BYTES    = 16ULL * 1024ULL * 1024ULL; // 16 MB
constexpr size_t DEFAULT_MINDS_TRANSIENT_BLOCK_BYTES = 4ULL * 1024ULL * 1024ULL;  // 4 MB

struct MindSlotAccessSpec {
    MindBufferSlot slot     = MindBufferSlot::COUNT;
    BufferAccessMode access = BufferAccessMode::READ;
};

template <typename T_Logic>
struct MindTaskDescriptor;

#define DEFINE_MIND_TASK_DESCRIPTOR(LogicStruct, EnumVal, S0, A0, S1, A1, S2, A2, S3, A3) \
    template <typename T>                                                                 \
    struct MindTaskDescriptor<LogicStruct<T>> {                                           \
        static constexpr auto logic_id = EnumVal;                                         \
        static constexpr std::array<MindSlotAccessSpec, 4> default_grants = {{            \
            {MindBufferSlot::S0, BufferAccessMode::A0},                                   \
            {MindBufferSlot::S1, BufferAccessMode::A1},                                   \
            {MindBufferSlot::S2, BufferAccessMode::A2},                                   \
            {MindBufferSlot::S3, BufferAccessMode::A3}                                    \
        }};                                                                               \
    }

// --- Compile-Time Task Grant & LogicID Table (MND1 - MND12) ---
DEFINE_MIND_TASK_DESCRIPTOR(SensorySignalDecodeTransformLogic, MindsTransformLogicID::SensorySignalDecode,
    MIND_OBSERVATION_POD, READ_WRITE, MIND_SENSORY_OBSERVATION_EVENT_RING, READ_WRITE, MIND_SENSORY_RING_CURSOR, READ_WRITE, MIND_FACT_POD, READ);
DEFINE_MIND_TASK_DESCRIPTOR(SensoryObservationWakeQueryLogic, MindsQueryLogicID::SensoryObservationWake,
    MIND_BELIEF_CONFIDENCE, READ, MIND_OBSERVATION_POD, READ, MIND_AGENT_ATTENTION_THRESHOLD, READ, MIND_AGENT_BELIEF_CSR_OFFSETS, READ);
DEFINE_MIND_TASK_DESCRIPTOR(InstinctiveAffectAppraisalTransformLogic, MindsTransformLogicID::InstinctiveAffectAppraisal,
    MIND_BELIEF_EMOTIONAL_CHARGE, READ_WRITE, MIND_AGENT_TRAIT_WEIGHTS_POD, READ, MIND_AGENT_EXCITATION_LEVEL, READ_WRITE, MIND_AGENT_ACTIVE_EMOTION_MASK, READ_WRITE);
DEFINE_MIND_TASK_DESCRIPTOR(StimulusLatencyTriageMetadataLogic, MindsMetadataLogicID::StimulusLatencyTriage,
    MIND_BELIEF_STIMULUS_PRIORITY, READ_WRITE, MIND_BELIEF_BELIEVED_VALUE, READ, MIND_BELIEF_PREV_BELIEVED_VALUE, READ_WRITE, MIND_BELIEF_EMOTIONAL_CHARGE, READ);
DEFINE_MIND_TASK_DESCRIPTOR(AbductiveHypothesisTransformLogic, MindsTransformLogicID::AbductiveHypothesis,
    MIND_BELIEF_ACCEPTANCE, READ_WRITE, MIND_HYPOTHESIS_POD, READ_WRITE, MIND_BELIEF_INTUITION_CHARGE, READ_WRITE, MIND_EXPLANATION_POD, READ_WRITE);
DEFINE_MIND_TASK_DESCRIPTOR(IntuitiveLeapStochasticQueryLogic, MindsQueryLogicID::IntuitiveLeapStochastic,
    MIND_BELIEF_INTUITION_CHARGE, READ_WRITE, MIND_BELIEF_ACCEPTANCE, READ_WRITE, MIND_INTUITION_CONTRADICTION_RING, READ_WRITE, MIND_INTUITION_RING_CURSOR, READ_WRITE);
DEFINE_MIND_TASK_DESCRIPTOR(InductiveEvidenceAccumulateTransformLogic, MindsTransformLogicID::InductiveEvidenceAccumulate,
    MIND_BELIEF_CONFIDENCE, READ_WRITE, MIND_EVIDENCE_POD, READ, MIND_BELIEF_POD, READ_WRITE, MIND_AGENT_KNOWLEDGE_THRESHOLD, READ);
DEFINE_MIND_TASK_DESCRIPTOR(DeductiveSyllogismPropagateTransformLogic, MindsTransformLogicID::DeductiveSyllogismPropagate,
    MIND_SYLLOGISM_PROPOSITION_POD, READ_WRITE, MIND_BELIEF_CONFIDENCE, READ_WRITE, MIND_BELIEF_ACCEPTANCE, READ_WRITE, MIND_BELIEF_POD, READ_WRITE);
DEFINE_MIND_TASK_DESCRIPTOR(DeliberationAndDeceptionTransformLogic, MindsTransformLogicID::DeliberationAndDeception,
    MIND_BELIEF_POD, READ_WRITE, MIND_FACT_POD, READ, MIND_SPEECH_CLAIM_EVENT_RING, READ_WRITE, MIND_SPEECH_CLAIM_RING_CURSOR, READ_WRITE);
DEFINE_MIND_TASK_DESCRIPTOR(DiscourseTurnAndProsodyTransformLogic, MindsTransformLogicID::DiscourseTurnAndProsody,
    MIND_DISCOURSE_TURN_POD, READ_WRITE, MIND_DISCOURSE_SPEAKER_MASK, READ_WRITE, MIND_DISCOURSE_LISTENER_MASK, READ_WRITE, MIND_SPEECH_CLAIM_EVENT_RING, READ_WRITE);
DEFINE_MIND_TASK_DESCRIPTOR(ContradictionAndTrustReconcileTransformLogic, MindsTransformLogicID::ContradictionAndTrustReconcile,
    MIND_BELIEF_POD, READ_WRITE, MIND_SPEECH_CLAIM_EVENT_RING, READ_WRITE, MIND_INTUITION_CONTRADICTION_RING, READ_WRITE, MIND_BELIEF_CATALOGUE_ENTRY_POD, READ_WRITE);
DEFINE_MIND_TASK_DESCRIPTOR(SememeAndLexiconMorphologyTransformLogic, MindsTransformLogicID::SememeAndLexiconMorphology,
    MIND_SEMEME_TOKEN_STREAM, READ_WRITE, MIND_DISCOURSE_TURN_POD, READ, MIND_FACT_POD, READ, MIND_BELIEF_CATALOGUE_ENTRY_POD, READ);

#undef DEFINE_MIND_TASK_DESCRIPTOR

} // anonymous namespace

MindDOD::MindDOD(
    std::shared_ptr<MemoryManagerDOD> p_memory_manager,
    const MindsBufferCapacities &p_capacities
)
    : memory_manager_(std::move(p_memory_manager)),
      capacities_(p_capacities) {
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_MINDS_MASTER_BLOCK_BYTES,
            DEFAULT_MINDS_TRANSIENT_BLOCK_BYTES
        );
    }
    default_task_graph_ = std::make_shared<TaskGraphDOD>(memory_manager_.get());
    default_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
    initialize_buffers(capacities_);
}

void MindDOD::initialize_buffers(const MindsBufferCapacities &p_capacities) {
    capacities_ = p_capacities;
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_MINDS_MASTER_BLOCK_BYTES,
            DEFAULT_MINDS_TRANSIENT_BLOCK_BYTES
        );
    }

    tome_allocator_.initialize(capacities_.max_akashic_tomes);
    fact_allocator_.initialize(capacities_.max_facts);
    agent_allocator_.initialize(capacities_.max_agents);
    belief_allocator_.initialize(capacities_.max_beliefs);
    observation_allocator_.initialize(capacities_.max_observations);
    hypothesis_allocator_.initialize(capacities_.max_hypotheses);
    evidence_allocator_.initialize(capacities_.max_evidence_rows);
    syllogism_allocator_.initialize(capacities_.max_syllogisms);
    explanation_allocator_.initialize(capacities_.max_explanations);
    discourse_turn_allocator_.initialize(capacities_.max_discourse_turns);

    for (size_t i = 0; i < MIND_BUFFER_SLOT_COUNT; ++i) {
        const MindBufferSpec &spec = MIND_BUFFER_SPECS[i];
        const size_t max_elems = std::max<size_t>(1u, capacities_.get_capacity(spec.capacity_kind));
        const size_t bytes     = max_elems * spec.element_stride;
        const uint32_t buf_id  = memory_manager_->create_shadowed_buffer(
            spec.layout, bytes, static_cast<int64_t>(max_elems), spec.selection_mode
        );
        if (MemoryBufferPOD *buf = memory_manager_->get_buffer(buf_id)) {
            buf->element_stride = spec.element_stride;
            buf->current_count  = 0;
            if (buf->master_block_ptr) {
                std::memset(buf->master_block_ptr + buf->memory_offset, 0, bytes);
            }
        }
        manifest_.buffer_ids[i] = buf_id;
    }

    reset_all_slots();
}

uint32_t MindDOD::_active_count_for_group(MindTableGroup p_group) const noexcept {
    switch (p_group) {
        case MindTableGroup::AKASHIC_TOME_TABLE:            return tome_allocator_.get_high_water_mark();
        case MindTableGroup::WORLD_GROUND_TRUTH_FACT_TABLE: return fact_allocator_.get_high_water_mark();
        case MindTableGroup::AGENT_TRAIT_AND_STATE_TABLE:   return agent_allocator_.get_high_water_mark();
        case MindTableGroup::AGENT_BELIEF_CSR_ROW:          return agent_allocator_.get_high_water_mark() ? (agent_allocator_.get_high_water_mark() + 1u) : 0u;
        case MindTableGroup::AGENT_BELIEF_SPARSE_TABLE:     return belief_allocator_.get_high_water_mark();
        case MindTableGroup::OBSERVATION_TABLE:             return observation_allocator_.get_high_water_mark();
        case MindTableGroup::HYPOTHESIS_TABLE:              return hypothesis_allocator_.get_high_water_mark();
        case MindTableGroup::EVIDENCE_TABLE:                return evidence_allocator_.get_high_water_mark();
        case MindTableGroup::SYLLOGISM_TABLE:               return syllogism_allocator_.get_high_water_mark();
        case MindTableGroup::EXPLANATION_TABLE:             return explanation_allocator_.get_high_water_mark();
        case MindTableGroup::EXPLANATION_CSR_SLICE:         return std::max(explanation_fact_csr_cursor_, explanation_cause_csr_cursor_);
        case MindTableGroup::DISCOURSE_AND_PROSODY_TABLE:   return discourse_turn_allocator_.get_high_water_mark();
        case MindTableGroup::SEMEME_TOKEN_SLICE:            return sememe_token_cursor_;
        case MindTableGroup::EVENT_RING_OR_COUNTER:
        default:                                            return 0u;
    }
}

void MindDOD::sync_selection_ranges() noexcept {
    if (!memory_manager_) return;
    for (size_t i = 0; i < MIND_BUFFER_SLOT_COUNT; ++i) {
        MemoryBufferPOD *buf = memory_manager_->get_buffer(manifest_.buffer_ids[i]);
        if (!buf) continue;
        const MindBufferSpec &spec = MIND_BUFFER_SPECS[i];
        buf->current_count = (spec.table_group == MindTableGroup::EVENT_RING_OR_COUNTER)
            ? buf->max_elements
            : std::min<int64_t>(static_cast<int64_t>(_active_count_for_group(spec.table_group)), buf->max_elements);
        buf->used_bytes = static_cast<size_t>(buf->current_count) * buf->element_stride;
    }
}

void MindDOD::reset_all_slots() noexcept {
    tome_allocator_.clear();
    fact_allocator_.clear();
    agent_allocator_.clear();
    belief_allocator_.clear();
    observation_allocator_.clear();
    hypothesis_allocator_.clear();
    evidence_allocator_.clear();
    syllogism_allocator_.clear();
    explanation_allocator_.clear();
    discourse_turn_allocator_.clear();

    explanation_fact_csr_cursor_  = 0u;
    explanation_cause_csr_cursor_ = 0u;
    sememe_token_cursor_          = 0u;

    auto s_cur = get_column_span<uint32_t>(MindBufferSlot::MIND_SENSORY_RING_CURSOR);
    std::fill(s_cur.begin(), s_cur.end(), 0u);
    auto c_cur = get_column_span<uint32_t>(MindBufferSlot::MIND_SPEECH_CLAIM_RING_CURSOR);
    std::fill(c_cur.begin(), c_cur.end(), 0u);
    auto i_cur = get_column_span<uint32_t>(MindBufferSlot::MIND_INTUITION_RING_CURSOR);
    std::fill(i_cur.begin(), i_cur.end(), 0u);

    sync_selection_ranges();
}

void MindDOD::populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing) const {
    auto set_key = [&](std::string_view p_key, uint32_t p_val) {
        if (p_key.empty() || p_val == INVALID_ID) return;
        const godot::String key_str(std::string(p_key).c_str());
        if (p_overwrite_existing || !r_props.has(key_str)) {
            r_props[key_str] = static_cast<int64_t>(p_val);
        }
    };
    for (size_t i = 0; i < MIND_BUFFER_SLOT_COUNT; ++i) {
        const uint32_t buf_id = manifest_.buffer_ids[i];
        set_key(MIND_BUFFER_SPECS[i].canonical_name, buf_id);
        for (std::string_view alias : MIND_BUFFER_SPECS[i].aliases) {
            set_key(alias, buf_id);
        }
    }
}

GrantPartPOD MindDOD::make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept {
    if (memory_manager_ && p_buffer_id != INVALID_ID) {
        if (const MemoryBufferPOD *buf = memory_manager_->get_buffer(p_buffer_id)) {
            return make_grant_requirement(p_buffer_id, p_access, buf->element_stride, buf->max_elements, buf->default_selection_mode);
        }
    }
    return make_grant_requirement(p_buffer_id, p_access, sizeof(uint32_t), 0);
}

void MindDOD::_bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept {
    auto *dst       = static_cast<uint32_t *>(r_logic_bytes);
    const auto *src = static_cast<const uint32_t *>(p_default_bytes);
    for (size_t i = 0, n = p_size / sizeof(uint32_t); i < n; ++i) {
        if (dst[i] == INVALID_ID && src[i] != INVALID_ID) {
            dst[i] = src[i];
        }
    }
}

// ============================================================================
// Table 1: WorldGroundTruthFactTable (Multi-Source Akashic Tomes & Facts)
// ============================================================================

uint32_t MindDOD::allocate_akashic_tome(const AkashicTomeHeaderPod &p_header) {
    const uint32_t slot = tome_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    if (auto *tomes = get_column_ptr<AkashicTomeHeaderPod>(MindBufferSlot::MIND_AKASHIC_TOME_HEADER_POD)) {
        tomes[slot] = p_header;
    }
    sync_selection_ranges();
    return slot;
}

AkashicTomeHeaderPod MindDOD::get_akashic_tome_header(uint32_t p_tome_slot) const noexcept {
    if (p_tome_slot >= capacities_.max_akashic_tomes) return AkashicTomeHeaderPod{};
    const auto *tomes = get_column_ptr<AkashicTomeHeaderPod>(MindBufferSlot::MIND_AKASHIC_TOME_HEADER_POD);
    return tomes ? tomes[p_tome_slot] : AkashicTomeHeaderPod{};
}

uint32_t MindDOD::allocate_ground_truth_fact(const FactPod &p_fact, uint16_t p_tome_id) {
    const uint32_t slot = fact_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    set_ground_truth_fact(slot, p_fact, p_tome_id);
    sync_selection_ranges();
    return slot;
}

FactPod MindDOD::get_ground_truth_fact(uint32_t p_fact_slot) const noexcept {
    if (p_fact_slot >= capacities_.max_facts) return FactPod{};
    const auto *facts = get_column_ptr<FactPod>(MindBufferSlot::MIND_FACT_POD);
    return facts ? facts[p_fact_slot] : FactPod{};
}

void MindDOD::set_ground_truth_fact(uint32_t p_fact_slot, const FactPod &p_fact, uint16_t p_tome_id) noexcept {
    if (p_fact_slot >= capacities_.max_facts) return;
    FactPod copy = p_fact;
    if (copy.fact_id == 0u) {
        copy.fact_id = p_fact_slot + 1u;
    }
    const uint16_t resolved_tome = (p_tome_id != TOME_PHYSICAL_REALITY) ? p_tome_id : copy.get_tome_id();
    copy.set_tome_id(resolved_tome);

    if (auto *facts = get_column_ptr<FactPod>(MindBufferSlot::MIND_FACT_POD)) facts[p_fact_slot] = copy;
    if (auto *subj  = get_column_ptr<uint32_t>(MindBufferSlot::MIND_FACT_SUBJECT_ID)) subj[p_fact_slot] = copy.subject_id;
    if (auto *pred  = get_column_ptr<uint16_t>(MindBufferSlot::MIND_FACT_PREDICATE_ID)) pred[p_fact_slot] = copy.predicate_id;
    if (auto *tval  = get_column_ptr<int64_t>(MindBufferSlot::MIND_FACT_TRUTH_VALUE)) tval[p_fact_slot] = copy.truth_value;
    if (auto *loc   = get_column_ptr<uint32_t>(MindBufferSlot::MIND_FACT_LOCATION_ID)) loc[p_fact_slot] = copy.location_id;
    if (auto *ts    = get_column_ptr<float>(MindBufferSlot::MIND_FACT_TIMESTAMP)) ts[p_fact_slot] = copy.timestamp;
    if (auto *sal   = get_column_ptr<float>(MindBufferSlot::MIND_FACT_BASE_SALIENCE)) sal[p_fact_slot] = copy.base_salience;
    if (auto *flg   = get_column_ptr<uint16_t>(MindBufferSlot::MIND_FACT_FLAGS)) flg[p_fact_slot] = copy.get_flag_bits();
    if (auto *tome  = get_column_ptr<uint16_t>(MindBufferSlot::MIND_FACT_TOME_ID)) tome[p_fact_slot] = resolved_tome;
}

// ============================================================================
// Table 2: AgentTraitAndMentalStateTable
// ============================================================================

uint32_t MindDOD::allocate_agent(
    const InstinctiveTraitWeightsPod &p_traits,
    float p_attention_threshold,
    float p_knowledge_threshold,
    float p_excitation_level,
    uint32_t p_active_emotion_mask,
    uint16_t p_primary_tome_id
) {
    const uint32_t slot = agent_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    if (auto *tw  = get_column_ptr<InstinctiveTraitWeightsPod>(MindBufferSlot::MIND_AGENT_TRAIT_WEIGHTS_POD)) tw[slot] = p_traits;
    if (auto *exc = get_column_ptr<float>(MindBufferSlot::MIND_AGENT_EXCITATION_LEVEL)) exc[slot] = p_excitation_level;
    if (auto *att = get_column_ptr<float>(MindBufferSlot::MIND_AGENT_ATTENTION_THRESHOLD)) att[slot] = p_attention_threshold;
    if (auto *kno = get_column_ptr<float>(MindBufferSlot::MIND_AGENT_KNOWLEDGE_THRESHOLD)) kno[slot] = p_knowledge_threshold;
    if (auto *emo = get_column_ptr<uint32_t>(MindBufferSlot::MIND_AGENT_ACTIVE_EMOTION_MASK)) emo[slot] = p_active_emotion_mask;
    if (auto *tom = get_column_ptr<uint16_t>(MindBufferSlot::MIND_AGENT_PRIMARY_TOME_ID)) tom[slot] = p_primary_tome_id;

    sync_selection_ranges();
    return slot;
}

InstinctiveTraitWeightsPod MindDOD::get_agent_traits(uint32_t p_agent_slot) const noexcept {
    if (p_agent_slot >= capacities_.max_agents) return InstinctiveTraitWeightsPod{};
    const auto *tw = get_column_ptr<InstinctiveTraitWeightsPod>(MindBufferSlot::MIND_AGENT_TRAIT_WEIGHTS_POD);
    return tw ? tw[p_agent_slot] : InstinctiveTraitWeightsPod{};
}

void MindDOD::set_agent_traits(uint32_t p_agent_slot, const InstinctiveTraitWeightsPod &p_traits) noexcept {
    if (p_agent_slot >= capacities_.max_agents) return;
    if (auto *tw = get_column_ptr<InstinctiveTraitWeightsPod>(MindBufferSlot::MIND_AGENT_TRAIT_WEIGHTS_POD)) {
        tw[p_agent_slot] = p_traits;
    }
}

// ============================================================================
// Table 3: AgentBeliefSparseTable (Epistemic Reference Catalogue)
// ============================================================================

uint32_t MindDOD::allocate_belief_catalogue_row(
    const BeliefPod &p_belief,
    const BeliefCatalogueEntryPod &p_catalogue_entry
) {
    const uint32_t slot = belief_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    if (auto *bpod = get_column_ptr<BeliefPod>(MindBufferSlot::MIND_BELIEF_POD)) bpod[slot] = p_belief;
    if (auto *cpod = get_column_ptr<BeliefCatalogueEntryPod>(MindBufferSlot::MIND_BELIEF_CATALOGUE_ENTRY_POD)) cpod[slot] = p_catalogue_entry;
    if (auto *cfid = get_column_ptr<uint32_t>(MindBufferSlot::MIND_BELIEF_CATALOGUED_FACT_ID)) cfid[slot] = p_catalogue_entry.catalogued_fact_id;
    if (auto *tfid = get_column_ptr<uint32_t>(MindBufferSlot::MIND_BELIEF_TRUE_FACT_ID)) tfid[slot] = p_catalogue_entry.true_fact_id;
    if (auto *btom = get_column_ptr<uint16_t>(MindBufferSlot::MIND_BELIEF_BELIEVED_TOME_ID)) btom[slot] = p_catalogue_entry.believed_tome_id;
    if (auto *ttom = get_column_ptr<uint16_t>(MindBufferSlot::MIND_BELIEF_TRUE_TOME_ID)) ttom[slot] = p_catalogue_entry.true_tome_id;
    if (auto *bval = get_column_ptr<int64_t>(MindBufferSlot::MIND_BELIEF_BELIEVED_VALUE)) bval[slot] = p_belief.believed_value;
    if (auto *pval = get_column_ptr<int64_t>(MindBufferSlot::MIND_BELIEF_PREV_BELIEVED_VALUE)) pval[slot] = p_belief.believed_value;
    if (auto *conf = get_column_ptr<float>(MindBufferSlot::MIND_BELIEF_CONFIDENCE)) conf[slot] = p_belief.confidence;
    if (auto *acc  = get_column_ptr<float>(MindBufferSlot::MIND_BELIEF_ACCEPTANCE)) acc[slot] = p_belief.acceptance;
    if (auto *emo  = get_column_ptr<float>(MindBufferSlot::MIND_BELIEF_EMOTIONAL_CHARGE)) emo[slot] = p_belief.emotional_charge;
    if (auto *intu = get_column_ptr<float>(MindBufferSlot::MIND_BELIEF_INTUITION_CHARGE)) intu[slot] = p_belief.intuition_charge;
    if (auto *src  = get_column_ptr<uint32_t>(MindBufferSlot::MIND_BELIEF_SOURCE_AGENT_ID)) src[slot] = p_belief.source_agent_id;
    if (auto *prio = get_column_ptr<uint8_t>(MindBufferSlot::MIND_BELIEF_STIMULUS_PRIORITY)) prio[slot] = p_catalogue_entry.stimulus_priority;
    if (auto *flg  = get_column_ptr<uint8_t>(MindBufferSlot::MIND_BELIEF_FLAGS)) flg[slot] = p_catalogue_entry.catalogue_flags;

    sync_selection_ranges();
    return slot;
}

BeliefPod MindDOD::get_belief_pod(uint32_t p_belief_slot) const noexcept {
    if (p_belief_slot >= capacities_.max_beliefs) return BeliefPod{};
    const auto *bpod = get_column_ptr<BeliefPod>(MindBufferSlot::MIND_BELIEF_POD);
    return bpod ? bpod[p_belief_slot] : BeliefPod{};
}

BeliefCatalogueEntryPod MindDOD::get_belief_catalogue_entry(uint32_t p_belief_slot) const noexcept {
    if (p_belief_slot >= capacities_.max_beliefs) return BeliefCatalogueEntryPod{};
    const auto *cpod = get_column_ptr<BeliefCatalogueEntryPod>(MindBufferSlot::MIND_BELIEF_CATALOGUE_ENTRY_POD);
    return cpod ? cpod[p_belief_slot] : BeliefCatalogueEntryPod{};
}

void MindDOD::bake_from_information_matrix(
    const InformationMatrixDOD &p_matrix,
    std::span<const uint32_t> p_agent_ids
) {
    tome_allocator_.clear();
    fact_allocator_.clear();
    belief_allocator_.clear();
    hypothesis_allocator_.clear();
    syllogism_allocator_.clear();
    explanation_allocator_.clear();

    // 1. Bake Akashic Tomes
    for (const auto &tome : p_matrix.akashic_tomes()) {
        (void)allocate_akashic_tome(tome);
    }

    // 2. Bake Ground-Truth Facts
    for (const auto &fact : p_matrix.ground_truth_facts()) {
        (void)allocate_ground_truth_fact(fact, fact.get_tome_id());
    }

    // 3. Bake Hypotheses, Syllogisms & Explanations
    for (const auto &h : p_matrix.hypotheses()) {
        (void)allocate_hypothesis(h);
    }
    for (const auto &s : p_matrix.syllogisms()) {
        (void)allocate_syllogism(s);
    }
    for (const auto &e : p_matrix.explanations()) {
        (void)allocate_explanation(e);
    }

    // 4. Bake Per-Agent Reference Catalogues (`BeliefPod` + `BeliefCatalogueEntryPod`) & CSR Offsets
    auto *csr_offsets = get_column_ptr<uint32_t>(MindBufferSlot::MIND_AGENT_BELIEF_CSR_OFFSETS);
    auto *csr_counts  = get_column_ptr<uint16_t>(MindBufferSlot::MIND_AGENT_BELIEF_CSR_COUNTS);

    for (size_t i = 0; i < p_agent_ids.size(); ++i) {
        const uint32_t agent_id = p_agent_ids[i];
        const uint32_t start_offset = belief_allocator_.get_high_water_mark();
        if (csr_offsets && i < capacities_.max_agents) {
            csr_offsets[i] = start_offset;
        }

        const auto *beliefs   = p_matrix.get_agent_beliefs(agent_id);
        const auto *catalogue = p_matrix.get_agent_catalogue(agent_id);
        uint16_t count = 0u;
        if (beliefs) {
            for (size_t j = 0; j < beliefs->size(); ++j) {
                BeliefCatalogueEntryPod cat_entry{};
                if (catalogue && j < catalogue->size()) {
                    cat_entry = (*catalogue)[j];
                } else {
                    cat_entry.catalogued_fact_id = (*beliefs)[j].fact_id;
                    cat_entry.true_fact_id       = (*beliefs)[j].fact_id;
                }
                if (allocate_belief_catalogue_row((*beliefs)[j], cat_entry) != INVALID_ID) {
                    ++count;
                }
            }
        }
        if (csr_counts && i < capacities_.max_agents) {
            csr_counts[i] = count;
        }
    }
    if (csr_offsets && p_agent_ids.size() <= capacities_.max_agents) {
        csr_offsets[p_agent_ids.size()] = belief_allocator_.get_high_water_mark();
    }

    sync_selection_ranges();
}

void MindDOD::sync_to_information_matrix(
    InformationMatrixDOD &r_matrix,
    std::span<const uint32_t> p_agent_ids
) const {
    const auto *csr_offsets = get_column_ptr<uint32_t>(MindBufferSlot::MIND_AGENT_BELIEF_CSR_OFFSETS);
    const auto *csr_counts  = get_column_ptr<uint16_t>(MindBufferSlot::MIND_AGENT_BELIEF_CSR_COUNTS);
    if (!csr_offsets || !csr_counts) return;

    const auto *bval = get_column_ptr<int64_t>(MindBufferSlot::MIND_BELIEF_BELIEVED_VALUE);
    const auto *conf = get_column_ptr<float>(MindBufferSlot::MIND_BELIEF_CONFIDENCE);
    const auto *acc  = get_column_ptr<float>(MindBufferSlot::MIND_BELIEF_ACCEPTANCE);
    const auto *emo  = get_column_ptr<float>(MindBufferSlot::MIND_BELIEF_EMOTIONAL_CHARGE);
    const auto *intu = get_column_ptr<float>(MindBufferSlot::MIND_BELIEF_INTUITION_CHARGE);
    const auto *src  = get_column_ptr<uint32_t>(MindBufferSlot::MIND_BELIEF_SOURCE_AGENT_ID);
    const auto *cfid = get_column_ptr<uint32_t>(MindBufferSlot::MIND_BELIEF_CATALOGUED_FACT_ID);
    const auto *tfid = get_column_ptr<uint32_t>(MindBufferSlot::MIND_BELIEF_TRUE_FACT_ID);
    const auto *btom = get_column_ptr<uint16_t>(MindBufferSlot::MIND_BELIEF_BELIEVED_TOME_ID);
    const auto *ttom = get_column_ptr<uint16_t>(MindBufferSlot::MIND_BELIEF_TRUE_TOME_ID);
    const auto *prio = get_column_ptr<uint8_t>(MindBufferSlot::MIND_BELIEF_STIMULUS_PRIORITY);

    for (size_t i = 0; i < p_agent_ids.size() && i < capacities_.max_agents; ++i) {
        const uint32_t agent_id = p_agent_ids[i];
        const uint32_t start    = csr_offsets[i];
        const uint16_t count    = csr_counts[i];
        for (uint16_t j = 0; j < count; ++j) {
            const uint32_t slot = start + j;
            if (slot >= belief_allocator_.get_high_water_mark()) break;

            BeliefPod b = get_belief_pod(slot);
            if (cfid) b.fact_id          = cfid[slot];
            if (src)  b.source_agent_id  = src[slot];
            if (bval) b.believed_value   = bval[slot];
            if (conf) b.confidence       = conf[slot];
            if (acc)  b.acceptance       = acc[slot];
            if (emo)  b.emotional_charge = emo[slot];
            if (intu) b.intuition_charge = intu[slot];

            const uint32_t true_f   = tfid ? tfid[slot] : b.fact_id;
            const uint16_t bel_tome = btom ? btom[slot] : TOME_PHYSICAL_REALITY;
            const uint16_t tru_tome = ttom ? ttom[slot] : TOME_PHYSICAL_REALITY;
            const auto st_prio      = prio ? static_cast<StimulusPriority>(prio[slot]) : StimulusPriority::OLD_NEUTRAL_UNCHANGED;

            r_matrix.upsert_agent_belief(agent_id, b, true_f, bel_tome, tru_tome, st_prio);
        }
    }
}

// ============================================================================
// Table 4 & Table 5: Reasoning, Theories & Spoken Dialogue Staging
// ============================================================================

uint32_t MindDOD::allocate_observation(const ObservationPod &p_obs) {
    const uint32_t slot = observation_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    if (auto *obs = get_column_ptr<ObservationPod>(MindBufferSlot::MIND_OBSERVATION_POD)) {
        obs[slot] = p_obs;
    }
    sync_selection_ranges();
    return slot;
}

uint32_t MindDOD::allocate_hypothesis(const HypothesisPod &p_hyp) {
    const uint32_t slot = hypothesis_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    if (auto *hyp = get_column_ptr<HypothesisPod>(MindBufferSlot::MIND_HYPOTHESIS_POD)) {
        hyp[slot] = p_hyp;
    }
    sync_selection_ranges();
    return slot;
}

uint32_t MindDOD::allocate_evidence(const EvidencePod &p_ev) {
    const uint32_t slot = evidence_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    if (auto *ev = get_column_ptr<EvidencePod>(MindBufferSlot::MIND_EVIDENCE_POD)) {
        ev[slot] = p_ev;
    }
    sync_selection_ranges();
    return slot;
}

uint32_t MindDOD::allocate_syllogism(const SyllogisticPropositionPod &p_syl) {
    const uint32_t slot = syllogism_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    if (auto *syl = get_column_ptr<SyllogisticPropositionPod>(MindBufferSlot::MIND_SYLLOGISM_PROPOSITION_POD)) {
        syl[slot] = p_syl;
    }
    sync_selection_ranges();
    return slot;
}

uint32_t MindDOD::allocate_explanation(
    const ExplanationPod &p_exp,
    std::span<const uint32_t> p_fact_ids,
    std::span<const uint32_t> p_cause_ids
) {
    if (explanation_fact_csr_cursor_ + p_fact_ids.size() > capacities_.max_explanation_csr_indices ||
        explanation_cause_csr_cursor_ + p_cause_ids.size() > capacities_.max_explanation_csr_indices) {
        return INVALID_ID;
    }
    const uint32_t slot = explanation_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    ExplanationPod copy = p_exp;
    if (!p_fact_ids.empty()) {
        if (auto *f_csr = get_column_ptr<uint32_t>(MindBufferSlot::MIND_EXPLANATION_FACT_CSR_IDS)) {
            copy.fact_slice_offset = explanation_fact_csr_cursor_;
            copy.fact_slice_count  = static_cast<uint16_t>(p_fact_ids.size());
            std::copy(p_fact_ids.begin(), p_fact_ids.end(), f_csr + explanation_fact_csr_cursor_);
            explanation_fact_csr_cursor_ += copy.fact_slice_count;
        }
    }
    if (!p_cause_ids.empty()) {
        if (auto *c_csr = get_column_ptr<uint32_t>(MindBufferSlot::MIND_EXPLANATION_CAUSE_CSR_IDS)) {
            copy.cause_slice_offset = explanation_cause_csr_cursor_;
            copy.cause_slice_count  = static_cast<uint16_t>(p_cause_ids.size());
            std::copy(p_cause_ids.begin(), p_cause_ids.end(), c_csr + explanation_cause_csr_cursor_);
            explanation_cause_csr_cursor_ += copy.cause_slice_count;
        }
    }

    if (auto *exps = get_column_ptr<ExplanationPod>(MindBufferSlot::MIND_EXPLANATION_POD)) {
        exps[slot] = copy;
    }
    sync_selection_ranges();
    return slot;
}

uint32_t MindDOD::allocate_discourse_turn(
    const DiscourseTurnPod &p_turn,
    uint64_t p_speaker_mask,
    uint64_t p_listener_mask
) {
    const uint32_t slot = discourse_turn_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    if (auto *turns = get_column_ptr<DiscourseTurnPod>(MindBufferSlot::MIND_DISCOURSE_TURN_POD)) turns[slot] = p_turn;
    if (auto *smask = get_column_ptr<uint64_t>(MindBufferSlot::MIND_DISCOURSE_SPEAKER_MASK)) smask[slot] = p_speaker_mask;
    if (auto *lmask = get_column_ptr<uint64_t>(MindBufferSlot::MIND_DISCOURSE_LISTENER_MASK)) lmask[slot] = p_listener_mask;
    if (auto *spl   = get_column_ptr<uint32_t>(MindBufferSlot::MIND_DISCOURSE_PROSODY_SPLINE_ID)) spl[slot] = p_turn.prosody_spline_2d_id;

    sync_selection_ranges();
    return slot;
}

// ============================================================================
// Templated Task Node Builder (MND1 - MND12) & Pipeline Topology
// ============================================================================

template <typename T_Logic>
NodeID MindDOD::add_mind_task_node(
    TaskGraphDOD &r_graph,
    const T_Logic &p_logic_override,
    const MindsNodeExecutionConfig &p_config
) const {
    using Descriptor = MindTaskDescriptor<T_Logic>;

    T_Logic logic = p_logic_override;
    T_Logic defaults{};
    godot::Dictionary manifest_props;
    populate_task_buffer_dictionary(manifest_props, true);
    defaults.apply_properties(manifest_props);
    _bind_default_logic_buffers(&logic, sizeof(T_Logic), &defaults);

    const NodeID node_id = r_graph.add_task_node(IsQueryLogic<T_Logic> ? TaskTypeDOD::QUERY_CULLER : TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    if constexpr (requires { T_Logic::transient_workspace_bytes; }) {
        if (T_Logic::transient_workspace_bytes > 0u) {
            r_graph.set_node_transient_requirement(node_id, static_cast<uint32_t>(T_Logic::transient_workspace_bytes));
        }
    }

    std::unique_ptr<INativeTask> native_task;
    if (p_config.use_registry_factory) {
        if constexpr (IsTransformLogic<T_Logic>) {
            native_task = MindsTransformTaskRegistry::create(
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        } else if constexpr (IsQueryLogic<T_Logic>) {
            native_task = MindsQueryTaskRegistry::create(
                static_cast<uint32_t>(p_config.query_op),
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        } else if constexpr (IsMetadataLogic<T_Logic>) {
            native_task = MindsMetadataTaskRegistry::create(
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        }
        if (native_task) {
            manifest_props["target_buffer_id"] = static_cast<int64_t>(logic.get_target_buffer_id());
            native_task->apply_properties(manifest_props);
        }
    }

    if (!native_task) {
        if constexpr (IsTransformLogic<T_Logic>) {
            native_task = std::make_unique<TransformTask<T_Logic>>(logic);
        } else if constexpr (IsQueryLogic<T_Logic>) {
            native_task = (p_config.query_op == QueryOp::ADD)
                ? std::unique_ptr<INativeTask>(std::make_unique<QueryTask<T_Logic, QueryOp::ADD>>(logic))
                : std::unique_ptr<INativeTask>(std::make_unique<QueryTask<T_Logic, QueryOp::CULL>>(logic));
        } else if constexpr (IsMetadataLogic<T_Logic>) {
            native_task = std::make_unique<MetadataTask<T_Logic>>(logic);
        }
    }

    r_graph.configure_native_interface(node_id, std::move(native_task));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        std::array<GrantPartPOD, 4> reqs = {{
            make_slot_requirement(Descriptor::default_grants[0].slot, Descriptor::default_grants[0].access),
            make_slot_requirement(Descriptor::default_grants[1].slot, Descriptor::default_grants[1].access),
            make_slot_requirement(Descriptor::default_grants[2].slot, Descriptor::default_grants[2].access),
            make_slot_requirement(Descriptor::default_grants[3].slot, Descriptor::default_grants[3].access)
        }};
        const uint32_t target_id = logic.get_target_buffer_id();
        if (target_id != INVALID_ID && reqs[0].buffer_id != target_id) {
            reqs[0] = make_buffer_requirement(target_id, Descriptor::default_grants[0].access);
        }
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

#define INSTANTIATE_MIND_TASK_NODE(LogicType) \
    template NodeID MindDOD::add_mind_task_node<LogicType>(TaskGraphDOD &, const LogicType &, const MindsNodeExecutionConfig &) const

INSTANTIATE_MIND_TASK_NODE(SensorySignalDecodeTransformLogic<>);
INSTANTIATE_MIND_TASK_NODE(SensoryObservationWakeQueryLogic<>);
INSTANTIATE_MIND_TASK_NODE(InstinctiveAffectAppraisalTransformLogic<>);
INSTANTIATE_MIND_TASK_NODE(StimulusLatencyTriageMetadataLogic<>);
INSTANTIATE_MIND_TASK_NODE(AbductiveHypothesisTransformLogic<>);
INSTANTIATE_MIND_TASK_NODE(IntuitiveLeapStochasticQueryLogic<>);
INSTANTIATE_MIND_TASK_NODE(InductiveEvidenceAccumulateTransformLogic<>);
INSTANTIATE_MIND_TASK_NODE(DeductiveSyllogismPropagateTransformLogic<>);
INSTANTIATE_MIND_TASK_NODE(DeliberationAndDeceptionTransformLogic<>);
INSTANTIATE_MIND_TASK_NODE(DiscourseTurnAndProsodyTransformLogic<>);
INSTANTIATE_MIND_TASK_NODE(ContradictionAndTrustReconcileTransformLogic<>);
INSTANTIATE_MIND_TASK_NODE(SememeAndLexiconMorphologyTransformLogic<>);

#undef INSTANTIATE_MIND_TASK_NODE

NodeID MindDOD::add_transform_node_by_id(
    TaskGraphDOD &r_graph,
    MindsTransformLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const MindsNodeExecutionConfig &p_config
) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_custom_props.is_empty()) l.apply_properties(p_custom_props);
        return add_mind_task_node(r_graph, l, p_config);
    };
    switch (p_logic_id) {
        case MindsTransformLogicID::AbductiveHypothesis:            return build.template operator()<AbductiveHypothesisTransformLogic<>>();
        case MindsTransformLogicID::ContradictionAndTrustReconcile: return build.template operator()<ContradictionAndTrustReconcileTransformLogic<>>();
        case MindsTransformLogicID::DeductiveSyllogismPropagate:    return build.template operator()<DeductiveSyllogismPropagateTransformLogic<>>();
        case MindsTransformLogicID::DeliberationAndDeception:       return build.template operator()<DeliberationAndDeceptionTransformLogic<>>();
        case MindsTransformLogicID::DiscourseTurnAndProsody:        return build.template operator()<DiscourseTurnAndProsodyTransformLogic<>>();
        case MindsTransformLogicID::InductiveEvidenceAccumulate:    return build.template operator()<InductiveEvidenceAccumulateTransformLogic<>>();
        case MindsTransformLogicID::InstinctiveAffectAppraisal:     return build.template operator()<InstinctiveAffectAppraisalTransformLogic<>>();
        case MindsTransformLogicID::SememeAndLexiconMorphology:     return build.template operator()<SememeAndLexiconMorphologyTransformLogic<>>();
        case MindsTransformLogicID::SensorySignalDecode:            return build.template operator()<SensorySignalDecodeTransformLogic<>>();
        default:                                                    return INVALID_ID;
    }
}

NodeID MindDOD::add_query_node_by_id(
    TaskGraphDOD &r_graph,
    MindsQueryLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const MindsNodeExecutionConfig &p_config
) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_custom_props.is_empty()) l.apply_properties(p_custom_props);
        return add_mind_task_node(r_graph, l, p_config);
    };
    switch (p_logic_id) {
        case MindsQueryLogicID::IntuitiveLeapStochastic: return build.template operator()<IntuitiveLeapStochasticQueryLogic<>>();
        case MindsQueryLogicID::SensoryObservationWake:  return build.template operator()<SensoryObservationWakeQueryLogic<>>();
        default:                                         return INVALID_ID;
    }
}

NodeID MindDOD::add_metadata_node_by_id(
    TaskGraphDOD &r_graph,
    MindsMetadataLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const MindsNodeExecutionConfig &p_config
) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_custom_props.is_empty()) l.apply_properties(p_custom_props);
        return add_mind_task_node(r_graph, l, p_config);
    };
    switch (p_logic_id) {
        case MindsMetadataLogicID::StimulusLatencyTriage: return build.template operator()<StimulusLatencyTriageMetadataLogic<>>();
        default:                                          return INVALID_ID;
    }
}

MindsPipelineTopology MindDOD::inject_pipeline_stages(
    TaskGraphDOD &r_graph,
    uint32_t p_stage_mask,
    bool p_auto_connect_stages
) {
    MindsPipelineTopology topo{};
    bool prev_was_parallel = false;

    auto append_stage = [&](MindsPipelineStageFlags p_flag, const std::vector<NodeID> &p_nodes, bool p_chain_internal) {
        if (p_nodes.empty()) return;
        MindsStageAnchor anchor{p_flag, p_nodes.front(), p_nodes.back(), p_nodes};

        if (p_chain_internal) {
            for (size_t i = 0; i + 1 < p_nodes.size(); ++i) {
                r_graph.connect_nodes(p_nodes[i], 0, p_nodes[i + 1], 0);
            }
        }
        if (p_auto_connect_stages && !topo.stages.empty()) {
            const MindsStageAnchor &prev = topo.stages.back();
            const std::vector<NodeID> up_exits = prev_was_parallel ? prev.nodes : std::vector<NodeID>{prev.exit_node};
            const std::vector<NodeID> down_entries = p_chain_internal ? std::vector<NodeID>{anchor.entry_node} : anchor.nodes;
            for (NodeID u : up_exits) {
                for (NodeID d : down_entries) {
                    r_graph.connect_nodes(u, 0, d, 0);
                }
            }
        }
        if (topo.first_node == INVALID_ID) topo.first_node = anchor.entry_node;
        topo.last_node    = anchor.exit_node;
        prev_was_parallel = !p_chain_internal;
        topo.stages.push_back(std::move(anchor));
    };

    // Wave 1: Sensory Decode & Wake (MND1 -> MND2)
    if (p_stage_mask & MINDS_STAGE_WAVE_1_SENSORY_PERCEPTION) {
        MindsNodeExecutionConfig wake_cfg{};
        wake_cfg.query_op = QueryOp::ADD;
        append_stage(
            MINDS_STAGE_WAVE_1_SENSORY_PERCEPTION,
            {
                add_sensory_signal_decode_node(r_graph),
                add_sensory_observation_wake_node(r_graph, {}, wake_cfg)
            },
            true
        );
    }

    // Wave 2: Instinctive Emotional Appraisal (MND3)
    if (p_stage_mask & MINDS_STAGE_WAVE_2_EMOTIONAL_APPRAISAL) {
        append_stage(
            MINDS_STAGE_WAVE_2_EMOTIONAL_APPRAISAL,
            {add_instinctive_affect_appraisal_node(r_graph)},
            true
        );
    }

    // Wave 3: Stimulus Latency Triage & Abductive / Intuitive / Inductive Reasoning (MND4 -> MND5 -> MND6 -> MND7)
    if (p_stage_mask & MINDS_STAGE_WAVE_3_LATENCY_AND_REASONING) {
        MindsNodeExecutionConfig leap_cfg{};
        leap_cfg.query_op = QueryOp::ADD;
        append_stage(
            MINDS_STAGE_WAVE_3_LATENCY_AND_REASONING,
            {
                add_stimulus_latency_triage_node(r_graph),
                add_abductive_hypothesis_node(r_graph),
                add_intuitive_leap_stochastic_node(r_graph, {}, leap_cfg),
                add_inductive_evidence_accumulate_node(r_graph)
            },
            true
        );
    }

    // Wave 4: Deductive Syllogisms, Deceptive Deliberation & Spoken Discourse Prosody (MND8 -> MND9 -> MND10)
    if (p_stage_mask & MINDS_STAGE_WAVE_4_DEDUCTION_AND_DISCOURSE) {
        append_stage(
            MINDS_STAGE_WAVE_4_DEDUCTION_AND_DISCOURSE,
            {
                add_deductive_syllogism_propagate_node(r_graph),
                add_deliberation_and_deception_node(r_graph),
                add_discourse_turn_and_prosody_node(r_graph)
            },
            true
        );
    }

    // Wave 5: Epistemic Contradiction Reconciliation & Sememe/Lexicon Morphology (MND11 -> MND12)
    if (p_stage_mask & MINDS_STAGE_WAVE_5_REFLECTION_AND_SEMEMES) {
        append_stage(
            MINDS_STAGE_WAVE_5_REFLECTION_AND_SEMEMES,
            {
                add_contradiction_and_trust_reconcile_node(r_graph),
                add_sememe_and_lexicon_morphology_node(r_graph)
            },
            true
        );
    }

    active_topology_ = topo;
    return topo;
}

bool MindDOD::schedule_node_with_stage(
    TaskGraphDOD &r_graph,
    const MindsPipelineTopology &p_topology,
    MindsPipelineStageFlags p_target_stage,
    NodeID p_custom_node,
    bool p_join_downstream
) {
    if (p_custom_node == INVALID_ID) return false;
    for (size_t i = 0; i < p_topology.stages.size(); ++i) {
        if (p_topology.stages[i].stage == p_target_stage) {
            if (i > 0 && p_topology.stages[i - 1].exit_node != INVALID_ID) {
                r_graph.connect_nodes(p_topology.stages[i - 1].exit_node, 0, p_custom_node, 0);
            }
            if (p_join_downstream && (i + 1) < p_topology.stages.size() && p_topology.stages[i + 1].entry_node != INVALID_ID) {
                r_graph.connect_nodes(p_custom_node, 0, p_topology.stages[i + 1].entry_node, 0);
            }
            return true;
        }
    }
    return false;
}

bool MindDOD::schedule_node_between_stages(
    TaskGraphDOD &r_graph,
    const MindsPipelineTopology &p_topology,
    MindsPipelineStageFlags p_upstream_stage,
    MindsPipelineStageFlags p_downstream_stage,
    NodeID p_custom_node
) {
    if (p_custom_node == INVALID_ID) return false;
    const MindsStageAnchor *up   = p_topology.get_stage_anchor(p_upstream_stage);
    const MindsStageAnchor *down = p_topology.get_stage_anchor(p_downstream_stage);
    if (!up || !down || !up->is_valid() || !down->is_valid()) return false;

    r_graph.connect_nodes(up->exit_node, 0, p_custom_node, 0);
    r_graph.connect_nodes(p_custom_node, 0, down->entry_node, 0);
    return true;
}

void MindDOD::execute_step(double p_delta, TaskGraphDOD *p_override_graph) {
    sync_selection_ranges();
    if (TaskGraphDOD *target_graph = p_override_graph ? p_override_graph : default_task_graph_.get()) {
        if (target_graph->get_node_count() > 0) {
            MemoryGrantPOD topo_grant{};
            if (target_graph->bake_topology_grant(topo_grant) && memory_manager_) {
                memory_manager_->release_grant(topo_grant);
            }
        }
        target_graph->execute_graph_dod(p_delta);
    }
}

} // namespace ideam::core
