#include "tiling_region_2d.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace ideam::core {

namespace {

struct GridCoordKey {
    int64_t x = 0;
    int64_t y = 0;
    bool operator==(const GridCoordKey& other) const noexcept {
        return x == other.x && y == other.y;
    }
};

struct GridCoordKeyHash {
    size_t operator()(const GridCoordKey& k) const noexcept {
        // Fibonacci spatial hash mix
        const uint64_t ux = static_cast<uint64_t>(k.x) * 0x9E3779B97F4A7C15ULL;
        const uint64_t uy = static_cast<uint64_t>(k.y) * 0xBF58476D1CE4E5B9ULL;
        return static_cast<size_t>(ux ^ (uy + 0x9E3779B97F4A7C15ULL + (ux << 6) + (ux >> 2)));
    }
};

[[nodiscard]] constexpr uint64_t make_undirected_edge_key(uint32_t a, uint32_t b) noexcept {
    const uint32_t lo = (a < b) ? a : b;
    const uint32_t hi = (a < b) ? b : a;
    return (static_cast<uint64_t>(lo) << 32u) | static_cast<uint64_t>(hi);
}

} // namespace

template <FloatingPointScalar T>
void TilingRegion2D<T>::clear() noexcept {
    cells_.clear();
    cell_transforms_.clear();
    shared_vertices_.clear();
    cell_vertex_indices_.clear();
    edges_.clear();
    edge_metadata_.clear();
    cell_edge_indices_.clear();
    cell_neighbor_indices_.clear();
    triangle_indices_.clear();
    extents_ = AABB2D<T>{};

    border_cell_ids_.clear();
    border_edge_ids_.clear();
    border_vertex_ids_.clear();
    orphan_edge_ids_.clear();
    non_manifold_edge_ids_.clear();
    border_outlines_.clear();
    border_outline_slices_.clear();

    hex_super_cells_.clear();
    hex_center_vertex_ids_.clear();
    hex_child_cell_indices_.clear();
    hex_edge_ids_.clear();
    hex_edges_.clear();
    bridge_quads_.clear();
}

template <FloatingPointScalar T>
uint8_t TilingRegion2D<T>::_get_target_hex_valence(WallpaperGroupId p_group) noexcept {
    switch (p_group) {
        case WallpaperGroupId::P3:
            return 3u;  // 3 rhombi meet at the hex center vertex
        case WallpaperGroupId::P31M:
        case WallpaperGroupId::P3M1:
        case WallpaperGroupId::P6:
            return 6u;  // 6 triangles meet at the hex center vertex
        case WallpaperGroupId::P6M:
            return 12u; // 12 right-triangles meet at the hex center vertex
        default:
            return 0u;
    }
}

// ============================================================================
// Region Building & O(N) Spatial-Hashed Topological Deduplication
// ============================================================================

template <FloatingPointScalar T>
void TilingRegion2D<T>::build_from_aabb(
    const AABB2D<T>& p_bounds,
    const Vec2& p_seed_point,
    bool p_auto_hexize,
    int p_edge_subdivisions
) {
    clear();
    if (!p_bounds.is_valid()) return;

    const TilingRegionFillResult<T> fill = tiling_.fill_region_aabb(p_bounds, p_seed_point);
    if (fill.cells.empty()) return;

    const bool use_deformed = tiling_.has_any_custom_edge_spline();
    std::vector<std::vector<Vec2>> cell_polys;
    cell_polys.reserve(fill.cells.size());

    for (const auto& cell : fill.cells) {
        cell_polys.push_back(tiling_.get_cell_world_polygon(
            cell.lattice_i,
            cell.lattice_j,
            cell.aspect_index,
            use_deformed,
            p_edge_subdivisions,
            true
        ));
    }

    rebuild_topology_from_polygons(cell_polys, fill.cells, fill.transforms);
    if (p_auto_hexize && _get_target_hex_valence(tiling_.get_wallpaper_group()) > 0u) {
        hexize();
    }
}

template <FloatingPointScalar T>
void TilingRegion2D<T>::build_from_splines(
    std::span<const Spline2> p_region_splines,
    const Vec2& p_seed_point,
    TilingCoverageMode p_coverage_mode,
    int p_spline_subdivisions,
    bool p_auto_hexize
) {
    clear();
    if (p_region_splines.empty()) return;

    const TilingRegionFillResult<T> fill = tiling_.fill_spline_region(
        p_region_splines,
        p_seed_point,
        p_coverage_mode,
        p_spline_subdivisions
    );
    if (fill.cells.empty()) return;

    const bool use_deformed = tiling_.has_any_custom_edge_spline();
    std::vector<std::vector<Vec2>> cell_polys;
    cell_polys.reserve(fill.cells.size());

    size_t clipped_cursor = 0;
    for (size_t i = 0; i < fill.cells.size(); ++i) {
        const IsoCellPOD& cell = fill.cells[i];
        if ((cell.flags & CELL_FLAG_CLIPPED_CAP) != 0u &&
            clipped_cursor < fill.clipped_boundary_polygons.size()) {
            std::vector<Vec2> poly = fill.clipped_boundary_polygons[clipped_cursor++];
            PolygonMath2D<T>::ensure_winding(poly, WindingOrder::COUNTER_CLOCKWISE);
            cell_polys.push_back(std::move(poly));
        } else {
            cell_polys.push_back(tiling_.get_cell_world_polygon(
                cell.lattice_i,
                cell.lattice_j,
                cell.aspect_index,
                use_deformed,
                p_spline_subdivisions,
                true
            ));
        }
    }

    rebuild_topology_from_polygons(cell_polys, fill.cells, fill.transforms);
    if (p_auto_hexize && _get_target_hex_valence(tiling_.get_wallpaper_group()) > 0u) {
        hexize();
    }
}

