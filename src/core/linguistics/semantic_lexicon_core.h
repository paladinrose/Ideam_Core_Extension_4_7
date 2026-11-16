#pragma once

/**
 * semantic_lexicon_core.h
 * Pure C++26 Industry-Standard NLP & DOD Engine (`src/core/linguistics/`):
 *   - Hybrid Symbolic + Binary-Quantized Matryoshka (MRL) Sememe Matcher
 *   - PanPhon 16-bit Articulatory Feature Table + CMUdict ARPAbet & Context-Sensitive G2P
 *   - Needleman-Wunsch / Levenshtein-Hamming Phonetic Sequence Alignment Decoder
 *   - SentencePiece / Hugging Face BPE Subword Tokenizer
 *   - WordNet Taxonomic Synset Graph & FrameNet `SemanticFramePOD` Role Validator
 *   - GBNF / XGrammar Constrained Decoding Compiler for Local SLMs/LLMs
 */

#include "linguistics_common.h"
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

struct MorphologicalDecompositionResultPOD {
    uint32_t matched_lexeme_id        = 0xFFFFFFFFu;
    uint32_t matched_sememe_id        = 0xFFFFFFFFu;
    uint32_t prefix_morpheme_id       = 0xFFFFFFFFu;
    uint32_t root_morpheme_id         = 0xFFFFFFFFu;
    uint32_t suffix_morpheme_id       = 0xFFFFFFFFu;
    uint32_t stem_hash                = 0u;
    UniversalPOS inferred_category    = UniversalPOS::UNDEFINED;
    bool     is_negated               = false;
    SememeDescriptorPOD composed_descriptor{};
};

class SemanticLexiconCore {
private:
    std::vector<SemeDescriptorPOD> semes_;
    std::vector<SememeDescriptorPOD> sememes_;
    std::vector<PhonemePOD> phonemes_;
    std::vector<MorphemePOD> morphemes_;
    std::vector<LexemePOD> lexemes_;
    std::vector<LectDescriptorPOD> lects_;
    std::vector<SemanticFramePOD> semantic_frames_;
    std::vector<SynsetRelationPOD> synset_relations_;
    std::vector<BPEVocabEntryPOD> bpe_vocab_;
    std::vector<BPEMergeRulePOD> bpe_merges_;

    // Flat CSR tables for zero-pointer-chasing DOD access
    std::vector<uint16_t> lexeme_phoneme_feature_csr_;
    std::vector<uint32_t> lexeme_morpheme_id_csr_;
    std::vector<uint16_t> morpheme_phoneme_feature_csr_;

    // Parallel surface-form string tables for Godot-side inspection & external NLP import/export
    std::vector<std::string> seme_names_;
    std::vector<std::string> sememe_labels_;
    std::vector<std::string> morpheme_forms_;
    std::vector<std::string> lexeme_surfaces_;
    std::vector<std::string> frame_names_;
    std::vector<std::string> bpe_pieces_;

public:
    SemanticLexiconCore() = default;
    ~SemanticLexiconCore() = default;

    void clear() noexcept;

