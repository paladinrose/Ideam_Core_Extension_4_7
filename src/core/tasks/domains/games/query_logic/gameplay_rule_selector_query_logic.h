#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../transform_logic/entity_time_scale_cascade_transform_logic.h"
#include "../transform_logic/gameplay_rule_apply_transform_logic.h"
#include "../transform_logic/piece_action_score_refresh_transform_logic.h"
#include "../transform_logic/property_modifier_aggregate_transform_logic.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_GAMEPLAY_RULE_PSEUDO_STATE_DEFINED
#define IDEAM_GAMES_GAMEPLAY_RULE_PSEUDO_STATE_DEFINED

/**
 * Compiled pseudo-state selectors supported by GameplayRuleSelectorQueryLogic (G16),
 * mirroring GameplayRule::matches_pseudo_state() in src/godot/games/gameplay/gameplay_rule.cpp.
 */
enum GameplayRulePseudoState : uint8_t {
    PSEUDO_STATE_NONE        = 0,  // No pseudo-state constraint
    PSEUDO_STATE_IS_EXHAUSTED= 1,  // :is_exhausted / :exhausted (PROP_FLAG_IS_EXHAUSTED)
    PSEUDO_STATE_LOCKED      = 2,  // :locked (PROP_FLAG_LOCKED)
    PSEUDO_STATE_AT_MAX      = 3,  // :at_max / :full (PROP_FLAG_AT_MAX)
    PSEUDO_STATE_AT_MIN      = 4,  // :at_min / :empty (PROP_FLAG_IS_EXHAUSTED)
    PSEUDO_STATE_IN_PROGRESS = 5,  // :in_progress (ACTION_STATUS_IN_USE)
    PSEUDO_STATE_AVAILABLE   = 6,  // :available (ACTION_STATUS_AVAILABLE)
    PSEUDO_STATE_ENABLED     = 7,  // :enabled (ENTITY_FLAG_ENABLED)
    PSEUDO_STATE_DISABLED    = 8,  // :disabled (!ENTITY_FLAG_ENABLED)
    PSEUDO_STATE_PAUSED      = 9,  // :paused / :is_paused (ENTITY_FLAG_PAUSED)
    PSEUDO_STATE_POSSESSED   = 10, // :possessed / :controlled (controlling_player_id != INVALID_ID)
    PSEUDO_STATE_UNPOSSESSED = 11  // :unpossessed / :ai (controlling_player_id == INVALID_ID)
};

static constexpr uint8_t RULE_SCOPE_ALL_COMBINED = 5u; // Matches both self and descendants (RuleScope::ALL)

#endif // IDEAM_GAMES_GAMEPLAY_RULE_PSEUDO_STATE_DEFINED

/**
 * GameplayRuleSelectorQueryLogic<T> (G16)
 * Evaluates compiled GameplayRule selectors (RuleScope hierarchy relationship, target_class archetype_mask,
 * target_title_or_group 64-bit group_mask, interned target_property token ID, and runtime :pseudo_state)
 * to build or cull the exact MemoryBufferSelectionPOD for GameplayRuleApplyTransformLogic (G5).
 * Replaces GameplayRule::matches_object(), matches_entity(), and matches_pseudo_state().
 */
template <typename T = int32_t>
struct alignas(64) GameplayRuleSelectorQueryLogic {
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

    static constexpr std::string_view display_name = "Gameplay Rule Selector";
    static constexpr std::string_view ui_category  = "Rules";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint64_t required_group_mask          = 0ULL;                    // 8B: 64-bit group/title bitmask (0 = match any)
    uint32_t target_buffer_id             = INVALID_ID;              // 4B: Primary selection buffer to filter/wake
    uint32_t archetype_mask_buffer_id     = INVALID_ID;              // 4B: EntityHierarchyBuffer::archetype_mask (uint32_t)
    uint32_t group_mask_buffer_id         = INVALID_ID;              // 4B: Optional uint64_t group_mask column
    uint32_t parent_entity_id_buffer_id   = INVALID_ID;              // 4B: Optional EntityHierarchyBuffer::parent_entity_id (uint32_t)
    uint32_t entity_active_flags_buffer_id= INVALID_ID;              // 4B: Optional EntityHierarchyBuffer::active_flags (uint32_t)
    uint32_t property_flags_buffer_id     = INVALID_ID;              // 4B: Optional GamePropertyBuffer::property_flags (uint16_t/uint32_t)
    uint32_t property_name_id_buffer_id   = INVALID_ID;              // 4B: Optional GamePropertyBuffer::property_name_id (uint32_t)
    uint32_t status_or_player_buffer_id   = INVALID_ID;              // 4B: Optional action status or controlling_player_id (int32_t/uint32_t)
    uint32_t scope_root_entity_id         = INVALID_ID;              // 4B: Root entity ID for SELF / CHILDREN / DESCENDANTS / ALL scopes
    uint32_t required_archetype_mask      = ENTITY_ARCHETYPE_ALL;    // 4B: EntityArchetypeMask bitmask filter
    uint32_t required_property_name_id    = 0u;                      // 4B: Interned StringName token ID for target_property (0 = any)