template <FloatingPointScalar T>
void TilingRegion2D<T>::rebuild_topology_from_polygons(
    std::span<const std::vector<Vec2>> p_cell_polygons,
    std::span<const IsoCellPOD> p_cell_templates,
    std::span<const TileTransformPOD> p_transforms,
    T p_weld_tolerance
) {
    clear();
    if (p_cell_polygons.empty()) return;

    const size_t n_cells = p_cell_polygons.size();
    const T weld_eps     = std::max(p_weld_tolerance, GeometryConstants<T>::EPSILON);
    const T weld_eps_sq  = weld_eps * weld_eps;
    const T inv_cell_sz  = static_cast<T>(1.0) / (weld_eps * static_cast<T>(2.0));

    std::unordered_map<GridCoordKey, std::vector<uint32_t>, GridCoordKeyHash> spatial_vert_Buckets;
    spatial_vert_Buckets.reserve(n_cells * 4);

    auto find_or_insert_vertex = [&](const Vec2& p_pt) -> uint32_t {
        const int64_t gx = static_cast<int64_t>(std::floor(p_pt.x * inv_cell_sz));
        const int64_t gy = static_cast<int64_t>(std::floor(p_pt.y * inv_cell_sz));

        for (int64_t dy = -1; dy <= 1; ++dy) {
            for (int64_t dx = -1; dx <= 1; ++dx) {
                auto it = spatial_vert_Buckets.find(GridCoordKey{gx + dx, gy + dy});
                if (it == spatial_vert_Buckets.end()) continue;
                for (uint32_t vid : it->second) {
                    const Vec2 diff = shared_vertices_[vid] - p_pt;
                    if (glm::dot(diff, diff) <= weld_eps_sq) {
                        return vid;
                    }
                }
            }
        }

        const uint32_t new_vid = static_cast<uint32_t>(shared_vertices_.size());
        shared_vertices_.push_back(p_pt);
        extents_.expand_to(p_pt);
        spatial_vert_Buckets[GridCoordKey{gx, gy}].push_back(new_vid);
        return new_vid;
    };

    cells_.reserve(n_cells);
    cell_transforms_.reserve(n_cells);
    cell_vertex_indices_.reserve(n_cells * 4);
    cell_edge_indices_.reserve(n_cells * 4);
    cell_neighbor_indices_.reserve(n_cells * 4);
    triangle_indices_.reserve(n_cells * 6);

    // Map undirected vertex pair (min(va,vb), max(va,vb)) -> edge index in edges_
    std::unordered_map<uint64_t, uint32_t> edge_lookup;
    edge_lookup.reserve(n_cells * 3);

    std::vector<uint32_t> local_tri;

    for (size_t c = 0; c < n_cells; ++c) {
        const std::vector<Vec2>& raw_poly = p_cell_polygons[c];
        if (raw_poly.size() < 3) continue;

        // Deduplicate vertices of this cell
        std::vector<uint32_t> cell_vids;
        cell_vids.reserve(raw_poly.size());
        for (const auto& pt : raw_poly) {
            const uint32_t vid = find_or_insert_vertex(pt);
            if (cell_vids.empty() || cell_vids.back() != vid) {
                cell_vids.push_back(vid);
            }
        }
        if (cell_vids.size() > 2 && cell_vids.front() == cell_vids.back()) {
            cell_vids.pop_back();
        }
        if (cell_vids.size() < 3) continue;

        const uint32_t new_cell_id = static_cast<uint32_t>(cells_.size());
        IsoCellPOD cell_pod        = (c < p_cell_templates.size()) ? p_cell_templates[c] : IsoCellPOD{};
        TileTransformPOD xform_pod = (c < p_transforms.size())     ? p_transforms[c]     : TileTransformPOD{};

        // Compute exact polygon centroid in world space
        std::vector<Vec2> welded_poly(cell_vids.size());
        for (size_t k = 0; k < cell_vids.size(); ++k) {
            welded_poly[k] = shared_vertices_[cell_vids[k]];
        }
        const Vec2 cent = PolygonMath2D<T>::centroid(welded_poly);

        cell_pod.vertex_count     = static_cast<uint8_t>(std::min<size_t>(cell_vids.size(), 255));
        cell_pod.first_vertex_idx = static_cast<uint32_t>(cell_vertex_indices_.size());
        cell_pod.first_edge_idx   = static_cast<uint32_t>(cell_edge_indices_.size());
        cell_pod.center           = glm::vec2(static_cast<float>(cent.x), static_cast<float>(cent.y));
        xform_pod.cell_id         = new_cell_id;
        if (c >= p_transforms.size()) {
            xform_pod.position = cell_pod.center;
        }

        for (uint32_t vid : cell_vids) {
            cell_vertex_indices_.push_back(vid);
        }

        // Build or match topological edges for this cell
        const size_t nv = cell_vids.size();
        for (size_t e = 0; e < nv; ++e) {
            const uint32_t va = cell_vids[e];
            const uint32_t vb = cell_vids[(e + 1) % nv];
            const uint64_t e_key = make_undirected_edge_key(va, vb);

            auto it = edge_lookup.find(e_key);
            if (it == edge_lookup.end()) {
                const uint32_t eid = static_cast<uint32_t>(edges_.size());
                edges_.push_back(TileEdgePOD{
                    .vertex_a = va,
                    .vertex_b = vb,
                    .cell_a   = static_cast<int32_t>(new_cell_id),
                    .cell_b   = -1
                });
                const float len = static_cast<float>(glm::distance(shared_vertices_[va], shared_vertices_[vb]));
                edge_metadata_.push_back(TileEdgeMetadataPOD{
                    .edge_length    = len,
                    .cross_distance = 1.0f,
                    .port_dir       = TilePortDirection::PORT_FREE,
                    .edge_state     = 0u,
                    .shape_type     = static_cast<uint8_t>(
                        (e < tiling_.get_baked_prototile().num_vertices)
                            ? tiling_.get_baked_prototile().edges[e].shape
                            : TactileEdgeShape::EDGE_I
                    ),
                    .is_reversed    = 0u,
                    .reserved       = 0u
                });
                edge_lookup.emplace(e_key, eid);
                cell_edge_indices_.push_back(eid);
                cell_neighbor_indices_.push_back(-1);
            } else {
                const uint32_t eid = it->second;
                TileEdgePOD& existing = edges_[eid];
                if (existing.cell_b < 0 && existing.cell_a != static_cast<int32_t>(new_cell_id)) {
                    existing.cell_b = static_cast<int32_t>(new_cell_id);
                }
                cell_edge_indices_.push_back(eid);
                cell_neighbor_indices_.push_back(existing.cell_a);
            }
        }

        // Triangulate cell into shared triangle_indices_
        if (nv == 3) {
            triangle_indices_.push_back(cell_vids[0]);
            triangle_indices_.push_back(cell_vids[1]);
            triangle_indices_.push_back(cell_vids[2]);
        } else if (nv == 4) {
            triangle_indices_.push_back(cell_vids[0]);
            triangle_indices_.push_back(cell_vids[1]);
            triangle_indices_.push_back(cell_vids[2]);

            triangle_indices_.push_back(cell_vids[0]);
            triangle_indices_.push_back(cell_vids[2]);
            triangle_indices_.push_back(cell_vids[3]);
        } else {
            PolygonMath2D<T>::triangulate_into(welded_poly, local_tri);
            for (uint32_t li : local_tri) {
                if (li < cell_vids.size()) {
                    triangle_indices_.push_back(cell_vids[li]);
                }
            }
        }

        cells_.push_back(cell_pod);
        cell_transforms_.push_back(xform_pod);
    }

    // Finalize bidirectional cell_neighbor_indices_ and centroid-to-centroid cross_distance
    for (size_t c = 0; c < cells_.size(); ++c) {
        const IsoCellPOD& cell = cells_[c];
        for (uint8_t e = 0; e < cell.vertex_count; ++e) {
            const size_t slot  = static_cast<size_t>(cell.first_edge_idx) + e;
            const uint32_t eid = cell_edge_indices_[slot];
            const TileEdgePOD& edge = edges_[eid];
            const int32_t other = (edge.cell_a == static_cast<int32_t>(c)) ? edge.cell_b : edge.cell_a;
            cell_neighbor_indices_[slot] = other;
        }
    }

    for (size_t eid = 0; eid < edges_.size(); ++eid) {
        const TileEdgePOD& edge = edges_[eid];
        if (edge.cell_a >= 0 && edge.cell_b >= 0) {
            const glm::vec2 ca = cells_[static_cast<size_t>(edge.cell_a)].center;
            const glm::vec2 cb = cells_[static_cast<size_t>(edge.cell_b)].center;
            edge_metadata_[eid].cross_distance = std::max(1e-4f, glm::distance(ca, cb));
        }
    }

    find_borders();
}

