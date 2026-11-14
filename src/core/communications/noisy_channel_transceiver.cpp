#include "noisy_channel_transceiver.h"
#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace ideam::core {

namespace {

[[nodiscard]] constexpr uint64_t splitmix64_next(uint64_t &r_state) noexcept {
    uint64_t z = (r_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30u)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27u)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31u);
}

[[nodiscard]] constexpr float random_unit_float(uint64_t &r_state) noexcept {
    const uint64_t bits = splitmix64_next(r_state);
    return static_cast<float>((bits >> 40u) & 0xFFFFFFu) / static_cast<float>(0x1000000u);
}

[[nodiscard]] std::string trim_ascii(std::string_view p_sv) {
    size_t b = 0;
    while (b < p_sv.size() && std::isspace(static_cast<unsigned char>(p_sv[b]))) ++b;
    size_t e = p_sv.size();
    while (e > b && std::isspace(static_cast<unsigned char>(p_sv[e - 1]))) --e;
    return std::string(p_sv.substr(b, e - b));
}

[[nodiscard]] std::string lower_ascii(std::string_view p_sv) {
    std::string out;
    out.reserve(p_sv.size());
    for (char c : p_sv) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '\'') {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return out;
}

[[nodiscard]] std::string extract_xml_attr(std::string_view p_tag, std::string_view p_attr_name) {
    std::string needle = std::string(p_attr_name) + "=";
    size_t pos = p_tag.find(needle);
    if (pos == std::string_view::npos) return {};
    pos += needle.size();
    if (pos >= p_tag.size()) return {};
    const char quote = p_tag[pos];
    if (quote != '"' && quote != '\'') return {};
    ++pos;
    const size_t end = p_tag.find(quote, pos);
    if (end == std::string_view::npos) return {};
    return std::string(p_tag.substr(pos, end - pos));
}

} // namespace

// ============================================================================
// Stage 1: Encoder
// ============================================================================

void NoisyChannelTransceiver::encode_text_to_phoneme_stream(
    std::string_view p_text,
    const SemanticLexiconCore &p_lexicon,
    const TransmitterStatePOD &p_tx,
    uint64_t p_seed,
    std::vector<uint16_t> &r_phoneme_masks,
    std::vector<uint32_t> &r_word_phoneme_offsets,
    std::vector<uint32_t> &r_word_phoneme_counts,
    std::vector<std::string> &r_source_words
) {
    r_phoneme_masks.clear();
    r_word_phoneme_offsets.clear();
    r_word_phoneme_counts.clear();
    r_source_words.clear();

    uint64_t rng = p_seed ^ (static_cast<uint64_t>(p_tx.transmitter_id) << 32u);
    const float slur_prob = std::clamp(0.75f - p_tx.elocution_encode_skill, 0.0f, 0.65f);

    size_t i = 0;
    while (i < p_text.size()) {
        while (i < p_text.size() && std::isspace(static_cast<unsigned char>(p_text[i]))) ++i;
        if (i >= p_text.size()) break;
        const size_t start = i;
        while (i < p_text.size() && !std::isspace(static_cast<unsigned char>(p_text[i]))) ++i;

        std::string raw_tok(p_text.substr(start, i - start));
        std::string clean_tok = lower_ascii(raw_tok);
        if (clean_tok.empty()) continue;

        r_source_words.push_back(raw_tok);
        const uint32_t offset = static_cast<uint32_t>(r_phoneme_masks.size());
        r_word_phoneme_offsets.push_back(offset);

        // Check if lexicon has a known LexemePOD with CSR phoneme masks
        const int32_t lex_idx = p_lexicon.find_lexeme_by_surface(clean_tok, p_tx.active_lect_id);
        bool encoded_from_lexicon = false;
        if (lex_idx >= 0) {
            const std::span<const uint16_t> masks = p_lexicon.get_lexeme_phoneme_masks(static_cast<uint32_t>(lex_idx));
            if (!masks.empty()) {
                for (uint16_t mask : masks) {
                    if (slur_prob > 0.0f && random_unit_float(rng) < slur_prob) {
                        mask = static_cast<uint16_t>(mask & ~(FEAT_STRIDENT | FEAT_GLOTTAL));
                    }
                    r_phoneme_masks.push_back(mask);
                }
                encoded_from_lexicon = true;
            }
        }

        if (!encoded_from_lexicon) {
            std::vector<uint16_t> g2p_masks;
            p_lexicon.approximate_phonemes_from_text(clean_tok, g2p_masks);
            for (uint16_t mask : g2p_masks) {
                if (slur_prob > 0.0f && random_unit_float(rng) < slur_prob) {
                    mask = static_cast<uint16_t>(mask & ~(FEAT_STRIDENT | FEAT_GLOTTAL));
                }
                r_phoneme_masks.push_back(mask);
            }
        }

        const uint32_t count = static_cast<uint32_t>(r_phoneme_masks.size()) - offset;
        r_word_phoneme_counts.push_back(count);
    }
}

