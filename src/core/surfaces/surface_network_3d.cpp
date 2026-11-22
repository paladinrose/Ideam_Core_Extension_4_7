#include "surface_network_3d.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace ideam::core {

namespace {

struct QuantizedPosKey {
    int64_t x = 0;
    int64_t y = 0;
    int64_t z = 0;

    bool operator==(const QuantizedPosKey &p_other) const noexcept {
        return x == p_other.x && y == p_other.y && z == p_other.z;
    }
};

struct QuantizedPosKeyHash {
    size_t operator()(const QuantizedPosKey &k) const noexcept {
        uint64_t h = static_cast<uint64_t>(k.x) * 73856093ULL;
        h ^= static_cast<uint64_t>(k.y) * 19349663ULL;
        h ^= static_cast<uint64_t>(k.z) * 83492791ULL;
        return static_cast<size_t>(h);
    }
};

inline uint64_t pack_undirected_edge(uint32_t a, uint32_t b) noexcept {
    const uint32_t lo = std::min(a, b);
    const uint32_t hi = std::max(a, b);
    return (static_cast<uint64_t>(lo) << 32u) | static_cast<uint64_t>(hi);
}

template <FloatingPointScalar T>
glm::vec<3, T> outward_edge_tangent(const SurfaceFrame3DPOD<T> &p_frame, SurfaceBoundaryEdge p_edge) noexcept {
    switch (p_edge) {
        case SurfaceBoundaryEdge::NORTH_V0: return -p_frame.tangent_v;
        case SurfaceBoundaryEdge::SOUTH_V1: return  p_frame.tangent_v;
        case SurfaceBoundaryEdge::WEST_U0:  return -p_frame.tangent_u;
        case SurfaceBoundaryEdge::EAST_U1:  return  p_frame.tangent_u;
    }
    return p_frame.tangent_u;
}

} // namespace

template <FloatingPointScalar T>
void SurfaceNetwork3D<T>::clear() {
    patches_.clear();
    bridges_.clear();
    poles_.clear();
}

template <FloatingPointScalar T>
uint32_t SurfaceNetwork3D<T>::add_nurbs_patch(
    const NurbsPatch3D<T> &p_patch,
    std::string p_name,
    uint32_t p_base_u_divs,
    uint32_t p_base_v_divs
) {
    const uint32_t id = static_cast<uint32_t>(patches_.size());
    SurfaceNetworkPatchEntry<T> entry{};
    entry.patch_id    = id;
    entry.name        = p_name.empty() ? ("NurbsPatch_" + std::to_string(id)) : std::move(p_name);
    entry.kind        = SurfacePatchKind::NURBS;
    entry.nurbs       = p_patch;
    entry.base_u_divs = std::max<uint32_t>(2u, p_base_u_divs);
    entry.base_v_divs = std::max<uint32_t>(2u, p_base_v_divs);
    patches_.push_back(std::move(entry));
    return id;
}

template <FloatingPointScalar T>
uint32_t SurfaceNetwork3D<T>::add_coons_patch(
    const CoonsPatch3D<T> &p_patch,
    std::string p_name,
    uint32_t p_base_u_divs,
    uint32_t p_base_v_divs
) {
    const uint32_t id = static_cast<uint32_t>(patches_.size());
    SurfaceNetworkPatchEntry<T> entry{};
    entry.patch_id    = id;
    entry.name        = p_name.empty() ? ("CoonsPatch_" + std::to_string(id)) : std::move(p_name);
    entry.kind        = SurfacePatchKind::COONS;
    entry.coons       = p_patch;
    entry.base_u_divs = std::max<uint32_t>(2u, p_base_u_divs);
    entry.base_v_divs = std::max<uint32_t>(2u, p_base_v_divs);
    patches_.push_back(std::move(entry));
    return id;
}

template <FloatingPointScalar T>
void SurfaceNetwork3D<T>::add_quad_bridge(const QuadBridgeSpec &p_spec) {
    bridges_.push_back(p_spec);
}

template <FloatingPointScalar T>
void SurfaceNetwork3D<T>::add_pole_stitch(const PoleStitchSpec &p_spec) {
    poles_.push_back(p_spec);
}

