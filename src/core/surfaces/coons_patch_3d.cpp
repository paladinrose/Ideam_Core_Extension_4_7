#include "coons_patch_3d.h"

#include <algorithm>
#include <cmath>

namespace ideam::core {

template <FloatingPointScalar T>
typename CoonsPatch3D<T>::Vec3 CoonsPatch3D<T>::_eval_curve(const Spline3D<T> &p_curve, T p_t, bool p_reverse) const {
    if (p_curve.get_point_count() == 0u) {
        return Vec3(static_cast<T>(0));
    }
    const T clamped = std::clamp<T>(p_t, static_cast<T>(0), static_cast<T>(1));
    const T t = p_reverse ? (static_cast<T>(1) - clamped) : clamped;
    return p_curve.get_point(t, !use_arc_length_);
}

template <FloatingPointScalar T>
T CoonsPatch3D<T>::_blend_weight(T p_t) const noexcept {
    const T t = std::clamp<T>(p_t, static_cast<T>(0), static_cast<T>(1));
    if (blend_mode_ == CoonsBlendMode::CUBIC_HERMITE) {
        return t * t * (static_cast<T>(3) - static_cast<T>(2) * t);
    }
    return t;
}

template <FloatingPointScalar T>
T CoonsPatch3D<T>::_lagrange_basis(size_t p_idx, T p_t, std::span<const T> p_nodes) noexcept {
    T prod = static_cast<T>(1);
    const T xi = p_nodes[p_idx];
    for (size_t j = 0; j < p_nodes.size(); ++j) {
        if (j == p_idx) continue;
        const T denom = xi - p_nodes[j];
        if (std::abs(denom) > static_cast<T>(1e-9)) {
            prod *= (p_t - p_nodes[j]) / denom;
        }
    }
    return prod;
}

template <FloatingPointScalar T>
void CoonsPatch3D<T>::set_boundary_curves(
    const Spline3D<T> &p_north_v0,
    const Spline3D<T> &p_south_v1,
    const Spline3D<T> &p_west_u0,
    const Spline3D<T> &p_east_u1,
    bool p_rev_north,
    bool p_rev_south,
    bool p_rev_west,
    bool p_rev_east
) {
    north_v0_      = p_north_v0;
    south_v1_      = p_south_v1;
    west_u0_       = p_west_u0;
    east_u1_       = p_east_u1;
    reverse_north_ = p_rev_north;
    reverse_south_ = p_rev_south;
    reverse_west_  = p_rev_west;
    reverse_east_  = p_rev_east;
}

template <FloatingPointScalar T>
void CoonsPatch3D<T>::clear_interior_guides() {
    interior_u_guides_.clear();
    interior_v_guides_.clear();
}

template <FloatingPointScalar T>
void CoonsPatch3D<T>::add_interior_u_guide(T p_v_param, const Spline3D<T> &p_curve, bool p_reverse) {
    const T clamped = std::clamp<T>(p_v_param, static_cast<T>(0.02), static_cast<T>(0.98));
    interior_u_guides_.push_back(GordonGuideCurve3D<T>{clamped, p_curve, p_reverse});
    std::sort(interior_u_guides_.begin(), interior_u_guides_.end(), [](const auto &a, const auto &b) {
        return a.param < b.param;
    });
}

template <FloatingPointScalar T>
void CoonsPatch3D<T>::add_interior_v_guide(T p_u_param, const Spline3D<T> &p_curve, bool p_reverse) {
    const T clamped = std::clamp<T>(p_u_param, static_cast<T>(0.02), static_cast<T>(0.98));
    interior_v_guides_.push_back(GordonGuideCurve3D<T>{clamped, p_curve, p_reverse});
    std::sort(interior_v_guides_.begin(), interior_v_guides_.end(), [](const auto &a, const auto &b) {
        return a.param < b.param;
    });
}

template <FloatingPointScalar T>
void CoonsPatch3D<T>::setup_default_pillow_patch(T p_width, T p_depth, T p_arch_height) {
    const T hw = p_width * static_cast<T>(0.5);
    const T hd = p_depth * static_cast<T>(0.5);

    auto make_arch = [&](const Vec3 &p0, const Vec3 &p1, const Vec3 &mid_offset) -> Spline3D<T> {
        Spline3D<T> s;
        s.set_default_curve_type(CurveType::CATMULL_ROM);
        s.add_point(p0, CurveType::CATMULL_ROM);
        s.add_point((p0 + p1) * static_cast<T>(0.5) + mid_offset, CurveType::CATMULL_ROM);
        s.add_point(p1, CurveType::CATMULL_ROM);
        return s;
    };

    const Vec3 c00(-hw, static_cast<T>(0), -hd);
    const Vec3 c10( hw, static_cast<T>(0), -hd);
    const Vec3 c01(-hw, static_cast<T>(0),  hd);
    const Vec3 c11( hw, static_cast<T>(0),  hd);
    const Vec3 lift(static_cast<T>(0), p_arch_height, static_cast<T>(0));

    north_v0_ = make_arch(c00, c10, lift * static_cast<T>(0.5));
    south_v1_ = make_arch(c01, c11, lift * static_cast<T>(0.5));
    west_u0_  = make_arch(c00, c01, lift * static_cast<T>(0.5));
    east_u1_  = make_arch(c10, c11, lift * static_cast<T>(0.5));

    reverse_north_ = false;
    reverse_south_ = false;
    reverse_west_  = false;
    reverse_east_  = false;

    clear_interior_guides();
    // Add one central rib along U at v = 0.5 with full arch crown to showcase Gordon interpolation
    add_interior_u_guide(
        static_cast<T>(0.5),
        make_arch(
            (c00 + c01) * static_cast<T>(0.5) + lift * static_cast<T>(0.5),
            (c10 + c11) * static_cast<T>(0.5) + lift * static_cast<T>(0.5),
            lift * static_cast<T>(0.5)
        )
    );
}

template <FloatingPointScalar T>
typename CoonsPatch3D<T>::Vec3 CoonsPatch3D<T>::sample_position(T p_u, T p_v) const {
    if (!is_valid()) {
        return Vec3(static_cast<T>(0));
    }

    const T u = std::clamp<T>(p_u, static_cast<T>(0), static_cast<T>(1));
    const T v = std::clamp<T>(p_v, static_cast<T>(0), static_cast<T>(1));

    // Fast-path classic Coons patch when no interior Gordon guide curves exist
    if (interior_u_guides_.empty() && interior_v_guides_.empty()) {
        const T bu = _blend_weight(u);
        const T bv = _blend_weight(v);

        const Vec3 c_n = _eval_curve(north_v0_, u, reverse_north_);
        const Vec3 c_s = _eval_curve(south_v1_, u, reverse_south_);
        const Vec3 c_w = _eval_curve(west_u0_,  v, reverse_west_);
        const Vec3 c_e = _eval_curve(east_u1_,  v, reverse_east_);

        // Average adjacent boundary endpoints for corner-compatibility robustness
        const Vec3 p00 = static_cast<T>(0.5) * (_eval_curve(north_v0_, static_cast<T>(0), reverse_north_) +
                                                _eval_curve(west_u0_,  static_cast<T>(0), reverse_west_));
        const Vec3 p10 = static_cast<T>(0.5) * (_eval_curve(north_v0_, static_cast<T>(1), reverse_north_) +
                                                _eval_curve(east_u1_,  static_cast<T>(0), reverse_east_));
        const Vec3 p01 = static_cast<T>(0.5) * (_eval_curve(south_v1_, static_cast<T>(0), reverse_south_) +
                                                _eval_curve(west_u0_,  static_cast<T>(1), reverse_west_));
        const Vec3 p11 = static_cast<T>(0.5) * (_eval_curve(south_v1_, static_cast<T>(1), reverse_south_) +
                                                _eval_curve(east_u1_,  static_cast<T>(1), reverse_east_));

        const Vec3 loft_u = c_n * (static_cast<T>(1) - bv) + c_s * bv;
        const Vec3 loft_v = c_w * (static_cast<T>(1) - bu) + c_e * bu;
        const Vec3 bilin  =
            p00 * ((static_cast<T>(1) - bu) * (static_cast<T>(1) - bv)) +
            p10 * (bu * (static_cast<T>(1) - bv)) +
            p01 * ((static_cast<T>(1) - bu) * bv) +
            p11 * (bu * bv);

        return loft_u + loft_v - bilin;
    }

    // General Gordon Boolean-Sum Surface: S(u, v) = S_u(u, v) + S_v(u, v) - S_uv(u, v)
    std::vector<T> v_nodes;
    v_nodes.reserve(interior_u_guides_.size() + 2u);
    v_nodes.push_back(static_cast<T>(0));
    for (const auto &g : interior_u_guides_) v_nodes.push_back(g.param);
    v_nodes.push_back(static_cast<T>(1));

    std::vector<T> u_nodes;
    u_nodes.reserve(interior_v_guides_.size() + 2u);
    u_nodes.push_back(static_cast<T>(0));
    for (const auto &g : interior_v_guides_) u_nodes.push_back(g.param);
    u_nodes.push_back(static_cast<T>(1));

    auto eval_u_rib = [&](size_t p_j, T p_u_val) -> Vec3 {
        if (p_j == 0u) return _eval_curve(north_v0_, p_u_val, reverse_north_);
        if (p_j + 1u == v_nodes.size()) return _eval_curve(south_v1_, p_u_val, reverse_south_);
        const auto &g = interior_u_guides_[p_j - 1u];
        return _eval_curve(g.curve, p_u_val, g.reverse);
    };

    auto eval_v_rib = [&](size_t p_i, T p_v_val) -> Vec3 {
        if (p_i == 0u) return _eval_curve(west_u0_, p_v_val, reverse_west_);
        if (p_i + 1u == u_nodes.size()) return _eval_curve(east_u1_, p_v_val, reverse_east_);
        const auto &g = interior_v_guides_[p_i - 1u];
        return _eval_curve(g.curve, p_v_val, g.reverse);
    };

    // Precompute basis weights along U and V
    std::vector<T> basis_v(v_nodes.size(), static_cast<T>(0));
    for (size_t j = 0; j < v_nodes.size(); ++j) {
        basis_v[j] = (v_nodes.size() == 2u)
            ? (j == 0u ? (static_cast<T>(1) - _blend_weight(v)) : _blend_weight(v))
            : _lagrange_basis(j, v, v_nodes);
    }

    std::vector<T> basis_u(u_nodes.size(), static_cast<T>(0));
    for (size_t i = 0; i < u_nodes.size(); ++i) {
        basis_u[i] = (u_nodes.size() == 2u)
            ? (i == 0u ? (static_cast<T>(1) - _blend_weight(u)) : _blend_weight(u))
            : _lagrange_basis(i, u, u_nodes);
    }

    Vec3 S_u(static_cast<T>(0));
    for (size_t j = 0; j < v_nodes.size(); ++j) {
        S_u += eval_u_rib(j, u) * basis_v[j];
    }

    Vec3 S_v(static_cast<T>(0));
    for (size_t i = 0; i < u_nodes.size(); ++i) {
        S_v += eval_v_rib(i, v) * basis_u[i];
    }

    Vec3 S_uv(static_cast<T>(0));
    for (size_t i = 0; i < u_nodes.size(); ++i) {
        for (size_t j = 0; j < v_nodes.size(); ++j) {
            const Vec3 q_ij = static_cast<T>(0.5) * (eval_u_rib(j, u_nodes[i]) + eval_v_rib(i, v_nodes[j]));
            S_uv += q_ij * (basis_u[i] * basis_v[j]);
        }
    }

    return S_u + S_v - S_uv;
}

template <FloatingPointScalar T>
typename CoonsPatch3D<T>::Frame CoonsPatch3D<T>::sample_frame(T p_u, T p_v, uint32_t p_patch_id) const {
    if (!is_valid()) {
        return Frame{};
    }

    const T u = std::clamp<T>(p_u, static_cast<T>(0), static_cast<T>(1));
    const T v = std::clamp<T>(p_v, static_cast<T>(0), static_cast<T>(1));
    constexpr T kH = static_cast<T>(1e-4);

    const Vec3 pos = sample_position(u, v);

    const T u0 = std::max<T>(static_cast<T>(0), u - kH);
    const T u1 = std::min<T>(static_cast<T>(1), u + kH);
    const Vec3 du = (u1 > u0) ? ((sample_position(u1, v) - sample_position(u0, v)) / (u1 - u0)) : Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));

    const T v0 = std::max<T>(static_cast<T>(0), v - kH);
    const T v1 = std::min<T>(static_cast<T>(1), v + kH);
    const Vec3 dv = (v1 > v0) ? ((sample_position(u, v1) - sample_position(u, v0)) / (v1 - v0)) : Vec3(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1));

    return make_surface_frame_from_partials(pos, du, dv, u, v, p_patch_id);
}

