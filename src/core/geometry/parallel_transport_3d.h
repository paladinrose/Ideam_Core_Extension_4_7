#pragma once

/**
 * parallel_transport_3d.h
 * 3D Curve Frame Evaluation (Frenet-Serret & Wang Double-Reflection Rotation-Minimizing
 * Bishop Frames), Closed-Loop Holonomy Correction, and 2D Profile Sweep/Extrusion Kernels.
 *
 * Eliminates classic Frenet-frame gimbal flips at inflection points when extruding
 * Spline3D / Spline4D geometry (where W modulates cross-section scale or banking roll)
 * and prepares contiguous frame buffers for SplineDOD & GeometryDOD.
 */

#include "geometry_common.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * CurveFrame3DPOD (64 Bytes, Cache-Line Aligned)
 * Orthonormal moving frame (T, N, B) + position, cumulative arc-length, cross-section
 * scale, and banking roll at a sample along a 3D or 4D spline.
 */
template <FloatingPointScalar T = float>
struct alignas(16) CurveFrame3DPOD {
    using Vec3 = glm::vec<3, T>;

    Vec3 position{static_cast<T>(0)};
    T arc_length = static_cast<T>(0);

    Vec3 tangent{static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)};
    T scale = static_cast<T>(1);

    Vec3 normal{static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)};
    T roll_rad = static_cast<T>(0);

    Vec3 binormal{static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)};
    T reserved = static_cast<T>(0);

    /**
     * Transforms a 2D profile vertex (x along normal, y along binormal) into 3D world space.
     */
    [[nodiscard]] constexpr Vec3 transform_profile_point(const glm::vec<2, T>& p_profile_pt) const noexcept {
        return position + (normal * (p_profile_pt.x * scale)) + (binormal * (p_profile_pt.y * scale));
    }
};

/**
 * ExtrudedMeshBuffers3D
 * Structure-of-Arrays (SoA) output of a 2D cross-section swept along a 3D frame sequence.
 * Ready for zero-copy upload to GeometryDOD or Godot's RenderingServer / ArrayMesh.
 */
template <FloatingPointScalar T = float>
struct ExtrudedMeshBuffers3D {
    std::vector<glm::vec<3, T>> positions;
    std::vector<glm::vec<3, T>> normals;
    std::vector<glm::vec<4, T>> tangents; // (tx, ty, tz, handedness_sign)
    std::vector<glm::vec<2, T>> uvs;
    std::vector<uint32_t> indices;
};

template <FloatingPointScalar T = float>
struct ParallelTransport3D {
    using Vec2  = glm::vec<2, T>;
    using Vec3  = glm::vec<3, T>;
    using Vec4  = glm::vec<4, T>;
    using Frame = CurveFrame3DPOD<T>;

    // ========================================================================
    // Single-Step & Sequence Rotation-Minimizing Frame (Bishop RMF) Evaluation
    // ========================================================================

    /**
     * Constructs an initial orthonormal frame at p_position with unit tangent p_tangent
     * and reference up-hint p_up_hint (falling back to Duff's branchless basis if collinear).
     */
    [[nodiscard]] static Frame make_initial_frame(
        const Vec3& p_position,
        const Vec3& p_tangent,
        const Vec3& p_up_hint = Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)),
        T p_scale = static_cast<T>(1),
        T p_roll_rad = static_cast<T>(0)
    ) noexcept;

    /**
     * Propagates a rotation-minimizing frame from p_prev to (p_next_pos, p_next_tangent)
     * using Wang et al.'s (2008) Double-Reflection method (4th-order accurate, branchless,
     * and immune to inflection-point flips).
     */
    [[nodiscard]] static Frame step_double_reflection_frame(
        const Frame& p_prev,
        const Vec3& p_next_pos,
        const Vec3& p_next_tangent,
        T p_next_scale = static_cast<T>(1),
        T p_next_roll_rad = static_cast<T>(0)
    ) noexcept;

    /**
     * Rotates the frame's (normal, binormal) plane around its tangent axis by p_angle_rad.
     */
    [[nodiscard]] static Frame apply_axial_roll(const Frame& p_frame, T p_angle_rad) noexcept;

    /**
     * Generates a complete sequence of twist-free Rotation-Minimizing Frames along a
     * polyline of 3D positions (automatically estimating central-difference tangents if
     * p_tangents is empty, and evenly distributing closing holonomy twist if p_closed_loop == true).
     */
    [[nodiscard]] static std::vector<Frame> compute_bishop_frames(
        std::span<const Vec3> p_positions,
        std::span<const Vec3> p_tangents = {},
        const Vec3& p_initial_up = Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)),
        bool p_closed_loop = false,
        std::span<const T> p_scales = {},
        std::span<const T> p_rolls_rad = {}
    );

    /**
     * Overload for Spline4D samples where XYZ is 3D position and W encodes either
     * cross-section scale (p_w_is_roll == false) or banking roll angle in radians (p_w_is_roll == true).
     */
    [[nodiscard]] static std::vector<Frame> compute_bishop_frames_4d(
        std::span<const Vec4> p_points_4d,
        const Vec3& p_initial_up = Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)),
        bool p_closed_loop = false,
        bool p_w_is_roll = false
    );

    // ========================================================================
    // 2D Cross-Section Sweep / Extrusion Kernel
    // ========================================================================

    /**
     * Sweeps a 2D cross-section profile (e.g., from Spline2D::get_smoothed_points)
     * along a sequence of 3D CurveFrame3DPOD frames, generating positions, smooth normals,
     * orthonormal tangents, UVs, and triangle indices.
     */
    [[nodiscard]] static ExtrudedMeshBuffers3D<T> extrude_profile_along_frames(
        std::span<const Frame> p_frames,
        std::span<const Vec2> p_profile_2d,
        bool p_closed_profile = true,
        bool p_closed_path = false
    );

    // ========================================================================
    // [SPECULATIVE / FUTURE PHASE] Curvilinear Lattice / Cage Deformation (FFD)
    // ========================================================================
    /*
     * 1. Curvilinear Ribbon / Tube Embedding (Deforming Wallpaper Tilings or 3D Meshes Along a Spline):
     *    - Given a 3D mesh or 2D Wallpaper Tiling in local (s, n, b) coordinates where
     *      s in [0, total_arc_length] is longitudinal distance and (n, b) are lateral offsets,
     *      maps each vertex smoothly into the interpolated CurveFrame3DPOD at s:
     *        static void deform_points_along_frames(
     *            std::span<const Frame> p_frames,
     *            std::span<const Vec3> p_local_snb_points,
     *            std::span<Vec3> r_out_world_points
     *        ) noexcept;
     *
     * 2. Surface-Constrained Darboux Frames (Curves on 3D Terrains / SDFs):
     *    - Replaces the free normal N with the underlying surface normal grad(SDF) and
     *      computes the geodesic binormal B = T x N for roads/rivers hugging 3D terrain:
     *        [[nodiscard]] static Frame compute_darboux_frame(
     *            const Vec3& p_pos, const Vec3& p_tangent, const Vec3& p_surface_normal, T p_scale = static_cast<T>(1)) noexcept;
     */
};

static_assert(std::is_trivially_copyable_v<CurveFrame3DPOD<float>> && std::is_standard_layout_v<CurveFrame3DPOD<float>>);
static_assert(sizeof(CurveFrame3DPOD<float>) == 64, "CurveFrame3DPOD<float> must fit exactly in one 64-byte cache line!");

} // namespace ideam::core