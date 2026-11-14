#pragma once

/**
 * communications_common.h
 * Pure C++26 Shannon-Weaver Noisy-Channel Signal Transmission, Psycholinguistic
 * Articulatory Feature Degradation, Sacks-Schegloff-Jefferson Turn-Taking, and
 * Epistemic Misreference Handoff primitives for `IdeamCommunications`
 * (`src/core/communications/`).
 */

#include "../linguistics/linguistics_common.h"
#include "../minds/mind_common.h"
#include <algorithm>
#include <cstdint>

namespace ideam::core {

enum class CommunicationModality : uint8_t {
    SPOKEN_ACOUSTIC        = 0,
    WRITTEN_INSCRIPTION    = 1,
    RADIO_ELECTROMAGNETIC  = 2,
    NON_VERBAL_GESTURE     = 3,
    CYMATIC_RESONANCE      = 4
};

/**
 * Extended Sacks-Schegloff-Jefferson Turn Allocation Outcomes for `IdeamCommunications`
 * (aligned with `TurnAllocationRule` in `mind_common.h` while adding explicit
 * `PRAGMATIC_INTERRUPTION` and `NONE` states).
 */
enum class DiscourseAllocationOutcome : uint8_t {
    OUTCOME_NONE                    = 0,
    OUTCOME_1A_SPEAKER_SELECTS_NEXT = 1, ///< Rule 1a (`TurnAllocationRule::SPEAKER_SELECTS_NEXT`)
    OUTCOME_1B_SELF_SELECTION       = 2, ///< Rule 1b (`TurnAllocationRule::SELF_SELECT_FIRST`)
    OUTCOME_1C_SPEAKER_CONTINUES    = 3, ///< Rule 1c (`TurnAllocationRule::SPEAKER_CONTINUES`)
    OUTCOME_PRAGMATIC_INTERRUPTION  = 4  ///< Floor seized mid-TCU via overwhelming interruption energy
};

/**
 * Perceptual Noise Vulnerability Tiers (Miller & Nicely 1955 / Master Plan 1.2).
 */
enum class NoiseVulnerabilityTier : uint8_t {
    TIER_0_CLEAN                      = 0, ///< SNR >= 18 dB: Uncorrupted articulatory stream
    TIER_1_MODERATE_PLACE_FLIP        = 1, ///< 10 dB <= SNR < 18 dB: Place-of-Articulation bit errors
    TIER_2_HEAVY_MANNER_FLIP          = 2, ///< 3 dB <= SNR < 10 dB: Place + Manner bit errors
    TIER_3_EXTREME_VOICING_VOWEL_MASK = 3, ///< -5 dB <= SNR < 3 dB: Voicing/Vowel erasure, prosody only
    TIER_4_UNINTELLIGIBLE_DROPOUT     = 4  ///< SNR < -5 dB: Signal buried below noise floor
};

/**
 * Bitmasks partitioning the 16-bit PanPhon `ArticulatoryFeatureBits` into
 * psycholinguistic vulnerability tiers for `NoisyChannelTransceiver`.
 */
namespace PhoneticVulnerabilityMasks {
    /// Tier 4 (Most vulnerable — flips first under moderate noise): Place of Articulation
    inline constexpr uint16_t PLACE_BITS_MASK = static_cast<uint16_t>(FEAT_TIER4_PLACE_MASK);

    /// Tier 3 (Flips under heavy noise): Manner of Articulation
    inline constexpr uint16_t MANNER_BITS_MASK = static_cast<uint16_t>(FEAT_TIER3_MANNER_MASK);