    uint8_t rule_scope                    = RULE_SCOPE_GLOBAL;       // 1B: GameplayRuleScope (0=Self,1=Children,2=Descendants,3=Group,4=Global,5=All)
    uint8_t pseudo_state                  = PSEUDO_STATE_NONE;       // 1B: GameplayRulePseudoState
    bool negate_pseudo_state              = false;                   // 1B: Negates pseudo_state evaluation (!state / :not_state)
    uint8_t max_ancestor_depth            = 8u;                      // 1B: Maximum parent hops for DESCENDANTS / ALL scope checks
    uint8_t reserved_padding[8]           = {0};                     // 8B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary arch_prop;
        arch_prop["name"] = "archetype_mask_buffer_id";
        arch_prop["type"] = godot::Variant::INT;
        arch_prop["hint_string"] = "buffer_option";
        props.push_back(arch_prop);

        godot::Dictionary grp_prop;
        grp_prop["name"] = "group_mask_buffer_id";
        grp_prop["type"] = godot::Variant::INT;
        grp_prop["hint_string"] = "buffer_option";
        props.push_back(grp_prop);

        godot::Dictionary par_prop;
        par_prop["name"] = "parent_entity_id_buffer_id";
        par_prop["type"] = godot::Variant::INT;
        par_prop["hint_string"] = "buffer_option";
        props.push_back(par_prop);

        godot::Dictionary eflg_prop;
        eflg_prop["name"] = "entity_active_flags_buffer_id";
        eflg_prop["type"] = godot::Variant::INT;
        eflg_prop["hint_string"] = "buffer_option";
        props.push_back(eflg_prop);

        godot::Dictionary pflg_prop;
        pflg_prop["name"] = "property_flags_buffer_id";
        pflg_prop["type"] = godot::Variant::INT;
        pflg_prop["hint_string"] = "buffer_option";
        props.push_back(pflg_prop);

        godot::Dictionary pnm_prop;
        pnm_prop["name"] = "property_name_id_buffer_id";
        pnm_prop["type"] = godot::Variant::INT;
        pnm_prop["hint_string"] = "buffer_option";
        props.push_back(pnm_prop);

        godot::Dictionary st_prop;
        st_prop["name"] = "status_or_player_buffer_id";
        st_prop["type"] = godot::Variant::INT;
        st_prop["hint_string"] = "buffer_option";
        props.push_back(st_prop);

        godot::Dictionary scp_prop;
        scp_prop["name"] = "rule_scope";
        scp_prop["type"] = godot::Variant::INT;
        scp_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        scp_prop["hint_string"] = "Self:0,Children:1,Descendants:2,Group:3,Global:4,All (Self + Descendants):5";
        props.push_back(scp_prop);

        godot::Dictionary root_prop;
        root_prop["name"] = "scope_root_entity_id";
        root_prop["type"] = godot::Variant::INT;
        root_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(root_prop);

        godot::Dictionary rarch_prop;
        rarch_prop["name"] = "required_archetype_mask";
        rarch_prop["type"] = godot::Variant::INT;
        rarch_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        rarch_prop["hint_string"] = "Game,Board,Agent,Piece,AgentAction,PieceAction,Interaction";
        props.push_back(rarch_prop);

