#pragma once

/**
 * isohedral_tiling_2d.h
 * Unified C++26 Wallpaper Group (17 IUCr groups) & Isohedral Tiling (81 Tactile IH01..IH93 types)
 * mathematical engine for `src/core/tiles/`.
 *
 * Bridges three systems into one Data-Oriented API:
 *   1. `src/external/tactile/tiling.hpp` (`csk::IsohedralTiling` & `csk::FillAlgorithm`) for
 *      the 81 isohedral tile topologies, 0..6 vertex parameters, 1..12 aspect matrices,
 *      J/U/S/I edge symmetry rules, and 3-color modular coloring.
 *   2. `src/core/geometry/symmetry_2d.h` (`SymmetryMath2D<T>`) & `old_unity_code/Topology/Topology.cs`
 *      for the 17 Wallpaper Groups (`P1`..`P6M`), explicit symmetry element handles
 *      (`AxesOfSymmetry`, `PointsOfRotation`, `Glides`), and legacy fundamental domain presets
 *      (`Quad`, `Brick`, `RightTriangle`, `EquilateralTriangle`, `FlatTriangle`, `TallTriangle`, `Rhombus`).
 *   3. `src/core/splines/spline_2d.h` (`Spline2D<T>`) & `src/core/geometry/csg_2d.h` (`CSGMath2D<T>`)
 *      for Escher-style curved tile edges and multi-modal `Spline2D` region filling.
 */

#include "tile_common.h"
#include "../geometry/csg_2d.h"
#include "../geometry/polygon_math_2d.h"
#include "../geometry/symmetry_2d.h"
#include "../splines/spline_2d.h"
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * Result of enumerating and classifying tiles across a 2D AABB or closed `Spline2D` region.
 * Directly feeds `TilingRegion2D`, `TileDOD` buffers, and Godot `MultiMesh` / `ArrayMesh` builders.
 */
template <FloatingPointScalar T = float>
struct TilingRegionFillResult {
    using Vec2 = glm::vec<2, T>;

    std::vector<IsoCellPOD> cells;
    std::vector<TileTransformPOD> transforms;
    std::vector<ClipClassification2D> classifications;

    // Indices into `cells` partitioned for Wave 2 / Wave 3 DOD execution
    std::vector<uint32_t> interior_cell_indices;
    std::vector<uint32_t> boundary_seam_cell_indices;

    // Optional exact CSG-clipped boundary polygons (populated when mode == CLIP_EXACT_TO_SPLINE)
    std::vector<std::vector<Vec2>> clipped_boundary_polygons;
};

template <FloatingPointScalar T = float>
class IsohedralTiling2D {
public:
    using Vec2      = glm::vec<2, T>;
    using Vec2i     = glm::vec<2, int32_t>;
    using Mat2      = glm::mat<2, 2, T>;
    using Mat3      = glm::mat<3, 3, T>;
    using Spline2   = Spline2D<T>;
    using BakedPOD  = BakedPrototilePOD<T>;
    using HandlesPOD= SymmetryElementHandlesPOD<T>;

private:
    TilingAuthoringMode authoring_mode_        = TilingAuthoringMode::WALLPAPER_GROUP_17;
    WallpaperGroupId wallpaper_group_          = WallpaperGroupId::P1;
    uint8_t tactile_ih_type_                   = 1u;
    FundamentalShapePreset shape_preset_       = FundamentalShapePreset::TACTILE_DEFAULT;

    Vec2 lattice_scale_{static_cast<T>(1.0), static_cast<T>(1.0)};
    T lattice_rotation_rad_                    = static_cast<T>(0.0);
    Vec2 lattice_origin_{static_cast<T>(0.0), static_cast<T>(0.0)};

    HandlesPOD symmetry_handles_{};
    BakedPOD baked_{};

