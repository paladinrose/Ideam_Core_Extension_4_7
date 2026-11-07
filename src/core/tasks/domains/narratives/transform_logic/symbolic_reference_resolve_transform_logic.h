#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "narreme_symbolic_similarity_transform_logic.h"
#include "role_best_fit_assignment_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_CONDITION_TABLE_POD_DEFINED
#define IDEAM_NARRATIVES_CONDITION_TABLE_POD_DEFINED

/**
 * DOD NarremeConditionStatus values (mirrors ideam::godot_ext::NarremeConditionStatus in narreme.h).
 * Stored in ConditionEvaluationTableBuffer::current_status and expected_status (int8_t).
 */
enum NarremeConditionStatusDOD : int8_t {
    COND_STATUS_CANNOT_MEET = -2,
    COND_STATUS_NOT_MET     = -1,
    COND_STATUS_UNKNOWN     = 0,
    COND_STATUS_MET         = 1,
    COND_STATUS_PERMANENT   = 2
};

/**
 * Condition archetype identifiers in ConditionEvaluationTableBuffer::condition_type (uint8_t).
 */
enum NarrativeConditionType : uint8_t {
    CONDITION_TYPE_NARRATIVE = 0, // Narrative_Condition (atomic subject-predicate-object check)
    CONDITION_TYPE_CAUSAL    = 1, // Causal_Condition (hierarchical child conditions + causes tree)
    CONDITION_TYPE_GAMEPLAY  = 2  // Gameplay_Condition (external gameplay property / trigger / event state)
};

/**
 * RelationshipEdgePOD (16-Byte Aligned)
 * Compressed Sparse Row (CSR) edge entry for NarrativeRelationshipCSRBuffer (Section 3.2 Buffer 3).
 * Replaces per-Character TypedArray<Relationship> heap objects with contiguous BridgeView slices.
 */
struct alignas(16) RelationshipEdgePOD {
    uint32_t target_narreme_id        = INVALID_ID; // 4B: Target Narreme ID (or Symbolic ID prior to resolution)
    uint32_t relationship_title_token = 0u;         // 4B: Interned StringName token for Relationship::title
    float affinity_weight             = 1.0f;       // 4B: Relationship strength / affinity weight
    uint32_t flags                    = 0u;         // 4B: Bit 0 = target_is_symbolic, Bits [7:1] = NarremeType -> Total = 16B
};

static_assert(sizeof(RelationshipEdgePOD) == 16, "RelationshipEdgePOD must be 16 bytes!");

/**
 * ConditionEvaluationRowPOD (24-Byte Packed / 32-Byte Aligned)
 * Convenience descriptor for ConditionEvaluationTableBuffer (Section 3.2 Buffer 4).
 */
struct alignas(32) ConditionEvaluationRowPOD {
    uint32_t condition_id        = INVALID_ID;              // 4B: Unique condition row ID
    uint32_t subject_id          = INVALID_ID;              // 4B: Subject Narreme ID (or Symbolic ID)
    uint32_t object_id           = INVALID_ID;              // 4B: Object Narreme ID (or Symbolic ID)
    uint32_t resolved_subject_id = INVALID_ID;              // 4B: Resolved Literal subject ID
    uint32_t resolved_object_id  = INVALID_ID;              // 4B: Resolved Literal object ID
    uint16_t predicate_op        = 0u;                      // 2B: Atomic condition predicate ID
    uint8_t condition_type       = CONDITION_TYPE_NARRATIVE;// 1B: NarrativeConditionType
    int8_t expected_status       = COND_STATUS_MET;         // 1B: Expected NarremeConditionStatusDOD
    int8_t current_status        = COND_STATUS_UNKNOWN;     // 1B: Evaluated NarremeConditionStatusDOD
    uint8_t subject_is_symbolic  = 0u;                      // 1B: Non-zero when subject_id is a Symbolic slot
    uint8_t object_is_symbolic   = 0u;                      // 1B: Non-zero when object_id is a Symbolic slot
    uint8_t reserved_pad[5]      = {0};                     // 5B -> Total = 32B
};

static_assert(sizeof(ConditionEvaluationRowPOD) == 32, "ConditionEvaluationRowPOD must be 32 bytes!");

#endif // IDEAM_NARRATIVES_CONDITION_TABLE_POD_DEFINED

