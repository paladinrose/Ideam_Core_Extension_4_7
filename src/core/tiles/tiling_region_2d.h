#pragma once

/**
 * tiling_region_2d.h
 * Concrete 2D Tiling Region & Topological Mesh/Graph Container (`src/core/tiles/`).
 *
 * Modernized C++26 Data-Oriented replacement for `SymmetryGroup<TGroup, TSymmetry>` and
 * `HexagonalSymmetryGroup<TGroup, TSymmetry>` in `old_unity_code/Topology/`:
 *   1. Replaces the O(N^2) `CoverRegion` coroutine with an O(N) analytical lattice scanline fill
 *      plus O(1) spatial-hashed vertex & undirected-edge deduplication.
 *   2. Preserves and accelerates `FindBorders` -> closed `Spline2D<T>` boundary loop extraction
 *      (`get_border_outlines()`), including multi-island and interior hole contours.
 *   3. Preserves and generalizes `Hexize()` & `GetHexEdges()` for hexagonal symmetry groups
 *      (`P3`, `P31M`, `P3M1`, `P6`, `P6MM`), aggregating valence-3, -6, or -12 fundamental cells
 *      into `HexSuperCellPOD` macro-hexagons with O(H) neighbor linking.
 *   4. Preserves `RemoveSymmetries` (`remove_cells`), `GetMaskedSymmetries` (`get_masked_cells`),
 *      `GetMaskIntersectedSymmetries` (`get_mask_intersected_cells`), `ContainsPoint`,
 *      `HexContainsPoint`, and `GetSymmetryIDsByGeneration` (`get_cell_ids_by_generation`).
 */

#include "isohedral_tiling_2d.h"
#include "tile_common.h"
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
class TilingRegion2D {
public:
    using Vec2    = glm::vec<2, T>;
    using Spline2 = Spline2D<T>;
    using Tiling  = IsohedralTiling2D<T>;

private:
    Tiling tiling_{};
    int32_t priority_ = 0;

    // --- Flat Contiguous Cell, Vertex & Edge Buffers (Ready for zero-copy TileDOD upload) ---
    std::vector<IsoCellPOD> cells_;
    std::vector<TileTransformPOD> cell_transforms_;
    std::vector<Vec2> shared_vertices_;

    // CSR-packed per-cell vertex indices (`cells_[c].first_vertex_idx` .. `+ vertex_count`)
    std::vector<uint32_t> cell_vertex_indices_;

    // Shared topological edges and 1:1 edge metadata
    std::vector<TileEdgePOD> edges_;
    std::vector<TileEdgeMetadataPOD> edge_metadata_;

    // CSR-packed per-cell edge IDs and neighbor cell IDs (`cells_[c].first_edge_idx` .. `+ vertex_count`)
    std::vector<uint32_t> cell_edge_indices_;
    std::vector<int32_t> cell_neighbor_indices_;

    // Ear-clipped triangle indices into `shared_vertices_` (3 per triangle)
    std::vector<uint32_t> triangle_indices_;
    AABB2D<T> extents_{};

    // --- Border Topology (`FindBorders` -> `Outline` in `SymmetryGroup.cs`) ---
    std::vector<uint32_t> border_cell_ids_;
    std::vector<uint32_t> border_edge_ids_;
    std::vector<uint32_t> border_vertex_ids_;
    std::vector<uint32_t> orphan_edge_ids_;
    std::vector<uint32_t> non_manifold_edge_ids_;
    std::vector<Spline2> border_outlines_;
    std::vector<PolylineSlicePOD> border_outline_slices_;

    // --- Dual-Level Hexagonal Super-Cell Hierarchy (`Hexize` in `HexagonalSymmetryGroup.cs`) ---
    std::vector<HexSuperCellPOD> hex_super_cells_;
    std::vector<uint32_t> hex_center_vertex_ids_;
    std::vector<uint32_t> hex_child_cell_indices_;
    std::vector<uint32_t> hex_edge_ids_;
    std::vector<TileEdgePOD> hex_edges_;

