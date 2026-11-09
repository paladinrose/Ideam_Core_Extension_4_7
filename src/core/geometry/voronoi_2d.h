#pragma once

/**
 * voronoi_2d.h
 * 2D Delaunay Triangulation (Bowyer-Watson), Circumcircle Predicates, Clipped Voronoi Diagrams,
 * and Compressed Sparse Row (CSR) Dual Graph Adjacency Extraction.
 *
 * Bridges geometric partitions directly into:
 *   - NarrativeDOD::rebuild_location_adjacency_csr (territory / region graphs for Locations)
 *   - IdeamGraphDOD (dual graph topologies)
 *   - Games / Tiles (procedural mesh shattering, biome cellular generation, and tactical zones)
 */

#include "geometry_common.h"
#include "csg_2d.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * DelaunayTrianglePOD
 * Indexed triangle with circumcircle cache for Bowyer-Watson incremental insertion.
 */
template <FloatingPointScalar T = float>
struct DelaunayTrianglePOD {
    uint32_t v0 = 0u;
    uint32_t v1 = 0u;
    uint32_t v2 = 0u;
    glm::vec<2, T> circumcenter{static_cast<T>(0)};
    T circumradius_sq = static_cast<T>(0);
};

/**
 * VoronoiCell2D
 * Represents a single generator site's clipped Voronoi polygon and its dual Delaunay neighbors.
 */
template <FloatingPointScalar T = float>
struct VoronoiCell2D {
    uint32_t site_index = 0u;
    glm::vec<2, T> site_position{static_cast<T>(0)};
    std::vector<glm::vec<2, T>> polygon_vertices; // Ordered CCW
    std::vector<uint32_t> neighbor_site_indices;  // Dual graph neighbors
};

template <FloatingPointScalar T = float>
struct VoronoiMath2D {
    using Vec2 = glm::vec<2, T>;

    // ========================================================================
    // Geometric Predicates & Circumcircle Solvers
    // ========================================================================

    /**
     * 2D orientation predicate: > 0 if (a, b, c) is Counter-Clockwise, < 0 if Clockwise, 0 if Collinear.
     */
    [[nodiscard]] static constexpr T orient2d(const Vec2& p_a, const Vec2& p_b, const Vec2& p_c) noexcept {
        return (p_b.x - p_a.x) * (p_c.y - p_a.y) - (p_b.y - p_a.y) * (p_c.x - p_a.x);
    }

    /**
     * Computes the circumcenter and squared circumradius of triangle (p_a, p_b, p_c).
     * Returns false if the triangle is degenerate (collinear).
     */
    [[nodiscard]] static bool compute_circumcircle(
        const Vec2& p_a,
        const Vec2& p_b,
        const Vec2& p_c,
        Vec2& r_center,
        T& r_radius_sq
    ) noexcept;

    /**
     * Returns true if p_point lies strictly inside the circumcircle of triangle (p_a, p_b, p_c).
     */
    [[nodiscard]] static bool in_circumcircle(
        const Vec2& p_a,
        const Vec2& p_b,
        const Vec2& p_c,
        const Vec2& p_point
    ) noexcept;

    // ========================================================================
    // Delaunay Triangulation & Dual Voronoi Diagram Construction
    // ========================================================================

    /**
     * Bowyer-Watson Delaunay Triangulation of a 2D point set.
     * Returns a flat array of triangle vertex indices (3 * num_triangles, ordered CCW).
     */
    [[nodiscard]] static std::vector<uint32_t> triangulate_delaunay(
        std::span<const Vec2> p_sites,
        std::vector<DelaunayTrianglePOD<T>>* r_out_triangles = nullptr
    );

    /**
     * Constructs clipped 2D Voronoi cells for each generator site within p_bounds
     * by intersecting perpendicular bisector half-planes against the bounding box
     * (using CSGMath2D::clip_polygon_half_plane).
     */
    [[nodiscard]] static std::vector<VoronoiCell2D<T>> build_clipped_voronoi_cells(
        std::span<const Vec2> p_sites,
        const AABB2D<T>& p_bounds
    );

    // ========================================================================
    // DOD / CSR Graph Dualization Bridges
    // ========================================================================

    /**
     * Converts a triangle index buffer (3 indices per triangle) into a symmetric,
     * deduplicated Compressed Sparse Row (CSR) adjacency table (r_row_offsets of size N+1,
     * r_col_neighbors of size 2*E) compatible with NarrativeDOD::rebuild_location_adjacency_csr
     * and IdeamGraphDOD.
     */
    static void extract_delaunay_adjacency_csr(
        size_t p_site_count,
        std::span<const uint32_t> p_triangle_indices,
        std::vector<uint32_t>& r_row_offsets,
        std::vector<uint32_t>& r_col_neighbors
    );

    // ========================================================================
    // [SPECULATIVE / FUTURE PHASE] Constrained Delaunay (CDT), Power Diagrams & Lloyd Relaxation
    // ========================================================================
    /*
     * 1. Constrained Delaunay Triangulation (CDT) with Spline2D Hard Constraint Edges:
     *    - Inserts flattened Spline2D segments into the Delaunay mesh via Lawson edge-flipping
     *      and optionally removes exterior/hole triangles for curved dynamic NavMeshes:
     *        [[nodiscard]] static std::vector<uint32_t> triangulate_constrained_delaunay(
     *            std::span<const Vec2> p_vertices,
     *            std::span<const glm::uvec2> p_constraint_edges,
     *            bool p_remove_exterior = true
     *        );
     *
     * 2. Generalized Laguerre / Power Diagrams (Weighted Voronoi):
     *    - Shifts the bisector plane between site i (weight w_i) and site j (weight w_j)
     *      by (w_i - w_j) / (2 * |p_j - p_i|^2), enabling territory size control for
     *      Narrative Locations or GameAgent influence zones:
     *        [[nodiscard]] static std::vector<VoronoiCell2D<T>> build_power_diagram(
     *            std::span<const Vec2> p_sites,
     *            std::span<const T> p_site_weights,
     *            const AABB2D<T>& p_bounds
     *        );
     *
     * 3. Lloyd's Relaxation Step (Centroidal Voronoi Tessellation / Blue-Noise Scatter):
     *    - Moves each generator site toward its Voronoi cell's area-weighted centroid
     *      (PolygonMath2D<T>::centroid):
     *        static void lloyd_relaxation_step(
     *            std::span<Vec2> r_sites,
     *            const AABB2D<T>& p_bounds,
     *            T p_step_strength = static_cast<T>(1.0)
     *        );
     */
};

} // namespace ideam::core