template <FloatingPointScalar T>
const SurfaceNetworkPatchEntry<T> *SurfaceNetwork3D<T>::_find_patch(uint32_t p_patch_id) const noexcept {
    if (p_patch_id < patches_.size() && patches_[p_patch_id].patch_id == p_patch_id) {
        return &patches_[p_patch_id];
    }
    for (const auto &p : patches_) {
        if (p.patch_id == p_patch_id) {
            return &p;
        }
    }
    return nullptr;
}

template <FloatingPointScalar T>
uint32_t SurfaceNetwork3D<T>::_edge_divs_for_patch(
    const SurfaceNetworkPatchEntry<T> &p_patch,
    SurfaceBoundaryEdge p_edge,
    uint8_t p_lod_level
) const noexcept {
    const bool is_u_edge = (p_edge == SurfaceBoundaryEdge::NORTH_V0 || p_edge == SurfaceBoundaryEdge::SOUTH_V1);
    return resolve_surface_lod_divs(is_u_edge ? p_patch.base_u_divs : p_patch.base_v_divs, p_lod_level);
}

template <FloatingPointScalar T>
typename SurfaceNetwork3D<T>::Frame SurfaceNetwork3D<T>::sample_frame(uint32_t p_patch_id, T p_u, T p_v) const {
    const auto *patch = _find_patch(p_patch_id);
    if (!patch || !patch->is_valid()) {
        return Frame{};
    }
    return patch->sample_frame(p_u, p_v);
}

template <FloatingPointScalar T>
SurfaceQuadMesh3D<T> SurfaceNetwork3D<T>::_build_quad_bridge(
    const QuadBridgeSpec &p_spec,
    uint8_t p_lod_level,
    uint32_t p_bridge_patch_id
) const {
    SurfaceQuadMesh3D<T> mesh;
    const auto *pa = _find_patch(p_spec.patch_a);
    const auto *pb = _find_patch(p_spec.patch_b);
    if (!pa || !pb || !pa->is_valid() || !pb->is_valid()) {
        return mesh;
    }

    const uint32_t divs_a = _edge_divs_for_patch(*pa, p_spec.edge_a, p_lod_level);
    const uint32_t divs_b = _edge_divs_for_patch(*pb, p_spec.edge_b, p_lod_level);
    const uint32_t edge_divs = std::min(divs_a, divs_b);
    const uint32_t rings = std::max<uint32_t>(1u, p_spec.bridge_rings);

    const std::vector<Frame> edge_a_frames = pa->sample_boundary_edge(p_spec.edge_a, edge_divs, false);
    const std::vector<Frame> edge_b_frames = pb->sample_boundary_edge(p_spec.edge_b, edge_divs, p_spec.reverse_b);

    const uint32_t cols = edge_divs + 1u;
    const uint32_t rows = rings + 1u;
    const size_t vcount = static_cast<size_t>(cols) * rows;

    mesh.positions.reserve(vcount);
    mesh.normals.reserve(vcount);
    mesh.tangents.reserve(vcount);
    mesh.uvs.reserve(vcount);
    mesh.vertex_patch_ids.reserve(vcount);
    mesh.vertex_ring_indices.reserve(vcount);
    mesh.vertex_min_lod_levels.reserve(vcount);

    const T strength = static_cast<T>(std::clamp(p_spec.tangent_strength, 0.0f, 1.5f));

    for (uint32_t j = 0u; j < rows; ++j) {
        const T s = static_cast<T>(j) / static_cast<T>(rings);
        const T s2 = s * s;
        const T s3 = s2 * s;
        const T h00 = static_cast<T>(2) * s3 - static_cast<T>(3) * s2 + static_cast<T>(1);
        const T h10 = s3 - static_cast<T>(2) * s2 + s;
        const T h01 = static_cast<T>(-2) * s3 + static_cast<T>(3) * s2;
        const T h11 = s3 - s2;

        for (uint32_t i = 0u; i < cols; ++i) {
            const Frame &fa = edge_a_frames[i];
            const Frame &fb = edge_b_frames[i];
            const T dist = glm::length(fb.position - fa.position);

            const Vec3 m0 = outward_edge_tangent(fa, p_spec.edge_a) * (dist * strength);
            const Vec3 m1 = -outward_edge_tangent(fb, p_spec.edge_b) * (dist * strength);

            const Vec3 pos = fa.position * h00 + m0 * h10 + fb.position * h01 + m1 * h11;
            Vec3 nrm = fa.normal * (static_cast<T>(1) - s) + fb.normal * s;
            const T nlen = glm::length(nrm);
            nrm = (nlen > static_cast<T>(1e-6)) ? (nrm / nlen) : Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));

            Vec3 tan = fa.tangent_u * (static_cast<T>(1) - s) + fb.tangent_u * s;
            const T tlen = glm::length(tan);
            tan = (tlen > static_cast<T>(1e-6)) ? (tan / tlen) : Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));

            mesh.positions.push_back(pos);
            mesh.normals.push_back(nrm);
            mesh.tangents.emplace_back(tan.x, tan.y, tan.z, static_cast<T>(1));
            mesh.uvs.emplace_back(static_cast<T>(i) / static_cast<T>(edge_divs), s);
            mesh.vertex_patch_ids.push_back(p_bridge_patch_id);
            mesh.vertex_ring_indices.push_back(j);
            mesh.vertex_min_lod_levels.push_back(p_lod_level);
        }
    }

    mesh.quads.reserve(static_cast<size_t>(edge_divs) * rings);
    mesh.quad_patch_ids.reserve(static_cast<size_t>(edge_divs) * rings);
    for (uint32_t j = 0u; j < rings; ++j) {
        for (uint32_t i = 0u; i < edge_divs; ++i) {
            const uint32_t v00 = j * cols + i;
            const uint32_t v10 = j * cols + (i + 1u);
            const uint32_t v11 = (j + 1u) * cols + (i + 1u);
            const uint32_t v01 = (j + 1u) * cols + i;
            mesh.quads.emplace_back(v00, v10, v11, v01);
            mesh.quad_patch_ids.push_back(p_bridge_patch_id);
        }
    }

    return mesh;
}

