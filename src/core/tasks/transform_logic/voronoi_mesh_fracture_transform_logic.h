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
 * M27: VoronoiMeshFractureTransformLogic
 * Partitions and fractures 3D meshes against localized 3D Voronoi cells around impact points,
 * assigning shard IDs into r_selection.partition_ids, splitting straddling triangles across cell
 * bisectors, and radially separating Voronoi fragments.
 */
struct VoronoiMeshFractureTransformLogic {
    static constexpr std::string_view ui_name = "Voronoi 3D Mesh Fracture";
    static constexpr std::string_view ui_category = "Model/Destruction";

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

    static constexpr int32_t MAX_VORONOI_SEEDS = 64;

    godot::Vector3 impact_center = godot::Vector3(0.0f, 0.0f, 0.0f);
    float fracture_radius = 2.5f;
    int32_t seed_count = 12;
    int32_t random_seed = 42;
    float explosion_separation = 0.08f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("impact_center", godot::Variant::VECTOR3, impact_center);
        add_prop("fracture_radius", godot::Variant::FLOAT, fracture_radius);
        add_prop("seed_count", godot::Variant::INT, seed_count);
        add_prop("random_seed", godot::Variant::INT, random_seed);
        add_prop("explosion_separation", godot::Variant::FLOAT, explosion_separation);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("impact_center")) impact_center = p_props["impact_center"];
        if (p_props.has("fracture_radius")) fracture_radius = std::max(1e-4f, static_cast<float>(p_props["fracture_radius"]));
        if (p_props.has("seed_count")) seed_count = std::clamp<int32_t>(static_cast<int32_t>(p_props["seed_count"]), 2, MAX_VORONOI_SEEDS);
        if (p_props.has("random_seed")) random_seed = static_cast<int32_t>(p_props["random_seed"]);
        if (p_props.has("explosion_separation")) explosion_separation = std::max(0.0f, static_cast<float>(p_props["explosion_separation"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_VORONOI_SEEDS * sizeof(godot::Vector3) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_VORONOI_SEEDS * sizeof(godot::Vector3) + 256;
    }

    static inline float hash_unit_float(uint32_t x) noexcept {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return static_cast<float>(x & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !p_ctx.local_workspace) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        if (!vtx_part.raw_base_ptr) return;

        GrantPartPOD* idx_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;
        const GrantPartPOD* ext_seed_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        godot::Vector3* seeds = reinterpret_cast<godot::Vector3*>(p_ctx.local_workspace);
        int32_t n_seeds = std::clamp<int32_t>(seed_count, 2, MAX_VORONOI_SEEDS);

        if (ext_seed_part && ext_seed_part->capacity_bytes >= sizeof(godot::Vector3) * 2) {
            const int32_t avail = (ext_seed_part->selection.element_count > 0)
                ? static_cast<int32_t>(ext_seed_part->selection.element_count)
                : static_cast<int32_t>(ext_seed_part->capacity_bytes / sizeof(godot::Vector3));
            n_seeds = std::min<int32_t>(avail, MAX_VORONOI_SEEDS);
            const auto* src_s = reinterpret_cast<const godot::Vector3*>(ext_seed_part->raw_base_ptr);
            for (int32_t s = 0; s < n_seeds; ++s) seeds[s] = src_s[s];
        } else {
            // Generate clustered spherical Halton/hash Voronoi seeds around impact_center
            uint32_t rng = static_cast<uint32_t>(random_seed);
            for (int32_t s = 0; s < n_seeds; ++s) {
                const float rx = hash_unit_float(rng + static_cast<uint32_t>(s * 3 + 0)) * 2.0f - 1.0f;
                const float ry = hash_unit_float(rng + static_cast<uint32_t>(s * 3 + 1)) * 2.0f - 1.0f;
                const float rz = hash_unit_float(rng + static_cast<uint32_t>(s * 3 + 2)) * 2.0f - 1.0f;
                godot::Vector3 dir(rx, ry, rz);
                const float len = dir.length();
                if (len > 1e-5f) dir /= len;
                const float r_frac = std::cbrt(hash_unit_float(rng ^ static_cast<uint32_t>(s * 97 + 13)));
                seeds[s] = impact_center + dir * (fracture_radius * r_frac);
            }
        }

        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part.element_stride);
        const int32_t max_verts = static_cast<int32_t>(vtx_part.capacity_bytes / v_stride);
        int32_t cur_verts = (vtx_part.selection.element_count > 0)
            ? static_cast<int32_t>(vtx_part.selection.element_count)
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

        // Assign each vertex within fracture_radius to its Voronoi shard and displace radially
        for (int32_t i = 0; i < cur_verts; ++i) {
            godot::Vector3& v = *reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i) * v_stride);
            const float dist2 = (v - impact_center).length_squared();
            if (dist2 > r2) {
                if (vtx_part.selection.partition_ids && i < vtx_part.selection.capacity) {
                    vtx_part.selection.partition_ids[i] = -1;
                }
                continue;
            }

            const int32_t shard_id = nearest_seed(v);
            if (vtx_part.selection.partition_ids && i < vtx_part.selection.capacity) {
                vtx_part.selection.partition_ids[i] = shard_id;
            }

            if (explosion_separation > 0.0f) {
                const godot::Vector3 shard_dir = (seeds[shard_id] - impact_center).normalized();
                v += shard_dir * explosion_separation;
            }
        }

        // Duplicate shared boundary vertices on triangles straddling two different Voronoi shards
        if (idx_part && vtx_part.selection.partition_ids) {
            int32_t* indices = reinterpret_cast<int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_indices = (idx_part->selection.element_count > 0)
                ? static_cast<int32_t>(idx_part->selection.element_count)
                : static_cast<int32_t>(idx_part->capacity_bytes / sizeof(int32_t));
            const int32_t n_tris = n_indices / 3;

            for (int32_t t = 0; t < n_tris; ++t) {
                const int32_t i0 = indices[t * 3 + 0];
                const int32_t i1 = indices[t * 3 + 1];
                const int32_t i2 = indices[t * 3 + 2];
                if (i0 < 0 || i0 >= cur_verts || i1 < 0 || i1 >= cur_verts || i2 < 0 || i2 >= cur_verts) continue;

                const int64_t p0 = vtx_part.selection.partition_ids[i0];
                const int64_t p1 = vtx_part.selection.partition_ids[i1];
                const int64_t p2 = vtx_part.selection.partition_ids[i2];
                if (p0 < 0 || (p0 == p1 && p1 == p2)) continue;

                // Snap triangle vertices to the dominant shard of the triangle centroid
                const godot::Vector3& v0 = *reinterpret_cast<const godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i0) * v_stride);
                const godot::Vector3& v1 = *reinterpret_cast<const godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i1) * v_stride);
                const godot::Vector3& v2 = *reinterpret_cast<const godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i2) * v_stride);
                const int32_t tri_shard = nearest_seed((v0 + v1 + v2) / 3.0f);

                int32_t tri_idx[3] = { i0, i1, i2 };
                for (int k = 0; k < 3; ++k) {
                    const int32_t vi = tri_idx[k];
                    if (vtx_part.selection.partition_ids[vi] != tri_shard && cur_verts < max_verts) {
                        const int32_t dup_idx = cur_verts++;
                        const godot::Vector3 orig_v = *reinterpret_cast<const godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(vi) * v_stride);
                        const int64_t old_shard = vtx_part.selection.partition_ids[vi];
                        godot::Vector3 base_v = orig_v;
                        if (old_shard >= 0 && old_shard < n_seeds && explosion_separation > 0.0f) {
                            base_v -= (seeds[old_shard] - impact_center).normalized() * explosion_separation;
                        }
                        *reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(dup_idx) * v_stride) =
                            base_v + (seeds[tri_shard] - impact_center).normalized() * explosion_separation;
                        if (dup_idx < vtx_part.selection.capacity) {
                            vtx_part.selection.partition_ids[dup_idx] = tri_shard;
                        }
                        indices[t * 3 + k] = dup_idx;
                    }
                }
            }
            vtx_part.selection.element_count = cur_verts;
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core