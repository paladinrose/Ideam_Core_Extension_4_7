#pragma once

/**
 * receiver_epistemic_handoff_transform_logic.h
 * `COM4`: 64-byte cache-aligned `TransformLogic` kernel for `IdeamCommunications`
 * (`src/core/tasks/domains/communications/transform_logic/`).
 *
 * Executes Stage 4 (Receiver & Epistemic Handoff to `IdeamMinds`):
 *   - Decodes each `SignalPacketPOD`'s degraded `uint16_t` phoneme stream
 *     against `COMM_LEXEME_POD_TABLE` and `COMM_LEXEME_PHONEME_CSR_MASKS` using
 *     `SemanticLexiconCore::compute_phoneme_slice_distance` biased by the
 *     listener's `ReceiverStatePOD::expected_sememe_id` and `active_lect_id`.
 *   - Writes `decoded_sememe_id` to `COMM_DECODED_SEMEME_STREAM` and populates
 *     `COMM_EPISTEMIC_HANDOFF_POD` (`EpistemicHandoffPOD`).
 *   - When a phonetic bit-flip causes `decoded_sememe_id != intended_sememe_id`,
 *     flags `honest_misreference = 1` (`FACT_FLAG_MISREFERENCED`) and offsets
 *     `decoded_fact_id` so `MindDOD` records the resulting proposition in the
 *     listener's `AgentMindResource` Reference Catalogue as an **Honest Misreference**!
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
struct alignas(64) ReceiverEpistemicHandoffTransformLogic {
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

    static constexpr std::string_view display_name = "COM4: Receiver Phonetic Decode & Mind Epistemic Handoff";
    static constexpr std::string_view ui_category  = "Communications/Decoder";

    // --- 64-Byte Cache-Line Member Layout ---
    uint32_t target_buffer_id                  = INVALID_ID; ///< Offset 0  (4B): `COMM_EPISTEMIC_HANDOFF_POD`
    uint32_t signal_packet_buffer_id           = INVALID_ID; ///< Offset 4  (4B): `COMM_SIGNAL_PACKET_POD`
    uint32_t phoneme_feature_stream_id         = INVALID_ID; ///< Offset 8  (4B): `COMM_PHONEME_FEATURE_STREAM`
    uint32_t receiver_state_buffer_id          = INVALID_ID; ///< Offset 12 (4B): `COMM_RECEIVER_STATE_POD`
    uint32_t lexeme_pod_buffer_id              = INVALID_ID; ///< Offset 16 (4B): `COMM_LEXEME_POD_TABLE`
    uint32_t lexeme_phoneme_csr_masks_buffer_id= INVALID_ID; ///< Offset 20 (4B): `COMM_LEXEME_PHONEME_CSR_MASKS`
    uint32_t decoded_sememe_stream_buffer_id   = INVALID_ID; ///< Offset 24 (4B): `COMM_DECODED_SEMEME_STREAM`
    float    unintelligible_distance_ceiling   = 0.55f;      ///< Offset 28 (4B): Max normalized DTW distance
    float    expectation_semantic_bias         = 0.12f;      ///< Offset 32 (4B): Distance reduction when matching expected_sememe_id
    uint16_t default_tome_id                   = 0;          ///< Offset 36 (2B): Default Akashic Tome ID
    uint8_t  reserved_padding[26]              = {0};        ///< Offset 38 (26B) -> Total = 64 Bytes

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",                   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("signal_packet_buffer_id",            godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("phoneme_feature_stream_id",          godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("receiver_state_buffer_id",           godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("lexeme_pod_buffer_id",               godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("lexeme_phoneme_csr_masks_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("decoded_sememe_stream_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("unintelligible_distance_ceiling",    godot::Variant::FLOAT, 0.55);
        add_prop("expectation_semantic_bias",          godot::Variant::FLOAT, 0.12);
        add_prop("default_tome_id",                    godot::Variant::INT,   static_cast<int64_t>(0));
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                   target_buffer_id                   = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("signal_packet_buffer_id"))            signal_packet_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["signal_packet_buffer_id"]));
        if (p_props.has("phoneme_feature_stream_id"))          phoneme_feature_stream_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["phoneme_feature_stream_id"]));
        if (p_props.has("receiver_state_buffer_id"))           receiver_state_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["receiver_state_buffer_id"]));
        if (p_props.has("lexeme_pod_buffer_id"))               lexeme_pod_buffer_id               = static_cast<uint32_t>(static_cast<int64_t>(p_props["lexeme_pod_buffer_id"]));
        if (p_props.has("lexeme_phoneme_csr_masks_buffer_id")) lexeme_phoneme_csr_masks_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["lexeme_phoneme_csr_masks_buffer_id"]));
        if (p_props.has("decoded_sememe_stream_buffer_id"))    decoded_sememe_stream_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["decoded_sememe_stream_buffer_id"]));
        if (p_props.has("unintelligible_distance_ceiling"))    unintelligible_distance_ceiling    = static_cast<float>(static_cast<double>(p_props["unintelligible_distance_ceiling"]));
        if (p_props.has("expectation_semantic_bias"))          expectation_semantic_bias          = static_cast<float>(static_cast<double>(p_props["expectation_semantic_bias"]));
        if (p_props.has("default_tome_id"))                    default_tome_id                    = static_cast<uint16_t>(static_cast<int64_t>(p_props["default_tome_id"]));
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
        size_t handoff_cap = 0, pkt_cap = 0, ph_cap = 0, rx_cap = 0, lex_cap = 0, csr_cap = 0, dec_cap = 0;
        auto *handoffs        = _resolve_ptr<EpistemicHandoffPOD>(p_ctx, target_buffer_id, handoff_cap);
        auto *packets         = _resolve_ptr<SignalPacketPOD>(p_ctx, signal_packet_buffer_id, pkt_cap);
        const auto *ph_stream = _resolve_ptr<const uint16_t>(p_ctx, phoneme_feature_stream_id, ph_cap);
        const auto *rxs       = _resolve_ptr<const ReceiverStatePOD>(p_ctx, receiver_state_buffer_id, rx_cap);
        const auto *lexemes   = _resolve_ptr<const LexemePOD>(p_ctx, lexeme_pod_buffer_id, lex_cap);
        const auto *csr_masks = _resolve_ptr<const uint16_t>(p_ctx, lexeme_phoneme_csr_masks_buffer_id, csr_cap);
        auto *decoded_stream  = _resolve_ptr<uint32_t>(p_ctx, decoded_sememe_stream_buffer_id, dec_cap);

        if (!packets || pkt_cap == 0) return;

        for (size_t i = 0; i < pkt_cap; ++i) {
            SignalPacketPOD &pkt = packets[i];
            const ReceiverStatePOD rx = (rxs && pkt.receiver_id < rx_cap)
                ? rxs[pkt.receiver_id]
                : ((rxs && rx_cap > 0) ? rxs[0] : ReceiverStatePOD{});

            uint32_t best_sememe = pkt.intended_sememe_id;

            if (ph_stream && lexemes && csr_masks &&
                pkt.phoneme_count > 0u && pkt.phoneme_csr_offset + pkt.phoneme_count <= ph_cap) {
                const std::span<const uint16_t> noisy_slice(ph_stream + pkt.phoneme_csr_offset, pkt.phoneme_count);

                float best_dist = 1e9f;
                uint32_t cand_sem = 0xFFFFFFFFu;
                for (size_t l = 0; l < lex_cap; ++l) {
                    const LexemePOD &lx = lexemes[l];
                    if (lx.phoneme_count == 0u || lx.phoneme_csr_offset + lx.phoneme_count > csr_cap) {
                        continue;
                    }
                    const std::span<const uint16_t> cand_slice(csr_masks + lx.phoneme_csr_offset, lx.phoneme_count);
                    float dist = SemanticLexiconCore::compute_phoneme_slice_distance(noisy_slice, cand_slice);

                    if (rx.expected_sememe_id != 0xFFFFFFFFu && lx.sememe_id == rx.expected_sememe_id) {
                        dist = std::max(0.0f, dist - expectation_semantic_bias * rx.listen_decode_skill);
                    }
                    if (lx.lect_id == pkt.active_lect_id) {
                        dist = std::max(0.0f, dist - 0.04f);
                    }
                    if (dist < best_dist) {
                        best_dist = dist;
                        cand_sem  = lx.sememe_id;
                    }
                }
                if (best_dist <= unintelligible_distance_ceiling) {
                    best_sememe = cand_sem;
                } else {
                    best_sememe = 0xFFFFFFFFu;
                }
            }

            pkt.decoded_sememe_id = best_sememe;
            if (decoded_stream && i < dec_cap) {
                decoded_stream[i] = best_sememe;
            }

            const bool misref = (pkt.intended_sememe_id != 0xFFFFFFFFu &&
                                 best_sememe != 0xFFFFFFFFu &&
                                 best_sememe != pkt.intended_sememe_id);
            pkt.honest_misreference = misref ? 1u : 0u;
            if (misref && pkt.source_fact_id != 0xFFFFFFFFu) {
                pkt.decoded_fact_id = pkt.source_fact_id ^ (best_sememe + 1u);
            } else {
                pkt.decoded_fact_id = pkt.source_fact_id;
            }

            if (handoffs && i < handoff_cap) {
                const float snr_norm = std::clamp((pkt.effective_snr_db + 5.0f) / 25.0f, 0.05f, 1.0f);
                EpistemicHandoffPOD &ho   = handoffs[i];
                ho.packet_id              = pkt.packet_id;
                ho.receiver_mind_agent_id = rx.mind_agent_id;
                ho.source_fact_id         = pkt.source_fact_id;
                ho.decoded_fact_id        = pkt.decoded_fact_id;
                ho.decoded_sememe_id      = pkt.decoded_sememe_id;
                ho.effective_snr_db       = pkt.effective_snr_db;
                ho.decoded_confidence     = snr_norm;
                ho.tome_id                = default_tome_id;
                ho.honest_misreference    = pkt.honest_misreference;
                ho.garbled_syllable_count = pkt.garbled_syllable_count;
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(ReceiverEpistemicHandoffTransformLogic<>) == 64, "ReceiverEpistemicHandoffTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<ReceiverEpistemicHandoffTransformLogic<>>, "ReceiverEpistemicHandoffTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
