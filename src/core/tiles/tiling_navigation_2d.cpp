#include "tiling_navigation_2d.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace ideam::core {

namespace {

template <FloatingPointScalar T>
[[nodiscard]] constexpr T compute_heuristic_2d(
    const glm::vec2& p_a,
    const glm::vec2& p_b,
    bool p_manhattan
) noexcept {
    const T dx = std::abs(static_cast<T>(p_a.x) - static_cast<T>(p_b.x));
    const T dy = std::abs(static_cast<T>(p_a.y) - static_cast<T>(p_b.y));
    return p_manhattan ? (dx + dy) : std::sqrt(dx * dx + dy * dy);
}

[[nodiscard]] constexpr bool can_traverse_edge(
    const TileEdgePOD& p_edge,
    const TileEdgeMetadataPOD& p_meta,
    uint32_t p_from_cell
) noexcept {
    switch (p_meta.port_dir) {
        case TilePortDirection::PORT_FREE:
            return true;
        case TilePortDirection::PORT_OUT_ONLY:
            return p_edge.cell_a == static_cast<int32_t>(p_from_cell);
        case TilePortDirection::PORT_IN_ONLY:
            return p_edge.cell_b == static_cast<int32_t>(p_from_cell);
        case TilePortDirection::PORT_CLOSED:
        default:
            return false;
    }
}

} // namespace

// ============================================================================
// Dual-Level A* Pathfinding (Fundamental Cells & `Hexize` Super-Cells)
// ============================================================================

