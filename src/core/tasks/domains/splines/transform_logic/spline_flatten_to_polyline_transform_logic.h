#pragma once

/**
 * spline_flatten_to_polyline_transform_logic.h
 * SplineDOD Transform Task (S_T2): Subdivides 2D and 3D splines directly from
 * `SPLINE_HEADER_POD` + `SPLINE_SEGMENT_COEFFS_4D_POD` into `GeometryDOD`'s bridged
 * `BRIDGED_GEOM_POLYLINE_SLICE_POD`, `BRIDGED_GEOM_POLYLINE_VERTEX_2D`, and
 * `BRIDGED_GEOM_POLYLINE_VERTEX_3D` buffers.
 *
 * This is the Wave 1 zero-copy bridge that allows `GeometryDOD` and `TileDOD` tasks
 * in Wave 2/3 to classify, triangulate, offset, or clip against live `Spline2D` / `Spline3D`
 * contours without touching Godot `Variant` or OOP `Object` pointers.
 */

#include "../metadata_logic/spline_arc_length_lut_metadata_logic.h"
#include "../../../../geometry/geometry_common.h"
#include "../../../../geometry/polygon_math_2d.h"
#include "../../../../splines/spline_math.h"
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

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) SplineFlattenToPolylineTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::VECTOR2 | DataType::VECTOR3;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline Flatten to GeometryDOD Polyline";
    static constexpr std::string_view ui_category  = "Splines -> Geometry";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B
    uint32_t spline_header_buffer_id      = INVALID_ID; // 4B: SplineHeaderPOD (64B AoS)
    uint32_t segment_coeffs_4d_buffer_id  = INVALID_ID; // 4B: SplineSegmentCoeffsPOD<4, float>
    uint32_t polyline_slice_buffer_id     = INVALID_ID; // 4B: Bridged GeometryDOD PolylineSlicePOD
    uint32_t polyline_vertex_2d_buffer_id = INVALID_ID; // 4B: Bridged GeometryDOD glm::vec2
    uint32_t polyline_vertex_3d_buffer_id = INVALID_ID; // 4B: Bridged GeometryDOD glm::vec3
    uint16_t default_subdivisions         = 12u;        // 2B: Fallback subdivisions per section
    uint8_t reserved_padding[38]          = {0};        // 38B -> Total = 64B

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
        add_buf("segment_coeffs_4d_buffer_id");
        add_buf("polyline_slice_buffer_id");
        add_buf("polyline_vertex_2d_buffer_id");
        add_buf("polyline_vertex_3d_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))             target_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("spline_header_buffer_id"))      spline_header_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["spline_header_buffer_id"]));
        if (p_props.has("segment_coeffs_4d_buffer_id"))  segment_coeffs_4d_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["segment_coeffs_4d_buffer_id"]));
        if (p_props.has("polyline_slice_buffer_id"))     polyline_slice_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_slice_buffer_id"]));
        if (p_props.has("polyline_vertex_2d_buffer_id")) polyline_vertex_2d_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_2d_buffer_id"]));
        if (p_props.has("polyline_vertex_3d_buffer_id")) polyline_vertex_3d_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_3d_buffer_id"]));
        if (p_props.has("default_subdivisions"))         default_subdivisions         = static_cast<uint16_t>(static_cast<int64_t>(p_props["default_subdivisions"]));
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
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *hdr_part   = p_context.get_grant_part(spline_header_buffer_id);
        const GrantPartPOD *seg_part   = p_context.get_grant_part(segment_coeffs_4d_buffer_id);
        const GrantPartPOD *slice_part = p_context.get_grant_part(polyline_slice_buffer_id);
        const GrantPartPOD *v2_part    = p_context.get_grant_part(polyline_vertex_2d_buffer_id);
        const GrantPartPOD *v3_part    = p_context.get_grant_part(polyline_vertex_3d_buffer_id);

        if (!hdr_part || !hdr_part->raw_base_ptr ||
            !seg_part || !seg_part->raw_base_ptr ||
            !slice_part || !slice_part->raw_base_ptr) {
            return;
        }

        const auto *headers = reinterpret_cast<const SplineHeaderPOD *>(hdr_part->raw_base_ptr);
        const auto *coeffs  = reinterpret_cast<const SplineSegmentCoeffsPOD<4, float> *>(seg_part->raw_base_ptr);
        auto *slices        = reinterpret_cast<PolylineSlicePOD *>(slice_part->raw_base_ptr);
        auto *verts_2d      = (v2_part && v2_part->raw_base_ptr)
            ? reinterpret_cast<glm::vec2 *>(v2_part->raw_base_ptr)
            : nullptr;
        auto *verts_3d      = (v3_part && v3_part->raw_base_ptr)
            ? reinterpret_cast<glm::vec3 *>(v3_part->raw_base_ptr)
            : nullptr;

        const int64_t spline_count = hdr_part->selection.element_count;
        const int64_t seg_cap      = seg_part->selection.capacity;
        const int64_t slice_cap    = slice_part->selection.capacity;
        const int64_t v2_cap       = v2_part ? v2_part->selection.capacity : 0;
        const int64_t v3_cap       = v3_part ? v3_part->selection.capacity : 0;

        for (int64_t i = 0; i < spline_count; ++i) {
            const SplineHeaderPOD &hdr = headers[i];
            const uint32_t p_slot      = (hdr.geom_polyline_slot != 0xFFFFFFFFu)
                ? hdr.geom_polyline_slot
                : static_cast<uint32_t>(i);
            if (static_cast<int64_t>(p_slot) >= slice_cap || hdr.segment_count == 0u) {
                continue;
            }
            if (static_cast<int64_t>(hdr.segment_offset + hdr.segment_count) > seg_cap) {
                continue;
            }

            PolylineSlicePOD &ps   = slices[p_slot];
            const uint32_t subdivs = std::max<uint32_t>(1u, hdr.subdivisions_per_section ? hdr.subdivisions_per_section : default_subdivisions);
            const bool closed      = (hdr.is_wrap != 0u);
            const uint32_t total_v = closed ? (hdr.segment_count * subdivs) : (hdr.segment_count * subdivs + 1u);

            if (hdr.dimension == 2u && verts_2d && static_cast<int64_t>(ps.vertex_offset + total_v) <= v2_cap) {
                uint32_t w_idx = ps.vertex_offset;
                for (uint32_t s = 0u; s < hdr.segment_count; ++s) {
                    const auto &c = coeffs[hdr.segment_offset + s];
                    for (uint32_t k = 0u; k < subdivs; ++k) {
                        const float t  = static_cast<float>(k) / static_cast<float>(subdivs);
                        const glm::vec4 p4 = c.evaluate(t);
                        verts_2d[w_idx++]  = glm::vec2(p4.x, p4.y);
                    }
                }
                if (!closed) {
                    const glm::vec4 end4 = coeffs[hdr.segment_offset + hdr.segment_count - 1u].evaluate(1.0f);
                    verts_2d[w_idx++]    = glm::vec2(end4.x, end4.y);
                }
                ps.vertex_count = total_v;
                ps.dimension    = 2u;
                ps.is_closed    = closed ? 1u : 0u;
                const std::span<const glm::vec2> span_2d(verts_2d + ps.vertex_offset, total_v);
                const AABB2D<float> box = PolygonMath2D<float>::compute_aabb(span_2d);
                ps.bounds_min    = box.min_bounds;
                ps.bounds_max    = box.max_bounds;
                ps.total_length  = PolygonMath2D<float>::perimeter(span_2d, closed);
                ps.winding_order = (PolygonMath2D<float>::get_winding_order(span_2d) == WindingOrder::CLOCKWISE) ? 0u : 1u;
            } else if (hdr.dimension >= 3u && verts_3d && static_cast<int64_t>(ps.vertex_offset + total_v) <= v3_cap) {
                uint32_t w_idx = ps.vertex_offset;
                AABB2D<float> box;
                float len_accum = 0.0f;
                for (uint32_t s = 0u; s < hdr.segment_count; ++s) {
                    const auto &c = coeffs[hdr.segment_offset + s];
                    for (uint32_t k = 0u; k < subdivs; ++k) {
                        const float t      = static_cast<float>(k) / static_cast<float>(subdivs);
                        const glm::vec4 p4 = c.evaluate(t);
                        const glm::vec3 p3(p4.x, p4.y, p4.z);
                        if (w_idx > ps.vertex_offset) {
                            len_accum += glm::distance(verts_3d[w_idx - 1u], p3);
                        }
                        verts_3d[w_idx++] = p3;
                        box.expand_to(glm::vec2(p3.x, p3.y));
                    }
                }
                if (!closed) {
                    const glm::vec4 end4 = coeffs[hdr.segment_offset + hdr.segment_count - 1u].evaluate(1.0f);
                    const glm::vec3 end3(end4.x, end4.y, end4.z);
                    if (w_idx > ps.vertex_offset) {
                        len_accum += glm::distance(verts_3d[w_idx - 1u], end3);
                    }
                    verts_3d[w_idx++] = end3;
                    box.expand_to(glm::vec2(end3.x, end3.y));
                }
                ps.vertex_count = total_v;
                ps.dimension    = 3u;
                ps.is_closed    = closed ? 1u : 0u;
                ps.bounds_min   = box.min_bounds;
                ps.bounds_max   = box.max_bounds;
                ps.total_length = len_accum;
            }
        }
    }
};

static_assert(sizeof(SplineFlattenToPolylineTransformLogic<>) == 64, "SplineFlattenToPolylineTransformLogic must be 64 bytes!");

} // namespace ideam::core
