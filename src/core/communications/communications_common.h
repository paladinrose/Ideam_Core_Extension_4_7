#pragma once

/**
 * communications_common.h
 * Pure C++26 Shannon-Weaver Noisy-Channel Signal Transmission & Sacks-Schegloff-Jefferson
 * Discourse Turn-Taking primitives for `IdeamCommunications` (`src/core/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (`IdeamCommunications`)
 * ============================================================================
 * 1. SHANNON-WEAVER PROGRAMMATIC COMMUNICATION PIPELINE:
 *    - Breaks all in-game communication into 6 explicit, composable stages:
 *        a) **Encoder**: Converts `IdeamMinds` propositions (`SememeID` + `SememicForm`)
 *           into a sequence of `LexemePOD` -> `PhonemePOD` (or `GraphemePOD`) feature masks,
 *           modulated by the speaker's `encode_or_elocution_skill` ("Speak Clearly").
 *        b) **Transmitter**: Emits the encoded signal into the environment with a given
 *           `projection_power` ("Project Voice") and `Spline2D` prosody contour (X = Volume, Y = Pitch).
 *        c) **Medium**: Propagates the signal through space/material (air in a crowded tavern,
 *           stone wall occlusion, radio channel, written parchment), attenuating signal power.
 *        d) **Noise**: Injects structured interference (crowd babble, broadband rain, static)
 *           that flips bits in each `PhonemePOD`'s 16-bit `ArticulatoryFeatureBits` mask
 *           according to the psycholinguistic vulnerability hierarchy:
 *             - Low SNR first flips **Place of Articulation** (`FEAT_LABIAL`, `FEAT_CORONAL`, `FEAT_DORSAL`),
 *               causing `/p/`, `/t/`, `/k/` or `/f/`, `/s/` confusions.
 *             - Lower SNR next flips **Manner / Continuant** (`FEAT_CONTINUANT`, `FEAT_NASAL`).
 *             - Extreme noise finally masks **Voicing & Vowel Prosody**, reducing syllables to
 *               garbled ellipses (`"..."`).
 *        e) **Receiver**: Intercepts the degraded signal, applying the listener's RPG attributes
 *           (`listen_skill`, `perception_bonus`, directional attention focus) to boost effective SNR.
 *        f) **Decoder**: Hands the received `uint16_t` phoneme feature masks to `IdeamLinguistics`
 *           (`SemanticLexiconCore`) and `IdeamMinds` (`AgentMindResource`) to reconstruct the
 *           most likely words and `Sememe`s—resulting in speech-box dialogue that can be
 *           phonetically accurate yet misheard ("take it" -> "make it"), partially garbled,
 *           or recorded in the listener's Mind Catalogue as an **Honest Misreference**!
 *
 * 2. DISCOURSE GRAPH & TURN-TAKING (PORTING `Discourse.cs`):
 *    - Preserves the Sacks-Schegloff-Jefferson conversation analysis model from `Discourse.cs`:
 *      * `TurnConstructionUnitType`: `LEXICAL`, `PHRASAL`, `CLAUSAL`, `SENTENTIAL`.
 *      * `TurnResolutionType`: `UNRESOLVED`, `INTONATIONAL` (prosody pitch/volume drop),
 *        `GRAMMATICAL` (syntactic completion), `PRAGMATICAL` (situational interruption).
 */

#include "../linguistics/linguistics_common.h"
#include <cstdint>

