#pragma once

/**
 * channel_snr_audibility_query_logic.h
 * `COM2`: 64-byte cache-aligned `QueryLogic` kernel for `IdeamCommunications`
 * (`src/core/tasks/domains/communications/query_logic/`).
 *
 * Evaluates Stage 2 of the Shannon-Weaver pipeline across `SignalPacketPOD`
 * buffers by combining:
 *   - `TransmitterStatePOD` (`projection_power_db`, `elocution_encode_skill`, 3D pos),
 *   - `TransmissionMediumPOD` (`ambient_noise_db`, `attenuation_db_per_m`, `occlusion_damping_db`),
 *   - `ReceiverStatePOD` (`perception_bonus_db`, `listen_decode_skill`, 3D pos),
 * writing `effective_snr_db` and `noise_tier` (`NoiseVulnerabilityTier`) into
 * each `SignalPacketPOD` (and optional SOA buffers `COMM_EFFECTIVE_SNR_DB` /
 * `COMM_NOISE_TIER_MASK`) and selecting audible packets (`effective_snr_db >= min_audible_snr_db`)
 * into `r_selection`.
 */

#include "../../../../communications/noisy_channel_transceiver.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) ChannelSNRAudibilityQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "COM2: Channel SNR Audibility & Perception Query";
    static constexpr std::string_view ui_category  = "Communications/Channel";

    // --- 64-Byte Cache-Line Member Layout ---
    uint32_t target_buffer_id              = INVALID_ID; ///< Offset 0  (4B): `COMM_SIGNAL_PACKET_POD`
    uint32_t transmitter_state_buffer_id   = INVALID_ID; ///< Offset 4  (4B): `COMM_TRANSMITTER_STATE_POD`
    uint32_t transmission_medium_buffer_id = INVALID_ID; ///< Offset 8  (4B): `COMM_TRANSMISSION_MEDIUM_POD`
    uint32_t receiver_state_buffer_id      = INVALID_ID; ///< Offset 12 (4B): `COMM_RECEIVER_STATE_POD`
    uint32_t effective_snr_out_buffer_id   = INVALID_ID; ///< Offset 16 (4B): Optional `COMM_EFFECTIVE_SNR_DB`
    uint32_t noise_tier_out_buffer_id      = INVALID_ID; ///< Offset 20 (4B): Optional `COMM_NOISE_TIER_MASK`
    float    min_audible_snr_db            = -5.0f;      ///< Offset 24 (4B): Minimum SNR (dB) to pass audibility query
    uint8_t  reserved_padding[36]          = {0};        ///< Offset 28 (36B) -> Total = 64 Bytes

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",              godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("transmitter_state_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("transmission_medium_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("receiver_state_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("effective_snr_out_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("noise_tier_out_buffer_id",      godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("min_audible_snr_db",            godot::Variant::FLOAT, -5.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))              target_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("transmitter_state_buffer_id"))   transmitter_state_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["transmitter_state_buffer_id"]));
        if (p_props.has("transmission_medium_buffer_id")) transmission_medium_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transmission_medium_buffer_id"]));
        if (p_props.has("receiver_state_buffer_id"))      receiver_state_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["receiver_state_buffer_id"]));
        if (p_props.has("effective_snr_out_buffer_id"))   effective_snr_out_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["effective_snr_out_buffer_id"]));
        if (p_props.has("noise_tier_out_buffer_id"))      noise_tier_out_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["noise_tier_out_buffer_id"]));
        if (p_props.has("min_audible_snr_db"))            min_audible_snr_db            = static_cast<float>(static_cast<double>(p_props["min_audible_snr_db"]));
        if (p_props.has("threshold"))                     min_audible_snr_db            = static_cast<float>(static_cast<double>(p_props["threshold"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_Elem>
    [[nodiscard]] static T_Elem *_resolve_ptr(const TaskContextPOD &p_ctx, uint32_t p_buf_id, size_t &r_cap) noexcept {
        r_cap = 0u;
        if (p_buf_id == INVALID_ID) return nullptr;
        if (const GrantPartPOD *part = p_ctx.get_grant_part(p_buf_id)) {
            if (part->raw_base_ptr) {
                const size_t cap = part->capacity_bytes / sizeof(T_Elem);
                r_cap = (part->selection.element_count > 0)
                    ? std::min<size_t>(static_cast<size_t>(part->selection.element_count), cap)
                    : cap;
                return reinterpret_cast<T_Elem *>(part->raw_base_ptr);
            }
        }
        if (p_ctx.manager) {
            if (MemoryBufferPOD *buf = p_ctx.manager->get_buffer(p_buf_id)) {
                if (buf->master_block_ptr) {
                    const size_t cap = static_cast<size_t>(std::max<int64_t>(0, buf->max_elements));
                    r_cap = (buf->current_count > 0)
                        ? std::min<size_t>(static_cast<size_t>(buf->current_count), cap)
                        : cap;
                    return reinterpret_cast<T_Elem *>(buf->master_block_ptr + buf->memory_offset);
                }
            }
        }
        return nullptr;
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t pkt_cap = 0, tx_cap = 0, med_cap = 0, rx_cap = 0, snr_cap = 0, tier_cap = 0;
        auto *packets    = _resolve_ptr<SignalPacketPOD>(p_ctx, target_buffer_id, pkt_cap);
        const auto *txs  = _resolve_ptr<const TransmitterStatePOD>(p_ctx, transmitter_state_buffer_id, tx_cap);
        const auto *meds = _resolve_ptr<const TransmissionMediumPOD>(p_ctx, transmission_medium_buffer_id, med_cap);
        const auto *rxs  = _resolve_ptr<const ReceiverStatePOD>(p_ctx, receiver_state_buffer_id, rx_cap);
        auto *snr_out    = _resolve_ptr<float>(p_ctx, effective_snr_out_buffer_id, snr_cap);
        auto *tier_out   = _resolve_ptr<uint32_t>(p_ctx, noise_tier_out_buffer_id, tier_cap);

        if (!packets || pkt_cap == 0) return;

        GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id));
        int64_t matched_count = 0;

        for (size_t i = 0; i < pkt_cap; ++i) {
            SignalPacketPOD &pkt = packets[i];
            const TransmitterStatePOD tx = (txs && pkt.transmitter_id < tx_cap)
                ? txs[pkt.transmitter_id]
                : ((txs && tx_cap > 0) ? txs[0] : TransmitterStatePOD{});
            const TransmissionMediumPOD med = (meds && pkt.medium_id < med_cap)
                ? meds[pkt.medium_id]
                : ((meds && med_cap > 0) ? meds[0] : TransmissionMediumPOD{});
            const ReceiverStatePOD rx = (rxs && pkt.receiver_id < rx_cap)
                ? rxs[pkt.receiver_id]
                : ((rxs && rx_cap > 0) ? rxs[0] : ReceiverStatePOD{});

            const float vol_gain = (pkt.current_volume > 0.0f) ? pkt.current_volume : 1.0f;
            const float snr_db   = NoisyChannelTransceiver::compute_effective_snr_db(tx, med, rx, vol_gain);
            const NoiseVulnerabilityTier tier = NoisyChannelTransceiver::classify_noise_tier(snr_db);

            pkt.effective_snr_db = snr_db;
            pkt.noise_tier       = static_cast<uint8_t>(tier);
            pkt.active_lect_id   = tx.active_lect_id;

            if (snr_out && i < snr_cap)   snr_out[i]  = snr_db;
            if (tier_out && i < tier_cap) tier_out[i] = static_cast<uint32_t>(tier);

            if (snr_db >= min_audible_snr_db) {
                if (target_part && target_part->selection.mode == SelectionMode::SPARSE && target_part->selection.data.indices) {
                    target_part->selection.data.indices[matched_count] = static_cast<int64_t>(i);
                }
                ++matched_count;
            }
        }

        if (target_part && target_part->selection.mode == SelectionMode::SPARSE) {
            target_part->selection.element_count = matched_count;
        }
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_ctx,
        const T_ActiveView &p_view
    ) const noexcept {
        (void)r_selection;
        T_ActiveView &mut_view = const_cast<T_ActiveView &>(p_view);
        execute<Op, T_ActiveView, T_ActiveStrategy>(p_ctx, mut_view);
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_query(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<Op, T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(ChannelSNRAudibilityQueryLogic<>) == 64, "ChannelSNRAudibilityQueryLogic must be 64 bytes!");
static_assert(IsQueryLogic<ChannelSNRAudibilityQueryLogic<>>, "ChannelSNRAudibilityQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core
