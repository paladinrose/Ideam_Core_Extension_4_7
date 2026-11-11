#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * G_RD10: RDWorkgroupTilePartitionMetadataLogic
 * Computes 1D linear or 2D Morton (Z-order) compute shader workgroup IDs into
 * r_selection.partition_ids and subgroup wave lane masks into r_selection.group_masks
 * for cache-coherent shared-memory (LDS) GPU compute dispatch.
 */
struct RDWorkgroupTilePartitionMetadataLogic {
    static constexpr std::string_view ui_name = "RD Compute Workgroup & Subgroup Tile Partition";
    static constexpr std::string_view ui_category = "GPU/Metadata";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 3> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SPATIAL_2D,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::UINT32,
        DataType::INT32,
        DataType::FLOAT32
    };

    int32_t tiling_mode = 0; // 0 = 1D Linear Workgroups, 1 = 2D Morton Z-Order Tiles
    int32_t local_size_x = 64;
    int32_t local_size_y = 8;
    int32_t grid_width = 256;
    int32_t subgroup_size = 32;

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
        add_prop("tiling_mode", godot::Variant::INT, tiling_mode, 2, "1D Linear Workgroups,2D Morton Z-Order Tiles");
        add_prop("local_size_x", godot::Variant::INT, local_size_x);
        add_prop("local_size_y", godot::Variant::INT, local_size_y);
        add_prop("grid_width", godot::Variant::INT, grid_width);
        add_prop("subgroup_size", godot::Variant::INT, subgroup_size);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("tiling_mode")) tiling_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["tiling_mode"]), 0, 1);
        if (p_props.has("local_size_x")) local_size_x = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_x"]));
        if (p_props.has("local_size_y")) local_size_y = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_y"]));
        if (p_props.has("grid_width")) grid_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["grid_width"]));
        if (p_props.has("subgroup_size")) subgroup_size = std::clamp<int32_t>(static_cast<int32_t>(p_props["subgroup_size"]), 4, 64);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

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

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& part = p_ctx.grant->parts[0];
        MemoryBufferSelectionPOD& sel = part.selection;
        if (!sel.partition_ids || sel.capacity <= 0) return;

        const int64_t n_elems = (sel.element_count > 0)
            ? std::min<int64_t>(sel.element_count, sel.capacity)
            : sel.capacity;

        const int64_t lx = std::max<int64_t>(1, local_size_x);
        const int64_t ly = std::max<int64_t>(1, local_size_y);
        const int64_t gw = std::max<int64_t>(1, grid_width);
        const int64_t sg = std::max<int64_t>(1, subgroup_size);

        for (int64_t i = 0; i < n_elems; ++i) {
            if (tiling_mode == 1) {
                const uint32_t px = static_cast<uint32_t>((i % gw) / lx);
                const uint32_t py = static_cast<uint32_t>((i / gw) / ly);
                sel.partition_ids[i] = static_cast<int64_t>(morton_encode_2d(px, py));
            } else {
                sel.partition_ids[i] = i / lx;
            }

            if (sel.group_masks) {
                const uint32_t subgroup_id = static_cast<uint32_t>((i % lx) / sg) & 31u;
                sel.group_masks[i] = (1u << subgroup_id);
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