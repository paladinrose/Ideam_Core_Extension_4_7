#pragma once

/**
 * sdf_math.h
 * 2D & 3D Signed Distance Field (SDF) primitives, smooth CSG boolean combinators,
 * periodic lattice domain folding, Chladni standing-wave nodal evaluation (for CymaticMap),
 * and Marching Squares iso-contour extraction.
 */

#include "geometry_common.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * MarchingSquaresSegment2D
 * Iso-contour line segment extracted from a 2D scalar grid by Marching Squares.
 */
template <FloatingPointScalar T = float>
struct MarchingSquaresSegment2D {
    glm::vec<2, T> start{static_cast<T>(0)};
    glm::vec<2, T> end{static_cast<T>(0)};
    uint32_t cell_x = 0u;
    uint32_t cell_y = 0u;
};

template <FloatingPointScalar T = float>
struct SDFMath {
    using Vec2 = glm::vec<2, T>;
    using Vec3 = glm::vec<3, T>;

    // ========================================================================
    // 2D Exact Signed Distance Primitives
    // ========================================================================

    [[nodiscard]] static inline T sd_circle(const Vec2& p_p, const Vec2& p_center, T p_radius) noexcept {
        return glm::distance(p_p, p_center) - p_radius;
    }

    [[nodiscard]] static inline T sd_box_2d(const Vec2& p_p, const Vec2& p_center, const Vec2& p_half_extents) noexcept {
        const Vec2 d = glm::abs(p_p - p_center) - p_half_extents;
        return glm::length(glm::max(d, Vec2(static_cast<T>(0.0)))) +
               std::min(std::max(d.x, d.y), static_cast<T>(0.0));
    }

    [[nodiscard]] static T sd_oriented_box_2d(
        const Vec2& p_p,
        const Vec2& p_a,
        const Vec2& p_b,
        T p_thickness
    ) noexcept;

    [[nodiscard]] static T sd_segment_2d(
        const Vec2& p_p,
        const Vec2& p_a,
        const Vec2& p_b,
        T p_radius = static_cast<T>(0.0)
    ) noexcept;

    /**
     * Exact signed distance from p_p to an arbitrary closed 2D polygon (negative inside, positive outside).
     */
    [[nodiscard]] static T sd_polygon_2d(std::span<const Vec2> p_polygon, const Vec2& p_p) noexcept;

    // ========================================================================
    // 3D Exact Signed Distance Primitives
    // ========================================================================

    [[nodiscard]] static inline T sd_sphere(const Vec3& p_p, const Vec3& p_center, T p_radius) noexcept {
        return glm::distance(p_p, p_center) - p_radius;
    }

    [[nodiscard]] static inline T sd_box_3d(const Vec3& p_p, const Vec3& p_center, const Vec3& p_half_extents) noexcept {
        const Vec3 q = glm::abs(p_p - p_center) - p_half_extents;
        return glm::length(glm::max(q, Vec3(static_cast<T>(0.0)))) +
               std::min(std::max(q.x, std::max(q.y, q.z)), static_cast<T>(0.0));
    }

    [[nodiscard]] static T sd_capsule_3d(
        const Vec3& p_p,
        const Vec3& p_a,
        const Vec3& p_b,
        T p_radius
    ) noexcept;

    [[nodiscard]] static T sd_torus_3d(
        const Vec3& p_p,
        T p_major_radius,
        T p_minor_radius
    ) noexcept;

    // ========================================================================
    // Sharp & Smooth CSG Boolean Combinators
    // ========================================================================

    [[nodiscard]] static constexpr T op_union(T p_d1, T p_d2) noexcept {
        return std::min(p_d1, p_d2);
    }

    [[nodiscard]] static constexpr T op_subtraction(T p_d1, T p_d2) noexcept {
        return std::max(p_d1, -p_d2); // Subtracts d2 from d1
    }

    [[nodiscard]] static constexpr T op_intersection(T p_d1, T p_d2) noexcept {
        return std::max(p_d1, p_d2);
    }

    /**
     * Quadratic polynomial smooth union (C1 continuous fillet of radius p_k).
     */
    [[nodiscard]] static constexpr T op_smooth_union(T p_d1, T p_d2, T p_k) noexcept {
        if (p_k <= GeometryConstants<T>::EPSILON) return std::min(p_d1, p_d2);
        const T h = std::clamp(static_cast<T>(0.5) + static_cast<T>(0.5) * (p_d2 - p_d1) / p_k, static_cast<T>(0.0), static_cast<T>(1.0));
        return glm::mix(p_d2, p_d1, h) - p_k * h * (static_cast<T>(1.0) - h);
    }

    [[nodiscard]] static constexpr T op_smooth_subtraction(T p_d1, T p_d2, T p_k) noexcept {
        if (p_k <= GeometryConstants<T>::EPSILON) return std::max(p_d1, -p_d2);
        const T h = std::clamp(static_cast<T>(0.5) - static_cast<T>(0.5) * (p_d1 + p_d2) / p_k, static_cast<T>(0.0), static_cast<T>(1.0));
        return glm::mix(p_d1, -p_d2, h) + p_k * h * (static_cast<T>(1.0) - h);
    }

    [[nodiscard]] static constexpr T op_smooth_intersection(T p_d1, T p_d2, T p_k) noexcept {
        if (p_k <= GeometryConstants<T>::EPSILON) return std::max(p_d1, p_d2);
        const T h = std::clamp(static_cast<T>(0.5) - static_cast<T>(0.5) * (p_d2 - p_d1) / p_k, static_cast<T>(0.0), static_cast<T>(1.0));
        return glm::mix(p_d2, p_d1, h) + p_k * h * (static_cast<T>(1.0) - h);
    }

