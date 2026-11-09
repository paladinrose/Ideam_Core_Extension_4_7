#pragma once

/**
 * geometry_common.h
 * Tier 0/1 Pure C++26 geometric concepts, mathematical constants, spatial primitives
 * (AABB2D/3D, Segment2D/3D, Ray2D/3D, Plane3D, Barycentric), shared triangle/basis/grid
 * kernels, and canonical POD buffer schemas for GeometryDOD.
 *
 * Designed with zero Godot Variant/Object dependencies so it can be consumed directly
 * by src/core/splines, src/core/tiles, src/core/tasks, src/external/tactile, and SIMD/DOD kernels.
 */

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <type_traits>

namespace ideam::core {

// ============================================================================
// C++26 Concepts & Numerical Traits
// ============================================================================

template <typename T>
concept FloatingPointScalar = std::floating_point<T>;

template <FloatingPointScalar T = float>
struct GeometryConstants {
    static constexpr T PI           = std::numbers::pi_v<T>;
    static constexpr T TAU          = static_cast<T>(2.0) * std::numbers::pi_v<T>;
    static constexpr T HALF_PI      = static_cast<T>(0.5) * std::numbers::pi_v<T>;
    static constexpr T INV_PI       = std::numbers::inv_pi_v<T>;
    static constexpr T INV_TAU      = static_cast<T>(0.5) * std::numbers::inv_pi_v<T>;
    static constexpr T DEG_TO_RAD   = std::numbers::pi_v<T> / static_cast<T>(180.0);
    static constexpr T RAD_TO_DEG   = static_cast<T>(180.0) / std::numbers::pi_v<T>;

    static constexpr T EPSILON      = static_cast<T>(1e-6);
    static constexpr T SQ_EPSILON   = static_cast<T>(1e-12);
    static constexpr T WELD_EPSILON = static_cast<T>(1e-4);
};

enum class WindingOrder : int8_t {
    CLOCKWISE         = -1,
    COLINEAR          = 0,
    COUNTER_CLOCKWISE = 1
};

enum class PointContainment : uint8_t {
    OUTSIDE     = 0,
    INSIDE      = 1,
    ON_BOUNDARY = 2
};

// ============================================================================
// Core Spatial Primitives (2D & 3D)
// ============================================================================

template <FloatingPointScalar T = float>
struct AABB2D {
    using Vec2 = glm::vec<2, T>;

    Vec2 min_bounds{std::numeric_limits<T>::max(), std::numeric_limits<T>::max()};
    Vec2 max_bounds{std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest()};

    constexpr AABB2D() noexcept = default;
    constexpr AABB2D(const Vec2& p_min, const Vec2& p_max) noexcept
        : min_bounds(p_min), max_bounds(p_max) {}

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return min_bounds.x <= max_bounds.x && min_bounds.y <= max_bounds.y;
    }

    [[nodiscard]] constexpr Vec2 get_size() const noexcept {
        return is_valid() ? (max_bounds - min_bounds) : Vec2(static_cast<T>(0));
    }

    [[nodiscard]] constexpr Vec2 get_center() const noexcept {
        return (min_bounds + max_bounds) * static_cast<T>(0.5);
    }

    [[nodiscard]] constexpr T get_area() const noexcept {
        if (!is_valid()) return static_cast<T>(0);
        const Vec2 sz = max_bounds - min_bounds;
        return sz.x * sz.y;
    }

    constexpr void expand_to(const Vec2& p_point) noexcept {
        min_bounds = glm::min(min_bounds, p_point);
        max_bounds = glm::max(max_bounds, p_point);
    }

    constexpr void merge_with(const AABB2D<T>& p_other) noexcept {
        if (!p_other.is_valid()) return;
        min_bounds = glm::min(min_bounds, p_other.min_bounds);
        max_bounds = glm::max(max_bounds, p_other.max_bounds);
    }

    [[nodiscard]] constexpr AABB2D<T> grown(T p_margin) const noexcept {
        const Vec2 pad(p_margin, p_margin);
        return AABB2D<T>(min_bounds - pad, max_bounds + pad);
    }

    [[nodiscard]] constexpr bool contains_point(const Vec2& p_pt) const noexcept {
        return p_pt.x >= min_bounds.x && p_pt.x <= max_bounds.x &&
               p_pt.y >= min_bounds.y && p_pt.y <= max_bounds.y;
    }

