#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../transform_logic/symbolic_reference_resolve_transform_logic.h"
#include "../transform_logic/incident_status_resolve_transform_logic.h"
#include "../transform_logic/plot_status_evaluate_transform_logic.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_CONDITION_BITMASK_QUERY_ENUMS_DEFINED
#define IDEAM_NARRATIVES_CONDITION_BITMASK_QUERY_ENUMS_DEFINED

/**
 * Target evaluation domains for NarremeConditionBitmaskQueryLogic (N12).
 */
enum NarremeConditionQueryDomain : uint8_t {
    COND_QUERY_DOMAIN_CONDITIONS = 0, // Evaluates ConditionEvaluationRowPOD or int8_t/int32_t condition_status against NarremeConditionStatusMask
    COND_QUERY_DOMAIN_INCIDENTS  = 1, // Evaluates Incident status + prerequisite/causal condition slices (e.g. waking Incidents whose prereqs are MET)
    COND_QUERY_DOMAIN_PLOTS      = 2  // Evaluates Plot status + prerequisite condition slices
};

/**
 * Bitmask values for NarremeConditionStatusDOD (-2..2), shifted by +2 (`1u << (status + 2)`):
 * - COND_STATUS_CANNOT_MEET (-2) -> bit 0 (1 << 0 = 0x01)
 * - COND_STATUS_NOT_MET     (-1) -> bit 1 (1 << 1 = 0x02)
 * - COND_STATUS_UNKNOWN     ( 0) -> bit 2 (1 << 2 = 0x04)
 * - COND_STATUS_MET         ( 1) -> bit 3 (1 << 3 = 0x08)
 * - COND_STATUS_PERMANENT   ( 2) -> bit 4 (1 << 4 = 0x10)
 */
enum NarremeConditionStatusMask : uint8_t {
    COND_STATUS_MASK_NONE          = 0u,
    COND_STATUS_MASK_CANNOT_MEET   = 1u << 0, // -2
    COND_STATUS_MASK_NOT_MET       = 1u << 1, // -1
    COND_STATUS_MASK_UNKNOWN       = 1u << 2, //  0
    COND_STATUS_MASK_MET           = 1u << 3, //  1
    COND_STATUS_MASK_PERMANENT     = 1u << 4, //  2
    COND_STATUS_MASK_SATISFIED     = COND_STATUS_MASK_MET | COND_STATUS_MASK_PERMANENT,
    COND_STATUS_MASK_UNSATISFIED   = COND_STATUS_MASK_CANNOT_MEET | COND_STATUS_MASK_NOT_MET | COND_STATUS_MASK_UNKNOWN,
    COND_STATUS_MASK_ALL           = 0x1Fu
};

/**
 * Bitmask values for IncidentStatusDOD (0..2) (`1u << status`):
 * - INCIDENT_STATUS_HAVE_NOT_OCCURRED (0) -> bit 0 (0x01)
 * - INCIDENT_STATUS_ONGOING           (1) -> bit 1 (0x02)
 * - INCIDENT_STATUS_COMPLETE          (2) -> bit 2 (0x04)
 */
enum IncidentStatusMaskDOD : uint8_t {
    INCIDENT_STATUS_MASK_NONE              = 0u,
    INCIDENT_STATUS_MASK_HAVE_NOT_OCCURRED = 1u << 0,
    INCIDENT_STATUS_MASK_ONGOING           = 1u << 1,
    INCIDENT_STATUS_MASK_COMPLETE          = 1u << 2,
    INCIDENT_STATUS_MASK_ACTIVE            = INCIDENT_STATUS_MASK_HAVE_NOT_OCCURRED | INCIDENT_STATUS_MASK_ONGOING,
    INCIDENT_STATUS_MASK_ALL               = 0x07u
};

/**
 * Bitmask values for PlotStatusDOD (0..4) (`1u << status`):
 * - PLOT_STATUS_UNAVAILABLE (0) -> bit 0 (0x01)
 * - PLOT_STATUS_AVAILABLE   (1) -> bit 1 (0x02)
 * - PLOT_STATUS_IN_PROGRESS (2) -> bit 2 (0x04)
 * - PLOT_STATUS_SUCCESS     (3) -> bit 3 (0x08)
 * - PLOT_STATUS_FAILURE     (4) -> bit 4 (0x10)
 */
