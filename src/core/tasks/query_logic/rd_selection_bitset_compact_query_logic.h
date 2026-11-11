#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * G_RD9: RDSelectionBitsetCompactQueryLogic
 * Reads a GPU compute predicate buffer (either 32-bit per-element 0/1 flags or pre-packed
 * 64-bit bitset words) and compacts/updates MemoryBufferSelectionPOD (DENSE 64-bit bitset
 * with hardware std::popcount or SPARSE index list).
 */
struct RDSelectionBitsetCompactQueryLogic {
    static constexpr std::string_view ui_name = "RD GPU Predicate to Selection Bitset Compact";
    static constexpr std::string_view ui_category = "GPU/Query";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::UINT32,
        DataType::INT32
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    int32_t predicate_layout = 0; // 0 = One uint32 predicate per element, 1 = Packed uint64 bitset words
    uint32_t predicate_threshold = 1;
    bool clear_selection_before_compact = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("predicate_layout", godot::Variant::INT, predicate_layout, 2, "Per-Element uint32 Flags,Packed uint64 Bitset Words");
        add_prop("predicate_threshold", godot::Variant::INT, static_cast<int32_t>(predicate_threshold));
        add_prop("clear_selection_before_compact", godot::Variant::BOOL, clear_selection_before_compact);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("predicate_layout")) predicate_layout = std::clamp<int32_t>(static_cast<int32_t>(p_props["predicate_layout"]), 0, 1);
        if (p_props.has("predicate_threshold")) predicate_threshold = static_cast<uint32_t>(std::max<int64_t>(1, static_cast<int64_t>(p_props["predicate_threshold"])));
        if (p_props.has("clear_selection_before_compact")) clear_selection_before_compact = static_cast<bool>(p_props["clear_selection_before_compact"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& target_part = p_ctx.grant->parts[0];
        const GrantPartPOD& pred_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? p_ctx.grant->parts[1]
            : target_part;
        if (!pred_part.raw_base_ptr) return;

        MemoryBufferSelectionPOD& sel = target_part.selection;
        if (sel.capacity <= 0) return;

        if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
            const size_t num_words = static_cast<size_t>((sel.capacity + 63) / 64);
            if (clear_selection_before_compact && p_op == QueryOp::WAKE) {
                for (size_t w = 0; w < num_words; ++w) sel.data.bitset[w] = 0ULL;
            }

            if (predicate_layout == 1) {
                const size_t avail_words = std::min<size_t>(num_words, pred_part.capacity_bytes / sizeof(uint64_t));
                const uint64_t* src_words = reinterpret_cast<const uint64_t*>(pred_part.raw_base_ptr);
                for (size_t w = 0; w < avail_words; ++w) {
                    if (p_op == QueryOp::WAKE) sel.data.bitset[w] |= src_words[w];
                    else if (p_op == QueryOp::SLEEP) sel.data.bitset[w] &= ~src_words[w];
                    else if (p_op == QueryOp::INVERT) sel.data.bitset[w] ^= src_words[w];
                }
            } else {
                const uint32_t stride = std::max<uint32_t>(sizeof(uint32_t), pred_part.element_stride);
                const int64_t n_flags = std::min<int64_t>(sel.capacity, static_cast<int64_t>(pred_part.capacity_bytes / stride));
                for (int64_t i = 0; i < n_flags; ++i) {
                    const uint32_t flag = *reinterpret_cast<const uint32_t*>(pred_part.raw_base_ptr + static_cast<size_t>(i) * stride);
                    if (flag >= predicate_threshold) {
                        const uint64_t w = static_cast<uint64_t>(i) >> 6;
                        const uint64_t b = 1ULL << (i & 63);
                        if (p_op == QueryOp::WAKE) sel.data.bitset[w] |= b;
                        else if (p_op == QueryOp::SLEEP) sel.data.bitset[w] &= ~b;
                        else if (p_op == QueryOp::INVERT) sel.data.bitset[w] ^= b;
                    }
                }
            }

            int64_t total_active = 0;
            for (size_t w = 0; w < num_words; ++w) {
                total_active += std::popcount(sel.data.bitset[w]);
            }
            sel.element_count = total_active;
        } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
            if (clear_selection_before_compact) {
                sel.element_count = 0;
            }
            if (predicate_layout == 1) {
                const size_t avail_words = pred_part.capacity_bytes / sizeof(uint64_t);
                const uint64_t* src_words = reinterpret_cast<const uint64_t*>(pred_part.raw_base_ptr);
                for (size_t w = 0; w < avail_words; ++w) {
                    uint64_t mask = src_words[w];
                    while (mask != 0ULL && sel.element_count < sel.capacity) {
                        const int32_t b = std::countr_zero(mask);
                        const int64_t idx = static_cast<int64_t>(w * 64 + b);
                        if (idx < sel.capacity) {
                            sel.data.indices[sel.element_count++] = idx;
                        }
                        mask &= (mask - 1ULL);
                    }
                }
            } else {
                const uint32_t stride = std::max<uint32_t>(sizeof(uint32_t), pred_part.element_stride);
                const int64_t n_flags = std::min<int64_t>(sel.capacity, static_cast<int64_t>(pred_part.capacity_bytes / stride));
                for (int64_t i = 0; i < n_flags && sel.element_count < sel.capacity; ++i) {
                    const uint32_t flag = *reinterpret_cast<const uint32_t*>(pred_part.raw_base_ptr + static_cast<size_t>(i) * stride);
                    if (flag >= predicate_threshold) {
                        sel.data.indices[sel.element_count++] = i;
                    }
                }
            }
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, QueryOp p_op, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, p_op);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }
};

} // namespace ideam::core