template <FloatingPointScalar T>
SurfaceQuadMesh3D<T> SurfaceNetwork3D<T>::_build_pole_stitch(
    const PoleStitchSpec &p_spec,
    uint8_t p_lod_level,
    uint32_t p_pole_patch_id
) const {
    SurfaceQuadMesh3D<T> mesh;
    if (p_spec.edges.size() < 3u) {
        return mesh;
    }

    // Collect closed boundary perimeter ring of frames around the N-sided hole
    std::vector<Frame> perimeter;
    for (const auto &eref : p_spec.edges) {
        const auto *patch = _find_patch(eref.patch_id);
        if (!patch || !patch->is_valid()) {
            return mesh;
        }
        const uint32_t divs = _edge_divs_for_patch(*patch, eref.edge, p_lod_level);
        const std::vector<Frame> sampled = patch->sample_boundary_edge(eref.edge, divs, eref.reverse);
        // Append all samples except the final endpoint (shared with next edge start)
        for (uint32_t i = 0u; i < divs; ++i) {
            perimeter.push_back(sampled[i]);
        }
    }

    const size_t n = perimeter.size();
    if (n < 4u) {
        return mesh;
    }

    // Ensure an even number of perimeter vertices so a Catmull-style pole star emits 100% pure quads
    // (each pair of consecutive perimeter edges forms one quad with the pole center!).
    Vec3 center_pos(static_cast<T>(0));
    Vec3 center_nrm(static_cast<T>(0));
    for (const auto &f : perimeter) {
        center_pos += f.position;
        center_nrm += f.normal;
    }
    center_pos /= static_cast<T>(n);
    const T cnlen = glm::length(center_nrm);
    center_nrm = (cnlen > static_cast<T>(1e-6)) ? (center_nrm / cnlen) : Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
    center_pos += center_nrm * static_cast<T>(p_spec.center_normal_bias);

    // Emit center vertex (index 0), then perimeter vertices (1 .. n)
    mesh.positions.reserve(n + 1u);
    mesh.normals.reserve(n + 1u);
    mesh.tangents.reserve(n + 1u);
    mesh.uvs.reserve(n + 1u);
    mesh.vertex_patch_ids.reserve(n + 1u);
    mesh.vertex_ring_indices.reserve(n + 1u);
    mesh.vertex_min_lod_levels.reserve(n + 1u);

    mesh.positions.push_back(center_pos);
    mesh.normals.push_back(center_nrm);
    mesh.tangents.emplace_back(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));
    mesh.uvs.emplace_back(static_cast<T>(0.5), static_cast<T>(0.5));
    mesh.vertex_patch_ids.push_back(p_pole_patch_id);
    mesh.vertex_ring_indices.push_back(0u);
    mesh.vertex_min_lod_levels.push_back(p_lod_level);

    for (size_t i = 0; i < n; ++i) {
        const auto &f = perimeter[i];
        mesh.positions.push_back(f.position);
        mesh.normals.push_back(f.normal);
        mesh.tangents.emplace_back(f.tangent_u.x, f.tangent_u.y, f.tangent_u.z, static_cast<T>(1));
        const T angle = static_cast<T>(i) / static_cast<T>(n) * static_cast<T>(6.283185307179586);
        mesh.uvs.emplace_back(static_cast<T>(0.5) + static_cast<T>(0.5) * std::cos(angle),
                              static_cast<T>(0.5) + static_cast<T>(0.5) * std::sin(angle));
        mesh.vertex_patch_ids.push_back(p_pole_patch_id);
        mesh.vertex_ring_indices.push_back(1u);
        mesh.vertex_min_lod_levels.push_back(p_lod_level);
    }

    // Group pairs of adjacent perimeter edges into pure quads (center, p[2k], p[2k+1], p[2k+2])
    const size_t num_quads = n / 2u;
    mesh.quads.reserve(num_quads);
    mesh.quad_patch_ids.reserve(num_quads);
    for (size_t k = 0; k < num_quads; ++k) {
        const uint32_t i0 = 0u;
        const uint32_t i1 = static_cast<uint32_t>(1u + (2u * k) % n);
        const uint32_t i2 = static_cast<uint32_t>(1u + (2u * k + 1u) % n);
        const uint32_t i3 = static_cast<uint32_t>(1u + (2u * k + 2u) % n);
        mesh.quads.emplace_back(i0, i1, i2, i3);
        mesh.quad_patch_ids.push_back(p_pole_patch_id);
    }

    return mesh;
}

