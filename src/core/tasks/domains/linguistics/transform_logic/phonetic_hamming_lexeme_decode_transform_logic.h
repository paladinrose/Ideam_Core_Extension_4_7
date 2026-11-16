#pragma once

/**
 * phonetic_hamming_lexeme_decode_transform_logic.h
 * IdeamLinguistics Transform Task (`LNG5` — Wave 3 PanPhon/CMUdict Needleman-Wunsch Phonetic Decode):
 *
 * Reconstructs the most likely `LexemePOD` and `SememeDescriptorPOD` from noise-degraded
 * `uint16_t` PanPhon articulatory feature streams (`LING_PHONEME_FEATURE_STREAM` +
 * `LING_PHONEME_UTTERANCE_SPAN_POD`) using stack-allocated Needleman-Wunsch / Levenshtein-Hamming
 * Dynamic Time Warping (`SemanticLexiconCore::compute_phoneme_slice_distance`), biased by listener
 * context expectations (`expected_sememe_id`) and dialect (`preferred_lect_id`), writing decoded
 * sememe IDs into `LING_DECODED_SEMEME_STREAM`.
 */

#include "../../../../linguistics/semantic_lexicon_core.h"
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

/**
 * PhoneticUtteranceSpanPOD (32 Bytes, alignas(16))
 * Describes a contiguous slice of noisy `uint16_t` PanPhon articulatory masks in
 * `LING_PHONEME_FEATURE_STREAM` to be decoded into a `LexemePOD` / `SememeDescriptorPOD`.
 */