enum PlotStatusMaskDOD : uint8_t {
    PLOT_STATUS_MASK_NONE        = 0u,
    PLOT_STATUS_MASK_UNAVAILABLE = 1u << 0,
    PLOT_STATUS_MASK_AVAILABLE   = 1u << 1,
    PLOT_STATUS_MASK_IN_PROGRESS = 1u << 2,
    PLOT_STATUS_MASK_SUCCESS     = 1u << 3,
    PLOT_STATUS_MASK_FAILURE     = 1u << 4,
    PLOT_STATUS_MASK_OPEN        = PLOT_STATUS_MASK_AVAILABLE | PLOT_STATUS_MASK_IN_PROGRESS,
    PLOT_STATUS_MASK_TERMINAL    = PLOT_STATUS_MASK_SUCCESS | PLOT_STATUS_MASK_FAILURE,
    PLOT_STATUS_MASK_ALL         = 0x1Fu
};

/**
 * Slice reduction modes when evaluating prerequisite/causal condition slices for Incidents or Plots.
 */
enum ConditionSliceMatchMode : uint8_t {
    SLICE_MATCH_IGNORE                    = 0, // Do not inspect prerequisite/causal condition slices (status mask only)
    SLICE_MATCH_ALL_IN_MASK               = 1, // Every condition in the slice must match required_condition_status_mask
    SLICE_MATCH_ANY_IN_MASK               = 2, // At least one condition in the slice must match required_condition_status_mask
    SLICE_MATCH_ALL_MET_AT_LEAST_1_DIRTY  = 3  // All conditions in slice are MET/PERMANENT AND at least one has dirty/transition flag set
};

#endif // IDEAM_NARRATIVES_CONDITION_BITMASK_QUERY_ENUMS_DEFINED

/**
 * NarremeConditionBitmaskQueryLogic<T> (N12)
 * Filters (QueryOp::CULL / QueryOp::ADD) Conditions, Incidents, or Plots matching a target
 * NarremeConditionStatus / IncidentStatus / PlotStatus bitmask and optional prerequisite
 * condition slice reduction (e.g., waking only HAVE_NOT_OCCURRED Incidents whose prerequisites
 * and causal conditions just turned MET).
 */
