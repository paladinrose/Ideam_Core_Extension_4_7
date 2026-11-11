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
 * M38: RayTriangleIntersectQueryLogic
 * Möller-Trumbore ray/segment intersection query over 3D vertex and triangle buffers,
 * waking or sleeping hit triangles/vertices (and optional splash-radius neighbors around the closest hit).
 */
struct RayTriangleIntersectQueryLogic {
    static constexpr std::string_view ui_name = "Ray-Triangle Möller-Trumbore Query";
    static constexpr std::string_view ui_category = "Model/Query";

    static constexpr std::array<MemoryView, 1> supported_views = {
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

    godot::Vector3 ray_origin = godot::Vector3(0.0f, 0.0f, 5.0f);
    godot::Vector3 ray_direction = godot::Vector3(0.0f, 0.0f, -1.0f);
    float max_distance = 100.0f;
    float hit_splash_radius = 0.0f;
    bool backface_cull = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("ray_origin", godot::Variant::VECTOR3, ray_origin);
        add_prop("ray_direction", godot::Variant::VECTOR3, ray_direction);
        add_prop("max_distance", godot::Variant::FLOAT, max_distance);
        add_prop("hit_splash_radius", godot::Variant::FLOAT, hit_splash_radius);
        add_prop("backface_cull", godot::Variant::BOOL, backface_cull);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("ray_origin")) ray_origin = p_props["ray_origin"];
        if (p_props.has("ray_direction")) {
            godot::Vector3 d = p_props["ray_direction"];
            ray_direction = (d.length_squared() > 1e-8f) ? d.normalized() : godot::Vector3(0.0f, 0.0f, -1.0f);
        }
        if (p_props.has("max_distance")) max_distance = std::max(1e-4f, static_cast<float>(p_props["max_distance"]));
        if (p_props.has("hit_splash_radius")) hit_splash_radius = std::max(0.0f, static_cast<float>(p_props["hit_splash_radius"]));
        if (p_props.has("backface_cull")) backface_cull = static_cast<bool>(p_props["backface_cull"]);
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

    bool moller_trumbore(
        const godot::Vector3& orig, const godot::Vector3& dir,
        const godot::Vector3& v0, const godot::Vector3& v1, const godot::Vector3& v2,
        float& r_t) const noexcept
    {
        constexpr float EPS = 1e-7f;
        const godot::Vector3 e1 = v1 - v0;
        const godot::Vector3 e2 = v2 - v0;
        const godot::Vector3 pvec = dir.cross(e2);
        const float det = e1.dot(pvec);

        if (backface_cull) {
            if (det < EPS) return false;
        } else {
            if (std::abs(det) < EPS) return false;
        }

        const float inv_det = 1.0f / det;
        const godot::Vector3 tvec = orig - v0;
        const float u = tvec.dot(pvec) * inv_det;
        if (u < 0.0f || u > 1.0f) return false;

        const godot::Vector3 qvec = tvec.cross(e1);
        const float v = dir.dot(qvec) * inv_det;
        if (v < 0.0f || u + v > 1.0f) return false;

        const float t = e2.dot(qvec) * inv_det;
        if (t < 0.0f || t > max_distance) return false;
        r_t = t;
        return true;
    }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        if (!vtx_part.raw_base_ptr) return;

        const GrantPartPOD* idx_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        MemoryBufferSelectionPOD& sel = vtx_part.selection;
        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part.element_stride);
        const int32_t n_verts = (sel.element_count > 0)
            ? static_cast<int32_t>(sel.element_count)
            : static_cast<int32_t>(vtx_part.capacity_bytes / v_stride);
        if (n_verts <= 0) return;

        auto read_v = [&](int32_t idx) noexcept -> const godot::Vector3& {
            return *reinterpret_cast<const godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(idx) * v_stride);
        };

        const godot::Vector3 dir = (ray_direction.length_squared() > 1e-8f)
            ? ray_direction.normalized()
            : godot::Vector3(0.0f, 0.0f, -1.0f);

        float closest_t = max_distance + 1.0f;
        bool found_hit = false;

        if (idx_part) {
            const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_tris = static_cast<int32_t>(((idx_part->selection.element_count > 0)
                ? idx_part->selection.element_count
                : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t))) / 3);

            for (int32_t t = 0; t < n_tris; ++t) {
                const int32_t i0 = indices[t * 3 + 0];
                const int32_t i1 = indices[t * 3 + 1];
                const int32_t i2 = indices[t * 3 + 2];
                if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;

                float hit_t = 0.0f;
                if (moller_trumbore(ray_origin, dir, read_v(i0), read_v(i1), read_v(i2), hit_t)) {
                    apply_selection_op(sel, i0, p_op);
                    apply_selection_op(sel, i1, p_op);
                    apply_selection_op(sel, i2, p_op);
                    if (hit_t < closest_t) {
                        closest_t = hit_t;
                        found_hit = true;
                    }
                }
            }
        } else {
            // Unindexed triangle soup (every 3 consecutive vertices form a triangle)
            for (int32_t i = 0; i + 2 < n_verts; i += 3) {
                float hit_t = 0.0f;
                if (moller_trumbore(ray_origin, dir, read_v(i), read_v(i + 1), read_v(i + 2), hit_t)) {
                    apply_selection_op(sel, i + 0, p_op);
                    apply_selection_op(sel, i + 1, p_op);
                    apply_selection_op(sel, i + 2, p_op);
                    if (hit_t < closest_t) {
                        closest_t = hit_t;
                        found_hit = true;
                    }
                }
            }
        }

        if (found_hit && hit_splash_radius > 0.0f) {
            const godot::Vector3 hit_pt = ray_origin + dir * closest_t;
            const float r2 = hit_splash_radius * hit_splash_radius;
            for (int32_t i = 0; i < n_verts; ++i) {
                if ((read_v(i) - hit_pt).length_squared() <= r2) {
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