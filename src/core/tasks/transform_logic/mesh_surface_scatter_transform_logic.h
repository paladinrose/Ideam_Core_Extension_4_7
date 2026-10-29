#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * MeshSurfaceScatterTransformLogic<T>
 * Samples M surface points across a target mesh's Vertex + Index buffers using a
 * prefix-sum Cumulative Distribution Function (CDF) of triangle areas built in
 * local_workspace and deterministic PCG barycentric coordinates (u, v).
 * Outputs scattered positions, surface normals, orthonormal tangents (Vector4), or UVs.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshSurfaceScatterTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    // Base transient flag (>0 triggers dynamic get_transient_requirement query in TaskGraphDOD)
    static constexpr size_t transient_workspace_bytes = 64;

    static constexpr std::string_view display_name = "Mesh Surface Scatter";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t out_position_buffer_id = INVALID_ID; // 4B: Primary target buffer (scattered 3D positions)
    uint32_t out_normal_buffer_id   = INVALID_ID; // 4B: Optional output buffer for surface normals (Vector3)
    uint32_t out_tangent_buffer_id  = INVALID_ID; // 4B: Optional output buffer for surface tangents (Vector4)
    uint32_t out_uv_buffer_id       = INVALID_ID; // 4B: Optional output buffer for interpolated UVs (Vector2)

    uint32_t mesh_vertex_buffer_id  = INVALID_ID; // 4B: Source mesh vertices (Vector3)
    uint32_t mesh_index_buffer_id   = INVALID_ID; // 4B: Source mesh triangle indices (int32_t triplets)
    uint32_t mesh_normal_buffer_id  = INVALID_ID; // 4B: Optional source mesh vertex normals (Vector3)
    uint32_t mesh_uv_buffer_id      = INVALID_ID; // 4B: Optional source mesh vertex UVs (Vector2)

    float normal_offset             = 0.0f;       // 4B: Offset along surface normal (e.g., 0.005m to prevent decal z-fighting)
    uint32_t seed                   = 0x7F4A7C15u;// 4B: Deterministic PCG random seed
    bool area_weighted              = true;       // 1B: Use triangle area CDF in local_workspace for uniform spatial density
    bool animate_seed               = false;      // 1B: Shift seed per buffer_version_at_issue
    uint8_t reserved_padding[22]    = {0};        // 22B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (!area_weighted) return 0;
        const int64_t faces = _resolve_face_count(p_context);
        if (faces > 0) {
            return static_cast<size_t>(faces) * sizeof(float);
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary opos_prop;
        opos_prop["name"] = "out_position_buffer_id";
        opos_prop["type"] = godot::Variant::INT;
        opos_prop["hint_string"] = "buffer_option";
        props.push_back(opos_prop);

        godot::Dictionary onorm_prop;
        onorm_prop["name"] = "out_normal_buffer_id";
        onorm_prop["type"] = godot::Variant::INT;
        onorm_prop["hint_string"] = "buffer_option";
        props.push_back(onorm_prop);

        godot::Dictionary otan_prop;
        otan_prop["name"] = "out_tangent_buffer_id";
        otan_prop["type"] = godot::Variant::INT;
        otan_prop["hint_string"] = "buffer_option";
        props.push_back(otan_prop);

        godot::Dictionary ouv_prop;
        ouv_prop["name"] = "out_uv_buffer_id";
        ouv_prop["type"] = godot::Variant::INT;
        ouv_prop["hint_string"] = "buffer_option";
        props.push_back(ouv_prop);

        godot::Dictionary mvert_prop;
        mvert_prop["name"] = "mesh_vertex_buffer_id";
        mvert_prop["type"] = godot::Variant::INT;
        mvert_prop["hint_string"] = "buffer_option";
        props.push_back(mvert_prop);

        godot::Dictionary midx_prop;
        midx_prop["name"] = "mesh_index_buffer_id";
        midx_prop["type"] = godot::Variant::INT;
        midx_prop["hint_string"] = "buffer_option";
        props.push_back(midx_prop);

        godot::Dictionary mnorm_prop;
        mnorm_prop["name"] = "mesh_normal_buffer_id";
        mnorm_prop["type"] = godot::Variant::INT;
        mnorm_prop["hint_string"] = "buffer_option";
        props.push_back(mnorm_prop);

        godot::Dictionary muv_prop;
        muv_prop["name"] = "mesh_uv_buffer_id";
        muv_prop["type"] = godot::Variant::INT;
        muv_prop["hint_string"] = "buffer_option";
        props.push_back(muv_prop);

        godot::Dictionary off_prop;
        off_prop["name"] = "normal_offset";
        off_prop["type"] = godot::Variant::FLOAT;
        off_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        off_prop["hint_string"] = "-1.0,1.0,0.001,suffix:m";
        props.push_back(off_prop);

        godot::Dictionary seed_prop;
        seed_prop["name"] = "seed";
        seed_prop["type"] = godot::Variant::INT;
        seed_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(seed_prop);

        godot::Dictionary area_prop;
        area_prop["name"] = "area_weighted";
        area_prop["type"] = godot::Variant::BOOL;
        area_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(area_prop);

        godot::Dictionary anim_prop;
        anim_prop["name"] = "animate_seed";
        anim_prop["type"] = godot::Variant::BOOL;
        anim_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(anim_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return out_position_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("out_position_buffer_id")) {
            out_position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["out_position_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            out_position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("out_normal_buffer_id")) {
            out_normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["out_normal_buffer_id"]));
        }
        if (p_props.has("out_tangent_buffer_id")) {
            out_tangent_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["out_tangent_buffer_id"]));
        }
        if (p_props.has("out_uv_buffer_id")) {
            out_uv_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["out_uv_buffer_id"]));
        }
        if (p_props.has("mesh_vertex_buffer_id")) {
            mesh_vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mesh_vertex_buffer_id"]));
        }
        if (p_props.has("mesh_index_buffer_id")) {
            mesh_index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mesh_index_buffer_id"]));
        }
        if (p_props.has("mesh_normal_buffer_id")) {
            mesh_normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mesh_normal_buffer_id"]));
        }
        if (p_props.has("mesh_uv_buffer_id")) {
            mesh_uv_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mesh_uv_buffer_id"]));
        }
        if (p_props.has("normal_offset")) {
            normal_offset = static_cast<float>(p_props["normal_offset"]);
        }
        if (p_props.has("seed")) {
            seed = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed"]));
        }
        if (p_props.has("area_weighted")) {
            area_weighted = static_cast<bool>(p_props["area_weighted"]);
        }
        if (p_props.has("animate_seed")) {
            animate_seed = static_cast<bool>(p_props["animate_seed"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_pos_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* opos_part  = p_context.get_grant_part(out_position_buffer_id);
            const GrantPartPOD* mvert_part = p_context.get_grant_part(mesh_vertex_buffer_id);
            if (!opos_part || !mvert_part || !opos_part->raw_base_ptr || !mvert_part->raw_base_ptr) return;

            const godot::Vector3* __restrict mesh_verts = reinterpret_cast<const godot::Vector3*>(mvert_part->raw_base_ptr);
            const int64_t vert_count = mvert_part->selection.capacity;
            if (vert_count < 3) return;

            const GrantPartPOD* midx_part = (mesh_index_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(mesh_index_buffer_id)
                : nullptr;
            const int32_t* __restrict mesh_indices = (midx_part && midx_part->raw_base_ptr)
                ? reinterpret_cast<const int32_t*>(midx_part->raw_base_ptr)
                : nullptr;

            const int64_t face_count = _resolve_face_count(p_context);
            if (face_count <= 0) return;

            const GrantPartPOD* mnorm_part = (mesh_normal_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(mesh_normal_buffer_id)
                : nullptr;
            const godot::Vector3* __restrict mesh_normals = (mnorm_part && mnorm_part->raw_base_ptr && mnorm_part->selection.capacity >= vert_count)
                ? reinterpret_cast<const godot::Vector3*>(mnorm_part->raw_base_ptr)
                : nullptr;

            const GrantPartPOD* muv_part = (mesh_uv_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(mesh_uv_buffer_id)
                : nullptr;
            const godot::Vector2* __restrict mesh_uvs = (muv_part && muv_part->raw_base_ptr && muv_part->selection.capacity >= vert_count)
                ? reinterpret_cast<const godot::Vector2*>(muv_part->raw_base_ptr)
                : nullptr;

            // Optional secondary output buffers
            godot::Vector3* __restrict out_normals = nullptr;
            int64_t out_norm_cap = 0;
            if (out_normal_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* onorm_part = p_context.get_grant_part(out_normal_buffer_id)) {
                    out_normals  = reinterpret_cast<godot::Vector3*>(onorm_part->raw_base_ptr);
                    out_norm_cap = static_cast<int64_t>(onorm_part->capacity_bytes / sizeof(godot::Vector3));
                }
            }

            godot::Vector4* __restrict out_tangents = nullptr;
            int64_t out_tan_cap = 0;
            if (out_tangent_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* otan_part = p_context.get_grant_part(out_tangent_buffer_id)) {
                    out_tangents = reinterpret_cast<godot::Vector4*>(otan_part->raw_base_ptr);
                    out_tan_cap  = static_cast<int64_t>(otan_part->capacity_bytes / sizeof(godot::Vector4));
                }
            }

            godot::Vector2* __restrict out_uvs = nullptr;
            int64_t out_uv_cap = 0;
            if (out_uv_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* ouv_part = p_context.get_grant_part(out_uv_buffer_id)) {
                    out_uvs    = reinterpret_cast<godot::Vector2*>(ouv_part->raw_base_ptr);
                    out_uv_cap = static_cast<int64_t>(ouv_part->capacity_bytes / sizeof(godot::Vector2));
                }
            }

            // 1. Build prefix-sum triangle area CDF in local_workspace if available
            float* __restrict cdf_table = (area_weighted && p_context.local_workspace)
                ? reinterpret_cast<float*>(p_context.local_workspace)
                : nullptr;
            float total_area = 0.0f;

            if (cdf_table) {
                for (int64_t f = 0; f < face_count; ++f) {
                    const int64_t base_i = f * 3;
                    const int32_t i0 = mesh_indices ? mesh_indices[base_i]     : static_cast<int32_t>(base_i);
                    const int32_t i1 = mesh_indices ? mesh_indices[base_i + 1] : static_cast<int32_t>(base_i + 1);
                    const int32_t i2 = mesh_indices ? mesh_indices[base_i + 2] : static_cast<int32_t>(base_i + 2);

                    float area = 0.0f;
                    if (i0 >= 0 && i1 >= 0 && i2 >= 0 && i0 < vert_count && i1 < vert_count && i2 < vert_count) {
                        const godot::Vector3 e1 = mesh_verts[i1] - mesh_verts[i0];
                        const godot::Vector3 e2 = mesh_verts[i2] - mesh_verts[i0];
                        area = 0.5f * e2.cross(e1).length();
                    }
                    total_area += area;
                    cdf_table[f] = total_area;
                }
            }

            const uint32_t active_seed = animate_seed
                ? (seed ^ (opos_part->buffer_version_at_issue * 0x1B873593u))
                : seed;

            auto scatter_sample = [&](int64_t sample_idx) noexcept {
                uint32_t rng_state = active_seed ^ static_cast<uint32_t>(sample_idx * 0x9E3779B9u + 0x7F4A7C15u);

                // Select triangle face (via binary search on area CDF or uniform fallback)
                int64_t f = 0;
                if (cdf_table && total_area > 1e-12f) {
                    const float target_area = _pcg_unit_float(rng_state) * total_area;
                    const float* it = std::lower_bound(cdf_table, cdf_table + face_count, target_area);
                    f = std::clamp<int64_t>(static_cast<int64_t>(it - cdf_table), 0, face_count - 1);
                } else {
                    const float u_face = _pcg_unit_float(rng_state);
                    f = std::clamp<int64_t>(static_cast<int64_t>(u_face * static_cast<float>(face_count)), 0, face_count - 1);
                }

                // Generate uniform barycentric coordinates via Turk's square-root folding
                const float r1 = _pcg_unit_float(rng_state);
                const float r2 = _pcg_unit_float(rng_state);
                const float sqrt_r1 = std::sqrt(r1);
                const float b0 = 1.0f - sqrt_r1;
                const float b1 = sqrt_r1 * (1.0f - r2);
                const float b2 = sqrt_r1 * r2;

                const int64_t base_i = f * 3;
                const int32_t i0 = mesh_indices ? mesh_indices[base_i]     : static_cast<int32_t>(base_i);
                const int32_t i1 = mesh_indices ? mesh_indices[base_i + 1] : static_cast<int32_t>(base_i + 1);
                const int32_t i2 = mesh_indices ? mesh_indices[base_i + 2] : static_cast<int32_t>(base_i + 2);

                if (i0 < 0 || i1 < 0 || i2 < 0 || i0 >= vert_count || i1 >= vert_count || i2 >= vert_count) return;

                const godot::Vector3& v0 = mesh_verts[i0];
                const godot::Vector3& v1 = mesh_verts[i1];
                const godot::Vector3& v2 = mesh_verts[i2];

                const godot::Vector3 e1 = v1 - v0;
                const godot::Vector3 e2 = v2 - v0;

                godot::Vector3 interp_norm;
                if (mesh_normals) {
                    interp_norm = (mesh_normals[i0] * b0 + mesh_normals[i1] * b1 + mesh_normals[i2] * b2).normalized();
                } else {
                    interp_norm = e2.cross(e1).normalized();
                }
                if (interp_norm.length_squared() < 1e-8f) {
                    interp_norm = godot::Vector3(0.0f, 1.0f, 0.0f);
                }

                const godot::Vector3 sample_pos = (v0 * b0 + v1 * b1 + v2 * b2) + (interp_norm * normal_offset);
                _write_view(p_pos_view, sample_idx, sample_pos);

                if (out_normals && sample_idx < out_norm_cap) {
                    out_normals[sample_idx] = interp_norm;
                }

                if (out_tangents && sample_idx < out_tan_cap) {
                    godot::Vector3 tan_dir = e1 - interp_norm * interp_norm.dot(e1);
                    if (tan_dir.length_squared() > 1e-8f) {
                        tan_dir = tan_dir.normalized();
                    } else {
                        const godot::Vector3 ref_axis = (std::abs(interp_norm.x) < 0.9f)
                            ? godot::Vector3(1.0f, 0.0f, 0.0f)
                            : godot::Vector3(0.0f, 1.0f, 0.0f);
                        tan_dir = interp_norm.cross(ref_axis).normalized();
                    }
                    out_tangents[sample_idx] = godot::Vector4(tan_dir.x, tan_dir.y, tan_dir.z, 1.0f);
                }

                if (out_uvs && mesh_uvs && sample_idx < out_uv_cap) {
                    out_uvs[sample_idx] = godot::Vector2(
                        mesh_uvs[i0].x * b0 + mesh_uvs[i1].x * b1 + mesh_uvs[i2].x * b2,
                        mesh_uvs[i0].y * b0 + mesh_uvs[i1].y * b1 + mesh_uvs[i2].y * b2
                    );
                }
            };

            const MemoryBufferSelectionPOD& sel = opos_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) scatter_sample(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    scatter_sample(sel.data.indices[i]);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end; ++i) {
                    scatter_sample(i);
                }
            }
        }
    }

