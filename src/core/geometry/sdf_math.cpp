#include "sdf_math.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

template <FloatingPointScalar T>
T SDFMath<T>::sd_oriented_box_2d(
    const Vec2& p_p,
    const Vec2& p_a,
    const Vec2& p_b,
    T p_thickness
) noexcept {
    const Vec2 ba = p_b - p_a;
    const T l = glm::length(ba);
    if (l <= GeometryConstants<T>::EPSILON) {
        return glm::distance(p_p, p_a) - p_thickness * static_cast<T>(0.5);
    }
    const Vec2 d = ba / l;
    Vec2 q = p_p - (p_a + p_b) * static_cast<T>(0.5);
    q = Vec2(d.x * q.x + d.y * q.y, -d.y * q.x + d.x * q.y);
    q = glm::abs(q) - Vec2(l, p_thickness) * static_cast<T>(0.5);
    return glm::length(glm::max(q, Vec2(static_cast<T>(0.0)))) +
           std::min(std::max(q.x, q.y), static_cast<T>(0.0));
}

template <FloatingPointScalar T>
T SDFMath<T>::sd_segment_2d(
    const Vec2& p_p,
    const Vec2& p_a,
    const Vec2& p_b,
    T p_radius
) noexcept {
    const Vec2 pa = p_p - p_a;
    const Vec2 ba = p_b - p_a;
    const T denom = glm::dot(ba, ba);
    const T h = (denom > GeometryConstants<T>::SQ_EPSILON)
        ? std::clamp(glm::dot(pa, ba) / denom, static_cast<T>(0.0), static_cast<T>(1.0))
        : static_cast<T>(0.0);
    return glm::length(pa - ba * h) - p_radius;
}

template <FloatingPointScalar T>
T SDFMath<T>::sd_polygon_2d(std::span<const Vec2> p_polygon, const Vec2& p_p) noexcept {
    if (p_polygon.empty()) return static_cast<T>(0.0);
    if (p_polygon.size() == 1) return glm::distance(p_p, p_polygon[0]);

    T d_sq = glm::dot(p_p - p_polygon[0], p_p - p_polygon[0]);
    T sign = static_cast<T>(1.0);

    for (size_t i = 0, j = p_polygon.size() - 1; i < p_polygon.size(); j = i++) {
        const Vec2 e = p_polygon[j] - p_polygon[i];
        const Vec2 w = p_p - p_polygon[i];
        const T ee = glm::dot(e, e);
        const T t = (ee > GeometryConstants<T>::SQ_EPSILON)
            ? std::clamp(glm::dot(w, e) / ee, static_cast<T>(0.0), static_cast<T>(1.0))
            : static_cast<T>(0.0);
        const Vec2 b = w - e * t;
        d_sq = std::min(d_sq, glm::dot(b, b));

        const bool c0 = (p_p.y >= p_polygon[i].y);
        const bool c1 = (p_p.y <  p_polygon[j].y);
        const bool c2 = (e.x * w.y > e.y * w.x);
        if ((c0 && c1 && c2) || (!c0 && !c1 && !c2)) {
            sign = -sign;
        }
    }

    return sign * std::sqrt(d_sq);
}

template <FloatingPointScalar T>
T SDFMath<T>::sd_capsule_3d(
    const Vec3& p_p,
    const Vec3& p_a,
    const Vec3& p_b,
    T p_radius
) noexcept {
    const Vec3 pa = p_p - p_a;
    const Vec3 ba = p_b - p_a;
    const T denom = glm::dot(ba, ba);
    const T h = (denom > GeometryConstants<T>::SQ_EPSILON)
        ? std::clamp(glm::dot(pa, ba) / denom, static_cast<T>(0.0), static_cast<T>(1.0))
        : static_cast<T>(0.0);
    return glm::length(pa - ba * h) - p_radius;
}

template <FloatingPointScalar T>
T SDFMath<T>::sd_torus_3d(
    const Vec3& p_p,
    T p_major_radius,
    T p_minor_radius
) noexcept {
    const Vec2 q(glm::length(Vec2(p_p.x, p_p.z)) - p_major_radius, p_p.y);
    return glm::length(q) - p_minor_radius;
}

