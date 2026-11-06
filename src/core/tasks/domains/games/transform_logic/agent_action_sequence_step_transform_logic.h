#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "entity_time_scale_cascade_transform_logic.h"
#include "piece_action_score_refresh_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_SEQUENCE_STEP_POD_DEFINED
#define IDEAM_GAMES_SEQUENCE_STEP_POD_DEFINED

/**
 * SequenceStepPOD
 * Flat 8-byte POD replacing nested Array[piece_id, piece_action_id] pairs inside GameAgentAction::sequence.
 * Defined per Section 2.2 Buffer 4 (ActionSequenceBuffer) of the Games Tasks Bridge specification.
 */
struct SequenceStepPOD {
    uint32_t piece_idx        = INVALID_ID; // 4B: Target GamePiece index in EntityHierarchyBuffer
    uint32_t piece_action_idx = INVALID_ID; // 4B: Target GamePieceAction index in ActionSequenceBuffer
};

static_assert(sizeof(SequenceStepPOD) == 8, "SequenceStepPOD must be 8 bytes!");

/**
 * Operations supported by AgentActionSequenceStepTransformLogic.
 */
enum AgentSequenceStepOp : uint8_t {
    STEP_OP_EVALUATE_CURRENT = 0, // Aggregates active step score (and auto-advances if step completed)
    STEP_OP_ADVANCE_NEXT     = 1, // Advances current_step_idx + 1 (mirrors GameAgentAction::next_in_sequence)
    STEP_OP_STEP_BACKWARD    = 2, // Moves current_step_idx - 1 (mirrors GameAgentAction::previous_in_sequence)
    STEP_OP_START_SEQUENCE   = 3, // Starts sequence at step 0 (sets status = ACTION_STATUS_IN_USE)
    STEP_OP_RESET_SEQUENCE   = 4  // Ends/resets sequence (sets current_step_idx = -1, status = ACTION_STATUS_AVAILABLE)
};

#endif // IDEAM_GAMES_SEQUENCE_STEP_POD_DEFINED

/**
 * AgentActionSequenceStepTransformLogic<T> (G7)
 * Advances GameAgentAction step indices (SequenceStepPOD), aggregates sub-piece action scores into
 * the parent GameAgentAction's current_score, and triggers step transitions without nested Array allocations.
 * Replaces GameAgentAction::next_in_sequence(), previous_in_sequence(), start_action(), and get_current_value().
 */
