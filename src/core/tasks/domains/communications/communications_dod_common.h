#pragma once

/**
 * communications_dod_common.h
 * Foundational pure C++26 DOD buffer slots, stage flags, capacities, and table-driven
 * manifest specifications for `IdeamCommunications` (`src/core/tasks/domains/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (`IdeamCommunications` DOD Substrate)
 * ============================================================================
 * Implements the 4-Wave Shannon-Weaver & Sacks-Schegloff-Jefferson DOD pipeline:
 *   - Wave 1 (`COMM_STAGE_WAVE_1_DISCOURSE_TURN_ALLOCATION`):
 *       * `COM1` `DiscourseTurnAllocationMetadataLogic`: Evaluates Sacks-Schegloff-Jefferson
 *         Turn Construction Units (`LEXICAL`, `PHRASAL`, `CLAUSAL`, `SENTENTIAL`),
 *         prosody/interruption TRPs, and Rules 1a/1b/1c, stamping turn priority tiers
 *         into `r_selection.lod_levels`.
 *   - Wave 2 (`COMM_STAGE_WAVE_2_CHANNEL_SNR_AUDIBILITY_QUERY`):
 *       * `COM2` `ChannelSNRAudibilityQueryLogic`: Combines `TransmitterStatePOD`
 *         (`projection_power_db`, `elocution_encode_skill`), `TransmissionMediumPOD`
 *         (`ambient_noise_db`, `attenuation_db_per_m`, `occlusion_damping_db`), and
 *         `ReceiverStatePOD` (`perception_bonus_db`, `listen_decode_skill`) to compute
 *         `effective_snr_db` and `NoiseVulnerabilityTier` per `SignalPacketPOD`, culling
 *         inaudible packets or populating `r_selection.data.indices`.
 *   - Wave 3 (`COMM_STAGE_WAVE_3_NOISY_MEDIUM_PHONEME_DEGRADE`):
 *       * `COM3` `NoisyMediumPhonemeDegradeTransformLogic`: Flips bits in `uint16_t`
 *         PanPhon articulatory feature streams (`COMM_PHONEME_FEATURE_STREAM`) according
 *         to the psycholinguistic vulnerability hierarchy (`Place -> Manner -> Voicing`),
 *         updating `garbled_syllable_count` and applying `ProsodySamplePOD` envelopes.
 *   - Wave 4 (`COMM_STAGE_WAVE_4_RECEIVER_DECODE_AND_MIND_HANDOFF`):
 *       * `COM4` `ReceiverEpistemicHandoffTransformLogic`: Decodes the degraded phoneme
 *         stream via `SemanticLexiconCore` Needleman-Wunsch DTW (or `COMM_DECODED_SEMEME_STREAM`
 *         from `IdeamLinguistics::LNG5`), populates `EpistemicHandoffPOD`, and flags
 *         `honest_misreference = 1` (`FACT_FLAG_MISREFERENCED`) when a phonetic flip
 *         shifts the decoded word/fact for `IdeamMinds` (`MindDOD`).
 */

#include "../domain_dod_common.h"
#include "../../../communications/noisy_channel_transceiver.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

// ============================================================================
// 1. Pipeline Stage Bitflags & Execution Configuration
// ============================================================================

enum CommunicationsPipelineStageFlags : uint32_t {
    COMM_STAGE_NONE                                    = 0u,
    COMM_STAGE_WAVE_1_DISCOURSE_TURN_ALLOCATION        = 1u << 0, ///< COM1: Sacks-Schegloff-Jefferson turn allocation
    COMM_STAGE_WAVE_2_CHANNEL_SNR_AUDIBILITY_QUERY     = 1u << 1, ///< COM2: Transmitter + Medium + Receiver SNR & audibility cull
    COMM_STAGE_WAVE_3_NOISY_MEDIUM_PHONEME_DEGRADE     = 1u << 2, ///< COM3: Tiered Place/Manner/Voicing articulatory bit-flipping
    COMM_STAGE_WAVE_4_RECEIVER_DECODE_AND_MIND_HANDOFF = 1u << 3, ///< COM4: Phonetic DTW decode & EpistemicHandoffPOD (`HONEST_MISREFERENCE`)

