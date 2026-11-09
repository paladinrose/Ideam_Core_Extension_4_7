#include "csg_2d.h"
#include <algorithm>
#include <cmath>

namespace ideam::core {

template <FloatingPointScalar T>
ClipClassification2D CSGMath2D<T>::classify_polygon_half_plane(
    std::span<const Vec2> p_polygon,
    const Vec2& p_plane_origin,
    const Vec2& p_inside_normal,
    T p_epsilon
) noexcept {
    if (p_polygon.empty()) return ClipClassification2D::FULLY_OUTSIDE;

    bool any_inside  = false;
    bool any_outside = false;

    for (const auto& pt : p_polygon) {
        const T signed_dist = glm::dot(pt - p_plane_origin, p_inside_normal);
        if (signed_dist > p_epsilon) {
            any_inside = true;
        } else if (signed_dist < -p_epsilon) {
            any_outside = true;
        }
        if (any_inside && any_outside) {
            return ClipClassification2D::STRADDLING_BOUNDARY;
        }
    }

    return any_outside ? ClipClassification2D::FULLY_OUTSIDE : ClipClassification2D::FULLY_INSIDE;
}

template <FloatingPointScalar T>
ClipClassification2D CSGMath2D<T>::classify_polygon_against_boundary(
    std::span<const Vec2> p_tile_polygon,
    std::span<const Vec2> p_boundary_polygon,
    const AABB2D<T>& p_boundary_aabb
) noexcept {
    if (p_tile_polygon.size() < 3 || p_boundary_polygon.size() < 3) {
        return ClipClassification2D::FULLY_OUTSIDE;
    }

    // Fast AABB rejection
    const AABB2D<T> tile_aabb = PolygonMath2D<T>::compute_aabb(p_tile_polygon);
    if (!tile_aabb.intersects(p_boundary_aabb)) {
        return ClipClassification2D::FULLY_OUTSIDE;
    }

    // Check if any tile edge intersects any boundary edge
    Vec2 hit_pt;
    for (size_t i = 0; i < p_tile_polygon.size(); ++i) {
        const Vec2& a1 = p_tile_polygon[i];
        const Vec2& a2 = p_tile_polygon[(i + 1) % p_tile_polygon.size()];
        for (size_t j = 0; j < p_boundary_polygon.size(); ++j) {
            const Vec2& b1 = p_boundary_polygon[j];
            const Vec2& b2 = p_boundary_polygon[(j + 1) % p_boundary_polygon.size()];
            if (PolygonMath2D<T>::line_intersection(a1, a2, b1, b2, hit_pt)) {
                return ClipClassification2D::STRADDLING_BOUNDARY;
            }
        }
    }

    // Check vertex containment
    size_t inside_count = 0;
    for (const auto& v : p_tile_polygon) {
        if (PolygonMath2D<T>::contains_point(p_boundary_polygon, v)) {
            ++inside_count;
        }
    }

    if (inside_count == p_tile_polygon.size()) {
        // Also ensure no boundary vertex lies strictly inside the tile polygon
        for (const auto& bv : p_boundary_polygon) {
            if (tile_aabb.contains_point(bv) && PolygonMath2D<T>::contains_point(p_tile_polygon, bv)) {
                return ClipClassification2D::STRADDLING_BOUNDARY;
            }
        }
        return ClipClassification2D::FULLY_INSIDE;
    }
    if (inside_count > 0) {
        return ClipClassification2D::STRADDLING_BOUNDARY;
    }

    // Edge case: boundary is completely contained inside the tile
    if (PolygonMath2D<T>::contains_point(p_tile_polygon, p_boundary_polygon[0])) {
        return ClipClassification2D::STRADDLING_BOUNDARY;
    }

    return ClipClassification2D::FULLY_OUTSIDE;
}

template <FloatingPointScalar T>
std::vector<typename CSGMath2D<T>::Vec2> CSGMath2D<T>::clip_polygon_half_plane(
    std::span<const Vec2> p_subject,
    const Vec2& p_plane_origin,
    const Vec2& p_inside_normal
) {
    std::vector<Vec2> output;
    if (p_subject.size() < 3) return output;

    output.reserve(p_subject.size() + 2);

    Vec2 s = p_subject.back();
    T s_dist = glm::dot(s - p_plane_origin, p_inside_normal);
    bool s_inside = (s_dist >= -GeometryConstants<T>::EPSILON);

    for (const Vec2& e : p_subject) {
        const T e_dist = glm::dot(e - p_plane_origin, p_inside_normal);
        const bool e_inside = (e_dist >= -GeometryConstants<T>::EPSILON);

        if (e_inside) {
            if (!s_inside) {
                const T denom = s_dist - e_dist;
                const T t = (std::abs(denom) > GeometryConstants<T>::SQ_EPSILON)
                    ? (s_dist / denom)
                    : static_cast<T>(0.5);
                output.push_back(glm::mix(s, e, std::clamp(t, static_cast<T>(0.0), static_cast<T>(1.0))));
            }
            output.push_back(e);
        } else if (s_inside) {
            const T denom = s_dist - e_dist;
            const T t = (std::abs(denom) > GeometryConstants<T>::SQ_EPSILON)
                ? (s_dist / denom)
                : static_cast<T>(0.5);
            output.push_back(glm::mix(s, e, std::clamp(t, static_cast<T>(0.0), static_cast<T>(1.0))));
        }

        s = e;
        s_dist = e_dist;
        s_inside = e_inside;
    }

    return output;
}

template <FloatingPointScalar T>
std::vector<typename CSGMath2D<T>::Vec2> CSGMath2D<T>::clip_polygon_convex(
    std::span<const Vec2> p_subject,
    std::span<const Vec2> p_convex_clip
) {
    if (p_subject.size() < 3 || p_convex_clip.size() < 3) {
        return {};
    }

    // Determine winding of clip polygon so inward normal always points into the interior
    const T clip_signed_area = PolygonMath2D<T>::area(p_convex_clip);
    const T winding_sign = (clip_signed_area >= static_cast<T>(0.0))
        ? static_cast<T>(1.0)
        : static_cast<T>(-1.0);

    std::vector<Vec2> working(p_subject.begin(), p_subject.end());
    // Strip duplicate closing vertex if present
    if (working.size() > 3 && glm::distance(working.front(), working.back()) < GeometryConstants<T>::WELD_EPSILON) {
        working.pop_back();
    }

    size_t clip_n = p_convex_clip.size();
    if (clip_n > 3 && glm::distance(p_convex_clip.front(), p_convex_clip.back()) < GeometryConstants<T>::WELD_EPSILON) {
        clip_n -= 1;
    }

    for (size_t i = 0; i < clip_n; ++i) {
        const Vec2& c0 = p_convex_clip[i];
        const Vec2& c1 = p_convex_clip[(i + 1) % clip_n];
        const Vec2 edge = c1 - c0;
        if (glm::dot(edge, edge) <= GeometryConstants<T>::SQ_EPSILON) continue;

        // Inward normal for CCW edge (dx, dy) is (-dy, dx)
        const Vec2 inward_normal = GeometryKernelMath<T>::perp_ccw(edge) * winding_sign;
        working = clip_polygon_half_plane(working, c0, inward_normal);
        if (working.size() < 3) {
            working.clear();
            break;
        }
    }

    return working;
}

template <FloatingPointScalar T>
bool CSGMath2D<T>::clip_segment_convex(
    const Vec2& p_a,
    const Vec2& p_b,
    std::span<const Vec2> p_convex_clip,
    Vec2& r_clipped_a,
    Vec2& r_clipped_b
) noexcept {
    if (p_convex_clip.size() < 3) return false;

    const T clip_signed_area = PolygonMath2D<T>::area(p_convex_clip);
    const T winding_sign = (clip_signed_area >= static_cast<T>(0.0))
        ? static_cast<T>(1.0)
        : static_cast<T>(-1.0);

    T t_enter = static_cast<T>(0.0);
    T t_leave = static_cast<T>(1.0);
    const Vec2 dir = p_b - p_a;

    for (size_t i = 0; i < p_convex_clip.size(); ++i) {
        const Vec2& c0 = p_convex_clip[i];
        const Vec2& c1 = p_convex_clip[(i + 1) % p_convex_clip.size()];
        const Vec2 edge = c1 - c0;
        if (glm::dot(edge, edge) <= GeometryConstants<T>::SQ_EPSILON) continue;

        const Vec2 inward_normal = GeometryKernelMath<T>::perp_ccw(edge) * winding_sign;
        const T num = glm::dot(c0 - p_a, inward_normal);
        const T den = glm::dot(dir, inward_normal);

        if (std::abs(den) <= GeometryConstants<T>::SQ_EPSILON) {
            // Segment is parallel to clip edge; reject if outside
            if (num > GeometryConstants<T>::EPSILON) return false;
        } else {
            const T t = num / den;
            if (den > static_cast<T>(0.0)) {
                // Entering half-plane
                t_enter = std::max(t_enter, t);
            } else {
                // Leaving half-plane
                t_leave = std::min(t_leave, t);
            }
            if (t_enter > t_leave) return false;
        }
    }

    r_clipped_a = p_a + dir * t_enter;
    r_clipped_b = p_a + dir * t_leave;
    return true;
}

template <FloatingPointScalar T>
std::vector<PolygonIntersectionEventPOD<T>> CSGMath2D<T>::find_polygon_intersections(
    std::span<const Vec2> p_poly_a,
    std::span<const Vec2> p_poly_b
) {
    std::vector<PolygonIntersectionEventPOD<T>> events;
    if (p_poly_a.size() < 2 || p_poly_b.size() < 2) return events;

    Vec2 hit;
    T ta = static_cast<T>(0.0);
    T tb = static_cast<T>(0.0);

    for (size_t i = 0; i < p_poly_a.size(); ++i) {
        const Vec2& a1 = p_poly_a[i];
        const Vec2& a2 = p_poly_a[(i + 1) % p_poly_a.size()];
        for (size_t j = 0; j < p_poly_b.size(); ++j) {
            const Vec2& b1 = p_poly_b[j];
            const Vec2& b2 = p_poly_b[(j + 1) % p_poly_b.size()];
            if (PolygonMath2D<T>::line_intersection(a1, a2, b1, b2, hit, &ta, &tb)) {
                events.push_back(PolygonIntersectionEventPOD<T>{
                    .point   = hit,
                    .edge_a  = static_cast<uint32_t>(i),
                    .edge_b  = static_cast<uint32_t>(j),
                    .param_a = ta,
                    .param_b = tb
                });
            }
        }
    }
    return events;
}

template <FloatingPointScalar T>
void CSGMath2D<T>::weld_polygons_along_seam(
    std::span<std::vector<Vec2>> r_left_tiles,
    std::span<std::vector<Vec2>> r_right_tiles,
    std::span<const Vec2> p_seam_polyline,
    T p_weld_tolerance
) {
    if (p_seam_polyline.size() < 2) return;

    const AABB2D<T> seam_aabb = PolygonMath2D<T>::compute_aabb(p_seam_polyline).grown(p_weld_tolerance * static_cast<T>(4.0));

    auto clip_and_weld_side = [&](std::span<std::vector<Vec2>> r_tiles, T p_side_sign) {
        for (auto& poly : r_tiles) {
            if (poly.size() < 3) continue;

            const AABB2D<T> tile_aabb = PolygonMath2D<T>::compute_aabb(poly);
            if (!tile_aabb.intersects(seam_aabb)) continue;

            // Find the closest seam segment to the tile centroid to orient the local half-plane clip
            const Vec2 center = PolygonMath2D<T>::centroid(poly);
            size_t best_seg   = 0;
            Vec2 closest_pt   = p_seam_polyline[0];
            T best_dist_sq    = std::numeric_limits<T>::max();

            for (size_t s = 0; s + 1 < p_seam_polyline.size(); ++s) {
                const Vec2& a = p_seam_polyline[s];
                const Vec2& b = p_seam_polyline[s + 1];
                const Vec2 cand = PolygonMath2D<T>::closest_point_on_segment(a, b, center);
                const Vec2 diff = center - cand;
                const T d2 = glm::dot(diff, diff);
                if (d2 < best_dist_sq) {
                    best_dist_sq = d2;
                    best_seg = s;
                    closest_pt = cand;
                }
            }

            // Clip against any seam segments whose bounding box overlaps this tile
            for (size_t s = 0; s + 1 < p_seam_polyline.size(); ++s) {
                const Vec2& a = p_seam_polyline[s];
                const Vec2& b = p_seam_polyline[s + 1];
                AABB2D<T> seg_box;
                seg_box.expand_to(a);
                seg_box.expand_to(b);
                if (!tile_aabb.grown(p_weld_tolerance).intersects(seg_box)) continue;

                // Only clip against the dominant local segment or segments that actually intersect the polygon
                bool intersects_seg = (s == best_seg);
                if (!intersects_seg) {
                    Vec2 hit;
                    for (size_t e = 0; e < poly.size(); ++e) {
                        if (PolygonMath2D<T>::line_intersection(poly[e], poly[(e + 1) % poly.size()], a, b, hit)) {
                            intersects_seg = true;
                            break;
                        }
                    }
                }
                if (!intersects_seg) continue;

                const Vec2 seg_dir = b - a;
                const T len_sq = glm::dot(seg_dir, seg_dir);
                if (len_sq <= GeometryConstants<T>::SQ_EPSILON) continue;

                // Left normal of directed segment (a -> b) is (-dy, dx)
                const Vec2 inside_normal = GeometryKernelMath<T>::perp_ccw(seg_dir) *
                                           (p_side_sign / std::sqrt(len_sq));
                poly = clip_polygon_half_plane(poly, a, inside_normal);
                if (poly.size() < 3) {
                    poly.clear();
                    break;
                }
            }

            if (poly.size() < 3) continue;

            // Snap vertices within p_weld_tolerance of the seam polyline or its vertices
            for (auto& v : poly) {
                // First check exact polyline vertices so both left & right tiles snap to identical bend coordinates
                bool snapped_to_vert = false;
                for (const auto& sv : p_seam_polyline) {
                    if (glm::distance(v, sv) <= p_weld_tolerance) {
                        v = sv;
                        snapped_to_vert = true;
                        break;
                    }
                }
                if (!snapped_to_vert) {
                    Vec2 proj;
                    const T dist = PolygonMath2D<T>::distance_to_boundary(
                        p_seam_polyline, v, &proj, nullptr, false
                    );
                    if (dist <= p_weld_tolerance) {
                        v = proj;
                    }
                }
            }

            // Insert any intermediate seam bend vertices that lie along a clipped polygon edge
            // so left and right tiles share exact T-junction-free vertices along curved seams.
            std::vector<Vec2> welded;
            welded.reserve(poly.size() + 4);
            for (size_t i = 0; i < poly.size(); ++i) {
                const Vec2& v0 = poly[i];
                const Vec2& v1 = poly[(i + 1) % poly.size()];
                welded.push_back(v0);

                const T edge_len = glm::distance(v0, v1);
                if (edge_len <= p_weld_tolerance) continue;

                struct InsertedSeamVert {
                    T t;
                    Vec2 pt;
                };
                std::vector<InsertedSeamVert> mids;
                for (const auto& sv : p_seam_polyline) {
                    if (glm::distance(sv, v0) <= p_weld_tolerance ||
                        glm::distance(sv, v1) <= p_weld_tolerance) {
                        continue;
                    }
                    T t_param = static_cast<T>(0.0);
                    const Vec2 on_edge = PolygonMath2D<T>::closest_point_on_segment(v0, v1, sv, &t_param);
                    if (t_param > static_cast<T>(0.01) && t_param < static_cast<T>(0.99) &&
                        glm::distance(sv, on_edge) <= p_weld_tolerance * static_cast<T>(2.0)) {
                        mids.push_back({t_param, sv});
                    }
                }
                if (!mids.empty()) {
                    std::sort(mids.begin(), mids.end(), [](const InsertedSeamVert& lhs, const InsertedSeamVert& rhs) {
                        return lhs.t < rhs.t;
                    });
                    for (const auto& m : mids) {
                        welded.push_back(m.pt);
                    }
                }
            }

            // Deduplicate consecutive near-identical vertices
            std::vector<Vec2> cleaned;
            cleaned.reserve(welded.size());
            for (const auto& pt : welded) {
                if (cleaned.empty() || glm::distance(cleaned.back(), pt) > GeometryConstants<T>::EPSILON) {
                    cleaned.push_back(pt);
                }
            }
            if (cleaned.size() > 2 && glm::distance(cleaned.front(), cleaned.back()) <= GeometryConstants<T>::EPSILON) {
                cleaned.pop_back();
            }

            poly = (cleaned.size() >= 3) ? std::move(cleaned) : std::vector<Vec2>{};
        }
    };

    clip_and_weld_side(r_left_tiles,  static_cast<T>(1.0));
    clip_and_weld_side(r_right_tiles, static_cast<T>(-1.0));
}

// Explicit Instantiations
template struct CSGMath2D<float>;
template struct CSGMath2D<double>;

} // namespace ideam::core