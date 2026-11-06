#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_CONSEQUENCE_COMMAND_POD_DEFINED
#define IDEAM_GAMES_CONSEQUENCE_COMMAND_POD_DEFINED

/**
 * Participant roles within an InteractionArenaBuffer slice.
 */
enum InteractionParticipantRole : uint8_t {
    PARTICIPANT_ROLE_INSTIGATOR  = 0,
    PARTICIPANT_ROLE_COOPERATION = 1,
    PARTICIPANT_ROLE_COMPETITION = 2
};

/**
 * Bitmask flags for ConsequenceCommandPOD::consequence_mask.
 */
enum ConsequenceMaskFlags : uint32_t {
    CONSEQUENCE_FLAG_NONE              = 0u,
    CONSEQUENCE_FLAG_VICTORY           = 1u << 0, // Margin of victory (> 0)
    CONSEQUENCE_FLAG_FAILURE           = 1u << 1, // Margin of failure (< 0)
    CONSEQUENCE_FLAG_TIE               = 1u << 2, // Score diff == 0
    CONSEQUENCE_FLAG_INSTIGATOR_TEAM   = 1u << 3, // Instigator or Cooperation participant
    CONSEQUENCE_FLAG_COMPETITION_TEAM  = 1u << 4, // Competition participant
    CONSEQUENCE_FLAG_APPLY_TO_PROPERTY = 1u << 5, // Apply score_delta directly to target property
    CONSEQUENCE_FLAG_EMIT_SIGNAL       = 1u << 6  // Emit action_consequences on target GameEntity
};

/**
 * InteractionParticipantPOD
 * Flat 16-byte POD representing an instigator, cooperator, or competitor in an interaction slice.
 */
struct alignas(16) InteractionParticipantPOD {
    uint32_t agent_entity_idx  = INVALID_ID;                 // 4B: Participant GameAgent / GameEntity index
    uint32_t action_idx        = INVALID_ID;                 // 4B: Bound GameAgentAction index in ActionSequenceBuffer
    float weight_multiplier    = 1.0f;                       // 4B: Per-participant GameplayRule INTERACTION_WEIGHT multiplier
    uint8_t role               = PARTICIPANT_ROLE_INSTIGATOR;// 1B: InteractionParticipantRole
    uint8_t active             = 1u;                         // 1B: Non-zero when participant is active
    uint16_t reserved          = 0u;                         // 2B -> Total = 16B
};

static_assert(sizeof(InteractionParticipantPOD) == 16, "InteractionParticipantPOD must be 16 bytes!");

/**
 * ConsequenceCommandPOD
 * Flat 12-byte command queued into a RingStrategy buffer by InteractionTugOfWarResolveTransformLogic (G8)
 * and drained at frame end by ConsequenceRingDrainTransformLogic (G12).
 * Defined per Section 2.2 Buffer 5 (InteractionArenaBuffer) of the Games Tasks Bridge specification.
 */
struct ConsequenceCommandPOD {
    uint32_t target_entity_idx = INVALID_ID; // 4B: Target GameEntity / GameProperty index
    int32_t score_delta        = 0;          // 4B: Signed margin of victory/failure or property delta
    uint32_t consequence_mask  = 0u;         // 4B: ConsequenceMaskFlags bitmask -> Total = 12B
};

static_assert(sizeof(ConsequenceCommandPOD) == 12, "ConsequenceCommandPOD must be 12 bytes!");

#endif // IDEAM_GAMES_CONSEQUENCE_COMMAND_POD_DEFINED

/**
 * InteractionTugOfWarResolveTransformLogic<T> (G8)
 * Sums (instigator + cooperation) vs. (competition) participant scores weighted by GameplayRule
 * INTERACTION_WEIGHT multipliers, writes score_diff into InteractionArenaBuffer, and pushes
 * ConsequenceCommandPOD entries into the consequence ring buffer.
 * Replaces GameInteraction::evaluate_interaction(), GameplayRule::compute_weighted_interaction_score(),
 * and GameplayRule::scale_consequence_score().
 */