// ============================================================================
// `FindBorders` -> Closed `Spline2D` Contour Extraction
// ============================================================================

template <FloatingPointScalar T>
void TilingRegion2D<T>::find_borders() {
    border_cell_ids_.clear();
    border_edge_ids_.clear();
    border_vertex_ids_.clear();
    orphan_edge_ids_.clear();
    non_manifold_edge_ids_.clear();
    border_outlines_.clear();
    border_outline_slices_.clear();

    if (cells_.empty() || edges_.empty()) return;

    std::vector<uint8_t> edge_use_counts(edges_.size(), 0u);
    for (auto& cell : cells_) {
        cell.flags &= static_cast<uint8_t>(~CELL_FLAG_ON_BORDER);
        for (uint8_t e = 0; e < cell.vertex_count; ++e) {
            const uint32_t eid = cell_edge_indices_[static_cast<size_t>(cell.first_edge_idx) + e];
            if (eid < edge_use_counts.size()) {
                ++edge_use_counts[eid];
            }
        }
    }

    // Directed border half-edge for contour walking (oriented CCW around owning cell_a)
    struct DirectedBorderEdge {
        uint32_t edge_id   = 0u;
        uint32_t start_vid = 0u;
        uint32_t end_vid   = 0u;
        uint32_t owner_cell= 0u;
        bool visited       = false;
    };

    std::vector<DirectedBorderEdge> directed_border_edges;
    std::unordered_multimap<uint32_t, size_t> start_vert_to_border_edge;
    std::vector<uint8_t> cell_border_marked(cells_.size(), 0u);
    std::vector<uint8_t> vert_border_marked(shared_vertices_.size(), 0u);

    for (size_t eid = 0; eid < edges_.size(); ++eid) {
        const uint8_t count = edge_use_counts[eid];
        if (count == 0u) {
            orphan_edge_ids_.push_back(static_cast<uint32_t>(eid));
        } else if (count == 1u) {
            TileEdgePOD& edge = edges_[eid];
            const int32_t owner = (edge.cell_a >= 0) ? edge.cell_a : edge.cell_b;
            edge.cell_a = owner;
            edge.cell_b = -1;

            border_edge_ids_.push_back(static_cast<uint32_t>(eid));

            if (owner >= 0 && static_cast<size_t>(owner) < cells_.size()) {
                IsoCellPOD& c_pod = cells_[static_cast<size_t>(owner)];
                c_pod.flags |= CELL_FLAG_ON_BORDER;
                if (!cell_border_marked[static_cast<size_t>(owner)]) {
                    cell_border_marked[static_cast<size_t>(owner)] = 1u;
                    border_cell_ids_.push_back(static_cast<uint32_t>(owner));
                }

                // Determine directed orientation (va -> vb) matching the cell's CCW perimeter
                uint32_t dir_va = edge.vertex_a;
                uint32_t dir_vb = edge.vertex_b;
                const std::span<const uint32_t> c_verts = get_cell_vertices(static_cast<uint32_t>(owner));
                for (size_t k = 0; k < c_verts.size(); ++k) {
                    const uint32_t v0 = c_verts[k];
                    const uint32_t v1 = c_verts[(k + 1) % c_verts.size()];
                    if ((v0 == edge.vertex_a && v1 == edge.vertex_b) ||
                        (v0 == edge.vertex_b && v1 == edge.vertex_a)) {
                        dir_va = v0;
                        dir_vb = v1;
                        break;
                    }
                }

                const size_t d_idx = directed_border_edges.size();
                directed_border_edges.push_back(DirectedBorderEdge{
                    .edge_id    = static_cast<uint32_t>(eid),
                    .start_vid  = dir_va,
                    .end_vid    = dir_vb,
                    .owner_cell = static_cast<uint32_t>(owner),
                    .visited    = false
                });
                start_vert_to_border_edge.emplace(dir_va, d_idx);
            }

            if (edge.vertex_a < vert_border_marked.size() && !vert_border_marked[edge.vertex_a]) {
                vert_border_marked[edge.vertex_a] = 1u;
                border_vertex_ids_.push_back(edge.vertex_a);
            }
            if (edge.vertex_b < vert_border_marked.size() && !vert_border_marked[edge.vertex_b]) {
                vert_border_marked[edge.vertex_b] = 1u;
                border_vertex_ids_.push_back(edge.vertex_b);
            }
        } else if (count > 2u) {
            non_manifold_edge_ids_.push_back(static_cast<uint32_t>(eid));
        }
    }

    // Walk directed border half-edges to extract closed `Spline2D<T>` boundary loops
    uint32_t vertex_cursor = 0u;
    for (size_t i = 0; i < directed_border_edges.size(); ++i) {
        if (directed_border_edges[i].visited) continue;

        std::vector<Vec2> loop_pts;
        size_t curr_idx = i;
        const uint32_t loop_start_vid = directed_border_edges[i].start_vid;

        while (!directed_border_edges[curr_idx].visited) {
            directed_border_edges[curr_idx].visited = true;
            const uint32_t sv = directed_border_edges[curr_idx].start_vid;
            const uint32_t ev = directed_border_edges[curr_idx].end_vid;
            loop_pts.push_back(shared_vertices_[sv]);

            if (ev == loop_start_vid) {
                break; // Closed loop completed
            }

            // Find next unvisited border half-edge starting at ev
            auto range = start_vert_to_border_edge.equal_range(ev);
            size_t next_idx = directed_border_edges.size();
            for (auto it = range.first; it != range.second; ++it) {
                if (!directed_border_edges[it->second].visited) {
                    next_idx = it->second;
                    break;
                }
            }
            if (next_idx >= directed_border_edges.size()) {
                // Append final endpoint if open chain
                loop_pts.push_back(shared_vertices_[ev]);
                break;
            }
            curr_idx = next_idx;
        }

        if (loop_pts.size() >= 3) {
            const AABB2D<T> loop_box = PolygonMath2D<T>::compute_aabb(loop_pts);
            const T loop_len         = PolygonMath2D<T>::perimeter(loop_pts, true);
            const WindingOrder wind  = PolygonMath2D<T>::get_winding_order(loop_pts);

            border_outline_slices_.push_back(PolylineSlicePOD{
                .vertex_offset = vertex_cursor,
                .vertex_count  = static_cast<uint32_t>(loop_pts.size()),
                .total_length  = static_cast<float>(loop_len),
                .dimension     = 2u,
                .is_closed     = 1u,
                .winding_order = (wind == WindingOrder::CLOCKWISE) ? uint8_t{0} : uint8_t{1},
                .flags         = 0u,
                .bounds_min    = glm::vec2(static_cast<float>(loop_box.min_bounds.x), static_cast<float>(loop_box.min_bounds.y)),
                .bounds_max    = glm::vec2(static_cast<float>(loop_box.max_bounds.x), static_cast<float>(loop_box.max_bounds.y))
            });
            vertex_cursor += static_cast<uint32_t>(loop_pts.size());
            border_outlines_.push_back(Spline2::from_vertices(loop_pts, true, CurveType::LINEAR));
        }
    }
}