    [[nodiscard]] constexpr bool intersects(const AABB2D<T>& p_other) const noexcept {
        return (min_bounds.x <= p_other.max_bounds.x && max_bounds.x >= p_other.min_bounds.x) &&
               (min_bounds.y <= p_other.max_bounds.y && max_bounds.y >= p_other.min_bounds.y);
    }
};

template <FloatingPointScalar T = float>
struct AABB3D {
    using Vec3 = glm::vec<3, T>;

    Vec3 min_bounds{std::numeric_limits<T>::max(), std::numeric_limits<T>::max(), std::numeric_limits<T>::max()};
    Vec3 max_bounds{std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest()};

    constexpr AABB3D() noexcept = default;
    constexpr AABB3D(const Vec3& p_min, const Vec3& p_max) noexcept
        : min_bounds(p_min), max_bounds(p_max) {}

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return min_bounds.x <= max_bounds.x &&
               min_bounds.y <= max_bounds.y &&
               min_bounds.z <= max_bounds.z;
    }

    [[nodiscard]] constexpr Vec3 get_size() const noexcept {
        return is_valid() ? (max_bounds - min_bounds) : Vec3(static_cast<T>(0));
    }

    [[nodiscard]] constexpr Vec3 get_center() const noexcept {
        return (min_bounds + max_bounds) * static_cast<T>(0.5);
    }

    [[nodiscard]] inline T get_bounding_radius() const noexcept {
        if (!is_valid()) return static_cast<T>(0);
        return glm::length((max_bounds - min_bounds) * static_cast<T>(0.5));
    }

    constexpr void expand_to(const Vec3& p_point) noexcept {
        min_bounds = glm::min(min_bounds, p_point);
        max_bounds = glm::max(max_bounds, p_point);
    }

    constexpr void merge_with(const AABB3D<T>& p_other) noexcept {
        if (!p_other.is_valid()) return;
        min_bounds = glm::min(min_bounds, p_other.min_bounds);
        max_bounds = glm::max(max_bounds, p_other.max_bounds);
    }

    [[nodiscard]] constexpr AABB3D<T> grown(T p_margin) const noexcept {
        const Vec3 pad(p_margin, p_margin, p_margin);
        return AABB3D<T>(min_bounds - pad, max_bounds + pad);
    }

    [[nodiscard]] constexpr bool contains_point(const Vec3& p_pt) const noexcept {
        return p_pt.x >= min_bounds.x && p_pt.x <= max_bounds.x &&
               p_pt.y >= min_bounds.y && p_pt.y <= max_bounds.y &&
               p_pt.z >= min_bounds.z && p_pt.z <= max_bounds.z;
    }

    [[nodiscard]] constexpr bool intersects(const AABB3D<T>& p_other) const noexcept {
        return (min_bounds.x <= p_other.max_bounds.x && max_bounds.x >= p_other.min_bounds.x) &&
               (min_bounds.y <= p_other.max_bounds.y && max_bounds.y >= p_other.min_bounds.y) &&
               (min_bounds.z <= p_other.max_bounds.z && max_bounds.z >= p_other.min_bounds.z);
    }
};

template <FloatingPointScalar T = float>
struct Segment2D {
    glm::vec<2, T> a{static_cast<T>(0)};
    glm::vec<2, T> b{static_cast<T>(0)};

    [[nodiscard]] constexpr glm::vec<2, T> direction_unnormalized() const noexcept { return b - a; }
    [[nodiscard]] inline T length() const noexcept { return glm::distance(a, b); }
    [[nodiscard]] constexpr glm::vec<2, T> midpoint() const noexcept { return (a + b) * static_cast<T>(0.5); }
};

template <FloatingPointScalar T = float>
struct Segment3D {
    glm::vec<3, T> a{static_cast<T>(0)};
    glm::vec<3, T> b{static_cast<T>(0)};

    [[nodiscard]] constexpr glm::vec<3, T> direction_unnormalized() const noexcept { return b - a; }
    [[nodiscard]] inline T length() const noexcept { return glm::distance(a, b); }
    [[nodiscard]] constexpr glm::vec<3, T> midpoint() const noexcept { return (a + b) * static_cast<T>(0.5); }
};

template <FloatingPointScalar T = float>
struct Ray2D {
    glm::vec<2, T> origin{static_cast<T>(0)};
    glm::vec<2, T> direction{static_cast<T>(1), static_cast<T>(0)};

    [[nodiscard]] constexpr glm::vec<2, T> at(T p_t) const noexcept { return origin + direction * p_t; }
};