// ============================================================================
// Stage 2: Transmitter & Medium
// ============================================================================

float NoisyChannelTransceiver::compute_effective_snr_db(
    const TransmitterStatePOD &p_tx,
    const TransmissionMediumPOD &p_medium,
    const ReceiverStatePOD &p_rx,
    float p_prosody_volume_gain
) noexcept {
    const float dx = p_rx.pos_x - p_tx.pos_x;
    const float dy = p_rx.pos_y - p_tx.pos_y;
    const float dz = p_rx.pos_z - p_tx.pos_z;
    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    const float volume_clamped = std::clamp(p_prosody_volume_gain, 0.1f, 4.0f);
    const float prosody_db_offset = 10.0f * std::log10(volume_clamped);

    const float tx_gain = p_tx.projection_power_db
                        + 6.0f * (p_tx.elocution_encode_skill - 1.0f)
                        + prosody_db_offset;
    const float rx_gain = p_rx.perception_bonus_db
                        + 6.0f * (p_rx.listen_decode_skill - 1.0f);

    const float reverb_db = static_cast<float>(p_medium.reverb_intelligibility_penalty) * 0.04f;
    const float path_loss = dist * p_medium.attenuation_db_per_m
                          + p_medium.occlusion_damping_db
                          + reverb_db;

    return (tx_gain + rx_gain) - (p_medium.ambient_noise_db + path_loss);
}

NoiseVulnerabilityTier NoisyChannelTransceiver::classify_noise_tier(
    float p_effective_snr_db
) noexcept {
    if (p_effective_snr_db >= 18.0f) {
        return NoiseVulnerabilityTier::TIER_0_CLEAN;
    }
    if (p_effective_snr_db >= 10.0f) {
        return NoiseVulnerabilityTier::TIER_1_MODERATE_PLACE_FLIP;
    }
    if (p_effective_snr_db >= 3.0f) {
        return NoiseVulnerabilityTier::TIER_2_HEAVY_MANNER_FLIP;
    }
    if (p_effective_snr_db >= -5.0f) {
        return NoiseVulnerabilityTier::TIER_3_EXTREME_VOICING_VOWEL_MASK;
    }
    return NoiseVulnerabilityTier::TIER_4_UNINTELLIGIBLE_DROPOUT;
}

float NoisyChannelTransceiver::compute_bit_flip_probability(
    float p_effective_snr_db,
    float p_tier_sensitivity,
    uint8_t p_reverb_penalty
) noexcept {
    if (p_effective_snr_db >= 22.0f || p_tier_sensitivity <= 0.0f) {
        return 0.0f;
    }
    const float deficit_db = std::max(0.0f, 18.0f - p_effective_snr_db);
    const float reverb_boost = static_cast<float>(p_reverb_penalty) / 510.0f;
    const float raw_prob = (1.0f - std::exp(-0.085f * deficit_db)) * p_tier_sensitivity + reverb_boost;
    return std::clamp(raw_prob, 0.0f, 0.95f);
}

// ============================================================================
// Stage 3: Noise (Psycholinguistic Articulatory Feature Degradation)
// ============================================================================

