#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "narreme_symbolic_similarity_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_SYMBOLIC_ASSIGNMENT_POD_DEFINED
#define IDEAM_NARRATIVES_SYMBOLIC_ASSIGNMENT_POD_DEFINED

/**
 * Operations supported by RoleBestFitAssignmentTransformLogic (N2).
 */
enum RoleAssignmentOp : uint8_t {
    ASSIGN_OP_FILL_OPEN_SLOTS    = 0, // Assigns best-fit unmasked Literal candidates to currently vacant (-1) Symbolic slots
    ASSIGN_OP_REASSIGN_IF_BETTER = 1, // Fills vacant slots and upgrades filled slots when a strictly better fit score is found
    ASSIGN_OP_FORCE_REASSIGN_ALL = 2, // Recomputes and overwrites assignments for all selected Symbolic slots
    ASSIGN_OP_VACATE_SELECTED    = 3  // Vacates (-1) selected Symbolic slots (mirrors vacate_role/symbol/place/moment/story)
};

/**
 * SymbolicAssignmentRecordPOD (16-Byte Aligned)
 * Compact POD row for SymbolicAssignmentMapBuffer (Section 3.2 Buffer 2),
 * mapping symbolic_id <-> literal_id alongside fit_score, fit_mode, and replacement_policy.
 */
struct alignas(16) SymbolicAssignmentRecordPOD {
    uint32_t symbolic_id       = INVALID_ID;                 // 4B: Symbolic Narreme ID (Role, Symbol, Place, Moment, Story)
    int32_t literal_id         = -1;                         // 4B: Assigned Literal Narreme ID (-1 if vacant)
    int32_t fit_score          = NARRATIVE_MAX_DISCREPANCY_SCORE; // 4B: Resolved fit score for this assignment
    uint8_t fit_mode           = NARRATIVE_FIT_THOROUGH;     // 1B: NarrativeBestFitMode
    uint8_t replacement_policy = NARRATIVE_REPLACE_THOROUGH; // 1B: NarrativeReplacementPolicy
    uint8_t narreme_type       = NARREME_TYPE_CHARACTER;     // 1B: NarremeType of the slot
    uint8_t dirty_flags        = 0u;                         // 1B: Bit 0 = filled, Bit 1 = vacated -> Total = 16B
};

static_assert(sizeof(SymbolicAssignmentRecordPOD) == 16, "SymbolicAssignmentRecordPOD must be 16 bytes!");

/**
 * Transition flags written to assignment_dirty_flags_buffer_id for boundary signal emission (N11).
 */
enum SymbolicAssignmentDirtyFlags : uint8_t {
    ASSIGNMENT_DIRTY_NONE    = 0u,
    ASSIGNMENT_DIRTY_FILLED  = 1u << 0, // Emits role_filled / symbol_filled / place_filled / moment_filled / story_filled
    ASSIGNMENT_DIRTY_VACATED = 1u << 1  // Emits role_vacated / symbol_vacated / place_vacated / moment_vacated / story_vacated
};

#endif // IDEAM_NARRATIVES_SYMBOLIC_ASSIGNMENT_POD_DEFINED

/**
 * RoleBestFitAssignmentTransformLogic<T> (N2)
 * Resolves highest-scoring (lowest discrepancy) unmasked Literal candidates for open Symbolic slots
 * (fill_role, fill_symbol, fill_place, fill_moment, fill_story) and updates SymbolicAssignmentMapBuffer
 * with O(1) bidirectional lookup (symbolic_to_literal and literal_to_symbolic).
 *
 * Eliminates the 5 pairs of parallel TypedArray<int> *_assignments arrays in Narrative (narrative.h/.cpp).
 */
template <typename T = int32_t>
struct alignas(64) RoleBestFitAssignmentTransformLogic {
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

