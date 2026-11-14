#pragma once

/**
 * seme_componential_affinity_query_logic.h
 * IdeamLinguistics Query Task (`LNG3` — Wave 2 Hybrid Symbolic + Peripheral Tversky Query):
 *
 * Matches candidate `SememeDescriptorPOD` / `LinguisticProfilePOD` rows against the active
 * query context (`LING_QUERY_CONTEXT_POD`: `[0]` = required/query descriptor, `[1]` = excluded
 * seme descriptor) using:
 *   1. Hard Symbolic Exclusion Check (`(cand.nuclear[0] & excluded.nuclear[0]) == 0`)
 *   2. Optional Strict Nuclear Containment (`cand.contains_all_nuclear(query)`)
 *   3. Hybrid Symbolic Tversky / Jaccard + 128-bit MRL Binary Embedding Affinity
 *      (`SemanticLexiconCore::compute_descriptor_affinity` or `compute_descriptor_tversky_affinity`),
 *      writing per-row scores to `LING_SEMEME_AFFINITY_SCORE` and culling/waking rows in
 *      the target `SparseSetView` selection.
 */

#include "../../../../linguistics/semantic_lexicon_core.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/sparse_set_view.h"
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
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SparseSetView
>
struct alignas(64) SemeComponentialAffinityQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::INT32 | DataType::UINT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Seme Componential Affinity & Tag Match Query";
    static constexpr std::string_view ui_category  = "Linguistics/SemanticQuery";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                 = INVALID_ID; // 4B: Primary selection / affinity buffer
    uint32_t sememe_descriptor_pod_buffer_id  = INVALID_ID; // 4B: Candidate SememeDescriptorPOD / LinguisticProfilePOD (64B AoS)
    uint32_t query_context_pod_buffer_id      = INVALID_ID; // 4B: SememeDescriptorPOD[2] ([0]=query, [1]=excluded)
    uint32_t sememe_affinity_score_buffer_id  = INVALID_ID; // 4B: float output affinity column
    uint64_t inline_required_symbolic_mask64  = 0ULL;       // 8B: Optional inline 64-bit symbolic mask when query buffer is unset
    uint64_t inline_excluded_symbolic_mask64  = 0ULL;       // 8B: Optional inline 64-bit excluded symbolic mask
    float    threshold                        = 0.35f;      // 4B: Minimum affinity score to pass query
    float    nuclear_weight                   = 0.75f;      // 4B: Nuclear weight (or Tversky alpha)
    float    peripheral_weight                = 0.25f;      // 4B: Peripheral weight (or Tversky beta)
    bool     use_tversky_asymmetric_metric    = false;      // 1B: Use Tversky asymmetric contrast instead of hybrid affinity
    bool     require_all_symbolic_nuclear_bits= false;      // 1B: Require candidate to contain all symbolic bits in query.nuclear[0]
    uint8_t  reserved_padding[18]             = {0};        // 18B -> Total = 64B

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
        add_buf("sememe_descriptor_pod_buffer_id");
        add_buf("query_context_pod_buffer_id");
        add_buf("sememe_affinity_score_buffer_id");

        godot::Dictionary thr;
        thr["name"]        = "threshold";
        thr["type"]        = godot::Variant::FLOAT;
        thr["hint"]        = godot::PROPERTY_HINT_RANGE;
        thr["hint_string"] = "0.0,1.0,0.01";
        props.push_back(thr);

        godot::Dictionary tve;
        tve["name"] = "use_tversky_asymmetric_metric";
        tve["type"] = godot::Variant::BOOL;
        props.push_back(tve);

        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                return target_buffer_id;
        if (sememe_affinity_score_buffer_id != INVALID_ID) return sememe_affinity_score_buffer_id;
        return sememe_descriptor_pod_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                 target_buffer_id                  = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sememe_descriptor_pod_buffer_id"))  sememe_descriptor_pod_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_descriptor_pod_buffer_id"]));
        if (p_props.has("query_context_pod_buffer_id"))      query_context_pod_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["query_context_pod_buffer_id"]));
        if (p_props.has("query_mask_buffer_id"))             query_context_pod_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["query_mask_buffer_id"]));
        if (p_props.has("sememe_affinity_score_buffer_id"))  sememe_affinity_score_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["sememe_affinity_score_buffer_id"]));
        if (p_props.has("inline_required_symbolic_mask64"))  inline_required_symbolic_mask64   = static_cast<uint64_t>(static_cast<int64_t>(p_props["inline_required_symbolic_mask64"]));
        if (p_props.has("inline_excluded_symbolic_mask64"))  inline_excluded_symbolic_mask64   = static_cast<uint64_t>(static_cast<int64_t>(p_props["inline_excluded_symbolic_mask64"]));
        if (p_props.has("threshold"))                        threshold                         = static_cast<float>(static_cast<double>(p_props["threshold"]));
        if (p_props.has("nuclear_weight"))                   nuclear_weight                    = static_cast<float>(static_cast<double>(p_props["nuclear_weight"]));
        if (p_props.has("peripheral_weight"))                peripheral_weight                 = static_cast<float>(static_cast<double>(p_props["peripheral_weight"]));
        if (p_props.has("use_tversky_asymmetric_metric"))    use_tversky_asymmetric_metric     = static_cast<bool>(p_props["use_tversky_asymmetric_metric"]);
        if (p_props.has("require_all_symbolic_nuclear_bits"))require_all_symbolic_nuclear_bits = static_cast<bool>(p_props["require_all_symbolic_nuclear_bits"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *cand_part  = p_ctx.get_grant_part(sememe_descriptor_pod_buffer_id);
        const GrantPartPOD *query_part = p_ctx.get_grant_part(query_context_pod_buffer_id);
        const GrantPartPOD *score_part = p_ctx.get_grant_part(sememe_affinity_score_buffer_id);
        const GrantPartPOD *tgt_part   = p_ctx.get_grant_part(get_target_buffer_id());

        if (!cand_part || !cand_part->raw_base_ptr) return;

        const auto *candidates = reinterpret_cast<const SememeDescriptorPOD *>(cand_part->raw_base_ptr);
        const size_t cand_cap  = cand_part->capacity_bytes / sizeof(SememeDescriptorPOD);

        SememeDescriptorPOD query_desc{};
        SememeDescriptorPOD excluded_desc{};
        if (query_part && query_part->raw_base_ptr && query_part->capacity_bytes >= sizeof(SememeDescriptorPOD)) {
            const auto *q_pods = reinterpret_cast<const SememeDescriptorPOD *>(query_part->raw_base_ptr);
            query_desc = q_pods[0];
            if (query_part->capacity_bytes >= 2u * sizeof(SememeDescriptorPOD)) {
                excluded_desc = q_pods[1];
            }
        }
        query_desc.nuclear_seme_words[0]    |= inline_required_symbolic_mask64;
        excluded_desc.nuclear_seme_words[0] |= inline_excluded_symbolic_mask64;

        auto *scores = (score_part && score_part->raw_base_ptr)
            ? reinterpret_cast<float *>(score_part->raw_base_ptr)
            : nullptr;
        const size_t score_cap = scores ? (score_part->capacity_bytes / sizeof(float)) : 0u;

        // If target selection uses SparseSet, update active indices based on QueryOp
        MemoryBufferSelectionPOD *sel = tgt_part ? const_cast<MemoryBufferSelectionPOD *>(&tgt_part->selection) : nullptr;
        int64_t out_sparse_count = 0;

        for (size_t i = 0; i < cand_cap; ++i) {
            const SememeDescriptorPOD &cand = candidates[i];
            float aff = 0.0f;

            const bool is_excluded = (excluded_desc.nuclear_seme_words[0] != 0ULL) &&
                                     ((cand.nuclear_seme_words[0] & excluded_desc.nuclear_seme_words[0]) != 0ULL);
            const bool missing_req = require_all_symbolic_nuclear_bits &&
                                     ((cand.nuclear_seme_words[0] & query_desc.nuclear_seme_words[0]) != query_desc.nuclear_seme_words[0]);

            if (!is_excluded && !missing_req) {
                if (use_tversky_asymmetric_metric) {
                    aff = SemanticLexiconCore::compute_descriptor_tversky_affinity(
                        query_desc,
                        cand,
                        nuclear_weight,
                        peripheral_weight
                    );
                } else {
                    aff = SemanticLexiconCore::compute_descriptor_affinity(
                        query_desc,
                        cand,
                        nuclear_weight,
                        peripheral_weight
                    );
                }
            }

            if (scores && i < score_cap) {
                scores[i] = aff;
            }

            if (sel && sel->sparse_indices && static_cast<int64_t>(i) < sel->capacity) {
                const bool passes = (aff >= threshold);
                if constexpr (Op == QueryOp::ADD) {
                    if (passes && out_sparse_count < sel->capacity) {
                        sel->sparse_indices[out_sparse_count++] = static_cast<int64_t>(i);
                    }
                } else {
                    // QueryOp::CULL keeps only passing elements
                    if (passes && out_sparse_count < sel->capacity) {
                        sel->sparse_indices[out_sparse_count++] = static_cast<int64_t>(i);
                    }
                }
            }
        }

        if (sel && sel->sparse_indices) {
            sel->element_count = out_sparse_count;
        }
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_query(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<Op, T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(SemeComponentialAffinityQueryLogic<>) == 64, "SemeComponentialAffinityQueryLogic must be 64 bytes!");
static_assert(IsQueryLogic<SemeComponentialAffinityQueryLogic<>>, "SemeComponentialAffinityQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core