    /**
     * Custom bevel/fillet union modulated by a 1D curve callback (e.g., Spline1D::get_point).
     */
    template <typename CurveFn>
    [[nodiscard]] static inline T op_spline_bevel_union(T p_d1, T p_d2, T p_k, CurveFn&& p_curve_fn) {
        if (p_k <= GeometryConstants<T>::EPSILON) return std::min(p_d1, p_d2);
        const T h = std::clamp(static_cast<T>(0.5) + static_cast<T>(0.5) * (p_d2 - p_d1) / p_k, static_cast<T>(0.0), static_cast<T>(1.0));
        const T profile = static_cast<T>(p_curve_fn(h));
        return glm::mix(p_d2, p_d1, h) - p_k * profile;
    }

    // ========================================================================
    // Periodic Domain Folding & Cymatic Standing-Wave Field Kernels
    // ========================================================================

    /**
     * Folds a 2D coordinate into a repeating rectangular cell of period p_period.
     */
    [[nodiscard]] static inline Vec2 op_repeat_2d(const Vec2& p_p, const Vec2& p_period) noexcept {
        return p_p - p_period * glm::round(p_p / p_period);
    }

    /**
     * Evaluates the 2D Chladni plate standing-wave modal equation:
     *   f(x, y) = a * sin(pi * n * x) * sin(pi * m * y) + b * sin(pi * m * x) * sin(pi * n * y)
     * Zero-crossings (f(x, y) == 0) define the exact cymatic nodal lines for CymaticMap!
     */
    [[nodiscard]] static T evaluate_chladni_wave_2d(
        const Vec2& p_normalized_uv,
        T p_mode_n,
        T p_mode_m,
        T p_amp_a = static_cast<T>(1.0),
        T p_amp_b = static_cast<T>(-1.0)
    ) noexcept;

    // ========================================================================
    // 2D Scalar Grid Sampling, Gradients & Marching Squares Iso-Contour Extraction
    // ========================================================================

    /**
     * Bilinearly samples a row-major 2D scalar field (width * height).
     */
    [[nodiscard]] static T sample_grid_bilinear_2d(
        std::span<const T> p_grid,
        uint32_t p_width,
        uint32_t p_height,
        const Vec2& p_grid_coord
    ) noexcept;

    /**
     * Computes the central-difference gradient vector (df/dx, df/dy) at integer cell (p_x, p_y).
     */
    [[nodiscard]] static Vec2 compute_grid_gradient_2d(
        std::span<const T> p_grid,
        uint32_t p_width,
        uint32_t p_height,
        uint32_t p_x,
        uint32_t p_y,
        const Vec2& p_cell_size = Vec2(static_cast<T>(1.0))
    ) noexcept;

    /**
     * Extracts 2D iso-contour segments at threshold p_iso_value from a row-major 2D scalar grid
     * using Marching Squares with linear edge root interpolation and saddle-point disambiguation.
     * Directly bridges CymaticMap / SDF grids into Spline2D / PolylineBuffer contours.
     */
    [[nodiscard]] static std::vector<MarchingSquaresSegment2D<T>> extract_marching_squares_contours(
        std::span<const T> p_grid,
        uint32_t p_width,
        uint32_t p_height,
        T p_iso_value = static_cast<T>(0.0),
        const Vec2& p_origin = Vec2(static_cast<T>(0.0)),
        const Vec2& p_cell_size = Vec2(static_cast<T>(1.0))
    );

    // ========================================================================
    // [SPECULATIVE / FUTURE PHASE] Dual Contouring, JFA & Eikonal Flow-Fields
    // ========================================================================
    /*
     * 1. Marching Squares Segment-to-Closed-Spline2D Contour Stitching:
     *    - Welds extracted MarchingSquaresSegment2D<T> Soup into ordered closed polylines
     *      and fits cubic Catmull-Rom / Bezier Spline2D curves to nodal lines:
     *        [[nodiscard]] static std::vector<std::vector<Vec2>> stitch_contour_polylines(
     *            std::span<const MarchingSquaresSegment2D<T>> p_segments,
     *            T p_weld_eps = GeometryConstants<T>::WELD_EPSILON);
     *
     * 2. 2D/3D Jump Flooding Algorithm (JFA) & Saito-Toriwaki Chamfer Distance Transform:
     *    - Converts a binary occupancy bitmask (e.g., SpatialProjectionBridgeQueryLogic output)
     *      into an exact Euclidean Distance Field in O(N log N) passes inside SwapView:
     *        static void jump_flood_sdf_step_2d(
     *            std::span<const glm::ivec2> p_read_seeds,
     *            std::span<glm::ivec2> r_write_seeds,
     *            uint32_t p_width, uint32_t p_height, int32_t p_step_stride) noexcept;
     *
     * 3. Eikonal Wavefront / Fast Sweeping Flow-Field Solver:
     *    - Solves |grad T(x)| * F(x) = 1 over a 2D grid or Wallpaper Group quotient graph
     *      for multi-agent navigation and acoustic wave arrival times:
     *        static void solve_eikonal_fast_sweeping_2d(
     *            std::span<const T> p_speed_field,
     *            std::span<T> r_arrival_time,
     *            uint32_t p_width, uint32_t p_height, T p_cell_size, uint32_t p_sweeps = 4) noexcept;
     *
     * 4. 3D Dual Contouring & Surface Nets (Hermite QEF Solver):
     *    - Extracts sharp-feature 3D meshes from 3D SDF grids using Quadric Error Metrics.
     */
};

} // namespace ideam::core