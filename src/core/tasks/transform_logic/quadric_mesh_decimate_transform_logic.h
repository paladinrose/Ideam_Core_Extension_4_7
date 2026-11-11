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
 * M37: QuadricMeshDecimateTransformLogic
 * Garland-Heckbert 4x4 symmetric error quadric (Q_v = sum(p * p^T)) iterative edge-collapse
 * mesh decimator in transient workspace for dynamic LOD generation and simplified Jolt collision hulls.
 */
struct QuadricMeshDecimateTransformLogic {
    static constexpr std::string_view ui_name = "Garland-Heckbert Quadric Mesh Decimate";
    static constexpr std::string_view ui_category = "Model/Procedural";

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

    static constexpr int32_t MAX_QEM_VERTS = 8192;

    // 10 unique floats of a symmetric 4x4 matrix Q
    struct SymMat4Pod {
        float m[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        inline void add_plane(float a, float b, float c, float d) noexcept {
            m[0] += a * a; m[1] += a * b; m[2] += a * c; m[3] += a * d;
                           m[4] += b * b; m[5] += b * c; m[6] += b * d;
                                          m[7] += c * c; m[8] += c * d;
                                                         m[9] += d * d;
        }

        inline SymMat4Pod operator+(const SymMat4Pod& o) const noexcept {
            SymMat4Pod r;
            for (int i = 0; i < 10; ++i) r.m[i] = m[i] + o.m[i];
            return r;
        }

        inline SymMat4Pod& operator+=(const SymMat4Pod& o) noexcept {
            for (int i = 0; i < 10; ++i) m[i] += o.m[i];
            return *this;
        }

        inline float evaluate(const godot::Vector3& v) const noexcept {
            const float x = v.x, y = v.y, z = v.z;
            return x * x * m[0] + 2.0f * x * y * m[1] + 2.0f * x * z * m[2] + 2.0f * x * m[3] +
                   y * y * m[4] + 2.0f * y * z * m[5] + 2.0f * y * m[6] +
                   z * z * m[7] + 2.0f * z * m[8] +
                   m[9];
        }
    };

