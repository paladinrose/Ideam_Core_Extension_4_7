#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/stencil_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * VoronoiSeedRegionPartitionMetadataLogic<T, T_Strategy, T_View> (GEO12)
 * Partitions 2D/3D spatial grids or point clouds into nearest-seed Voronoi regions,
 * writing the closest seed ID into r_selection.partition_ids, quantized distance rings
 * into r_selection.lod_levels, and optional region-border bitflags into r_selection.group_masks.
 */
template <
    typename T = godot::Vector2,
    typename T_Strategy = Spatial2DStrategy,
    template <typename, typename> class T_View = StencilView
>
struct alignas(64) VoronoiSeedRegionPartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities =
        ViewCapability::LINEAR_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types =
        DataType::VECTOR2 | DataType::VECTOR3 | DataType::INT32 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Voronoi Seed Region Partition";
    static constexpr std::string_view ui_category  = "Geometry/Voronoi";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id      = INVALID_ID; // 4B
    uint32_t seed_buffer_id        = INVALID_ID; // 4B
    int32_t grid_width             = 128;        // 4B
    int32_t grid_height            = 128;        // 4B -> 16B
    int32_t grid_depth             = 1;          // 4B
    int32_t distance_metric        = 0;          // 4B: 0 = Euclidean (L2), 1 = Manhattan (L1), 2 = Chebyshev (L-inf)
    float lod_ring_spacing         = 8.0f;       // 4B
    int32_t border_mask_bit        = 1;          // 4B -> 32B
    bool stamp_border_group_mask   = true;       // 1B
    uint8_t reserved_padding[31]   = {0};        // 31B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("seed_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("grid_width", godot::Variant::INT, 128);
        add_prop("grid_height", godot::Variant::INT, 128);
        add_prop("grid_depth", godot::Variant::INT, 1);
        add_prop("distance_metric", godot::Variant::INT, 0, 2, "Euclidean (L2),Manhattan (L1),Chebyshev (L-inf)");
        add_prop("lod_ring_spacing", godot::Variant::FLOAT, 8.0f);
        add_prop("stamp_border_group_mask", godot::Variant::BOOL, true);
        add_prop("border_mask_bit", godot::Variant::INT, 1);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : seed_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("seed_buffer_id")) seed_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_buffer_id"]));
        if (p_props.has("grid_width")) grid_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["grid_width"]));
        if (p_props.has("grid_height")) grid_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["grid_height"]));
        if (p_props.has("grid_depth")) grid_depth = std::max<int32_t>(1, static_cast<int32_t>(p_props["grid_depth"]));
        if (p_props.has("distance_metric")) distance_metric = std::clamp<int32_t>(static_cast<int32_t>(p_props["distance_metric"]), 0, 2);
        if (p_props.has("lod_ring_spacing")) lod_ring_spacing = std::max(1e-4f, static_cast<float>(p_props["lod_ring_spacing"]));
        if (p_props.has("stamp_border_group_mask")) stamp_border_group_mask = static_cast<bool>(p_props["stamp_border_group_mask"]);
        if (p_props.has("border_mask_bit")) border_mask_bit = std::clamp<int32_t>(static_cast<int32_t>(p_props["border_mask_bit"]), 0, 31);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    inline float eval_distance(const godot::Vector3& a, const godot::Vector3& b) const noexcept {
        const float dx = std::abs(a.x - b.x);
        const float dy = std::abs(a.y - b.y);
        const float dz = std::abs(a.z - b.z);
        if (distance_metric == 1) return dx + dy + dz;
        if (distance_metric == 2) return std::max({dx, dy, dz});
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const noexcept
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!r_selection.partition_ids || r_selection.capacity <= 0) return;

            const GrantPartPOD* primary = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            const GrantPartPOD* seed_part = (seed_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(seed_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);

            const int32_t w = std::max<int32_t>(1, grid_width);
            const int32_t h = std::max<int32_t>(1, grid_height);
            const int32_t d = std::max<int32_t>(1, grid_depth);
            const int64_t n_elems = std::min<int64_t>(
                r_selection.capacity,
                (r_selection.element_count > 0) ? r_selection.element_count : static_cast<int64_t>(w) * h * d
            );

            auto read_pos = [&](const GrantPartPOD* part, int64_t idx) noexcept -> godot::Vector3 {
                if (part && part->raw_base_ptr && part->element_stride >= sizeof(godot::Vector3)) {
                    return *reinterpret_cast<const godot::Vector3*>(part->raw_base_ptr + static_cast<size_t>(idx) * part->element_stride);
                }
                if (part && part->raw_base_ptr && part->element_stride >= sizeof(godot::Vector2)) {
                    const godot::Vector2& v2 = *reinterpret_cast<const godot::Vector2*>(part->raw_base_ptr + static_cast<size_t>(idx) * part->element_stride);
                    return godot::Vector3(v2.x, v2.y, 0.0f);
                }
                const int64_t z = idx / (static_cast<int64_t>(w) * h);
                const int64_t rem = idx % (static_cast<int64_t>(w) * h);
                const int64_t y = rem / w;
                const int64_t x = rem % w;
                return godot::Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
            };

            int64_t seed_count = 0;
            if (seed_part && seed_part->raw_base_ptr) {
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
                        const godot::Vector3 sp = read_pos(seed_part, s);
                        const float dist = eval_distance(p, sp);
                        if (dist < best_dist) {
                            best_dist = dist;
                            best_id = s;
                        }
                    }
                } else {
                    const float gx = std::round(p.x / lod_ring_spacing) * lod_ring_spacing;
                    const float gy = std::round(p.y / lod_ring_spacing) * lod_ring_spacing;
                    const float gz = std::round(p.z / lod_ring_spacing) * lod_ring_spacing;
                    best_dist = eval_distance(p, godot::Vector3(gx, gy, gz));
                    best_id = (static_cast<int64_t>(gx) * 73856093LL) ^
                              (static_cast<int64_t>(gy) * 19349663LL) ^
                              (static_cast<int64_t>(gz) * 83492791LL);
                }

                r_selection.partition_ids[i] = best_id;
                if (r_selection.lod_levels) {
                    const int32_t ring = std::clamp<int32_t>(static_cast<int32_t>(best_dist / lod_ring_spacing), 0, 255);
                    r_selection.lod_levels[i] = static_cast<uint8_t>(ring);
                }
            }

            if (stamp_border_group_mask && r_selection.group_masks && w > 1 && h > 1) {
                const uint32_t bit = (1u << border_mask_bit);
                const int64_t grid_total = std::min<int64_t>(n_elems, static_cast<int64_t>(w) * h * d);
                for (int64_t idx = 0; idx < grid_total; ++idx) {
                    const int64_t pid = r_selection.partition_ids[idx];
                    const int64_t x = idx % w;
                    const int64_t y = (idx / w) % h;
                    bool is_border = false;
                    if (x + 1 < w && r_selection.partition_ids[idx + 1] != pid) is_border = true;
                    else if (x > 0 && r_selection.partition_ids[idx - 1] != pid) is_border = true;
                    else if (y + 1 < h && r_selection.partition_ids[idx + w] != pid) is_border = true;
                    else if (y > 0 && r_selection.partition_ids[idx - w] != pid) is_border = true;

                    if (is_border) {
                        r_selection.group_masks[idx] |= bit;
                    } else {
                        r_selection.group_masks[idx] &= ~bit;
                    }
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_metadata(
        T_ActiveView& p_view,
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_context
    ) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(r_selection, p_context, p_view);
    }
};

static_assert(sizeof(VoronoiSeedRegionPartitionMetadataLogic<>) == 64, "VoronoiSeedRegionPartitionMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<VoronoiSeedRegionPartitionMetadataLogic<>>, "VoronoiSeedRegionPartitionMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core