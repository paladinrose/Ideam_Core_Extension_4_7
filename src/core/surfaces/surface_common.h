#pragma once

/**
 * surface_common.h
 * Core C++26 PODs, frame queries, quad-mesh buffers, and bridge/pole descriptors
 * for the `IdeamSurfaces` parametric surfacing engine (`src/core/surfaces/`).
 *
 * Zero Godot dependency — mirrors `geometry_common.h`, `parallel_transport_3d.h`,
 * and `character_common.h` (`QuadLoopVertexPod` / `QuadFacePod`) so downstream
 * `IdeamDesign` scaffolds and `IdeamCharacters` 4-layer anchored stacks can
 * consume surface frames and LOD-scalable quad meshes directly.
 */

#include "../geometry/geometry_common.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace ideam::core {

/**
 * Boundary edge selector for a parametric surface patch `(u, v) in [0, 1]^2`.
 */
enum class SurfaceBoundaryEdge : uint8_t {
    NORTH_V0 = 0, ///< v = 0, u in [0, 1]
    SOUTH_V1 = 1, ///< v = 1, u in [0, 1]
    WEST_U0  = 2, ///< u = 0, v in [0, 1]
    EAST_U1  = 3  ///< u = 1, v in [0, 1]
};

/**
 * SurfaceFrame3DPOD (64 Bytes, Cache-Line Aligned)
 * Orthonormal local surface frame at `(patch_id, u, v)`:
 *   - `position`: 3D point on the surface S(u, v)
 *   - `tangent_u`: Unit tangent along increasing `u` (normalized dS/du)
 *   - `tangent_v`: Orthonormalized bitangent along increasing `v`
 *   - `normal`: Unit outward surface normal `normalize(cross(tangent_u, tangent_v))`
 *
 * Powers non-destructive relative curvilinear anchoring (`sample_frame(patch_id, u, v)`)
 * for `IdeamCharacters` Topology, Features, Details, and `CharacterApparelResource`.
 */
template <FloatingPointScalar T = float>
struct alignas(16) SurfaceFrame3DPOD {
    using Vec3 = glm::vec<3, T>;

    Vec3 position{static_cast<T>(0)};
    T u = static_cast<T>(0);

    Vec3 tangent_u{static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)};
    T v = static_cast<T>(0);

    Vec3 tangent_v{static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)};
    uint32_t patch_id = 0u;

    Vec3 normal{static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)};
    T area_scale = static_cast<T>(1);

    /**
     * Displaces from the surface anchor along local `(d_normal, d_tangent_u, d_tangent_v)`.
     */
    [[nodiscard]] constexpr Vec3 offset_point(
        T p_d_normal,
        T p_d_tangent_u = static_cast<T>(0),
        T p_d_tangent_v = static_cast<T>(0)
    ) const noexcept {
        return position + (normal * p_d_normal) + (tangent_u * p_d_tangent_u) + (tangent_v * p_d_tangent_v);
    }
};

/**
 * SurfaceQuadFacePod (16 Bytes, alignas(16))
 * Counter-clockwise quad face indices `(v00, v10, v11, v01)` matching `QuadFacePod`.
 */
struct alignas(16) SurfaceQuadFacePod {
    std::array<uint32_t, 4> indices = {0u, 0u, 0u, 0u};

    constexpr SurfaceQuadFacePod() = default;
    constexpr SurfaceQuadFacePod(uint32_t p_i0, uint32_t p_i1, uint32_t p_i2, uint32_t p_i3) noexcept
        : indices{p_i0, p_i1, p_i2, p_i3} {}

    [[nodiscard]] constexpr uint32_t &operator[](size_t p_idx) noexcept { return indices[p_idx]; }
    [[nodiscard]] constexpr const uint32_t &operator[](size_t p_idx) const noexcept { return indices[p_idx]; }
};
static_assert(sizeof(SurfaceQuadFacePod) == 16, "SurfaceQuadFacePod must be 16 bytes.");

/**
 * SurfaceManifoldStats
 * Edge-valence diagnostics for verifying watertightness of a stitched quad mesh.
 */
struct SurfaceManifoldStats {
    uint32_t vertex_count       = 0u;
    uint32_t quad_count         = 0u;
    uint32_t triangle_count     = 0u;
    uint32_t boundary_edges     = 0u; ///< Edges shared by exactly 1 quad
    uint32_t manifold_edges     = 0u; ///< Edges shared by exactly 2 quads
    uint32_t non_manifold_edges = 0u; ///< Edges shared by > 2 quads

    [[nodiscard]] constexpr bool is_manifold() const noexcept {
        return non_manifold_edges == 0u;
    }
    [[nodiscard]] constexpr bool is_watertight() const noexcept {
        return quad_count > 0u && boundary_edges == 0u && non_manifold_edges == 0u;
    }
};

/**
 * QuadBridgeSpec
 * Connects one boundary edge of `patch_a` to one boundary edge of `patch_b` with a
 * smooth all-quad bridge strip of `bridge_rings` intermediate quad rows.
 */
