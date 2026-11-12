#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * RDWorkgroupTilePartitionMetadataLogic<T, T_Strategy, T_View> (G_RD10)
 * Computes 1D linear or 2D Morton (Z-order) compute shader workgroup IDs into
 * r_selection.partition_ids and subgroup wave lane masks into r_selection.group_masks
 * for cache-coherent shared-memory (LDS) GPU compute dispatch.
 */
template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) RDWorkgroupTilePartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    =
        BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types =
        DataType::UINT32 | DataType::INT32 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD Compute Workgroup & Subgroup Tile Partition";
    static constexpr std::string_view ui_category  = "GPU/Metadata";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id    = INVALID_ID; // 4B
    int32_t tiling_mode          = 0;          // 4B: 0 = 1D Linear Workgroups, 1 = 2D Morton Z-Order Tiles
    int32_t local_size_x         = 64;         // 4B
    int32_t local_size_y         = 8;          // 4B -> 16B
    int32_t grid_width           = 256;        // 4B
    int32_t subgroup_size        = 32;         // 4B
    uint8_t reserved_padding[40] = {0};        // 40B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

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
        add_prop("tiling_mode", godot::Variant::INT, 0, 2, "1D Linear Workgroups,2D Morton Z-Order Tiles");
        add_prop("local_size_x", godot::Variant::INT, 64);
        add_prop("local_size_y", godot::Variant::INT, 8);
        add_prop("grid_width", godot::Variant::INT, 256);
        add_prop("subgroup_size", godot::Variant::INT, 32);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("tiling_mode")) tiling_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["tiling_mode"]), 0, 1);
        if (p_props.has("local_size_x")) local_size_x = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_x"]));
        if (p_props.has("local_size_y")) local_size_y = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_y"]));
        if (p_props.has("grid_width")) grid_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["grid_width"]));
        if (p_props.has("subgroup_size")) subgroup_size = std::clamp<int32_t>(static_cast<int32_t>(p_props["subgroup_size"]), 4, 64);
    }

    static inline uint32_t part1by1(uint32_t x) noexcept {
        x &= 0x0000ffffu;
        x = (x ^ (x << 8)) & 0x00ff00ffu;
        x = (x ^ (x << 4)) & 0x0f0f0f0fu;
        x = (x ^ (x << 2)) & 0x33333333u;
        x = (x ^ (x << 1)) & 0x55555555u;
        return x;
    }

    static inline uint32_t morton_encode_2d(uint32_t x, uint32_t y) noexcept {
        return (part1by1(y) << 1) | part1by1(x);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const
    {
        (void)p_ctx;
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!r_selection.partition_ids || r_selection.capacity <= 0) return;

            const int64_t n_elems = (r_selection.element_count > 0)
                ? std::min<int64_t>(r_selection.element_count, r_selection.capacity)
                : r_selection.capacity;

            const int64_t lx = std::max<int64_t>(1, local_size_x);
            const int64_t ly = std::max<int64_t>(1, local_size_y);
            const int64_t gw = std::max<int64_t>(1, grid_width);
            const int64_t sg = std::max<int64_t>(1, subgroup_size);

            for (int64_t i = 0; i < n_elems; ++i) {
                if (tiling_mode == 1) {
                    const uint32_t px = static_cast<uint32_t>((i % gw) / lx);
                    const uint32_t py = static_cast<uint32_t>((i / gw) / ly);
                    r_selection.partition_ids[i] = static_cast<int64_t>(morton_encode_2d(px, py));
                } else {
                    r_selection.partition_ids[i] = i / lx;
                }

                if (r_selection.group_masks) {
                    const uint32_t subgroup_id = static_cast<uint32_t>((i % lx) / sg) & 31u;
                    r_selection.group_masks[i] = (1u << subgroup_id);
                }
            }
        }
    }
};

static_assert(sizeof(RDWorkgroupTilePartitionMetadataLogic<>) == 64, "RDWorkgroupTilePartitionMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<RDWorkgroupTilePartitionMetadataLogic<>>, "RDWorkgroupTilePartitionMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core