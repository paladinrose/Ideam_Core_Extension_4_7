#pragma once

/**
 * hyperbolic_tiling_2d.h
 * Non-Euclidean Hyperbolic `{p, q}` Poincaré Disk Tiling Generator & `SU(1, 1)` Möbius
 * Isometry Engine (`src/core/tiles/`).
 *
 * Implements Section 3.4 of the Ideam Core Extension Master Plan:
 *   1. Generalizes Euclidean Wallpaper symmetry groups to hyperbolic `{p, q}` regular tessellations
 *      satisfying `(p - 2) * (q - 2) > 4` inside the unit Poincaré disk (`|z| < 1`).
 *   2. Represents hyperbolic isometries via exact `SU(1, 1)` complex Möbius transformations
 *      (`MobiusSU11POD<T>`) and geodesic circle inversions, supporting interactive hyperbolic
 *      recentering (`apply_poincare_translation`) without floating-point drift near `|z| -> 1`.
 *   3. Computes exact orthogonal Euclidean circular-arc geodesics (`PoincareGeodesicArcPOD<T>`)
 *      and populates a standard `TilingRegion2D<T>` so `TilingNavigation2D<T>` (A* pathfinding
 *      and Dijkstra/Eikonal flow fields) and `TileDOD` operate seamlessly on hyperbolic boards.
 */

#include "tiling_region_2d.h"
#include "../geometry/geometry_common.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * MobiusSU11POD<T>
 * Represents an element of PSU(1, 1) (or anti-holomorphic reflection when `conj_z == true`)
 * acting on complex coordinates `z = x + i*y` in the unit Poincaré disk:
 *   z' = (a * z^* + b) / (conj(b) * z^* + conj(a)),  where |a|^2 - |b|^2 = 1.
 */
template <FloatingPointScalar T = float>
struct alignas(16) MobiusSU11POD {
    using Vec2 = glm::vec<2, T>;

    Vec2 a{static_cast<T>(1.0), static_cast<T>(0.0)}; // Complex real/imag
    Vec2 b{static_cast<T>(0.0), static_cast<T>(0.0)}; // Complex real/imag
    bool reflect_conjugate = false;                    // If true, conjugates input z before Mobius map
};

/**
 * PoincareGeodesicArcPOD<T>
 * Euclidean representation of the unique hyperbolic geodesic segment between two points
 * `start` and `end` inside the unit Poincaré disk.
 */
template <FloatingPointScalar T = float>
struct PoincareGeodesicArcPOD {
    using Vec2 = glm::vec<2, T>;

    Vec2 start{static_cast<T>(0.0)};
    Vec2 end{static_cast<T>(0.0)};
    Vec2 circle_center{static_cast<T>(0.0)};
    T circle_radius  = static_cast<T>(0.0);
    T start_angle    = static_cast<T>(0.0);
    T delta_angle    = static_cast<T>(0.0);
    bool is_diameter = true;
};

template <FloatingPointScalar T = float>
class HyperbolicTiling2D {
public:
    using Vec2   = glm::vec<2, T>;
    using Region = TilingRegion2D<T>;

private:
    uint32_t schlafli_p_       = 7u; // Polygon vertex/edge count p (e.g. 7 for heptagons)
    uint32_t schlafli_q_       = 3u; // Vertex valence q (e.g. 3 polygons meeting at each vertex)
    uint32_t generation_rings_ = 2u; // Breadth-first reflection rings (0..5)
    uint32_t arc_subdivisions_ = 8u; // Subdivisions per hyperbolic geodesic edge
    Vec2 mobius_focus_{static_cast<T>(0.0), static_cast<T>(0.0)};

    // Uncurved disk vertices (before geodesic arc subdivision) for each cell
    std::vector<std::vector<Vec2>> raw_cell_disk_vertices_;

public:
    HyperbolicTiling2D() = default;
    HyperbolicTiling2D(uint32_t p_p, uint32_t p_q, uint32_t p_rings = 2u) noexcept;
    ~HyperbolicTiling2D() = default;

    // ========================================================================
    // Hyperbolic {p, q} Geometry & SU(1, 1) Möbius Math Primitives
    // ========================================================================

    /**
     * Returns true iff `{p, q}` forms a valid hyperbolic tessellation: `(p - 2) * (q - 2) > 4`.
     */
    [[nodiscard]] static constexpr bool is_valid_hyperbolic_pq(uint32_t p_p, uint32_t p_q) noexcept {
        if (p_p < 3u || p_q < 3u) return false;
        return (p_p - 2u) * (p_q - 2u) > 4u;
    }

