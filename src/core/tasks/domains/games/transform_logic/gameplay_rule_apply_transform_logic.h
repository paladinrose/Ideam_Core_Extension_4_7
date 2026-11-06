#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/aosoa_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "property_modifier_aggregate_transform_logic.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_GAMEPLAY_RULE_POD_DEFINED
#define IDEAM_GAMES_GAMEPLAY_RULE_POD_DEFINED

/**
 * DOD RuleScope bit/enum values (mirrors ideam::godot_ext::RuleScope).
 */
enum GameplayRuleScope : uint8_t {
    RULE_SCOPE_SELF        = 0,
    RULE_SCOPE_CHILDREN    = 1,
    RULE_SCOPE_DESCENDANTS = 2,
    RULE_SCOPE_GROUP       = 3,
    RULE_SCOPE_GLOBAL      = 4
};

/**
 * DOD RuleCascadeLayer ordering (mirrors ideam::godot_ext::RuleCascadeLayer).
 * Rules execute in ascending (cascade_layer, priority) order.
 */
enum GameplayRuleCascadeLayer : uint8_t {
    RULE_LAYER_STYLE                = 0,
    RULE_LAYER_DIFFICULTY           = 1,
    RULE_LAYER_PERSPECTIVE          = 2,
    RULE_LAYER_MODE                 = 3,
    RULE_LAYER_CONTROL_MAP          = 4,
    RULE_LAYER_ACCESSIBILITY        = 5,
    RULE_LAYER_PLAYER_ACCESSIBILITY = 6
};

/**
 * DOD RuleOp operation codes (mirrors ideam::godot_ext::RuleOp).
 */
enum GameplayRuleOp : uint8_t {
    RULE_OP_SET                = 0,
    RULE_OP_ADD                = 1,
    RULE_OP_MULTIPLY           = 2,
    RULE_OP_CLAMP              = 3,
    RULE_OP_INTERACTION_WEIGHT = 4
};

/**
 * DOD RulePropertyTarget channels (mirrors ideam::godot_ext::RulePropertyTarget).
 */
enum GameplayRuleTargetChannel : uint8_t {
    RULE_TARGET_VALUE     = 0,
    RULE_TARGET_MAX_VALUE = 1,
    RULE_TARGET_MIN_VALUE = 2
};

/**
 * CompiledGameplayRulePOD
 * Cache-aligned 32-byte POD representation of a compiled GameplayRule declaration
 * pre-sorted by (cascade_layer, priority) for zero-allocation DOD execution.
 */
struct alignas(16) CompiledGameplayRulePOD {
    uint32_t target_entity_id        = INVALID_ID; // 4B: Specific entity/property index (INVALID_ID = match all in selection)
    uint32_t required_archetype_mask = 0xFFFFFFFFu;// 4B: EntityArchetypeMask filter
    uint64_t required_group_mask     = 0ULL;       // 8B: 64-bit group bitmask filter (0 = no group filter)
    float value                      = 0.0f;       // 4B: Operand for SET, ADD, MULTIPLY, INTERACTION_WEIGHT
    float clamp_min                  = 0.0f;       // 4B: Lower bound for CLAMP
    float clamp_max                  = 100.0f;     // 4B: Upper bound for CLAMP
    uint8_t operation                = RULE_OP_ADD;// 1B: GameplayRuleOp
    uint8_t cascade_layer            = RULE_LAYER_STYLE; // 1B: GameplayRuleCascadeLayer
    uint8_t target_channel           = RULE_TARGET_VALUE;// 1B: GameplayRuleTargetChannel
    uint8_t enabled                  = 1u;         // 1B: Non-zero when rule is active
};

static_assert(sizeof(CompiledGameplayRulePOD) == 32, "CompiledGameplayRulePOD must be 32 bytes!");

#endif // IDEAM_GAMES_GAMEPLAY_RULE_POD_DEFINED

