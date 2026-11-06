#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../transform_logic/entity_time_scale_cascade_transform_logic.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_AGENT_EXCLUSIVITY_ENUMS_DEFINED
#define IDEAM_GAMES_AGENT_EXCLUSIVITY_ENUMS_DEFINED

/**
 * DOD AgentExclusivity modes (mirrors ideam::godot_ext::AgentExclusivity in game_agent.h).
 */
enum GameAgentExclusivityMode : uint8_t {
    AGENT_EXCLUSIVITY_NONE            = 0, // Multiple agents can be controlled simultaneously without restriction
    AGENT_EXCLUSIVITY_EXCLUSIVE       = 1, // Controlling this agent disconnects all other controlled agents
    AGENT_EXCLUSIVITY_GROUP_INCLUSIVE = 2, // Retains only controlled agents that share at least one group bit
    AGENT_EXCLUSIVITY_GROUP_EXCLUSIVE = 3  // Disconnects controlled agents that share any group bit
};

/**
 * Evaluation modes for AgentExclusivityFilterQueryLogic (G15).
 */
enum AgentExclusivityFilterMode : uint8_t {
    EXCLUSIVITY_FILTER_RETAIN_CONTROLLED     = 0, // Keeps controlled agents that survive new_agent possession (!do_disconnect)
    EXCLUSIVITY_FILTER_RELEASE_CONFLICTING   = 1, // Selects controlled agents that must be disconnected (do_disconnect)
    EXCLUSIVITY_FILTER_CANDIDATE_POSSESSABLE = 2  // Selects candidate agents eligible for possession by requesting_player_id
};

#endif // IDEAM_GAMES_AGENT_EXCLUSIVITY_ENUMS_DEFINED

/**
 * AgentExclusivityFilterQueryLogic<T> (G15)
 * Filters candidate or currently controlled GameAgents by AgentExclusivity
 * (NONE, EXCLUSIVE, GROUP_INCLUSIVE, GROUP_EXCLUSIVE) and 64-bit group masks during possession requests.
 * Replaces the nested TypedArray<GameAgent> and TypedArray<StringName> group loops in GamePlayer::control_game_agent().
 */
