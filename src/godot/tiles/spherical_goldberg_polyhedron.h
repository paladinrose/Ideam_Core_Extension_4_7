#pragma once

/**
 * spherical_goldberg_polyhedron.h
 * Spherical Goldberg Polyhedron `GP(f, 0)` (Hexagon / 12-Pentagon Geodesic Planet Grid)
 * Generator & Topological Graph Builder (`src/core/tiles/`).
 *
 * Implements Section 3.4 of the Ideam Core Extension Master Plan:
 *   1. Subdivides a canonical regular icosahedron by frequency `f >= 1` (`10*f^2 + 2` dual cells),
 *      projects vertices onto the unit sphere, and dualizes the geodesic triangulation into
 *      12 regular pentagons and `10*(f^2 - 1)` hexagons.
 *   2. Stores each spherical tile in a cache-friendly `GoldbergCell3DPOD<T>` (3D unit normal,
 *      orthonormal tangent basis, 5 or 6 spherical boundary vertices, 5 or 6 neighbor cell IDs,
 *      3-coloring, and radial elevation offset).
 *   3. Simultaneously populates a `TilingRegion2D<T>` topological graph so `TilingNavigation2D<T>`
 *      (A* pathfinding and multi-goal Eikonal/Dijkstra flow fields) runs seamlessly across the
 *      entire spherical planet with zero polar singularities or seam discontinuities.
 */

#include "tiling_region_2d.h"
#include "../geometry/geometry_common.h"
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

template <FloatingPointScalar T = float>
struct GoldbergCell3DPOD {
    using Vec3 = glm::vec<3, T>;

    uint32_t cell_id          = 0u;
    uint8_t valence           = 6u; // 5 for the 12 icosahedral pentagons, 6 for all hexagons
    uint8_t colour_id         = 0u; // 0, 1, or 2
    uint16_t generation_ring  = 0u; // Topological distance from North Pole pentagon (cell 0)
    T elevation               = static_cast<T>(0.0);

    Vec3 unit_normal{static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0)};
    Vec3 tangent_u{static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0)};
    Vec3 tangent_v{static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0)};

    std::array<Vec3, 6> unit_vertices{};
    std::array<int32_t, 6> neighbor_cell_ids{{-1, -1, -1, -1, -1, -1}};
};

template <FloatingPointScalar T = float>
class SphericalGoldbergPolyhedron {
public:
    using Vec2   = glm::vec<2, T>;
    using Vec3   = glm::vec<3, T>;
    using Cell3D = GoldbergCell3DPOD<T>;
    using Region = TilingRegion2D<T>;

private:
    uint32_t frequency_ = 3u; // Subdivision frequency f >= 1 -> 10*f^2 + 2 cells
    T radius_           = static_cast<T>(5.0);
    std::vector<Cell3D> cells_3d_;

public:
    SphericalGoldbergPolyhedron() = default;
    explicit SphericalGoldbergPolyhedron(uint32_t p_frequency, T p_radius = static_cast<T>(5.0)) noexcept;
    ~SphericalGoldbergPolyhedron() = default;

    void set_frequency(uint32_t p_frequency) noexcept {
        frequency_ = std::clamp(p_frequency, 1u, 24u);
    }
    [[nodiscard]] constexpr uint32_t get_frequency() const noexcept { return frequency_; }

    void set_radius(T p_radius) noexcept {
        radius_ = std::max(static_cast<T>(0.1), p_radius);
    }
    [[nodiscard]] constexpr T get_radius() const noexcept { return radius_; }

    [[nodiscard]] size_t get_cell_count() const noexcept { return cells_3d_.size(); }
    [[nodiscard]] std::span<const Cell3D> get_cells_3d() const noexcept { return cells_3d_; }
    [[nodiscard]] std::span<Cell3D> get_cells_3d_mut() noexcept { return cells_3d_; }

    /**
     * Builds the Goldberg polyhedron `GP(frequency_, 0)` (`10 * f^2 + 2` cells) and populates
     * `r_out_topology_region` with the exact spherical cell adjacency graph and geodesic edge distances
     * so `TilingNavigation2D<T>::find_path_cells()` and `compute_flow_field()` operate directly on the sphere.
     */
    void build_goldberg_sphere(Region& r_out_topology_region);

    /**
     * Finds the Goldberg cell whose spherical cone contains `p_direction` (in O(N) dot-product scan).
     */
    [[nodiscard]] int32_t find_closest_cell(const Vec3& p_direction) const noexcept;

    /**
     * Computes the great-circle geodesic distance `radius_ * acos(dot(n_i, n_j))` between two cells.
     */
    [[nodiscard]] T spherical_distance_between_cells(uint32_t p_cell_a, uint32_t p_cell_b) const noexcept;
};

} // namespace ideam::core