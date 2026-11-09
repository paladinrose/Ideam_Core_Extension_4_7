#include "symmetry_2d.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

template <FloatingPointScalar T>
LatticeBasis2DPOD<T> SymmetryMath2D<T>::make_lattice_basis(
    const Vec2& p_t1,
    const Vec2& p_t2,
    const Vec2& p_origin
) noexcept {
    LatticeBasis2DPOD<T> basis;
    basis.t1 = p_t1;
    basis.t2 = p_t2;
    basis.origin = p_origin;

    const T det = p_t1.x * p_t2.y - p_t2.x * p_t1.y;
    if (std::abs(det) > GeometryConstants<T>::SQ_EPSILON) {
        const T inv_det = static_cast<T>(1.0) / det;
        // GLM column-major: col 0 = (t2.y, -t1.y) * inv_det, col 1 = (-t2.x, t1.x) * inv_det
        basis.inv_basis[0][0] =  p_t2.y * inv_det;
        basis.inv_basis[0][1] = -p_t1.y * inv_det;
        basis.inv_basis[1][0] = -p_t2.x * inv_det;
        basis.inv_basis[1][1] =  p_t1.x * inv_det;
    } else {
        basis.inv_basis = Mat2(static_cast<T>(1.0));
    }
    return basis;
}

template <FloatingPointScalar T>
LatticeBasis2DPOD<T> SymmetryMath2D<T>::make_canonical_lattice(
    LatticeType2D p_type,
    T p_scale_u,
    T p_scale_v,
    T p_oblique_angle_rad,
    const Vec2& p_origin
) noexcept {
    switch (p_type) {
        case LatticeType2D::SQUARE:
            return make_lattice_basis(
                Vec2(p_scale_u, static_cast<T>(0.0)),
                Vec2(static_cast<T>(0.0), p_scale_u),
                p_origin
            );
        case LatticeType2D::RECTANGULAR:
            return make_lattice_basis(
                Vec2(p_scale_u, static_cast<T>(0.0)),
                Vec2(static_cast<T>(0.0), p_scale_v),
                p_origin
            );
        case LatticeType2D::HEXAGONAL: {
            constexpr T cos_60 = static_cast<T>(0.5);
            constexpr T sin_60 = static_cast<T>(0.86602540378443864676);
            return make_lattice_basis(
                Vec2(p_scale_u, static_cast<T>(0.0)),
                Vec2(p_scale_u * cos_60, p_scale_u * sin_60),
                p_origin
            );
        }
        case LatticeType2D::RHOMBIC: {
            const T half_u = p_scale_u * static_cast<T>(0.5);
            const T half_v = p_scale_v * static_cast<T>(0.5);
            return make_lattice_basis(
                Vec2(half_u, half_v),
                Vec2(half_u, -half_v),
                p_origin
            );
        }
        case LatticeType2D::OBLIQUE:
        default:
            return make_lattice_basis(
                Vec2(p_scale_u, static_cast<T>(0.0)),
                Vec2(p_scale_v * std::cos(p_oblique_angle_rad), p_scale_v * std::sin(p_oblique_angle_rad)),
                p_origin
            );
    }
}

template <FloatingPointScalar T>
typename SymmetryMath2D<T>::Mat3 SymmetryMath2D<T>::make_rotation(
    T p_angle_rad,
    const Vec2& p_pivot
) noexcept {
    const T c = std::cos(p_angle_rad);
    const T s = std::sin(p_angle_rad);

    Mat3 m(static_cast<T>(1.0));
    m[0][0] =  c;
    m[0][1] =  s;
    m[1][0] = -s;
    m[1][1] =  c;
    m[2][0] = p_pivot.x - (c * p_pivot.x - s * p_pivot.y);
    m[2][1] = p_pivot.y - (s * p_pivot.x + c * p_pivot.y);
    return m;
}

template <FloatingPointScalar T>
typename SymmetryMath2D<T>::Mat3 SymmetryMath2D<T>::make_reflection_across_line(
    const Vec2& p_line_origin,
    const Vec2& p_line_unit_dir
) noexcept {
    const T len_sq = glm::dot(p_line_unit_dir, p_line_unit_dir);
    const Vec2 d = (len_sq > GeometryConstants<T>::SQ_EPSILON)
        ? (p_line_unit_dir * (static_cast<T>(1.0) / std::sqrt(len_sq)))
        : Vec2(static_cast<T>(1.0), static_cast<T>(0.0));

    // Householder reflection matrix R = 2 * (d * d^T) - I
    const T r00 = static_cast<T>(2.0) * d.x * d.x - static_cast<T>(1.0);
    const T r01 = static_cast<T>(2.0) * d.x * d.y;
    const T r11 = static_cast<T>(2.0) * d.y * d.y - static_cast<T>(1.0);

    Mat3 m(static_cast<T>(1.0));
    m[0][0] = r00;
    m[0][1] = r01;
    m[1][0] = r01;
    m[1][1] = r11;
    m[2][0] = p_line_origin.x - (r00 * p_line_origin.x + r01 * p_line_origin.y);
    m[2][1] = p_line_origin.y - (r01 * p_line_origin.x + r11 * p_line_origin.y);
    return m;
}