    COMM_STAGE_DEFAULT_PIPELINE_MASK =
        COMM_STAGE_WAVE_1_DISCOURSE_TURN_ALLOCATION |
        COMM_STAGE_WAVE_2_CHANNEL_SNR_AUDIBILITY_QUERY |
        COMM_STAGE_WAVE_3_NOISY_MEDIUM_PHONEME_DEGRADE |
        COMM_STAGE_WAVE_4_RECEIVER_DECODE_AND_MIND_HANDOFF,

    COMM_STAGE_ALL_MASK = COMM_STAGE_DEFAULT_PIPELINE_MASK
};

using CommunicationsNodeExecutionConfig = DomainNodeExecutionConfig<COMM_STAGE_DEFAULT_PIPELINE_MASK>;
using CommunicationsStageAnchor         = DomainStageAnchor;
using CommunicationsPipelineTopology    = DomainPipelineTopology<4>;

// ============================================================================
// 2. Buffer Capacities & Table-Driven Buffer Slot Manifest
// ============================================================================

enum class CommunicationsCapacityKind : uint8_t {
    MAX_SIGNAL_PACKETS = 0,
    MAX_PHONEME_STREAM_LEN,
    MAX_TRANSMITTERS,
    MAX_MEDIUMS,
    MAX_RECEIVERS,
    MAX_DISCOURSE_PARTICIPANTS,
    MAX_DISCOURSE_TURNS,
    COUNT
};

enum class CommunicationsTableGroup : uint8_t {
    SIGNAL_AND_PHONEME_STREAM = 0,
    TRANSCEIVER_AND_MEDIUM_STATE,
    DISCOURSE_TURN_STATE,
    EPISTEMIC_MIND_HANDOFF,
    COUNT
};

struct CommunicationsBufferCapacities {
    int64_t max_signal_packets         = 512;
    int64_t max_phoneme_stream_len     = 16384;
    int64_t max_transmitters           = 256;
    int64_t max_mediums                = 64;
    int64_t max_receivers              = 256;
    int64_t max_discourse_participants = 256;
    int64_t max_discourse_turns        = 128;

    [[nodiscard]] constexpr int64_t resolve(CommunicationsCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case CommunicationsCapacityKind::MAX_SIGNAL_PACKETS:         return max_signal_packets;
            case CommunicationsCapacityKind::MAX_PHONEME_STREAM_LEN:     return max_phoneme_stream_len;
            case CommunicationsCapacityKind::MAX_TRANSMITTERS:           return max_transmitters;
            case CommunicationsCapacityKind::MAX_MEDIUMS:                return max_mediums;
            case CommunicationsCapacityKind::MAX_RECEIVERS:              return max_receivers;
            case CommunicationsCapacityKind::MAX_DISCOURSE_PARTICIPANTS: return max_discourse_participants;
            case CommunicationsCapacityKind::MAX_DISCOURSE_TURNS:        return max_discourse_turns;
            default:                                                     return 256;
        }
    }
};

/**
 * CommunicationsBufferSlot
 * Preserves legacy enum aliases (`PRIMARY_DESCRIPTOR_POD`..`DECODED_SEMEME_STREAM`)
 * while providing explicit typed slots for the full 4-wave communications pipeline.
 */
enum class CommunicationsBufferSlot : uint8_t {
    COMM_SIGNAL_PACKET_POD        = 0, ///< `SignalPacketPOD` (64B)
    COMM_PHONEME_CSR_OFFSETS      = 1, ///< `uint32_t` CSR word/packet phoneme offsets
    COMM_PHONEME_FEATURE_STREAM   = 2, ///< `uint16_t` 16-bit PanPhon articulatory masks (in-flight / degraded)
    COMM_DECODED_SEMEME_STREAM    = 3, ///< `uint32_t` decoded `SememeID` stream
    COMM_TRANSMITTER_STATE_POD    = 4, ///< `TransmitterStatePOD` (32B)
    COMM_TRANSMISSION_MEDIUM_POD  = 5, ///< `TransmissionMediumPOD` (32B)
    COMM_RECEIVER_STATE_POD       = 6, ///< `ReceiverStatePOD` (32B)
    COMM_DISCOURSE_PARTICIPANT_POD= 7, ///< `DiscourseParticipantPOD` (32B)
    COMM_DISCOURSE_TURN_STATE_POD = 8, ///< `DiscourseTurnStatePOD` (32B)
    COMM_PROSODY_SAMPLE_POD       = 9, ///< `ProsodySamplePOD` (16B)
    COMM_EFFECTIVE_SNR_DB         = 10,///< `float` SOA effective SNR (dB) per packet
    COMM_NOISE_TIER_MASK          = 11,///< `uint32_t` SOA `NoiseVulnerabilityTier` per packet
    COMM_EPISTEMIC_HANDOFF_POD    = 12,///< `EpistemicHandoffPOD` (32B) for `IdeamMinds`
    COMM_LEXEME_POD_TABLE         = 13,///< `LexemePOD` (32B) reference lexicon table
    COMM_LEXEME_PHONEME_CSR_MASKS = 14,///< `uint16_t` reference lexicon CSR phoneme masks
    COUNT                         = 15,