template <FloatingPointScalar T>
std::vector<typename CoonsPatch3D<T>::Frame> CoonsPatch3D<T>::sample_boundary_edge(
    SurfaceBoundaryEdge p_edge,
    uint32_t p_divs,
    bool p_reverse,
    uint32_t p_patch_id
) const {
    const uint32_t divs = std::max<uint32_t>(1u, p_divs);
    std::vector<Frame> out;
    out.reserve(divs + 1u);

    for (uint32_t i = 0u; i <= divs; ++i) {
        T t = static_cast<T>(i) / static_cast<T>(divs);
        if (p_reverse) {
            t = static_cast<T>(1) - t;
        }
        T u = static_cast<T>(0);
        T v = static_cast<T>(0);
        switch (p_edge) {
            case SurfaceBoundaryEdge::NORTH_V0: u = t; v = static_cast<T>(0); break;
            case SurfaceBoundaryEdge::SOUTH_V1: u = t; v = static_cast<T>(1); break;
            case SurfaceBoundaryEdge::WEST_U0:  u = static_cast<T>(0); v = t; break;
            case SurfaceBoundaryEdge::EAST_U1:  u = static_cast<T>(1); v = t; break;
        }
        out.push_back(sample_frame(u, v, p_patch_id));
    }
    return out;
}

