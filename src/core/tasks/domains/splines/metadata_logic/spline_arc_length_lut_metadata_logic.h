#pragma once

/**
 * spline_arc_length_lut_metadata_logic.h
 * SplineDOD Metadata Task (S_M1): Audits active splines in `SPLINE_HEADER_POD`,
 * stamps curve complexity LOD tiers into `r_selection.lod_levels`, and partitions
 * splines by dimension (1D, 2D, 3D, 4D) and closed-loop status in `r_selection.partition_ids`.
 */

#include "../../../../geometry/geometry_common.h"
#include "../../../../splines/spline_math.h"
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

#ifndef IDEAM_SPLINES_HEADER_POD_DEFINED
#define IDEAM_SPLINES_HEADER_POD_DEFINED

/**
 * SplineHeaderPOD (64 Bytes, Cache-Line Aligned)
 * Master row descriptor in `SplineDOD`'s `SPLINE_HEADER_POD` table.
 */
struct alignas(16) SplineHeaderPOD {
    uint32_t segment_offset          = 0u;    // Start index in SPLINE_SEGMENT_COEFFS_4D_POD
    uint32_t segment_count           = 0u;    // Number of cubic Horner sections
    uint32_t weight_lut_offset       = 0u;    // Start index in SPLINE_WEIGHT_LUT_POD
    uint32_t distance_lut_offset     = 0u;    // Start index in SPLINE_DISTANCE_LUT_POD
    uint32_t lut_bin_count           = 64u;   // Number of LUT bins (default 64)
    uint32_t geom_polyline_slot      = 0xFFFFFFFFu; // Linked slot in GeometryDOD's GEOM_POLYLINE_SLICE_POD
    uint32_t geom_frame_offset       = 0u;    // Linked start offset in GeometryDOD's GEOM_CURVE_FRAME_3D_POD
    uint32_t geom_frame_count        = 0u;    // Number of Bishop frames written for this spline
    float total_weight               = 0.0f;  // Sum of section weights
    float total_length               = 0.0f;  // Total arc length
    uint8_t dimension                = 3u;    // 1, 2, 3, or 4
    uint8_t is_wrap                  = 0u;    // 1 if closed loop
    uint8_t w_channel_mode           = 0u;    // 0 = Scale, 1 = Roll (radians), 2 = RGBA/HSVA color
    uint8_t subdivisions_per_section = 12u;   // Target subdivisions when flattening to GeometryDOD
    glm::vec4 bounds_min{0.0f};               // 4D AABB min
};

static_assert(sizeof(SplineHeaderPOD) == 64, "SplineHeaderPOD must be 64 bytes!");

#endif // IDEAM_SPLINES_HEADER_POD_DEFINED

template <typename T = int32_t>
struct alignas(64) SplineArcLengthLUTMetadataLogic {
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

    static constexpr std::string_view display_name = "Spline Arc-Length & Dimension Partition";
    static constexpr std::string_view ui_category  = "Splines";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id        = INVALID_ID; // 4B
    uint32_t spline_header_buffer_id = INVALID_ID; // 4B: SplineHeaderPOD (64B AoS)
    uint32_t lod_segment_threshold_1 = 8u;         // 4B: Segment count for LOD 1
    uint32_t lod_segment_threshold_2 = 32u;        // 4B: Segment count for LOD 2
    bool partition_by_dimension      = true;       // 1B: Stamp dimension (1..4) into partition_ids
    bool write_lod_levels            = true;       // 1B: Stamp segment complexity tier into lod_levels
    uint8_t reserved_padding[46]     = {0};        // 46B -> Total = 64B

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
        add_buf("spline_header_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))        target_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("spline_header_buffer_id")) spline_header_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["spline_header_buffer_id"]));
        if (p_props.has("lod_segment_threshold_1")) lod_segment_threshold_1 = static_cast<uint32_t>(static_cast<int64_t>(p_props["lod_segment_threshold_1"]));
        if (p_props.has("lod_segment_threshold_2")) lod_segment_threshold_2 = static_cast<uint32_t>(static_cast<int64_t>(p_props["lod_segment_threshold_2"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : spline_header_buffer_id;
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
        const GrantPartPOD *hdr_part = p_context.get_grant_part(spline_header_buffer_id);
        const auto *headers = (hdr_part && hdr_part->raw_base_ptr)
            ? reinterpret_cast<const SplineHeaderPOD *>(hdr_part->raw_base_ptr)
            : nullptr;
        if (!headers) return;

        const int64_t count = std::min<int64_t>(
            std::min<int64_t>(r_selection.capacity, hdr_part->selection.capacity),
            64
        );

        for (int64_t i = 0; i < count; ++i) {
            const SplineHeaderPOD &h = headers[i];
            if (partition_by_dimension) {
                r_selection.partition_ids[i] = static_cast<int32_t>(h.dimension) * 10 + static_cast<int32_t>(h.is_wrap);
            }
            if (write_lod_levels) {
                uint8_t lod = 0u;
                if (h.segment_count >= lod_segment_threshold_2) {
                    lod = 2u;
                } else if (h.segment_count >= lod_segment_threshold_1) {
                    lod = 1u;
                }
                r_selection.lod_levels[i] = lod;
            }
        }
    }
};

static_assert(sizeof(SplineArcLengthLUTMetadataLogic<>) == 64, "SplineArcLengthLUTMetadataLogic must be 64 bytes!");

} // namespace ideam::core
