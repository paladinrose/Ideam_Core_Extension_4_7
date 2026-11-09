#pragma once

/**
 * spline_math.h
 * Pure, stateless mathematical evaluation kernels, analytic 1st/2nd derivatives,
 * and pre-baked Horner polynomial coefficient PODs for 1D-4D splines.
 */

#include "../geometry/geometry_common.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ideam::core {

enum class CurveType : uint8_t {
    LINEAR = 0,
    CATMULL_ROM,
    CUBIC_BEZIER,
    STEP,
    NONE
};

/**
 * SplineSegmentCoeffsPOD<N, T>
 * Pre-baked cubic polynomial coefficients in Horner form for a single spline section on t in [0, 1]:
 *   P(t)   = ((c3 * t + c2) * t + c1) * t + c0
 *   P'(t)  = (3 * c3 * t + 2 * c2) * t + c1
 *   P''(t) = 6 * c3 * t + 2 * c2
 *
 * Unifies LINEAR, CATMULL_ROM, CUBIC_BEZIER, and STEP (plus handle overrides) into a single
 * branchless representation so SplineDOD tasks can evaluate mixed-type splines with 3 FMAs
 * and zero control-flow divergence.
 */
template <size_t N, FloatingPointScalar T = float>
struct alignas(16) SplineSegmentCoeffsPOD {
    using VecType = glm::vec<N, T>;

    VecType c3{static_cast<T>(0)};
    VecType c2{static_cast<T>(0)};
    VecType c1{static_cast<T>(0)};
    VecType c0{static_cast<T>(0)};

    [[nodiscard]] constexpr VecType evaluate(T p_local_t) const noexcept {
        return ((c3 * p_local_t + c2) * p_local_t + c1) * p_local_t + c0;
    }

    [[nodiscard]] constexpr VecType evaluate_derivative(T p_local_t) const noexcept {
        return (c3 * (static_cast<T>(3.0) * p_local_t) + c2 * static_cast<T>(2.0)) * p_local_t + c1;
    }

    [[nodiscard]] constexpr VecType evaluate_second_derivative(T p_local_t) const noexcept {
        return c3 * (static_cast<T>(6.0) * p_local_t) + c2 * static_cast<T>(2.0);
    }
};

/**
 * SplineParametricLUTEntryPOD<T>
 * Fixed-stride lookup table entry mapping a normalized global parameter bin u in [0, 1]
 * directly to (section_index, local_t_start + frac * local_t_slope) in O(1) time for SplineDOD.
 */
template <FloatingPointScalar T = float>
struct SplineParametricLUTEntryPOD {
    uint32_t section_index = 0u;
    T local_t_start        = static_cast<T>(0);
    T local_t_slope        = static_cast<T>(0);
};

static_assert(std::is_trivially_copyable_v<SplineSegmentCoeffsPOD<4, float>> &&
              std::is_standard_layout_v<SplineSegmentCoeffsPOD<4, float>>);
static_assert(sizeof(SplineSegmentCoeffsPOD<4, float>) == 64, "4D float SplineSegmentCoeffsPOD must be 64 bytes!");
static_assert(std::is_trivially_copyable_v<SplineParametricLUTEntryPOD<float>> &&
              std::is_standard_layout_v<SplineParametricLUTEntryPOD<float>>);

// Pure, stateless math operations for SIMD/DOD kernels and OOP evaluation
template <size_t N, FloatingPointScalar T = float>
struct SplineMath {
    using VecType   = glm::vec<N, T>;
    using CoeffsPOD = SplineSegmentCoeffsPOD<N, T>;

    // ========================================================================
    // Catmull-Rom Position, 1st Derivative (Tangent), & 2nd Derivative
    // ========================================================================

    [[nodiscard]] static constexpr VecType evaluate_catmull_rom(
        T t,
        const VecType& p0,
        const VecType& p1,
        const VecType& p2,
        const VecType& p3
    ) noexcept {
        const T t2 = t * t;
        const T t3 = t2 * t;

        return static_cast<T>(0.5) * (
            (-p0 + static_cast<T>(3.0) * p1 - static_cast<T>(3.0) * p2 + p3) * t3 +
            (static_cast<T>(2.0) * p0 - static_cast<T>(5.0) * p1 + static_cast<T>(4.0) * p2 - p3) * t2 +
            (-p0 + p2) * t +
            (static_cast<T>(2.0) * p1)
        );
    }

    [[nodiscard]] static constexpr VecType evaluate_catmull_rom_derivative(
        T t,
        const VecType& p0,
        const VecType& p1,
        const VecType& p2,
        const VecType& p3
    ) noexcept {
        const T t2 = t * t;

        return static_cast<T>(0.5) * (
            (-p0 + static_cast<T>(3.0) * p1 - static_cast<T>(3.0) * p2 + p3) * (static_cast<T>(3.0) * t2) +
            (static_cast<T>(2.0) * p0 - static_cast<T>(5.0) * p1 + static_cast<T>(4.0) * p2 - p3) * (static_cast<T>(2.0) * t) +
            (-p0 + p2)
        );
    }

