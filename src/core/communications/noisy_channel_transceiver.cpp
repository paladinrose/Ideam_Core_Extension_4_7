#include "noisy_channel_transceiver.h"
#include <cmath>

namespace ideam::core {

float NoisyChannelTransceiver::compute_effective_snr_db(
    const TransmitterStatePOD &p_tx,
    const TransmissionMediumPOD &p_medium,
    const ReceiverStatePOD &p_rx
) noexcept {
    const float dx = p_rx.pos_x - p_tx.pos_x;
    const float dy = p_rx.pos_y - p_tx.pos_y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    const float tx_gain = p_tx.projection_power_db + 6.0f * (p_tx.elocution_encode_skill - 1.0f);
    const float rx_gain = p_rx.perception_bonus_db + 6.0f * (p_rx.listen_decode_skill - 1.0f);
    const float path_loss = dist * p_medium.attenuation_db_per_m + p_medium.occlusion_damping_db;
    return (tx_gain + rx_gain) - (p_medium.ambient_noise_db + path_loss);
}

void NoisyChannelTransceiver::degrade_phoneme_features_in_medium(
    const TransmissionMediumPOD &p_medium,
    float p_effective_snr_db,
    uint64_t p_random_seed,
    std::span<uint16_t> r_phoneme_feature_masks
) noexcept {
    (void)p_medium;
    (void)p_effective_snr_db;
    (void)p_random_seed;
    (void)r_phoneme_feature_masks;
    // Architectural stub: Tiered distinctive-feature bit-flipping (Place -> Manner -> Voicing)
    // driven by p_effective_snr_db and SeedStreamPOD(p_random_seed) will be authored here.
}

} // namespace ideam::core