template <FloatingPointScalar T = float>
struct Ray3D {
    glm::vec<3, T> origin{static_cast<T>(0)};
    glm::vec<3, T> direction{static_cast<T>(0), static_cast<T>(0), static_cast<T>(-1)};

    [[nodiscard]] constexpr glm::vec<3, T> at(T p_t) const noexcept { return origin + direction * p_t; }
};

template <FloatingPointScalar T = float>
struct Plane3D {
    glm::vec<3, T> normal{static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)};
    T d{static_cast<T>(0)}; // Plane equation: dot(normal, p) - d = 0

    [[nodiscard]] constexpr T signed_distance(const glm::vec<3, T>& p_point) const noexcept {
        return glm::dot(normal, p_point) - d;
    }

    [[nodiscard]] constexpr glm::vec<3, T> project_point(const glm::vec<3, T>& p_point) const noexcept {
        return p_point - normal * signed_distance(p_point);
    }
};

template <FloatingPointScalar T = float>
struct Circle2D {
    glm::vec<2, T> center{static_cast<T>(0)};
    T radius{static_cast<T>(0)};

    [[nodiscard]] constexpr bool contains_point(const glm::vec<2, T>& p_pt) const noexcept {
        const glm::vec<2, T> d = p_pt - center;
        return glm::dot(d, d) <= (radius * radius);
    }
};

// ============================================================================
// Shared Stateless Geometric Kernels (Consolidated from Tasks & Splines)
// ============================================================================

/**
 * GeometryKernelMath
 * Consolidates fundamental vector, triangle, barycentric, orthonormal basis,
 * and spatial-grid quantization formulas previously duplicated across:
 *   - FaceCenterAndAreaTransformLogic
 *   - NormalRecalcTransformLogic
 *   - TangentRecalcTransformLogic
 *   - MeshSurfaceScatterTransformLogic
 *   - SpatialInclusionBridgeQueryLogic / SpatialProjectionBridgeQueryLogic
 */
template <FloatingPointScalar T = float>
struct GeometryKernelMath {
    using Vec2 = glm::vec<2, T>;
    using Vec3 = glm::vec<3, T>;
    using Vec4 = glm::vec<4, T>;

    /**
     * 2D perp-dot (2D cross product z-component): a.x * b.y - a.y * b.x.
     */
    [[nodiscard]] static constexpr T cross_2d(const Vec2& p_a, const Vec2& p_b) noexcept {
        return p_a.x * p_b.y - p_a.y * p_b.x;
    }

    /**
     * 2D counter-clockwise 90-degree perpendicular vector: (-v.y, v.x).
     */
    [[nodiscard]] static constexpr Vec2 perp_ccw(const Vec2& p_v) noexcept {
        return Vec2(-p_v.y, p_v.x);
    }

    /**
     * 3D Triangle centroid: (v0 + v1 + v2) / 3.
     */
    [[nodiscard]] static constexpr Vec3 triangle_centroid(const Vec3& p_v0, const Vec3& p_v1, const Vec3& p_v2) noexcept {
        constexpr T one_third = static_cast<T>(1.0 / 3.0);
        return (p_v0 + p_v1 + p_v2) * one_third;
    }

    /**
     * Unnormalized area-weighted 3D triangle normal: (v2 - v0) x (v1 - v0).
     * Magnitude is exactly 2 * triangle_area.
     */
    [[nodiscard]] static constexpr Vec3 triangle_area_weighted_normal(
        const Vec3& p_v0,
        const Vec3& p_v1,
        const Vec3& p_v2,
        bool p_invert_winding = false
    ) noexcept {
        const Vec3 e1 = p_v1 - p_v0;
        const Vec3 e2 = p_v2 - p_v0;
        const T sign  = p_invert_winding ? static_cast<T>(-1.0) : static_cast<T>(1.0);
        return glm::cross(e2, e1) * sign;
    }

