#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * M40: MeshEdgeStrainQueryLogic
 * Evaluates relative edge strain |L_current - L_rest| / L_rest across deformed and rest-pose
 * 3D vertex buffers (and optional triangle index buffer), waking/sleeping over-strained vertices.
 */
struct MeshEdgeStrainQueryLogic {
    static constexpr std::string_view ui_name = "Mesh Edge Strain Threshold Query";
    static constexpr std::string_view ui_category = "Model/Query";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::INT32
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    float strain_threshold = 0.25f;
    float fallback_rest_length = 0.25f;
    bool evaluate_compression = false;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("strain_threshold", godot::Variant::FLOAT, strain_threshold);
        add_prop("fallback_rest_length", godot::Variant::FLOAT, fallback_rest_length);
        add_prop("evaluate_compression", godot::Variant::BOOL, evaluate_compression);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("strain_threshold")) strain_threshold = std::max(1e-5f, static_cast<float>(p_props["strain_threshold"]));
        if (p_props.has("fallback_rest_length")) fallback_rest_length = std::max(1e-5f, static_cast<float>(p_props["fallback_rest_length"]));
        if (p_props.has("evaluate_compression")) evaluate_compression = static_cast<bool>(p_props["evaluate_compression"]);
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
        const GrantPartPOD& rest_part = p_ctx.grant->parts[1];
        if (!cur_part.raw_base_ptr || !rest_part.raw_base_ptr) return;

        const GrantPartPOD* idx_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        MemoryBufferSelectionPOD& sel = cur_part.selection;
        const godot::Vector3* cur_v = reinterpret_cast<const godot::Vector3*>(cur_part.raw_base_ptr);
        const godot::Vector3* rest_v = reinterpret_cast<const godot::Vector3*>(rest_part.raw_base_ptr);
        const int32_t n_verts = std::min<int32_t>(
            static_cast<int32_t>(cur_part.capacity_bytes / sizeof(godot::Vector3)),
            static_cast<int32_t>(rest_part.capacity_bytes / sizeof(godot::Vector3))
        );
        if (n_verts <= 0) return;

        if (idx_part) {
            const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_tris = static_cast<int32_t>(((idx_part->selection.element_count > 0)
                ? idx_part->selection.element_count
                : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t))) / 3);

            for (int32_t t = 0; t < n_tris; ++t) {
                const int32_t tri[3] = { indices[t * 3 + 0], indices[t * 3 + 1], indices[t * 3 + 2] };
                if (tri[0] < 0 || tri[0] >= n_verts || tri[1] < 0 || tri[1] >= n_verts || tri[2] < 0 || tri[2] >= n_verts) continue;

                for (int e = 0; e < 3; ++e) {
                    const int32_t a = tri[e];
                    const int32_t b = tri[(e + 1) % 3];
                    const float l_cur = (cur_v[b] - cur_v[a]).length();
                    const float l_rest = std::max(1e-5f, (rest_v[b] - rest_v[a]).length());
                    const float strain = evaluate_compression
                        ? (std::abs(l_cur - l_rest) / l_rest)
                        : ((l_cur - l_rest) / l_rest);
                    if (strain >= strain_threshold) {
                        apply_selection_op(sel, a, p_op);
                        apply_selection_op(sel, b, p_op);
                    }
                }
            }
        } else {
            // Direct vertex displacement strain against fallback_rest_length
            const float inv_rest = 1.0f / std::max(1e-5f, fallback_rest_length);
            for (int32_t i = 0; i < n_verts; ++i) {
                const float disp_strain = (cur_v[i] - rest_v[i]).length() * inv_rest;
                if (disp_strain >= strain_threshold) {
                    apply_selection_op(sel, i, p_op);
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