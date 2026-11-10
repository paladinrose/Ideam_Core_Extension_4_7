#pragma once

/**
 * tiling_quotient_astar_and_flow_field_transform_logic.h
 * TileDOD Transform Task (T_T4): Symmetry-Accelerated A* Pathfinding & Eikonal Flow-Field Wavefront.
 *
 * Evaluates batch `TileNavRequestPOD` records across the CSR tile adjacency graph
 * (`iso_cell_buffer_id`, `cell_edge_csr_buffer_id`, `cell_neighbor_csr_buffer_id`,
 * `tile_edge_buffer_id`, `tile_edge_metadata_buffer_id`), respecting directional edge ports
 * (`PORT_FREE`, `PORT_IN_ONLY`, `PORT_OUT_ONLY`, `PORT_CLOSED`), and optionally computes a
 * global multi-goal Dijkstra / Eikonal wavefront (`flow_cost_float32_buffer_id` and
 * `flow_vector_vec2_buffer_id`).
 */

#include "../../../../tiles/tile_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <string_view>
#include <vector>

namespace ideam::core {

#ifndef IDEAM_TILES_NAV_REQUEST_POD_DEFINED
#define IDEAM_TILES_NAV_REQUEST_POD_DEFINED

/**
 * TileNavRequestPOD (32 Bytes, 16-Byte Aligned)
 * Batch navigation query descriptor for `TilingQuotientAStarAndFlowFieldTransformLogic`.
 */
struct alignas(16) TileNavRequestPOD {
    uint32_t start_cell_id     = 0xFFFFFFFFu; // Starting IsoCellPOD index
    uint32_t goal_cell_id      = 0xFFFFFFFFu; // Target IsoCellPOD index
    uint32_t path_slice_offset = 0u;          // Output start index in TILE_NAV_PATH_CELL_U32
    uint32_t max_path_capacity = 64u;         // Max cells allocated for this request's path slice
    uint32_t out_path_length   = 0u;          // Populated by T_T4: number of cells in found path
    float out_total_cost       = 0.0f;        // Populated by T_T4: total traversal cost
    uint8_t use_manhattan      = 0u;          // 1 = L1 heuristic, 0 = Euclidean L2 heuristic
    uint8_t emit_flow_goal     = 1u;          // 1 = include `goal_cell_id` as a seed in the Eikonal flow field
    uint16_t reserved          = 0u;
};

static_assert(sizeof(TileNavRequestPOD) == 32, "TileNavRequestPOD must be 32 bytes!");

#endif // IDEAM_TILES_NAV_REQUEST_POD_DEFINED

template <typename T = int32_t>
struct alignas(64) TilingQuotientAStarAndFlowFieldTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Tiling Quotient A* & Flow-Field";
    static constexpr std::string_view ui_category  = "Tiles";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id             = INVALID_ID; // 4B
    uint32_t iso_cell_buffer_id           = INVALID_ID; // 4B: IsoCellPOD (32B AoS)
    uint32_t cell_edge_csr_buffer_id      = INVALID_ID; // 4B: uint32_t CSR edge IDs
    uint32_t cell_neighbor_csr_buffer_id  = INVALID_ID; // 4B: int32_t CSR neighbor cell IDs
    uint32_t tile_edge_buffer_id          = INVALID_ID; // 4B: TileEdgePOD (16B AoS)
    uint32_t tile_edge_metadata_buffer_id = INVALID_ID; // 4B: TileEdgeMetadataPOD (16B AoS)
    uint32_t nav_request_buffer_id        = INVALID_ID; // 4B: TileNavRequestPOD (32B AoS)
    uint32_t nav_path_cell_u32_buffer_id  = INVALID_ID; // 4B: uint32_t output path cells
    uint32_t flow_cost_float32_buffer_id  = INVALID_ID; // 4B: float per-cell wavefront cost
    uint32_t flow_vector_vec2_buffer_id   = INVALID_ID; // 4B: glm::vec2 per-cell steering vector
    bool compute_global_flow_field        = true;       // 1B
    uint8_t reserved_padding[23]          = {0};        // 23B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char *p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("iso_cell_buffer_id");
        add_buf("cell_edge_csr_buffer_id");
        add_buf("cell_neighbor_csr_buffer_id");
        add_buf("tile_edge_buffer_id");
        add_buf("tile_edge_metadata_buffer_id");
        add_buf("nav_request_buffer_id");
        add_buf("nav_path_cell_u32_buffer_id");
        add_buf("flow_cost_float32_buffer_id");
        add_buf("flow_vector_vec2_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) {
        if (p_props.has("target_buffer_id"))             target_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("iso_cell_buffer_id"))           iso_cell_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["iso_cell_buffer_id"]));
        if (p_props.has("cell_edge_csr_buffer_id"))      cell_edge_csr_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["cell_edge_csr_buffer_id"]));
        if (p_props.has("cell_neighbor_csr_buffer_id"))  cell_neighbor_csr_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["cell_neighbor_csr_buffer_id"]));
        if (p_props.has("tile_edge_buffer_id"))          tile_edge_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["tile_edge_buffer_id"]));
        if (p_props.has("tile_edge_metadata_buffer_id")) tile_edge_metadata_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["tile_edge_metadata_buffer_id"]));
        if (p_props.has("nav_request_buffer_id"))        nav_request_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["nav_request_buffer_id"]));
        if (p_props.has("nav_path_cell_u32_buffer_id"))  nav_path_cell_u32_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["nav_path_cell_u32_buffer_id"]));
        if (p_props.has("flow_cost_float32_buffer_id"))  flow_cost_float32_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["flow_cost_float32_buffer_id"]));
        if (p_props.has("flow_vector_vec2_buffer_id"))   flow_vector_vec2_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["flow_vector_vec2_buffer_id"]));
        if (p_props.has("compute_global_flow_field"))    compute_global_flow_field    = static_cast<bool>(p_props["compute_global_flow_field"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : nav_request_buffer_id;
    }

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    [[nodiscard]] static inline bool _can_traverse_edge(
        uint32_t p_from_cell,
        const TileEdgePOD &p_edge,
        const TileEdgeMetadataPOD &p_meta
    ) noexcept {
        if (p_meta.port_dir == TilePortDirection::PORT_CLOSED || p_meta.cross_distance <= 0.0f) {
            return false;
        }
        if (p_meta.port_dir == TilePortDirection::PORT_FREE) return true;
        const bool from_is_a = (static_cast<int32_t>(p_from_cell) == p_edge.cell_a);
        return (p_meta.port_dir == TilePortDirection::PORT_OUT_ONLY) ? from_is_a : !from_is_a;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute_transform(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *cell_part = p_context.get_grant_part(iso_cell_buffer_id);
        const GrantPartPOD *nbr_part  = p_context.get_grant_part(cell_neighbor_csr_buffer_id);
        const GrantPartPOD *req_part  = p_context.get_grant_part(nav_request_buffer_id);
        if (!cell_part || !cell_part->raw_base_ptr ||
            !nbr_part || !nbr_part->raw_base_ptr ||
            !req_part || !req_part->raw_base_ptr) {
            return;
        }

        const int64_t n_cells = cell_part->selection.element_count;
        const int64_t n_nbrs  = nbr_part->selection.capacity;
        const int64_t n_reqs  = req_part->selection.element_count;
        if (n_cells <= 0) return;

        const auto *cells   = reinterpret_cast<const IsoCellPOD *>(cell_part->raw_base_ptr);
        const auto *nbr_csr = reinterpret_cast<const int32_t *>(nbr_part->raw_base_ptr);
        auto *reqs          = reinterpret_cast<TileNavRequestPOD *>(req_part->raw_base_ptr);

        const GrantPartPOD *ec_part   = p_context.get_grant_part(cell_edge_csr_buffer_id);
        const GrantPartPOD *edge_part = p_context.get_grant_part(tile_edge_buffer_id);
        const GrantPartPOD *meta_part = p_context.get_grant_part(tile_edge_metadata_buffer_id);
        const GrantPartPOD *path_part = p_context.get_grant_part(nav_path_cell_u32_buffer_id);

        const auto *edge_csr = (ec_part && ec_part->raw_base_ptr)
            ? reinterpret_cast<const uint32_t *>(ec_part->raw_base_ptr)
            : nullptr;
        const int64_t n_edge_csr = ec_part ? ec_part->selection.capacity : 0;

        const auto *edges = (edge_part && edge_part->raw_base_ptr)
            ? reinterpret_cast<const TileEdgePOD *>(edge_part->raw_base_ptr)
            : nullptr;
        const auto *metas = (meta_part && meta_part->raw_base_ptr)
            ? reinterpret_cast<const TileEdgeMetadataPOD *>(meta_part->raw_base_ptr)
            : nullptr;
        const int64_t n_edges = (edge_part && meta_part)
            ? std::min(edge_part->selection.capacity, meta_part->selection.capacity)
            : 0;

        auto *path_out = (path_part && path_part->raw_base_ptr)
            ? reinterpret_cast<uint32_t *>(path_part->raw_base_ptr)
            : nullptr;
        const int64_t max_path_out = path_part ? path_part->selection.capacity : 0;

        constexpr float INF = std::numeric_limits<float>::infinity();
        std::vector<float> g_score(static_cast<size_t>(n_cells), INF);
        std::vector<int32_t> came_from(static_cast<size_t>(n_cells), -1);
        using QNode = std::pair<float, uint32_t>;

        for (int64_t r = 0; r < n_reqs; ++r) {
            TileNavRequestPOD &req = reqs[r];
            req.out_path_length    = 0u;
            req.out_total_cost     = 0.0f;

            if (static_cast<int64_t>(req.start_cell_id) >= n_cells ||
                static_cast<int64_t>(req.goal_cell_id) >= n_cells) {
                continue;
            }

            std::fill(g_score.begin(), g_score.end(), INF);
            std::fill(came_from.begin(), came_from.end(), -1);

            std::priority_queue<QNode, std::vector<QNode>, std::greater<QNode>> open_set;
            g_score[req.start_cell_id] = 0.0f;
            open_set.push({0.0f, req.start_cell_id});

            const glm::vec2 goal_pos = cells[req.goal_cell_id].center;
            bool found = false;

            while (!open_set.empty()) {
                const auto [f_cur, u] = open_set.top();
                open_set.pop();
                (void)f_cur;

                if (u == req.goal_cell_id) {
                    found = true;
                    break;
                }

                const IsoCellPOD &cu = cells[u];
                const uint32_t deg   = cu.vertex_count;
                if (cu.first_edge_idx == INVALID_ID ||
                    static_cast<int64_t>(cu.first_edge_idx + deg) > n_nbrs) {
                    continue;
                }

                for (uint32_t k = 0; k < deg; ++k) {
                    const int32_t v_signed = nbr_csr[cu.first_edge_idx + k];
                    if (v_signed < 0 || static_cast<int64_t>(v_signed) >= n_cells) continue;
                    const uint32_t v = static_cast<uint32_t>(v_signed);

                    float step_cost = glm::length(cells[v].center - cu.center);
                    if (edge_csr && edges && metas &&
                        static_cast<int64_t>(cu.first_edge_idx + k) < n_edge_csr) {
                        const uint32_t eid = edge_csr[cu.first_edge_idx + k];
                        if (static_cast<int64_t>(eid) < n_edges) {
                            if (!_can_traverse_edge(u, edges[eid], metas[eid])) continue;
                            step_cost = std::max(1e-4f, metas[eid].cross_distance);
                        }
                    }

                    const float tentative_g = g_score[u] + step_cost;
                    if (tentative_g < g_score[v]) {
                        came_from[v] = static_cast<int32_t>(u);
                        g_score[v]   = tentative_g;
                        const glm::vec2 diff = goal_pos - cells[v].center;
                        const float h = (req.use_manhattan != 0u)
                            ? (std::abs(diff.x) + std::abs(diff.y))
                            : glm::length(diff);
                        open_set.push({tentative_g + h, v});
                    }
                }
            }

            if (found) {
                req.out_total_cost = g_score[req.goal_cell_id];
                std::vector<uint32_t> rev_path;
                int32_t cur = static_cast<int32_t>(req.goal_cell_id);
                while (cur >= 0) {
                    rev_path.push_back(static_cast<uint32_t>(cur));
                    if (static_cast<uint32_t>(cur) == req.start_cell_id) break;
                    cur = came_from[static_cast<size_t>(cur)];
                }
                std::reverse(rev_path.begin(), rev_path.end());
                const uint32_t write_len = std::min<uint32_t>(
                    static_cast<uint32_t>(rev_path.size()),
                    req.max_path_capacity
                );
                req.out_path_length = write_len;
                if (path_out && static_cast<int64_t>(req.path_slice_offset + write_len) <= max_path_out) {
                    for (uint32_t i = 0; i < write_len; ++i) {
                        path_out[req.path_slice_offset + i] = rev_path[i];
                    }
                }
            }
        }

        // Global Multi-Goal Eikonal / Dijkstra Flow Field
        const GrantPartPOD *fc_part = p_context.get_grant_part(flow_cost_float32_buffer_id);
        const GrantPartPOD *fv_part = p_context.get_grant_part(flow_vector_vec2_buffer_id);
        if (compute_global_flow_field &&
            fc_part && fc_part->raw_base_ptr &&
            fv_part && fv_part->raw_base_ptr &&
            fc_part->selection.capacity >= n_cells &&
            fv_part->selection.capacity >= n_cells) {
            auto *flow_costs = reinterpret_cast<float *>(fc_part->raw_base_ptr);
            auto *flow_vecs  = reinterpret_cast<glm::vec2 *>(fv_part->raw_base_ptr);

            std::fill_n(flow_costs, n_cells, INF);
            std::priority_queue<QNode, std::vector<QNode>, std::greater<QNode>> wave_pq;

            for (int64_t r = 0; r < n_reqs; ++r) {
                if (reqs[r].emit_flow_goal != 0u && static_cast<int64_t>(reqs[r].goal_cell_id) < n_cells) {
                    const uint32_t g = reqs[r].goal_cell_id;
                    flow_costs[g] = 0.0f;
                    wave_pq.push({0.0f, g});
                }
            }

            while (!wave_pq.empty()) {
                const auto [d_cur, u] = wave_pq.top();
                wave_pq.pop();
                if (d_cur > flow_costs[u]) continue;

                const IsoCellPOD &cu = cells[u];
                const uint32_t deg   = cu.vertex_count;
                if (cu.first_edge_idx == INVALID_ID ||
                    static_cast<int64_t>(cu.first_edge_idx + deg) > n_nbrs) {
                    continue;
                }

                for (uint32_t k = 0; k < deg; ++k) {
                    const int32_t nb_signed = nbr_csr[cu.first_edge_idx + k];
                    if (nb_signed < 0 || static_cast<int64_t>(nb_signed) >= n_cells) continue;
                    const uint32_t nb = static_cast<uint32_t>(nb_signed);

                    float step_cost = glm::length(cells[nb].center - cu.center);
                    if (edge_csr && edges && metas &&
                        static_cast<int64_t>(cu.first_edge_idx + k) < n_edge_csr) {
                        const uint32_t eid = edge_csr[cu.first_edge_idx + k];
                        if (static_cast<int64_t>(eid) < n_edges) {
                            if (!_can_traverse_edge(nb, edges[eid], metas[eid])) continue;
                            step_cost = std::max(1e-4f, metas[eid].cross_distance);
                        }
                    }
                    if (flow_costs[u] + step_cost < flow_costs[nb]) {
                        flow_costs[nb] = flow_costs[u] + step_cost;
                        wave_pq.push({flow_costs[nb], nb});
                    }
                }
            }

            for (int64_t c = 0; c < n_cells; ++c) {
                if (!std::isfinite(flow_costs[c]) || flow_costs[c] <= 1e-5f) {
                    flow_vecs[c] = glm::vec2(0.0f, 0.0f);
                    continue;
                }
                const IsoCellPOD &cu = cells[c];
                const uint32_t deg   = cu.vertex_count;
                float best_cost      = flow_costs[c];
                glm::vec2 best_dir(0.0f, 0.0f);
                if (cu.first_edge_idx != INVALID_ID &&
                    static_cast<int64_t>(cu.first_edge_idx + deg) <= n_nbrs) {
                    for (uint32_t k = 0; k < deg; ++k) {
                        const int32_t nb_signed = nbr_csr[cu.first_edge_idx + k];
                        if (nb_signed >= 0 && static_cast<int64_t>(nb_signed) < n_cells) {
                            const uint32_t nb = static_cast<uint32_t>(nb_signed);
                            if (flow_costs[nb] < best_cost) {
                                best_cost = flow_costs[nb];
                                const glm::vec2 delta = cells[nb].center - cu.center;
                                const float len = glm::length(delta);
                                if (len > 1e-5f) best_dir = delta / len;
                            }
                        }
                    }
                }
                flow_vecs[c] = best_dir;
            }
        }
    }
};

static_assert(
    sizeof(TilingQuotientAStarAndFlowFieldTransformLogic<>) == 64,
    "TilingQuotientAStarAndFlowFieldTransformLogic must be 64 bytes!"
);

} // namespace ideam::core