// ============================================================================
// `Hexize()` & `GetHexEdges()` Dual-Level Super-Cell Aggregation
// ============================================================================

template <FloatingPointScalar T>
void TilingRegion2D<T>::hexize() {
    hex_super_cells_.clear();
    hex_center_vertex_ids_.clear();
    hex_child_cell_indices_.clear();
    hex_edge_ids_.clear();
    hex_edges_.clear();

    for (auto& c : cells_) {
        c.super_hex_id = 0xFFFFFFFFu;
        c.flags &= static_cast<uint8_t>(~CELL_FLAG_HEX_MEMBER);
    }

    const uint8_t target_valence = _get_target_hex_valence(tiling_.get_wallpaper_group());
    if (target_valence == 0u || cells_.empty() || shared_vertices_.empty()) {
        return;
    }

    // Build vertex -> incident cell list
    std::vector<std::vector<uint32_t>> vert_incident_cells(shared_vertices_.size());
    for (uint32_t cid = 0; cid < static_cast<uint32_t>(cells_.size()); ++cid) {
        if ((cells_[cid].flags & CELL_FLAG_BRIDGE_QUAD) != 0u) continue;
        const std::span<const uint32_t> c_verts = get_cell_vertices(cid);
        for (uint32_t vid : c_verts) {
            if (vid < vert_incident_cells.size()) {
                vert_incident_cells[vid].push_back(cid);
            }
        }
    }

    // Identify which local vertex slot on the prototile is the hex center for 3-cell (P3) and 6-cell (P31M/P6) groups:
    // - For P3M1 (equilateral triangles), every interior vertex has valence 6; we select the lattice-aligned
    //   hex centers (where lattice_i == cell_i, lattice_j == cell_j, or greedily non-overlapping 6-clusters).
    // - For P3 (valence 3), P31M (valence 6 at apex), P6 (valence 6 at apex), and P6MM (valence 12 at apex),
    //   the incident cells around the hex center vertex partition the tiling into disjoint hexagons!
    struct CandidateCenter {
        uint32_t vid = 0u;
        T dist_to_origin_sq = static_cast<T>(0.0);
    };
    std::vector<CandidateCenter> candidates;
    for (uint32_t vid = 0; vid < static_cast<uint32_t>(vert_incident_cells.size()); ++vid) {
        if (vert_incident_cells[vid].size() == target_valence) {
            const Vec2& p = shared_vertices_[vid];
            candidates.push_back({vid, glm::dot(p, p)});
        }
    }

    // Sort candidates outward from origin so greedy non-overlapping packing (for P3M1) is deterministic and lattice-aligned
    std::sort(candidates.begin(), candidates.end(), [](const CandidateCenter& a, const CandidateCenter& b) {
        return a.dist_to_origin_sq < b.dist_to_origin_sq;
    });

    std::vector<uint8_t> cell_claimed(cells_.size(), 0u);
    std::unordered_map<uint64_t, std::pair<uint32_t, uint8_t>> outer_edge_to_hex;

    for (const auto& cand : candidates) {
        const uint32_t center_vid = cand.vid;
        const std::vector<uint32_t>& inc_cells = vert_incident_cells[center_vid];

        // Ensure none of the incident fundamental cells are already claimed by another hex super-cell
        bool any_claimed = false;
        for (uint32_t cid : inc_cells) {
            if (cell_claimed[cid]) {
                any_claimed = true;
                break;
            }
        }
        if (any_claimed) continue;

        const uint32_t hex_id = static_cast<uint32_t>(hex_super_cells_.size());
        const Vec2& center_pt = shared_vertices_[center_vid];

        HexSuperCellPOD hex{};
        hex.center           = glm::vec2(static_cast<float>(center_pt.x), static_cast<float>(center_pt.y));
        hex.center_vertex_id = center_vid;
        hex.set_child_slice(
            static_cast<uint32_t>(hex_child_cell_indices_.size()),
            static_cast<uint8_t>(inc_cells.size())
        );

        // Collect all unique perimeter vertices (vertices of incident cells other than center_vid)
        std::vector<uint32_t> perim_vids;
        perim_vids.reserve(12);

        for (uint32_t cid : inc_cells) {
            cell_claimed[cid]        = 1u;
            cells_[cid].super_hex_id = hex_id;
            cells_[cid].flags       |= CELL_FLAG_HEX_MEMBER;
            hex_child_cell_indices_.push_back(cid);

            const std::span<const uint32_t> c_verts = get_cell_vertices(cid);
            for (uint32_t v : c_verts) {
                if (v == center_vid) continue;
                if (std::find(perim_vids.begin(), perim_vids.end(), v) == perim_vids.end()) {
                    perim_vids.push_back(v);
                }
            }
        }

        // Order perimeter vertices counter-clockwise around center_pt and pick the 6 primary corner vertices
        std::sort(perim_vids.begin(), perim_vids.end(), [&](uint32_t va, uint32_t vb) {
            const Vec2 da = shared_vertices_[va] - center_pt;
            const Vec2 db = shared_vertices_[vb] - center_pt;
            return std::atan2(da.y, da.x) < std::atan2(db.y, db.x);
        });

        if (perim_vids.size() > 6) {
            // In P6MM (12 right triangles), 6 perimeter vertices are hex corners (further from center)
            // and 6 are edge midpoints (closer to center). Keep the 6 furthest corner vertices!
            std::vector<uint32_t> by_radius = perim_vids;
            std::sort(by_radius.begin(), by_radius.end(), [&](uint32_t va, uint32_t vb) {
                const Vec2 da = shared_vertices_[va] - center_pt;
                const Vec2 db = shared_vertices_[vb] - center_pt;
                return glm::dot(da, da) > glm::dot(db, db);
            });
            by_radius.resize(6);
            std::sort(by_radius.begin(), by_radius.end(), [&](uint32_t va, uint32_t vb) {
                const Vec2 da = shared_vertices_[va] - center_pt;
                const Vec2 db = shared_vertices_[vb] - center_pt;
                return std::atan2(da.y, da.x) < std::atan2(db.y, db.x);
            });
            perim_vids = std::move(by_radius);
        }

        for (size_t k = 0; k < std::min<size_t>(6, perim_vids.size()); ++k) {
            hex.perimeter_vertex_ids[k] = perim_vids[k];
        }

        hex_center_vertex_ids_.push_back(center_vid);
        hex_super_cells_.push_back(hex);

        // Link 6-way neighbor_hex_ids across shared perimeter corner pairs
        if (perim_vids.size() == 6) {
            for (uint8_t side = 0; side < 6u; ++side) {
                const uint32_t va = hex.perimeter_vertex_ids[side];
                const uint32_t vb = hex.perimeter_vertex_ids[(side + 1u) % 6u];
                const uint64_t key = make_undirected_edge_key(va, vb);

                auto it = outer_edge_to_hex.find(key);
                if (it == outer_edge_to_hex.end()) {
                    outer_edge_to_hex.emplace(key, std::make_pair(hex_id, side));
                } else {
                    const uint32_t other_hex  = it->second.first;
                    const uint8_t other_side  = it->second.second;
                    hex_super_cells_[hex_id].neighbor_hex_ids[side]             = static_cast<int32_t>(other_hex);
                    hex_super_cells_[other_hex].neighbor_hex_ids[other_side]    = static_cast<int32_t>(hex_id);
                }
            }
        }
    }

    // Modernized `GetHexEdges()`: collect all edges belonging to hex member cells that do NOT touch a hex center vertex
    std::unordered_set<uint32_t> center_set(hex_center_vertex_ids_.begin(), hex_center_vertex_ids_.end());
    for (uint32_t eid = 0; eid < static_cast<uint32_t>(edges_.size()); ++eid) {
        const TileEdgePOD& e = edges_[eid];
        if (center_set.contains(e.vertex_a) || center_set.contains(e.vertex_b)) {
            continue;
        }
        const bool a_in_hex = (e.cell_a >= 0 && (cells_[static_cast<size_t>(e.cell_a)].flags & CELL_FLAG_HEX_MEMBER) != 0u);
        const bool b_in_hex = (e.cell_b >= 0 && (cells_[static_cast<size_t>(e.cell_b)].flags & CELL_FLAG_HEX_MEMBER) != 0u);
        if (a_in_hex || b_in_hex) {
            hex_edge_ids_.push_back(eid);
            TileEdgePOD he = e;
            he.cell_a = a_in_hex ? static_cast<int32_t>(cells_[static_cast<size_t>(e.cell_a)].super_hex_id) : -1;
            he.cell_b = b_in_hex ? static_cast<int32_t>(cells_[static_cast<size_t>(e.cell_b)].super_hex_id) : -1;
            // Only keep edges on the outer boundary of a hex (where cell_a != cell_b)
            if (he.cell_a != he.cell_b) {
                hex_edges_.push_back(he);
            }
        }
    }
}

