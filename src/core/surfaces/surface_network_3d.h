#pragma once

/**
 * surface_network_3d.h
 * Multi-patch watertight curve-network quad-mesh engine (`ideam::core::SurfaceNetwork3D<T>`).
 *
 * Combines `NurbsPatch3D<T>` and `CoonsPatch3D<T>` patches with explicit:
 *   - `QuadBridgeSpec`: smooth all-quad bridge strips connecting two patch boundary edges
 *     with Hermite tangent continuity and matched edge counts.
 *   - `PoleStitchSpec`: N-sided star/pole all-quad connectors (for 3, 5, or N boundaries
 *     meeting at a shoulder, armpit, clavicle, or dome junction) that subdivide each
 *     incident boundary edge so every emitted face is a valid quad (zero triangles).
 *   - Spatial-hash vertex welding (`weld_vertices`) and manifold edge verification
 *     (`compute_manifold_stats`).
 *   - Unified `sample_frame(patch_id, u, v)` surface coordinate queries across the network.
 */

#include "coons_patch_3d.h"
#include "nurbs_patch_3d.h"
#include "surface_common.h"
#include "../splines/spline_3d.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ideam::core {

enum class SurfacePatchKind : uint8_t {
    NURBS = 0,
    COONS = 1
};

template <FloatingPointScalar T = float>
struct SurfaceNetworkPatchEntry {
    uint32_t patch_id = 0u;
    std::string name;
    SurfacePatchKind kind = SurfacePatchKind::NURBS;
    NurbsPatch3D<T> nurbs;
    CoonsPatch3D<T> coons;
    uint32_t base_u_divs = 12u;
    uint32_t base_v_divs = 12u;
    bool visible = true;

    [[nodiscard]] bool is_valid() const noexcept {
        return (kind == SurfacePatchKind::NURBS) ? nurbs.is_valid() : coons.is_valid();
    }

    [[nodiscard]] SurfaceFrame3DPOD<T> sample_frame(T p_u, T p_v) const {
        return (kind == SurfacePatchKind::NURBS)
            ? nurbs.sample_frame(p_u, p_v, patch_id)
            : coons.sample_frame(p_u, p_v, patch_id);
    }

    [[nodiscard]] std::vector<SurfaceFrame3DPOD<T>> sample_boundary_edge(
        SurfaceBoundaryEdge p_edge,
        uint32_t p_divs,
        bool p_reverse = false
    ) const {
        return (kind == SurfacePatchKind::NURBS)
            ? nurbs.sample_boundary_edge(p_edge, p_divs, p_reverse, patch_id)
            : coons.sample_boundary_edge(p_edge, p_divs, p_reverse, patch_id);
    }

    [[nodiscard]] SurfaceQuadMesh3D<T> tessellate(uint8_t p_lod_level) const {
        return (kind == SurfacePatchKind::NURBS)
            ? nurbs.tessellate_quad_mesh(base_u_divs, base_v_divs, p_lod_level, patch_id)
            : coons.tessellate_quad_mesh(base_u_divs, base_v_divs, p_lod_level, patch_id);
    }
};

template <FloatingPointScalar T = float>
class SurfaceNetwork3D {
public:
    using Vec2  = glm::vec<2, T>;
    using Vec3  = glm::vec<3, T>;
    using Vec4  = glm::vec<4, T>;
    using Frame = SurfaceFrame3DPOD<T>;

private:
    std::vector<SurfaceNetworkPatchEntry<T>> patches_;
    std::vector<QuadBridgeSpec> bridges_;
    std::vector<PoleStitchSpec> poles_;

    [[nodiscard]] const SurfaceNetworkPatchEntry<T> *_find_patch(uint32_t p_patch_id) const noexcept;
    [[nodiscard]] uint32_t _edge_divs_for_patch(
        const SurfaceNetworkPatchEntry<T> &p_patch,
        SurfaceBoundaryEdge p_edge,
        uint8_t p_lod_level
    ) const noexcept;

    [[nodiscard]] SurfaceQuadMesh3D<T> _build_quad_bridge(
        const QuadBridgeSpec &p_spec,
        uint8_t p_lod_level,
        uint32_t p_bridge_patch_id
    ) const;

    [[nodiscard]] SurfaceQuadMesh3D<T> _build_pole_stitch(
        const PoleStitchSpec &p_spec,
        uint8_t p_lod_level,
        uint32_t p_pole_patch_id
    ) const;

public:
    SurfaceNetwork3D() = default;
    ~SurfaceNetwork3D() = default;

    void clear();

    uint32_t add_nurbs_patch(
        const NurbsPatch3D<T> &p_patch,
        std::string p_name = "",
        uint32_t p_base_u_divs = 12u,
        uint32_t p_base_v_divs = 12u
    );

    uint32_t add_coons_patch(
        const CoonsPatch3D<T> &p_patch,
        std::string p_name = "",
        uint32_t p_base_u_divs = 12u,
        uint32_t p_base_v_divs = 12u
    );

    void add_quad_bridge(const QuadBridgeSpec &p_spec);
    void add_pole_stitch(const PoleStitchSpec &p_spec);

    [[nodiscard]] size_t get_patch_count() const noexcept { return patches_.size(); }
    [[nodiscard]] size_t get_bridge_count() const noexcept { return bridges_.size(); }
    [[nodiscard]] size_t get_pole_count() const noexcept { return poles_.size(); }

