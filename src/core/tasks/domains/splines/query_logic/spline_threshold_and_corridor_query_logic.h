#pragma once

/**
 * spline_threshold_and_corridor_query_logic.h
 * SplineDOD Query Task (S_Q1): Filters batch samples or spatial entities against:
 *   1. `SPLINE_QUERY_CORRIDOR_PROXIMITY`: Distance to closest point on spline <= `corridor_radius`
 *      (or <= the evaluated 4D `W` attribute at the closest point when `use_w_channel_as_radius` is true).
 *   2. `SPLINE_QUERY_1D_VALUE_RANGE`: Evaluated 1D spline output `.x` in `[min_value, max_value]`.
 *   3. `SPLINE_QUERY_2D_POLYGON_CONTAINMENT`: Point containment inside a closed 2D spline flattened
 *      into `GeometryDOD`'s `BRIDGED_GEOM_POLYLINE_SLICE_POD` + `BRIDGED_GEOM_POLYLINE_VERTEX_2D`.
 */

#include "../metadata_logic/spline_arc_length_lut_metadata_logic.h"
#include "../../../../geometry/polygon_math_2d.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

enum SplineQueryFilterMode : uint8_t {
    SPLINE_QUERY_CORRIDOR_PROXIMITY     = 0,
    SPLINE_QUERY_1D_VALUE_RANGE         = 1,
    SPLINE_QUERY_2D_POLYGON_CONTAINMENT = 2
};

