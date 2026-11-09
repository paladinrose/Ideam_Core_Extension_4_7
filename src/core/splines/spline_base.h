#pragma once

/**
 * spline_base.h
 * Dimension-agnostic (1D-4D) OOP & DOD-ready spline container.
 *
 * Includes fixes for:
 *   1. Full random-access point, handle, and section mutators for Gizmos, Controls, and IdeamUndoRedo.
 *   2. Const-correct lazy arc-length evaluation and prefix-sum cumulative weight/length tables.
 *   3. Direct O(1) per-section evaluation (`evaluate_section`, `evaluate_section_derivative`,
 *      `evaluate_section_second_derivative`) eliminating uniform index-division distortion.
 *   4. Analytic tangents (`get_derivative`, `get_tangent`) and DOD polynomial coefficient baking
 *      (`bake_segment_coefficients`, `bake_parametric_lut`).
 */

#include <glm/glm.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include "spline_math.h"

namespace ideam::core {

enum class PointSearchType : uint8_t {
    ANY = 0,
    BEFORE,
    AFTER
};

template <typename T>
struct SplineSection {
    CurveType curve_type{CurveType::LINEAR};
    T weight{static_cast<T>(1.0)};
    mutable T length{static_cast<T>(0.0)};
};

// AoS Layout for the OOP side.
// DOD systems use SoA / SplineSegmentCoeffsPOD via bake_segment_coefficients().
template <size_t N, typename T>
struct SplinePoint {
    glm::vec<N, T> point{static_cast<T>(0)};
    glm::vec<N, T> back_handle{static_cast<T>(0)};
    glm::vec<N, T> fore_handle{static_cast<T>(0)};
    bool use_back_handle{false};
    bool use_fore_handle{false};
};

template <size_t N, typename T = float>
class SplineBase {
public:
    using VecType       = glm::vec<N, T>;
    using PointType     = SplinePoint<N, T>;
    using SectionType   = SplineSection<T>;
    using CoeffsPOD     = SplineSegmentCoeffsPOD<N, T>;
    using LUTEntryPOD   = SplineParametricLUTEntryPOD<T>;

protected:
    std::vector<PointType> points;
    mutable std::vector<SectionType> sections;

    // Prefix-sum tables (size == sections.size()) for O(log M) binary search in get_section_at_percent
    std::vector<T> cumulative_weights;
    mutable std::vector<T> cumulative_lengths;

    VecType start_cap{static_cast<T>(0)};
    VecType end_cap{static_cast<T>(0)};

    bool is_wrap{false};
    mutable bool is_dirty{true};

    CurveType default_curve_type{CurveType::LINEAR};
    T default_weight{static_cast<T>(1.0)};

    T total_weight{static_cast<T>(0.0)};
    mutable T total_length{static_cast<T>(0.0)};

    void set_cap_points();
    void update_totals();
    void ensure_lengths_updated(int p_subdivisions_per_section = 16) const noexcept;

    /**
     * Resolves the 4 control vectors (a, b, c, d) for section `p_section`:
     *   - b = start anchor `points[p_section].point`
     *   - c = end anchor `points[p_section + 1].point`
     *   - a = preceding neighbor / start control handle
     *   - d = succeeding neighbor / end control handle
     */
    void resolve_section_control_points(
        size_t p_section,
        VecType& r_a,
        VecType& r_b,
        VecType& r_c,
        VecType& r_d
    ) const noexcept;

public:
    SplineBase() = default;
    virtual ~SplineBase() = default;

    // ========================================================================
    // State & Configuration Accessors
    // ========================================================================

    [[nodiscard]] inline size_t get_length() const noexcept {
        if (points.empty()) return 0;
        return (is_wrap && points.size() > 1) ? (points.size() - 1) : points.size();
    }

    [[nodiscard]] inline size_t get_point_count() const noexcept { return points.size(); }
    [[nodiscard]] inline size_t get_section_count() const noexcept { return sections.size(); }

    [[nodiscard]] inline bool get_wrap() const noexcept { return is_wrap; }
    void set_wrap(bool p_wrap);

    [[nodiscard]] inline CurveType get_default_curve_type() const noexcept { return default_curve_type; }
    inline void set_default_curve_type(CurveType p_type) noexcept {
        if (p_type != CurveType::NONE) default_curve_type = p_type;
    }

    [[nodiscard]] inline T get_default_weight() const noexcept { return default_weight; }
    inline void set_default_weight(T p_weight) noexcept {
        default_weight = std::max(static_cast<T>(0.0), p_weight);
    }

    [[nodiscard]] inline T get_total_weight() const noexcept { return total_weight; }

    // ========================================================================
    // Structural & Random-Access Mutators (For Gizmos, Controls & IdeamUndoRedo)
    // ========================================================================

    void add_point(const VecType& p_point, CurveType p_type = CurveType::NONE, T p_weight = static_cast<T>(-1.0));
    void insert_point(size_t p_index, const VecType& p_point, CurveType p_type = CurveType::NONE, T p_weight = static_cast<T>(-1.0));
    void remove_point(size_t p_index);
    void clear();

    void set_point(size_t p_index, const PointType& p_point);
    void set_point_position(size_t p_index, const VecType& p_position);
    [[nodiscard]] VecType get_point_position(size_t p_index) const noexcept;