template <typename T = int32_t>
struct alignas(64) AgentExclusivityFilterQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull          = true;
    static constexpr bool supports_addition      = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Agent Exclusivity Filter";
    static constexpr std::string_view ui_category  = "Agents";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint64_t new_agent_group_mask            = 0ULL;                                    // 8B: 64-bit group mask of the newly possessed agent
    uint32_t target_buffer_id                = INVALID_ID;                              // 4B: Primary GameAgent selection buffer
    uint32_t exclusivity_buffer_id           = INVALID_ID;                              // 4B: AgentExclusivity (int32_t) column
    uint32_t group_mask_buffer_id            = INVALID_ID;                              // 4B: Optional uint64_t group_mask column
    uint32_t controlling_player_id_buffer_id = INVALID_ID;                              // 4B: Optional uint32_t player_id column (INVALID_ID = none)
    uint32_t active_flags_buffer_id          = INVALID_ID;                              // 4B: Optional EntityHierarchyBuffer::active_flags (uint32_t)
    uint32_t new_agent_idx                   = INVALID_ID;                              // 4B: Optional index of the newly possessed GameAgent
    uint32_t requesting_player_id            = 0u;                                      // 4B: Player ID requesting possession

    uint8_t new_agent_exclusivity            = AGENT_EXCLUSIVITY_EXCLUSIVE;             // 1B: GameAgentExclusivityMode
    uint8_t filter_mode                      = EXCLUSIVITY_FILTER_RETAIN_CONTROLLED;    // 1B: AgentExclusivityFilterMode
    bool require_enabled                     = true;                                    // 1B: Require ENTITY_FLAG_ENABLED on candidates
    bool allow_already_controlled_by_self    = true;                                    // 1B: Allow agents already controlled by requesting_player_id
    uint8_t reserved_padding[24]             = {0};                                     // 24B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary exc_prop;
        exc_prop["name"] = "exclusivity_buffer_id";
        exc_prop["type"] = godot::Variant::INT;
        exc_prop["hint_string"] = "buffer_option";
        props.push_back(exc_prop);

        godot::Dictionary grp_prop;
        grp_prop["name"] = "group_mask_buffer_id";
        grp_prop["type"] = godot::Variant::INT;
        grp_prop["hint_string"] = "buffer_option";
        props.push_back(grp_prop);

        godot::Dictionary cpl_prop;
        cpl_prop["name"] = "controlling_player_id_buffer_id";
        cpl_prop["type"] = godot::Variant::INT;
        cpl_prop["hint_string"] = "buffer_option";
        props.push_back(cpl_prop);

        godot::Dictionary flg_prop;
        flg_prop["name"] = "active_flags_buffer_id";
        flg_prop["type"] = godot::Variant::INT;
        flg_prop["hint_string"] = "buffer_option";
        props.push_back(flg_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "filter_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Retain Controlled (!disconnect):0,Release Conflicting (disconnect):1,Candidate Possessable:2";
        props.push_back(mode_prop);

        godot::Dictionary nexc_prop;
        nexc_prop["name"] = "new_agent_exclusivity";
        nexc_prop["type"] = godot::Variant::INT;
        nexc_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        nexc_prop["hint_string"] = "None:0,Exclusive:1,Group Inclusive:2,Group Exclusive:3";
        props.push_back(nexc_prop);

        godot::Dictionary ngm_prop;
        ngm_prop["name"] = "new_agent_group_mask";
        ngm_prop["type"] = godot::Variant::INT;
        ngm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ngm_prop);

        godot::Dictionary nidx_prop;
        nidx_prop["name"] = "new_agent_idx";
        nidx_prop["type"] = godot::Variant::INT;
        nidx_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(nidx_prop);

        godot::Dictionary rpl_prop;
        rpl_prop["name"] = "requesting_player_id";
        rpl_prop["type"] = godot::Variant::INT;
        rpl_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rpl_prop);

        godot::Dictionary en_prop;
        en_prop["name"] = "require_enabled";
        en_prop["type"] = godot::Variant::BOOL;
        en_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(en_prop);

        godot::Dictionary slf_prop;
        slf_prop["name"] = "allow_already_controlled_by_self";
        slf_prop["type"] = godot::Variant::BOOL;
        slf_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(slf_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)      return target_buffer_id;
        if (exclusivity_buffer_id != INVALID_ID) return exclusivity_buffer_id;
        return active_flags_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("exclusivity_buffer_id")) {
            exclusivity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["exclusivity_buffer_id"]));
        }
        if (p_props.has("group_mask_buffer_id")) {
            group_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["group_mask_buffer_id"]));
        }
        if (p_props.has("controlling_player_id_buffer_id")) {
            controlling_player_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["controlling_player_id_buffer_id"]));
        }
        if (p_props.has("active_flags_buffer_id")) {
            active_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["active_flags_buffer_id"]));
        }
        if (p_props.has("new_agent_group_mask")) {
            new_agent_group_mask = static_cast<uint64_t>(static_cast<int64_t>(p_props["new_agent_group_mask"]));
        }
        if (p_props.has("new_agent_idx")) {
            new_agent_idx = static_cast<uint32_t>(static_cast<int64_t>(p_props["new_agent_idx"]));
        }
        if (p_props.has("requesting_player_id")) {
            requesting_player_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["requesting_player_id"]));
        }
        if (p_props.has("new_agent_exclusivity")) {
            new_agent_exclusivity = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["new_agent_exclusivity"]), 0, 3));
        }
        if (p_props.has("filter_mode")) {
            filter_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["filter_mode"]), 0, 2));
        }
        if (p_props.has("require_enabled")) {
            require_enabled = static_cast<bool>(p_props["require_enabled"]);
        }
        if (p_props.has("allow_already_controlled_by_self")) {
            allow_already_controlled_by_self = static_cast<bool>(p_props["allow_already_controlled_by_self"]);
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_context,
        const T_View& p_view
    ) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, int32_t>) {
            const uint32_t tgt_id = get_target_buffer_id();
            const uint32_t exc_id = (exclusivity_buffer_id != INVALID_ID) ? exclusivity_buffer_id : tgt_id;

            const GrantPartPOD* exc_part = (exc_id != INVALID_ID)                          ? p_context.get_grant_part(exc_id)                          : nullptr;
            const GrantPartPOD* grp_part = (group_mask_buffer_id != INVALID_ID)            ? p_context.get_grant_part(group_mask_buffer_id)            : nullptr;
            const GrantPartPOD* cpl_part = (controlling_player_id_buffer_id != INVALID_ID) ? p_context.get_grant_part(controlling_player_id_buffer_id) : nullptr;
            const GrantPartPOD* flg_part = (active_flags_buffer_id != INVALID_ID)          ? p_context.get_grant_part(active_flags_buffer_id)          : nullptr;

            const int32_t* __restrict exclusivities = (exc_part && exc_part->raw_base_ptr)
                ? reinterpret_cast<const int32_t*>(exc_part->raw_base_ptr)
                : nullptr;
            const uint64_t* __restrict group_masks64 = (grp_part && grp_part->raw_base_ptr && grp_part->element_stride == sizeof(uint64_t))
                ? reinterpret_cast<const uint64_t*>(grp_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict group_masks32 = (grp_part && grp_part->raw_base_ptr && grp_part->element_stride == sizeof(uint32_t))
                ? reinterpret_cast<const uint32_t*>(grp_part->raw_base_ptr)
                : r_selection.group_masks;
            const uint32_t* __restrict ctrl_players = (cpl_part && cpl_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(cpl_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict active_flags = (flg_part && flg_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(flg_part->raw_base_ptr)
                : nullptr;

            const int64_t exc_cap = exclusivities ? static_cast<int64_t>(exc_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t grp_cap = grp_part
                ? static_cast<int64_t>(grp_part->capacity_bytes / (grp_part->element_stride > 0 ? grp_part->element_stride : sizeof(uint64_t)))
                : r_selection.capacity;
            const int64_t cpl_cap = ctrl_players  ? static_cast<int64_t>(cpl_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t flg_cap = active_flags  ? static_cast<int64_t>(flg_part->capacity_bytes / sizeof(uint32_t)) : 0;

            auto read_group_mask = [&](int64_t idx) noexcept -> uint64_t {
                if (idx < 0 || idx >= grp_cap) return 0ULL;
                if (group_masks64) return group_masks64[idx];
                if (group_masks32) return static_cast<uint64_t>(group_masks32[idx]);
                return 0ULL;
            };

            uint8_t eff_new_exc   = new_agent_exclusivity;
            uint64_t eff_new_grp  = new_agent_group_mask;
            if (new_agent_idx != INVALID_ID) {
                const int64_t nidx = static_cast<int64_t>(new_agent_idx);
                if (exclusivities && nidx >= 0 && nidx < exc_cap) {
                    eff_new_exc = static_cast<uint8_t>(std::clamp<int32_t>(exclusivities[nidx], 0, 3));
                }
                if (eff_new_grp == 0ULL) {
                    eff_new_grp = read_group_mask(nidx);
                }
            }

            auto evaluate_agent = [&](int64_t idx) noexcept -> bool {
                if (require_enabled && active_flags && idx >= 0 && idx < flg_cap) {
                    if ((active_flags[idx] & ENTITY_FLAG_ENABLED) == 0u) {
                        return false;
                    }
                }

                const uint64_t agent_grp = read_group_mask(idx);

                if (filter_mode == EXCLUSIVITY_FILTER_CANDIDATE_POSSESSABLE) {
                    if (ctrl_players && idx >= 0 && idx < cpl_cap) {
                        const uint32_t cur_owner = ctrl_players[idx];
                        if (cur_owner != INVALID_ID) {
                            if (cur_owner == requesting_player_id) {
                                if (!allow_already_controlled_by_self) return false;
                            } else {
                                return false; // Possessed by another player
                            }
                        }
                    }
                    if (eff_new_grp != 0ULL) {
                        const uint8_t cand_exc = (exclusivities && idx >= 0 && idx < exc_cap)
                            ? static_cast<uint8_t>(std::clamp<int32_t>(exclusivities[idx], 0, 3))
                            : eff_new_exc;
                        if (cand_exc == AGENT_EXCLUSIVITY_GROUP_INCLUSIVE && ((agent_grp & eff_new_grp) == 0ULL)) {
                            return false;
                        }
                        if (cand_exc == AGENT_EXCLUSIVITY_GROUP_EXCLUSIVE && ((agent_grp & eff_new_grp) != 0ULL)) {
                            return false;
                        }
                    }
                    return true;
                }

                // Exact GamePlayer::control_game_agent() exclusivity evaluation on controlled agents:
                // New agent itself is always retained (never disconnected)
                if (new_agent_idx != INVALID_ID && static_cast<uint32_t>(idx) == new_agent_idx) {
                    return (filter_mode == EXCLUSIVITY_FILTER_RETAIN_CONTROLLED);
                }

                bool do_disconnect = false;
                if (eff_new_exc == AGENT_EXCLUSIVITY_EXCLUSIVE) {
                    do_disconnect = true;
                } else if (eff_new_exc == AGENT_EXCLUSIVITY_GROUP_INCLUSIVE) {
                    do_disconnect = ((agent_grp & eff_new_grp) == 0ULL);
                } else if (eff_new_exc == AGENT_EXCLUSIVITY_GROUP_EXCLUSIVE) {
                    do_disconnect = ((agent_grp & eff_new_grp) != 0ULL);
                }

                return (filter_mode == EXCLUSIVITY_FILTER_RELEASE_CONFLICTING) ? do_disconnect : !do_disconnect;
            };

            _apply_query_op<Op>(r_selection, p_context, tgt_id, evaluate_agent);
        }
    }

private:
    template <QueryOp Op, typename PredFn>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _apply_query_op(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_context,
        uint32_t p_target_id,
        PredFn&& p_eval
    ) noexcept {
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                uint64_t* bitset = r_selection.data.bitset;
                for (int64_t i = 0; i < r_selection.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        if (!p_eval(i)) {
                            bitset[i >> 6] &= ~(1ULL << (i & 63));
                            --r_selection.element_count;
                        }
                    }
                }
            } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                int64_t write_ptr = 0;
                int64_t* indices  = r_selection.data.indices;
                for (int64_t i = 0; i < r_selection.element_count; ++i) {
                    const int64_t idx = indices[i];
                    if (p_eval(idx)) {
                        indices[write_ptr++] = idx;
                    }
                }
                r_selection.element_count = write_ptr;
            } else if (r_selection.mode == SelectionMode::RANGE && r_selection.data.indices) {
                const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                const int64_t end   = std::min<int64_t>(start + r_selection.element_count, r_selection.capacity);
                int64_t write_ptr   = 0;
                for (int64_t i = start; i < end; ++i) {
                    if (p_eval(i)) {
                        r_selection.data.indices[write_ptr++] = i;
                    }
                }
                r_selection.mode          = SelectionMode::SPARSE;
                r_selection.start_index   = 0;
                r_selection.element_count = write_ptr;
            }
        } else if constexpr (Op == QueryOp::ADD) {
            if (r_selection.unclaimed_mask) {
                const uint64_t* unclaimed = r_selection.unclaimed_mask;
                const int64_t words = (r_selection.capacity + 63) >> 6;
                for (int64_t w = 0; w < words; ++w) {
                    uint64_t mask = unclaimed[w];
                    while (mask != 0ULL) {
                        const int bit_index      = std::countr_zero(mask);
                        const int64_t global_idx = (w << 6) + bit_index;
                        if (global_idx >= r_selection.capacity) break;
                        if (p_eval(global_idx)) {
                            p_context.queue_selection_command(p_target_id, global_idx);
                        }
                        mask &= (mask - 1ULL);
                    }
                }
            } else {
                for (int64_t i = 0; i < r_selection.capacity; ++i) {
                    if (p_eval(i)) {
                        p_context.queue_selection_command(p_target_id, i);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(AgentExclusivityFilterQueryLogic<>) == 64, "AgentExclusivityFilterQueryLogic must be exactly 64 bytes!");

} // namespace ideam::core