template <typename T = int32_t>
struct alignas(64) SplineThresholdAndCorridorQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32 | DataType::VECTOR4;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline Threshold & Corridor Query";
    static constexpr std::string_view ui_category  = "Splines";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID;                      // 4B
    uint32_t eval_output_vec4_buffer_id   = INVALID_ID;                      // 4B: glm::vec4 evaluated sample or closest point
    uint32_t closest_distance_buffer_id   = INVALID_ID;                      // 4B: float distance to spline
    uint32_t query_point_vec4_buffer_id   = INVALID_ID;                      // 4B: glm::vec4 query point
    uint32_t polyline_slice_buffer_id     = INVALID_ID;                      // 4B: Bridged GeometryDOD PolylineSlicePOD
    uint32_t polyline_vertex_2d_buffer_id = INVALID_ID;                      // 4B: Bridged GeometryDOD glm::vec2
    uint32_t target_polyline_slot         = 0u;                              // 4B: Closed polygon slot for mode 2
    float corridor_radius                 = 2.0f;                            // 4B: Max capture distance for mode 0
    float min_value                       = 0.0f;                            // 4B: Min 1D value for mode 1
    float max_value                       = 1.0f;                            // 4B: Max 1D value for mode 1
    uint8_t query_mode                    = SPLINE_QUERY_CORRIDOR_PROXIMITY; // 1B
    bool use_w_channel_as_radius          = false;                           // 1B: If true, uses closest_point.w as corridor radius
    bool negate_result                    = false;                           // 1B
    uint8_t reserved_padding[21]          = {0};                             // 21B -> Total = 64B

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
        add_buf("eval_output_vec4_buffer_id");
        add_buf("closest_distance_buffer_id");
        add_buf("query_point_vec4_buffer_id");
        add_buf("polyline_slice_buffer_id");
        add_buf("polyline_vertex_2d_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))             target_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("eval_output_vec4_buffer_id"))   eval_output_vec4_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["eval_output_vec4_buffer_id"]));
        if (p_props.has("closest_distance_buffer_id"))   closest_distance_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["closest_distance_buffer_id"]));
        if (p_props.has("query_point_vec4_buffer_id"))   query_point_vec4_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["query_point_vec4_buffer_id"]));
        if (p_props.has("polyline_slice_buffer_id"))     polyline_slice_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_slice_buffer_id"]));
        if (p_props.has("polyline_vertex_2d_buffer_id")) polyline_vertex_2d_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_2d_buffer_id"]));
        if (p_props.has("target_polyline_slot"))         target_polyline_slot         = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_polyline_slot"]));
        if (p_props.has("corridor_radius"))              corridor_radius              = static_cast<float>(static_cast<double>(p_props["corridor_radius"]));
        if (p_props.has("min_value"))                    min_value                    = static_cast<float>(static_cast<double>(p_props["min_value"]));
        if (p_props.has("max_value"))                    max_value                    = static_cast<float>(static_cast<double>(p_props["max_value"]));
        if (p_props.has("query_mode"))                   query_mode                   = static_cast<uint8_t>(static_cast<int64_t>(p_props["query_mode"]));
        if (p_props.has("use_w_channel_as_radius"))      use_w_channel_as_radius      = static_cast<bool>(p_props["use_w_channel_as_radius"]);
        if (p_props.has("negate_result"))                negate_result                = static_cast<bool>(p_props["negate_result"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : closest_distance_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    [[nodiscard]] inline bool evaluate_element(
        int64_t p_index,
        const T_View &p_view,
        const TaskContextPOD &p_context
    ) const noexcept {
        (void)p_view;
        bool match = false;

        if (query_mode == SPLINE_QUERY_CORRIDOR_PROXIMITY) {
            const GrantPartPOD *dist_part = p_context.get_grant_part(closest_distance_buffer_id);
            const GrantPartPOD *eval_part = p_context.get_grant_part(eval_output_vec4_buffer_id);
            if (dist_part && dist_part->raw_base_ptr && p_index < dist_part->selection.capacity) {
                const float dist = reinterpret_cast<const float *>(dist_part->raw_base_ptr)[p_index];
                float rad        = corridor_radius;
                if (use_w_channel_as_radius && eval_part && eval_part->raw_base_ptr && p_index < eval_part->selection.capacity) {
                    const float w_rad = reinterpret_cast<const glm::vec4 *>(eval_part->raw_base_ptr)[p_index].w;
                    if (w_rad > 1e-5f) rad = w_rad;
                }
                match = (dist <= rad);
            }
        } else if (query_mode == SPLINE_QUERY_1D_VALUE_RANGE) {
            const GrantPartPOD *eval_part = p_context.get_grant_part(eval_output_vec4_buffer_id);
            if (eval_part && eval_part->raw_base_ptr && p_index < eval_part->selection.capacity) {
                const float val = reinterpret_cast<const glm::vec4 *>(eval_part->raw_base_ptr)[p_index].x;
                match           = (val >= min_value && val <= max_value);
            }
        } else if (query_mode == SPLINE_QUERY_2D_POLYGON_CONTAINMENT) {
            const GrantPartPOD *qpt_part   = p_context.get_grant_part(query_point_vec4_buffer_id);
            const GrantPartPOD *slice_part = p_context.get_grant_part(polyline_slice_buffer_id);
            const GrantPartPOD *vert_part  = p_context.get_grant_part(polyline_vertex_2d_buffer_id);
            if (qpt_part && qpt_part->raw_base_ptr &&
                slice_part && slice_part->raw_base_ptr &&
                vert_part && vert_part->raw_base_ptr &&
                p_index < qpt_part->selection.capacity &&
                static_cast<int64_t>(target_polyline_slot) < slice_part->selection.capacity) {
                const glm::vec4 q4        = reinterpret_cast<const glm::vec4 *>(qpt_part->raw_base_ptr)[p_index];
                const PolylineSlicePOD &s = reinterpret_cast<const PolylineSlicePOD *>(slice_part->raw_base_ptr)[target_polyline_slot];
                const auto *verts         = reinterpret_cast<const glm::vec2 *>(vert_part->raw_base_ptr);
                if (s.vertex_count >= 3u && static_cast<int64_t>(s.vertex_offset + s.vertex_count) <= vert_part->selection.capacity) {
                    const std::span<const glm::vec2> poly(verts + s.vertex_offset, s.vertex_count);
                    match = PolygonMath2D<float>::contains_point(poly, glm::vec2(q4.x, q4.y));
                }
            }
        }

        return negate_result ? !match : match;
    }
};

static_assert(sizeof(SplineThresholdAndCorridorQueryLogic<>) == 64, "SplineThresholdAndCorridorQueryLogic must be 64 bytes!");

} // namespace ideam::core
