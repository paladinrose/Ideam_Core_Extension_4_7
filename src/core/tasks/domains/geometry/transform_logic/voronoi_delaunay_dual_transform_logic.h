#pragma once

/**
 * voronoi_delaunay_dual_transform_logic.h
 * GeometryDOD Transform Task (G_T3): Builds a 2D Bowyer-Watson Delaunay triangulation,
 * clipped Voronoi cell polygons (`VoronoiMath2D::build_clipped_voronoi_cells`), optional
 * Lloyd centroidal relaxation, and a symmetric Compressed Sparse Row (CSR) dual adjacency
 * graph (`VoronoiMath2D::extract_delaunay_adjacency_csr`) directly in `GeometryDOD` buffers.
 */

#include "../../../../geometry/geometry_common.h"
#include "../../../../geometry/polygon_math_2d.h"
#include "../../../../geometry/voronoi_2d.h"
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

template <typename T = int32_t>
struct alignas(64) VoronoiDelaunayDualTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::VECTOR2;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Voronoi & Delaunay Dual Graph";
    static constexpr std::string_view ui_category  = "Voronoi 2D";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                = INVALID_ID; // 4B
    uint32_t voronoi_site_2d_buffer_id       = INVALID_ID; // 4B: glm::vec2 generator sites
    uint32_t voronoi_cell_slice_buffer_id    = INVALID_ID; // 4B: PolylineSlicePOD per Voronoi cell
    uint32_t polyline_vertex_2d_buffer_id    = INVALID_ID; // 4B: glm::vec2 cell polygon vertices
    uint32_t voronoi_csr_offsets_buffer_id   = INVALID_ID; // 4B: uint32_t CSR row_offsets (N+1)
    uint32_t voronoi_csr_neighbors_buffer_id = INVALID_ID; // 4B: uint32_t CSR col_neighbors
    float clip_min_x                         = -5.0f;      // 4B
    float clip_min_y                         = -5.0f;      // 4B
    float clip_max_x                         = 5.0f;       // 4B
    float clip_max_y                         = 5.0f;       // 4B
    float lloyd_step_strength                = 0.0f;       // 4B: >0 applies 1 Lloyd relaxation pass
    uint8_t lloyd_iterations                 = 0u;         // 1B
    uint8_t reserved_padding[19]             = {0};        // 19B -> Total = 64B

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
        add_buf("voronoi_site_2d_buffer_id");
        add_buf("voronoi_cell_slice_buffer_id");
        add_buf("polyline_vertex_2d_buffer_id");
        add_buf("voronoi_csr_offsets_buffer_id");
        add_buf("voronoi_csr_neighbors_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))                target_buffer_id                = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("voronoi_site_2d_buffer_id"))       voronoi_site_2d_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["voronoi_site_2d_buffer_id"]));
        if (p_props.has("voronoi_cell_slice_buffer_id"))    voronoi_cell_slice_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["voronoi_cell_slice_buffer_id"]));
        if (p_props.has("polyline_vertex_2d_buffer_id"))    polyline_vertex_2d_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_2d_buffer_id"]));
        if (p_props.has("voronoi_csr_offsets_buffer_id"))   voronoi_csr_offsets_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["voronoi_csr_offsets_buffer_id"]));
        if (p_props.has("voronoi_csr_neighbors_buffer_id")) voronoi_csr_neighbors_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["voronoi_csr_neighbors_buffer_id"]));
        if (p_props.has("clip_min_x"))                      clip_min_x                      = static_cast<float>(static_cast<double>(p_props["clip_min_x"]));
        if (p_props.has("clip_min_y"))                      clip_min_y                      = static_cast<float>(static_cast<double>(p_props["clip_min_y"]));
        if (p_props.has("clip_max_x"))                      clip_max_x                      = static_cast<float>(static_cast<double>(p_props["clip_max_x"]));
        if (p_props.has("clip_max_y"))                      clip_max_y                      = static_cast<float>(static_cast<double>(p_props["clip_max_y"]));
        if (p_props.has("lloyd_step_strength"))             lloyd_step_strength             = static_cast<float>(static_cast<double>(p_props["lloyd_step_strength"]));
        if (p_props.has("lloyd_iterations"))                lloyd_iterations                = static_cast<uint8_t>(static_cast<int64_t>(p_props["lloyd_iterations"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : voronoi_site_2d_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *site_part = p_context.get_grant_part(voronoi_site_2d_buffer_id);
        const GrantPartPOD *cell_part = p_context.get_grant_part(voronoi_cell_slice_buffer_id);
        const GrantPartPOD *vert_part = p_context.get_grant_part(polyline_vertex_2d_buffer_id);
        const GrantPartPOD *roff_part = p_context.get_grant_part(voronoi_csr_offsets_buffer_id);
        const GrantPartPOD *cnbr_part = p_context.get_grant_part(voronoi_csr_neighbors_buffer_id);

        if (!site_part || !site_part->raw_base_ptr) return;

        auto *sites = reinterpret_cast<glm::vec2 *>(site_part->raw_base_ptr);
        const size_t site_count = static_cast<size_t>(std::max<int64_t>(0, site_part->selection.element_count));
        if (site_count == 0u) return;

        const AABB2D<float> bounds(
            glm::vec2(clip_min_x, clip_min_y),
            glm::vec2(clip_max_x, clip_max_y)
        );

        std::span<glm::vec2> site_span(sites, site_count);
        std::vector<VoronoiCell2D<float>> cells = VoronoiMath2D<float>::build_clipped_voronoi_cells(site_span, bounds);

        const uint8_t relax_iters = (lloyd_step_strength > 1e-5f && lloyd_iterations == 0u) ? 1u : lloyd_iterations;
        const float alpha = std::clamp(lloyd_step_strength, 0.0f, 1.0f);
        for (uint8_t iter = 0u; iter < relax_iters; ++iter) {
            for (size_t i = 0; i < site_count && i < cells.size(); ++i) {
                if (cells[i].polygon_vertices.size() >= 3u) {
                    const glm::vec2 c = PolygonMath2D<float>::centroid(cells[i].polygon_vertices);
                    site_span[i] = glm::mix(site_span[i], c, alpha);
                }
            }
            cells = VoronoiMath2D<float>::build_clipped_voronoi_cells(site_span, bounds);
        }

        if (cell_part && cell_part->raw_base_ptr) {
            auto *cell_slices = reinterpret_cast<PolylineSlicePOD *>(cell_part->raw_base_ptr);
            auto *vert_out    = (vert_part && vert_part->raw_base_ptr)
                ? reinterpret_cast<glm::vec2 *>(vert_part->raw_base_ptr)
                : nullptr;
            const int64_t cell_cap = cell_part->selection.capacity;
            const int64_t vert_cap = vert_part ? vert_part->selection.capacity : 0;
            uint32_t vert_cursor   = 0u;

            for (size_t i = 0; i < cells.size() && static_cast<int64_t>(i) < cell_cap; ++i) {
                PolylineSlicePOD &cs = cell_slices[i];
                const auto &pverts   = cells[i].polygon_vertices;
                const AABB2D<float> box = PolygonMath2D<float>::compute_aabb(pverts);
                cs.dimension     = 2u;
                cs.is_closed     = 1u;
                cs.winding_order = 1u;
                cs.total_length  = PolygonMath2D<float>::perimeter(pverts, true);
                cs.bounds_min    = box.min_bounds;
                cs.bounds_max    = box.max_bounds;

                if (vert_out && static_cast<int64_t>(vert_cursor + pverts.size()) <= vert_cap) {
                    cs.vertex_offset = vert_cursor;
                    cs.vertex_count  = static_cast<uint32_t>(pverts.size());
                    std::copy(pverts.begin(), pverts.end(), vert_out + vert_cursor);
                    vert_cursor += cs.vertex_count;
                } else {
                    cs.vertex_offset = 0u;
                    cs.vertex_count  = 0u;
                }
            }
        }

        if (roff_part && roff_part->raw_base_ptr && cnbr_part && cnbr_part->raw_base_ptr && site_count >= 3u) {
            const std::vector<uint32_t> tris = VoronoiMath2D<float>::triangulate_delaunay(site_span);
            std::vector<uint32_t> row_offsets;
            std::vector<uint32_t> col_neighbors;
            VoronoiMath2D<float>::extract_delaunay_adjacency_csr(site_count, tris, row_offsets, col_neighbors);

            auto *roff_out = reinterpret_cast<uint32_t *>(roff_part->raw_base_ptr);
            auto *cnbr_out = reinterpret_cast<uint32_t *>(cnbr_part->raw_base_ptr);
            const size_t r_copy = std::min<size_t>(row_offsets.size(), static_cast<size_t>(roff_part->selection.capacity));
            const size_t c_copy = std::min<size_t>(col_neighbors.size(), static_cast<size_t>(cnbr_part->selection.capacity));
            std::copy_n(row_offsets.data(), r_copy, roff_out);
            std::copy_n(col_neighbors.data(), c_copy, cnbr_out);
        }
    }
};

static_assert(sizeof(VoronoiDelaunayDualTransformLogic<>) == 64, "VoronoiDelaunayDualTransformLogic must be 64 bytes!");

} // namespace ideam::core
