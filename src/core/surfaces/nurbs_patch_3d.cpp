#include "nurbs_patch_3d.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace ideam::core {

namespace {
constexpr uint32_t kMaxSupportedDegree = 7u;
}

template <FloatingPointScalar T>
uint32_t NurbsPatch3D<T>::_effective_degree_u() const noexcept {
    if (count_u_ < 2u) return 1u;
    return std::min<uint32_t>(std::min<uint32_t>(degree_u_, count_u_ - 1u), kMaxSupportedDegree);
}

template <FloatingPointScalar T>
uint32_t NurbsPatch3D<T>::_effective_degree_v() const noexcept {
    if (count_v_ < 2u) return 1u;
    return std::min<uint32_t>(std::min<uint32_t>(degree_v_, count_v_ - 1u), kMaxSupportedDegree);
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::_rebuild_clamped_knots(std::vector<T> &r_knots, uint32_t p_count, uint32_t p_degree) const {
    r_knots.clear();
    if (p_count < 2u) return;
    const uint32_t p = std::min<uint32_t>(p_degree, p_count - 1u);
    const uint32_t m = p_count + p + 1u;
    r_knots.resize(m, static_cast<T>(0));

    const uint32_t interior_spans = p_count - p;
    for (uint32_t i = 0u; i <= p; ++i) {
        r_knots[i] = static_cast<T>(0);
    }
    for (uint32_t i = 1u; i < interior_spans; ++i) {
        r_knots[p + i] = static_cast<T>(i) / static_cast<T>(interior_spans);
    }
    for (uint32_t i = p_count; i < m; ++i) {
        r_knots[i] = static_cast<T>(1);
    }
}

template <FloatingPointScalar T>
uint32_t NurbsPatch3D<T>::_find_knot_span(
    uint32_t p_n_ctrl,
    uint32_t p_degree,
    T p_param,
    std::span<const T> p_knots
) noexcept {
    const uint32_t n = p_n_ctrl - 1u;
    if (p_param >= p_knots[n + 1u]) {
        return n;
    }
    if (p_param <= p_knots[p_degree]) {
        return p_degree;
    }

    uint32_t low = p_degree;
    uint32_t high = n + 1u;
    uint32_t mid = (low + high) / 2u;
    while (p_param < p_knots[mid] || p_param >= p_knots[mid + 1u]) {
        if (p_param < p_knots[mid]) {
            high = mid;
        } else {
            low = mid;
        }
        mid = (low + high) / 2u;
        if (low >= high) {
            break;
        }
    }
    return mid;
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::_eval_basis_and_derivs(
    uint32_t p_span,
    T p_param,
    uint32_t p_degree,
    std::span<const T> p_knots,
    T *r_basis,
    T *r_deriv
) noexcept {
    // Piegl & Tiller Algorithm A2.3 (DersBasisFuns) specialized for k = 1
    constexpr uint32_t kStride = kMaxSupportedDegree + 1u;
    T ndu[kStride][kStride]{};
    T left[kStride]{};
    T right[kStride]{};

    ndu[0][0] = static_cast<T>(1);
    for (uint32_t j = 1u; j <= p_degree; ++j) {
        left[j]  = p_param - p_knots[p_span + 1u - j];
        right[j] = p_knots[p_span + j] - p_param;
        T saved = static_cast<T>(0);
        for (uint32_t r = 0u; r < j; ++r) {
            ndu[j][r] = right[r + 1u] + left[j - r];
            const T denom = ndu[j][r];
            const T temp = (std::abs(denom) > static_cast<T>(1e-12)) ? (ndu[r][j - 1u] / denom) : static_cast<T>(0);
            ndu[r][j] = saved + right[r + 1u] * temp;
            saved = left[j - r] * temp;
        }
        ndu[j][j] = saved;
    }

    for (uint32_t j = 0u; j <= p_degree; ++j) {
        r_basis[j] = ndu[j][p_degree];
        r_deriv[j] = static_cast<T>(0);
    }

    if (p_degree == 0u) {
        return;
    }

    // First derivative: N'_{i,p}(u) = p * ( N_{i,p-1}/(u_{i+p}-u_i) - N_{i+1,p-1}/(u_{i+p+1}-u_{i+1}) )
    for (uint32_t r = 0u; r <= p_degree; ++r) {
        T d = static_cast<T>(0);
        if (r >= 1u) {
            const T denom = ndu[p_degree][r - 1u];
            if (std::abs(denom) > static_cast<T>(1e-12)) {
                d += ndu[r - 1u][p_degree - 1u] / denom;
            }
        }
        if (r < p_degree) {
            const T denom = ndu[p_degree][r];
            if (std::abs(denom) > static_cast<T>(1e-12)) {
                d -= ndu[r][p_degree - 1u] / denom;
            }
        }
        r_deriv[r] = d * static_cast<T>(p_degree);
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::resize_grid(uint32_t p_count_u, uint32_t p_count_v, uint32_t p_degree_u, uint32_t p_degree_v) {
    count_u_  = p_count_u;
    count_v_  = p_count_v;
    degree_u_ = std::clamp<uint32_t>(p_degree_u, 1u, kMaxSupportedDegree);
    degree_v_ = std::clamp<uint32_t>(p_degree_v, 1u, kMaxSupportedDegree);

    const size_t total = static_cast<size_t>(count_u_) * count_v_;
    control_points_.assign(total, Vec3(static_cast<T>(0)));
    weights_.assign(total, static_cast<T>(1));
    custom_knots_u_ = false;
    custom_knots_v_ = false;
    reset_uniform_knots();
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::clear() {
    count_u_ = 0u;
    count_v_ = 0u;
    control_points_.clear();
    weights_.clear();
    knots_u_.clear();
    knots_v_.clear();
    custom_knots_u_ = false;
    custom_knots_v_ = false;
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::set_degree_u(uint32_t p_degree) {
    degree_u_ = std::clamp<uint32_t>(p_degree, 1u, kMaxSupportedDegree);
    if (!custom_knots_u_ && count_u_ >= 2u) {
        _rebuild_clamped_knots(knots_u_, count_u_, _effective_degree_u());
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::set_degree_v(uint32_t p_degree) {
    degree_v_ = std::clamp<uint32_t>(p_degree, 1u, kMaxSupportedDegree);
    if (!custom_knots_v_ && count_v_ >= 2u) {
        _rebuild_clamped_knots(knots_v_, count_v_, _effective_degree_v());
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::set_control_point(uint32_t p_u, uint32_t p_v, const Vec3 &p_pos, T p_weight) {
    if (p_u >= count_u_ || p_v >= count_v_) return;
    const size_t idx = flat_index(p_u, p_v);
    control_points_[idx] = p_pos;
    weights_[idx] = std::max<T>(static_cast<T>(1e-4), p_weight);
}

template <FloatingPointScalar T>
typename NurbsPatch3D<T>::Vec3 NurbsPatch3D<T>::get_control_point(uint32_t p_u, uint32_t p_v) const noexcept {
    if (p_u >= count_u_ || p_v >= count_v_) return Vec3(static_cast<T>(0));
    return control_points_[flat_index(p_u, p_v)];
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::set_weight(uint32_t p_u, uint32_t p_v, T p_weight) {
    if (p_u >= count_u_ || p_v >= count_v_) return;
    weights_[flat_index(p_u, p_v)] = std::max<T>(static_cast<T>(1e-4), p_weight);
}

template <FloatingPointScalar T>
T NurbsPatch3D<T>::get_weight(uint32_t p_u, uint32_t p_v) const noexcept {
    if (p_u >= count_u_ || p_v >= count_v_) return static_cast<T>(1);
    return weights_[flat_index(p_u, p_v)];
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::set_control_points_bulk(
    uint32_t p_count_u,
    uint32_t p_count_v,
    std::span<const Vec3> p_points,
    std::span<const T> p_weights
) {
    const size_t total = static_cast<size_t>(p_count_u) * p_count_v;
    if (p_count_u < 2u || p_count_v < 2u || p_points.size() != total) {
        return;
    }
    count_u_ = p_count_u;
    count_v_ = p_count_v;
    control_points_.assign(p_points.begin(), p_points.end());
    if (p_weights.size() == total) {
        weights_.resize(total);
        for (size_t i = 0; i < total; ++i) {
            weights_[i] = std::max<T>(static_cast<T>(1e-4), p_weights[i]);
        }
    } else {
        weights_.assign(total, static_cast<T>(1));
    }
    if (!custom_knots_u_ || knots_u_.size() != count_u_ + _effective_degree_u() + 1u) {
        custom_knots_u_ = false;
        _rebuild_clamped_knots(knots_u_, count_u_, _effective_degree_u());
    }
    if (!custom_knots_v_ || knots_v_.size() != count_v_ + _effective_degree_v() + 1u) {
        custom_knots_v_ = false;
        _rebuild_clamped_knots(knots_v_, count_v_, _effective_degree_v());
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::set_custom_knots_u(std::span<const T> p_knots) {
    if (p_knots.size() == count_u_ + _effective_degree_u() + 1u) {
        knots_u_.assign(p_knots.begin(), p_knots.end());
        custom_knots_u_ = true;
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::set_custom_knots_v(std::span<const T> p_knots) {
    if (p_knots.size() == count_v_ + _effective_degree_v() + 1u) {
        knots_v_.assign(p_knots.begin(), p_knots.end());
        custom_knots_v_ = true;
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::reset_uniform_knots() {
    custom_knots_u_ = false;
    custom_knots_v_ = false;
    if (count_u_ >= 2u) _rebuild_clamped_knots(knots_u_, count_u_, _effective_degree_u());
    if (count_v_ >= 2u) _rebuild_clamped_knots(knots_v_, count_v_, _effective_degree_v());
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::loft_from_splines(
    std::span<const Spline3D<T>> p_ribs,
    uint32_t p_samples_u,
    uint32_t p_degree_u,
    uint32_t p_degree_v,
    bool p_arc_length_parametrize
) {
    if (p_ribs.size() < 2u) {
        clear();
        return;
    }
    const uint32_t nu = std::max<uint32_t>(2u, p_samples_u);
    const uint32_t nv = static_cast<uint32_t>(p_ribs.size());
    resize_grid(nu, nv, p_degree_u, p_degree_v);

    const bool use_weighted = !p_arc_length_parametrize;
    for (uint32_t j = 0u; j < nv; ++j) {
        const Spline3D<T> &rib = p_ribs[j];
        for (uint32_t i = 0u; i < nu; ++i) {
            const T u = static_cast<T>(i) / static_cast<T>(nu - 1u);
            const Vec3 pt = (rib.get_point_count() > 0u) ? rib.get_point(u, use_weighted) : Vec3(static_cast<T>(0));
            set_control_point(i, j, pt, static_cast<T>(1));
        }
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::setup_flat_grid(T p_width, T p_depth, uint32_t p_count_u, uint32_t p_count_v) {
    const uint32_t nu = std::max<uint32_t>(2u, p_count_u);
    const uint32_t nv = std::max<uint32_t>(2u, p_count_v);
    resize_grid(nu, nv, std::min<uint32_t>(3u, nu - 1u), std::min<uint32_t>(3u, nv - 1u));
    wrap_u_ = false;
    wrap_v_ = false;

    for (uint32_t j = 0u; j < nv; ++j) {
        const T fv = static_cast<T>(j) / static_cast<T>(nv - 1u);
        const T z = (fv - static_cast<T>(0.5)) * p_depth;
        for (uint32_t i = 0u; i < nu; ++i) {
            const T fu = static_cast<T>(i) / static_cast<T>(nu - 1u);
            const T x = (fu - static_cast<T>(0.5)) * p_width;
            set_control_point(i, j, Vec3(x, static_cast<T>(0), z), static_cast<T>(1));
        }
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::setup_cylinder_patch(T p_radius, T p_height, T p_arc_radians, uint32_t p_count_u, uint32_t p_count_v) {
    const uint32_t nu = std::max<uint32_t>(4u, p_count_u);
    const uint32_t nv = std::max<uint32_t>(2u, p_count_v);
    resize_grid(nu, nv, std::min<uint32_t>(3u, nu - 1u), std::min<uint32_t>(3u, nv - 1u));
    wrap_u_ = std::abs(p_arc_radians - static_cast<T>(6.283185307179586)) < static_cast<T>(1e-3);
    wrap_v_ = false;

    for (uint32_t j = 0u; j < nv; ++j) {
        const T fv = static_cast<T>(j) / static_cast<T>(nv - 1u);
        const T y = (fv - static_cast<T>(0.5)) * p_height;
        for (uint32_t i = 0u; i < nu; ++i) {
            const T fu = static_cast<T>(i) / static_cast<T>(nu - 1u);
            const T theta = fu * p_arc_radians;
            set_control_point(i, j, Vec3(std::cos(theta) * p_radius, y, std::sin(theta) * p_radius), static_cast<T>(1));
        }
    }
}

template <FloatingPointScalar T>
void NurbsPatch3D<T>::setup_dome_patch(T p_radius, T p_height, uint32_t p_count_u, uint32_t p_count_v) {
    const uint32_t nu = std::max<uint32_t>(3u, p_count_u);
    const uint32_t nv = std::max<uint32_t>(3u, p_count_v);
    resize_grid(nu, nv, std::min<uint32_t>(3u, nu - 1u), std::min<uint32_t>(3u, nv - 1u));
    wrap_u_ = false;
    wrap_v_ = false;

    for (uint32_t j = 0u; j < nv; ++j) {
        const T fv = static_cast<T>(j) / static_cast<T>(nv - 1u);
        const T nz = (fv - static_cast<T>(0.5)) * static_cast<T>(2.0);
        for (uint32_t i = 0u; i < nu; ++i) {
            const T fu = static_cast<T>(i) / static_cast<T>(nu - 1u);
            const T nx = (fu - static_cast<T>(0.5)) * static_cast<T>(2.0);
            const T r2 = nx * nx + nz * nz;
            const T y = p_height * std::max<T>(static_cast<T>(0), static_cast<T>(1.0) - static_cast<T>(0.5) * r2);
            set_control_point(i, j, Vec3(nx * p_radius, y, nz * p_radius), static_cast<T>(1));
        }
    }
}

template <FloatingPointScalar T>
typename NurbsPatch3D<T>::Vec3 NurbsPatch3D<T>::sample_position(T p_u, T p_v) const noexcept {
    return sample_frame(p_u, p_v, 0u).position;
}

template <FloatingPointScalar T>
typename NurbsPatch3D<T>::Frame NurbsPatch3D<T>::sample_frame(T p_u, T p_v, uint32_t p_patch_id) const noexcept {
    if (!is_valid()) {
        return Frame{};
    }

    const uint32_t pu = _effective_degree_u();
    const uint32_t pv = _effective_degree_v();
    if (knots_u_.size() < count_u_ + pu + 1u || knots_v_.size() < count_v_ + pv + 1u) {
        return Frame{};
    }

    const T u_min = knots_u_[pu];
    const T u_max = knots_u_[count_u_];
    const T v_min = knots_v_[pv];
    const T v_max = knots_v_[count_v_];

    const T u_clamped = std::clamp<T>(p_u, static_cast<T>(0), static_cast<T>(1));
    const T v_clamped = std::clamp<T>(p_v, static_cast<T>(0), static_cast<T>(1));
    const T u_knot = u_min + u_clamped * (u_max - u_min);
    const T v_knot = v_min + v_clamped * (v_max - v_min);

    const uint32_t span_u = _find_knot_span(count_u_, pu, u_knot, knots_u_);
    const uint32_t span_v = _find_knot_span(count_v_, pv, v_knot, knots_v_);

    T Nu[kMaxSupportedDegree + 1u]{};
    T dNu[kMaxSupportedDegree + 1u]{};
    T Nv[kMaxSupportedDegree + 1u]{};
    T dNv[kMaxSupportedDegree + 1u]{};

    _eval_basis_and_derivs(span_u, u_knot, pu, knots_u_, Nu, dNu);
    _eval_basis_and_derivs(span_v, v_knot, pv, knots_v_, Nv, dNv);

    // Homogeneous coordinates: H = (w*P, w), dHu, dHv
    Vec3 A(static_cast<T>(0));
    Vec3 Au(static_cast<T>(0));
    Vec3 Av(static_cast<T>(0));
    T W  = static_cast<T>(0);
    T Wu = static_cast<T>(0);
    T Wv = static_cast<T>(0);

    for (uint32_t l = 0u; l <= pv; ++l) {
        const uint32_t vj = span_v - pv + l;
        Vec3 temp_a(static_cast<T>(0));
        Vec3 temp_au(static_cast<T>(0));
        T temp_w  = static_cast<T>(0);
        T temp_wu = static_cast<T>(0);

        for (uint32_t k = 0u; k <= pu; ++k) {
            const uint32_t ui = span_u - pu + k;
            const size_t idx = flat_index(ui, vj);
            const T w = weights_[idx];
            const Vec3 wp = control_points_[idx] * w;

            temp_a  += wp * Nu[k];
            temp_w  += w  * Nu[k];
            temp_au += wp * dNu[k];
            temp_wu += w  * dNu[k];
        }

        A  += temp_a  * Nv[l];
        W  += temp_w  * Nv[l];
        Au += temp_au * Nv[l];
        Wu += temp_wu * Nv[l];
        Av += temp_a  * dNv[l];
        Wv += temp_w  * dNv[l];
    }

    const T inv_w = (std::abs(W) > static_cast<T>(1e-9)) ? (static_cast<T>(1) / W) : static_cast<T>(1);
    const Vec3 S  = A * inv_w;
    // Quotient rule for rational surface derivatives: dS/du = (Au - Wu * S) / W
    const Vec3 Su = (Au - S * Wu) * inv_w;
    const Vec3 Sv = (Av - S * Wv) * inv_w;

    return make_surface_frame_from_partials(S, Su, Sv, u_clamped, v_clamped, p_patch_id);
}

template <FloatingPointScalar T>
std::vector<typename NurbsPatch3D<T>::Frame> NurbsPatch3D<T>::sample_boundary_edge(
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
SurfaceQuadMesh3D<T> NurbsPatch3D<T>::tessellate_quad_mesh(
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

template class NurbsPatch3D<float>;
template class NurbsPatch3D<double>;

} // namespace ideam::core
