#include "tiling_seam_stitcher_2d.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace ideam::core {

namespace {

template <FloatingPointScalar T>
struct ExposedSeamEdgeCandidate {
    uint32_t original_cell_id  = 0u;
    uint32_t merged_cell_id    = 0u;
    uint16_t local_edge_slot   = 0u;
    uint16_t generation        = 0u;
    glm::vec<2, T> vert_a{static_cast<T>(0)};
    glm::vec<2, T> vert_b{static_cast<T>(0)};
    glm::vec<2, T> midpoint{static_cast<T>(0)};
};

template <FloatingPointScalar T>
[[nodiscard]] T estimate_average_edge_length(const TilingRegion2D<T>& p_region) noexcept {
    const auto meta = p_region.get_edge_metadata();
    if (meta.empty()) return static_cast<T>(1.0);
    T accum = static_cast<T>(0.0);
    for (const auto& m : meta) {
        accum += static_cast<T>(m.edge_length);
    }
    return std::max(static_cast<T>(1e-3), accum / static_cast<T>(meta.size()));
}

} // namespace

template <FloatingPointScalar T>
void TilingSeamStitcher2D<T>::merge_into_dominant(
    Region& r_dominant,
    const Region& p_subordinate,
    TilingStitchMode p_stitch_mode,
    const Spline2* p_explicit_seam_spline,
    T p_max_bridge_span
) {
    if (p_subordinate.is_empty()) return;
    if (r_dominant.is_empty()) {
        r_dominant = p_subordinate;
        return;
    }

    const T avg_edge_dom = estimate_average_edge_length(r_dominant);
    const T avg_edge_sub = estimate_average_edge_length(p_subordinate);
    const T max_span     = (p_max_bridge_span > GeometryConstants<T>::EPSILON)
        ? p_max_bridge_span
        : std::max(avg_edge_dom, avg_edge_sub) * static_cast<T>(3.0);

    // Resolve mask splines from explicit seam or r_dominant's extracted border outlines
    std::vector<Spline2> mask_splines;
    if (p_explicit_seam_spline != nullptr && p_explicit_seam_spline->get_point_count() >= 2) {
        mask_splines.push_back(*p_explicit_seam_spline);
    } else {
        const auto dom_outlines = r_dominant.get_border_outlines();
        mask_splines.assign(dom_outlines.begin(), dom_outlines.end());
    }

    // Identify masked (overlapped) cells of p_subordinate
    std::vector<uint32_t> masked_sub_ids;
    if (!mask_splines.empty() && mask_splines[0].get_wrap()) {
        masked_sub_ids = p_subordinate.get_masked_cells(mask_splines, true, 12);
    }

    std::vector<uint8_t> is_sub_masked(p_subordinate.get_cell_count(), 0u);
    for (uint32_t id : masked_sub_ids) {
        if (id < is_sub_masked.size()) {
            is_sub_masked[id] = 1u;
        }
    }

    // ========================================================================
    // Mode 2 & 3: Topological Seam Weld & Curvilinear Metric Morph
    // ========================================================================
    if (p_stitch_mode == TilingStitchMode::TOPOLOGICAL_SEAM_WELD ||
        p_stitch_mode == TilingStitchMode::CURVILINEAR_METRIC_MORPH) {

        std::vector<Vec2> seam_polyline;
        if (!mask_splines.empty()) {
            seam_polyline = mask_splines[0].get_smoothed_points(16);
        }

        std::vector<std::vector<Vec2>> dom_polys;
        std::vector<IsoCellPOD> dom_cells;
        std::vector<TileTransformPOD> dom_xforms;
        dom_polys.reserve(r_dominant.get_cell_count());
        dom_cells.reserve(r_dominant.get_cell_count());
        dom_xforms.reserve(r_dominant.get_cell_count());

        for (uint32_t c = 0; c < static_cast<uint32_t>(r_dominant.get_cell_count()); ++c) {
            dom_polys.push_back(r_dominant.get_cell_polygon_points(c));
            dom_cells.push_back(r_dominant.get_cells()[c]);
            dom_xforms.push_back(r_dominant.get_cell_transforms()[c]);
        }

        std::vector<std::vector<Vec2>> sub_polys;
        std::vector<IsoCellPOD> sub_cells;
        std::vector<TileTransformPOD> sub_xforms;
        sub_polys.reserve(p_subordinate.get_cell_count());
        sub_cells.reserve(p_subordinate.get_cell_count());
        sub_xforms.reserve(p_subordinate.get_cell_count());

        // For TOPOLOGICAL_SEAM_WELD with an explicit seam curve, include straddling subordinate cells so they get clipped
        std::vector<uint32_t> intersected_sub_ids;
        std::vector<uint32_t> dummy_mask_ids;
        if (!masked_sub_ids.empty()) {
            p_subordinate.get_mask_intersected_cells(
                mask_splines, masked_sub_ids, intersected_sub_ids, dummy_mask_ids, 12
            );
        }
        std::vector<uint8_t> is_sub_intersected(p_subordinate.get_cell_count(), 0u);
        for (uint32_t id : intersected_sub_ids) {
            if (id < is_sub_intersected.size()) is_sub_intersected[id] = 1u;
        }

        for (uint32_t c = 0; c < static_cast<uint32_t>(p_subordinate.get_cell_count()); ++c) {
            if (is_sub_masked[c] && !is_sub_intersected[c]) continue;
            IsoCellPOD sc = p_subordinate.get_cells()[c];
            sc.group_slot = static_cast<uint8_t>(std::max<int>(1, sc.group_slot + 1));
            sub_polys.push_back(p_subordinate.get_cell_polygon_points(c));
            sub_cells.push_back(sc);
            sub_xforms.push_back(p_subordinate.get_cell_transforms()[c]);
        }

        if (p_stitch_mode == TilingStitchMode::CURVILINEAR_METRIC_MORPH && seam_polyline.size() >= 2) {
            // Smoothly pull boundary vertices within max_span toward the seam curve using cubic Hermite falloff
            auto morph_towards_seam = [&](std::vector<std::vector<Vec2>>& r_polys) {
                for (auto& poly : r_polys) {
                    for (auto& v : poly) {
                        Vec2 closest_seam;
                        const T dist = PolygonMath2D<T>::distance_to_boundary(
                            seam_polyline, v, &closest_seam, nullptr, mask_splines[0].get_wrap()
                        );
                        if (dist < max_span) {
                            const T u = static_cast<T>(1.0) - (dist / max_span);
                            const T w = u * u * (static_cast<T>(3.0) - static_cast<T>(2.0) * u) * static_cast<T>(0.35);
                            v = glm::mix(v, closest_seam, w);
                        }
                    }
                }
            };
            morph_towards_seam(dom_polys);
            morph_towards_seam(sub_polys);
        }

        if (seam_polyline.size() >= 2) {
            CSGMath2D<T>::weld_polygons_along_seam(dom_polys, sub_polys, seam_polyline);
        }

        std::vector<std::vector<Vec2>> combined_polys;
        std::vector<IsoCellPOD> combined_cells;
        std::vector<TileTransformPOD> combined_xforms;

        for (size_t i = 0; i < dom_polys.size(); ++i) {
            if (dom_polys[i].size() < 3) continue;
            combined_polys.push_back(std::move(dom_polys[i]));
            combined_cells.push_back(dom_cells[i]);
            combined_xforms.push_back(dom_xforms[i]);
        }
        for (size_t i = 0; i < sub_polys.size(); ++i) {
            if (sub_polys[i].size() < 3) continue;
            combined_polys.push_back(std::move(sub_polys[i]));
            combined_cells.push_back(sub_cells[i]);
            combined_xforms.push_back(sub_xforms[i]);
        }

        r_dominant.rebuild_topology_from_polygons(combined_polys, combined_cells, combined_xforms);
        return;
    }

    // ========================================================================
    // Mode 1: `QUAD_BRIDGE` (Modernized `ComplexSymmetryGroup::MergeGroup`)
    // ========================================================================

    // 1. Collect all dominant cells & unmasked subordinate cells, tracking their merged cell indices
    std::vector<std::vector<Vec2>> merged_polys;
    std::vector<IsoCellPOD> merged_cells;
    std::vector<TileTransformPOD> merged_xforms;

    const size_t dom_count = r_dominant.get_cell_count();
    const size_t sub_count = p_subordinate.get_cell_count();

    merged_polys.reserve(dom_count + sub_count + 64);
    merged_cells.reserve(dom_count + sub_count + 64);
    merged_xforms.reserve(dom_count + sub_count + 64);

    for (uint32_t c = 0; c < static_cast<uint32_t>(dom_count); ++c) {
        merged_polys.push_back(r_dominant.get_cell_polygon_points(c));
        merged_cells.push_back(r_dominant.get_cells()[c]);
        merged_xforms.push_back(r_dominant.get_cell_transforms()[c]);
    }

    std::vector<int32_t> sub_to_merged_id(sub_count, -1);
    for (uint32_t c = 0; c < static_cast<uint32_t>(sub_count); ++c) {
        if (is_sub_masked[c]) continue;
        sub_to_merged_id[c] = static_cast<int32_t>(merged_cells.size());

        IsoCellPOD sc = p_subordinate.get_cells()[c];
        sc.group_slot = static_cast<uint8_t>(std::max<int>(1, sc.group_slot + 1));

        merged_polys.push_back(p_subordinate.get_cell_polygon_points(c));
        merged_cells.push_back(sc);
        merged_xforms.push_back(p_subordinate.get_cell_transforms()[c]);
    }

    // 2. Identify exposed border edges on r_dominant and newly exposed border edges on p_subordinate
    std::vector<ExposedSeamEdgeCandidate<T>> dom_bridge_edges;
    std::vector<ExposedSeamEdgeCandidate<T>> sub_bridge_edges;

    const auto dom_edges = r_dominant.get_edges();
    const auto dom_verts = r_dominant.get_vertices();
    for (uint32_t eid : r_dominant.get_border_edge_ids()) {
        if (eid >= dom_edges.size()) continue;
        const TileEdgePOD& e = dom_edges[eid];
        if (e.cell_a < 0) continue;

        const uint32_t cid = static_cast<uint32_t>(e.cell_a);
        const auto c_edges = r_dominant.get_cell_edges(cid);
        uint16_t slot = 0u;
        for (size_t k = 0; k < c_edges.size(); ++k) {
            if (c_edges[k] == eid) {
                slot = static_cast<uint16_t>(k);
                break;
            }
        }

        const Vec2& va = dom_verts[e.vertex_a];
        const Vec2& vb = dom_verts[e.vertex_b];
        dom_bridge_edges.push_back(ExposedSeamEdgeCandidate<T>{
            .original_cell_id = cid,
            .merged_cell_id   = cid,
            .local_edge_slot  = slot,
            .generation       = r_dominant.get_cells()[cid].generation,
            .vert_a           = va,
            .vert_b           = vb,
            .midpoint         = (va + vb) * static_cast<T>(0.5)
        });
    }

    const auto sub_edges = p_subordinate.get_edges();
    const auto sub_verts = p_subordinate.get_vertices();
    for (uint32_t eid = 0; eid < static_cast<uint32_t>(sub_edges.size()); ++eid) {
        const TileEdgePOD& e = sub_edges[eid];
        const bool a_valid_unmasked = (e.cell_a >= 0 && !is_sub_masked[static_cast<size_t>(e.cell_a)]);
        const bool b_valid_unmasked = (e.cell_b >= 0 && !is_sub_masked[static_cast<size_t>(e.cell_b)]);

        // An edge of p_subordinate is an exposed seam candidate if exactly ONE of its adjacent cells
        // remains unmasked (either because the other cell was pruned by the mask, or because it was an outer border edge)
        if (a_valid_unmasked == b_valid_unmasked) continue;

        const uint32_t unmasked_cid = a_valid_unmasked
            ? static_cast<uint32_t>(e.cell_a)
            : static_cast<uint32_t>(e.cell_b);
        const int32_t merged_cid = sub_to_merged_id[unmasked_cid];
        if (merged_cid < 0) continue;

        const auto c_edges = p_subordinate.get_cell_edges(unmasked_cid);
        uint16_t slot = 0u;
        for (size_t k = 0; k < c_edges.size(); ++k) {
            if (c_edges[k] == eid) {
                slot = static_cast<uint16_t>(k);
                break;
            }
        }

        const Vec2& va = sub_verts[e.vertex_a];
        const Vec2& vb = sub_verts[e.vertex_b];
        sub_bridge_edges.push_back(ExposedSeamEdgeCandidate<T>{
            .original_cell_id = unmasked_cid,
            .merged_cell_id   = static_cast<uint32_t>(merged_cid),
            .local_edge_slot  = slot,
            .generation       = p_subordinate.get_cells()[unmasked_cid].generation,
            .vert_a           = va,
            .vert_b           = vb,
            .midpoint         = (va + vb) * static_cast<T>(0.5)
        });
    }

    // 3. Pair exposed dominant and subordinate edges within `max_span` to synthesize BridgeQuadCellPODs
    std::vector<BridgeQuadCellPOD> synthesized_bridges(
        r_dominant.get_bridge_quads().begin(),
        r_dominant.get_bridge_quads().end()
    );
    std::vector<uint8_t> used_sub_edge(sub_bridge_edges.size(), 0u);

    for (size_t i = 0; i < dom_bridge_edges.size(); ++i) {
        const auto& dom_e = dom_bridge_edges[i];
        const Vec2& dA = dom_e.vert_a;
        const Vec2& dB = dom_e.vert_b;

        T best_score  = max_span * static_cast<T>(2.0);
        int best_j    = -1;
        int best_mode = 0; // 0: dA->dB->gB->gA, 1: dA->dB->gA->gB

        for (size_t j = 0; j < sub_bridge_edges.size(); ++j) {
            if (used_sub_edge[j]) continue;
            const auto& sub_e = sub_bridge_edges[j];

            if (glm::distance(dom_e.midpoint, sub_e.midpoint) > max_span) {
                continue;
            }

            const Vec2& gA = sub_e.vert_a;
            const Vec2& gB = sub_e.vert_b;

            const T dist_aa = glm::distance(dA, gA);
            const T dist_bb = glm::distance(dB, gB);
            const T dist_ab = glm::distance(dA, gB);
            const T dist_ba = glm::distance(dB, gA);

            const T score_0 = dist_aa + dist_bb;
            const T score_1 = dist_ab + dist_ba;

            // Choose the non-self-intersecting quad pairing (where diagonals (dA-gB, dB-gA) or (dA-gA, dB-gB) do not bow-tie)
            Vec2 dummy_hit;
            const bool bow_tie_0 = PolygonMath2D<T>::line_intersection(dB, gB, gA, dA, dummy_hit);
            const bool bow_tie_1 = PolygonMath2D<T>::line_intersection(dB, gA, gB, dA, dummy_hit);

            if (!bow_tie_0 && score_0 < best_score) {
                best_score = score_0;
                best_j     = static_cast<int>(j);
                best_mode  = 0;
            }
            if (!bow_tie_1 && score_1 < best_score) {
                best_score = score_1;
                best_j     = static_cast<int>(j);
                best_mode  = 1;
            }
        }

        if (best_j < 0) continue;

        used_sub_edge[static_cast<size_t>(best_j)] = 1u;
        const auto& sub_e = sub_bridge_edges[static_cast<size_t>(best_j)];

        std::vector<Vec2> quad_poly(4);
        quad_poly[0] = dA;
        quad_poly[1] = dB;
        if (best_mode == 0) {
            quad_poly[2] = sub_e.vert_b;
            quad_poly[3] = sub_e.vert_a;
        } else {
            quad_poly[2] = sub_e.vert_a;
            quad_poly[3] = sub_e.vert_b;
        }

        PolygonMath2D<T>::ensure_winding(quad_poly, WindingOrder::COUNTER_CLOCKWISE);
        if (PolygonMath2D<T>::abs_area(quad_poly) <= GeometryConstants<T>::EPSILON) {
            continue;
        }

        const uint32_t bridge_cell_id = static_cast<uint32_t>(merged_cells.size());
        const Vec2 quad_center = PolygonMath2D<T>::centroid(quad_poly);
        const uint16_t gen = static_cast<uint16_t>(std::min<uint32_t>(65535u, dom_e.generation + 1u));

        IsoCellPOD bridge_pod{};
        bridge_pod.lattice_i    = 0;
        bridge_pod.lattice_j    = 0;
        bridge_pod.group_slot   = 0xFFu; // Bridge seam layer marker
        bridge_pod.aspect_index = 0u;
        bridge_pod.vertex_count = 4u;
        bridge_pod.flags        = CELL_FLAG_BRIDGE_QUAD;
        bridge_pod.generation   = gen;
        bridge_pod.colour_id    = 2u;
        bridge_pod.center       = glm::vec2(static_cast<float>(quad_center.x), static_cast<float>(quad_center.y));

        TileTransformPOD bridge_xform{};
        bridge_xform.position    = bridge_pod.center;
        bridge_xform.scale       = glm::vec2(1.0f, 1.0f);
        bridge_xform.angle_rad   = 0.0f;
        bridge_xform.cell_id     = bridge_cell_id;
        bridge_xform.custom_data = glm::vec2(1.0f, static_cast<float>(gen));

        merged_polys.push_back( std::move(quad_poly));
        merged_cells.push_back(bridge_pod);
        merged_xforms.push_back(bridge_xform);

        synthesized_bridges.push_back(BridgeQuadCellPOD{
            .cell_id               = bridge_cell_id,
            .dominant_cell_id      = dom_e.merged_cell_id,
            .subordinate_cell_id   = sub_e.merged_cell_id,
            .dominant_edge_slot    = dom_e.local_edge_slot,
            .subordinate_edge_slot = sub_e.local_edge_slot,
            .quad_vertex_ids       = {0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu}
        });
    }

    // 4. Rebuild unified topology and populate bridge_quads_ with welded vertex IDs
    r_dominant.rebuild_topology_from_polygons(merged_polys, merged_cells, merged_xforms);

    for (auto& bq : synthesized_bridges) {
        if (bq.cell_id < r_dominant.get_cell_count()) {
            const auto vids = r_dominant.get_cell_vertices(bq.cell_id);
            for (size_t k = 0; k < std::min<size_t>(4, vids.size()); ++k) {
                bq.quad_vertex_ids[k] = vids[k];
            }
        }
    }
    r_dominant.set_bridge_quads(std::move(synthesized_bridges));
}

template <FloatingPointScalar T>
typename TilingSeamStitcher2D<T>::Region TilingSeamStitcher2D<T>::populate_composite_region(
    std::span<const Region> p_regions,
    std::span<const int32_t> p_priorities,
    TilingStitchMode p_stitch_mode,
    T p_max_bridge_span
) {
    Region composite;
    if (p_regions.empty()) return composite;

    std::vector<size_t> order(p_regions.size());
    std::iota(order.begin(), order.end(), 0);

    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) noexcept {
        const int32_t pa = (a < p_priorities.size()) ? p_priorities[a] : p_regions[a].get_priority();
        const int32_t pb = (b < p_priorities.size()) ? p_priorities[b] : p_regions[b].get_priority();
        return pa < pb;
    });

    composite = p_regions[order[0]];
    for (size_t i = 1; i < order.size(); ++i) {
        merge_into_dominant(composite, p_regions[order[i]], p_stitch_mode, nullptr, p_max_bridge_span);
    }
    return composite;
}

// Explicit Instantiations
template struct TilingSeamStitcher2D<float>;
template struct TilingSeamStitcher2D<double>;

} // namespace ideam::core