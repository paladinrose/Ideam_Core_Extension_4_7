#pragma once

/**
 * wfc_entropy_partition_metadata_logic.h
 * 64-byte cache-aligned MetadataLogic task struct (`WFC1`) for `IdeamWaveFunctionCollapse`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * - Operates on the shared `ProceduralEnvironment` (`ENVIRONMENT_SLOT_POD` &
 *   `TOKEN_DESCRIPTOR_POD`) populated by `IdeamProceduralGeneration`.
 * - Computes frequency-weighted Shannon, Popcount MRV, or Top-Down Hierarchy Shannon
 *   entropy for every slot in the environment.
 * - Quantizes uncollapsed slots into 16 entropy buckets (`0` = collapsed, `1..14` =
 *   low-to-high entropy, `15` = empty/contradicted), writing the bucket to
 *   `EnvironmentSlotPOD::entropy_bucket` and `r_selection.lod_levels[i]`, while
 *   packing the slot's hierarchy root/parent group into `r_selection.group_masks[i]`
 *   for lock-free parallel subtree collapse.
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include "../../../../wave_function_collapse/wfc_common.h"
#include "../../../../procedural_generation/procedural_environment.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) WFCEntropyPartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32;

    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "WFC Shannon & Bitmask Entropy Partition";
    static constexpr std::string_view ui_category  = "WaveFunctionCollapse";

    uint32_t target_buffer_id           = INVALID_ID; ///< `ENVIRONMENT_SLOT_POD` (`EnvironmentSlotPOD`)
    uint32_t metadata_buffer_id         = INVALID_ID; ///< Optional alias for `ENVIRONMENT_SLOT_POD`
    uint32_t token_buffer_id            = INVALID_ID; ///< `TOKEN_DESCRIPTOR_POD` (`TokenDescriptorPOD`)
    uint32_t slot_entropy_buffer_id     = INVALID_ID; ///< `SLOT_ENTROPY_FLOATS` (`float`)
    uint32_t pointer_conflict_buffer_id = INVALID_ID; ///< `POINTER_OR_CONFLICT_MASK` (`uint32_t`)
    uint32_t max_horizon_radius         = 4;
    WFCEntropyHeuristic heuristic       = WFCEntropyHeuristic::HEURISTIC_SHANNON_WEIGHTED;
    uint8_t  reserved_padding[39]       = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",           godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("metadata_buffer_id",         godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("token_buffer_id",            godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("slot_entropy_buffer_id",     godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_conflict_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("max_horizon_radius",         godot::Variant::INT, static_cast<int64_t>(4));
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))           target_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("metadata_buffer_id"))         metadata_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["metadata_buffer_id"]));
        if (p_props.has("token_buffer_id"))            token_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["token_buffer_id"]));
        if (p_props.has("slot_entropy_buffer_id"))     slot_entropy_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["slot_entropy_buffer_id"]));
        if (p_props.has("pointer_conflict_buffer_id")) pointer_conflict_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_conflict_buffer_id"]));
        if (p_props.has("max_horizon_radius"))         max_horizon_radius         = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_horizon_radius"]));
        if (p_props.has("heuristic"))                  heuristic                  = static_cast<WFCEntropyHeuristic>(static_cast<int64_t>(p_props["heuristic"]));
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
        const uint32_t slot_buf_id = (metadata_buffer_id != INVALID_ID) ? metadata_buffer_id : target_buffer_id;
        size_t slot_cap = 0, tok_cap = 0, ent_cap = 0, ptr_cap = 0;
        auto *slots        = _resolve_ptr<EnvironmentSlotPOD>(p_ctx, slot_buf_id, slot_cap);
        const auto *tokens = _resolve_ptr<const TokenDescriptorPOD>(p_ctx, token_buffer_id, tok_cap);
        auto *entropies    = _resolve_ptr<float>(p_ctx, slot_entropy_buffer_id, ent_cap);
        auto *status_masks = _resolve_ptr<uint32_t>(p_ctx, pointer_conflict_buffer_id, ptr_cap);

        if (!slots || slot_cap == 0) {
            return;
        }

        const uint32_t n_tokens = std::min<uint32_t>(256u, (tok_cap > 0u) ? static_cast<uint32_t>(tok_cap) : 256u);
        const float max_possible_ent = (n_tokens > 1u) ? std::log2(static_cast<float>(n_tokens)) : 1.0f;
        const float inv_max_ent      = (max_possible_ent > 1e-5f) ? (1.0f / max_possible_ent) : 1.0f;

        GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(slot_buf_id));

        for (size_t i = 0; i < slot_cap; ++i) {
            EnvironmentSlotPOD &slot = slots[i];
            const uint32_t pop       = slot.domain.popcount();

            float ent = 0.0f;
            if (pop > 1u) {
                if (heuristic == WFCEntropyHeuristic::HEURISTIC_POPCOUNT_MRV) {
                    ent = static_cast<float>(pop);
                } else {
                    double sum_w       = 0.0;
                    double sum_w_log_w = 0.0;
                    for (uint32_t s = 0; s < n_tokens; ++s) {
                        if (!slot.domain.test_bit(s)) continue;
                        const double w = (tokens && s < tok_cap && tokens[s].base_frequency_weight > 0.0f)
                            ? static_cast<double>(tokens[s].base_frequency_weight)
                            : 1.0;
                        sum_w += w;
                        sum_w_log_w += w * std::log2(w);
                    }
                    if (sum_w > 0.0) {
                        ent = static_cast<float>(std::max(0.0, std::log2(sum_w) - (sum_w_log_w / sum_w)));
                    }
                }
            }

            if (entropies && i < ent_cap) {
                entropies[i] = ent;
            }

            uint8_t bucket = 0u;
            if (pop == 0u || slot.has_conflict != 0u) {
                bucket = 15u;
                slot.has_conflict = 1u;
            } else if (pop == 1u) {
                bucket = 0u;
                slot.collapsed_token_id = slot.domain.first_set_state();
            } else {
                const float norm = std::clamp(ent * inv_max_ent, 0.0f, 1.0f);
                bucket = static_cast<uint8_t>(1u + static_cast<uint32_t>(std::min(13.0f, std::floor(norm * 13.99f))));
            }
            slot.entropy_bucket = bucket;

            if (status_masks && i < ptr_cap) {
                uint32_t flags = SLOT_STATUS_CLEAN;
                if (slot.is_pointer_state != 0u) flags |= SLOT_STATUS_POINTER_STATE;
                if (slot.has_conflict != 0u)     flags |= SLOT_STATUS_HAS_CONFLICT;
                if (pop == 1u)                   flags |= SLOT_STATUS_COLLAPSED;
                if (slot.locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) flags |= SLOT_STATUS_PINNED;
                status_masks[i] = flags;
            }

            if (target_part) {
                MemoryBufferSelectionPOD &sel = target_part->selection;
                const size_t sel_lim = static_cast<size_t>(std::max<int64_t>(0, sel.element_count));
                if (sel.lod_levels && i < sel_lim) {
                    sel.lod_levels[i] = bucket;
                }
                if (sel.group_masks && i < sel_lim) {
                    const uint32_t root_group = (slot.parent_slot_id == 0xFFFFFFFFu)
                        ? (static_cast<uint32_t>(i) & 31u)
                        : (slot.parent_slot_id & 31u);
                    sel.group_masks[i] = (1u << root_group);
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_ctx,
        T_ActiveView &p_view
    ) const noexcept {
        (void)r_selection;
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_metadata(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(WFCEntropyPartitionMetadataLogic<>) == 64, "WFCEntropyPartitionMetadataLogic must be 64 bytes!");
static_assert(IsMetadataLogic<WFCEntropyPartitionMetadataLogic<>>, "WFCEntropyPartitionMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
