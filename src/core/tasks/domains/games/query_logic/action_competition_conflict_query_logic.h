#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../transform_logic/piece_action_score_refresh_transform_logic.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_ACTION_CONFLICT_QUERY_MODE_DEFINED
#define IDEAM_GAMES_ACTION_CONFLICT_QUERY_MODE_DEFINED

/**
 * Conflict evaluation modes for ActionCompetitionConflictQueryLogic (G14).
 */
enum ActionConflictQueryMode : uint8_t {
    CONFLICT_MODE_SURVIVES_PRIORITY          = 0, // True when action has no competing conflict or has higher/equal priority
    CONFLICT_MODE_HAS_CONFLICT_LOSS          = 1, // True when action conflicts with an active/instigator action & loses priority
    CONFLICT_MODE_COMPETES_WITH_INSTIGATOR   = 2, // True when (competition_mask[i] & instigator_group_mask) != 0
    CONFLICT_MODE_COOPERATES_WITH_INSTIGATOR = 3  // True when (action_group_mask[i] & instigator_group_mask) != 0 and not competing
};

#endif // IDEAM_GAMES_ACTION_CONFLICT_QUERY_MODE_DEFINED

/**
 * ActionCompetitionConflictQueryLogic<T> (G14)
 * Evaluates 64-bit competition_mask bitwise intersections ((mask_a & mask_b) != 0) and priority
 * comparisons across ActionSequenceBuffer columns to cull, wake, or interrupt competing GameAgentActions.
 * Replaces string-based competes_with_groups Array scans in GameAgent::find_best_action() and GameAgentAction.
 */
