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
 * M39: SweptSphereSurfaceQueryLogic
 * Evaluates a swept sphere (capsule volume) from sweep_start to sweep_end with sphere_radius
 * and optional front-face normal dot-product filtering, waking/sleeping intersected surface vertices.
 */
struct SweptSphereSurfaceQueryLogic {
    static constexpr std::string_view ui_name = "Swept Sphere / Capsule Surface Query";
    static constexpr std::string_view ui_category = "Model/Query";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 1> supported_types = {
        DataType::VECTOR3
    };
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    godot::Vector3 sweep_start = godot::Vector3(0.0f, 1.0f, 0.0f);
    godot::Vector3 sweep_end = godot::Vector3(0.0f, -1.0f, 0.0f);
    float sphere_radius = 1.0f;
    float min_normal_dot = -1.0f;
    bool require_front_facing = false;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("sweep_start", godot::Variant::VECTOR3, sweep_start);
        add_prop("sweep_end", godot::Variant::VECTOR3, sweep_end);
        add_prop("sphere_radius", godot::Variant::FLOAT, sphere_radius);
        add_prop("min_normal_dot", godot::Variant::FLOAT, min_normal_dot);
        add_prop("require_front_facing", godot::Variant::BOOL, require_front_facing);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("sweep_start")) sweep_start = p_props["sweep_start"];
        if (p_props.has("sweep_end")) sweep_end = p_props["sweep_end"];
        if (p_props.has("sphere_radius")) sphere_radius = std::max(1e-4f, static_cast<float>(p_props["sphere_radius"]));
        if (p_props.has("min_normal_dot")) min_normal_dot = std::clamp(static_cast<float>(p_props["min_normal_dot"]), -1.0f, 1.0f);
        if (p_props.has("require_front_facing")) require_front_facing = static_cast<bool>(p_props["require_front_facing"]);
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
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        if (!vtx_part.raw_base_ptr) return;

        const GrantPartPOD* nrm_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        MemoryBufferSelectionPOD& sel = vtx_part.selection;
        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part.element_stride);
        const int64_t n_verts = (sel.element_count > 0)
            ? sel.element_count
            : static_cast<int64_t>(vtx_part.capacity_bytes / v_stride);

        const godot::Vector3 seg = sweep_end - sweep_start;
        const float seg_len2 = seg.length_squared();
        const godot::Vector3 sweep_dir = (seg_len2 > 1e-10f)
            ? (seg / std::sqrt(seg_len2))
            : godot::Vector3(0.0f, -1.0f, 0.0f);
        const float r2 = sphere_radius * sphere_radius;

        for (int64_t i = 0; i < n_verts; ++i) {
            const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i) * v_stride);
            const float t = (seg_len2 > 1e-10f) ? std::clamp((p - sweep_start).dot(seg) / seg_len2, 0.0f, 1.0f) : 0.0f;
            const godot::Vector3 closest = sweep_start + seg * t;
            const float d2 = (p - closest).length_squared();
            if (d2 > r2) continue;

            if (require_front_facing && nrm_part) {
                const uint32_t n_stride = std::max<uint32_t>(sizeof(godot::Vector3), nrm_part->element_stride);
                if (static_cast<size_t>(i + 1) * n_stride <= nrm_part->capacity_bytes) {
                    const godot::Vector3& nrm = *reinterpret_cast<const godot::Vector3*>(nrm_part->raw_base_ptr + static_cast<size_t>(i) * n_stride);
                    // Front-facing relative to incoming sweep means normal opposes sweep_dir (-nrm.dot(sweep_dir) >= min_normal_dot)
                    if (-nrm.dot(sweep_dir) < min_normal_dot) continue;
                }
            }

            apply_selection_op(sel, i, p_op);
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