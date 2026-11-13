#pragma once

/**
 * tensor_field_2d.h
 * Pure C++26 Data-Oriented 2D/3D Symmetric Tensor Math, Basis Field Evaluation,
 * Poincaré Singularity Detection, RK4 Hyperstreamline Tracing, and DCEL Planar
 * Parcel Region Extraction (`src/core/geometry/`).
 *
 * Mathematical Foundation (Demystifying Legacy `Tensor Fields/*.cs`):
 * -------------------------------------------------------------------
 * 1. Why a 2D Symmetric Traceless Tensor instead of a Vector?
 *    A standard 2D vector `v = (vx, vy)` has a head and a tail (`v != -v`), so blending
 *    two opposite directions cancels out to zero, and blending two orthogonal street grids
 *    averages to a 45-degree diagonal.
 *    A 2D Symmetric Tensor encodes an *undirected line element* (or orthogonal cross) using
 *    the double-angle representation:
 *      T = R * [  cos(2*theta)   sin(2*theta) ] + trace * [ 1  0 ] = [ trace + a      b     ]
 *              [  sin(2*theta)  -cos(2*theta) ]           [ 0  1 ]   [     b      trace - a ]
 *    where:
 *      - `a = R * cos(2*theta)` and `b = R * sin(2*theta)` are the deviatoric components.
 *      - Because `2*(theta + pi) = 2*theta + 2*pi`, rotating a line by 180 degrees (`pi`)
 *        produces the EXACT same `(a, b)` tensor! Head/tail ambiguity disappears completely.
 *      - Every tensor simultaneously defines TWO orthogonal principal axes (eigenvectors):
 *          * Major Eigenvector `e_major` at angle `theta = 0.5 * atan2(b, a)` (eigenvalue `trace + R`)
 *          * Minor Eigenvector `e_minor` at angle `theta + pi/2`              (eigenvalue `trace - R`)
 *      - Trig-Free Double-Angle Identity:
 *        Given any direction vector `d = (dx, dy)` with squared length `L2 = dx*dx + dy*dy > 0`:
 *          `cos(2*theta) = (dx*dx - dy*dy) / L2`
 *          `sin(2*theta) = (2 * dx * dy) / L2`
 *        This eliminates `atan2`, `cos`, and `sin` from basis field evaluation and eigenvector
 *        extraction!
 */

#include "geometry_common.h"
#include "polygon_math_2d.h"
#include "sdf_math.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

// ============================================================================
// Enumerations & Constants
// ============================================================================

/**
 * TensorBasisType
 * Unified enum replacing the legacy OOP `ITensorField` class hierarchy
 * (`GridlineTensorField`, `RadialTensorField`, `PolylineTensorField`, `HeightmapTensorField`).
 */
enum TensorBasisType : uint8_t {
    TENSOR_BASIS_GRIDLINE            = 0, // Uniform directional cross-field along `direction`
    TENSOR_BASIS_RADIAL              = 1, // Radial spokes (major) & concentric rings (minor) around `position`
    TENSOR_BASIS_VORTEX              = 2, // Logarithmic spiral / vortex rotated by `aux_param` radians
    TENSOR_BASIS_POLYLINE_ATTRACTOR  = 3, // Aligns major axis to closest segment of a `PolylineSlicePOD`
    TENSOR_BASIS_SDF_GRADIENT        = 4, // Aligns major axis to iso-contours of an `SDFGridHeaderPOD`
    TENSOR_BASIS_WEDGE_SINGULARITY   = 5, // Topological +1/2 Poincaré index singularity (`theta / 2`)
    TENSOR_BASIS_TRISECTOR_SINGULARITY = 6 // Topological -1/2 Poincaré index singularity (`-theta / 2`)
};

/**
 * TensorSingularityType
 * Topological classification of degenerate points where tensor anisotropy `R = sqrt(a^2 + b^2) -> 0`.
 */
enum TensorSingularityType : uint8_t {
    TENSOR_SINGULARITY_NONE      = 0,
    TENSOR_SINGULARITY_WEDGE     = 1, // Poincaré index +1/2 (1 self-seeding streamline)
    TENSOR_SINGULARITY_TRISECTOR = 2, // Poincaré index -1/2 (3 asymptote separatrices at 120 deg)
    TENSOR_SINGULARITY_NODE_CENTER = 3, // Poincaré index +1   (radial or concentric focus)
    TENSOR_SINGULARITY_SADDLE    = 4  // Poincaré index -1   (4 hyperbolic separatrices at 90 deg)
};