uint8_t NoisyChannelTransceiver::degrade_phoneme_features_in_medium(
    const TransmissionMediumPOD &p_medium,
    float p_effective_snr_db,
    uint64_t p_random_seed,
    std::span<uint16_t> r_phoneme_feature_masks
) noexcept {
    const NoiseVulnerabilityTier tier = classify_noise_tier(p_effective_snr_db);
    if (tier == NoiseVulnerabilityTier::TIER_0_CLEAN || r_phoneme_feature_masks.empty()) {
        return 0u;
    }

    uint64_t rng = p_random_seed ^ 0xA5A5A5A55A5A5A5AULL;
    uint32_t garbled_count = 0u;

    const float p_place = compute_bit_flip_probability(
        p_effective_snr_db, p_medium.place_flip_sensitivity, p_medium.reverb_intelligibility_penalty);
    const float p_manner = (static_cast<uint8_t>(tier) >= static_cast<uint8_t>(NoiseVulnerabilityTier::TIER_2_HEAVY_MANNER_FLIP))
        ? compute_bit_flip_probability(p_effective_snr_db - 4.0f, p_medium.manner_flip_sensitivity, p_medium.reverb_intelligibility_penalty)
        : 0.0f;
    const float p_voicing = (static_cast<uint8_t>(tier) >= static_cast<uint8_t>(NoiseVulnerabilityTier::TIER_3_EXTREME_VOICING_VOWEL_MASK))
        ? compute_bit_flip_probability(p_effective_snr_db - 10.0f, p_medium.voicing_flip_sensitivity, p_medium.reverb_intelligibility_penalty)
        : 0.0f;

    constexpr uint16_t PLACE_BITS[5]  = { FEAT_LABIAL, FEAT_CORONAL, FEAT_ANTERIOR, FEAT_DORSAL, FEAT_GLOTTAL };
    constexpr uint16_t MANNER_BITS[4] = { FEAT_CONTINUANT, FEAT_NASAL, FEAT_STRIDENT, FEAT_LATERAL };

    for (uint16_t &mask : r_phoneme_feature_masks) {
        if (mask == 0u) continue;

        if (tier == NoiseVulnerabilityTier::TIER_4_UNINTELLIGIBLE_DROPOUT) {
            mask = 0u;
            ++garbled_count;
            continue;
        }

        // Tier 1+: Flip Place-of-Articulation bits (/p/ <-> /t/ <-> /k/, /f/ <-> /s/)
        for (uint16_t bit : PLACE_BITS) {
            if (random_unit_float(rng) < p_place) {
                mask ^= bit;
            }
        }

        // Tier 2+: Flip Manner bits (Continuant / Nasal / Strident / Lateral)
        if (p_manner > 0.0f) {
            for (uint16_t bit : MANNER_BITS) {
                if (random_unit_float(rng) < p_manner) {
                    mask ^= bit;
                }
            }
        }

        // Tier 3+: Extreme noise erases Voicing & Vowel features into unrecoverable syllable markers
        if (p_voicing > 0.0f && random_unit_float(rng) < p_voicing) {
            mask = 0u;
            ++garbled_count;
        }
    }

    return static_cast<uint8_t>(std::min<uint32_t>(255u, garbled_count));
}

// ============================================================================
// Stage 4: Receiver & Decoder
// ============================================================================

uint32_t NoisyChannelTransceiver::decode_word_phoneme_slice(
    std::span<const uint16_t> p_degraded_slice,
    const SemanticLexiconCore &p_lexicon,
    const ReceiverStatePOD &p_rx,
    uint16_t p_preferred_lect_id,
    float p_unintelligible_ceiling,
    float &r_best_distance,
    uint32_t &r_decoded_sememe_id
) noexcept {
    r_best_distance     = 1e9f;
    r_decoded_sememe_id = 0xFFFFFFFFu;

    if (p_degraded_slice.empty()) {
        return 0xFFFFFFFFu;
    }

    bool any_nonzero = false;
    for (uint16_t m : p_degraded_slice) {
        if (m != 0u) {
            any_nonzero = true;
            break;
        }
    }
    if (!any_nonzero) {
        return 0xFFFFFFFFu;
    }

    const auto &lexemes   = p_lexicon.lexemes();
    const auto &csr_masks = p_lexicon.lexeme_phoneme_feature_csr();
    if (lexemes.empty() || csr_masks.empty()) {
        return 0xFFFFFFFFu;
    }

    uint32_t best_lex_id = 0xFFFFFFFFu;

    for (size_t l = 0; l < lexemes.size(); ++l) {
        const LexemePOD &lx = lexemes[l];
        if (lx.phoneme_count == 0u || lx.phoneme_csr_offset + lx.phoneme_count > csr_masks.size()) {
            continue;
        }
        const std::span<const uint16_t> cand_slice(
            csr_masks.data() + lx.phoneme_csr_offset,
            lx.phoneme_count
        );

        float dist = SemanticLexiconCore::compute_phoneme_slice_distance(p_degraded_slice, cand_slice);

        if (p_rx.expected_sememe_id != 0xFFFFFFFFu && lx.sememe_id == p_rx.expected_sememe_id) {
            dist = std::max(0.0f, dist - 0.12f * p_rx.listen_decode_skill);
        }
        if (lx.lect_id == p_preferred_lect_id) {
            dist = std::max(0.0f, dist - 0.04f);
        }

        if (dist < r_best_distance) {
            r_best_distance     = dist;
            best_lex_id         = static_cast<uint32_t>(l);
            r_decoded_sememe_id = lx.sememe_id;
        }
    }

    if (r_best_distance > p_unintelligible_ceiling) {
        r_decoded_sememe_id = 0xFFFFFFFFu;
        return 0xFFFFFFFFu;
    }
    return best_lex_id;
}