    /// Tier 1 & 2 (Most resilient — masked only under extreme noise): Prosodic & Vowel features
    inline constexpr uint16_t VOICING_AND_VOWEL_MASK =
        static_cast<uint16_t>(FEAT_TIER1_PROSODIC_MASK | FEAT_TIER2_VOWEL_MASK);
} // namespace PhoneticVulnerabilityMasks

/**
 * TransmitterStatePOD (32 Bytes, alignas(16))
 * Represents a Speaker or Signal Emitter in `IdeamCommunications`.
 */
struct alignas(16) TransmitterStatePOD {
    uint32_t transmitter_id          = 0;
    uint32_t mind_agent_id           = 0xFFFFFFFFu;
    float    elocution_encode_skill  = 1.0f;  ///< RPG "Speak Clearly" skill multiplier
    float    projection_power_db     = 65.0f; ///< RPG "Project Voice" source dB
    float    pos_x                   = 0.0f;
    float    pos_y                   = 0.0f;
    uint16_t active_lect_id          = 0;
    uint16_t prosody_spline_id       = 0;
    float    pos_z                   = 0.0f;  ///< 3D elevation coordinate (0.0 in 2D)
};
static_assert(sizeof(TransmitterStatePOD) == 32, "TransmitterStatePOD must be 32 bytes.");

/**
 * TransmissionMediumPOD (32 Bytes, alignas(16))
 * Represents the physical or logical Medium and its Noise characteristics.
 */
struct alignas(16) TransmissionMediumPOD {
    uint32_t medium_id                      = 0;
    float    ambient_noise_db               = 50.0f; ///< Crowd party chatter / rain / static floor
    float    attenuation_db_per_m           = 0.6f;  ///< Distance decay rate (dB per meter)
    float    occlusion_damping_db           = 0.0f;  ///< Wall / door / bulkhead muffling in dB
    float    place_flip_sensitivity         = 1.0f;  ///< Multiplier for Place-of-Articulation bit flips
    float    manner_flip_sensitivity        = 0.55f; ///< Multiplier for Manner/Continuant bit flips
    float    voicing_flip_sensitivity       = 0.20f; ///< Multiplier for Voicing/Vowel bit flips
    CommunicationModality modality          = CommunicationModality::SPOKEN_ACOUSTIC;
    uint8_t  reverb_intelligibility_penalty = 0;     ///< 0..255 reverberation smear factor
    uint8_t  reserved[2]                    = {0};
};
static_assert(sizeof(TransmissionMediumPOD) == 32, "TransmissionMediumPOD must be 32 bytes.");

/**
 * ReceiverStatePOD (32 Bytes, alignas(16))
 * Represents a Listener or Signal Receiver (player or NPC).
 */
struct alignas(16) ReceiverStatePOD {
    uint32_t receiver_id             = 0;
    uint32_t mind_agent_id           = 0xFFFFFFFFu;
    float    listen_decode_skill     = 1.0f;  ///< RPG "Listen" skill multiplier
    float    perception_bonus_db     = 0.0f;  ///< RPG "Perception" attribute SNR bonus in dB
    float    pos_x                   = 0.0f;
    float    pos_y                   = 0.0f;
    uint32_t expected_sememe_id      = 0xFFFFFFFFu; ///< Prior expectation from AgentMindResource
    float    pos_z                   = 0.0f;  ///< 3D elevation coordinate (0.0 in 2D)
};
static_assert(sizeof(ReceiverStatePOD) == 32, "ReceiverStatePOD must be 32 bytes.");

/**
 * ProsodySamplePOD (16 Bytes, alignas(16))
 * Sampled prosodic envelope point from a `Spline2DResource` (`X = Volume, Y = Pitch`).
 */
struct alignas(16) ProsodySamplePOD {
    float volume_gain      = 1.0f; ///< Normalized or dB-scaled volume multiplier (`Spline2D.X`)
    float pitch_multiplier = 1.0f; ///< Pitch contour multiplier (`Spline2D.Y`, 1.0 = neutral)
    float speaking_rate    = 1.0f; ///< Relative syllable rate multiplier
    float normalized_time  = 0.0f; ///< Position `[0.0 .. 1.0]` within the Turn Construction Unit
};
static_assert(sizeof(ProsodySamplePOD) == 16, "ProsodySamplePOD must be 16 bytes.");

/**
 * DiscourseParticipantPOD (32 Bytes, alignas(16))
 * Represents an interlocutor in a Sacks-Schegloff-Jefferson `DiscourseResource` session.
 */
struct alignas(16) DiscourseParticipantPOD {
    uint32_t participant_id      = 0;
    uint32_t mind_agent_id       = 0xFFFFFFFFu;
    uint32_t transmitter_id      = 0;
    uint32_t receiver_id         = 0;
    float    excitation_urgency  = 0.0f; ///< From `AgentMindResource::excitation_level_` (`[0..1]`)
    float    turn_claim_priority = 0.0f; ///< Composite self-selection weight for Rule 1b
    uint32_t addressed_target_id = 0xFFFFFFFFu; ///< Explicit next-speaker target for Rule 1a
    uint16_t active_lect_id      = 0;
    uint8_t  is_speaking         = 0;
    uint8_t  wants_floor         = 0;
};
static_assert(sizeof(DiscourseParticipantPOD) == 32, "DiscourseParticipantPOD must be 32 bytes.");

/**
 * DiscourseTurnStatePOD (32 Bytes, alignas(16))
 * Tracks the active turn and Transition Relevance Place (TRP) resolution state.
 */
struct alignas(16) DiscourseTurnStatePOD {
    uint32_t discourse_id                    = 0;
    uint32_t current_speaker_id              = 0xFFFFFFFFu;
    uint32_t next_selected_speaker_id        = 0xFFFFFFFFu;
    uint32_t current_turn_index              = 0;
    uint16_t active_tcu_index                = 0;
    uint16_t tcu_count                       = 1;
    float    prosody_pitch_delta             = 0.0f; ///< Negative/falling or rising terminal pitch contour
    float    interruption_margin_db          = 0.0f; ///< Competing speaker dB excess over current speaker
    TurnConstructionUnitType tcu_type        = TurnConstructionUnitType::SENTENTIAL;
    TurnResolutionType resolution            = TurnResolutionType::UNRESOLVED;
    DiscourseAllocationOutcome applied_rule  = DiscourseAllocationOutcome::OUTCOME_NONE;
    uint8_t  trp_reached                     = 0;
};
static_assert(sizeof(DiscourseTurnStatePOD) == 32, "DiscourseTurnStatePOD must be 32 bytes.");

/**
 * EpistemicHandoffPOD (32 Bytes, alignas(16))
 * Output record bridging `IdeamCommunications` Receiver decoding into `IdeamMinds`
 * (`AgentMindResource` Reference Catalogue & Akashic Tome `HONEST_MISREFERENCE` tracking).
 */
struct alignas(16) EpistemicHandoffPOD {
    uint32_t packet_id              = 0;
    uint32_t receiver_mind_agent_id = 0xFFFFFFFFu;
    uint32_t source_fact_id         = 0xFFFFFFFFu; ///< True Akashic Tome fact ID intended by Speaker
    uint32_t decoded_fact_id        = 0xFFFFFFFFu; ///< Fact ID decoded by Listener after noise degradation
    uint32_t decoded_sememe_id      = 0xFFFFFFFFu; ///< Best-matching `SememeDescriptorPOD` ID
    float    effective_snr_db       = 20.0f;
    float    decoded_confidence     = 1.0f;        ///< `[0.0 .. 1.0]` inductive confidence scaled by SNR
    uint16_t tome_id                = 0;           ///< Target Akashic Tome ID (`TOME_PHYSICAL_REALITY = 0`)
    uint8_t  honest_misreference    = 0;           ///< `1` when phonetic bit-flip caused `decoded != source`
    uint8_t  garbled_syllable_count = 0;
};
static_assert(sizeof(EpistemicHandoffPOD) == 32, "EpistemicHandoffPOD must be 32 bytes.");

/**
 * SignalPacketPOD (64 Bytes, alignas(64))
 * A single in-flight utterance or communication packet traveling from Transmitter
 * through Medium + Noise to Receiver.
 */
struct alignas(64) SignalPacketPOD {
    uint32_t packet_id                = 0;
    uint32_t transmitter_id           = 0;
    uint32_t receiver_id              = 0;
    uint32_t medium_id                = 0;
    uint32_t source_fact_id           = 0xFFFFFFFFu; ///< Akashic Tome fact ID intended by Speaker
    uint32_t decoded_fact_id          = 0xFFFFFFFFu; ///< Fact ID decoded by Listener (may misreference!)
    uint32_t phoneme_csr_offset       = 0;
    uint32_t phoneme_count            = 0;
    float    effective_snr_db         = 20.0f;
    float    percent_transmitted      = 0.0f;
    float    current_volume           = 1.0f;
    float    current_pitch            = 1.0f;
    TurnConstructionUnitType tcu_type = TurnConstructionUnitType::SENTENTIAL;
    TurnResolutionType resolution     = TurnResolutionType::UNRESOLVED;
    uint8_t  garbled_syllable_count   = 0;
    uint8_t  honest_misreference      = 0;           ///< `1` if phonetic degradation shifted decoded meaning
    uint32_t intended_sememe_id       = 0xFFFFFFFFu; ///< Speaker's intended primary `SememeID`
    uint32_t decoded_sememe_id        = 0xFFFFFFFFu; ///< Listener's decoded `SememeID`
    uint16_t active_lect_id           = 0;           ///< Dialect/sociolect ID of utterance
    uint8_t  noise_tier               = 0;           ///< `NoiseVulnerabilityTier` (`0..4`)
    uint8_t  reserved[1]              = {0};

    [[nodiscard]] constexpr bool is_audible(float p_min_snr_db = -5.0f) const noexcept {
        return effective_snr_db >= p_min_snr_db;
    }

    [[nodiscard]] constexpr bool is_honest_misreference() const noexcept {
        return honest_misreference != 0u;
    }
};
static_assert(sizeof(SignalPacketPOD) == 64, "SignalPacketPOD must be 64 bytes.");

} // namespace ideam::core
