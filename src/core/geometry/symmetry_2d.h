#pragma once

/**
 * symmetry_2d.h
 * 2D Affine, Dihedral (Dn), Bravais Lattice, Crystallographic Wallpaper Group (p1..p6m),
 * and Isohedral Tile Edge-Matching Symmetry Math (bridging src/external/tactile and Spline2D).
 *
 * Designed for Data-Oriented Design (TileDOD / GeometryDOD):
 *   - O(1) world <-> lattice (u, v) coordinate projections via pre-inverted 2x2 basis matrix.
 *   - Table-driven constexpr descriptor array for all 17 Euclidean Wallpaper Groups.
 *   - Symmetric curve edge deformation (J, U, S, I edge shapes) for Escher-style Spline2D tile borders.
 */

#include "geometry_common.h"
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace ideam::core {

/**
 * The 5 2D Bravais Lattice types underlying all 17 Euclidean Wallpaper Groups.
 */
enum class LatticeType2D : uint8_t {
    OBLIQUE     = 0, // Parallelogram cell: |t1| != |t2|, angle != 90 deg (p1, p2)
    RECTANGULAR = 1, // Orthogonal cell: |t1| != |t2|, angle == 90 deg (pm, pg, pmm, pmg, pgg)
    RHOMBIC     = 2, // Centered rectangular / diamond: |t1| == |t2|, angle != 90/60/120 (cm, cmm)
    SQUARE      = 3, // Orthogonal equal cell: |t1| == |t2|, angle == 90 deg (p4, p4m, p4g)
    HEXAGONAL   = 4  // 60/120 deg equal cell: |t1| == |t2|, angle == 60 or 120 deg (p3, p3m1, p31m, p6, p6m)
};

/**
 * The 17 Crystallographic Wallpaper Groups (IUCr standard notation).
 */
enum class WallpaperGroupId : uint8_t {
    P1   = 0,
    P2   = 1,
    PM   = 2,
    PG   = 3,
    CM   = 4,
    PMM  = 5,
    PMG  = 6,
    PGG  = 7,
    CMM  = 8,
    P4   = 9,
    P4M  = 10,
    P4G  = 11,
    P3   = 12,
    P3M1 = 13,
    P31M = 14,
    P6   = 15,
    P6M  = 16,
    COUNT = 17
};

/**
 * Isohedral tile edge symmetry shapes matching csk::EdgeShape in src/external/tactile/tiling.hpp.
 * Used when deforming fundamental domain edges with a Spline2D curve:
 *   - EDGE_J: Unconstrained free curve from vertex P to vertex Q.
 *   - EDGE_U: Mirror-symmetric across the perpendicular bisector of PQ (first half mirrored into second half).
 *   - EDGE_S: Centrosymmetric (180-deg rotational symmetry around midpoint of PQ).
 *   - EDGE_I: Rigid straight line segment from P to Q (cannot be deformed without breaking self-matching).
 */
enum class TactileEdgeShape : uint8_t {
    EDGE_J = 0,
    EDGE_U = 1,
    EDGE_S = 2,
    EDGE_I = 3
};

/**
 * WallpaperGroupDescriptorPOD
 * Compile-time table entry describing each of the 17 Wallpaper Groups and its
 * connection to the 81 Isohedral Tilings (IH01..IH93) in src/external/tactile.
 */
struct WallpaperGroupDescriptorPOD {
    WallpaperGroupId group_id       = WallpaperGroupId::P1;
    std::string_view iucr_symbol    = "p1";
    std::string_view orbifold_symbol= "o";
    LatticeType2D lattice_type      = LatticeType2D::OBLIQUE;
    uint8_t rotational_order        = 1u;  // 1, 2, 3, 4, or 6
    uint8_t num_aspects_per_cell    = 1u;  // Number of fundamental domains in translational unit cell (1..12)
    uint8_t fundamental_vertices    = 4u;  // Default fundamental domain polygon vertex count (3, 4, or 6)
    bool has_reflection             = false;
    bool has_glide_reflection       = false;
    uint8_t primary_tactile_ih_type = 1u;  // Representative Isohedral Tiling ID (1..93) in csk::tiling_types
};

/**
 * Canonical constexpr table of all 17 Euclidean Wallpaper Groups.
 */
inline constexpr std::array<WallpaperGroupDescriptorPOD, 17> kWallpaperGroupDescriptors = {{
    { WallpaperGroupId::P1,   "p1",   "o",    LatticeType2D::OBLIQUE,     1u, 1u,  4u, false, false, 1u  }, // IH01 / IH41
    { WallpaperGroupId::P2,   "p2",   "2222", LatticeType2D::OBLIQUE,     2u, 2u,  4u, false, false, 2u  }, // IH02 / IH47
    { WallpaperGroupId::PM,   "pm",   "**",   LatticeType2D::RECTANGULAR, 1u, 2u,  4u, true,  false, 42u }, // IH42
    { WallpaperGroupId::PG,   "pg",   "xx",   LatticeType2D::RECTANGULAR, 1u, 2u,  4u, false, true,  3u  }, // IH03 / IH43
    { WallpaperGroupId::CM,   "cm",   "*x",   LatticeType2D::RHOMBIC,     1u, 2u,  4u, true,  true,  44u }, // IH44
    { WallpaperGroupId::PMM,  "pmm",  "*2222",LatticeType2D::RECTANGULAR, 2u, 4u,  4u, true,  false, 50u }, // IH50
    { WallpaperGroupId::PMG,  "pmg",  "22*",  LatticeType2D::RECTANGULAR, 2u, 4u,  4u, true,  true,  5u  }, // IH05 / IH49
    { WallpaperGroupId::PGG,  "pgg",  "22x",  LatticeType2D::RECTANGULAR, 2u, 4u,  4u, false, true,  6u  }, // IH06 / IH51
    { WallpaperGroupId::CMM,  "cmm",  "2*22", LatticeType2D::RHOMBIC,     2u, 4u,  4u, true,  true,  52u }, // IH52
    { WallpaperGroupId::P4,   "p4",   "442",  LatticeType2D::SQUARE,      4u, 4u,  4u, false, false, 55u }, // IH55
    { WallpaperGroupId::P4M,  "p4m",  "*442", LatticeType2D::SQUARE,      4u, 8u,  3u, true,  true,  81u }, // IH81
    { WallpaperGroupId::P4G,  "p4g",  "4*2",  LatticeType2D::SQUARE,      4u, 8u,  4u, true,  true,  56u }, // IH56
    { WallpaperGroupId::P3,   "p3",   "333",  LatticeType2D::HEXAGONAL,   3u, 3u,  4u, false, false, 7u  }, // IH07 / IH33
    { WallpaperGroupId::P3M1, "p3m1", "*333", LatticeType2D::HEXAGONAL,   3u, 6u,  3u, true,  true,  38u }, // IH38
    { WallpaperGroupId::P31M, "p31m", "3*3",  LatticeType2D::HEXAGONAL,   3u, 6u,  4u, true,  true,  30u }, // IH30
    { WallpaperGroupId::P6,   "p6",   "632",  LatticeType2D::HEXAGONAL,   6u, 6u,  4u, false, false, 21u }, // IH21 / IH31
    { WallpaperGroupId::P6M,  "p6m",  "*632", LatticeType2D::HEXAGONAL,   6u, 12u, 3u, true,  true,  77u }  // IH77
}};

/**
 * LatticeBasis2DPOD<T>
 * Stores 2D translational basis vectors (t1, t2), origin offset, and the precomputed
 * inverse basis matrix B^-1 so DOD tasks can project world coordinates into integer
 * lattice cell coordinates (i, j) and fractional unit-cell UV in O(1) without division.
 */
template <FloatingPointScalar T = float>
struct alignas(16) LatticeBasis2DPOD {
    using Vec2 = glm::vec<2, T>;
    using Mat2 = glm::mat<2, 2, T>;

    Vec2 t1{static_cast<T>(1.0), static_cast<T>(0.0)};
    Vec2 t2{static_cast<T>(0.0), static_cast<T>(1.0)};
    Vec2 origin{static_cast<T>(0.0), static_cast<T>(0.0)};
    Mat2 inv_basis{static_cast<T>(1.0)}; // Columns of B^-1 where B = [t1 | t2]
};

template <FloatingPointScalar T = float>
struct SymmetryMath2D {
    using Vec2   = glm::vec<2, T>;
    using Vec2i  = glm::vec<2, int32_t>;
    using Mat2   = glm::mat<2, 2, T>;
    using Mat3   = glm::mat<3, 3, T>;
    using Mat3x2 = glm::mat<3, 2, T>;

    // ========================================================================
    // Wallpaper Group & Lattice Basis Construction
    // ========================================================================

    [[nodiscard]] static constexpr const WallpaperGroupDescriptorPOD& get_group_descriptor(WallpaperGroupId p_group) noexcept {
        const size_t idx = static_cast<size_t>(p_group);
        return (idx < kWallpaperGroupDescriptors.size())
            ? kWallpaperGroupDescriptors[idx]
            : kWallpaperGroupDescriptors[0];
    }

    /**
     * Builds a LatticeBasis2DPOD with precomputed inverse matrix B^-1 from basis vectors t1, t2.
     */
    [[nodiscard]] static LatticeBasis2DPOD<T> make_lattice_basis(
        const Vec2& p_t1,
        const Vec2& p_t2,
        const Vec2& p_origin = Vec2(static_cast<T>(0.0))
    ) noexcept;

    /**
     * Generates canonical unit-scale basis vectors (t1, t2) for any LatticeType2D.
     */
    [[nodiscard]] static LatticeBasis2DPOD<T> make_canonical_lattice(
        LatticeType2D p_type,
        T p_scale_u = static_cast<T>(1.0),
        T p_scale_v = static_cast<T>(1.0),
        T p_oblique_angle_rad = GeometryConstants<T>::PI / static_cast<T>(3.0),
        const Vec2& p_origin = Vec2(static_cast<T>(0.0))
    ) noexcept;

    /**
     * O(1) projection from 2D world space into continuous lattice coordinates (u, v).
     */
    [[nodiscard]] static constexpr Vec2 world_to_lattice(
        const LatticeBasis2DPOD<T>& p_basis,
        const Vec2& p_world_pt
    ) noexcept {
        return p_basis.inv_basis * (p_world_pt - p_basis.origin);
    }

    /**
     * O(1) reconstruction from continuous lattice coordinates (u, v) to 2D world space.
     */
    [[nodiscard]] static constexpr Vec2 lattice_to_world(
        const LatticeBasis2DPOD<T>& p_basis,
        const Vec2& p_lattice_uv
    ) noexcept {
        return p_basis.origin + (p_basis.t1 * p_lattice_uv.x) + (p_basis.t2 * p_lattice_uv.y);
    }

    /**
     * Decomposes a world point into integer unit-cell indices (cell_i, cell_j) and
     * normalized fractional cell coordinates frac_uv in [0, 1)^2.
     */
    static inline void world_to_cell_coord(
        const LatticeBasis2DPOD<T>& p_basis,
        const Vec2& p_world_pt,
        Vec2i& r_cell_ij,
        Vec2& r_frac_uv
    ) noexcept {
        const Vec2 uv = world_to_lattice(p_basis, p_world_pt);
        const T fl_u = std::floor(uv.x);
        const T fl_v = std::floor(uv.y);
        r_cell_ij = Vec2i(static_cast<int32_t>(fl_u), static_cast<int32_t>(fl_v));
        r_frac_uv = Vec2(uv.x - fl_u, uv.y - fl_v);
    }

    // ========================================================================
    // 2D Homogeneous Affine & Dihedral Symmetry Transforms (Mat3 compatible with Tactile)
    // ========================================================================

    [[nodiscard]] static constexpr Mat3 make_identity() noexcept {
        return Mat3(static_cast<T>(1.0));
    }

    [[nodiscard]] static constexpr Mat3 make_translation(const Vec2& p_delta) noexcept {
        Mat3 m(static_cast<T>(1.0));
        m[2][0] = p_delta.x;
        m[2][1] = p_delta.y;
        return m;
    }

    [[nodiscard]] static Mat3 make_rotation(T p_angle_rad, const Vec2& p_pivot = Vec2(static_cast<T>(0.0))) noexcept;

    /**
     * Builds a mirror reflection matrix across a line passing through p_line_origin
     * with unit direction vector p_line_unit_dir.
     */
    [[nodiscard]] static Mat3 make_reflection_across_line(
        const Vec2& p_line_origin,
        const Vec2& p_line_unit_dir
    ) noexcept;

    /**
     * Builds a glide-reflection matrix (mirror reflection across line + translation along line).
     */
    [[nodiscard]] static Mat3 make_glide_reflection(
        const Vec2& p_line_origin,
        const Vec2& p_line_unit_dir,
        T p_glide_distance
    ) noexcept;

    /**
     * Generates all 2*N (or N if p_include_reflections == false) symmetry matrices of
     * the Dihedral group D_n centered at p_pivot.
     */
    [[nodiscard]] static std::vector<Mat3> generate_dihedral_group(
        uint32_t p_n,
        const Vec2& p_pivot = Vec2(static_cast<T>(0.0)),
        bool p_include_reflections = true
    );

    /**
     * Applies a 3x3 2D affine matrix to a 2D point (homogeneous w = 1).
     */
    [[nodiscard]] static constexpr Vec2 transform_point(const Mat3& p_mat, const Vec2& p_pt) noexcept {
        return Vec2(
            p_mat[0][0] * p_pt.x + p_mat[1][0] * p_pt.y + p_mat[2][0],
            p_mat[0][1] * p_pt.x + p_mat[1][1] * p_pt.y + p_mat[2][1]
        );
    }

    /**
     * Applies a 3x3 2D affine matrix to a 2D direction/tangent vector (homogeneous w = 0).
     */
    [[nodiscard]] static constexpr Vec2 transform_direction(const Mat3& p_mat, const Vec2& p_dir) noexcept {
        return Vec2(
            p_mat[0][0] * p_dir.x + p_mat[1][0] * p_dir.y,
            p_mat[0][1] * p_dir.x + p_mat[1][1] * p_dir.y
        );
    }

    // ========================================================================
    // Tactile / Escher Tile Edge-Matching & Curve Deformation
    // ========================================================================

    /**
     * Builds the similarity transform mapping canonical unit segment [(0,0) -> (1,0)]
     * onto target world edge [p_p -> p_q], with optional reflection (p_flip) and 180-deg
     * reversal (p_rotate_180), matching csk::IsohedralTiling's `match(p, q) * M_orients`.
     */
    [[nodiscard]] static Mat3 make_edge_match_transform(
        const Vec2& p_p,
        const Vec2& p_q,
        bool p_flip = false,
        bool p_rotate_180 = false
    ) noexcept;

    /**
     * Maps a canonical curve (defined over x in [0, 1] from (0,0) to (1,0), e.g., sampled
     * from a Spline2DResource) onto a fundamental-domain tile edge [p_edge_start -> p_edge_end]
     * while enforcing the required TactileEdgeShape symmetry (J, U, S, or I).
     */
    [[nodiscard]] static std::vector<Vec2> evaluate_symmetric_tile_edge(
        std::span<const Vec2> p_canonical_curve,
        const Vec2& p_edge_start,
        const Vec2& p_edge_end,
        TactileEdgeShape p_shape = TactileEdgeShape::EDGE_J,
        bool p_reversed = false
    );

    // ========================================================================
    // [SPECULATIVE / FUTURE PHASE] Symmetry-Accelerated Pathfinding & Non-Euclidean Tilings
    // ========================================================================
    /*
     * 1. Quotient-Graph Adjacency Table for O(1) Symmetry-Accelerated A* & Flow-Fields:
     *    - Because a wallpaper tiling is periodic, every tile aspect k in [0, M-1] inside
     *      any unit cell (i, j) shares identical relative neighbor offsets:
     *        struct WallpaperNeighborStepPOD {
     *            int8_t delta_i = 0;
     *            int8_t delta_j = 0;
     *            uint8_t next_aspect = 0;
     *            uint8_t shared_edge_id = 0;
     *            float base_step_cost = 1.0f;
     *        };
     *    - A constexpr lookup table `kWallpaperAdjacency[17][12][6]` allows TileDOD pathfinding
     *      tasks to traverse 100,000+ non-square tiles with zero per-tile adjacency memory.
     *
     * 2. Continuous Metric Morphing (Curvilinear Wallpaper Lattice Seam):
     *    - Smoothly interpolates lattice basis B(n) = mix(B_A, B_B, w) along the normal distance n
     *      to a Spline2D seam curve so a p4m square grid warps continuously into a p6m hex grid:
     *        [[nodiscard]] static LatticeBasis2DPOD<T> blend_lattice_bases(
     *            const LatticeBasis2DPOD<T>& p_a, const LatticeBasis2DPOD<T>& p_b, T p_weight) noexcept;
     *
     * 3. Non-Euclidean Hyperbolic {p, q} Poincare Disk Tilings (SU(1,1) Mobius Matrices):
     *    - Replaces 2D affine Mat3 with complex SU(1,1) Mobius transformations
     *      z' = (a*z + b) / (conj(b)*z + conj(a)), |a|^2 - |b|^2 = 1:
     *        struct MobiusSU11POD {
     *            glm::vec<2, T> a{1, 0}; // Complex real/imag
     *            glm::vec<2, T> b{0, 0};
     *        };
     *        [[nodiscard]] static Vec2 apply_poincare_mobius(const MobiusSU11POD& p_m, const Vec2& p_z) noexcept;
     *        [[nodiscard]] static void compute_poincare_geodesic_arc(
     *            const Vec2& p_p, const Vec2& p_q, Vec2& r_circle_center, T& r_radius, bool& r_is_diameter) noexcept;
     */
};

static_assert(std::is_trivially_copyable_v<LatticeBasis2DPOD<float>> && std::is_standard_layout_v<LatticeBasis2DPOD<float>>);

} // namespace ideam::core