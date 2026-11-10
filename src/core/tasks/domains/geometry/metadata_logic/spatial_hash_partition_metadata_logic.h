#pragma once

/**
 * spatial_hash_partition_metadata_logic.h
 * GeometryDOD Metadata Task (G_M1): Computes 2D/3D spatial grid hashes and AABB area/size
 * LOD tiers for polylines, polygons, and Voronoi sites in `GeometryDOD`.
 *
 * Writes spatial cell hashes into `r_selection.partition_ids`, LOD tiers into
 * `r_selection.lod_levels`, and optional `(cell_hash, primitive_id)` pairs into
 * `GEOM_SPATIAL_HASH_ENTRY_POD`.
 */

#include "../../../../geometry/geometry_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/stencil_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) SpatialHashPartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = StencilView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spatial Hash Partition";
    static constexpr std::string_view ui_category  = "Spatial";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B: Primary selection buffer
    uint32_t polyline_slice_buffer_id     = INVALID_ID; // 4B: PolylineSlicePOD (32B AoS)
    uint32_t spatial_hash_entry_buffer_id = INVALID_ID; // 4B: SpatialHashEntryPOD (8B AoS)
    float grid_origin_x                   = -50.0f;     // 4B: 2D spatial grid origin X
    float grid_origin_y                   = -50.0f;     // 4B: 2D spatial grid origin Y
    float cell_size_x                     = 5.0f;       // 4B: 2D spatial grid cell width
    float cell_size_y                     = 5.0f;       // 4B: 2D spatial grid cell height
    uint32_t grid_width                   = 32u;        // 4B: Grid columns
    uint32_t grid_height                  = 32u;        // 4B: Grid rows
    float lod_extent_threshold_1          = 2.0f;       // 4B: AABB extent threshold for LOD 1
    float lod_extent_threshold_2          = 10.0f;      // 4B: AABB extent threshold for LOD 2
    bool write_to_partition_ids           = true;       // 1B
    bool write_to_lod_levels              = true;       // 1B
    uint8_t reserved_padding[18]          = {0};        // 18B -> Total = 64B

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
        add_buf("polyline_slice_buffer_id");
        add_buf("spatial_hash_entry_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))             target_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("polyline_slice_buffer_id"))     polyline_slice_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_slice_buffer_id"]));
        if (p_props.has("spatial_hash_entry_buffer_id")) spatial_hash_entry_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["spatial_hash_entry_buffer_id"]));
        if (p_props.has("grid_origin_x"))                grid_origin_x                = static_cast<float>(static_cast<double>(p_props["grid_origin_x"]));
        if (p_props.has("grid_origin_y"))                grid_origin_y                = static_cast<float>(static_cast<double>(p_props["grid_origin_y"]));
        if (p_props.has("cell_size_x"))                  cell_size_x                  = static_cast<float>(static_cast<double>(p_props["cell_size_x"]));
        if (p_props.has("cell_size_y"))                  cell_size_y                  = static_cast<float>(static_cast<double>(p_props["cell_size_y"]));
        if (p_props.has("grid_width"))                   grid_width                   = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_width"]));
        if (p_props.has("grid_height"))                  grid_height                  = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_height"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : polyline_slice_buffer_id;
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
        const GrantPartPOD *slice_part = p_context.get_grant_part(polyline_slice_buffer_id);
        const GrantPartPOD *hash_part  = p_context.get_grant_part(spatial_hash_entry_buffer_id);

        const auto *slices = (slice_part && slice_part->raw_base_ptr)
            ? reinterpret_cast<const PolylineSlicePOD *>(slice_part->raw_base_ptr)
            : nullptr;
        auto *hash_entries = (hash_part && hash_part->raw_base_ptr)
            ? reinterpret_cast<SpatialHashEntryPOD *>(hash_part->raw_base_ptr)
            : nullptr;

        const int64_t count = slices
            ? std::min<int64_t>(r_selection.capacity, slice_part->selection.capacity)
            : r_selection.capacity;

        const glm::vec2 origin(grid_origin_x, grid_origin_y);
        const glm::vec2 cell_sz(
            std::max(1e-4f, cell_size_x),
            std::max(1e-4f, cell_size_y)
        );
        const uint32_t gw = std::max(1u, grid_width);
        const uint32_t gh = std::max(1u, grid_height);

        for (int64_t i = 0; i < count; ++i) {
            int32_t cell_id = 0;
            uint8_t lod     = 0u;

            if (slices) {
                const PolylineSlicePOD &s = slices[i];
                const glm::vec2 center    = (s.bounds_min + s.bounds_max) * 0.5f;
                const glm::vec2 ext       = glm::max(s.bounds_max - s.bounds_min, glm::vec2(0.0f));
                const float max_ext       = std::max(ext.x, ext.y);

                const int64_t flat_cell = GeometryKernelMath<float>::spatial_hash_2d(
                    center, origin, cell_sz, gw, gh
                );
                cell_id = (flat_cell >= 0) ? static_cast<int32_t>(flat_cell) : -1;

                if (max_ext >= lod_extent_threshold_2) {
                    lod = 2u;
                } else if (max_ext >= lod_extent_threshold_1) {
                    lod = 1u;
                }
            }

            if (write_to_partition_ids && i < 64) {
                r_selection.partition_ids[i] = cell_id;
            }
            if (write_to_lod_levels && i < 64) {
                r_selection.lod_levels[i] = lod;
            }
            if (hash_entries && i < hash_part->selection.capacity) {
                hash_entries[i].cell_hash    = static_cast<uint32_t>(std::max(0, cell_id));
                hash_entries[i].primitive_id = static_cast<uint32_t>(i);
            }
        }
    }
};

static_assert(sizeof(SpatialHashPartitionMetadataLogic<>) == 64, "SpatialHashPartitionMetadataLogic must be 64 bytes!");

} // namespace ideam::core
