#pragma once

/**
 * semantic_lexicon_core.h
 * Pure C++26 Lexicon, Componential Sememe Matcher, Phonetic Hamming Decoder, and
 * Neuro-Symbolic SLM Vocabulary Mask Generator (`src/core/linguistics/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * 1. PHONETIC HAMMING DECODING FOR NOISY DIALOGUE:
 *    - When `IdeamCommunications` delivers a noise-degraded sequence of `uint16_t`
 *      articulatory phoneme masks to a Listener, `decode_noisy_phoneme_slice()` searches
 *      the active `Lexicon` for the `LexemePOD` minimizing:
 *        Score(L) = w_phonetic * HammingDistance(received_features, L.phonemes)
 *                 - w_context  * SememeAffinity(L.sememe_id, listener_expected_sememes)
 *                 - w_freq     * L.corpus_frequency
 *    - This naturally produces context-sensitive mondegreens (hearing what you expect/fear
 *      when the room is loud) and feeds the resulting `SememeID` into `IdeamMinds`!
 *
 * 2. UNIVERSAL ASSET TAGGING & WIZARD PROMPT PARSING:
 *    - `parse_text_to_seme_masks()` tokenizes natural language strings like
 *      `"top down, medieval, fantasy RPG"`, resolves matching `LexemePOD -> SememeDescriptorPOD`
 *      entries, and unions their `nuclear_seme_words` and `peripheral_seme_words` so any
 *      Editor Wizard or `ProceduralPattern` can match assets in O(1) bitwise operations.
 */

#include "linguistics_common.h"
#include <span>
#include <vector>

namespace ideam::core {

class SemanticLexiconCore {
private:
    std::vector<SemeDescriptorPOD> semes_;
    std::vector<SememeDescriptorPOD> sememes_;
    std::vector<PhonemePOD> phonemes_;
    std::vector<MorphemePOD> morphemes_;
    std::vector<LexemePOD> lexemes_;
    std::vector<uint16_t> lexeme_phoneme_feature_csr_;

public:
    SemanticLexiconCore() = default;
    ~SemanticLexiconCore() = default;

    void clear() noexcept;

    [[nodiscard]] const std::vector<SemeDescriptorPOD> &semes() const noexcept { return semes_; }
    [[nodiscard]] const std::vector<SememeDescriptorPOD> &sememes() const noexcept { return sememes_; }
    [[nodiscard]] const std::vector<PhonemePOD> &phonemes() const noexcept { return phonemes_; }
    [[nodiscard]] const std::vector<MorphemePOD> &morphemes() const noexcept { return morphemes_; }
    [[nodiscard]] const std::vector<LexemePOD> &lexemes() const noexcept { return lexemes_; }

    [[nodiscard]] uint32_t find_best_matching_lexeme_from_noisy_phonemes(
        std::span<const uint16_t> p_noisy_phoneme_masks,
        uint32_t p_expected_sememe_id = 0xFFFFFFFFu,
        uint16_t p_preferred_lect_id = 0
    ) const noexcept;

    [[nodiscard]] float compute_sememe_affinity(
        uint32_t p_sememe_a,
        uint32_t p_sememe_b
    ) const noexcept;
};

} // namespace ideam::core
