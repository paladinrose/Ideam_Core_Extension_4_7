#pragma once

/**
 * linguistics_dod_common.h
 * Foundational pure C++26 DOD buffer slots, table-driven manifest descriptors,
 * capacity configurations, and composable TaskGraph pipeline stage flags for
 * `IdeamLinguistics <-> IdeamTasks` (`src/core/tasks/domains/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & INDUSTRY-STANDARD NLP DOD SUBSTRATE
 * ============================================================================
 * Implements 4 contiguous memory table groups across 19 shadowed DOD buffer slots:
 *   1. `SememeAndObjectProfileTable` (`SememeDescriptorPOD` / `LinguisticProfilePOD` 64B AoS +
 *      SOA columns: `nuclear_symbolic_word0` 64-bit exact ontology invariants, `mrl_binary_lo64`
 *      and `mrl_binary_hi64` 128-bit Binary-Quantized Matryoshka embeddings, `coherence_mask32`
 *      for WFC / Quantum Darwinism, `upos_and_domain`, and `sememe_affinity_score`).
 *   2. `LexiconMorphemeAndBPEVocabTable` (`LexemePOD` 32B AoS, flattened `uint16_t` PanPhon/CMUdict
 *      articulatory feature CSR masks, `MorphemePOD` 32B AoS, `BPEVocabEntryPOD` 16B AoS, and
 *      `BPEMergeRulePOD` 16B AoS for SentencePiece / Hugging Face subword tokenization).
 *   3. `WordNetAndFrameNetOntologyTable` (`SynsetRelationPOD` 16B AoS directed WordNet hypernym /
 *      holonym graph, `SemanticFramePOD` 32B AoS Berkeley FrameNet thematic role schemas, and
 *      `FrameClauseInstancePOD` 32B AoS active predicate-argument clauses).
 *   4. `PhoneticAndSLMStreamTable` (`uint16_t` noisy PanPhon feature stream, `PhoneticUtteranceSpanPOD`
 *      32B AoS Needleman-Wunsch DTW spans, `uint32_t` decoded sememe / BPE token stream,
 *      `uint32_t` constrained SLM vocabulary logit mask, and `SememeDescriptorPOD[2]` query context).
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
#include "../../../linguistics/semantic_lexicon_core.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

// Include the 3 Linguistics task registry headers
#include "registration/linguistics_metadata_task_registry.h"
#include "registration/linguistics_query_task_registry.h"
#include "registration/linguistics_transform_task_registry.h"

// Include the 7 IdeamLinguistics Task Logic Structs (LNG1 - LNG7)
#include "metadata_logic/lexicon_phoneme_feature_index_metadata_logic.h"
#include "query_logic/seme_componential_affinity_query_logic.h"
#include "query_logic/matryoshka_binary_hamming_query_logic.h"
#include "transform_logic/phonetic_hamming_lexeme_decode_transform_logic.h"
#include "transform_logic/wordnet_synset_hypernym_propagate_transform_logic.h"
#include "transform_logic/bpe_subword_and_morphology_transform_logic.h"
#include "transform_logic/semantic_frame_and_slm_mask_transform_logic.h"

namespace ideam::core {

/**
 * LinguisticsPipelineStageFlags
 * Composable bitmask flags representing the 4 NLP waves in `LinguisticsDOD`.
 */
enum LinguisticsPipelineStageFlags : uint32_t {
    LING_STAGE_NONE                                   = 0u,

    // Wave 1: LNG1 LexiconPhonemeFeatureIndexMetadataLogic + LNG2 WordNetSynsetHypernymPropagateTransformLogic
    LING_STAGE_WAVE_1_LEXICON_AND_ONTOLOGY_INDEX      = 1u << 0,

    // Wave 2: LNG3 SemeComponentialAffinityQueryLogic + LNG4 MatryoshkaBinaryHammingQueryLogic
    LING_STAGE_WAVE_2_HYBRID_SEME_AND_MRL_QUERY       = 1u << 1,

