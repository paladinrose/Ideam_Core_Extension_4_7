#pragma once

/**
 * tiling_boundary_clip_and_seam_stitch_transform_logic.h
 * TileDOD Transform Task (T_T2): Wave 3B Boundary Cell Clipping & Curvilinear Seam Morphing.
 *
 * Updates boundary-clipped cells (`CELL_FLAG_ON_BORDER | CELL_FLAG_CLIPPED_CAP`) and
 * seam-bridge quad cells (`CELL_FLAG_BRIDGE_QUAD` / `BridgeQuadCellPOD`) in `GeometryDOD`'s
 * bridged `polygon_vertex_2d_buffer_id` and `polyline_slice_buffer_id`, optionally morphing
 * seam vertices along a shared `Spline2D` contour within `seam_blend_radius`.
 */

#include "../../../../geometry/csg_2d.h"
#include "../../../../geometry/polygon_math_2d.h"
#include "../../../../tiles/tile_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) TilingBoundaryClipAndSeamStitchTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Tiling Boundary Clip & Seam Stitch";
    static constexpr std::string_view ui_category  = "Tiles";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B
    uint32_t iso_cell_buffer_id           = INVALID_ID; // 4B: IsoCellPOD (32B AoS)
    uint32_t cell_vertex_csr_buffer_id    = INVALID_ID; // 4B: uint32_t CSR vertex indices
    uint32_t bridge_quad_buffer_id        = INVALID_ID; // 4B: BridgeQuadCellPOD (32B AoS)
    uint32_t polygon_vertex_2d_buffer_id  = INVALID_ID; // 4B: Bridged GeometryDOD glm::vec2
    uint32_t polyline_slice_buffer_id     = INVALID_ID; // 4B: Bridged GeometryDOD PolylineSlicePOD
    uint32_t polyline_vertex_2d_buffer_id = INVALID_ID; // 4B: Bridged GeometryDOD glm::vec2
    uint32_t seam_polyline_slot           = INVALID_ID; // 4B: Active seam polyline slot
    float seam_blend_radius               = 1.5f;       // 4B
    bool enable_curvilinear_seam_morph    = false;      // 1B
    uint8_t reserved_padding[27]          = {0};        // 27B -> Total = 64B

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
        add_buf("bridge_quad_buffer_id");
        add_buf("polygon_vertex_2d_buffer_id");
        add_buf("polyline_slice_buffer_id");
        add_buf("polyline_vertex_2d_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))             target_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("iso_cell_buffer_id"))           iso_cell_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["iso_cell_buffer_id"]));
        if (p_props.has("cell_vertex_csr_buffer_id"))    cell_vertex_csr_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["cell_vertex_csr_buffer_id"]));
        if (p_props.has("bridge_quad_buffer_id"))        bridge_quad_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["bridge_quad_buffer_id"]));
        if (p_props.has("polygon_vertex_2d_buffer_id"))  polygon_vertex_2d_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["polygon_vertex_2d_buffer_id"]));
        if (p_props.has("polyline_slice_buffer_id"))     polyline_slice_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_slice_buffer_id"]));
        if (p_props.has("polyline_vertex_2d_buffer_id")) polyline_vertex_2d_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["polyline_vertex_2d_buffer_id"]));
        if (p_props.has("seam_polyline_slot"))           seam_polyline_slot           = static_cast<uint32_t>(static_cast<int64_t>(p_props["seam_polyline_slot"]));
        if (p_props.has("seam_blend_radius"))            seam_blend_radius            = static_cast<float>(static_cast<double>(p_props["seam_blend_radius"]));
        if (p_props.has("enable_curvilinear_seam_morph"))enable_curvilinear_seam_morph= static_cast<bool>(p_props["enable_curvilinear_seam_morph"]);
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
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *cell_part = p_context.get_grant_part(iso_cell_buffer_id);
        const GrantPartPOD *csr_part  = p_context.get_grant_part(cell_vertex_csr_buffer_id);
        const GrantPartPOD *vtx_part  = p_context.get_grant_part(polygon_vertex_2d_buffer_id);
        if (!cell_part || !cell_part->raw_base_ptr ||
            !csr_part || !csr_part->raw_base_ptr ||
            !vtx_part || !vtx_part->raw_base_ptr) {
            return;
        }

        const int64_t max_cells = cell_part->selection.element_count;
        const int64_t max_csr   = csr_part->selection.capacity;
        const int64_t max_verts = vtx_part->selection.capacity;
        if (max_cells <= 0) return;

        auto *cells       = reinterpret_cast<IsoCellPOD *>(cell_part->raw_base_ptr);
        const auto *csr   = reinterpret_cast<const uint32_t *>(csr_part->raw_base_ptr);
        auto *verts       = reinterpret_cast<glm::vec2 *>(vtx_part->raw_base_ptr);

        const glm::vec2 *seam_pts = nullptr;
        uint32_t seam_pt_count    = 0u;
        if (enable_curvilinear_seam_morph && seam_polyline_slot != INVALID_ID) {
            const GrantPartPOD *sl_part = p_context.get_grant_part(polyline_slice_buffer_id);
            const GrantPartPOD *vt_part = p_context.get_grant_part(polyline_vertex_2d_buffer_id);
            if (sl_part && sl_part->raw_base_ptr && vt_part && vt_part->raw_base_ptr &&
                static_cast<int64_t>(seam_polyline_slot) < sl_part->selection.capacity) {
                const PolylineSlicePOD &sl = reinterpret_cast<const PolylineSlicePOD *>(sl_part->raw_base_ptr)[seam_polyline_slot];
                if (sl.vertex_count >= 2u &&
                    static_cast<int64_t>(sl.vertex_offset + sl.vertex_count) <= vt_part->selection.capacity) {
                    seam_pts      = reinterpret_cast<const glm::vec2 *>(vt_part->raw_base_ptr) + sl.vertex_offset;
                    seam_pt_count = sl.vertex_count;
                }
            }
        }

        for (int64_t c = 0; c < max_cells; ++c) {
            IsoCellPOD &cell = cells[c];
            if ((cell.flags & (CELL_FLAG_ON_BORDER | CELL_FLAG_CLIPPED_CAP | CELL_FLAG_BRIDGE_QUAD)) == 0u) {
                continue;
            }
            const uint32_t v_cnt = cell.vertex_count;
            if (v_cnt < 3u || cell.first_vertex_idx == INVALID_ID ||
                static_cast<int64_t>(cell.first_vertex_idx + v_cnt) > max_csr) {
                continue;
            }

            glm::vec2 centroid_accum(0.0f, 0.0f);
            uint32_t valid_v = 0u;
            for (uint32_t k = 0; k < v_cnt; ++k) {
                const uint32_t vid = csr[cell.first_vertex_idx + k];
                if (static_cast<int64_t>(vid) < max_verts) {
                    glm::vec2 p = verts[vid];
                    if (seam_pts && seam_pt_count >= 2u && seam_blend_radius > 1e-4f) {
                        float best_d2 = seam_blend_radius * seam_blend_radius;
                        glm::vec2 best_proj = p;
                        for (uint32_t s = 0; s + 1u < seam_pt_count; ++s) {
                            const glm::vec2 a  = seam_pts[s];
                            const glm::vec2 b  = seam_pts[s + 1u];
                            const glm::vec2 ab = b - a;
                            const float len2   = glm::dot(ab, ab);
                            const float t      = (len2 > 1e-8f) ? std::clamp(glm::dot(p - a, ab) / len2, 0.0f, 1.0f) : 0.0f;
                            const glm::vec2 q  = a + ab * t;
                            const glm::vec2 d  = p - q;
                            const float d2     = glm::dot(d, d);
                            if (d2 < best_d2) {
                                best_d2   = d2;
                                best_proj = q;
                            }
                        }
                        if (best_d2 < seam_blend_radius * seam_blend_radius) {
                            const float u = 1.0f - std::sqrt(best_d2) / seam_blend_radius;
                            const float w = u * u * (3.0f - 2.0f * u) * 0.15f;
                            p             = glm::mix(p, best_proj, w);
                            verts[vid]    = p;
                        }
                    }
                    centroid_accum += p;
                    ++valid_v;
                }
            }
            if (valid_v > 0u) {
                cell.center = centroid_accum / static_cast<float>(valid_v);
            }
        }
    }
};

static_assert(
    sizeof(TilingBoundaryClipAndSeamStitchTransformLogic<>) == 64,
    "TilingBoundaryClipAndSeamStitchTransformLogic must be 64 bytes!"
);

} // namespace ideam::core