template <typename T = int32_t>
struct alignas(64) InteractionTugOfWarResolveTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = BridgeView<T, T, 1, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    =
        BufferLayoutType::FLAT | BufferLayoutType::SOA |
        BufferLayoutType::AOS  | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Interaction Tug Of War Resolve";
    static constexpr std::string_view ui_category  = "Interactions";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t score_diff_buffer_id           = INVALID_ID; // 4B: Primary output (InteractionArenaBuffer::score_diff int32_t)
    uint32_t instigator_score_buffer_id     = INVALID_ID; // 4B: InteractionArenaBuffer::instigator_total_score (int32_t)
    uint32_t competition_score_buffer_id    = INVALID_ID; // 4B: InteractionArenaBuffer::competition_total_score (int32_t)
    uint32_t instigator_action_idx_buffer_id= INVALID_ID; // 4B: InteractionArenaBuffer::instigator_action_idx (uint32_t)
    uint32_t instigator_weight_buffer_id    = INVALID_ID; // 4B: InteractionArenaBuffer::instigator_weight (float)
    uint32_t competition_weight_buffer_id   = INVALID_ID; // 4B: InteractionArenaBuffer::competition_weight (float)
    uint32_t participant_offset_buffer_id   = INVALID_ID; // 4B: InteractionArenaBuffer::participant_slice_offset (uint32_t)
    uint32_t participant_count_buffer_id    = INVALID_ID; // 4B: InteractionArenaBuffer::participant_count (uint16_t/uint32_t)
    uint32_t participant_pod_buffer_id      = INVALID_ID; // 4B: Contiguous InteractionParticipantPOD array buffer
    uint32_t action_score_buffer_id         = INVALID_ID; // 4B: Secondary bridged ActionSequenceBuffer::current_score (int32_t)
    uint32_t consequence_ring_buffer_id     = INVALID_ID; // 4B: Output ConsequenceCommandPOD ring buffer
    uint32_t ring_cursor_buffer_id          = INVALID_ID; // 4B: Optional uint32_t[2] [read_idx, write_idx] cursor buffer

    float default_instigator_weight         = 1.0f;       // 4B: Fallback instigator/cooperation weight multiplier
    float default_competition_weight        = 1.0f;       // 4B: Fallback competition weight multiplier
    float consequence_scale_multiplier      = 1.0f;       // 4B: Scales resolved score_diff for ConsequenceCommandPOD

    bool emit_consequence_commands          = true;       // 1B: Queue ConsequenceCommandPOD entries when resolved
    bool emit_on_zero_diff                  = false;      // 1B: Emit consequence commands even when score_diff == 0
    uint8_t reserved_padding[2]             = {0};        // 2B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary diff_prop;
        diff_prop["name"] = "score_diff_buffer_id";
        diff_prop["type"] = godot::Variant::INT;
        diff_prop["hint_string"] = "buffer_option";
        props.push_back(diff_prop);

        godot::Dictionary iscr_prop;
        iscr_prop["name"] = "instigator_score_buffer_id";
        iscr_prop["type"] = godot::Variant::INT;
        iscr_prop["hint_string"] = "buffer_option";
        props.push_back(iscr_prop);

        godot::Dictionary cscr_prop;
        cscr_prop["name"] = "competition_score_buffer_id";
        cscr_prop["type"] = godot::Variant::INT;
        cscr_prop["hint_string"] = "buffer_option";
        props.push_back(cscr_prop);

        godot::Dictionary iact_prop;
        iact_prop["name"] = "instigator_action_idx_buffer_id";
        iact_prop["type"] = godot::Variant::INT;
        iact_prop["hint_string"] = "buffer_option";
        props.push_back(iact_prop);

        godot::Dictionary iwgt_prop;
        iwgt_prop["name"] = "instigator_weight_buffer_id";
        iwgt_prop["type"] = godot::Variant::INT;
        iwgt_prop["hint_string"] = "buffer_option";
        props.push_back(iwgt_prop);

        godot::Dictionary cwgt_prop;
        cwgt_prop["name"] = "competition_weight_buffer_id";
        cwgt_prop["type"] = godot::Variant::INT;
        cwgt_prop["hint_string"] = "buffer_option";
        props.push_back(cwgt_prop);

        godot::Dictionary poff_prop;
        poff_prop["name"] = "participant_offset_buffer_id";
        poff_prop["type"] = godot::Variant::INT;
        poff_prop["hint_string"] = "buffer_option";
        props.push_back(poff_prop);

        godot::Dictionary pcnt_prop;
        pcnt_prop["name"] = "participant_count_buffer_id";
        pcnt_prop["type"] = godot::Variant::INT;
        pcnt_prop["hint_string"] = "buffer_option";
        props.push_back(pcnt_prop);

        godot::Dictionary ppod_prop;
        ppod_prop["name"] = "participant_pod_buffer_id";
        ppod_prop["type"] = godot::Variant::INT;
        ppod_prop["hint_string"] = "buffer_option";
        props.push_back(ppod_prop);

        godot::Dictionary ascr_prop;
        ascr_prop["name"] = "action_score_buffer_id";
        ascr_prop["type"] = godot::Variant::INT;
        ascr_prop["hint_string"] = "buffer_option";
        props.push_back(ascr_prop);

        godot::Dictionary cring_prop;
        cring_prop["name"] = "consequence_ring_buffer_id";
        cring_prop["type"] = godot::Variant::INT;
        cring_prop["hint_string"] = "buffer_option";
        props.push_back(cring_prop);

        godot::Dictionary rcur_prop;
        rcur_prop["name"] = "ring_cursor_buffer_id";
        rcur_prop["type"] = godot::Variant::INT;
        rcur_prop["hint_string"] = "buffer_option";
        props.push_back(rcur_prop);

        godot::Dictionary diw_prop;
        diw_prop["name"] = "default_instigator_weight";
        diw_prop["type"] = godot::Variant::FLOAT;
        diw_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        diw_prop["hint_string"] = "0.0,10.0,0.05";
        props.push_back(diw_prop);

        godot::Dictionary dcw_prop;
        dcw_prop["name"] = "default_competition_weight";
        dcw_prop["type"] = godot::Variant::FLOAT;
        dcw_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        dcw_prop["hint_string"] = "0.0,10.0,0.05";
        props.push_back(dcw_prop);

        godot::Dictionary csc_prop;
        csc_prop["name"] = "consequence_scale_multiplier";
        csc_prop["type"] = godot::Variant::FLOAT;
        csc_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        csc_prop["hint_string"] = "0.0,10.0,0.05";
        props.push_back(csc_prop);

        godot::Dictionary emit_prop;
        emit_prop["name"] = "emit_consequence_commands";
        emit_prop["type"] = godot::Variant::BOOL;
        emit_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(emit_prop);

        godot::Dictionary zero_prop;
        zero_prop["name"] = "emit_on_zero_diff";
        zero_prop["type"] = godot::Variant::BOOL;
        zero_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(zero_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (score_diff_buffer_id != INVALID_ID)        return score_diff_buffer_id;
        if (instigator_score_buffer_id != INVALID_ID)  return instigator_score_buffer_id;
        return competition_score_buffer_id;
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
            const uint32_t sec_id = (action_score_buffer_id != INVALID_ID)
                ? action_score_buffer_id
                : get_target_buffer_id();
            if (sec_id != INVALID_ID) {
                p_view.bind_secondary(p_context.get_grant_part(sec_id));
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("score_diff_buffer_id")) {
            score_diff_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["score_diff_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && score_diff_buffer_id == INVALID_ID) {
            score_diff_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("instigator_score_buffer_id")) {
            instigator_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["instigator_score_buffer_id"]));
        }
        if (p_props.has("competition_score_buffer_id")) {
            competition_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["competition_score_buffer_id"]));
        }
        if (p_props.has("instigator_action_idx_buffer_id")) {
            instigator_action_idx_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["instigator_action_idx_buffer_id"]));
        }
        if (p_props.has("instigator_weight_buffer_id")) {
            instigator_weight_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["instigator_weight_buffer_id"]));
        }
        if (p_props.has("competition_weight_buffer_id")) {
            competition_weight_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["competition_weight_buffer_id"]));
        }
        if (p_props.has("participant_offset_buffer_id")) {
            participant_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["participant_offset_buffer_id"]));
        }
        if (p_props.has("participant_count_buffer_id")) {
            participant_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["participant_count_buffer_id"]));
        }
        if (p_props.has("participant_pod_buffer_id")) {
            participant_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["participant_pod_buffer_id"]));
        }
        if (p_props.has("action_score_buffer_id")) {
            action_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["action_score_buffer_id"]));
        }
        if (p_props.has("consequence_ring_buffer_id")) {
            consequence_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["consequence_ring_buffer_id"]));
        }
        if (p_props.has("ring_cursor_buffer_id")) {
            ring_cursor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ring_cursor_buffer_id"]));
        }
        if (p_props.has("default_instigator_weight")) {
            default_instigator_weight = static_cast<float>(p_props["default_instigator_weight"]);
        }
        if (p_props.has("default_competition_weight")) {
            default_competition_weight = static_cast<float>(p_props["default_competition_weight"]);
        }
        if (p_props.has("consequence_scale_multiplier")) {
            consequence_scale_multiplier = static_cast<float>(p_props["consequence_scale_multiplier"]);
        }
        if (p_props.has("emit_consequence_commands")) {
            emit_consequence_commands = static_cast<bool>(p_props["emit_consequence_commands"]);
        }
        if (p_props.has("emit_on_zero_diff")) {
            emit_on_zero_diff = static_cast<bool>(p_props["emit_on_zero_diff"]);
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
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, float>) {
            const GrantPartPOD* diff_part  = (score_diff_buffer_id != INVALID_ID)            ? p_context.get_grant_part(score_diff_buffer_id)            : nullptr;
            const GrantPartPOD* iscr_part  = (instigator_score_buffer_id != INVALID_ID)      ? p_context.get_grant_part(instigator_score_buffer_id)      : nullptr;
            const GrantPartPOD* cscr_part  = (competition_score_buffer_id != INVALID_ID)     ? p_context.get_grant_part(competition_score_buffer_id)     : nullptr;
            const GrantPartPOD* iact_part  = (instigator_action_idx_buffer_id != INVALID_ID) ? p_context.get_grant_part(instigator_action_idx_buffer_id) : nullptr;
            const GrantPartPOD* iwgt_part  = (instigator_weight_buffer_id != INVALID_ID)     ? p_context.get_grant_part(instigator_weight_buffer_id)     : nullptr;
            const GrantPartPOD* cwgt_part  = (competition_weight_buffer_id != INVALID_ID)    ? p_context.get_grant_part(competition_weight_buffer_id)    : nullptr;
            const GrantPartPOD* poff_part  = (participant_offset_buffer_id != INVALID_ID)    ? p_context.get_grant_part(participant_offset_buffer_id)    : nullptr;
            const GrantPartPOD* pcnt_part  = (participant_count_buffer_id != INVALID_ID)     ? p_context.get_grant_part(participant_count_buffer_id)     : nullptr;
            const GrantPartPOD* ppod_part  = (participant_pod_buffer_id != INVALID_ID)       ? p_context.get_grant_part(participant_pod_buffer_id)       : nullptr;
            const GrantPartPOD* ascr_part  = (action_score_buffer_id != INVALID_ID)          ? p_context.get_grant_part(action_score_buffer_id)          : nullptr;
            const GrantPartPOD* cring_part = (consequence_ring_buffer_id != INVALID_ID)      ? p_context.get_grant_part(consequence_ring_buffer_id)      : nullptr;
            const GrantPartPOD* rcur_part  = (ring_cursor_buffer_id != INVALID_ID)           ? p_context.get_grant_part(ring_cursor_buffer_id)           : nullptr;

            const GrantPartPOD* primary_part = diff_part ? diff_part : (iscr_part ? iscr_part : cscr_part);
            if (!primary_part || !primary_part->raw_base_ptr) return;

            int32_t* __restrict score_diffs           = (diff_part && diff_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(diff_part->raw_base_ptr)         : nullptr;
            int32_t* __restrict instigator_scores     = (iscr_part && iscr_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(iscr_part->raw_base_ptr)         : nullptr;
            int32_t* __restrict competition_scores    = (cscr_part && cscr_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(cscr_part->raw_base_ptr)         : nullptr;
            const uint32_t* __restrict inst_actions   = (iact_part && iact_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(iact_part->raw_base_ptr)   : nullptr;
            const float* __restrict inst_weights      = (iwgt_part && iwgt_part->raw_base_ptr) ? reinterpret_cast<const float*>(iwgt_part->raw_base_ptr)     : nullptr;
            const float* __restrict comp_weights      = (cwgt_part && cwgt_part->raw_base_ptr) ? reinterpret_cast<const float*>(cwgt_part->raw_base_ptr)     : nullptr;
            const uint32_t* __restrict part_offsets   = (poff_part && poff_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(poff_part->raw_base_ptr)   : nullptr;
            const uint8_t* __restrict pcnt_raw        = (pcnt_part && pcnt_part->raw_base_ptr) ? pcnt_part->raw_base_ptr                                     : nullptr;
            const uint32_t pcnt_stride                = pcnt_part ? pcnt_part->element_stride : 0;
            const InteractionParticipantPOD* __restrict participants = (ppod_part && ppod_part->raw_base_ptr)
                ? reinterpret_cast<const InteractionParticipantPOD*>(ppod_part->raw_base_ptr)
                : nullptr;
            const int32_t* __restrict action_scores   = (ascr_part && ascr_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(ascr_part->raw_base_ptr)   : nullptr;

            ConsequenceCommandPOD* __restrict ring_cmds = (cring_part && cring_part->raw_base_ptr)
                ? reinterpret_cast<ConsequenceCommandPOD*>(cring_part->raw_base_ptr)
                : nullptr;
            const uint32_t ring_cap = ring_cmds ? static_cast<uint32_t>(cring_part->capacity_bytes / sizeof(ConsequenceCommandPOD)) : 0u;
            uint32_t* __restrict ring_cursors = (rcur_part && rcur_part->raw_base_ptr && rcur_part->capacity_bytes >= 2 * sizeof(uint32_t))
                ? reinterpret_cast<uint32_t*>(rcur_part->raw_base_ptr)
                : nullptr;
            uint32_t local_write_idx = ring_cursors ? ring_cursors[1] : 0u;
            const uint32_t local_read_idx = ring_cursors ? ring_cursors[0] : 0u;

            const int64_t part_pod_cap = participants  ? static_cast<int64_t>(ppod_part->capacity_bytes / sizeof(InteractionParticipantPOD)) : 0;
            const int64_t ascr_cap     = action_scores ? static_cast<int64_t>(ascr_part->capacity_bytes / sizeof(int32_t))                   : 0;
            const int64_t iact_cap     = inst_actions  ? static_cast<int64_t>(iact_part->capacity_bytes / sizeof(uint32_t))                  : 0;
            const int64_t iwgt_cap     = inst_weights  ? static_cast<int64_t>(iwgt_part->capacity_bytes / sizeof(float))                     : 0;
            const int64_t cwgt_cap     = comp_weights  ? static_cast<int64_t>(cwgt_part->capacity_bytes / sizeof(float))                     : 0;

            int64_t interaction_count = static_cast<int64_t>(primary_part->capacity_bytes / sizeof(int32_t));
            if (diff_part) interaction_count = std::min<int64_t>(interaction_count, static_cast<int64_t>(diff_part->capacity_bytes / sizeof(int32_t)));
            if (iscr_part) interaction_count = std::min<int64_t>(interaction_count, static_cast<int64_t>(iscr_part->capacity_bytes / sizeof(int32_t)));
            if (cscr_part) interaction_count = std::min<int64_t>(interaction_count, static_cast<int64_t>(cscr_part->capacity_bytes / sizeof(int32_t)));
            if (interaction_count <= 0) return;

            auto push_consequence = [&](uint32_t target_entity, int32_t delta, uint32_t mask) noexcept {
                if (!ring_cmds || ring_cap == 0u || target_entity == INVALID_ID) return;
                if (ring_cursors) {
                    const uint32_t next_write = (local_write_idx + 1u) % ring_cap;
                    if (next_write == local_read_idx) return; // Ring full
                    ring_cmds[local_write_idx % ring_cap] = ConsequenceCommandPOD{target_entity, delta, mask};
                    local_write_idx = next_write;
                } else if (local_write_idx < ring_cap) {
                    ring_cmds[local_write_idx++] = ConsequenceCommandPOD{target_entity, delta, mask};
                }
            };

            auto eval_interaction = [&](int64_t idx) noexcept {
                const float inst_w = std::max(0.0f, (inst_weights && idx < iwgt_cap) ? inst_weights[idx] : default_instigator_weight);
                const float comp_w = std::max(0.0f, (comp_weights && idx < cwgt_cap) ? comp_weights[idx] : default_competition_weight);

                double inst_coop_sum = 0.0;
                double comp_sum      = 0.0;

                // Optional direct instigator action score when not duplicated in participant slice
                if (inst_actions && action_scores && idx < iact_cap) {
                    const uint32_t ia_idx = inst_actions[idx];
                    if (ia_idx != INVALID_ID && static_cast<int64_t>(ia_idx) < ascr_cap) {
                        inst_coop_sum += static_cast<double>(action_scores[ia_idx]) * static_cast<double>(inst_w);
                    }
                }

                int64_t slice_off = 0;
                int64_t slice_cnt = 0;
                if (participants && part_offsets && pcnt_raw) {
                    slice_off = static_cast<int64_t>(part_offsets[idx]);
                    const uint32_t raw_cnt = (pcnt_stride == sizeof(uint32_t))
                        ? reinterpret_cast<const uint32_t*>(pcnt_raw)[idx]
                        : static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(pcnt_raw)[idx]);
                    slice_cnt = static_cast<int64_t>(raw_cnt);
                    const int64_t end_p = std::min<int64_t>(slice_off + slice_cnt, part_pod_cap);

                    for (int64_t p = slice_off; p < end_p; ++p) {
                        const InteractionParticipantPOD& part = participants[p];
                        if (part.active == 0u) continue;
                        const int64_t a_idx = static_cast<int64_t>(part.action_idx);
                        if (!action_scores || a_idx < 0 || a_idx >= ascr_cap) continue;

                        const double raw_score = static_cast<double>(action_scores[a_idx]);
                        const double p_weight  = static_cast<double>(std::max(0.0f, part.weight_multiplier));

                        if (part.role == PARTICIPANT_ROLE_COMPETITION) {
                            comp_sum += raw_score * static_cast<double>(comp_w) * p_weight;
                        } else {
                            inst_coop_sum += raw_score * static_cast<double>(inst_w) * p_weight;
                        }
                    }
                } else {
                    // Pre-populated column mode: read instigator_scores[idx] and competition_scores[idx] weighted by inst_w / comp_w
                    if (instigator_scores && inst_coop_sum == 0.0) {
                        inst_coop_sum = static_cast<double>(instigator_scores[idx]) * static_cast<double>(inst_w);
                    }
                    if (competition_scores) {
                        comp_sum = static_cast<double>(competition_scores[idx]) * static_cast<double>(comp_w);
                    }
                }

                const int32_t resolved_inst = static_cast<int32_t>(std::lround(inst_coop_sum));
                const int32_t resolved_comp = static_cast<int32_t>(std::lround(comp_sum));
                const int32_t raw_diff      = resolved_inst - resolved_comp;
                const int32_t scaled_diff   = static_cast<int32_t>(std::lround(
                    static_cast<double>(raw_diff) * static_cast<double>(std::max(0.0f, consequence_scale_multiplier))
                ));

                if (instigator_scores)  instigator_scores[idx]  = resolved_inst;
                if (competition_scores) competition_scores[idx] = resolved_comp;
                if (score_diffs)        score_diffs[idx]        = scaled_diff;

                // Queue ConsequenceCommandPOD entries (mirrors GameInteraction::evaluate_interaction)
                if (emit_consequence_commands && ring_cmds && (scaled_diff != 0 || emit_on_zero_diff)) {
                    const uint32_t inst_outcome = (scaled_diff > 0)
                        ? CONSEQUENCE_FLAG_VICTORY
                        : ((scaled_diff < 0) ? CONSEQUENCE_FLAG_FAILURE : CONSEQUENCE_FLAG_TIE);
                    const uint32_t comp_outcome = (scaled_diff < 0)
                        ? CONSEQUENCE_FLAG_VICTORY
                        : ((scaled_diff > 0) ? CONSEQUENCE_FLAG_FAILURE : CONSEQUENCE_FLAG_TIE);
                    const uint32_t base_flags = CONSEQUENCE_FLAG_APPLY_TO_PROPERTY | CONSEQUENCE_FLAG_EMIT_SIGNAL;

                    if (participants && slice_cnt > 0) {
                        const int64_t end_p = std::min<int64_t>(slice_off + slice_cnt, part_pod_cap);
                        for (int64_t p = slice_off; p < end_p; ++p) {
                            const InteractionParticipantPOD& part = participants[p];
                            if (part.active == 0u || part.agent_entity_idx == INVALID_ID) continue;
                            if (part.role == PARTICIPANT_ROLE_COMPETITION) {
                                push_consequence(
                                    part.agent_entity_idx,
                                    -scaled_diff,
                                    base_flags | CONSEQUENCE_FLAG_COMPETITION_TEAM | comp_outcome
                                );
                            } else {
                                push_consequence(
                                    part.agent_entity_idx,
                                    scaled_diff,
                                    base_flags | CONSEQUENCE_FLAG_INSTIGATOR_TEAM | inst_outcome
                                );
                            }
                        }
                    } else {
                        push_consequence(
                            static_cast<uint32_t>(idx),
                            scaled_diff,
                            base_flags | CONSEQUENCE_FLAG_INSTIGATOR_TEAM | inst_outcome
                        );
                    }
                }
            };

            _iterate_selection(primary_part->selection, interaction_count, eval_interaction);

            if (ring_cursors) {
                ring_cursors[1] = local_write_idx;
            } else if (cring_part && !ring_cursors && emit_consequence_commands) {
                const_cast<GrantPartPOD*>(cring_part)->selection.element_count = static_cast<int64_t>(local_write_idx);
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

static_assert(sizeof(InteractionTugOfWarResolveTransformLogic<>) == 64, "InteractionTugOfWarResolveTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core