struct QuadBridgeSpec {
    uint32_t patch_a = 0u;
    SurfaceBoundaryEdge edge_a = SurfaceBoundaryEdge::EAST_U1;
    uint32_t patch_b = 0u;
    SurfaceBoundaryEdge edge_b = SurfaceBoundaryEdge::WEST_U0;
    bool reverse_b = false;
    uint32_t bridge_rings = 2u;   ///< Number of quad rings spanning the bridge (>= 1)
    float tangent_strength = 0.35f; ///< Hermite tangent weighting along the bridge
};

/**
 * PoleEdgeRef
 * References one boundary edge participating in an N-sided star/pole stitch connector.
 */
struct PoleEdgeRef {
    uint32_t patch_id = 0u;
    SurfaceBoundaryEdge edge = SurfaceBoundaryEdge::NORTH_V0;
    bool reverse = false;
};

/**
 * PoleStitchSpec
 * N-sided all-quad star/pole connector for 3, 5, or N patch boundaries meeting at a
 * shared organic junction (e.g., shoulder/armpit, clavicle, or dome cap).
 */
struct PoleStitchSpec {
    std::vector<PoleEdgeRef> edges;
    float center_normal_bias = 0.0f; ///< Optional outward/inward crown offset at the pole center
};

/**
 * Resolves LOD-scaled subdivision count while guaranteeing even edge counts >= 2
 * so adjacent patches and quad bridges weld cleanly at every LOD level (`0..3`).
 *   - LOD 0 (highest detail): 100% of base subdivisions
 *   - LOD 1: 50%
 *   - LOD 2: 25%
 *   - LOD 3 (coarsest cage): 12.5% (clamped to >= 2)
 */
[[nodiscard]] inline uint32_t resolve_surface_lod_divs(uint32_t p_base_divs, uint8_t p_lod_level) noexcept {
    const uint32_t clamped_base = std::max<uint32_t>(2u, p_base_divs);
    const uint8_t shift = static_cast<uint8_t>(std::min<uint8_t>(p_lod_level, 3u));
    uint32_t scaled = clamped_base >> shift;
    if (scaled < 2u) {
        scaled = 2u;
    }
    // Keep even for clean quad-loop subdivision compatibility when base is even
    if ((clamped_base & 1u) == 0u && (scaled & 1u) != 0u && scaled > 2u) {
        scaled += 1u;
    }
    return scaled;
}

/**
 * Builds an orthonormal `SurfaceFrame3DPOD` from raw position and partial derivatives `(du, dv)`.
 * Handles degenerate poles (e.g. collapsed north/south caps) cleanly using Duff's branchless basis.
 */
template <FloatingPointScalar T = float>
[[nodiscard]] inline SurfaceFrame3DPOD<T> make_surface_frame_from_partials(
    const glm::vec<3, T> &p_pos,
    const glm::vec<3, T> &p_du,
    const glm::vec<3, T> &p_dv,
    T p_u,
    T p_v,
    uint32_t p_patch_id = 0u
) noexcept {
    using Vec3 = glm::vec<3, T>;
    constexpr T kEps = static_cast<T>(1e-6);

    SurfaceFrame3DPOD<T> frame{};
    frame.position = p_pos;
    frame.u = p_u;
    frame.v = p_v;
    frame.patch_id = p_patch_id;

    const Vec3 raw_n = glm::cross(p_du, p_dv);
    const T raw_n_len = glm::length(raw_n);
    frame.area_scale = raw_n_len;

    const T du_len = glm::length(p_du);
    const T dv_len = glm::length(p_dv);

    if (raw_n_len > kEps) {
        frame.normal = raw_n / raw_n_len;
        frame.tangent_u = (du_len > kEps) ? (p_du / du_len) : Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
        // Re-orthogonalize tangent_v so (tangent_u, normal, tangent_v) is strictly orthonormal
        Vec3 tv = glm::cross(frame.normal, frame.tangent_u);
        const T tv_len = glm::length(tv);
        frame.tangent_v = (tv_len > kEps) ? (tv / tv_len) : Vec3(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));
        if (glm::dot(frame.tangent_v, p_dv) < static_cast<T>(0)) {
            frame.tangent_v = -frame.tangent_v;
        }
    } else if (du_len > kEps) {
        frame.tangent_u = p_du / du_len;
        const Vec3 ref_up = (std::abs(frame.tangent_u.y) < static_cast<T>(0.9))
            ? Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0))
            : Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
        frame.tangent_v = glm::normalize(glm::cross(ref_up, frame.tangent_u));
        frame.normal = glm::normalize(glm::cross(frame.tangent_u, frame.tangent_v));
    } else if (dv_len > kEps) {
        frame.tangent_v = p_dv / dv_len;
        const Vec3 ref_up = (std::abs(frame.tangent_v.y) < static_cast<T>(0.9))
            ? Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0))
            : Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
        frame.tangent_u = glm::normalize(glm::cross(frame.tangent_v, ref_up));
        frame.normal = glm::normalize(glm::cross(frame.tangent_u, frame.tangent_v));
    } else {
        frame.tangent_u = Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
        frame.normal    = Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
        frame.tangent_v = Vec3(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));
    }

    return frame;
}

