#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/aosoa_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "narreme_symbolic_similarity_transform_logic.h"
#include "symbolic_reference_resolve_transform_logic.h"
#include "gameplay_condition_bridge_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_ATOMIC_PREDICATES_DEFINED
#define IDEAM_NARRATIVES_ATOMIC_PREDICATES_DEFINED

/**
 * Atomic Narrative_Condition predicate IDs stored in ConditionEvaluationTableBuffer::predicate_op (uint16_t).
 * Encodes all unary (object == INVALID_ID) and binary (subject, object) checks across:
 *   - Character::check_narrative_condition() (character.cpp)
 *   - Prop::check_narrative_condition()      (prop.cpp)
 *   - Location::check_narrative_condition()  (location.cpp)
 *   - Incident::check_narrative_condition()  (incident.cpp)
 *   - Plot::check_narrative_condition()      (plot.cpp)
 *   - Narrative::check_narrative_condition() (narrative.cpp)
 */
enum NarrativeAtomicPredicateOp : uint16_t {
    // --- Generic / Type-Dispatched Legacy Indices (0..9) ---
    // When predicate_op < 16, evaluation automatically dispatches on (subject_type, object_type, predicate_op)
    // to match the exact 0-based switch(p_condition_id) tables in Character, Prop, Location, Incident, Plot, and Narrative.
    NARRATIVE_PRED_AUTO_LEGACY_0              = 0,
    NARRATIVE_PRED_AUTO_LEGACY_1              = 1,
    NARRATIVE_PRED_AUTO_LEGACY_2              = 2,
    NARRATIVE_PRED_AUTO_LEGACY_3              = 3,
    NARRATIVE_PRED_AUTO_LEGACY_4              = 4,

    // --- Explicit Binary Predicates (subject, object != INVALID_ID) ---
    NARRATIVE_PRED_IS_SAME_NARREME            = 16, // subject_id == object_id ("is")
    NARRATIVE_PRED_SAME_LOCATION              = 17, // location_id[subject] == location_id[object] ("is at the same location as")
    NARRATIVE_PRED_SHARES_GOAL                = 18, // (goal_mask[subject] & goal_mask[object]) != 0 or CSR goal overlap ("has the same goal as")
    NARRATIVE_PRED_HAS_RELATIONSHIP_WITH      = 19, // CSR relationship slice of subject contains object_id ("has a relationship with")
    NARRATIVE_PRED_CHAR_POSSESSES_PROP        = 20, // possessor_id[object] == subject_id ("possesses")
    NARRATIVE_PRED_IS_AT_LOCATION             = 21, // location_id[subject] == object_id ("is at")
    NARRATIVE_PRED_CHAR_HAS_GOAL_PLOT         = 22, // Character subject has Plot object as goal ("has goal")
    NARRATIVE_PRED_PROP_POSSESSED_BY_CHAR     = 23, // possessor_id[subject] == object_id ("is possessed by")
    NARRATIVE_PRED_PROP_SAME_POSSESSOR        = 24, // possessor_id[subject] == possessor_id[object] ("has the same possessor as")
    NARRATIVE_PRED_LOCATION_HAS_CHARACTER     = 25, // location_id[object] == subject_id ("has character")
    NARRATIVE_PRED_LOCATION_HAS_PROP          = 26, // location_id[object] == subject_id ("has prop")
    NARRATIVE_PRED_INCIDENT_HAS_SUBJECT       = 27, // Incident subject has a condition with subject == object ("has condition subject")
    NARRATIVE_PRED_INCIDENT_HAS_OBJECT        = 28, // Incident subject has a condition with object == object ("has condition object")
    NARRATIVE_PRED_PLOT_HAS_EVENT_INCIDENT    = 29, // Plot subject has a Plot_Event referencing Incident object ("has plot event incident")

