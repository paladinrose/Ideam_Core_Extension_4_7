#pragma once

/**
 * polygon_boundary_cull_query_logic.h
 * GeometryDOD Query Task (G_Q1): Classifies 2D polylines/polygons against a reference
 * closed boundary polygon (`CSGMath2D::classify_polygon_against_boundary`), writing
 * `ClipClassification2D` (`FULLY_OUTSIDE`, `FULLY_INSIDE`, `STRADDLING_BOUNDARY`) into
 * `GEOM_CLIP_CLASSIFICATION_FLAGS` and filtering the selection bitmask.
 *
 * Directly powers Wave 2 boundary cell classification for both `GeometryDOD` and `TileDOD`
 * (separating interior instances that need zero clipping from boundary-straddling instances).
 */

#include "../../../../geometry/csg_2d.h"
#include "../../../../geometry/geometry_common.h"
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

enum GeometryBoundaryFilterMode : uint8_t {
    GEOM_FILTER_NON_OUTSIDE         = 0, // Matches FULLY_INSIDE or STRADDLING_BOUNDARY
    GEOM_FILTER_STRICT_INTERIOR     = 1, // Matches only FULLY_INSIDE (InteriorBitmask)
    GEOM_FILTER_STRADDLING_BOUNDARY = 2, // Matches only STRADDLING_BOUNDARY (BoundarySeamBitmask)
    GEOM_FILTER_FULLY_OUTSIDE       = 3  // Matches only FULLY_OUTSIDE
};

template <typename T = int32_t>
struct alignas(64) PolygonBoundaryCullQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Polygon Boundary Cull";
    static constexpr std::string_view ui_category  = "Spatial";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id              = INVALID_ID;              // 4B: Primary selection buffer
    uint32_t polyline_slice_buffer_id      = INVALID_ID;              // 4B: PolylineSlicePOD (32B AoS)
    uint32_t polyline_vertex_2d_buffer_id  = INVALID_ID;              // 4B: Shared glm::vec2 vertex array
    uint32_t clip_classification_buffer_id = INVALID_ID;              // 4B: uint8_t ClipClassification2D output column
    uint32_t boundary_polyline_slot        = 0u;                      // 4B: Index of reference boundary contour in polyline_slice_buffer_id
    uint8_t filter_mode                    = GEOM_FILTER_NON_OUTSIDE; // 1B: GeometryBoundaryFilterMode
    bool negate_result                     = false;                   // 1B
    uint8_t reserved_padding[42]           = {0};                     // 42B -> Total = 64B

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
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))              target_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("polyline_slice_buffer_id"))      polyline_slice_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_slice_buffer_id"]));
        if (p_props.has("polyline_vertex_2d_buffer_id"))  polyline_vertex_2d_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_2d_buffer_id"]));
        if (p_props.has("clip_classification_buffer_id")) clip_classification_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["clip_classification_buffer_id"]));
        if (p_props.has("boundary_polyline_slot"))        boundary_polyline_slot        = static_cast<uint32_t>(static_cast<int64_t>(p_props["boundary_polyline_slot"]));
        if (p_props.has("filter_mode"))                   filter_mode                   = static_cast<uint8_t>(static_cast<int64_t>(p_props["filter_mode"]));
        if (p_props.has("negate_result"))                 negate_result                 = static_cast<bool>(p_props["negate_result"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : clip_classification_buffer_id;
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
        const GrantPartPOD *slice_part = p_context.get_grant_part(polyline_slice_buffer_id);
        const GrantPartPOD *vert_part  = p_context.get_grant_part(polyline_vertex_2d_buffer_id);
        const GrantPartPOD *class_part = p_context.get_grant_part(clip_classification_buffer_id);

        if (!slice_part || !slice_part->raw_base_ptr || !vert_part || !vert_part->raw_base_ptr) {
            return !negate_result;
        }

        const auto *slices = reinterpret_cast<const PolylineSlicePOD *>(slice_part->raw_base_ptr);
        const auto *verts  = reinterpret_cast<const glm::vec2 *>(vert_part->raw_base_ptr);
        auto *out_flags    = (class_part && class_part->raw_base_ptr)
            ? reinterpret_cast<uint8_t *>(class_part->raw_base_ptr)
            : nullptr;

        const int64_t slice_cap = slice_part->selection.capacity;
        const int64_t vert_cap  = vert_part->selection.capacity;
        if (p_index < 0 || p_index >= slice_cap || static_cast<int64_t>(boundary_polyline_slot) >= slice_cap) {
            return false;
        }

        if (static_cast<uint32_t>(p_index) == boundary_polyline_slot) {
            if (out_flags && p_index < class_part->selection.capacity) {
                out_flags[p_index] = static_cast<uint8_t>(ClipClassification2D::FULLY_INSIDE);
            }
            return !negate_result;
        }

        const PolylineSlicePOD &subj = slices[p_index];
        const PolylineSlicePOD &bnd  = slices[boundary_polyline_slot];
        if (subj.vertex_count < 3u || bnd.vertex_count < 3u ||
            static_cast<int64_t>(subj.vertex_offset + subj.vertex_count) > vert_cap ||
            static_cast<int64_t>(bnd.vertex_offset + bnd.vertex_count) > vert_cap) {
            return false;
        }

        const std::span<const glm::vec2> subj_span(verts + subj.vertex_offset, subj.vertex_count);
        const std::span<const glm::vec2> bnd_span(verts + bnd.vertex_offset, bnd.vertex_count);
        const AABB2D<float> bnd_aabb(bnd.bounds_min, bnd.bounds_max);

        const ClipClassification2D cls = CSGMath2D<float>::classify_polygon_against_boundary(
            subj_span, bnd_span, bnd_aabb
        );

        if (out_flags && p_index < class_part->selection.capacity) {
            out_flags[p_index] = static_cast<uint8_t>(cls);
        }

        bool match = false;
        switch (filter_mode) {
            case GEOM_FILTER_STRICT_INTERIOR:
                match = (cls == ClipClassification2D::FULLY_INSIDE);
                break;
            case GEOM_FILTER_STRADDLING_BOUNDARY:
                match = (cls == ClipClassification2D::STRADDLING_BOUNDARY);
                break;
            case GEOM_FILTER_FULLY_OUTSIDE:
                match = (cls == ClipClassification2D::FULLY_OUTSIDE);
                break;
            case GEOM_FILTER_NON_OUTSIDE:
            default:
                match = (cls != ClipClassification2D::FULLY_OUTSIDE);
                break;
        }
        return negate_result ? !match : match;
    }
};

static_assert(sizeof(PolygonBoundaryCullQueryLogic<>) == 64, "PolygonBoundaryCullQueryLogic must be 64 bytes!");

} // namespace ideam::core
