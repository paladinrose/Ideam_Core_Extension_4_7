#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/swap_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "symbolic_reference_resolve_transform_logic.h"
#include "gameplay_condition_bridge_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_CAUSAL_LINK_POD_DEFINED
#define IDEAM_NARRATIVES_CAUSAL_LINK_POD_DEFINED

/**
 * CausalLinkPOD (8-Byte Aligned)
 * Flattened child link entry for Causal_Condition trees (conditions + causes arrays in causal_condition.h).
 * Each Causal_Condition row in ConditionEvaluationTableBuffer owns a contiguous slice of CausalLinkPODs.
 */
struct alignas(8) CausalLinkPOD {
    uint32_t child_condition_id = INVALID_ID;      // 4B: Child row index in ConditionEvaluationTableBuffer
    int8_t required_cause_status= COND_STATUS_MET; // 1B: NarremeConditionStatusDOD required from child_condition_id
    uint8_t active              = 1u;              // 1B: Non-zero when link is active
    uint16_t reserved           = 0u;              // 2B -> Total = 8B
};

static_assert(sizeof(CausalLinkPOD) == 8, "CausalLinkPOD must be 8 bytes!");

#endif // IDEAM_NARRATIVES_CAUSAL_LINK_POD_DEFINED

/**
 * CausalConditionWavePropagateTransformLogic<T> (N6)
 * Evaluates hierarchical Causal_Condition trees (conditions + causes arrays) over BridgeView slices,
 * propagating CANNOT_MEET (-2) and PERMANENT (2) states across causal depths via ping-pong SwapView
 * wavefronts until convergence.
 *
 * Replaces recursive Causal_Condition::evaluate_condition() pointer-chasing in causal_condition.cpp.
 */