// ============================================================================
// O(N + V + E) Cell Removal (`RemoveSymmetries`)
// ============================================================================

template <FloatingPointScalar T>
void TilingRegion2D<T>::remove_cells(std::span<const uint32_t> p_cell_ids_to_remove) {
    if (p_cell_ids_to_remove.empty() || cells_.empty()) return;

    std::vector<uint8_t> remove_mask(cells_.size(), 0u);
    size_t remove_count = 0;
    for (uint32_t cid : p_cell_ids_to_remove) {
        if (cid < remove_mask.size() && !remove_mask[cid]) {
            remove_mask[cid] = 1u;
            ++remove_count;
        }
    }
    if (remove_count == 0) return;
    if (remove_count == cells_.size()) {
        clear();
        return;
    }

    std::vector<std::vector<Vec2>> kept_polys;
    std::vector<IsoCellPOD> kept_cells;
    std::vector<TileTransformPOD> kept_xforms;
    const size_t remaining = cells_.size() - remove_count;

    kept_polys.reserve(remaining);
    kept_cells.reserve(remaining);
    kept_xforms.reserve(remaining);

    for (uint32_t cid = 0; cid < static_cast<uint32_t>(cells_.size()); ++cid) {
        if (remove_mask[cid]) continue;
        kept_polys.push_back(get_cell_polygon_points(cid));
        kept_cells.push_back(cells_[cid]);
        kept_xforms.push_back(cell_transforms_[cid]);
    }

    const bool had_hex = !hex_super_cells_.empty();
    rebuild_topology_from_polygons(kept_polys, kept_cells, kept_xforms);
    if (had_hex) {
        hexize();
    }
}