template <typename T = int32_t>
struct alignas(64) AgentActionSequenceStepTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::CUSTOM;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Agent Action Sequence Step";
    static constexpr std::string_view ui_category  = "Actions";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t agent_current_score_buffer_id = INVALID_ID;              // 4B: Primary target (ActionSequenceBuffer::current_score int32_t)
    uint32_t agent_base_score_buffer_id    = INVALID_ID;              // 4B: ActionSequenceBuffer::base_score (int32_t)
    uint32_t agent_status_buffer_id        = INVALID_ID;              // 4B: ActionSequenceBuffer::status (int32_t)
    uint32_t current_step_idx_buffer_id    = INVALID_ID;              // 4B: ActionSequenceBuffer::current_step_idx (int16_t/int32_t)
    uint32_t step_offset_buffer_id         = INVALID_ID;              // 4B: ActionSequenceBuffer::sequence_step_offset (uint32_t)
    uint32_t step_count_buffer_id          = INVALID_ID;              // 4B: ActionSequenceBuffer::sequence_step_count (uint16_t/uint32_t)
    uint32_t sequence_step_pod_buffer_id   = INVALID_ID;              // 4B: Contiguous SequenceStepPOD array buffer
    uint32_t piece_action_score_buffer_id  = INVALID_ID;              // 4B: Sub-piece ActionSequenceBuffer::current_score (int32_t)
    uint32_t piece_action_status_buffer_id = INVALID_ID;              // 4B: Optional sub-piece ActionSequenceBuffer::status (int32_t)
    uint32_t piece_enabled_flags_buffer_id = INVALID_ID;              // 4B: Optional EntityHierarchyBuffer::active_flags (uint32_t)

    uint8_t step_operation                 = STEP_OP_EVALUATE_CURRENT;// 1B: AgentSequenceStepOp
    bool aggregate_all_steps               = false;                   // 1B: Sum all steps in slice vs. current active step only
    bool auto_advance_on_step_complete     = true;                    // 1B: Advance when current piece action finishes IN_USE
    bool loop_sequence                     = false;                   // 1B: Wrap to step 0 instead of completing on final step
    bool skip_disabled_pieces              = true;                    // 1B: Skip steps whose piece lacks ENTITY_FLAG_ENABLED
    uint8_t reserved_padding[19]           = {0};                     // 19B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary cscr_prop;
        cscr_prop["name"] = "agent_current_score_buffer_id";
        cscr_prop["type"] = godot::Variant::INT;
        cscr_prop["hint_string"] = "buffer_option";
        props.push_back(cscr_prop);

        godot::Dictionary bscr_prop;
        bscr_prop["name"] = "agent_base_score_buffer_id";
        bscr_prop["type"] = godot::Variant::INT;
        bscr_prop["hint_string"] = "buffer_option";
        props.push_back(bscr_prop);

        godot::Dictionary astat_prop;
        astat_prop["name"] = "agent_status_buffer_id";
        astat_prop["type"] = godot::Variant::INT;
        astat_prop["hint_string"] = "buffer_option";
        props.push_back(astat_prop);

        godot::Dictionary cstp_prop;
        cstp_prop["name"] = "current_step_idx_buffer_id";
        cstp_prop["type"] = godot::Variant::INT;
        cstp_prop["hint_string"] = "buffer_option";
        props.push_back(cstp_prop);

        godot::Dictionary soff_prop;
        soff_prop["name"] = "step_offset_buffer_id";
        soff_prop["type"] = godot::Variant::INT;
        soff_prop["hint_string"] = "buffer_option";
        props.push_back(soff_prop);

        godot::Dictionary scnt_prop;
        scnt_prop["name"] = "step_count_buffer_id";
        scnt_prop["type"] = godot::Variant::INT;
        scnt_prop["hint_string"] = "buffer_option";
        props.push_back(scnt_prop);

        godot::Dictionary spod_prop;
        spod_prop["name"] = "sequence_step_pod_buffer_id";
        spod_prop["type"] = godot::Variant::INT;
        spod_prop["hint_string"] = "buffer_option";
        props.push_back(spod_prop);

        godot::Dictionary pscr_prop;
        pscr_prop["name"] = "piece_action_score_buffer_id";
        pscr_prop["type"] = godot::Variant::INT;
        pscr_prop["hint_string"] = "buffer_option";
        props.push_back(pscr_prop);

        godot::Dictionary pstat_prop;
        pstat_prop["name"] = "piece_action_status_buffer_id";
        pstat_prop["type"] = godot::Variant::INT;
        pstat_prop["hint_string"] = "buffer_option";
        props.push_back(pstat_prop);

        godot::Dictionary pflg_prop;
        pflg_prop["name"] = "piece_enabled_flags_buffer_id";
        pflg_prop["type"] = godot::Variant::INT;
        pflg_prop["hint_string"] = "buffer_option";
        props.push_back(pflg_prop);

        godot::Dictionary op_prop;
        op_prop["name"] = "step_operation";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Evaluate Current:0,Advance Next:1,Step Backward:2,Start Sequence:3,Reset Sequence:4";
        props.push_back(op_prop);

        godot::Dictionary agg_prop;
        agg_prop["name"] = "aggregate_all_steps";
        agg_prop["type"] = godot::Variant::BOOL;
        agg_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(agg_prop);

        godot::Dictionary adv_prop;
        adv_prop["name"] = "auto_advance_on_step_complete";
        adv_prop["type"] = godot::Variant::BOOL;
        adv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(adv_prop);

        godot::Dictionary loop_prop;
        loop_prop["name"] = "loop_sequence";
        loop_prop["type"] = godot::Variant::BOOL;
        loop_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(loop_prop);

        godot::Dictionary skp_prop;
        skp_prop["name"] = "skip_disabled_pieces";
        skp_prop["type"] = godot::Variant::BOOL;
        skp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(skp_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (agent_current_score_buffer_id != INVALID_ID) return agent_current_score_buffer_id;
        if (current_step_idx_buffer_id != INVALID_ID)    return current_step_idx_buffer_id;
        return sequence_step_pod_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("agent_current_score_buffer_id")) {
            agent_current_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_current_score_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && agent_current_score_buffer_id == INVALID_ID) {
            agent_current_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("agent_base_score_buffer_id")) {
            agent_base_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_base_score_buffer_id"]));
        }
        if (p_props.has("agent_status_buffer_id")) {
            agent_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_status_buffer_id"]));
        }
        if (p_props.has("current_step_idx_buffer_id")) {
            current_step_idx_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["current_step_idx_buffer_id"]));
        }
        if (p_props.has("step_offset_buffer_id")) {
            step_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["step_offset_buffer_id"]));
        }
        if (p_props.has("step_count_buffer_id")) {
            step_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["step_count_buffer_id"]));
        }
        if (p_props.has("sequence_step_pod_buffer_id")) {
            sequence_step_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["sequence_step_pod_buffer_id"]));
        }
        if (p_props.has("piece_action_score_buffer_id")) {
            piece_action_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["piece_action_score_buffer_id"]));
        }
        if (p_props.has("piece_action_status_buffer_id")) {
            piece_action_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["piece_action_status_buffer_id"]));
        }
        if (p_props.has("piece_enabled_flags_buffer_id")) {
            piece_enabled_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["piece_enabled_flags_buffer_id"]));
        }
        if (p_props.has("step_operation")) {
            step_operation = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["step_operation"]), 0, 4));
        }
        if (p_props.has("aggregate_all_steps")) {
            aggregate_all_steps = static_cast<bool>(p_props["aggregate_all_steps"]);
        }
        if (p_props.has("auto_advance_on_step_complete")) {
            auto_advance_on_step_complete = static_cast<bool>(p_props["auto_advance_on_step_complete"]);
        }
        if (p_props.has("loop_sequence")) {
            loop_sequence = static_cast<bool>(p_props["loop_sequence"]);
        }
        if (p_props.has("skip_disabled_pieces")) {
            skip_disabled_pieces = static_cast<bool>(p_props["skip_disabled_pieces"]);
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
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, void*>) {
            const GrantPartPOD* cscr_part  = (agent_current_score_buffer_id != INVALID_ID) ? p_context.get_grant_part(agent_current_score_buffer_id) : nullptr;
            const GrantPartPOD* bscr_part  = (agent_base_score_buffer_id != INVALID_ID)    ? p_context.get_grant_part(agent_base_score_buffer_id)    : nullptr;
            const GrantPartPOD* astat_part = (agent_status_buffer_id != INVALID_ID)        ? p_context.get_grant_part(agent_status_buffer_id)        : nullptr;
            const GrantPartPOD* cstp_part  = (current_step_idx_buffer_id != INVALID_ID)    ? p_context.get_grant_part(current_step_idx_buffer_id)    : nullptr;
            const GrantPartPOD* soff_part  = (step_offset_buffer_id != INVALID_ID)         ? p_context.get_grant_part(step_offset_buffer_id)         : nullptr;
            const GrantPartPOD* scnt_part  = (step_count_buffer_id != INVALID_ID)          ? p_context.get_grant_part(step_count_buffer_id)          : nullptr;
            const GrantPartPOD* spod_part  = (sequence_step_pod_buffer_id != INVALID_ID)   ? p_context.get_grant_part(sequence_step_pod_buffer_id)   : nullptr;
            const GrantPartPOD* pscr_part  = (piece_action_score_buffer_id != INVALID_ID)  ? p_context.get_grant_part(piece_action_score_buffer_id)  : nullptr;
            const GrantPartPOD* pstat_part = (piece_action_status_buffer_id != INVALID_ID) ? p_context.get_grant_part(piece_action_status_buffer_id) : nullptr;
            const GrantPartPOD* pflg_part  = (piece_enabled_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(piece_enabled_flags_buffer_id) : nullptr;

            const GrantPartPOD* primary_part = cscr_part ? cscr_part : cstp_part;
            if (!primary_part || !primary_part->raw_base_ptr) return;

            int32_t* __restrict agent_scores          = (cscr_part && cscr_part->raw_base_ptr)   ? reinterpret_cast<int32_t*>(cscr_part->raw_base_ptr)         : nullptr;
            const int32_t* __restrict base_scores     = (bscr_part && bscr_part->raw_base_ptr)   ? reinterpret_cast<const int32_t*>(bscr_part->raw_base_ptr)   : nullptr;
            int32_t* __restrict agent_statuses        = (astat_part && astat_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(astat_part->raw_base_ptr)       : nullptr;
            uint8_t* __restrict cstp_raw              = (cstp_part && cstp_part->raw_base_ptr)   ? cstp_part->raw_base_ptr                                     : nullptr;
            const uint32_t cstp_stride                = cstp_part ? cstp_part->element_stride : 0;
            const uint32_t* __restrict step_offsets   = (soff_part && soff_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(soff_part->raw_base_ptr)   : nullptr;
            const uint8_t* __restrict scnt_raw        = (scnt_part && scnt_part->raw_base_ptr)   ? scnt_part->raw_base_ptr                                     : nullptr;
            const uint32_t scnt_stride                = scnt_part ? scnt_part->element_stride : 0;
            const SequenceStepPOD* __restrict steps   = (spod_part && spod_part->raw_base_ptr)   ? reinterpret_cast<const SequenceStepPOD*>(spod_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict piece_scores    = (pscr_part && pscr_part->raw_base_ptr)   ? reinterpret_cast<const int32_t*>(pscr_part->raw_base_ptr)   : nullptr;
            int32_t* __restrict piece_statuses        = (pstat_part && pstat_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(pstat_part->raw_base_ptr)       : nullptr;
            const uint32_t* __restrict piece_flags    = (pflg_part && pflg_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(pflg_part->raw_base_ptr)   : nullptr;

            const int64_t step_pod_cap   = steps        ? static_cast<int64_t>(spod_part->capacity_bytes / sizeof(SequenceStepPOD)) : 0;
            const int64_t pscr_cap       = piece_scores ? static_cast<int64_t>(pscr_part->capacity_bytes / sizeof(int32_t))         : 0;
            const int64_t pstat_cap      = piece_statuses ? static_cast<int64_t>(pstat_part->capacity_bytes / sizeof(int32_t))      : 0;
            const int64_t pflg_cap       = piece_flags  ? static_cast<int64_t>(pflg_part->capacity_bytes / sizeof(uint32_t))        : 0;

            int64_t agent_action_count = static_cast<int64_t>(primary_part->capacity_bytes / (primary_part->element_stride > 0 ? primary_part->element_stride : sizeof(int32_t)));
            if (cscr_part)  agent_action_count = std::min<int64_t>(agent_action_count, static_cast<int64_t>(cscr_part->capacity_bytes / sizeof(int32_t)));
            if (bscr_part)  agent_action_count = std::min<int64_t>(agent_action_count, static_cast<int64_t>(bscr_part->capacity_bytes / sizeof(int32_t)));
            if (astat_part) agent_action_count = std::min<int64_t>(agent_action_count, static_cast<int64_t>(astat_part->capacity_bytes / sizeof(int32_t)));
            if (cstp_part && cstp_stride > 0) agent_action_count = std::min<int64_t>(agent_action_count, static_cast<int64_t>(cstp_part->capacity_bytes / cstp_stride));
            if (soff_part)  agent_action_count = std::min<int64_t>(agent_action_count, static_cast<int64_t>(soff_part->capacity_bytes / sizeof(uint32_t)));
            if (scnt_part && scnt_stride > 0) agent_action_count = std::min<int64_t>(agent_action_count, static_cast<int64_t>(scnt_part->capacity_bytes / scnt_stride));
            if (agent_action_count <= 0) return;

            auto read_step_idx = [&](int64_t idx) noexcept -> int32_t {
                if (!cstp_raw) return 0;
                if (cstp_stride == sizeof(int16_t)) {
                    return static_cast<int32_t>(reinterpret_cast<const int16_t*>(cstp_raw)[idx]);
                }
                return reinterpret_cast<const int32_t*>(cstp_raw)[idx];
            };

            auto write_step_idx = [&](int64_t idx, int32_t val) noexcept {
                if (!cstp_raw) return;
                if (cstp_stride == sizeof(int16_t)) {
                    reinterpret_cast<int16_t*>(cstp_raw)[idx] = static_cast<int16_t>(std::clamp<int32_t>(val, -32768, 32767));
                } else {
                    reinterpret_cast<int32_t*>(cstp_raw)[idx] = val;
                }
            };

            auto read_step_count = [&](int64_t idx) noexcept -> int32_t {
                if (!scnt_raw) return 0;
                if (scnt_stride == sizeof(uint16_t)) {
                    return static_cast<int32_t>(reinterpret_cast<const uint16_t*>(scnt_raw)[idx]);
                }
                return static_cast<int32_t>(reinterpret_cast<const uint32_t*>(scnt_raw)[idx]);
            };

            auto is_step_piece_enabled = [&](const SequenceStepPOD& st) noexcept -> bool {
                if (!skip_disabled_pieces || !piece_flags) return true;
                const int64_t pidx = static_cast<int64_t>(st.piece_idx);
                if (pidx < 0 || pidx >= pflg_cap) return true;
                return (piece_flags[pidx] & ENTITY_FLAG_ENABLED) != 0u;
            };

            auto eval_agent_action = [&](int64_t idx) noexcept {
                const int32_t base_val = base_scores ? base_scores[idx] : 0;
                const int32_t step_cnt = read_step_count(idx);
                const int64_t slice_off = step_offsets ? static_cast<int64_t>(step_offsets[idx]) : 0;
                int32_t cur_step = read_step_idx(idx);
                int32_t a_status = agent_statuses ? agent_statuses[idx] : static_cast<int32_t>(ACTION_STATUS_IN_USE);

                // 1. Handle explicit sequence lifecycle operations
                if (step_operation == STEP_OP_RESET_SEQUENCE) {
                    write_step_idx(idx, -1);
                    if (agent_statuses) agent_statuses[idx] = ACTION_STATUS_AVAILABLE;
                    if (agent_scores)   agent_scores[idx]   = base_val;
                    return;
                }

                if (step_operation == STEP_OP_START_SEQUENCE) {
                    cur_step = (step_cnt > 0) ? 0 : -1;
                    a_status = (step_cnt > 0) ? ACTION_STATUS_IN_USE : ACTION_STATUS_AVAILABLE;
                    if (steps && cur_step == 0 && (slice_off < step_pod_cap)) {
                        const int64_t pa_idx = static_cast<int64_t>(steps[slice_off].piece_action_idx);
                        if (piece_statuses && pa_idx >= 0 && pa_idx < pstat_cap) {
                            piece_statuses[pa_idx] = ACTION_STATUS_IN_USE;
                        }
                    }
                } else if (step_operation == STEP_OP_ADVANCE_NEXT && a_status == ACTION_STATUS_IN_USE) {
                    // Mark outgoing step refreshing
                    if (steps && cur_step >= 0 && cur_step < step_cnt && (slice_off + cur_step < step_pod_cap)) {
                        const int64_t prev_pa = static_cast<int64_t>(steps[slice_off + cur_step].piece_action_idx);
                        if (piece_statuses && prev_pa >= 0 && prev_pa < pstat_cap) {
                            piece_statuses[prev_pa] = ACTION_STATUS_REFRESHING;
                        }
                    }
                    ++cur_step;
                    if (cur_step >= step_cnt) {
                        if (loop_sequence && step_cnt > 0) {
                            cur_step = 0;
                        } else {
                            cur_step = -1;
                            a_status = ACTION_STATUS_REFRESHING;
                        }
                    }
                    if (steps && cur_step >= 0 && cur_step < step_cnt && (slice_off + cur_step < step_pod_cap)) {
                        const int64_t next_pa = static_cast<int64_t>(steps[slice_off + cur_step].piece_action_idx);
                        if (piece_statuses && next_pa >= 0 && next_pa < pstat_cap) {
                            piece_statuses[next_pa] = ACTION_STATUS_IN_USE;
                        }
                    }
                } else if (step_operation == STEP_OP_STEP_BACKWARD && a_status == ACTION_STATUS_IN_USE) {
                    cur_step = (step_cnt > 0) ? std::max<int32_t>(0, cur_step - 1) : -1;
                } else if (step_operation == STEP_OP_EVALUATE_CURRENT && auto_advance_on_step_complete &&
                           a_status == ACTION_STATUS_IN_USE && steps && piece_statuses &&
                           cur_step >= 0 && cur_step < step_cnt && (slice_off + cur_step < step_pod_cap)) {
                    const int64_t cur_pa = static_cast<int64_t>(steps[slice_off + cur_step].piece_action_idx);
                    if (cur_pa >= 0 && cur_pa < pstat_cap && piece_statuses[cur_pa] != ACTION_STATUS_IN_USE) {
                        ++cur_step;
                        if (cur_step >= step_cnt) {
                            if (loop_sequence && step_cnt > 0) {
                                cur_step = 0;
                            } else {
                                cur_step = -1;
                                a_status = ACTION_STATUS_REFRESHING;
                            }
                        }
                        if (cur_step >= 0 && cur_step < step_cnt && (slice_off + cur_step < step_pod_cap)) {
                            const int64_t next_pa = static_cast<int64_t>(steps[slice_off + cur_step].piece_action_idx);
                            if (next_pa >= 0 && next_pa < pstat_cap) {
                                piece_statuses[next_pa] = ACTION_STATUS_IN_USE;
                            }
                        }
                    }
                }

                write_step_idx(idx, cur_step);
                if (agent_statuses) {
                    agent_statuses[idx] = a_status;
                }

                // 2. Compute aggregated agent action score (mirrors GameAgentAction::get_current_value)
                if (agent_scores) {
                    int64_t total_score = static_cast<int64_t>(base_val);
                    if (steps && piece_scores && step_cnt > 0 && a_status == ACTION_STATUS_IN_USE) {
                        if (aggregate_all_steps) {
                            const int64_t end_s = std::min<int64_t>(slice_off + step_cnt, step_pod_cap);
                            for (int64_t s = slice_off; s < end_s; ++s) {
                                const SequenceStepPOD& st = steps[s];
                                if (!is_step_piece_enabled(st)) continue;
                                const int64_t pa_idx = static_cast<int64_t>(st.piece_action_idx);
                                if (pa_idx >= 0 && pa_idx < pscr_cap) {
                                    total_score += static_cast<int64_t>(piece_scores[pa_idx]);
                                }
                            }
                        } else if (cur_step >= 0 && cur_step < step_cnt && (slice_off + cur_step < step_pod_cap)) {
                            const SequenceStepPOD& st = steps[slice_off + cur_step];
                            if (is_step_piece_enabled(st)) {
                                const int64_t pa_idx = static_cast<int64_t>(st.piece_action_idx);
                                if (pa_idx >= 0 && pa_idx < pscr_cap) {
                                    total_score += static_cast<int64_t>(piece_scores[pa_idx]);
                                }
                            }
                        }
                    }
                    agent_scores[idx] = static_cast<int32_t>(std::clamp<int64_t>(total_score, INT32_MIN, INT32_MAX));
                }
            };

            _iterate_selection(primary_part->selection, agent_action_count, eval_agent_action);
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

static_assert(sizeof(AgentActionSequenceStepTransformLogic<>) == 64, "AgentActionSequenceStepTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core