/**
 * SymbolicReferenceResolveTransformLogic<T> (N3)
 * Substitutes symbolic subject_id / object_id handles in ConditionEvaluationTableBuffer
 * (and optional NarrativeRelationshipCSRBuffer / PlotEventPOD slices) with their currently bound
 * literal narreme_ids from SymbolicAssignmentMapBuffer prior to condition evaluation.
 *
 * Implements NarrativeReplacement policies (REPLACE_SYMBOLIC, REPLACE_LITERAL, REPLACE_THOROUGH)
 * from Narrative::fill_role(), fill_symbol(), fill_place(), fill_moment(), and fill_story()
 * in a single branch-minimal linear pass without scanning nested TypedArrays.
 */
template <typename T = int32_t>
struct alignas(64) SymbolicReferenceResolveTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Symbolic Reference Resolve";
    static constexpr std::string_view ui_category  = "Casting";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t subject_id_buffer_id           = INVALID_ID;                  // 4B: ConditionEvaluationTableBuffer::subject_id (uint32_t/int32_t)
    uint32_t object_id_buffer_id            = INVALID_ID;                  // 4B: ConditionEvaluationTableBuffer::object_id (uint32_t/int32_t)
    uint32_t resolved_subject_id_buffer_id  = INVALID_ID;                  // 4B: Optional shadow resolved_subject_id (in-place if INVALID_ID)
    uint32_t resolved_object_id_buffer_id   = INVALID_ID;                  // 4B: Optional shadow resolved_object_id (in-place if INVALID_ID)
    uint32_t subject_is_symbolic_buffer_id  = INVALID_ID;                  // 4B: ConditionEvaluationTableBuffer::subject_is_symbolic (uint8_t)
    uint32_t object_is_symbolic_buffer_id   = INVALID_ID;                  // 4B: ConditionEvaluationTableBuffer::object_is_symbolic (uint8_t)
    uint32_t condition_status_buffer_id     = INVALID_ID;                  // 4B: Optional ConditionEvaluationTableBuffer::current_status (int8_t)
    uint32_t symbolic_to_literal_buffer_id  = INVALID_ID;                  // 4B: SymbolicAssignmentMapBuffer::symbolic_to_literal (int32_t)
    uint32_t previous_literal_buffer_id     = INVALID_ID;                  // 4B: Optional outgoing literal_id column for REPLACE_LITERAL
    uint32_t assignment_replace_buffer_id   = INVALID_ID;                  // 4B: Optional per-slot NarrativeReplacement policy column
    uint32_t relationship_edge_pod_buffer_id= INVALID_ID;                  // 4B: Optional RelationshipEdgePOD CSR buffer to resolve relations
    uint32_t symbolic_title_token_buffer_id = INVALID_ID;                  // 4B: Optional uint32_t official_name token per Symbolic slot

    uint8_t replacement_policy              = NARRATIVE_REPLACE_THOROUGH;  // 1B: NarrativeReplacementPolicy fallback
    int8_t unresolved_status_fallback       = COND_STATUS_CANNOT_MEET;     // 1B: Status written when a required symbolic slot is vacant
    bool clear_symbolic_flag_in_place       = false;                       // 1B: Clear subject/object_is_symbolic when resolving in-place
    bool update_status_on_unresolved        = true;                        // 1B: Write unresolved_status_fallback when bound literal is < 0
    bool reset_status_to_unknown_on_rebind  = true;                        // 1B: Reset current_status to UNKNOWN when subject/object rebinds
    bool wake_rebound_conditions            = true;                        // 1B: Queue rebound condition indices into wave_commands
    uint8_t reserved_padding[10]            = {0};                         // 10B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

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

        godot::Dictionary rsub_prop;
        rsub_prop["name"] = "resolved_subject_id_buffer_id";
        rsub_prop["type"] = godot::Variant::INT;
        rsub_prop["hint_string"] = "buffer_option";
        props.push_back(rsub_prop);

        godot::Dictionary robj_prop;
        robj_prop["name"] = "resolved_object_id_buffer_id";
        robj_prop["type"] = godot::Variant::INT;
        robj_prop["hint_string"] = "buffer_option";
        props.push_back(robj_prop);

        godot::Dictionary ssym_prop;
        ssym_prop["name"] = "subject_is_symbolic_buffer_id";
        ssym_prop["type"] = godot::Variant::INT;
        ssym_prop["hint_string"] = "buffer_option";
        props.push_back(ssym_prop);

        godot::Dictionary osym_prop;
        osym_prop["name"] = "object_is_symbolic_buffer_id";
        osym_prop["type"] = godot::Variant::INT;
        osym_prop["hint_string"] = "buffer_option";
        props.push_back(osym_prop);

        godot::Dictionary cstat_prop;
        cstat_prop["name"] = "condition_status_buffer_id";
        cstat_prop["type"] = godot::Variant::INT;
        cstat_prop["hint_string"] = "buffer_option";
        props.push_back(cstat_prop);

        godot::Dictionary map_prop;
        map_prop["name"] = "symbolic_to_literal_buffer_id";
        map_prop["type"] = godot::Variant::INT;
        map_prop["hint_string"] = "buffer_option";
        props.push_back(map_prop);

        godot::Dictionary prev_prop;
        prev_prop["name"] = "previous_literal_buffer_id";
        prev_prop["type"] = godot::Variant::INT;
        prev_prop["hint_string"] = "buffer_option";
        props.push_back(prev_prop);

        godot::Dictionary rpol_prop;
        rpol_prop["name"] = "assignment_replace_buffer_id";
        rpol_prop["type"] = godot::Variant::INT;
        rpol_prop["hint_string"] = "buffer_option";
        props.push_back(rpol_prop);

        godot::Dictionary redg_prop;
        redg_prop["name"] = "relationship_edge_pod_buffer_id";
        redg_prop["type"] = godot::Variant::INT;
        redg_prop["hint_string"] = "buffer_option";
        props.push_back(redg_prop);

        godot::Dictionary stit_prop;
        stit_prop["name"] = "symbolic_title_token_buffer_id";
        stit_prop["type"] = godot::Variant::INT;
        stit_prop["hint_string"] = "buffer_option";
        props.push_back(stit_prop);

        godot::Dictionary pol_prop;
        pol_prop["name"] = "replacement_policy";
        pol_prop["type"] = godot::Variant::INT;
        pol_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        pol_prop["hint_string"] = "Replace Symbolic:0,Replace Literal:1,Replace Thorough:2";
        props.push_back(pol_prop);

        godot::Dictionary ufb_prop;
        ufb_prop["name"] = "unresolved_status_fallback";
        ufb_prop["type"] = godot::Variant::INT;
        ufb_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        ufb_prop["hint_string"] = "Cannot Meet:-2,Not Met:-1,Unknown:0";
        props.push_back(ufb_prop);

        godot::Dictionary clr_prop;
        clr_prop["name"] = "clear_symbolic_flag_in_place";
        clr_prop["type"] = godot::Variant::BOOL;
        clr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clr_prop);

        godot::Dictionary upd_prop;
        upd_prop["name"] = "update_status_on_unresolved";
        upd_prop["type"] = godot::Variant::BOOL;
        upd_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(upd_prop);

        godot::Dictionary rst_prop;
        rst_prop["name"] = "reset_status_to_unknown_on_rebind";
        rst_prop["type"] = godot::Variant::BOOL;
        rst_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rst_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_rebound_conditions";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (resolved_subject_id_buffer_id != INVALID_ID) return resolved_subject_id_buffer_id;
        if (subject_id_buffer_id != INVALID_ID)          return subject_id_buffer_id;
        if (resolved_object_id_buffer_id != INVALID_ID)  return resolved_object_id_buffer_id;
        return object_id_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("subject_id_buffer_id")) {
            subject_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["subject_id_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && subject_id_buffer_id == INVALID_ID) {
            subject_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("object_id_buffer_id")) {
            object_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["object_id_buffer_id"]));
        }
        if (p_props.has("resolved_subject_id_buffer_id")) {
            resolved_subject_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["resolved_subject_id_buffer_id"]));
        }
        if (p_props.has("resolved_object_id_buffer_id")) {
            resolved_object_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["resolved_object_id_buffer_id"]));
        }
        if (p_props.has("subject_is_symbolic_buffer_id")) {
            subject_is_symbolic_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["subject_is_symbolic_buffer_id"]));
        }
        if (p_props.has("object_is_symbolic_buffer_id")) {
            object_is_symbolic_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["object_is_symbolic_buffer_id"]));
        }
        if (p_props.has("condition_status_buffer_id")) {
            condition_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_status_buffer_id"]));
        }
        if (p_props.has("symbolic_to_literal_buffer_id")) {
            symbolic_to_literal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["symbolic_to_literal_buffer_id"]));
        }
        if (p_props.has("previous_literal_buffer_id")) {
            previous_literal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["previous_literal_buffer_id"]));
        }
        if (p_props.has("assignment_replace_buffer_id")) {
            assignment_replace_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["assignment_replace_buffer_id"]));
        }
        if (p_props.has("relationship_edge_pod_buffer_id")) {
            relationship_edge_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["relationship_edge_pod_buffer_id"]));
        }
        if (p_props.has("symbolic_title_token_buffer_id")) {
            symbolic_title_token_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["symbolic_title_token_buffer_id"]));
        }
        if (p_props.has("replacement_policy")) {
            replacement_policy = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["replacement_policy"]), 0, 2));
        }
        if (p_props.has("unresolved_status_fallback")) {
            unresolved_status_fallback = static_cast<int8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["unresolved_status_fallback"]), -2, 0));
        }
        if (p_props.has("clear_symbolic_flag_in_place")) {
            clear_symbolic_flag_in_place = static_cast<bool>(p_props["clear_symbolic_flag_in_place"]);
        }
        if (p_props.has("update_status_on_unresolved")) {
            update_status_on_unresolved = static_cast<bool>(p_props["update_status_on_unresolved"]);
        }
        if (p_props.has("reset_status_to_unknown_on_rebind")) {
            reset_status_to_unknown_on_rebind = static_cast<bool>(p_props["reset_status_to_unknown_on_rebind"]);
        }
        if (p_props.has("wake_rebound_conditions")) {
            wake_rebound_conditions = static_cast<bool>(p_props["wake_rebound_conditions"]);
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
        if constexpr (std::is_same_v<T, int32_t>) {
            const GrantPartPOD* map_part  = (symbolic_to_literal_buffer_id != INVALID_ID)   ? p_context.get_grant_part(symbolic_to_literal_buffer_id)   : nullptr;
            if (!map_part || !map_part->raw_base_ptr) return;

            const GrantPartPOD* sub_part  = (subject_id_buffer_id != INVALID_ID)            ? p_context.get_grant_part(subject_id_buffer_id)            : nullptr;
            const GrantPartPOD* obj_part  = (object_id_buffer_id != INVALID_ID)             ? p_context.get_grant_part(object_id_buffer_id)             : nullptr;
            const GrantPartPOD* rsub_part = (resolved_subject_id_buffer_id != INVALID_ID)   ? p_context.get_grant_part(resolved_subject_id_buffer_id)   : sub_part;
            const GrantPartPOD* robj_part = (resolved_object_id_buffer_id != INVALID_ID)    ? p_context.get_grant_part(resolved_object_id_buffer_id)    : obj_part;
            const GrantPartPOD* ssym_part = (subject_is_symbolic_buffer_id != INVALID_ID)   ? p_context.get_grant_part(subject_is_symbolic_buffer_id)   : nullptr;
            const GrantPartPOD* osym_part = (object_is_symbolic_buffer_id != INVALID_ID)    ? p_context.get_grant_part(object_is_symbolic_buffer_id)    : nullptr;
            const GrantPartPOD* cstat_part= (condition_status_buffer_id != INVALID_ID)      ? p_context.get_grant_part(condition_status_buffer_id)      : nullptr;
            const GrantPartPOD* prev_part = (previous_literal_buffer_id != INVALID_ID)      ? p_context.get_grant_part(previous_literal_buffer_id)      : nullptr;
            const GrantPartPOD* rpol_part = (assignment_replace_buffer_id != INVALID_ID)    ? p_context.get_grant_part(assignment_replace_buffer_id)    : nullptr;
            const GrantPartPOD* redg_part = (relationship_edge_pod_buffer_id != INVALID_ID) ? p_context.get_grant_part(relationship_edge_pod_buffer_id) : nullptr;
            const GrantPartPOD* stit_part = (symbolic_title_token_buffer_id != INVALID_ID)  ? p_context.get_grant_part(symbolic_title_token_buffer_id)  : nullptr;

            const int32_t* __restrict sym_to_lit    = reinterpret_cast<const int32_t*>(map_part->raw_base_ptr);
            const int64_t map_cap                   = static_cast<int64_t>(map_part->capacity_bytes / sizeof(int32_t));
            if (map_cap <= 0) return;

            const int32_t* __restrict prev_literals = (prev_part && prev_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(prev_part->raw_base_ptr) : nullptr;
            const int64_t prev_cap                  = prev_literals ? static_cast<int64_t>(prev_part->capacity_bytes / sizeof(int32_t)) : 0;
            const uint8_t* __restrict rpol_raw      = (rpol_part && rpol_part->raw_base_ptr) ? rpol_part->raw_base_ptr : nullptr;
            const uint32_t rpol_stride              = rpol_part ? rpol_part->element_stride : 0;
            const int64_t rpol_cap                  = (rpol_raw && rpol_stride > 0) ? static_cast<int64_t>(rpol_part->capacity_bytes / rpol_stride) : 0;
            const uint32_t* __restrict sym_tokens   = (stit_part && stit_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(stit_part->raw_base_ptr) : nullptr;
            const int64_t stit_cap                  = sym_tokens ? static_cast<int64_t>(stit_part->capacity_bytes / sizeof(uint32_t)) : 0;

            auto get_slot_policy = [&](int64_t slot_idx) noexcept -> uint8_t {
                if (!rpol_raw || slot_idx < 0 || slot_idx >= rpol_cap) return replacement_policy;
                if (rpol_stride == sizeof(int32_t)) {
                    return static_cast<uint8_t>(reinterpret_cast<const int32_t*>(rpol_raw)[slot_idx]);
                }
                return rpol_raw[slot_idx];
            };

            // Helper: resolves a handle (either symbolic slot ID or outgoing literal ID under REPLACE_LITERAL/THOROUGH)
            auto resolve_handle = [&](uint32_t raw_id, bool is_sym, bool& r_unresolved, bool& r_rebound) noexcept -> uint32_t {
                r_unresolved = false;
                r_rebound    = false;
                if (raw_id == INVALID_ID) return INVALID_ID;

                // 1. Symbolic reference resolution (REPLACE_SYMBOLIC or REPLACE_THOROUGH)
                if (is_sym) {
                    const int64_t sidx = static_cast<int64_t>(raw_id);
                    if (sidx < 0 || sidx >= map_cap) {
                        r_unresolved = true;
                        return INVALID_ID;
                    }
                    const uint8_t pol = get_slot_policy(sidx);
                    if (pol == NARRATIVE_REPLACE_SYMBOLIC || pol == NARRATIVE_REPLACE_THOROUGH) {
                        const int32_t mapped = sym_to_lit[sidx];
                        if (mapped >= 0 && static_cast<uint32_t>(mapped) != INVALID_ID) {
                            r_rebound = true;
                            return static_cast<uint32_t>(mapped);
                        }
                        r_unresolved = true;
                        return INVALID_ID;
                    }
                    return raw_id;
                }

                // 2. Literal replacement resolution (REPLACE_LITERAL or REPLACE_THOROUGH)
                // If raw_id matches an outgoing previous_literal[s], substitute with the newly assigned sym_to_lit[s]
                if (prev_literals && prev_cap > 0) {
                    const int64_t scan_cnt = std::min<int64_t>(map_cap, prev_cap);
                    for (int64_t s = 0; s < scan_cnt; ++s) {
                        const int32_t prev_lit = prev_literals[s];
                        if (prev_lit >= 0 && static_cast<uint32_t>(prev_lit) == raw_id) {
                            const uint8_t pol = get_slot_policy(s);
                            if (pol == NARRATIVE_REPLACE_LITERAL || pol == NARRATIVE_REPLACE_THOROUGH) {
                                const int32_t new_lit = sym_to_lit[s];
                                r_rebound = true;
                                if (new_lit >= 0 && static_cast<uint32_t>(new_lit) != INVALID_ID) {
                                    return static_cast<uint32_t>(new_lit);
                                }
                                r_unresolved = true;
                                return INVALID_ID;
                            }
                        }
                    }
                }

                return raw_id;
            };

            // --- Pass 1: Resolve ConditionEvaluationTableBuffer subject_id & object_id ---
            const GrantPartPOD* primary_cond_part = rsub_part ? rsub_part : (sub_part ? sub_part : robj_part);
            if (primary_cond_part && primary_cond_part->raw_base_ptr) {
                const uint32_t* __restrict src_subjects = (sub_part && sub_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(sub_part->raw_base_ptr) : nullptr;
                const uint32_t* __restrict src_objects  = (obj_part && obj_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(obj_part->raw_base_ptr) : nullptr;
                uint32_t* __restrict dst_subjects       = (rsub_part && rsub_part->raw_base_ptr)? reinterpret_cast<uint32_t*>(rsub_part->raw_base_ptr)       : nullptr;
                uint32_t* __restrict dst_objects        = (robj_part && robj_part->raw_base_ptr)? reinterpret_cast<uint32_t*>(robj_part->raw_base_ptr)       : nullptr;

                uint8_t* __restrict ssym_raw = (ssym_part && ssym_part->raw_base_ptr) ? ssym_part->raw_base_ptr : nullptr;
                const uint32_t ssym_stride   = ssym_part ? ssym_part->element_stride : 0;
                const int64_t ssym_cap       = (ssym_raw && ssym_stride > 0) ? static_cast<int64_t>(ssym_part->capacity_bytes / ssym_stride) : 0;

                uint8_t* __restrict osym_raw = (osym_part && osym_part->raw_base_ptr) ? osym_part->raw_base_ptr : nullptr;
                const uint32_t osym_stride   = osym_part ? osym_part->element_stride : 0;
                const int64_t osym_cap       = (osym_raw && osym_stride > 0) ? static_cast<int64_t>(osym_part->capacity_bytes / osym_stride) : 0;

                uint8_t* __restrict cstat_raw = (cstat_part && cstat_part->raw_base_ptr) ? cstat_part->raw_base_ptr : nullptr;
                const uint32_t cstat_stride   = cstat_part ? cstat_part->element_stride : 0;
                const int64_t cstat_cap       = (cstat_raw && cstat_stride > 0) ? static_cast<int64_t>(cstat_part->capacity_bytes / cstat_stride) : 0;

                int64_t cond_count = static_cast<int64_t>(primary_cond_part->capacity_bytes / sizeof(uint32_t));
                if (sub_part)  cond_count = std::min<int64_t>(cond_count, static_cast<int64_t>(sub_part->capacity_bytes / sizeof(uint32_t)));
                if (rsub_part) cond_count = std::min<int64_t>(cond_count, static_cast<int64_t>(rsub_part->capacity_bytes / sizeof(uint32_t)));
                if (obj_part)  cond_count = std::min<int64_t>(cond_count, static_cast<int64_t>(obj_part->capacity_bytes / sizeof(uint32_t)));
                if (robj_part) cond_count = std::min<int64_t>(cond_count, static_cast<int64_t>(robj_part->capacity_bytes / sizeof(uint32_t)));

                auto read_flag = [](const uint8_t* raw, uint32_t stride, int64_t idx) noexcept -> bool {
                    if (!raw) return false;
                    if (stride == sizeof(uint32_t)) return reinterpret_cast<const uint32_t*>(raw)[idx] != 0u;
                    return raw[idx] != 0u;
                };

                auto clear_flag = [](uint8_t* raw, uint32_t stride, int64_t idx) noexcept {
                    if (!raw) return;
                    if (stride == sizeof(uint32_t)) reinterpret_cast<uint32_t*>(raw)[idx] = 0u;
                    else                            raw[idx] = 0u;
                };

                auto write_status = [&](int64_t idx, int8_t st) noexcept {
                    if (!cstat_raw || idx < 0 || idx >= cstat_cap) return;
                    if (cstat_stride == sizeof(int32_t)) {
                        reinterpret_cast<int32_t*>(cstat_raw)[idx] = static_cast<int32_t>(st);
                    } else {
                        reinterpret_cast<int8_t*>(cstat_raw)[idx] = st;
                    }
                };

                const bool sub_in_place = (dst_subjects == src_subjects);
                const bool obj_in_place = (dst_objects == src_objects);
                const uint32_t wake_buf_id = get_target_buffer_id();

                auto eval_condition = [&](int64_t idx) noexcept {
                    bool any_unresolved = false;
                    bool any_rebound    = false;

                    if (src_subjects && dst_subjects) {
                        const bool is_sym = (idx < ssym_cap) ? read_flag(ssym_raw, ssym_stride, idx) : (!prev_literals);
                        bool unres = false, reb = false;
                        const uint32_t prev_val = dst_subjects[idx];
                        const uint32_t res_val  = resolve_handle(src_subjects[idx], is_sym, unres, reb);
                        dst_subjects[idx] = res_val;
                        if (res_val != prev_val) any_rebound = true;
                        any_unresolved = any_unresolved || unres;
                        any_rebound    = any_rebound    || reb;

                        if (sub_in_place && clear_symbolic_flag_in_place && reb && !unres && idx < ssym_cap) {
                            clear_flag(ssym_raw, ssym_stride, idx);
                        }
                    }

                    if (src_objects && dst_objects) {
                        const bool is_sym = (idx < osym_cap) ? read_flag(osym_raw, osym_stride, idx) : false;
                        if (src_objects[idx] != INVALID_ID || is_sym) {
                            bool unres = false, reb = false;
                            const uint32_t prev_val = dst_objects[idx];
                            const uint32_t res_val  = resolve_handle(src_objects[idx], is_sym, unres, reb);
                            dst_objects[idx] = res_val;
                            if (res_val != prev_val) any_rebound = true;
                            any_unresolved = any_unresolved || unres;
                            any_rebound    = any_rebound    || reb;

                            if (obj_in_place && clear_symbolic_flag_in_place && reb && !unres && idx < osym_cap) {
                                clear_flag(osym_raw, osym_stride, idx);
                            }
                        }
                    }

                    if (any_unresolved && update_status_on_unresolved) {
                        write_status(idx, unresolved_status_fallback);
                    } else if (any_rebound && reset_status_to_unknown_on_rebind) {
                        write_status(idx, COND_STATUS_UNKNOWN);
                    }

                    if (any_rebound && wake_rebound_conditions && wake_buf_id != INVALID_ID) {
                        p_context.queue_selection_command(wake_buf_id, idx);
                    }
                };

                if (cond_count > 0) {
                    _iterate_selection(primary_cond_part->selection, cond_count, eval_condition);
                }
            }

            // --- Pass 2: Optional NarrativeRelationshipCSRBuffer symbolic edge resolution ---
            // Mirrors Narrative::fill_role/symbol/place/moment/story relationship sweeps in narrative.cpp
            if (redg_part && redg_part->raw_base_ptr) {
                RelationshipEdgePOD* __restrict edges = reinterpret_cast<RelationshipEdgePOD*>(redg_part->raw_base_ptr);
                const int64_t edge_cap = static_cast<int64_t>(redg_part->capacity_bytes / sizeof(RelationshipEdgePOD));
                if (edge_cap > 0) {
                    auto eval_edge = [&](int64_t eidx) noexcept {
                        RelationshipEdgePOD& edge = edges[eidx];
                        const bool edge_is_sym = (edge.flags & 1u) != 0u;
                        bool unres = false, reb = false;

                        // Match by symbolic title token (e.g. char_rel->get_title() == role->get_official_name())
                        if (sym_tokens && edge.relationship_title_token != 0u) {
                            const int64_t tok_cnt = std::min<int64_t>(map_cap, stit_cap);
                            for (int64_t s = 0; s < tok_cnt; ++s) {
                                if (sym_tokens[s] == edge.relationship_title_token) {
                                    const uint8_t pol = get_slot_policy(s);
                                    if (pol == NARRATIVE_REPLACE_SYMBOLIC || pol == NARRATIVE_REPLACE_THOROUGH) {
                                        const int32_t mapped = sym_to_lit[s];
                                        edge.target_narreme_id = (mapped >= 0) ? static_cast<uint32_t>(mapped) : INVALID_ID;
                                        return;
                                    }
                                }
                            }
                        }

                        const uint32_t resolved = resolve_handle(edge.target_narreme_id, edge_is_sym, unres, reb);
                        if (reb || unres) {
                            edge.target_narreme_id = resolved;
                            if (clear_symbolic_flag_in_place && !unres) {
                                edge.flags &= ~1u;
                            }
                        }
                    };
                    _iterate_selection(redg_part->selection, edge_cap, eval_edge);
                }
            }
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

static_assert(sizeof(SymbolicReferenceResolveTransformLogic<>) == 64, "SymbolicReferenceResolveTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core