// ============================================================================
// Cache-Aligned POD Primitives (16B & 32B)
// ============================================================================

/**
 * SymmetricTensor2DPod (16 Bytes, 16-Byte Aligned)
 * Represents the 2x2 symmetric matrix:
 *   [ trace + a      b     ]
 *   [     b      trace - a ]
 * where `a = R * cos(2*theta)` and `b = R * sin(2*theta)`.
 */
struct alignas(16) SymmetricTensor2DPod {
    float a      = 0.0f; // Deviatoric cosine component: R * cos(2 * theta)
    float b      = 0.0f; // Deviatoric sine component:   R * sin(2 * theta)
    float trace  = 0.0f; // Isotropic mean eigenvalue:   0.5 * (lambda_major + lambda_minor)
    float weight = 1.0f; // Accumulated blending weight / confidence

    [[nodiscard]] inline float anisotropy_sq() const noexcept {
        return a * a + b * b;
    }

    [[nodiscard]] inline float anisotropy() const noexcept {
        return std::sqrt(a * a + b * b);
    }

    [[nodiscard]] constexpr bool is_degenerate(float p_eps = 1e-6f) const noexcept {
        return (a * a + b * b) <= (p_eps * p_eps);
    }

    /**
     * Constructs a tensor from polar anisotropy `p_r >= 0` and orientation `p_theta` (radians).
     */
    [[nodiscard]] static inline SymmetricTensor2DPod from_r_theta(
        float p_r,
        float p_theta,
        float p_trace = 0.0f,
        float p_weight = 1.0f
    ) noexcept {
        const float two_theta = 2.0f * p_theta;
        return SymmetricTensor2DPod{
            p_r * std::cos(two_theta),
            p_r * std::sin(two_theta),
            p_trace,
            p_weight
        };
    }

    /**
     * Trig-free construction from a 2D tangent vector `p_dir = (dx, dy)` using:
     *   cos(2*theta) = (dx^2 - dy^2) / (dx^2 + dy^2)
     *   sin(2*theta) = (2 * dx * dy) / (dx^2 + dy^2)
     */
    [[nodiscard]] static inline SymmetricTensor2DPod from_tangent(
        const glm::vec2 &p_dir,
        float p_r = 1.0f,
        float p_trace = 0.0f,
        float p_weight = 1.0f
    ) noexcept {
        const float l2 = glm::dot(p_dir, p_dir);
        if (l2 <= 1e-12f) {
            return SymmetricTensor2DPod{0.0f, 0.0f, p_trace, p_weight};
        }
        const float scale = p_r / l2;
        return SymmetricTensor2DPod{
            (p_dir.x * p_dir.x - p_dir.y * p_dir.y) * scale,
            (2.0f * p_dir.x * p_dir.y) * scale,
            p_trace,
            p_weight
        };
    }

    /**
     * Trig-free radial tensor from displacement `p_delta = pos - center`.
     * Major eigenvector aligns with concentric rings (perpendicular to `p_delta`),
     * Minor eigenvector aligns with radial spokes (parallel to `p_delta`), matching
     * legacy `RadialTensorField.cs` (`diffSquares = y^2 - x^2`, `xy2 = -2*x*y`).
     */
    [[nodiscard]] static inline SymmetricTensor2DPod from_radial_offset(
        const glm::vec2 &p_delta,
        float p_r = 1.0f,
        float p_trace = 0.0f,
        float p_weight = 1.0f
    ) noexcept {
        const float l2 = glm::dot(p_delta, p_delta);
        if (l2 <= 1e-12f) {
            return SymmetricTensor2DPod{0.0f, 0.0f, p_trace, p_weight};
        }
        const float scale = p_r / l2;
        return SymmetricTensor2DPod{
            (p_delta.y * p_delta.y - p_delta.x * p_delta.x) * scale,
            (-2.0f * p_delta.x * p_delta.y) * scale,
            p_trace,
            p_weight
        };
    }

