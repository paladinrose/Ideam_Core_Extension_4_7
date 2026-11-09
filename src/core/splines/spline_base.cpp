#include "spline_base.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ideam::core {

template <size_t N, typename T>
void SplineBase<N, T>::set_cap_points() {
    const size_t l = points.size();
    if (l > 1) {
        VecType point_diff = points[1].point - points[0].point;
        start_cap = points[0].point - point_diff;

        if (l >= 3) {
            if (glm::distance(points[0].point, points[l - 1].point) < GeometryConstants<T>::WELD_EPSILON) {
                start_cap = points[l - 2].point;
                end_cap   = points[1].point;
            } else {
                point_diff = points[l - 1].point - points[l - 2].point;
                end_cap    = points[l - 1].point + point_diff;
            }
        } else {
            end_cap = points[1].point + point_diff;
        }
    } else if (l == 1) {
        start_cap = points[0].point;
        end_cap   = points[0].point;
    } else {
        start_cap = VecType(static_cast<T>(0));
        end_cap   = VecType(static_cast<T>(0));
    }
}

template <size_t N, typename T>
void SplineBase<N, T>::update_totals() {
    total_weight = static_cast<T>(0.0);
    cumulative_weights.resize(sections.size());

    for (size_t i = 0; i < sections.size(); ++i) {
        total_weight += std::max(static_cast<T>(0.0), sections[i].weight);
        cumulative_weights[i] = total_weight;
    }

    // Mark arc-length cache as dirty so the next distance query recomputes section lengths lazily.
    is_dirty = true;
}

template <size_t N, typename T>
void SplineBase<N, T>::ensure_lengths_updated(int p_subdivisions_per_section) const noexcept {
    if (!is_dirty) return;

    if (p_subdivisions_per_section < 4) {
        p_subdivisions_per_section = 4;
    }

    total_length = static_cast<T>(0.0);
    cumulative_lengths.resize(sections.size());

    const T inv_steps = static_cast<T>(1.0) / static_cast<T>(p_subdivisions_per_section);

    for (size_t j = 0; j < sections.size(); ++j) {
        T sec_len = static_cast<T>(0.0);
        const CurveType ctype = sections[j].curve_type;

        if (ctype == CurveType::LINEAR) {
            sec_len = glm::distance(points[j].point, points[j + 1].point);
        } else if (ctype == CurveType::STEP) {
            // Step curve holds constant value until jump; chord distance provides fallback parameterization
            sec_len = glm::distance(points[j].point, points[j + 1].point);
        } else {
            VecType prev_pt = points[j].point;
            for (int s = 1; s <= p_subdivisions_per_section; ++s) {
                const T local_t = static_cast<T>(s) * inv_steps;
                const VecType curr_pt = evaluate_section(j, local_t);
                sec_len += glm::distance(prev_pt, curr_pt);
                prev_pt = curr_pt;
            }
        }

        sections[j].length    = sec_len;
        total_length         += sec_len;
        cumulative_lengths[j] = total_length;
    }

    is_dirty = false;
}

template <size_t N, typename T>
void SplineBase<N, T>::resolve_section_control_points(
    size_t p_section,
    VecType& r_a,
    VecType& r_b,
    VecType& r_c,
    VecType& r_d
) const noexcept {
    const size_t l = sections.size();
    if (l == 0 || p_section >= l) {
        const VecType fallback = points.empty() ? VecType(static_cast<T>(0)) : points[0].point;
        r_a = r_b = r_c = r_d = fallback;
        return;
    }

    r_b = points[p_section].point;
    r_c = points[p_section + 1].point;

    // 1. Resolve logical neighbor points for Catmull-Rom
    if (p_section == 0 && l > 1) {
        r_a = start_cap;
        r_d = points[2].point;
    } else if (p_section == 0) {
        r_a = start_cap;
        r_d = end_cap;
    } else if (p_section == l - 1) {
        r_a = points[l - 2].point;
        r_d = end_cap;
    } else {
        r_a = points[p_section - 1].point;
        r_d = points[p_section + 2].point;
    }

    const CurveType ctype = sections[p_section].curve_type;
    if (ctype == CurveType::CUBIC_BEZIER) {
        // For CUBIC_BEZIER, r_b is B0 (start), r_a is B1 (start tangent handle),
        // r_d is B2 (end tangent handle), and r_c is B3 (end).
        // Support both outgoing fore_handle / legacy back_handle on start point,
        // and incoming back_handle / legacy fore_handle on end point.
        if (points[p_section].use_fore_handle) {
            r_a = r_b + points[p_section].fore_handle;
        } else if (points[p_section].use_back_handle) {
            r_a = r_b + points[p_section].back_handle;
        } else {
            // Smooth Catmull-Rom-equivalent default Bezier handle: B0 + (c - a_neighbor) / 6
            r_a = r_b + (r_c - r_a) * static_cast<T>(1.0 / 6.0);
        }

        if (points[p_section + 1].use_back_handle) {
            r_d = r_c + points[p_section + 1].back_handle;
        } else if (points[p_section + 1].use_fore_handle) {
            r_d = r_c + points[p_section + 1].fore_handle;
        } else {
            // Smooth Catmull-Rom-equivalent default Bezier handle: B3 - (d_neighbor - b) / 6
            r_d = r_c - (r_d - r_b) * static_cast<T>(1.0 / 6.0);
        }
    } else {
        // For CATMULL_ROM, override virtual neighbor points if explicit handles are enabled
        if (points[p_section].use_back_handle) {
            r_a = r_b + points[p_section].back_handle;
        }
        if (points[p_section + 1].use_fore_handle) {
            r_d = r_c + points[p_section + 1].fore_handle;
        }
    }
}

