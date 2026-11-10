#pragma once

/**
 * csg_clip_and_seam_weld_transform_logic.h
 * GeometryDOD Transform Task (G_T2): Clips boundary-straddling 2D polygons against a
 * reference convex/half-plane boundary (`CSGMath2D::clip_polygon_half_plane` /
 * `CSGMath2D::clip_polygon_convex`), and optionally triangulates the clipped boundary caps
 * into `GEOM_POLYGON_MESH_SLICE_POD`.
 */

#include "../../../../geometry/csg_2d.h"
#include "../../../../geometry/geometry_common.h"
#include "../../../../geometry/polygon_math_2d.h"
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

enum GeometryCSGClipMode : uint8_t {
    GEOM_CSG_CLIP_CONVEX_BOUNDARY = 0, // Sutherland-Hodgman clip against boundary_polyline_slot
    GEOM_CSG_CLIP_HALF_PLANE      = 1  // Half-plane clip against (plane_origin, inside_normal)
};

template <typename T = int32_t>
struct alignas(64) CSGClipAndSeamWeldTransformLogic {
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

    static constexpr std::string_view display_name = "CSG Clip & Seam Weld";
    static constexpr std::string_view ui_category  = "CSG 2D";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id              = INVALID_ID;                   // 4B
    uint32_t polyline_slice_buffer_id      = INVALID_ID;                   // 4B
    uint32_t polyline_vertex_2d_buffer_id  = INVALID_ID;                   // 4B
    uint32_t clip_classification_buffer_id = INVALID_ID;                   // 4B
    uint32_t polygon_mesh_slice_buffer_id  = INVALID_ID;                   // 4B
    uint32_t polygon_vertex_2d_buffer_id   = INVALID_ID;                   // 4B
    uint32_t polygon_index_u32_buffer_id   = INVALID_ID;                   // 4B
    uint32_t boundary_polyline_slot        = 0u;                           // 4B
    float plane_origin_x                   = 0.0f;                         // 4B
    float plane_origin_y                   = 0.0f;                         // 4B
    float inside_normal_x                  = 0.0f;                         // 4B
    float inside_normal_y                  = 1.0f;                         // 4B
    uint8_t clip_mode                      = GEOM_CSG_CLIP_CONVEX_BOUNDARY;// 1B
    bool only_clip_straddling              = true;                         // 1B
    uint8_t reserved_padding[14]           = {0};                          // 14B -> Total = 64B

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
        add_buf("clip_classification_buffer_id");
        add_buf("polygon_mesh_slice_buffer_id");
        add_buf("polygon_vertex_2d_buffer_id");
        add_buf("polygon_index_u32_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))              target_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("polyline_slice_buffer_id"))      polyline_slice_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_slice_buffer_id"]));
        if (p_props.has("polyline_vertex_2d_buffer_id"))  polyline_vertex_2d_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_2d_buffer_id"]));
        if (p_props.has("clip_classification_buffer_id")) clip_classification_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["clip_classification_buffer_id"]));
        if (p_props.has("polygon_mesh_slice_buffer_id"))  polygon_mesh_slice_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_mesh_slice_buffer_id"]));
        if (p_props.has("polygon_vertex_2d_buffer_id"))   polygon_vertex_2d_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_vertex_2d_buffer_id"]));
        if (p_props.has("polygon_index_u32_buffer_id"))   polygon_index_u32_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_index_u32_buffer_id"]));
        if (p_props.has("boundary_polyline_slot"))        boundary_polyline_slot        = static_cast<uint32_t>(static_cast<int64_t>(p_props["boundary_polyline_slot"]));
        if (p_props.has("clip_mode"))                     clip_mode                     = static_cast<uint8_t>(static_cast<int64_t>(p_props["clip_mode"]));
        if (p_props.has("only_clip_straddling"))          only_clip_straddling          = static_cast<bool>(p_props["only_clip_straddling"]);
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
        const GrantPartPOD *class_part = p_context.get_grant_part(clip_classification_buffer_id);
        const GrantPartPOD *mesh_part  = p_context.get_grant_part(polygon_mesh_slice_buffer_id);
        const GrantPartPOD *mvert_part = p_context.get_grant_part(polygon_vertex_2d_buffer_id);
        const GrantPartPOD *idx_part   = p_context.get_grant_part(polygon_index_u32_buffer_id);

        if (!pline_part || !pline_part->raw_base_ptr ||
            !pvert_part || !pvert_part->raw_base_ptr ||
            !mesh_part  || !mesh_part->raw_base_ptr) {
            return;
        }

        const auto *pline_slices = reinterpret_cast<const PolylineSlicePOD *>(pline_part->raw_base_ptr);
        const auto *pline_verts  = reinterpret_cast<const glm::vec2 *>(pvert_part->raw_base_ptr);
        const auto *cls_flags    = (class_part && class_part->raw_base_ptr)
            ? reinterpret_cast<const uint8_t *>(class_part->raw_base_ptr)
            : nullptr;
        auto *mesh_slices        = reinterpret_cast<PolygonMeshSlicePOD *>(mesh_part->raw_base_ptr);
        auto *mesh_verts         = (mvert_part && mvert_part->raw_base_ptr)
            ? reinterpret_cast<glm::vec2 *>(mvert_part->raw_base_ptr)
            : nullptr;
        auto *tri_indices        = (idx_part && idx_part->raw_base_ptr)
            ? reinterpret_cast<uint32_t *>(idx_part->raw_base_ptr)
            : nullptr;

        const int64_t poly_count = std::min(pline_part->selection.element_count, mesh_part->selection.capacity);
        const int64_t pvert_cap  = pvert_part->selection.capacity;
        const int64_t mvert_cap  = mvert_part ? mvert_part->selection.capacity : 0;
        const int64_t idx_cap    = idx_part ? idx_part->selection.capacity : 0;

        std::span<const glm::vec2> bnd_span{};
        if (static_cast<int64_t>(boundary_polyline_slot) < pline_part->selection.capacity) {
            const PolylineSlicePOD &bnd = pline_slices[boundary_polyline_slot];
            if (bnd.vertex_count >= 3u &&
                static_cast<int64_t>(bnd.vertex_offset + bnd.vertex_count) <= pvert_cap) {
                bnd_span = std::span<const glm::vec2>(pline_verts + bnd.vertex_offset, bnd.vertex_count);
            }
        }

        uint32_t write_vert_cursor = 0u;
        uint32_t write_idx_cursor  = 0u;
        std::vector<uint32_t> local_tris;

        for (int64_t i = 0; i < poly_count; ++i) {
            if (static_cast<uint32_t>(i) == boundary_polyline_slot && clip_mode == GEOM_CSG_CLIP_CONVEX_BOUNDARY) {
                continue;
            }
            if (only_clip_straddling && cls_flags && i < class_part->selection.capacity) {
                if (cls_flags[i] != static_cast<uint8_t>(ClipClassification2D::STRADDLING_BOUNDARY)) {
                    continue;
                }
            }

            const PolylineSlicePOD &ps = pline_slices[i];
            if (ps.vertex_count < 3u ||
                static_cast<int64_t>(ps.vertex_offset + ps.vertex_count) > pvert_cap) {
                continue;
            }

            const std::span<const glm::vec2> subj_span(pline_verts + ps.vertex_offset, ps.vertex_count);
            std::vector<glm::vec2> clipped;
            if (clip_mode == GEOM_CSG_CLIP_HALF_PLANE) {
                clipped = CSGMath2D<float>::clip_polygon_half_plane(
                    subj_span,
                    glm::vec2(plane_origin_x, plane_origin_y),
                    glm::vec2(inside_normal_x, inside_normal_y)
                );
            } else if (!bnd_span.empty()) {
                clipped = CSGMath2D<float>::clip_polygon_convex(subj_span, bnd_span);
            }

            PolygonMeshSlicePOD &ms = mesh_slices[i];
            if (clipped.size() < 3u) {
                ms = PolygonMeshSlicePOD{};
                continue;
            }

            local_tris.clear();
            PolygonMath2D<float>::triangulate_into(clipped, local_tris);
            ms.signed_area = PolygonMath2D<float>::signed_area(clipped);
            ms.centroid    = PolygonMath2D<float>::centroid(clipped);

            if (mesh_verts && static_cast<int64_t>(write_vert_cursor + clipped.size()) <= mvert_cap) {
                ms.vertex_offset = write_vert_cursor;
                ms.vertex_count  = static_cast<uint32_t>(clipped.size());
                std::copy(clipped.begin(), clipped.end(), mesh_verts + write_vert_cursor);
                write_vert_cursor += ms.vertex_count;
            }
            if (tri_indices && static_cast<int64_t>(write_idx_cursor + local_tris.size()) <= idx_cap) {
                ms.index_offset = write_idx_cursor;
                ms.index_count  = static_cast<uint32_t>(local_tris.size());
                std::copy(local_tris.begin(), local_tris.end(), tri_indices + write_idx_cursor);
                write_idx_cursor += ms.index_count;
            }
        }
    }
};

static_assert(sizeof(CSGClipAndSeamWeldTransformLogic<>) == 64, "CSGClipAndSeamWeldTransformLogic must be 64 bytes!");

} // namespace ideam::core