    // Wave 3: LNG5 PhoneticHammingLexemeDecodeTransformLogic + LNG6 BPESubwordAndMorphologyTransformLogic
    LING_STAGE_WAVE_3_PHONETIC_DTW_AND_BPE_DECODE     = 1u << 2,

    // Wave 4: LNG7 SemanticFrameAndSLMMaskTransformLogic
    LING_STAGE_WAVE_4_FRAMENET_AND_SLM_CONSTRAINTS    = 1u << 3,

    // Preset masks:
    LING_STAGE_DEFAULT_PIPELINE_MASK                  =
        LING_STAGE_WAVE_1_LEXICON_AND_ONTOLOGY_INDEX  |
        LING_STAGE_WAVE_2_HYBRID_SEME_AND_MRL_QUERY   |
        LING_STAGE_WAVE_3_PHONETIC_DTW_AND_BPE_DECODE |
        LING_STAGE_WAVE_4_FRAMENET_AND_SLM_CONSTRAINTS,

    LING_STAGE_ALL_MASK                               = 0x000Fu
};

/**
 * LinguisticsNodeExecutionConfig
 * Optional overrides when instantiating any Linguistics task node in a `TaskGraphDOD`.
 */
struct LinguisticsNodeExecutionConfig {
    MemoryView view            = MemoryView::SingleElementView;
    MemoryStrategy strategy    = MemoryStrategy::FlatStrategy;
    MemoryTypes element_type   = MemoryTypes::UINT32;
    QueryOp query_op           = QueryOp::CULL;
    int32_t execution_priority = 0;
    bool use_registry_factory  = false;
    std::span<const GrantPartPOD> custom_requirements{};
};

/**
 * LinguisticsStageAnchor
 * Topological handle returned for each injected linguistic wave stage in a `TaskGraphDOD`.
 */
struct LinguisticsStageAnchor {
    LinguisticsPipelineStageFlags stage = LING_STAGE_NONE;
    NodeID entry_node                   = INVALID_ID;
    NodeID exit_node                    = INVALID_ID;
    std::vector<NodeID> nodes;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return entry_node != INVALID_ID && exit_node != INVALID_ID;
    }
};

/**
 * LinguisticsPipelineTopology
 * Describes the set of `LinguisticsStageAnchor`s injected into a `TaskGraphDOD`.
 */
struct LinguisticsPipelineTopology {
    NodeID first_node = INVALID_ID;
    NodeID last_node  = INVALID_ID;
    std::vector<LinguisticsStageAnchor> stages;

