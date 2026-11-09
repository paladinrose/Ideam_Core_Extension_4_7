#pragma once

/**
 * csg_2d.h
 * 2D Polygon Clipping, Half-Plane Splitting, Boundary Cell Classification, Seam Welding, and Boolean CSG.
 *
 * Directly supports:
 *   1. TileDOD Wave 2 (WallpaperLatticeCullQuery): Classifying fundamental domain cells
 *      into FULLY_INSIDE (InteriorBitmask, >90% of tiles needing zero clipping),
 *      FULLY_OUTSIDE, or STRADDLING_BOUNDARY (BoundarySeamBitmask).
 *   2. TileDOD Wave 3 (WallpaperBoundaryClipTransform): Clipping boundary tiles against
 *      Spline2D segments or convex cells via Sutherland-Hodgman, and welding left/right
 *      tile polygons water-tightly along a shared Spline2D seam curve.
 *   3. Spline2D boolean operations (combine_polygons, subtract_polygons, intersect_polygons).
 */

#include "geometry_common.h"
#include "polygon_math_2d.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

enum class CSGOperation2D : uint8_t {
    UNION        = 0, // A U B
    INTERSECTION = 1, // A ^ B
    DIFFERENCE   = 2, // A - B
    XOR          = 3  // (A U B) - (A ^ B)
};

/**
 * Classification result for TileDOD Wave 2 (InteriorBitmask vs BoundarySeamBitmask).
 */
enum class ClipClassification2D : uint8_t {
    FULLY_OUTSIDE       = 0, // Discard tile instance
    FULLY_INSIDE        = 1, // Emit into InteriorBitmask (fast SIMD transform path, zero clipping)
    STRADDLING_BOUNDARY = 2  // Emit into BoundarySeamBitmask (clip against boundary in Wave 3)
};

/**
 * PolygonIntersectionEventPOD
 * Records an exact edge-edge intersection between Subject polygon A (edge i)
 * and Clip polygon B (edge j).
 */
template <FloatingPointScalar T = float>
struct PolygonIntersectionEventPOD {
    glm::vec<2, T> point{static_cast<T>(0)};
    uint32_t edge_a = 0u;
    uint32_t edge_b = 0u;
    T param_a       = static_cast<T>(0);
    T param_b       = static_cast<T>(0);
};

template <FloatingPointScalar T = float>
struct CSGMath2D {
    using Vec2 = glm::vec<2, T>;

    // ========================================================================
    // Wave 2 Cell / Polygon Boundary Classification
    // ========================================================================

    /**
     * Classifies a polygon against an oriented half-plane defined by
     * dot(p - p_plane_origin, p_inside_normal) >= 0.
     */
    [[nodiscard]] static ClipClassification2D classify_polygon_half_plane(
        std::span<const Vec2> p_polygon,
        const Vec2& p_plane_origin,
        const Vec2& p_inside_normal,
        T p_epsilon = GeometryConstants<T>::EPSILON
    ) noexcept;

    /**
     * Classifies a 2D cell or tile polygon against an arbitrary closed boundary polygon.
     * Used in TileDOD Wave 2 to partition tiles into InteriorBitmask vs BoundarySeamBitmask.
     */
    [[nodiscard]] static ClipClassification2D classify_polygon_against_boundary(
        std::span<const Vec2> p_tile_polygon,
        std::span<const Vec2> p_boundary_polygon,
        const AABB2D<T>& p_boundary_aabb
    ) noexcept;

    // ========================================================================
    // Sutherland-Hodgman Half-Plane & Convex Polygon Clipping
    // ========================================================================

    /**
     * Clips a 2D polygon against a single half-plane: dot(p - p_plane_origin, p_inside_normal) >= 0.
     * Essential for splitting Wallpaper Group tiles along a linear seam or bisector.
     */
    [[nodiscard]] static std::vector<Vec2> clip_polygon_half_plane(
        std::span<const Vec2> p_subject,
        const Vec2& p_plane_origin,
        const Vec2& p_inside_normal
    );

    /**
     * Clips an arbitrary (convex or concave) subject polygon against a convex clip polygon
     * using the Sutherland-Hodgman algorithm. Automatically detects the winding orientation
     * of p_convex_clip so inward normals always point toward the clip interior.
     */
    [[nodiscard]] static std::vector<Vec2> clip_polygon_convex(
        std::span<const Vec2> p_subject,
        std::span<const Vec2> p_convex_clip
    );

    /**
     * Clips a 2D line segment [p_a, p_b] against a convex polygon (Cyrus-Beck / Liang-Barsky).
     * Returns true if any portion of the segment remains inside p_convex_clip.
     */
    [[nodiscard]] static bool clip_segment_convex(
        const Vec2& p_a,
        const Vec2& p_b,
        std::span<const Vec2> p_convex_clip,
        Vec2& r_clipped_a,
        Vec2& r_clipped_b
    ) noexcept;

    // ========================================================================
    // Pairwise Polygon Intersection & Topological Seam Welding
    // ========================================================================

    /**
     * Finds all pairwise segment intersections between two closed polygons.
     */
    [[nodiscard]] static std::vector<PolygonIntersectionEventPOD<T>> find_polygon_intersections(
        std::span<const Vec2> p_poly_a,
        std::span<const Vec2> p_poly_b
    );

    /**
     * Topological Seam Welding Along a Spline2D Curve:
     * Clips Group A's boundary tiles (`r_left_tiles`) against the left normal of `p_seam_polyline`
     * and Group B's boundary tiles (`r_right_tiles`) against the right normal, snapping shared
     * vertices and inserting intermediate polyline bend points so the resulting mesh and
     * navigation graph are watertight with zero T-junction cracks.
     */
    static void weld_polygons_along_seam(
        std::span<std::vector<Vec2>> r_left_tiles,
        std::span<std::vector<Vec2>> r_right_tiles,
        std::span<const Vec2> p_seam_polyline,
        T p_weld_tolerance = GeometryConstants<T>::WELD_EPSILON
    );
};

} // namespace ideam::core