DecodedUtteranceResult NoisyChannelTransceiver::transmit_and_decode_utterance(
    std::string_view p_raw_text,
    uint32_t p_source_fact_id,
    uint32_t p_intended_sememe_id,
    const TransmitterStatePOD &p_tx,
    const TransmissionMediumPOD &p_medium,
    const ReceiverStatePOD &p_rx,
    const SemanticLexiconCore &p_lexicon,
    uint64_t p_random_seed,
    ProsodySamplePOD p_prosody
) {
    DecodedUtteranceResult res{};
    res.original_text = std::string(p_raw_text);

    std::vector<uint32_t> word_offsets;
    std::vector<uint32_t> word_counts;
    std::vector<std::string> source_words;

    encode_text_to_phoneme_stream(
        p_raw_text,
        p_lexicon,
        p_tx,
        p_random_seed,
        res.transmitted_phoneme_masks,
        word_offsets,
        word_counts,
        source_words
    );

    res.degraded_phoneme_masks = res.transmitted_phoneme_masks;

    const float snr_db = compute_effective_snr_db(p_tx, p_medium, p_rx, p_prosody.volume_gain);
    const NoiseVulnerabilityTier tier = classify_noise_tier(snr_db);
    const uint8_t garbled = degrade_phoneme_features_in_medium(
        p_medium,
        snr_db,
        p_random_seed,
        res.degraded_phoneme_masks
    );

    res.packet.transmitter_id         = p_tx.transmitter_id;
    res.packet.receiver_id            = p_rx.receiver_id;
    res.packet.medium_id              = p_medium.medium_id;
    res.packet.source_fact_id         = p_source_fact_id;
    res.packet.decoded_fact_id        = p_source_fact_id;
    res.packet.phoneme_csr_offset     = 0u;
    res.packet.phoneme_count          = static_cast<uint32_t>(res.degraded_phoneme_masks.size());
    res.packet.effective_snr_db       = snr_db;
    res.packet.percent_transmitted    = 1.0f;
    res.packet.current_volume         = p_prosody.volume_gain;
    res.packet.current_pitch          = p_prosody.pitch_multiplier;
    res.packet.garbled_syllable_count = garbled;
    res.packet.intended_sememe_id     = p_intended_sememe_id;
    res.packet.decoded_sememe_id      = p_intended_sememe_id;
    res.packet.active_lect_id         = p_tx.active_lect_id;
    res.packet.noise_tier             = static_cast<uint8_t>(tier);

    std::ostringstream oss;
    bool any_misheard_word = false;
    uint32_t primary_decoded_sememe = p_intended_sememe_id;

    for (size_t w = 0; w < source_words.size(); ++w) {
        if (w > 0) oss << ' ';
        const uint32_t off = word_offsets[w];
        const uint32_t cnt = word_counts[w];
        if (cnt == 0u || off + cnt > res.degraded_phoneme_masks.size()) {
            oss << "...";
            continue;
        }

        const std::span<const uint16_t> deg_slice(res.degraded_phoneme_masks.data() + off, cnt);
        const std::span<const uint16_t> orig_slice(res.transmitted_phoneme_masks.data() + off, cnt);

        bool identical = true;
        for (uint32_t k = 0; k < cnt; ++k) {
            if (deg_slice[k] != orig_slice[k]) {
                identical = false;
                break;
            }
        }

        float best_dist = 0.0f;
        uint32_t dec_sem = 0xFFFFFFFFu;
        const uint32_t dec_lex = decode_word_phoneme_slice(
            deg_slice,
            p_lexicon,
            p_rx,
            p_tx.active_lect_id,
            0.55f,
            best_dist,
            dec_sem
        );

        res.decoded_lexeme_ids.push_back(dec_lex);
        res.decoded_sememe_ids.push_back(dec_sem);

        if (identical && tier == NoiseVulnerabilityTier::TIER_0_CLEAN) {
            oss << source_words[w];
            if (dec_sem != 0xFFFFFFFFu && primary_decoded_sememe == 0xFFFFFFFFu) {
                primary_decoded_sememe = dec_sem;
            }
            continue;
        }

        if (dec_lex != 0xFFFFFFFFu) {
            const std::string_view orth = p_lexicon.get_lexeme_surface(dec_lex);
            oss << (orth.empty() ? source_words[w] : std::string(orth));
            if (lower_ascii(source_words[w]) != orth) {
                any_misheard_word = true;
                primary_decoded_sememe = dec_sem;
            }
        } else if (identical) {
            oss << source_words[w];
        } else {
            oss << "...";
        }
    }

    res.reconstructed_text = oss.str();
    res.packet.decoded_sememe_id = primary_decoded_sememe;

    const bool sememe_shifted = (p_intended_sememe_id != 0xFFFFFFFFu &&
                                 primary_decoded_sememe != 0xFFFFFFFFu &&
                                 primary_decoded_sememe != p_intended_sememe_id);
    const bool is_misref = sememe_shifted || any_misheard_word;

    if (is_misref && p_source_fact_id != 0xFFFFFFFFu) {
        const uint32_t shift = (primary_decoded_sememe != 0xFFFFFFFFu)
            ? (primary_decoded_sememe + 1u)
            : 1u;
        res.packet.decoded_fact_id     = p_source_fact_id ^ shift;
        res.packet.honest_misreference = 1u;
    }

    const float snr_norm = std::clamp((snr_db + 5.0f) / 25.0f, 0.05f, 1.0f);
    res.epistemic_handoff.packet_id              = res.packet.packet_id;
    res.epistemic_handoff.receiver_mind_agent_id = p_rx.mind_agent_id;
    res.epistemic_handoff.source_fact_id         = p_source_fact_id;
    res.epistemic_handoff.decoded_fact_id        = res.packet.decoded_fact_id;
    res.epistemic_handoff.decoded_sememe_id      = res.packet.decoded_sememe_id;
    res.epistemic_handoff.effective_snr_db       = snr_db;
    res.epistemic_handoff.decoded_confidence     = snr_norm;
    res.epistemic_handoff.tome_id                = static_cast<uint16_t>(TOME_PHYSICAL_REALITY);
    res.epistemic_handoff.honest_misreference    = res.packet.honest_misreference;
    res.epistemic_handoff.garbled_syllable_count = garbled;

    res.synthesized_belief.fact_id          = res.packet.decoded_fact_id;
    res.synthesized_belief.source_agent_id  = p_tx.mind_agent_id;
    res.synthesized_belief.believed_value   = static_cast<int64_t>(res.packet.decoded_sememe_id);
    res.synthesized_belief.confidence       = snr_norm;
    res.synthesized_belief.acceptance       = is_misref ? 0.65f : 0.90f;
    res.synthesized_belief.emotional_charge = std::abs(p_prosody.pitch_multiplier - 1.0f);
    res.synthesized_belief.intuition_charge = (tier >= NoiseVulnerabilityTier::TIER_2_HEAVY_MANNER_FLIP) ? 0.35f : 0.0f;

    return res;
}