    // Optional custom Spline2D curves per Tactile edge_shape_id (up to 6)
    std::array<Spline2, BakedPOD::MAX_VERTICES> edge_splines_{};
    std::array<bool, BakedPOD::MAX_VERTICES> has_edge_spline_{false, false, false, false, false, false};

    // Cached deformed prototile contour & ear-clipped triangles in prototile local space
    mutable std::vector<Vec2> cached_deformed_outline_;
    mutable std::vector<uint32_t> cached_deformed_triangles_;
    mutable int cached_outline_subdivisions_   = -1;
    mutable bool outline_dirty_                = true;

    void _rebuild_from_tactile(const double* p_override_params = nullptr) noexcept;
    void _rebuild_default_symmetry_handles() noexcept;
    void _decompose_aspect_transforms() noexcept;
    void _compute_quotient_adjacency() noexcept;
    void _ensure_deformed_outline(int p_subdivisions_per_edge) const;

public:
    IsohedralTiling2D() noexcept;
    explicit IsohedralTiling2D(WallpaperGroupId p_group, FundamentalShapePreset p_preset = FundamentalShapePreset::TACTILE_DEFAULT) noexcept;
    explicit IsohedralTiling2D(uint8_t p_tactile_ih_type) noexcept;
    ~IsohedralTiling2D() = default;

    // ========================================================================
    // Static Catalogue & Wallpaper <-> Tactile IH Lookup Helpers
    // ========================================================================

    /**
     * Returns true if `p_ih_type` (1..93) is one of the 81 valid Isohedral Tiling IDs in `csk::tiling_types`.
     */
    [[nodiscard]] static bool is_valid_ih_type(uint8_t p_ih_type) noexcept;

    /**
     * Returns the span of all 81 valid Isohedral Tiling IDs (`csk::tiling_types`).
     */
    [[nodiscard]] static std::span<const uint8_t> get_all_ih_types() noexcept;

    /**
     * Returns all Isohedral Tiling IDs (`IH01`..`IH93`) whose underlying crystallographic
     * symmetry group is `p_group`.
     */
    [[nodiscard]] static std::vector<uint8_t> get_ih_types_for_wallpaper_group(WallpaperGroupId p_group);

    /**
     * Maps any valid Tactile Isohedral Tiling ID (`1..93`) to its owning `WallpaperGroupId` (`P1`..`P6M`).
     */
    [[nodiscard]] static WallpaperGroupId map_ih_type_to_wallpaper_group(uint8_t p_ih_type) noexcept;

    /**
     * Returns the default legacy `FundamentalShapePreset` for `p_group` matching `old_unity_code/Topology/Topology.cs`.
     */
    [[nodiscard]] static constexpr FundamentalShapePreset get_default_legacy_shape_preset(WallpaperGroupId p_group) noexcept;

    /**
     * Generates the canonical primitive vertices for a legacy `FundamentalShapePreset`
     * (`Quad`, `Rhombus`, `RightTriangle`, `EquilateralTriangle`, `TallTriangle`, `FlatTriangle`, `Brick`).
     */
    [[nodiscard]] static std::vector<Vec2> get_legacy_primitive_vertices(
        FundamentalShapePreset p_preset,
        T p_scale = static_cast<T>(1.0)
    );

    // ========================================================================
    // Reset & Mode Configuration
    // ========================================================================

    void reset_from_wallpaper_group(
        WallpaperGroupId p_group,
        FundamentalShapePreset p_preset = FundamentalShapePreset::TACTILE_DEFAULT
    ) noexcept;

    bool reset_from_ih_type(uint8_t p_ih_type) noexcept;

    [[nodiscard]] constexpr TilingAuthoringMode get_authoring_mode() const noexcept { return authoring_mode_; }
    [[nodiscard]] constexpr WallpaperGroupId get_wallpaper_group() const noexcept { return wallpaper_group_; }
    [[nodiscard]] constexpr uint8_t get_tactile_ih_type() const noexcept { return tactile_ih_type_; }
    [[nodiscard]] constexpr FundamentalShapePreset get_shape_preset() const noexcept { return shape_preset_; }
    void set_shape_preset(FundamentalShapePreset p_preset) noexcept;

