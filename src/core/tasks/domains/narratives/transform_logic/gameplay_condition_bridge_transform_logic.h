#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "symbolic_reference_resolve_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_GAMEPLAY_CONDITION_BRIDGE_DEFINED
#define IDEAM_NARRATIVES_GAMEPLAY_CONDITION_BRIDGE_DEFINED

/**
 * Predicates supported by GameplayConditionBridgeTransformLogic (N4) when evaluating
 * external GamePropertyBuffer columns or trigger volumes for Gameplay_Condition rows.
 */
enum GameplayConditionPredicateOp : uint16_t {
    GAMEPLAY_PRED_PROP_GREATER_EQUAL = 0, // modded_value[subject_id] >= threshold -> MET, else NOT_MET
    GAMEPLAY_PRED_PROP_LESS_EQUAL    = 1, // modded_value[subject_id] <= threshold -> MET, else NOT_MET
    GAMEPLAY_PRED_PROP_EQUAL         = 2, // modded_value[subject_id] == threshold -> MET, else NOT_MET
    GAMEPLAY_PRED_PROP_NOT_EQUAL     = 3, // modded_value[subject_id] != threshold -> MET, else NOT_MET
    GAMEPLAY_PRED_PROP_GREATER       = 4, // modded_value[subject_id] >  threshold -> MET, else NOT_MET
    GAMEPLAY_PRED_PROP_LESS          = 5, // modded_value[subject_id] <  threshold -> MET, else NOT_MET
    GAMEPLAY_PRED_PROP_IS_EXHAUSTED  = 6, // (flags & PROP_FLAG_IS_EXHAUSTED) or modded_value <= modded_min -> MET
    GAMEPLAY_PRED_PROP_AT_MAX        = 7, // (flags & PROP_FLAG_AT_MAX) or modded_value >= modded_max -> MET
    GAMEPLAY_PRED_PROP_IS_LOCKED     = 8, // (flags & PROP_FLAG_LOCKED) != 0 -> MET (or CANNOT_MEET if lock_means_cannot_meet)
    GAMEPLAY_PRED_TRIGGER_ACTIVE     = 9, // trigger_or_status_buffer_id[subject_id] != 0 -> MET, else NOT_MET
    GAMEPLAY_PRED_DIRECT_STATUS      = 10 // Directly copies NarremeConditionStatusDOD from trigger_or_status_buffer_id
};

/**
 * Bitflags matching GamePropertyBuffer::property_flags in IdeamGames (kept self-contained
 * so IdeamNarratives compiles with zero cross-domain header coupling).
 */
enum NarrativeBridgedPropertyFlags : uint16_t {
    NARRATIVE_PROP_FLAG_IS_EXHAUSTED = 1u << 9,
    NARRATIVE_PROP_FLAG_LOCKED       = 1u << 10,
    NARRATIVE_PROP_FLAG_AT_MAX       = 1u << 11
};

/**
 * GameplayConditionEventPOD (16-Byte Aligned)
 * Queued into a RingView / RingStrategy buffer by external gameplay triggers, physics volumes,
 * or interaction consequences and drained by GameplayConditionBridgeTransformLogic (N4).
 */
struct alignas(16) GameplayConditionEventPOD {
    uint32_t condition_id       = INVALID_ID;       // 4B: Target row in ConditionEvaluationTableBuffer
    int32_t gameplay_value      = 0;                // 4B: Optional numeric payload / property value snapshot
    uint32_t source_entity_id   = INVALID_ID;       // 4B: Optional instigating GameEntity / Trigger ID
    int8_t target_status        = COND_STATUS_MET;  // 1B: NarremeConditionStatusDOD (MET, NOT_MET, PERMANENT, CANNOT_MEET, UNKNOWN)
    uint8_t force_override      = 0u;               // 1B: Non-zero to override terminal PERMANENT / CANNOT_MEET latches
    uint16_t reserved           = 0u;               // 2B -> Total = 16B
};

static_assert(sizeof(GameplayConditionEventPOD) == 16, "GameplayConditionEventPOD must be 16 bytes!");

/**
 * Transition bitflags written to condition_transition_flags_buffer_id when a Gameplay_Condition
 * changes state, driving N11 signal emission ("met", "unmet", "cannot_meet", "became_unknown").
 */