    // --- Explicit Unary Predicates (object_id == INVALID_ID) ---
    NARRATIVE_PRED_CHAR_IS_SOMEWHERE          = 32, // location_id[subject] != INVALID_ID ("is somewhere.")
    NARRATIVE_PRED_CHAR_HAS_POSSESSIONS       = 33, // possession_count[subject] > 0 ("has possessions.")
    NARRATIVE_PRED_CHAR_HAS_RELATIONSHIPS     = 34, // relationship_count[subject] > 0 ("has relationships.")
    NARRATIVE_PRED_CHAR_HAS_GOALS             = 35, // goal_count[subject] > 0 ("has goals.")
    NARRATIVE_PRED_PROP_IS_LOST               = 36, // possessor_id == INVALID_ID && location_id == INVALID_ID ("is lost")
    NARRATIVE_PRED_PROP_HAS_POSSESSOR         = 37, // possessor_id != INVALID_ID ("has possessor")
    NARRATIVE_PRED_PROP_IS_SOMEWHERE          = 38, // location_id != INVALID_ID ("is somewhere")
    NARRATIVE_PRED_LOCATION_IS_EMPTY          = 39, // char_count == 0 && prop_count == 0 ("is empty")
    NARRATIVE_PRED_LOCATION_HAS_CHARACTERS    = 40, // char_count > 0 ("has characters present")
    NARRATIVE_PRED_LOCATION_HAS_PROPS         = 41, // prop_count > 0 ("has props present")
    NARRATIVE_PRED_INCIDENT_HAS_NOT_HAPPENED  = 42, // incident_status == HAS_NOT_HAPPENED (0)
    NARRATIVE_PRED_INCIDENT_HAS_HAPPENED      = 43, // incident_status == HAS_HAPPENED (1)
    NARRATIVE_PRED_INCIDENT_CANNOT_HAPPEN     = 44, // incident_status == CANNOT_HAPPEN (2)
    NARRATIVE_PRED_PLOT_NOT_BEGUN             = 45, // plot_status == PLOT_NOT_BEGUN (0)
    NARRATIVE_PRED_PLOT_IN_PROGRESS           = 46, // plot_status == PLOT_IN_PROGRESS (1)
    NARRATIVE_PRED_PLOT_COMPLETE              = 47, // plot_status == PLOT_COMPLETE (2)
    NARRATIVE_PRED_PLOT_CANNOT_COMPLETE       = 48, // plot_status == PLOT_CANNOT_COMPLETE (3)
    NARRATIVE_PRED_PLOT_FAILED                = 49  // plot_status == PLOT_FAILED (4)
};

#endif // IDEAM_NARRATIVES_ATOMIC_PREDICATES_DEFINED

/**
 * NarrativeConditionEvaluateTransformLogic<T> (N5)
 * Evaluates atomic Narrative_Condition predicates (location co-presence, prop possession,
 * relationship existence, goal status, incident status, and plot status) over
 * NarremeIdentityAndTraitBuffer and NarrativeRelationshipCSRBuffer in SIMD / TiledSoA.
 *
 * Replaces virtual Narreme::check_narrative_condition() across Character, Prop, Location,
 * Incident, Plot, and Narrative, as well as Narrative_Condition::evaluate_condition().
 */
