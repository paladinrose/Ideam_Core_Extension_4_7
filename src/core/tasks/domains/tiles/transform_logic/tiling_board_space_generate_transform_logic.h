#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

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
 * TilingBoardSpaceGenerateTransformLogic<T, T_Strategy, T_View> (TIL7)
 * Bridges active TileDOD cell buffers into GameDOD board space coordinate buffers and
 * directional neighbor adjacency tables (up to 8 directional ports per board space).
 */
template <
    typename T = godot::Vector2,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) TilingBoardSpaceGenerateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr int32_t MAX_NEIGHBOR_PORTS = 8;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR2 | DataType::VECTOR3 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Tiling to GameBoard Space Generator";
    static constexpr std::string_view ui_category  = "Tiles/GamesBridge";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID; // 4B
    uint32_t tile_cell_buffer_id        = INVALID_ID; // 4B
    uint32_t board_space_buffer_id      = INVALID_ID; // 4B
    uint32_t neighbor_port_buffer_id    = INVALID_ID; // 4B -> 16B
    float board_elevation_y             = 0.0f;       // 4B
    float reference_cell_spacing        = 1.0f;       // 4B
    float neighbor_distance_tolerance   = 1.25f;      // 4B
    int32_t valence_ports               = 8;          // 4B -> 32B: 4 = Orthogonal, 6 = Hexagonal, 8 = Chebyshev
    bool project_2d_to_xz               = true;       // 1B
    uint8_t reserved_padding[31]        = {0};        // 31B -> Total = 64B

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
        add_buf("tile_cell_buffer_id");
        add_buf("board_space_buffer_id");
        add_buf("neighbor_port_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("board_elevation_y", godot::Variant::FLOAT, 0.0f);
        add_prop("reference_cell_spacing", godot::Variant::FLOAT, 1.0f);
        add_prop("neighbor_distance_tolerance", godot::Variant::FLOAT, 1.25f);
        add_prop("valence_ports", godot::Variant::INT, 8);
        add_prop("project_2d_to_xz", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (board_space_buffer_id != INVALID_ID) ? board_space_buffer_id : tile_cell_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("tile_cell_buffer_id")) tile_cell_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["tile_cell_buffer_id"]));
        if (p_props.has("board_space_buffer_id")) board_space_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["board_space_buffer_id"]));
        if (p_props.has("neighbor_port_buffer_id")) neighbor_port_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["neighbor_port_buffer_id"]));
        if (p_props.has("board_elevation_y")) board_elevation_y = static_cast<float>(p_props["board_elevation_y"]);
        if (p_props.has("reference_cell_spacing")) reference_cell_spacing = std::max(1e-4f, static_cast<float>(p_props["reference_cell_spacing"]));
        if (p_props.has("neighbor_distance_tolerance")) neighbor_distance_tolerance = std::max(0.1f, static_cast<float>(p_props["neighbor_distance_tolerance"]));
        if (p_props.has("valence_ports")) valence_ports = std::clamp<int32_t>(static_cast<int32_t>(p_props["valence_ports"]), 1, MAX_NEIGHBOR_PORTS);
        if (p_props.has("project_2d_to_xz")) project_2d_to_xz = static_cast<bool>(p_props["project_2d_to_xz"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const noexcept {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* tile_part = (tile_cell_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(tile_cell_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            GrantPartPOD* space_pos_part = (board_space_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(board_space_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!tile_part || !tile_part->raw_base_ptr || !space_pos_part || !space_pos_part->raw_base_ptr) return;

            GrantPartPOD* adj_part = (neighbor_port_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(neighbor_port_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);

            const uint32_t t_stride = std::max<uint32_t>(sizeof(godot::Vector2), tile_part->element_stride);
            const int32_t n_tiles = (tile_part->selection.element_count > 0)
                ? static_cast<int32_t>(tile_part->selection.element_count)
                : static_cast<int32_t>(tile_part->capacity_bytes / t_stride);
            if (n_tiles <= 0) return;

            auto read_tile_2d = [&](int32_t idx) noexcept -> godot::Vector2 {
                const float* fp = reinterpret_cast<const float*>(tile_part->raw_base_ptr + static_cast<size_t>(idx) * t_stride);
                return godot::Vector2(fp[0], fp[1]);
            };

            const uint32_t out_stride = std::max<uint32_t>(sizeof(godot::Vector2), space_pos_part->element_stride);
            const int32_t max_out_spaces = static_cast<int32_t>(space_pos_part->capacity_bytes / out_stride);
            const int32_t count = std::min(n_tiles, max_out_spaces);

            for (int32_t i = 0; i < count; ++i) {
                const godot::Vector2 p2 = read_tile_2d(i);
                uint8_t* dst = space_pos_part->raw_base_ptr + static_cast<size_t>(i) * out_stride;
                if (project_2d_to_xz && out_stride >= sizeof(godot::Vector3)) {
                    *reinterpret_cast<godot::Vector3*>(dst) = godot::Vector3(p2.x, board_elevation_y, p2.y);
                } else {
                    *reinterpret_cast<godot::Vector2*>(dst) = p2;
                }
            }
            space_pos_part->selection.element_count = count;

            if (adj_part && adj_part->raw_base_ptr) {
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
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(TilingBoardSpaceGenerateTransformLogic<>) == 64, "TilingBoardSpaceGenerateTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<TilingBoardSpaceGenerateTransformLogic<>>, "TilingBoardSpaceGenerateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core