enum ConditionStatusTransitionFlags : uint8_t {
    COND_TRANSITION_NONE           = 0u,
    COND_TRANSITION_MET            = 1u << 0, // Emits Incident_Condition "met"
    COND_TRANSITION_UNMET          = 1u << 1, // Emits Incident_Condition "unmet"
    COND_TRANSITION_CANNOT_MEET    = 1u << 2, // Emits Incident_Condition "cannot_meet"
    COND_TRANSITION_BECAME_UNKNOWN = 1u << 3  // Emits Gameplay_Condition "became_unknown"
};

#endif // IDEAM_NARRATIVES_GAMEPLAY_CONDITION_BRIDGE_DEFINED

/**
 * GameplayConditionBridgeTransformLogic<T> (N4)
 * Syncs external gameplay state (from GamePropertyBuffer columns, trigger volume buffers,
 * or RingView GameplayConditionEventPOD queues) into Gameplay_Condition status slots
 * (MET, NOT_MET, UNKNOWN, PERMANENT, CANNOT_MEET) inside ConditionEvaluationTableBuffer.
 *
 * Strictly preserves Gameplay_Condition::evaluate_condition() terminal latching semantics:
 * once a condition reaches CANNOT_MEET (-2) or PERMANENT (2), subsequent updates are ignored
 * unless allow_override_terminal_states is enabled.
 */