template <size_t N, typename T>
void SplineBase<N, T>::set_wrap(bool p_wrap) {
    if (p_wrap != is_wrap) {
        if (p_wrap) {
            if (points.size() > 1 &&
                glm::distance(points.front().point, points.back().point) >= GeometryConstants<T>::WELD_EPSILON) {
                points.push_back(points.front());
            }
        } else {
            if (points.size() > 1 &&
                glm::distance(points.front().point, points.back().point) < GeometryConstants<T>::WELD_EPSILON) {
                points.pop_back();
            }
        }

        if (points.size() <= 1) {
            sections.clear();
        } else {
            sections.resize(points.size() - 1, SectionType{default_curve_type, default_weight, static_cast<T>(0.0)});
        }

        is_wrap = p_wrap;
        set_cap_points();
        update_totals();
    }
}

template <size_t N, typename T>
void SplineBase<N, T>::add_point(const VecType& p_point, CurveType p_type, T p_weight) {
    const CurveType final_type = (p_type == CurveType::NONE) ? default_curve_type : p_type;
    const T final_weight = (p_weight < static_cast<T>(0.0)) ? default_weight : p_weight;

    PointType new_point;
    new_point.point = p_point;

    if (is_wrap && points.size() > 1 &&
        glm::distance(points.front().point, points.back().point) < GeometryConstants<T>::WELD_EPSILON) {
        // Insert before the closing duplicate vertex so wrapped loop stays intact
        points.insert(points.end() - 1, new_point);
        sections.insert(sections.end() - 1, SectionType{final_type, final_weight, static_cast<T>(0.0)});
    } else {
        points.push_back(new_point);
        if (points.size() > 1) {
            sections.push_back(SectionType{final_type, final_weight, static_cast<T>(0.0)});
        }
    }

    set_cap_points();
    update_totals();
}

template <size_t N, typename T>
void SplineBase<N, T>::insert_point(size_t p_index, const VecType& p_point, CurveType p_type, T p_weight) {
    if (p_index >= points.size()) {
        add_point(p_point, p_type, p_weight);
        return;
    }

    const CurveType final_type = (p_type == CurveType::NONE) ? default_curve_type : p_type;
    const T final_weight = (p_weight < static_cast<T>(0.0)) ? default_weight : p_weight;

    PointType new_point;
    new_point.point = p_point;

    points.insert(points.begin() + static_cast<std::ptrdiff_t>(p_index), new_point);

    if (points.size() > 1) {
        const size_t sec_idx = std::min(p_index, sections.size());
        sections.insert(
            sections.begin() + static_cast<std::ptrdiff_t>(sec_idx),
            SectionType{final_type, final_weight, static_cast<T>(0.0)}
        );
    }

    if (is_wrap && p_index == 0 && points.size() > 2) {
        points.back().point = points.front().point;
    }

    set_cap_points();
    update_totals();
}