/**
 * SurfaceQuadMesh3D
 * Structure-of-Arrays (SoA) quad-mesh output ready for Godot `ArrayMesh` upload,
 * vertex welding, normal smoothing, and `QuadLoopSubdivisionAndLODTransformLogic` export.
 */
template <FloatingPointScalar T = float>
struct SurfaceQuadMesh3D {
    using Vec2 = glm::vec<2, T>;
    using Vec3 = glm::vec<3, T>;
    using Vec4 = glm::vec<4, T>;

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec4> tangents; ///< (tx, ty, tz, handedness_sign)
    std::vector<Vec2> uvs;
    std::vector<uint32_t> vertex_patch_ids;
    std::vector<uint32_t> vertex_ring_indices;
    std::vector<uint8_t>  vertex_min_lod_levels;

    std::vector<SurfaceQuadFacePod> quads;
    std::vector<uint32_t> quad_patch_ids;

    void clear() {
        positions.clear();
        normals.clear();
        tangents.clear();
        uvs.clear();
        vertex_patch_ids.clear();
        vertex_ring_indices.clear();
        vertex_min_lod_levels.clear();
        quads.clear();
        quad_patch_ids.clear();
    }

    /**
     * Appends another quad mesh into this buffer, offsetting all quad vertex indices.
     */
    void append(const SurfaceQuadMesh3D<T> &p_other) {
        const uint32_t base_v = static_cast<uint32_t>(positions.size());
        positions.insert(positions.end(), p_other.positions.begin(), p_other.positions.end());
        normals.insert(normals.end(), p_other.normals.begin(), p_other.normals.end());
        tangents.insert(tangents.end(), p_other.tangents.begin(), p_other.tangents.end());
        uvs.insert(uvs.end(), p_other.uvs.begin(), p_other.uvs.end());
        vertex_patch_ids.insert(vertex_patch_ids.end(), p_other.vertex_patch_ids.begin(), p_other.vertex_patch_ids.end());
        vertex_ring_indices.insert(vertex_ring_indices.end(), p_other.vertex_ring_indices.begin(), p_other.vertex_ring_indices.end());
        vertex_min_lod_levels.insert(vertex_min_lod_levels.end(), p_other.vertex_min_lod_levels.begin(), p_other.vertex_min_lod_levels.end());

        quads.reserve(quads.size() + p_other.quads.size());
        for (const auto &q : p_other.quads) {
            quads.emplace_back(q[0] + base_v, q[1] + base_v, q[2] + base_v, q[3] + base_v);
        }
        quad_patch_ids.insert(quad_patch_ids.end(), p_other.quad_patch_ids.begin(), p_other.quad_patch_ids.end());
    }

    /**
     * Triangulates every quad face `(v0, v1, v2, v3)` into two CCW triangles `(v0, v1, v2)`
     * and `(v0, v2, v3)`, skipping degenerate triangles.
     */
    [[nodiscard]] std::vector<uint32_t> build_triangle_indices() const {
        std::vector<uint32_t> tris;
        tris.reserve(quads.size() * 6u);
        for (const auto &q : quads) {
            const uint32_t i0 = q[0];
            const uint32_t i1 = q[1];
            const uint32_t i2 = q[2];
            const uint32_t i3 = q[3];
            if (i0 != i1 && i1 != i2 && i2 != i0) {
                tris.push_back(i0);
                tris.push_back(i1);
                tris.push_back(i2);
            }
            if (i0 != i2 && i2 != i3 && i3 != i0) {
                tris.push_back(i0);
                tris.push_back(i2);
                tris.push_back(i3);
            }
        }
        return tris;
    }

    /**
     * Recomputes smooth area-weighted vertex normals across all quad faces.
     */
    void recompute_smooth_normals() {
        const size_t vcount = positions.size();
        normals.assign(vcount, Vec3(static_cast<T>(0)));

        for (const auto &q : quads) {
            const uint32_t i0 = q[0];
            const uint32_t i1 = q[1];
            const uint32_t i2 = q[2];
            const uint32_t i3 = q[3];
            if (i0 >= vcount || i1 >= vcount || i2 >= vcount || i3 >= vcount) {
                continue;
            }
            const Vec3 n0 = glm::cross(positions[i1] - positions[i0], positions[i2] - positions[i0]);
            const Vec3 n1 = glm::cross(positions[i2] - positions[i0], positions[i3] - positions[i0]);
            const Vec3 face_n = n0 + n1;
            normals[i0] += face_n;
            normals[i1] += face_n;
            normals[i2] += face_n;
            normals[i3] += face_n;
        }

        for (size_t i = 0; i < vcount; ++i) {
            const T len = glm::length(normals[i]);
            if (len > static_cast<T>(1e-6)) {
                normals[i] /= len;
            } else {
                normals[i] = Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
            }
        }
    }
};

} // namespace ideam::core
