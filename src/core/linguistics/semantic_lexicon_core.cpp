#include "semantic_lexicon_core.h"

namespace ideam::core {

void SemanticLexiconCore::clear() noexcept {
    semes_.clear();
    sememes_.clear();
    phonemes_.clear();
    morphemes_.clear();
    lexemes_.clear();
    lexeme_phoneme_feature_csr_.clear();
}

uint32_t SemanticLexiconCore::find_best_matching_lexeme_from_noisy_phonemes(
    std::span<const uint16_t> p_noisy_phoneme_masks,
    uint32_t p_expected_sememe_id,
    uint16_t p_preferred_lect_id
) const noexcept {
    (void)p_noisy_phoneme_masks;
    (void)p_expected_sememe_id;
    (void)p_preferred_lect_id;
    // Architectural stub: Full SIMD Hamming distance + listener expectation prior scoring
    // over lexeme_phoneme_feature_csr_ will be authored here.
    return 0xFFFFFFFFu;
}

float SemanticLexiconCore::compute_sememe_affinity(
    uint32_t p_sememe_a,
    uint32_t p_sememe_b
) const noexcept {
    if (p_sememe_a >= sememes_.size() || p_sememe_b >= sememes_.size()) {
        return 0.0f;
    }
    // Architectural stub: Weighted Jaccard / Tversky similarity across nuclear_seme_words
    // and peripheral_seme_words will be authored here.
    return (p_sememe_a == p_sememe_b) ? 1.0f : 0.0f;
}

} // namespace ideam::core
