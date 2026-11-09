#include "polygon_math_2d.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

template <FloatingPointScalar T>
bool PolygonMath2D<T>::contains_point(std::span<const Vec2> p_polygon, const Vec2& p_point) noexcept {
    if (p_polygon.size() < 3) return false;

    bool inside = false;
    for (size_t i = 0, j = p_polygon.size() - 1; i < p_polygon.size(); j = i++) {
        const Vec2& pi = p_polygon[i];
        const Vec2& pj = p_polygon[j];

        if (((pi.y <= p_point.y && p_point.y < pj.y) || (pj.y <= p_point.y && p_point.y < pi.y)) &&
            (p_point.x < (pj.x - pi.x) * (p_point.y - pi.y) / (pj.y - pi.y != static_cast<T>(0.0) ? pj.y - pi.y : static_cast<T>(1.0)) + pi.x)) {
            inside = !inside;
        }
    }
    return inside;
}

template <FloatingPointScalar T>
T PolygonMath2D<T>::area(std::span<const Vec2> p_polygon) noexcept {
    if (p_polygon.size() < 3) return static_cast<T>(0.0);

    T a = static_cast<T>(0.0);
    for (size_t p = p_polygon.size() - 1, q = 0; q < p_polygon.size(); p = q++) {
        a += (p_polygon[p].x * p_polygon[q].y) - (p_polygon[q].x * p_polygon[p].y);
    }
    return a * static_cast<T>(0.5);
}

template <FloatingPointScalar T>
T PolygonMath2D<T>::perimeter(std::span<const Vec2> p_polygon, bool p_closed) noexcept {
    if (p_polygon.size() < 2) return static_cast<T>(0.0);

    T total = static_cast<T>(0.0);
    for (size_t i = 0; i + 1 < p_polygon.size(); ++i) {
        total += glm::distance(p_polygon[i], p_polygon[i + 1]);
    }
    if (p_closed && p_polygon.size() > 2) {
        total += glm::distance(p_polygon.back(), p_polygon.front());
    }
    return total;
}

template <FloatingPointScalar T>
typename PolygonMath2D<T>::Vec2 PolygonMath2D<T>::centroid(std::span<const Vec2> p_polygon) noexcept {
    if (p_polygon.empty()) return Vec2(static_cast<T>(0.0));
    if (p_polygon.size() == 1) return p_polygon[0];
    if (p_polygon.size() == 2) return (p_polygon[0] + p_polygon[1]) * static_cast<T>(0.5);

    T twice_area = static_cast<T>(0.0);
    Vec2 accum(static_cast<T>(0.0));

    for (size_t p = p_polygon.size() - 1, q = 0; q < p_polygon.size(); p = q++) {
        const T cross = (p_polygon[p].x * p_polygon[q].y) - (p_polygon[q].x * p_polygon[p].y);
        twice_area += cross;
        accum += (p_polygon[p] + p_polygon[q]) * cross;
    }

    if (std::abs(twice_area) > GeometryConstants<T>::EPSILON) {
        return accum / (static_cast<T>(3.0) * twice_area);
    }

    // Fallback to arithmetic mean for collinear/degenerate polygons
    Vec2 mean(static_cast<T>(0.0));
    for (const auto& pt : p_polygon) {
        mean += pt;
    }
    return mean / static_cast<T>(p_polygon.size());
}

template <FloatingPointScalar T>
AABB2D<T> PolygonMath2D<T>::compute_aabb(std::span<const Vec2> p_points) noexcept {
    AABB2D<T> box;
    for (const auto& pt : p_points) {
        box.expand_to(pt);
    }
    return box;
}

template <FloatingPointScalar T>
WindingOrder PolygonMath2D<T>::get_winding_order(std::span<const Vec2> p_polygon) noexcept {
    const T s_area = area(p_polygon);
    if (s_area > GeometryConstants<T>::EPSILON) return WindingOrder::COUNTER_CLOCKWISE;
    if (s_area < -GeometryConstants<T>::EPSILON) return WindingOrder::CLOCKWISE;
    return WindingOrder::COLINEAR;
}