    // --- Synthesized Seam Bridge Quads (populated by `TilingSeamStitcher2D`) ---
    std::vector<BridgeQuadCellPOD> bridge_quads_;

    [[nodiscard]] static uint8_t _get_target_hex_valence(WallpaperGroupId p_group) noexcept;

public:
    TilingRegion2D() = default;
    explicit TilingRegion2D(const Tiling& p_tiling, int32_t p_priority = 0) noexcept
        : tiling_(p_tiling), priority_(p_priority) {}
    ~TilingRegion2D() = default;

    // ========================================================================
    // Configuration & Underlying Generator Access
    // ========================================================================

    void set_tiling(const Tiling& p_tiling) noexcept { tiling_ = p_tiling; }
    [[nodiscard]] constexpr const Tiling& get_tiling() const noexcept { return tiling_; }
    [[nodiscard]] constexpr Tiling& get_tiling_mut() noexcept { return tiling_; }

    void set_priority(int32_t p_priority) noexcept { priority_ = p_priority; }
    [[nodiscard]] constexpr int32_t get_priority() const noexcept { return priority_; }

    void clear() noexcept;
    [[nodiscard]] bool is_empty() const noexcept { return cells_.empty(); }

    // ========================================================================
    // Region Building, Vertex/Edge Deduplication, `FindBorders` & `Hexize`
    // ========================================================================

    /**
     * Fills an axis-aligned 2D bounding box (`p_bounds`), welds shared vertices and edges
     * in O(N) time, extracts closed border `Spline2D` outlines, and optionally runs `hexize()`.
     */
    void build_from_aabb(
        const AABB2D<T>& p_bounds,
        const Vec2& p_seed_point = Vec2(static_cast<T>(0.0)),
        bool p_auto_hexize = true,
        int p_edge_subdivisions = 12
    );

    /**
     * Fills one or more closed `Spline2D` boundary contours using `p_coverage_mode`,
     * welds shared vertices and edges in O(N) time, extracts border `Spline2D` outlines,
     * and optionally runs `hexize()`.
     */
    void build_from_splines(
        std::span<const Spline2> p_region_splines,
        const Vec2& p_seed_point = Vec2(static_cast<T>(0.0)),
        TilingCoverageMode p_coverage_mode = TilingCoverageMode::INCLUDE_STRADDLING,
        int p_spline_subdivisions = 12,
        bool p_auto_hexize = true
    );

    /**
     * Low-level O(N) topological builder that takes a list of world-space cell polygons,
     * cell descriptors, and cell transforms, deduplicates vertices within `p_weld_tolerance`
     * via spatial hashing, pairs shared edges into `TileEdgePOD` twins, triangulates all cells,
     * and invokes `find_borders()`.
     */
    void rebuild_topology_from_polygons(
        std::span<const std::vector<Vec2>> p_cell_polygons,
        std::span<const IsoCellPOD> p_cell_templates,
        std::span<const TileTransformPOD> p_transforms,
        T p_weld_tolerance = GeometryConstants<T>::WELD_EPSILON
    );

    /**
     * Modernized `SymmetryGroup::FindBorders`:
     * Counts edge incidence across active cells, marks `CELL_FLAG_ON_BORDER` on perimeter cells,
     * and walks directed boundary half-edges into closed `Spline2D<T>` contours (`border_outlines_`).
     */
    void find_borders();

    /**
     * Modernized `HexagonalSymmetryGroup::Hexize` & `GetHexEdges`:
     * Discovers interior vertices matching the group's hexagonal center valence (3 for `P3`,
     * 6 for `P31M`/`P3M1`/`P6`, 12 for `P6MM`), aggregates their incident fundamental cells
     * into `HexSuperCellPOD` macro-hexagons, extracts non-central outer `hex_edges_`, and
     * links 6-way hex-to-hex adjacency in O(H) time.
     */
    void hexize();

