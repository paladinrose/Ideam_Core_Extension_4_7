#pragma once

/**
 * bpe_subword_and_morphology_transform_logic.h
 * IdeamLinguistics Transform Task (`LNG6` — Wave 3 SentencePiece / Hugging Face BPE Subword Merge):
 *
 * Applies priority-ordered `BPEMergeRulePOD` `(left_token_id, right_token_id) -> merged_token_id`
 * merges across contiguous token ID sequences in `LING_DECODED_SEMEME_STREAM` (or BPE token stream),
 * and optionally maps merged BPE subword IDs to their corresponding `SememeDescriptorPOD` IDs via
 * `BPEVocabEntryPOD::mapped_sememe_id`.
 */

#include "../../../../linguistics/linguistics_common.h"
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
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) BPESubwordAndMorphologyTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::UINT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "BPE Subword & Morphology Merge";
    static constexpr std::string_view ui_category  = "Linguistics/Tokenization";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                = INVALID_ID; // 4B
    uint32_t decoded_sememe_stream_buffer_id = INVALID_ID; // 4B: uint32_t token ID stream (in-place BPE merges)
    uint32_t bpe_merge_rule_pod_buffer_id    = INVALID_ID; // 4B: BPEMergeRulePOD (16B AoS)
    uint32_t bpe_vocab_pod_buffer_id         = INVALID_ID; // 4B: BPEVocabEntryPOD (16B AoS)
    uint16_t max_bpe_merge_passes            = 16u;        // 2B: Maximum iterative BPE merge passes
    bool     map_bpe_to_sememe_ids_on_finish = false;      // 1B: Replace BPE token IDs with mapped_sememe_id after merging
    uint8_t  reserved_padding[45]            = {0};        // 45B -> Total = 64B

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
        add_buf("decoded_sememe_stream_buffer_id");
        add_buf("bpe_merge_rule_pod_buffer_id");
        add_buf("bpe_vocab_pod_buffer_id");

        godot::Dictionary passes;
        passes["name"]        = "max_bpe_merge_passes";
        passes["type"]        = godot::Variant::INT;
        passes["hint"]        = godot::PROPERTY_HINT_RANGE;
        passes["hint_string"] = "1,64,1";
        props.push_back(passes);

        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                return target_buffer_id;
        if (decoded_sememe_stream_buffer_id != INVALID_ID) return decoded_sememe_stream_buffer_id;
        return bpe_merge_rule_pod_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                target_buffer_id                = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("decoded_sememe_stream_buffer_id")) decoded_sememe_stream_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["decoded_sememe_stream_buffer_id"]));
        if (p_props.has("bpe_merge_rule_pod_buffer_id"))    bpe_merge_rule_pod_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["bpe_merge_rule_pod_buffer_id"]));
        if (p_props.has("bpe_vocab_pod_buffer_id"))         bpe_vocab_pod_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["bpe_vocab_pod_buffer_id"]));
        if (p_props.has("max_bpe_merge_passes"))            max_bpe_merge_passes            = static_cast<uint16_t>(std::clamp<int64_t>(p_props["max_bpe_merge_passes"], 1, 256));
        if (p_props.has("map_bpe_to_sememe_ids_on_finish")) map_bpe_to_sememe_ids_on_finish = static_cast<bool>(p_props["map_bpe_to_sememe_ids_on_finish"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *tok_part   = p_ctx.get_grant_part(decoded_sememe_stream_buffer_id);
        const GrantPartPOD *merge_part = p_ctx.get_grant_part(bpe_merge_rule_pod_buffer_id);
        const GrantPartPOD *vocab_part = p_ctx.get_grant_part(bpe_vocab_pod_buffer_id);

        if (!tok_part || !tok_part->raw_base_ptr || !merge_part || !merge_part->raw_base_ptr) {
            return;
        }

        auto *tokens          = reinterpret_cast<uint32_t *>(tok_part->raw_base_ptr);
        size_t active_len     = (tok_part->selection.element_count > 0)
            ? std::min<size_t>(static_cast<size_t>(tok_part->selection.element_count), tok_part->capacity_bytes / sizeof(uint32_t))
            : (tok_part->capacity_bytes / sizeof(uint32_t));

        const auto *merges    = reinterpret_cast<const BPEMergeRulePOD *>(merge_part->raw_base_ptr);
        const size_t m_count  = (merge_part->selection.element_count > 0)
            ? std::min<size_t>(static_cast<size_t>(merge_part->selection.element_count), merge_part->capacity_bytes / sizeof(BPEMergeRulePOD))
            : (merge_part->capacity_bytes / sizeof(BPEMergeRulePOD));

        for (uint16_t pass = 0; pass < max_bpe_merge_passes && active_len >= 2u && m_count > 0u; ++pass) {
            uint32_t best_rank   = 0xFFFFFFFFu;
            uint32_t best_merged = 0xFFFFFFFFu;
            size_t   best_pos    = active_len;

            for (size_t i = 0; i + 1 < active_len; ++i) {
                const uint32_t left_id  = tokens[i];
                const uint32_t right_id = tokens[i + 1];
                if (left_id == 0xFFFFFFFFu || right_id == 0xFFFFFFFFu) continue;

                for (size_t r = 0; r < m_count; ++r) {
                    if (merges[r].left_token_id == left_id &&
                        merges[r].right_token_id == right_id &&
                        merges[r].priority_rank < best_rank) {
                        best_rank   = merges[r].priority_rank;
                        best_merged = merges[r].merged_token_id;
                        best_pos    = i;
                    }
                }
            }

            if (best_pos >= active_len || best_merged == 0xFFFFFFFFu) {
                break;
            }

            tokens[best_pos] = best_merged;
            for (size_t j = best_pos + 1; j + 1 < active_len; ++j) {
                tokens[j] = tokens[j + 1];
            }
            tokens[active_len - 1] = 0xFFFFFFFFu;
            --active_len;
        }

        if (map_bpe_to_sememe_ids_on_finish && vocab_part && vocab_part->raw_base_ptr) {
            const auto *vocab    = reinterpret_cast<const BPEVocabEntryPOD *>(vocab_part->raw_base_ptr);
            const size_t v_count = vocab_part->capacity_bytes / sizeof(BPEVocabEntryPOD);
            for (size_t i = 0; i < active_len; ++i) {
                const uint32_t tid = tokens[i];
                for (size_t v = 0; v < v_count; ++v) {
                    if (vocab[v].bpe_token_id == tid && vocab[v].mapped_sememe_id != 0xFFFFFFFFu) {
                        tokens[i] = vocab[v].mapped_sememe_id;
                        break;
                    }
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(BPESubwordAndMorphologyTransformLogic<>) == 64, "BPESubwordAndMorphologyTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<BPESubwordAndMorphologyTransformLogic<>>, "BPESubwordAndMorphologyTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