    /**
     * Computes unit normal and surface area of a 3D triangle simultaneously.
     */
    static inline void triangle_normal_and_area(
        const Vec3& p_v0,
        const Vec3& p_v1,
        const Vec3& p_v2,
        Vec3& r_unit_normal,
        T& r_area,
        bool p_invert_winding = false
    ) noexcept {
        const Vec3 cross_vec = triangle_area_weighted_normal(p_v0, p_v1, p_v2, p_invert_winding);
        const T len_sq = glm::dot(cross_vec, cross_vec);
        if (len_sq > GeometryConstants<T>::SQ_EPSILON) {
            const T len = std::sqrt(len_sq);
            r_area = static_cast<T>(0.5) * len;
            r_unit_normal = cross_vec * (static_cast<T>(1.0) / len);
        } else {
            r_area = static_cast<T>(0.0);
            r_unit_normal = Vec3(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0));
        }
    }

    /**
     * Turk's square-root folding mapping two uniform random variables (r1, r2) in [0, 1]
     * to uniform barycentric coordinates (b0, b1, b2) on a triangle.
     */
    [[nodiscard]] static inline Vec3 sample_uniform_barycentric(T p_r1, T p_r2) noexcept {
        const T sqrt_r1 = std::sqrt(std::max(static_cast<T>(0.0), p_r1));
        const T b0 = static_cast<T>(1.0) - sqrt_r1;
        const T b1 = sqrt_r1 * (static_cast<T>(1.0) - p_r2);
        const T b2 = sqrt_r1 * p_r2;
        return Vec3(b0, b1, b2);
    }

    /**
     * Computes barycentric coordinates (u, v, w) of p_point with respect to 2D triangle (a, b, c).
     */
    [[nodiscard]] static constexpr Vec3 compute_barycentric_2d(
        const Vec2& p_a,
        const Vec2& p_b,
        const Vec2& p_c,
        const Vec2& p_point
    ) noexcept {
        const Vec2 v0 = p_b - p_a;
        const Vec2 v1 = p_c - p_a;
        const Vec2 v2 = p_point - p_a;
        const T den = v0.x * v1.y - v1.x * v0.y;
        if (std::abs(den) <= GeometryConstants<T>::SQ_EPSILON) {
            return Vec3(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0));
        }
        const T inv_den = static_cast<T>(1.0) / den;
        const T v = (v2.x * v1.y - v1.x * v2.y) * inv_den;
        const T w = (v0.x * v2.y - v2.x * v0.y) * inv_den;
        const T u = static_cast<T>(1.0) - v - w;
        return Vec3(u, v, w);
    }

    /**
     * Branchless orthonormal basis construction (Duff et al., Pixar 2017) from a unit normal.
     */
    static inline void build_orthonormal_basis(
        const Vec3& p_unit_normal,
        Vec3& r_tangent,
        Vec3& r_bitangent
    ) noexcept {
        const T sign = std::copysign(static_cast<T>(1.0), p_unit_normal.z);
        const T a = static_cast<T>(-1.0) / (sign + p_unit_normal.z);
        const T b = p_unit_normal.x * p_unit_normal.y * a;
        r_tangent   = Vec3(static_cast<T>(1.0) + sign * p_unit_normal.x * p_unit_normal.x * a, sign * b, -sign * p_unit_normal.x);
        r_bitangent = Vec3(b, sign + p_unit_normal.y * p_unit_normal.y * a, -p_unit_normal.y);
    }

    /**
     * Gram-Schmidt tangent orthogonalization against a unit normal with automatic
     * degenerate fallback (shared by TangentRecalcTransformLogic and ParallelTransport3D).
     */
    [[nodiscard]] static inline Vec4 orthogonalize_tangent(
        const Vec3& p_unit_normal,
        const Vec3& p_accum_tangent,
        const Vec3& p_accum_bitangent,
        bool p_invert_handedness = false
    ) noexcept {
        Vec3 ortho_t = p_accum_tangent - p_unit_normal * glm::dot(p_unit_normal, p_accum_tangent);
        const T len_sq = glm::dot(ortho_t, ortho_t);
        if (len_sq > GeometryConstants<T>::SQ_EPSILON) {
            ortho_t *= (static_cast<T>(1.0) / std::sqrt(len_sq));
        } else {
            Vec3 dummy_b;
            build_orthonormal_basis(p_unit_normal, ortho_t, dummy_b);
        }

        T w_sign = (glm::dot(glm::cross(p_unit_normal, p_accum_tangent), p_accum_bitangent) < static_cast<T>(0.0))
            ? static_cast<T>(-1.0)
            : static_cast<T>(1.0);
        if (p_invert_handedness) {
            w_sign = -w_sign;
        }
        return Vec4(ortho_t.x, ortho_t.y, ortho_t.z, w_sign);
    }

    /**
     * 2D World-to-Grid linear cell index helper (shared by SpatialInclusion/Projection bridges,
     * PolygonScanlineGridBuild, and CymaticMap). Returns -1 if out of bounds.
     */
    [[nodiscard]] static inline int64_t world_to_grid_index_2d(
        const Vec2& p_pos,
        const Vec2& p_origin,
        const Vec2& p_cell_size,
        uint32_t p_grid_width,
        uint32_t p_grid_height
    ) noexcept {
        if (std::abs(p_cell_size.x) <= GeometryConstants<T>::EPSILON ||
            std::abs(p_cell_size.y) <= GeometryConstants<T>::EPSILON) {
            return -1;
        }
        const int32_t x = static_cast<int32_t>(std::floor((p_pos.x - p_origin.x) / p_cell_size.x));
        const int32_t y = static_cast<int32_t>(std::floor((p_pos.y - p_origin.y) / p_cell_size.y));
        if (x < 0 || x >= static_cast<int32_t>(p_grid_width) ||
            y < 0 || y >= static_cast<int32_t>(p_grid_height)) {
            return -1;
        }
        return static_cast<int64_t>(x) + static_cast<int64_t>(y) * static_cast<int64_t>(p_grid_width);
    }

    /**
     * 3D World-to-Grid linear cell index helper. Returns -1 if out of bounds.
     */
    [[nodiscard]] static inline int64_t world_to_grid_index_3d(
        const Vec3& p_pos,
        const Vec3& p_origin,
        const Vec3& p_cell_size,
        uint32_t p_grid_width,
        uint32_t p_grid_height,
        uint32_t p_grid_depth
    ) noexcept {
        if (std::abs(p_cell_size.x) <= GeometryConstants<T>::EPSILON ||
            std::abs(p_cell_size.y) <= GeometryConstants<T>::EPSILON ||
            std::abs(p_cell_size.z) <= GeometryConstants<T>::EPSILON) {
            return -1;
        }
        const int32_t x = static_cast<int32_t>(std::floor((p_pos.x - p_origin.x) / p_cell_size.x));
        const int32_t y = static_cast<int32_t>(std::floor((p_pos.y - p_origin.y) / p_cell_size.y));
        const int32_t z = static_cast<int32_t>(std::floor((p_pos.z - p_origin.z) / p_cell_size.z));
        if (x < 0 || x >= static_cast<int32_t>(p_grid_width) ||
            y < 0 || y >= static_cast<int32_t>(p_grid_height) ||
            z < 0 || z >= static_cast<int32_t>(p_grid_depth)) {
            return -1;
        }
        const int64_t w = static_cast<int64_t>(p_grid_width);
        const int64_t h = static_cast<int64_t>(p_grid_height);
        return static_cast<int64_t>(x) + (static_cast<int64_t>(y) * w) + (static_cast<int64_t>(z) * w * h);
    }
};