    /**
     * Normalizes the deviatoric magnitude `R = sqrt(a^2 + b^2)` to 1.0 (if non-degenerate).
     */
    [[nodiscard]] inline SymmetricTensor2DPod normalized() const noexcept {
        const float r = anisotropy();
        if (r <= 1e-6f) {
            return SymmetricTensor2DPod{0.0f, 0.0f, trace, weight};
        }
        const float inv_r = 1.0f / r;
        return SymmetricTensor2DPod{a * inv_r, b * inv_r, trace, weight};
    }

    /**
     * Computes the major and minor eigenvalues (`lambda_major >= lambda_minor`).
     */
    inline void eigenvalues(float &r_major_val, float &r_minor_val) const noexcept {
        const float r = anisotropy();
        r_major_val   = trace + r;
        r_minor_val   = trace - r;
    }

    /**
     * Closed-form, trig-free, numerically stable unit eigenvector extraction.
     * Avoids catastrophic cancellation near `a = -R` by branching on the sign of `a`:
     *   - When `a >= 0`: `e_major = normalize(a + R, b)`
     *   - When `a <  0`: `e_major = normalize(b, R - a)`
     *   - `e_minor = (-e_major.y, e_major.x)` (orthogonal 90-degree CCW rotation).
     */
    inline void eigenvectors(glm::vec2 &r_major_unit, glm::vec2 &r_minor_unit) const noexcept {
        const float r2 = a * a + b * b;
        if (r2 <= 1e-12f) {
            r_major_unit = glm::vec2(1.0f, 0.0f);
            r_minor_unit = glm::vec2(0.0f, 1.0f);
            return;
        }
        const float r = std::sqrt(r2);
        glm::vec2 maj = (a >= 0.0f) ? glm::vec2(a + r, b) : glm::vec2(b, r - a);
        const float m_len = glm::length(maj);
        if (m_len > 1e-6f) {
            maj *= (1.0f / m_len);
        } else {
            maj = glm::vec2(1.0f, 0.0f);
        }
        r_major_unit = maj;
        r_minor_unit = glm::vec2(-maj.y, maj.x);
    }

    constexpr SymmetricTensor2DPod &operator+=(const SymmetricTensor2DPod &p_rhs) noexcept {
        a      += p_rhs.a;
        b      += p_rhs.b;
        trace  += p_rhs.trace;
        weight += p_rhs.weight;
        return *this;
    }

    [[nodiscard]] friend constexpr SymmetricTensor2DPod operator+(
        SymmetricTensor2DPod p_lhs,
        const SymmetricTensor2DPod &p_rhs
    ) noexcept {
        p_lhs += p_rhs;
        return p_lhs;
    }

    [[nodiscard]] friend constexpr SymmetricTensor2DPod operator*(
        float p_scalar,
        const SymmetricTensor2DPod &p_rhs
    ) noexcept {
        return SymmetricTensor2DPod{
            p_scalar * p_rhs.a,
            p_scalar * p_rhs.b,
            p_scalar * p_rhs.trace,
            p_scalar * p_rhs.weight
        };
    }

    [[nodiscard]] friend constexpr SymmetricTensor2DPod operator*(
        const SymmetricTensor2DPod &p_lhs,
        float p_scalar
    ) noexcept {
        return p_scalar * p_lhs;
    }
};

/**
 * SymmetricTensor3DPod (32 Bytes, 16-Byte Aligned)
 * 3D symmetric metric/stress/guidance tensor for 3D streamline & mesh deformation tasks:
 *   [ m00  m01  m02 ]
 *   [ m01  m11  m12 ]
 *   [ m02  m12  m22 ]
 */
struct alignas(16) SymmetricTensor3DPod {
    float m00        = 1.0f;
    float m11        = 1.0f;
    float m22        = 1.0f;
    float m01        = 0.0f;
    float m02        = 0.0f;
    float m12        = 0.0f;
    float anisotropy = 0.0f;
    uint32_t flags   = 0u;

    /**
     * Constructs a 3D rank-1 directional tensor `w * (d x d)` aligned with `p_dir`.
     */
    [[nodiscard]] static inline SymmetricTensor3DPod from_direction_3d(
        const glm::vec3 &p_dir,
        float p_weight = 1.0f
    ) noexcept {
        const float l2 = glm::dot(p_dir, p_dir);
        if (l2 <= 1e-12f) return SymmetricTensor3DPod{};
        const glm::vec3 u = p_dir * (1.0f / std::sqrt(l2));
        SymmetricTensor3DPod t{};
        t.m00        = p_weight * u.x * u.x;
        t.m11        = p_weight * u.y * u.y;
        t.m22        = p_weight * u.z * u.z;
        t.m01        = p_weight * u.x * u.y;
        t.m02        = p_weight * u.x * u.z;
        t.m12        = p_weight * u.y * u.z;
        t.anisotropy = p_weight;
        return t;
    }
};

