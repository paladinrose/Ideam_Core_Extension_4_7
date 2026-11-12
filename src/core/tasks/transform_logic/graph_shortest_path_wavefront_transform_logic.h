#pragma once

/**
 * graph_shortest_path_wavefront_transform_logic.h
 * Ecosystem Companion Bridge (GR2): Batched BFS / Dijkstra / A* Wavefronts over `IdeamGraphDOD` CSR Edge Tables.
 *
 * Supports both:
 *   1. Iterative Ping-Pong Wavefront Relaxation (`SwapView` / `FlatStrategy`) across CSR adjacency tables
 *      (`read_cost_buffer_id` -> `write_cost_buffer_id` + `predecessor_node_buffer_id`), and
 *   2. Batched Multi-Request BFS, Dijkstra, and Heuristic A* Path Queries (`GraphPathRequestPOD`)
 *      populating flattened path slices (`path_output_buffer_id`).
 */

#include "../../graphs/ideam_graph_dod.h"
#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ideam::core {

#ifndef IDEAM_GRAPH_SHORTEST_PATH_PODS_DEFINED
#define IDEAM_GRAPH_SHORTEST_PATH_PODS_DEFINED

enum GraphSearchAlgorithm : uint8_t {
    GRAPH_SEARCH_WAVEFRONT_RELAX = 0, // Parallel ping-pong SwapView Bellman-Ford / BFS wavefront step(s)
    GRAPH_SEARCH_BATCH_BFS       = 1, // Unweighted Breadth-First Search over CSR edge tables
    GRAPH_SEARCH_BATCH_DIJKSTRA  = 2, // Weighted Dijkstra shortest-path search
    GRAPH_SEARCH_BATCH_ASTAR     = 3  // Heuristic-guided A* search using node_heuristic_buffer_id
};

/**
 * GraphPathRequestPOD (32 Bytes, 16-Byte Aligned)
 * Batch pathfinding request descriptor for `GraphShortestPathWavefrontTransformLogic` (GR2).
 */
struct alignas(16) GraphPathRequestPOD {
    uint32_t start_node_id     = INVALID_ID; // Origin NodeID in IdeamGraphDOD
    uint32_t goal_node_id      = INVALID_ID; // Target NodeID in IdeamGraphDOD
    uint32_t path_slice_offset = 0u;         // Offset into path_output_buffer_id
    uint32_t max_path_capacity = 64u;        // Capacity of the allocated path slice
    uint32_t out_path_length   = 0u;         // Populated by GR2: number of NodeIDs written
    float out_total_cost       = 0.0f;       // Populated by GR2: total path cost
    uint8_t search_override    = 0xFFu;      // 0xFF = use task's default algorithm; 1=BFS, 2=Dijkstra, 3=A*
    uint8_t seed_wavefront     = 1u;         // 1 = seed goal_node_id (cost 0) in global wavefront
    uint16_t reserved          = 0u;
};

static_assert(sizeof(GraphPathRequestPOD) == 32, "GraphPathRequestPOD must be 32 bytes!");

#endif // IDEAM_GRAPH_SHORTEST_PATH_PODS_DEFINED

