#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * QuadricMeshDecimateTransformLogic<T> (M37)
 * Garland-Heckbert 4x4 symmetric error quadric (Q_v = sum(p * p^T)) iterative edge-collapse
 * mesh decimator in transient workspace for dynamic LOD generation and simplified Jolt collision hulls.
 */
template <typename T = godot::Vector3>
struct alignas(64) QuadricMeshDecimateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr int32_t MAX_QEM_VERTS = 8192;

    // 10 unique floats of a symmetric 4x4 error quadric matrix Q
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

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::INT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_QEM_VERTS * (sizeof(SymMat4Pod) + sizeof(int32_t)) + 256;

    static constexpr std::string_view display_name = "Garland-Heckbert Quadric Mesh Decimate";
    static constexpr std::string_view ui_category  = "Model/Procedural";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id      = INVALID_ID; // 4B: Primary target vertex buffer (Vector3)
    uint32_t index_buffer_id       = INVALID_ID; // 4B: Triangle index buffer (int32_t triplets)
    float target_reduction_ratio   = 0.5f;       // 4B: Target remaining triangle ratio in [0.05, 0.99]
    float max_error_threshold      = 0.05f;      // 4B: Maximum quadric collapse error
    int32_t max_collapses_per_pass = 256;        // 4B: Cap on edge collapses per execution pass
    uint8_t reserved_padding[44]   = {0};        // 44B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (const GrantPartPOD* vtx_part = p_context.get_grant_part(vertex_buffer_id)) {
            if (vtx_part->selection.capacity > 0) {
                const size_t n = std::min<size_t>(static_cast<size_t>(vtx_part->selection.capacity), MAX_QEM_VERTS);
                return n * (sizeof(SymMat4Pod) + sizeof(int32_t)) + 256;
            }
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vtx_prop;
        vtx_prop["name"] = "vertex_buffer_id";
        vtx_prop["type"] = godot::Variant::INT;
        vtx_prop["hint_string"] = "buffer_option";
        props.push_back(vtx_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary rat_prop;
        rat_prop["name"] = "target_reduction_ratio";
        rat_prop["type"] = godot::Variant::FLOAT;
        rat_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rat_prop["hint_string"] = "0.05,0.99,0.01";
        props.push_back(rat_prop);

        godot::Dictionary err_prop;
        err_prop["name"] = "max_error_threshold";
        err_prop["type"] = godot::Variant::FLOAT;
        err_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        err_prop["hint_string"] = "0.0001,10.0,0.001";
        props.push_back(err_prop);

        godot::Dictionary col_prop;
        col_prop["name"] = "max_collapses_per_pass";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        col_prop["hint_string"] = "1,4096,1";
        props.push_back(col_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (vertex_buffer_id != INVALID_ID) ? vertex_buffer_id : index_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("target_reduction_ratio")) {
            target_reduction_ratio = std::clamp(static_cast<float>(p_props["target_reduction_ratio"]), 0.05f, 0.99f);
        }
        if (p_props.has("max_error_threshold")) {
            max_error_threshold = std::max(1e-7f, static_cast<float>(p_props["max_error_threshold"]));
        }
        if (p_props.has("max_collapses_per_pass")) {
            max_collapses_per_pass = std::max<int32_t>(1, static_cast<int32_t>(p_props["max_collapses_per_pass"]));
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, int32_t>) {
            if (!p_context.local_workspace) return;

            GrantPartPOD* vtx_part = const_cast<GrantPartPOD*>(p_context.get_grant_part(vertex_buffer_id));
            GrantPartPOD* idx_part = const_cast<GrantPartPOD*>(p_context.get_grant_part(index_buffer_id));
            if (!vtx_part || !idx_part || !vtx_part->raw_base_ptr || !idx_part->raw_base_ptr) return;

            godot::Vector3* __restrict verts = reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr);
            const int32_t raw_v = (vtx_part->selection.element_count > 0)
                ? static_cast<int32_t>(vtx_part->selection.element_count)
                : static_cast<int32_t>(vtx_part->capacity_bytes / sizeof(godot::Vector3));
            const int32_t n_verts = std::min<int32_t>(raw_v, MAX_QEM_VERTS);

            int32_t* __restrict indices = reinterpret_cast<int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_indices = (idx_part->selection.element_count > 0)
                ? static_cast<int32_t>(idx_part->selection.element_count)
                : static_cast<int32_t>(idx_part->capacity_bytes / sizeof(int32_t));
            const int32_t n_tris = n_indices / 3;
            if (n_verts < 4 || n_tris < 2) return;

            uint8_t* ws = static_cast<uint8_t*>(p_context.local_workspace);
            SymMat4Pod* quadrics = reinterpret_cast<SymMat4Pod*>(ws);
            int32_t* remap = reinterpret_cast<int32_t*>(quadrics + n_verts);

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
                    const int32_t i0 = _find_root(remap, std::clamp(indices[t * 3 + 0], 0, n_verts - 1));
                    const int32_t i1 = _find_root(remap, std::clamp(indices[t * 3 + 1], 0, n_verts - 1));
                    const int32_t i2 = _find_root(remap, std::clamp(indices[t * 3 + 2], 0, n_verts - 1));
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

                const int32_t i0 = _find_root(remap, raw0);
                const int32_t i1 = _find_root(remap, raw1);
                const int32_t i2 = _find_root(remap, raw2);
                if (i0 == i1 || i1 == i2 || i2 == i0) continue;

                indices[out_indices++] = i0;
                indices[out_indices++] = i1;
                indices[out_indices++] = i2;
            }

            idx_part->selection.element_count = out_indices;
        }
    }

private:
    static inline int32_t _find_root(int32_t* p_remap, int32_t p_v) noexcept {
        while (p_remap[p_v] != p_v) {
            p_remap[p_v] = p_remap[p_remap[p_v]];
            p_v = p_remap[p_v];
        }
        return p_v;
    }
};

static_assert(sizeof(QuadricMeshDecimateTransformLogic<godot::Vector3>) == 64, "QuadricMeshDecimateTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<QuadricMeshDecimateTransformLogic<godot::Vector3>>, "QuadricMeshDecimateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core