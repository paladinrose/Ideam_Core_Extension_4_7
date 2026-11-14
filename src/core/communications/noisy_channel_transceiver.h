#pragma once

/**
 * noisy_channel_transceiver.h
 * Pure C++26 Shannon-Weaver 6-Stage Noisy-Channel Transceiver, Sacks-Schegloff-Jefferson
 * Turn-Allocation State Machine, and W3C SSML 1.1 Prosody/Phoneme Parser
 * (`src/core/communications/`).
 */

#include "communications_common.h"
#include "../linguistics/semantic_lexicon_core.h"
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

struct DecodedUtteranceResult {
    SignalPacketPOD          packet{};
    EpistemicHandoffPOD      epistemic_handoff{};
    BeliefPod                synthesized_belief{};
    std::vector<uint16_t>    transmitted_phoneme_masks;
    std::vector<uint16_t>    degraded_phoneme_masks;
    std::vector<uint32_t>    decoded_lexeme_ids;
    std::vector<uint32_t>    decoded_sememe_ids;
    std::string              original_text;
    std::string              reconstructed_text;
};

struct SSMLParsedUtterance {
    std::string                    plain_text;
    std::string                    voice_name;
    int32_t                        gender            = 0;
    int32_t                        age               = 0;
    int32_t                        variant           = 0;
    std::vector<std::string>       desired_languages;
    int32_t                        volume            = 80;   ///< TTS volume `[0..100]`
    float                          pitch             = 1.0f; ///< TTS pitch `[0.0..2.0]`
    float                          rate              = 1.0f; ///< TTS rate `[0.1..10.0]`
    int32_t                        utterance_id      = 0;
    bool                           interrupt         = false;
    std::vector<ProsodySamplePOD>  prosody_contour;          ///< `(Volume, Pitch)` control points for `Spline2DResource`
    std::vector<uint16_t>          phoneme_masks;            ///< Explicit `<phoneme>` + G2P 16-bit PanPhon masks
};

class NoisyChannelTransceiver {
public:
    NoisyChannelTransceiver() = default;
    ~NoisyChannelTransceiver() = default;

    // Stage 1: Encoder
    static void encode_text_to_phoneme_stream(
        std::string_view p_text,
        const SemanticLexiconCore &p_lexicon,
        const TransmitterStatePOD &p_tx,
        uint64_t p_seed,
        std::vector<uint16_t> &r_phoneme_masks,
        std::vector<uint32_t> &r_word_phoneme_offsets,
        std::vector<uint32_t> &r_word_phoneme_counts,
        std::vector<std::string> &r_source_words
    );

    // Stage 2: Transmitter & Medium
    [[nodiscard]] static float compute_effective_snr_db(
        const TransmitterStatePOD &p_tx,
        const TransmissionMediumPOD &p_medium,
        const ReceiverStatePOD &p_rx,
        float p_prosody_volume_gain = 1.0f
    ) noexcept;

    [[nodiscard]] static NoiseVulnerabilityTier classify_noise_tier(
        float p_effective_snr_db
    ) noexcept;

    [[nodiscard]] static float compute_bit_flip_probability(
        float p_effective_snr_db,
        float p_tier_sensitivity,
        uint8_t p_reverb_penalty = 0
    ) noexcept;

    // Stage 3: Noise
    static uint8_t degrade_phoneme_features_in_medium(
        const TransmissionMediumPOD &p_medium,
        float p_effective_snr_db,
        uint64_t p_random_seed,
        std::span<uint16_t> r_phoneme_feature_masks
    ) noexcept;

    // Stage 4: Receiver & Decoder
    [[nodiscard]] static uint32_t decode_word_phoneme_slice(
        std::span<const uint16_t> p_degraded_slice,
        const SemanticLexiconCore &p_lexicon,
        const ReceiverStatePOD &p_rx,
        uint16_t p_preferred_lect_id,
        float p_unintelligible_ceiling,
        float &r_best_distance,
        uint32_t &r_decoded_sememe_id
    ) noexcept;

    [[nodiscard]] static DecodedUtteranceResult transmit_and_decode_utterance(
        std::string_view p_raw_text,
        uint32_t p_source_fact_id,
        uint32_t p_intended_sememe_id,
        const TransmitterStatePOD &p_tx,
        const TransmissionMediumPOD &p_medium,
        const ReceiverStatePOD &p_rx,
        const SemanticLexiconCore &p_lexicon,
        uint64_t p_random_seed = 0x9E3779B97F4A7C15ULL,
        ProsodySamplePOD p_prosody = ProsodySamplePOD{}
    );

    // Sacks-Schegloff-Jefferson (1974) Discourse Turn Allocation State Machine
    static DiscourseAllocationOutcome evaluate_discourse_turn_allocation(
        DiscourseTurnStatePOD &r_turn_state,
        std::span<DiscourseParticipantPOD> r_participants
    ) noexcept;

    // SSML 1.1 Parser & Exporter
    [[nodiscard]] static SSMLParsedUtterance parse_ssml_document(
        std::string_view p_ssml,
        const SemanticLexiconCore *p_optional_lexicon = nullptr
    );

    [[nodiscard]] static std::string export_to_ssml_document(
        const SSMLParsedUtterance &p_utterance
    );
};

} // namespace ideam::core