template <FloatingPointScalar T>
void PolygonMath2D<T>::ensure_winding(std::span<Vec2> r_polygon, WindingOrder p_desired) noexcept {
    if (r_polygon.size() < 3 || p_desired == WindingOrder::COLINEAR) return;
    const WindingOrder current = get_winding_order(r_polygon);
    if (current != WindingOrder::COLINEAR && current != p_desired) {
        std::reverse(r_polygon.begin(), r_polygon.end());
    }
}

template <FloatingPointScalar T>
bool PolygonMath2D<T>::line_intersection(
    const Vec2& p_a1,
    const Vec2& p_a2,
    const Vec2& p_b1,
    const Vec2& p_b2,
    Vec2& r_intersect,
    T* r_t_a,
    T* r_t_b
) noexcept {
    const Vec2 s1 = p_a2 - p_a1;
    const Vec2 s2 = p_b2 - p_b1;

    const T determinant = (-s2.x * s1.y + s1.x * s2.y);
    if (std::abs(determinant) < GeometryConstants<T>::EPSILON) {
        return false; // Collinear or parallel
    }

    const T inv_det = static_cast<T>(1.0) / determinant;
    const T s = (-s1.y * (p_a1.x - p_b1.x) + s1.x * (p_a1.y - p_b1.y)) * inv_det;
    const T t = ( s2.x * (p_a1.y - p_b1.y) - s2.y * (p_a1.x - p_b1.x)) * inv_det;

    if (s >= static_cast<T>(0.0) && s <= static_cast<T>(1.0) &&
        t >= static_cast<T>(0.0) && t <= static_cast<T>(1.0)) {
        r_intersect = p_a1 + (t * s1);
        if (r_t_a) *r_t_a = t;
        if (r_t_b) *r_t_b = s;
        return true;
    }
    return false;
}

template <FloatingPointScalar T>
typename PolygonMath2D<T>::Vec2 PolygonMath2D<T>::closest_point_on_segment(
    const Vec2& p_a,
    const Vec2& p_b,
    const Vec2& p_point,
    T* r_t
) noexcept {
    const Vec2 ab = p_b - p_a;
    const T len_sq = glm::dot(ab, ab);
    if (len_sq <= GeometryConstants<T>::SQ_EPSILON) {
        if (r_t) *r_t = static_cast<T>(0.0);
        return p_a;
    }
    const T t = std::clamp(glm::dot(p_point - p_a, ab) / len_sq, static_cast<T>(0.0), static_cast<T>(1.0));
    if (r_t) *r_t = t;
    return p_a + ab * t;
}

template <FloatingPointScalar T>
T PolygonMath2D<T>::distance_to_boundary(
    std::span<const Vec2> p_polygon,
    const Vec2& p_point,
    Vec2* r_closest_point,
    size_t* r_edge_index,
    bool p_closed
) noexcept {
    if (p_polygon.empty()) return static_cast<T>(0.0);
    if (p_polygon.size() == 1) {
        if (r_closest_point) *r_closest_point = p_polygon[0];
        if (r_edge_index) *r_edge_index = 0;
        return glm::distance(p_point, p_polygon[0]);
    }

    T best_dist_sq = std::numeric_limits<T>::max();
    Vec2 best_pt   = p_polygon[0];
    size_t best_e  = 0;

    const size_t edge_count = p_closed ? p_polygon.size() : (p_polygon.size() - 1);
    for (size_t i = 0; i < edge_count; ++i) {
        const Vec2& a = p_polygon[i];
        const Vec2& b = p_polygon[(i + 1) % p_polygon.size()];
        const Vec2 candidate = closest_point_on_segment(a, b, p_point);
        const Vec2 diff = p_point - candidate;
        const T d_sq = glm::dot(diff, diff);
        if (d_sq < best_dist_sq) {
            best_dist_sq = d_sq;
            best_pt = candidate;
            best_e = i;
        }
    }

    if (r_closest_point) *r_closest_point = best_pt;
    if (r_edge_index) *r_edge_index = best_e;
    return std::sqrt(best_dist_sq);
}