        godot::Dictionary rgrp_prop;
        rgrp_prop["name"] = "required_group_mask";
        rgrp_prop["type"] = godot::Variant::INT;
        rgrp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rgrp_prop);

        godot::Dictionary rpnm_prop;
        rpnm_prop["name"] = "required_property_name_id";
        rpnm_prop["type"] = godot::Variant::INT;
        rpnm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rpnm_prop);

        godot::Dictionary pst_prop;
        pst_prop["name"] = "pseudo_state";
        pst_prop["type"] = godot::Variant::INT;
        pst_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        pst_prop["hint_string"] = "None:0,Is Exhausted:1,Locked:2,At Max:3,At Min:4,In Progress:5,Available:6,Enabled:7,Disabled:8,Paused:9,Possessed:10,Unpossessed:11";
        props.push_back(pst_prop);

        godot::Dictionary neg_prop;
        neg_prop["name"] = "negate_pseudo_state";
        neg_prop["type"] = godot::Variant::BOOL;
        neg_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(neg_prop);

        godot::Dictionary dep_prop;
        dep_prop["name"] = "max_ancestor_depth";
        dep_prop["type"] = godot::Variant::INT;
        dep_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        dep_prop["hint_string"] = "1,16,1";
        props.push_back(dep_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)         return target_buffer_id;
        if (archetype_mask_buffer_id != INVALID_ID) return archetype_mask_buffer_id;
        if (property_flags_buffer_id != INVALID_ID) return property_flags_buffer_id;
        return entity_active_flags_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("archetype_mask_buffer_id")) {
            archetype_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["archetype_mask_buffer_id"]));
        }
        if (p_props.has("group_mask_buffer_id")) {
            group_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["group_mask_buffer_id"]));
        }
        if (p_props.has("parent_entity_id_buffer_id")) {
            parent_entity_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_entity_id_buffer_id"]));
        }
        if (p_props.has("entity_active_flags_buffer_id")) {
            entity_active_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["entity_active_flags_buffer_id"]));
        }
        if (p_props.has("property_flags_buffer_id")) {
            property_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_flags_buffer_id"]));
        }
        if (p_props.has("property_name_id_buffer_id")) {
            property_name_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_name_id_buffer_id"]));
        }
        if (p_props.has("status_or_player_buffer_id")) {
            status_or_player_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["status_or_player_buffer_id"]));
        }
        if (p_props.has("scope_root_entity_id")) {
            scope_root_entity_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["scope_root_entity_id"]));
        }
        if (p_props.has("required_archetype_mask")) {
            required_archetype_mask = static_cast<uint32_t>(static_cast<int64_t>(p_props["required_archetype_mask"]));
        }
        if (p_props.has("required_group_mask")) {
            required_group_mask = static_cast<uint64_t>(static_cast<int64_t>(p_props["required_group_mask"]));
        }
        if (p_props.has("required_property_name_id")) {
            required_property_name_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["required_property_name_id"]));
        }
        if (p_props.has("rule_scope")) {
            rule_scope = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["rule_scope"]), 0, 5));
        }
        if (p_props.has("pseudo_state")) {
            pseudo_state = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["pseudo_state"]), 0, 11));
        }
        if (p_props.has("negate_pseudo_state")) {
            negate_pseudo_state = static_cast<bool>(p_props["negate_pseudo_state"]);
        }
        if (p_props.has("max_ancestor_depth")) {
            max_ancestor_depth = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_ancestor_depth"]), 1, 32));
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

            const GrantPartPOD* arch_part = (archetype_mask_buffer_id != INVALID_ID)      ? p_context.get_grant_part(archetype_mask_buffer_id)      : nullptr;
            const GrantPartPOD* grp_part  = (group_mask_buffer_id != INVALID_ID)          ? p_context.get_grant_part(group_mask_buffer_id)          : nullptr;
            const GrantPartPOD* par_part  = (parent_entity_id_buffer_id != INVALID_ID)    ? p_context.get_grant_part(parent_entity_id_buffer_id)    : nullptr;
            const GrantPartPOD* eflg_part = (entity_active_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(entity_active_flags_buffer_id) : nullptr;
            const GrantPartPOD* pflg_part = (property_flags_buffer_id != INVALID_ID)      ? p_context.get_grant_part(property_flags_buffer_id)      : nullptr;
            const GrantPartPOD* pnm_part  = (property_name_id_buffer_id != INVALID_ID)    ? p_context.get_grant_part(property_name_id_buffer_id)    : nullptr;
            const GrantPartPOD* st_part   = (status_or_player_buffer_id != INVALID_ID)    ? p_context.get_grant_part(status_or_player_buffer_id)    : nullptr;

            const uint32_t* __restrict archetypes    = (arch_part && arch_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(arch_part->raw_base_ptr) : nullptr;
            const uint64_t* __restrict group_masks64 = (grp_part && grp_part->raw_base_ptr && grp_part->element_stride == sizeof(uint64_t))
                ? reinterpret_cast<const uint64_t*>(grp_part->raw_base_ptr)
                : nullptr;
            const uint32_t* __restrict group_masks32 = (grp_part && grp_part->raw_base_ptr && grp_part->element_stride == sizeof(uint32_t))
                ? reinterpret_cast<const uint32_t*>(grp_part->raw_base_ptr)
                : r_selection.group_masks;
            const uint32_t* __restrict parents       = (par_part && par_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(par_part->raw_base_ptr)  : nullptr;
            const uint32_t* __restrict ent_flags     = (eflg_part && eflg_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(eflg_part->raw_base_ptr) : nullptr;
            const uint8_t* __restrict pflg_raw       = (pflg_part && pflg_part->raw_base_ptr) ? pflg_part->raw_base_ptr                                    : nullptr;
            const uint32_t pflg_stride               = pflg_part ? pflg_part->element_stride : 0;
            const uint32_t* __restrict prop_names    = (pnm_part && pnm_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(pnm_part->raw_base_ptr)  : nullptr;
            const int32_t* __restrict st_or_player   = (st_part && st_part->raw_base_ptr)     ? reinterpret_cast<const int32_t*>(st_part->raw_base_ptr)    : nullptr;

            const int64_t arch_cap = archetypes   ? static_cast<int64_t>(arch_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t grp_cap  = grp_part
                ? static_cast<int64_t>(grp_part->capacity_bytes / (grp_part->element_stride > 0 ? grp_part->element_stride : sizeof(uint64_t)))
                : r_selection.capacity;
            const int64_t par_cap  = parents      ? static_cast<int64_t>(par_part->capacity_bytes / sizeof(uint32_t))  : 0;
            const int64_t eflg_cap = ent_flags    ? static_cast<int64_t>(eflg_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t pflg_cap = (pflg_part && pflg_stride > 0) ? static_cast<int64_t>(pflg_part->capacity_bytes / pflg_stride) : 0;
            const int64_t pnm_cap  = prop_names   ? static_cast<int64_t>(pnm_part->capacity_bytes / sizeof(uint32_t))  : 0;
            const int64_t st_cap   = st_or_player ? static_cast<int64_t>(st_part->capacity_bytes / sizeof(int32_t))    : 0;

            auto read_group_mask = [&](int64_t idx) noexcept -> uint64_t {
                if (idx < 0 || idx >= grp_cap) return 0ULL;
                if (group_masks64) return group_masks64[idx];
                if (group_masks32) return static_cast<uint64_t>(group_masks32[idx]);
                return 0ULL;
            };

            const bool has_group_masks = (group_masks64 != nullptr) || (group_masks32 != nullptr);

            auto read_prop_flags = [&](int64_t idx) noexcept -> uint16_t {
                if (!pflg_raw || idx < 0 || idx >= pflg_cap) return PROP_FLAG_DEFAULT_MASK;
                if (pflg_stride == sizeof(uint32_t)) {
                    return static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pflg_raw)[idx]);
                }
                return reinterpret_cast<const uint16_t*>(pflg_raw)[idx];
            };

            auto matches_scope = [&](int64_t idx) noexcept -> bool {
                if (scope_root_entity_id == INVALID_ID || rule_scope == RULE_SCOPE_GLOBAL || rule_scope == RULE_SCOPE_GROUP) {
                    return true;
                }
                const uint32_t u_idx = static_cast<uint32_t>(idx);
                if (rule_scope == RULE_SCOPE_SELF) {
                    return u_idx == scope_root_entity_id;
                }
                if (!parents || idx < 0 || idx >= par_cap) {
                    return (rule_scope == RULE_SCOPE_ALL_COMBINED) && (u_idx == scope_root_entity_id);
                }
                if (rule_scope == RULE_SCOPE_CHILDREN) {
                    return parents[idx] == scope_root_entity_id;
                }
                // DESCENDANTS or ALL
                if (rule_scope == RULE_SCOPE_ALL_COMBINED && u_idx == scope_root_entity_id) {
                    return true;
                }
                uint32_t cur_parent = parents[idx];
                for (uint8_t d = 0u; d < max_ancestor_depth && cur_parent != INVALID_ID; ++d) {
                    if (cur_parent == scope_root_entity_id) return true;
                    const int64_t pidx = static_cast<int64_t>(cur_parent);
                    if (pidx < 0 || pidx >= par_cap || pidx == static_cast<int64_t>(cur_parent == parents[pidx] ? pidx : -1)) break;
                    cur_parent = parents[pidx];
                }
                return false;
            };

            auto matches_pseudo = [&](int64_t idx) noexcept -> bool {
                if (pseudo_state == PSEUDO_STATE_NONE) return true;
                bool state_val = false;
                switch (pseudo_state) {
                    case PSEUDO_STATE_IS_EXHAUSTED:
                    case PSEUDO_STATE_AT_MIN:
                        state_val = (read_prop_flags(idx) & PROP_FLAG_IS_EXHAUSTED) != 0u;
                        break;
                    case PSEUDO_STATE_LOCKED:
                        state_val = (read_prop_flags(idx) & PROP_FLAG_LOCKED) != 0u;
                        break;
                    case PSEUDO_STATE_AT_MAX:
                        state_val = (read_prop_flags(idx) & PROP_FLAG_AT_MAX) != 0u;
                        break;
                    case PSEUDO_STATE_IN_PROGRESS:
                        state_val = (st_or_player && idx >= 0 && idx < st_cap) && (st_or_player[idx] == ACTION_STATUS_IN_USE);
                        break;
                    case PSEUDO_STATE_AVAILABLE:
                        state_val = (st_or_player && idx >= 0 && idx < st_cap) && (st_or_player[idx] == ACTION_STATUS_AVAILABLE);
                        break;
                    case PSEUDO_STATE_ENABLED:
                        state_val = (!ent_flags || idx < 0 || idx >= eflg_cap) || ((ent_flags[idx] & ENTITY_FLAG_ENABLED) != 0u);
                        break;
                    case PSEUDO_STATE_DISABLED:
                        state_val = (ent_flags && idx >= 0 && idx < eflg_cap) && ((ent_flags[idx] & ENTITY_FLAG_ENABLED) == 0u);
                        break;
                    case PSEUDO_STATE_PAUSED:
                        state_val = (ent_flags && idx >= 0 && idx < eflg_cap) && ((ent_flags[idx] & ENTITY_FLAG_PAUSED) != 0u);
                        break;
                    case PSEUDO_STATE_POSSESSED:
                        state_val = (st_or_player && idx >= 0 && idx < st_cap) && (static_cast<uint32_t>(st_or_player[idx]) != INVALID_ID);
                        break;
                    case PSEUDO_STATE_UNPOSSESSED:
                        state_val = (!st_or_player || idx < 0 || idx >= st_cap) || (static_cast<uint32_t>(st_or_player[idx]) == INVALID_ID);
                        break;
                    default:
                        state_val = true;
                        break;
                }
                return negate_pseudo_state ? !state_val : state_val;
            };

            auto evaluate_selector = [&](int64_t idx) noexcept -> bool {
                if (!matches_scope(idx)) return false;

                if (archetypes && idx >= 0 && idx < arch_cap) {
                    if ((archetypes[idx] & required_archetype_mask) == 0u) return false;
                }

                if (required_group_mask != 0ULL) {
                    if (!has_group_masks || idx < 0 || idx >= grp_cap) return false;
                    if ((read_group_mask(idx) & required_group_mask) != required_group_mask) return false;
                }

                if (required_property_name_id != 0u && prop_names && idx >= 0 && idx < pnm_cap) {
                    if (prop_names[idx] != required_property_name_id) return false;
                }

                return matches_pseudo(idx);
            };

            _apply_query_op<Op>(r_selection, p_context, tgt_id, evaluate_selector);
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

static_assert(sizeof(GameplayRuleSelectorQueryLogic<>) == 64, "GameplayRuleSelectorQueryLogic must be exactly 64 bytes!");

} // namespace ideam::core