    static constexpr std::string_view display_name = "Role Best Fit Assignment";
    static constexpr std::string_view ui_category  = "Casting";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t symbolic_to_literal_buffer_id   = INVALID_ID;                  // 4B: Primary target (SymbolicAssignmentMapBuffer::literal_id int32_t)
    uint32_t literal_to_symbolic_buffer_id   = INVALID_ID;                  // 4B: Optional inverse map (literal_id -> symbolic_id int32_t)
    uint32_t assignment_fit_score_buffer_id  = INVALID_ID;                  // 4B: SymbolicAssignmentMapBuffer::fit_score (int32_t)
    uint32_t assignment_fit_mode_buffer_id   = INVALID_ID;                  // 4B: Optional per-slot BestNarrativeFit (uint8_t/int32_t)
    uint32_t assignment_replace_buffer_id    = INVALID_ID;                  // 4B: Optional per-slot NarrativeReplacement (uint8_t/int32_t)
    uint32_t previous_literal_buffer_id      = INVALID_ID;                  // 4B: Optional int32_t column recording outgoing literal_id on change
    uint32_t assignment_dirty_flags_buffer_id= INVALID_ID;                  // 4B: Optional uint8_t SymbolicAssignmentDirtyFlags column
    uint32_t candidate_score_buffer_id       = INVALID_ID;                  // 4B: Candidate scores from N1 (1D or 2D [slot * stride + cand])
    uint32_t candidate_mask_buffer_id        = INVALID_ID;                  // 4B: Optional candidate exclusion mask (uint8_t/uint32_t/int32_t)
    uint32_t candidate_type_buffer_id        = INVALID_ID;                  // 4B: Optional NarremeIdentityAndTraitBuffer::narreme_type for candidates
    uint32_t symbolic_type_buffer_id         = INVALID_ID;                  // 4B: Optional NarremeIdentityAndTraitBuffer::narreme_type for slots

    int32_t max_acceptable_discrepancy       = NARRATIVE_MAX_DISCREPANCY_SCORE - 1; // 4B: Reject candidates with score > threshold
    uint32_t candidate_row_stride            = 0u;                          // 4B: If > 0, treats candidate_score_buffer_id as 2D matrix per slot
    int32_t explicit_literal_id              = -1;                          // 4B: If >= 0, directly assigns this literal_id (direct fill_* call)

    uint8_t assignment_op                    = ASSIGN_OP_FILL_OPEN_SLOTS;   // 1B: RoleAssignmentOp
    uint8_t default_fit_mode                 = NARRATIVE_FIT_THOROUGH;      // 1B: Fallback BestNarrativeFit written to map
    uint8_t default_replacement_policy       = NARRATIVE_REPLACE_THOROUGH;  // 1B: Fallback NarrativeReplacement written to map
    bool enforce_unique_literals             = true;                        // 1B: Prevent one literal from filling multiple symbolic slots at once
    bool higher_score_is_better              = false;                       // 1B: False = min discrepancy (Narrative default); True = max similarity
    bool wake_on_assignment_change           = true;                        // 1B: Queue changed symbolic_id into TaskContextPOD::wave_commands
    uint8_t reserved_padding[2]              = {0};                         // 2B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary s2l_prop;
        s2l_prop["name"] = "symbolic_to_literal_buffer_id";
        s2l_prop["type"] = godot::Variant::INT;
        s2l_prop["hint_string"] = "buffer_option";
        props.push_back(s2l_prop);

        godot::Dictionary l2s_prop;
        l2s_prop["name"] = "literal_to_symbolic_buffer_id";
        l2s_prop["type"] = godot::Variant::INT;
        l2s_prop["hint_string"] = "buffer_option";
        props.push_back(l2s_prop);

        godot::Dictionary fscr_prop;
        fscr_prop["name"] = "assignment_fit_score_buffer_id";
        fscr_prop["type"] = godot::Variant::INT;
        fscr_prop["hint_string"] = "buffer_option";
        props.push_back(fscr_prop);

        godot::Dictionary fmod_prop;
        fmod_prop["name"] = "assignment_fit_mode_buffer_id";
        fmod_prop["type"] = godot::Variant::INT;
        fmod_prop["hint_string"] = "buffer_option";
        props.push_back(fmod_prop);

        godot::Dictionary rpol_prop;
        rpol_prop["name"] = "assignment_replace_buffer_id";
        rpol_prop["type"] = godot::Variant::INT;
        rpol_prop["hint_string"] = "buffer_option";
        props.push_back(rpol_prop);

        godot::Dictionary prev_prop;
        prev_prop["name"] = "previous_literal_buffer_id";
        prev_prop["type"] = godot::Variant::INT;
        prev_prop["hint_string"] = "buffer_option";
        props.push_back(prev_prop);

