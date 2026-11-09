#include "voronoi_2d.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

template <FloatingPointScalar T>
bool VoronoiMath2D<T>::compute_circumcircle(
    const Vec2& p_a,
    const Vec2& p_b,
    const Vec2& p_c,
    Vec2& r_center,
    T& r_radius_sq
) noexcept {
    const T ax = p_b.x - p_a.x;
    const T ay = p_b.y - p_a.y;
    const T bx = p_c.x - p_a.x;
    const T by = p_c.y - p_a.y;

    const T d = static_cast<T>(2.0) * (ax * by - ay * bx);
    if (std::abs(d) <= GeometryConstants<T>::SQ_EPSILON) {
        r_center = (p_a + p_b + p_c) * static_cast<T>(1.0 / 3.0);
        r_radius_sq = static_cast<T>(0.0);
        return false;
    }

    const T inv_d = static_cast<T>(1.0) / d;
    const T a_sq = ax * ax + ay * ay;
    const T b_sq = bx * bx + by * by;

    const T ux = (by * a_sq - ay * b_sq) * inv_d;
    const T uy = (ax * b_sq - bx * a_sq) * inv_d;

    r_center = Vec2(p_a.x + ux, p_a.y + uy);
    r_radius_sq = ux * ux + uy * uy;
    return true;
}

template <FloatingPointScalar T>
bool VoronoiMath2D<T>::in_circumcircle(
    const Vec2& p_a,
    const Vec2& p_b,
    const Vec2& p_c,
    const Vec2& p_point
) noexcept {
    Vec2 center;
    T rad_sq = static_cast<T>(0.0);
    if (!compute_circumcircle(p_a, p_b, p_c, center, rad_sq)) {
        return false;
    }
    const Vec2 diff = p_point - center;
    return glm::dot(diff, diff) <= (rad_sq * (static_cast<T>(1.0) + GeometryConstants<T>::EPSILON));
}

template <FloatingPointScalar T>
std::vector<uint32_t> VoronoiMath2D<T>::triangulate_delaunay(
    std::span<const Vec2> p_sites,
    std::vector<DelaunayTrianglePOD<T>>* r_out_triangles
) {
    std::vector<uint32_t> indices;
    if (p_sites.size() < 3) return indices;

    const AABB2D<T> bounds = PolygonMath2D<T>::compute_aabb(p_sites);
    const Vec2 center = bounds.get_center();
    const Vec2 sz = bounds.get_size();
    const T max_dim = std::max(std::max(sz.x, sz.y), static_cast<T>(1.0));

    // Create temporary vertex list with 3 super-triangle vertices appended at the end
    const uint32_t n = static_cast<uint32_t>(p_sites.size());
    std::vector<Vec2> all_pts(p_sites.begin(), p_sites.end());
    all_pts.push_back(Vec2(center.x - static_cast<T>(20.0) * max_dim, center.y - static_cast<T>(10.0) * max_dim));
    all_pts.push_back(Vec2(center.x + static_cast<T>(20.0) * max_dim, center.y - static_cast<T>(10.0) * max_dim));
    all_pts.push_back(Vec2(center.x,                                  center.y + static_cast<T>(20.0) * max_dim));

    std::vector<DelaunayTrianglePOD<T>> triangles;
    triangles.reserve(p_sites.size() * 2 + 4);

    DelaunayTrianglePOD<T> super_tri{n, n + 1u, n + 2u};
    if (compute_circumcircle(all_pts[n], all_pts[n + 1u], all_pts[n + 2u], super_tri.circumcenter, super_tri.circumradius_sq)) {
        triangles.push_back(super_tri);
    }

    struct DirectedEdge {
        uint32_t a;
        uint32_t b;
        bool is_shared = false;
    };
    std::vector<DirectedEdge> polygon_hole;
    std::vector<DelaunayTrianglePOD<T>> next_triangles;

    for (uint32_t i = 0; i < n; ++i) {
        const Vec2& pt = all_pts[i];
        polygon_hole.clear();
        next_triangles.clear();

        for (const auto& tri : triangles) {
            const Vec2 diff = pt - tri.circumcenter;
            if (glm::dot(diff, diff) <= tri.circumradius_sq * (static_cast<T>(1.0) + GeometryConstants<T>::EPSILON)) {
                polygon_hole.push_back({tri.v0, tri.v1, false});
                polygon_hole.push_back({tri.v1, tri.v2, false});
                polygon_hole.push_back({tri.v2, tri.v0, false});
            } else {
                next_triangles.push_back(tri);
            }
        }

        // Mark duplicate/shared interior edges of the cavity
        for (size_t e1 = 0; e1 < polygon_hole.size(); ++e1) {
            for (size_t e2 = e1 + 1; e2 < polygon_hole.size(); ++e2) {
                if ((polygon_hole[e1].a == polygon_hole[e2].a && polygon_hole[e1].b == polygon_hole[e2].b) ||
                    (polygon_hole[e1].a == polygon_hole[e2].b && polygon_hole[e1].b == polygon_hole[e2].a)) {
                    polygon_hole[e1].is_shared = true;
                    polygon_hole[e2].is_shared = true;
                }
            }
        }

        // Retriangulate cavity boundary with site i
        for (const auto& edge : polygon_hole) {
            if (edge.is_shared) continue;
            uint32_t u = edge.a;
            uint32_t v = edge.b;
            if (orient2d(all_pts[u], all_pts[v], pt) < static_cast<T>(0.0)) {
                std::swap(u, v);
            }
            DelaunayTrianglePOD<T> new_tri{u, v, i};
            if (compute_circumcircle(all_pts[u], all_pts[v], pt, new_tri.circumcenter, new_tri.circumradius_sq)) {
                next_triangles.push_back(new_tri);
            }
        }

        triangles.swap(next_triangles);
    }

    // Filter out any triangles connected to the super-triangle vertices [n, n+1, n+2]
    if (r_out_triangles) r_out_triangles->clear();
    indices.reserve(triangles.size() * 3);

    for (const auto& tri : triangles) {
        if (tri.v0 >= n || tri.v1 >= n || tri.v2 >= n) continue;
        indices.push_back(tri.v0);
        indices.push_back(tri.v1);
        indices.push_back(tri.v2);
        if (r_out_triangles) {
            r_out_triangles->push_back(tri);
        }
    }

    return indices;
}