    // ========================================================================
    // Tactile Shape Parameters (0..6 Vertex Parameters)
    // ========================================================================

    [[nodiscard]] constexpr uint8_t get_parameter_count() const noexcept { return baked_.num_params; }
    [[nodiscard]] std::span<const T> get_parameters() const noexcept {
        return std::span<const T>(baked_.parameters.data(), baked_.num_params);
    }
    void set_parameters(std::span<const T> p_params) noexcept;
    void set_parameter(size_t p_index, T p_value) noexcept;
    [[nodiscard]] T get_parameter(size_t p_index) const noexcept;

    // ========================================================================
    // Lattice Transform & Explicit Symmetry Element Handles (`Topology.cs`)
    // ========================================================================

    void set_lattice_transform(
        const Vec2& p_scale,
        T p_rotation_rad = static_cast<T>(0.0),
        const Vec2& p_origin = Vec2(static_cast<T>(0.0))
    ) noexcept;

    [[nodiscard]] constexpr Vec2 get_lattice_scale() const noexcept { return lattice_scale_; }
    [[nodiscard]] constexpr T get_lattice_rotation_rad() const noexcept { return lattice_rotation_rad_; }
    [[nodiscard]] constexpr Vec2 get_lattice_origin() const noexcept { return lattice_origin_; }
    [[nodiscard]] constexpr const LatticeBasis2DPOD<T>& get_lattice_basis() const noexcept { return baked_.lattice_basis; }

    [[nodiscard]] constexpr const HandlesPOD& get_symmetry_handles() const noexcept { return symmetry_handles_; }
    void set_symmetry_handles(const HandlesPOD& p_handles) noexcept;

    // ========================================================================
    // Custom `Spline2D` Escher Edge Curves (`J`, `U`, `S`, `I`)
    // ========================================================================

    [[nodiscard]] constexpr uint8_t get_edge_shape_count() const noexcept { return baked_.num_edge_shapes; }
    [[nodiscard]] TactileEdgeShape get_edge_shape_constraint(uint8_t p_edge_shape_id) const noexcept;

    void set_edge_spline(uint8_t p_edge_shape_id, const Spline2& p_canonical_spline);
    [[nodiscard]] bool has_edge_spline(uint8_t p_edge_shape_id) const noexcept;
    [[nodiscard]] const Spline2* get_edge_spline(uint8_t p_edge_shape_id) const noexcept;
    void clear_edge_spline(uint8_t p_edge_shape_id) noexcept;
    void clear_all_edge_splines() noexcept;
    [[nodiscard]] bool has_any_custom_edge_spline() const noexcept;

    // ========================================================================
    // Prototile Geometry, Aspect Transforms & Quotient Adjacency Queries
    // ========================================================================

    [[nodiscard]] constexpr const BakedPOD& get_baked_prototile() const noexcept { return baked_; }
    [[nodiscard]] constexpr uint8_t get_vertex_count() const noexcept { return baked_.num_vertices; }
    [[nodiscard]] constexpr uint8_t get_aspect_count() const noexcept { return baked_.num_aspects; }

    [[nodiscard]] std::span<const Vec2> get_base_prototile_vertices() const noexcept {
        return std::span<const Vec2>(baked_.base_vertices.data(), baked_.num_vertices);
    }

    /**
     * Returns the prototile boundary polygon with all `J/U/S/I` `Spline2D` edge curves applied.
     */
    [[nodiscard]] std::span<const Vec2> get_deformed_prototile_outline(int p_subdivisions_per_edge = 12) const;

    /**
     * Returns the ear-clipped triangle indices for `get_deformed_prototile_outline`.
     */
    [[nodiscard]] std::span<const uint32_t> get_deformed_prototile_triangles(int p_subdivisions_per_edge = 12) const;