    /**
     * Modernized `SymmetryGroup::RemoveSymmetries(int[] ids)`:
     * Removes the specified cell indices in O(N + V + E) linear time, compacts and remaps
     * all vertex, edge, neighbor, and hex super-cell references, and refreshes `find_borders()`.
     */
    void remove_cells(std::span<const uint32_t> p_cell_ids_to_remove);

    // ========================================================================
    // Contiguous Span Accessors (For `TileDOD`, `TilingSeamStitcher2D` & Godot Nodes)
    // ========================================================================

    [[nodiscard]] size_t get_cell_count() const noexcept { return cells_.size(); }
    [[nodiscard]] size_t get_vertex_count() const noexcept { return shared_vertices_.size(); }
    [[nodiscard]] size_t get_edge_count() const noexcept { return edges_.size(); }
    [[nodiscard]] size_t get_hex_super_cell_count() const noexcept { return hex_super_cells_.size(); }

    [[nodiscard]] std::span<const IsoCellPOD> get_cells() const noexcept { return cells_; }
    [[nodiscard]] std::span<IsoCellPOD> get_cells_mut() noexcept { return cells_; }

    [[nodiscard]] std::span<const TileTransformPOD> get_cell_transforms() const noexcept { return cell_transforms_; }
    [[nodiscard]] std::span<const Vec2> get_vertices() const noexcept { return shared_vertices_; }
    [[nodiscard]] std::span<const uint32_t> get_cell_vertex_indices() const noexcept { return cell_vertex_indices_; }

    [[nodiscard]] std::span<const TileEdgePOD> get_edges() const noexcept { return edges_; }
    [[nodiscard]] std::span<TileEdgePOD> get_edges_mut() noexcept { return edges_; }
    [[nodiscard]] std::span<const TileEdgeMetadataPOD> get_edge_metadata() const noexcept { return edge_metadata_; }
    [[nodiscard]] std::span<TileEdgeMetadataPOD> get_edge_metadata_mut() noexcept { return edge_metadata_; }

    [[nodiscard]] std::span<const uint32_t> get_cell_edge_indices() const noexcept { return cell_edge_indices_; }
    [[nodiscard]] std::span<const int32_t> get_cell_neighbor_indices() const noexcept { return cell_neighbor_indices_; }
    [[nodiscard]] std::span<int32_t> get_cell_neighbor_indices_mut() noexcept { return cell_neighbor_indices_; }

    [[nodiscard]] std::span<const uint32_t> get_triangle_indices() const noexcept { return triangle_indices_; }
    [[nodiscard]] constexpr const AABB2D<T>& get_extents() const noexcept { return extents_; }

    // --- Per-Cell Slice Helpers ---
    [[nodiscard]] std::span<const uint32_t> get_cell_vertices(uint32_t p_cell_id) const noexcept;
    [[nodiscard]] std::span<const uint32_t> get_cell_edges(uint32_t p_cell_id) const noexcept;
    [[nodiscard]] std::span<const int32_t> get_cell_neighbors(uint32_t p_cell_id) const noexcept;
    [[nodiscard]] std::vector<Vec2> get_cell_polygon_points(uint32_t p_cell_id) const;
    [[nodiscard]] AABB2D<T> get_cell_bounds(uint32_t p_cell_id) const noexcept;

    // --- Border & Outline Accessors ---
    [[nodiscard]] std::span<const uint32_t> get_border_cell_ids() const noexcept { return border_cell_ids_; }
    [[nodiscard]] std::span<const uint32_t> get_border_edge_ids() const noexcept { return border_edge_ids_; }
    [[nodiscard]] std::span<const uint32_t> get_border_vertex_ids() const noexcept { return border_vertex_ids_; }
    [[nodiscard]] std::span<const Spline2> get_border_outlines() const noexcept { return border_outlines_; }
    [[nodiscard]] std::span<const PolylineSlicePOD> get_border_outline_slices() const noexcept { return border_outline_slices_; }

