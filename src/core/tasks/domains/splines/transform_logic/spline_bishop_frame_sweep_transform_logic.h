#pragma once

/**
 * spline_bishop_frame_sweep_transform_logic.h
 * SplineDOD Transform Task (S_T3): Evaluates twist-free Wang Double-Reflection
 * Rotation-Minimizing Bishop Frames (`ParallelTransport3D<float>`) along 3D and 4D
 * splines (using the 4D `W` coordinate to modulate cross-section `scale` or axial
 * banking `roll_rad`) and writes the resulting `CurveFrame3DPOD<float>` sequence directly
 * into `GeometryDOD`'s bridged `BRIDGED_GEOM_CURVE_FRAME_3D_POD` buffer.
 */

#include "../metadata_logic/spline_arc_length_lut_metadata_logic.h"
#include "../../../../geometry/parallel_transport_3d.h"
#include "../../../../splines/spline_math.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) SplineBishopFrameSweepTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::VECTOR3 | DataType::VECTOR4;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline 3D/4D Bishop Frame Sweep";
    static constexpr std::string_view ui_category  = "Splines -> Geometry";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id            = INVALID_ID; // 4B
    uint32_t spline_header_buffer_id     = INVALID_ID; // 4B: SplineHeaderPOD (64B AoS)
    uint32_t segment_coeffs_4d_buffer_id = INVALID_ID; // 4B: SplineSegmentCoeffsPOD<4, float>
    uint32_t curve_frame_3d_buffer_id    = INVALID_ID; // 4B: Bridged GeometryDOD CurveFrame3DPOD<float>
    float up_hint_x                      = 0.0f;       // 4B
    float up_hint_y                      = 1.0f;       // 4B
    float up_hint_z                      = 0.0f;       // 4B
    uint8_t reserved_padding[36]         = {0};        // 36B -> Total = 64B

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
        add_buf("curve_frame_3d_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))            target_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("spline_header_buffer_id"))     spline_header_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["spline_header_buffer_id"]));
        if (p_props.has("segment_coeffs_4d_buffer_id")) segment_coeffs_4d_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["segment_coeffs_4d_buffer_id"]));
        if (p_props.has("curve_frame_3d_buffer_id"))    curve_frame_3d_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["curve_frame_3d_buffer_id"]));
        if (p_props.has("up_hint_x"))                   up_hint_x                   = static_cast<float>(static_cast<double>(p_props["up_hint_x"]));
        if (p_props.has("up_hint_y"))                   up_hint_y                   = static_cast<float>(static_cast<double>(p_props["up_hint_y"]));
        if (p_props.has("up_hint_z"))                   up_hint_z                   = static_cast<float>(static_cast<double>(p_props["up_hint_z"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : curve_frame_3d_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *hdr_part = p_context.get_grant_part(spline_header_buffer_id);
        const GrantPartPOD *seg_part = p_context.get_grant_part(segment_coeffs_4d_buffer_id);
        const GrantPartPOD *frm_part = p_context.get_grant_part(curve_frame_3d_buffer_id);

        if (!hdr_part || !hdr_part->raw_base_ptr ||
            !seg_part || !seg_part->raw_base_ptr ||
            !frm_part || !frm_part->raw_base_ptr) {
            return;
        }

        const auto *headers = reinterpret_cast<const SplineHeaderPOD *>(hdr_part->raw_base_ptr);
        const auto *coeffs  = reinterpret_cast<const SplineSegmentCoeffsPOD<4, float> *>(seg_part->raw_base_ptr);
        auto *frames        = reinterpret_cast<CurveFrame3DPOD<float> *>(frm_part->raw_base_ptr);

        const int64_t spline_count = hdr_part->selection.element_count;
        const int64_t seg_cap      = seg_part->selection.capacity;
        const int64_t frm_cap      = frm_part->selection.capacity;
        const glm::vec3 up_hint(up_hint_x, up_hint_y, up_hint_z);

        for (int64_t i = 0; i < spline_count; ++i) {
            const SplineHeaderPOD &hdr = headers[i];
            if (hdr.dimension < 3u || hdr.segment_count == 0u || hdr.geom_frame_count == 0u) {
                continue;
            }
            if (static_cast<int64_t>(hdr.segment_offset + hdr.segment_count) > seg_cap ||
                static_cast<int64_t>(hdr.geom_frame_offset + hdr.geom_frame_count) > frm_cap) {
                continue;
            }

            const uint32_t sample_cnt = hdr.geom_frame_count;
            const bool w_is_roll      = (hdr.dimension == 4u && hdr.w_channel_mode == 1u);
            const bool w_is_scale     = (hdr.dimension == 4u && hdr.w_channel_mode == 0u);

            CurveFrame3DPOD<float> prev_frame{};
            for (uint32_t s = 0u; s < sample_cnt; ++s) {
                const float u = (sample_cnt > 1u)
                    ? (static_cast<float>(s) / static_cast<float>(sample_cnt - 1u))
                    : 0.0f;
                const float scaled  = u * static_cast<float>(hdr.segment_count);
                const uint32_t sec  = std::min(static_cast<uint32_t>(scaled), hdr.segment_count - 1u);
                const float local_t = std::clamp(scaled - static_cast<float>(sec), 0.0f, 1.0f);

                const auto &c      = coeffs[hdr.segment_offset + sec];
                const glm::vec4 p4 = c.evaluate(local_t);
                const glm::vec4 d4 = c.evaluate_derivative(local_t);

                const glm::vec3 pos(p4.x, p4.y, p4.z);
                const glm::vec3 tan(d4.x, d4.y, d4.z);
                const float scale = w_is_scale ? std::max(1e-4f, p4.w) : 1.0f;
                const float roll  = w_is_roll ? p4.w : 0.0f;

                if (s == 0u) {
                    prev_frame = ParallelTransport3D<float>::make_initial_frame(pos, tan, up_hint, scale, roll);
                } else {
                    prev_frame = ParallelTransport3D<float>::step_double_reflection_frame(prev_frame, pos, tan, scale, roll);
                }
                frames[hdr.geom_frame_offset + s] = prev_frame;
            }
        }
    }
};

static_assert(sizeof(SplineBishopFrameSweepTransformLogic<>) == 64, "SplineBishopFrameSweepTransformLogic must be 64 bytes!");

} // namespace ideam::core