    void set_point_back_handle(size_t p_index, const VecType& p_handle, bool p_enable = true);
    void set_point_fore_handle(size_t p_index, const VecType& p_handle, bool p_enable = true);
    void set_point_handles(
        size_t p_index,
        const VecType& p_back_handle,
        bool p_use_back,
        const VecType& p_fore_handle,
        bool p_use_fore
    );
    void set_use_back_handle(size_t p_index, bool p_enable);
    void set_use_fore_handle(size_t p_index, bool p_enable);

    void set_section_curve_type(size_t p_section_index, CurveType p_type);
    [[nodiscard]] CurveType get_section_curve_type(size_t p_section_index) const noexcept;

    void set_section_weight(size_t p_section_index, T p_weight);
    [[nodiscard]] T get_section_weight(size_t p_section_index) const noexcept;
    [[nodiscard]] T get_section_length(size_t p_section_index) const noexcept;

    // ========================================================================
    // Span Data Access
    // ========================================================================

    [[nodiscard]] std::span<const PointType> get_points() const noexcept { return points; }
    [[nodiscard]] std::span<const SectionType> get_sections() const noexcept { return sections; }

    // ========================================================================
    // Direct Per-Section & Global Parametric Evaluation
    // ========================================================================

    /**
     * Evaluates section `p_section` directly at local parameter `p_local_t` in [0, 1] in O(1) time.
     * Bypasses global weight/distance lookup so subdivision loops never suffer from unequal section scaling.
     */
    [[nodiscard]] VecType evaluate_section(size_t p_section, T p_local_t) const noexcept;

    /**
     * Evaluates the first derivative dP/dt_local of section `p_section` at `p_local_t` in [0, 1].
     */
    [[nodiscard]] VecType evaluate_section_derivative(size_t p_section, T p_local_t) const noexcept;

    /**
     * Evaluates the second derivative d^2P/dt_local^2 of section `p_section` at `p_local_t` in [0, 1].
     */
    [[nodiscard]] VecType evaluate_section_second_derivative(size_t p_section, T p_local_t) const noexcept;

    /**
     * Maps a (section_index, local_t) pair back into a normalized global percentage in [0, 1].
     */
    [[nodiscard]] T section_local_to_global_percent(size_t p_section, T p_local_t, bool p_weighted = true) const noexcept;

    /**
     * Resolves a normalized global percentage `p_percent` in [0, 1] to its owning section index
     * and local section parameter `r_local_percent` in [0, 1] via O(log M) binary search.
     */
    [[nodiscard]] size_t get_section_at_percent(T p_percent, bool p_weighted, T& r_local_percent) const noexcept;

    /**
     * Evaluates the spline position at global percentage `p_percent` (weighted or arc-length distance).
     */
    [[nodiscard]] VecType get_point(T p_percent, bool p_weighted = true) const;

    /**
     * Evaluates the raw first derivative vector at global percentage `p_percent`.
     */
    [[nodiscard]] VecType get_derivative(T p_percent, bool p_weighted = true) const;

    /**
     * Evaluates the unit tangent direction vector at global percentage `p_percent`.
     */
    [[nodiscard]] VecType get_tangent(T p_percent, bool p_weighted = true) const;

    /**
     * Subdivides each section directly over local_t in [0, 1] into a flattened polyline.
     * Linear/Step sections emit only their anchor endpoints; curved sections emit `p_max_subdivisions` steps.
     */
    [[nodiscard]] std::vector<VecType> get_smoothed_points(int p_max_subdivisions = 10) const;

    /**
     * Computes the total arc length of the spline (const-safe; caches section lengths when dirty).
     */
    [[nodiscard]] T get_total_spline_length(T p_sample_percent = static_cast<T>(0.05), bool p_force_recalculate = false) const;

    // ========================================================================
    // Shared Spatial Queries (AABB & Hierarchical Proximity Projection)
    // ========================================================================

    /**
     * Calculates the N-dimensional Axis-Aligned Bounding Box (AABB) of the spline.
     */
    void get_min_max(VecType& r_min, VecType& r_max, bool p_use_smoothed = false) const;

    /**
     * Hierarchical coarse-to-fine proximity solver that finds the closest point on the spline
     * to `p_point` by evaluating each section directly over `local_t` in [0, 1].
     */
    [[nodiscard]] VecType get_closest_point_on_spline(
        const VecType& p_point,
        int p_samples = 10,
        int* r_closest_anchor_before = nullptr,
        T* r_closest_percent = nullptr,
        bool p_weighted_percent = false
    ) const;

    // ========================================================================
    // DOD Baking Helpers (For SplineDOD & MemoryManagerDOD)
    // ========================================================================

    /**
     * Bakes all sections into a contiguous array of branchless cubic Horner polynomials.
     */
    [[nodiscard]] std::vector<CoeffsPOD> bake_segment_coefficients() const;

    /**
     * Bakes a fixed-stride lookup table of `p_lut_samples` bins mapping normalized u in [0, 1]
     * to (section_index, local_t_start, local_t_slope) for O(1) DOD evaluation.
     */
    [[nodiscard]] std::vector<LUTEntryPOD> bake_parametric_lut(
        size_t p_lut_samples = 64,
        bool p_weighted = false
    ) const;
};

} // namespace ideam::core