template <FloatingPointScalar T>
std::vector<uint32_t> TilingNavigation2D<T>::find_path_cells(
    const Region& p_region,
    uint32_t p_start_cell,
    uint32_t p_end_cell,
    std::span<const T> p_cell_cost_multipliers,
    bool p_manhattan_heuristic
) {
    const size_t n_cells = p_region.get_cell_count();
    if (p_start_cell >= n_cells || p_end_cell >= n_cells) {
        return {};
    }
    if (p_start_cell == p_end_cell) {
        return { p_start_cell };
    }

    struct HeapNode {
        T f_score      = static_cast<T>(0.0);
        uint32_t cell  = 0u;
        bool operator>(const HeapNode& other) const noexcept {
            return f_score > other.f_score;
        }
    };

    const auto cells = p_region.get_cells();
    const auto edges = p_region.get_edges();
    const auto meta  = p_region.get_edge_metadata();

    std::vector<T> g_score(n_cells, std::numeric_limits<T>::max());
    std::vector<int32_t> parent(n_cells, -1);
    std::vector<uint8_t> closed(n_cells, 0u);

    std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<HeapNode>> open_heap;

    g_score[p_start_cell] = static_cast<T>(0.0);
    open_heap.push(HeapNode{
        .f_score = compute_heuristic_2d<T>(cells[p_start_cell].center, cells[p_end_cell].center, p_manhattan_heuristic),
        .cell    = p_start_cell
    });

    while (!open_heap.empty()) {
        const HeapNode current = open_heap.top();
        open_heap.pop();

        const uint32_t u = current.cell;
        if (u == p_end_cell) {
            std::vector<uint32_t> path;
            for (int32_t cur = static_cast<int32_t>(p_end_cell); cur >= 0; cur = parent[static_cast<size_t>(cur)]) {
                path.push_back(static_cast<uint32_t>(cur));
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (closed[u]) continue;
        closed[u] = 1u;

        if ((cells[u].flags & CELL_FLAG_MASKED) != 0u) continue;

        const auto u_edges     = p_region.get_cell_edges(u);
        const auto u_neighbors = p_region.get_cell_neighbors(u);

        for (size_t k = 0; k < u_edges.size(); ++k) {
            const int32_t nb_signed = u_neighbors[k];
            if (nb_signed < 0) continue;

            const uint32_t v = static_cast<uint32_t>(nb_signed);
            if (v >= n_cells || closed[v] || (cells[v].flags & CELL_FLAG_MASKED) != 0u) {
                continue;
            }

            const uint32_t eid = u_edges[k];
            if (eid < edges.size() && eid < meta.size()) {
                if (!can_traverse_edge(edges[eid], meta[eid], u)) {
                    continue;
                }
            }

            const T base_step = (eid < meta.size())
                ? static_cast<T>(meta[eid].cross_distance)
                : static_cast<T>(1.0);
            const T mult = (v < p_cell_cost_multipliers.size())
                ? std::max(static_cast<T>(0.0), p_cell_cost_multipliers[v])
                : static_cast<T>(1.0);
            if (mult >= std::numeric_limits<T>::max() * static_cast<T>(0.25)) {
                continue; // Impassable cell cost multiplier
            }

            const T tentative_g = g_score[u] + base_step * mult;
            if (tentative_g < g_score[v]) {
                g_score[v] = tentative_g;
                parent[v]  = static_cast<int32_t>(u);
                const T h  = compute_heuristic_2d<T>(cells[v].center, cells[p_end_cell].center, p_manhattan_heuristic);
                open_heap.push(HeapNode{tentative_g + h, v});
            }
        }
    }

    return {};
}

template <FloatingPointScalar T>
std::vector<uint32_t> TilingNavigation2D<T>::find_path_hex_super_cells(
    const Region& p_region,
    uint32_t p_start_hex,
    uint32_t p_end_hex,
    bool p_manhattan_heuristic
) {
    const auto hexes = p_region.get_hex_super_cells();
    const size_t n_hex = hexes.size();
    if (p_start_hex >= n_hex || p_end_hex >= n_hex) {
        return {};
    }
    if (p_start_hex == p_end_hex) {
        return { p_start_hex };
    }

    struct HeapNode {
        T f_score     = static_cast<T>(0.0);
        uint32_t hex  = 0u;
        bool operator>(const HeapNode& other) const noexcept {
            return f_score > other.f_score;
        }
    };

    std::vector<T> g_score(n_hex, std::numeric_limits<T>::max());
    std::vector<int32_t> parent(n_hex, -1);
    std::vector<uint8_t> closed(n_hex, 0u);

    std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<HeapNode>> open_heap;

    g_score[p_start_hex] = static_cast<T>(0.0);
    open_heap.push(HeapNode{
        .f_score = compute_heuristic_2d<T>(hexes[p_start_hex].center, hexes[p_end_hex].center, p_manhattan_heuristic),
        .hex     = p_start_hex
    });

    while (!open_heap.empty()) {
        const HeapNode current = open_heap.top();
        open_heap.pop();

        const uint32_t u = current.hex;
        if (u == p_end_hex) {
            std::vector<uint32_t> path;
            for (int32_t cur = static_cast<int32_t>(p_end_hex); cur >= 0; cur = parent[static_cast<size_t>(cur)]) {
                path.push_back(static_cast<uint32_t>(cur));
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (closed[u]) continue;
        closed[u] = 1u;

        for (int32_t nb_signed : hexes[u].neighbor_hex_ids) {
            if (nb_signed < 0) continue;
            const uint32_t v = static_cast<uint32_t>(nb_signed);
            if (v >= n_hex || closed[v]) continue;

            const T step_cost = static_cast<T>(glm::distance(hexes[u].center, hexes[v].center));
            const T tentative_g = g_score[u] + std::max(static_cast<T>(1e-4), step_cost);

            if (tentative_g < g_score[v]) {
                g_score[v] = tentative_g;
                parent[v]  = static_cast<int32_t>(u);
                const T h  = compute_heuristic_2d<T>(hexes[v].center, hexes[p_end_hex].center, p_manhattan_heuristic);
                open_heap.push(HeapNode{tentative_g + h, v});
            }
        }
    }

    return {};
}

// ============================================================================
// Multi-Source Dijkstra / Eikonal Wavefront Flow-Field Solver
// ============================================================================

template <FloatingPointScalar T>
TilingFlowFieldResult<T> TilingNavigation2D<T>::compute_flow_field(
    const Region& p_region,
    std::span<const uint32_t> p_goal_cells,
    std::span<const T> p_cell_cost_multipliers
) {
    TilingFlowFieldResult<T> out;
    const size_t n_cells = p_region.get_cell_count();
    if (n_cells == 0) return out;

    out.arrival_costs.assign(n_cells, std::numeric_limits<T>::max());
    out.next_cell_ids.assign(n_cells, -1);
    out.wave_steps.assign(n_cells, 0xFFFFu);
    out.flow_vectors.assign(n_cells, Vec2(static_cast<T>(0.0)));

    struct WaveItem {
        T cost        = static_cast<T>(0.0);
        uint32_t cell = 0u;
        bool operator>(const WaveItem& other) const noexcept {
            return cost > other.cost;
        }
    };

    std::priority_queue<WaveItem, std::vector<WaveItem>, std::greater<WaveItem>> pq;
    const auto cells = p_region.get_cells();
    const auto edges = p_region.get_edges();
    const auto meta  = p_region.get_edge_metadata();

    for (uint32_t goal : p_goal_cells) {
        if (goal < n_cells && (cells[goal].flags & CELL_FLAG_MASKED) == 0u) {
            out.arrival_costs[goal] = static_cast<T>(0.0);
            out.next_cell_ids[goal] = static_cast<int32_t>(goal);
            out.wave_steps[goal]    = 0u;
            pq.push(WaveItem{static_cast<T>(0.0), goal});
        }
    }

    while (!pq.empty()) {
        const WaveItem top = pq.top();
        pq.pop();

        const uint32_t u = top.cell;
        if (top.cost > out.arrival_costs[u]) continue;

        const auto u_edges     = p_region.get_cell_edges(u);
        const auto u_neighbors = p_region.get_cell_neighbors(u);

        for (size_t k = 0; k < u_edges.size(); ++k) {
            const int32_t nb_signed = u_neighbors[k];
            if (nb_signed < 0) continue;
            const uint32_t v = static_cast<uint32_t>(nb_signed);
            if (v >= n_cells || (cells[v].flags & CELL_FLAG_MASKED) != 0u) continue;

            // Check if an agent at neighbor `v` can traverse edge `eid` into `u`
            const uint32_t eid = u_edges[k];
            if (eid < edges.size() && eid < meta.size()) {
                if (!can_traverse_edge(edges[eid], meta[eid], v)) {
                    continue;
                }
            }

            const T base_step = (eid < meta.size())
                ? static_cast<T>(meta[eid].cross_distance)
                : static_cast<T>(1.0);
            const T mult = (v < p_cell_cost_multipliers.size())
                ? std::max(static_cast<T>(0.0), p_cell_cost_multipliers[v])
                : static_cast<T>(1.0);
            if (mult >= std::numeric_limits<T>::max() * static_cast<T>(0.25)) continue;

            const T new_cost = out.arrival_costs[u] + base_step * mult;
            if (new_cost < out.arrival_costs[v]) {
                out.arrival_costs[v] = new_cost;
                out.next_cell_ids[v] = static_cast<int32_t>(u);
                out.wave_steps[v]    = static_cast<uint16_t>(std::min<uint32_t>(65534u, out.wave_steps[u] + 1u));
                pq.push(WaveItem{new_cost, v});
            }
        }
    }

    // Reconstruct smooth 2D steering vectors from arrival_costs gradients
    for (uint32_t c = 0; c < static_cast<uint32_t>(n_cells); ++c) {
        const T my_cost = out.arrival_costs[c];
        if (my_cost <= GeometryConstants<T>::EPSILON || my_cost >= std::numeric_limits<T>::max() * static_cast<T>(0.5)) {
            continue;
        }

        const Vec2 my_center(static_cast<T>(cells[c].center.x), static_cast<T>(cells[c].center.y));
        const auto c_neighbors = p_region.get_cell_neighbors(c);
        Vec2 accum_dir(static_cast<T>(0.0));

        for (int32_t nb_signed : c_neighbors) {
            if (nb_signed < 0) continue;
            const uint32_t nb = static_cast<uint32_t>(nb_signed);
            const T nb_cost   = out.arrival_costs[nb];
            if (nb_cost < my_cost) {
                const Vec2 nb_center(static_cast<T>(cells[nb].center.x), static_cast<T>(cells[nb].center.y));
                const Vec2 delta = nb_center - my_center;
                const T len = glm::length(delta);
                if (len > GeometryConstants<T>::EPSILON) {
                    const T drop = my_cost - nb_cost;
                    accum_dir += (delta / len) * drop;
                }
            }
        }

        const T accum_len = glm::length(accum_dir);
        if (accum_len > GeometryConstants<T>::EPSILON) {
            out.flow_vectors[c] = accum_dir / accum_len;
        } else if (out.next_cell_ids[c] >= 0) {
            const uint32_t next_id = static_cast<uint32_t>(out.next_cell_ids[c]);
            const Vec2 next_center(static_cast<T>(cells[next_id].center.x), static_cast<T>(cells[next_id].center.y));
            const Vec2 d = next_center - my_center;
            const T dl = glm::length(d);
            if (dl > GeometryConstants<T>::EPSILON) {
                out.flow_vectors[c] = d / dl;
            }
        }
    }

    return out;
}

// ============================================================================
// Barycentric Sub-Triangle Splat Evaluation & Atlas Generation (`EquilateralHexSplats`)
// ============================================================================

template <FloatingPointScalar T>
BarycentricSubSplatPOD<T> TilingNavigation2D<T>::evaluate_triangle_sub_splat(
    const Vec2& p_v0,
    const Vec2& p_v1,
    const Vec2& p_v2,
    const Vec2& p_point
) noexcept {
    BarycentricSubSplatPOD<T> out{};
    out.barycentric_uvw = GeometryKernelMath<T>::compute_barycentric_2d(p_v0, p_v1, p_v2, p_point);

    const T u = out.barycentric_uvw.x;
    const T v = out.barycentric_uvw.y;
    const T w = out.barycentric_uvw.z;

    // A triangle subdivided by its 3 medians (connecting centroid to midpoints of v0-v1, v1-v2, v2-v0)
    // partitions into 6 sectors determined by the ordering of barycentric coordinates (u, v, w):
    //   Sector 0: u >= w >= v (bottom-left near v0 toward v2)
    //   Sector 1: u >= v >  w (left near v0 toward v1)
    //   Sector 2: v >  u >= w (top-left near v1 toward v0)
    //   Sector 3: v >= w >  u (top-right near v1 toward v2)
    //   Sector 4: w >  v >= u (right near v2 toward v1)
    //   Sector 5: w >= u >  v (bottom-right near v2 toward v0)
    if (u >= v && u >= w) {
        out.active_sub_triangle = (w >= v) ? 0u : 1u;
    } else if (v >= u && v >= w) {
        out.active_sub_triangle = (u >= w) ? 2u : 3u;
    } else {
        out.active_sub_triangle = (v >= u) ? 4u : 5u;
    }

    out.sector_weights = {
        std::max(static_cast<T>(0.0), u - v),
        std::max(static_cast<T>(0.0), u - w),
        std::max(static_cast<T>(0.0), v - w),
        std::max(static_cast<T>(0.0), v - u),
        std::max(static_cast<T>(0.0), w - u),
        std::max(static_cast<T>(0.0), w - v)
    };
    return out;
}

template <FloatingPointScalar T>
void TilingNavigation2D<T>::generate_equilateral_hex_splat_atlas(
    uint32_t p_splat_size,
    std::vector<Vec3>& r_out_pixels,
    std::vector<uint32_t>& r_out_splat_count_offsets
) {
    r_out_pixels.clear();
    r_out_splat_count_offsets.clear();
    if (p_splat_size < 2) return;

    const T scale = static_cast<T>(p_splat_size - 1u);
    std::vector<Vec2> corner_points = {
        Vec2(static_cast<T>(-0.5), static_cast<T>(-0.28875)) * scale,
        Vec2(static_cast<T>( 0.0), static_cast<T>( 0.57750)) * scale,
        Vec2(static_cast<T>( 0.5), static_cast<T>(-0.28875)) * scale
    };

    Vec2 splat_offset(static_cast<T>(0.0));
    for (const auto& cp : corner_points) {
        splat_offset.x = std::min(splat_offset.x, cp.x);
        splat_offset.y = std::min(splat_offset.y, cp.y);
    }
    splat_offset = -splat_offset + Vec2(static_cast<T>(1.0));
    for (auto& cp : corner_points) {
        cp += splat_offset;
    }

    constexpr std::array<T, 7> kSubdivFracs = {
        static_cast<T>(0.0),
        static_cast<T>(0.2),
        static_cast<T>(0.333334),
        static_cast<T>(0.5),
        static_cast<T>(0.666667),
        static_cast<T>(0.8),
        static_cast<T>(1.0)
    };

    std::array<Vec2, 7> bottom_verts{};
    std::array<Vec2, 7> left_verts{};
    std::array<Vec2, 7> right_verts{};

    for (size_t i = 0; i < 7; ++i) {
        bottom_verts[i] = glm::mix(corner_points[0], corner_points[2], kSubdivFracs[i]);
        left_verts[i]   = glm::mix(corner_points[0], corner_points[1], kSubdivFracs[i]);
        right_verts[i]  = glm::mix(corner_points[1], corner_points[2], kSubdivFracs[i]);
    }

    const size_t tile_pix_count = static_cast<size_t>(p_splat_size) * p_splat_size;
    std::vector<Vec3> mask_pix(tile_pix_count, Vec3(static_cast<T>(0.0)));
    uint32_t mask_count = 0u;

    auto rasterize_sub_tri = [&](const Vec2& a, const Vec2& b, const Vec2& c, const Vec3& color, bool overwrite) {
        for (uint32_t y = 0; y < p_splat_size; ++y) {
            for (uint32_t x = 0; x < p_splat_size; ++x) {
                const size_t idx = static_cast<size_t>(y) * p_splat_size + x;
                if (!overwrite && glm::dot(mask_pix[idx], mask_pix[idx]) > static_cast<T>(0.0)) {
                    continue;
                }
                if (PolygonMath2D<T>::is_point_in_triangle(a, b, c, Vec2(static_cast<T>(x), static_cast<T>(y)))) {
                    mask_pix[idx] = color;
                } else if (overwrite) {
                    mask_pix[idx] = Vec3(static_cast<T>(0.0));
                }
            }
        }
    };

    // 0: Black mask
    r_out_pixels.insert(r_out_pixels.end(), mask_pix.begin(), mask_pix.end());
    ++mask_count;

    // 1: Solid Triangle
    rasterize_sub_tri(corner_points[0], corner_points[1], corner_points[2], Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)), true);
    r_out_pixels.insert(r_out_pixels.end(), mask_pix.begin(), mask_pix.end());
    ++mask_count;
    r_out_splat_count_offsets.push_back(mask_count);

    // 2: Two-Splat Masks
    for (size_t b = 1; b < 6; ++b) {
        for (size_t l = 1; l < 7; ++l) {
            rasterize_sub_tri(bottom_verts[0], left_verts[l], bottom_verts[b], Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)), true);
            r_out_pixels.insert(r_out_pixels.end(), mask_pix.begin(), mask_pix.end());
            ++mask_count;

            if (l == 6) {
                for (size_t r = 1; r < 6; ++r) {
                    rasterize_sub_tri(bottom_verts[b], left_verts[l], right_verts[r], Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)), false);
                    r_out_pixels.insert(r_out_pixels.end(), mask_pix.begin(), mask_pix.end());
                    ++mask_count;
                }
            }
        }
    }
    r_out_splat_count_offsets.push_back(mask_count);

    // 3: Representative 6-Sector Centroidal Splat Mask (channels encoded in R, RG, G, GB, B, RB)
    for (size_t b = 1; b < 7; b += 2) {
        for (size_t l = 1; l < 6; l += 2) {
            for (size_t r = 1; r < 6; r += 2) {
                const Vec2 mid = (bottom_verts[b] + left_verts[l] + right_verts[r]) * static_cast<T>(1.0 / 3.0);
                std::fill(mask_pix.begin(), mask_pix.end(), Vec3(static_cast<T>(0.0)));
                rasterize_sub_tri(bottom_verts[0], mid, bottom_verts[b], Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)), false);
                rasterize_sub_tri(bottom_verts[0], left_verts[l], mid,   Vec3(static_cast<T>(1), static_cast<T>(1), static_cast<T>(0)), false);
                rasterize_sub_tri(left_verts[l], left_verts[6], mid,     Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)), false);
                rasterize_sub_tri(left_verts[6], right_verts[r], mid,    Vec3(static_cast<T>(0), static_cast<T>(1), static_cast<T>(1)), false);
                rasterize_sub_tri(right_verts[r], right_verts[6], mid,   Vec3(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)), false);
                rasterize_sub_tri(right_verts[6], bottom_verts[b], mid,  Vec3(static_cast<T>(1), static_cast<T>(0), static_cast<T>(1)), false);
                r_out_pixels.insert(r_out_pixels.end(), mask_pix.begin(), mask_pix.end());
                ++mask_count;
            }
        }
    }
    r_out_splat_count_offsets.push_back(mask_count);
}

// Explicit Instantiations
template struct TilingNavigation2D<float>;
template struct TilingNavigation2D<double>;

} // namespace ideam::core