// ============================================================================
// Sacks-Schegloff-Jefferson (1974) Discourse Turn Allocation State Machine
// ============================================================================

DiscourseAllocationOutcome NoisyChannelTransceiver::evaluate_discourse_turn_allocation(
    DiscourseTurnStatePOD &r_turn_state,
    std::span<DiscourseParticipantPOD> r_participants
) noexcept {
    if (r_participants.empty()) {
        r_turn_state.applied_rule = DiscourseAllocationOutcome::OUTCOME_NONE;
        return DiscourseAllocationOutcome::OUTCOME_NONE;
    }

    if (r_turn_state.interruption_margin_db >= 6.0f ||
        r_turn_state.resolution == TurnResolutionType::PRAGMATICAL) {
        uint32_t interrupter_id = r_turn_state.current_speaker_id;
        float best_claim = -1.0f;
        for (const DiscourseParticipantPOD &p : r_participants) {
            if (p.participant_id == r_turn_state.current_speaker_id) continue;
            const float score = p.excitation_urgency + p.turn_claim_priority;
            if (p.wants_floor != 0u && score > best_claim) {
                best_claim     = score;
                interrupter_id = p.participant_id;
            }
        }
        if (interrupter_id != r_turn_state.current_speaker_id) {
            r_turn_state.resolution               = TurnResolutionType::PRAGMATICAL;
            r_turn_state.trp_reached              = 1u;
            r_turn_state.next_selected_speaker_id = interrupter_id;
            r_turn_state.current_speaker_id       = interrupter_id;
            ++r_turn_state.current_turn_index;
            r_turn_state.active_tcu_index         = 0u;
            r_turn_state.applied_rule             = DiscourseAllocationOutcome::OUTCOME_PRAGMATIC_INTERRUPTION;
            for (DiscourseParticipantPOD &p : r_participants) {
                p.is_speaking = (p.participant_id == interrupter_id) ? 1u : 0u;
            }
            return DiscourseAllocationOutcome::OUTCOME_PRAGMATIC_INTERRUPTION;
        }
    }

    const bool intonational_trp = (std::abs(r_turn_state.prosody_pitch_delta) >= 0.18f);
    const bool grammatical_trp  = (r_turn_state.resolution == TurnResolutionType::GRAMMATICAL ||
                                   r_turn_state.resolution == TurnResolutionType::INTONATIONAL ||
                                   (r_turn_state.active_tcu_index + 1u >= r_turn_state.tcu_count));

    if (!intonational_trp && !grammatical_trp) {
        r_turn_state.trp_reached  = 0u;
        r_turn_state.applied_rule = DiscourseAllocationOutcome::OUTCOME_1C_SPEAKER_CONTINUES;
        return DiscourseAllocationOutcome::OUTCOME_1C_SPEAKER_CONTINUES;
    }

    r_turn_state.trp_reached = 1u;
    if (r_turn_state.resolution == TurnResolutionType::UNRESOLVED) {
        r_turn_state.resolution = intonational_trp
            ? TurnResolutionType::INTONATIONAL
            : TurnResolutionType::GRAMMATICAL;
    }

    const DiscourseParticipantPOD *current_speaker = nullptr;
    for (const DiscourseParticipantPOD &p : r_participants) {
        if (p.participant_id == r_turn_state.current_speaker_id) {
            current_speaker = &p;
            break;
        }
    }

    // Rule 1a: Current speaker explicitly selects next speaker (`addressed_target_id`)
    const uint32_t explicit_target = (current_speaker && current_speaker->addressed_target_id != 0xFFFFFFFFu)
        ? current_speaker->addressed_target_id
        : r_turn_state.next_selected_speaker_id;

    if (explicit_target != 0xFFFFFFFFu && explicit_target != r_turn_state.current_speaker_id) {
        for (const DiscourseParticipantPOD &p : r_participants) {
            if (p.participant_id == explicit_target) {
                r_turn_state.next_selected_speaker_id = explicit_target;
                r_turn_state.current_speaker_id       = explicit_target;
                ++r_turn_state.current_turn_index;
                r_turn_state.active_tcu_index         = 0u;
                r_turn_state.applied_rule             = DiscourseAllocationOutcome::OUTCOME_1A_SPEAKER_SELECTS_NEXT;
                for (DiscourseParticipantPOD &part : r_participants) {
                    part.is_speaking = (part.participant_id == explicit_target) ? 1u : 0u;
                }
                return DiscourseAllocationOutcome::OUTCOME_1A_SPEAKER_SELECTS_NEXT;
            }
        }
    }

    // Rule 1b: Self-selection by highest excitation_urgency + turn_claim_priority
    uint32_t best_self_selector = 0xFFFFFFFFu;
    float best_self_score       = 0.15f;
    for (const DiscourseParticipantPOD &p : r_participants) {
        if (p.participant_id == r_turn_state.current_speaker_id) continue;
        const float score = p.excitation_urgency * 0.65f + p.turn_claim_priority * 0.35f + (p.wants_floor ? 0.25f : 0.0f);
        if (score > best_self_score) {
            best_self_score    = score;
            best_self_selector = p.participant_id;
        }
    }

    if (best_self_selector != 0xFFFFFFFFu) {
        r_turn_state.next_selected_speaker_id = best_self_selector;
        r_turn_state.current_speaker_id       = best_self_selector;
        ++r_turn_state.current_turn_index;
        r_turn_state.active_tcu_index         = 0u;
        r_turn_state.applied_rule             = DiscourseAllocationOutcome::OUTCOME_1B_SELF_SELECTION;
        for (DiscourseParticipantPOD &part : r_participants) {
            part.is_speaking = (part.participant_id == best_self_selector) ? 1u : 0u;
        }
        return DiscourseAllocationOutcome::OUTCOME_1B_SELF_SELECTION;
    }

    // Rule 1c: Current speaker continues with next Turn Construction Unit
    ++r_turn_state.active_tcu_index;
    r_turn_state.next_selected_speaker_id = r_turn_state.current_speaker_id;
    r_turn_state.applied_rule             = DiscourseAllocationOutcome::OUTCOME_1C_SPEAKER_CONTINUES;
    return DiscourseAllocationOutcome::OUTCOME_1C_SPEAKER_CONTINUES;
}