template <FloatingPointScalar T>
SurfaceQuadMesh3D<T> CoonsPatch3D<T>::tessellate_quad_mesh(
    uint32_t p_base_u_divs,
    uint32_t p_base_v_divs,
    uint8_t p_lod_level,
    uint32_t p_patch_id
) const {
    SurfaceQuadMesh3D<T> mesh;
    if (!is_valid()) {
        return mesh;
    }

    const uint32_t u_divs = resolve_surface_lod_divs(p_base_u_divs, p_lod_level);
    const uint32_t v_divs = resolve_surface_lod_divs(p_base_v_divs, p_lod_level);
    const uint32_t cols = u_divs + 1u;
    const uint32_t rows = v_divs + 1u;
    const size_t vcount = static_cast<size_t>(cols) * rows;

    mesh.positions.reserve(vcount);
    mesh.normals.reserve(vcount);
    mesh.tangents.reserve(vcount);
    mesh.uvs.reserve(vcount);
    mesh.vertex_patch_ids.reserve(vcount);
    mesh.vertex_ring_indices.reserve(vcount);
    mesh.vertex_min_lod_levels.reserve(vcount);

    for (uint32_t j = 0u; j < rows; ++j) {
        const T v = static_cast<T>(j) / static_cast<T>(v_divs);
        for (uint32_t i = 0u; i < cols; ++i) {
            const T u = static_cast<T>(i) / static_cast<T>(u_divs);
            const Frame f = sample_frame(u, v, p_patch_id);
            mesh.positions.push_back(f.position);
            mesh.normals.push_back(f.normal);
            mesh.tangents.emplace_back(f.tangent_u.x, f.tangent_u.y, f.tangent_u.z, static_cast<T>(1));
            mesh.uvs.emplace_back(u, v);
            mesh.vertex_patch_ids.push_back(p_patch_id);
            mesh.vertex_ring_indices.push_back(j);
            mesh.vertex_min_lod_levels.push_back(p_lod_level);
        }
    }

    mesh.quads.reserve(static_cast<size_t>(u_divs) * v_divs);
    mesh.quad_patch_ids.reserve(static_cast<size_t>(u_divs) * v_divs);

    for (uint32_t j = 0u; j < v_divs; ++j) {
        for (uint32_t i = 0u; i < u_divs; ++i) {
            const uint32_t v00 = j * cols + i;
            const uint32_t v10 = j * cols + (i + 1u);
            const uint32_t v11 = (j + 1u) * cols + (i + 1u);
            const uint32_t v01 = (j + 1u) * cols + i;
            mesh.quads.emplace_back(v00, v10, v11, v01);
            mesh.quad_patch_ids.push_back(p_patch_id);
        }
    }

    return mesh;
}

template class CoonsPatch3D<float>;
template class CoonsPatch3D<double>;

} // namespace ideam::core