template <FloatingPointScalar T>
T SDFMath<T>::evaluate_chladni_wave_2d(
    const Vec2& p_normalized_uv,
    T p_mode_n,
    T p_mode_m,
    T p_amp_a,
    T p_amp_b
) noexcept {
    constexpr T pi = GeometryConstants<T>::PI;
    const T term1 = p_amp_a * std::sin(pi * p_mode_n * p_normalized_uv.x) * std::sin(pi * p_mode_m * p_normalized_uv.y);
    const T term2 = p_amp_b * std::sin(pi * p_mode_m * p_normalized_uv.x) * std::sin(pi * p_mode_n * p_normalized_uv.y);
    return term1 + term2;
}

template <FloatingPointScalar T>
T SDFMath<T>::sample_grid_bilinear_2d(
    std::span<const T> p_grid,
    uint32_t p_width,
    uint32_t p_height,
    const Vec2& p_grid_coord
) noexcept {
    if (p_width == 0 || p_height == 0 || p_grid.size() < static_cast<size_t>(p_width) * p_height) {
        return static_cast<T>(0.0);
    }

    const T gx = std::clamp(p_grid_coord.x, static_cast<T>(0.0), static_cast<T>(p_width - 1));
    const T gy = std::clamp(p_grid_coord.y, static_cast<T>(0.0), static_cast<T>(p_height - 1));

    const uint32_t x0 = static_cast<uint32_t>(std::floor(gx));
    const uint32_t y0 = static_cast<uint32_t>(std::floor(gy));
    const uint32_t x1 = std::min(x0 + 1u, p_width - 1u);
    const uint32_t y1 = std::min(y0 + 1u, p_height - 1u);

    const T tx = gx - static_cast<T>(x0);
    const T ty = gy - static_cast<T>(y0);

    const T v00 = p_grid[static_cast<size_t>(y0) * p_width + x0];
    const T v10 = p_grid[static_cast<size_t>(y0) * p_width + x1];
    const T v01 = p_grid[static_cast<size_t>(y1) * p_width + x0];
    const T v11 = p_grid[static_cast<size_t>(y1) * p_width + x1];

    const T vx0 = glm::mix(v00, v10, tx);
    const T vx1 = glm::mix(v01, v11, tx);
    return glm::mix(vx0, vx1, ty);
}

template <FloatingPointScalar T>
typename SDFMath<T>::Vec2 SDFMath<T>::compute_grid_gradient_2d(
    std::span<const T> p_grid,
    uint32_t p_width,
    uint32_t p_height,
    uint32_t p_x,
    uint32_t p_y,
    const Vec2& p_cell_size
) noexcept {
    if (p_width < 2 || p_height < 2 || p_grid.size() < static_cast<size_t>(p_width) * p_height) {
        return Vec2(static_cast<T>(0.0));
    }

    const uint32_t xm = (p_x > 0u) ? (p_x - 1u) : 0u;
    const uint32_t xp = std::min(p_x + 1u, p_width - 1u);
    const uint32_t ym = (p_y > 0u) ? (p_y - 1u) : 0u;
    const uint32_t yp = std::min(p_y + 1u, p_height - 1u);

    const T dx_span = static_cast<T>(xp - xm) * std::max(p_cell_size.x, GeometryConstants<T>::EPSILON);
    const T dy_span = static_cast<T>(yp - ym) * std::max(p_cell_size.y, GeometryConstants<T>::EPSILON);

    const T df_dx = (p_grid[static_cast<size_t>(p_y) * p_width + xp] -
                     p_grid[static_cast<size_t>(p_y) * p_width + xm]) / dx_span;
    const T df_dy = (p_grid[static_cast<size_t>(yp) * p_width + p_x] -
                     p_grid[static_cast<size_t>(ym) * p_width + p_x]) / dy_span;

    return Vec2(df_dx, df_dy);
}