template <size_t N, typename T>
void SplineBase<N, T>::remove_point(size_t p_index) {
    if (p_index >= points.size()) return;

    points.erase(points.begin() + static_cast<std::ptrdiff_t>(p_index));

    if (p_index < sections.size()) {
        sections.erase(sections.begin() + static_cast<std::ptrdiff_t>(p_index));
    } else if (!sections.empty()) {
        sections.pop_back();
    }

    if (is_wrap && points.size() > 1) {
        points.back().point = points.front().point;
    }

    set_cap_points();
    update_totals();
}

template <size_t N, typename T>
void SplineBase<N, T>::clear() {
    points.clear();
    sections.clear();
    cumulative_weights.clear();
    cumulative_lengths.clear();
    total_weight = static_cast<T>(0.0);
    total_length = static_cast<T>(0.0);
    is_dirty     = false;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_point(size_t p_index, const PointType& p_point) {
    if (p_index >= points.size()) return;
    points[p_index] = p_point;

    if (is_wrap && points.size() > 1) {
        if (p_index == 0) {
            points.back().point = p_point.point;
        } else if (p_index == points.size() - 1) {
            points.front().point = p_point.point;
        }
    }

    set_cap_points();
    is_dirty = true;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_point_position(size_t p_index, const VecType& p_position) {
    if (p_index >= points.size()) return;
    points[p_index].point = p_position;

    if (is_wrap && points.size() > 1) {
        if (p_index == 0) {
            points.back().point = p_position;
        } else if (p_index == points.size() - 1) {
            points.front().point = p_position;
        }
    }

    set_cap_points();
    is_dirty = true;
}

template <size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::get_point_position(size_t p_index) const noexcept {
    if (p_index >= points.size()) return VecType(static_cast<T>(0));
    return points[p_index].point;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_point_back_handle(size_t p_index, const VecType& p_handle, bool p_enable) {
    if (p_index >= points.size()) return;
    points[p_index].back_handle     = p_handle;
    points[p_index].use_back_handle = p_enable;
    is_dirty = true;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_point_fore_handle(size_t p_index, const VecType& p_handle, bool p_enable) {
    if (p_index >= points.size()) return;
    points[p_index].fore_handle     = p_handle;
    points[p_index].use_fore_handle = p_enable;
    is_dirty = true;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_point_handles(
    size_t p_index,
    const VecType& p_back_handle,
    bool p_use_back,
    const VecType& p_fore_handle,
    bool p_use_fore
) {
    if (p_index >= points.size()) return;
    points[p_index].back_handle     = p_back_handle;
    points[p_index].use_back_handle = p_use_back;
    points[p_index].fore_handle     = p_fore_handle;
    points[p_index].use_fore_handle = p_use_fore;
    is_dirty = true;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_use_back_handle(size_t p_index, bool p_enable) {
    if (p_index >= points.size()) return;
    points[p_index].use_back_handle = p_enable;
    is_dirty = true;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_use_fore_handle(size_t p_index, bool p_enable) {
    if (p_index >= points.size()) return;
    points[p_index].use_fore_handle = p_enable;
    is_dirty = true;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_section_curve_type(size_t p_section_index, CurveType p_type) {
    if (p_section_index >= sections.size() || p_type == CurveType::NONE) return;
    sections[p_section_index].curve_type = p_type;
    is_dirty = true;
}

template <size_t N, typename T>
CurveType SplineBase<N, T>::get_section_curve_type(size_t p_section_index) const noexcept {
    if (p_section_index >= sections.size()) return default_curve_type;
    return sections[p_section_index].curve_type;
}

template <size_t N, typename T>
void SplineBase<N, T>::set_section_weight(size_t p_section_index, T p_weight) {
    if (p_section_index >= sections.size()) return;
    sections[p_section_index].weight = std::max(static_cast<T>(0.0), p_weight);

    // Recompute weight prefix sums without invalidating geometric arc lengths
    total_weight = static_cast<T>(0.0);
    cumulative_weights.resize(sections.size());
    for (size_t i = 0; i < sections.size(); ++i) {
        total_weight += sections[i].weight;
        cumulative_weights[i] = total_weight;
    }
}

template <size_t N, typename T>
T SplineBase<N, T>::get_section_weight(size_t p_section_index) const noexcept {
    if (p_section_index >= sections.size()) return static_cast<T>(0.0);
    return sections[p_section_index].weight;
}

template <size_t N, typename T>
T SplineBase<N, T>::get_section_length(size_t p_section_index) const noexcept {
    if (p_section_index >= sections.size()) return static_cast<T>(0.0);
    ensure_lengths_updated();
    return sections[p_section_index].length;
}

template <size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::evaluate_section(size_t p_section, T p_local_t) const noexcept {
    if (points.empty()) return VecType(static_cast<T>(0));
    if (points.size() == 1 || sections.empty()) return points[0].point;

    const size_t sec_idx = std::min(p_section, sections.size() - 1);
    const T local_t = std::clamp(p_local_t, static_cast<T>(0.0), static_cast<T>(1.0));
    const CurveType ctype = sections[sec_idx].curve_type;

    if (ctype == CurveType::LINEAR) {
        return glm::mix(points[sec_idx].point, points[sec_idx + 1].point, local_t);
    }
    if (ctype == CurveType::STEP) {
        return (local_t >= static_cast<T>(1.0)) ? points[sec_idx + 1].point : points[sec_idx].point;
    }

    VecType a, b, c, d;
    resolve_section_control_points(sec_idx, a, b, c, d);

    if (ctype == CurveType::CATMULL_ROM) {
        return SplineMath<N, T>::evaluate_catmull_rom(local_t, a, b, c, d);
    }
    if (ctype == CurveType::CUBIC_BEZIER) {
        return SplineMath<N, T>::evaluate_cubic_bezier(local_t, b, a, d, c);
    }
    return points[sec_idx].point;
}

template <size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::evaluate_section_derivative(size_t p_section, T p_local_t) const noexcept {
    if (points.size() < 2 || sections.empty()) return VecType(static_cast<T>(0));

    const size_t sec_idx = std::min(p_section, sections.size() - 1);
    const T local_t = std::clamp(p_local_t, static_cast<T>(0.0), static_cast<T>(1.0));
    const CurveType ctype = sections[sec_idx].curve_type;

    if (ctype == CurveType::LINEAR || ctype == CurveType::STEP) {
        return points[sec_idx + 1].point - points[sec_idx].point;
    }

    VecType a, b, c, d;
    resolve_section_control_points(sec_idx, a, b, c, d);

    if (ctype == CurveType::CATMULL_ROM) {
        return SplineMath<N, T>::evaluate_catmull_rom_derivative(local_t, a, b, c, d);
    }
    if (ctype == CurveType::CUBIC_BEZIER) {
        return SplineMath<N, T>::evaluate_cubic_bezier_derivative(local_t, b, a, d, c);
    }
    return points[sec_idx + 1].point - points[sec_idx].point;
}

template <size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::evaluate_section_second_derivative(size_t p_section, T p_local_t) const noexcept {
    if (points.size() < 2 || sections.empty()) return VecType(static_cast<T>(0));

    const size_t sec_idx = std::min(p_section, sections.size() - 1);
    const T local_t = std::clamp(p_local_t, static_cast<T>(0.0), static_cast<T>(1.0));
    const CurveType ctype = sections[sec_idx].curve_type;

    if (ctype == CurveType::LINEAR || ctype == CurveType::STEP) {
        return VecType(static_cast<T>(0));
    }

    VecType a, b, c, d;
    resolve_section_control_points(sec_idx, a, b, c, d);

    if (ctype == CurveType::CATMULL_ROM) {
        return SplineMath<N, T>::evaluate_catmull_rom_second_derivative(local_t, a, b, c, d);
    }
    if (ctype == CurveType::CUBIC_BEZIER) {
        return SplineMath<N, T>::evaluate_cubic_bezier_second_derivative(local_t, b, a, d, c);
    }
    return VecType(static_cast<T>(0));
}

template <size_t N, typename T>
T SplineBase<N, T>::section_local_to_global_percent(size_t p_section, T p_local_t, bool p_weighted) const noexcept {
    if (sections.empty()) return static_cast<T>(0.0);

    const size_t sec_idx = std::min(p_section, sections.size() - 1);
    const T local_t = std::clamp(p_local_t, static_cast<T>(0.0), static_cast<T>(1.0));

    if (p_weighted) {
        if (total_weight <= GeometryConstants<T>::EPSILON || cumulative_weights.size() != sections.size()) {
            return (static_cast<T>(sec_idx) + local_t) / static_cast<T>(sections.size());
        }
        const T prev_w = (sec_idx == 0) ? static_cast<T>(0.0) : cumulative_weights[sec_idx - 1];
        return (prev_w + sections[sec_idx].weight * local_t) / total_weight;
    } else {
        ensure_lengths_updated();
        if (total_length <= GeometryConstants<T>::EPSILON || cumulative_lengths.size() != sections.size()) {
            return (static_cast<T>(sec_idx) + local_t) / static_cast<T>(sections.size());
        }
        const T prev_len = (sec_idx == 0) ? static_cast<T>(0.0) : cumulative_lengths[sec_idx - 1];
        return (prev_len + sections[sec_idx].length * local_t) / total_length;
    }
}

template <size_t N, typename T>
size_t SplineBase<N, T>::get_section_at_percent(T p_percent, bool p_weighted, T& r_local_percent) const noexcept {
    if (sections.empty()) {
        r_local_percent = static_cast<T>(0.0);
        return 0;
    }

    T t = p_percent;
    if (is_wrap) {
        t = t - std::floor(t);
        // Preserve exact 1.0 endpoint when caller passes 1.0 on a wrapped spline
        if (p_percent == static_cast<T>(1.0) && t == static_cast<T>(0.0)) {
            r_local_percent = static_cast<T>(1.0);
            return sections.size() - 1;
        }
    } else {
        t = std::clamp(t, static_cast<T>(0.0), static_cast<T>(1.0));
    }

    if (t <= static_cast<T>(0.0)) {
        r_local_percent = static_cast<T>(0.0);
        return 0;
    }
    if (t >= static_cast<T>(1.0)) {
        r_local_percent = static_cast<T>(1.0);
        return sections.size() - 1;
    }

    if (p_weighted) {
        if (total_weight <= GeometryConstants<T>::EPSILON || cumulative_weights.size() != sections.size()) {
            const T scaled = t * static_cast<T>(sections.size());
            const size_t idx = std::min(static_cast<size_t>(std::floor(scaled)), sections.size() - 1);
            r_local_percent = scaled - static_cast<T>(idx);
            return idx;
        }

        const T target_w = t * total_weight;
        auto it = std::upper_bound(cumulative_weights.begin(), cumulative_weights.end(), target_w);
        const size_t idx = std::min(static_cast<size_t>(it - cumulative_weights.begin()), sections.size() - 1);
        const T prev_w   = (idx == 0) ? static_cast<T>(0.0) : cumulative_weights[idx - 1];
        const T span_w   = sections[idx].weight;
        r_local_percent  = (span_w > GeometryConstants<T>::EPSILON)
            ? std::clamp((target_w - prev_w) / span_w, static_cast<T>(0.0), static_cast<T>(1.0))
            : static_cast<T>(0.0);
        return idx;
    } else {
        ensure_lengths_updated();

        if (total_length <= GeometryConstants<T>::EPSILON || cumulative_lengths.size() != sections.size()) {
            const T scaled = t * static_cast<T>(sections.size());
            const size_t idx = std::min(static_cast<size_t>(std::floor(scaled)), sections.size() - 1);
            r_local_percent = scaled - static_cast<T>(idx);
            return idx;
        }

        const T target_d = t * total_length;
        auto it = std::upper_bound(cumulative_lengths.begin(), cumulative_lengths.end(), target_d);
        const size_t idx = std::min(static_cast<size_t>(it - cumulative_lengths.begin()), sections.size() - 1);
        const T prev_d   = (idx == 0) ? static_cast<T>(0.0) : cumulative_lengths[idx - 1];
        const T span_d   = sections[idx].length;
        r_local_percent  = (span_d > GeometryConstants<T>::EPSILON)
            ? std::clamp((target_d - prev_d) / span_d, static_cast<T>(0.0), static_cast<T>(1.0))
            : static_cast<T>(0.0);
        return idx;
    }
}

template <size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::get_point(T p_percent, bool p_weighted) const {
    if (points.empty()) return VecType(static_cast<T>(0));
    if (points.size() == 1) return points[0].point;

    T local_t = static_cast<T>(0.0);
    const size_t section_idx = get_section_at_percent(p_percent, p_weighted, local_t);
    return evaluate_section(section_idx, local_t);
}

template <size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::get_derivative(T p_percent, bool p_weighted) const {
    if (points.size() < 2) return VecType(static_cast<T>(0));

    T local_t = static_cast<T>(0.0);
    const size_t section_idx = get_section_at_percent(p_percent, p_weighted, local_t);
    return evaluate_section_derivative(section_idx, local_t);
}

template <size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::get_tangent(T p_percent, bool p_weighted) const {
    const VecType deriv = get_derivative(p_percent, p_weighted);
    const T len_sq = glm::dot(deriv, deriv);
    if (len_sq > GeometryConstants<T>::SQ_EPSILON) {
        return deriv * (static_cast<T>(1.0) / std::sqrt(len_sq));
    }
    return VecType(static_cast<T>(0));
}

template <size_t N, typename T>
std::vector<typename SplineBase<N, T>::VecType> SplineBase<N, T>::get_smoothed_points(int p_max_subdivisions) const {
    if (p_max_subdivisions < 3) p_max_subdivisions = 3;

    std::vector<VecType> smooth_points;
    if (points.empty()) return smooth_points;
    if (points.size() == 1) {
        smooth_points.push_back(points[0].point);
        return smooth_points;
    }

    smooth_points.reserve(sections.size() * static_cast<size_t>(p_max_subdivisions) + 1);
    const T inv_sub = static_cast<T>(1.0) / static_cast<T>(p_max_subdivisions);

    for (size_t i = 0; i < sections.size(); ++i) {
        smooth_points.push_back(points[i].point);

        const CurveType ctype = sections[i].curve_type;
        if (ctype == CurveType::CATMULL_ROM || ctype == CurveType::CUBIC_BEZIER) {
            for (int j = 1; j < p_max_subdivisions; ++j) {
                const T local_t = static_cast<T>(j) * inv_sub;
                smooth_points.push_back(evaluate_section(i, local_t));
            }
        }
    }

    smooth_points.push_back(points.back().point);
    return smooth_points;
}

template <size_t N, typename T>
T SplineBase<N, T>::get_total_spline_length(T p_sample_percent, bool p_force_recalculate) const {
    if (p_force_recalculate) {
        is_dirty = true;
    }
    const int subdivisions = (p_sample_percent > GeometryConstants<T>::EPSILON)
        ? std::max(4, static_cast<int>(std::round(static_cast<T>(1.0) / p_sample_percent)))
        : 16;
    ensure_lengths_updated(subdivisions);
    return total_length;
}

template <size_t N, typename T>
void SplineBase<N, T>::get_min_max(VecType& r_min, VecType& r_max, bool p_use_smoothed) const {
    if (points.empty()) {
        r_min = VecType(static_cast<T>(0.0));
        r_max = VecType(static_cast<T>(0.0));
        return;
    }

    if (p_use_smoothed) {
        const std::vector<VecType> pts = get_smoothed_points();
        r_min = pts[0];
        r_max = pts[0];
        for (const auto& p : pts) {
            r_min = glm::min(r_min, p);
            r_max = glm::max(r_max, p);
        }
    } else {
        r_min = points[0].point;
        r_max = points[0].point;
        for (const auto& pt : points) {
            r_min = glm::min(r_min, pt.point);
            r_max = glm::max(r_max, pt.point);
        }
    }
}

template <size_t N, typename T>
typename SplineBase<N, T>::VecType SplineBase<N, T>::get_closest_point_on_spline(
    const VecType& p_point,
    int p_samples,
    int* r_closest_anchor_before,
    T* r_closest_percent,
    bool p_weighted_percent
) const {
    if (points.empty()) {
        if (r_closest_anchor_before) *r_closest_anchor_before = 0;
        if (r_closest_percent) *r_closest_percent = static_cast<T>(0.0);
        return VecType(static_cast<T>(0.0));
    }
    if (points.size() == 1 || sections.empty()) {
        if (r_closest_anchor_before) *r_closest_anchor_before = 0;
        if (r_closest_percent) *r_closest_percent = static_cast<T>(0.0);
        return points[0].point;
    }

    if (p_samples < 4) p_samples = 4;

    // 1. Coarse search across all sections using 4 interior samples per section
    //    so highly curved or unequally weighted sections are never missed.
    constexpr int kCoarseSteps = 4;
    const T inv_coarse = static_cast<T>(1.0) / static_cast<T>(kCoarseSteps);

    size_t best_section = 0;
    T best_local_t      = static_cast<T>(0.0);
    VecType best_pt     = points[0].point;
    T best_dist_sq      = glm::dot(p_point - best_pt, p_point - best_pt);

    for (size_t s = 0; s < sections.size(); ++s) {
        for (int k = 0; k <= kCoarseSteps; ++k) {
            const T lt = static_cast<T>(k) * inv_coarse;
            const VecType cand = evaluate_section(s, lt);
            const VecType diff = p_point - cand;
            const T d_sq = glm::dot(diff, diff);
            if (d_sq < best_dist_sq) {
                best_dist_sq = d_sq;
                best_section = s;
                best_local_t = lt;
                best_pt      = cand;
            }
        }
    }

    // 2. Fine hierarchical refinement within [lo, hi] around best_local_t inside best_section
    T lo_t = std::max(static_cast<T>(0.0), best_local_t - inv_coarse);
    T hi_t = std::min(static_cast<T>(1.0), best_local_t + inv_coarse);

    VecType lo_pt = evaluate_section(best_section, lo_t);
    VecType hi_pt = evaluate_section(best_section, hi_t);
    T lo_d_sq     = glm::dot(p_point - lo_pt, p_point - lo_pt);
    T hi_d_sq     = glm::dot(p_point - hi_pt, p_point - hi_pt);

    for (int iter = 0; iter < p_samples; ++iter) {
        const T mid_t = (lo_t + hi_t) * static_cast<T>(0.5);
        const VecType mid_pt = evaluate_section(best_section, mid_t);
        const T mid_d_sq = glm::dot(p_point - mid_pt, p_point - mid_pt);

        if (mid_d_sq < best_dist_sq) {
            best_dist_sq = mid_d_sq;
            best_local_t = mid_t;
            best_pt      = mid_pt;
        }

        if (lo_d_sq < hi_d_sq) {
            hi_t    = mid_t;
            hi_pt   = mid_pt;
            hi_d_sq = mid_d_sq;
        } else {
            lo_t    = mid_t;
            lo_pt   = mid_pt;
            lo_d_sq = mid_d_sq;
        }
    }

    if (lo_d_sq < best_dist_sq) {
        best_dist_sq = lo_d_sq;
        best_local_t = lo_t;
        best_pt      = lo_pt;
    }
    if (hi_d_sq < best_dist_sq) {
        best_dist_sq = hi_d_sq;
        best_local_t = hi_t;
        best_pt      = hi_pt;
    }

    if (r_closest_anchor_before) {
        *r_closest_anchor_before = static_cast<int>(best_section);
    }
    if (r_closest_percent) {
        *r_closest_percent = section_local_to_global_percent(best_section, best_local_t, p_weighted_percent);
    }
    return best_pt;
}

template <size_t N, typename T>
std::vector<typename SplineBase<N, T>::CoeffsPOD> SplineBase<N, T>::bake_segment_coefficients() const {
    std::vector<CoeffsPOD> baked;
    baked.reserve(sections.size());

    for (size_t i = 0; i < sections.size(); ++i) {
        VecType a, b, c, d;
        resolve_section_control_points(i, a, b, c, d);
        baked.push_back(SplineMath<N, T>::bake_section_coefficients(sections[i].curve_type, a, b, c, d));
    }
    return baked;
}

template <size_t N, typename T>
std::vector<typename SplineBase<N, T>::LUTEntryPOD> SplineBase<N, T>::bake_parametric_lut(
    size_t p_lut_samples,
    bool p_weighted
) const {
    if (p_lut_samples < 4) p_lut_samples = 4;
    std::vector<LUTEntryPOD> lut(p_lut_samples);
    if (sections.empty()) return lut;

    const T inv_bins = static_cast<T>(1.0) / static_cast<T>(p_lut_samples);
    for (size_t b = 0; b < p_lut_samples; ++b) {
        const T u0 = static_cast<T>(b) * inv_bins;
        const T u1 = static_cast<T>(b + 1) * inv_bins;

        T lt0 = static_cast<T>(0.0);
        T lt1 = static_cast<T>(0.0);
        const size_t sec0 = get_section_at_percent(u0, p_weighted, lt0);
        const size_t sec1 = get_section_at_percent(u1, p_weighted, lt1);

        lut[b].section_index = static_cast<uint32_t>(sec0);
        lut[b].local_t_start = lt0;
        lut[b].local_t_slope = (sec1 == sec0) ? (lt1 - lt0) : (static_cast<T>(1.0) - lt0);
    }
    return lut;
}

// ============================================================================
// Explicit Template Instantiations
// ============================================================================

// 32-Bit Float Splines
template class SplineBase<1, float>;
template class SplineBase<2, float>;
template class SplineBase<3, float>;
template class SplineBase<4, float>;

// 64-Bit Double Splines
template class SplineBase<1, double>;
template class SplineBase<2, double>;
template class SplineBase<3, double>;
template class SplineBase<4, double>;

} // namespace ideam::core