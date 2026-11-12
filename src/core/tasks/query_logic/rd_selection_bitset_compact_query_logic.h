#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * RDSelectionBitsetCompactQueryLogic<T, T_Strategy, T_View> (G_RD9)
 * Reads a GPU compute predicate buffer (either 32-bit per-element 0/1 flags or pre-packed
 * 64-bit bitset words) and compacts/updates MemoryBufferSelectionPOD (DENSE 64-bit bitset
 * with hardware std::popcount or SPARSE index list).
 */
template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) RDSelectionBitsetCompactQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::UINT32 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD GPU Predicate to Selection Bitset Compact";
    static constexpr std::string_view ui_category  = "GPU/Query";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B
    uint32_t predicate_buffer_id          = INVALID_ID; // 4B
    int32_t predicate_layout              = 0;          // 4B: 0 = Per-element uint32, 1 = Packed uint64 bitset words
    uint32_t predicate_threshold          = 1;          // 4B -> 16B
    bool clear_selection_before_compact   = true;       // 1B
    uint8_t reserved_padding[47]          = {0};        // 47B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("predicate_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("predicate_layout", godot::Variant::INT, 0, 2, "Per-Element uint32 Flags,Packed uint64 Bitset Words");
        add_prop("predicate_threshold", godot::Variant::INT, 1);
        add_prop("clear_selection_before_compact", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : predicate_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("predicate_buffer_id")) predicate_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["predicate_buffer_id"]));
        if (p_props.has("predicate_layout")) predicate_layout = std::clamp<int32_t>(static_cast<int32_t>(p_props["predicate_layout"]), 0, 1);
        if (p_props.has("predicate_threshold")) predicate_threshold = static_cast<uint32_t>(std::max<int64_t>(1, static_cast<int64_t>(p_props["predicate_threshold"])));
        if (p_props.has("clear_selection_before_compact")) clear_selection_before_compact = static_cast<bool>(p_props["clear_selection_before_compact"]);
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        const T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* pred_part = (predicate_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(predicate_buffer_id)
                : ((target_buffer_id != INVALID_ID)
                    ? p_ctx.get_grant_part(target_buffer_id)
                    : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr));
            if (!pred_part || !pred_part->raw_base_ptr || r_selection.capacity <= 0) return;

            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                const size_t num_words = static_cast<size_t>((r_selection.capacity + 63) / 64);
                if (clear_selection_before_compact && Op == QueryOp::ADD) {
                    for (size_t w = 0; w < num_words; ++w) r_selection.data.bitset[w] = 0ULL;
                }

                if (predicate_layout == 1) {
                    const size_t avail_words = std::min<size_t>(num_words, pred_part->capacity_bytes / sizeof(uint64_t));
                    const uint64_t* src_words = reinterpret_cast<const uint64_t*>(pred_part->raw_base_ptr);
                    for (size_t w = 0; w < avail_words; ++w) {
                        if constexpr (Op == QueryOp::ADD) r_selection.data.bitset[w] |= src_words[w];
                        else r_selection.data.bitset[w] &= src_words[w];
                    }
                } else {
                    const uint32_t stride = std::max<uint32_t>(sizeof(uint32_t), pred_part->element_stride);
                    const int64_t n_flags = std::min<int64_t>(r_selection.capacity, static_cast<int64_t>(pred_part->capacity_bytes / stride));
                    for (int64_t i = 0; i < n_flags; ++i) {
                        const uint32_t flag = *reinterpret_cast<const uint32_t*>(pred_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                        const bool pass = (flag >= predicate_threshold);
                        const uint64_t w = static_cast<uint64_t>(i) >> 6;
                        const uint64_t b = 1ULL << (i & 63);
                        if constexpr (Op == QueryOp::ADD) {
                            if (pass) {
                                r_selection.data.bitset[w] |= b;
                                p_ctx.queue_selection_command(target_buffer_id, i);
                            }
                        } else {
                            if (!pass) r_selection.data.bitset[w] &= ~b;
                        }
                    }
                }

                int64_t total_active = 0;
                for (size_t w = 0; w < num_words; ++w) {
                    total_active += std::popcount(r_selection.data.bitset[w]);
                }
                r_selection.element_count = total_active;
            } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                if (clear_selection_before_compact) {
                    r_selection.element_count = 0;
                }
                if (predicate_layout == 1) {
                    const size_t avail_words = pred_part->capacity_bytes / sizeof(uint64_t);
                    const uint64_t* src_words = reinterpret_cast<const uint64_t*>(pred_part->raw_base_ptr);
                    for (size_t w = 0; w < avail_words; ++w) {
                        uint64_t mask = src_words[w];
                        while (mask != 0ULL && r_selection.element_count < r_selection.capacity) {
                            const int32_t b = std::countr_zero(mask);
                            const int64_t idx = static_cast<int64_t>(w * 64 + b);
                            if (idx < r_selection.capacity) {
                                r_selection.data.indices[r_selection.element_count++] = idx;
                            }
                            mask &= (mask - 1ULL);
                        }
                    }
                } else {
                    const uint32_t stride = std::max<uint32_t>(sizeof(uint32_t), pred_part->element_stride);
                    const int64_t n_flags = std::min<int64_t>(r_selection.capacity, static_cast<int64_t>(pred_part->capacity_bytes / stride));
                    for (int64_t i = 0; i < n_flags && r_selection.element_count < r_selection.capacity; ++i) {
                        const uint32_t flag = *reinterpret_cast<const uint32_t*>(pred_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                        if (flag >= predicate_threshold) {
                            r_selection.data.indices[r_selection.element_count++] = i;
                        }
                    }
                }
            }
        }
    }
};

static_assert(sizeof(RDSelectionBitsetCompactQueryLogic<>) == 64, "RDSelectionBitsetCompactQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<RDSelectionBitsetCompactQueryLogic<>>, "RDSelectionBitsetCompactQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core