    /**
     * Computes the world-space 3x3 homogeneous affine matrix for tile instance `(p_t1, p_t2, p_aspect)`.
     */
    [[nodiscard]] Mat3 get_cell_world_matrix(int32_t p_t1, int32_t p_t2, uint8_t p_aspect) const noexcept;

    /**
     * Computes the `TileTransformPOD` (position, signed scale, rotation) for tile `(p_t1, p_t2, p_aspect)`.
     */
    [[nodiscard]] TileTransformPOD get_cell_transform_pod(
        int32_t p_t1,
        int32_t p_t2,
        uint8_t p_aspect,
        uint32_t p_cell_id = 0xFFFFFFFFu,
        uint16_t p_generation = 0u
    ) const noexcept;

    /**
     * Transforms the prototile vertices (either base corners or deformed spline outline) into
     * world space for tile instance `(p_t1, p_t2, p_aspect)`, ensuring CCW winding when `p_enforce_ccw` is true.
     */
    [[nodiscard]] std::vector<Vec2> get_cell_world_polygon(
        int32_t p_t1,
        int32_t p_t2,
        uint8_t p_aspect,
        bool p_use_deformed_edges = false,
        int p_subdivisions_per_edge = 12,
        bool p_enforce_ccw = true
    ) const;

    /**
     * Evaluates the Tactile 3-coloring index (0, 1, or 2) for `(p_t1, p_t2, p_aspect)` in O(1).
     */
    [[nodiscard]] uint8_t get_colour(int32_t p_t1, int32_t p_t2, uint8_t p_aspect) const noexcept;

    /**
     * O(1) Quotient-Graph neighbor step lookup across local edge `p_edge_slot` of `(p_t1, p_t2, p_aspect)`.
     */
    [[nodiscard]] bool step_to_neighbor(
        int32_t p_t1,
        int32_t p_t2,
        uint8_t p_aspect,
        uint8_t p_edge_slot,
        int32_t& r_next_t1,
        int32_t& r_next_t2,
        uint8_t& r_next_aspect,
        uint8_t* r_shared_edge_slot = nullptr
    ) const noexcept;

    /**
     * Locates the cell `(r_t1, r_t2, r_aspect)` containing `p_world_point` in O(1) lattice time
     * by checking the aspects in the $3 \times 3$ unit-cell neighborhood of `world_to_cell_coord`.
     */
    [[nodiscard]] bool locate_cell_at_world_point(
        const Vec2& p_world_point,
        int32_t& r_t1,
        int32_t& r_t2,
        uint8_t& r_aspect
    ) const noexcept;

    // ========================================================================
    // Analytical Region Enumeration & Multi-Modal `Spline2D` Fill
    // ========================================================================

    /**
     * Fills an axis-aligned 2D bounding box (`p_bounds`) analytically using lattice scanline
     * conversion (`csk::FillAlgorithm` + basis projection) with zero `WithinTolerance` searches.
     */
    [[nodiscard]] TilingRegionFillResult<T> fill_region_aabb(
        const AABB2D<T>& p_bounds,
        const Vec2& p_seed_point = Vec2(static_cast<T>(0.0))
    ) const;

    /**
     * Fills the interior of one or more closed `Spline2D` boundary contours using
     * `CSGMath2D::classify_polygon_against_boundary` and `p_coverage_mode`, partitioning
     * cells into `interior_cell_indices` (fast SIMD MultiMesh path) and `boundary_seam_cell_indices`
     * (exact CSG clip / seam stitch path).
     */
    [[nodiscard]] TilingRegionFillResult<T> fill_spline_region(
        std::span<const Spline2> p_region_splines,
        const Vec2& p_seed_point = Vec2(static_cast<T>(0.0)),
        TilingCoverageMode p_coverage_mode = TilingCoverageMode::INCLUDE_STRADDLING,
        int p_spline_subdivisions = 12
    ) const;
};

} // namespace ideam::core