template <FloatingPointScalar T>
SurfaceQuadMesh3D<T> SurfaceNetwork3D<T>::build_watertight_quad_mesh(
    uint8_t p_lod_level,
    T p_weld_epsilon,
    bool p_recompute_smooth_normals
) const {
    SurfaceQuadMesh3D<T> combined;

    for (const auto &entry : patches_) {
        if (!entry.visible || !entry.is_valid()) {
            continue;
        }
        combined.append(entry.tessellate(p_lod_level));
    }

    uint32_t aux_patch_id = static_cast<uint32_t>(patches_.size());
    for (const auto &bridge : bridges_) {
        combined.append(_build_quad_bridge(bridge, p_lod_level, aux_patch_id++));
    }
    for (const auto &pole : poles_) {
        combined.append(_build_pole_stitch(pole, p_lod_level, aux_patch_id++));
    }

    if (p_weld_epsilon > static_cast<T>(0)) {
        weld_vertices(combined, p_weld_epsilon);
    }
    if (p_recompute_smooth_normals && !combined.quads.empty()) {
        combined.recompute_smooth_normals();
    }

    return combined;
}

template <FloatingPointScalar T>
void SurfaceNetwork3D<T>::weld_vertices(SurfaceQuadMesh3D<T> &r_mesh, T p_epsilon) {
    const size_t n = r_mesh.positions.size();
    if (n == 0u || p_epsilon <= static_cast<T>(0)) {
        return;
    }

    const T inv_eps = static_cast<T>(1) / p_epsilon;
    std::unordered_map<QuantizedPosKey, uint32_t, QuantizedPosKeyHash> grid;
    grid.reserve(n);

    std::vector<uint32_t> remap(n, 0u);
    SurfaceQuadMesh3D<T> welded;
    welded.positions.reserve(n);
    welded.normals.reserve(n);
    welded.tangents.reserve(n);
    welded.uvs.reserve(n);
    welded.vertex_patch_ids.reserve(n);
    welded.vertex_ring_indices.reserve(n);
    welded.vertex_min_lod_levels.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        const Vec3 &p = r_mesh.positions[i];
        const QuantizedPosKey key{
            static_cast<int64_t>(std::llround(static_cast<double>(p.x * inv_eps))),
            static_cast<int64_t>(std::llround(static_cast<double>(p.y * inv_eps))),
            static_cast<int64_t>(std::llround(static_cast<double>(p.z * inv_eps)))
        };

        auto it = grid.find(key);
        if (it != grid.end()) {
            remap[i] = it->second;
            // Average normal across welded seam
            welded.normals[it->second] += r_mesh.normals[i];
        } else {
            const uint32_t new_idx = static_cast<uint32_t>(welded.positions.size());
            grid.emplace(key, new_idx);
            remap[i] = new_idx;
            welded.positions.push_back(p);
            welded.normals.push_back(r_mesh.normals[i]);
            welded.tangents.push_back(r_mesh.tangents[i]);
            welded.uvs.push_back(r_mesh.uvs[i]);
            welded.vertex_patch_ids.push_back(r_mesh.vertex_patch_ids[i]);
            welded.vertex_ring_indices.push_back(r_mesh.vertex_ring_indices[i]);
            welded.vertex_min_lod_levels.push_back(r_mesh.vertex_min_lod_levels[i]);
        }
    }

    for (auto &nrm : welded.normals) {
        const T len = glm::length(nrm);
        if (len > static_cast<T>(1e-6)) {
            nrm /= len;
        }
    }

    welded.quads.reserve(r_mesh.quads.size());
    welded.quad_patch_ids.reserve(r_mesh.quad_patch_ids.size());
    for (size_t qi = 0; qi < r_mesh.quads.size(); ++qi) {
        const auto &q = r_mesh.quads[qi];
        const uint32_t i0 = remap[q[0]];
        const uint32_t i1 = remap[q[1]];
        const uint32_t i2 = remap[q[2]];
        const uint32_t i3 = remap[q[3]];
        // Drop completely collapsed quads
        if ((i0 == i1 && i2 == i3) || (i0 == i3 && i1 == i2)) {
            continue;
        }
        welded.quads.emplace_back(i0, i1, i2, i3);
        if (qi < r_mesh.quad_patch_ids.size()) {
            welded.quad_patch_ids.push_back(r_mesh.quad_patch_ids[qi]);
        }
    }

    r_mesh = std::move(welded);
}