template <typename T = int32_t>
struct alignas(64) NarrativeConditionEvaluateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = TiledSoAStrategy;
    using DefaultView     = AOSOAView<T, 8, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Narrative Condition Evaluate";
    static constexpr std::string_view ui_category  = "Conditions";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t condition_status_buffer_id      = INVALID_ID; // 4B: Primary target (ConditionEvaluationTableBuffer::current_status)
    uint32_t condition_type_buffer_id        = INVALID_ID; // 4B: ConditionEvaluationTableBuffer::condition_type (uint8_t)
    uint32_t subject_id_buffer_id            = INVALID_ID; // 4B: Resolved subject_id (uint32_t) in ConditionEvaluationTableBuffer
    uint32_t object_id_buffer_id             = INVALID_ID; // 4B: Resolved object_id (uint32_t) in ConditionEvaluationTableBuffer
    uint32_t predicate_op_buffer_id          = INVALID_ID; // 4B: ConditionEvaluationTableBuffer::predicate_op (uint16_t/uint32_t)
    uint32_t narreme_type_buffer_id          = INVALID_ID; // 4B: NarremeIdentityAndTraitBuffer::narreme_type (uint8_t)
    uint32_t location_id_buffer_id           = INVALID_ID; // 4B: NarremeIdentityAndTraitBuffer::location_id (uint32_t)
    uint32_t possessor_id_buffer_id          = INVALID_ID; // 4B: NarremeIdentityAndTraitBuffer::possessor_id (uint32_t)
    uint32_t goal_or_occupancy_mask_buffer_id= INVALID_ID; // 4B: uint64_t per-Narreme goal bitmask / occupancy summary
    uint32_t rel_csr_offset_buffer_id        = INVALID_ID; // 4B: NarrativeRelationshipCSRBuffer row offset (uint32_t)
    uint32_t rel_csr_count_buffer_id         = INVALID_ID; // 4B: NarrativeRelationshipCSRBuffer row count (uint16_t/uint32_t)
    uint32_t rel_csr_edge_pod_buffer_id      = INVALID_ID; // 4B: NarrativeRelationshipCSRBuffer RelationshipEdgePOD array
    uint32_t incident_or_plot_status_buffer_id=INVALID_ID; // 4B: IncidentAndPlotStateBuffer status column (int8_t/int32_t)
    uint32_t transition_flags_buffer_id      = INVALID_ID; // 4B: Optional uint8_t ConditionStatusTransitionFlags column

    uint16_t default_predicate_op            = 0u;         // 2B: Fallback predicate_op when predicate_op_buffer_id is unbound

    bool filter_narrative_condition_type     = true;       // 1B: Only evaluate rows where condition_type == CONDITION_TYPE_NARRATIVE
    bool preserve_permanent_and_cannot_meet  = true;       // 1B: Do not overwrite terminal CANNOT_MEET / PERMANENT states
    bool wake_changed_conditions             = true;       // 1B: Queue changed condition_id into TaskContextPOD::wave_commands
    uint8_t reserved_padding[3]              = {0};        // 3B -> Total = 64B

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

        godot::Dictionary ntyp_prop;
        ntyp_prop["name"] = "narreme_type_buffer_id";
        ntyp_prop["type"] = godot::Variant::INT;
        ntyp_prop["hint_string"] = "buffer_option";
        props.push_back(ntyp_prop);

        godot::Dictionary loc_prop;
        loc_prop["name"] = "location_id_buffer_id";
        loc_prop["type"] = godot::Variant::INT;
        loc_prop["hint_string"] = "buffer_option";
        props.push_back(loc_prop);

        godot::Dictionary pos_prop;
        pos_prop["name"] = "possessor_id_buffer_id";
        pos_prop["type"] = godot::Variant::INT;
        pos_prop["hint_string"] = "buffer_option";
        props.push_back(pos_prop);

        godot::Dictionary gmsk_prop;
        gmsk_prop["name"] = "goal_or_occupancy_mask_buffer_id";
        gmsk_prop["type"] = godot::Variant::INT;
        gmsk_prop["hint_string"] = "buffer_option";
        props.push_back(gmsk_prop);

        godot::Dictionary roff_prop;
        roff_prop["name"] = "rel_csr_offset_buffer_id";
        roff_prop["type"] = godot::Variant::INT;
        roff_prop["hint_string"] = "buffer_option";
        props.push_back(roff_prop);

        godot::Dictionary rcnt_prop;
        rcnt_prop["name"] = "rel_csr_count_buffer_id";
        rcnt_prop["type"] = godot::Variant::INT;
        rcnt_prop["hint_string"] = "buffer_option";
        props.push_back(rcnt_prop);

        godot::Dictionary redg_prop;
        redg_prop["name"] = "rel_csr_edge_pod_buffer_id";
        redg_prop["type"] = godot::Variant::INT;
        redg_prop["hint_string"] = "buffer_option";
        props.push_back(redg_prop);

        godot::Dictionary ipst_prop;
        ipst_prop["name"] = "incident_or_plot_status_buffer_id";
        ipst_prop["type"] = godot::Variant::INT;
        ipst_prop["hint_string"] = "buffer_option";
        props.push_back(ipst_prop);

        godot::Dictionary trn_prop;
        trn_prop["name"] = "transition_flags_buffer_id";
        trn_prop["type"] = godot::Variant::INT;
        trn_prop["hint_string"] = "buffer_option";
        props.push_back(trn_prop);

        godot::Dictionary dpop_prop;
        dpop_prop["name"] = "default_predicate_op";
        dpop_prop["type"] = godot::Variant::INT;
        dpop_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(dpop_prop);

        godot::Dictionary flt_prop;
        flt_prop["name"] = "filter_narrative_condition_type";
        flt_prop["type"] = godot::Variant::BOOL;
        flt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(flt_prop);

        godot::Dictionary prv_prop;
        prv_prop["name"] = "preserve_permanent_and_cannot_meet";
        prv_prop["type"] = godot::Variant::BOOL;
        prv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(prv_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_changed_conditions";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (condition_status_buffer_id != INVALID_ID) return condition_status_buffer_id;
        return subject_id_buffer_id;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        (void)p_context;
        (void)p_part;
        if constexpr (requires { p_view.strategy.elements_per_tile; p_view.strategy.tile_stride_bytes; }) {
            if (p_view.strategy.elements_per_tile == 0u) {
                p_view.strategy.elements_per_tile = 8u;
                p_view.strategy.tile_stride_bytes = static_cast<uint32_t>(8u * sizeof(T));
            }
        }
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
        if (p_props.has("narreme_type_buffer_id")) {
            narreme_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["narreme_type_buffer_id"]));
        }
        if (p_props.has("location_id_buffer_id")) {
            location_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["location_id_buffer_id"]));
        }
        if (p_props.has("possessor_id_buffer_id")) {
            possessor_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["possessor_id_buffer_id"]));
        }
        if (p_props.has("goal_or_occupancy_mask_buffer_id")) {
            goal_or_occupancy_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["goal_or_occupancy_mask_buffer_id"]));
        }
        if (p_props.has("rel_csr_offset_buffer_id")) {
            rel_csr_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rel_csr_offset_buffer_id"]));
        }
        if (p_props.has("rel_csr_count_buffer_id")) {
            rel_csr_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rel_csr_count_buffer_id"]));
        }
        if (p_props.has("rel_csr_edge_pod_buffer_id")) {
            rel_csr_edge_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rel_csr_edge_pod_buffer_id"]));
        }
        if (p_props.has("incident_or_plot_status_buffer_id")) {
            incident_or_plot_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["incident_or_plot_status_buffer_id"]));
        }
        if (p_props.has("transition_flags_buffer_id")) {
            transition_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transition_flags_buffer_id"]));
        }
        if (p_props.has("default_predicate_op")) {
            default_predicate_op = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["default_predicate_op"]), 0, 65535));
        }
        if (p_props.has("filter_narrative_condition_type")) {
            filter_narrative_condition_type = static_cast<bool>(p_props["filter_narrative_condition_type"]);
        }
        if (p_props.has("preserve_permanent_and_cannot_meet")) {
            preserve_permanent_and_cannot_meet = static_cast<bool>(p_props["preserve_permanent_and_cannot_meet"]);
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
            const GrantPartPOD* cstat_part = (tgt_id != INVALID_ID) ? p_context.get_grant_part(tgt_id) : nullptr;
            if (!cstat_part || !cstat_part->raw_base_ptr) return;

            const GrantPartPOD* ctyp_part = (condition_type_buffer_id != INVALID_ID)        ? p_context.get_grant_part(condition_type_buffer_id)        : nullptr;
            const GrantPartPOD* sub_part  = (subject_id_buffer_id != INVALID_ID)            ? p_context.get_grant_part(subject_id_buffer_id)            : nullptr;
            const GrantPartPOD* obj_part  = (object_id_buffer_id != INVALID_ID)             ? p_context.get_grant_part(object_id_buffer_id)             : nullptr;
            const GrantPartPOD* pop_part  = (predicate_op_buffer_id != INVALID_ID)          ? p_context.get_grant_part(predicate_op_buffer_id)          : nullptr;
            const GrantPartPOD* ntyp_part = (narreme_type_buffer_id != INVALID_ID)          ? p_context.get_grant_part(narreme_type_buffer_id)          : nullptr;
            const GrantPartPOD* loc_part  = (location_id_buffer_id != INVALID_ID)           ? p_context.get_grant_part(location_id_buffer_id)           : nullptr;
            const GrantPartPOD* pos_part  = (possessor_id_buffer_id != INVALID_ID)          ? p_context.get_grant_part(possessor_id_buffer_id)          : nullptr;
            const GrantPartPOD* gmsk_part = (goal_or_occupancy_mask_buffer_id != INVALID_ID)? p_context.get_grant_part(goal_or_occupancy_mask_buffer_id): nullptr;
            const GrantPartPOD* roff_part = (rel_csr_offset_buffer_id != INVALID_ID)        ? p_context.get_grant_part(rel_csr_offset_buffer_id)        : nullptr;
            const GrantPartPOD* rcnt_part = (rel_csr_count_buffer_id != INVALID_ID)         ? p_context.get_grant_part(rel_csr_count_buffer_id)         : nullptr;
            const GrantPartPOD* redg_part = (rel_csr_edge_pod_buffer_id != INVALID_ID)      ? p_context.get_grant_part(rel_csr_edge_pod_buffer_id)      : nullptr;
            const GrantPartPOD* ipst_part = (incident_or_plot_status_buffer_id != INVALID_ID)?p_context.get_grant_part(incident_or_plot_status_buffer_id):nullptr;
            const GrantPartPOD* trn_part  = (transition_flags_buffer_id != INVALID_ID)      ? p_context.get_grant_part(transition_flags_buffer_id)      : nullptr;

            uint8_t* __restrict cstat_raw = cstat_part->raw_base_ptr;
            const uint32_t cstat_stride   = (cstat_part->element_stride > 0) ? cstat_part->element_stride : sizeof(T);
            const int64_t cond_cap        = static_cast<int64_t>(cstat_part->capacity_bytes / cstat_stride);
            if (cond_cap <= 0) return;

            const uint8_t* __restrict ctyp_raw    = (ctyp_part && ctyp_part->raw_base_ptr) ? ctyp_part->raw_base_ptr : nullptr;
            const uint32_t ctyp_stride            = ctyp_part ? ctyp_part->element_stride : 0;
            const int64_t ctyp_cap                = (ctyp_raw && ctyp_stride > 0) ? static_cast<int64_t>(ctyp_part->capacity_bytes / ctyp_stride) : 0;

            const uint32_t* __restrict subjects   = (sub_part && sub_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(sub_part->raw_base_ptr)   : nullptr;
            const int64_t sub_cap                 = subjects ? static_cast<int64_t>(sub_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint32_t* __restrict objects    = (obj_part && obj_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(obj_part->raw_base_ptr)   : nullptr;
            const int64_t obj_cap                 = objects ? static_cast<int64_t>(obj_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict pop_raw     = (pop_part && pop_part->raw_base_ptr)   ? pop_part->raw_base_ptr : nullptr;
            const uint32_t pop_stride             = pop_part ? pop_part->element_stride : 0;
            const int64_t pop_cap                 = (pop_raw && pop_stride > 0) ? static_cast<int64_t>(pop_part->capacity_bytes / pop_stride) : 0;

            const uint8_t* __restrict ntyp_raw    = (ntyp_part && ntyp_part->raw_base_ptr) ? ntyp_part->raw_base_ptr : nullptr;
            const uint32_t ntyp_stride            = ntyp_part ? ntyp_part->element_stride : 0;
            const int64_t ntyp_cap                = (ntyp_raw && ntyp_stride > 0) ? static_cast<int64_t>(ntyp_part->capacity_bytes / ntyp_stride) : 0;

            const uint32_t* __restrict loc_ids    = (loc_part && loc_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(loc_part->raw_base_ptr)   : nullptr;
            const int64_t loc_cap                 = loc_ids ? static_cast<int64_t>(loc_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint32_t* __restrict pos_ids    = (pos_part && pos_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(pos_part->raw_base_ptr)   : nullptr;
            const int64_t pos_cap                 = pos_ids ? static_cast<int64_t>(pos_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint64_t* __restrict goal_masks = (gmsk_part && gmsk_part->raw_base_ptr) ? reinterpret_cast<const uint64_t*>(gmsk_part->raw_base_ptr)  : nullptr;
            const int64_t gmsk_cap                = goal_masks ? static_cast<int64_t>(gmsk_part->capacity_bytes / sizeof(uint64_t)) : 0;

            const uint32_t* __restrict csr_offsets= (roff_part && roff_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(roff_part->raw_base_ptr)  : nullptr;
            const int64_t roff_cap                = csr_offsets ? static_cast<int64_t>(roff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict rcnt_raw    = (rcnt_part && rcnt_part->raw_base_ptr) ? rcnt_part->raw_base_ptr : nullptr;
            const uint32_t rcnt_stride            = rcnt_part ? rcnt_part->element_stride : 0;
            const int64_t rcnt_cap                = (rcnt_raw && rcnt_stride > 0) ? static_cast<int64_t>(rcnt_part->capacity_bytes / rcnt_stride) : 0;

            const RelationshipEdgePOD* __restrict csr_edges = (redg_part && redg_part->raw_base_ptr)
                ? reinterpret_cast<const RelationshipEdgePOD*>(redg_part->raw_base_ptr)
                : nullptr;
            const int64_t redg_cap                = csr_edges ? static_cast<int64_t>(redg_part->capacity_bytes / sizeof(RelationshipEdgePOD)) : 0;

            const uint8_t* __restrict ipst_raw    = (ipst_part && ipst_part->raw_base_ptr) ? ipst_part->raw_base_ptr : nullptr;
            const uint32_t ipst_stride            = ipst_part ? ipst_part->element_stride : 0;
            const int64_t ipst_cap                = (ipst_raw && ipst_stride > 0) ? static_cast<int64_t>(ipst_part->capacity_bytes / ipst_stride) : 0;

            uint8_t* __restrict trn_raw           = (trn_part && trn_part->raw_base_ptr)   ? trn_part->raw_base_ptr : nullptr;
            const uint32_t trn_stride             = trn_part ? trn_part->element_stride : 0;
            const int64_t trn_cap                 = (trn_raw && trn_stride > 0) ? static_cast<int64_t>(trn_part->capacity_bytes / trn_stride) : 0;

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

            auto read_narreme_type = [&](uint32_t nid) noexcept -> uint8_t {
                if (!ntyp_raw || nid == INVALID_ID || static_cast<int64_t>(nid) >= ntyp_cap) return NARREME_TYPE_CHARACTER;
                if (ntyp_stride == sizeof(int32_t)) {
                    return static_cast<uint8_t>(reinterpret_cast<const int32_t*>(ntyp_raw)[nid]);
                }
                return ntyp_raw[nid];
            };

            auto read_csr_count = [&](uint32_t nid) noexcept -> uint32_t {
                if (!rcnt_raw || nid == INVALID_ID || static_cast<int64_t>(nid) >= rcnt_cap) return 0u;
                if (rcnt_stride == sizeof(uint32_t)) {
                    return reinterpret_cast<const uint32_t*>(rcnt_raw)[nid];
                }
                return static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(rcnt_raw)[nid]);
            };

            auto has_csr_edge_to = [&](uint32_t sub_id, uint32_t obj_id) noexcept -> bool {
                if (!csr_edges || !csr_offsets || !rcnt_raw || sub_id == INVALID_ID || obj_id == INVALID_ID) return false;
                if (static_cast<int64_t>(sub_id) >= roff_cap || static_cast<int64_t>(sub_id) >= rcnt_cap) return false;
                const int64_t off = static_cast<int64_t>(csr_offsets[sub_id]);
                const int64_t cnt = static_cast<int64_t>(read_csr_count(sub_id));
                const int64_t end = std::min<int64_t>(off + cnt, redg_cap);
                for (int64_t e = off; e < end; ++e) {
                    if (csr_edges[e].target_narreme_id == obj_id) return true;
                }
                return false;
            };

            auto read_inc_or_plot_status = [&](uint32_t nid) noexcept -> int32_t {
                if (!ipst_raw || nid == INVALID_ID || static_cast<int64_t>(nid) >= ipst_cap) return 0;
                if (ipst_stride == sizeof(int32_t)) {
                    return reinterpret_cast<const int32_t*>(ipst_raw)[nid];
                }
                return static_cast<int32_t>(reinterpret_cast<const int8_t*>(ipst_raw)[nid]);
            };

            auto resolve_canonical_predicate = [&](uint16_t raw_op, uint32_t sub_id, uint32_t obj_id) noexcept -> uint16_t {
                if (raw_op >= 16u) return raw_op;
                const uint8_t stype = read_narreme_type(sub_id);
                const bool has_obj  = (obj_id != INVALID_ID);

                if (!has_obj) {
                    switch (stype) {
                        case NARREME_TYPE_CHARACTER:
                            return static_cast<uint16_t>(NARRATIVE_PRED_CHAR_IS_SOMEWHERE + std::min<uint16_t>(raw_op, 3u));
                        case NARREME_TYPE_PROP:
                            return static_cast<uint16_t>(NARRATIVE_PRED_PROP_IS_LOST + std::min<uint16_t>(raw_op, 2u));
                        case NARREME_TYPE_LOCATION:
                            return static_cast<uint16_t>(NARRATIVE_PRED_LOCATION_IS_EMPTY + std::min<uint16_t>(raw_op, 2u));
                        case NARREME_TYPE_INCIDENT:
                            return static_cast<uint16_t>(NARRATIVE_PRED_INCIDENT_HAS_NOT_HAPPENED + std::min<uint16_t>(raw_op, 2u));
                        case NARREME_TYPE_PLOT:
                            return static_cast<uint16_t>(NARRATIVE_PRED_PLOT_NOT_BEGUN + std::min<uint16_t>(raw_op, 4u));
                        default:
                            return NARRATIVE_PRED_CHAR_IS_SOMEWHERE;
                    }
                }

                const uint8_t otype = read_narreme_type(obj_id);
                switch (stype) {
                    case NARREME_TYPE_CHARACTER:
                        if (otype == NARREME_TYPE_CHARACTER) {
                            if (raw_op == 0u) return NARRATIVE_PRED_IS_SAME_NARREME;
                            if (raw_op == 1u) return NARRATIVE_PRED_SAME_LOCATION;
                            if (raw_op == 2u) return NARRATIVE_PRED_SHARES_GOAL;
                            return NARRATIVE_PRED_HAS_RELATIONSHIP_WITH;
                        }
                        if (otype == NARREME_TYPE_PROP) {
                            return (raw_op == 0u) ? NARRATIVE_PRED_CHAR_POSSESSES_PROP : NARRATIVE_PRED_HAS_RELATIONSHIP_WITH;
                        }
                        if (otype == NARREME_TYPE_LOCATION) {
                            return (raw_op == 0u) ? NARRATIVE_PRED_IS_AT_LOCATION : NARRATIVE_PRED_HAS_RELATIONSHIP_WITH;
                        }
                        if (otype == NARREME_TYPE_PLOT) {
                            return (raw_op == 0u) ? NARRATIVE_PRED_CHAR_HAS_GOAL_PLOT : NARRATIVE_PRED_HAS_RELATIONSHIP_WITH;
                        }
                        return NARRATIVE_PRED_HAS_RELATIONSHIP_WITH;

                    case NARREME_TYPE_PROP:
                        if (otype == NARREME_TYPE_CHARACTER) {
                            return (raw_op == 0u) ? NARRATIVE_PRED_PROP_POSSESSED_BY_CHAR : NARRATIVE_PRED_SAME_LOCATION;
                        }
                        if (otype == NARREME_TYPE_PROP) {
                            if (raw_op == 0u) return NARRATIVE_PRED_IS_SAME_NARREME;
                            if (raw_op == 1u) return NARRATIVE_PRED_PROP_SAME_POSSESSOR;
                            return NARRATIVE_PRED_SAME_LOCATION;
                        }
                        return NARRATIVE_PRED_IS_AT_LOCATION;

                    case NARREME_TYPE_LOCATION:
                        if (otype == NARREME_TYPE_CHARACTER) return NARRATIVE_PRED_LOCATION_HAS_CHARACTER;
                        if (otype == NARREME_TYPE_PROP)      return NARRATIVE_PRED_LOCATION_HAS_PROP;
                        return NARRATIVE_PRED_IS_SAME_NARREME;

                    case NARREME_TYPE_INCIDENT:
                        return (raw_op == 0u) ? NARRATIVE_PRED_INCIDENT_HAS_SUBJECT : NARRATIVE_PRED_INCIDENT_HAS_OBJECT;

                    case NARREME_TYPE_PLOT:
                        return (otype == NARREME_TYPE_INCIDENT) ? NARRATIVE_PRED_PLOT_HAS_EVENT_INCIDENT : NARRATIVE_PRED_IS_SAME_NARREME;

                    default:
                        return NARRATIVE_PRED_IS_SAME_NARREME;
                }
            };

            auto eval_predicate = [&](uint32_t sub_id, uint32_t obj_id, uint16_t raw_op) noexcept -> int8_t {
                if (sub_id == INVALID_ID) {
                    return COND_STATUS_CANNOT_MEET;
                }

                const uint16_t op = resolve_canonical_predicate(raw_op, sub_id, obj_id);
                const int64_t sidx = static_cast<int64_t>(sub_id);
                const int64_t oidx = (obj_id != INVALID_ID) ? static_cast<int64_t>(obj_id) : -1;

                bool met = false;
                switch (op) {
                    case NARRATIVE_PRED_IS_SAME_NARREME:
                        met = (sub_id == obj_id);
                        break;
                    case NARRATIVE_PRED_SAME_LOCATION:
                        met = (loc_ids && oidx >= 0 && sidx < loc_cap && oidx < loc_cap) &&
                              (loc_ids[sidx] == loc_ids[oidx]);
                        break;
                    case NARRATIVE_PRED_SHARES_GOAL:
                        if (goal_masks && oidx >= 0 && sidx < gmsk_cap && oidx < gmsk_cap) {
                            met = (goal_masks[sidx] & goal_masks[oidx]) != 0ULL;
                        }
                        break;
                    case NARRATIVE_PRED_HAS_RELATIONSHIP_WITH:
                    case NARRATIVE_PRED_INCIDENT_HAS_SUBJECT:
                    case NARRATIVE_PRED_INCIDENT_HAS_OBJECT:
                    case NARRATIVE_PRED_PLOT_HAS_EVENT_INCIDENT:
                        met = has_csr_edge_to(sub_id, obj_id);
                        break;
                    case NARRATIVE_PRED_CHAR_POSSESSES_PROP:
                        if (pos_ids && oidx >= 0 && oidx < pos_cap) {
                            met = (pos_ids[oidx] == sub_id);
                        } else {
                            met = has_csr_edge_to(sub_id, obj_id);
                        }
                        break;
                    case NARRATIVE_PRED_IS_AT_LOCATION:
                        met = (loc_ids && sidx < loc_cap && loc_ids[sidx] == obj_id);
                        break;
                    case NARRATIVE_PRED_CHAR_HAS_GOAL_PLOT:
                        if (goal_masks && sidx < gmsk_cap && obj_id < 64u) {
                            met = (goal_masks[sidx] & (1ULL << obj_id)) != 0ULL;
                        } else {
                            met = has_csr_edge_to(sub_id, obj_id);
                        }
                        break;
                    case NARRATIVE_PRED_PROP_POSSESSED_BY_CHAR:
                        met = (pos_ids && sidx < pos_cap && pos_ids[sidx] == obj_id);
                        break;
                    case NARRATIVE_PRED_PROP_SAME_POSSESSOR:
                        met = (pos_ids && oidx >= 0 && sidx < pos_cap && oidx < pos_cap) &&
                              (pos_ids[sidx] == pos_ids[oidx]);
                        break;
                    case NARRATIVE_PRED_LOCATION_HAS_CHARACTER:
                    case NARRATIVE_PRED_LOCATION_HAS_PROP:
                        if (loc_ids && oidx >= 0 && oidx < loc_cap) {
                            met = (loc_ids[oidx] == sub_id);
                        } else {
                            met = has_csr_edge_to(sub_id, obj_id);
                        }
                        break;
                    case NARRATIVE_PRED_CHAR_IS_SOMEWHERE:
                    case NARRATIVE_PRED_PROP_IS_SOMEWHERE:
                        met = (loc_ids && sidx < loc_cap && loc_ids[sidx] != INVALID_ID);
                        break;
                    case NARRATIVE_PRED_CHAR_HAS_POSSESSIONS: {
                        if (goal_masks && sidx < gmsk_cap) {
                            met = (goal_masks[sidx] != 0ULL);
                        } else if (pos_ids && pos_cap > 0) {
                            for (int64_t p = 0; p < pos_cap; ++p) {
                                if (pos_ids[p] == sub_id) { met = true; break; }
                            }
                        } else {
                            met = (read_csr_count(sub_id) > 0u);
                        }
                        break;
                    }
                    case NARRATIVE_PRED_CHAR_HAS_RELATIONSHIPS:
                        met = (read_csr_count(sub_id) > 0u);
                        break;
                    case NARRATIVE_PRED_CHAR_HAS_GOALS:
                        met = (goal_masks && sidx < gmsk_cap) ? (goal_masks[sidx] != 0ULL) : (read_csr_count(sub_id) > 0u);
                        break;
                    case NARRATIVE_PRED_PROP_IS_LOST:
                        met = (!pos_ids || sidx >= pos_cap || pos_ids[sidx] == INVALID_ID) &&
                              (!loc_ids || sidx >= loc_cap || loc_ids[sidx] == INVALID_ID);
                        break;
                    case NARRATIVE_PRED_PROP_HAS_POSSESSOR:
                        met = (pos_ids && sidx < pos_cap && pos_ids[sidx] != INVALID_ID);
                        break;
                    case NARRATIVE_PRED_LOCATION_IS_EMPTY:
                        if (goal_masks && sidx < gmsk_cap) {
                            met = (goal_masks[sidx] == 0ULL);
                        } else if (rcnt_raw && sidx < rcnt_cap) {
                            met = (read_csr_count(sub_id) == 0u);
                        } else if (loc_ids && loc_cap > 0) {
                            met = true;
                            for (int64_t n = 0; n < loc_cap; ++n) {
                                if (n != sidx && loc_ids[n] == sub_id) { met = false; break; }
                            }
                        }
                        break;
                    case NARRATIVE_PRED_LOCATION_HAS_CHARACTERS:
                    case NARRATIVE_PRED_LOCATION_HAS_PROPS: {
                        const uint8_t wanted_type = (op == NARRATIVE_PRED_LOCATION_HAS_CHARACTERS)
                            ? NARREME_TYPE_CHARACTER
                            : NARREME_TYPE_PROP;
                        if (loc_ids && loc_cap > 0) {
                            for (int64_t n = 0; n < loc_cap; ++n) {
                                if (n != sidx && loc_ids[n] == sub_id) {
                                    if (!ntyp_raw || read_narreme_type(static_cast<uint32_t>(n)) == wanted_type) {
                                        met = true;
                                        break;
                                    }
                                }
                            }
                        } else {
                            met = (read_csr_count(sub_id) > 0u);
                        }
                        break;
                    }
                    case NARRATIVE_PRED_INCIDENT_HAS_NOT_HAPPENED:
                    case NARRATIVE_PRED_INCIDENT_HAS_HAPPENED:
                    case NARRATIVE_PRED_INCIDENT_CANNOT_HAPPEN: {
                        const int32_t target_inc_st = static_cast<int32_t>(op - NARRATIVE_PRED_INCIDENT_HAS_NOT_HAPPENED);
                        met = (read_inc_or_plot_status(sub_id) == target_inc_st);
                        break;
                    }
                    case NARRATIVE_PRED_PLOT_NOT_BEGUN:
                    case NARRATIVE_PRED_PLOT_IN_PROGRESS:
                    case NARRATIVE_PRED_PLOT_COMPLETE:
                    case NARRATIVE_PRED_PLOT_CANNOT_COMPLETE:
                    case NARRATIVE_PRED_PLOT_FAILED: {
                        const int32_t target_plt_st = static_cast<int32_t>(op - NARRATIVE_PRED_PLOT_NOT_BEGUN);
                        met = (read_inc_or_plot_status(sub_id) == target_plt_st);
                        break;
                    }
                    default:
                        return COND_STATUS_UNKNOWN;
                }

                return met ? COND_STATUS_MET : COND_STATUS_NOT_MET;
            };

            auto eval_row = [&](int64_t idx) noexcept {
                if (filter_narrative_condition_type && ctyp_raw && idx < ctyp_cap) {
                    const uint8_t ctype = (ctyp_stride == sizeof(int32_t))
                        ? static_cast<uint8_t>(reinterpret_cast<const int32_t*>(ctyp_raw)[idx])
                        : ctyp_raw[idx];
                    if (ctype != CONDITION_TYPE_NARRATIVE) return;
                }

                const int8_t cur_st = read_status(idx);
                if (preserve_permanent_and_cannot_meet &&
                    (cur_st == COND_STATUS_CANNOT_MEET || cur_st == COND_STATUS_PERMANENT)) {
                    return;
                }

                const uint32_t sub_id = (subjects && idx < sub_cap) ? subjects[idx] : INVALID_ID;
                const uint32_t obj_id = (objects  && idx < obj_cap) ? objects[idx]  : INVALID_ID;
                uint16_t pred_op = default_predicate_op;
                if (pop_raw && idx < pop_cap) {
                    pred_op = (pop_stride == sizeof(uint32_t))
                        ? static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pop_raw)[idx])
                        : reinterpret_cast<const uint16_t*>(pop_raw)[idx];
                }

                const int8_t new_st = eval_predicate(sub_id, obj_id, pred_op);
                if (new_st != cur_st) {
                    write_status(idx, new_st);

                    if (trn_raw && idx < trn_cap) {
                        uint8_t tflag = COND_TRANSITION_NONE;
                        if (new_st == COND_STATUS_MET && cur_st == COND_STATUS_NOT_MET) {
                            tflag = COND_TRANSITION_MET;
                        } else if (new_st == COND_STATUS_NOT_MET && cur_st == COND_STATUS_MET) {
                            tflag = COND_TRANSITION_UNMET;
                        } else if (new_st == COND_STATUS_CANNOT_MEET && (cur_st == COND_STATUS_NOT_MET || cur_st == COND_STATUS_MET)) {
                            tflag = COND_TRANSITION_CANNOT_MEET;
                        }
                        if (tflag != COND_TRANSITION_NONE) {
                            if (trn_stride == sizeof(uint32_t)) {
                                reinterpret_cast<uint32_t*>(trn_raw)[idx] |= static_cast<uint32_t>(tflag);
                            } else {
                                trn_raw[idx] |= tflag;
                            }
                        }
                    }

                    if (wake_changed_conditions) {
                        p_context.queue_selection_command(cstat_part->buffer_id, idx);
                    }
                }
            };

            _iterate_selection(cstat_part->selection, cond_cap, eval_row);
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

static_assert(sizeof(NarrativeConditionEvaluateTransformLogic<>) == 64, "NarrativeConditionEvaluateTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core