// ============================================================================
// SSML 1.1 Parser & Exporter
// ============================================================================

SSMLParsedUtterance NoisyChannelTransceiver::parse_ssml_document(
    std::string_view p_ssml,
    const SemanticLexiconCore *p_optional_lexicon
) {
    SSMLParsedUtterance out{};
    std::ostringstream text_stream;

    float active_volume_norm = static_cast<float>(out.volume) / 100.0f;
    float active_pitch       = out.pitch;
    float active_rate        = out.rate;

    size_t pos = 0;
    while (pos < p_ssml.size()) {
        const size_t lt = p_ssml.find('<', pos);
        if (lt == std::string_view::npos) {
            std::string tail = trim_ascii(p_ssml.substr(pos));
            if (!tail.empty()) {
                if (text_stream.tellp() > 0) text_stream << ' ';
                text_stream << tail;
            }
            break;
        }

        if (lt > pos) {
            std::string chunk = trim_ascii(p_ssml.substr(pos, lt - pos));
            if (!chunk.empty()) {
                if (text_stream.tellp() > 0) text_stream << ' ';
                text_stream << chunk;
                ProsodySamplePOD sample{};
                sample.volume_gain      = active_volume_norm;
                sample.pitch_multiplier = active_pitch;
                sample.speaking_rate    = active_rate;
                sample.normalized_time  = static_cast<float>(out.prosody_contour.size()) * 0.25f;
                out.prosody_contour.push_back(sample);
            }
        }

        const size_t gt = p_ssml.find('>', lt + 1);
        if (gt == std::string_view::npos) break;

        std::string_view tag = p_ssml.substr(lt + 1, gt - lt - 1);
        if (tag.starts_with("voice")) {
            const std::string name = extract_xml_attr(tag, "name");
            if (!name.empty()) out.voice_name = name;
            const std::string gender = extract_xml_attr(tag, "gender");
            if (gender == "female") out.gender = 1;
            else if (gender == "male") out.gender = 2;
            else if (gender == "neutral") out.gender = 3;
            const std::string age = extract_xml_attr(tag, "age");
            if (!age.empty()) out.age = std::atoi(age.c_str());
            const std::string variant = extract_xml_attr(tag, "variant");
            if (!variant.empty()) out.variant = std::atoi(variant.c_str());
            const std::string lang = extract_xml_attr(tag, "xml:lang");
            if (!lang.empty()) out.desired_languages.push_back(lang);
        } else if (tag.starts_with("prosody")) {
            const std::string pitch_str = extract_xml_attr(tag, "pitch");
            if (!pitch_str.empty()) {
                if (pitch_str == "high" || pitch_str == "x-high") active_pitch = 1.35f;
                else if (pitch_str == "low" || pitch_str == "x-low") active_pitch = 0.75f;
                else if (pitch_str.ends_with("%")) {
                    const float pct = static_cast<float>(std::atof(pitch_str.c_str()));
                    active_pitch = std::clamp(1.0f + pct / 100.0f, 0.25f, 2.0f);
                } else {
                    active_pitch = std::clamp(static_cast<float>(std::atof(pitch_str.c_str())), 0.25f, 2.0f);
                }
                out.pitch = active_pitch;
            }

            const std::string rate_str = extract_xml_attr(tag, "rate");
            if (!rate_str.empty()) {
                if (rate_str == "fast" || rate_str == "x-fast") active_rate = 1.4f;
                else if (rate_str == "slow" || rate_str == "x-slow") active_rate = 0.7f;
                else if (rate_str.ends_with("%")) {
                    const float pct = static_cast<float>(std::atof(rate_str.c_str()));
                    active_rate = std::clamp(pct / 100.0f, 0.1f, 10.0f);
                } else {
                    active_rate = std::clamp(static_cast<float>(std::atof(rate_str.c_str())), 0.1f, 10.0f);
                }
                out.rate = active_rate;
            }

            const std::string vol_str = extract_xml_attr(tag, "volume");
            if (!vol_str.empty()) {
                if (vol_str == "loud" || vol_str == "x-loud") out.volume = 100;
                else if (vol_str == "soft" || vol_str == "x-soft") out.volume = 35;
                else if (vol_str == "silent") out.volume = 0;
                else {
                    out.volume = std::clamp(std::atoi(vol_str.c_str()), 0, 100);
                }
                active_volume_norm = static_cast<float>(out.volume) / 100.0f;
            }

            ProsodySamplePOD sample{};
            sample.volume_gain      = active_volume_norm;
            sample.pitch_multiplier = active_pitch;
            sample.speaking_rate    = active_rate;
            sample.normalized_time  = static_cast<float>(out.prosody_contour.size()) * 0.25f;
            out.prosody_contour.push_back(sample);
        } else if (tag.starts_with("phoneme")) {
            const std::string ph_str = extract_xml_attr(tag, "ph");
            if (!ph_str.empty()) {
                std::istringstream iss(ph_str);
                std::string sym;
                while (iss >> sym) {
                    const uint16_t mask = SemanticLexiconCore::arpabet_to_articulatory_mask(sym);
                    if (mask != 0u) {
                        out.phoneme_masks.push_back(mask);
                    }
                }
            }
        }

        pos = gt + 1;
    }

    out.plain_text = text_stream.str();

    if (out.prosody_contour.empty()) {
        ProsodySamplePOD s0{ active_volume_norm, active_pitch, active_rate, 0.0f };
        ProsodySamplePOD s1{ active_volume_norm, active_pitch, active_rate, 1.0f };
        out.prosody_contour.push_back(s0);
        out.prosody_contour.push_back(s1);
    } else if (out.prosody_contour.size() > 1) {
        const float denom = static_cast<float>(out.prosody_contour.size() - 1);
        for (size_t k = 0; k < out.prosody_contour.size(); ++k) {
            out.prosody_contour[k].normalized_time = static_cast<float>(k) / denom;
        }
    }

    if (out.phoneme_masks.empty() && p_optional_lexicon != nullptr && !out.plain_text.empty()) {
        p_optional_lexicon->approximate_phonemes_from_text(out.plain_text, out.phoneme_masks);
    }

    return out;
}

