#pragma once

/**
 * tiling_lattice_cull_and_mask_query_logic.h
 * TileDOD Query Task (T_Q1): Wave 2 Wallpaper Lattice AABB & Spline2D Boundary Classification.
 *
 * Evaluates each `IsoCellPOD` in `TileDOD` against:
 *   1. `cull_min` / `cull_max` 2D AABB bounds, AND/OR
 *   2. A closed 2D boundary polygon / flattened `Spline2D` contour stored in `GeometryDOD`'s
 *      bridged `polyline_slice_buffer_id` and `polyline_vertex_2d_buffer_id`.
 *
 * Writes `ClipClassification2D` (`STRICTLY_OUTSIDE = 0`, `STRADDLING_BOUNDARY = 1`,
 * `FULLY_INSIDE = 2`) into `clip_classification_buffer_id` and filters cells according to
 * `coverage_mode` (`TilingCoverageMode`).
 */

#include "../../../../geometry/csg_2d.h"
#include "../../../../geometry/polygon_math_2d.h"
#include "../../../../tiles/tile_common.h"
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

template <typename T = int32_t>
struct alignas(64) TilingLatticeCullAndMaskQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Tiling Lattice & Spline Mask Query";
    static constexpr std::string_view ui_category  = "Tiles";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id              = INVALID_ID; // 4B
    uint32_t iso_cell_buffer_id            = INVALID_ID; // 4B: IsoCellPOD (32B AoS)
    uint32_t cell_vertex_csr_buffer_id     = INVALID_ID; // 4B: uint32_t CSR vertex indices
    uint32_t polygon_vertex_2d_buffer_id   = INVALID_ID; // 4B: Bridged GeometryDOD glm::vec2
    uint32_t polyline_slice_buffer_id      = INVALID_ID; // 4B: Bridged GeometryDOD PolylineSlicePOD
    uint32_t polyline_vertex_2d_buffer_id  = INVALID_ID; // 4B: Bridged GeometryDOD glm::vec2
    uint32_t clip_classification_buffer_id = INVALID_ID; // 4B: Bridged GeometryDOD uint8_t flags
    uint32_t boundary_polyline_slot        = INVALID_ID; // 4B: Active boundary contour slot
    glm::vec2 cull_min{-1.0e5f, -1.0e5f};                // 8B
    glm::vec2 cull_max{ 1.0e5f,  1.0e5f};                // 8B
    uint8_t coverage_mode                  = static_cast<uint8_t>(TilingCoverageMode::INCLUDE_STRADDLING); // 1B
    uint8_t filter_group_slot              = 0xFFu;      // 1B: 0xFF = accept all group_slots
    bool negate_result                     = false;      // 1B
    uint8_t reserved_padding[13]           = {0};        // 13B -> Total = 64B

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
        add_buf("cell_vertex_csr_buffer_id");
        add_buf("polygon_vertex_2d_buffer_id");
        add_buf("polyline_slice_buffer_id");
        add_buf("polyline_vertex_2d_buffer_id");
        add_buf("clip_classification_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))              target_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("iso_cell_buffer_id"))            iso_cell_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["iso_cell_buffer_id"]));
        if (p_props.has("cell_vertex_csr_buffer_id"))     cell_vertex_csr_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["cell_vertex_csr_buffer_id"]));
        if (p_props.has("polygon_vertex_2d_buffer_id"))   polygon_vertex_2d_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_vertex_2d_buffer_id"]));
        if (p_props.has("polyline_slice_buffer_id"))      polyline_slice_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_slice_buffer_id"]));
        if (p_props.has("polyline_vertex_2d_buffer_id"))  polyline_vertex_2d_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_2d_buffer_id"]));
        if (p_props.has("clip_classification_buffer_id")) clip_classification_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["clip_classification_buffer_id"]));
        if (p_props.has("boundary_polyline_slot"))        boundary_polyline_slot        = static_cast<uint32_t>(static_cast<int64_t>(p_props["boundary_polyline_slot"]));
        if (p_props.has("coverage_mode"))                 coverage_mode                 = static_cast<uint8_t>(static_cast<int64_t>(p_props["coverage_mode"]));
        if (p_props.has("filter_group_slot"))             filter_group_slot             = static_cast<uint8_t>(static_cast<int64_t>(p_props["filter_group_slot"]));
        if (p_props.has("negate_result"))                 negate_result                 = static_cast<bool>(p_props["negate_result"]);
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
    [[nodiscard]] inline bool evaluate_element(
        int64_t p_index,
        const T_View &p_view,
        const TaskContextPOD &p_context
    ) const noexcept {
        (void)p_view;
        const GrantPartPOD *cell_part = p_context.get_grant_part(iso_cell_buffer_id);
        if (!cell_part || !cell_part->raw_base_ptr || p_index < 0 || p_index >= cell_part->selection.capacity) {
            return false;
        }

        const auto *cells       = reinterpret_cast<const IsoCellPOD *>(cell_part->raw_base_ptr);
        const IsoCellPOD &cell  = cells[p_index];

        const GrantPartPOD *flg_part = p_context.get_grant_part(clip_classification_buffer_id);
        uint8_t *cls_flags = (flg_part && flg_part->raw_base_ptr && p_index < flg_part->selection.capacity)
            ? reinterpret_cast<uint8_t *>(flg_part->raw_base_ptr)
            : nullptr;

        if ((cell.flags & CELL_FLAG_MASKED) != 0u) {
            if (cls_flags) cls_flags[p_index] = static_cast<uint8_t>(ClipClassification2D::STRICTLY_OUTSIDE);
            return negate_result;
        }
        if (filter_group_slot != 0xFFu && cell.group_slot != filter_group_slot) {
            return negate_result;
        }

        // 1. Coarse AABB Cull
        if (cell.center.x < cull_min.x || cell.center.x > cull_max.x ||
            cell.center.y < cull_min.y || cell.center.y > cull_max.y) {
            if (cls_flags) cls_flags[p_index] = static_cast<uint8_t>(ClipClassification2D::STRICTLY_OUTSIDE);
            return negate_result;
        }

        // 2. Check optional Bridged Spline2D boundary contour
        const GrantPartPOD *sl_part = p_context.get_grant_part(polyline_slice_buffer_id);
        const GrantPartPOD *vt_part = p_context.get_grant_part(polyline_vertex_2d_buffer_id);
        const glm::vec2 *boundary_verts = nullptr;
        uint32_t boundary_count         = 0u;

        if (boundary_polyline_slot != INVALID_ID &&
            sl_part && sl_part->raw_base_ptr &&
            vt_part && vt_part->raw_base_ptr &&
            static_cast<int64_t>(boundary_polyline_slot) < sl_part->selection.capacity) {
            const PolylineSlicePOD &sl = reinterpret_cast<const PolylineSlicePOD *>(sl_part->raw_base_ptr)[boundary_polyline_slot];
            if (sl.vertex_count >= 3u &&
                static_cast<int64_t>(sl.vertex_offset + sl.vertex_count) <= vt_part->selection.capacity) {
                boundary_verts = reinterpret_cast<const glm::vec2 *>(vt_part->raw_base_ptr) + sl.vertex_offset;
                boundary_count = sl.vertex_count;
            }
        }

        bool match = true;
        if (!boundary_verts || boundary_count < 3u) {
            const ClipClassification2D cls = ((cell.flags & (CELL_FLAG_ON_BORDER | CELL_FLAG_CLIPPED_CAP)) != 0u)
                ? ClipClassification2D::STRADDLING_BOUNDARY
                : ClipClassification2D::FULLY_INSIDE;
            if (cls_flags) cls_flags[p_index] = static_cast<uint8_t>(cls);
            match = (coverage_mode == static_cast<uint8_t>(TilingCoverageMode::STRICT_INTERIOR))
                ? (cls == ClipClassification2D::FULLY_INSIDE)
                : true;
        } else {
            const std::span<const glm::vec2> boundary_span(boundary_verts, boundary_count);
            const GrantPartPOD *csr_part = p_context.get_grant_part(cell_vertex_csr_buffer_id);
            const GrantPartPOD *pv_part  = p_context.get_grant_part(polygon_vertex_2d_buffer_id);

            uint32_t inside_verts = 0u;
            const uint32_t v_cnt  = cell.vertex_count;
            if (csr_part && csr_part->raw_base_ptr && pv_part && pv_part->raw_base_ptr &&
                cell.first_vertex_idx != INVALID_ID &&
                static_cast<int64_t>(cell.first_vertex_idx + v_cnt) <= csr_part->selection.capacity) {
                const auto *csr = reinterpret_cast<const uint32_t *>(csr_part->raw_base_ptr);
                const auto *pvs = reinterpret_cast<const glm::vec2 *>(pv_part->raw_base_ptr);
                const int64_t pv_cap = pv_part->selection.capacity;
                for (uint32_t k = 0; k < v_cnt; ++k) {
                    const uint32_t vid = csr[cell.first_vertex_idx + k];
                    if (static_cast<int64_t>(vid) < pv_cap &&
                        PolygonMath2D<float>::contains_point(boundary_span, pvs[vid])) {
                        ++inside_verts;
                    }
                }
            }

            const bool center_in = PolygonMath2D<float>::contains_point(boundary_span, cell.center);
            ClipClassification2D cls = ClipClassification2D::STRICTLY_OUTSIDE;
            if (v_cnt > 0u && inside_verts == v_cnt) {
                cls = ClipClassification2D::FULLY_INSIDE;
            } else if (inside_verts > 0u || center_in) {
                cls = ClipClassification2D::STRADDLING_BOUNDARY;
            }
            if (cls_flags) cls_flags[p_index] = static_cast<uint8_t>(cls);

            switch (static_cast<TilingCoverageMode>(coverage_mode)) {
                case TilingCoverageMode::STRICT_INTERIOR:
                    match = (cls == ClipClassification2D::FULLY_INSIDE);
                    break;
                case TilingCoverageMode::CENTROID_INSIDE:
                    match = center_in;
                    break;
                case TilingCoverageMode::INCLUDE_STRADDLING:
                case TilingCoverageMode::CLIP_EXACT_TO_SPLINE:
                default:
                    match = (cls != ClipClassification2D::STRICTLY_OUTSIDE);
                    break;
            }
        }

        return negate_result ? !match : match;
    }
};

static_assert(
    sizeof(TilingLatticeCullAndMaskQueryLogic<>) == 64,
    "TilingLatticeCullAndMaskQueryLogic must be 64 bytes!"
);

} // namespace ideam::core