// ============================================================================
// Per-Cell Slice & Geometry Helpers
// ============================================================================

template <FloatingPointScalar T>
std::span<const uint32_t> TilingRegion2D<T>::get_cell_vertices(uint32_t p_cell_id) const noexcept {
    if (p_cell_id >= cells_.size()) return {};
    const IsoCellPOD& c = cells_[p_cell_id];
    if (static_cast<size_t>(c.first_vertex_idx) + c.vertex_count > cell_vertex_indices_.size()) {
        return {};
    }
    return std::span<const uint32_t>(
        cell_vertex_indices_.data() + c.first_vertex_idx,
        c.vertex_count
    );
}

template <FloatingPointScalar T>
std::span<const uint32_t> TilingRegion2D<T>::get_cell_edges(uint32_t p_cell_id) const noexcept {
    if (p_cell_id >= cells_.size()) return {};
    const IsoCellPOD& c = cells_[p_cell_id];
    if (static_cast<size_t>(c.first_edge_idx) + c.vertex_count > cell_edge_indices_.size()) {
        return {};
    }
    return std::span<const uint32_t>(
        cell_edge_indices_.data() + c.first_edge_idx,
        c.vertex_count
    );
}

template <FloatingPointScalar T>
std::span<const int32_t> TilingRegion2D<T>::get_cell_neighbors(uint32_t p_cell_id) const noexcept {
    if (p_cell_id >= cells_.size()) return {};
    const IsoCellPOD& c = cells_[p_cell_id];
    if (static_cast<size_t>(c.first_edge_idx) + c.vertex_count > cell_neighbor_indices_.size()) {
        return {};
    }
    return std::span<const int32_t>(
        cell_neighbor_indices_.data() + c.first_edge_idx,
        c.vertex_count
    );
}