/**
 * TensorBasisElementPod (32 Bytes, 16-Byte Aligned)
 * Flat, cache-aligned POD descriptor for any tensor field generator primitive.
 */
struct alignas(16) TensorBasisElementPod {
    glm::vec2 position{0.0f, 0.0f};      // 8B: Center / origin in 2D world space
    glm::vec2 direction{1.0f, 0.0f};     // 8B: Unit tangent or axis vector (for Gridline)
    float weight               = 1.0f;   // 4B: Multiplicative blend strength
    float decay                = 0.05f;  // 4B: Exponential distance decay `exp(-decay * d^2)` (0 = uniform)
    float aux_param            = 0.0f;   // 4B: Swirl angle (Vortex) or isotropic trace offset
    uint16_t source_slice_id   = 0xFFFFu;// 2B: Bound PolylineSlicePOD or SDFGridHeaderPOD slot
    uint8_t basis_type         = TENSOR_BASIS_GRIDLINE; // 1B: TensorBasisType
    uint8_t enabled            = 1u;     // 1B: 1 = active, 0 = muted
};

/**
 * TensorGridHeaderPod (32 Bytes, 16-Byte Aligned)
 * Descriptor for a 2D pre-sampled Tensor & Eigenvector Field grid in `GeometryDOD`.
 */
struct alignas(16) TensorGridHeaderPod {
    uint32_t cell_offset  = 0u;   // Start index in GEOM_TENSOR_CELL_POD / EIGEN_MAJOR / EIGEN_MINOR
    uint32_t width        = 64u;  // Grid width in cells
    uint32_t height       = 64u;  // Grid height in cells
    uint16_t basis_offset = 0u;   // Start index in GEOM_TENSOR_BASIS_POD
    uint16_t basis_count  = 0u;   // Number of basis elements contributing to this grid
    uint32_t sdf_grid_slot= 0xFFFFFFFFu; // Optional bound SDFGridHeaderPOD slot for terrain/coastline alignment
    glm::vec2 origin{-5.0f, -5.0f};      // World-space min corner
    glm::vec2 cell_size{0.15873f, 0.15873f}; // World-space step per cell
};

/**
 * TensorSingularityPod (16 Bytes, 16-Byte Aligned)
 * Detected topological singularity (Wedge, Trisector, Center/Node, Saddle) in a 2D tensor field.
 */
struct alignas(16) TensorSingularityPod {
    glm::vec2 position{0.0f, 0.0f}; // 8B: World-space location of the degenerate point
    float poincare_index      = 0.0f; // 4B: +0.5 (Wedge), -0.5 (Trisector), +1.0 (Center), -1.0 (Saddle)
    uint16_t cell_x           = 0u;   // 2B: Grid cell X coordinate
    uint8_t singularity_type  = TENSOR_SINGULARITY_NONE; // 1B: TensorSingularityType
    uint8_t flags             = 0u;   // 1B: Bit 0: seeded separatrices
};

/**
 * TensorStreamlineSeedPod (16 Bytes, 16-Byte Aligned)
 * Priority-queue seed record for hyperstreamline network tracing.
 */
struct alignas(16) TensorStreamlineSeedPod {
    glm::vec2 position{0.0f, 0.0f}; // 8B: World-space seed point
    float priority            = 1.0f; // 4B: Priority score (e.g. distance from coastline + 1/separation)
    uint8_t follow_minor      = 0u;   // 1B: 0 = trace Major eigenvector, 1 = trace Minor eigenvector
    uint8_t hierarchy_tier    = 0u;   // 1B: 0 = Highway/Arterial, 1 = Collector, 2 = Local Parcel Street
    uint8_t active            = 1u;   // 1B: 1 = unconsumed/valid, 0 = culled/consumed
    uint8_t reserved          = 0u;   // 1B: Alignment padding
};

