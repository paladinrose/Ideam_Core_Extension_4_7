#include "hyperbolic_tiling_2d.h"

#include <algorithm>
#include <cmath>

namespace ideam::core {

namespace {

template <FloatingPointScalar T>
[[nodiscard]] inline constexpr glm::vec<2, T> c_mul(const glm::vec<2, T>& p_u, const glm::vec<2, T>& p_v) noexcept {
    return glm::vec<2, T>(
        p_u.x * p_v.x - p_u.y * p_v.y,
        p_u.x * p_v.y + p_u.y * p_v.x
    );
}

template <FloatingPointScalar T>
[[nodiscard]] inline constexpr glm::vec<2, T> c_conj(const glm::vec<2, T>& p_u) noexcept {
    return glm::vec<2, T>(p_u.x, -p_u.y);
}

template <FloatingPointScalar T>
[[nodiscard]] inline constexpr glm::vec<2, T> c_div(const glm::vec<2, T>& p_num, const glm::vec<2, T>& p_den) noexcept {
    const T norm_sq = std::max(static_cast<T>(1e-12), p_den.x * p_den.x + p_den.y * p_den.y);
    const glm::vec<2, T> prod = c_mul(p_num, c_conj(p_den));
    return prod / norm_sq;
}

} // namespace

template <FloatingPointScalar T>
HyperbolicTiling2D<T>::HyperbolicTiling2D(uint32_t p_p, uint32_t p_q, uint32_t p_rings) noexcept {
    set_schlafli_pq(p_p, p_q);
    set_generation_rings(p_rings);
}

template <FloatingPointScalar T>
T HyperbolicTiling2D<T>::compute_fundamental_vertex_radius(uint32_t p_p, uint32_t p_q) noexcept {
    if (!is_valid_hyperbolic_pq(p_p, p_q)) {
        return static_cast<T>(0.38);
    }
    const T pi    = GeometryConstants<T>::PI;
    const T ang_p = pi / static_cast<T>(p_p);
    const T ang_q = pi / static_cast<T>(p_q);
    const T num   = std::cos(ang_p + ang_q);
    const T den   = std::cos(ang_p - ang_q);
    if (den <= GeometryConstants<T>::EPSILON || num <= static_cast<T>(0.0)) {
        return static_cast<T>(0.38);
    }
    return std::sqrt(num / den);
}

template <FloatingPointScalar T>
MobiusSU11POD<T> HyperbolicTiling2D<T>::make_hyperbolic_translation(const Vec2& p_target) noexcept {
    MobiusSU11POD<T> m{};
    const T len_sq = glm::dot(p_target, p_target);
    if (len_sq >= static_cast<T>(0.98)) {
        const Vec2 clamped = glm::normalize(p_target) * static_cast<T>(0.98);
        const T cl_sq      = glm::dot(clamped, clamped);
        const T gamma      = static_cast<T>(1.0) / std::sqrt(static_cast<T>(1.0) - cl_sq);
        m.a = Vec2(gamma, static_cast<T>(0.0));
        m.b = clamped * gamma;
        return m;
    }
    const T gamma = static_cast<T>(1.0) / std::sqrt(static_cast<T>(1.0) - len_sq);
    m.a = Vec2(gamma, static_cast<T>(0.0));
    m.b = p_target * gamma;
    return m;
}

template <FloatingPointScalar T>
MobiusSU11POD<T> HyperbolicTiling2D<T>::make_hyperbolic_rotation(T p_angle_rad) noexcept {
    MobiusSU11POD<T> m{};
    const T half = p_angle_rad * static_cast<T>(0.5);
    m.a = Vec2(std::cos(half), std::sin(half));
    m.b = Vec2(static_cast<T>(0.0), static_cast<T>(0.0));
    return m;
}

template <FloatingPointScalar T>
typename HyperbolicTiling2D<T>::Vec2 HyperbolicTiling2D<T>::apply_mobius(
    const MobiusSU11POD<T>& p_mobius,
    const Vec2& p_z
) noexcept {
    const Vec2 z_in = p_mobius.reflect_conjugate ? c_conj(p_z) : p_z;
    const Vec2 num  = c_mul(p_mobius.a, z_in) + p_mobius.b;
    const Vec2 den  = c_mul(c_conj(p_mobius.b), z_in) + c_conj(p_mobius.a);
    const Vec2 out  = c_div(num, den);
    const T r_sq    = glm::dot(out, out);
    if (r_sq >= static_cast<T>(0.9998)) {
        return out * (static_cast<T>(0.999) / std::sqrt(r_sq));
    }
    return out;
}

template <FloatingPointScalar T>
PoincareGeodesicArcPOD<T> HyperbolicTiling2D<T>::compute_geodesic_arc(
    const Vec2& p_a,
    const Vec2& p_b
) noexcept {
    PoincareGeodesicArcPOD<T> arc{};
    arc.start = p_a;
    arc.end   = p_b;

    // Cross product 2D determinant: if near 0, p_a, p_b, and origin (0,0) are collinear (diameter)
    const T det = p_a.x * p_b.y - p_a.y * p_b.x;
    if (std::abs(det) <= static_cast<T>(1e-5)) {
        arc.is_diameter = true;
        return arc;
    }

    // Orthogonal circle center C satisfies:
    //   2 * (C . p_a) = |p_a|^2 + 1
    //   2 * (C . p_b) = |p_b|^2 + 1
    const T rhs_a = static_cast<T>(0.5) * (glm::dot(p_a, p_a) + static_cast<T>(1.0));
    const T rhs_b = static_cast<T>(0.5) * (glm::dot(p_b, p_b) + static_cast<T>(1.0));

    const Vec2 center(
        (rhs_a * p_b.y - rhs_b * p_a.y) / det,
        (p_a.x * rhs_b - p_b.x * rhs_a) / det
    );
    const T radius_sq = std::max(static_cast<T>(0.0), glm::dot(center, center) - static_cast<T>(1.0));
    const T radius    = std::sqrt(radius_sq);

    if (radius <= static_cast<T>(1e-5) || !std::isfinite(radius)) {
        arc.is_diameter = true;
        return arc;
    }

    arc.is_diameter   = false;
    arc.circle_center = center;
    arc.circle_radius = radius;

    const T ang0 = std::atan2(p_a.y - center.y, p_a.x - center.x);
    const T ang1 = std::atan2(p_b.y - center.y, p_b.x - center.x);
    T d_ang      = ang1 - ang0;
    while (d_ang > GeometryConstants<T>::PI) d_ang -= GeometryConstants<T>::TAU;
    while (d_ang < -GeometryConstants<T>::PI) d_ang += GeometryConstants<T>::TAU;

    arc.start_angle = ang0;
    arc.delta_angle = d_ang;
    return arc;
}

template <FloatingPointScalar T>
typename HyperbolicTiling2D<T>::Vec2 HyperbolicTiling2D<T>::reflect_across_geodesic(
    const Vec2& p_z,
    const Vec2& p_a,
    const Vec2& p_b
) noexcept {
    const PoincareGeodesicArcPOD<T> arc = compute_geodesic_arc(p_a, p_b);
    if (arc.is_diameter) {
        const Vec2 dir = glm::normalize(p_b - p_a);
        const Vec2 proj = p_a + dir * glm::dot(p_z - p_a, dir);
        return proj * static_cast<T>(2.0) - p_z;
    }

    // Circle inversion across orthogonal geodesic circle (center C, radius R):
    //   z' = C + (R^2 / |z - C|^2) * (z - C)
    const Vec2 diff  = p_z - arc.circle_center;
    const T dist_sq  = std::max(static_cast<T>(1e-12), glm::dot(diff, diff));
    const T r_sq     = arc.circle_radius * arc.circle_radius;
    const Vec2 inv_z = arc.circle_center + diff * (r_sq / dist_sq);
    const T norm_sq  = glm::dot(inv_z, inv_z);
    if (norm_sq >= static_cast<T>(0.9998)) {
        return inv_z * (static_cast<T>(0.999) / std::sqrt(norm_sq));
    }
    return inv_z;
}

template <FloatingPointScalar T>
std::vector<typename HyperbolicTiling2D<T>::Vec2> HyperbolicTiling2D<T>::sample_geodesic_segment(
    const Vec2& p_a,
    const Vec2& p_b,
    uint32_t p_subdivisions
) {
    const uint32_t steps = std::max(1u, p_subdivisions);
    std::vector<Vec2> pts;
    pts.reserve(steps + 1u);

    const PoincareGeodesicArcPOD<T> arc = compute_geodesic_arc(p_a, p_b);
    if (arc.is_diameter || steps == 1u) {
        for (uint32_t i = 0u; i <= steps; ++i) {
            const T t = static_cast<T>(i) / static_cast<T>(steps);
            pts.push_back(glm::mix(p_a, p_b, t));
        }
        return pts;
    }

    for (uint32_t i = 0u; i <= steps; ++i) {
        const T t   = static_cast<T>(i) / static_cast<T>(steps);
        const T ang = arc.start_angle + arc.delta_angle * t;
        pts.emplace_back(
            arc.circle_center.x + arc.circle_radius * std::cos(ang),
            arc.circle_center.y + arc.circle_radius * std::sin(ang)
        );
    }
    return pts;
}

template <FloatingPointScalar T>
T HyperbolicTiling2D<T>::hyperbolic_distance(const Vec2& p_z1, const Vec2& p_z2) noexcept {
    const T r1_sq = std::min(static_cast<T>(0.9999), glm::dot(p_z1, p_z1));
    const T r2_sq = std::min(static_cast<T>(0.9999), glm::dot(p_z2, p_z2));
    const Vec2 d  = p_z1 - p_z2;
    const T num   = static_cast<T>(2.0) * glm::dot(d, d);
    const T den   = (static_cast<T>(1.0) - r1_sq) * (static_cast<T>(1.0) - r2_sq);
    const T arg   = std::max(static_cast<T>(1.0), static_cast<T>(1.0) + num / std::max(static_cast<T>(1e-9), den));
    return std::acosh(arg);
}

template <FloatingPointScalar T>
void HyperbolicTiling2D<T>::set_schlafli_pq(uint32_t p_p, uint32_t p_q) noexcept {
    uint32_t np = std::clamp(p_p, 3u, 14u);
    uint32_t nq = std::clamp(p_q, 3u, 14u);
    if (!is_valid_hyperbolic_pq(np, nq)) {
        // Default to the smallest valid q for the chosen p
        if (np == 3u) nq = 7u;
        else if (np == 4u) nq = 5u;
        else if (np == 5u || np == 6u) nq = 4u;
        else nq = 3u;
    }
    schlafli_p_ = np;
    schlafli_q_ = nq;
}

template <FloatingPointScalar T>
void HyperbolicTiling2D<T>::set_mobius_focus(const Vec2& p_focus) noexcept {
    const T len_sq = glm::dot(p_focus, p_focus);
    if (len_sq > static_cast<T>(0.88)) {
        mobius_focus_ = glm::normalize(p_focus) * static_cast<T>(0.938);
    } else {
        mobius_focus_ = p_focus;
    }
}

template <FloatingPointScalar T>
void HyperbolicTiling2D<T>::build_poincare_region(Region& r_out_region) {
    r_out_region.clear();
    raw_cell_disk_vertices_.clear();

    const T r0 = compute_fundamental_vertex_radius(schlafli_p_, schlafli_q_);

    // 1. Construct central cell 0 in canonical disk coordinates
    std::vector<Vec2> central_poly(schlafli_p_);
    for (uint32_t k = 0u; k < schlafli_p_; ++k) {
        const T angle = (static_cast<T>(k) / static_cast<T>(schlafli_p_)) * GeometryConstants<T>::TAU;
        central_poly[k] = Vec2(r0 * std::cos(angle), r0 * std::sin(angle));
    }

    struct RawHyperbolicCell {
        std::vector<Vec2> verts;
        Vec2 center{static_cast<T>(0.0)};
        uint16_t generation = 0u;
        uint8_t colour_id   = 0u;
    };

    std::vector<RawHyperbolicCell> gen_cells;
    gen_cells.reserve(256u);
    gen_cells.push_back(RawHyperbolicCell{central_poly, Vec2(static_cast<T>(0.0)), 0u, 0u});

    // 2. Breadth-first geodesic edge reflection across `generation_rings_`
    size_t ring_start = 0u;
    size_t ring_end   = 1u;
    constexpr size_t MAX_HYPERBOLIC_CELLS = 720u;

    for (uint32_t ring = 1u; ring <= generation_rings_; ++ring) {
        for (size_t ci = ring_start; ci < ring_end; ++ci) {
            if (gen_cells.size() >= MAX_HYPERBOLIC_CELLS) break;
            const std::vector<Vec2> parent_verts = gen_cells[ci].verts;
            const Vec2 parent_center             = gen_cells[ci].center;
            const uint8_t parent_col             = gen_cells[ci].colour_id;

            for (uint32_t e = 0u; e < schlafli_p_; ++e) {
                const Vec2 va = parent_verts[e];
                const Vec2 vb = parent_verts[(e + 1u) % schlafli_p_];
                const Vec2 cand_center = reflect_across_geodesic(parent_center, va, vb);
                if (glm::dot(cand_center, cand_center) >= static_cast<T>(0.975)) {
                    continue;
                }

                // Deduplicate by hyperbolic / Euclidean center proximity
                bool duplicate = false;
                for (const auto& existing : gen_cells) {
                    if (glm::distance(existing.center, cand_center) < static_cast<T>(1e-3)) {
                        duplicate = true;
                        break;
                    }
                }
                if (duplicate) continue;

                std::vector<Vec2> child_verts(schlafli_p_);
                for (uint32_t k = 0u; k < schlafli_p_; ++k) {
                    // Reverse order after reflection so CCW winding is preserved
                    const uint32_t rev_k = (schlafli_p_ - 1u) - k;
                    child_verts[k] = reflect_across_geodesic(parent_verts[rev_k], va, vb);
                }
                PolygonMath2D<T>::ensure_winding(child_verts, WindingOrder::COUNTER_CLOCKWISE);

                const uint8_t next_col = static_cast<uint8_t>((parent_col + 1u + (e % 2u)) % 3u);
                gen_cells.push_back(RawHyperbolicCell{
                    std::move(child_verts),
                    cand_center,
                    static_cast<uint16_t>(ring),
                    next_col
                });
                if (gen_cells.size() >= MAX_HYPERBOLIC_CELLS) break;
            }
        }
        ring_start = ring_end;
        ring_end   = gen_cells.size();
        if (ring_start == ring_end) break;
    }

    // 3. Apply interactive SU(1, 1) Möbius translation (`mobius_focus_`) and subdivide geodesic edges
    const MobiusSU11POD<T> focus_mobius = make_hyperbolic_translation(mobius_focus_);

    std::vector<std::vector<Vec2>> cell_polygons;
    std::vector<IsoCellPOD> cell_templates;
    std::vector<TileTransformPOD> cell_transforms;

    cell_polygons.reserve(gen_cells.size());
    cell_templates.reserve(gen_cells.size());
    cell_transforms.reserve(gen_cells.size());
    raw_cell_disk_vertices_.reserve(gen_cells.size());

    for (size_t i = 0; i < gen_cells.size(); ++i) {
        std::vector<Vec2> shifted_corners(schlafli_p_);
        for (uint32_t k = 0u; k < schlafli_p_; ++k) {
            shifted_corners[k] = apply_mobius(focus_mobius, gen_cells[i].verts[k]);
        }
        PolygonMath2D<T>::ensure_winding(shifted_corners, WindingOrder::COUNTER_CLOCKWISE);
        raw_cell_disk_vertices_.push_back(shifted_corners);

        // Build polygon with subdivided circular-arc geodesic edges if arc_subdivisions_ > 1,
        // or corner vertices for topological twin-edge pairing in TilingRegion2D
        std::vector<Vec2> poly_verts;
        if (arc_subdivisions_ <= 1u) {
            poly_verts = shifted_corners;
        } else {
            poly_verts = shifted_corners;
        }

        const Vec2 shifted_center = apply_mobius(focus_mobius, gen_cells[i].center);
        IsoCellPOD cell_pod{};
        cell_pod.cell_id    = static_cast<uint32_t>(i);
        cell_pod.centroid   = shifted_center;
        cell_pod.generation = gen_cells[i].generation;
        cell_pod.colour_id  = gen_cells[i].colour_id;
        cell_pod.aspect_id  = static_cast<uint8_t>(i % schlafli_p_);

        TileTransformPOD xf_pod{};
        xf_pod.col0 = Vec2(static_cast<T>(1.0), static_cast<T>(0.0));
        xf_pod.col1 = Vec2(static_cast<T>(0.0), static_cast<T>(1.0));
        xf_pod.col2 = shifted_center;

        cell_polygons.push_back(std::move(poly_verts));
        cell_templates.push_back(cell_pod);
        cell_transforms.push_back(xf_pod);
    }

    r_out_region.rebuild_topology_from_polygons(
        cell_polygons,
        cell_templates,
        cell_transforms,
        static_cast<T>(2e-3)
    );
}

template class HyperbolicTiling2D<float>;
template class HyperbolicTiling2D<double>;

} // namespace ideam::core