/**
 * GameplayRuleApplyTransformLogic<T> (G5)
 * Executes compiled GameplayRule declarations (SET, ADD, MULTIPLY, CLAMP, INTERACTION_WEIGHT)
 * over selected GamePropertyBuffer (int32_t) or EntityHierarchyBuffer / InteractionArenaBuffer (float) columns.
 * Supports both SIMD lane execution (AOSOAView) for single-rule passes and multi-rule cascade buffer evaluation.
 * Replaces GameplayRule::apply_to_entity(), apply_to_game_property(), and evaluate_numeric_value().
 */
template <typename T = int32_t>
struct alignas(64) GameplayRuleApplyTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = AOSOAView<T, 8, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Gameplay Rule Apply";
    static constexpr std::string_view ui_category  = "Rules";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id          = INVALID_ID;            // 4B: Target property/entity column (int32_t or float)
    uint32_t source_buffer_id          = INVALID_ID;            // 4B: Optional source/base column (if INVALID_ID, transforms in-place)
    uint32_t property_flags_buffer_id  = INVALID_ID;            // 4B: Optional GamePropertyBuffer::property_flags (uint16_t/uint32_t)
    uint32_t archetype_mask_buffer_id  = INVALID_ID;            // 4B: Optional EntityHierarchyBuffer::archetype_mask (uint32_t)
    uint32_t group_mask_buffer_id      = INVALID_ID;            // 4B: Optional 64-bit group_mask column (uint64_t)
    uint32_t compiled_rule_buffer_id   = INVALID_ID;            // 4B: Optional CompiledGameplayRulePOD array buffer
    uint32_t required_archetype_mask   = 0xFFFFFFFFu;           // 4B: Archetype bitmask filter for inline rule execution
    uint32_t required_group_mask_lo    = 0u;                    // 4B: Low 32-bit group mask filter (0 = match all)

    float rule_value                   = 0.0f;                  // 4B: Inline rule operand (SET, ADD, MULTIPLY, INTERACTION_WEIGHT)
    float clamp_min                    = 0.0f;                  // 4B: Inline lower bound for CLAMP
    float clamp_max                    = 100.0f;                // 4B: Inline upper bound for CLAMP
    int32_t rule_priority              = 0;                     // 4B: Inline rule priority metadata

    uint8_t operation                  = RULE_OP_ADD;           // 1B: GameplayRuleOp
    uint8_t cascade_layer              = RULE_LAYER_STYLE;      // 1B: GameplayRuleCascadeLayer
    uint8_t target_channel             = RULE_TARGET_VALUE;     // 1B: GameplayRuleTargetChannel (0=Value, 1=Max, 2=Min)
    bool respect_allow_flags           = true;                  // 1B: Respect allow_*_set / allow_*_modifiers in property_flags
    bool skip_locked_properties        = false;                 // 1B: Skip elements with PROP_FLAG_LOCKED set
    uint8_t reserved_padding[11]       = {0};                   // 11B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary src_prop;
        src_prop["name"] = "source_buffer_id";
        src_prop["type"] = godot::Variant::INT;
        src_prop["hint_string"] = "buffer_option";
        props.push_back(src_prop);

        godot::Dictionary pflg_prop;
        pflg_prop["name"] = "property_flags_buffer_id";
        pflg_prop["type"] = godot::Variant::INT;
        pflg_prop["hint_string"] = "buffer_option";
        props.push_back(pflg_prop);

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

        godot::Dictionary crul_prop;
        crul_prop["name"] = "compiled_rule_buffer_id";
        crul_prop["type"] = godot::Variant::INT;
        crul_prop["hint_string"] = "buffer_option";
        props.push_back(crul_prop);

        godot::Dictionary op_prop;
        op_prop["name"] = "operation";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Set:0,Add:1,Multiply:2,Clamp:3,Interaction Weight:4";
        props.push_back(op_prop);

        godot::Dictionary lay_prop;
        lay_prop["name"] = "cascade_layer";
        lay_prop["type"] = godot::Variant::INT;
        lay_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        lay_prop["hint_string"] = "Style:0,Difficulty:1,Perspective:2,Mode:3,ControlMap:4,Accessibility:5,PlayerAccessibility:6";
        props.push_back(lay_prop);

        godot::Dictionary tch_prop;
        tch_prop["name"] = "target_channel";
        tch_prop["type"] = godot::Variant::INT;
        tch_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        tch_prop["hint_string"] = "Value:0,Max Value:1,Min Value:2";
        props.push_back(tch_prop);

        godot::Dictionary val_prop;
        val_prop["name"] = "rule_value";
        val_prop["type"] = godot::Variant::FLOAT;
        val_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(val_prop);

        godot::Dictionary cmin_prop;
        cmin_prop["name"] = "clamp_min";
        cmin_prop["type"] = godot::Variant::FLOAT;
        cmin_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cmin_prop);

        godot::Dictionary cmax_prop;
        cmax_prop["name"] = "clamp_max";
        cmax_prop["type"] = godot::Variant::FLOAT;
        cmax_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cmax_prop);

        godot::Dictionary prio_prop;
        prio_prop["name"] = "rule_priority";
        prio_prop["type"] = godot::Variant::INT;
        prio_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(prio_prop);

        godot::Dictionary req_arch_prop;
        req_arch_prop["name"] = "required_archetype_mask";
        req_arch_prop["type"] = godot::Variant::INT;
        req_arch_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        req_arch_prop["hint_string"] = "Game,Board,Agent,Piece,AgentAction,PieceAction,Interaction";
        props.push_back(req_arch_prop);

        godot::Dictionary req_grp_prop;
        req_grp_prop["name"] = "required_group_mask_lo";
        req_grp_prop["type"] = godot::Variant::INT;
        req_grp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(req_grp_prop);

        godot::Dictionary alw_prop;
        alw_prop["name"] = "respect_allow_flags";
        alw_prop["type"] = godot::Variant::BOOL;
        alw_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(alw_prop);

        godot::Dictionary lck_prop;
        lck_prop["name"] = "skip_locked_properties";
        lck_prop["type"] = godot::Variant::BOOL;
        lck_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(lck_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && target_buffer_id == INVALID_ID) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        }
        if (p_props.has("property_flags_buffer_id")) {
            property_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_flags_buffer_id"]));
        }
        if (p_props.has("archetype_mask_buffer_id")) {
            archetype_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["archetype_mask_buffer_id"]));
        }
        if (p_props.has("group_mask_buffer_id")) {
            group_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["group_mask_buffer_id"]));
        }
        if (p_props.has("compiled_rule_buffer_id")) {
            compiled_rule_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["compiled_rule_buffer_id"]));
        }
        if (p_props.has("required_archetype_mask")) {
            required_archetype_mask = static_cast<uint32_t>(static_cast<int64_t>(p_props["required_archetype_mask"]));
        }
        if (p_props.has("required_group_mask_lo")) {
            required_group_mask_lo = static_cast<uint32_t>(static_cast<int64_t>(p_props["required_group_mask_lo"]));
        }
        if (p_props.has("rule_value")) {
            rule_value = static_cast<float>(p_props["rule_value"]);
        }
        if (p_props.has("clamp_min")) {
            clamp_min = static_cast<float>(p_props["clamp_min"]);
        }
        if (p_props.has("clamp_max")) {
            clamp_max = static_cast<float>(p_props["clamp_max"]);
        }
        if (p_props.has("rule_priority")) {
            rule_priority = static_cast<int32_t>(static_cast<int64_t>(p_props["rule_priority"]));
        }
        if (p_props.has("operation")) {
            operation = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["operation"]), 0, 4));
        }
        if (p_props.has("cascade_layer")) {
            cascade_layer = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["cascade_layer"]), 0, 6));
        }
        if (p_props.has("target_channel")) {
            target_channel = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["target_channel"]), 0, 2));
        }
        if (p_props.has("respect_allow_flags")) {
            respect_allow_flags = static_cast<bool>(p_props["respect_allow_flags"]);
        }
        if (p_props.has("skip_locked_properties")) {
            skip_locked_properties = static_cast<bool>(p_props["skip_locked_properties"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, float>) {
            const GrantPartPOD* dst_part = (target_buffer_id != INVALID_ID) ? p_context.get_grant_part(target_buffer_id) : nullptr;
            if (!dst_part || !dst_part->raw_base_ptr) return;

            const GrantPartPOD* src_part  = (source_buffer_id != INVALID_ID && source_buffer_id != target_buffer_id)
                ? p_context.get_grant_part(source_buffer_id)
                : nullptr;
            const GrantPartPOD* pflg_part = (property_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(property_flags_buffer_id) : nullptr;
            const GrantPartPOD* arch_part = (archetype_mask_buffer_id != INVALID_ID) ? p_context.get_grant_part(archetype_mask_buffer_id) : nullptr;
            const GrantPartPOD* grp_part  = (group_mask_buffer_id != INVALID_ID)     ? p_context.get_grant_part(group_mask_buffer_id)     : nullptr;
            const GrantPartPOD* crul_part = (compiled_rule_buffer_id != INVALID_ID)  ? p_context.get_grant_part(compiled_rule_buffer_id)  : nullptr;

            T* __restrict dst_vals       = reinterpret_cast<T*>(dst_part->raw_base_ptr);
            const T* __restrict src_vals = (src_part && src_part->raw_base_ptr)
                ? reinterpret_cast<const T*>(src_part->raw_base_ptr)
                : nullptr;

            const uint8_t* __restrict pflg_raw    = (pflg_part && pflg_part->raw_base_ptr) ? pflg_part->raw_base_ptr : nullptr;
            const uint32_t pflg_stride            = pflg_part ? pflg_part->element_stride : 0;
            const int64_t pflg_cap                = (pflg_part && pflg_stride > 0)
                ? static_cast<int64_t>(pflg_part->capacity_bytes / pflg_stride)
                : 0;
            const uint32_t* __restrict archetypes = (arch_part && arch_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(arch_part->raw_base_ptr)
                : nullptr;
            const int64_t arch_cap                = arch_part ? static_cast<int64_t>(arch_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const uint64_t* __restrict groups     = (grp_part && grp_part->raw_base_ptr)
                ? reinterpret_cast<const uint64_t*>(grp_part->raw_base_ptr)
                : nullptr;
            const int64_t grp_cap                 = grp_part ? static_cast<int64_t>(grp_part->capacity_bytes / sizeof(uint64_t)) : 0;

            int64_t elem_count = static_cast<int64_t>(dst_part->capacity_bytes / sizeof(T));
            if (src_part) elem_count = std::min<int64_t>(elem_count, static_cast<int64_t>(src_part->capacity_bytes / sizeof(T)));
            if (elem_count <= 0) return;

            auto read_prop_flags = [&](int64_t idx) noexcept -> uint16_t {
                if (!pflg_raw || idx < 0 || idx >= pflg_cap) return PROP_FLAG_DEFAULT_MASK;
                if (pflg_stride == sizeof(uint32_t)) {
                    return static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pflg_raw)[idx]);
                }
                return reinterpret_cast<const uint16_t*>(pflg_raw)[idx];
            };

            auto is_operation_allowed = [&](int64_t idx, uint8_t op_code, uint8_t channel) noexcept -> bool {
                if (!pflg_raw || (!respect_allow_flags && !skip_locked_properties)) return true;
                const uint16_t flags = read_prop_flags(idx);
                if (skip_locked_properties && (flags & PROP_FLAG_LOCKED) != 0u) return false;
                if (!respect_allow_flags) return true;

                const bool is_set_or_clamp = (op_code == RULE_OP_SET || op_code == RULE_OP_CLAMP);
                if (channel == RULE_TARGET_VALUE) {
                    return is_set_or_clamp
                        ? ((flags & PROP_FLAG_ALLOW_VALUE_SET) != 0u)
                        : ((flags & PROP_FLAG_ALLOW_VALUE_MODS) != 0u);
                } else if (channel == RULE_TARGET_MAX_VALUE) {
                    return is_set_or_clamp
                        ? ((flags & PROP_FLAG_ALLOW_MAX_SET) != 0u)
                        : ((flags & PROP_FLAG_ALLOW_MAX_MODS) != 0u);
                } else {
                    return is_set_or_clamp
                        ? ((flags & PROP_FLAG_ALLOW_MIN_SET) != 0u)
                        : ((flags & PROP_FLAG_ALLOW_MIN_MODS) != 0u);
                }
            };

            const MemoryBufferSelectionPOD& sel = dst_part->selection;

            // --- Path A: Compiled GameplayRule POD Buffer Cascade ---
            if (crul_part && crul_part->raw_base_ptr) {
                const CompiledGameplayRulePOD* __restrict rules =
                    reinterpret_cast<const CompiledGameplayRulePOD*>(crul_part->raw_base_ptr);
                const int64_t rule_count = static_cast<int64_t>(crul_part->capacity_bytes / sizeof(CompiledGameplayRulePOD));
                if (rule_count <= 0) return;

                // Copy base/source values first when source_buffer_id is bound
                if (src_vals) {
                    auto copy_src = [&](int64_t idx) noexcept {
                        dst_vals[idx] = src_vals[idx];
                    };
                    _iterate_selection(sel, elem_count, copy_src);
                }

                for (int64_t r = 0; r < rule_count; ++r) {
                    const CompiledGameplayRulePOD& rule = rules[r];
                    if (rule.enabled == 0u || rule.target_channel != target_channel) continue;

                    auto apply_rule_at = [&](int64_t idx) noexcept {
                        if (rule.target_entity_id != INVALID_ID && static_cast<int64_t>(rule.target_entity_id) != idx) {
                            return;
                        }
                        if (archetypes && idx < arch_cap && ((archetypes[idx] & rule.required_archetype_mask) == 0u)) {
                            return;
                        }
                        if (groups && idx < grp_cap && rule.required_group_mask != 0ULL &&
                            ((groups[idx] & rule.required_group_mask) != rule.required_group_mask)) {
                            return;
                        }
                        if (!is_operation_allowed(idx, rule.operation, rule.target_channel)) {
                            return;
                        }
                        dst_vals[idx] = _eval_rule_math(dst_vals[idx], rule.operation, rule.value, rule.clamp_min, rule.clamp_max);
                    };
                    _iterate_selection(sel, elem_count, apply_rule_at);
                }
                return;
            }

            // --- Path B: Fast-path SIMD Lane Execution for Unfiltered Inline Rules on AOSOAView ---
            const bool has_filters = (pflg_raw && (respect_allow_flags || skip_locked_properties)) ||
                                     (archetypes != nullptr) ||
                                     (groups != nullptr && required_group_mask_lo != 0u);

            if constexpr (requires { p_view.lane_count(); p_view.get_lane(0); T_View::lane_width; }) {
                if (!has_filters && !src_vals && sel.mode == SelectionMode::RANGE && sel.start_index == 0) {
                    constexpr size_t L_WIDTH = T_View::lane_width;
                    const size_t lanes       = p_view.lane_count();
                    const int64_t active_cnt = (sel.element_count > 0) ? std::min<int64_t>(sel.element_count, elem_count) : elem_count;
                    const size_t full_lanes  = std::min<size_t>(lanes, static_cast<size_t>(active_cnt) / L_WIDTH);

                    for (size_t l = 0; l < full_lanes; ++l) {
                        T* __restrict lane_ptr = p_view.get_lane(l);
                        for (size_t k = 0; k < L_WIDTH; ++k) {
                            lane_ptr[k] = _eval_rule_math(lane_ptr[k], operation, rule_value, clamp_min, clamp_max);
                        }
                    }
                    const int64_t tail_start = static_cast<int64_t>(full_lanes * L_WIDTH);
                    for (int64_t i = tail_start; i < active_cnt; ++i) {
                        dst_vals[i] = _eval_rule_math(dst_vals[i], operation, rule_value, clamp_min, clamp_max);
                    }
                    return;
                }
            }

            // --- Path C: Scalar / Selection-Aware Inline Rule Execution ---
            const uint64_t req_grp = static_cast<uint64_t>(required_group_mask_lo);
            auto eval_inline = [&](int64_t idx) noexcept {
                const T in_v = src_vals ? src_vals[idx] : dst_vals[idx];
                if (archetypes && idx < arch_cap && ((archetypes[idx] & required_archetype_mask) == 0u)) {
                    if (src_vals) dst_vals[idx] = in_v;
                    return;
                }
                if (groups && idx < grp_cap && req_grp != 0ULL && ((groups[idx] & req_grp) != req_grp)) {
                    if (src_vals) dst_vals[idx] = in_v;
                    return;
                }
                if (!is_operation_allowed(idx, operation, target_channel)) {
                    if (src_vals) dst_vals[idx] = in_v;
                    return;
                }
                dst_vals[idx] = _eval_rule_math(in_v, operation, rule_value, clamp_min, clamp_max);
            };
            _iterate_selection(sel, elem_count, eval_inline);
        }
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    [[nodiscard]] static inline T _eval_rule_math(
        T p_current,
        uint8_t p_op,
        float p_val,
        float p_cmin,
        float p_cmax
    ) noexcept {
        const float cur = static_cast<float>(p_current);
        float out = cur;
        switch (p_op) {
            case RULE_OP_SET:
                out = p_val;
                break;
            case RULE_OP_ADD:
                out = cur + p_val;
                break;
            case RULE_OP_MULTIPLY:
                out = cur * p_val;
                break;
            case RULE_OP_CLAMP: {
                const float lo = std::min(p_cmin, p_cmax);
                const float hi = std::max(p_cmin, p_cmax);
                out = std::clamp(cur, lo, hi);
                break;
            }
            case RULE_OP_INTERACTION_WEIGHT:
                out = cur * std::max(0.0f, p_val);
                break;
            default:
                break;
        }
        if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(std::lround(out));
        } else {
            return static_cast<T>(out);
        }
    }

    template <typename Fn>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _iterate_selection(const MemoryBufferSelectionPOD& p_sel, int64_t p_cap, Fn&& p_fn) noexcept {
        if (p_sel.mode == SelectionMode::DENSE && p_sel.data.bitset) {
            const uint64_t* bitset = p_sel.data.bitset;
            const int64_t cap = std::min<int64_t>(p_sel.capacity, p_cap);
            for (int64_t i = 0; i < cap; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) {
                    p_fn(i);
                }
            }
        } else if (p_sel.mode == SelectionMode::SPARSE && p_sel.data.indices) {
            for (int64_t i = 0; i < p_sel.element_count; ++i) {
                const int64_t idx = p_sel.data.indices[i];
                if (idx >= 0 && idx < p_cap) {
                    p_fn(idx);
                }
            }
        } else {
            const int64_t start = std::max<int64_t>(0, p_sel.start_index);
            const int64_t count = (p_sel.element_count > 0) ? p_sel.element_count : p_cap;
            const int64_t end   = std::min<int64_t>(start + count, p_cap);
            for (int64_t i = start; i < end; ++i) {
                p_fn(i);
            }
        }
    }
};

static_assert(sizeof(GameplayRuleApplyTransformLogic<>) == 64, "GameplayRuleApplyTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core