static_assert(sizeof(SymmetricTensor2DPod) == 16, "SymmetricTensor2DPod must be 16 bytes!");
static_assert(sizeof(SymmetricTensor3DPod) == 32, "SymmetricTensor3DPod must be 32 bytes!");
static_assert(sizeof(TensorBasisElementPod) == 32, "TensorBasisElementPod must be 32 bytes!");
static_assert(sizeof(TensorGridHeaderPod) == 32, "TensorGridHeaderPod must be 32 bytes!");
static_assert(sizeof(TensorSingularityPod) == 16, "TensorSingularityPod must be 16 bytes!");
static_assert(sizeof(TensorStreamlineSeedPod) == 16, "TensorStreamlineSeedPod must be 16 bytes!");

static_assert(std::is_trivially_copyable_v<SymmetricTensor2DPod> && std::is_standard_layout_v<SymmetricTensor2DPod>);
static_assert(std::is_trivially_copyable_v<SymmetricTensor3DPod> && std::is_standard_layout_v<SymmetricTensor3DPod>);
static_assert(std::is_trivially_copyable_v<TensorBasisElementPod> && std::is_standard_layout_v<TensorBasisElementPod>);
static_assert(std::is_trivially_copyable_v<TensorGridHeaderPod> && std::is_standard_layout_v<TensorGridHeaderPod>);
static_assert(std::is_trivially_copyable_v<TensorSingularityPod> && std::is_standard_layout_v<TensorSingularityPod>);
static_assert(std::is_trivially_copyable_v<TensorStreamlineSeedPod> && std::is_standard_layout_v<TensorStreamlineSeedPod>);

// ============================================================================
// Streamline & Planar Network Tracing Configuration / Output
// ============================================================================

struct TensorTracingConfig2D {
    AABB2D<float> world_bounds{{-5.0f, -5.0f}, {5.0f, 5.0f}};
    float major_separation        = 1.25f;   // Minimum spacing between major streamlines
    float minor_separation        = 1.25f;   // Minimum spacing between minor cross-streamlines
    float step_size               = 0.20f;   // RK4 integration step length
    float merge_distance          = 0.35f;   // Snap/weld radius to existing intersections
    float cos_search_cone_angle   = 0.92388f;// cos(22.5 deg) forward vertex snap cone
    float collinear_cos_threshold = 0.999f;  // Dot-product threshold for polyline vertex reduction
    uint32_t max_steps_per_stream = 256u;
    uint32_t max_streamlines      = 256u;
    bool trace_boundary_loop      = true;
    bool extract_closed_regions   = true;
};

struct TensorNetworkResult2D {
    std::vector<std::vector<glm::vec2>> streamlines;
    std::vector<uint8_t> streamline_is_minor;
    std::vector<glm::vec2> graph_vertices;
    std::vector<PlanarHalfEdgePOD> half_edges;
    std::vector<std::vector<glm::vec2>> closed_regions;
    std::vector<TensorSingularityPod> singularities;
};

// ============================================================================
// Stateless DOD Tensor Field Kernels (`TensorFieldMath2D`)
// ============================================================================

struct TensorFieldMath2D {
    /**
     * Exponential Gaussian distance decay `exp(-decay * dist_sq)`.
     * Matches `PointDistanceDecayTensorField::DistanceDecay` in legacy C#.
     */
    [[nodiscard]] static inline float evaluate_distance_decay(float p_dist_sq, float p_decay) noexcept {
        if (p_decay <= 0.0f) return 1.0f;
        return std::exp(-p_decay * p_dist_sq);
    }

    /**
     * Evaluates a single `TensorBasisElementPod` at `p_world_pos`, optionally consulting
     * shared `PolylineSlicePOD` contours or `SDFGridHeaderPOD` scalar fields.
     */
    [[nodiscard]] static SymmetricTensor2DPod evaluate_single_basis(
        const glm::vec2 &p_world_pos,
        const TensorBasisElementPod &p_basis,
        std::span<const PolylineSlicePOD> p_polylines = {},
        std::span<const glm::vec2> p_polyline_verts = {},
        const SDFGridHeaderPOD *p_sdf_header = nullptr,
        std::span<const float> p_sdf_cells = {}
    ) noexcept;