    // Legacy scaffold aliases
    PRIMARY_DESCRIPTOR_POD = COMM_SIGNAL_PACKET_POD,
    SECONDARY_SLICE_CSR    = COMM_PHONEME_CSR_OFFSETS,
    FEATURE_BITMASK_STREAM = COMM_PHONEME_FEATURE_STREAM,
    DECODED_SEMEME_STREAM  = COMM_DECODED_SEMEME_STREAM
};

struct CommunicationsBufferSpec {
    CommunicationsBufferSlot   slot;
    std::string_view           buffer_name;
    std::string_view           group_name;
    DataType                   data_type;
    size_t                     element_bytes;
    CommunicationsCapacityKind capacity_kind;
    CommunicationsTableGroup   table_group;
};

inline constexpr std::array<CommunicationsBufferSpec, static_cast<size_t>(CommunicationsBufferSlot::COUNT)>
COMMUNICATIONS_BUFFER_SPECS = {{
    { CommunicationsBufferSlot::COMM_SIGNAL_PACKET_POD,         "Comm_SignalPacketPOD",        "Comm_SignalStream",      DataType::BYTE,    sizeof(SignalPacketPOD),         CommunicationsCapacityKind::MAX_SIGNAL_PACKETS,         CommunicationsTableGroup::SIGNAL_AND_PHONEME_STREAM },
    { CommunicationsBufferSlot::COMM_PHONEME_CSR_OFFSETS,       "Comm_PhonemeCSROffsets",      "Comm_SignalStream",      DataType::INT32,   sizeof(uint32_t),                CommunicationsCapacityKind::MAX_SIGNAL_PACKETS,         CommunicationsTableGroup::SIGNAL_AND_PHONEME_STREAM },
    { CommunicationsBufferSlot::COMM_PHONEME_FEATURE_STREAM,    "Comm_PhonemeFeatureStream",   "Comm_SignalStream",      DataType::BYTE,    sizeof(uint16_t),                CommunicationsCapacityKind::MAX_PHONEME_STREAM_LEN,     CommunicationsTableGroup::SIGNAL_AND_PHONEME_STREAM },
    { CommunicationsBufferSlot::COMM_DECODED_SEMEME_STREAM,     "Comm_DecodedSememeStream",    "Comm_SignalStream",      DataType::INT32,   sizeof(uint32_t),                CommunicationsCapacityKind::MAX_SIGNAL_PACKETS,         CommunicationsTableGroup::SIGNAL_AND_PHONEME_STREAM },
    { CommunicationsBufferSlot::COMM_TRANSMITTER_STATE_POD,     "Comm_TransmitterStatePOD",    "Comm_TransceiverState",  DataType::BYTE,    sizeof(TransmitterStatePOD),     CommunicationsCapacityKind::MAX_TRANSMITTERS,           CommunicationsTableGroup::TRANSCEIVER_AND_MEDIUM_STATE },
    { CommunicationsBufferSlot::COMM_TRANSMISSION_MEDIUM_POD,   "Comm_TransmissionMediumPOD",  "Comm_TransceiverState",  DataType::BYTE,    sizeof(TransmissionMediumPOD),   CommunicationsCapacityKind::MAX_MEDIUMS,                CommunicationsTableGroup::TRANSCEIVER_AND_MEDIUM_STATE },
    { CommunicationsBufferSlot::COMM_RECEIVER_STATE_POD,        "Comm_ReceiverStatePOD",       "Comm_TransceiverState",  DataType::BYTE,    sizeof(ReceiverStatePOD),        CommunicationsCapacityKind::MAX_RECEIVERS,              CommunicationsTableGroup::TRANSCEIVER_AND_MEDIUM_STATE },
    { CommunicationsBufferSlot::COMM_DISCOURSE_PARTICIPANT_POD, "Comm_DiscourseParticipantPOD","Comm_DiscourseState",    DataType::BYTE,    sizeof(DiscourseParticipantPOD), CommunicationsCapacityKind::MAX_DISCOURSE_PARTICIPANTS, CommunicationsTableGroup::DISCOURSE_TURN_STATE },
    { CommunicationsBufferSlot::COMM_DISCOURSE_TURN_STATE_POD,  "Comm_DiscourseTurnStatePOD",  "Comm_DiscourseState",    DataType::BYTE,    sizeof(DiscourseTurnStatePOD),   CommunicationsCapacityKind::MAX_DISCOURSE_TURNS,        CommunicationsTableGroup::DISCOURSE_TURN_STATE },
    { CommunicationsBufferSlot::COMM_PROSODY_SAMPLE_POD,        "Comm_ProsodySamplePOD",       "Comm_SignalStream",      DataType::BYTE,    sizeof(ProsodySamplePOD),        CommunicationsCapacityKind::MAX_SIGNAL_PACKETS,         CommunicationsTableGroup::SIGNAL_AND_PHONEME_STREAM },
    { CommunicationsBufferSlot::COMM_EFFECTIVE_SNR_DB,          "Comm_EffectiveSNR_dB",        "Comm_TransceiverState",  DataType::FLOAT32, sizeof(float),                   CommunicationsCapacityKind::MAX_SIGNAL_PACKETS,         CommunicationsTableGroup::TRANSCEIVER_AND_MEDIUM_STATE },
    { CommunicationsBufferSlot::COMM_NOISE_TIER_MASK,           "Comm_NoiseTierMask",          "Comm_TransceiverState",  DataType::INT32,   sizeof(uint32_t),                CommunicationsCapacityKind::MAX_SIGNAL_PACKETS,         CommunicationsTableGroup::TRANSCEIVER_AND_MEDIUM_STATE },
    { CommunicationsBufferSlot::COMM_EPISTEMIC_HANDOFF_POD,     "Comm_EpistemicHandoffPOD",    "Comm_EpistemicHandoff",  DataType::BYTE,    sizeof(EpistemicHandoffPOD),     CommunicationsCapacityKind::MAX_SIGNAL_PACKETS,         CommunicationsTableGroup::EPISTEMIC_MIND_HANDOFF },
    { CommunicationsBufferSlot::COMM_LEXEME_POD_TABLE,          "Comm_LexemePODTable",         "Comm_EpistemicHandoff",  DataType::BYTE,    sizeof(LexemePOD),               CommunicationsCapacityKind::MAX_SIGNAL_PACKETS,         CommunicationsTableGroup::EPISTEMIC_MIND_HANDOFF },
    { CommunicationsBufferSlot::COMM_LEXEME_PHONEME_CSR_MASKS,  "Comm_LexemePhonemeCSRMasks",  "Comm_EpistemicHandoff",  DataType::BYTE,    sizeof(uint16_t),                CommunicationsCapacityKind::MAX_PHONEME_STREAM_LEN,     CommunicationsTableGroup::EPISTEMIC_MIND_HANDOFF }
}};

consteval bool validate_communications_buffer_specs() {
    for (size_t i = 0; i < COMMUNICATIONS_BUFFER_SPECS.size(); ++i) {
        if (static_cast<size_t>(COMMUNICATIONS_BUFFER_SPECS[i].slot) != i) return false;
        if (COMMUNICATIONS_BUFFER_SPECS[i].element_bytes == 0) return false;
    }
    return true;
}
static_assert(validate_communications_buffer_specs(), "COMMUNICATIONS_BUFFER_SPECS must align 1:1 with CommunicationsBufferSlot!");

using CommunicationsBufferManifest = DomainBufferManifest<CommunicationsBufferSlot>;

} // namespace ideam::core