template <FloatingPointScalar T>
typename SymmetryMath2D<T>::Mat3 SymmetryMath2D<T>::make_glide_reflection(
    const Vec2& p_line_origin,
    const Vec2& p_line_unit_dir,
    T p_glide_distance
) noexcept {
    Mat3 m = make_reflection_across_line(p_line_origin, p_line_unit_dir);
    const T len_sq = glm::dot(p_line_unit_dir, p_line_unit_dir);
    const Vec2 d = (len_sq > GeometryConstants<T>::SQ_EPSILON)
        ? (p_line_unit_dir * (static_cast<T>(1.0) / std::sqrt(len_sq)))
        : Vec2(static_cast<T>(1.0), static_cast<T>(0.0));

    m[2][0] += d.x * p_glide_distance;
    m[2][1] += d.y * p_glide_distance;
    return m;
}

template <FloatingPointScalar T>
std::vector<typename SymmetryMath2D<T>::Mat3> SymmetryMath2D<T>::generate_dihedral_group(
    uint32_t p_n,
    const Vec2& p_pivot,
    bool p_include_reflections
) {
    if (p_n == 0) p_n = 1;
    std::vector<Mat3> ops;
    ops.reserve(p_include_reflections ? (p_n * 2u) : p_n);

    const T step = GeometryConstants<T>::TAU / static_cast<T>(p_n);
    for (uint32_t k = 0; k < p_n; ++k) {
        ops.push_back(make_rotation(step * static_cast<T>(k), p_pivot));
    }

    if (p_include_reflections) {
        for (uint32_t k = 0; k < p_n; ++k) {
            const T axis_angle = static_cast<T>(0.5) * step * static_cast<T>(k);
            const Vec2 axis_dir(std::cos(axis_angle), std::sin(axis_angle));
            ops.push_back(make_reflection_across_line(p_pivot, axis_dir));
        }
    }
    return ops;
}

template <FloatingPointScalar T>
typename SymmetryMath2D<T>::Mat3 SymmetryMath2D<T>::make_edge_match_transform(
    const Vec2& p_p,
    const Vec2& p_q,
    bool p_flip,
    bool p_rotate_180
) noexcept {
    // Matches csk::match(p, q) * M_orients[2*fl + ro] in src/external/tactile/tiling.cpp
    const Mat3 seg_match(
        p_q.x - p_p.x, p_q.y - p_p.y, static_cast<T>(0.0),
        p_p.y - p_q.y, p_q.x - p_p.x, static_cast<T>(0.0),
        p_p.x,         p_p.y,         static_cast<T>(1.0)
    );

    Mat3 orient(static_cast<T>(1.0));
    if (!p_flip && p_rotate_180) {
        // ROT: (-1, 0, 0, -1, 1, 0)
        orient[0][0] = static_cast<T>(-1.0);
        orient[1][1] = static_cast<T>(-1.0);
        orient[2][0] = static_cast<T>(1.0);
    } else if (p_flip && !p_rotate_180) {
        // FLIP: (-1, 0, 0, 1, 1, 0)
        orient[0][0] = static_cast<T>(-1.0);
        orient[1][1] = static_cast<T>(1.0);
        orient[2][0] = static_cast<T>(1.0);
    } else if (p_flip && p_rotate_180) {
        // ROFL: (1, 0, 0, -1, 0, 0)
        orient[0][0] = static_cast<T>(1.0);
        orient[1][1] = static_cast<T>(-1.0);
    }

    return seg_match * orient;
}

template <FloatingPointScalar T>
std::vector<typename SymmetryMath2D<T>::Vec2> SymmetryMath2D<T>::evaluate_symmetric_tile_edge(
    std::span<const Vec2> p_canonical_curve,
    const Vec2& p_edge_start,
    const Vec2& p_edge_end,
    TactileEdgeShape p_shape,
    bool p_reversed
) {
    if (p_shape == TactileEdgeShape::EDGE_I || p_canonical_curve.size() < 2) {
        if (p_reversed) return { p_edge_end, p_edge_start };
        return { p_edge_start, p_edge_end };
    }

    const Mat3 edge_xform = make_edge_match_transform(p_edge_start, p_edge_end, false, false);
    std::vector<Vec2> out;

    if (p_shape == TactileEdgeShape::EDGE_J) {
        out.reserve(p_canonical_curve.size());
        for (const auto& pt : p_canonical_curve) {
            out.push_back(transform_point(edge_xform, pt));
        }
    } else {
        // For U (mirror-symmetric) and S (centrosymmetric) edges, the canonical curve
        // defines the first half [0, 0.5] and is reflected/inverted into [0.5, 1.0],
        // matching TSPI_U and TSPI_S in src/external/tactile/tiling.cpp.
        const size_t n = p_canonical_curve.size();
        out.reserve(n * 2 - 1);

        for (size_t i = 0; i < n; ++i) {
            const Vec2 half_pt(p_canonical_curve[i].x * static_cast<T>(0.5),
                               p_canonical_curve[i].y * static_cast<T>(0.5));
            out.push_back(transform_point(edge_xform, half_pt));
        }
        for (size_t i = n - 1; i > 0; --i) {
            const Vec2& src = p_canonical_curve[i - 1];
            const Vec2 second_half = (p_shape == TactileEdgeShape::EDGE_U)
                ? Vec2(static_cast<T>(1.0) - src.x * static_cast<T>(0.5),  src.y * static_cast<T>(0.5))
                : Vec2(static_cast<T>(1.0) - src.x * static_cast<T>(0.5), -src.y * static_cast<T>(0.5));
            out.push_back(transform_point(edge_xform, second_half));
        }
    }

    if (p_reversed) {
        std::reverse(out.begin(), out.end());
    }
    return out;
}

// Explicit Instantiations
template struct SymmetryMath2D<float>;
template struct SymmetryMath2D<double>;

} // namespace ideam::core