struct alignas(16) PhoneticUtteranceSpanPOD {
    uint32_t phoneme_stream_offset = 0;          // 4B: Start index in LING_PHONEME_FEATURE_STREAM
    uint16_t phoneme_count         = 0;          // 2B: Number of uint16_t phoneme masks
    uint16_t preferred_lect_id     = 0;          // 2B: Listener/speaker dialect lect ID
    uint32_t expected_sememe_id    = 0xFFFFFFFFu;// 4B: Listener expectation bias (or 0xFFFFFFFF if none)
    uint32_t decoded_lexeme_id     = 0xFFFFFFFFu;// 4B: Output best matching LexemePOD index
    uint32_t decoded_sememe_id     = 0xFFFFFFFFu;// 4B: Output best matching SememeDescriptorPOD index
    float    phonetic_distance     = 1.0f;       // 4B: Normalized Needleman-Wunsch DTW distance [0..1]
    uint32_t honest_misreference   = 0;          // 4B: 1 if decoded_sememe_id != expected_sememe_id -> 32B
};
static_assert(sizeof(PhoneticUtteranceSpanPOD) == 32, "PhoneticUtteranceSpanPOD must be 32 bytes.");

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) PhoneticHammingLexemeDecodeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Phonetic Hamming Lexeme Decode & SLM Mask";
    static constexpr std::string_view ui_category  = "Linguistics/Phonology";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                    = INVALID_ID; // 4B
    uint32_t phoneme_utterance_span_pod_buffer_id= INVALID_ID; // 4B: PhoneticUtteranceSpanPOD (32B AoS)
    uint32_t phoneme_feature_stream_buffer_id    = INVALID_ID; // 4B: uint16_t noisy PanPhon feature stream
    uint32_t lexeme_pod_buffer_id                = INVALID_ID; // 4B: LexemePOD (32B AoS)
    uint32_t lexeme_phoneme_csr_masks_buffer_id  = INVALID_ID; // 4B: uint16_t lexicon CSR phoneme masks
    uint32_t decoded_sememe_stream_buffer_id     = INVALID_ID; // 4B: uint32_t decoded sememe ID output stream
    float    expectation_semantic_bias           = 0.12f;      // 4B: Distance discount when lexeme matches expected_sememe_id
    float    lect_match_bias                     = 0.04f;      // 4B: Distance discount when lexeme matches preferred_lect_id
    float    unintelligible_distance_ceiling     = 0.65f;      // 4B: Above this DTW distance, utterance is unintelligible
    uint8_t  reserved_padding[28]                = {0};        // 28B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char *p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("phoneme_utterance_span_pod_buffer_id");
        add_buf("phoneme_feature_stream_buffer_id");
        add_buf("lexeme_pod_buffer_id");
        add_buf("lexeme_phoneme_csr_masks_buffer_id");
        add_buf("decoded_sememe_stream_buffer_id");

        godot::Dictionary bias;
        bias["name"]        = "expectation_semantic_bias";
        bias["type"]        = godot::Variant::FLOAT;
        bias["hint"]        = godot::PROPERTY_HINT_RANGE;
        bias["hint_string"] = "0.0,0.5,0.01";
        props.push_back(bias);

        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                     return target_buffer_id;
        if (phoneme_utterance_span_pod_buffer_id != INVALID_ID) return phoneme_utterance_span_pod_buffer_id;
        return decoded_sememe_stream_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                     target_buffer_id                     = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("phoneme_utterance_span_pod_buffer_id")) phoneme_utterance_span_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["phoneme_utterance_span_pod_buffer_id"]));
        if (p_props.has("phoneme_feature_stream_buffer_id"))     phoneme_feature_stream_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["phoneme_feature_stream_buffer_id"]));
        if (p_props.has("phoneme_buffer_id"))                    phoneme_feature_stream_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["phoneme_buffer_id"]));
        if (p_props.has("lexeme_pod_buffer_id"))                 lexeme_pod_buffer_id                 = static_cast<uint32_t>(static_cast<int64_t>(p_props["lexeme_pod_buffer_id"]));
        if (p_props.has("lexicon_buffer_id"))                    lexeme_pod_buffer_id                 = static_cast<uint32_t>(static_cast<int64_t>(p_props["lexicon_buffer_id"]));
        if (p_props.has("lexeme_phoneme_csr_masks_buffer_id"))   lexeme_phoneme_csr_masks_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["lexeme_phoneme_csr_masks_buffer_id"]));
        if (p_props.has("decoded_sememe_stream_buffer_id"))      decoded_sememe_stream_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["decoded_sememe_stream_buffer_id"]));
        if (p_props.has("expectation_semantic_bias"))            expectation_semantic_bias            = static_cast<float>(static_cast<double>(p_props["expectation_semantic_bias"]));
        if (p_props.has("lect_match_bias"))                      lect_match_bias                      = static_cast<float>(static_cast<double>(p_props["lect_match_bias"]));
        if (p_props.has("unintelligible_distance_ceiling"))      unintelligible_distance_ceiling      = static_cast<float>(static_cast<double>(p_props["unintelligible_distance_ceiling"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *span_part = p_ctx.get_grant_part(phoneme_utterance_span_pod_buffer_id);
        const GrantPartPOD *ph_part   = p_ctx.get_grant_part(phoneme_feature_stream_buffer_id);
        const GrantPartPOD *lex_part  = p_ctx.get_grant_part(lexeme_pod_buffer_id);
        const GrantPartPOD *csr_part  = p_ctx.get_grant_part(lexeme_phoneme_csr_masks_buffer_id);
        const GrantPartPOD *dec_part  = p_ctx.get_grant_part(decoded_sememe_stream_buffer_id);

        if (!span_part || !span_part->raw_base_ptr ||
            !ph_part   || !ph_part->raw_base_ptr   ||
            !lex_part  || !lex_part->raw_base_ptr  ||
            !csr_part  || !csr_part->raw_base_ptr) {
            return;
        }

        auto *spans            = reinterpret_cast<PhoneticUtteranceSpanPOD *>(span_part->raw_base_ptr);
        const size_t span_cap  = (span_part->selection.element_count > 0)
            ? std::min<size_t>(static_cast<size_t>(span_part->selection.element_count), span_part->capacity_bytes / sizeof(PhoneticUtteranceSpanPOD))
            : (span_part->capacity_bytes / sizeof(PhoneticUtteranceSpanPOD));

        const auto *ph_stream  = reinterpret_cast<const uint16_t *>(ph_part->raw_base_ptr);
        const size_t ph_cap    = ph_part->capacity_bytes / sizeof(uint16_t);

        const auto *lexemes    = reinterpret_cast<const LexemePOD *>(lex_part->raw_base_ptr);
        const size_t lex_cap   = (lex_part->selection.element_count > 0)
            ? std::min<size_t>(static_cast<size_t>(lex_part->selection.element_count), lex_part->capacity_bytes / sizeof(LexemePOD))
            : (lex_part->capacity_bytes / sizeof(LexemePOD));

        const auto *csr_masks  = reinterpret_cast<const uint16_t *>(csr_part->raw_base_ptr);
        const size_t csr_cap   = csr_part->capacity_bytes / sizeof(uint16_t);

        auto *decoded_stream   = (dec_part && dec_part->raw_base_ptr)
            ? reinterpret_cast<uint32_t *>(dec_part->raw_base_ptr)
            : nullptr;
        const size_t dec_cap   = decoded_stream ? (dec_part->capacity_bytes / sizeof(uint32_t)) : 0u;

        for (size_t s = 0; s < span_cap; ++s) {
            PhoneticUtteranceSpanPOD &span = spans[s];
            if (span.phoneme_count == 0u || span.phoneme_stream_offset + span.phoneme_count > ph_cap) {
                continue;
            }
            const std::span<const uint16_t> noisy_slice(
                ph_stream + span.phoneme_stream_offset,
                span.phoneme_count
            );

            float best_score       = 1e9f;
            uint32_t best_lex_id   = 0xFFFFFFFFu;
            uint32_t best_sem_id   = 0xFFFFFFFFu;

            for (size_t l = 0; l < lex_cap; ++l) {
                const LexemePOD &lx = lexemes[l];
                if (lx.phoneme_count == 0u || lx.phoneme_csr_offset + lx.phoneme_count > csr_cap) {
                    continue;
                }
                const std::span<const uint16_t> cand_slice(
                    csr_masks + lx.phoneme_csr_offset,
                    lx.phoneme_count
                );
                float dist = SemanticLexiconCore::compute_phoneme_slice_distance(noisy_slice, cand_slice);
                if (span.expected_sememe_id != 0xFFFFFFFFu && lx.sememe_id == span.expected_sememe_id) {
                    dist = std::max(0.0f, dist - expectation_semantic_bias);
                }
                if (lx.lect_id == span.preferred_lect_id) {
                    dist = std::max(0.0f, dist - lect_match_bias);
                }
                if (dist < best_score) {
                    best_score  = dist;
                    best_lex_id = static_cast<uint32_t>(l);
                    best_sem_id = lx.sememe_id;
                }
            }

            span.phonetic_distance = best_score;
            if (best_score <= unintelligible_distance_ceiling) {
                span.decoded_lexeme_id   = best_lex_id;
                span.decoded_sememe_id   = best_sem_id;
                span.honest_misreference = (span.expected_sememe_id != 0xFFFFFFFFu &&
                                            best_sem_id != span.expected_sememe_id) ? 1u : 0u;
            } else {
                span.decoded_lexeme_id   = 0xFFFFFFFFu;
                span.decoded_sememe_id   = 0xFFFFFFFFu;
                span.honest_misreference = 0u;
            }

            if (decoded_stream && s < dec_cap) {
                decoded_stream[s] = span.decoded_sememe_id;
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(PhoneticHammingLexemeDecodeTransformLogic<>) == 64, "PhoneticHammingLexemeDecodeTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<PhoneticHammingLexemeDecodeTransformLogic<>>, "PhoneticHammingLexemeDecodeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