template <typename T = float, typename T_Strategy = FlatStrategy>
struct alignas(64) GraphShortestPathWavefrontTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA |
        BufferLayoutType::SPARSE_SET | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Graph Shortest Path Wavefront";
    static constexpr std::string_view ui_category  = "Graphs/Search";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t read_cost_buffer_id        = INVALID_ID;                   // 4B: Primary wavefront cost buffer (float / int32_t)
    uint32_t write_cost_buffer_id       = INVALID_ID;                   // 4B: Secondary ping-pong SwapView write cost buffer
    uint32_t csr_row_offsets_buffer_id  = INVALID_ID;                   // 4B: CSR row offsets [N_nodes + 1] or BuildNodesSoA::output_edge_offset
    uint32_t csr_edge_counts_buffer_id  = INVALID_ID;                   // 4B: Optional BuildNodesSoA::output_edge_count (uint32_t)
    uint32_t csr_edges_buffer_id        = INVALID_ID;                   // 4B: CSR target NodeIDs (uint32_t / int32_t) or GraphEdgeData
    uint32_t edge_weights_buffer_id     = INVALID_ID;                   // 4B: Optional per-edge traversal cost (float)
    uint32_t node_heuristic_buffer_id   = INVALID_ID;                   // 4B: Optional per-node A* heuristic h(n) (float)
    uint32_t predecessor_node_buffer_id = INVALID_ID;                   // 4B: Optional per-node parent/predecessor NodeID (uint32_t / int32_t)
    uint32_t path_request_buffer_id     = INVALID_ID;                   // 4B: Optional GraphPathRequestPOD batch buffer
    uint32_t path_output_buffer_id      = INVALID_ID;                   // 4B: Optional uint32_t reconstructed path NodeID buffer
    uint32_t source_seed_node_id        = INVALID_ID;                   // 4B: Optional single seed NodeID initialized to cost 0.0f
    float default_edge_weight           = 1.0f;                         // 4B: Uniform step cost when edge_weights_buffer_id is unbound -> 48B

    uint8_t search_algorithm            = GRAPH_SEARCH_WAVEFRONT_RELAX; // 1B: GraphSearchAlgorithm (0..3)
    uint8_t relaxation_iterations       = 1u;                           // 1B: Wavefront passes per tick (1..64)
    bool initialize_unseeded_to_inf     = false;                        // 1B: Reset non-seed nodes to infinity before wavefront
    bool wake_updated_nodes             = false;                        // 1B: Queue nodes whose cost improved into wave_commands -> 52B
    uint8_t reserved_padding[12]        = {0};                          // 12B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("read_cost_buffer_id");
        add_buf("write_cost_buffer_id");
        add_buf("csr_row_offsets_buffer_id");
        add_buf("csr_edge_counts_buffer_id");
        add_buf("csr_edges_buffer_id");
        add_buf("edge_weights_buffer_id");
        add_buf("node_heuristic_buffer_id");
        add_buf("predecessor_node_buffer_id");
        add_buf("path_request_buffer_id");
        add_buf("path_output_buffer_id");

        godot::Dictionary alg_prop;
        alg_prop["name"]        = "search_algorithm";
        alg_prop["type"]        = godot::Variant::INT;
        alg_prop["hint"]        = godot::PROPERTY_HINT_ENUM;
        alg_prop["hint_string"] = "Wavefront Relaxation (SwapView):0,Batch BFS:1,Batch Dijkstra:2,Batch A*:3";
        props.push_back(alg_prop);

        godot::Dictionary seed_prop;
        seed_prop["name"] = "source_seed_node_id";
        seed_prop["type"] = godot::Variant::INT;
        seed_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(seed_prop);

        godot::Dictionary w_prop;
        w_prop["name"]        = "default_edge_weight";
        w_prop["type"]        = godot::Variant::FLOAT;
        w_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        w_prop["hint_string"] = "0.001,1000.0,0.01";
        props.push_back(w_prop);

        godot::Dictionary iter_prop;
        iter_prop["name"]        = "relaxation_iterations";
        iter_prop["type"]        = godot::Variant::INT;
        iter_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        iter_prop["hint_string"] = "1,64,1";
        props.push_back(iter_prop);

        godot::Dictionary init_prop;
        init_prop["name"] = "initialize_unseeded_to_inf";
        init_prop["type"] = godot::Variant::BOOL;
        init_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(init_prop);

        godot::Dictionary wake_prop;
        wake_prop["name"] = "wake_updated_nodes";
        wake_prop["type"] = godot::Variant::BOOL;
        wake_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wake_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (read_cost_buffer_id != INVALID_ID)    return read_cost_buffer_id;
        if (write_cost_buffer_id != INVALID_ID)   return write_cost_buffer_id;
        return path_request_buffer_id;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { p_view.bind_secondary(p_part); }) {
            const GrantPartPOD* w_part = (write_cost_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(write_cost_buffer_id)
                : p_part;
            p_view.bind_secondary(w_part ? w_part : p_part);
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("read_cost_buffer_id")) {
            read_cost_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["read_cost_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && read_cost_buffer_id == INVALID_ID) {
            read_cost_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("write_cost_buffer_id")) {
            write_cost_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["write_cost_buffer_id"]));
        }
        if (p_props.has("csr_row_offsets_buffer_id")) {
            csr_row_offsets_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["csr_row_offsets_buffer_id"]));
        }
        if (p_props.has("csr_edge_counts_buffer_id")) {
            csr_edge_counts_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["csr_edge_counts_buffer_id"]));
        }
        if (p_props.has("csr_edges_buffer_id")) {
            csr_edges_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["csr_edges_buffer_id"]));
        }
        if (p_props.has("edge_weights_buffer_id")) {
            edge_weights_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["edge_weights_buffer_id"]));
        }
        if (p_props.has("node_heuristic_buffer_id")) {
            node_heuristic_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["node_heuristic_buffer_id"]));
        }
        if (p_props.has("predecessor_node_buffer_id")) {
            predecessor_node_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["predecessor_node_buffer_id"]));
        }
        if (p_props.has("path_request_buffer_id")) {
            path_request_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["path_request_buffer_id"]));
        }
        if (p_props.has("path_output_buffer_id")) {
            path_output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["path_output_buffer_id"]));
        }
        if (p_props.has("source_seed_node_id")) {
            source_seed_node_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_seed_node_id"]));
        }
        if (p_props.has("default_edge_weight")) {
            default_edge_weight = std::max(1e-5f, static_cast<float>(p_props["default_edge_weight"]));
        }
        if (p_props.has("search_algorithm")) {
            search_algorithm = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["search_algorithm"]), 0, 3));
        }
        if (p_props.has("relaxation_iterations")) {
            relaxation_iterations = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["relaxation_iterations"]), 1, 64));
        }
        if (p_props.has("initialize_unseeded_to_inf")) {
            initialize_unseeded_to_inf = static_cast<bool>(p_props["initialize_unseeded_to_inf"]);
        }
        if (p_props.has("wake_updated_nodes")) {
            wake_updated_nodes = static_cast<bool>(p_props["wake_updated_nodes"]);
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int32_t>) {
            const GrantPartPOD* off_part = (csr_row_offsets_buffer_id != INVALID_ID) ? p_context.get_grant_part(csr_row_offsets_buffer_id) : nullptr;
            const GrantPartPOD* cnt_part = (csr_edge_counts_buffer_id != INVALID_ID) ? p_context.get_grant_part(csr_edge_counts_buffer_id) : nullptr;
            const GrantPartPOD* edg_part = (csr_edges_buffer_id != INVALID_ID)       ? p_context.get_grant_part(csr_edges_buffer_id)       : nullptr;
            if (!off_part || !off_part->raw_base_ptr || !edg_part || !edg_part->raw_base_ptr) return;

            const GrantPartPOD* rcost_part = (read_cost_buffer_id != INVALID_ID)        ? p_context.get_grant_part(read_cost_buffer_id)        : nullptr;
            const GrantPartPOD* wcost_part = (write_cost_buffer_id != INVALID_ID)       ? p_context.get_grant_part(write_cost_buffer_id)       : rcost_part;
            const GrantPartPOD* ew_part    = (edge_weights_buffer_id != INVALID_ID)     ? p_context.get_grant_part(edge_weights_buffer_id)     : nullptr;
            const GrantPartPOD* heur_part  = (node_heuristic_buffer_id != INVALID_ID)   ? p_context.get_grant_part(node_heuristic_buffer_id)   : nullptr;
            const GrantPartPOD* pred_part  = (predecessor_node_buffer_id != INVALID_ID) ? p_context.get_grant_part(predecessor_node_buffer_id) : nullptr;
            const GrantPartPOD* req_part   = (path_request_buffer_id != INVALID_ID)     ? p_context.get_grant_part(path_request_buffer_id)     : nullptr;
            const GrantPartPOD* pout_part  = (path_output_buffer_id != INVALID_ID)      ? p_context.get_grant_part(path_output_buffer_id)      : nullptr;

            const uint32_t* __restrict csr_offsets = reinterpret_cast<const uint32_t*>(off_part->raw_base_ptr);
            const uint32_t* __restrict csr_counts  = (cnt_part && cnt_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(cnt_part->raw_base_ptr) : nullptr;
            const uint8_t* __restrict edges_raw    = edg_part->raw_base_ptr;
            const float* __restrict edge_weights   = (ew_part && ew_part->raw_base_ptr)   ? reinterpret_cast<const float*>(ew_part->raw_base_ptr)     : nullptr;
            const float* __restrict heuristics     = (heur_part && heur_part->raw_base_ptr) ? reinterpret_cast<const float*>(heur_part->raw_base_ptr) : nullptr;
            uint32_t* __restrict predecessors      = (pred_part && pred_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(pred_part->raw_base_ptr)    : nullptr;

            const int64_t off_cap    = static_cast<int64_t>(off_part->capacity_bytes / sizeof(uint32_t));
            const int64_t cnt_cap    = csr_counts ? static_cast<int64_t>(cnt_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t node_count = csr_counts ? std::min(off_cap, cnt_cap) : std::max<int64_t>(0, off_cap - 1);
            if (node_count <= 0) return;

            const uint32_t edge_stride = (edg_part->element_stride > 0u) ? edg_part->element_stride : static_cast<uint32_t>(sizeof(uint32_t));
            const int64_t edge_cap     = static_cast<int64_t>(edg_part->capacity_bytes / edge_stride);
            const int64_t ew_cap       = edge_weights ? static_cast<int64_t>(ew_part->capacity_bytes / sizeof(float)) : 0;
            const int64_t heur_cap     = heuristics   ? static_cast<int64_t>(heur_part->capacity_bytes / sizeof(float)) : 0;
            const int64_t pred_cap     = predecessors ? static_cast<int64_t>(pred_part->capacity_bytes / sizeof(uint32_t)) : 0;

            auto read_target_node = [&](uint32_t e_idx) noexcept -> uint32_t {
                if (static_cast<int64_t>(e_idx) >= edge_cap) return INVALID_ID;
                const uint8_t* ptr = edges_raw + static_cast<size_t>(e_idx) * edge_stride;
                if (edge_stride >= sizeof(GraphEdgeData)) {
                    return reinterpret_cast<const GraphEdgeData*>(ptr)->to_node;
                }
                return *reinterpret_cast<const uint32_t*>(ptr);
            };

            auto read_edge_cost = [&](uint32_t e_idx, bool unweighted_bfs) noexcept -> float {
                if (unweighted_bfs) return 1.0f;
                if (edge_weights && static_cast<int64_t>(e_idx) < ew_cap) {
                    return std::max(0.0f, edge_weights[e_idx]);
                }
                return default_edge_weight;
            };

            // 1. Batched Path Requests (BFS / Dijkstra / A*)
            if (req_part && req_part->raw_base_ptr) {
                auto* __restrict reqs = reinterpret_cast<GraphPathRequestPOD*>(req_part->raw_base_ptr);
                const int64_t n_reqs  = static_cast<int64_t>(req_part->capacity_bytes / sizeof(GraphPathRequestPOD));
                const int64_t active_reqs = (req_part->selection.element_count > 0)
                    ? std::min<int64_t>(req_part->selection.element_count, n_reqs)
                    : n_reqs;

                uint32_t* __restrict path_out = (pout_part && pout_part->raw_base_ptr)
                    ? reinterpret_cast<uint32_t*>(pout_part->raw_base_ptr)
                    : nullptr;
                const int64_t max_path_out = path_out ? static_cast<int64_t>(pout_part->capacity_bytes / sizeof(uint32_t)) : 0;

                constexpr float INF = std::numeric_limits<float>::infinity();
                std::vector<float> g_score(static_cast<size_t>(node_count), INF);
                std::vector<uint32_t> came_from(static_cast<size_t>(node_count), INVALID_ID);
                using QItem = std::pair<float, uint32_t>;

                for (int64_t r = 0; r < active_reqs; ++r) {
                    GraphPathRequestPOD& req = reqs[r];
                    req.out_path_length      = 0u;
                    req.out_total_cost       = 0.0f;

                    if (req.start_node_id == INVALID_ID || req.goal_node_id == INVALID_ID ||
                        static_cast<int64_t>(req.start_node_id) >= node_count ||
                        static_cast<int64_t>(req.goal_node_id) >= node_count) {
                        continue;
                    }

                    const uint8_t mode = (req.search_override <= GRAPH_SEARCH_BATCH_ASTAR) ? req.search_override : search_algorithm;
                    const bool is_bfs  = (mode == GRAPH_SEARCH_BATCH_BFS);
                    const bool is_astar= (mode == GRAPH_SEARCH_BATCH_ASTAR);

                    std::fill(g_score.begin(), g_score.end(), INF);
                    std::fill(came_from.begin(), came_from.end(), INVALID_ID);

                    std::priority_queue<QItem, std::vector<QItem>, std::greater<QItem>> open_pq;
                    g_score[req.start_node_id] = 0.0f;
                    open_pq.push({0.0f, req.start_node_id});
                    bool found = false;

                    while (!open_pq.empty()) {
                        const auto [f_val, u] = open_pq.top();
                        open_pq.pop();
                        (void)f_val;

                        if (u == req.goal_node_id) {
                            found = true;
                            break;
                        }

                        const uint32_t start_e = csr_offsets[u];
                        const uint32_t count_e = csr_counts
                            ? csr_counts[u]
                            : ((csr_offsets[u + 1] >= start_e) ? (csr_offsets[u + 1] - start_e) : 0u);
                        const uint32_t end_e = std::min<uint32_t>(start_e + count_e, static_cast<uint32_t>(edge_cap));

                        for (uint32_t e = start_e; e < end_e; ++e) {
                            const uint32_t v = read_target_node(e);
                            if (v == INVALID_ID || static_cast<int64_t>(v) >= node_count) continue;

                            const float step_cost   = read_edge_cost(e, is_bfs);
                            const float tentative_g = g_score[u] + step_cost;
                            if (tentative_g < g_score[v]) {
                                g_score[v]   = tentative_g;
                                came_from[v] = u;
                                const float h = (is_astar && heuristics && static_cast<int64_t>(v) < heur_cap)
                                    ? heuristics[v]
                                    : 0.0f;
                                open_pq.push({tentative_g + h, v});
                            }
                        }
                    }

                    if (found) {
                        req.out_total_cost = g_score[req.goal_node_id];
                        std::vector<uint32_t> rev_path;
                        uint32_t cur = req.goal_node_id;
                        size_t guard = 0;
                        while (cur != INVALID_ID && guard <= static_cast<size_t>(node_count)) {
                            rev_path.push_back(cur);
                            if (cur == req.start_node_id) break;
                            cur = came_from[cur];
                            ++guard;
                        }
                        std::reverse(rev_path.begin(), rev_path.end());
                        const uint32_t write_len = std::min<uint32_t>(
                            static_cast<uint32_t>(rev_path.size()),
                            req.max_path_capacity
                        );
                        req.out_path_length = write_len;
                        if (path_out && static_cast<int64_t>(req.path_slice_offset + write_len) <= max_path_out) {
                            for (uint32_t i = 0u; i < write_len; ++i) {
                                path_out[req.path_slice_offset + i] = rev_path[i];
                            }
                        }
                        if (predecessors) {
                            const int64_t copy_n = std::min<int64_t>(node_count, pred_cap);
                            for (int64_t n = 0; n < copy_n; ++n) {
                                if (came_from[static_cast<size_t>(n)] != INVALID_ID) {
                                    predecessors[n] = came_from[static_cast<size_t>(n)];
                                }
                            }
                        }
                    }
                }
            }

            // 2. Ping-Pong Wavefront Relaxation over Read/Write Cost Buffers (SwapView)
            if (rcost_part && rcost_part->raw_base_ptr && wcost_part && wcost_part->raw_base_ptr) {
                const int64_t r_cap = static_cast<int64_t>(rcost_part->capacity_bytes / sizeof(T));
                const int64_t w_cap = static_cast<int64_t>(wcost_part->capacity_bytes / sizeof(T));
                const int64_t n_nodes = std::min({node_count, r_cap, w_cap});
                if (n_nodes <= 0) return;

                T* r_costs = reinterpret_cast<T*>(rcost_part->raw_base_ptr);
                T* w_costs = reinterpret_cast<T*>(wcost_part->raw_base_ptr);

                const T inf_val = std::is_floating_point_v<T>
                    ? std::numeric_limits<T>::infinity()
                    : static_cast<T>(std::numeric_limits<int32_t>::max() / 4);

                if (initialize_unseeded_to_inf) {
                    for (int64_t i = 0; i < n_nodes; ++i) {
                        r_costs[i] = inf_val;
                        w_costs[i] = inf_val;
                    }
                }
                if (source_seed_node_id != INVALID_ID && static_cast<int64_t>(source_seed_node_id) < n_nodes) {
                    r_costs[source_seed_node_id] = static_cast<T>(0);
                    w_costs[source_seed_node_id] = static_cast<T>(0);
                }

                const uint8_t iters = std::clamp<uint8_t>(relaxation_iterations, 1u, 64u);
                const bool unweighted = (search_algorithm == GRAPH_SEARCH_BATCH_BFS);
                const uint32_t wake_buf_id = (write_cost_buffer_id != INVALID_ID) ? write_cost_buffer_id : read_cost_buffer_id;

                for (uint8_t it = 0u; it < iters; ++it) {
                    if (r_costs != w_costs) {
                        for (int64_t i = 0; i < n_nodes; ++i) {
                            if (r_costs[i] < w_costs[i]) {
                                w_costs[i] = r_costs[i];
                            }
                        }
                    }

                    bool any_improved = false;
                    for (int64_t u = 0; u < n_nodes; ++u) {
                        const T u_cost = r_costs[u];
                        if (u_cost >= inf_val) continue;

                        const uint32_t start_e = csr_offsets[u];
                        const uint32_t count_e = csr_counts
                            ? csr_counts[u]
                            : ((csr_offsets[u + 1] >= start_e) ? (csr_offsets[u + 1] - start_e) : 0u);
                        const uint32_t end_e = std::min<uint32_t>(start_e + count_e, static_cast<uint32_t>(edge_cap));

                        for (uint32_t e = start_e; e < end_e; ++e) {
                            const uint32_t v = read_target_node(e);
                            if (v == INVALID_ID || static_cast<int64_t>(v) >= n_nodes) continue;

                            const T step = static_cast<T>(read_edge_cost(e, unweighted));
                            const T cand = u_cost + step;
                            if (cand < w_costs[v]) {
                                w_costs[v]   = cand;
                                any_improved = true;
                                if (predecessors && static_cast<int64_t>(v) < pred_cap) {
                                    predecessors[v] = static_cast<uint32_t>(u);
                                }
                                if (wake_updated_nodes && it + 1u == iters && wake_buf_id != INVALID_ID) {
                                    p_context.queue_selection_command(wake_buf_id, static_cast<int64_t>(v));
                                }
                            }
                        }
                    }

                    if (r_costs != w_costs) {
                        for (int64_t i = 0; i < n_nodes; ++i) {
                            r_costs[i] = w_costs[i];
                        }
                    }
                    if (!any_improved) break;
                }
            }
        }
    }
};

static_assert(sizeof(GraphShortestPathWavefrontTransformLogic<>) == 64, "GraphShortestPathWavefrontTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<GraphShortestPathWavefrontTransformLogic<>>, "GraphShortestPathWavefrontTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
