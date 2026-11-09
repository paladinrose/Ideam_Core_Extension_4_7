#pragma once

#include "spline_base.h"
#include <vector>

namespace ideam::core {

/**
 * Spline1D
 * Represents a single-dimensional curve, often used for
 * animation curves, value easing, reversible Utility AI curves, or alpha tracks.
 */
template <typename T = float>
class Spline1D : public SplineBase<1, T> {
public:
    using VecType = typename SplineBase<1, T>::VecType;

    Spline1D() = default;
    ~Spline1D() override = default;

    // --- 1D Specific Queries ---

    /**
     * Solves for normalized global t (percent) given a target output value f(t) = p_value.
     * Subdivides each section directly over local_t in [0, 1] via evaluate_section() to
     * capture multi-modal roots and cubic overshoot, then refines each crossing via bisection
     * and maps (section_idx, local_t) to global percent via section_local_to_global_percent().
     * @param p_value The output value to search for.
     * @param p_samples The number of binary search iterations per root bracket.
     * @param p_weighted Whether returned percentages are in weighted space (true) or distance space (false).
     * @return A list of normalized percentages (t) where the curve crosses p_value.
     */
    [[nodiscard]] std::vector<T> get_percents_at_value(
        T p_value,
        int p_samples = 10,
        bool p_weighted = true
    ) const;

    /**
     * @param p_use_smoothed If true, evaluates smoothed curve samples to capture cubic overshoot extrema.
     * @return The minimum value along the spline.
     */
    [[nodiscard]] T get_min_value(bool p_use_smoothed = false) const noexcept;

    /**
     * @param p_use_smoothed If true, evaluates smoothed curve samples to capture cubic overshoot extrema.
     * @return The maximum value along the spline.
     */
    [[nodiscard]] T get_max_value(bool p_use_smoothed = false) const noexcept;

    // --- 1D Spline Builders ---

    /**
     * Creates a standard normalized linear spline rising from 0 to 1.
     */
    [[nodiscard]] static Spline1D<T> create_zero_to_one();

    /**
     * Creates a standard normalized linear spline falling from 1 to 0.
     */
    [[nodiscard]] static Spline1D<T> create_one_to_zero();

    /**
     * Creates a spline with `p_count` points distributed linearly from 0 to 1.
     */
    [[nodiscard]] static Spline1D<T> create_equidistant(size_t p_count, CurveType p_type = CurveType::LINEAR);
};

} // namespace ideam::core