std::string NoisyChannelTransceiver::export_to_ssml_document(
    const SSMLParsedUtterance &p_utterance
) {
    std::ostringstream oss;
    oss << "<speak version=\"1.1\" xmlns=\"http://www.w3.org/2001/10/synthesis\"";
    if (!p_utterance.desired_languages.empty()) {
        oss << " xml:lang=\"" << p_utterance.desired_languages.front() << "\"";
    } else {
        oss << " xml:lang=\"en-US\"";
    }
    oss << ">";

    const bool has_voice = !p_utterance.voice_name.empty() || p_utterance.gender > 0 || p_utterance.age > 0;
    if (has_voice) {
        oss << "<voice";
        if (!p_utterance.voice_name.empty()) oss << " name=\"" << p_utterance.voice_name << "\"";
        if (p_utterance.gender == 1) oss << " gender=\"female\"";
        else if (p_utterance.gender == 2) oss << " gender=\"male\"";
        else if (p_utterance.gender == 3) oss << " gender=\"neutral\"";
        if (p_utterance.age > 0) oss << " age=\"" << p_utterance.age << "\"";
        if (p_utterance.variant > 0) oss << " variant=\"" << p_utterance.variant << "\"";
        oss << ">";
    }

    oss << "<prosody pitch=\"" << p_utterance.pitch
        << "\" rate=\"" << p_utterance.rate
        << "\" volume=\"" << p_utterance.volume << "\">";
    oss << p_utterance.plain_text;
    oss << "</prosody>";

    if (has_voice) {
        oss << "</voice>";
    }
    oss << "</speak>";
    return oss.str();
}

} // namespace ideam::core