template <FloatingPointScalar T>
bool PolygonMath2D<T>::is_point_in_triangle(
    const Vec2& p_a,
    const Vec2& p_b,
    const Vec2& p_c,
    const Vec2& p_p
) noexcept {
    const T cx = p_p.x, cy = p_p.y;
    const T ax = p_a.x, ay = p_a.y;
    const T bx = p_b.x, by = p_b.y;
    const T ccx = p_c.x, ccy = p_c.y;

    const T w0 = (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
    const T w1 = (cx - bx) * (ccy - by) - (cy - by) * (ccx - bx);
    const T w2 = (cx - ccx) * (ay - ccy) - (cy - ccy) * (ax - ccx);

    return (w0 >= static_cast<T>(0.0) && w1 >= static_cast<T>(0.0) && w2 >= static_cast<T>(0.0)) ||
           (w0 <= static_cast<T>(0.0) && w1 <= static_cast<T>(0.0) && w2 <= static_cast<T>(0.0));
}

template <FloatingPointScalar T>
bool PolygonMath2D<T>::snip(
    int u,
    int v,
    int w,
    int n,
    const std::vector<int>& V,
    std::span<const Vec2> p_polygon
) noexcept {
    const Vec2 a = p_polygon[V[u]];
    const Vec2 b = p_polygon[V[v]];
    const Vec2 c = p_polygon[V[w]];

    if (GeometryConstants<T>::EPSILON > (((b.x - a.x) * (c.y - a.y)) - ((b.y - a.y) * (c.x - a.x)))) {
        return false;
    }

    for (int p = 0; p < n; p++) {
        if ((p == u) || (p == v) || (p == w)) continue;
        if (is_point_in_triangle(a, b, c, p_polygon[V[p]])) return false;
    }
    return true;
}

template <FloatingPointScalar T>
bool PolygonMath2D<T>::triangulate_into(std::span<const Vec2> p_polygon, std::vector<uint32_t>& r_out_indices) {
    r_out_indices.clear();
    int n = static_cast<int>(p_polygon.size());
    if (n < 3) return false;

    // Drop duplicate closing vertex if it overlaps index 0 (common in wrapped Spline2D)
    if (glm::distance(p_polygon.front(), p_polygon.back()) < GeometryConstants<T>::WELD_EPSILON) {
        n -= 1;
    }
    if (n < 3) return false;

    r_out_indices.reserve(static_cast<size_t>(n - 2) * 3);

    std::vector<int> V(n);
    if (area(p_polygon.subspan(0, n)) > static_cast<T>(0.0)) {
        for (int v = 0; v < n; v++) V[v] = v;
    } else {
        for (int v = 0; v < n; v++) V[v] = (n - 1) - v;
    }

    int nv = n;
    int count = 2 * n;

    for (int v = nv - 1; nv > 2;) {
        if ((count--) <= 0) return false; // Fallback on self-intersecting / degenerate contour

        int u = v; if (nv <= u) u = 0;
        v = u + 1; if (nv <= v) v = 0;
        int w = v + 1; if (nv <= w) w = 0;

        if (snip(u, v, w, nv, V, p_polygon)) {
            r_out_indices.push_back(static_cast<uint32_t>(V[u]));
            r_out_indices.push_back(static_cast<uint32_t>(V[v]));
            r_out_indices.push_back(static_cast<uint32_t>(V[w]));

            for (int s = v, t = v + 1; t < nv; s++, t++) {
                V[s] = V[t];
            }
            nv--;
            count = 2 * nv;
        }
    }
    return true;
}

template <FloatingPointScalar T>
std::vector<uint32_t> PolygonMath2D<T>::triangulate(std::span<const Vec2> p_polygon) {
    std::vector<uint32_t> indices;
    triangulate_into(p_polygon, indices);
    return indices;
}

template <FloatingPointScalar T>
std::vector<typename PolygonMath2D<T>::Vec2> PolygonMath2D<T>::convex_hull(std::span<const Vec2> p_points) {
    if (p_points.size() <= 2) {
        return std::vector<Vec2>(p_points.begin(), p_points.end());
    }

    std::vector<Vec2> sorted(p_points.begin(), p_points.end());
    std::sort(sorted.begin(), sorted.end(), [](const Vec2& a, const Vec2& b) noexcept {
        return (a.x < b.x) || (a.x == b.x && a.y < b.y);
    });

    std::vector<Vec2> hull;
    hull.reserve(sorted.size() * 2);

    // Lower hull
    for (const auto& pt : sorted) {
        while (hull.size() >= 2 &&
               GeometryKernelMath<T>::cross_2d(hull[hull.size() - 1] - hull[hull.size() - 2], pt - hull[hull.size() - 2]) <= static_cast<T>(0.0)) {
            hull.pop_back();
        }
        hull.push_back(pt);
    }

    // Upper hull
    const size_t lower_size = hull.size() + 1;
    for (size_t i = sorted.size() - 1; i > 0; --i) {
        const Vec2& pt = sorted[i - 1];
        while (hull.size() >= lower_size &&
               GeometryKernelMath<T>::cross_2d(hull[hull.size() - 1] - hull[hull.size() - 2], pt - hull[hull.size() - 2]) <= static_cast<T>(0.0)) {
            hull.pop_back();
        }
        hull.push_back(pt);
    }

    if (hull.size() > 1) {
        hull.pop_back(); // Remove duplicate start/end point
    }
    return hull;
}

template <FloatingPointScalar T>
std::vector<typename PolygonMath2D<T>::Vec2> PolygonMath2D<T>::offset_polygon(
    std::span<const Vec2> p_polygon,
    T p_delta,
    JoinStyle2D p_join,
    T p_miter_limit
) {
    if (p_polygon.size() < 3) {
        return std::vector<Vec2>(p_polygon.begin(), p_polygon.end());
    }

    // Strip duplicate closing vertex if present
    size_t n = p_polygon.size();
    if (n > 3 && glm::distance(p_polygon.front(), p_polygon.back()) < GeometryConstants<T>::WELD_EPSILON) {
        n -= 1;
    }

    const std::span<const Vec2> ring = p_polygon.subspan(0, n);
    if (std::abs(p_delta) <= GeometryConstants<T>::EPSILON) {
        return std::vector<Vec2>(ring.begin(), ring.end());
    }

    // Determine polygon winding so outward normal always points toward exterior
    // For CCW winding (signed area > 0), outward unit normal of edge (dx, dy) is (dy, -dx) / len.
    const T signed_a = area(ring);
    const T outward_sign = (signed_a >= static_cast<T>(0.0)) ? static_cast<T>(1.0) : static_cast<T>(-1.0);

    std::vector<Vec2> edge_normals(n);
    for (size_t i = 0; i < n; ++i) {
        const Vec2 edge = ring[(i + 1) % n] - ring[i];
        const T len_sq = glm::dot(edge, edge);
        if (len_sq > GeometryConstants<T>::SQ_EPSILON) {
            const T inv_len = static_cast<T>(1.0) / std::sqrt(len_sq);
            edge_normals[i] = Vec2(edge.y, -edge.x) * (inv_len * outward_sign);
        } else {
            edge_normals[i] = Vec2(static_cast<T>(0.0), static_cast<T>(1.0));
        }
    }

    const T max_miter = std::max(static_cast<T>(1.0), p_miter_limit);
    std::vector<Vec2> result;
    result.reserve(n * 2);

    for (size_t i = 0; i < n; ++i) {
        const Vec2& n_prev = edge_normals[(i + n - 1) % n];
        const Vec2& n_curr = edge_normals[i];
        const Vec2& v      = ring[i];

        // Cosine of angle between adjacent normals
        const T cos_theta = std::clamp(glm::dot(n_prev, n_curr), static_cast<T>(-1.0), static_cast<T>(1.0));
        const T denom     = static_cast<T>(1.0) + cos_theta; // = 2 * cos^2(theta / 2)

        // Check if corner is convex relative to offset direction
        const T cross_n = GeometryKernelMath<T>::cross_2d(n_prev, n_curr) * outward_sign *
                          (p_delta >= static_cast<T>(0.0) ? static_cast<T>(1.0) : static_cast<T>(-1.0));
        const bool is_outer_turn = (cross_n < -GeometryConstants<T>::EPSILON);

        if (p_join == JoinStyle2D::BEVEL && is_outer_turn) {
            result.push_back(v + n_prev * p_delta);
            result.push_back(v + n_curr * p_delta);
            continue;
        }

        if (p_join == JoinStyle2D::ROUND && is_outer_turn) {
            const T a0 = std::atan2(n_prev.y, n_prev.x);
            T a1       = std::atan2(n_curr.y, n_curr.x);
            T diff     = a1 - a0;
            while (diff >  GeometryConstants<T>::PI) diff -= GeometryConstants<T>::TAU;
            while (diff < -GeometryConstants<T>::PI) diff += GeometryConstants<T>::TAU;

            const int steps = std::clamp(static_cast<int>(std::ceil(std::abs(diff) / static_cast<T>(0.35))), 2, 8);
            for (int s = 0; s <= steps; ++s) {
                const T t = static_cast<T>(s) / static_cast<T>(steps);
                const T ang = a0 + diff * t;
                result.push_back(v + Vec2(std::cos(ang), std::sin(ang)) * p_delta);
            }
            continue;
        }

        // Default MITER (or inner reflex corner)
        if (denom > GeometryConstants<T>::EPSILON) {
            const T miter_scale = std::sqrt(static_cast<T>(2.0) / denom);
            const Vec2 bisector = glm::normalize(n_prev + n_curr);
            if (miter_scale <= max_miter || !is_outer_turn) {
                result.push_back(v + bisector * (p_delta * std::min(miter_scale, max_miter)));
            } else {
                // Miter limit exceeded on outer turn -> bevel fallback
                result.push_back(v + n_prev * p_delta);
                result.push_back(v + n_curr * p_delta);
            }
        } else {
            // 180-degree spike fallback
            result.push_back(v + n_prev * p_delta);
            result.push_back(v + n_curr * p_delta);
        }
    }

    return result;
}

template <FloatingPointScalar T>
std::vector<typename PolygonMath2D<T>::Vec2> PolygonMath2D<T>::generate_rectangle_vertices(
    const Vec2& p_center,
    const Vec2& p_size
) {
    const T half_x = p_size.x * static_cast<T>(0.5);
    const T half_y = p_size.y * static_cast<T>(0.5);
    return {
        Vec2(p_center.x - half_x, p_center.y - half_y),
        Vec2(p_center.x - half_x, p_center.y + half_y),
        Vec2(p_center.x + half_x, p_center.y + half_y),
        Vec2(p_center.x + half_x, p_center.y - half_y)
    };
}

template <FloatingPointScalar T>
std::vector<typename PolygonMath2D<T>::Vec2> PolygonMath2D<T>::generate_regular_polygon_vertices(
    int p_sides,
    T p_radius,
    T p_angle_offset,
    bool p_reverse_winding
) {
    if (p_sides < 3) p_sides = 3;
    std::vector<Vec2> verts;
    verts.reserve(static_cast<size_t>(p_sides));

    const T angle_step = GeometryConstants<T>::TAU / static_cast<T>(p_sides);
    if (p_reverse_winding) {
        for (int i = p_sides; i > 0; --i) {
            const T a = (angle_step * static_cast<T>(i)) + p_angle_offset;
            verts.emplace_back(p_radius * std::cos(a), p_radius * std::sin(a));
        }
    } else {
        for (int i = 0; i < p_sides; ++i) {
            const T a = (angle_step * static_cast<T>(i)) + p_angle_offset;
            verts.emplace_back(p_radius * std::cos(a), p_radius * std::sin(a));
        }
    }
    return verts;
}

template <FloatingPointScalar T>
std::vector<typename PolygonMath2D<T>::Vec2> PolygonMath2D<T>::generate_star_vertices(
    int p_sides,
    T p_outer_radius,
    T p_inner_radius,
    T p_angle_offset
) {
    if (p_sides < 3) p_sides = 3;
    const int total_points = p_sides * 2;
    std::vector<Vec2> verts;
    verts.reserve(static_cast<size_t>(total_points));

    const T angle_step = GeometryConstants<T>::TAU / static_cast<T>(total_points);
    bool inner = false;
    for (int i = 1; i <= total_points; ++i) {
        const T a = (angle_step * static_cast<T>(i)) + p_angle_offset;
        const T r = inner ? p_inner_radius : p_outer_radius;
        verts.emplace_back(r * std::cos(a), r * std::sin(a));
        inner = !inner;
    }
    return verts;
}

// Explicit Instantiations
template struct PolygonMath2D<float>;
template struct PolygonMath2D<double>;

} // namespace ideam::core