        godot::Dictionary drt_prop;
        drt_prop["name"] = "assignment_dirty_flags_buffer_id";
        drt_prop["type"] = godot::Variant::INT;
        drt_prop["hint_string"] = "buffer_option";
        props.push_back(drt_prop);

        godot::Dictionary cscr_prop;
        cscr_prop["name"] = "candidate_score_buffer_id";
        cscr_prop["type"] = godot::Variant::INT;
        cscr_prop["hint_string"] = "buffer_option";
        props.push_back(cscr_prop);

        godot::Dictionary cmsk_prop;
        cmsk_prop["name"] = "candidate_mask_buffer_id";
        cmsk_prop["type"] = godot::Variant::INT;
        cmsk_prop["hint_string"] = "buffer_option";
        props.push_back(cmsk_prop);

        godot::Dictionary ctyp_prop;
        ctyp_prop["name"] = "candidate_type_buffer_id";
        ctyp_prop["type"] = godot::Variant::INT;
        ctyp_prop["hint_string"] = "buffer_option";
        props.push_back(ctyp_prop);

        godot::Dictionary styp_prop;
        styp_prop["name"] = "symbolic_type_buffer_id";
        styp_prop["type"] = godot::Variant::INT;
        styp_prop["hint_string"] = "buffer_option";
        props.push_back(styp_prop);

        godot::Dictionary op_prop;
        op_prop["name"] = "assignment_op";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Fill Open Slots:0,Reassign If Better:1,Force Reassign All:2,Vacate Selected:3";
        props.push_back(op_prop);

        godot::Dictionary dfm_prop;
        dfm_prop["name"] = "default_fit_mode";
        dfm_prop["type"] = godot::Variant::INT;
        dfm_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dfm_prop["hint_string"] = "Fit Basic:0,Fit Symbolic:1,Fit Literal:2,Fit Thorough:3";
        props.push_back(dfm_prop);

        godot::Dictionary drp_prop;
        drp_prop["name"] = "default_replacement_policy";
        drp_prop["type"] = godot::Variant::INT;
        drp_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        drp_prop["hint_string"] = "Replace Symbolic:0,Replace Literal:1,Replace Thorough:2";
        props.push_back(drp_prop);

        godot::Dictionary maxd_prop;
        maxd_prop["name"] = "max_acceptable_discrepancy";
        maxd_prop["type"] = godot::Variant::INT;
        maxd_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(maxd_prop);

        godot::Dictionary str_prop;
        str_prop["name"] = "candidate_row_stride";
        str_prop["type"] = godot::Variant::INT;
        str_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(str_prop);

        godot::Dictionary exl_prop;
        exl_prop["name"] = "explicit_literal_id";
        exl_prop["type"] = godot::Variant::INT;
        exl_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(exl_prop);

        godot::Dictionary unq_prop;
        unq_prop["name"] = "enforce_unique_literals";
        unq_prop["type"] = godot::Variant::BOOL;
        unq_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(unq_prop);

