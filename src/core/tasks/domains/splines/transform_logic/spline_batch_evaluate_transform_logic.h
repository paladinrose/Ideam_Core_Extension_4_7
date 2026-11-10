#pragma once

/**
 * spline_batch_evaluate_transform_logic.h
 * SplineDOD Transform Task (S_T1): Branchless SIMD-friendly batch evaluator for 1D-4D splines.
 *
 * Evaluates an array of `SplineSampleRequestPOD`s (`spline_slot`, `percent`, `is_weighted`,
 * `compute_tangent`) in O(1) time per sample by indexing `SPLINE_WEIGHT_LUT_POD` or
 * `SPLINE_DISTANCE_LUT_POD` and evaluating `SplineSegmentCoeffsPOD<4, float>` via Horner's rule:
 *   P(t)  = ((c3 * t + c2) * t + c1) * t + c0
 *   P'(t) = (3 * c3 * t + 2 * c2) * t + c1
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
#include <string_view>

namespace ideam::core {

#ifndef IDEAM_SPLINES_SAMPLE_REQUEST_POD_DEFINED
#define IDEAM_SPLINES_SAMPLE_REQUEST_POD_DEFINED

/**
 * SplineSampleRequestPOD (16 Bytes, Cache-Line Aligned)
 * Single batch evaluation request in `SPLINE_SAMPLE_REQUEST_POD`.
 */
struct alignas(16) SplineSampleRequestPOD {
    uint32_t spline_slot    = 0u;   // Target row in SPLINE_HEADER_POD
    float percent           = 0.0f; // Global parameter u in [0, 1]
    uint8_t is_weighted     = 1u;   // 1 = use weight LUT, 0 = use arc-length distance LUT
    uint8_t compute_tangent = 1u;   // 1 = normalize derivative into SPLINE_EVAL_TANGENT_VEC4
    uint16_t flags          = 0u;
    uint32_t user_tag       = 0u;   // Optional entity/agent slot ID
};

static_assert(sizeof(SplineSampleRequestPOD) == 16, "SplineSampleRequestPOD must be 16 bytes!");

#endif // IDEAM_SPLINES_SAMPLE_REQUEST_POD_DEFINED

