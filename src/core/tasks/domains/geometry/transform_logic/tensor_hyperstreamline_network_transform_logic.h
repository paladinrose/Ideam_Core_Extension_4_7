#pragma once

/**
 * tensor_hyperstreamline_network_transform_logic.h
 * G_T6: Traces orthogonal Major and Minor RK4 Hyperstreamlines across a baked 2D Tensor Field
 * (`GEOM_TENSOR_CELL_POD`), splits segment intersections, welds planar road/grid graph vertices,
 * populates `GEOM_POLYLINE_SLICE_POD` + `GEOM_POLYLINE_VERTEX_2D` with traced streamlines,
 * builds DCEL half-edges in `GEOM_PLANAR_HALF_EDGE_POD`, and extracts closed parcel polygons
 * into `GEOM_POLYGON_MESH_SLICE_POD` + `GEOM_POLYGON_VERTEX_2D`.
 *
 * Target View / Strategy: `SingleElementView` / `FlatStrategy`
 * UI Category: `"Geometry/TensorField"`
 */

#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "../../../../geometry/tensor_field_2d.h"
#include "../../../../memory/memory_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <typename T = float>
struct alignas(64) TensorHyperstreamlineNetworkTransformLogic {
    using ValueType = T;

    // --- Compile-Time DOD Traits ---
    static constexpr std::string_view display_name        = "Tensor Field: Trace Hyperstreamlines & Parcel Regions";
    static constexpr std::string_view ui_category         = "Geometry/TensorField";
    static constexpr ViewCapability required_capabilities = ViewCapability::READ_WRITE;
    static constexpr ViewLayout compatible_layouts        = ViewLayout::SINGLE_ELEMENT;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32 | DataType::BYTE;
    static constexpr size_t required_dimensions           = 0;