template <FloatingPointScalar T>
std::vector<MarchingSquaresSegment2D<T>> SDFMath<T>::extract_marching_squares_contours(
    std::span<const T> p_grid,
    uint32_t p_width,
    uint32_t p_height,
    T p_iso_value,
    const Vec2& p_origin,
    const Vec2& p_cell_size
) {
    std::vector<MarchingSquaresSegment2D<T>> segments;
    if (p_width < 2 || p_height < 2 || p_grid.size() < static_cast<size_t>(p_width) * p_height) {
        return segments;
    }

    auto interp_edge = [p_iso_value](const Vec2& p0, const Vec2& p1, T v0, T v1) noexcept -> Vec2 {
        const T denom = v1 - v0;
        if (std::abs(denom) <= GeometryConstants<T>::SQ_EPSILON) {
            return (p0 + p1) * static_cast<T>(0.5);
        }
        const T t = std::clamp((p_iso_value - v0) / denom, static_cast<T>(0.0), static_cast<T>(1.0));
        return glm::mix(p0, p1, t);
    };

    for (uint32_t y = 0; y + 1 < p_height; ++y) {
        for (uint32_t x = 0; x + 1 < p_width; ++x) {
            const T v0 = p_grid[static_cast<size_t>(y) * p_width + x];             // Bottom-Left
            const T v1 = p_grid[static_cast<size_t>(y) * p_width + (x + 1)];       // Bottom-Right
            const T v2 = p_grid[static_cast<size_t>(y + 1) * p_width + (x + 1)];   // Top-Right
            const T v3 = p_grid[static_cast<size_t>(y + 1) * p_width + x];         // Top-Left

            uint8_t mask = 0u;
            if (v0 >= p_iso_value) mask |= 1u;
            if (v1 >= p_iso_value) mask |= 2u;
            if (v2 >= p_iso_value) mask |= 4u;
            if (v3 >= p_iso_value) mask |= 8u;

            if (mask == 0u || mask == 15u) continue;

            const Vec2 c0 = p_origin + Vec2(static_cast<T>(x) * p_cell_size.x, static_cast<T>(y) * p_cell_size.y);
            const Vec2 c1 = c0 + Vec2(p_cell_size.x, static_cast<T>(0.0));
            const Vec2 c2 = c0 + p_cell_size;
            const Vec2 c3 = c0 + Vec2(static_cast<T>(0.0), p_cell_size.y);

            // Edge crossings: e0 = bottom (c0-c1), e1 = right (c1-c2), e2 = top (c3-c2), e3 = left (c0-c3)
            const Vec2 e0 = interp_edge(c0, c1, v0, v1);
            const Vec2 e1 = interp_edge(c1, c2, v1, v2);
            const Vec2 e2 = interp_edge(c3, c2, v3, v2);
            const Vec2 e3 = interp_edge(c0, c3, v0, v3);

            auto push_seg = [&](const Vec2& a, const Vec2& b) {
                segments.push_back(MarchingSquaresSegment2D<T>{a, b, x, y});
            };

            switch (mask) {
                case 1u:  case 14u: push_seg(e3, e0); break;
                case 2u:  case 13u: push_seg(e0, e1); break;
                case 3u:  case 12u: push_seg(e3, e1); break;
                case 4u:  case 11u: push_seg(e1, e2); break;
                case 6u:  case 9u:  push_seg(e0, e2); break;
                case 7u:  case 8u:  push_seg(e3, e2); break;
                case 5u: {
                    // Saddle-point disambiguation using bilinear center average
                    const T center_v = static_cast<T>(0.25) * (v0 + v1 + v2 + v3);
                    if (center_v >= p_iso_value) {
                        push_seg(e3, e2);
                        push_seg(e0, e1);
                    } else {
                        push_seg(e3, e0);
                        push_seg(e1, e2);
                    }
                    break;
                }
                case 10u: {
                    const T center_v = static_cast<T>(0.25) * (v0 + v1 + v2 + v3);
                    if (center_v >= p_iso_value) {
                        push_seg(e3, e0);
                        push_seg(e1, e2);
                    } else {
                        push_seg(e3, e2);
                        push_seg(e0, e1);
                    }
                    break;
                }
                default: break;
            }
        }
    }

    return segments;
}

// Explicit Instantiations
template struct SDFMath<float>;
template struct SDFMath<double>;

} // namespace ideam::core