#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../tasks/i_native_task.h"
#include "../../tasks/task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * SPL9: Spline1DInverseRootSolveTransformLogic
 * Batch solver for S(t) = y_target over pre-baked 1D cubic segments a*u^3 + b*u^2 + c*u + d = y,
 * combining analytic Cardano / quadratic root isolation on u in [0, 1] with safeguarded Newton-Raphson
 * polishing for reversible Utility AI and progression curves.
 */
struct Spline1DInverseRootSolveTransformLogic {
    static constexpr std::string_view ui_name = "Spline 1D Inverse Root Solver";
    static constexpr std::string_view ui_category = "Splines/1D";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::AOSOA,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::FLOAT32,
        DataType::VECTOR4
    };

    float fallback_target_y = 0.5f;
    int32_t root_selection_mode = 0; // 0 = First Root, 1 = Last Root, 2 = Closest to reference_t
    float reference_t = 0.5f;
    int32_t newton_iterations = 6;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("fallback_target_y", godot::Variant::FLOAT, fallback_target_y);
        add_prop("root_selection_mode", godot::Variant::INT, root_selection_mode, 2, "First Root in [0,1],Last Root in [0,1],Closest to Reference t");
        add_prop("reference_t", godot::Variant::FLOAT, reference_t);
        add_prop("newton_iterations", godot::Variant::INT, newton_iterations);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("fallback_target_y")) fallback_target_y = static_cast<float>(p_props["fallback_target_y"]);
        if (p_props.has("root_selection_mode")) root_selection_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["root_selection_mode"]), 0, 2);
        if (p_props.has("reference_t")) reference_t = std::clamp(static_cast<float>(p_props["reference_t"]), 0.0f, 1.0f);
        if (p_props.has("newton_iterations")) newton_iterations = std::clamp<int32_t>(static_cast<int32_t>(p_props["newton_iterations"]), 1, 16);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return 0;
    }

    size_t get_transient_requirement() const noexcept {
        return 0;
    }

    // Solves a*u^3 + b*u^2 + c*u + (d - y) = 0 for real roots u in [0, 1]
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
            // Depressed cubic: u = t - b/(3a), t^3 + p*t + q = 0
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

            // Safeguarded Newton-Raphson polishing
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

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& primary = p_ctx.grant->parts[0];
        if (!primary.raw_base_ptr) return;

        const GrantPartPOD* seg_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;
        GrantPartPOD* out_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : &primary;

        const uint32_t in_stride = std::max<uint32_t>(sizeof(float), primary.element_stride);
        const int64_t n_elems = (primary.selection.element_count > 0)
            ? primary.selection.element_count
            : static_cast<int64_t>(primary.capacity_bytes / in_stride);

        const uint32_t out_stride = std::max<uint32_t>(sizeof(float), out_part->element_stride);

        int32_t n_segs = 0;
        const godot::Vector4* seg_coeffs = nullptr;
        if (seg_part && seg_part->capacity_bytes >= sizeof(godot::Vector4)) {
            seg_coeffs = reinterpret_cast<const godot::Vector4*>(seg_part->raw_base_ptr);
            n_segs = (seg_part->selection.element_count > 0)
                ? static_cast<int32_t>(seg_part->selection.element_count)
                : static_cast<int32_t>(seg_part->capacity_bytes / sizeof(godot::Vector4));
        }

        for (int64_t i = 0; i < n_elems; ++i) {
            const float target_y = *reinterpret_cast<const float*>(primary.raw_base_ptr + static_cast<size_t>(i) * in_stride);
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
                // Default smoothstep S-curve inverse: 3t^2 - 2t^3 = y
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

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core