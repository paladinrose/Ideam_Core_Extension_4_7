#pragma once

/**
 * polygon_math_2d.h
 * Pure, stateless 2D polygon and polyline geometry operations.
 *
 * MIGRATION NOTE: Moved from src/core/splines/polygon_math_2d.h into src/core/geometry/
 * so both IdeamSplines, IdeamTiles, IdeamCymatics, and GeometryDOD can share triangulation,
 * point-in-polygon, intersection, convex hull, polygon offsetting, and parametric polygon
 * vertex generation without cross-plugin dependencies.
 */

#include "geometry_common.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * Corner join style for inward/outward 2D polygon offsetting (`PolygonMath2D::offset_polygon`).
 */
enum class JoinStyle2D : uint8_t {
    MITER = 0, // Sharp miter corner clamped to p_miter_limit
    BEVEL = 1, // Flat bevel chamfer across the corner
    ROUND = 2  // Circular arc fan connecting the two offset edge normals
};

template <FloatingPointScalar T = float>
struct PolygonMath2D {
    using Vec2 = glm::vec<2, T>;

    // ========================================================================
    // Containment & Area / Centroid / Winding Queries
    // ========================================================================

    /**
     * Ray-casting even-odd winding algorithm to check if a point is inside a polygon.
     */
    [[nodiscard]] static bool contains_point(std::span<const Vec2> p_polygon, const Vec2& p_point) noexcept;

    /**
     * Calculates the signed surface area of a polygon using the shoelace formula.
     * Positive for Counter-Clockwise (CCW) in standard math coordinates, negative for Clockwise (CW).
     * Retained as `area()` for 100% compatibility with existing Spline2D callers.
     */
    [[nodiscard]] static T area(std::span<const Vec2> p_polygon) noexcept;

    /**
     * Explicit alias for signed shoelace area.
     */
    [[nodiscard]] static inline T signed_area(std::span<const Vec2> p_polygon) noexcept {
        return area(p_polygon);
    }

    /**
     * Unsigned (absolute) surface area of the polygon.
     */
    [[nodiscard]] static inline T abs_area(std::span<const Vec2> p_polygon) noexcept {
        return std::abs(area(p_polygon));
    }

    /**
     * Computes the total arc-length perimeter of a 2D polygon or open polyline.
     */
    [[nodiscard]] static T perimeter(std::span<const Vec2> p_polygon, bool p_closed = true) noexcept;

    /**
     * Computes the area-weighted centroid (center of mass) of a 2D polygon.
     * Falls back to arithmetic vertex mean if the polygon is degenerate (zero area).
     */
    [[nodiscard]] static Vec2 centroid(std::span<const Vec2> p_polygon) noexcept;

    /**
     * Computes the 2D Axis-Aligned Bounding Box (AABB2D) of a point span.
     */
    [[nodiscard]] static AABB2D<T> compute_aabb(std::span<const Vec2> p_points) noexcept;

    /**
     * Determines the winding order of a polygon from its signed shoelace area.
     */
    [[nodiscard]] static WindingOrder get_winding_order(std::span<const Vec2> p_polygon) noexcept;

    /**
     * In-place winding enforcement (ensures polygon vertices are ordered CCW or CW).
     */
    static void ensure_winding(std::span<Vec2> r_polygon, WindingOrder p_desired = WindingOrder::COUNTER_CLOCKWISE) noexcept;

    // ========================================================================
    // Segment Intersections & Closest-Point / Boundary Distance Queries
    // ========================================================================

    /**
     * Calculates the intersection point between two closed line segments (A1-A2 and B1-B2).
     * @param r_intersect Written with the world-space intersection coordinate if found.
     * @param r_t_a Optional normalized parameter along segment A [0, 1].
     * @param r_t_b Optional normalized parameter along segment B [0, 1].
     * @return True if an intersection exists within both segment bounds.
     */
    [[nodiscard]] static bool line_intersection(
        const Vec2& p_a1,
        const Vec2& p_a2,
        const Vec2& p_b1,
        const Vec2& p_b2,
        Vec2& r_intersect,
        T* r_t_a = nullptr,
        T* r_t_b = nullptr
    ) noexcept;

