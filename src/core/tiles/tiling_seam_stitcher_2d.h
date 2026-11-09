#pragma once

/**
 * tiling_seam_stitcher_2d.h
 * Multi-Group Priority Masking, Quad Bridge Seam Synthesis (`MergeGroup` & `PopulateGroup`),
 * Topological CSG Seam Welding, and Curvilinear Metric Morphing (`src/core/tiles/`).
 *
 * Modernized C++26 Data-Oriented replacement for `ComplexSymmetryGroup` in
 * `old_unity_code/Topology/ComplexSymmetryGroup.cs`:
 *   1. `merge_into_dominant`: Merges a subordinate `TilingRegion2D<T>` into a dominant
 *      `TilingRegion2D<T>` using one of three `TilingStitchMode` algorithms:
 *        - `QUAD_BRIDGE`: Prunes overlapped subordinate cells against the dominant region's
 *          extracted `Spline2D` `border_outlines`, identifies newly exposed border edges on
 *          both sides of the seam, pairs closest non-crossing edge endpoints (`distAA + distBB`
 *          vs. `distAB + distBA`), and synthesizes 4-sided `BridgeQuadCellPOD` cells.
 *        - `TOPOLOGICAL_SEAM_WELD`: Clips straddling boundary cells on both sides of a shared
 *          `Spline2D` seam curve (`CSGMath2D::weld_polygons_along_seam`) and welds shared vertices.
 *        - `CURVILINEAR_METRIC_MORPH`: Smoothly interpolates boundary-corridor vertex positions
 *          across the seam transition band before welding topology.
 *   2. `populate_composite_region`: Sorts an array of `TilingRegion2D<T>` layers by priority
 *      and merges them sequentially into a single unified composite region.
 */

#include "tile_common.h"
#include "tiling_region_2d.h"
#include "../geometry/csg_2d.h"
#include "../geometry/polygon_math_2d.h"
#include "../splines/spline_2d.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

template <FloatingPointScalar T = float>
struct TilingSeamStitcher2D {
    using Vec2    = glm::vec<2, T>;
    using Spline2 = Spline2D<T>;
    using Region  = TilingRegion2D<T>;

    /**
     * Merges `p_subordinate` into `r_dominant`, preserving `r_dominant` wherever the two regions
     * overlap and stitching the seam between them using `p_stitch_mode`.
     *
     * @param r_dominant The dominant (higher-priority) region modified in-place.
     * @param p_subordinate The incoming (lower-priority) region to prune and stitch.
     * @param p_stitch_mode Seam stitching algorithm (`QUAD_BRIDGE`, `TOPOLOGICAL_SEAM_WELD`, or `CURVILINEAR_METRIC_MORPH`).
     * @param p_explicit_seam_spline Optional explicit `Spline2D<T>` seam curve (if nullptr, uses `r_dominant.get_border_outlines()`).
     * @param p_max_bridge_span Maximum world-space distance allowed when pairing exposed border edges into bridge quads (0 = auto-estimate from cell size).
     */
    static void merge_into_dominant(
        Region& r_dominant,
        const Region& p_subordinate,
        TilingStitchMode p_stitch_mode = TilingStitchMode::QUAD_BRIDGE,
        const Spline2* p_explicit_seam_spline = nullptr,
        T p_max_bridge_span = static_cast<T>(0.0)
    );

    /**
     * Modernized `ComplexSymmetryGroup::PopulateGroup`:
     * Sorts `p_regions` by `p_priorities` (ascending priority order, matching `x.priority.CompareTo(y.priority)`
     * in `ComplexSymmetryGroup.cs` where index 0 is merged first as the dominant baseline) and iteratively
     * stitches all regions into a single unified `TilingRegion2D<T>`.
     */
    [[nodiscard]] static Region populate_composite_region(
        std::span<const Region> p_regions,
        std::span<const int32_t> p_priorities = {},
        TilingStitchMode p_stitch_mode = TilingStitchMode::QUAD_BRIDGE,
        T p_max_bridge_span = static_cast<T>(0.0)
    );
};

} // namespace ideam::core