template <typename T = int8_t>
struct alignas(64) NarremeConditionBitmaskQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::BYTE | DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Narreme Condition Bitmask";
    static constexpr std::string_view ui_category  = "Conditions";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                = INVALID_ID;                      // 4B: Primary selection buffer (Condition, Incident, or Plot)
    uint32_t status_buffer_id                = INVALID_ID;                      // 4B: Optional status column (int8_t/int32_t or ConditionEvaluationRowPOD)
    uint32_t condition_row_buffer_id         = INVALID_ID;                      // 4B: ConditionEvaluationRowPOD AoS or condition status buffer
    uint32_t prereq_cond_offset_buffer_id    = INVALID_ID;                      // 4B: Optional Incident/Plot prereq_cond_offset (uint32_t)
    uint32_t prereq_cond_count_buffer_id     = INVALID_ID;                      // 4B: Optional Incident/Plot prereq_cond_count (uint16_t/uint32_t)
    uint32_t causal_cond_offset_buffer_id    = INVALID_ID;                      // 4B: Optional Incident causal_cond_offset (uint32_t)
    uint32_t causal_cond_count_buffer_id     = INVALID_ID;                      // 4B: Optional Incident causal_cond_count (uint16_t/uint32_t)
    uint32_t condition_index_table_buffer_id = INVALID_ID;                      // 4B: Optional uint32_t indirection table into ConditionTableBuffer
    uint32_t transition_flags_buffer_id      = INVALID_ID;                      // 4B: Optional uint8_t transition/dirty flags buffer
    uint32_t unresolved_symbolic_count_buf_id= INVALID_ID;                      // 4B: Optional uint16_t/uint32_t unresolved symbolic count buffer

    uint8_t query_domain                     = COND_QUERY_DOMAIN_CONDITIONS;    // 1B: NarremeConditionQueryDomain (0=Conditions, 1=Incidents, 2=Plots)
    uint8_t required_condition_status_mask   = COND_STATUS_MASK_SATISFIED;      // 1B: NarremeConditionStatusMask (bits 0..4 for statuses -2..2)
    uint8_t required_incident_status_mask    = INCIDENT_STATUS_MASK_HAVE_NOT_OCCURRED; // 1B: IncidentStatusMaskDOD (bits 0..2)
    uint8_t required_plot_status_mask        = PLOT_STATUS_MASK_OPEN;           // 1B: PlotStatusMaskDOD (bits 0..4)
    uint8_t slice_match_mode                 = SLICE_MATCH_ALL_IN_MASK;         // 1B: ConditionSliceMatchMode for Incident/Plot prereq slices
    uint8_t required_transition_mask         = 0u;                              // 1B: Required transition flags bitmask (0 = any)
    uint8_t condition_type_filter_mask       = 0x07u;                           // 1B: Bitmask of NarrativeConditionType (bit0=Narrative, bit1=Causal, bit2=Gameplay)
    bool require_zero_unresolved_symbolics   = false;                           // 1B: Reject rows with unresolved_symbolic_count > 0
    bool allow_empty_condition_slice         = false;                           // 1B: Whether 0 conditions in a slice counts as matching
    bool negate_result                       = false;                           // 1B: Invert final predicate evaluation
    uint8_t reserved_padding[14]             = {0};                             // 14B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary st_prop;
        st_prop["name"] = "status_buffer_id";
        st_prop["type"] = godot::Variant::INT;
        st_prop["hint_string"] = "buffer_option";
        props.push_back(st_prop);

        godot::Dictionary crow_prop;
        crow_prop["name"] = "condition_row_buffer_id";
        crow_prop["type"] = godot::Variant::INT;
        crow_prop["hint_string"] = "buffer_option";
        props.push_back(crow_prop);

        godot::Dictionary poff_prop;
        poff_prop["name"] = "prereq_cond_offset_buffer_id";
        poff_prop["type"] = godot::Variant::INT;
        poff_prop["hint_string"] = "buffer_option";
        props.push_back(poff_prop);

        godot::Dictionary pcnt_prop;
        pcnt_prop["name"] = "prereq_cond_count_buffer_id";
        pcnt_prop["type"] = godot::Variant::INT;
        pcnt_prop["hint_string"] = "buffer_option";
        props.push_back(pcnt_prop);

        godot::Dictionary coff_prop;
        coff_prop["name"] = "causal_cond_offset_buffer_id";
        coff_prop["type"] = godot::Variant::INT;
        coff_prop["hint_string"] = "buffer_option";
        props.push_back(coff_prop);

        godot::Dictionary ccnt_prop;
        ccnt_prop["name"] = "causal_cond_count_buffer_id";
        ccnt_prop["type"] = godot::Variant::INT;
        ccnt_prop["hint_string"] = "buffer_option";
        props.push_back(ccnt_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "condition_index_table_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary trn_prop;
        trn_prop["name"] = "transition_flags_buffer_id";
        trn_prop["type"] = godot::Variant::INT;
        trn_prop["hint_string"] = "buffer_option";
        props.push_back(trn_prop);

        godot::Dictionary sym_prop;
        sym_prop["name"] = "unresolved_symbolic_count_buf_id";
        sym_prop["type"] = godot::Variant::INT;
        sym_prop["hint_string"] = "buffer_option";
        props.push_back(sym_prop);

        godot::Dictionary dom_prop;
        dom_prop["name"] = "query_domain";
        dom_prop["type"] = godot::Variant::INT;
        dom_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dom_prop["hint_string"] = "Conditions:0,Incidents:1,Plots:2";
        props.push_back(dom_prop);

        godot::Dictionary cmsk_prop;
        cmsk_prop["name"] = "required_condition_status_mask";
        cmsk_prop["type"] = godot::Variant::INT;
        cmsk_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        cmsk_prop["hint_string"] = "Cannot Meet (-2),Not Met (-1),Unknown (0),Met (1),Permanent (2)";
        props.push_back(cmsk_prop);

        godot::Dictionary imsk_prop;
        imsk_prop["name"] = "required_incident_status_mask";
        imsk_prop["type"] = godot::Variant::INT;
        imsk_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        imsk_prop["hint_string"] = "Have Not Occurred (0),Ongoing (1),Complete (2)";
        props.push_back(imsk_prop);

        godot::Dictionary pmsk_prop;
        pmsk_prop["name"] = "required_plot_status_mask";
        pmsk_prop["type"] = godot::Variant::INT;
        pmsk_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        pmsk_prop["hint_string"] = "Unavailable (0),Available (1),In Progress (2),Success (3),Failure (4)";
        props.push_back(pmsk_prop);

        godot::Dictionary sm_prop;
        sm_prop["name"] = "slice_match_mode";
        sm_prop["type"] = godot::Variant::INT;
        sm_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        sm_prop["hint_string"] = "Ignore Slices:0,All Conditions In Mask:1,Any Condition In Mask:2,All Met And At Least 1 Dirty:3";
        props.push_back(sm_prop);

        godot::Dictionary tmsk_prop;
        tmsk_prop["name"] = "required_transition_mask";
        tmsk_prop["type"] = godot::Variant::INT;
        tmsk_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(tmsk_prop);

        godot::Dictionary ctyp_prop;
        ctyp_prop["name"] = "condition_type_filter_mask";
        ctyp_prop["type"] = godot::Variant::INT;
        ctyp_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        ctyp_prop["hint_string"] = "Narrative Condition,Causal Condition,Gameplay Condition";
        props.push_back(ctyp_prop);

        godot::Dictionary rsym_prop;
        rsym_prop["name"] = "require_zero_unresolved_symbolics";
        rsym_prop["type"] = godot::Variant::BOOL;
        rsym_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rsym_prop);

        godot::Dictionary emp_prop;
        emp_prop["name"] = "allow_empty_condition_slice";
        emp_prop["type"] = godot::Variant::BOOL;
        emp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(emp_prop);

        godot::Dictionary neg_prop;
        neg_prop["name"] = "negate_result";
        neg_prop["type"] = godot::Variant::BOOL;
        neg_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(neg_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)        return target_buffer_id;
        if (status_buffer_id != INVALID_ID)        return status_buffer_id;
        if (condition_row_buffer_id != INVALID_ID) return condition_row_buffer_id;
        return transition_flags_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("status_buffer_id")) {
            status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["status_buffer_id"]));
        }
        if (p_props.has("condition_row_buffer_id")) {
            condition_row_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_row_buffer_id"]));
        }
        if (p_props.has("prereq_cond_offset_buffer_id")) {
            prereq_cond_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["prereq_cond_offset_buffer_id"]));
        }
        if (p_props.has("prereq_cond_count_buffer_id")) {
            prereq_cond_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["prereq_cond_count_buffer_id"]));
        }
        if (p_props.has("causal_cond_offset_buffer_id")) {
            causal_cond_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["causal_cond_offset_buffer_id"]));
        }
        if (p_props.has("causal_cond_count_buffer_id")) {
            causal_cond_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["causal_cond_count_buffer_id"]));
        }
        if (p_props.has("condition_index_table_buffer_id")) {
            condition_index_table_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_index_table_buffer_id"]));
        }
        if (p_props.has("transition_flags_buffer_id")) {
            transition_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transition_flags_buffer_id"]));
        }
        if (p_props.has("unresolved_symbolic_count_buf_id")) {
            unresolved_symbolic_count_buf_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["unresolved_symbolic_count_buf_id"]));
        }
        if (p_props.has("query_domain")) {
            query_domain = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["query_domain"]), 0, 2));
        }
        if (p_props.has("required_condition_status_mask")) {
            required_condition_status_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["required_condition_status_mask"]) & 0x1F);
        }
        if (p_props.has("required_incident_status_mask")) {
            required_incident_status_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["required_incident_status_mask"]) & 0x07);
        }
        if (p_props.has("required_plot_status_mask")) {
            required_plot_status_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["required_plot_status_mask"]) & 0x1F);
        }
        if (p_props.has("slice_match_mode")) {
            slice_match_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["slice_match_mode"]), 0, 3));
        }
        if (p_props.has("required_transition_mask")) {
            required_transition_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["required_transition_mask"]));
        }
        if (p_props.has("condition_type_filter_mask")) {
            condition_type_filter_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["condition_type_filter_mask"]) & 0x07);
        }
        if (p_props.has("require_zero_unresolved_symbolics")) {
            require_zero_unresolved_symbolics = static_cast<bool>(p_props["require_zero_unresolved_symbolics"]);
        }
        if (p_props.has("allow_empty_condition_slice")) {
            allow_empty_condition_slice = static_cast<bool>(p_props["allow_empty_condition_slice"]);
        }
        if (p_props.has("negate_result")) {
            negate_result = static_cast<bool>(p_props["negate_result"]);
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
        if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t> || std::is_same_v<T, int32_t>) {
            const uint32_t tgt_id = get_target_buffer_id();
            const uint32_t st_id  = (status_buffer_id != INVALID_ID) ? status_buffer_id : tgt_id;

            const GrantPartPOD* st_part   = (st_id != INVALID_ID)                            ? p_context.get_grant_part(st_id)                            : nullptr;
            const GrantPartPOD* crow_part = (condition_row_buffer_id != INVALID_ID)          ? p_context.get_grant_part(condition_row_buffer_id)          : st_part;
            const GrantPartPOD* poff_part = (prereq_cond_offset_buffer_id != INVALID_ID)     ? p_context.get_grant_part(prereq_cond_offset_buffer_id)     : nullptr;
            const GrantPartPOD* pcnt_part = (prereq_cond_count_buffer_id != INVALID_ID)      ? p_context.get_grant_part(prereq_cond_count_buffer_id)      : nullptr;
            const GrantPartPOD* coff_part = (causal_cond_offset_buffer_id != INVALID_ID)     ? p_context.get_grant_part(causal_cond_offset_buffer_id)     : nullptr;
            const GrantPartPOD* ccnt_part = (causal_cond_count_buffer_id != INVALID_ID)      ? p_context.get_grant_part(causal_cond_count_buffer_id)      : nullptr;
            const GrantPartPOD* idx_part  = (condition_index_table_buffer_id != INVALID_ID)  ? p_context.get_grant_part(condition_index_table_buffer_id)  : nullptr;
            const GrantPartPOD* trn_part  = (transition_flags_buffer_id != INVALID_ID)       ? p_context.get_grant_part(transition_flags_buffer_id)       : nullptr;
            const GrantPartPOD* sym_part  = (unresolved_symbolic_count_buf_id != INVALID_ID) ? p_context.get_grant_part(unresolved_symbolic_count_buf_id) : nullptr;

            const uint8_t* __restrict st_raw = (st_part && st_part->raw_base_ptr) ? st_part->raw_base_ptr : nullptr;
            const uint32_t st_stride         = st_part ? st_part->element_stride : 0u;
            const int64_t st_cap             = (st_part && st_stride > 0u) ? static_cast<int64_t>(st_part->capacity_bytes / st_stride) : 0;

            const uint8_t* __restrict crow_raw = (crow_part && crow_part->raw_base_ptr) ? crow_part->raw_base_ptr : nullptr;
            const uint32_t crow_stride         = crow_part ? crow_part->element_stride : 0u;
            const int64_t crow_cap             = (crow_part && crow_stride > 0u) ? static_cast<int64_t>(crow_part->capacity_bytes / crow_stride) : 0;

            const uint32_t* __restrict p_offsets = (poff_part && poff_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(poff_part->raw_base_ptr)
                : nullptr;
            const int64_t poff_cap = p_offsets ? static_cast<int64_t>(poff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict pcnt_raw = (pcnt_part && pcnt_part->raw_base_ptr) ? pcnt_part->raw_base_ptr : nullptr;
            const uint32_t pcnt_stride         = pcnt_part ? pcnt_part->element_stride : 0u;
            const int64_t pcnt_cap             = (pcnt_part && pcnt_stride > 0u) ? static_cast<int64_t>(pcnt_part->capacity_bytes / pcnt_stride) : 0;

            const uint32_t* __restrict c_offsets = (coff_part && coff_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(coff_part->raw_base_ptr)
                : nullptr;
            const int64_t coff_cap = c_offsets ? static_cast<int64_t>(coff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict ccnt_raw = (ccnt_part && ccnt_part->raw_base_ptr) ? ccnt_part->raw_base_ptr : nullptr;
            const uint32_t ccnt_stride         = ccnt_part ? ccnt_part->element_stride : 0u;
            const int64_t ccnt_cap             = (ccnt_part && ccnt_stride > 0u) ? static_cast<int64_t>(ccnt_part->capacity_bytes / ccnt_stride) : 0;

            const uint32_t* __restrict cond_indices = (idx_part && idx_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(idx_part->raw_base_ptr)
                : nullptr;
            const int64_t idx_cap = cond_indices ? static_cast<int64_t>(idx_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict trn_flags = (trn_part && trn_part->raw_base_ptr)
                ? reinterpret_cast<const uint8_t*>(trn_part->raw_base_ptr)
                : nullptr;
            const int64_t trn_cap = trn_flags ? static_cast<int64_t>(trn_part->capacity_bytes / sizeof(uint8_t)) : 0;

            const uint8_t* __restrict sym_raw = (sym_part && sym_part->raw_base_ptr) ? sym_part->raw_base_ptr : nullptr;
            const uint32_t sym_stride         = sym_part ? sym_part->element_stride : 0u;
            const int64_t sym_cap             = (sym_part && sym_stride > 0u) ? static_cast<int64_t>(sym_part->capacity_bytes / sym_stride) : 0;

            auto read_count = [](const uint8_t* raw, uint32_t stride, int64_t cap, int64_t idx) noexcept -> uint32_t {
                if (!raw || idx < 0 || idx >= cap) return 0u;
                if (stride == sizeof(uint32_t)) return reinterpret_cast<const uint32_t*>(raw)[idx];
                if (stride == sizeof(uint16_t)) return static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(raw)[idx]);
                return static_cast<uint32_t>(raw[idx]);
            };

            auto cond_status_to_bit = [](int8_t st) noexcept -> uint8_t {
                const int32_t shifted = static_cast<int32_t>(st) + 2;
                if (shifted < 0 || shifted > 4) return 0u;
                return static_cast<uint8_t>(1u << shifted);
            };

            auto read_cond_entry = [&](uint32_t slot, int8_t& r_status, uint8_t& r_dirty, uint8_t& r_type) noexcept -> bool {
                uint32_t cid = slot;
                if (cond_indices) {
                    if (static_cast<int64_t>(slot) >= idx_cap) return false;
                    cid = cond_indices[slot];
                }
                if (!crow_raw || cid == INVALID_ID || static_cast<int64_t>(cid) >= crow_cap) return false;
                if (crow_stride == sizeof(ConditionEvaluationRowPOD)) {
                    const auto& row = reinterpret_cast<const ConditionEvaluationRowPOD*>(crow_raw)[cid];
                    r_status = row.condition_status;
                    r_dirty  = row.status_dirty_flag;
                    r_type   = row.condition_type;
                } else if (crow_stride == sizeof(int32_t)) {
                    r_status = static_cast<int8_t>(reinterpret_cast<const int32_t*>(crow_raw)[cid]);
                    r_dirty  = (trn_flags && static_cast<int64_t>(cid) < trn_cap) ? trn_flags[cid] : 0u;
                    r_type   = CONDITION_TYPE_NARRATIVE;
                } else {
                    r_status = reinterpret_cast<const int8_t*>(crow_raw)[cid];
                    r_dirty  = (trn_flags && static_cast<int64_t>(cid) < trn_cap) ? trn_flags[cid] : 0u;
                    r_type   = CONDITION_TYPE_NARRATIVE;
                }
                if (trn_flags && static_cast<int64_t>(cid) < trn_cap) {
                    r_dirty |= trn_flags[cid];
                }
                return true;
            };

            auto eval_row = [&](int64_t idx) noexcept -> bool {
                bool result = false;

                if (require_zero_unresolved_symbolics && sym_raw && idx >= 0 && idx < sym_cap) {
                    if (read_count(sym_raw, sym_stride, sym_cap, idx) > 0u) {
                        return negate_result ? true : false;
                    }
                }

                if (query_domain == COND_QUERY_DOMAIN_CONDITIONS) {
                    int8_t cst   = COND_STATUS_UNKNOWN;
                    uint8_t cflg = 0u;
                    uint8_t ctyp = CONDITION_TYPE_NARRATIVE;
                    if (crow_raw && idx >= 0 && idx < crow_cap) {
                        if (crow_stride == sizeof(ConditionEvaluationRowPOD)) {
                            const auto& row = reinterpret_cast<const ConditionEvaluationRowPOD*>(crow_raw)[idx];
                            cst  = row.condition_status;
                            cflg = row.status_dirty_flag;
                            ctyp = row.condition_type;
                        } else if (crow_stride == sizeof(int32_t)) {
                            cst = static_cast<int8_t>(reinterpret_cast<const int32_t*>(crow_raw)[idx]);
                        } else {
                            cst = reinterpret_cast<const int8_t*>(crow_raw)[idx];
                        }
                    } else if (st_raw && idx >= 0 && idx < st_cap) {
                        cst = (st_stride == sizeof(int32_t))
                            ? static_cast<int8_t>(reinterpret_cast<const int32_t*>(st_raw)[idx])
                            : reinterpret_cast<const int8_t*>(st_raw)[idx];
                    } else {
                        return negate_result ? true : false;
                    }

                    if (trn_flags && idx >= 0 && idx < trn_cap) {
                        cflg |= trn_flags[idx];
                    }

                    const bool type_ok = ((condition_type_filter_mask & (1u << (ctyp & 7u))) != 0u);
                    const bool mask_ok = ((cond_status_to_bit(cst) & required_condition_status_mask) != 0u);
                    const bool trn_ok  = (required_transition_mask == 0u) || ((cflg & required_transition_mask) != 0u);
                    result = type_ok && mask_ok && trn_ok;
                } else {
                    int8_t primary_st = 0;
                    if (st_raw && idx >= 0 && idx < st_cap) {
                        primary_st = (st_stride == sizeof(int32_t))
                            ? static_cast<int8_t>(reinterpret_cast<const int32_t*>(st_raw)[idx])
                            : reinterpret_cast<const int8_t*>(st_raw)[idx];
                    }

                    const uint8_t req_primary_mask = (query_domain == COND_QUERY_DOMAIN_INCIDENTS)
                        ? required_incident_status_mask
                        : required_plot_status_mask;
                    const uint8_t primary_bit = (primary_st >= 0 && primary_st < 7)
                        ? static_cast<uint8_t>(1u << primary_st)
                        : 0u;
                    bool primary_ok = (req_primary_mask == 0u) || ((primary_bit & req_primary_mask) != 0u);

                    if (primary_ok && required_transition_mask != 0u && trn_flags && idx >= 0 && idx < trn_cap) {
                        primary_ok = (trn_flags[idx] & required_transition_mask) != 0u;
                    }

                    if (!primary_ok) {
                        result = false;
                    } else if (slice_match_mode == SLICE_MATCH_IGNORE || (!p_offsets && !c_offsets)) {
                        result = true;
                    } else {
                        const uint32_t p_off = (p_offsets && idx >= 0 && idx < poff_cap) ? p_offsets[idx] : 0u;
                        const uint32_t p_cnt = read_count(pcnt_raw, pcnt_stride, pcnt_cap, idx);
                        const uint32_t c_off = (c_offsets && idx >= 0 && idx < coff_cap) ? c_offsets[idx] : 0u;
                        const uint32_t c_cnt = read_count(ccnt_raw, ccnt_stride, ccnt_cap, idx);
                        const uint32_t total = p_cnt + c_cnt;

                        if (total == 0u) {
                            result = allow_empty_condition_slice;
                        } else {
                            bool all_match = true;
                            bool any_match = false;
                            bool any_dirty = false;

                            auto check_slice = [&](uint32_t off, uint32_t cnt) noexcept {
                                for (uint32_t k = 0u; k < cnt; ++k) {
                                    int8_t cst   = COND_STATUS_UNKNOWN;
                                    uint8_t cflg = 0u;
                                    uint8_t ctyp = 0u;
                                    if (!read_cond_entry(off + k, cst, cflg, ctyp)) {
                                        all_match = false;
                                        continue;
                                    }
                                    const uint8_t target_mask = (slice_match_mode == SLICE_MATCH_ALL_MET_AT_LEAST_1_DIRTY)
                                        ? static_cast<uint8_t>(COND_STATUS_MASK_SATISFIED)
                                        : required_condition_status_mask;
                                    const bool m = ((cond_status_to_bit(cst) & target_mask) != 0u);
                                    if (m) any_match = true;
                                    else   all_match = false;
                                    if (cflg != 0u) any_dirty = true;
                                }
                            };

                            check_slice(p_off, p_cnt);
                            check_slice(c_off, c_cnt);

                            if (slice_match_mode == SLICE_MATCH_ALL_IN_MASK) {
                                result = all_match;
                            } else if (slice_match_mode == SLICE_MATCH_ANY_IN_MASK) {
                                result = any_match;
                            } else {
                                result = all_match && any_dirty;
                            }
                        }
                    }
                }

                return negate_result ? !result : result;
            };

            _apply_query_op<Op>(r_selection, p_context, tgt_id, eval_row);
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
                const int64_t words       = (r_selection.capacity + 63) >> 6;
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

static_assert(sizeof(NarremeConditionBitmaskQueryLogic<>) == 64, "NarremeConditionBitmaskQueryLogic must be exactly 64 bytes!");

} // namespace ideam::core