    /**
     * Computes the Euclidean distance `r_0 in (0, 1)` from the origin `(0, 0)` to the vertices
     * of the fundamental central `{p, q}` polygon in the Poincaré disk:
     *   r_0 = sqrt( cos(pi/p + pi/q) / cos(pi/p - pi/q) ).
     */
    [[nodiscard]] static T compute_fundamental_vertex_radius(uint32_t p_p, uint32_t p_q) noexcept;

    /**
     * Builds an SU(1, 1) hyperbolic translation mapping the origin `(0, 0)` to `p_target` (`|p_target| < 1`).
     */
    [[nodiscard]] static MobiusSU11POD<T> make_hyperbolic_translation(const Vec2& p_target) noexcept;

    /**
     * Builds an SU(1, 1) rotation around the origin by `p_angle_rad`.
     */
    [[nodiscard]] static MobiusSU11POD<T> make_hyperbolic_rotation(T p_angle_rad) noexcept;

    /**
     * Applies `p_mobius` to a point `p_z` inside the unit Poincaré disk.
     */
    [[nodiscard]] static Vec2 apply_mobius(const MobiusSU11POD<T>& p_mobius, const Vec2& p_z) noexcept;

    /**
     * Reflects point `p_z` across the unique hyperbolic geodesic passing through disk points `p_a` and `p_b`
     * (Euclidean circle inversion across the orthogonal geodesic circle, or line reflection if collinear with origin).
     */
    [[nodiscard]] static Vec2 reflect_across_geodesic(
        const Vec2& p_z,
        const Vec2& p_a,
        const Vec2& p_b
    ) noexcept;

    /**
     * Computes the exact Euclidean circular arc (orthogonal to the unit circle `|z| = 1`)
     * connecting `p_a` and `p_b` inside the Poincaré disk.
     */
    [[nodiscard]] static PoincareGeodesicArcPOD<T> compute_geodesic_arc(
        const Vec2& p_a,
        const Vec2& p_b
    ) noexcept;

    /**
     * Samples `p_subdivisions + 1` points along the hyperbolic geodesic arc from `p_a` to `p_b`.
     */
    [[nodiscard]] static std::vector<Vec2> sample_geodesic_segment(
        const Vec2& p_a,
        const Vec2& p_b,
        uint32_t p_subdivisions = 8u
    );

    /**
     * Hyperbolic metric distance `d_H(z1, z2) = acosh(1 + 2*|z1 - z2|^2 / ((1 - |z1|^2)*(1 - |z2|^2)))`.
     */
    [[nodiscard]] static T hyperbolic_distance(const Vec2& p_z1, const Vec2& p_z2) noexcept;

    // ========================================================================
    // Configuration & TilingRegion2D Generation
    // ========================================================================

    void set_schlafli_pq(uint32_t p_p, uint32_t p_q) noexcept;
    [[nodiscard]] constexpr uint32_t get_schlafli_p() const noexcept { return schlafli_p_; }
    [[nodiscard]] constexpr uint32_t get_schlafli_q() const noexcept { return schlafli_q_; }

    void set_generation_rings(uint32_t p_rings) noexcept {
        generation_rings_ = std::min(p_rings, 5u);
    }
    [[nodiscard]] constexpr uint32_t get_generation_rings() const noexcept { return generation_rings_; }

    void set_arc_subdivisions(uint32_t p_subdivs) noexcept {
        arc_subdivisions_ = std::clamp(p_subdivs, 1u, 32u);
    }
    [[nodiscard]] constexpr uint32_t get_arc_subdivisions() const noexcept { return arc_subdivisions_; }

    void set_mobius_focus(const Vec2& p_focus) noexcept;
    [[nodiscard]] constexpr Vec2 get_mobius_focus() const noexcept { return mobius_focus_; }

    [[nodiscard]] std::span<const std::vector<Vec2>> get_raw_cell_disk_vertices() const noexcept {
        return raw_cell_disk_vertices_;
    }

    /**
     * Generates the `{p, q}` hyperbolic tessellation up to `generation_rings_` around the origin,
     * applies the `mobius_focus_` SU(1, 1) hyperbolic translation, subdivides geodesic edges, and
     * builds the topological `TilingRegion2D<T>` in `r_out_region`.
     */
    void build_poincare_region(Region& r_out_region);
};

} // namespace ideam::core