template <typename T = int32_t>
struct alignas(64) GameplayConditionBridgeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Gameplay Condition Bridge";
    static constexpr std::string_view ui_category  = "Conditions";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t condition_status_buffer_id     = INVALID_ID;                        // 4B: Primary target (ConditionEvaluationTableBuffer::current_status)
    uint32_t condition_type_buffer_id       = INVALID_ID;                        // 4B: Optional ConditionEvaluationTableBuffer::condition_type (uint8_t)
    uint32_t subject_id_buffer_id           = INVALID_ID;                        // 4B: Source property/trigger index per condition (uint32_t)
    uint32_t object_id_buffer_id            = INVALID_ID;                        // 4B: Optional threshold operand per condition (int32_t/uint32_t)
    uint32_t predicate_op_buffer_id         = INVALID_ID;                        // 4B: Optional GameplayConditionPredicateOp (uint16_t/uint32_t)
    uint32_t transition_flags_buffer_id     = INVALID_ID;                        // 4B: Optional uint8_t ConditionStatusTransitionFlags column
    uint32_t prop_modded_value_buffer_id    = INVALID_ID;                        // 4B: Bridged GamePropertyBuffer::modded_value (int32_t)
    uint32_t prop_modded_min_buffer_id      = INVALID_ID;                        // 4B: Optional GamePropertyBuffer::modded_min (int32_t)
    uint32_t prop_modded_max_buffer_id      = INVALID_ID;                        // 4B: Optional GamePropertyBuffer::modded_max (int32_t)
    uint32_t prop_flags_buffer_id           = INVALID_ID;                        // 4B: Optional GamePropertyBuffer::property_flags (uint16_t/uint32_t)
    uint32_t trigger_or_status_buffer_id    = INVALID_ID;                        // 4B: Optional external gameplay_status or trigger volume buffer
    uint32_t gameplay_event_ring_buffer_id  = INVALID_ID;                        // 4B: Optional GameplayConditionEventPOD ring buffer
    uint32_t ring_cursor_buffer_id          = INVALID_ID;                        // 4B: Optional uint32_t[2] [read_idx, write_idx] ring cursor buffer

    int32_t default_threshold_value         = 1;                                 // 4B: Fallback threshold when object_id_buffer_id is unbound
    uint16_t default_predicate_op           = GAMEPLAY_PRED_PROP_GREATER_EQUAL;  // 2B: Fallback GameplayConditionPredicateOp

    bool filter_gameplay_condition_type     = true;                              // 1B: Only evaluate rows where condition_type == CONDITION_TYPE_GAMEPLAY
    bool latch_met_as_permanent             = false;                             // 1B: Promote MET transitions directly to PERMANENT (2)
    bool lock_means_cannot_meet             = false;                             // 1B: Set CANNOT_MEET (-2) when bridged GameProperty is locked
    bool allow_override_terminal_states     = false;                             // 1B: Allow overwriting CANNOT_MEET / PERMANENT states
    bool wake_changed_conditions            = true;                              // 1B: Queue changed condition_id into TaskContextPOD::wave_commands
    uint8_t reserved_padding[1]             = {0};                               // 1B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary cstat_prop;
        cstat_prop["name"] = "condition_status_buffer_id";
        cstat_prop["type"] = godot::Variant::INT;
        cstat_prop["hint_string"] = "buffer_option";
        props.push_back(cstat_prop);

        godot::Dictionary ctyp_prop;
        ctyp_prop["name"] = "condition_type_buffer_id";
        ctyp_prop["type"] = godot::Variant::INT;
        ctyp_prop["hint_string"] = "buffer_option";
        props.push_back(ctyp_prop);

        godot::Dictionary sub_prop;
        sub_prop["name"] = "subject_id_buffer_id";
        sub_prop["type"] = godot::Variant::INT;
        sub_prop["hint_string"] = "buffer_option";
        props.push_back(sub_prop);

        godot::Dictionary obj_prop;
        obj_prop["name"] = "object_id_buffer_id";
        obj_prop["type"] = godot::Variant::INT;
        obj_prop["hint_string"] = "buffer_option";
        props.push_back(obj_prop);

        godot::Dictionary pop_prop;
        pop_prop["name"] = "predicate_op_buffer_id";
        pop_prop["type"] = godot::Variant::INT;
        pop_prop["hint_string"] = "buffer_option";
        props.push_back(pop_prop);

        godot::Dictionary trn_prop;
        trn_prop["name"] = "transition_flags_buffer_id";
        trn_prop["type"] = godot::Variant::INT;
        trn_prop["hint_string"] = "buffer_option";
        props.push_back(trn_prop);

        godot::Dictionary pval_prop;
        pval_prop["name"] = "prop_modded_value_buffer_id";
        pval_prop["type"] = godot::Variant::INT;
        pval_prop["hint_string"] = "buffer_option";
        props.push_back(pval_prop);

        godot::Dictionary pmin_prop;
        pmin_prop["name"] = "prop_modded_min_buffer_id";
        pmin_prop["type"] = godot::Variant::INT;
        pmin_prop["hint_string"] = "buffer_option";
        props.push_back(pmin_prop);

        godot::Dictionary pmax_prop;
        pmax_prop["name"] = "prop_modded_max_buffer_id";
        pmax_prop["type"] = godot::Variant::INT;
        pmax_prop["hint_string"] = "buffer_option";
        props.push_back(pmax_prop);

        godot::Dictionary pflg_prop;
        pflg_prop["name"] = "prop_flags_buffer_id";
        pflg_prop["type"] = godot::Variant::INT;
        pflg_prop["hint_string"] = "buffer_option";
        props.push_back(pflg_prop);

        godot::Dictionary trg_prop;
        trg_prop["name"] = "trigger_or_status_buffer_id";
        trg_prop["type"] = godot::Variant::INT;
        trg_prop["hint_string"] = "buffer_option";
        props.push_back(trg_prop);

        godot::Dictionary rng_prop;
        rng_prop["name"] = "gameplay_event_ring_buffer_id";
        rng_prop["type"] = godot::Variant::INT;
        rng_prop["hint_string"] = "buffer_option";
        props.push_back(rng_prop);

        godot::Dictionary rcur_prop;
        rcur_prop["name"] = "ring_cursor_buffer_id";
        rcur_prop["type"] = godot::Variant::INT;
        rcur_prop["hint_string"] = "buffer_option";
        props.push_back(rcur_prop);

        godot::Dictionary dpop_prop;
        dpop_prop["name"] = "default_predicate_op";
        dpop_prop["type"] = godot::Variant::INT;
        dpop_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dpop_prop["hint_string"] = "Prop >= Threshold:0,Prop <= Threshold:1,Prop == Threshold:2,Prop != Threshold:3,Prop > Threshold:4,Prop < Threshold:5,Prop Is Exhausted:6,Prop At Max:7,Prop Is Locked:8,Trigger Volume Active:9,Direct Status Copy:10";
        props.push_back(dpop_prop);

        godot::Dictionary dthr_prop;
        dthr_prop["name"] = "default_threshold_value";
        dthr_prop["type"] = godot::Variant::INT;
        dthr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(dthr_prop);

        godot::Dictionary flt_prop;
        flt_prop["name"] = "filter_gameplay_condition_type";
        flt_prop["type"] = godot::Variant::BOOL;
        flt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(flt_prop);

        godot::Dictionary lpm_prop;
        lpm_prop["name"] = "latch_met_as_permanent";
        lpm_prop["type"] = godot::Variant::BOOL;
        lpm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(lpm_prop);

        godot::Dictionary lcm_prop;
        lcm_prop["name"] = "lock_means_cannot_meet";
        lcm_prop["type"] = godot::Variant::BOOL;
        lcm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(lcm_prop);

        godot::Dictionary ovr_prop;
        ovr_prop["name"] = "allow_override_terminal_states";
        ovr_prop["type"] = godot::Variant::BOOL;
        ovr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ovr_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_changed_conditions";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (condition_status_buffer_id != INVALID_ID)    return condition_status_buffer_id;
        if (trigger_or_status_buffer_id != INVALID_ID)   return trigger_or_status_buffer_id;
        return gameplay_event_ring_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("condition_status_buffer_id")) {
            condition_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_status_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && condition_status_buffer_id == INVALID_ID) {
            condition_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("condition_type_buffer_id")) {
            condition_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_type_buffer_id"]));
        }
        if (p_props.has("subject_id_buffer_id")) {
            subject_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["subject_id_buffer_id"]));
        }
        if (p_props.has("object_id_buffer_id")) {
            object_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["object_id_buffer_id"]));
        }
        if (p_props.has("predicate_op_buffer_id")) {
            predicate_op_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["predicate_op_buffer_id"]));
        }
        if (p_props.has("transition_flags_buffer_id")) {
            transition_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transition_flags_buffer_id"]));
        }
        if (p_props.has("prop_modded_value_buffer_id")) {
            prop_modded_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["prop_modded_value_buffer_id"]));
        }
        if (p_props.has("prop_modded_min_buffer_id")) {
            prop_modded_min_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["prop_modded_min_buffer_id"]));
        }
        if (p_props.has("prop_modded_max_buffer_id")) {
            prop_modded_max_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["prop_modded_max_buffer_id"]));
        }
        if (p_props.has("prop_flags_buffer_id")) {
            prop_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["prop_flags_buffer_id"]));
        }
        if (p_props.has("trigger_or_status_buffer_id")) {
            trigger_or_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["trigger_or_status_buffer_id"]));
        }
        if (p_props.has("gameplay_event_ring_buffer_id")) {
            gameplay_event_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["gameplay_event_ring_buffer_id"]));
        }
        if (p_props.has("ring_cursor_buffer_id")) {
            ring_cursor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ring_cursor_buffer_id"]));
        }
        if (p_props.has("default_threshold_value")) {
            default_threshold_value = static_cast<int32_t>(static_cast<int64_t>(p_props["default_threshold_value"]));
        }
        if (p_props.has("default_predicate_op")) {
            default_predicate_op = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["default_predicate_op"]), 0, 10));
        }
        if (p_props.has("filter_gameplay_condition_type")) {
            filter_gameplay_condition_type = static_cast<bool>(p_props["filter_gameplay_condition_type"]);
        }
        if (p_props.has("latch_met_as_permanent")) {
            latch_met_as_permanent = static_cast<bool>(p_props["latch_met_as_permanent"]);
        }
        if (p_props.has("lock_means_cannot_meet")) {
            lock_means_cannot_meet = static_cast<bool>(p_props["lock_means_cannot_meet"]);
        }
        if (p_props.has("allow_override_terminal_states")) {
            allow_override_terminal_states = static_cast<bool>(p_props["allow_override_terminal_states"]);
        }
        if (p_props.has("wake_changed_conditions")) {
            wake_changed_conditions = static_cast<bool>(p_props["wake_changed_conditions"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint8_t>) {
            const uint32_t tgt_id = get_target_buffer_id();
            const GrantPartPOD* cstat_part = (condition_status_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(condition_status_buffer_id)
                : ((tgt_id != INVALID_ID) ? p_context.get_grant_part(tgt_id) : nullptr);
            if (!cstat_part || !cstat_part->raw_base_ptr) return;

            const GrantPartPOD* ctyp_part = (condition_type_buffer_id != INVALID_ID)      ? p_context.get_grant_part(condition_type_buffer_id)      : nullptr;
            const GrantPartPOD* sub_part  = (subject_id_buffer_id != INVALID_ID)          ? p_context.get_grant_part(subject_id_buffer_id)          : nullptr;
            const GrantPartPOD* obj_part  = (object_id_buffer_id != INVALID_ID)           ? p_context.get_grant_part(object_id_buffer_id)           : nullptr;
            const GrantPartPOD* pop_part  = (predicate_op_buffer_id != INVALID_ID)        ? p_context.get_grant_part(predicate_op_buffer_id)        : nullptr;
            const GrantPartPOD* trn_part  = (transition_flags_buffer_id != INVALID_ID)    ? p_context.get_grant_part(transition_flags_buffer_id)    : nullptr;
            const GrantPartPOD* pval_part = (prop_modded_value_buffer_id != INVALID_ID)   ? p_context.get_grant_part(prop_modded_value_buffer_id)   : nullptr;
            const GrantPartPOD* pmin_part = (prop_modded_min_buffer_id != INVALID_ID)     ? p_context.get_grant_part(prop_modded_min_buffer_id)     : nullptr;
            const GrantPartPOD* pmax_part = (prop_modded_max_buffer_id != INVALID_ID)     ? p_context.get_grant_part(prop_modded_max_buffer_id)     : nullptr;
            const GrantPartPOD* pflg_part = (prop_flags_buffer_id != INVALID_ID)          ? p_context.get_grant_part(prop_flags_buffer_id)          : nullptr;
            const GrantPartPOD* trg_part  = (trigger_or_status_buffer_id != INVALID_ID)   ? p_context.get_grant_part(trigger_or_status_buffer_id)   : nullptr;
            const GrantPartPOD* rng_part  = (gameplay_event_ring_buffer_id != INVALID_ID) ? p_context.get_grant_part(gameplay_event_ring_buffer_id) : nullptr;
            const GrantPartPOD* rcur_part = (ring_cursor_buffer_id != INVALID_ID)         ? p_context.get_grant_part(ring_cursor_buffer_id)         : nullptr;

            uint8_t* __restrict cstat_raw = cstat_part->raw_base_ptr;
            const uint32_t cstat_stride   = (cstat_part->element_stride > 0) ? cstat_part->element_stride : sizeof(T);
            const int64_t cond_cap        = static_cast<int64_t>(cstat_part->capacity_bytes / cstat_stride);
            if (cond_cap <= 0) return;

            const uint8_t* __restrict ctyp_raw    = (ctyp_part && ctyp_part->raw_base_ptr) ? ctyp_part->raw_base_ptr : nullptr;
            const uint32_t ctyp_stride            = ctyp_part ? ctyp_part->element_stride : 0;
            const int64_t ctyp_cap                = (ctyp_raw && ctyp_stride > 0) ? static_cast<int64_t>(ctyp_part->capacity_bytes / ctyp_stride) : 0;

            const uint32_t* __restrict subjects   = (sub_part && sub_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(sub_part->raw_base_ptr)   : nullptr;
            const int64_t sub_cap                 = subjects ? static_cast<int64_t>(sub_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const int32_t* __restrict objects     = (obj_part && obj_part->raw_base_ptr)   ? reinterpret_cast<const int32_t*>(obj_part->raw_base_ptr)    : nullptr;
            const int64_t obj_cap                 = objects ? static_cast<int64_t>(obj_part->capacity_bytes / sizeof(int32_t)) : 0;

            const uint8_t* __restrict pop_raw     = (pop_part && pop_part->raw_base_ptr)   ? pop_part->raw_base_ptr : nullptr;
            const uint32_t pop_stride             = pop_part ? pop_part->element_stride : 0;
            const int64_t pop_cap                 = (pop_raw && pop_stride > 0) ? static_cast<int64_t>(pop_part->capacity_bytes / pop_stride) : 0;

            uint8_t* __restrict trn_raw           = (trn_part && trn_part->raw_base_ptr)   ? trn_part->raw_base_ptr : nullptr;
            const uint32_t trn_stride             = trn_part ? trn_part->element_stride : 0;
            const int64_t trn_cap                 = (trn_raw && trn_stride > 0) ? static_cast<int64_t>(trn_part->capacity_bytes / trn_stride) : 0;

            const int32_t* __restrict prop_vals   = (pval_part && pval_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(pval_part->raw_base_ptr)   : nullptr;
            const int32_t* __restrict prop_mins   = (pmin_part && pmin_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(pmin_part->raw_base_ptr)   : nullptr;
            const int32_t* __restrict prop_maxs   = (pmax_part && pmax_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(pmax_part->raw_base_ptr)   : nullptr;
            const uint8_t* __restrict pflg_raw    = (pflg_part && pflg_part->raw_base_ptr) ? pflg_part->raw_base_ptr : nullptr;
            const uint32_t pflg_stride            = pflg_part ? pflg_part->element_stride : 0;

            const int64_t pval_cap                = prop_vals ? static_cast<int64_t>(pval_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t pmin_cap                = prop_mins ? static_cast<int64_t>(pmin_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t pmax_cap                = prop_maxs ? static_cast<int64_t>(pmax_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t pflg_cap                = (pflg_raw && pflg_stride > 0) ? static_cast<int64_t>(pflg_part->capacity_bytes / pflg_stride) : 0;

            const uint8_t* __restrict trg_raw     = (trg_part && trg_part->raw_base_ptr)   ? trg_part->raw_base_ptr : nullptr;
            const uint32_t trg_stride             = trg_part ? trg_part->element_stride : 0;
            const int64_t trg_cap                 = (trg_raw && trg_stride > 0) ? static_cast<int64_t>(trg_part->capacity_bytes / trg_stride) : 0;

            auto read_status = [&](int64_t idx) noexcept -> int8_t {
                if (cstat_stride == sizeof(int32_t)) {
                    return static_cast<int8_t>(reinterpret_cast<const int32_t*>(cstat_raw)[idx]);
                }
                return reinterpret_cast<const int8_t*>(cstat_raw)[idx];
            };

            auto write_status = [&](int64_t idx, int8_t val) noexcept {
                if (cstat_stride == sizeof(int32_t)) {
                    reinterpret_cast<int32_t*>(cstat_raw)[idx] = static_cast<int32_t>(val);
                } else {
                    reinterpret_cast<int8_t*>(cstat_raw)[idx] = val;
                }
            };

            auto write_transition_flag = [&](int64_t idx, uint8_t flag_bit) noexcept {
                if (!trn_raw || idx < 0 || idx >= trn_cap) return;
                if (trn_stride == sizeof(uint32_t)) {
                    reinterpret_cast<uint32_t*>(trn_raw)[idx] |= static_cast<uint32_t>(flag_bit);
                } else {
                    trn_raw[idx] |= flag_bit;
                }
            };

            // Applies a new status following Gameplay_Condition::evaluate_condition() latching & signal transition rules
            auto commit_gameplay_status = [&](int64_t idx, int8_t new_status, bool force_ovr) noexcept {
                if (idx < 0 || idx >= cond_cap) return;
                const int8_t cur_status = read_status(idx);
                if (new_status == cur_status) return;

                if (!allow_override_terminal_states && !force_ovr) {
                    if (cur_status == COND_STATUS_CANNOT_MEET || cur_status == COND_STATUS_PERMANENT) {
                        return;
                    }
                }

                if (latch_met_as_permanent && new_status == COND_STATUS_MET) {
                    new_status = COND_STATUS_PERMANENT;
                }

                uint8_t tflag = COND_TRANSITION_NONE;
                switch (new_status) {
                    case COND_STATUS_UNKNOWN:
                        tflag = COND_TRANSITION_BECAME_UNKNOWN;
                        break;
                    case COND_STATUS_MET:
                    case COND_STATUS_PERMANENT:
                        tflag = COND_TRANSITION_MET;
                        break;
                    case COND_STATUS_CANNOT_MEET:
                        tflag = COND_TRANSITION_CANNOT_MEET;
                        break;
                    case COND_STATUS_NOT_MET:
                        tflag = COND_TRANSITION_UNMET;
                        break;
                    default:
                        break;
                }

                write_status(idx, new_status);
                if (tflag != COND_TRANSITION_NONE) {
                    write_transition_flag(idx, tflag);
                }
                if (wake_changed_conditions) {
                    p_context.queue_selection_command(cstat_part->buffer_id, idx);
                }
            };

            // --- Phase 1: Drain GameplayConditionEventPOD Ring Buffer (if bound) ---
            if (rng_part && rng_part->raw_base_ptr) {
                const GameplayConditionEventPOD* __restrict events =
                    reinterpret_cast<const GameplayConditionEventPOD*>(rng_part->raw_base_ptr);
                const uint32_t ring_cap = static_cast<uint32_t>(rng_part->capacity_bytes / sizeof(GameplayConditionEventPOD));
                if (ring_cap > 0u) {
                    uint32_t* __restrict cursors = (rcur_part && rcur_part->raw_base_ptr && rcur_part->capacity_bytes >= 2 * sizeof(uint32_t))
                        ? reinterpret_cast<uint32_t*>(rcur_part->raw_base_ptr)
                        : nullptr;

                    if (cursors) {
                        uint32_t r_idx = cursors[0] % ring_cap;
                        const uint32_t w_idx = cursors[1] % ring_cap;
                        while (r_idx != w_idx) {
                            const GameplayConditionEventPOD& evt = events[r_idx];
                            if (evt.condition_id != INVALID_ID) {
                                commit_gameplay_status(
                                    static_cast<int64_t>(evt.condition_id),
                                    evt.target_status,
                                    evt.force_override != 0u
                                );
                            }
                            r_idx = (r_idx + 1u) % ring_cap;
                        }
                        cursors[0] = r_idx;
                    } else {
                        const uint32_t cnt = static_cast<uint32_t>(std::clamp<int64_t>(
                            rng_part->selection.element_count, 0, static_cast<int64_t>(ring_cap)
                        ));
                        for (uint32_t i = 0u; i < cnt; ++i) {
                            const GameplayConditionEventPOD& evt = events[i];
                            if (evt.condition_id != INVALID_ID) {
                                commit_gameplay_status(
                                    static_cast<int64_t>(evt.condition_id),
                                    evt.target_status,
                                    evt.force_override != 0u
                                );
                            }
                        }
                        const_cast<GrantPartPOD*>(rng_part)->selection.element_count = 0;
                    }
                }
            }

            // If neither GamePropertyBuffer nor trigger_or_status_buffer_id is bound, ring drain completed the pass
            if (!prop_vals && !pflg_raw && !trg_raw) return;

            auto read_cond_type = [&](int64_t idx) noexcept -> uint8_t {
                if (!ctyp_raw || idx < 0 || idx >= ctyp_cap) return CONDITION_TYPE_GAMEPLAY;
                if (ctyp_stride == sizeof(int32_t)) {
                    return static_cast<uint8_t>(reinterpret_cast<const int32_t*>(ctyp_raw)[idx]);
                }
                return ctyp_raw[idx];
            };

            auto read_pred_op = [&](int64_t idx) noexcept -> uint16_t {
                if (!pop_raw || idx < 0 || idx >= pop_cap) return default_predicate_op;
                if (pop_stride == sizeof(uint32_t)) {
                    return static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pop_raw)[idx]);
                }
                return reinterpret_cast<const uint16_t*>(pop_raw)[idx];
            };

            auto read_prop_flags = [&](int64_t pidx) noexcept -> uint16_t {
                if (!pflg_raw || pidx < 0 || pidx >= pflg_cap) return 0u;
                if (pflg_stride == sizeof(uint32_t)) {
                    return static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pflg_raw)[pidx]);
                }
                return reinterpret_cast<const uint16_t*>(pflg_raw)[pidx];
            };

            auto read_trigger_val = [&](int64_t tidx) noexcept -> int32_t {
                if (!trg_raw || tidx < 0 || tidx >= trg_cap) return 0;
                if (trg_stride == sizeof(int32_t)) {
                    return reinterpret_cast<const int32_t*>(trg_raw)[tidx];
                }
                return static_cast<int32_t>(reinterpret_cast<const int8_t*>(trg_raw)[tidx]);
            };

            // --- Phase 2: Evaluate Bridged GamePropertyBuffer & Trigger Volume Columns ---
            auto eval_gameplay_cond = [&](int64_t idx) noexcept {
                if (filter_gameplay_condition_type && ctyp_raw && read_cond_type(idx) != CONDITION_TYPE_GAMEPLAY) {
                    return;
                }

                const int64_t src_idx = (subjects && idx < sub_cap && subjects[idx] != INVALID_ID)
                    ? static_cast<int64_t>(subjects[idx])
                    : idx;
                const int32_t threshold = (objects && idx < obj_cap) ? objects[idx] : default_threshold_value;
                const uint16_t pred_op  = read_pred_op(idx);

                // Direct status mirror mode (or fallback when only trigger_or_status_buffer_id is bound)
                if (pred_op == GAMEPLAY_PRED_DIRECT_STATUS || (!prop_vals && !pflg_raw && trg_raw && pred_op != GAMEPLAY_PRED_TRIGGER_ACTIVE)) {
                    if (src_idx >= 0 && src_idx < trg_cap) {
                        const int32_t raw_st = read_trigger_val(src_idx);
                        const int8_t clamped_st = static_cast<int8_t>(std::clamp<int32_t>(raw_st, COND_STATUS_CANNOT_MEET, COND_STATUS_PERMANENT));
                        commit_gameplay_status(idx, clamped_st, false);
                    }
                    return;
                }

                if (pred_op == GAMEPLAY_PRED_TRIGGER_ACTIVE) {
                    if (src_idx >= 0 && src_idx < trg_cap) {
                        const int8_t st = (read_trigger_val(src_idx) != 0) ? COND_STATUS_MET : COND_STATUS_NOT_MET;
                        commit_gameplay_status(idx, st, false);
                    }
                    return;
                }

                // GamePropertyBuffer evaluation
                const uint16_t pflags = read_prop_flags(src_idx);
                if (lock_means_cannot_meet && (pflags & NARRATIVE_PROP_FLAG_LOCKED) != 0u) {
                    commit_gameplay_status(idx, COND_STATUS_CANNOT_MEET, false);
                    return;
                }

                const int32_t pval = (prop_vals && src_idx >= 0 && src_idx < pval_cap) ? prop_vals[src_idx] : 0;
                bool is_met = false;

                switch (pred_op) {
                    case GAMEPLAY_PRED_PROP_GREATER_EQUAL:
                        is_met = (pval >= threshold);
                        break;
                    case GAMEPLAY_PRED_PROP_LESS_EQUAL:
                        is_met = (pval <= threshold);
                        break;
                    case GAMEPLAY_PRED_PROP_EQUAL:
                        is_met = (pval == threshold);
                        break;
                    case GAMEPLAY_PRED_PROP_NOT_EQUAL:
                        is_met = (pval != threshold);
                        break;
                    case GAMEPLAY_PRED_PROP_GREATER:
                        is_met = (pval > threshold);
                        break;
                    case GAMEPLAY_PRED_PROP_LESS:
                        is_met = (pval < threshold);
                        break;
                    case GAMEPLAY_PRED_PROP_IS_EXHAUSTED: {
                        const int32_t pmin = (prop_mins && src_idx >= 0 && src_idx < pmin_cap) ? prop_mins[src_idx] : 0;
                        is_met = ((pflags & NARRATIVE_PROP_FLAG_IS_EXHAUSTED) != 0u) ||
                                 (prop_vals && src_idx >= 0 && src_idx < pval_cap && pval <= pmin);
                        break;
                    }
                    case GAMEPLAY_PRED_PROP_AT_MAX: {
                        const int32_t pmax = (prop_maxs && src_idx >= 0 && src_idx < pmax_cap) ? prop_maxs[src_idx] : 100;
                        is_met = ((pflags & NARRATIVE_PROP_FLAG_AT_MAX) != 0u) ||
                                 (prop_vals && src_idx >= 0 && src_idx < pval_cap && pval >= pmax);
                        break;
                    }
                    case GAMEPLAY_PRED_PROP_IS_LOCKED:
                        is_met = ((pflags & NARRATIVE_PROP_FLAG_LOCKED) != 0u);
                        break;
                    default:
                        is_met = (pval >= threshold);
                        break;
                }

                commit_gameplay_status(idx, is_met ? COND_STATUS_MET : COND_STATUS_NOT_MET, false);
            };

            _iterate_selection(cstat_part->selection, cond_cap, eval_gameplay_cond);
        }
    }

private:
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

static_assert(sizeof(GameplayConditionBridgeTransformLogic<>) == 64, "GameplayConditionBridgeTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core