template <FloatingPointScalar T>
std::vector<typename TilingRegion2D<T>::Vec2> TilingRegion2D<T>::get_cell_polygon_points(uint32_t p_cell_id) const {
    const std::span<const uint32_t> vids = get_cell_vertices(p_cell_id);
    std::vector<Vec2> pts;
    pts.reserve(vids.size());
    for (uint32_t vid : vids) {
        if (vid < shared_vertices_.size()) {
            pts.push_back(shared_vertices_[vid]);
        }
    }
    return pts;
}

template <FloatingPointScalar T>
AABB2D<T> TilingRegion2D<T>::get_cell_bounds(uint32_t p_cell_id) const noexcept {
    AABB2D<T> box;
    const std::span<const uint32_t> vids = get_cell_vertices(p_cell_id);
    for (uint32_t vid : vids) {
        if (vid < shared_vertices_.size()) {
            box.expand_to(shared_vertices_[vid]);
        }
    }
    return box;
}

template <FloatingPointScalar T>
std::vector<typename TilingRegion2D<T>::Vec2> TilingRegion2D<T>::get_hexagon_center_points() const {
    std::vector<Vec2> pts;
    pts.reserve(hex_center_vertex_ids_.size());
    for (uint32_t vid : hex_center_vertex_ids_) {
        if (vid < shared_vertices_.size()) {
            pts.push_back(shared_vertices_[vid]);
        }
    }
    return pts;
}

// ============================================================================
// Spatial Containment, Masking & Generation Queries
// ============================================================================

template <FloatingPointScalar T>
bool TilingRegion2D<T>::contains_point(const Vec2& p_point, int32_t& r_cell_id) const noexcept {
    r_cell_id = -1;
    if (!extents_.contains_point(p_point)) {
        return false;
    }

    for (uint32_t cid = 0; cid < static_cast<uint32_t>(cells_.size()); ++cid) {
        const std::span<const uint32_t> vids = get_cell_vertices(cid);
        if (vids.size() < 3) continue;

        bool inside = false;
        for (size_t i = 0, j = vids.size() - 1; i < vids.size(); j = i++) {
            const Vec2& pi = shared_vertices_[vids[i]];
            const Vec2& pj = shared_vertices_[vids[j]];

            if (((pi.y <= p_point.y && p_point.y < pj.y) || (pj.y <= p_point.y && p_point.y < pi.y)) &&
                (p_point.x < (pj.x - pi.x) * (p_point.y - pi.y) / (pj.y - pi.y != static_cast<T>(0.0) ? pj.y - pi.y : static_cast<T>(1.0)) + pi.x)) {
                inside = !inside;
            }
        }

        if (inside) {
            r_cell_id = static_cast<int32_t>(cid);
            return true;
        }
    }
    return false;
}

template <FloatingPointScalar T>
bool TilingRegion2D<T>::hex_contains_point(
    const Vec2& p_point,
    int32_t& r_hex_id,
    int32_t* r_sub_cell_id
) const noexcept {
    r_hex_id = -1;
    int32_t cell_id = -1;
    if (!contains_point(p_point, cell_id) || cell_id < 0) {
        if (r_sub_cell_id) *r_sub_cell_id = -1;
        return false;
    }
    if (r_sub_cell_id) *r_sub_cell_id = cell_id;

    const IsoCellPOD& c = cells_[static_cast<size_t>(cell_id)];
    if ((c.flags & CELL_FLAG_HEX_MEMBER) != 0u && c.super_hex_id < hex_super_cells_.size()) {
        r_hex_id = static_cast<int32_t>(c.super_hex_id);
        return true;
    }
    return false;
}

template <FloatingPointScalar T>
std::vector<uint32_t> TilingRegion2D<T>::get_masked_cells(
    std::span<const Spline2> p_mask,
    bool p_thorough_check,
    int p_subdivisions
) const {
    std::vector<uint32_t> masked;
    if (p_mask.empty() || cells_.empty()) return masked;

    std::vector<std::vector<Vec2>> mask_polys;
    std::vector<AABB2D<T>> mask_boxes;
    for (const auto& sp : p_mask) {
        std::vector<Vec2> pts = sp.get_smoothed_points(p_subdivisions);
        if (pts.size() >= 3) {
            mask_boxes.push_back(PolygonMath2D<T>::compute_aabb(pts));
            mask_polys.push_back(std::move(pts));
        }
    }
    if (mask_polys.empty()) return masked;

    for (uint32_t cid = 0; cid < static_cast<uint32_t>(cells_.size()); ++cid) {
        const Vec2 c_pt(static_cast<T>(cells_[cid].center.x), static_cast<T>(cells_[cid].center.y));
        bool hit = false;

        for (size_t m = 0; m < mask_polys.size(); ++m) {
            if (mask_boxes[m].contains_point(c_pt) && PolygonMath2D<T>::contains_point(mask_polys[m], c_pt)) {
                hit = true;
                break;
            }
        }

        if (!hit && p_thorough_check) {
            const std::span<const uint32_t> vids = get_cell_vertices(cid);
            for (uint32_t vid : vids) {
                const Vec2& v_pt = shared_vertices_[vid];
                for (size_t m = 0; m < mask_polys.size(); ++m) {
                    if (mask_boxes[m].contains_point(v_pt) && PolygonMath2D<T>::contains_point(mask_polys[m], v_pt)) {
                        hit = true;
                        break;
                    }
                }
                if (hit) break;
            }
        }

        if (hit) {
            masked.push_back(cid);
        }
    }

    return masked;
}

