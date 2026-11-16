#include "linguistics_dod.h"

#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

constexpr size_t DEFAULT_LINGUISTICS_MASTER_BLOCK_BYTES    = 8ULL * 1024ULL * 1024ULL; // 8 MB
constexpr size_t DEFAULT_LINGUISTICS_TRANSIENT_BLOCK_BYTES = 2ULL * 1024ULL * 1024ULL; // 2 MB

struct LinguisticsSlotAccessSpec {
    LinguisticsBufferSlot slot = LinguisticsBufferSlot::COUNT;
    BufferAccessMode access    = BufferAccessMode::READ;
};

template <typename T_Logic>
struct LinguisticsTaskDescriptor;

#define DEFINE_LINGUISTICS_TASK_DESCRIPTOR(LogicStruct, EnumVal, S0, A0, S1, A1, S2, A2, S3, A3) \
    template <typename T>                                                                        \
    struct LinguisticsTaskDescriptor<LogicStruct<T>> {                                           \
        static constexpr auto logic_id = EnumVal;                                                \
        static constexpr std::array<LinguisticsSlotAccessSpec, 4> default_grants = {{            \
            {LinguisticsBufferSlot::S0, BufferAccessMode::A0},                                   \
            {LinguisticsBufferSlot::S1, BufferAccessMode::A1},                                   \
            {LinguisticsBufferSlot::S2, BufferAccessMode::A2},                                   \
            {LinguisticsBufferSlot::S3, BufferAccessMode::A3}                                    \
        }};                                                                                      \
    }

// --- Compile-Time Task Grant & LogicID Table (LNG1 - LNG7) ---
DEFINE_LINGUISTICS_TASK_DESCRIPTOR(
    LexiconPhonemeFeatureIndexMetadataLogic,
    LinguisticsMetadataLogicID::LexiconPhonemeFeatureIndex,
    LING_SEMEME_DESCRIPTOR_POD, READ,
    LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0, READ_WRITE,
    LING_SEMEME_MRL_BINARY_LO64, READ_WRITE,
    LING_SEMEME_COHERENCE_MASK32, READ_WRITE
);

DEFINE_LINGUISTICS_TASK_DESCRIPTOR(
    WordNetSynsetHypernymPropagateTransformLogic,
    LinguisticsTransformLogicID::WordNetSynsetHypernymPropagate,
    LING_SEMEME_DESCRIPTOR_POD, READ_WRITE,
    LING_SYNSET_RELATION_POD, READ,
    LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0, READ_WRITE,
    LING_SEMEME_COHERENCE_MASK32, READ_WRITE
);

DEFINE_LINGUISTICS_TASK_DESCRIPTOR(
    SemeComponentialAffinityQueryLogic,
    LinguisticsQueryLogicID::SemeComponentialAffinity,
    LING_SEMEME_AFFINITY_SCORE, READ_WRITE,
    LING_SEMEME_DESCRIPTOR_POD, READ,
    LING_QUERY_CONTEXT_POD, READ,
    LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0, READ
);

DEFINE_LINGUISTICS_TASK_DESCRIPTOR(
    MatryoshkaBinaryHammingQueryLogic,
    LinguisticsQueryLogicID::MatryoshkaBinaryHamming,
    LING_SEMEME_AFFINITY_SCORE, READ_WRITE,
    LING_SEMEME_MRL_BINARY_LO64, READ,
    LING_SEMEME_MRL_BINARY_HI64, READ,
    LING_QUERY_CONTEXT_POD, READ
);

DEFINE_LINGUISTICS_TASK_DESCRIPTOR(
    PhoneticHammingLexemeDecodeTransformLogic,
    LinguisticsTransformLogicID::PhoneticHammingLexemeDecode,
    LING_PHONEME_UTTERANCE_SPAN_POD, READ_WRITE,
    LING_PHONEME_FEATURE_STREAM, READ,
    LING_LEXEME_POD, READ,
    LING_DECODED_SEMEME_STREAM, READ_WRITE
);

DEFINE_LINGUISTICS_TASK_DESCRIPTOR(
    BPESubwordAndMorphologyTransformLogic,
    LinguisticsTransformLogicID::BPESubwordAndMorphology,
    LING_DECODED_SEMEME_STREAM, READ_WRITE,
    LING_BPE_MERGE_RULE_POD, READ,
    LING_BPE_VOCAB_POD, READ,
    LING_SEMEME_DESCRIPTOR_POD, READ
);

DEFINE_LINGUISTICS_TASK_DESCRIPTOR(
    SemanticFrameAndSLMMaskTransformLogic,
    LinguisticsTransformLogicID::SemanticFrameAndSLMMask,
    LING_FRAME_CLAUSE_INSTANCE_POD, READ_WRITE,
    LING_SEMANTIC_FRAME_POD, READ,
    LING_SEMEME_DESCRIPTOR_POD, READ_WRITE,
    LING_SLM_VOCAB_LOGIT_MASK, READ_WRITE
);

