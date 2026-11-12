#pragma once

/**
 * graph_state_transition_transform_logic.h
 * Ecosystem Companion Bridge (GR1): Parallel FSM, Behavior Tree & Utility-AI Graph State Transitions.
 *
 * Evaluates parallel Finite State Machine (FSM), Behavior Tree (Sequence/Selector/Parallel/Action),
 * and Utility-AI transitions for thousands of agents indexing into a shared `IdeamGraphDOD` CSR
 * topology (`csr_row_offsets_buffer_id` / `csr_edge_counts_buffer_id` + `csr_edges_buffer_id`).
 */

#include "../../graphs/ideam_graph_dod.h"
#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GRAPH_STATE_TRANSITION_PODS_DEFINED
#define IDEAM_GRAPH_STATE_TRANSITION_PODS_DEFINED

enum GraphStateEvalMode : uint8_t {
    GRAPH_STATE_MODE_FSM           = 0, // Takes highest-priority outgoing edge whose condition_mask is satisfied
    GRAPH_STATE_MODE_BEHAVIOR_TREE = 1, // Evaluates BT composite/leaf nodes (Action, Sequence, Selector, Parallel)
    GRAPH_STATE_MODE_UTILITY_AI    = 2  // Selects outgoing edge maximizing utility_weight among valid conditions
};

enum GraphBTNodeType : uint8_t {
    GRAPH_BT_NODE_ACTION   = 0,
    GRAPH_BT_NODE_SEQUENCE = 1,
    GRAPH_BT_NODE_SELECTOR = 2,
    GRAPH_BT_NODE_PARALLEL = 3
};

enum GraphBTStatus : uint8_t {
    GRAPH_BT_STATUS_IDLE    = 0,
    GRAPH_BT_STATUS_RUNNING = 1,
    GRAPH_BT_STATUS_SUCCESS = 2,
    GRAPH_BT_STATUS_FAILURE = 3
};

enum GraphTransitionEdgeFlags : uint8_t {
    GRAPH_EDGE_FLAG_NONE            = 0u,
    GRAPH_EDGE_FLAG_INVERT_MASK     = 1u << 0, // Require condition_mask bits to be CLEAR instead of SET
    GRAPH_EDGE_FLAG_ANY_BIT_MATCH   = 1u << 1, // Match if ANY bit in condition_mask is set (default is ALL bits)
    GRAPH_EDGE_FLAG_IGNORE_DWELL    = 1u << 2  // Immediate interrupt edge bypassing min_dwell_time
};

/**
 * GraphTransitionEdgePOD (16 Bytes, 8-Byte Aligned)
 * Compact CSR transition edge descriptor for shared `IdeamGraphDOD` FSM / BT / AgentAIGraph topologies.
 */
struct alignas(8) GraphTransitionEdgePOD {
    uint32_t to_node_id     = INVALID_ID; // Target state / BT child NodeID
    uint32_t condition_mask = 0u;         // Required blackboard condition bitmask (0 = unconditional)
    float utility_weight    = 1.0f;       // Utility score multiplier for GRAPH_STATE_MODE_UTILITY_AI
    int16_t priority        = 0;          // Deterministic tie-breaker (higher wins)
    uint8_t bt_policy       = 0u;         // Behavior tree child index / policy tag
    uint8_t flags           = 0u;         // GraphTransitionEdgeFlags bitmask
};

static_assert(sizeof(GraphTransitionEdgePOD) == 16, "GraphTransitionEdgePOD must be 16 bytes!");

#endif // IDEAM_GRAPH_STATE_TRANSITION_PODS_DEFINED