    // --- Read-Only Table Accessors ---
    [[nodiscard]] const std::vector<SemeDescriptorPOD> &semes() const noexcept { return semes_; }
    [[nodiscard]] const std::vector<SememeDescriptorPOD> &sememes() const noexcept { return sememes_; }
    [[nodiscard]] const std::vector<PhonemePOD> &phonemes() const noexcept { return phonemes_; }
    [[nodiscard]] const std::vector<MorphemePOD> &morphemes() const noexcept { return morphemes_; }
    [[nodiscard]] const std::vector<LexemePOD> &lexemes() const noexcept { return lexemes_; }
    [[nodiscard]] const std::vector<LectDescriptorPOD> &lects() const noexcept { return lects_; }
    [[nodiscard]] const std::vector<SemanticFramePOD> &semantic_frames() const noexcept { return semantic_frames_; }
    [[nodiscard]] const std::vector<SynsetRelationPOD> &synset_relations() const noexcept { return synset_relations_; }
    [[nodiscard]] const std::vector<BPEVocabEntryPOD> &bpe_vocab() const noexcept { return bpe_vocab_; }
    [[nodiscard]] const std::vector<BPEMergeRulePOD> &bpe_merges() const noexcept { return bpe_merges_; }
    [[nodiscard]] const std::vector<uint16_t> &lexeme_phoneme_feature_csr() const noexcept {
        return lexeme_phoneme_feature_csr_;
    }
    [[nodiscard]] const std::vector<std::string> &seme_names() const noexcept { return seme_names_; }
    [[nodiscard]] const std::vector<std::string> &sememe_labels() const noexcept { return sememe_labels_; }
    [[nodiscard]] const std::vector<std::string> &morpheme_forms() const noexcept { return morpheme_forms_; }
    [[nodiscard]] const std::vector<std::string> &lexeme_surfaces() const noexcept { return lexeme_surfaces_; }
    [[nodiscard]] const std::vector<std::string> &frame_names() const noexcept { return frame_names_; }
    [[nodiscard]] const std::vector<std::string> &bpe_pieces() const noexcept { return bpe_pieces_; }

    // --- Default Inventory Bootstrap ---
    void populate_default_english_ipa_phonemes();
    void populate_default_core_semes_and_morphemes();
    void merge_from(const SemanticLexiconCore &p_other);

    // --- Registration & Lookup API ---
    uint32_t register_seme(
        std::string_view p_tag_name,
        uint32_t p_preferred_bit = 0xFFFFFFFFu,
        uint16_t p_domain_id = static_cast<uint16_t>(SemanticDomain::GENERAL),
        uint16_t p_dimension_id = 0,
        uint16_t p_antonym_seme_id = 0xFFFFu,
        bool p_is_nuclear_default = true
    );
    [[nodiscard]] uint32_t resolve_seme_bit(std::string_view p_tag_name) const noexcept;
    [[nodiscard]] static uint32_t resolve_canonical_seme_bit(std::string_view p_tag_name) noexcept;

    uint32_t register_phoneme(
        uint32_t p_ipa_codepoint,
        uint16_t p_articulatory_mask,
        uint16_t p_duration_ms = 80,
        uint8_t p_sonority_rank = 1,
        uint8_t p_stress_level = 0,
        std::string_view p_arpabet_code = {}
    );
    [[nodiscard]] uint16_t find_phoneme_mask_by_codepoint(uint32_t p_ipa_codepoint) const noexcept;
    [[nodiscard]] static uint16_t arpabet_to_articulatory_mask(
        std::string_view p_arpabet,
        uint8_t *r_out_stress = nullptr
    ) noexcept;

    uint32_t register_sememe(
        std::string_view p_canonical_label,
        const SememeDescriptorPOD &p_descriptor
    );
    [[nodiscard]] int32_t find_sememe_by_label(std::string_view p_label) const noexcept;
    [[nodiscard]] std::string_view get_sememe_label(uint32_t p_sememe_id) const noexcept;

    uint32_t register_morpheme(
        std::string_view p_form,
        MorphemeKind p_kind,
        uint16_t p_primary_seme_id,
        uint64_t p_seme_modifier_mask = 0ULL,
        bool p_negates_root = false,
        UniversalPOS p_category_shift = UniversalPOS::UNDEFINED,
        std::span<const uint16_t> p_phoneme_masks = {}
    );

    uint32_t register_lexeme(
        std::string_view p_surface_form,
        uint32_t p_sememe_id,
        UniversalPOS p_category = UniversalPOS::NOUN,
        uint16_t p_lect_id = 0,
        uint8_t p_register_formality = 128,
        float p_corpus_frequency = 1.0f,
        uint32_t p_slm_bpe_token_id = 0xFFFFFFFFu,
        std::span<const uint16_t> p_explicit_phoneme_masks = {},
        std::span<const uint32_t> p_morpheme_ids = {}
    );
    [[nodiscard]] int32_t find_lexeme_by_surface(
        std::string_view p_surface_form,
        uint16_t p_preferred_lect_id = 0
    ) const noexcept;
    [[nodiscard]] std::string_view get_lexeme_surface(uint32_t p_lexeme_id) const noexcept;
    [[nodiscard]] std::span<const uint16_t> get_lexeme_phoneme_masks(uint32_t p_lexeme_id) const noexcept;

