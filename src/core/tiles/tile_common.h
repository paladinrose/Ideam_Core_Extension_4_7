#pragma once

/**
 * tile_common.h
 * Foundational pure C++26 Data-Oriented Design (DOD) enums, constants, and cache-aligned
 * POD schemas for `src/core/tiles/` and `src/core/tasks/domains/tiles/` (`TileDOD`).
 *
 * Replaces the legacy OOP classes and heap-allocating structs in `old_unity_code/Topology/`:
 *   - `IsoSymmetry`, `Symmetry3`, `Symmetry4`, `Symmetry6` -> `IsoCellPOD` (32B) & `TileTransformPOD` (32B)
 *   - `SymmetryEdge` & `SymmetryNeighbor`                  -> `TileEdgePOD` (16B) & `TileEdgeMetadataPOD` (16B)
 *   - `HexagonalSymmetryGroup::hexSymmetries` (`Hexize`)   -> `HexSuperCellPOD` (64B)
 *   - `ComplexSymmetryGroup::MergeGroup` bridge quads      -> `BridgeQuadCellPOD` (32B)
 *   - `ITiling` / `BaseTiling` symmetry handles            -> `SymmetryElementHandlesPOD<T>`
 *   - Periodic neighbor step lookup                        -> `QuotientNeighborStepPOD` (8B)
 */