    [[nodiscard]] const LinguisticsStageAnchor *get_stage_anchor(LinguisticsPipelineStageFlags p_stage) const noexcept {
        for (const auto &anchor : stages) {
            if (anchor.stage == p_stage) {
                return &anchor;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool has_stage(LinguisticsPipelineStageFlags p_stage) const noexcept {
        const LinguisticsStageAnchor *anchor = get_stage_anchor(p_stage);
        return anchor != nullptr && anchor->is_valid();
    }
};

/**
 * LinguisticsCapacityKind
 */
enum class LinguisticsCapacityKind : uint8_t {
    SEMEMES = 0,
    LEXEMES,
    PHONEME_CSR_MASKS,
    MORPHEMES,
    BPE_VOCAB,
    BPE_MERGE_RULES,
    SYNSET_RELATIONS,
    SEMANTIC_FRAMES,
    FRAME_CLAUSE_INSTANCES,
    PHONEME_STREAM,
    UTTERANCE_SPANS,
    DECODED_SEMEME_STREAM,
    SLM_VOCAB_MASK,
    QUERY_CONTEXT_PAIR
};

/**
 * LinguisticsTableGroup
 */
enum class LinguisticsTableGroup : uint8_t {
    SEMEME_AND_PROFILE_TABLE = 0,
    LEXEME_TABLE,
    LEXEME_PHONEME_CSR_SLICE,
    MORPHEME_TABLE,
    BPE_VOCAB_TABLE,
    BPE_MERGE_RULE_TABLE,
    SYNSET_RELATION_TABLE,
    SEMANTIC_FRAME_TABLE,
    FRAME_CLAUSE_INSTANCE_TABLE,
    PHONEME_FEATURE_STREAM_SLICE,
    UTTERANCE_SPAN_TABLE,
    DECODED_SEMEME_STREAM_SLICE,
    SLM_VOCAB_MASK_SLICE,
    QUERY_CONTEXT_SINGLETON
};

/**
 * LinguisticsBufferCapacities
 */
struct LinguisticsBufferCapacities {
    uint32_t max_sememes                = 4096;
    uint32_t max_lexemes                = 8192;
    uint32_t max_phoneme_csr_masks      = 32768;
    uint32_t max_morphemes              = 1024;
    uint32_t max_bpe_vocab              = 16384;
    uint32_t max_bpe_merge_rules        = 16384;
    uint32_t max_synset_relations       = 8192;
    uint32_t max_semantic_frames        = 512;
    uint32_t max_frame_clause_instances = 512;
    uint32_t max_phoneme_stream         = 8192;
    uint32_t max_utterance_spans        = 512;
    uint32_t max_decoded_sememe_stream  = 4096;
    uint32_t max_slm_vocab_mask         = 8192;

    [[nodiscard]] constexpr uint32_t get_capacity(LinguisticsCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case LinguisticsCapacityKind::SEMEMES:                return max_sememes;
            case LinguisticsCapacityKind::LEXEMES:                return max_lexemes;
            case LinguisticsCapacityKind::PHONEME_CSR_MASKS:      return max_phoneme_csr_masks;
            case LinguisticsCapacityKind::MORPHEMES:              return max_morphemes;
            case LinguisticsCapacityKind::BPE_VOCAB:              return max_bpe_vocab;
            case LinguisticsCapacityKind::BPE_MERGE_RULES:        return max_bpe_merge_rules;
            case LinguisticsCapacityKind::SYNSET_RELATIONS:       return max_synset_relations;
            case LinguisticsCapacityKind::SEMANTIC_FRAMES:        return max_semantic_frames;
            case LinguisticsCapacityKind::FRAME_CLAUSE_INSTANCES: return max_frame_clause_instances;
            case LinguisticsCapacityKind::PHONEME_STREAM:         return max_phoneme_stream;
            case LinguisticsCapacityKind::UTTERANCE_SPANS:        return max_utterance_spans;
            case LinguisticsCapacityKind::DECODED_SEMEME_STREAM:  return max_decoded_sememe_stream;
            case LinguisticsCapacityKind::SLM_VOCAB_MASK:         return max_slm_vocab_mask;
            case LinguisticsCapacityKind::QUERY_CONTEXT_PAIR:     return 2u;
            default:                                              return max_sememes;
        }
    }
};

/**
 * LinguisticsBufferSlot
 * Enumerates all 19 DOD buffers managed by `LinguisticsDOD`.
 * Preserves legacy aliases (`PRIMARY_DESCRIPTOR_POD`, `SECONDARY_SLICE_CSR`,
 * `FEATURE_BITMASK_STREAM`, `DECODED_SEMEME_STREAM`) for full backwards compatibility.
 */
enum class LinguisticsBufferSlot : uint8_t {
    // --- Table 1: Sememe & Object Profile Table (Hybrid Symbolic + 128-bit MRL Binary) ---
    LING_SEMEME_DESCRIPTOR_POD = 0,
    LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0,
    LING_SEMEME_MRL_BINARY_LO64,
    LING_SEMEME_MRL_BINARY_HI64,
    LING_SEMEME_COHERENCE_MASK32,
    LING_SEMEME_UPOS_AND_DOMAIN,
    LING_SEMEME_AFFINITY_SCORE,

    // --- Table 2: Lexicon, Morpheme, PanPhon CSR & BPE Subword Table ---
    LING_LEXEME_POD,
    LING_LEXEME_PHONEME_CSR_MASKS,
    LING_MORPHEME_POD,
    LING_BPE_VOCAB_POD,
    LING_BPE_MERGE_RULE_POD,

    // --- Table 3: WordNet Synsets & FrameNet Semantic Frames ---
    LING_SYNSET_RELATION_POD,
    LING_SEMANTIC_FRAME_POD,
    LING_FRAME_CLAUSE_INSTANCE_POD,

    // --- Table 4: Phonetic Streams, Decoded Tokens, SLM Mask & Query Context ---
    LING_PHONEME_FEATURE_STREAM,
    LING_PHONEME_UTTERANCE_SPAN_POD,
    LING_DECODED_SEMEME_STREAM,
    LING_SLM_VOCAB_LOGIT_MASK,
    LING_QUERY_CONTEXT_POD,

    COUNT,

    // Legacy slot aliases
    PRIMARY_DESCRIPTOR_POD = LING_SEMEME_DESCRIPTOR_POD,
    SECONDARY_SLICE_CSR    = LING_LEXEME_PHONEME_CSR_MASKS,
    FEATURE_BITMASK_STREAM = LING_PHONEME_FEATURE_STREAM,
    DECODED_SEMEME_STREAM  = LING_DECODED_SEMEME_STREAM
};

constexpr size_t LINGUISTICS_BUFFER_SLOT_COUNT = static_cast<size_t>(LinguisticsBufferSlot::COUNT);

struct LinguisticsBufferSpec {
    LinguisticsBufferSlot slot              = LinguisticsBufferSlot::COUNT;
    std::string_view canonical_name         = "";
    std::array<std::string_view, 3> aliases = {"", "", ""};
    uint32_t element_stride                 = 4;
    BufferLayoutType layout                 = BufferLayoutType::FLAT;
    SelectionMode selection_mode            = SelectionMode::RANGE;
    LinguisticsCapacityKind capacity_kind   = LinguisticsCapacityKind::SEMEMES;
    LinguisticsTableGroup table_group       = LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE;

    [[nodiscard]] constexpr bool matches_name(std::string_view p_query) const noexcept {
        if (p_query.empty()) return false;
        if (canonical_name == p_query) return true;
        for (std::string_view alias : aliases) {
            if (!alias.empty() && alias == p_query) return true;
        }
        return false;
    }
};

constexpr std::array<LinguisticsBufferSpec, LINGUISTICS_BUFFER_SLOT_COUNT> LINGUISTICS_BUFFER_SPECS = {{
    // Table 1: Sememe & Object Profile Table
    {
        LinguisticsBufferSlot::LING_SEMEME_DESCRIPTOR_POD,
        "sememe_descriptor_pod_buffer_id",
        {"primary_descriptor_pod_buffer_id", "linguistic_profile_pod_buffer_id", ""},
        sizeof(SememeDescriptorPOD), BufferLayoutType::AOS, SelectionMode::SPARSE_SET,
        LinguisticsCapacityKind::SEMEMES, LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE
    },
    {
        LinguisticsBufferSlot::LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0,
        "sememe_nuclear_symbolic_word0_buffer_id",
        {"nuclear_symbolic_word0_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        LinguisticsCapacityKind::SEMEMES, LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE
    },
    {
        LinguisticsBufferSlot::LING_SEMEME_MRL_BINARY_LO64,
        "sememe_mrl_binary_lo64_buffer_id",
        {"mrl_binary_lo64_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        LinguisticsCapacityKind::SEMEMES, LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE
    },
    {
        LinguisticsBufferSlot::LING_SEMEME_MRL_BINARY_HI64,
        "sememe_mrl_binary_hi64_buffer_id",
        {"mrl_binary_hi64_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        LinguisticsCapacityKind::SEMEMES, LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE
    },
    {
        LinguisticsBufferSlot::LING_SEMEME_COHERENCE_MASK32,
        "sememe_coherence_mask32_buffer_id",
        {"coherence_trait_mask32_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        LinguisticsCapacityKind::SEMEMES, LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE
    },
    {
        LinguisticsBufferSlot::LING_SEMEME_UPOS_AND_DOMAIN,
        "sememe_upos_and_domain_buffer_id",
        {"upos_and_domain_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        LinguisticsCapacityKind::SEMEMES, LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE
    },
    {
        LinguisticsBufferSlot::LING_SEMEME_AFFINITY_SCORE,
        "sememe_affinity_score_buffer_id",
        {"affinity_score_buffer_id", "", ""},
        sizeof(float), BufferLayoutType::FLAT, SelectionMode::SPARSE_SET,
        LinguisticsCapacityKind::SEMEMES, LinguisticsTableGroup::SEMEME_AND_PROFILE_TABLE
    },

    // Table 2: Lexicon, Morpheme, PanPhon CSR & BPE Subword Table
    {
        LinguisticsBufferSlot::LING_LEXEME_POD,
        "lexeme_pod_buffer_id",
        {"lexicon_buffer_id", "", ""},
        sizeof(LexemePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::LEXEMES, LinguisticsTableGroup::LEXEME_TABLE
    },
    {
        LinguisticsBufferSlot::LING_LEXEME_PHONEME_CSR_MASKS,
        "lexeme_phoneme_csr_masks_buffer_id",
        {"secondary_slice_csr_buffer_id", "phoneme_csr_masks_buffer_id", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        LinguisticsCapacityKind::PHONEME_CSR_MASKS, LinguisticsTableGroup::LEXEME_PHONEME_CSR_SLICE
    },
    {
        LinguisticsBufferSlot::LING_MORPHEME_POD,
        "morpheme_pod_buffer_id",
        {"morpheme_buffer_id", "", ""},
        sizeof(MorphemePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::MORPHEMES, LinguisticsTableGroup::MORPHEME_TABLE
    },
    {
        LinguisticsBufferSlot::LING_BPE_VOCAB_POD,
        "bpe_vocab_pod_buffer_id",
        {"bpe_vocab_buffer_id", "", ""},
        sizeof(BPEVocabEntryPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::BPE_VOCAB, LinguisticsTableGroup::BPE_VOCAB_TABLE
    },
    {
        LinguisticsBufferSlot::LING_BPE_MERGE_RULE_POD,
        "bpe_merge_rule_pod_buffer_id",
        {"bpe_merge_buffer_id", "", ""},
        sizeof(BPEMergeRulePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::BPE_MERGE_RULES, LinguisticsTableGroup::BPE_MERGE_RULE_TABLE
    },

    // Table 3: WordNet Synsets & FrameNet Semantic Frames
    {
        LinguisticsBufferSlot::LING_SYNSET_RELATION_POD,
        "synset_relation_pod_buffer_id",
        {"wordnet_synset_relation_buffer_id", "", ""},
        sizeof(SynsetRelationPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::SYNSET_RELATIONS, LinguisticsTableGroup::SYNSET_RELATION_TABLE
    },
    {
        LinguisticsBufferSlot::LING_SEMANTIC_FRAME_POD,
        "semantic_frame_pod_buffer_id",
        {"framenet_frame_pod_buffer_id", "", ""},
        sizeof(SemanticFramePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::SEMANTIC_FRAMES, LinguisticsTableGroup::SEMANTIC_FRAME_TABLE
    },
    {
        LinguisticsBufferSlot::LING_FRAME_CLAUSE_INSTANCE_POD,
        "frame_instance_pod_buffer_id",
        {"frame_clause_instance_pod_buffer_id", "", ""},
        sizeof(FrameClauseInstancePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::FRAME_CLAUSE_INSTANCES, LinguisticsTableGroup::FRAME_CLAUSE_INSTANCE_TABLE
    },

    // Table 4: Phonetic Streams, Decoded Tokens, SLM Mask & Query Context
    {
        LinguisticsBufferSlot::LING_PHONEME_FEATURE_STREAM,
        "phoneme_feature_stream_buffer_id",
        {"phoneme_buffer_id", "feature_bitmask_stream_buffer_id", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        LinguisticsCapacityKind::PHONEME_STREAM, LinguisticsTableGroup::PHONEME_FEATURE_STREAM_SLICE
    },
    {
        LinguisticsBufferSlot::LING_PHONEME_UTTERANCE_SPAN_POD,
        "phoneme_utterance_span_pod_buffer_id",
        {"utterance_span_pod_buffer_id", "", ""},
        sizeof(PhoneticUtteranceSpanPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::UTTERANCE_SPANS, LinguisticsTableGroup::UTTERANCE_SPAN_TABLE
    },
    {
        LinguisticsBufferSlot::LING_DECODED_SEMEME_STREAM,
        "decoded_sememe_stream_buffer_id",
        {"bpe_token_stream_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        LinguisticsCapacityKind::DECODED_SEMEME_STREAM, LinguisticsTableGroup::DECODED_SEMEME_STREAM_SLICE
    },
    {
        LinguisticsBufferSlot::LING_SLM_VOCAB_LOGIT_MASK,
        "slm_vocab_logit_mask_buffer_id",
        {"slm_allowed_bpe_tokens_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        LinguisticsCapacityKind::SLM_VOCAB_MASK, LinguisticsTableGroup::SLM_VOCAB_MASK_SLICE
    },
    {
        LinguisticsBufferSlot::LING_QUERY_CONTEXT_POD,
        "query_context_pod_buffer_id",
        {"query_mask_buffer_id", "", ""},
        sizeof(SememeDescriptorPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        LinguisticsCapacityKind::QUERY_CONTEXT_PAIR, LinguisticsTableGroup::QUERY_CONTEXT_SINGLETON
    }
}};

[[nodiscard]] consteval bool validate_linguistics_buffer_specs() noexcept {
    for (size_t i = 0; i < LINGUISTICS_BUFFER_SLOT_COUNT; ++i) {
        if (static_cast<size_t>(LINGUISTICS_BUFFER_SPECS[i].slot) != i) {
            return false;
        }
        if (LINGUISTICS_BUFFER_SPECS[i].canonical_name.empty() || LINGUISTICS_BUFFER_SPECS[i].element_stride == 0u) {
            return false;
        }
    }
    return true;
}

static_assert(validate_linguistics_buffer_specs(), "LINGUISTICS_BUFFER_SPECS indices must match LinguisticsBufferSlot enum order!");

/**
 * LinguisticsBufferManifest
 * Table-driven directory of all `MemoryManagerDOD` buffer IDs backing `LinguisticsDOD`.
 */
struct LinguisticsBufferManifest {
    std::array<uint32_t, LINGUISTICS_BUFFER_SLOT_COUNT> buffer_ids = []() consteval {
        std::array<uint32_t, LINGUISTICS_BUFFER_SLOT_COUNT> arr{};
        arr.fill(INVALID_ID);
        return arr;
    }();

    [[nodiscard]] constexpr uint32_t &operator[](LinguisticsBufferSlot p_slot) noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr const uint32_t &operator[](LinguisticsBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr uint32_t get(LinguisticsBufferSlot p_slot) const noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        return (idx < LINGUISTICS_BUFFER_SLOT_COUNT) ? buffer_ids[idx] : INVALID_ID;
    }

    constexpr void set(LinguisticsBufferSlot p_slot, uint32_t p_buffer_id) noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        if (idx < LINGUISTICS_BUFFER_SLOT_COUNT) {
            buffer_ids[idx] = p_buffer_id;
        }
    }

    constexpr void reset() noexcept {
        buffer_ids.fill(INVALID_ID);
    }

    [[nodiscard]] constexpr uint32_t get_by_name(std::string_view p_name) const noexcept {
        for (size_t i = 0; i < LINGUISTICS_BUFFER_SLOT_COUNT; ++i) {
            if (LINGUISTICS_BUFFER_SPECS[i].matches_name(p_name)) {
                return buffer_ids[i];
            }
        }
        return INVALID_ID;
    }

    [[nodiscard]] constexpr bool is_initialized() const noexcept {
        return get(LinguisticsBufferSlot::LING_SEMEME_DESCRIPTOR_POD) != INVALID_ID &&
               get(LinguisticsBufferSlot::LING_LEXEME_POD) != INVALID_ID &&
               get(LinguisticsBufferSlot::LING_PHONEME_FEATURE_STREAM) != INVALID_ID;
    }
};

} // namespace ideam::core