template <FloatingPointScalar T>
SurfaceManifoldStats SurfaceNetwork3D<T>::compute_manifold_stats(const SurfaceQuadMesh3D<T> &p_mesh) {
    SurfaceManifoldStats stats{};
    stats.vertex_count = static_cast<uint32_t>(p_mesh.positions.size());
    stats.quad_count   = static_cast<uint32_t>(p_mesh.quads.size());

    std::unordered_map<uint64_t, uint32_t> edge_counts;
    edge_counts.reserve(p_mesh.quads.size() * 4u);

    for (const auto &q : p_mesh.quads) {
        const uint32_t idx[4] = {q[0], q[1], q[2], q[3]};
        if (idx[0] != idx[1] && idx[1] != idx[2] && idx[2] != idx[0]) stats.triangle_count += 1u;
        if (idx[0] != idx[2] && idx[2] != idx[3] && idx[3] != idx[0]) stats.triangle_count += 1u;
        for (int e = 0; e < 4; ++e) {
            const uint32_t a = idx[e];
            const uint32_t b = idx[(e + 1) % 4];
            if (a == b) continue;
            edge_counts[pack_undirected_edge(a, b)] += 1u;
        }
    }

    for (const auto &kv : edge_counts) {
        if (kv.second == 1u) {
            stats.boundary_edges += 1u;
        } else if (kv.second == 2u) {
            stats.manifold_edges += 1u;
        } else {
            stats.non_manifold_edges += 1u;
        }
    }
    return stats;
}

template class SurfaceNetwork3D<float>;
template class SurfaceNetwork3D<double>;

} // namespace ideam::core