        godot::Dictionary hsb_prop;
        hsb_prop["name"] = "higher_score_is_better";
        hsb_prop["type"] = godot::Variant::BOOL;
        hsb_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(hsb_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_on_assignment_change";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (symbolic_to_literal_buffer_id != INVALID_ID)  return symbolic_to_literal_buffer_id;
        if (assignment_fit_score_buffer_id != INVALID_ID) return assignment_fit_score_buffer_id;
        return candidate_score_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("symbolic_to_literal_buffer_id")) {
            symbolic_to_literal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["symbolic_to_literal_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && symbolic_to_literal_buffer_id == INVALID_ID) {
            symbolic_to_literal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("literal_to_symbolic_buffer_id")) {
            literal_to_symbolic_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["literal_to_symbolic_buffer_id"]));
        }
        if (p_props.has("assignment_fit_score_buffer_id")) {
            assignment_fit_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["assignment_fit_score_buffer_id"]));
        }
        if (p_props.has("assignment_fit_mode_buffer_id")) {
            assignment_fit_mode_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["assignment_fit_mode_buffer_id"]));
        }
        if (p_props.has("assignment_replace_buffer_id")) {
            assignment_replace_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["assignment_replace_buffer_id"]));
        }
        if (p_props.has("previous_literal_buffer_id")) {
            previous_literal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["previous_literal_buffer_id"]));
        }
        if (p_props.has("assignment_dirty_flags_buffer_id")) {
            assignment_dirty_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["assignment_dirty_flags_buffer_id"]));
        }
        if (p_props.has("candidate_score_buffer_id")) {
            candidate_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["candidate_score_buffer_id"]));
        }
        if (p_props.has("candidate_mask_buffer_id")) {
            candidate_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["candidate_mask_buffer_id"]));
        }
        if (p_props.has("candidate_type_buffer_id")) {
            candidate_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["candidate_type_buffer_id"]));
        }
        if (p_props.has("symbolic_type_buffer_id")) {
            symbolic_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["symbolic_type_buffer_id"]));
        }
        if (p_props.has("max_acceptable_discrepancy")) {
            max_acceptable_discrepancy = static_cast<int32_t>(static_cast<int64_t>(p_props["max_acceptable_discrepancy"]));
        }
        if (p_props.has("candidate_row_stride")) {
            candidate_row_stride = static_cast<uint32_t>(std::max<int64_t>(0, static_cast<int64_t>(p_props["candidate_row_stride"])));
        }
        if (p_props.has("explicit_literal_id")) {
            explicit_literal_id = static_cast<int32_t>(static_cast<int64_t>(p_props["explicit_literal_id"]));
        }
        if (p_props.has("assignment_op")) {
            assignment_op = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["assignment_op"]), 0, 3));
        }
        if (p_props.has("default_fit_mode")) {
            default_fit_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["default_fit_mode"]), 0, 3));
        }
        if (p_props.has("default_replacement_policy")) {
            default_replacement_policy = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["default_replacement_policy"]), 0, 2));
        }
        if (p_props.has("enforce_unique_literals")) {
            enforce_unique_literals = static_cast<bool>(p_props["enforce_unique_literals"]);
        }
        if (p_props.has("higher_score_is_better")) {
            higher_score_is_better = static_cast<bool>(p_props["higher_score_is_better"]);
        }
        if (p_props.has("wake_on_assignment_change")) {
            wake_on_assignment_change = static_cast<bool>(p_props["wake_on_assignment_change"]);
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
            const uint32_t tgt_id = get_target_buffer_id();
            const GrantPartPOD* s2l_part = (tgt_id != INVALID_ID) ? p_context.get_grant_part(tgt_id) : nullptr;
            if (!s2l_part || !s2l_part->raw_base_ptr) return;

            const GrantPartPOD* l2s_part  = (literal_to_symbolic_buffer_id != INVALID_ID)    ? p_context.get_grant_part(literal_to_symbolic_buffer_id)    : nullptr;
            const GrantPartPOD* fscr_part = (assignment_fit_score_buffer_id != INVALID_ID)   ? p_context.get_grant_part(assignment_fit_score_buffer_id)   : nullptr;
            const GrantPartPOD* fmod_part = (assignment_fit_mode_buffer_id != INVALID_ID)    ? p_context.get_grant_part(assignment_fit_mode_buffer_id)    : nullptr;
            const GrantPartPOD* rpol_part = (assignment_replace_buffer_id != INVALID_ID)     ? p_context.get_grant_part(assignment_replace_buffer_id)     : nullptr;
            const GrantPartPOD* prev_part = (previous_literal_buffer_id != INVALID_ID)       ? p_context.get_grant_part(previous_literal_buffer_id)       : nullptr;
            const GrantPartPOD* drt_part  = (assignment_dirty_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(assignment_dirty_flags_buffer_id) : nullptr;
            const GrantPartPOD* cscr_part = (candidate_score_buffer_id != INVALID_ID)        ? p_context.get_grant_part(candidate_score_buffer_id)        : nullptr;
            const GrantPartPOD* cmsk_part = (candidate_mask_buffer_id != INVALID_ID)         ? p_context.get_grant_part(candidate_mask_buffer_id)         : nullptr;
            const GrantPartPOD* ctyp_part = (candidate_type_buffer_id != INVALID_ID)         ? p_context.get_grant_part(candidate_type_buffer_id)         : nullptr;
            const GrantPartPOD* styp_part = (symbolic_type_buffer_id != INVALID_ID)          ? p_context.get_grant_part(symbolic_type_buffer_id)          : nullptr;

            int32_t* __restrict sym_to_lit        = reinterpret_cast<int32_t*>(s2l_part->raw_base_ptr);
            int32_t* __restrict lit_to_sym        = (l2s_part && l2s_part->raw_base_ptr)   ? reinterpret_cast<int32_t*>(l2s_part->raw_base_ptr)         : nullptr;
            int32_t* __restrict fit_scores        = (fscr_part && fscr_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(fscr_part->raw_base_ptr)       : nullptr;
            uint8_t* __restrict fmod_raw          = (fmod_part && fmod_part->raw_base_ptr) ? fmod_part->raw_base_ptr                                    : nullptr;
            const uint32_t fmod_stride            = fmod_part ? fmod_part->element_stride : 0;
            uint8_t* __restrict rpol_raw          = (rpol_part && rpol_part->raw_base_ptr) ? rpol_part->raw_base_ptr                                    : nullptr;
            const uint32_t rpol_stride            = rpol_part ? rpol_part->element_stride : 0;
            int32_t* __restrict prev_literals     = (prev_part && prev_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(prev_part->raw_base_ptr)       : nullptr;
            uint8_t* __restrict dirty_raw         = (drt_part && drt_part->raw_base_ptr)   ? drt_part->raw_base_ptr                                     : nullptr;
            const uint32_t dirty_stride           = drt_part ? drt_part->element_stride : 0;
            const int32_t* __restrict cand_scores = (cscr_part && cscr_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(cscr_part->raw_base_ptr) : nullptr;
            const uint8_t* __restrict cmsk_raw    = (cmsk_part && cmsk_part->raw_base_ptr) ? cmsk_part->raw_base_ptr                                    : nullptr;
            const uint32_t cmsk_stride            = cmsk_part ? cmsk_part->element_stride : 0;
            const uint8_t* __restrict ctyp_raw    = (ctyp_part && ctyp_part->raw_base_ptr) ? ctyp_part->raw_base_ptr                                    : nullptr;
            const uint32_t ctyp_stride            = ctyp_part ? ctyp_part->element_stride : 0;
            const uint8_t* __restrict styp_raw    = (styp_part && styp_part->raw_base_ptr) ? styp_part->raw_base_ptr                                    : nullptr;
            const uint32_t styp_stride            = styp_part ? styp_part->element_stride : 0;

            const int64_t slot_cap = static_cast<int64_t>(s2l_part->capacity_bytes / sizeof(int32_t));
            if (slot_cap <= 0) return;

            const int64_t l2s_cap  = lit_to_sym    ? static_cast<int64_t>(l2s_part->capacity_bytes / sizeof(int32_t))  : 0;
            const int64_t fscr_cap = fit_scores    ? static_cast<int64_t>(fscr_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t fmod_cap = (fmod_raw && fmod_stride > 0) ? static_cast<int64_t>(fmod_part->capacity_bytes / fmod_stride) : 0;
            const int64_t rpol_cap = (rpol_raw && rpol_stride > 0) ? static_cast<int64_t>(rpol_part->capacity_bytes / rpol_stride) : 0;
            const int64_t prev_cap = prev_literals ? static_cast<int64_t>(prev_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t drt_cap  = (dirty_raw && dirty_stride > 0) ? static_cast<int64_t>(drt_part->capacity_bytes / dirty_stride) : 0;
            const int64_t cscr_cap = cand_scores   ? static_cast<int64_t>(cscr_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t cmsk_cap = (cmsk_raw && cmsk_stride > 0) ? static_cast<int64_t>(cmsk_part->capacity_bytes / cmsk_stride) : 0;
            const int64_t ctyp_cap = (ctyp_raw && ctyp_stride > 0) ? static_cast<int64_t>(ctyp_part->capacity_bytes / ctyp_stride) : 0;
            const int64_t styp_cap = (styp_raw && styp_stride > 0) ? static_cast<int64_t>(styp_part->capacity_bytes / styp_stride) : 0;

            auto write_dirty_flag = [&](int64_t sidx, uint8_t flag_bits) noexcept {
                if (!dirty_raw || sidx < 0 || sidx >= drt_cap) return;
                if (dirty_stride == sizeof(uint32_t)) {
                    reinterpret_cast<uint32_t*>(dirty_raw)[sidx] |= static_cast<uint32_t>(flag_bits);
                } else {
                    dirty_raw[sidx] |= flag_bits;
                }
            };

            auto write_slot_metadata = [&](int64_t sidx) noexcept {
                if (fmod_raw && sidx >= 0 && sidx < fmod_cap) {
                    if (fmod_stride == sizeof(int32_t)) {
                        reinterpret_cast<int32_t*>(fmod_raw)[sidx] = static_cast<int32_t>(default_fit_mode);
                    } else {
                        fmod_raw[sidx] = default_fit_mode;
                    }
                }
                if (rpol_raw && sidx >= 0 && sidx < rpol_cap) {
                    if (rpol_stride == sizeof(int32_t)) {
                        reinterpret_cast<int32_t*>(rpol_raw)[sidx] = static_cast<int32_t>(default_replacement_policy);
                    } else {
                        rpol_raw[sidx] = default_replacement_policy;
                    }
                }
            };

            auto is_cand_masked = [&](int64_t cidx) noexcept -> bool {
                if (!cmsk_raw || cidx < 0 || cidx >= cmsk_cap) return false;
                if (cmsk_stride == sizeof(uint32_t)) {
                    return reinterpret_cast<const uint32_t*>(cmsk_raw)[cidx] != 0u;
                }
                if (cmsk_stride == sizeof(uint64_t)) {
                    return reinterpret_cast<const uint64_t*>(cmsk_raw)[cidx] != 0ULL;
                }
                return cmsk_raw[cidx] != 0u;
            };

            auto read_type = [](const uint8_t* raw, uint32_t stride, int64_t idx) noexcept -> uint8_t {
                if (stride == sizeof(int32_t)) {
                    return static_cast<uint8_t>(reinterpret_cast<const int32_t*>(raw)[idx]);
                }
                return raw[idx];
            };

            auto is_literal_claimed_by_other = [&](int32_t cand_id, int64_t current_slot) noexcept -> bool {
                if (!enforce_unique_literals || cand_id < 0) return false;
                if (lit_to_sym && cand_id < l2s_cap) {
                    const int32_t owner_slot = lit_to_sym[cand_id];
                    return (owner_slot >= 0 && static_cast<int64_t>(owner_slot) != current_slot);
                }
                for (int64_t s = 0; s < slot_cap; ++s) {
                    if (s != current_slot && sym_to_lit[s] == cand_id) {
                        return true;
                    }
                }
                return false;
            };

            auto eval_slot = [&](int64_t sidx) noexcept {
                const int32_t cur_lit = sym_to_lit[sidx];
                const bool is_currently_valid = (cur_lit >= 0 && static_cast<uint32_t>(cur_lit) != INVALID_ID);

                // 1. Handle explicit vacation (vacate_role / vacate_symbol / vacate_place / vacate_moment / vacate_story)
                if (assignment_op == ASSIGN_OP_VACATE_SELECTED) {
                    if (is_currently_valid) {
                        if (prev_literals && sidx < prev_cap) prev_literals[sidx] = cur_lit;
                        if (lit_to_sym && cur_lit < l2s_cap && lit_to_sym[cur_lit] == static_cast<int32_t>(sidx)) {
                            lit_to_sym[cur_lit] = -1;
                        }
                        sym_to_lit[sidx] = -1;
                        if (fit_scores && sidx < fscr_cap) {
                            fit_scores[sidx] = higher_score_is_better ? 0 : NARRATIVE_MAX_DISCREPANCY_SCORE;
                        }
                        write_dirty_flag(sidx, ASSIGNMENT_DIRTY_VACATED);
                        if (wake_on_assignment_change) {
                            p_context.queue_selection_command(tgt_id, sidx);
                        }
                    }
                    return;
                }

                // 2. Skip already-filled slots in ASSIGN_OP_FILL_OPEN_SLOTS mode
                if (assignment_op == ASSIGN_OP_FILL_OPEN_SLOTS && is_currently_valid) {
                    return;
                }

                int32_t chosen_lit   = -1;
                int32_t chosen_score = higher_score_is_better ? INT32_MIN : NARRATIVE_MAX_DISCREPANCY_SCORE;

                // 3. Direct explicit assignment override (fill_role / fill_symbol / fill_place / fill_moment / fill_story)
                if (explicit_literal_id >= 0) {
                    if (!is_cand_masked(explicit_literal_id) && !is_literal_claimed_by_other(explicit_literal_id, sidx)) {
                        chosen_lit   = explicit_literal_id;
                        chosen_score = 0;
                    }
                } else if (cand_scores && cscr_cap > 0) {
                    // 4. Scan candidate score slice to find the best-fit unmasked Literal Narreme
                    const int64_t row_off = (candidate_row_stride > 0u)
                        ? (sidx * static_cast<int64_t>(candidate_row_stride))
                        : 0;
                    const int64_t cand_count = (candidate_row_stride > 0u)
                        ? std::min<int64_t>(static_cast<int64_t>(candidate_row_stride), cscr_cap - row_off)
                        : cscr_cap;

                    const bool check_types = (styp_raw != nullptr && ctyp_raw != nullptr && sidx < styp_cap);
                    const uint8_t req_type = check_types ? read_type(styp_raw, styp_stride, sidx) : static_cast<uint8_t>(NARREME_TYPE_ANY);

                    for (int64_t c = 0; c < cand_count; ++c) {
                        if (is_cand_masked(c)) continue;
                        if (check_types && c < ctyp_cap && req_type != NARREME_TYPE_ANY) {
                            if (read_type(ctyp_raw, ctyp_stride, c) != req_type) continue;
                        }
                        if (is_literal_claimed_by_other(static_cast<int32_t>(c), sidx)) continue;

                        const int32_t sc = cand_scores[row_off + c];
                        if (higher_score_is_better) {
                            if (sc > chosen_score) {
                                chosen_score = sc;
                                chosen_lit   = static_cast<int32_t>(c);
                            }
                        } else {
                            if (sc <= max_acceptable_discrepancy && sc < chosen_score) {
                                chosen_score = sc;
                                chosen_lit   = static_cast<int32_t>(c);
                            }
                        }
                    }
                }

                if (chosen_lit < 0) return;

                // In REASSIGN_IF_BETTER mode, only replace an existing valid assignment if strictly better
                if (assignment_op == ASSIGN_OP_REASSIGN_IF_BETTER && is_currently_valid && fit_scores && sidx < fscr_cap) {
                    const int32_t existing_score = fit_scores[sidx];
                    const bool is_better = higher_score_is_better
                        ? (chosen_score > existing_score)
                        : (chosen_score < existing_score);
                    if (!is_better) return;
                }

                // 5. Commit new assignment into SymbolicAssignmentMapBuffer
                if (chosen_lit != cur_lit) {
                    if (prev_literals && sidx < prev_cap) {
                        prev_literals[sidx] = is_currently_valid ? cur_lit : -1;
                    }
                    if (is_currently_valid && lit_to_sym && cur_lit < l2s_cap && lit_to_sym[cur_lit] == static_cast<int32_t>(sidx)) {
                        lit_to_sym[cur_lit] = -1;
                    }
                    sym_to_lit[sidx] = chosen_lit;
                    if (lit_to_sym && chosen_lit < l2s_cap) {
                        lit_to_sym[chosen_lit] = static_cast<int32_t>(sidx);
                    }

                    const uint8_t dflags = is_currently_valid
                        ? (ASSIGNMENT_DIRTY_VACATED | ASSIGNMENT_DIRTY_FILLED)
                        : ASSIGNMENT_DIRTY_FILLED;
                    write_dirty_flag(sidx, dflags);

                    if (wake_on_assignment_change) {
                        p_context.queue_selection_command(tgt_id, sidx);
                    }
                }

                if (fit_scores && sidx < fscr_cap) {
                    fit_scores[sidx] = chosen_score;
                }
                write_slot_metadata(sidx);
            };

            _iterate_selection(s2l_part->selection, slot_cap, eval_slot);
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

static_assert(sizeof(RoleBestFitAssignmentTransformLogic<>) == 64, "RoleBestFitAssignmentTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core