// ============================================================================
// Canonical GeometryDOD POD Buffer Schemas (For MemoryManagerDOD Sharing)
// ============================================================================

/**
 * PolylineSlicePOD (32 Bytes)
 * Canonical CSR slice descriptor in GeometryDOD's PolylineBuffer.
 * Allows SplineDOD to flatten a Spline2D/3D in Wave 1 and TileDOD or CSG tasks
 * to consume the flattened vertices in Wave 2 with zero Variant/OOP marshaling.
 */
struct alignas(16) PolylineSlicePOD {
    uint32_t vertex_offset  = 0u;    // Start index in shared Vec2/Vec3 vertex buffer
    uint32_t vertex_count   = 0u;    // Number of vertices in this polyline contour
    float total_length      = 0.0f;  // Cached arc length of the flattened polyline
    uint8_t dimension       = 2u;    // 2 = 2D (Vec2), 3 = 3D (Vec3), 4 = 4D (Vec4)
    uint8_t is_closed       = 0u;    // 1 if closed loop / polygon ring
    uint8_t winding_order   = 1u;    // 1 = CCW (outer ring), 0 = CW (interior hole)
    uint8_t flags           = 0u;    // Dirty/clipping classification flags
    glm::vec2 bounds_min{0.0f, 0.0f};// 2D XY AABB min
    glm::vec2 bounds_max{0.0f, 0.0f};// 2D XY AABB max
};

/**
 * PolygonMeshSlicePOD (32 Bytes)
 * Canonical descriptor for triangulated 2D/3D polygon meshes in GeometryDOD's PolygonMeshBuffer.
 */
