#pragma once

/**
 * tiling_navigation_2d.h
 * Symmetry-Accelerated A* Pathfinding (Fundamental Cells & `Hexize` Super-Cells),
 * Multi-Source Eikonal / Dijkstra Wavefront Flow-Fields, and Barycentric Sub-Triangle Splat
 * Mask Generation (`src/core/tiles/`).
 *
 * Modernized C++26 Data-Oriented replacement for:
 *   - `SymmetryGroup::GraphSearch` (`old_unity_code/Topology/SymmetryGroup.cs`)
 *   - `HexagonalSymmetryGroup::GraphSearch` (`old_unity_code/Topology/HexagonalSymmetryGroup.cs`)
 *   - `Topology::EquilateralHexSplats` & `GetTrianglePixels` (`old_unity_code/Topology/Topology.cs`)
 */

#include "tile_common.h"
#include "tiling_region_2d.h"
#include "../geometry/geometry_common.h"
#include "../geometry/polygon_math_2d.h"
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * Output of `TilingNavigation2D::compute_flow_field` across a `TilingRegion2D<T>`.
 * Ready for direct consumption by `TileDOD` (`TILE_FLOW_ARRIVAL_TIME`, `TILE_FLOW_VECTOR_2D`)
 * or `GameAgent` steering tasks.
 */
template <FloatingPointScalar T = float>
struct TilingFlowFieldResult {
    using Vec2 = glm::vec<2, T>;

    std::vector<T> arrival_costs;          // Geodesic cost to reach nearest goal (infinity if unreachable)
    std::vector<int32_t> next_cell_ids;    // Optimal next neighbor cell ID along shortest path to goal
    std::vector<uint16_t> wave_steps;      // Topological hop count from nearest goal
    std::vector<Vec2> flow_vectors;        // Normalized 2D world-space steering direction per cell
};

/**
 * Analytical 6-subtriangle barycentric weight breakdown for a point inside a triangle/hex cell.
 * Generalizes `Topology.EquilateralHexSplats` without requiring CPU pixel rasterization.
 */
template <FloatingPointScalar T = float>
struct BarycentricSubSplatPOD {
    glm::vec<3, T> barycentric_uvw{static_cast<T>(0)}; // Standard (u, v, w) vertex weights
    uint8_t active_sub_triangle = 0u;                  // 0..5 sub-triangle sector around centroid
    std::array<T, 6> sector_weights{};                 // Smoothly blended weights across the 6 sectors
};

template <FloatingPointScalar T = float>
struct TilingNavigation2D {
    using Vec2   = glm::vec<2, T>;
    using Vec3   = glm::vec<3, T>;
    using Region = TilingRegion2D<T>;

    // ========================================================================
    // Dual-Level A* Pathfinding (Fundamental Cells & `Hexize` Super-Cells)
    // ========================================================================

    /**
     * Binary min-heap A* search across fundamental/stitched cells in `p_region`
     * (modernizing `SymmetryGroup::GraphSearch`).
     * Respects `TileEdgeMetadataPOD::port_dir` (`PORT_FREE`, `PORT_IN_ONLY`, `PORT_OUT_ONLY`, `PORT_CLOSED`),
     * `TileEdgeMetadataPOD::cross_distance`, and optional per-cell cost multipliers.
     *
     * @return Ordered sequence of cell IDs from `p_start_cell` to `p_end_cell` (inclusive), or empty if unreachable.
     */
    [[nodiscard]] static std::vector<uint32_t> find_path_cells(
        const Region& p_region,
        uint32_t p_start_cell,
        uint32_t p_end_cell,
        std::span<const T> p_cell_cost_multipliers = {},
        bool p_manhattan_heuristic = false
    );

    /**
     * Binary min-heap A* search across aggregated `HexSuperCellPOD` macro-hexagons in `p_region`
     * (modernizing `HexagonalSymmetryGroup::GraphSearch`).
     *
     * @return Ordered sequence of hex super-cell IDs from `p_start_hex` to `p_end_hex` (inclusive), or empty if unreachable.
     */
    [[nodiscard]] static std::vector<uint32_t> find_path_hex_super_cells(
        const Region& p_region,
        uint32_t p_start_hex,
        uint32_t p_end_hex,
        bool p_manhattan_heuristic = false
    );

    // ========================================================================
    // Multi-Source Dijkstra / Eikonal Wavefront Flow-Field Solver
    // ========================================================================

    /**
     * Computes a complete multi-goal navigation flow-field across all cells in `p_region`
     * (including across stitched `BridgeQuadCellPOD` seams and port-constrained edges).
     */
    [[nodiscard]] static TilingFlowFieldResult<T> compute_flow_field(
        const Region& p_region,
        std::span<const uint32_t> p_goal_cells,
        std::span<const T> p_cell_cost_multipliers = {}
    );

    // ========================================================================
    // Barycentric Sub-Triangle Splat Evaluation & Atlas Generation (`EquilateralHexSplats`)
    // ========================================================================

    /**
     * Analytically evaluates the 3-vertex barycentric coordinates and 6-sector centroidal
     * sub-triangle splat weights for `p_point` inside triangle `(p_v0, p_v1, p_v2)`.
     */
    [[nodiscard]] static BarycentricSubSplatPOD<T> evaluate_triangle_sub_splat(
        const Vec2& p_v0,
        const Vec2& p_v1,
        const Vec2& p_v2,
        const Vec2& p_point
    ) noexcept;

    /**
     * Modernized C++26 port of `Topology::EquilateralHexSplats(int splatSize, out Float3[] pix, out int[] splatCountIDs)`:
     * Generates the 1-to-6 sub-triangle RGB splat masks across the canonical equilateral triangle
     * edge subdivisions (`0.2, 0.333334, 0.5, 0.666667, 0.8`) for terrain/biome texture blending.
     */
    static void generate_equilateral_hex_splat_atlas(
        uint32_t p_splat_size,
        std::vector<Vec3>& r_out_pixels,
        std::vector<uint32_t>& r_out_splat_count_offsets
    );
};

} // namespace ideam::core