private:
    [[nodiscard]] inline int64_t _resolve_face_count(const TaskContextPOD& p_context) const noexcept {
        if (mesh_index_buffer_id != INVALID_ID) {
            if (const GrantPartPOD* idx_part = p_context.get_grant_part(mesh_index_buffer_id)) {
                const int64_t idx_count = idx_part->selection.element_count > 0
                    ? idx_part->selection.element_count
                    : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t));
                return idx_count / 3;
            }
        }
        if (mesh_vertex_buffer_id != INVALID_ID) {
            if (const GrantPartPOD* vert_part = p_context.get_grant_part(mesh_vertex_buffer_id)) {
                const int64_t vert_count = vert_part->selection.element_count > 0
                    ? vert_part->selection.element_count
                    : vert_part->selection.capacity;
                return vert_count / 3;
            }
        }
        return 0;
    }

    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline float _pcg_unit_float(uint32_t& r_state) noexcept {
        r_state = r_state * 747796405u + 2891336453u;
        uint32_t word = ((r_state >> ((r_state >> 28u) + 4u)) ^ r_state) * 277803737u;
        word = (word >> 22u) ^ word;
        // Map 23 mantissa bits into [1.0f, 2.0f) and subtract 1.0f -> [0.0f, 1.0f)
        const uint32_t bits = (word & 0x007FFFFFu) | 0x3F800000u;
        return std::bit_cast<float>(bits) - 1.0f;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _write_view(const T_View& p_view, int64_t idx, const T& value) noexcept {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<T*>(p_view[idx]) = value;
        } else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value;
        } else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } else {
            (void)(p_view[idx] = value);
        }
    }
};

static_assert(sizeof(MeshSurfaceScatterTransformLogic<>) == 64, "MeshSurfaceScatterTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core