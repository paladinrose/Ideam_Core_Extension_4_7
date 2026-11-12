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
 * VoronoiMeshFractureTransformLogic<T> (M27)
 * Partitions and fractures 3D meshes against localized 3D Voronoi cells around impact points,
 * assigning shard IDs into r_selection.partition_ids, splitting straddling triangles across cell
 * bisectors, and radially separating Voronoi fragments.
 */
template <typename T = godot::Vector3>
struct alignas(64) VoronoiMeshFractureTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr int32_t MAX_VORONOI_SEEDS = 64;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::INT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_VORONOI_SEEDS * sizeof(godot::Vector3) + 256;

    static constexpr std::string_view display_name = "Voronoi 3D Mesh Fracture";
    static constexpr std::string_view ui_category  = "Model/Destruction";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id = INVALID_ID; // 4B: Primary target vertex buffer (Vector3)
    uint32_t index_buffer_id  = INVALID_ID; // 4B: Optional triangle index buffer (int32_t)
    uint32_t seed_buffer_id   = INVALID_ID; // 4B: Optional external Voronoi seed points buffer (Vector3)

    godot::Vector3 impact_center{0.0f, 0.0f, 0.0f}; // 12B: Local-space fracture center

    float fracture_radius        = 2.5f;  // 4B: Radial fracture zone radius (meters)
    float explosion_separation   = 0.08f; // 4B: Outward radial shard separation offset (meters)
    int32_t seed_count           = 12;    // 4B: Number of procedural Voronoi cells [2..64]
    int32_t random_seed          = 42;    // 4B: Deterministic PRNG seed
    uint8_t reserved_padding[24] = {0};   // 24B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& /*p_context*/) const noexcept {
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

        godot::Dictionary seed_buf_prop;
        seed_buf_prop["name"] = "seed_buffer_id";
        seed_buf_prop["type"] = godot::Variant::INT;
        seed_buf_prop["hint_string"] = "buffer_option";
        props.push_back(seed_buf_prop);

        godot::Dictionary ctr_prop;
        ctr_prop["name"] = "impact_center";
        ctr_prop["type"] = godot::Variant::VECTOR3;
        ctr_prop["hint_string"] = "suffix:m";
        props.push_back(ctr_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "fracture_radius";
        rad_prop["type"] = godot::Variant::FLOAT;
        rad_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rad_prop["hint_string"] = "0.01,50.0,0.01,suffix:m";
        props.push_back(rad_prop);

        godot::Dictionary cnt_prop;
        cnt_prop["name"] = "seed_count";
        cnt_prop["type"] = godot::Variant::INT;
        cnt_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        cnt_prop["hint_string"] = "2,64,1";
        props.push_back(cnt_prop);

        godot::Dictionary rng_prop;
        rng_prop["name"] = "random_seed";
        rng_prop["type"] = godot::Variant::INT;
        props.push_back(rng_prop);

        godot::Dictionary sep_prop;
        sep_prop["name"] = "explosion_separation";
        sep_prop["type"] = godot::Variant::FLOAT;
        sep_prop["hint_string"] = "suffix:m";
        props.push_back(sep_prop);

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
        if (p_props.has("seed_buffer_id")) {
            seed_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_buffer_id"]));
        }
        if (p_props.has("impact_center")) {
            impact_center = static_cast<godot::Vector3>(p_props["impact_center"]);
        }
        if (p_props.has("fracture_radius")) {
            fracture_radius = std::max(1e-4f, static_cast<float>(p_props["fracture_radius"]));
        }
        if (p_props.has("seed_count")) {
            seed_count = std::clamp<int32_t>(static_cast<int32_t>(p_props["seed_count"]), 2, MAX_VORONOI_SEEDS);
        }
        if (p_props.has("random_seed")) {
            random_seed = static_cast<int32_t>(p_props["random_seed"]);
        }
        if (p_props.has("explosion_separation")) {
            explosion_separation = std::max(0.0f, static_cast<float>(p_props["explosion_separation"]));
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
            if (!vtx_part || !vtx_part->raw_base_ptr) return;

            GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_context.get_grant_part(index_buffer_id))
                : nullptr;
            const GrantPartPOD* ext_seed_part = (seed_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(seed_buffer_id)
                : nullptr;

            godot::Vector3* seeds = reinterpret_cast<godot::Vector3*>(p_context.local_workspace);
            int32_t n_seeds = std::clamp<int32_t>(seed_count, 2, MAX_VORONOI_SEEDS);

            if (ext_seed_part && ext_seed_part->raw_base_ptr && ext_seed_part->capacity_bytes >= sizeof(godot::Vector3) * 2) {
                const int32_t avail = (ext_seed_part->selection.element_count > 0)
                    ? static_cast<int32_t>(ext_seed_part->selection.element_count)
                    : static_cast<int32_t>(ext_seed_part->capacity_bytes / sizeof(godot::Vector3));
                n_seeds = std::clamp<int32_t>(avail, 2, MAX_VORONOI_SEEDS);
                const auto* src_s = reinterpret_cast<const godot::Vector3*>(ext_seed_part->raw_base_ptr);
                for (int32_t s = 0; s < n_seeds; ++s) seeds[s] = src_s[s];
            } else {
                const uint32_t rng = static_cast<uint32_t>(random_seed);
                for (int32_t s = 0; s < n_seeds; ++s) {
                    const float rx = _hash_unit_float(rng + static_cast<uint32_t>(s * 3 + 0)) * 2.0f - 1.0f;
                    const float ry = _hash_unit_float(rng + static_cast<uint32_t>(s * 3 + 1)) * 2.0f - 1.0f;
                    const float rz = _hash_unit_float(rng + static_cast<uint32_t>(s * 3 + 2)) * 2.0f - 1.0f;
                    godot::Vector3 dir(rx, ry, rz);
                    const float len = dir.length();
                    if (len > 1e-5f) dir /= len;
                    const float r_frac = std::cbrt(_hash_unit_float(rng ^ static_cast<uint32_t>(s * 97 + 13)));
                    seeds[s] = impact_center + dir * (fracture_radius * r_frac);
                }
            }

            const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part->element_stride);
            const int32_t max_verts = static_cast<int32_t>(vtx_part->capacity_bytes / v_stride);
            int32_t cur_verts = (vtx_part->selection.element_count > 0)
                ? std::min<int32_t>(static_cast<int32_t>(vtx_part->selection.element_count), max_verts)
                : max_verts;

            auto nearest_seed = [&](const godot::Vector3& p) noexcept -> int32_t {
                int32_t best_s = 0;
                float best_d2 = 1e30f;
                for (int32_t s = 0; s < n_seeds; ++s) {
                    const float d2 = (p - seeds[s]).length_squared();
                    if (d2 < best_d2) {
                        best_d2 = d2;
                        best_s = s;
                    }
                }
                return best_s;
            };

            const float r2 = fracture_radius * fracture_radius;

            for (int32_t i = 0; i < cur_verts; ++i) {
                godot::Vector3& v = *reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);
                const float dist2 = (v - impact_center).length_squared();
                if (dist2 > r2) {
                    if (vtx_part->selection.partition_ids && i < vtx_part->selection.capacity) {
                        vtx_part->selection.partition_ids[i] = -1;
                    }
                    continue;
                }

                const int32_t shard_id = nearest_seed(v);
                if (vtx_part->selection.partition_ids && i < vtx_part->selection.capacity) {
                    vtx_part->selection.partition_ids[i] = shard_id;
                }

                if (explosion_separation > 0.0f) {
                    const godot::Vector3 shard_dir = (seeds[shard_id] - impact_center).normalized();
                    v += shard_dir * explosion_separation;
                }
            }

            if (idx_part && idx_part->raw_base_ptr && vtx_part->selection.partition_ids) {
                int32_t* indices = reinterpret_cast<int32_t*>(idx_part->raw_base_ptr);
                const int32_t n_indices = (idx_part->selection.element_count > 0)
                    ? static_cast<int32_t>(idx_part->selection.element_count)
                    : static_cast<int32_t>(idx_part->capacity_bytes / sizeof(int32_t));
                const int32_t n_tris = n_indices / 3;
                const int32_t orig_verts = cur_verts;

                for (int32_t t = 0; t < n_tris; ++t) {
                    const int32_t i0 = indices[t * 3 + 0];
                    const int32_t i1 = indices[t * 3 + 1];
                    const int32_t i2 = indices[t * 3 + 2];
                    if (i0 < 0 || i0 >= orig_verts || i1 < 0 || i1 >= orig_verts || i2 < 0 || i2 >= orig_verts) continue;

                    const int64_t p0 = vtx_part->selection.partition_ids[i0];
                    const int64_t p1 = vtx_part->selection.partition_ids[i1];
                    const int64_t p2 = vtx_part->selection.partition_ids[i2];
                    if (p0 < 0 || (p0 == p1 && p1 == p2)) continue;

                    const godot::Vector3& v0 = *reinterpret_cast<const godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(i0) * v_stride);
                    const godot::Vector3& v1 = *reinterpret_cast<const godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(i1) * v_stride);
                    const godot::Vector3& v2 = *reinterpret_cast<const godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(i2) * v_stride);
                    const int32_t tri_shard = nearest_seed((v0 + v1 + v2) / 3.0f);

                    const int32_t tri_idx[3] = { i0, i1, i2 };
                    for (int k = 0; k < 3; ++k) {
                        const int32_t vi = tri_idx[k];
                        if (vtx_part->selection.partition_ids[vi] != tri_shard && cur_verts < max_verts) {
                            const int32_t dup_idx = cur_verts++;
                            const godot::Vector3 orig_v = *reinterpret_cast<const godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(vi) * v_stride);
                            const int64_t old_shard = vtx_part->selection.partition_ids[vi];
                            godot::Vector3 base_v = orig_v;
                            if (old_shard >= 0 && old_shard < n_seeds && explosion_separation > 0.0f) {
                                base_v -= (seeds[old_shard] - impact_center).normalized() * explosion_separation;
                            }
                            *reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(dup_idx) * v_stride) =
                                base_v + (seeds[tri_shard] - impact_center).normalized() * explosion_separation;
                            if (dup_idx < vtx_part->selection.capacity) {
                                vtx_part->selection.partition_ids[dup_idx] = tri_shard;
                            }
                            indices[t * 3 + k] = dup_idx;
                        }
                    }
                }
                vtx_part->selection.element_count = cur_verts;
            }
        }
    }

private:
    static inline float _hash_unit_float(uint32_t p_x) noexcept {
        p_x ^= p_x >> 16;
        p_x *= 0x7feb352du;
        p_x ^= p_x >> 15;
        p_x *= 0x846ca68bu;
        p_x ^= p_x >> 16;
        return static_cast<float>(p_x & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }
};

static_assert(sizeof(VoronoiMeshFractureTransformLogic<godot::Vector3>) == 64, "VoronoiMeshFractureTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<VoronoiMeshFractureTransformLogic<godot::Vector3>>, "VoronoiMeshFractureTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core