#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * Spline1DInverseRootSolveTransformLogic<T, T_Strategy, T_View> (SPL9)
 * Batch solver for S(t) = y_target over pre-baked 1D cubic segments a*u^3 + b*u^2 + c*u + d = y,
 * combining analytic Cardano / quadratic root isolation on u in [0, 1] with safeguarded Newton-Raphson
 * polishing for reversible Utility AI and progression curves.
 */
template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) Spline1DInverseRootSolveTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::VECTOR4;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline 1D Inverse Root Solver";
    static constexpr std::string_view ui_category  = "Splines/1D";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id         = INVALID_ID; // 4B
    uint32_t target_y_buffer_id       = INVALID_ID; // 4B
    uint32_t segment_coeffs_buffer_id = INVALID_ID; // 4B
    uint32_t output_t_buffer_id       = INVALID_ID; // 4B -> 16B
    float fallback_target_y           = 0.5f;       // 4B
    int32_t root_selection_mode       = 0;          // 4B: 0 = First Root, 1 = Last Root, 2 = Closest to reference_t
    float reference_t                 = 0.5f;       // 4B
    int32_t newton_iterations         = 6;          // 4B -> 32B
    uint8_t reserved_padding[32]      = {0};        // 32B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("target_y_buffer_id");
        add_buf("segment_coeffs_buffer_id");
        add_buf("output_t_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("fallback_target_y", godot::Variant::FLOAT, 0.5f);
        add_prop("root_selection_mode", godot::Variant::INT, 0, 2, "First Root in [0,1],Last Root in [0,1],Closest to Reference t");
        add_prop("reference_t", godot::Variant::FLOAT, 0.5f);
        add_prop("newton_iterations", godot::Variant::INT, 6);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (output_t_buffer_id != INVALID_ID) ? output_t_buffer_id : target_y_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("target_y_buffer_id")) target_y_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_y_buffer_id"]));
        if (p_props.has("segment_coeffs_buffer_id")) segment_coeffs_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["segment_coeffs_buffer_id"]));
        if (p_props.has("output_t_buffer_id")) output_t_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_t_buffer_id"]));
        if (p_props.has("fallback_target_y")) fallback_target_y = static_cast<float>(p_props["fallback_target_y"]);
        if (p_props.has("root_selection_mode")) root_selection_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["root_selection_mode"]), 0, 2);
        if (p_props.has("reference_t")) reference_t = std::clamp(static_cast<float>(p_props["reference_t"]), 0.0f, 1.0f);
        if (p_props.has("newton_iterations")) newton_iterations = std::clamp<int32_t>(static_cast<int32_t>(p_props["newton_iterations"]), 1, 16);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    int32_t solve_cubic_unit_interval(float a, float b, float c, float d_minus_y, float roots[3]) const noexcept {
        constexpr float EPS = 1e-6f;
        int32_t raw_count = 0;
        float cand[3] = {0.0f, 0.0f, 0.0f};

        if (std::abs(a) < 1e-7f) {
            if (std::abs(b) < 1e-7f) {
                if (std::abs(c) > 1e-7f) {
                    cand[raw_count++] = -d_minus_y / c;
                }
            } else {
                const float disc = c * c - 4.0f * b * d_minus_y;
                if (disc >= 0.0f) {
                    const float s = std::sqrt(disc);
                    cand[raw_count++] = (-c - s) / (2.0f * b);
                    cand[raw_count++] = (-c + s) / (2.0f * b);
                }
            }
        } else {
            const float inv_a = 1.0f / a;
            const float A = b * inv_a;
            const float B = c * inv_a;
            const float C = d_minus_y * inv_a;

            const float sq_A = A * A;
            const float p = (1.0f / 3.0f) * (-1.0f / 3.0f * sq_A + B);
            const float q = 0.5f * ((2.0f / 27.0f) * A * sq_A - (1.0f / 3.0f) * A * B + C);
            const float cb_p = p * p * p;
            const float D = q * q + cb_p;
            const float shift = (1.0f / 3.0f) * A;

            if (std::abs(D) < 1e-12f) {
                if (std::abs(q) < 1e-12f) {
                    cand[raw_count++] = -shift;
                } else {
                    const float u_root = std::cbrt(-q);
                    cand[raw_count++] = 2.0f * u_root - shift;
                    cand[raw_count++] = -u_root - shift;
                }
            } else if (D < 0.0f) {
                const float phi = (1.0f / 3.0f) * std::acos(std::clamp(-q / std::sqrt(-cb_p), -1.0f, 1.0f));
                const float t = 2.0f * std::sqrt(-p);
                constexpr float TWO_PI_3 = 2.0943951023931953f;
                cand[raw_count++] = t * std::cos(phi) - shift;
                cand[raw_count++] = -t * std::cos(phi + TWO_PI_3) - shift;
                cand[raw_count++] = -t * std::cos(phi - TWO_PI_3) - shift;
            } else {
                const float sqrt_D = std::sqrt(D);
                const float u_val = std::cbrt(sqrt_D - q);
                const float v_val = -std::cbrt(sqrt_D + q);
                cand[raw_count++] = (u_val + v_val) - shift;
            }
        }

        int32_t valid_count = 0;
        for (int32_t i = 0; i < raw_count; ++i) {
            float u = cand[i];
            if (u < -EPS || u > 1.0f + EPS) continue;
            u = std::clamp(u, 0.0f, 1.0f);

            for (int32_t iter = 0; iter < newton_iterations; ++iter) {
                const float f = ((a * u + b) * u + c) * u + d_minus_y;
                const float df = (3.0f * a * u + 2.0f * b) * u + c;
                if (std::abs(f) < 1e-7f || std::abs(df) < 1e-8f) break;
                u = std::clamp(u - f / df, 0.0f, 1.0f);
            }
            roots[valid_count++] = u;
        }

        if (valid_count > 1) {
            std::sort(roots, roots + valid_count);
        }
        return valid_count;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const noexcept {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            GrantPartPOD* primary = (target_y_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(target_y_buffer_id))
                : ((target_buffer_id != INVALID_ID)
                    ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(target_buffer_id))
                    : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr));
            if (!primary || !primary->raw_base_ptr) return;

            const GrantPartPOD* seg_part = (segment_coeffs_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(segment_coeffs_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr) ? &p_ctx.grant->parts[1] : nullptr);
            GrantPartPOD* out_part = (output_t_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(output_t_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : primary);
            if (!out_part || !out_part->raw_base_ptr) out_part = primary;

            const uint32_t in_stride = std::max<uint32_t>(sizeof(float), primary->element_stride);
            const int64_t n_elems = (primary->selection.element_count > 0)
                ? primary->selection.element_count
                : static_cast<int64_t>(primary->capacity_bytes / in_stride);

            const uint32_t out_stride = std::max<uint32_t>(sizeof(float), out_part->element_stride);

            int32_t n_segs = 0;
            const godot::Vector4* seg_coeffs = nullptr;
            if (seg_part && seg_part->raw_base_ptr && seg_part->capacity_bytes >= sizeof(godot::Vector4)) {
                seg_coeffs = reinterpret_cast<const godot::Vector4*>(seg_part->raw_base_ptr);
                n_segs = (seg_part->selection.element_count > 0)
                    ? static_cast<int32_t>(seg_part->selection.element_count)
                    : static_cast<int32_t>(seg_part->capacity_bytes / sizeof(godot::Vector4));
            }

            for (int64_t i = 0; i < n_elems; ++i) {
                const float target_y = *reinterpret_cast<const float*>(primary->raw_base_ptr + static_cast<size_t>(i) * in_stride);
                float resolved_t = -1.0f;
                float best_ref_dist = 1e30f;

                if (seg_coeffs && n_segs > 0) {
                    const float inv_segs = 1.0f / static_cast<float>(n_segs);
                    for (int32_t s = 0; s < n_segs; ++s) {
                        const godot::Vector4& c = seg_coeffs[s];
                        float local_roots[3];
                        const int32_t rc = solve_cubic_unit_interval(c.x, c.y, c.z, c.w - target_y, local_roots);
                        for (int32_t r = 0; r < rc; ++r) {
                            const float global_t = (static_cast<float>(s) + local_roots[r]) * inv_segs;
                            if (root_selection_mode == 0) {
                                if (resolved_t < 0.0f) resolved_t = global_t;
                            } else if (root_selection_mode == 1) {
                                resolved_t = global_t;
                            } else {
                                const float d = std::abs(global_t - reference_t);
                                if (d < best_ref_dist) {
                                    best_ref_dist = d;
                                    resolved_t = global_t;
                                }
                            }
                        }
                        if (root_selection_mode == 0 && resolved_t >= 0.0f) break;
                    }
                } else {
                    float local_roots[3];
                    const int32_t rc = solve_cubic_unit_interval(-2.0f, 3.0f, 0.0f, -target_y, local_roots);
                    if (rc > 0) {
                        resolved_t = local_roots[0];
                    }
                }

                if (resolved_t < 0.0f) {
                    resolved_t = std::clamp(target_y, 0.0f, 1.0f);
                }

                if (static_cast<size_t>(i + 1) * out_stride <= out_part->capacity_bytes) {
                    *reinterpret_cast<float*>(out_part->raw_base_ptr + static_cast<size_t>(i) * out_stride) = resolved_t;
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(Spline1DInverseRootSolveTransformLogic<>) == 64, "Spline1DInverseRootSolveTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<Spline1DInverseRootSolveTransformLogic<>>, "Spline1DInverseRootSolveTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core