    [[nodiscard]] const std::vector<SurfaceNetworkPatchEntry<T>> &get_patches() const noexcept { return patches_; }
    [[nodiscard]] const std::vector<QuadBridgeSpec> &get_bridges() const noexcept { return bridges_; }
    [[nodiscard]] const std::vector<PoleStitchSpec> &get_poles() const noexcept { return poles_; }

    /**
     * Unified surface coordinate query `sample_frame(patch_id, u, v)`.
     */
    [[nodiscard]] Frame sample_frame(uint32_t p_patch_id, T p_u, T p_v) const;

    /**
     * Builds a single welded, manifold quad mesh across all visible patches, quad bridges,
     * and N-sided pole stitches at the requested LOD level (`0..3`).
     */
    [[nodiscard]] SurfaceQuadMesh3D<T> build_watertight_quad_mesh(
        uint8_t p_lod_level = 0u,
        T p_weld_epsilon = static_cast<T>(1e-4),
        bool p_recompute_smooth_normals = true
    ) const;

    /**
     * Spatial-hash vertex welding for merging shared patch/bridge seams cleanly.
     */
    static void weld_vertices(SurfaceQuadMesh3D<T> &r_mesh, T p_epsilon = static_cast<T>(1e-4));

    /**
     * Computes manifold edge-valence diagnostics for any quad mesh.
     */
    [[nodiscard]] static SurfaceManifoldStats compute_manifold_stats(const SurfaceQuadMesh3D<T> &p_mesh);
};


/**
 * `PartSeamLoopSpec<T>` & `PartSeamContract<T>`:
 * Cross-part topological hole contract connecting two separable body-part meshes
 * (e.g., `Chest.ArmHole_L <-> LeftShoulder.ChestHole`) via a shared boundary spline
 * with a locked canonical ring vertex count and cross-part averaged boundary normals.
 */
template <FloatingPointScalar T = float>
struct PartSeamLoopSpec {
    uint32_t seam_id             = 0u;
    std::string seam_name;
    uint32_t part_a_id           = 0u;
    uint32_t part_b_id           = 0u;
    uint32_t canonical_ring_count = 12u; ///< Locked even ring vertex count shared by both mating holes
    Spline3D<T> shared_seam_spline;
    T start_t_offset             = static_cast<T>(0);
    bool reverse_on_part_b       = false;
};

template <FloatingPointScalar T = float>
struct PartSeamContract {
    using Vec3 = glm::vec<3, T>;

    /**
     * Samples the locked canonical boundary ring positions along `p_spec.shared_seam_spline`
     * so both adjacent part networks tessellate identical perimeter vertices.
     */
    [[nodiscard]] static std::vector<Vec3> sample_canonical_ring(
        const PartSeamLoopSpec<T> &p_spec,
        bool p_for_part_b = false
    ) {
        const uint32_t count = std::max<uint32_t>(4u, p_spec.canonical_ring_count);
        std::vector<Vec3> ring;
        ring.reserve(count);
        if (p_spec.shared_seam_spline.get_point_count() < 2u) {
            return ring;
        }
        for (uint32_t i = 0u; i < count; ++i) {
            T t = static_cast<T>(i) / static_cast<T>(count) + p_spec.start_t_offset;
            t = t - std::floor(t);
            if (p_for_part_b && p_spec.reverse_on_part_b) {
                t = static_cast<T>(1) - t;
                if (t >= static_cast<T>(1)) t = static_cast<T>(0);
            }
            ring.push_back(p_spec.shared_seam_spline.get_point(t, false));
        }
        return ring;
    }

    /**
     * Averages boundary vertex normals across two separable part meshes wherever vertices
     * coincide within `p_epsilon`, guaranteeing zero shading seam when assembled while
     * keeping the meshes cleanly separable at runtime (dismemberment / swappable limbs).
     * Returns the count of synchronized seam vertex pairs.
     */
    static uint32_t synchronize_seam_boundary_normals(
        SurfaceQuadMesh3D<T> &r_mesh_a,
        SurfaceQuadMesh3D<T> &r_mesh_b,
        T p_epsilon = static_cast<T>(2e-3)
    ) {
        const T eps2 = p_epsilon * p_epsilon;
        uint32_t synced = 0u;
        const size_t na = std::min(r_mesh_a.positions.size(), r_mesh_a.normals.size());
        const size_t nb = std::min(r_mesh_b.positions.size(), r_mesh_b.normals.size());
        for (size_t ia = 0; ia < na; ++ia) {
            const Vec3 &pa = r_mesh_a.positions[ia];
            for (size_t ib = 0; ib < nb; ++ib) {
                const Vec3 d = pa - r_mesh_b.positions[ib];
                if (glm::dot(d, d) <= eps2) {
                    const Vec3 avg = r_mesh_a.normals[ia] + r_mesh_b.normals[ib];
                    const T len = glm::length(avg);
                    if (len > static_cast<T>(1e-6)) {
                        const Vec3 n = avg / len;
                        r_mesh_a.normals[ia] = n;
                        r_mesh_b.normals[ib] = n;
                        ++synced;
                    }
                }
            }
        }
        return synced;
    }
};

} // namespace ideam::core