    uint32_t register_lect(const LectDescriptorPOD &p_lect);

    // --- WordNet Synset Relations & FrameNet Semantic Frames ---
    uint32_t register_synset_relation(
        uint32_t p_source_sememe_id,
        uint32_t p_target_sememe_id,
        WordNetRelationType p_relation = WordNetRelationType::WN_HYPERNYM,
        float p_weight = 1.0f
    );
    [[nodiscard]] bool is_hypernym_of(
        uint32_t p_candidate_hypernym_id,
        uint32_t p_hyponym_sememe_id,
        uint32_t p_max_hops = 6
    ) const noexcept;

    uint32_t register_semantic_frame(
        std::string_view p_frame_name,
        const SemanticFramePOD &p_frame
    );
    [[nodiscard]] int32_t find_semantic_frame_by_name(std::string_view p_frame_name) const noexcept;
    [[nodiscard]] float evaluate_semantic_frame_role_fit(
        uint32_t p_frame_id,
        const SememeDescriptorPOD &p_agent_desc,
        const SememeDescriptorPOD &p_patient_desc,
        const SememeDescriptorPOD &p_instrument_desc = SememeDescriptorPOD{}
    ) const noexcept;

    // --- Matryoshka (MRL) Float-to-Binary Quantizer & LSH SimHash ---
    static void quantize_mrl_float_embedding(
        std::span<const float> p_mrl_floats,
        SememeDescriptorPOD &r_target_descriptor,
        bool p_mean_center = true
    ) noexcept;

    [[nodiscard]] static std::array<uint64_t, 2> compute_character_ngram_simhash128(
        std::string_view p_text
    ) noexcept;
    static void compute_character_ngram_simhash128(
        std::string_view p_text,
        uint64_t &r_word_lo,
        uint64_t &r_word_hi
    ) noexcept {
        const auto h = compute_character_ngram_simhash128(p_text);
        r_word_lo = h[0];
        r_word_hi = h[1];
    }

    // --- CMUdict ARPAbet Importer & Context-Sensitive G2P ---
    bool import_cmudict_entry(
        std::string_view p_word,
        std::string_view p_arpabet_pronunciation,
        UniversalPOS p_default_pos = UniversalPOS::NOUN
    );
    bool import_cmudict_entry(
        std::string_view p_word,
        std::string_view p_arpabet_pronunciation,
        uint16_t p_lect_id,
        UniversalPOS p_default_pos = UniversalPOS::NOUN
    ) {
        (void)p_lect_id;
        return import_cmudict_entry(p_word, p_arpabet_pronunciation, p_default_pos);
    }
    uint32_t import_cmudict_lines(std::string_view p_multiline_cmudict_text);

    void approximate_phonemes_from_text(
        std::string_view p_text,
        std::vector<uint16_t> &r_out_masks
    ) const;

