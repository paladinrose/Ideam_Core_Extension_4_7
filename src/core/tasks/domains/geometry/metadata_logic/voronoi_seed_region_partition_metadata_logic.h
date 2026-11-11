#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../tasks/i_native_task.h"
#include "../../tasks/task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * GEO12: VoronoiSeedRegionPartitionMetadataLogic
 * Partitions 2D/3D spatial grids or point clouds into nearest-seed Voronoi regions,
 * writing the closest seed ID into r_selection.partition_ids, quantized distance rings
 * into r_selection.lod_levels, and optional region-border bitflags into r_selection.group_masks.
 */
struct VoronoiSeedRegionPartitionMetadataLogic {
    static constexpr std::string_view ui_name = "Voronoi Seed Region Partition";
    static constexpr std::string_view ui_category = "Geometry/Voronoi";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::STENCIL,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 3> supported_strategies = {
        MemoryStrategy::SPATIAL_2D,
        MemoryStrategy::SPATIAL_3D,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 4> supported_types = {
        DataType::VECTOR2,
        DataType::VECTOR3,
        DataType::INT32,
        DataType::FLOAT32
    };

    int32_t grid_width = 128;
    int32_t grid_height = 128;
    int32_t grid_depth = 1;
    int32_t distance_metric = 0; // 0 = Euclidean (L2), 1 = Manhattan (L1), 2 = Chebyshev (L-inf)
    float lod_ring_spacing = 8.0f;
    bool stamp_border_group_mask = true;
    int32_t border_mask_bit = 1;

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
        add_prop("grid_width", godot::Variant::INT, grid_width);
        add_prop("grid_height", godot::Variant::INT, grid_height);
        add_prop("grid_depth", godot::Variant::INT, grid_depth);
        add_prop("distance_metric", godot::Variant::INT, distance_metric, 2, "Euclidean (L2),Manhattan (L1),Chebyshev (L-inf)");
        add_prop("lod_ring_spacing", godot::Variant::FLOAT, lod_ring_spacing);
        add_prop("stamp_border_group_mask", godot::Variant::BOOL, stamp_border_group_mask);
        add_prop("border_mask_bit", godot::Variant::INT, border_mask_bit);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("grid_width")) grid_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["grid_width"]));
        if (p_props.has("grid_height")) grid_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["grid_height"]));
        if (p_props.has("grid_depth")) grid_depth = std::max<int32_t>(1, static_cast<int32_t>(p_props["grid_depth"]));
        if (p_props.has("distance_metric")) distance_metric = std::clamp<int32_t>(static_cast<int32_t>(p_props["distance_metric"]), 0, 2);
        if (p_props.has("lod_ring_spacing")) lod_ring_spacing = std::max(1e-4f, static_cast<float>(p_props["lod_ring_spacing"]));
        if (p_props.has("stamp_border_group_mask")) stamp_border_group_mask = static_cast<bool>(p_props["stamp_border_group_mask"]);
        if (p_props.has("border_mask_bit")) border_mask_bit = std::clamp<int32_t>(static_cast<int32_t>(p_props["border_mask_bit"]), 0, 31);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return 0;
    }

    size_t get_transient_requirement() const noexcept {
        return 0;
    }

    inline float eval_distance(const godot::Vector3& a, const godot::Vector3& b) const noexcept {
        const float dx = std::abs(a.x - b.x);
        const float dy = std::abs(a.y - b.y);
        const float dz = std::abs(a.z - b.z);
        if (distance_metric == 1) return dx + dy + dz;
        if (distance_metric == 2) return std::max({dx, dy, dz});
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& primary = p_ctx.grant->parts[0];
        MemoryBufferSelectionPOD& sel = primary.selection;
        if (!sel.partition_ids || sel.capacity <= 0) return;

        const GrantPartPOD* seed_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        const int32_t w = std::max<int32_t>(1, grid_width);
        const int32_t h = std::max<int32_t>(1, grid_height);
        const int32_t d = std::max<int32_t>(1, grid_depth);
        const int64_t n_elems = std::min<int64_t>(
            sel.capacity,
            (sel.element_count > 0) ? sel.element_count : static_cast<int64_t>(w) * h * d
        );

        auto read_pos = [&](const GrantPartPOD& part, int64_t idx) noexcept -> godot::Vector3 {
            if (part.raw_base_ptr && part.element_stride >= sizeof(godot::Vector3)) {
                return *reinterpret_cast<const godot::Vector3*>(part.raw_base_ptr + static_cast<size_t>(idx) * part.element_stride);
            }
            if (part.raw_base_ptr && part.element_stride >= sizeof(godot::Vector2)) {
                const godot::Vector2& v2 = *reinterpret_cast<const godot::Vector2*>(part.raw_base_ptr + static_cast<size_t>(idx) * part.element_stride);
                return godot::Vector3(v2.x, v2.y, 0.0f);
            }
            const int64_t z = idx / (static_cast<int64_t>(w) * h);
            const int64_t rem = idx % (static_cast<int64_t>(w) * h);
            const int64_t y = rem / w;
            const int64_t x = rem % w;
            return godot::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
        };

        int64_t seed_count = 0;
        if (seed_part) {
            const uint32_t s_stride = std::max<uint32_t>(sizeof(godot::Vector2), seed_part->element_stride);
            seed_count = (seed_part->selection.element_count > 0)
                ? seed_part->selection.element_count
                : static_cast<int64_t>(seed_part->capacity_bytes / s_stride);
        }

        for (int64_t i = 0; i < n_elems; ++i) {
            const godot::Vector3 p = read_pos(primary, i);
            int64_t best_id = 0;
            float best_dist = 1e30f;

            if (seed_part && seed_count > 0) {
                for (int64_t s = 0; s < seed_count; ++s) {
                    const godot::Vector3 sp = read_pos(*seed_part, s);
                    const float dist = eval_distance(p, sp);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_id = s;
                    }
                }
            } else {
                // Fallback lattice seed anchors if no separate seed buffer is bound
                const float gx = std::round(p.x / lod_ring_spacing) * lod_ring_spacing;
                const float gy = std::round(p.y / lod_ring_spacing) * lod_ring_spacing;
                const float gz = std::round(p.z / lod_ring_spacing) * lod_ring_spacing;
                best_dist = eval_distance(p, godot::Vector3(gx, gy, gz));
                best_id = (static_cast<int64_t>(gx) * 73856093LL) ^
                          (static_cast<int64_t>(gy) * 19349663LL) ^
                          (static_cast<int64_t>(gz) * 83492791LL);
            }

            sel.partition_ids[i] = best_id;
            if (sel.lod_levels) {
                const int32_t ring = std::clamp<int32_t>(static_cast<int32_t>(best_dist / lod_ring_spacing), 0, 255);
                sel.lod_levels[i] = static_cast<uint8_t>(ring);
            }
        }

        // Stamp border bitmask on cells whose 4-connected/6-connected neighbor belongs to a different partition
        if (stamp_border_group_mask && sel.group_masks && w > 1 && h > 1) {
            const uint32_t bit = (1u << border_mask_bit);
            const int64_t grid_total = std::min<int64_t>(n_elems, static_cast<int64_t>(w) * h * d);
            for (int64_t idx = 0; idx < grid_total; ++idx) {
                const int64_t pid = sel.partition_ids[idx];
                const int64_t x = idx % w;
                const int64_t y = (idx / w) % h;
                bool is_border = false;
                if (x + 1 < w && sel.partition_ids[idx + 1] != pid) is_border = true;
                else if (x > 0 && sel.partition_ids[idx - 1] != pid) is_border = true;
                else if (y + 1 < h && sel.partition_ids[idx + w] != pid) is_border = true;
                else if (y > 0 && sel.partition_ids[idx - w] != pid) is_border = true;

                if (is_border) {
                    sel.group_masks[idx] |= bit;
                } else {
                    sel.group_masks[idx] &= ~bit;
                }
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core