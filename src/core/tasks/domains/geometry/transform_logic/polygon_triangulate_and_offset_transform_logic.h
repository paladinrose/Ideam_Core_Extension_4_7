#pragma once

/**
 * polygon_triangulate_and_offset_transform_logic.h
 * GeometryDOD Transform Task (G_T1): Evaluates closed 2D polylines in `GEOM_POLYLINE_SLICE_POD`
 * + `GEOM_POLYLINE_VERTEX_2D`, optionally offsets them inward/outward (`PolygonMath2D::offset_polygon`),
 * computes signed shoelace area and area-weighted centroid, and triangulates via ear-clipping
 * (`PolygonMath2D::triangulate_into`) directly into `GEOM_POLYGON_MESH_SLICE_POD` and
 * `GEOM_POLYGON_INDEX_U32`.
 */

#include "../../../../geometry/geometry_common.h"
#include "../../../../geometry/polygon_math_2d.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) PolygonTriangulateAndOffsetTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Polygon Triangulate & Offset";
    static constexpr std::string_view ui_category  = "Geometry 2D";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B
    uint32_t polyline_slice_buffer_id     = INVALID_ID; // 4B: PolylineSlicePOD (32B AoS)
    uint32_t polyline_vertex_2d_buffer_id = INVALID_ID; // 4B: glm::vec2 input vertices
    uint32_t polygon_mesh_slice_buffer_id = INVALID_ID; // 4B: PolygonMeshSlicePOD (32B AoS)
    uint32_t polygon_vertex_2d_buffer_id  = INVALID_ID; // 4B: glm::vec2 output mesh vertices
    uint32_t polygon_index_u32_buffer_id  = INVALID_ID; // 4B: uint32_t triangle index buffer
    float offset_delta                    = 0.0f;       // 4B: Outward (>0) or inward (<0) offset delta
    float miter_limit                     = 2.0f;       // 4B: Miter limit for JOIN_STYLE_MITER
    uint8_t join_style                    = 0u;         // 1B: core::JoinStyle2D (0=MITER, 1=BEVEL, 2=ROUND)
    bool enforce_ccw_winding              = true;       // 1B
    uint8_t reserved_padding[30]          = {0};        // 30B -> Total = 64B

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
        add_buf("polyline_vertex_2d_buffer_id");
        add_buf("polygon_mesh_slice_buffer_id");
        add_buf("polygon_vertex_2d_buffer_id");
        add_buf("polygon_index_u32_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))             target_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("polyline_slice_buffer_id"))     polyline_slice_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_slice_buffer_id"]));
        if (p_props.has("polyline_vertex_2d_buffer_id")) polyline_vertex_2d_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_2d_buffer_id"]));
        if (p_props.has("polygon_mesh_slice_buffer_id")) polygon_mesh_slice_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_mesh_slice_buffer_id"]));
        if (p_props.has("polygon_vertex_2d_buffer_id"))  polygon_vertex_2d_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_vertex_2d_buffer_id"]));
        if (p_props.has("polygon_index_u32_buffer_id"))  polygon_index_u32_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_index_u32_buffer_id"]));
        if (p_props.has("offset_delta"))                 offset_delta                 = static_cast<float>(static_cast<double>(p_props["offset_delta"]));
        if (p_props.has("miter_limit"))                  miter_limit                  = static_cast<float>(static_cast<double>(p_props["miter_limit"]));
        if (p_props.has("join_style"))                   join_style                   = static_cast<uint8_t>(static_cast<int64_t>(p_props["join_style"]));
        if (p_props.has("enforce_ccw_winding"))          enforce_ccw_winding          = static_cast<bool>(p_props["enforce_ccw_winding"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : polygon_mesh_slice_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *pline_part = p_context.get_grant_part(polyline_slice_buffer_id);
        const GrantPartPOD *pvert_part = p_context.get_grant_part(polyline_vertex_2d_buffer_id);
        const GrantPartPOD *mesh_part  = p_context.get_grant_part(polygon_mesh_slice_buffer_id);
        const GrantPartPOD *mvert_part = p_context.get_grant_part(polygon_vertex_2d_buffer_id);
        const GrantPartPOD *idx_part   = p_context.get_grant_part(polygon_index_u32_buffer_id);

        if (!pline_part || !pline_part->raw_base_ptr ||
            !pvert_part || !pvert_part->raw_base_ptr ||
            !mesh_part  || !mesh_part->raw_base_ptr  ||
            !idx_part   || !idx_part->raw_base_ptr) {
            return;
        }

        const auto *pline_slices = reinterpret_cast<const PolylineSlicePOD *>(pline_part->raw_base_ptr);
        const auto *pline_verts  = reinterpret_cast<const glm::vec2 *>(pvert_part->raw_base_ptr);
        auto *mesh_slices        = reinterpret_cast<PolygonMeshSlicePOD *>(mesh_part->raw_base_ptr);
        auto *mesh_verts         = (mvert_part && mvert_part->raw_base_ptr)
            ? reinterpret_cast<glm::vec2 *>(mvert_part->raw_base_ptr)
            : nullptr;
        auto *tri_indices        = reinterpret_cast<uint32_t *>(idx_part->raw_base_ptr);

        const int64_t poly_count = std::min(pline_part->selection.element_count, mesh_part->selection.capacity);
        const int64_t pvert_cap  = pvert_part->selection.capacity;
        const int64_t mvert_cap  = mvert_part ? mvert_part->selection.capacity : 0;
        const int64_t idx_cap    = idx_part->selection.capacity;

        uint32_t write_vert_cursor = 0u;
        uint32_t write_idx_cursor  = 0u;
        std::vector<glm::vec2> work_verts;
        std::vector<uint32_t> local_tris;

        for (int64_t i = 0; i < poly_count; ++i) {
            const PolylineSlicePOD &ps = pline_slices[i];
            PolygonMeshSlicePOD &ms    = mesh_slices[i];

            if (ps.vertex_count < 3u ||
                static_cast<int64_t>(ps.vertex_offset + ps.vertex_count) > pvert_cap) {
                ms = PolygonMeshSlicePOD{};
                continue;
            }

            std::span<const glm::vec2> src_span(pline_verts + ps.vertex_offset, ps.vertex_count);
            if (std::abs(offset_delta) > 1e-6f) {
                work_verts = PolygonMath2D<float>::offset_polygon(
                    src_span,
                    offset_delta,
                    static_cast<JoinStyle2D>(join_style),
                    miter_limit
                );
            } else {
                work_verts.assign(src_span.begin(), src_span.end());
            }

            if (enforce_ccw_winding && work_verts.size() >= 3u) {
                PolygonMath2D<float>::ensure_winding(work_verts, WindingOrder::COUNTER_CLOCKWISE);
            }

            local_tris.clear();
            if (work_verts.size() >= 3u) {
                PolygonMath2D<float>::triangulate_into(work_verts, local_tris);
            }

            ms.signed_area = PolygonMath2D<float>::signed_area(work_verts);
            ms.centroid    = PolygonMath2D<float>::centroid(work_verts);

            if (mesh_verts && static_cast<int64_t>(write_vert_cursor + work_verts.size()) <= mvert_cap) {
                ms.vertex_offset = write_vert_cursor;
                ms.vertex_count  = static_cast<uint32_t>(work_verts.size());
                std::copy(work_verts.begin(), work_verts.end(), mesh_verts + write_vert_cursor);
                write_vert_cursor += ms.vertex_count;
            } else {
                ms.vertex_offset = ps.vertex_offset;
                ms.vertex_count  = ps.vertex_count;
            }

            if (static_cast<int64_t>(write_idx_cursor + local_tris.size()) <= idx_cap) {
                ms.index_offset = write_idx_cursor;
                ms.index_count  = static_cast<uint32_t>(local_tris.size());
                std::copy(local_tris.begin(), local_tris.end(), tri_indices + write_idx_cursor);
                write_idx_cursor += ms.index_count;
            } else {
                ms.index_offset = 0u;
                ms.index_count  = 0u;
            }
        }
    }
};

static_assert(sizeof(PolygonTriangulateAndOffsetTransformLogic<>) == 64, "PolygonTriangulateAndOffsetTransformLogic must be 64 bytes!");

} // namespace ideam::core