    float target_reduction_ratio = 0.5f;
    float max_error_threshold = 0.05f;
    int32_t max_collapses_per_pass = 256;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("target_reduction_ratio", godot::Variant::FLOAT, target_reduction_ratio);
        add_prop("max_error_threshold", godot::Variant::FLOAT, max_error_threshold);
        add_prop("max_collapses_per_pass", godot::Variant::INT, max_collapses_per_pass);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("target_reduction_ratio")) target_reduction_ratio = std::clamp(static_cast<float>(p_props["target_reduction_ratio"]), 0.05f, 0.99f);
        if (p_props.has("max_error_threshold")) max_error_threshold = std::max(1e-7f, static_cast<float>(p_props["max_error_threshold"]));
        if (p_props.has("max_collapses_per_pass")) max_collapses_per_pass = std::max<int32_t>(1, static_cast<int32_t>(p_props["max_collapses_per_pass"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_QEM_VERTS * (sizeof(SymMat4Pod) + sizeof(int32_t)) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_QEM_VERTS * (sizeof(SymMat4Pod) + sizeof(int32_t)) + 256;
    }

    static inline int32_t find_root(int32_t* remap, int32_t v) noexcept {
        while (remap[v] != v) {
            remap[v] = remap[remap[v]];
            v = remap[v];
        }
        return v;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2 || !p_ctx.local_workspace) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        GrantPartPOD& idx_part = p_ctx.grant->parts[1];
        if (!vtx_part.raw_base_ptr || !idx_part.raw_base_ptr) return;

        godot::Vector3* verts = reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr);
        const int32_t raw_v = (vtx_part.selection.element_count > 0)
            ? static_cast<int32_t>(vtx_part.selection.element_count)
            : static_cast<int32_t>(vtx_part.capacity_bytes / sizeof(godot::Vector3));
        const int32_t n_verts = std::min<int32_t>(raw_v, MAX_QEM_VERTS);

        int32_t* indices = reinterpret_cast<int32_t*>(idx_part.raw_base_ptr);
        const int32_t n_indices = (idx_part.selection.element_count > 0)
            ? static_cast<int32_t>(idx_part.selection.element_count)
            : static_cast<int32_t>(idx_part.capacity_bytes / sizeof(int32_t));
        const int32_t n_tris = n_indices / 3;
        if (n_verts < 4 || n_tris < 2) return;

        uint8_t* ws = static_cast<uint8_t*>(p_ctx.local_workspace);
        SymMat4Pod* quadrics = reinterpret_cast<SymMat4Pod*>(ws);
        int32_t* remap = reinterpret_cast<int32_t*>(quadrics + MAX_QEM_VERTS);

        for (int32_t i = 0; i < n_verts; ++i) {
            quadrics[i] = SymMat4Pod{};
            remap[i] = i;
        }

        // 1. Accumulate fundamental plane quadrics for all triangles
        for (int32_t t = 0; t < n_tris; ++t) {
            const int32_t i0 = indices[t * 3 + 0];
            const int32_t i1 = indices[t * 3 + 1];
            const int32_t i2 = indices[t * 3 + 2];
            if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;

            const godot::Vector3& v0 = verts[i0];
            const godot::Vector3& v1 = verts[i1];
            const godot::Vector3& v2 = verts[i2];
            const godot::Vector3 cross = (v1 - v0).cross(v2 - v0);
            const float len = cross.length();
            if (len < 1e-7f) continue;

            const godot::Vector3 n = cross / len;
            const float d = -n.dot(v0);
            quadrics[i0].add_plane(n.x, n.y, n.z, d);
            quadrics[i1].add_plane(n.x, n.y, n.z, d);
            quadrics[i2].add_plane(n.x, n.y, n.z, d);
        }

        // 2. Greedy quadric edge collapses up to target triangle count / max_collapses_per_pass
        const int32_t target_tris = std::max<int32_t>(1, static_cast<int32_t>(static_cast<float>(n_tris) * target_reduction_ratio));
        int32_t active_tris = n_tris;
        int32_t collapses = 0;

        for (int32_t pass = 0; pass < 4 && active_tris > target_tris && collapses < max_collapses_per_pass; ++pass) {
            const float pass_thresh = max_error_threshold * static_cast<float>(1 << pass);
            for (int32_t t = 0; t < n_tris && active_tris > target_tris && collapses < max_collapses_per_pass; ++t) {
                int32_t i0 = find_root(remap, std::clamp(indices[t * 3 + 0], 0, n_verts - 1));
                int32_t i1 = find_root(remap, std::clamp(indices[t * 3 + 1], 0, n_verts - 1));
                int32_t i2 = find_root(remap, std::clamp(indices[t * 3 + 2], 0, n_verts - 1));
                if (i0 == i1 || i1 == i2 || i2 == i0) continue;

                const int32_t edges[3][2] = {{i0, i1}, {i1, i2}, {i2, i0}};
                int best_e = -1;
                float best_err = pass_thresh;
                godot::Vector3 best_pos;

                for (int e = 0; e < 3; ++e) {
                    const int32_t u = edges[e][0];
                    const int32_t v = edges[e][1];
                    const SymMat4Pod q_sum = quadrics[u] + quadrics[v];

                    const godot::Vector3 mid = (verts[u] + verts[v]) * 0.5f;
                    const float err_u = q_sum.evaluate(verts[u]);
                    const float err_v = q_sum.evaluate(verts[v]);
                    const float err_m = q_sum.evaluate(mid);

                    float min_e = err_m;
                    godot::Vector3 opt_p = mid;
                    if (err_u < min_e) { min_e = err_u; opt_p = verts[u]; }
                    if (err_v < min_e) { min_e = err_v; opt_p = verts[v]; }

                    if (min_e < best_err) {
                        best_err = min_e;
                        best_e = e;
                        best_pos = opt_p;
                    }
                }

                if (best_e >= 0) {
                    const int32_t keep = edges[best_e][0];
                    const int32_t drop = edges[best_e][1];
                    verts[keep] = best_pos;
                    quadrics[keep] += quadrics[drop];
                    remap[drop] = keep;
                    --active_tris;
                    ++collapses;
                }
            }
        }

        // 3. Rewrite and compact non-degenerate triangles
        int32_t out_indices = 0;
        for (int32_t t = 0; t < n_tris; ++t) {
            const int32_t raw0 = indices[t * 3 + 0];
            const int32_t raw1 = indices[t * 3 + 1];
            const int32_t raw2 = indices[t * 3 + 2];
            if (raw0 < 0 || raw0 >= n_verts || raw1 < 0 || raw1 >= n_verts || raw2 < 0 || raw2 >= n_verts) continue;

            const int32_t i0 = find_root(remap, raw0);
            const int32_t i1 = find_root(remap, raw1);
            const int32_t i2 = find_root(remap, raw2);
            if (i0 == i1 || i1 == i2 || i2 == i0) continue;

            indices[out_indices++] = i0;
            indices[out_indices++] = i1;
            indices[out_indices++] = i2;
        }

        idx_part.selection.element_count = out_indices;
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core