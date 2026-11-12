#include "spherical_goldberg_polyhedron.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <utility>

namespace ideam::core {

template <FloatingPointScalar T>
SphericalGoldbergPolyhedron<T>::SphericalGoldbergPolyhedron(uint32_t p_frequency, T p_radius) noexcept {
    set_frequency(p_frequency);
    set_radius(p_radius);
}

template <FloatingPointScalar T>
void SphericalGoldbergPolyhedron<T>::build_goldberg_sphere(Region& r_out_topology_region) {
    r_out_topology_region.clear();
    cells_3d_.clear();

    const uint32_t f = std::clamp(frequency_, 1u, 24u);

    // 1. Canonical 12 vertices and 20 faces of the regular icosahedron
    const T phi = (static_cast<T>(1.0) + std::sqrt(static_cast<T>(5.0))) * static_cast<T>(0.5);
    const std::array<Vec3, 12> ico_verts = {{
        glm::normalize(Vec3(static_cast<T>(-1.0),  phi, static_cast<T>(0.0))),
        glm::normalize(Vec3( static_cast<T>(1.0),  phi, static_cast<T>(0.0))),
        glm::normalize(Vec3(static_cast<T>(-1.0), -phi, static_cast<T>(0.0))),
        glm::normalize(Vec3( static_cast<T>(1.0), -phi, static_cast<T>(0.0))),

        glm::normalize(Vec3(static_cast<T>(0.0), static_cast<T>(-1.0),  phi)),
        glm::normalize(Vec3(static_cast<T>(0.0),  static_cast<T>(1.0),  phi)),
        glm::normalize(Vec3(static_cast<T>(0.0), static_cast<T>(-1.0), -phi)),
        glm::normalize(Vec3(static_cast<T>(0.0),  static_cast<T>(1.0), -phi)),

        glm::normalize(Vec3( phi, static_cast<T>(0.0), static_cast<T>(-1.0))),
        glm::normalize(Vec3( phi, static_cast<T>(0.0),  static_cast<T>(1.0))),
        glm::normalize(Vec3(-phi, static_cast<T>(0.0), static_cast<T>(-1.0))),
        glm::normalize(Vec3(-phi, static_cast<T>(0.0),  static_cast<T>(1.0)))
    }};

    const std::array<glm::uvec3, 20> ico_faces = {{
        {0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
        {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
        {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1}
    }};

    // 2. Subdivide each icosahedral triangle into f^2 sub-triangles and deduplicate geodesic vertices
    std::vector<Vec3> geo_verts;
    std::vector<glm::uvec3> geo_tris;
    geo_verts.reserve(static_cast<size_t>(10u * f * f + 2u));
    geo_tris.reserve(static_cast<size_t>(20u * f * f));

    auto find_or_add_geo_vertex = [&](const Vec3& p_unit_pos) -> uint32_t {
        const T weld_cos = static_cast<T>(1.0) - static_cast<T>(1e-5) / static_cast<T>(f * f);
        for (size_t i = 0; i < geo_verts.size(); ++i) {
            if (glm::dot(geo_verts[i], p_unit_pos) >= weld_cos) {
                return static_cast<uint32_t>(i);
            }
        }
        const uint32_t id = static_cast<uint32_t>(geo_verts.size());
        geo_verts.push_back(p_unit_pos);
        return id;
    };

    for (const auto& face : ico_faces) {
        const Vec3 A = ico_verts[face.x];
        const Vec3 B = ico_verts[face.y];
        const Vec3 C = ico_verts[face.z];

        // Triangular barycentric grid index table [r][c] for r in 0..f, c in 0..(f - r)
        std::vector<std::vector<uint32_t>> grid_ids(f + 1u);
        for (uint32_t r = 0u; r <= f; ++r) {
            grid_ids[r].resize(f - r + 1u);
            for (uint32_t c = 0u; c <= f - r; ++c) {
                const uint32_t k = f - r - c;
                const Vec3 pos   = glm::normalize(
                    (A * static_cast<T>(k) + B * static_cast<T>(r) + C * static_cast<T>(c)) / static_cast<T>(f)
                );
                grid_ids[r][c] = find_or_add_geo_vertex(pos);
            }
        }

        for (uint32_t r = 0u; r < f; ++r) {
            for (uint32_t c = 0u; c < f - r; ++c) {
                geo_tris.emplace_back(grid_ids[r][c], grid_ids[r + 1u][c], grid_ids[r][c + 1u]);
                if (r + c + 1u < f) {
                    geo_tris.emplace_back(grid_ids[r + 1u][c], grid_ids[r + 1u][c + 1u], grid_ids[r][c + 1u]);
                }
            }
        }
    }

    // 3. Dualize: each geodesic vertex becomes a Goldberg polyhedron cell (pentagon or hexagon),
    //    and each incident geodesic triangle centroid becomes a vertex of that Goldberg cell!
    const size_t num_cells = geo_verts.size();
    const size_t num_tris  = geo_tris.size();

    std::vector<Vec3> tri_centers(num_tris);
    std::vector<std::vector<uint32_t>> vert_incident_tris(num_cells);
    std::vector<std::vector<uint32_t>> vert_neighbors(num_cells);

    for (size_t ti = 0; ti < num_tris; ++ti) {
        const glm::uvec3& t = geo_tris[ti];
        tri_centers[ti] = glm::normalize(geo_verts[t.x] + geo_verts[t.y] + geo_verts[t.z]);

        vert_incident_tris[t.x].push_back(static_cast<uint32_t>(ti));
        vert_incident_tris[t.y].push_back(static_cast<uint32_t>(ti));
        vert_incident_tris[t.z].push_back(static_cast<uint32_t>(ti));

        auto add_nbr = [&](uint32_t u, uint32_t v) {
            auto& list = vert_neighbors[u];
            if (std::find(list.begin(), list.end(), v) == list.end()) {
                list.push_back(v);
            }
        };
        add_nbr(t.x, t.y); add_nbr(t.y, t.x);
        add_nbr(t.y, t.z); add_nbr(t.z, t.y);
        add_nbr(t.z, t.x); add_nbr(t.x, t.z);
    }

    // Compute BFS generation rings from cell 0
    std::vector<uint16_t> gen_rings(num_cells, 0xFFFFu);
    if (num_cells > 0) {
        std::queue<uint32_t> bfs;
        gen_rings[0] = 0u;
        bfs.push(0u);
        while (!bfs.empty()) {
            const uint32_t u = bfs.front();
            bfs.pop();
            for (uint32_t v : vert_neighbors[u]) {
                if (gen_rings[v] == 0xFFFFu) {
                    gen_rings[v] = static_cast<uint16_t>(gen_rings[u] + 1u);
                    bfs.push(v);
                }
            }
        }
    }

    cells_3d_.resize(num_cells);
    std::vector<std::vector<Vec2>> chart_polygons_2d(num_cells);
    std::vector<IsoCellPOD> cell_templates(num_cells);
    std::vector<TileTransformPOD> cell_transforms(num_cells);

    for (size_t ci = 0; ci < num_cells; ++ci) {
        Cell3D& cell       = cells_3d_[ci];
        cell.cell_id       = static_cast<uint32_t>(ci);
        cell.unit_normal   = geo_verts[ci];
        cell.generation_ring = (gen_rings[ci] == 0xFFFFu) ? 0u : gen_rings[ci];

        // Orthonormal tangent plane basis (tangent_u, tangent_v) around unit_normal
        const Vec3 ref_up = (std::abs(cell.unit_normal.y) < static_cast<T>(0.9))
            ? Vec3(static_cast<T>(0.0), static_cast<T>(1.0), static_cast<T>(0.0))
            : Vec3(static_cast<T>(1.0), static_cast<T>(0.0), static_cast<T>(0.0));
        cell.tangent_u = glm::normalize(glm::cross(ref_up, cell.unit_normal));
        cell.tangent_v = glm::normalize(glm::cross(cell.unit_normal, cell.tangent_u));

        // Sort incident triangle centers CCW around unit_normal
        auto& inc_tris = vert_incident_tris[ci];
        std::sort(inc_tris.begin(), inc_tris.end(), [&](uint32_t ta, uint32_t tb) {
            const Vec3 da = tri_centers[ta] - cell.unit_normal;
            const Vec3 db = tri_centers[tb] - cell.unit_normal;
            const T ang_a = std::atan2(glm::dot(da, cell.tangent_v), glm::dot(da, cell.tangent_u));
            const T ang_b = std::atan2(glm::dot(db, cell.tangent_v), glm::dot(db, cell.tangent_u));
            return ang_a < ang_b;
        });

        const uint8_t val = static_cast<uint8_t>(std::min<size_t>(6u, inc_tris.size()));
        cell.valence      = val;
        cell.colour_id    = (val == 5u) ? 2u : static_cast<uint8_t>((ci + cell.generation_ring) % 3u);

        for (uint8_t k = 0u; k < val; ++k) {
            cell.unit_vertices[k] = tri_centers[inc_tris[k]];
        }

        // Sort neighbors in the same CCW angular order
        auto& nbrs = vert_neighbors[ci];
        std::sort(nbrs.begin(), nbrs.end(), [&](uint32_t na, uint32_t nb) {
            const Vec3 da = geo_verts[na] - cell.unit_normal;
            const Vec3 db = geo_verts[nb] - cell.unit_normal;
            const T ang_a = std::atan2(glm::dot(da, cell.tangent_v), glm::dot(da, cell.tangent_u));
            const T ang_b = std::atan2(glm::dot(db, cell.tangent_v), glm::dot(db, cell.tangent_u));
            return ang_a < ang_b;
        });
        for (size_t k = 0; k < std::min<size_t>(6u, nbrs.size()); ++k) {
            cell.neighbor_cell_ids[k] = static_cast<int32_t>(nbrs[k]);
        }

        // Stereographic / equirectangular 2D chart coordinates for TilingRegion2D & TilingNavigation2D
        const T lon = std::atan2(cell.unit_normal.x, cell.unit_normal.z);
        const T lat = std::asin(std::clamp(cell.unit_normal.y, static_cast<T>(-1.0), static_cast<T>(1.0)));
        const Vec2 center_2d(lon * radius_, lat * radius_);

        chart_polygons_2d[ci].resize(val);
        for (uint8_t k = 0u; k < val; ++k) {
            const Vec3 dv = cell.unit_vertices[k] - cell.unit_normal;
            chart_polygons_2d[ci][k] = center_2d + Vec2(
                glm::dot(dv, cell.tangent_u) * radius_,
                glm::dot(dv, cell.tangent_v) * radius_
            );
        }

        IsoCellPOD pod{};
        pod.cell_id      = static_cast<uint32_t>(ci);
        pod.vertex_count = val;
        pod.colour_id    = cell.colour_id;
        pod.generation   = cell.generation_ring;
        pod.centroid     = center_2d;
        cell_templates[ci] = pod;

        TileTransformPOD xf{};
        xf.col0 = Vec2(static_cast<T>(1.0), static_cast<T>(0.0));
        xf.col1 = Vec2(static_cast<T>(0.0), static_cast<T>(1.0));
        xf.col2 = center_2d;
        cell_transforms[ci] = xf;
    }

    r_out_topology_region.rebuild_topology_from_polygons(
        chart_polygons_2d,
        cell_templates,
        cell_transforms,
        static_cast<T>(1e-5)
    );

    // Explicitly wire exact spherical neighbor links into r_out_topology_region so A* and flow-fields
    // traverse seamlessly across the antimeridian and poles!
    auto reg_cells = r_out_topology_region.get_cells();
    auto reg_nbrs  = r_out_topology_region.get_cell_neighbor_indices_mut();
    auto reg_meta  = r_out_topology_region.get_edge_metadata_mut();

    for (size_t ci = 0; ci < num_cells && ci < reg_cells.size(); ++ci) {
        const auto& cell_3d   = cells_3d_[ci];
        const uint32_t e_base = reg_cells[ci].first_edge_idx;
        const uint8_t v_cnt   = reg_cells[ci].vertex_count;
        for (uint8_t k = 0u; k < v_cnt; ++k) {
            const int32_t nbr_id = cell_3d.neighbor_cell_ids[k];
            if (e_base + k < reg_nbrs.size()) {
                reg_nbrs[e_base + k] = nbr_id;
            }
            const auto cell_edges = r_out_topology_region.get_cell_edges(static_cast<uint32_t>(ci));
            if (k < cell_edges.size() && cell_edges[k] < reg_meta.size() && nbr_id >= 0) {
                reg_meta[cell_edges[k]].cross_distance = static_cast<float>(
                    spherical_distance_between_cells(static_cast<uint32_t>(ci), static_cast<uint32_t>(nbr_id))
                );
            }
        }
    }
}

template <FloatingPointScalar T>
int32_t SphericalGoldbergPolyhedron<T>::find_closest_cell(const Vec3& p_direction) const noexcept {
    if (cells_3d_.empty()) return -1;
    const Vec3 dir = glm::normalize(p_direction);
    int32_t best_id = 0;
    T best_dot      = glm::dot(cells_3d_[0].unit_normal, dir);
    for (size_t i = 1; i < cells_3d_.size(); ++i) {
        const T d = glm::dot(cells_3d_[i].unit_normal, dir);
        if (d > best_dot) {
            best_dot = d;
            best_id  = static_cast<int32_t>(i);
        }
    }
    return best_id;
}

template <FloatingPointScalar T>
T SphericalGoldbergPolyhedron<T>::spherical_distance_between_cells(
    uint32_t p_cell_a,
    uint32_t p_cell_b
) const noexcept {
    if (p_cell_a >= cells_3d_.size() || p_cell_b >= cells_3d_.size()) return static_cast<T>(0.0);
    const T c = std::clamp(
        glm::dot(cells_3d_[p_cell_a].unit_normal, cells_3d_[p_cell_b].unit_normal),
        static_cast<T>(-1.0),
        static_cast<T>(1.0)
    );
    return radius_ * std::acos(c);
}

template class SphericalGoldbergPolyhedron<float>;
template class SphericalGoldbergPolyhedron<double>;

} // namespace ideam::core