template <FloatingPointScalar T>
std::vector<VoronoiCell2D<T>> VoronoiMath2D<T>::build_clipped_voronoi_cells(
    std::span<const Vec2> p_sites,
    const AABB2D<T>& p_bounds
) {
    std::vector<VoronoiCell2D<T>> cells;
    if (p_sites.empty() || !p_bounds.is_valid()) return cells;

    const size_t n = p_sites.size();
    cells.resize(n);

    // Build Delaunay triangulation first to get exact dual neighbor candidates
    const std::vector<uint32_t> tri_indices = triangulate_delaunay(p_sites);
    std::vector<uint32_t> row_offsets;
    std::vector<uint32_t> col_neighbors;
    extract_delaunay_adjacency_csr(n, tri_indices, row_offsets, col_neighbors);

    const std::vector<Vec2> base_box = {
        Vec2(p_bounds.min_bounds.x, p_bounds.min_bounds.y),
        Vec2(p_bounds.max_bounds.x, p_bounds.min_bounds.y),
        Vec2(p_bounds.max_bounds.x, p_bounds.max_bounds.y),
        Vec2(p_bounds.min_bounds.x, p_bounds.max_bounds.y)
    };

    for (size_t i = 0; i < n; ++i) {
        cells[i].site_index = static_cast<uint32_t>(i);
        cells[i].site_position = p_sites[i];
        std::vector<Vec2> poly = base_box;

        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;
            const Vec2 mid = (p_sites[i] + p_sites[j]) * static_cast<T>(0.5);
            const Vec2 inward_normal = p_sites[i] - p_sites[j]; // Points toward site i
            if (glm::dot(inward_normal, inward_normal) <= GeometryConstants<T>::SQ_EPSILON) continue;

            poly = CSGMath2D<T>::clip_polygon_half_plane(poly, mid, inward_normal);
            if (poly.size() < 3) break;
        }

        cells[i].polygon_vertices = std::move(poly);

        if (i + 1 < row_offsets.size()) {
            const uint32_t start = row_offsets[i];
            const uint32_t end   = row_offsets[i + 1];
            cells[i].neighbor_site_indices.assign(
                col_neighbors.begin() + start,
                col_neighbors.begin() + end
            );
        }
    }

    return cells;
}

template <FloatingPointScalar T>
void VoronoiMath2D<T>::extract_delaunay_adjacency_csr(
    size_t p_site_count,
    std::span<const uint32_t> p_triangle_indices,
    std::vector<uint32_t>& r_row_offsets,
    std::vector<uint32_t>& r_col_neighbors
) {
    r_row_offsets.assign(p_site_count + 1, 0u);
    r_col_neighbors.clear();
    if (p_site_count == 0) return;

    std::vector<std::vector<uint32_t>> adj(p_site_count);
    for (size_t t = 0; t + 2 < p_triangle_indices.size(); t += 3) {
        const uint32_t i0 = p_triangle_indices[t];
        const uint32_t i1 = p_triangle_indices[t + 1];
        const uint32_t i2 = p_triangle_indices[t + 2];
        if (i0 >= p_site_count || i1 >= p_site_count || i2 >= p_site_count) continue;

        adj[i0].push_back(i1); adj[i0].push_back(i2);
        adj[i1].push_back(i0); adj[i1].push_back(i2);
        adj[i2].push_back(i0); adj[i2].push_back(i1);
    }

    uint32_t cursor = 0u;
    for (size_t i = 0; i < p_site_count; ++i) {
        r_row_offsets[i] = cursor;
        auto& list = adj[i];
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
        for (uint32_t nb : list) {
            r_col_neighbors.push_back(nb);
            ++cursor;
        }
    }
    r_row_offsets[p_site_count] = cursor;
}

// Explicit Instantiations
template struct VoronoiMath2D<float>;
template struct VoronoiMath2D<double>;

} // namespace ideam::core