#undef DEFINE_LINGUISTICS_TASK_DESCRIPTOR

} // anonymous namespace

LinguisticsDOD::LinguisticsDOD(
    std::shared_ptr<MemoryManagerDOD> p_memory_manager,
    const LinguisticsBufferCapacities &p_capacities
)
    : memory_manager_(std::move(p_memory_manager)),
      capacities_(p_capacities) {
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_LINGUISTICS_MASTER_BLOCK_BYTES,
            DEFAULT_LINGUISTICS_TRANSIENT_BLOCK_BYTES
        );
    }
    default_task_graph_ = std::make_shared<TaskGraphDOD>(memory_manager_.get());
    default_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
    initialize_buffers(capacities_);
}

void LinguisticsDOD::initialize_buffers(const LinguisticsBufferCapacities &p_capacities) {
    capacities_ = p_capacities;
    if (!memory_manager_ && !raw_memory_override_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_LINGUISTICS_MASTER_BLOCK_BYTES,
            DEFAULT_LINGUISTICS_TRANSIENT_BLOCK_BYTES
        );
    }
    MemoryManagerDOD *mgr = memory_manager();
    if (!mgr) return;

    sememe_allocator_.initialize(capacities_.max_sememes);
    lexeme_allocator_.initialize(capacities_.max_lexemes);
    morpheme_allocator_.initialize(capacities_.max_morphemes);
    bpe_vocab_allocator_.initialize(capacities_.max_bpe_vocab);
    bpe_merge_allocator_.initialize(capacities_.max_bpe_merge_rules);
    synset_relation_allocator_.initialize(capacities_.max_synset_relations);
    semantic_frame_allocator_.initialize(capacities_.max_semantic_frames);
    frame_clause_allocator_.initialize(capacities_.max_frame_clause_instances);
    utterance_span_allocator_.initialize(capacities_.max_utterance_spans);

    for (size_t i = 0; i < LINGUISTICS_BUFFER_SLOT_COUNT; ++i) {
        const LinguisticsBufferSpec &spec = LINGUISTICS_BUFFER_SPECS[i];
        const size_t max_elems = std::max<size_t>(1u, capacities_.get_capacity(spec.capacity_kind));
        const size_t bytes     = max_elems * spec.element_stride;
        const uint32_t buf_id  = mgr->create_shadowed_buffer(
            spec.layout, bytes, static_cast<int64_t>(max_elems), spec.selection_mode
        );
        if (MemoryBufferPOD *buf = mgr->get_buffer(buf_id)) {
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

uint32_t LinguisticsDOD::_active_count_for_group(LinguisticsTableGroup p_group) const noexcept {
    switch (p_group) {
        case LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE:     return sememe_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::LEXEME_TABLE:                 return lexeme_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::LEXEME_PHONEME_CSR_SLICE:     return lexeme_phoneme_csr_cursor_;
        case LinguisticsTableGroup::MORPHEME_TABLE:               return morpheme_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::BPE_VOCAB_TABLE:              return bpe_vocab_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::BPE_MERGE_RULE_TABLE:         return bpe_merge_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::SYNSET_RELATION_TABLE:        return synset_relation_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::SEMANTIC_FRAME_TABLE:         return semantic_frame_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::FRAME_CLAUSE_INSTANCE_TABLE:  return frame_clause_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::PHONEME_FEATURE_STREAM_SLICE: return phoneme_stream_cursor_;
        case LinguisticsTableGroup::UTTERANCE_SPAN_TABLE:         return utterance_span_allocator_.get_high_water_mark();
        case LinguisticsTableGroup::DECODED_SEMEME_STREAM_SLICE:  return decoded_sememe_stream_cursor_;
        case LinguisticsTableGroup::SLM_VOCAB_MASK_SLICE:         return slm_vocab_mask_cursor_;
        case LinguisticsTableGroup::QUERY_CONTEXT_SINGLETON:      return 2u;
        default:                                                  return 0u;
    }
}

void LinguisticsDOD::sync_selection_ranges() noexcept {
    MemoryManagerDOD *mgr = memory_manager();
    if (!mgr) return;
    for (size_t i = 0; i < LINGUISTICS_BUFFER_SLOT_COUNT; ++i) {
        MemoryBufferPOD *buf = mgr->get_buffer(manifest_.buffer_ids[i]);
        if (!buf) continue;
        const LinguisticsBufferSpec &spec = LINGUISTICS_BUFFER_SPECS[i];
        buf->current_count = std::min<int64_t>(
            static_cast<int64_t>(_active_count_for_group(spec.table_group)),
            buf->max_elements
        );
        buf->used_bytes = static_cast<size_t>(buf->current_count) * buf->element_stride;
    }
}

void LinguisticsDOD::reset_all_slots() noexcept {
    sememe_allocator_.clear();
    lexeme_allocator_.clear();
    morpheme_allocator_.clear();
    bpe_vocab_allocator_.clear();
    bpe_merge_allocator_.clear();
    synset_relation_allocator_.clear();
    semantic_frame_allocator_.clear();
    frame_clause_allocator_.clear();
    utterance_span_allocator_.clear();

    lexeme_phoneme_csr_cursor_    = 0u;
    phoneme_stream_cursor_        = 0u;
    decoded_sememe_stream_cursor_ = 0u;
    slm_vocab_mask_cursor_        = 0u;

    auto aff_span = get_column_span<float>(LinguisticsBufferSlot::LING_SEMEME_AFFINITY_SCORE);
    std::fill(aff_span.begin(), aff_span.end(), 0.0f);

    sync_selection_ranges();
}

void LinguisticsDOD::populate_task_buffer_dictionary(godot::Dictionary &r_props, bool p_overwrite_existing) const {
    auto set_key = [&](std::string_view p_key, uint32_t p_val) {
        if (p_key.empty() || p_val == INVALID_ID) return;
        const godot::String key_str(std::string(p_key).c_str());
        if (p_overwrite_existing || !r_props.has(key_str)) {
            r_props[key_str] = static_cast<int64_t>(p_val);
        }
    };
    for (size_t i = 0; i < LINGUISTICS_BUFFER_SLOT_COUNT; ++i) {
        const uint32_t buf_id = manifest_.buffer_ids[i];
        set_key(LINGUISTICS_BUFFER_SPECS[i].canonical_name, buf_id);
        for (std::string_view alias : LINGUISTICS_BUFFER_SPECS[i].aliases) {
            set_key(alias, buf_id);
        }
    }
}

GrantPartPOD LinguisticsDOD::make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept {
    MemoryManagerDOD *mgr = memory_manager();
    if (mgr && p_buffer_id != INVALID_ID) {
        if (const MemoryBufferPOD *buf = mgr->get_buffer(p_buffer_id)) {
            const int64_t active_elems = (buf->current_count > 0) ? buf->current_count : buf->max_elements;
            return make_grant_requirement(p_buffer_id, p_access, buf->element_stride, active_elems, ((buf->layout_type == BufferLayoutType::SPARSE_SET) ? SelectionMode::SPARSE : SelectionMode::RANGE));
        }
    }
    return make_grant_requirement(p_buffer_id, p_access, sizeof(uint32_t), 0);
}

void LinguisticsDOD::_bind_default_logic_buffers(void *r_logic_bytes, size_t p_size, const void *p_default_bytes) const noexcept {
    auto *dst       = static_cast<uint32_t *>(r_logic_bytes);
    const auto *src = static_cast<const uint32_t *>(p_default_bytes);
    for (size_t i = 0, n = p_size / sizeof(uint32_t); i < n; ++i) {
        if (dst[i] == INVALID_ID && src[i] != INVALID_ID) {
            dst[i] = src[i];
        }
    }
}

// ============================================================================
// Table 1: Sememe & Object Profile Staging + Query Context
// ============================================================================

uint32_t LinguisticsDOD::allocate_sememe_descriptor(const SememeDescriptorPOD &p_desc) {
    const uint32_t slot = sememe_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    set_sememe_descriptor(slot, p_desc);
    sync_selection_ranges();
    return slot;
}

SememeDescriptorPOD LinguisticsDOD::get_sememe_descriptor(uint32_t p_slot) const noexcept {
    if (p_slot >= capacities_.max_sememes) return SememeDescriptorPOD{};
    const auto *pods = get_column_ptr<SememeDescriptorPOD>(LinguisticsBufferSlot::LING_SEMEME_DESCRIPTOR_POD);
    return pods ? pods[p_slot] : SememeDescriptorPOD{};
}

void LinguisticsDOD::set_sememe_descriptor(uint32_t p_slot, const SememeDescriptorPOD &p_desc) noexcept {
    if (p_slot >= capacities_.max_sememes) return;
    if (auto *pods = get_column_ptr<SememeDescriptorPOD>(LinguisticsBufferSlot::LING_SEMEME_DESCRIPTOR_POD)) {
        pods[p_slot] = p_desc;
    }
    if (auto *sym = get_column_ptr<uint64_t>(LinguisticsBufferSlot::LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0)) {
        sym[p_slot] = p_desc.nuclear_seme_words[0];
    }
    if (auto *lo = get_column_ptr<uint64_t>(LinguisticsBufferSlot::LING_SEMEME_MRL_BINARY_LO64)) {
        lo[p_slot] = p_desc.nuclear_seme_words[1];
    }
    if (auto *hi = get_column_ptr<uint64_t>(LinguisticsBufferSlot::LING_SEMEME_MRL_BINARY_HI64)) {
        hi[p_slot] = p_desc.nuclear_seme_words[2];
    }
    if (auto *coh = get_column_ptr<uint32_t>(LinguisticsBufferSlot::LING_SEMEME_COHERENCE_MASK32)) {
        coh[p_slot] = p_desc.compute_coherence_trait_mask32();
    }
    if (auto *upos = get_column_ptr<uint32_t>(LinguisticsBufferSlot::LING_SEMEME_UPOS_AND_DOMAIN)) {
        upos[p_slot] = static_cast<uint32_t>(p_desc.taxeme_category) |
                       (static_cast<uint32_t>(p_desc.sememic_form) << 8u) |
                       (static_cast<uint32_t>(p_desc.primary_domain_id) << 16u);
    }
}

uint32_t LinguisticsDOD::stage_object_profiles(std::span<const LinguisticProfilePOD> p_profiles) {
    uint32_t first_slot = INVALID_ID;
    for (const LinguisticProfilePOD &prof : p_profiles) {
        const uint32_t slot = allocate_sememe_descriptor(prof.to_sememe_descriptor());
        if (first_slot == INVALID_ID) {
            first_slot = slot;
        }
    }
    return first_slot;
}

void LinguisticsDOD::set_query_context(
    const SememeDescriptorPOD &p_required_query,
    const SememeDescriptorPOD &p_excluded_query
) noexcept {
    if (auto *q_pods = get_column_ptr<SememeDescriptorPOD>(LinguisticsBufferSlot::LING_QUERY_CONTEXT_POD)) {
        q_pods[0] = p_required_query;
        q_pods[1] = p_excluded_query;
    }
    sync_selection_ranges();
}

float LinguisticsDOD::get_sememe_affinity_score(uint32_t p_slot) const noexcept {
    if (p_slot >= capacities_.max_sememes) return 0.0f;
    const auto *scores = get_column_ptr<float>(LinguisticsBufferSlot::LING_SEMEME_AFFINITY_SCORE);
    return scores ? scores[p_slot] : 0.0f;
}

// ============================================================================
// Table 2 & 3: SemanticLexiconCore Single-Pass Bake & Readback
// ============================================================================

uint32_t LinguisticsDOD::allocate_lexeme(
    const LexemePOD &p_lexeme,
    std::span<const uint16_t> p_phoneme_masks
) {
    const uint32_t slot = lexeme_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    LexemePOD copy = p_lexeme;
    copy.lexeme_id = slot;

    auto *csr_col = get_column_ptr<uint16_t>(LinguisticsBufferSlot::LING_LEXEME_PHONEME_CSR_MASKS);
    if (csr_col && !p_phoneme_masks.empty() &&
        lexeme_phoneme_csr_cursor_ + p_phoneme_masks.size() <= capacities_.max_phoneme_csr_masks) {
        copy.phoneme_csr_offset = lexeme_phoneme_csr_cursor_;
        copy.phoneme_count      = static_cast<uint16_t>(p_phoneme_masks.size());
        for (uint16_t m : p_phoneme_masks) {
            csr_col[lexeme_phoneme_csr_cursor_++] = m;
        }
    }

    if (auto *lex_col = get_column_ptr<LexemePOD>(LinguisticsBufferSlot::LING_LEXEME_POD)) {
        lex_col[slot] = copy;
    }
    sync_selection_ranges();
    return slot;
}

void LinguisticsDOD::bake_from_semantic_lexicon_core(const SemanticLexiconCore &p_lexicon) {
    // 1. Sememes
    for (const SememeDescriptorPOD &sem : p_lexicon.sememes()) {
        allocate_sememe_descriptor(sem);
    }

    // 2. Lexemes & PanPhon/CMUdict phoneme CSR slices
    const auto &lexemes = p_lexicon.lexemes();
    for (size_t i = 0; i < lexemes.size(); ++i) {
        std::span<const uint16_t> ph_slice = p_lexicon.get_lexeme_phoneme_masks(static_cast<uint32_t>(i));
        allocate_lexeme(lexemes[i], ph_slice);
    }

    // 3. Morphemes
    if (auto *morph_col = get_column_ptr<MorphemePOD>(LinguisticsBufferSlot::LING_MORPHEME_POD)) {
        for (const MorphemePOD &m : p_lexicon.morphemes()) {
            const uint32_t slot = morpheme_allocator_.allocate();
            if (slot == INVALID_ID) break;
            morph_col[slot] = m;
        }
    }

    // 4. BPE Vocab & Merge Rules
    if (auto *vocab_col = get_column_ptr<BPEVocabEntryPOD>(LinguisticsBufferSlot::LING_BPE_VOCAB_POD)) {
        for (const BPEVocabEntryPOD &v : p_lexicon.bpe_vocab()) {
            const uint32_t slot = bpe_vocab_allocator_.allocate();
            if (slot == INVALID_ID) break;
            vocab_col[slot] = v;
        }
    }
    if (auto *merge_col = get_column_ptr<BPEMergeRulePOD>(LinguisticsBufferSlot::LING_BPE_MERGE_RULE_POD)) {
        for (const BPEMergeRulePOD &mr : p_lexicon.bpe_merges()) {
            const uint32_t slot = bpe_merge_allocator_.allocate();
            if (slot == INVALID_ID) break;
            merge_col[slot] = mr;
        }
    }

    // 5. WordNet Synset Relations
    if (auto *rel_col = get_column_ptr<SynsetRelationPOD>(LinguisticsBufferSlot::LING_SYNSET_RELATION_POD)) {
        for (const SynsetRelationPOD &rel : p_lexicon.synset_relations()) {
            const uint32_t slot = synset_relation_allocator_.allocate();
            if (slot == INVALID_ID) break;
            rel_col[slot] = rel;
        }
    }

    // 6. FrameNet Semantic Frames
    if (auto *frame_col = get_column_ptr<SemanticFramePOD>(LinguisticsBufferSlot::LING_SEMANTIC_FRAME_POD)) {
        for (const SemanticFramePOD &fr : p_lexicon.semantic_frames()) {
            const uint32_t slot = semantic_frame_allocator_.allocate();
            if (slot == INVALID_ID) break;
            frame_col[slot] = fr;
        }
    }

    sync_selection_ranges();
}

void LinguisticsDOD::sync_to_semantic_lexicon_core(SemanticLexiconCore &r_lexicon) const {
    const auto *sem_col = get_column_ptr<SememeDescriptorPOD>(LinguisticsBufferSlot::LING_SEMEME_DESCRIPTOR_POD);
    if (!sem_col) return;
    const auto &labels  = r_lexicon.sememe_labels();
    const uint32_t hwm  = sememe_allocator_.get_high_water_mark();
    for (uint32_t i = 0; i < hwm && i < labels.size(); ++i) {
        if (sememe_allocator_.is_valid(i)) {
            r_lexicon.register_sememe(labels[i], sem_col[i]);
        }
    }
}

// ============================================================================
// Table 4: Noisy Phonetic Utterances, BPE Token Stream & Frame Clauses
// ============================================================================

uint32_t LinguisticsDOD::stage_phonetic_utterance(
    std::span<const uint16_t> p_noisy_phoneme_masks,
    uint32_t p_expected_sememe_id,
    uint16_t p_preferred_lect_id
) {
    if (p_noisy_phoneme_masks.empty()) return INVALID_ID;
    auto *ph_stream = get_column_ptr<uint16_t>(LinguisticsBufferSlot::LING_PHONEME_FEATURE_STREAM);
    auto *spans     = get_column_ptr<PhoneticUtteranceSpanPOD>(LinguisticsBufferSlot::LING_PHONEME_UTTERANCE_SPAN_POD);
    if (!ph_stream || !spans) return INVALID_ID;

    if (phoneme_stream_cursor_ + p_noisy_phoneme_masks.size() > capacities_.max_phoneme_stream) {
        return INVALID_ID;
    }
    const uint32_t span_slot = utterance_span_allocator_.allocate();
    if (span_slot == INVALID_ID) return INVALID_ID;

    const uint32_t start_offset = phoneme_stream_cursor_;
    for (uint16_t m : p_noisy_phoneme_masks) {
        ph_stream[phoneme_stream_cursor_++] = m;
    }

    PhoneticUtteranceSpanPOD span{};
    span.phoneme_stream_offset = start_offset;
    span.phoneme_count         = static_cast<uint16_t>(p_noisy_phoneme_masks.size());
    span.preferred_lect_id     = p_preferred_lect_id;
    span.expected_sememe_id    = p_expected_sememe_id;
    spans[span_slot]           = span;

    decoded_sememe_stream_cursor_ = std::max(decoded_sememe_stream_cursor_, span_slot + 1u);
    sync_selection_ranges();
    return span_slot;
}

PhoneticUtteranceSpanPOD LinguisticsDOD::get_utterance_span(uint32_t p_span_slot) const noexcept {
    if (p_span_slot >= capacities_.max_utterance_spans) return PhoneticUtteranceSpanPOD{};
    const auto *spans = get_column_ptr<PhoneticUtteranceSpanPOD>(LinguisticsBufferSlot::LING_PHONEME_UTTERANCE_SPAN_POD);
    return spans ? spans[p_span_slot] : PhoneticUtteranceSpanPOD{};
}

uint32_t LinguisticsDOD::stage_bpe_token_stream(std::span<const uint32_t> p_token_ids) noexcept {
    auto *tok_col = get_column_ptr<uint32_t>(LinguisticsBufferSlot::LING_DECODED_SEMEME_STREAM);
    if (!tok_col) return 0u;
    const uint32_t count = static_cast<uint32_t>(
        std::min<size_t>(p_token_ids.size(), capacities_.max_decoded_sememe_stream)
    );
    for (uint32_t i = 0; i < count; ++i) {
        tok_col[i] = p_token_ids[i];
    }
    decoded_sememe_stream_cursor_ = count;
    sync_selection_ranges();
    return count;
}

std::vector<uint32_t> LinguisticsDOD::read_decoded_sememe_or_bpe_stream() const {
    std::vector<uint32_t> out;
    const auto *tok_col = get_column_ptr<uint32_t>(LinguisticsBufferSlot::LING_DECODED_SEMEME_STREAM);
    if (!tok_col) return out;
    for (uint32_t i = 0; i < decoded_sememe_stream_cursor_; ++i) {
        if (tok_col[i] == 0xFFFFFFFFu) break;
        out.push_back(tok_col[i]);
    }
    return out;
}

uint32_t LinguisticsDOD::stage_frame_clause_instance(
    uint32_t p_frame_id,
    uint32_t p_agent_sememe_id,
    uint32_t p_patient_sememe_id,
    uint32_t p_instrument_sememe_id,
    uint32_t p_location_sememe_id
) {
    const uint32_t slot = frame_clause_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;
    if (auto *clauses = get_column_ptr<FrameClauseInstancePOD>(LinguisticsBufferSlot::LING_FRAME_CLAUSE_INSTANCE_POD)) {
        FrameClauseInstancePOD cl{};
        cl.frame_id             = p_frame_id;
        cl.agent_sememe_id      = p_agent_sememe_id;
        cl.patient_sememe_id    = p_patient_sememe_id;
        cl.instrument_sememe_id = p_instrument_sememe_id;
        cl.location_sememe_id   = p_location_sememe_id;
        clauses[slot]           = cl;
    }
    sync_selection_ranges();
    return slot;
}

FrameClauseInstancePOD LinguisticsDOD::get_frame_clause_instance(uint32_t p_clause_slot) const noexcept {
    if (p_clause_slot >= capacities_.max_frame_clause_instances) return FrameClauseInstancePOD{};
    const auto *clauses = get_column_ptr<FrameClauseInstancePOD>(LinguisticsBufferSlot::LING_FRAME_CLAUSE_INSTANCE_POD);
    return clauses ? clauses[p_clause_slot] : FrameClauseInstancePOD{};
}

std::vector<uint32_t> LinguisticsDOD::read_slm_allowed_bpe_tokens() const {
    std::vector<uint32_t> out;
    const auto *mask_col = get_column_ptr<uint32_t>(LinguisticsBufferSlot::LING_SLM_VOCAB_LOGIT_MASK);
    if (!mask_col) return out;
    for (uint32_t i = 0; i < capacities_.max_slm_vocab_mask; ++i) {
        if (mask_col[i] == 0xFFFFFFFFu || mask_col[i] == 0u) {
            if (mask_col[i] == 0xFFFFFFFFu) break;
            continue;
        }
        out.push_back(mask_col[i]);
    }
    return out;
}

// ============================================================================
// Templated Task Node Builder (LNG1 - LNG7) & Pipeline Topology
// ============================================================================

template <typename T_Logic>
NodeID LinguisticsDOD::add_linguistics_task_node(
    TaskGraphDOD &r_graph,
    const T_Logic &p_logic_override,
    const LinguisticsNodeExecutionConfig &p_config
) const {
    using Descriptor = LinguisticsTaskDescriptor<T_Logic>;

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
            native_task = LinguisticsTransformTaskRegistry::create(
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        } else if constexpr (IsQueryLogic<T_Logic>) {
            native_task = LinguisticsQueryTaskRegistry::create(
                static_cast<uint32_t>(p_config.query_op),
                static_cast<uint32_t>(Descriptor::logic_id),
                static_cast<uint32_t>(p_config.view),
                static_cast<uint32_t>(p_config.strategy),
                static_cast<uint32_t>(p_config.element_type)
            );
        } else if constexpr (IsMetadataLogic<T_Logic>) {
            native_task = LinguisticsMetadataTaskRegistry::create(
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
        // Collect all unique buffer IDs referenced in the logic struct so get_grant_part() finds every bound column
        std::vector<GrantPartPOD> reqs;
        reqs.reserve(8);
        auto add_unique_req = [&](uint32_t p_buf_id, BufferAccessMode p_acc) {
            if (p_buf_id == INVALID_ID) return;
            for (auto &existing : reqs) {
                if (existing.buffer_id == p_buf_id) {
                    if (p_acc == BufferAccessMode::READ_WRITE) {
                        existing.access_mode = BufferAccessMode::READ_WRITE;
                    }
                    return;
                }
            }
            reqs.push_back(make_buffer_requirement(p_buf_id, p_acc));
        };

        const uint32_t target_id = logic.get_target_buffer_id();
        add_unique_req(target_id, Descriptor::default_grants[0].access);
        for (const auto &dg : Descriptor::default_grants) {
            add_unique_req(manifest_.get(dg.slot), dg.access);
        }
        // Also scan all bound buffer_ids inside `logic` that match any slot in `manifest_`
        const auto *logic_u32 = reinterpret_cast<const uint32_t *>(&logic);
        for (size_t k = 0; k < 10; ++k) {
            const uint32_t candidate = logic_u32[k];
            for (size_t s = 0; s < LINGUISTICS_BUFFER_SLOT_COUNT; ++s) {
                if (manifest_.buffer_ids[s] != INVALID_ID && candidate == manifest_.buffer_ids[s]) {
                    add_unique_req(candidate, BufferAccessMode::READ_WRITE);
                }
            }
        }
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

#define INSTANTIATE_LINGUISTICS_TASK_NODE(LogicType) \
    template NodeID LinguisticsDOD::add_linguistics_task_node<LogicType>(TaskGraphDOD &, const LogicType &, const LinguisticsNodeExecutionConfig &) const

INSTANTIATE_LINGUISTICS_TASK_NODE(LexiconPhonemeFeatureIndexMetadataLogic<>);
INSTANTIATE_LINGUISTICS_TASK_NODE(WordNetSynsetHypernymPropagateTransformLogic<>);
INSTANTIATE_LINGUISTICS_TASK_NODE(SemeComponentialAffinityQueryLogic<>);
INSTANTIATE_LINGUISTICS_TASK_NODE(MatryoshkaBinaryHammingQueryLogic<>);
INSTANTIATE_LINGUISTICS_TASK_NODE(PhoneticHammingLexemeDecodeTransformLogic<>);
INSTANTIATE_LINGUISTICS_TASK_NODE(BPESubwordAndMorphologyTransformLogic<>);
INSTANTIATE_LINGUISTICS_TASK_NODE(SemanticFrameAndSLMMaskTransformLogic<>);

#undef INSTANTIATE_LINGUISTICS_TASK_NODE

NodeID LinguisticsDOD::add_transform_node_by_id(
    TaskGraphDOD &r_graph,
    LinguisticsTransformLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const LinguisticsNodeExecutionConfig &p_config
) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_custom_props.is_empty()) l.apply_properties(p_custom_props);
        return add_linguistics_task_node(r_graph, l, p_config);
    };
    switch (p_logic_id) {
        case LinguisticsTransformLogicID::PhoneticHammingLexemeDecode:    return build.template operator()<PhoneticHammingLexemeDecodeTransformLogic<>>();
        case LinguisticsTransformLogicID::WordNetSynsetHypernymPropagate: return build.template operator()<WordNetSynsetHypernymPropagateTransformLogic<>>();
        case LinguisticsTransformLogicID::BPESubwordAndMorphology:        return build.template operator()<BPESubwordAndMorphologyTransformLogic<>>();
        case LinguisticsTransformLogicID::SemanticFrameAndSLMMask:        return build.template operator()<SemanticFrameAndSLMMaskTransformLogic<>>();
        default:                                                          return INVALID_ID;
    }
}

NodeID LinguisticsDOD::add_query_node_by_id(
    TaskGraphDOD &r_graph,
    LinguisticsQueryLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const LinguisticsNodeExecutionConfig &p_config
) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_custom_props.is_empty()) l.apply_properties(p_custom_props);
        return add_linguistics_task_node(r_graph, l, p_config);
    };
    switch (p_logic_id) {
        case LinguisticsQueryLogicID::SemeComponentialAffinity: return build.template operator()<SemeComponentialAffinityQueryLogic<>>();
        case LinguisticsQueryLogicID::MatryoshkaBinaryHamming:  return build.template operator()<MatryoshkaBinaryHammingQueryLogic<>>();
        default:                                                return INVALID_ID;
    }
}