struct alignas(16) PolygonMeshSlicePOD {
    uint32_t vertex_offset  = 0u;    // Start index in vertex buffer
    uint32_t vertex_count   = 0u;    // Vertex count
    uint32_t index_offset   = 0u;    // Start index in uint32_t triangle index buffer
    uint32_t index_count    = 0u;    // Number of indices (3 * triangle_count)
    float signed_area       = 0.0f;  // Cached shoelace signed area
    glm::vec2 centroid{0.0f, 0.0f};  // Cached 2D centroid
    uint32_t material_or_group_id = 0u;
};

/**
 * PlanarHalfEdgePOD (24 Bytes)
 * Doubly-Connected Edge List (DCEL) / Half-Edge primitive for PlanarGraphCSRBuffer.
 * Enables O(1) twin/next/prev face traversal for Voronoi duals, CSG boolean overlays,
 * and stitched Wallpaper Group seam topology.
 */
struct PlanarHalfEdgePOD {
    uint32_t origin_vertex  = 0xFFFFFFFFu;
    uint32_t twin_edge      = 0xFFFFFFFFu;
    uint32_t next_edge      = 0xFFFFFFFFu;
    uint32_t prev_edge      = 0xFFFFFFFFu;
    uint32_t incident_face  = 0xFFFFFFFFu;
    uint32_t flags          = 0u; // Bit 0: hard constraint edge (Spline2D), Bit 1: wallpaper seam
};

/**
 * SpatialHashEntryPOD (8 Bytes)
 * Key-value pair for SpatialHashBuffer (Morton / spatial cell hash -> primitive index).
 */
struct SpatialHashEntryPOD {
    uint32_t cell_hash      = 0u;
    uint32_t primitive_id   = 0xFFFFFFFFu;
};

static_assert(std::is_trivially_copyable_v<PolylineSlicePOD> && std::is_standard_layout_v<PolylineSlicePOD>);
static_assert(sizeof(PolylineSlicePOD) == 32, "PolylineSlicePOD must be 32 bytes!");
static_assert(std::is_trivially_copyable_v<PolygonMeshSlicePOD> && std::is_standard_layout_v<PolygonMeshSlicePOD>);
static_assert(sizeof(PolygonMeshSlicePOD) == 32, "PolygonMeshSlicePOD must be 32 bytes!");
static_assert(std::is_trivially_copyable_v<PlanarHalfEdgePOD> && std::is_standard_layout_v<PlanarHalfEdgePOD>);
static_assert(std::is_trivially_copyable_v<SpatialHashEntryPOD> && std::is_standard_layout_v<SpatialHashEntryPOD>);

// ============================================================================
// [SPECULATIVE / FUTURE PHASE] Tiered GeometryDOD & Exact Predicates
// ============================================================================
/*
 * 1. Adaptive Exact Geometric Predicates (Shewchuk-style Expansion Arithmetic):
 *    - When Martínez-Rueda CSG or Constrained Delaunay encounters near-collinear
 *      spline subdivisions (det ~ 1e-12), add:
 *        [[nodiscard]] static int orient2d_exact(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c) noexcept;
 *        [[nodiscard]] static int incircle_exact(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c, const glm::dvec2& d) noexcept;
 *
 * 2. Flat BVH Node PODs (for 2D Segment / 3D Triangle Raycast & Proximity Queries):
 *    struct alignas(32) BVHNode2DPOD {
 *        glm::vec2 aabb_min;
 *        glm::vec2 aabb_max;
 *        uint32_t left_or_first_prim;
 *        uint32_t prim_count; // 0 = internal node, >0 = leaf node
 *        uint32_t rope_skip_node;
 *        uint32_t reserved;
 *    };
 *
 * 3. GeometryBufferSlot & GeometryDOD Manifest (src/core/tasks/domains/geometry/geometry_dod_common.h):
 *    - Mirroring NarrativeBufferSlot and NARRATIVE_BUFFER_SPECS, GeometryDOD will register:
 *        GEOM_POLYLINE_SLICE_POD, GEOM_POLYLINE_VERTEX_2D, GEOM_POLYLINE_VERTEX_3D,
 *        GEOM_POLYGON_MESH_SLICE_POD, GEOM_POLYGON_INDEX_U32,
 *        GEOM_PLANAR_HALF_EDGE_POD, GEOM_SPATIAL_HASH_ENTRY_POD, GEOM_SDF_GRID_FLOAT32.
 */

} // namespace ideam::core