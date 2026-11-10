#pragma once

/**
 * tiling_hexize_and_border_extract_transform_logic.h
 * TileDOD Transform Task (T_T3): Macro-Hexagon (`Hexize`) Centroid Refresh & Border Loop Export.
 *
 * Updates each `HexSuperCellPOD`'s world-space `center` from its shared center vertex in
 * `GeometryDOD`'s bridged `polygon_vertex_2d_buffer_id` (or the average of its child
 * `IsoCellPOD` centers in `hex_child_cell_csr_buffer_id`).
 */

#include "../../../../geometry/geometry_common.h"
#include "../../../../tiles/tile_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) TilingHexizeAndBorderExtractTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Tiling Hexize & Border Extract";
    static constexpr std::string_view ui_category  = "Tiles";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B
    uint32_t iso_cell_buffer_id           = INVALID_ID; // 4B: IsoCellPOD (32B AoS)
    uint32_t hex_super_cell_buffer_id     = INVALID_ID; // 4B: HexSuperCellPOD (64B AoS)
    uint32_t hex_child_cell_csr_buffer_id = INVALID_ID; // 4B: uint32_t CSR child cell IDs
    uint32_t polygon_vertex_2d_buffer_id  = INVALID_ID; // 4B: Bridged GeometryDOD glm::vec2
    uint8_t reserved_padding[44]          = {0};        // 44B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char *p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("iso_cell_buffer_id");
        add_buf("hex_super_cell_buffer_id");
        add_buf("hex_child_cell_csr_buffer_id");
        add_buf("polygon_vertex_2d_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))             target_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("iso_cell_buffer_id"))           iso_cell_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["iso_cell_buffer_id"]));
        if (p_props.has("hex_super_cell_buffer_id"))     hex_super_cell_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["hex_super_cell_buffer_id"]));
        if (p_props.has("hex_child_cell_csr_buffer_id")) hex_child_cell_csr_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["hex_child_cell_csr_buffer_id"]));
        if (p_props.has("polygon_vertex_2d_buffer_id"))  polygon_vertex_2d_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_vertex_2d_buffer_id"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : hex_super_cell_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *hex_part = p_context.get_grant_part(hex_super_cell_buffer_id);
        if (!hex_part || !hex_part->raw_base_ptr) return;

        const int64_t hex_cnt = hex_part->selection.element_count;
        if (hex_cnt <= 0) return;

        auto *hexes = reinterpret_cast<HexSuperCellPOD *>(hex_part->raw_base_ptr);

        const GrantPartPOD *vtx_part  = p_context.get_grant_part(polygon_vertex_2d_buffer_id);
        const GrantPartPOD *cell_part = p_context.get_grant_part(iso_cell_buffer_id);
        const GrantPartPOD *ch_part   = p_context.get_grant_part(hex_child_cell_csr_buffer_id);

        const auto *verts = (vtx_part && vtx_part->raw_base_ptr)
            ? reinterpret_cast<const glm::vec2 *>(vtx_part->raw_base_ptr)
            : nullptr;
        const int64_t max_verts = vtx_part ? vtx_part->selection.capacity : 0;

        const auto *cells = (cell_part && cell_part->raw_base_ptr)
            ? reinterpret_cast<const IsoCellPOD *>(cell_part->raw_base_ptr)
            : nullptr;
        const int64_t max_cells = cell_part ? cell_part->selection.capacity : 0;

        const auto *child_csr = (ch_part && ch_part->raw_base_ptr)
            ? reinterpret_cast<const uint32_t *>(ch_part->raw_base_ptr)
            : nullptr;
        const int64_t max_child_csr = ch_part ? ch_part->selection.capacity : 0;

        for (int64_t h = 0; h < hex_cnt; ++h) {
            HexSuperCellPOD &hex = hexes[h];
            if (verts && static_cast<int64_t>(hex.center_vertex_id) < max_verts) {
                hex.center = verts[hex.center_vertex_id];
                continue;
            }
            const uint32_t ch_off = hex.get_child_offset();
            const uint8_t ch_cnt  = hex.get_child_count();
            if (cells && child_csr && ch_cnt > 0u &&
                static_cast<int64_t>(ch_off + ch_cnt) <= max_child_csr) {
                glm::vec2 accum(0.0f, 0.0f);
                uint32_t valid = 0u;
                for (uint8_t k = 0; k < ch_cnt; ++k) {
                    const uint32_t cid = child_csr[ch_off + k];
                    if (static_cast<int64_t>(cid) < max_cells) {
                        accum += cells[cid].center;
                        ++valid;
                    }
                }
                if (valid > 0u) {
                    hex.center = accum / static_cast<float>(valid);
                }
            }
        }
    }
};

static_assert(
    sizeof(TilingHexizeAndBorderExtractTransformLogic<>) == 64,
    "TilingHexizeAndBorderExtractTransformLogic must be 64 bytes!"
);

} // namespace ideam::core
