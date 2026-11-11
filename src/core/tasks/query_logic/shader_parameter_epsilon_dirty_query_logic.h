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
#include <cmath>
#include <cstring>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * S7: ShaderParameterEpsilonDirtyQueryLogic
 * Compares current per-instance shader parameter values (Part 0) against last-committed shadow
 * values (Part 1), waking only instances whose L2 change exceeds epsilon (and optionally updating
 * Part 1 on wake) so downstream S3 tasks only touch modified instance RIDs.
 */
struct ShaderParameterEpsilonDirtyQueryLogic {
    static constexpr std::string_view ui_name = "Shader Parameter Epsilon Dirty Query";
    static constexpr std::string_view ui_category = "Shader/Query";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 5> supported_types = {
        DataType::FLOAT32,
        DataType::VECTOR2,
        DataType::VECTOR3,
        DataType::VECTOR4,
        DataType::COLOR
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    float epsilon = 1e-4f;
    bool commit_shadow_on_dirty = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("epsilon", godot::Variant::FLOAT, epsilon);
        add_prop("commit_shadow_on_dirty", godot::Variant::BOOL, commit_shadow_on_dirty);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("epsilon")) epsilon = std::max(0.0f, static_cast<float>(p_props["epsilon"]));
        if (p_props.has("commit_shadow_on_dirty")) commit_shadow_on_dirty = static_cast<bool>(p_props["commit_shadow_on_dirty"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline void apply_selection_op(MemoryBufferSelectionPOD& r_sel, int64_t idx, QueryOp p_op) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
            const uint64_t word = static_cast<uint64_t>(idx) >> 6;
            const uint64_t bit = 1ULL << (idx & 63);
            const bool was_set = (r_sel.data.bitset[word] & bit) != 0;
            if (p_op == QueryOp::WAKE && !was_set) {
                r_sel.data.bitset[word] |= bit;
                r_sel.element_count++;
            } else if (p_op == QueryOp::SLEEP && was_set) {
                r_sel.data.bitset[word] &= ~bit;
                r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
            } else if (p_op == QueryOp::INVERT) {
                r_sel.data.bitset[word] ^= bit;
                r_sel.element_count += was_set ? -1 : 1;
            }
        } else if (r_sel.mode == SelectionMode::SPARSE && r_sel.data.indices) {
            if (p_op == QueryOp::WAKE && r_sel.element_count < r_sel.capacity) {
                r_sel.data.indices[r_sel.element_count++] = idx;
            }
        }
    }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& cur_part = p_ctx.grant->parts[0];
        GrantPartPOD& prev_part = p_ctx.grant->parts[1];
        if (!cur_part.raw_base_ptr || !prev_part.raw_base_ptr) return;

        MemoryBufferSelectionPOD& sel = cur_part.selection;
        const uint32_t stride = std::max<uint32_t>(sizeof(float), cur_part.element_stride);
        const uint32_t floats_per_elem = std::max<uint32_t>(1, stride / sizeof(float));
        const int64_t n_elems = std::min<int64_t>(
            static_cast<int64_t>(cur_part.capacity_bytes / stride),
            static_cast<int64_t>(prev_part.capacity_bytes / stride)
        );
        const float eps2 = epsilon * epsilon;

        for (int64_t i = 0; i < n_elems; ++i) {
            const float* cur_f = reinterpret_cast<const float*>(cur_part.raw_base_ptr + static_cast<size_t>(i) * stride);
            float* prev_f = reinterpret_cast<float*>(prev_part.raw_base_ptr + static_cast<size_t>(i) * stride);

            float diff2 = 0.0f;
            for (uint32_t c = 0; c < floats_per_elem; ++c) {
                const float d = cur_f[c] - prev_f[c];
                diff2 += d * d;
            }

            if (diff2 > eps2) {
                apply_selection_op(sel, i, p_op);
                if (commit_shadow_on_dirty) {
                    std::memcpy(prev_f, cur_f, floats_per_elem * sizeof(float));
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