template <FloatingPointScalar T>
std::vector<uint32_t> TilingRegion2D<T>::get_unmasked_cells(
    std::span<const Spline2> p_mask,
    bool p_thorough_check,
    int p_subdivisions
) const {
    const std::vector<uint32_t> masked = get_masked_cells(p_mask, p_thorough_check, p_subdivisions);
    std::vector<uint8_t> is_masked(cells_.size(), 0u);
    for (uint32_t id : masked) {
        if (id < is_masked.size()) is_masked[id] = 1u;
    }

    std::vector<uint32_t> unmasked;
    unmasked.reserve(cells_.size() - masked.size());
    for (uint32_t cid = 0; cid < static_cast<uint32_t>(cells_.size()); ++cid) {
        if (!is_masked[cid]) {
            unmasked.push_back(cid);
        }
    }
    return unmasked;
}

template <FloatingPointScalar T>
void TilingRegion2D<T>::get_mask_intersected_cells(
    std::span<const Spline2> p_mask,
    std::span<const uint32_t> p_masked_cell_ids,
    std::vector<uint32_t>& r_intersected_cell_ids,
    std::vector<uint32_t>& r_mask_spline_ids,
    int p_subdivisions
) const {
    r_intersected_cell_ids.clear();
    r_mask_spline_ids.clear();
    if (p_mask.empty() || p_masked_cell_ids.empty()) return;

    std::vector<std::vector<Vec2>> mask_polys;
    std::vector<AABB2D<T>> mask_boxes;
    for (const auto& sp : p_mask) {
        std::vector<Vec2> pts = sp.get_smoothed_points(p_subdivisions);
        if (pts.size() >= 2) {
            mask_boxes.push_back(PolygonMath2D<T>::compute_aabb(pts));
            mask_polys.push_back(std::move(pts));
        }
    }

    Vec2 hit_pt;
    for (uint32_t cid : p_masked_cell_ids) {
        if (cid >= cells_.size()) continue;
        const std::vector<Vec2> cell_poly = get_cell_polygon_points(cid);
        if (cell_poly.size() < 3) continue;
        const AABB2D<T> cell_box = PolygonMath2D<T>::compute_aabb(cell_poly);

        bool intersected = false;
        uint32_t hit_mask_id = 0u;

        for (size_t m = 0; m < mask_polys.size(); ++m) {
            if (!cell_box.intersects(mask_boxes[m])) continue;
            const auto& mp = mask_polys[m];
            for (size_t i = 0; i < cell_poly.size() && !intersected; ++i) {
                const Vec2& a1 = cell_poly[i];
                const Vec2& a2 = cell_poly[(i + 1) % cell_poly.size()];
                for (size_t j = 0; j < mp.size(); ++j) {
                    const Vec2& b1 = mp[j];
                    const Vec2& b2 = mp[(j + 1) % mp.size()];
                    if (PolygonMath2D<T>::line_intersection(a1, a2, b1, b2, hit_pt)) {
                        intersected = true;
                        hit_mask_id = static_cast<uint32_t>(m);
                        break;
                    }
                }
            }
            if (intersected) break;
        }

        if (intersected) {
            r_intersected_cell_ids.push_back(cid);
            r_mask_spline_ids.push_back(hit_mask_id);
        }
    }
}

template <FloatingPointScalar T>
std::vector<uint32_t> TilingRegion2D<T>::get_cell_ids_by_generation() const {
    std::vector<uint32_t> ids(cells_.size());
    std::iota(ids.begin(), ids.end(), 0u);
    std::stable_sort(ids.begin(), ids.end(), [&](uint32_t a, uint32_t b) noexcept {
        return cells_[a].generation < cells_[b].generation;
    });
    return ids;
}

template <FloatingPointScalar T>
void TilingRegion2D<T>::offset_region(const Vec2& p_offset) noexcept {
    for (auto& v : shared_vertices_) {
        v += p_offset;
    }
    const glm::vec2 off_f(static_cast<float>(p_offset.x), static_cast<float>(p_offset.y));
    for (auto& c : cells_) {
        c.center += off_f;
    }
    for (auto& xf : cell_transforms_) {
        xf.position += off_f;
    }
    for (auto& h : hex_super_cells_) {
        h.center += off_f;
    }
    if (extents_.is_valid()) {
        extents_.min_bounds += p_offset;
        extents_.max_bounds += p_offset;
    }
}

template <FloatingPointScalar T>
void TilingRegion2D<T>::scale_region(const Vec2& p_scale) noexcept {
    extents_ = AABB2D<T>{};
    for (auto& v : shared_vertices_) {
        v *= p_scale;
        extents_.expand_to(v);
    }
    const glm::vec2 scl_f(static_cast<float>(p_scale.x), static_cast<float>(p_scale.y));
    for (auto& c : cells_) {
        c.center *= scl_f;
    }
    for (auto& xf : cell_transforms_) {
        xf.position *= scl_f;
        xf.scale    *= scl_f;
    }
    for (auto& h : hex_super_cells_) {
        h.center *= scl_f;
    }
}

// Explicit Instantiations
template class TilingRegion2D<float>;
template class TilingRegion2D<double>;

} // namespace ideam::core