    // --- 64-Byte Cache-Line Layout (16 x 4B = 64 Bytes) ---
    uint32_t target_buffer_id                = INVALID_ID; // 4B [0..3]   (GEOM_POLYLINE_SLICE_POD)
    uint32_t tensor_grid_header_buffer_id    = INVALID_ID; // 4B [4..7]   (GEOM_TENSOR_GRID_HEADER_POD)
    uint32_t tensor_cell_buffer_id           = INVALID_ID; // 4B [8..11]  (GEOM_TENSOR_CELL_POD)
    uint32_t tensor_basis_buffer_id          = INVALID_ID; // 4B [12..15] (GEOM_TENSOR_BASIS_POD)
    uint32_t tensor_seed_buffer_id           = INVALID_ID; // 4B [16..19] (GEOM_TENSOR_STREAMLINE_SEED_POD)
    uint32_t polyline_slice_buffer_id        = INVALID_ID; // 4B [20..23] (GEOM_POLYLINE_SLICE_POD)
    uint32_t polyline_vertex_2d_buffer_id    = INVALID_ID; // 4B [24..27] (GEOM_POLYLINE_VERTEX_2D)
    uint32_t planar_half_edge_buffer_id      = INVALID_ID; // 4B [28..31] (GEOM_PLANAR_HALF_EDGE_POD)
    uint32_t polygon_mesh_slice_buffer_id    = INVALID_ID; // 4B [32..35] (GEOM_POLYGON_MESH_SLICE_POD)
    uint32_t polygon_vertex_2d_buffer_id     = INVALID_ID; // 4B [36..39] (GEOM_POLYGON_VERTEX_2D)
    float major_separation                   = 1.25f;      // 4B [40..43]
    float minor_separation                   = 1.25f;      // 4B [44..47]
    float step_size                          = 0.20f;      // 4B [48..51]
    float merge_distance                     = 0.35f;      // 4B [52..55]
    uint32_t max_steps_per_streamline        = 192u;       // 4B [56..59]
    uint32_t extract_closed_regions          = 1u;         // 4B [60..63]

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : polyline_slice_buffer_id;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, const godot::Variant &p_default) {
            godot::Dictionary d;
            d["name"]    = godot::String(p_name);
            d["type"]    = static_cast<int64_t>(p_type);
            d["default"] = p_default;
            d["hint"]    = godot::PROPERTY_HINT_NONE;
            d["usage"]   = godot::PROPERTY_USAGE_DEFAULT;
            props.push_back(d);
        };
        add_prop("target_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_grid_header_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_cell_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_basis_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("tensor_seed_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("polyline_slice_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("polyline_vertex_2d_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("planar_half_edge_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("polygon_mesh_slice_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("polygon_vertex_2d_buffer_id", godot::Variant::INT, static_cast<int64_t>(INVALID_ID));
        add_prop("major_separation", godot::Variant::FLOAT, 1.25);
        add_prop("minor_separation", godot::Variant::FLOAT, 1.25);
        add_prop("step_size", godot::Variant::FLOAT, 0.20);
        add_prop("merge_distance", godot::Variant::FLOAT, 0.35);
        add_prop("max_steps_per_streamline", godot::Variant::INT, int64_t(192));
        add_prop("extract_closed_regions", godot::Variant::INT, int64_t(1));
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        auto read_u32 = [&](const char *p_key, uint32_t &r_dst) {
            const godot::String k(p_key);
            if (p_props.has(k)) {
                r_dst = static_cast<uint32_t>(static_cast<int64_t>(p_props[k]));
            }
        };
        read_u32("target_buffer_id", target_buffer_id);
        read_u32("tensor_grid_header_buffer_id", tensor_grid_header_buffer_id);
        read_u32("tensor_cell_buffer_id", tensor_cell_buffer_id);
        read_u32("tensor_basis_buffer_id", tensor_basis_buffer_id);
        read_u32("tensor_seed_buffer_id", tensor_seed_buffer_id);
        read_u32("polyline_slice_buffer_id", polyline_slice_buffer_id);
        read_u32("polyline_vertex_2d_buffer_id", polyline_vertex_2d_buffer_id);
        read_u32("planar_half_edge_buffer_id", planar_half_edge_buffer_id);
        read_u32("polygon_mesh_slice_buffer_id", polygon_mesh_slice_buffer_id);
        read_u32("polygon_vertex_2d_buffer_id", polygon_vertex_2d_buffer_id);
        read_u32("max_steps_per_streamline", max_steps_per_streamline);
        read_u32("extract_closed_regions", extract_closed_regions);

        if (p_props.has("major_separation")) {
            major_separation = static_cast<float>(static_cast<double>(p_props["major_separation"]));
        }
        if (p_props.has("minor_separation")) {
            minor_separation = static_cast<float>(static_cast<double>(p_props["minor_separation"]));
        }
        if (p_props.has("step_size")) {
            step_size = static_cast<float>(static_cast<double>(p_props["step_size"]));
        }
        if (p_props.has("merge_distance")) {
            merge_distance = static_cast<float>(static_cast<double>(p_props["merge_distance"]));
        }
        if (target_buffer_id == INVALID_ID) {
            target_buffer_id = polyline_slice_buffer_id;
        }
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        MemoryManagerDOD *mgr = p_context.manager;
        if (!mgr) return;

        auto get_max_span = [&]<typename Elem>(uint32_t p_buf_id) noexcept -> std::span<Elem> {
            if (p_buf_id == INVALID_ID) return {};
            MemoryBufferPOD *buf = mgr->get_buffer(p_buf_id);
            if (!buf || !buf->master_block_ptr || buf->max_elements <= 0) return {};
            return std::span<Elem>(
                reinterpret_cast<Elem *>(buf->master_block_ptr + buf->memory_offset),
                static_cast<size_t>(buf->max_elements)
            );
        };

        auto headers      = get_max_span.template operator()<const TensorGridHeaderPod>(tensor_grid_header_buffer_id);
        auto cells        = get_max_span.template operator()<const SymmetricTensor2DPod>(tensor_cell_buffer_id);
        auto bases        = get_max_span.template operator()<const TensorBasisElementPod>(tensor_basis_buffer_id);
        auto seeds        = get_max_span.template operator()<const TensorStreamlineSeedPod>(tensor_seed_buffer_id);
        auto poly_slices  = get_max_span.template operator()<PolylineSlicePOD>(get_target_buffer_id());
        auto poly_verts   = get_max_span.template operator()<glm::vec2>(polyline_vertex_2d_buffer_id);
        auto half_edges   = get_max_span.template operator()<PlanarHalfEdgePOD>(planar_half_edge_buffer_id);
        auto mesh_slices  = get_max_span.template operator()<PolygonMeshSlicePOD>(polygon_mesh_slice_buffer_id);
        auto mesh_verts   = get_max_span.template operator()<glm::vec2>(polygon_vertex_2d_buffer_id);

        if (headers.empty() && bases.empty()) return;

        const TensorGridHeaderPod hdr = !headers.empty() ? headers[0] : TensorGridHeaderPod{};
        TensorTracingConfig2D cfg{};
        cfg.world_bounds = AABB2D<float>(
            hdr.origin,
            hdr.origin + glm::vec2(
                static_cast<float>(hdr.width) * hdr.cell_size.x,
                static_cast<float>(hdr.height) * hdr.cell_size.y
            )
        );
        cfg.major_separation        = major_separation;
        cfg.minor_separation        = minor_separation;
        cfg.step_size               = step_size;
        cfg.merge_distance          = merge_distance;
        cfg.max_steps_per_stream    = std::max(8u, max_steps_per_streamline);
        cfg.extract_closed_regions  = (extract_closed_regions != 0u);

        MemoryBufferPOD *seed_buf = mgr->get_buffer(tensor_seed_buffer_id);
        const size_t active_seed_cnt = (seed_buf && seed_buf->current_count > 0)
            ? std::min<size_t>(static_cast<size_t>(seed_buf->current_count), seeds.size())
            : 0u;

        const TensorNetworkResult2D net = TensorFieldMath2D::build_hyperstreamline_network(
            hdr,
            cells,
            bases,
            seeds.subspan(0, active_seed_cnt),
            cfg
        );

        // 1. Write traced streamlines into PolylineBuffer
        uint32_t v_cursor = 0u;
        uint32_t s_cursor = 0u;
        for (size_t i = 0; i < net.streamlines.size() && s_cursor < poly_slices.size(); ++i) {
            const auto &pts = net.streamlines[i];
            if (pts.size() < 2u || v_cursor + pts.size() > poly_verts.size()) break;

            std::copy(pts.begin(), pts.end(), poly_verts.data() + v_cursor);
            const AABB2D<float> box = PolygonMath2D<float>::compute_aabb(pts);

            PolylineSlicePOD slice{};
            slice.vertex_offset = v_cursor;
            slice.vertex_count  = static_cast<uint32_t>(pts.size());
            slice.total_length  = PolygonMath2D<float>::perimeter(pts, false);
            slice.dimension     = 2u;
            slice.is_closed     = 0u;
            slice.flags         = (i < net.streamline_is_minor.size()) ? net.streamline_is_minor[i] : 0u;
            slice.bounds_min    = box.min_bounds;
            slice.bounds_max    = box.max_bounds;

            poly_slices[s_cursor++] = slice;
            v_cursor += static_cast<uint32_t>(pts.size());
        }
        if (MemoryBufferPOD *ps_buf = mgr->get_buffer(get_target_buffer_id())) {
            ps_buf->current_count = static_cast<int64_t>(s_cursor);
            ps_buf->used_bytes    = static_cast<size_t>(s_cursor) * ps_buf->element_stride;
        }
        if (MemoryBufferPOD *pv_buf = mgr->get_buffer(polyline_vertex_2d_buffer_id)) {
            pv_buf->current_count = static_cast<int64_t>(v_cursor);
            pv_buf->used_bytes    = static_cast<size_t>(v_cursor) * pv_buf->element_stride;
        }

        // 2. Write DCEL half-edges into PlanarHalfEdgeBuffer
        if (!half_edges.empty() && !net.half_edges.empty()) {
            const size_t he_cnt = std::min(half_edges.size(), net.half_edges.size());
            std::copy_n(net.half_edges.data(), he_cnt, half_edges.data());
            if (MemoryBufferPOD *he_buf = mgr->get_buffer(planar_half_edge_buffer_id)) {
                he_buf->current_count = static_cast<int64_t>(he_cnt);
                he_buf->used_bytes    = he_cnt * he_buf->element_stride;
            }
        }

        // 3. Write extracted closed parcel regions into PolygonMeshBuffer
        if (!mesh_slices.empty() && !mesh_verts.empty() && !net.closed_regions.empty()) {
            uint32_t mv_cursor = 0u;
            uint32_t ms_cursor = 0u;
            for (const auto &region : net.closed_regions) {
                if (ms_cursor >= mesh_slices.size() || mv_cursor + region.size() > mesh_verts.size()) break;
                std::copy(region.begin(), region.end(), mesh_verts.data() + mv_cursor);

                PolygonMeshSlicePOD ms{};
                ms.vertex_offset        = mv_cursor;
                ms.vertex_count         = static_cast<uint32_t>(region.size());
                ms.signed_area          = PolygonMath2D<float>::signed_area(region);
                ms.centroid             = PolygonMath2D<float>::centroid(region);
                ms.material_or_group_id = ms_cursor;

                mesh_slices[ms_cursor++] = ms;
                mv_cursor += static_cast<uint32_t>(region.size());
            }
            if (MemoryBufferPOD *ms_buf = mgr->get_buffer(polygon_mesh_slice_buffer_id)) {
                ms_buf->current_count = static_cast<int64_t>(ms_cursor);
                ms_buf->used_bytes    = static_cast<size_t>(ms_cursor) * ms_buf->element_stride;
            }
            if (MemoryBufferPOD *mv_buf = mgr->get_buffer(polygon_vertex_2d_buffer_id)) {
                mv_buf->current_count = static_cast<int64_t>(mv_cursor);
                mv_buf->used_bytes    = static_cast<size_t>(mv_cursor) * mv_buf->element_stride;
            }
        }
    }
};

static_assert(sizeof(TensorHyperstreamlineNetworkTransformLogic<float>) == 64, "TensorHyperstreamlineNetworkTransformLogic must be 64 bytes!");

} // namespace ideam::core