    // --- SentencePiece / Hugging Face BPE Subword Tokenizer ---
    uint32_t register_bpe_vocab_entry(
        std::string_view p_piece,
        uint32_t p_token_id,
        float p_unigram_score = 0.0f,
        uint32_t p_mapped_sememe_id = 0xFFFFFFFFu
    );
    uint32_t register_bpe_vocab_entry(
        std::string_view p_piece,
        uint32_t p_token_id,
        float p_unigram_score,
        uint32_t p_mapped_sememe_id,
        bool p_is_special
    ) {
        (void)p_is_special;
        return register_bpe_vocab_entry(p_piece, p_token_id, p_unigram_score, p_mapped_sememe_id);
    }
    uint32_t register_bpe_merge_rule(
        uint32_t p_left_token_id,
        uint32_t p_right_token_id,
        uint32_t p_merged_token_id,
        uint32_t p_priority_rank
    );
    [[nodiscard]] int32_t find_bpe_token_by_piece(std::string_view p_piece) const noexcept;
    [[nodiscard]] int32_t find_bpe_token_id(std::string_view p_piece) const noexcept {
        return find_bpe_token_by_piece(p_piece);
    }
    [[nodiscard]] std::string_view get_bpe_token_piece(uint32_t p_token_id) const noexcept {
        return (p_token_id < bpe_pieces_.size()) ? std::string_view(bpe_pieces_[p_token_id]) : std::string_view{};
    }
    void tokenize_bpe_subwords(
        std::string_view p_text,
        std::vector<uint32_t> &r_out_bpe_ids,
        std::vector<std::string> *r_out_pieces = nullptr
    ) const;
    [[nodiscard]] std::string decode_bpe_tokens(std::span<const uint32_t> p_bpe_ids) const;

    [[nodiscard]] MorphologicalDecompositionResultPOD decompose_word_morphemes(
        std::string_view p_word
    ) const;

    // --- Componential & Binary-Quantized MRL Bitvector Algebra ---
    [[nodiscard]] static float compute_bitarray_jaccard(
        const std::array<uint64_t, 3> &p_a,
        const std::array<uint64_t, 3> &p_b
    ) noexcept;

    [[nodiscard]] static float compute_bitarray_tversky(
        const std::array<uint64_t, 3> &p_prototype,
        const std::array<uint64_t, 3> &p_variant,
        float p_alpha = 0.7f,
        float p_beta = 0.3f
    ) noexcept;

    [[nodiscard]] static float compute_bitarray_cosine(
        const std::array<uint64_t, 3> &p_a,
        const std::array<uint64_t, 3> &p_b
    ) noexcept;

    [[nodiscard]] static float compute_descriptor_affinity(
        const SememeDescriptorPOD &p_a,
        const SememeDescriptorPOD &p_b,
        float p_nuclear_weight = 0.75f,
        float p_peripheral_weight = 0.25f
    ) noexcept;

    [[nodiscard]] static float compute_descriptor_tversky_affinity(
        const SememeDescriptorPOD &p_query_or_proto,
        const SememeDescriptorPOD &p_candidate,
        float p_alpha = 0.75f,
        float p_beta = 0.25f,
        float p_nuclear_weight = 0.75f,
        float p_peripheral_weight = 0.25f
    ) noexcept;

    [[nodiscard]] float compute_sememe_affinity(
        uint32_t p_sememe_a,
        uint32_t p_sememe_b
    ) const noexcept;

    // --- Needleman-Wunsch / Levenshtein-Hamming Phonetic Sequence Alignment Decoder ---
    [[nodiscard]] static float compute_phoneme_slice_distance(
        std::span<const uint16_t> p_slice_a,
        std::span<const uint16_t> p_slice_b
    ) noexcept;

    [[nodiscard]] uint32_t find_best_matching_lexeme_from_noisy_phonemes(
        std::span<const uint16_t> p_noisy_phoneme_masks,
        uint32_t p_expected_sememe_id = 0xFFFFFFFFu,
        uint16_t p_preferred_lect_id = 0
    ) const noexcept;

    // --- Neuro-Symbolic SLM Vocabulary Masking & GBNF Export ---
    void build_slm_vocabulary_mask(
        const SememeDescriptorPOD &p_context_descriptor,
        uint16_t p_lect_id,
        uint16_t p_allowed_category_mask,
        float p_min_affinity,
        std::vector<uint32_t> &r_allowed_bpe_tokens,
        std::vector<uint32_t> &r_allowed_lexeme_ids
    ) const;

    [[nodiscard]] std::string export_gbnf_grammar_for_constraints(
        const SememeDescriptorPOD &p_context_descriptor,
        uint16_t p_lect_id = 0,
        uint16_t p_allowed_category_mask = 0xFFFFu,
        float p_min_affinity = 0.15f
    ) const;
};

} // namespace ideam::core