#include "../geometry/geometry_common.h"
#include "../geometry/symmetry_2d.h"
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace ideam::core {

// ============================================================================
// Tiling Configuration & Topological Mode Enums
// ============================================================================

/**
 * Selects how a tiling's symmetry and prototile geometry are authored.
 */
enum class TilingAuthoringMode : uint8_t {
    WALLPAPER_GROUP_17   = 0, // Driven by WallpaperGroupId (P1..P6M) + parametric symmetry handles
    ISOHEDRAL_TACTILE_81 = 1  // Driven by csk::IsohedralTiling IH01..IH93 + 0..6 vertex params & J/U/S/I edges
};

/**
 * Multi-modal region coverage rules when filling or masking against a closed `Spline2D` contour.
 * Modernizes `requireAllPointsCoverage` and `GetMaskedSymmetries(thoroughCheck)` from `SymmetryGroup.cs`.
 */
enum class TilingCoverageMode : uint8_t {
    STRICT_INTERIOR      = 0, // Keep only cells whose vertices all lie strictly inside the boundary
    CENTROID_INSIDE      = 1, // Keep cells whose area centroid lies inside the boundary
    INCLUDE_STRADDLING   = 2, // Keep both FULLY_INSIDE and STRADDLING_BOUNDARY whole cells
    CLIP_EXACT_TO_SPLINE = 3  // Keep FULLY_INSIDE as fast MultiMesh instances; clip STRADDLING_BOUNDARY via CSG
};

/**
 * Seam-stitching strategies when merging disparate tilings (`ComplexSymmetryGroup::MergeGroup`).
 */
enum class TilingStitchMode : uint8_t {
    QUAD_BRIDGE              = 0, // Prunes overlapped subordinate cells and builds 4-sided BridgeQuadCellPODs
    TOPOLOGICAL_SEAM_WELD    = 1, // Clips both tilings along a shared Spline2D seam and welds shared vertices
    CURVILINEAR_METRIC_MORPH = 2  // Continuously interpolates lattice basis vectors across a seam corridor
};

/**
 * Directional traversal permissions per tile edge (`SymmetryNeighbor.PortDirection` in `old_unity_code`).
 */
enum class TilePortDirection : uint8_t {
    PORT_FREE     = 0, // Bidirectional traversal allowed
    PORT_IN_ONLY  = 1, // Incoming traversal only (cell_b -> cell_a)
    PORT_OUT_ONLY = 2, // Outgoing traversal only (cell_a -> cell_b)
    PORT_CLOSED   = 3  // Blocked / impassable wall edge
};

/**
 * Bitmask state flags stored on each `IsoCellPOD::flags`.
 */
enum TileCellFlags : uint8_t {
    CELL_FLAG_NONE        = 0u,
    CELL_FLAG_ON_BORDER   = 1u << 0, // Cell has at least one exposed boundary edge (cell_b == -1)
    CELL_FLAG_BRIDGE_QUAD = 1u << 1, // Synthesized 4-sided seam bridge cell from MergeGroup
    CELL_FLAG_MASKED      = 1u << 2, // Pruned / deactivated by a higher-priority group or mask spline
    CELL_FLAG_REFLECTED   = 1u << 3, // Aspect transform has negative determinant (mirror / glide reflection)
    CELL_FLAG_HEX_MEMBER  = 1u << 4, // Belongs to a valid HexSuperCellPOD cluster (`Hexize`)
    CELL_FLAG_CLIPPED_CAP = 1u << 5  // Boundary cell whose polygon was clipped against a Spline2D contour
};

/**
 * Legacy fundamental domain primitive presets from `old_unity_code/Topology/Topology.cs`.
 */
enum class FundamentalShapePreset : uint8_t {
    TACTILE_DEFAULT      = 0, // Uses csk::IsohedralTiling's computed prototile vertices
    QUAD                 = 1, // 4-sided unit square [-0.5, 0.5]^2 (P1, P2, PM, P2MM, P2MG, P2GG, P4)
    BRICK_HEX            = 2, // 6-sided collinear-split rectangle for staggered brick adjacency (PG)
    RIGHT_TRIANGLE       = 3, // 3-sided right isosceles triangle (C2MM, P4MM, P4MG)
    EQUILATERAL_TRIANGLE = 4, // 3-sided equilateral triangle (P3M1)
    FLAT_TRIANGLE        = 5, // 3-sided 120-30-30 obtuse triangle (CM, P31M, P6)
    TALL_TRIANGLE        = 6, // 3-sided 30-60-90 right triangle (P6MM)
    RHOMBUS_60           = 7  // 4-sided 60/120 degree rhombus (P3)
};

// ============================================================================
// Core Cache-Aligned POD Structs for `src/core/tiles/` & `TileDOD`
// ============================================================================

/**
 * IsoCellPOD (32 Bytes, Cache-Line Friendly)
 * Universal fixed-size DOD cell record replacing `IsoSymmetry`, `Symmetry3`, `Symmetry4`, and `Symmetry6`.
 */
struct alignas(16) IsoCellPOD {
    int16_t lattice_i             = 0;          // Translational unit-cell coordinate i along t1
    int16_t lattice_j             = 0;          // Translational unit-cell coordinate j along t2
    uint8_t group_slot            = 0u;         // Owning tiling group index in CompositeTiling / TileDOD
    uint8_t aspect_index          = 0u;         // Aspect index k in [0, num_aspects - 1] within unit cell
    uint8_t vertex_count          = 4u;         // Number of polygon vertices (3, 4, 6, or clipped N)
    uint8_t flags                 = CELL_FLAG_NONE; // Bitmask of TileCellFlags
    uint16_t generation           = 0u;         // Topological wave distance from seed or seam
    uint16_t colour_id            = 0u;         // Tactile 3-color index (0, 1, or 2) or material variant
    uint32_t first_vertex_idx     = 0xFFFFFFFFu;// Start offset in cell vertex index CSR slice
    uint32_t first_edge_idx       = 0xFFFFFFFFu;// Start offset in cell edge index CSR slice
    uint32_t super_hex_id         = 0xFFFFFFFFu;// Owning HexSuperCellPOD index (or 0xFFFFFFFFu if none)
    glm::vec2 center{0.0f, 0.0f};               // World-space centroid of the tile cell
};

/**
 * TileTransformPOD (32 Bytes, 16-Byte SIMD Aligned)
 * Compact 2D affine transform and instance payload for batch-populating Godot `MultiMesh` buffers.
 */
struct alignas(16) TileTransformPOD {
    glm::vec2 position{0.0f, 0.0f};             // World-space translation
    glm::vec2 scale{1.0f, 1.0f};                // Signed scale (scale.x * scale.y < 0 indicates reflection)
    float angle_rad               = 0.0f;       // Rotation angle in radians
    uint32_t cell_id              = 0xFFFFFFFFu;// Back-reference to IsoCellPOD slot
    glm::vec2 custom_data{0.0f, 0.0f};          // x = normalized colour_id, y = generation wave
};

/**
 * TileEdgePOD (16 Bytes)
 * Topological half-edge / shared-edge connectivity record replacing `SymmetryEdge`.
 */
struct alignas(16) TileEdgePOD {
    uint32_t vertex_a             = 0xFFFFFFFFu;// Start vertex index in shared vertex buffer
    uint32_t vertex_b             = 0xFFFFFFFFu;// End vertex index in shared vertex buffer
    int32_t cell_a                = -1;         // Primary owning cell index
    int32_t cell_b                = -1;         // Twin neighbor cell index (-1 if border edge)

    [[nodiscard]] constexpr bool is_border() const noexcept {
        return cell_a < 0 || cell_b < 0;
    }

    [[nodiscard]] constexpr bool shares_vertices(uint32_t p_va, uint32_t p_vb) const noexcept {
        return (vertex_a == p_va && vertex_b == p_vb) ||
               (vertex_a == p_vb && vertex_b == p_va);
    }
};

/**
 * TileEdgeMetadataPOD (16 Bytes)
 * Weighted graph and port traversal attributes associated 1:1 with `TileEdgePOD`.
 */
struct alignas(16) TileEdgeMetadataPOD {
    float edge_length             = 0.0f;       // Geometric length of the edge
    float cross_distance          = 1.0f;       // Traversal cost between cell_a and cell_b centroids
    TilePortDirection port_dir    = TilePortDirection::PORT_FREE;
    uint8_t edge_state            = 0u;         // User/gameplay state byte (door, wall, seam, road)
    uint8_t shape_type            = 0u;         // TactileEdgeShape (J=0, U=1, S=2, I=3)
    uint8_t is_reversed           = 0u;         // 1 if edge parameterization is reversed
    uint32_t reserved             = 0u;
};

/**
 * HexSuperCellPOD (64 Bytes, Single Hardware Cache Line)
 * Modernized DOD representation of `HexagonalSymmetryGroup::hexSymmetries` (`Symmetry6`).
 * Aggregates 3 rhombi (`P3`), 6 triangles (`P31M`, `P3M1`, `P6`), or 12 triangles (`P6MM`)
 * around a central hexagonal lattice vertex into a 6-sided macro-hexagon.
 *
 * Layout:
 *   - `center` (8B) + `center_vertex_id` (4B) + `child_slice_packed` (4B: 24-bit offset | 8-bit count) = 16B
 *   - `perimeter_vertex_ids` (6 * 4B = 24B)
 *   - `neighbor_hex_ids`     (6 * 4B = 24B)
 *   Total = 64 Bytes.
 */
struct alignas(16) HexSuperCellPOD {
    glm::vec2 center{0.0f, 0.0f};               // World-space coordinates of the hex center vertex
    uint32_t center_vertex_id     = 0xFFFFFFFFu;// Shared vertex buffer index of the hex center
    uint32_t child_slice_packed   = 0u;         // Low 24 bits: child_cell_offset, High 8 bits: child_cell_count
    std::array<uint32_t, 6> perimeter_vertex_ids = {
        0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu
    };
    std::array<int32_t, 6> neighbor_hex_ids = { -1, -1, -1, -1, -1, -1 };

    [[nodiscard]] constexpr uint32_t get_child_offset() const noexcept {
        return child_slice_packed & 0x00FFFFFFu;
    }

    [[nodiscard]] constexpr uint8_t get_child_count() const noexcept {
        return static_cast<uint8_t>((child_slice_packed >> 24u) & 0xFFu);
    }

    constexpr void set_child_slice(uint32_t p_offset, uint8_t p_count) noexcept {
        child_slice_packed = (p_offset & 0x00FFFFFFu) | (static_cast<uint32_t>(p_count) << 24u);
    }
};

/**
 * BridgeQuadCellPOD (32 Bytes)
 * Represents a 4-sided topological bridge cell synthesized by `TilingSeamStitcher2D`
 * (`ComplexSymmetryGroup::MergeGroup`) to stitch an exposed border edge of Group A to Group B.
 */
struct alignas(16) BridgeQuadCellPOD {
    uint32_t cell_id              = 0xFFFFFFFFu;// Index of this bridge cell in the unified IsoCellPOD array
    uint32_t dominant_cell_id     = 0xFFFFFFFFu;// Neighbor cell ID in Dominant Group A
    uint32_t subordinate_cell_id  = 0xFFFFFFFFu;// Neighbor cell ID in Subordinate Group B
    uint16_t dominant_edge_slot   = 0u;         // Local edge slot on dominant_cell_id
    uint16_t subordinate_edge_slot= 0u;         // Local edge slot on subordinate_cell_id
    std::array<uint32_t, 4> quad_vertex_ids = { // CCW ordered vertex IDs (v0, v1 from A; v2, v3 from B)
        0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu
    };
};

/**
 * QuotientNeighborStepPOD (8 Bytes)
 * Precomputed relative lattice step across edge `e` of aspect `k` inside any unit cell `(i, j)`.
 * Enables O(1) zero-memory adjacency lookup across unhindered periodic tilings.
 */
struct QuotientNeighborStepPOD {
    int8_t delta_i                = 0;   // Relative unit-cell step along t1 (-1, 0, or +1)
    int8_t delta_j                = 0;   // Relative unit-cell step along t2 (-1, 0, or +1)
    uint8_t next_aspect           = 0u;  // Target aspect index in [0, num_aspects - 1]
    uint8_t shared_edge_slot      = 0u;  // Matching local edge slot on the neighbor aspect
    uint8_t is_valid              = 0u;  // 1 if a periodic neighbor exists across this edge
    uint8_t reflection_change     = 0u;  // 1 if crossing this edge flips handedness
    uint16_t base_cost_fp8        = 256u;// Fixed-point (1.0 = 256) centroid-to-centroid step distance
};

/**
 * SymmetryElementHandlesPOD<T>
 * Modernized C++26 container for the explicit symmetry generators in `old_unity_code/Topology/Topology.cs`
 * (`ITiling::AxesOfSymmetry`, `PointsOfRotation`, `Glides`), used for interactive gizmos and custom group builds.
 */
template <FloatingPointScalar T = float>
struct SymmetryElementHandlesPOD {
    using Vec2 = glm::vec<2, T>;

    std::array<Vec2, 4> axes_of_symmetry{};
    std::array<Vec2, 5> points_of_rotation{};
    std::array<Vec2, 2> glides{};
    uint8_t axis_count            = 0u;
    uint8_t rotation_pivot_count  = 0u;
    uint8_t glide_count           = 0u;
    uint8_t reserved              = 0u;
};

/**
 * PrototileEdgeDescriptorPOD<T>
 * Describes one of the (3..6) fundamental domain edges, its Tactile symmetry rule (J, U, S, I),
 * and its similarity matrix mapping canonical [0, 1] onto the prototile edge.
 */
template <FloatingPointScalar T = float>
struct PrototileEdgeDescriptorPOD {
    using Vec2 = glm::vec<2, T>;
    using Mat3 = glm::mat<3, 3, T>;

    uint8_t edge_shape_id         = 0u;
    TactileEdgeShape shape        = TactileEdgeShape::EDGE_I;
    bool reversed                 = false;
    uint8_t reserved              = 0u;
    Vec2 start_vertex{static_cast<T>(0)};
    Vec2 end_vertex{static_cast<T>(0)};
    Mat3 canonical_to_edge_xform{static_cast<T>(1)};
};

/**
 * BakedPrototilePOD<T>
 * Complete precomputed snapshot of an Isohedral / Wallpaper prototile, its lattice basis,
 * its up to 12 unit-cell aspect matrices, and its 12x6 quotient neighbor table.
 */
template <FloatingPointScalar T = float>
struct BakedPrototilePOD {
    using Vec2 = glm::vec<2, T>;
    using Mat3 = glm::mat<3, 3, T>;

    static constexpr size_t MAX_VERTICES   = 6;
    static constexpr size_t MAX_ASPECTS    = 12;
    static constexpr size_t MAX_PARAMETERS = 6;

    WallpaperGroupId wallpaper_group       = WallpaperGroupId::P1;
    uint8_t tactile_ih_type                = 1u;
    uint8_t num_vertices                   = 4u;
    uint8_t num_aspects                    = 1u;
    uint8_t num_params                     = 0u;
    uint8_t num_edge_shapes                = 1u;
    FundamentalShapePreset shape_preset    = FundamentalShapePreset::TACTILE_DEFAULT;
    uint8_t reserved                       = 0u;

    LatticeBasis2DPOD<T> lattice_basis{};
    std::array<T, MAX_PARAMETERS> parameters{};
    std::array<Vec2, MAX_VERTICES> base_vertices{};
    std::array<PrototileEdgeDescriptorPOD<T>, MAX_VERTICES> edges{};

    std::array<Mat3, MAX_ASPECTS> aspect_xforms{};
    std::array<Mat3, MAX_ASPECTS> inv_aspect_xforms{};
    std::array<Vec2, MAX_ASPECTS> aspect_local_centers{};
    std::array<T, MAX_ASPECTS> aspect_angles_rad{};
    std::array<Vec2, MAX_ASPECTS> aspect_scales{};
    std::array<int8_t, MAX_ASPECTS> aspect_orientations{}; // +1 proper rotation, -1 reflection

    // 19-byte Tactile colouring table for O(1) 3-coloring lookup
    std::array<uint8_t, 19> colouring_table{};

    // O(1) Quotient-Graph Adjacency Table: [aspect_idx][edge_slot]
    std::array<std::array<QuotientNeighborStepPOD, MAX_VERTICES>, MAX_ASPECTS> quotient_adjacency{};
};

// Compile-time size and layout verifications
static_assert(std::is_trivially_copyable_v<IsoCellPOD> && std::is_standard_layout_v<IsoCellPOD>);
static_assert(sizeof(IsoCellPOD) == 32, "IsoCellPOD must be 32 bytes!");

static_assert(std::is_trivially_copyable_v<TileTransformPOD> && std::is_standard_layout_v<TileTransformPOD>);
static_assert(sizeof(TileTransformPOD) == 32, "TileTransformPOD must be 32 bytes!");

static_assert(std::is_trivially_copyable_v<TileEdgePOD> && std::is_standard_layout_v<TileEdgePOD>);
static_assert(sizeof(TileEdgePOD) == 16, "TileEdgePOD must be 16 bytes!");

static_assert(std::is_trivially_copyable_v<TileEdgeMetadataPOD> && std::is_standard_layout_v<TileEdgeMetadataPOD>);
static_assert(sizeof(TileEdgeMetadataPOD) == 16, "TileEdgeMetadataPOD must be 16 bytes!");

static_assert(std::is_trivially_copyable_v<HexSuperCellPOD> && std::is_standard_layout_v<HexSuperCellPOD>);
static_assert(sizeof(HexSuperCellPOD) == 64, "HexSuperCellPOD must fit in one 64-byte cache line!");

static_assert(std::is_trivially_copyable_v<BridgeQuadCellPOD> && std::is_standard_layout_v<BridgeQuadCellPOD>);
static_assert(sizeof(BridgeQuadCellPOD) == 32, "BridgeQuadCellPOD must be 32 bytes!");

static_assert(std::is_trivially_copyable_v<QuotientNeighborStepPOD> && std::is_standard_layout_v<QuotientNeighborStepPOD>);
static_assert(sizeof(QuotientNeighborStepPOD) == 8, "QuotientNeighborStepPOD must be 8 bytes!");

} // namespace ideam::core