NodeID LinguisticsDOD::add_metadata_node_by_id(
    TaskGraphDOD &r_graph,
    LinguisticsMetadataLogicID p_logic_id,
    const godot::Dictionary &p_custom_props,
    const LinguisticsNodeExecutionConfig &p_config
) const {
    auto build = [&]<typename T_Logic>() -> NodeID {
        T_Logic l{};
        if (!p_custom_props.is_empty()) l.apply_properties(p_custom_props);
        return add_linguistics_task_node(r_graph, l, p_config);
    };
    switch (p_logic_id) {
        case LinguisticsMetadataLogicID::LexiconPhonemeFeatureIndex: return build.template operator()<LexiconPhonemeFeatureIndexMetadataLogic<>>();
        default:                                                     return INVALID_ID;
    }
}

LinguisticsPipelineTopology LinguisticsDOD::inject_pipeline_stages(
    TaskGraphDOD &r_graph,
    uint32_t p_stage_mask,
    bool p_auto_connect_stages
) {
    LinguisticsPipelineTopology topo{};
    bool prev_was_parallel = false;

    auto append_stage = [&](LinguisticsPipelineStageFlags p_flag, const std::vector<NodeID> &p_nodes, bool p_chain_internal) {
        if (p_nodes.empty()) return;
        LinguisticsStageAnchor anchor{p_flag, p_nodes.front(), p_nodes.back(), p_nodes};

        if (p_chain_internal) {
            for (size_t i = 0; i + 1 < p_nodes.size(); ++i) {
                r_graph.connect_nodes(p_nodes[i], 0, p_nodes[i + 1], 0);
            }
        }
        if (p_auto_connect_stages && !topo.stages.empty()) {
            const LinguisticsStageAnchor &prev = topo.stages.back();
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

    // Wave 1: Lexicon SOA Extraction, Lect/UPOS Partitioning & WordNet Hypernym Propagation (LNG1 -> LNG2)
    if (p_stage_mask & LING_STAGE_WAVE_1_LEXICON_AND_ONTOLOGY_INDEX) {
        append_stage(
            LING_STAGE_WAVE_1_LEXICON_AND_ONTOLOGY_INDEX,
            {
                add_lexicon_phoneme_feature_index_node(r_graph),
                add_wordnet_synset_hypernym_propagate_node(r_graph)
            },
            true
        );
    }

    // Wave 2: Hybrid Componential Seme Affinity & 128-Bit Matryoshka Binary Hamming Search (LNG3 -> LNG4)
    if (p_stage_mask & LING_STAGE_WAVE_2_HYBRID_SEME_AND_MRL_QUERY) {
        LinguisticsNodeExecutionConfig q_cfg{};
        q_cfg.query_op = QueryOp::ADD;
        append_stage(
            LING_STAGE_WAVE_2_HYBRID_SEME_AND_MRL_QUERY,
            {
                add_seme_componential_affinity_node(r_graph, {}, q_cfg),
                add_matryoshka_binary_hamming_node(r_graph, {}, q_cfg)
            },
            true
        );
    }

    // Wave 3: PanPhon/CMUdict Needleman-Wunsch Phonetic Decode & BPE Subword Merges (LNG5 -> LNG6)
    if (p_stage_mask & LING_STAGE_WAVE_3_PHONETIC_DTW_AND_BPE_DECODE) {
        append_stage(
            LING_STAGE_WAVE_3_PHONETIC_DTW_AND_BPE_DECODE,
            {
                add_phonetic_hamming_lexeme_decode_node(r_graph),
                add_bpe_subword_and_morphology_node(r_graph)
            },
            true
        );
    }

    // Wave 4: FrameNet Thematic Role Fit & Constrained SLM Vocabulary Mask Compilation (LNG7)
    if (p_stage_mask & LING_STAGE_WAVE_4_FRAMENET_AND_SLM_CONSTRAINTS) {
        append_stage(
            LING_STAGE_WAVE_4_FRAMENET_AND_SLM_CONSTRAINTS,
            {
                add_semantic_frame_and_slm_mask_node(r_graph)
            },
            true
        );
    }

    active_topology_ = topo;
    return topo;
}

bool LinguisticsDOD::schedule_node_with_stage(
    TaskGraphDOD &r_graph,
    const LinguisticsPipelineTopology &p_topology,
    LinguisticsPipelineStageFlags p_target_stage,
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

bool LinguisticsDOD::schedule_node_between_stages(
    TaskGraphDOD &r_graph,
    const LinguisticsPipelineTopology &p_topology,
    LinguisticsPipelineStageFlags p_upstream_stage,
    LinguisticsPipelineStageFlags p_downstream_stage,
    NodeID p_custom_node
) {
    if (p_custom_node == INVALID_ID) return false;
    const LinguisticsStageAnchor *up   = p_topology.get_stage_anchor(p_upstream_stage);
    const LinguisticsStageAnchor *down = p_topology.get_stage_anchor(p_downstream_stage);
    if (!up || !down || !up->is_valid() || !down->is_valid()) return false;

    r_graph.connect_nodes(up->exit_node, 0, p_custom_node, 0);
    r_graph.connect_nodes(p_custom_node, 0, down->entry_node, 0);
    return true;
}

void LinguisticsDOD::execute_step(double p_delta, TaskGraphDOD *p_override_graph) {
    sync_selection_ranges();
    MemoryManagerDOD *mgr = memory_manager();
    if (TaskGraphDOD *target_graph = p_override_graph ? p_override_graph : task_graph()) {
        if (target_graph->get_node_count() > 0) {
            MemoryGrantPOD topo_grant{};
            if (target_graph->bake_topology_grant(topo_grant) && mgr) {
                mgr->release_grant(topo_grant);
            }
        }
        target_graph->execute_graph_dod(p_delta);
    }
}

} // namespace ideam::core
