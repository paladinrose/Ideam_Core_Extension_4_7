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
 * TIL7: TilingBoardSpaceGenerateTransformLogic
 * Bridges active TileDOD cell buffers into GameDOD board space coordinate buffers and
 * directional neighbor adjacency tables (up to 8 directional ports per board space).
 */
struct TilingBoardSpaceGenerateTransformLogic {
    static constexpr std::string_view ui_name = "Tiling to GameBoard Space Generator";
    static constexpr std::string_view ui_category = "Tiles/GamesBridge";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SINGLE_ELEMENT,
        MemoryView::BRIDGE
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::VECTOR2,
        DataType::VECTOR3,
        DataType::INT32
    };

    static constexpr int32_t MAX_NEIGHBOR_PORTS = 8;

    float board_elevation_y = 0.0f;
    float reference_cell_spacing = 1.0f;
    float neighbor_distance_tolerance = 1.25f;
    int32_t valence_ports = 8; // e.g., 4 = Orthogonal, 6 = Hexagonal, 8 = Chebyshev/Queen
    bool project_2d_to_xz = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("board_elevation_y", godot::Variant::FLOAT, board_elevation_y);
        add_prop("reference_cell_spacing", godot::Variant::FLOAT, reference_cell_spacing);
        add_prop("neighbor_distance_tolerance", godot::Variant::FLOAT, neighbor_distance_tolerance);
        add_prop("valence_ports", godot::Variant::INT, valence_ports);
        add_prop("project_2d_to_xz", godot::Variant::BOOL, project_2d_to_xz);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("board_elevation_y")) board_elevation_y = static_cast<float>(p_props["board_elevation_y"]);
        if (p_props.has("reference_cell_spacing")) reference_cell_spacing = std::max(1e-4f, static_cast<float>(p_props["reference_cell_spacing"]));
        if (p_props.has("neighbor_distance_tolerance")) neighbor_distance_tolerance = std::max(0.1f, static_cast<float>(p_props["neighbor_distance_tolerance"]));
        if (p_props.has("valence_ports")) valence_ports = std::clamp<int32_t>(static_cast<int32_t>(p_props["valence_ports"]), 1, MAX_NEIGHBOR_PORTS);
        if (p_props.has("project_2d_to_xz")) project_2d_to_xz = static_cast<bool>(p_props["project_2d_to_xz"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return 0;
    }

    size_t get_transient_requirement() const noexcept {
        return 0;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& tile_part = p_ctx.grant->parts[0];
        GrantPartPOD& space_pos_part = p_ctx.grant->parts[1];
        if (!tile_part.raw_base_ptr || !space_pos_part.raw_base_ptr) return;

        GrantPartPOD* adj_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        const uint32_t t_stride = std::max<uint32_t>(sizeof(godot::Vector2), tile_part.element_stride);
        const int32_t n_tiles = (tile_part.selection.element_count > 0)
            ? static_cast<int32_t>(tile_part.selection.element_count)
            : static_cast<int32_t>(tile_part.capacity_bytes / t_stride);
        if (n_tiles <= 0) return;

        auto read_tile_2d = [&](int32_t idx) noexcept -> godot::Vector2 {
            const float* fp = reinterpret_cast<const float*>(tile_part.raw_base_ptr + static_cast<size_t>(idx) * t_stride);
            return godot::Vector2(fp[0], fp[1]);
        };

        const uint32_t out_stride = std::max<uint32_t>(sizeof(godot::Vector2), space_pos_part.element_stride);
        const int32_t max_out_spaces = static_cast<int32_t>(space_pos_part.capacity_bytes / out_stride);
        const int32_t count = std::min(n_tiles, max_out_spaces);

        // 1. Populate GameDOD Board Space coordinates
        for (int32_t i = 0; i < count; ++i) {
            const godot::Vector2 p2 = read_tile_2d(i);
            uint8_t* dst = space_pos_part.raw_base_ptr + static_cast<size_t>(i) * out_stride;
            if (project_2d_to_xz && out_stride >= sizeof(godot::Vector3)) {
                *reinterpret_cast<godot::Vector3*>(dst) = godot::Vector3(p2.x, board_elevation_y, p2.y);
            } else {
                *reinterpret_cast<godot::Vector2*>(dst) = p2;
            }
        }
        space_pos_part.selection.element_count = count;

        // 2. Build directional neighbor port table [count * valence_ports] if adj_part is bound
        if (adj_part) {
            int32_t* adj_table = reinterpret_cast<int32_t*>(adj_part->raw_base_ptr);
            const int32_t max_adj_ints = static_cast<int32_t>(adj_part->capacity_bytes / sizeof(int32_t));
            const int32_t ports = std::clamp<int32_t>(valence_ports, 1, MAX_NEIGHBOR_PORTS);
            const float max_dist = reference_cell_spacing * neighbor_distance_tolerance;
            const float max_d2 = max_dist * max_dist;
            constexpr float TWO_PI = 6.283185307179586f;

            for (int32_t i = 0; i < count; ++i) {
                if ((i + 1) * ports > max_adj_ints) break;
                int32_t* row = adj_table + static_cast<size_t>(i) * ports;
                float best_port_d2[MAX_NEIGHBOR_PORTS];
                for (int32_t p = 0; p < ports; ++p) {
                    row[p] = -1;
                    best_port_d2[p] = max_d2;
                }

                const godot::Vector2 pi = read_tile_2d(i);
                for (int32_t j = 0; j < count; ++j) {
                    if (i == j) continue;
                    const godot::Vector2 diff = read_tile_2d(j) - pi;
                    const float d2 = diff.length_squared();
                    if (d2 < 1e-8f || d2 > max_d2) continue;

                    float angle = std::atan2(diff.y, diff.x);
                    if (angle < 0.0f) angle += TWO_PI;
                    const int32_t sector = static_cast<int32_t>(
                        std::floor(((angle / TWO_PI) * static_cast<float>(ports)) + 0.5f)
                    ) % ports;

                    if (d2 < best_port_d2[sector]) {
                        best_port_d2[sector] = d2;
                        row[sector] = j;
                    }
                }
            }
            adj_part->selection.element_count = static_cast<int64_t>(count) * ports;
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core