    /**
     * Evaluates the composite `SymmetricTensor2DPod` at `p_world_pos` across a span of
     * `TensorBasisElementPod` generators in a single branchless/flat pass.
     */
    [[nodiscard]] static SymmetricTensor2DPod sample_tensor_field(
        const glm::vec2 &p_world_pos,
        std::span<const TensorBasisElementPod> p_bases,
        std::span<const PolylineSlicePOD> p_polylines = {},
        std::span<const glm::vec2> p_polyline_verts = {},
        const SDFGridHeaderPOD *p_sdf_header = nullptr,
        std::span<const float> p_sdf_cells = {}
    ) noexcept;

    /**
     * Bakes a 2D grid of `SymmetricTensor2DPod` plus unit `major_eigenvectors` and
     * `minor_eigenvectors` over `p_header`.
     * Replaces `ResampleAndRescaleEigenField.Create` with zero heap allocations.
     */
    static void evaluate_tensor_grid(
        const TensorGridHeaderPod &p_header,
        std::span<const TensorBasisElementPod> p_bases,
        std::span<SymmetricTensor2DPod> r_out_tensors,
        std::span<glm::vec2> r_out_major_eigen,
        std::span<glm::vec2> r_out_minor_eigen,
        std::span<const PolylineSlicePOD> p_polylines = {},
        std::span<const glm::vec2> p_polyline_verts = {},
        const SDFGridHeaderPOD *p_sdf_header = nullptr,
        std::span<const float> p_sdf_cells = {}
    ) noexcept;

    /**
     * Bilinearly samples the pre-computed `SymmetricTensor2DPod` from a grid and extracts
     * the sign-consistent major or minor eigenvector aligned with `p_prev_dir`.
     */
    [[nodiscard]] static glm::vec2 sample_grid_eigenvector_aligned(
        const glm::vec2 &p_world_pos,
        const glm::vec2 &p_prev_dir,
        bool p_follow_minor,
        const TensorGridHeaderPod &p_header,
        std::span<const SymmetricTensor2DPod> p_grid_tensors
    ) noexcept;

    /**
     * Performs a single 4th-Order Runge-Kutta (RK4) step along the major or minor
     * eigenvector field, resolving the 180-degree sign ambiguity at each sub-step
     * via `dot(v, prev_dir) < 0 ? -v : v`.
     */
    [[nodiscard]] static glm::vec2 rk4_step_eigenvector_field(
        const glm::vec2 &p_pos,
        const glm::vec2 &p_prev_dir,
        float p_step_size,
        bool p_follow_minor,
        const TensorGridHeaderPod &p_header,
        std::span<const SymmetricTensor2DPod> p_grid_tensors,
        std::span<const TensorBasisElementPod> p_fallback_bases = {}
    ) noexcept;

    /**
     * Scans a baked 2D tensor grid for topological singularities (Wedges +1/2, Trisectors -1/2,
     * Centers +1, Saddles -1) by integrating the principal orientation jump around each 2x2 cell.
     */
    static uint32_t detect_grid_singularities(
        const TensorGridHeaderPod &p_header,
        std::span<const SymmetricTensor2DPod> p_grid_tensors,
        std::span<TensorSingularityPod> r_out_singularities
    ) noexcept;

    /**
     * Traces a single bidirectional hyperstreamline starting from `p_seed_pos` along either
     * the major (`p_follow_minor = false`) or minor (`p_follow_minor = true`) eigenvector field.
     */
    static void trace_single_hyperstreamline(
        const glm::vec2 &p_seed_pos,
        bool p_follow_minor,
        float p_step_size,
        uint32_t p_max_steps,
        const AABB2D<float> &p_bounds,
        const TensorGridHeaderPod &p_header,
        std::span<const SymmetricTensor2DPod> p_grid_tensors,
        std::span<const TensorBasisElementPod> p_fallback_bases,
        std::vector<glm::vec2> &r_out_points
    );

    /**
     * Traces a complete orthogonal Major/Minor Hyperstreamline Road/Parcel Network, performs
     * segment-segment intersection splitting, vertex welding, collinear polyline reduction,
     * and Planar Half-Edge (DCEL) closed parcel region extraction (`TracedRegionBuilder`).
     */
    static TensorNetworkResult2D build_hyperstreamline_network(
        const TensorGridHeaderPod &p_header,
        std::span<const SymmetricTensor2DPod> p_grid_tensors,
        std::span<const TensorBasisElementPod> p_fallback_bases,
        std::span<const TensorStreamlineSeedPod> p_custom_seeds,
        const TensorTracingConfig2D &p_config = {}
    );
};

} // namespace ideam::core