template <typename T = int64_t>
struct alignas(64) ActionCompetitionConflictQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT64 | DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull          = true;
    static constexpr bool supports_addition      = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Action Competition Conflict";
    static constexpr std::string_view ui_category  = "Actions";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint64_t instigator_group_mask        = 0ULL;                            // 8B: 64-bit group mask of the instigating action
    uint32_t target_buffer_id             = INVALID_ID;                      // 4B: Primary ActionSequenceBuffer selection target
    uint32_t competition_mask_buffer_id   = INVALID_ID;                      // 4B: ActionSequenceBuffer::competition_mask (uint64_t)
    uint32_t action_group_mask_buffer_id  = INVALID_ID;                      // 4B: Optional uint64_t group_mask column for actions
    uint32_t priority_buffer_id           = INVALID_ID;                      // 4B: ActionSequenceBuffer::priority (int32_t)
    uint32_t status_buffer_id             = INVALID_ID;                      // 4B: Optional ActionSequenceBuffer::status (int32_t)
    uint32_t owner_agent_id_buffer_id     = INVALID_ID;                      // 4B: Optional ActionSequenceBuffer::owner_agent_id (uint32_t)
    uint32_t instigator_action_idx        = INVALID_ID;                      // 4B: Optional row index of the instigating action
    int32_t instigator_priority           = 0;                               // 4B: Fallback priority of the instigating action

    uint8_t conflict_mode                 = CONFLICT_MODE_SURVIVES_PRIORITY; // 1B: ActionConflictQueryMode
    bool same_owner_only                  = false;                           // 1B: Only compare against active actions with same owner_agent_id
    bool allow_equal_priority             = true;                            // 1B: Equal priority survives conflict
    bool require_available_or_in_use      = false;                           // 1B: Reject UNAVAILABLE / REFRESHING candidate actions
    uint8_t reserved_padding[20]          = {0};                             // 20B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary cmp_prop;
        cmp_prop["name"] = "competition_mask_buffer_id";
        cmp_prop["type"] = godot::Variant::INT;
        cmp_prop["hint_string"] = "buffer_option";
        props.push_back(cmp_prop);

        godot::Dictionary grp_prop;
        grp_prop["name"] = "action_group_mask_buffer_id";
        grp_prop["type"] = godot::Variant::INT;
        grp_prop["hint_string"] = "buffer_option";
        props.push_back(grp_prop);

        godot::Dictionary pri_prop;
        pri_prop["name"] = "priority_buffer_id";
        pri_prop["type"] = godot::Variant::INT;
        pri_prop["hint_string"] = "buffer_option";
        props.push_back(pri_prop);

        godot::Dictionary st_prop;
        st_prop["name"] = "status_buffer_id";
        st_prop["type"] = godot::Variant::INT;
        st_prop["hint_string"] = "buffer_option";
        props.push_back(st_prop);

        godot::Dictionary own_prop;
        own_prop["name"] = "owner_agent_id_buffer_id";
        own_prop["type"] = godot::Variant::INT;
        own_prop["hint_string"] = "buffer_option";
        props.push_back(own_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "conflict_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Survives Priority:0,Has Conflict Loss (Interrupt):1,Competes With Instigator:2,Cooperates With Instigator:3";
        props.push_back(mode_prop);

        godot::Dictionary igm_prop;
        igm_prop["name"] = "instigator_group_mask";
        igm_prop["type"] = godot::Variant::INT;
        igm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(igm_prop);

        godot::Dictionary iidx_prop;
        iidx_prop["name"] = "instigator_action_idx";
        iidx_prop["type"] = godot::Variant::INT;
        iidx_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(iidx_prop);

        godot::Dictionary ipri_prop;
        ipri_prop["name"] = "instigator_priority";
        ipri_prop["type"] = godot::Variant::INT;
        ipri_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ipri_prop);

        godot::Dictionary own_only_prop;
        own_only_prop["name"] = "same_owner_only";
        own_only_prop["type"] = godot::Variant::BOOL;
        own_only_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(own_only_prop);

        godot::Dictionary eq_prop;
        eq_prop["name"] = "allow_equal_priority";
        eq_prop["type"] = godot::Variant::BOOL;
        eq_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(eq_prop);

        godot::Dictionary req_av_prop;
        req_av_prop["name"] = "require_available_or_in_use";
        req_av_prop["type"] = godot::Variant::BOOL;
        req_av_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(req_av_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)           return target_buffer_id;
        if (competition_mask_buffer_id != INVALID_ID) return competition_mask_buffer_id;
        return priority_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("competition_mask_buffer_id")) {
            competition_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["competition_mask_buffer_id"]));
        }
        if (p_props.has("action_group_mask_buffer_id")) {
            action_group_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["action_group_mask_buffer_id"]));
        }
        if (p_props.has("priority_buffer_id")) {
            priority_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["priority_buffer_id"]));
        }
        if (p_props.has("status_buffer_id")) {
            status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["status_buffer_id"]));
        }
        if (p_props.has("owner_agent_id_buffer_id")) {
            owner_agent_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["owner_agent_id_buffer_id"]));
        }
        if (p_props.has("instigator_group_mask")) {
            instigator_group_mask = static_cast<uint64_t>(static_cast<int64_t>(p_props["instigator_group_mask"]));
        }
        if (p_props.has("instigator_action_idx")) {
            instigator_action_idx = static_cast<uint32_t>(static_cast<int64_t>(p_props["instigator_action_idx"]));
        }
        if (p_props.has("instigator_priority")) {
            instigator_priority = static_cast<int32_t>(static_cast<int64_t>(p_props["instigator_priority"]));
        }
        if (p_props.has("conflict_mode")) {
            conflict_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["conflict_mode"]), 0, 3));
        }
        if (p_props.has("same_owner_only")) {
            same_owner_only = static_cast<bool>(p_props["same_owner_only"]);
        }
        if (p_props.has("allow_equal_priority")) {
            allow_equal_priority = static_cast<bool>(p_props["allow_equal_priority"]);
        }
        if (p_props.has("require_available_or_in_use")) {
            require_available_or_in_use = static_cast<bool>(p_props["require_available_or_in_use"]);
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
        if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, int32_t>) {
            const uint32_t tgt_id = get_target_buffer_id();
            const uint32_t cmp_id = (competition_mask_buffer_id != INVALID_ID) ? competition_mask_buffer_id : tgt_id;

            const GrantPartPOD* cmp_part = (cmp_id != INVALID_ID)                      ? p_context.get_grant_part(cmp_id)                      : nullptr;
            const GrantPartPOD* grp_part = (action_group_mask_buffer_id != INVALID_ID) ? p_context.get_grant_part(action_group_mask_buffer_id) : nullptr;
            const GrantPartPOD* pri_part = (priority_buffer_id != INVALID_ID)          ? p_context.get_grant_part(priority_buffer_id)          : nullptr;
            const GrantPartPOD* st_part  = (status_buffer_id != INVALID_ID)            ? p_context.get_grant_part(status_buffer_id)            : nullptr;
            const GrantPartPOD* own_part = (owner_agent_id_buffer_id != INVALID_ID)    ? p_context.get_grant_part(owner_agent_id_buffer_id)    : nullptr;

            const uint64_t* __restrict comp_masks = (cmp_part && cmp_part->raw_base_ptr && cmp_part->element_stride == sizeof(uint64_t))
                ? reinterpret_cast<const uint64_t*>(cmp_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict comp_masks32 = (cmp_part && cmp_part->raw_base_ptr && cmp_part->element_stride == sizeof(uint32_t))
                ? reinterpret_cast<const uint32_t*>(cmp_part->raw_base_ptr)
                : nullptr;
            const uint64_t* __restrict group_masks64 = (grp_part && grp_part->raw_base_ptr && grp_part->element_stride == sizeof(uint64_t))
                ? reinterpret_cast<const uint64_t*>(grp_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict group_masks32 = (grp_part && grp_part->raw_base_ptr && grp_part->element_stride == sizeof(uint32_t))
                ? reinterpret_cast<const uint32_t*>(grp_part->raw_base_ptr)
                : r_selection.group_masks;
            const int32_t* __restrict priorities   = (pri_part && pri_part->raw_base_ptr)
                ? reinterpret_cast<const int32_t*>(pri_part->raw_base_ptr)
                : nullptr;
            const int32_t* __restrict statuses     = (st_part && st_part->raw_base_ptr)
                ? reinterpret_cast<const int32_t*>(st_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict owners      = (own_part && own_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(own_part->raw_base_ptr)
                : nullptr;

            const int64_t cmp_cap = cmp_part
                ? static_cast<int64_t>(cmp_part->capacity_bytes / (cmp_part->element_stride > 0 ? cmp_part->element_stride : sizeof(uint64_t)))
                : 0;
            const int64_t grp_cap = grp_part
                ? static_cast<int64_t>(grp_part->capacity_bytes / (grp_part->element_stride > 0 ? grp_part->element_stride : sizeof(uint64_t)))
                : r_selection.capacity;
            const int64_t pri_cap = priorities ? static_cast<int64_t>(pri_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t st_cap  = statuses   ? static_cast<int64_t>(st_part->capacity_bytes / sizeof(int32_t))  : 0;
            const int64_t own_cap = owners     ? static_cast<int64_t>(own_part->capacity_bytes / sizeof(uint32_t)): 0;

            auto read_comp_mask = [&](int64_t idx) noexcept -> uint64_t {
                if (idx < 0 || idx >= cmp_cap) return 0ULL;
                if (comp_masks)   return comp_masks[idx];
                if (comp_masks32) return static_cast<uint64_t>(comp_masks32[idx]);
                return 0ULL;
            };

            auto read_group_mask = [&](int64_t idx) noexcept -> uint64_t {
                if (idx < 0 || idx >= grp_cap) return 0ULL;
                if (group_masks64) return group_masks64[idx];
                if (group_masks32) return static_cast<uint64_t>(group_masks32[idx]);
                return 0ULL;
            };

            auto read_priority = [&](int64_t idx) noexcept -> int32_t {
                if (!priorities || idx < 0 || idx >= pri_cap) return 0;
                return priorities[idx];
            };

            // Resolve effective instigator group mask and priority
            uint64_t eff_inst_group = instigator_group_mask;
            int32_t eff_inst_prio   = instigator_priority;
            uint32_t eff_inst_owner = INVALID_ID;
            if (instigator_action_idx != INVALID_ID) {
                const int64_t iidx = static_cast<int64_t>(instigator_action_idx);
                if (eff_inst_group == 0ULL) eff_inst_group = read_group_mask(iidx);
                if (priorities && iidx >= 0 && iidx < pri_cap) eff_inst_prio = priorities[iidx];
                if (owners && iidx >= 0 && iidx < own_cap)     eff_inst_owner = owners[iidx];
            }

            const bool has_group_masks = (group_masks64 != nullptr) || (group_masks32 != nullptr);

            auto evaluate_action = [&](int64_t idx) noexcept -> bool {
                if (require_available_or_in_use && statuses && idx >= 0 && idx < st_cap) {
                    const int32_t st = statuses[idx];
                    if (st != ACTION_STATUS_AVAILABLE && st != ACTION_STATUS_IN_USE) {
                        return false;
                    }
                }

                const uint64_t cand_comp_mask  = read_comp_mask(idx);
                const uint64_t cand_group_mask = read_group_mask(idx);
                const int32_t cand_prio        = read_priority(idx);

                if (conflict_mode == CONFLICT_MODE_COMPETES_WITH_INSTIGATOR) {
                    return (cand_comp_mask & eff_inst_group) != 0ULL;
                }
                if (conflict_mode == CONFLICT_MODE_COOPERATES_WITH_INSTIGATOR) {
                    return ((cand_group_mask & eff_inst_group) != 0ULL) && ((cand_comp_mask & eff_inst_group) == 0ULL);
                }

                // Evaluate priority conflict against instigator and/or other active (IN_USE) actions
                bool loses_priority = false;

                if (eff_inst_group != 0ULL && static_cast<uint32_t>(idx) != instigator_action_idx) {
                    const bool owner_match = !same_owner_only || eff_inst_owner == INVALID_ID ||
                        (owners && idx < own_cap && owners[idx] == eff_inst_owner);
                    if (owner_match && ((cand_comp_mask & eff_inst_group) != 0ULL)) {
                        loses_priority = allow_equal_priority ? (cand_prio < eff_inst_prio) : (cand_prio <= eff_inst_prio);
                    }
                }

                // Pairwise scan against currently IN_USE actions when status_buffer_id & group_masks are bound
                if (!loses_priority && statuses && has_group_masks) {
                    const int64_t scan_cnt = std::min<int64_t>(st_cap, grp_cap);
                    for (int64_t other = 0; other < scan_cnt; ++other) {
                        if (other == idx || statuses[other] != ACTION_STATUS_IN_USE) continue;
                        if (same_owner_only && owners && idx < own_cap && other < own_cap && owners[idx] != owners[other]) {
                            continue;
                        }
                        const uint64_t other_grp  = read_group_mask(other);
                        const uint64_t other_comp = read_comp_mask(other);
                        if (((cand_comp_mask & other_grp) != 0ULL) || ((other_comp & cand_group_mask) != 0ULL)) {
                            const int32_t other_prio = read_priority(other);
                            if (allow_equal_priority ? (cand_prio < other_prio) : (cand_prio <= other_prio)) {
                                loses_priority = true;
                                break;
                            }
                        }
                    }
                }

                return (conflict_mode == CONFLICT_MODE_HAS_CONFLICT_LOSS) ? loses_priority : !loses_priority;
            };

            _apply_query_op<Op>(r_selection, p_context, tgt_id, evaluate_action);
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

static_assert(sizeof(ActionCompetitionConflictQueryLogic<>) == 64, "ActionCompetitionConflictQueryLogic must be exactly 64 bytes!");

} // namespace ideam::core