    /**
     * Projects p_point onto line segment [p_a, p_b], clamping to segment endpoints.
     */
    [[nodiscard]] static Vec2 closest_point_on_segment(
        const Vec2& p_a,
        const Vec2& p_b,
        const Vec2& p_point,
        T* r_t = nullptr
    ) noexcept;

    /**
     * Finds the closest point on a polygon's perimeter (or open polyline) to p_point,
     * returning the unsigned Euclidean distance.
     */
    [[nodiscard]] static T distance_to_boundary(
        std::span<const Vec2> p_polygon,
        const Vec2& p_point,
        Vec2* r_closest_point = nullptr,
        size_t* r_edge_index = nullptr,
        bool p_closed = true
    ) noexcept;

    // ========================================================================
    // Triangulation, Convex Hull & Polygon Offsetting
    // ========================================================================

    /**
     * Checks if point p_p lies inside or on the boundary of 2D triangle (p_a, p_b, p_c).
     */
    [[nodiscard]] static bool is_point_in_triangle(
        const Vec2& p_a,
        const Vec2& p_b,
        const Vec2& p_c,
        const Vec2& p_p
    ) noexcept;

    /**
     * Ear-clipping triangulation. Appends triangle indices (3 per triangle) into r_out_indices.
     * Supports pre-reserved buffers to minimize heap allocations in DOD staging passes.
     */
    static bool triangulate_into(std::span<const Vec2> p_polygon, std::vector<uint32_t>& r_out_indices);

    /**
     * Ear-clipping triangulation returning a new flat vector of triangle vertex indices.
     */
    [[nodiscard]] static std::vector<uint32_t> triangulate(std::span<const Vec2> p_polygon);

    /**
     * Computes the 2D Counter-Clockwise Convex Hull of an arbitrary point cloud
     * using Andrew's O(N log N) Monotone Chain algorithm.
     */
    [[nodiscard]] static std::vector<Vec2> convex_hull(std::span<const Vec2> p_points);

    /**
     * Offsets a closed 2D polygon outward (p_delta > 0) or inward (p_delta < 0)
     * along vertex bisector normals using MITER, BEVEL, or ROUND corner joins.
     * Used for Wallpaper Tile grout/mortar margins, collision hull inflation, and SplinePolygon2D offsets.
     */
    [[nodiscard]] static std::vector<Vec2> offset_polygon(
        std::span<const Vec2> p_polygon,
        T p_delta,
        JoinStyle2D p_join = JoinStyle2D::MITER,
        T p_miter_limit = static_cast<T>(2.0)
    );

    // ========================================================================
    // Parametric 2D Polygon Vertex Generators (Shared by Spline2D & Tiles)
    // ========================================================================

    /**
     * Generates the 4 corner vertices of an axis-aligned rectangle centered at p_center.
     */
    [[nodiscard]] static std::vector<Vec2> generate_rectangle_vertices(const Vec2& p_center, const Vec2& p_size);

    /**
     * Generates the vertices of a regular N-gon with circumradius p_radius.
     */
    [[nodiscard]] static std::vector<Vec2> generate_regular_polygon_vertices(
        int p_sides = 3,
        T p_radius = static_cast<T>(0.5),
        T p_angle_offset = static_cast<T>(0.0),
        bool p_reverse_winding = true
    );

    /**
     * Generates the 2*N alternating outer/inner vertices of a regular star polygon.
     */
    [[nodiscard]] static std::vector<Vec2> generate_star_vertices(
        int p_sides = 3,
        T p_outer_radius = static_cast<T>(0.5),
        T p_inner_radius = static_cast<T>(0.25),
        T p_angle_offset = static_cast<T>(0.0)
    );

private:
    [[nodiscard]] static bool snip(
        int u,
        int v,
        int w,
        int n,
        const std::vector<int>& V,
        std::span<const Vec2> p_polygon
    ) noexcept;
};

} // namespace ideam::core