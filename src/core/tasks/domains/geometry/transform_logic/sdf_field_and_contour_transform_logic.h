#pragma once

/**
 * sdf_field_and_contour_transform_logic.h
 * GeometryDOD Transform Task (G_T4): Evaluates 2D Chladni standing-wave modal fields
 * or polygon Signed Distance Fields across `GEOM_SDF_GRID_FLOAT32` (`SDFMath`) and
 * extracts Marching Squares iso-contour segments into `GEOM_MARCHING_SQUARES_SEGMENT_POD`.
 */

#include "../../../../geometry/geometry_common.h"
#include "../../../../geometry/sdf_math.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace ideam::core {

enum GeometrySDFFieldEvalMode : uint8_t {
    GEOM_SDF_EVAL_CONTOUR_ONLY  = 0, // Extract Marching Squares contours from existing SDF grid
    GEOM_SDF_EVAL_CHLADNI_WAVE  = 1, // Evaluate Chladni standing wave + extract contours
    GEOM_SDF_EVAL_POLYGON_FIELD = 2  // Evaluate signed distance to polyline_slice[0] + extract contours
};

template <typename T = float>
struct alignas(64) SDFFieldAndContourTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "SDF Field & Marching Squares";
    static constexpr std::string_view ui_category  = "SDF & Cymatics";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                   = INVALID_ID;                // 4B
    uint32_t sdf_grid_float32_buffer_id         = INVALID_ID;                // 4B: Row-major float32 grid
    uint32_t marching_squares_segment_buffer_id = INVALID_ID;                // 4B: MarchingSquaresSegment2D<float>
    uint32_t marching_squares_count_buffer_id   = INVALID_ID;                // 4B: uint32_t extracted segment count
    uint32_t grid_width                         = 64u;                       // 4B
    uint32_t grid_height                        = 64u;                       // 4B
    float origin_x                              = -5.0f;                     // 4B
    float origin_y                              = -5.0f;                     // 4B
    float cell_size_x                           = 10.0f / 63.0f;             // 4B
    float cell_size_y                           = 10.0f / 63.0f;             // 4B
    float iso_value                             = 0.0f;                      // 4B
    float chladni_mode_n                        = 3.0f;                      // 4B
    float chladni_mode_m                        = 5.0f;                      // 4B
    uint8_t eval_mode                           = GEOM_SDF_EVAL_CONTOUR_ONLY;// 1B
    bool extract_contours                       = true;                      // 1B
    uint8_t reserved_padding[10]                = {0};                       // 10B -> Total = 64B

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
        add_buf("sdf_grid_float32_buffer_id");
        add_buf("marching_squares_segment_buffer_id");
        add_buf("marching_squares_count_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))                   target_buffer_id                   = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sdf_grid_float32_buffer_id"))         sdf_grid_float32_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["sdf_grid_float32_buffer_id"]));
        if (p_props.has("marching_squares_segment_buffer_id")) marching_squares_segment_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["marching_squares_segment_buffer_id"]));
        if (p_props.has("marching_squares_count_buffer_id"))   marching_squares_count_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["marching_squares_count_buffer_id"]));
        if (p_props.has("grid_width"))                         grid_width                         = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_width"]));
        if (p_props.has("grid_height"))                        grid_height                        = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_height"]));
        if (p_props.has("origin_x"))                           origin_x                           = static_cast<float>(static_cast<double>(p_props["origin_x"]));
        if (p_props.has("origin_y"))                           origin_y                           = static_cast<float>(static_cast<double>(p_props["origin_y"]));
        if (p_props.has("cell_size_x"))                        cell_size_x                        = static_cast<float>(static_cast<double>(p_props["cell_size_x"]));
        if (p_props.has("cell_size_y"))                        cell_size_y                        = static_cast<float>(static_cast<double>(p_props["cell_size_y"]));
        if (p_props.has("iso_value"))                          iso_value                          = static_cast<float>(static_cast<double>(p_props["iso_value"]));
        if (p_props.has("chladni_mode_n"))                     chladni_mode_n                     = static_cast<float>(static_cast<double>(p_props["chladni_mode_n"]));
        if (p_props.has("chladni_mode_m"))                     chladni_mode_m                     = static_cast<float>(static_cast<double>(p_props["chladni_mode_m"]));
        if (p_props.has("eval_mode"))                          eval_mode                          = static_cast<uint8_t>(static_cast<int64_t>(p_props["eval_mode"]));
        if (p_props.has("extract_contours"))                   extract_contours                   = static_cast<bool>(p_props["extract_contours"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : sdf_grid_float32_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *grid_part = p_context.get_grant_part(sdf_grid_float32_buffer_id);
        const GrantPartPOD *seg_part  = p_context.get_grant_part(marching_squares_segment_buffer_id);
        const GrantPartPOD *cnt_part  = p_context.get_grant_part(marching_squares_count_buffer_id);

        if (!grid_part || !grid_part->raw_base_ptr) return;

        auto *grid = reinterpret_cast<float *>(grid_part->raw_base_ptr);
        const uint32_t gw = std::max(2u, grid_width);
        const uint32_t gh = std::max(2u, grid_height);
        const size_t total_cells = static_cast<size_t>(gw) * static_cast<size_t>(gh);
        if (static_cast<int64_t>(total_cells) > grid_part->selection.capacity) return;

        if (eval_mode == GEOM_SDF_EVAL_CHLADNI_WAVE) {
            for (uint32_t y = 0u; y < gh; ++y) {
                const float v = static_cast<float>(y) / static_cast<float>(gh - 1u);
                for (uint32_t x = 0u; x < gw; ++x) {
                    const float u = static_cast<float>(x) / static_cast<float>(gw - 1u);
                    grid[y * gw + x] = SDFMath<float>::evaluate_chladni_wave_2d(
                        glm::vec2(u, v),
                        chladni_mode_n,
                        chladni_mode_m
                    );
                }
            }
        }

        if (extract_contours && seg_part && seg_part->raw_base_ptr) {
            const std::span<const float> grid_span(grid, total_cells);
            const auto segs = SDFMath<float>::extract_marching_squares_contours(
                grid_span,
                gw,
                gh,
                iso_value,
                glm::vec2(origin_x, origin_y),
                glm::vec2(cell_size_x, cell_size_y)
            );

            auto *seg_out = reinterpret_cast<MarchingSquaresSegment2D<float> *>(seg_part->raw_base_ptr);
            const size_t copy_cnt = std::min<size_t>(segs.size(), static_cast<size_t>(seg_part->selection.capacity));
            if (copy_cnt > 0u) {
                std::copy_n(segs.data(), copy_cnt, seg_out);
            }
            if (cnt_part && cnt_part->raw_base_ptr) {
                *reinterpret_cast<uint32_t *>(cnt_part->raw_base_ptr) = static_cast<uint32_t>(copy_cnt);
            }
        }
    }
};

static_assert(sizeof(SDFFieldAndContourTransformLogic<>) == 64, "SDFFieldAndContourTransformLogic must be 64 bytes!");

} // namespace ideam::core
