#pragma once

/**
 * matryoshka_binary_hamming_query_logic.h
 * IdeamLinguistics Query Task (`LNG4` — Wave 2 128-Bit Binary-Quantized Matryoshka Hamming Search):
 *
 * Industry-standard Matryoshka Representation Learning (MRL) + 1-Bit Binary Quantization
 * query kernel:
 *   Computes 2-cycle `std::popcount(a_lo ^ b_lo) + std::popcount(a_hi ^ b_hi)` Hamming
 *   similarity across contiguous SOA columns (`LING_SEMEME_MRL_BINARY_LO64` and
 *   `LING_SEMEME_MRL_BINARY_HI64`, or `SememeDescriptorPOD::nuclear_seme_words[1..2]`)
 *   with optional 1-cycle symbolic ontology pre-filtering (`LING_SEMEME_NUCLEAR_SYMBOLIC_WORD0`).
 */

#include "../../../../linguistics/linguistics_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/sparse_set_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SparseSetView
>
struct alignas(64) MatryoshkaBinaryHammingQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Matryoshka 128-Bit Binary Hamming Query";
    static constexpr std::string_view ui_category  = "Linguistics/SemanticQuery";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                        = INVALID_ID; // 4B
    uint32_t sememe_mrl_binary_lo64_buffer_id        = INVALID_ID; // 4B: uint64_t SOA MRL bits 0..63
    uint32_t sememe_mrl_binary_hi64_buffer_id        = INVALID_ID; // 4B: uint64_t SOA MRL bits 64..127
    uint32_t sememe_nuclear_symbolic_word0_buffer_id = INVALID_ID; // 4B: uint64_t SOA symbolic ontology filter
    uint32_t query_context_pod_buffer_id             = INVALID_ID; // 4B: SememeDescriptorPOD[2] query context
    uint32_t sememe_affinity_score_buffer_id         = INVALID_ID; // 4B: float output similarity score
    uint64_t inline_query_mrl_lo64                   = 0ULL;       // 8B: Direct MRL query low 64 bits
    uint64_t inline_query_mrl_hi64                   = 0ULL;       // 8B: Direct MRL query high 64 bits
    float    min_hamming_similarity                  = 0.60f;      // 4B: Similarity floor in [0.0 .. 1.0]
    uint8_t  matryoshka_prefix_bits                  = 128u;       // 1B: Prefix truncation (64 or 128 bits)
    bool     require_symbolic_filter_match           = false;      // 1B: Pre-filter on symbolic nuclear_seme_words[0]
    uint8_t  reserved_padding[18]                    = {0};        // 18B -> Total = 64B

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
        add_buf("sememe_mrl_binary_lo64_buffer_id");
        add_buf("sememe_mrl_binary_hi64_buffer_id");
        add_buf("sememe_nuclear_symbolic_word0_buffer_id");
        add_buf("query_context_pod_buffer_id");
        add_buf("sememe_affinity_score_buffer_id");

        godot::Dictionary sim;
        sim["name"]        = "min_hamming_similarity";
        sim["type"]        = godot::Variant::FLOAT;
        sim["hint"]        = godot::PROPERTY_HINT_RANGE;
        sim["hint_string"] = "0.0,1.0,0.01";
        props.push_back(sim);

        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                return target_buffer_id;
        if (sememe_affinity_score_buffer_id != INVALID_ID) return sememe_affinity_score_buffer_id;
        return sememe_mrl_binary_lo64_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                        target_buffer_id                        = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sememe_mrl_binary_lo64_buffer_id"))        sememe_mrl_binary_lo64_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_mrl_binary_lo64_buffer_id"]));
        if (p_props.has("sememe_mrl_binary_hi64_buffer_id"))        sememe_mrl_binary_hi64_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_mrl_binary_hi64_buffer_id"]));
        if (p_props.has("sememe_nuclear_symbolic_word0_buffer_id")) sememe_nuclear_symbolic_word0_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_nuclear_symbolic_word0_buffer_id"]));
        if (p_props.has("query_context_pod_buffer_id"))             query_context_pod_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["query_context_pod_buffer_id"]));
        if (p_props.has("sememe_affinity_score_buffer_id"))         sememe_affinity_score_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_affinity_score_buffer_id"]));
        if (p_props.has("inline_query_mrl_lo64"))                   inline_query_mrl_lo64                   = static_cast<uint64_t>(static_cast<int64_t>(p_props["inline_query_mrl_lo64"]));
        if (p_props.has("inline_query_mrl_hi64"))                   inline_query_mrl_hi64                   = static_cast<uint64_t>(static_cast<int64_t>(p_props["inline_query_mrl_hi64"]));
        if (p_props.has("min_hamming_similarity"))                  min_hamming_similarity                  = static_cast<float>(static_cast<double>(p_props["min_hamming_similarity"]));
        if (p_props.has("matryoshka_prefix_bits"))                  matryoshka_prefix_bits                  = static_cast<uint8_t>(static_cast<int64_t>(p_props["matryoshka_prefix_bits"]));
        if (p_props.has("require_symbolic_filter_match"))           require_symbolic_filter_match           = static_cast<bool>(p_props["require_symbolic_filter_match"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *lo_part    = p_ctx.get_grant_part(sememe_mrl_binary_lo64_buffer_id);
        const GrantPartPOD *hi_part    = p_ctx.get_grant_part(sememe_mrl_binary_hi64_buffer_id);
        const GrantPartPOD *sym_part   = p_ctx.get_grant_part(sememe_nuclear_symbolic_word0_buffer_id);
        const GrantPartPOD *query_part = p_ctx.get_grant_part(query_context_pod_buffer_id);
        const GrantPartPOD *score_part = p_ctx.get_grant_part(sememe_affinity_score_buffer_id);
        const GrantPartPOD *tgt_part   = p_ctx.get_grant_part(get_target_buffer_id());

        if (!lo_part || !lo_part->raw_base_ptr) return;

        const auto *lo_col  = reinterpret_cast<const uint64_t *>(lo_part->raw_base_ptr);
        const auto *hi_col  = (hi_part && hi_part->raw_base_ptr) ? reinterpret_cast<const uint64_t *>(hi_part->raw_base_ptr) : nullptr;
        const auto *sym_col = (sym_part && sym_part->raw_base_ptr) ? reinterpret_cast<const uint64_t *>(sym_part->raw_base_ptr) : nullptr;
        const size_t count  = lo_part->capacity_bytes / sizeof(uint64_t);

        uint64_t q_sym = 0ULL;
        uint64_t q_lo  = inline_query_mrl_lo64;
        uint64_t q_hi  = inline_query_mrl_hi64;
        if (query_part && query_part->raw_base_ptr && query_part->capacity_bytes >= sizeof(SememeDescriptorPOD)) {
            const auto *q_pods = reinterpret_cast<const SememeDescriptorPOD *>(query_part->raw_base_ptr);
            q_sym = q_pods[0].nuclear_seme_words[0];
            if (q_lo == 0ULL && q_hi == 0ULL) {
                q_lo = q_pods[0].nuclear_seme_words[1];
                q_hi = q_pods[0].nuclear_seme_words[2];
            }
        }

        auto *scores = (score_part && score_part->raw_base_ptr)
            ? reinterpret_cast<float *>(score_part->raw_base_ptr)
            : nullptr;
        const size_t score_cap = scores ? (score_part->capacity_bytes / sizeof(float)) : 0u;

        MemoryBufferSelectionPOD *sel = tgt_part ? const_cast<MemoryBufferSelectionPOD *>(&tgt_part->selection) : nullptr;
        int64_t out_sparse_count = 0;
        const bool use_64_only = (matryoshka_prefix_bits <= 64u) || (hi_col == nullptr);

        for (size_t i = 0; i < count; ++i) {
            if (require_symbolic_filter_match && sym_col && q_sym != 0ULL) {
                if ((sym_col[i] & q_sym) != q_sym) {
                    if (scores && i < score_cap) scores[i] = 0.0f;
                    continue;
                }
            }

            float sim = 0.0f;
            if (q_lo != 0ULL || q_hi != 0ULL) {
                if (use_64_only) {
                    const uint32_t dist = static_cast<uint32_t>(std::popcount(lo_col[i] ^ q_lo));
                    sim = 1.0f - (static_cast<float>(dist) / 64.0f);
                } else {
                    const uint32_t dist = static_cast<uint32_t>(
                        std::popcount(lo_col[i] ^ q_lo) + std::popcount(hi_col[i] ^ q_hi)
                    );
                    sim = 1.0f - (static_cast<float>(dist) / 128.0f);
                }
            }

            if (scores && i < score_cap) {
                // Blend with any existing symbolic affinity if already computed
                scores[i] = (scores[i] > 0.0f) ? (0.60f * scores[i] + 0.40f * sim) : sim;
            }

            const float effective_score = (scores && i < score_cap) ? scores[i] : sim;
            if (sel && sel->data.indices && static_cast<int64_t>(i) < sel->capacity) {
                if (effective_score >= min_hamming_similarity && out_sparse_count < sel->capacity) {
                    sel->data.indices[out_sparse_count++] = static_cast<int64_t>(i);
                }
            }
        }

        if (sel && sel->data.indices) {
            sel->element_count = out_sparse_count;
        }
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute(
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

static_assert(sizeof(MatryoshkaBinaryHammingQueryLogic<>) == 64, "MatryoshkaBinaryHammingQueryLogic must be 64 bytes!");
static_assert(IsQueryLogic<MatryoshkaBinaryHammingQueryLogic<>>, "MatryoshkaBinaryHammingQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core
