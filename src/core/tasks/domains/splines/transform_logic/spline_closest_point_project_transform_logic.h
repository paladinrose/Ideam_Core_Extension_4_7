#pragma once

/**
 * spline_closest_point_project_transform_logic.h
 * SplineDOD Transform Task (S_T4): Hierarchical coarse-to-fine proximity projection solver
 * in DOD (`get_closest_point_on_spline`).
 *
 * Projects batched 2D/3D world positions (`SPLINE_QUERY_POINT_VEC4`, where `.w` holds the
 * target `spline_slot` as float or defaults to `default_spline_slot`) onto the cubic Horner
 * segments of the target spline, writing:
 *   - `SPLINE_EVAL_OUTPUT_VEC4`: Closest 4D point on the spline (preserving the spline's `.w` attribute!)
 *   - `SPLINE_CLOSEST_PERCENT_FLOAT32`: Normalized global parameter `u` in `[0, 1]`
 *   - `SPLINE_CLOSEST_DISTANCE_FLOAT32`: Unsigned Euclidean distance in 2D/3D space
 *   - `SPLINE_CLOSEST_ANCHOR_INT32`: Index of the anchor point (`section_index`) immediately preceding the closest point
 */

#include "../metadata_logic/spline_arc_length_lut_metadata_logic.h"
#include "../../../../splines/spline_math.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) SplineClosestPointProjectTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32 | DataType::VECTOR4;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline Closest-Point Projection";
    static constexpr std::string_view ui_category  = "Splines";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id            = INVALID_ID; // 4B
    uint32_t spline_header_buffer_id     = INVALID_ID; // 4B: SplineHeaderPOD
    uint32_t segment_coeffs_4d_buffer_id = INVALID_ID; // 4B: SplineSegmentCoeffsPOD<4, float>
    uint32_t query_point_vec4_buffer_id  = INVALID_ID; // 4B: glm::vec4 (xyz = query pos, w = optional spline_slot)
    uint32_t eval_output_vec4_buffer_id  = INVALID_ID; // 4B: glm::vec4 closest point on spline
    uint32_t closest_percent_buffer_id   = INVALID_ID; // 4B: float closest global percent [0, 1]
    uint32_t closest_distance_buffer_id  = INVALID_ID; // 4B: float Euclidean distance
    uint32_t closest_anchor_buffer_id    = INVALID_ID; // 4B: int32_t anchor index before closest point
    uint32_t default_spline_slot         = 0u;         // 4B
    uint8_t coarse_samples_per_section   = 8u;         // 1B
    uint8_t use_query_w_as_spline_slot   = 0u;         // 1B
    uint8_t reserved_padding[26]         = {0};        // 26B -> Total = 64B

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
        add_buf("query_point_vec4_buffer_id");
        add_buf("eval_output_vec4_buffer_id");
        add_buf("closest_percent_buffer_id");
        add_buf("closest_distance_buffer_id");
        add_buf("closest_anchor_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))            target_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("spline_header_buffer_id"))     spline_header_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["spline_header_buffer_id"]));
        if (p_props.has("segment_coeffs_4d_buffer_id")) segment_coeffs_4d_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["segment_coeffs_4d_buffer_id"]));
        if (p_props.has("query_point_vec4_buffer_id"))  query_point_vec4_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["query_point_vec4_buffer_id"]));
        if (p_props.has("eval_output_vec4_buffer_id"))  eval_output_vec4_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["eval_output_vec4_buffer_id"]));
        if (p_props.has("closest_percent_buffer_id"))   closest_percent_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["closest_percent_buffer_id"]));
        if (p_props.has("closest_distance_buffer_id"))  closest_distance_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["closest_distance_buffer_id"]));
        if (p_props.has("closest_anchor_buffer_id"))    closest_anchor_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["closest_anchor_buffer_id"]));
        if (p_props.has("default_spline_slot"))         default_spline_slot         = static_cast<uint32_t>(static_cast<int64_t>(p_props["default_spline_slot"]));
        if (p_props.has("coarse_samples_per_section"))  coarse_samples_per_section  = static_cast<uint8_t>(static_cast<int64_t>(p_props["coarse_samples_per_section"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : eval_output_vec4_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *hdr_part  = p_context.get_grant_part(spline_header_buffer_id);
        const GrantPartPOD *seg_part  = p_context.get_grant_part(segment_coeffs_4d_buffer_id);
        const GrantPartPOD *qpt_part  = p_context.get_grant_part(query_point_vec4_buffer_id);
        const GrantPartPOD *out_part  = p_context.get_grant_part(eval_output_vec4_buffer_id);
        const GrantPartPOD *pct_part  = p_context.get_grant_part(closest_percent_buffer_id);
        const GrantPartPOD *dist_part = p_context.get_grant_part(closest_distance_buffer_id);
        const GrantPartPOD *anc_part  = p_context.get_grant_part(closest_anchor_buffer_id);

        if (!hdr_part || !hdr_part->raw_base_ptr ||
            !seg_part || !seg_part->raw_base_ptr ||
            !qpt_part || !qpt_part->raw_base_ptr) {
            return;
        }

        const auto *headers = reinterpret_cast<const SplineHeaderPOD *>(hdr_part->raw_base_ptr);
        const auto *coeffs  = reinterpret_cast<const SplineSegmentCoeffsPOD<4, float> *>(seg_part->raw_base_ptr);
        const auto *qpts    = reinterpret_cast<const glm::vec4 *>(qpt_part->raw_base_ptr);
        auto *out_pos       = (out_part && out_part->raw_base_ptr) ? reinterpret_cast<glm::vec4 *>(out_part->raw_base_ptr) : nullptr;
        auto *out_pct       = (pct_part && pct_part->raw_base_ptr) ? reinterpret_cast<float *>(pct_part->raw_base_ptr) : nullptr;
        auto *out_dist      = (dist_part && dist_part->raw_base_ptr) ? reinterpret_cast<float *>(dist_part->raw_base_ptr) : nullptr;
        auto *out_anc       = (anc_part && anc_part->raw_base_ptr) ? reinterpret_cast<int32_t *>(anc_part->raw_base_ptr) : nullptr;

        const int64_t q_count = qpt_part->selection.element_count;
        const int64_t hdr_cap = hdr_part->selection.capacity;
        const int64_t seg_cap = seg_part->selection.capacity;
        const uint32_t steps  = std::max<uint32_t>(4u, coarse_samples_per_section);

        auto dist_sq_fn = [](const glm::vec4 &a, const glm::vec4 &b, uint8_t dim) noexcept -> float {
            const float dx = a.x - b.x;
            if (dim <= 1u) return dx * dx;
            const float dy = a.y - b.y;
            if (dim == 2u) return dx * dx + dy * dy;
            const float dz = a.z - b.z;
            return dx * dx + dy * dy + dz * dz;
        };

        for (int64_t i = 0; i < q_count; ++i) {
            const glm::vec4 q = qpts[i];
            const uint32_t sp_slot = (use_query_w_as_spline_slot != 0u && q.w >= 0.0f)
                ? static_cast<uint32_t>(q.w)
                : default_spline_slot;

            if (static_cast<int64_t>(sp_slot) >= hdr_cap) continue;
            const SplineHeaderPOD &hdr = headers[sp_slot];
            if (hdr.segment_count == 0u || static_cast<int64_t>(hdr.segment_offset + hdr.segment_count) > seg_cap) {
                continue;
            }

            float best_d2      = std::numeric_limits<float>::max();
            uint32_t best_sec  = 0u;
            float best_local_t = 0.0f;
            glm::vec4 best_pt(0.0f);

            // Stage 1: Coarse scan across each section
            for (uint32_t s = 0u; s < hdr.segment_count; ++s) {
                const auto &c = coeffs[hdr.segment_offset + s];
                for (uint32_t k = 0u; k <= steps; ++k) {
                    const float t      = static_cast<float>(k) / static_cast<float>(steps);
                    const glm::vec4 pt = c.evaluate(t);
                    const float d2     = dist_sq_fn(pt, q, hdr.dimension);
                    if (d2 < best_d2) {
                        best_d2      = d2;
                        best_sec     = s;
                        best_local_t = t;
                        best_pt      = pt;
                    }
                }
            }

            // Stage 2: Fine ternary/halving refinement inside best_sec
            const auto &best_c = coeffs[hdr.segment_offset + best_sec];
            float half_window  = 1.0f / static_cast<float>(steps);
            for (int iter = 0; iter < 5; ++iter) {
                half_window *= 0.5f;
                for (float dir : {-1.0f, 1.0f}) {
                    const float cand_t = std::clamp(best_local_t + dir * half_window, 0.0f, 1.0f);
                    const glm::vec4 pt = best_c.evaluate(cand_t);
                    const float d2     = dist_sq_fn(pt, q, hdr.dimension);
                    if (d2 < best_d2) {
                        best_d2      = d2;
                        best_local_t = cand_t;
                        best_pt      = pt;
                    }
                }
            }

            const float global_pct = (static_cast<float>(best_sec) + best_local_t) / static_cast<float>(hdr.segment_count);
            if (out_pos && i < out_part->selection.capacity)   out_pos[i]  = best_pt;
            if (out_pct && i < pct_part->selection.capacity)   out_pct[i]  = global_pct;
            if (out_dist && i < dist_part->selection.capacity) out_dist[i] = std::sqrt(best_d2);
            if (out_anc && i < anc_part->selection.capacity)   out_anc[i]  = static_cast<int32_t>(best_sec);
        }
    }
};

static_assert(sizeof(SplineClosestPointProjectTransformLogic<>) == 64, "SplineClosestPointProjectTransformLogic must be 64 bytes!");

} // namespace ideam::core