template <typename T = int32_t, typename T_Strategy = FlatStrategy>
struct alignas(64) GraphStateTransitionTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::CUSTOM;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Graph State Transition";
    static constexpr std::string_view ui_category  = "Graphs/FSM";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t agent_state_buffer_id          = INVALID_ID;            // 4B: Per-agent active NodeID (uint32_t / int32_t)
    uint32_t agent_condition_mask_buffer_id = INVALID_ID;            // 4B: Per-agent active condition bitmask (uint32_t)
    uint32_t agent_state_timer_buffer_id    = INVALID_ID;            // 4B: Optional per-agent time-in-state float accumulator
    uint32_t agent_bt_status_buffer_id      = INVALID_ID;            // 4B: Optional per-agent GraphBTStatus (uint8_t / uint32_t)
    uint32_t csr_row_offsets_buffer_id      = INVALID_ID;            // 4B: CSR row offsets [N_nodes + 1] or BuildNodesSoA::output_edge_offset
    uint32_t csr_edge_counts_buffer_id      = INVALID_ID;            // 4B: Optional BuildNodesSoA::output_edge_count (uint32_t)
    uint32_t csr_edges_buffer_id            = INVALID_ID;            // 4B: GraphTransitionEdgePOD (16B), GraphEdgeData (20B), or uint32_t target IDs
    uint32_t node_type_buffer_id            = INVALID_ID;            // 4B: Optional BuildNodesSoA::type_id / GraphBTNodeType (uint32_t / uint8_t)
    uint32_t transition_count_buffer_id     = INVALID_ID;            // 4B: Optional uint32_t counter recording total transitions this tick
    uint32_t fallback_root_state_id         = 0u;                    // 4B: Fallback root NodeID if agent state is INVALID_ID
    float min_dwell_time                    = 0.0f;                  // 4B: Minimum dwell seconds before non-interrupt transition
    float time_scale                        = 1.0f;                  // 4B: Delta-time multiplier for state timers -> 48B

    uint8_t evaluation_mode                 = GRAPH_STATE_MODE_FSM;  // 1B: GraphStateEvalMode (0..2)
    uint8_t max_hops_per_tick               = 1u;                    // 1B: Max epsilon chain hops per agent per tick (1..8)
    bool reset_timer_on_transition          = true;                  // 1B: Reset agent_state_timer to 0.0f upon state change
    bool wake_transitioned_agents           = false;                 // 1B: Queue transitioned agents into wave_commands -> 52B
    uint8_t reserved_padding[12]            = {0};                   // 12B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("agent_state_buffer_id");
        add_buf("agent_condition_mask_buffer_id");
        add_buf("agent_state_timer_buffer_id");
        add_buf("agent_bt_status_buffer_id");
        add_buf("csr_row_offsets_buffer_id");
        add_buf("csr_edge_counts_buffer_id");
        add_buf("csr_edges_buffer_id");
        add_buf("node_type_buffer_id");
        add_buf("transition_count_buffer_id");

        godot::Dictionary mode_prop;
        mode_prop["name"]        = "evaluation_mode";
        mode_prop["type"]        = godot::Variant::INT;
        mode_prop["hint"]        = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "FSM Priority Match:0,Behavior Tree Step:1,Utility AI Max Weight:2";
        props.push_back(mode_prop);

        godot::Dictionary root_prop;
        root_prop["name"] = "fallback_root_state_id";
        root_prop["type"] = godot::Variant::INT;
        root_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(root_prop);

        godot::Dictionary dwell_prop;
        dwell_prop["name"]        = "min_dwell_time";
        dwell_prop["type"]        = godot::Variant::FLOAT;
        dwell_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        dwell_prop["hint_string"] = "0.0,60.0,0.01,suffix:s";
        props.push_back(dwell_prop);

        godot::Dictionary ts_prop;
        ts_prop["name"]        = "time_scale";
        ts_prop["type"]        = godot::Variant::FLOAT;
        ts_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        ts_prop["hint_string"] = "0.0,10.0,0.01,suffix:x";
        props.push_back(ts_prop);

        godot::Dictionary hops_prop;
        hops_prop["name"]        = "max_hops_per_tick";
        hops_prop["type"]        = godot::Variant::INT;
        hops_prop["hint"]        = godot::PROPERTY_HINT_RANGE;
        hops_prop["hint_string"] = "1,8,1";
        props.push_back(hops_prop);

        godot::Dictionary rst_prop;
        rst_prop["name"] = "reset_timer_on_transition";
        rst_prop["type"] = godot::Variant::BOOL;
        rst_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rst_prop);

        godot::Dictionary wake_prop;
        wake_prop["name"] = "wake_transitioned_agents";
        wake_prop["type"] = godot::Variant::BOOL;
        wake_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wake_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (agent_state_buffer_id != INVALID_ID) return agent_state_buffer_id;
        return agent_condition_mask_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("agent_state_buffer_id")) {
            agent_state_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_state_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && agent_state_buffer_id == INVALID_ID) {
            agent_state_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("agent_condition_mask_buffer_id")) {
            agent_condition_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_condition_mask_buffer_id"]));
        }
        if (p_props.has("agent_state_timer_buffer_id")) {
            agent_state_timer_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_state_timer_buffer_id"]));
        }
        if (p_props.has("agent_bt_status_buffer_id")) {
            agent_bt_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_bt_status_buffer_id"]));
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
        if (p_props.has("node_type_buffer_id")) {
            node_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["node_type_buffer_id"]));
        }
        if (p_props.has("transition_count_buffer_id")) {
            transition_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transition_count_buffer_id"]));
        }
        if (p_props.has("fallback_root_state_id")) {
            fallback_root_state_id = static_cast<uint32_t>( std::max<int64_t>(0, static_cast<int64_t>(p_props["fallback_root_state_id"])));
        }
        if (p_props.has("min_dwell_time")) {
            min_dwell_time = std::max(0.0f, static_cast<float>(p_props["min_dwell_time"]));
        }
        if (p_props.has("time_scale")) {
            time_scale = std::max(0.0f, static_cast<float>(p_props["time_scale"]));
        }
        if (p_props.has("evaluation_mode")) {
            evaluation_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["evaluation_mode"]), 0, 2));
        }
        if (p_props.has("max_hops_per_tick")) {
            max_hops_per_tick = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_hops_per_tick"]), 1, 8));
        }
        if (p_props.has("reset_timer_on_transition")) {
            reset_timer_on_transition = static_cast<bool>(p_props["reset_timer_on_transition"]);
        }
        if (p_props.has("wake_transitioned_agents")) {
            wake_transitioned_agents = static_cast<bool>(p_props["wake_transitioned_agents"]);
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
        if constexpr (!std::is_same_v<T, bool>) {
            const uint32_t state_buf_id = get_target_buffer_id();
            const GrantPartPOD* state_part = (state_buf_id != INVALID_ID)
                ? p_context.get_grant_part(state_buf_id)
                : ((p_context.grant && p_context.grant->part_count > 0) ? &p_context.grant->parts[0] : nullptr);
            if (!state_part || !state_part->raw_base_ptr) return;

            const GrantPartPOD* cond_part = (agent_condition_mask_buffer_id != INVALID_ID) ? p_context.get_grant_part(agent_condition_mask_buffer_id) : nullptr;
            const GrantPartPOD* tmr_part  = (agent_state_timer_buffer_id != INVALID_ID)    ? p_context.get_grant_part(agent_state_timer_buffer_id)    : nullptr;
            const GrantPartPOD* bt_part   = (agent_bt_status_buffer_id != INVALID_ID)      ? p_context.get_grant_part(agent_bt_status_buffer_id)      : nullptr;
            const GrantPartPOD* off_part  = (csr_row_offsets_buffer_id != INVALID_ID)      ? p_context.get_grant_part(csr_row_offsets_buffer_id)      : nullptr;
            const GrantPartPOD* cnt_part  = (csr_edge_counts_buffer_id != INVALID_ID)      ? p_context.get_grant_part(csr_edge_counts_buffer_id)      : nullptr;
            const GrantPartPOD* edg_part  = (csr_edges_buffer_id != INVALID_ID)            ? p_context.get_grant_part(csr_edges_buffer_id)            : nullptr;
            const GrantPartPOD* ntyp_part = (node_type_buffer_id != INVALID_ID)            ? p_context.get_grant_part(node_type_buffer_id)            : nullptr;
            const GrantPartPOD* tcnt_part = (transition_count_buffer_id != INVALID_ID)     ? p_context.get_grant_part(transition_count_buffer_id)     : nullptr;

            if (!off_part || !off_part->raw_base_ptr || !edg_part || !edg_part->raw_base_ptr) return;

            uint32_t* __restrict agent_states          = reinterpret_cast<uint32_t*>(state_part->raw_base_ptr);
            const uint32_t* __restrict agent_conds     = (cond_part && cond_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(cond_part->raw_base_ptr) : nullptr;
            float* __restrict agent_timers             = (tmr_part && tmr_part->raw_base_ptr)   ? reinterpret_cast<float*>(tmr_part->raw_base_ptr)           : nullptr;
            uint8_t* __restrict bt_raw                 = (bt_part && bt_part->raw_base_ptr)     ? bt_part->raw_base_ptr                                      : nullptr;
            const uint32_t* __restrict csr_offsets     = reinterpret_cast<const uint32_t*>(off_part->raw_base_ptr);
            const uint32_t* __restrict csr_counts      = (cnt_part && cnt_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(cnt_part->raw_base_ptr)  : nullptr;
            const uint8_t* __restrict edges_raw        = edg_part->raw_base_ptr;
            const uint8_t* __restrict node_types_raw   = (ntyp_part && ntyp_part->raw_base_ptr) ? ntyp_part->raw_base_ptr                                    : nullptr;
            uint32_t* __restrict transition_counter    = (tcnt_part && tcnt_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(tcnt_part->raw_base_ptr)       : nullptr;

            const int64_t agent_cap = static_cast<int64_t>(state_part->capacity_bytes / sizeof(uint32_t));
            const int64_t cond_cap  = agent_conds  ? static_cast<int64_t>(cond_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t tmr_cap   = agent_timers ? static_cast<int64_t>(tmr_part->capacity_bytes / sizeof(float))     : 0;
            const uint32_t bt_stride = bt_part ? (bt_part->element_stride > 0 ? bt_part->element_stride : 1u) : 0u;
            const int64_t bt_cap    = (bt_part && bt_stride > 0u) ? static_cast<int64_t>(bt_part->capacity_bytes / bt_stride) : 0;

            const int64_t off_cap   = static_cast<int64_t>(off_part->capacity_bytes / sizeof(uint32_t));
            const int64_t cnt_cap   = csr_counts ? static_cast<int64_t>(cnt_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t node_count = csr_counts ? std::min(off_cap, cnt_cap) : std::max<int64_t>(0, off_cap - 1);
            if (agent_cap <= 0 || node_count <= 0) return;

            const uint32_t edge_stride = (edg_part->element_stride > 0u)
                ? edg_part->element_stride
                : static_cast<uint32_t>(sizeof(GraphTransitionEdgePOD));
            const int64_t edge_cap = static_cast<int64_t>(edg_part->capacity_bytes / edge_stride);

            const uint32_t ntyp_stride = ntyp_part ? (ntyp_part->element_stride > 0u ? ntyp_part->element_stride : sizeof(uint32_t)) : 0u;
            const int64_t ntyp_cap = (ntyp_part && ntyp_stride > 0u) ? static_cast<int64_t>(ntyp_part->capacity_bytes / ntyp_stride) : 0;

            const float dt = static_cast<float>(p_context.delta) * time_scale;
            const uint8_t hops_limit = std::clamp<uint8_t>(max_hops_per_tick, 1u, 8u);
            uint32_t total_transitions = 0u;

            auto read_edge = [&](uint32_t e_idx) noexcept -> GraphTransitionEdgePOD {
                GraphTransitionEdgePOD out{};
                if (static_cast<int64_t>(e_idx) >= edge_cap) return out;
                const uint8_t* ptr = edges_raw + static_cast<size_t>(e_idx) * edge_stride;
                if (edge_stride >= sizeof(GraphEdgeData) && edge_stride != sizeof(GraphTransitionEdgePOD)) {
                    const auto* ged = reinterpret_cast<const GraphEdgeData*>(ptr);
                    out.to_node_id     = ged->to_node;
                    out.condition_mask = ged->from_port;
                    out.priority       = static_cast<int16_t>(ged->to_port);
                    out.utility_weight = 1.0f;
                } else if (edge_stride >= sizeof(GraphTransitionEdgePOD)) {
                    out = *reinterpret_cast<const GraphTransitionEdgePOD*>(ptr);
                } else if (edge_stride >= sizeof(uint32_t)) {
                    out.to_node_id = *reinterpret_cast<const uint32_t*>(ptr);
                }
                return out;
            };

            auto check_condition = [](uint32_t agent_mask, const GraphTransitionEdgePOD& edge) noexcept -> bool {
                if (edge.condition_mask == 0u) return true;
                const bool invert = (edge.flags & GRAPH_EDGE_FLAG_INVERT_MASK) != 0u;
                const bool any_bit = (edge.flags & GRAPH_EDGE_FLAG_ANY_BIT_MATCH) != 0u;
                const uint32_t matched = agent_mask & edge.condition_mask;
                const bool raw_pass = any_bit ? (matched != 0u) : (matched == edge.condition_mask);
                return invert ? !raw_pass : raw_pass;
            };

            auto read_node_type = [&](uint32_t nid) noexcept -> uint8_t {
                if (!node_types_raw || static_cast<int64_t>(nid) >= ntyp_cap) return GRAPH_BT_NODE_ACTION;
                if (ntyp_stride == sizeof(uint32_t)) {
                    return static_cast<uint8_t>(reinterpret_cast<const uint32_t*>(node_types_raw)[nid] & 0xFFu);
                }
                return node_types_raw[nid];
            };

            auto write_bt_status = [&](int64_t agent_idx, uint8_t status) noexcept {
                if (!bt_raw || agent_idx < 0 || agent_idx >= bt_cap) return;
                if (bt_stride == sizeof(uint32_t)) {
                    reinterpret_cast<uint32_t*>(bt_raw)[agent_idx] = static_cast<uint32_t>(status);
                } else {
                    bt_raw[agent_idx] = status;
                }
            };

            auto eval_agent = [&](int64_t idx) noexcept {
                uint32_t cur_state = agent_states[idx];
                if (cur_state == INVALID_ID || static_cast<int64_t>(cur_state) >= node_count) {
                    cur_state = (static_cast<int64_t>(fallback_root_state_id) < node_count) ? fallback_root_state_id : 0u;
                    agent_states[idx] = cur_state;
                }

                float cur_dwell = 0.0f;
                if (agent_timers && idx < tmr_cap) {
                    cur_dwell = agent_timers[idx] + dt;
                    agent_timers[idx] = cur_dwell;
                }

                const uint32_t cond_mask = (agent_conds && idx < cond_cap) ? agent_conds[idx] : 0xFFFFFFFFu;
                bool transitioned = false;

                for (uint8_t hop = 0u; hop < hops_limit; ++hop) {
                    if (static_cast<int64_t>(cur_state) >= node_count) break;

                    const uint32_t start_e = csr_offsets[cur_state];
                    const uint32_t count_e = csr_counts
                        ? csr_counts[cur_state]
                        : ((csr_offsets[cur_state + 1] >= start_e) ? (csr_offsets[cur_state + 1] - start_e) : 0u);
                    const uint32_t end_e = std::min<uint32_t>(start_e + count_e, static_cast<uint32_t>(edge_cap));
                    if (start_e >= end_e) break;

                    uint32_t next_state = INVALID_ID;

                    if (evaluation_mode == GRAPH_STATE_MODE_FSM) {
                        int16_t best_prio = std::numeric_limits<int16_t>::min();
                        for (uint32_t e = start_e; e < end_e; ++e) {
                            const GraphTransitionEdgePOD edge = read_edge(e);
                            if (edge.to_node_id == INVALID_ID || static_cast<int64_t>(edge.to_node_id) >= node_count) continue;
                            const bool ignore_dwell = (edge.flags & GRAPH_EDGE_FLAG_IGNORE_DWELL) != 0u;
                            if (!ignore_dwell && min_dwell_time > 0.0f && cur_dwell < min_dwell_time) continue;
                            if (check_condition(cond_mask, edge)) {
                                if (next_state == INVALID_ID || edge.priority > best_prio) {
                                    best_prio  = edge.priority;
                                    next_state = edge.to_node_id;
                                }
                            }
                        }
                    } else if (evaluation_mode == GRAPH_STATE_MODE_UTILITY_AI) {
                        float best_util = -std::numeric_limits<float>::infinity();
                        int16_t best_prio = std::numeric_limits<int16_t>::min();
                        for (uint32_t e = start_e; e < end_e; ++e) {
                            const GraphTransitionEdgePOD edge = read_edge(e);
                            if (edge.to_node_id == INVALID_ID || static_cast<int64_t>(edge.to_node_id) >= node_count) continue;
                            const bool ignore_dwell = (edge.flags & GRAPH_EDGE_FLAG_IGNORE_DWELL) != 0u;
                            if (!ignore_dwell && min_dwell_time > 0.0f && cur_dwell < min_dwell_time) continue;
                            if (check_condition(cond_mask, edge)) {
                                if (edge.utility_weight > best_util ||
                                    (edge.utility_weight == best_util && edge.priority > best_prio)) {
                                    best_util  = edge.utility_weight;
                                    best_prio  = edge.priority;
                                    next_state = edge.to_node_id;
                                }
                            }
                        }
                    } else {
                        // GRAPH_STATE_MODE_BEHAVIOR_TREE
                        const uint8_t ntype = read_node_type(cur_state);
                        if (ntype == GRAPH_BT_NODE_SEQUENCE) {
                            uint32_t last_passed = INVALID_ID;
                            bool all_passed = true;
                            for (uint32_t e = start_e; e < end_e; ++e) {
                                const GraphTransitionEdgePOD edge = read_edge(e);
                                if (edge.to_node_id == INVALID_ID || static_cast<int64_t>(edge.to_node_id) >= node_count) continue;
                                if (check_condition(cond_mask, edge)) {
                                    last_passed = edge.to_node_id;
                                } else {
                                    all_passed = false;
                                    break;
                                }
                            }
                            if (all_passed && last_passed != INVALID_ID) {
                                next_state = last_passed;
                                write_bt_status(idx, GRAPH_BT_STATUS_SUCCESS);
                            } else {
                                write_bt_status(idx, GRAPH_BT_STATUS_FAILURE);
                            }
                        } else {
                            // Selector / Parallel / Action step
                            for (uint32_t e = start_e; e < end_e; ++e) {
                                const GraphTransitionEdgePOD edge = read_edge(e);
                                if (edge.to_node_id == INVALID_ID || static_cast<int64_t>(edge.to_node_id) >= node_count) continue;
                                if (check_condition(cond_mask, edge)) {
                                    next_state = edge.to_node_id;
                                    break;
                                }
                            }
                            write_bt_status(idx, (next_state != INVALID_ID) ? GRAPH_BT_STATUS_RUNNING : GRAPH_BT_STATUS_FAILURE);
                        }
                    }

                    if (next_state != INVALID_ID && next_state != cur_state) {
                        cur_state    = next_state;
                        transitioned = true;
                        cur_dwell    = 0.0f;
                    } else {
                        break;
                    }
                }

                if (transitioned) {
                    agent_states[idx] = cur_state;
                    if (reset_timer_on_transition && agent_timers && idx < tmr_cap) {
                        agent_timers[idx] = 0.0f;
                    }
                    ++total_transitions;
                    if (wake_transitioned_agents && state_buf_id != INVALID_ID) {
                        p_context.queue_selection_command(state_buf_id, idx);
                    }
                }
            };

            const MemoryBufferSelectionPOD& sel = state_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                const int64_t cap = std::min<int64_t>(sel.capacity, agent_cap);
                for (int64_t i = 0; i < cap; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        eval_agent(i);
                    }
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    const int64_t idx = sel.data.indices[i];
                    if (idx >= 0 && idx < agent_cap) {
                        eval_agent(idx);
                    }
                }
            } else {
                const int64_t start = std::max<int64_t>(0, sel.start_index);
                const int64_t count = (sel.element_count > 0) ? sel.element_count : agent_cap;
                const int64_t end   = std::min<int64_t>(start + count, agent_cap);
                for (int64_t i = start; i < end; ++i) {
                    eval_agent(i);
                }
            }

            if (transition_counter) {
                *transition_counter = total_transitions;
            }
        }
    }
};

static_assert(sizeof(GraphStateTransitionTransformLogic<>) == 64, "GraphStateTransitionTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<GraphStateTransitionTransformLogic<>>, "GraphStateTransitionTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