template <typename T = uint8_t>
struct alignas(64) CausalConditionWavePropagateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::BYTE | DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Causal Condition Wave Propagate";
    static constexpr std::string_view ui_category  = "Conditions";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t write_status_buffer_id         = INVALID_ID; // 4B: Primary output ConditionEvaluationTableBuffer::current_status
    uint32_t read_status_buffer_id          = INVALID_ID; // 4B: Optional SwapView read status buffer (aliases write if INVALID_ID)
    uint32_t condition_type_buffer_id       = INVALID_ID; // 4B: Optional ConditionEvaluationTableBuffer::condition_type (uint8_t)
    uint32_t causal_slice_offset_buffer_id  = INVALID_ID; // 4B: Per-condition slice start offset into CausalLinkPOD / child arrays
    uint32_t causal_slice_count_buffer_id   = INVALID_ID; // 4B: Per-condition slice count (uint16_t/uint32_t)
    uint32_t causal_link_pod_buffer_id      = INVALID_ID; // 4B: Contiguous CausalLinkPOD array buffer (8B per link)
    uint32_t child_condition_id_buffer_id   = INVALID_ID; // 4B: Optional SoA child_condition_id (uint32_t) slice buffer
    uint32_t expected_cause_status_buffer_id= INVALID_ID; // 4B: Optional SoA causes (int8_t/int32_t) slice buffer
    uint32_t transition_flags_buffer_id     = INVALID_ID; // 4B: Optional uint8_t ConditionStatusTransitionFlags column
    uint32_t changed_count_buffer_id        = INVALID_ID; // 4B: Optional uint32_t counter recording transitions across wave passes

    uint8_t max_wave_iterations             = 4u;         // 1B: Maximum wavefront passes until convergence (1..16)
    bool filter_causal_condition_type       = true;       // 1B: Only evaluate rows where condition_type == CONDITION_TYPE_CAUSAL
    bool propagate_cannot_meet              = true;       // 1B: Transition to CANNOT_MEET when a child is terminally incompatible
    bool propagate_permanent                = true;       // 1B: Promote to PERMANENT when all children are permanently satisfied
    bool treat_permanent_as_met             = true;       // 1B: Child status PERMANENT satisfies required_cause_status == MET
    bool preserve_terminal_states           = true;       // 1B: Skip Causal_Conditions already locked in CANNOT_MEET / PERMANENT
    bool wake_changed_conditions            = true;       // 1B: Queue changed condition indices into TaskContextPOD::wave_commands
    uint8_t reserved_padding[17]            = {0};        // 17B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary wst_prop;
        wst_prop["name"] = "write_status_buffer_id";
        wst_prop["type"] = godot::Variant::INT;
        wst_prop["hint_string"] = "buffer_option";
        props.push_back(wst_prop);

        godot::Dictionary rst_prop;
        rst_prop["name"] = "read_status_buffer_id";
        rst_prop["type"] = godot::Variant::INT;
        rst_prop["hint_string"] = "buffer_option";
        props.push_back(rst_prop);

        godot::Dictionary ctyp_prop;
        ctyp_prop["name"] = "condition_type_buffer_id";
        ctyp_prop["type"] = godot::Variant::INT;
        ctyp_prop["hint_string"] = "buffer_option";
        props.push_back(ctyp_prop);

        godot::Dictionary coff_prop;
        coff_prop["name"] = "causal_slice_offset_buffer_id";
        coff_prop["type"] = godot::Variant::INT;
        coff_prop["hint_string"] = "buffer_option";
        props.push_back(coff_prop);

        godot::Dictionary ccnt_prop;
        ccnt_prop["name"] = "causal_slice_count_buffer_id";
        ccnt_prop["type"] = godot::Variant::INT;
        ccnt_prop["hint_string"] = "buffer_option";
        props.push_back(ccnt_prop);

        godot::Dictionary cpod_prop;
        cpod_prop["name"] = "causal_link_pod_buffer_id";
        cpod_prop["type"] = godot::Variant::INT;
        cpod_prop["hint_string"] = "buffer_option";
        props.push_back(cpod_prop);

        godot::Dictionary chld_prop;
        chld_prop["name"] = "child_condition_id_buffer_id";
        chld_prop["type"] = godot::Variant::INT;
        chld_prop["hint_string"] = "buffer_option";
        props.push_back(chld_prop);

        godot::Dictionary exp_prop;
        exp_prop["name"] = "expected_cause_status_buffer_id";
        exp_prop["type"] = godot::Variant::INT;
        exp_prop["hint_string"] = "buffer_option";
        props.push_back(exp_prop);

        godot::Dictionary trn_prop;
        trn_prop["name"] = "transition_flags_buffer_id";
        trn_prop["type"] = godot::Variant::INT;
        trn_prop["hint_string"] = "buffer_option";
        props.push_back(trn_prop);

        godot::Dictionary chg_prop;
        chg_prop["name"] = "changed_count_buffer_id";
        chg_prop["type"] = godot::Variant::INT;
        chg_prop["hint_string"] = "buffer_option";
        props.push_back(chg_prop);

        godot::Dictionary itr_prop;
        itr_prop["name"] = "max_wave_iterations";
        itr_prop["type"] = godot::Variant::INT;
        itr_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        itr_prop["hint_string"] = "1,16,1";
        props.push_back(itr_prop);

        godot::Dictionary flt_prop;
        flt_prop["name"] = "filter_causal_condition_type";
        flt_prop["type"] = godot::Variant::BOOL;
        flt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(flt_prop);

        godot::Dictionary pcm_prop;
        pcm_prop["name"] = "propagate_cannot_meet";
        pcm_prop["type"] = godot::Variant::BOOL;
        pcm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(pcm_prop);

        godot::Dictionary ppr_prop;
        ppr_prop["name"] = "propagate_permanent";
        ppr_prop["type"] = godot::Variant::BOOL;
        ppr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ppr_prop);

        godot::Dictionary tpm_prop;
        tpm_prop["name"] = "treat_permanent_as_met";
        tpm_prop["type"] = godot::Variant::BOOL;
        tpm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(tpm_prop);

        godot::Dictionary pts_prop;
        pts_prop["name"] = "preserve_terminal_states";
        pts_prop["type"] = godot::Variant::BOOL;
        pts_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(pts_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_changed_conditions";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (write_status_buffer_id != INVALID_ID) return write_status_buffer_id;
        return read_status_buffer_id;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        (void)p_part;
        if constexpr (requires { p_view.bind_secondary(p_part); }) {
            const uint32_t sec_id = (read_status_buffer_id != INVALID_ID) ? read_status_buffer_id : write_status_buffer_id;
            if (sec_id != INVALID_ID) {
                p_view.bind_secondary(p_context.get_grant_part(sec_id));
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("write_status_buffer_id")) {
            write_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["write_status_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && write_status_buffer_id == INVALID_ID) {
            write_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("read_status_buffer_id")) {
            read_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["read_status_buffer_id"]));
        }
        if (p_props.has("condition_type_buffer_id")) {
            condition_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_type_buffer_id"]));
        }
        if (p_props.has("causal_slice_offset_buffer_id")) {
            causal_slice_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["causal_slice_offset_buffer_id"]));
        }
        if (p_props.has("causal_slice_count_buffer_id")) {
            causal_slice_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["causal_slice_count_buffer_id"]));
        }
        if (p_props.has("causal_link_pod_buffer_id")) {
            causal_link_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["causal_link_pod_buffer_id"]));
        }
        if (p_props.has("child_condition_id_buffer_id")) {
            child_condition_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_condition_id_buffer_id"]));
        }
        if (p_props.has("expected_cause_status_buffer_id")) {
            expected_cause_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["expected_cause_status_buffer_id"]));
        }
        if (p_props.has("transition_flags_buffer_id")) {
            transition_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transition_flags_buffer_id"]));
        }
        if (p_props.has("changed_count_buffer_id")) {
            changed_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["changed_count_buffer_id"]));
        }
        if (p_props.has("max_wave_iterations")) {
            max_wave_iterations = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_wave_iterations"]), 1, 32));
        }
        if (p_props.has("filter_causal_condition_type")) {
            filter_causal_condition_type = static_cast<bool>(p_props["filter_causal_condition_type"]);
        }
        if (p_props.has("propagate_cannot_meet")) {
            propagate_cannot_meet = static_cast<bool>(p_props["propagate_cannot_meet"]);
        }
        if (p_props.has("propagate_permanent")) {
            propagate_permanent = static_cast<bool>(p_props["propagate_permanent"]);
        }
        if (p_props.has("treat_permanent_as_met")) {
            treat_permanent_as_met = static_cast<bool>(p_props["treat_permanent_as_met"]);
        }
        if (p_props.has("preserve_terminal_states")) {
            preserve_terminal_states = static_cast<bool>(p_props["preserve_terminal_states"]);
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
        if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int32_t>) {
            const uint32_t wst_id = get_target_buffer_id();
            const GrantPartPOD* wst_part = (wst_id != INVALID_ID) ? p_context.get_grant_part(wst_id) : nullptr;
            if (!wst_part || !wst_part->raw_base_ptr) return;

            const GrantPartPOD* rst_part  = (read_status_buffer_id != INVALID_ID && read_status_buffer_id != wst_id)
                ? p_context.get_grant_part(read_status_buffer_id)
                : wst_part;
            const GrantPartPOD* ctyp_part = (condition_type_buffer_id != INVALID_ID)        ? p_context.get_grant_part(condition_type_buffer_id)        : nullptr;
            const GrantPartPOD* coff_part = (causal_slice_offset_buffer_id != INVALID_ID)   ? p_context.get_grant_part(causal_slice_offset_buffer_id)   : nullptr;
            const GrantPartPOD* ccnt_part = (causal_slice_count_buffer_id != INVALID_ID)    ? p_context.get_grant_part(causal_slice_count_buffer_id)    : nullptr;
            const GrantPartPOD* cpod_part = (causal_link_pod_buffer_id != INVALID_ID)       ? p_context.get_grant_part(causal_link_pod_buffer_id)       : nullptr;
            const GrantPartPOD* chld_part = (child_condition_id_buffer_id != INVALID_ID)    ? p_context.get_grant_part(child_condition_id_buffer_id)    : nullptr;
            const GrantPartPOD* exp_part  = (expected_cause_status_buffer_id != INVALID_ID) ? p_context.get_grant_part(expected_cause_status_buffer_id) : nullptr;
            const GrantPartPOD* trn_part  = (transition_flags_buffer_id != INVALID_ID)      ? p_context.get_grant_part(transition_flags_buffer_id)      : nullptr;
            const GrantPartPOD* chg_part  = (changed_count_buffer_id != INVALID_ID)         ? p_context.get_grant_part(changed_count_buffer_id)         : nullptr;

            uint8_t* __restrict wst_raw   = wst_part->raw_base_ptr;
            const uint32_t wst_stride     = (wst_part->element_stride > 0) ? wst_part->element_stride : sizeof(T);
            uint8_t* __restrict rst_raw   = (rst_part && rst_part->raw_base_ptr) ? rst_part->raw_base_ptr : wst_raw;
            const uint32_t rst_stride     = (rst_part && rst_part->element_stride > 0) ? rst_part->element_stride : wst_stride;

            const int64_t cond_cap        = static_cast<int64_t>(wst_part->capacity_bytes / wst_stride);
            const int64_t rst_cap         = static_cast<int64_t>(rst_part->capacity_bytes / rst_stride);
            if (cond_cap <= 0 || rst_cap <= 0) return;

            const uint8_t* __restrict ctyp_raw      = (ctyp_part && ctyp_part->raw_base_ptr) ? ctyp_part->raw_base_ptr : nullptr;
            const uint32_t ctyp_stride              = ctyp_part ? ctyp_part->element_stride : 0;
            const int64_t ctyp_cap                  = (ctyp_raw && ctyp_stride > 0) ? static_cast<int64_t>(ctyp_part->capacity_bytes / ctyp_stride) : 0;

            const uint32_t* __restrict slice_offsets= (coff_part && coff_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(coff_part->raw_base_ptr) : nullptr;
            const int64_t coff_cap                  = slice_offsets ? static_cast<int64_t>(coff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict ccnt_raw      = (ccnt_part && ccnt_part->raw_base_ptr) ? ccnt_part->raw_base_ptr : nullptr;
            const uint32_t ccnt_stride              = ccnt_part ? ccnt_part->element_stride : 0;
            const int64_t ccnt_cap                  = (ccnt_raw && ccnt_stride > 0) ? static_cast<int64_t>(ccnt_part->capacity_bytes / ccnt_stride) : 0;

            const CausalLinkPOD* __restrict links   = (cpod_part && cpod_part->raw_base_ptr) ? reinterpret_cast<const CausalLinkPOD*>(cpod_part->raw_base_ptr) : nullptr;
            const int64_t cpod_cap                  = links ? static_cast<int64_t>(cpod_part->capacity_bytes / sizeof(CausalLinkPOD)) : 0;

            const uint32_t* __restrict child_ids    = (chld_part && chld_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(chld_part->raw_base_ptr) : nullptr;
            const int64_t chld_cap                  = child_ids ? static_cast<int64_t>(chld_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict exp_raw       = (exp_part && exp_part->raw_base_ptr)   ? exp_part->raw_base_ptr : nullptr;
            const uint32_t exp_stride               = exp_part ? exp_part->element_stride : 0;
            const int64_t exp_cap                   = (exp_raw && exp_stride > 0) ? static_cast<int64_t>(exp_part->capacity_bytes / exp_stride) : 0;

            uint8_t* __restrict trn_raw             = (trn_part && trn_part->raw_base_ptr)   ? trn_part->raw_base_ptr : nullptr;
            const uint32_t trn_stride               = trn_part ? trn_part->element_stride : 0;
            const int64_t trn_cap                   = (trn_raw && trn_stride > 0) ? static_cast<int64_t>(trn_part->capacity_bytes / trn_stride) : 0;

            uint32_t* __restrict changed_counter    = (chg_part && chg_part->raw_base_ptr)   ? reinterpret_cast<uint32_t*>(chg_part->raw_base_ptr) : nullptr;

            auto read_st = [](const uint8_t* raw, uint32_t stride, int64_t idx) noexcept -> int8_t {
                if (stride == sizeof(int32_t)) {
                    return static_cast<int8_t>(reinterpret_cast<const int32_t*>(raw)[idx]);
                }
                return reinterpret_cast<const int8_t*>(raw)[idx];
            };

            auto write_st = [](uint8_t* raw, uint32_t stride, int64_t idx, int8_t val) noexcept {
                if (stride == sizeof(int32_t)) {
                    reinterpret_cast<int32_t*>(raw)[idx] = static_cast<int32_t>(val);
                } else {
                    reinterpret_cast<int8_t*>(raw)[idx] = val;
                }
            };

            auto read_slice_count = [&](int64_t idx) noexcept -> uint32_t {
                if (!ccnt_raw || idx < 0 || idx >= ccnt_cap) return 0u;
                if (ccnt_stride == sizeof(uint32_t)) {
                    return reinterpret_cast<const uint32_t*>(ccnt_raw)[idx];
                }
                return static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(ccnt_raw)[idx]);
            };

            auto read_expected_cause = [&](int64_t lidx) noexcept -> int8_t {
                if (!exp_raw || lidx < 0 || lidx >= exp_cap) return COND_STATUS_MET;
                if (exp_stride == sizeof(int32_t)) {
                    return static_cast<int8_t>(reinterpret_cast<const int32_t*>(exp_raw)[lidx]);
                }
                return reinterpret_cast<const int8_t*>(exp_raw)[lidx];
            };

            const uint8_t max_passes = std::max<uint8_t>(1u, max_wave_iterations);
            uint32_t total_changed   = 0u;

            for (uint8_t pass = 0u; pass < max_passes; ++pass) {
                uint32_t pass_changed = 0u;

                auto eval_causal = [&](int64_t idx) noexcept {
                    if (filter_causal_condition_type && ctyp_raw && idx < ctyp_cap) {
                        const uint8_t ctype = (ctyp_stride == sizeof(int32_t))
                            ? static_cast<uint8_t>(reinterpret_cast<const int32_t*>(ctyp_raw)[idx])
                            : ctyp_raw[idx];
                        if (ctype != CONDITION_TYPE_CAUSAL) return;
                    }

                    const int8_t cur_st = read_st(wst_raw, wst_stride, idx);
                    if (preserve_terminal_states &&
                        (cur_st == COND_STATUS_CANNOT_MEET || cur_st == COND_STATUS_PERMANENT)) {
                        return;
                    }

                    if (!slice_offsets || idx >= coff_cap) return;
                    const int64_t slice_off = static_cast<int64_t>(slice_offsets[idx]);
                    const int64_t slice_cnt = static_cast<int64_t>(read_slice_count(idx));
                    if (slice_cnt <= 0) return;

                    // After pass 0 in dual-buffer SwapView mode, read latest converged values from wst_raw
                    const uint8_t* active_read_raw = (pass == 0u) ? rst_raw : wst_raw;
                    const uint32_t active_read_str = (pass == 0u) ? rst_stride : wst_stride;
                    const int64_t active_read_cap  = (pass == 0u) ? rst_cap : cond_cap;

                    int8_t new_status        = COND_STATUS_MET;
                    bool any_cannot_meet     = false;
                    bool all_children_perm   = true;
                    int64_t valid_link_count = 0;

                    for (int64_t k = 0; k < slice_cnt; ++k) {
                        const int64_t link_idx = slice_off + k;
                        uint32_t child_id = INVALID_ID;
                        int8_t req_cause  = COND_STATUS_MET;

                        if (links && link_idx < cpod_cap) {
                            const CausalLinkPOD& lnk = links[link_idx];
                            if (lnk.active == 0u) continue;
                            child_id  = lnk.child_condition_id;
                            req_cause = lnk.required_cause_status;
                        } else if (child_ids && link_idx < chld_cap) {
                            child_id  = child_ids[link_idx];
                            req_cause = read_expected_cause(link_idx);
                        }

                        // Skip self-referential cycle (mirrors `if (condition.ptr() == this) continue;` in causal_condition.cpp)
                        if (child_id == INVALID_ID || static_cast<int64_t>(child_id) == idx) continue;
                        const int64_t cidx = static_cast<int64_t>(child_id);
                        if (cidx < 0 || cidx >= active_read_cap) continue;

                        ++valid_link_count;
                        const int8_t child_st = read_st(active_read_raw, active_read_str, cidx);

                        const bool cause_matched = (child_st == req_cause) ||
                            (treat_permanent_as_met && req_cause == COND_STATUS_MET && child_st == COND_STATUS_PERMANENT);

                        const bool child_is_terminal =
                            (child_st == COND_STATUS_CANNOT_MEET || child_st == COND_STATUS_PERMANENT);

                        if (!cause_matched) {
                            new_status      = COND_STATUS_NOT_MET;
                            all_children_perm = false;
                            if (propagate_cannot_meet && child_is_terminal) {
                                any_cannot_meet = true;
                            }
                        } else if (!child_is_terminal) {
                            all_children_perm = false;
                        }
                    }

                    if (any_cannot_meet) {
                        new_status = COND_STATUS_CANNOT_MEET;
                    } else if (new_status == COND_STATUS_MET && propagate_permanent && valid_link_count > 0 && all_children_perm) {
                        new_status = COND_STATUS_PERMANENT;
                    }

                    if (new_status != cur_st) {
                        write_st(wst_raw, wst_stride, idx, new_status);
                        ++pass_changed;

                        if (trn_raw && idx < trn_cap) {
                            uint8_t tflag = COND_TRANSITION_NONE;
                            if ((new_status == COND_STATUS_MET || new_status == COND_STATUS_PERMANENT) && cur_st == COND_STATUS_NOT_MET) {
                                tflag = COND_TRANSITION_MET;
                            } else if (new_status == COND_STATUS_NOT_MET && cur_st == COND_STATUS_MET) {
                                tflag = COND_TRANSITION_UNMET;
                            } else if (new_status == COND_STATUS_CANNOT_MEET && (cur_st == COND_STATUS_NOT_MET || cur_st == COND_STATUS_MET)) {
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
                            p_context.queue_selection_command(wst_part->buffer_id, idx);
                        }
                    }
                };

                _iterate_selection(wst_part->selection, cond_cap, eval_causal);
                total_changed += pass_changed;
                if (pass_changed == 0u) break; // Wavefront converged
            }

            if (changed_counter) {
                *changed_counter = total_changed;
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

static_assert(sizeof(CausalConditionWavePropagateTransformLogic<>) == 64, "CausalConditionWavePropagateTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core