    // --- Hex Super-Cell Accessors ---
    [[nodiscard]] std::span<const HexSuperCellPOD> get_hex_super_cells() const noexcept { return hex_super_cells_; }
    [[nodiscard]] std::span<const uint32_t> get_hex_center_vertex_ids() const noexcept { return hex_center_vertex_ids_; }
    [[nodiscard]] std::span<const uint32_t> get_hex_child_cell_indices() const noexcept { return hex_child_cell_indices_; }
    [[nodiscard]] std::span<const uint32_t> get_hex_edge_ids() const noexcept { return hex_edge_ids_; }
    [[nodiscard]] std::span<const TileEdgePOD> get_hex_edges() const noexcept { return hex_edges_; }
    [[nodiscard]] std::vector<Vec2> get_hexagon_center_points() const;

    // --- Bridge Quad Accessors ---
    [[nodiscard]] std::span<const BridgeQuadCellPOD> get_bridge_quads() const noexcept { return bridge_quads_; }
    void set_bridge_quads(std::vector<BridgeQuadCellPOD> p_quads) noexcept { bridge_quads_ = std::move(p_quads); }

    // ========================================================================
    // Spatial Containment, Masking & Generation Queries
    // ========================================================================

    /**
     * Checks if `p_point` lies inside any active cell, writing the owning cell index to `r_cell_id`.
     */
    [[nodiscard]] bool contains_point(const Vec2& p_point, int32_t& r_cell_id) const noexcept;

    /**
     * Checks if `p_point` lies inside any `HexSuperCellPOD` (`HexContainsPoint` in `old_unity_code`),
     * writing the hex super-cell ID to `r_hex_id` and optionally the fundamental sub-cell ID to `r_sub_cell_id`.
     */
    [[nodiscard]] bool hex_contains_point(
        const Vec2& p_point,
        int32_t& r_hex_id,
        int32_t* r_sub_cell_id = nullptr
    ) const noexcept;

    /**
     * Returns all cell indices overlapped by `p_mask` (`GetMaskedSymmetries` in `SymmetryGroup.cs`).
     * When `p_thorough_check` is true, checks both cell centroids and all cell vertices.
     */
    [[nodiscard]] std::vector<uint32_t> get_masked_cells(
        std::span<const Spline2> p_mask,
        bool p_thorough_check = true,
        int p_subdivisions = 12
    ) const;

    /**
     * Returns all cell indices strictly outside `p_mask` (`GetUnmaskedSymmetries`).
     */
    [[nodiscard]] std::vector<uint32_t> get_unmasked_cells(
        std::span<const Spline2> p_mask,
        bool p_thorough_check = true,
        int p_subdivisions = 12
    ) const;

    /**
     * Identifies which masked cells have at least one edge that physically intersects a contour
     * in `p_mask` (`GetMaskIntersectedSymmetries` in `SymmetryGroup.cs`).
     */
    void get_mask_intersected_cells(
        std::span<const Spline2> p_mask,
        std::span<const uint32_t> p_masked_cell_ids,
        std::vector<uint32_t>& r_intersected_cell_ids,
        std::vector<uint32_t>& r_mask_spline_ids,
        int p_subdivisions = 12
    ) const;

    /**
     * Returns cell indices sorted in ascending order of their topological `generation` wave ring
     * (`GetSymmetryIDsByGeneration` in `SymmetryGroup.cs`).
     */
    [[nodiscard]] std::vector<uint32_t> get_cell_ids_by_generation() const;

    // --- In-Place Spatial Transforms (`Offset` & `Scale` from `SymmetryGroup.cs`) ---
    void offset_region(const Vec2& p_offset) noexcept;
    void scale_region(const Vec2& p_scale) noexcept;
};

} // namespace ideam::core