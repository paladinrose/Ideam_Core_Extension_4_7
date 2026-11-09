#include "spline_1d.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

template <typename T>
std::vector<T> Spline1D<T>::get_percents_at_value(T p_value, int p_samples, bool p_weighted) const {
    std::vector<T> percents;
    if (this->points.size() < 2 || this->sections.empty()) {
        return percents;
    }

    if (p_samples < 4) p_samples = 4;

    for (size_t sec_idx = 0; sec_idx < this->sections.size(); ++sec_idx) {
        const CurveType ctype = this->sections[sec_idx].curve_type;

        if (ctype == CurveType::STEP) {
            const T v0 = this->points[sec_idx].point.x;
            if (std::abs(v0 - p_value) <= GeometryConstants<T>::EPSILON) {
                percents.push_back(this->section_local_to_global_percent(sec_idx, static_cast<T>(0.0), p_weighted));
            }
            continue;
        }

        // Subdivide section over local_t in [0, 1] so cubic overshoot extrema are bracketed accurately
        const int coarse_steps = (ctype == CurveType::LINEAR) ? 1 : 8;
        const T inv_coarse = static_cast<T>(1.0) / static_cast<T>(coarse_steps);

        T t_prev = static_cast<T>(0.0);
        T y_prev = this->evaluate_section(sec_idx, t_prev).x;

        for (int k = 1; k <= coarse_steps; ++k) {
            const T t_curr = static_cast<T>(k) * inv_coarse;
            const T y_curr = this->evaluate_section(sec_idx, t_curr).x;

            const bool crosses = (p_value >= y_prev && p_value <= y_curr) ||
                                 (p_value <= y_prev && p_value >= y_curr);

            if (crosses) {
                if (std::abs(y_curr - y_prev) <= GeometryConstants<T>::SQ_EPSILON) {
                    percents.push_back(this->section_local_to_global_percent(sec_idx, t_prev, p_weighted));
                } else if (ctype == CurveType::LINEAR) {
                    const T local_hit = std::clamp((p_value - y_prev) / (y_curr - y_prev), static_cast<T>(0.0), static_cast<T>(1.0));
                    percents.push_back(this->section_local_to_global_percent(sec_idx, local_hit, p_weighted));
                } else {
                    // Bisection refinement on local_t in [t_prev, t_curr]
                    T lo_t = t_prev;
                    T hi_t = t_curr;
                    T lo_y = y_prev;
                    T hi_y = y_curr;
                    T mid_t = (lo_t + hi_t) * static_cast<T>(0.5);

                    for (int iter = 0; iter < p_samples; ++iter) {
                        mid_t = (lo_t + hi_t) * static_cast<T>(0.5);
                        const T mid_y = this->evaluate_section(sec_idx, mid_t).x;

                        if (std::abs(mid_y - p_value) <= GeometryConstants<T>::EPSILON) {
                            break;
                        }

                        if ((p_value >= lo_y && p_value <= mid_y) || (p_value <= lo_y && p_value >= mid_y)) {
                            hi_t = mid_t;
                            hi_y = mid_y;
                        } else {
                            lo_t = mid_t;
                            lo_y = mid_y;
                        }
                        (void)hi_y;
                    }
                    percents.push_back(this->section_local_to_global_percent(sec_idx, mid_t, p_weighted));
                }
            }

            t_prev = t_curr;
            y_prev = y_curr;
        }
    }

    std::sort(percents.begin(), percents.end());
    auto last = std::unique(percents.begin(), percents.end(), [](T l, T r) {
        return std::abs(l - r) < static_cast<T>(0.0001);
    });
    percents.erase(last, percents.end());

    return percents;
}

template <typename T>
T Spline1D<T>::get_min_value(bool p_use_smoothed) const noexcept {
    if (this->points.empty()) return static_cast<T>(0.0);

    VecType min_b, max_b;
    this->get_min_max(min_b, max_b, p_use_smoothed);
    return min_b.x;
}

template <typename T>
T Spline1D<T>::get_max_value(bool p_use_smoothed) const noexcept {
    if (this->points.empty()) return static_cast<T>(0.0);

    VecType min_b, max_b;
    this->get_min_max(min_b, max_b, p_use_smoothed);
    return max_b.x;
}

template <typename T>
Spline1D<T> Spline1D<T>::create_zero_to_one() {
    Spline1D<T> spline;
    spline.add_point(VecType(static_cast<T>(0.0)), CurveType::LINEAR);
    spline.add_point(VecType(static_cast<T>(1.0)), CurveType::LINEAR);
    return spline;
}

template <typename T>
Spline1D<T> Spline1D<T>::create_one_to_zero() {
    Spline1D<T> spline;
    spline.add_point(VecType(static_cast<T>(1.0)), CurveType::LINEAR);
    spline.add_point(VecType(static_cast<T>(0.0)), CurveType::LINEAR);
    return spline;
}

template <typename T>
Spline1D<T> Spline1D<T>::create_equidistant(size_t p_count, CurveType p_type) {
    Spline1D<T> spline;
    if (p_count < 2) p_count = 2;

    const T inv = static_cast<T>(1.0) / static_cast<T>(p_count - 1);
    for (size_t i = 0; i < p_count; ++i) {
        spline.add_point(VecType(inv * static_cast<T>(i)), p_type);
    }
    return spline;
}

// ============================================================================
// Explicit Template Instantiations
// ============================================================================

template class Spline1D<float>;
template class Spline1D<double>;

} // namespace ideam::core