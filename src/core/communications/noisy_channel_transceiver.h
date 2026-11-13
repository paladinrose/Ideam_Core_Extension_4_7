#pragma once

/**
 * noisy_channel_transceiver.h
 * Pure C++26 Shannon-Weaver Noisy-Channel Phonetic & Semantic Transceiver
 * (`src/core/communications/`).
 */

#include "communications_common.h"
#include "../linguistics/semantic_lexicon_core.h"
#include <span>

namespace ideam::core {

class NoisyChannelTransceiver {
public:
    NoisyChannelTransceiver() = default;
    ~NoisyChannelTransceiver() = default;

    [[nodiscard]] static float compute_effective_snr_db(
        const TransmitterStatePOD &p_tx,
        const TransmissionMediumPOD &p_medium,
        const ReceiverStatePOD &p_rx
    ) noexcept;

    static void degrade_phoneme_features_in_medium(
        const TransmissionMediumPOD &p_medium,
        float p_effective_snr_db,
        uint64_t p_random_seed,
        std::span<uint16_t> r_phoneme_feature_masks
    ) noexcept;
};

} // namespace ideam::core
