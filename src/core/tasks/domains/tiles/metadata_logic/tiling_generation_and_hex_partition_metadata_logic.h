#pragma once

/**
 * tiling_generation_and_hex_partition_metadata_logic.h
 * TileDOD Metadata Task (T_M1): Audits active `IsoCellPOD` cells, stamps topological
 * generation LOD tiers into `r_selection.lod_levels`, and partitions cells by
 * interior vs. boundary-clipped vs. seam-bridge status in `r_selection.partition_ids`.
 */

#include "../../../../tiles/tile_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/stencil_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) TilingGenerationAndHexPartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = StencilView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Tiling Generation & Hex Partition";
    static constexpr std::string_view ui_category  = "Tiles";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID; // 4B
    uint32_t iso_cell_buffer_id         = INVALID_ID; // 4B: IsoCellPOD (32B AoS)
    uint32_t generation_lod_threshold_1 = 4u;         // 4B: Generation ring for LOD 1
    uint32_t generation_lod_threshold_2 = 12u;        // 4B: Generation ring for LOD 2
    bool partition_by_boundary_flags    = true;       // 1B: 0=Interior, 1=Border/Clipped, 2=BridgeQuad
    bool write_lod_levels               = true;       // 1B
    uint8_t reserved_padding[46]        = {0};        // 46B -> Total = 64B

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
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))           target_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("iso_cell_buffer_id"))         iso_cell_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["iso_cell_buffer_id"]));
        if (p_props.has("generation_lod_threshold_1")) generation_lod_threshold_1 = static_cast<uint32_t>(static_cast<int64_t>(p_props["generation_lod_threshold_1"]));
        if (p_props.has("generation_lod_threshold_2")) generation_lod_threshold_2 = static_cast<uint32_t>(static_cast<int64_t>(p_props["generation_lod_threshold_2"]));
        if (p_props.has("partition_by_boundary_flags"))partition_by_boundary_flags= static_cast<bool>(p_props["partition_by_boundary_flags"]);
        if (p_props.has("write_lod_levels"))           write_lod_levels           = static_cast<bool>(p_props["write_lod_levels"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : iso_cell_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_metadata(
        T_View &p_view,
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context
    ) const noexcept {
        (void)p_view;
        const GrantPartPOD *cell_part = p_context.get_grant_part(iso_cell_buffer_id);
        const auto *cells = (cell_part && cell_part->raw_base_ptr)
            ? reinterpret_cast<const IsoCellPOD *>(cell_part->raw_base_ptr)
            : nullptr;
        if (!cells) return;

        const int64_t count = std::min<int64_t>(
            std::min<int64_t>(r_selection.capacity, cell_part->selection.capacity),
            64
        );

        for (int64_t i = 0; i < count; ++i) {
            const IsoCellPOD &c = cells[i];
            if (partition_by_boundary_flags) {
                int32_t part_id = static_cast<int32_t>(c.group_slot) * 10;
                if ((c.flags & CELL_FLAG_BRIDGE_QUAD) != 0u) {
                    part_id += 2;
                } else if ((c.flags & (CELL_FLAG_ON_BORDER | CELL_FLAG_CLIPPED_CAP)) != 0u) {
                    part_id += 1;
                }
                r_selection.partition_ids[i] = part_id;
            }
            if (write_lod_levels) {
                uint8_t lod = 0u;
                if (c.generation >= generation_lod_threshold_2) {
                    lod = 2u;
                } else if (c.generation >= generation_lod_threshold_1) {
                    lod = 1u;
                }
                r_selection.lod_levels[i] = lod;
            }
        }
    }
};

static_assert(
    sizeof(TilingGenerationAndHexPartitionMetadataLogic<>) == 64,
    "TilingGenerationAndHexPartitionMetadataLogic must be 64 bytes!"
);

} // namespace ideam::core