    [[nodiscard]] static constexpr VecType evaluate_catmull_rom_second_derivative(
        T t,
        const VecType& p0,
        const VecType& p1,
        const VecType& p2,
        const VecType& p3
    ) noexcept {
        return (-p0 + static_cast<T>(3.0) * p1 - static_cast<T>(3.0) * p2 + p3) * (static_cast<T>(3.0) * t) +
               (static_cast<T>(2.0) * p0 - static_cast<T>(5.0) * p1 + static_cast<T>(4.0) * p2 - p3);
    }

    // ========================================================================
    // Cubic Bezier Position, 1st Derivative (Tangent), & 2nd Derivative
    // ========================================================================

    [[nodiscard]] static constexpr VecType evaluate_cubic_bezier(
        T t,
        const VecType& p0,
        const VecType& p1,
        const VecType& p2,
        const VecType& p3
    ) noexcept {
        const T u  = static_cast<T>(1.0) - t;
        const T u2 = u * u;
        const T u3 = u2 * u;
        const T t2 = t * t;
        const T t3 = t2 * t;

        return (u3 * p0) +
               (static_cast<T>(3.0) * u2 * t * p1) +
               (static_cast<T>(3.0) * u * t2 * p2) +
               (t3 * p3);
    }

    [[nodiscard]] static constexpr VecType evaluate_cubic_bezier_derivative(
        T t,
        const VecType& p0,
        const VecType& p1,
        const VecType& p2,
        const VecType& p3
    ) noexcept {
        const T u  = static_cast<T>(1.0) - t;
        const T u2 = u * u;
        const T t2 = t * t;

        return (static_cast<T>(3.0) * u2 * (p1 - p0)) +
               (static_cast<T>(6.0) * u * t * (p2 - p1)) +
               (static_cast<T>(3.0) * t2 * (p3 - p2));
    }

    [[nodiscard]] static constexpr VecType evaluate_cubic_bezier_second_derivative(
        T t,
        const VecType& p0,
        const VecType& p1,
        const VecType& p2,
        const VecType& p3
    ) noexcept {
        const T u = static_cast<T>(1.0) - t;
        return (static_cast<T>(6.0) * u * (p2 - static_cast<T>(2.0) * p1 + p0)) +
               (static_cast<T>(6.0) * t * (p3 - static_cast<T>(2.0) * p2 + p1));
    }

    // ========================================================================
    // Branchless Horner Coefficient Baking (For SplineDOD & Fast Evaluation)
    // ========================================================================

    /**
     * Converts a 4-point resolved section (a, b, c, d) of any CurveType into canonical
     * cubic polynomial coefficients P(t) = ((c3 * t + c2) * t + c1) * t + c0.
     * Note: `b` is the start anchor P(0) and `c` is the end anchor P(1).
     */
    [[nodiscard]] static constexpr CoeffsPOD bake_section_coefficients(
        CurveType p_type,
        const VecType& p_a,
        const VecType& p_b,
        const VecType& p_c,
        const VecType& p_d
    ) noexcept {
        CoeffsPOD coeffs{};
        switch (p_type) {
            case CurveType::CATMULL_ROM: {
                constexpr T half = static_cast<T>(0.5);
                coeffs.c3 = half * (-p_a + static_cast<T>(3.0) * p_b - static_cast<T>(3.0) * p_c + p_d);
                coeffs.c2 = half * (static_cast<T>(2.0) * p_a - static_cast<T>(5.0) * p_b + static_cast<T>(4.0) * p_c - p_d);
                coeffs.c1 = half * (-p_a + p_c);
                coeffs.c0 = p_b;
                break;
            }
            case CurveType::CUBIC_BEZIER: {
                // In SplineBase, p_b is start anchor B0, p_a is control handle B1,
                // p_d is control handle B2, and p_c is end anchor B3.
                const VecType& b0 = p_b;
                const VecType& b1 = p_a;
                const VecType& b2 = p_d;
                const VecType& b3 = p_c;
                coeffs.c3 = -b0 + static_cast<T>(3.0) * b1 - static_cast<T>(3.0) * b2 + b3;
                coeffs.c2 = static_cast<T>(3.0) * b0 - static_cast<T>(6.0) * b1 + static_cast<T>(3.0) * b2;
                coeffs.c1 = static_cast<T>(3.0) * (b1 - b0);
                coeffs.c0 = b0;
                break;
            }
            case CurveType::LINEAR: {
                coeffs.c3 = VecType(static_cast<T>(0));
                coeffs.c2 = VecType(static_cast<T>(0));
                coeffs.c1 = p_c - p_b;
                coeffs.c0 = p_b;
                break;
            }
            case CurveType::STEP:
            default: {
                coeffs.c3 = VecType(static_cast<T>(0));
                coeffs.c2 = VecType(static_cast<T>(0));
                coeffs.c1 = VecType(static_cast<T>(0));
                coeffs.c0 = p_b;
                break;
            }
        }
        return coeffs;
    }
};

} // namespace ideam::core