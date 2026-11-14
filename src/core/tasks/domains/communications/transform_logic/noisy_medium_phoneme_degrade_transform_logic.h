#pragma once

/**
 * noisy_medium_phoneme_degrade_transform_logic.h
 * `COM3`: 64-byte cache-aligned `TransformLogic` kernel for `IdeamCommunications`
 * (`src/core/tasks/domains/communications/transform_logic/`).
 *
 * Executes Stage 3 of the Shannon-Weaver pipeline: applies deterministic
 * psycholinguistic bit-error masks to each `SignalPacketPOD`'s slice in
 * `COMM_PHONEME_FEATURE_STREAM` according to `effective_snr_db` and the medium's
 * Place -> Manner -> Voicing/Vowel vulnerability sensitivities, while preserving
 * the `ProsodySamplePOD` (`current_volume`, `current_pitch`) envelope.
 */

#include "../../../../communications/noisy_channel_transceiver.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) NoisyMediumPhonemeDegradeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "COM3: Noisy Medium Articulatory Feature Degrade";
    static constexpr std::string_view ui_category  = "Communications/Noise";

    // --- 64-Byte Cache-Line Member Layout ---
    uint32_t target_buffer_id              = INVALID_ID;           ///< Offset 0  (4B): `COMM_PHONEME_FEATURE_STREAM` or `COMM_SIGNAL_PACKET_POD`
    uint32_t signal_packet_buffer_id       = INVALID_ID;           ///< Offset 4  (4B): `COMM_SIGNAL_PACKET_POD`
    uint32_t phoneme_feature_stream_id     = INVALID_ID;           ///< Offset 8  (4B): `COMM_PHONEME_FEATURE_STREAM`
    uint32_t transmission_medium_buffer_id = INVALID_ID;           ///< Offset 12 (4B): `COMM_TRANSMISSION_MEDIUM_POD`
    uint32_t prosody_sample_buffer_id      = INVALID_ID;           ///< Offset 16 (4B): Optional `COMM_PROSODY_SAMPLE_POD`
    uint32_t lexicon_buffer_id             = INVALID_ID;           ///< Offset 20 (4B): Legacy alias
    uint64_t random_seed                   = 0x9E3779B97F4A7C15ULL;///< Offset 24 (8B): Deterministic PRNG seed
    uint8_t  reserved_padding[32]          = {0};                  ///< Offset 32 (32B) -> Total = 64 Bytes

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",              godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("signal_packet_buffer_id",       godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("phoneme_feature_stream_id",     godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("transmission_medium_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("prosody_sample_buffer_id",      godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("random_seed",                   godot::Variant::INT, static_cast<int64_t>(1337));
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))              target_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("signal_packet_buffer_id"))       signal_packet_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["signal_packet_buffer_id"]));
        if (p_props.has("phoneme_buffer_id"))             phoneme_feature_stream_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["phoneme_buffer_id"]));
        if (p_props.has("phoneme_feature_stream_id"))     phoneme_feature_stream_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["phoneme_feature_stream_id"]));
        if (p_props.has("transmission_medium_buffer_id")) transmission_medium_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transmission_medium_buffer_id"]));
        if (p_props.has("prosody_sample_buffer_id"))      prosody_sample_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["prosody_sample_buffer_id"]));
        if (p_props.has("lexicon_buffer_id"))             lexicon_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["lexicon_buffer_id"]));
        if (p_props.has("random_seed"))                   random_seed                   = static_cast<uint64_t>(static_cast<int64_t>(p_props["random_seed"]));
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

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const uint32_t ph_id  = (phoneme_feature_stream_id != INVALID_ID) ? phoneme_feature_stream_id : target_buffer_id;
        const uint32_t pkt_id = (signal_packet_buffer_id != INVALID_ID) ? signal_packet_buffer_id : target_buffer_id;

        size_t pkt_cap = 0, ph_cap = 0, med_cap = 0, pros_cap = 0;
        auto *packets       = _resolve_ptr<SignalPacketPOD>(p_ctx, pkt_id, pkt_cap);
        auto *ph_stream     = _resolve_ptr<uint16_t>(p_ctx, ph_id, ph_cap);
        const auto *meds    = _resolve_ptr<const TransmissionMediumPOD>(p_ctx, transmission_medium_buffer_id, med_cap);
        const auto *prosody = _resolve_ptr<const ProsodySamplePOD>(p_ctx, prosody_sample_buffer_id, pros_cap);

        if (!packets || !ph_stream || pkt_cap == 0 || ph_cap == 0) {
            return;
        }

        for (size_t i = 0; i < pkt_cap; ++i) {
            SignalPacketPOD &pkt = packets[i];
            if (prosody && i < pros_cap) {
                pkt.current_volume = prosody[i].volume_gain;
                pkt.current_pitch  = prosody[i].pitch_multiplier;
            }
            if (pkt.phoneme_count == 0u || pkt.phoneme_csr_offset + pkt.phoneme_count > ph_cap) {
                continue;
            }
            const TransmissionMediumPOD med = (meds && pkt.medium_id < med_cap)
                ? meds[pkt.medium_id]
                : ((meds && med_cap > 0) ? meds[0] : TransmissionMediumPOD{});

            std::span<uint16_t> slice(ph_stream + pkt.phoneme_csr_offset, pkt.phoneme_count);
            const uint64_t pkt_seed = random_seed ^ (static_cast<uint64_t>(pkt.packet_id + 1u) * 0x9E3779B97F4A7C15ULL);
            pkt.garbled_syllable_count = NoisyChannelTransceiver::degrade_phoneme_features_in_medium(
                med,
                pkt.effective_snr_db,
                pkt_seed,
                slice
            );
            pkt.noise_tier = static_cast<uint8_t>(NoisyChannelTransceiver::classify_noise_tier(pkt.effective_snr_db));
            pkt.percent_transmitted = 1.0f;
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(NoisyMediumPhonemeDegradeTransformLogic<>) == 64, "NoisyMediumPhonemeDegradeTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<NoisyMediumPhonemeDegradeTransformLogic<>>, "NoisyMediumPhonemeDegradeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
