#pragma once

/**
 * spline_2d.h
 * 2D curve and closed-polygon spline container.
 * Wired directly to Tier 1 `src/core/geometry/polygon_math_2d.h` and `csg_2d.h`.
 */

#include "spline_base.h"
#include "../geometry/polygon_math_2d.h"
#include "../geometry/csg_2d.h"
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

template <typename T = float>
class Spline2D : public SplineBase<2, T> {
public:
    using VecType = typename SplineBase<2, T>::VecType;

    Spline2D() = default;
    ~Spline2D() override = default;

    // --- 2D Polygon & Boundary Wrappers (Backed by PolygonMath2D<T> & CSGMath2D<T>) ---

    [[nodiscard]] bool contains_point(const VecType& p_point, int p_subdivisions = 10) const;
    [[nodiscard]] T get_area(int p_subdivisions = 10) const;
    [[nodiscard]] T get_abs_area(int p_subdivisions = 10) const;
    [[nodiscard]] T get_perimeter(int p_subdivisions = 10) const;
    [[nodiscard]] VecType get_centroid(int p_subdivisions = 10) const;
    [[nodiscard]] AABB2D<T> compute_aabb(int p_subdivisions = 10) const;
    [[nodiscard]] std::vector<uint32_t> triangulate(int p_subdivisions = 10) const;

    /**
     * Clips this closed 2D spline polygon against a convex clip polygon via Sutherland-Hodgman.
     */
    [[nodiscard]] std::vector<VecType> clip_with_convex_polygon(
        std::span<const VecType> p_convex_clip,
        int p_subdivisions = 10
    ) const;

    // --- 2D Spatial Transformations ---

    /**
     * Rotates all control points and handles around the 2D bounding-box center by p_angle_rad.
     */
    void rotate_spline(T p_angle_rad);

    /**
     * Scales all control points and handles relative to the 2D bounding-box center.
     */
    void scale_spline(const VecType& p_scale);

    // --- CSG Booleans (Stubs for General Non-Convex Martinez-Rueda Phase) ---
    //std::vector<Spline2D<T>> combine_polygons(const Spline2D<T>& p_other) const;
    //std::vector<Spline2D<T>> subtract_polygons(const Spline2D<T>& p_other) const;
    //std::vector<Spline2D<T>> intersect_polygons(const Spline2D<T>& p_other) const;

    // --- 2D Shape Generators (Backed by PolygonMath2D<T>::generate_*_vertices) ---

    [[nodiscard]] static Spline2D<T> from_vertices(
        std::span<const VecType> p_vertices,
        bool p_wrap = true,
        CurveType p_type = CurveType::LINEAR
    );

    [[nodiscard]] static Spline2D<T> create_rectangle(const VecType& p_center, const VecType& p_size);
    [[nodiscard]] static Spline2D<T> create_regular_polygon(
        int p_sides = 3,
        T p_radius = static_cast<T>(0.5),
        T p_angle_offset = static_cast<T>(0.0)
    );
    [[nodiscard]] static Spline2D<T> create_star(
        int p_sides = 3,
        T p_outer_radius = static_cast<T>(0.5),
        T p_inner_radius = static_cast<T>(0.25),
        T p_angle_offset = static_cast<T>(0.0)
    );
};

} // namespace ideam::core