namespace ideam::core {

enum class CommunicationModality : uint8_t {
    SPOKEN_ACOUSTIC        = 0,
    WRITTEN_INSCRIPTION    = 1,
    RADIO_ELECTROMAGNETIC  = 2,
    NON_VERBAL_GESTURE     = 3
};

enum class TurnConstructionUnitType : uint8_t {
    UNDEFINED  = 0,
    LEXICAL    = 1,
    PHRASAL    = 2,
    CLAUSAL    = 3,
    SENTENTIAL = 4
};

enum class TurnResolutionType : uint8_t {
    UNRESOLVED   = 0,
    INTONATIONAL = 1, // Resolved via prosody shift (Spline2D pitch/volume)
    GRAMMATICAL  = 2, // Resolved via syntactic completion
    PRAGMATICAL  = 3  // Resolved practically via situational interference/interruption
};

/**
 * TransmitterStatePOD (32 Bytes, alignas(16))
 * Represents a Speaker or Signal Emitter in `IdeamCommunications`.
 */
struct alignas(16) TransmitterStatePOD {
    uint32_t transmitter_id          = 0;
    uint32_t mind_agent_id           = 0xFFFFFFFFu;
    float    elocution_encode_skill  = 1.0f; // RPG "Speak Clearly" skill multiplier
    float    projection_power_db     = 65.0f;// RPG "Project Voice" source dB
    float    pos_x                   = 0.0f;
    float    pos_y                   = 0.0f;
    uint16_t active_lect_id          = 0;
    uint16_t prosody_spline_id       = 0;
    uint8_t  reserved[4]             = {0};
};
static_assert(sizeof(TransmitterStatePOD) == 32, "TransmitterStatePOD must be 32 bytes.");

/**
 * TransmissionMediumPOD (32 Bytes, alignas(16))
 * Represents the physical or logical Medium and its Noise characteristics.
 */
struct alignas(16) TransmissionMediumPOD {
    uint32_t medium_id               = 0;
    float    ambient_noise_db        = 50.0f; // Crowd party chatter / rain / static floor
    float    attenuation_db_per_m    = 0.6f;  // Distance decay rate
    float    occlusion_damping_db    = 0.0f;  // Wall / door muffling
    float    place_flip_sensitivity  = 1.0f;  // Multiplier for Place-of-Articulation bit flips
    float    manner_flip_sensitivity = 0.55f; // Multiplier for Manner/Continuant bit flips
    float    voicing_flip_sensitivity= 0.20f; // Multiplier for Voicing/Vowel bit flips
    CommunicationModality modality   = CommunicationModality::SPOKEN_ACOUSTIC;
    uint8_t  reserved[3]             = {0};
};
static_assert(sizeof(TransmissionMediumPOD) == 32, "TransmissionMediumPOD must be 32 bytes.");

/**
 * ReceiverStatePOD (32 Bytes, alignas(16))
 * Represents a Listener or Signal Receiver (player or NPC).
 */
struct alignas(16) ReceiverStatePOD {
    uint32_t receiver_id             = 0;
    uint32_t mind_agent_id           = 0xFFFFFFFFu;
    float    listen_decode_skill     = 1.0f;  // RPG "Listen" skill multiplier
    float    perception_bonus_db     = 0.0f;  // RPG "Perception" attribute SNR bonus in dB
    float    pos_x                   = 0.0f;
    float    pos_y                   = 0.0f;
    uint32_t expected_sememe_id      = 0xFFFFFFFFu; // Prior expectation from AgentMindResource
    uint8_t  reserved[4]             = {0};
};
static_assert(sizeof(ReceiverStatePOD) == 32, "ReceiverStatePOD must be 32 bytes.");

/**
 * SignalPacketPOD (64 Bytes, alignas(64))
 * A single in-flight utterance or communication packet traveling from Transmitter
 * through Medium + Noise to Receiver.
 */
struct alignas(64) SignalPacketPOD {
    uint32_t packet_id               = 0;
    uint32_t transmitter_id          = 0;
    uint32_t receiver_id             = 0;
    uint32_t medium_id               = 0;
    uint32_t source_fact_id          = 0xFFFFFFFFu; // Akashic Tome fact ID intended by Speaker
    uint32_t decoded_fact_id         = 0xFFFFFFFFu; // Fact ID decoded by Listener (may misreference!)
    uint32_t phoneme_csr_offset      = 0;
    uint32_t phoneme_count           = 0;
    float    effective_snr_db        = 20.0f;
    float    percent_transmitted     = 0.0f;
    float    current_volume          = 1.0f;
    float    current_pitch           = 1.0f;
    TurnConstructionUnitType tcu_type= TurnConstructionUnitType::SENTENTIAL;
    TurnResolutionType resolution    = TurnResolutionType::UNRESOLVED;
    uint8_t  garbled_syllable_count  = 0;
    uint8_t  reserved[13]            = {0};
};
static_assert(sizeof(SignalPacketPOD) == 64, "SignalPacketPOD must be 64 bytes.");

} // namespace ideam::core