template <typename T = int32_t>
struct alignas(64) SplineBatchEvaluateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::VECTOR4;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline Batch Horner Evaluate";
    static constexpr std::string_view ui_category  = "Splines";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B
    uint32_t spline_header_buffer_id      = INVALID_ID; // 4B: SplineHeaderPOD (64B AoS)
    uint32_t segment_coeffs_4d_buffer_id  = INVALID_ID; // 4B: SplineSegmentCoeffsPOD<4, float> (64B AoS)
    uint32_t weight_lut_buffer_id         = INVALID_ID; // 4B: SplineParametricLUTEntryPOD<float> (12B AoS)
    uint32_t distance_lut_buffer_id       = INVALID_ID; // 4B: SplineParametricLUTEntryPOD<float> (12B AoS)
    uint32_t sample_request_buffer_id     = INVALID_ID; // 4B: SplineSampleRequestPOD (16B AoS)
    uint32_t eval_output_vec4_buffer_id   = INVALID_ID; // 4B: glm::vec4 evaluated position/value
    uint32_t eval_tangent_vec4_buffer_id  = INVALID_ID; // 4B: glm::vec4 evaluated tangent/derivative
    uint8_t reserved_padding[32]          = {0};        // 32B -> Total = 64B

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
        add_buf("weight_lut_buffer_id");
        add_buf("distance_lut_buffer_id");
        add_buf("sample_request_buffer_id");
        add_buf("eval_output_vec4_buffer_id");
        add_buf("eval_tangent_vec4_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))            target_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("spline_header_buffer_id"))     spline_header_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["spline_header_buffer_id"]));
        if (p_props.has("segment_coeffs_4d_buffer_id")) segment_coeffs_4d_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["segment_coeffs_4d_buffer_id"]));
        if (p_props.has("weight_lut_buffer_id"))        weight_lut_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["weight_lut_buffer_id"]));
        if (p_props.has("distance_lut_buffer_id"))      distance_lut_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["distance_lut_buffer_id"]));
        if (p_props.has("sample_request_buffer_id"))    sample_request_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["sample_request_buffer_id"]));
        if (p_props.has("eval_output_vec4_buffer_id"))  eval_output_vec4_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["eval_output_vec4_buffer_id"]));
        if (p_props.has("eval_tangent_vec4_buffer_id")) eval_tangent_vec4_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["eval_tangent_vec4_buffer_id"]));
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
        const GrantPartPOD *wlut_part = p_context.get_grant_part(weight_lut_buffer_id);
        const GrantPartPOD *dlut_part = p_context.get_grant_part(distance_lut_buffer_id);
        const GrantPartPOD *req_part  = p_context.get_grant_part(sample_request_buffer_id);
        const GrantPartPOD *out_part  = p_context.get_grant_part(eval_output_vec4_buffer_id);
        const GrantPartPOD *tan_part  = p_context.get_grant_part(eval_tangent_vec4_buffer_id);

        if (!hdr_part || !hdr_part->raw_base_ptr ||
            !seg_part || !seg_part->raw_base_ptr ||
            !req_part || !req_part->raw_base_ptr ||
            !out_part || !out_part->raw_base_ptr) {
            return;
        }

        const auto *headers = reinterpret_cast<const SplineHeaderPOD *>(hdr_part->raw_base_ptr);
        const auto *coeffs  = reinterpret_cast<const SplineSegmentCoeffsPOD<4, float> *>(seg_part->raw_base_ptr);
        const auto *w_lut   = (wlut_part && wlut_part->raw_base_ptr)
            ? reinterpret_cast<const SplineParametricLUTEntryPOD<float> *>(wlut_part->raw_base_ptr)
            : nullptr;
        const auto *d_lut   = (dlut_part && dlut_part->raw_base_ptr)
            ? reinterpret_cast<const SplineParametricLUTEntryPOD<float> *>(dlut_part->raw_base_ptr)
            : nullptr;
        const auto *reqs    = reinterpret_cast<const SplineSampleRequestPOD *>(req_part->raw_base_ptr);
        auto *out_pos       = reinterpret_cast<glm::vec4 *>(out_part->raw_base_ptr);
        auto *out_tan       = (tan_part && tan_part->raw_base_ptr)
            ? reinterpret_cast<glm::vec4 *>(tan_part->raw_base_ptr)
            : nullptr;

        const int64_t req_count = std::min(req_part->selection.element_count, out_part->selection.capacity);
        const int64_t hdr_cap   = hdr_part->selection.capacity;
        const int64_t seg_cap   = seg_part->selection.capacity;

        for (int64_t i = 0; i < req_count; ++i) {
            const SplineSampleRequestPOD &req = reqs[i];
            if (static_cast<int64_t>(req.spline_slot) >= hdr_cap) {
                out_pos[i] = glm::vec4(0.0f);
                continue;
            }

            const SplineHeaderPOD &hdr = headers[req.spline_slot];
            if (hdr.segment_count == 0u) {
                out_pos[i] = glm::vec4(0.0f);
                continue;
            }

            float u = req.percent;
            if (hdr.is_wrap != 0u) {
                u = u - std::floor(u);
            } else {
                u = std::clamp(u, 0.0f, 1.0f);
            }

            uint32_t sec_idx = 0u;
            float local_t    = 0.0f;

            const auto *active_lut = (req.is_weighted != 0u) ? w_lut : d_lut;
            const uint32_t lut_off = (req.is_weighted != 0u) ? hdr.weight_lut_offset : hdr.distance_lut_offset;

            if (active_lut && hdr.lut_bin_count > 0u) {
                const float scaled  = u * static_cast<float>(hdr.lut_bin_count);
                const uint32_t bin  = std::min(static_cast<uint32_t>(scaled), hdr.lut_bin_count - 1u);
                const float frac    = std::clamp(scaled - static_cast<float>(bin), 0.0f, 1.0f);
                const auto &entry   = active_lut[lut_off + bin];
                sec_idx             = std::min(entry.section_index, hdr.segment_count - 1u);
                local_t             = std::clamp(entry.local_t_start + frac * entry.local_t_slope, 0.0f, 1.0f);
            } else {
                const float scaled = u * static_cast<float>(hdr.segment_count);
                sec_idx            = std::min(static_cast<uint32_t>(scaled), hdr.segment_count - 1u);
                local_t            = std::clamp(scaled - static_cast<float>(sec_idx), 0.0f, 1.0f);
            }

            const uint32_t global_seg = hdr.segment_offset + sec_idx;
            if (static_cast<int64_t>(global_seg) >= seg_cap) {
                out_pos[i] = glm::vec4(0.0f);
                continue;
            }

            const SplineSegmentCoeffsPOD<4, float> &seg = coeffs[global_seg];
            out_pos[i] = seg.evaluate(local_t);

            if (out_tan && i < tan_part->selection.capacity) {
                glm::vec4 deriv = seg.evaluate_derivative(local_t);
                if (req.compute_tangent != 0u && hdr.dimension >= 2u) {
                    const float len_sq = (hdr.dimension == 2u)
                        ? (deriv.x * deriv.x + deriv.y * deriv.y)
                        : (deriv.x * deriv.x + deriv.y * deriv.y + deriv.z * deriv.z);
                    if (len_sq > 1e-12f) {
                        const float inv_len = 1.0f / std::sqrt(len_sq);
                        deriv.x *= inv_len;
                        deriv.y *= inv_len;
                        if (hdr.dimension >= 3u) deriv.z *= inv_len;
                    }
                }
                out_tan[i] = deriv;
            }
        }
    }
};

static_assert(sizeof(SplineBatchEvaluateTransformLogic<>) == 64, "SplineBatchEvaluateTransformLogic must be 64 bytes!");

} // namespace ideam::core
