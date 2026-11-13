#pragma once

/**
 * discourse_turn_and_prosody_transform_logic.h
 * IdeamMinds Transform Task (`MND10` — Wave 4 Discourse & Prosody):
 * Ports `Discourse.cs` (`TurnAllocation`, `TurnConstructionUnit`, `TurnResolutionType`,
 * and `Utterance.prosody` `Spline2D` where `X = Volume, Y = Pitch`):
 *   Evaluates the 3 Sacks-Schegloff-Jefferson turn-taking rules:
 *     1a (`SPEAKER_SELECTS_NEXT`): Current speaker selects next speaker (`selected_next_id`).
 *     1b (`SELF_SELECT_FIRST`): First active listener in `discourse_listener_mask` claims turn.
 *     1c (`SPEAKER_CONTINUES`): Current speaker continues with another Turn Construction Unit (TCU).
 *   Advances `percent_transmitted` and samples Volume (`current_volume`) and Pitch (`current_pitch`)
 *   along the prosody curve.
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) DiscourseTurnAndProsodyTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Discourse Turn & Prosody";
    static constexpr std::string_view ui_category  = "Minds/Discourse";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                    = INVALID_ID; // 4B
    uint32_t discourse_turn_pod_buffer_id        = INVALID_ID; // 4B: DiscourseTurnPod (32B AoS)
    uint32_t discourse_speaker_mask_buffer_id    = INVALID_ID; // 4B: uint64_t active speaker bitmask
    uint32_t discourse_listener_mask_buffer_id   = INVALID_ID; // 4B: uint64_t active listener bitmask
    uint32_t discourse_prosody_spline_id_buffer_id = INVALID_ID; // 4B: uint32_t Spline2D handle ID
    uint32_t agent_excitation_level_buffer_id    = INVALID_ID; // 4B: float per-agent excitation (modulates pitch/volume)
    float utterance_transmission_rate            = 0.25f;      // 4B: Progress added to percent_transmitted per step
    float base_volume                            = 1.0f;       // 4B: Baseline X=Volume
    float base_pitch                             = 1.0f;       // 4B: Baseline Y=Pitch
    float intonational_pitch_drop                = 0.18f;      // 4B: Terminal pitch drop at end of INTONATIONAL TCU
    uint8_t reserved_padding[24]                 = {0};        // 24B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char *p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("discourse_turn_pod_buffer_id");
        add_buf("discourse_speaker_mask_buffer_id");
        add_buf("discourse_listener_mask_buffer_id");
        add_buf("discourse_prosody_spline_id_buffer_id");
        add_buf("agent_excitation_level_buffer_id");

        godot::Dictionary rate;
        rate["name"]        = "utterance_transmission_rate";
        rate["type"]        = godot::Variant::FLOAT;
        rate["hint"]        = godot::PROPERTY_HINT_RANGE;
        rate["hint_string"] = "0.01,1.0,0.01";
        props.push_back(rate);

        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                     target_buffer_id                     = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("discourse_turn_pod_buffer_id"))         discourse_turn_pod_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["discourse_turn_pod_buffer_id"]));
        if (p_props.has("discourse_speaker_mask_buffer_id"))     discourse_speaker_mask_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["discourse_speaker_mask_buffer_id"]));
        if (p_props.has("discourse_listener_mask_buffer_id"))    discourse_listener_mask_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["discourse_listener_mask_buffer_id"]));
        if (p_props.has("discourse_prosody_spline_id_buffer_id"))discourse_prosody_spline_id_buffer_id= static_cast<uint32_t>(static_cast<int64_t>(p_props["discourse_prosody_spline_id_buffer_id"]));
        if (p_props.has("agent_excitation_level_buffer_id"))     agent_excitation_level_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_excitation_level_buffer_id"]));
        if (p_props.has("utterance_transmission_rate"))          utterance_transmission_rate          = static_cast<float>(static_cast<double>(p_props["utterance_transmission_rate"]));
        if (p_props.has("base_volume"))                          base_volume                          = static_cast<float>(static_cast<double>(p_props["base_volume"]));
        if (p_props.has("base_pitch"))                           base_pitch                           = static_cast<float>(static_cast<double>(p_props["base_pitch"]));
        if (p_props.has("intonational_pitch_drop"))              intonational_pitch_drop              = static_cast<float>(static_cast<double>(p_props["intonational_pitch_drop"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)             return target_buffer_id;
        if (discourse_turn_pod_buffer_id != INVALID_ID) return discourse_turn_pod_buffer_id;
        return discourse_speaker_mask_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *turn_part = p_context.get_grant_part(discourse_turn_pod_buffer_id);
        const GrantPartPOD *smsk_part = p_context.get_grant_part(discourse_speaker_mask_buffer_id);
        const GrantPartPOD *lmsk_part = p_context.get_grant_part(discourse_listener_mask_buffer_id);
        const GrantPartPOD *exc_part  = p_context.get_grant_part(agent_excitation_level_buffer_id);

        if (!turn_part || !turn_part->raw_base_ptr) return;

        auto *turns          = reinterpret_cast<DiscourseTurnPod *>(turn_part->raw_base_ptr);
        const int64_t t_cap  = static_cast<int64_t>(turn_part->capacity_bytes / sizeof(DiscourseTurnPod));
        auto *speaker_masks  = (smsk_part && smsk_part->raw_base_ptr) ? reinterpret_cast<uint64_t *>(smsk_part->raw_base_ptr) : nullptr;
        auto *listener_masks = (lmsk_part && lmsk_part->raw_base_ptr) ? reinterpret_cast<uint64_t *>(lmsk_part->raw_base_ptr) : nullptr;
        const auto *excs     = (exc_part && exc_part->raw_base_ptr)   ? reinterpret_cast<const float *>(exc_part->raw_base_ptr): nullptr;
        const int64_t e_cap  = excs ? static_cast<int64_t>(exc_part->capacity_bytes / sizeof(float)) : 0;

        for (int64_t i = 0; i < t_cap; ++i) {
            DiscourseTurnPod &turn = turns[i];
            const float dt_step = (p_context.delta > 0.0)
                ? static_cast<float>(p_context.delta) * utterance_transmission_rate
                : utterance_transmission_rate;

            turn.percent_transmitted = std::clamp(turn.percent_transmitted + dt_step, 0.0f, 1.0f);

            const float speaker_exc = (excs && static_cast<int64_t>(turn.active_speaker_id) < e_cap)
                ? excs[turn.active_speaker_id]
                : 0.0f;

            // Prosody curve evaluation (X = Volume, Y = Pitch parameterized over t = percent_transmitted)
            const float t = turn.percent_transmitted;
            const float envelope = 4.0f * t * (1.0f - t); // Smooth bell envelope over utterance
            turn.current_volume  = base_volume * (0.85f + 0.30f * envelope + 0.25f * speaker_exc);
            turn.current_pitch   = base_pitch * (1.0f + 0.20f * speaker_exc - intonational_pitch_drop * t);

            if (turn.percent_transmitted >= 1.0f) {
                if (turn.resolution == TurnResolutionType::UNRESOLVED) {
                    turn.resolution = TurnResolutionType::INTONATIONAL;
                }

                // Sacks-Schegloff-Jefferson Turn Allocation (1a -> 1b -> 1c)
                if (turn.selected_next_id != INVALID_ID && turn.selected_next_id != 0u) {
                    turn.allocation_rule   = TurnAllocationRule::SPEAKER_SELECTS_NEXT;
                    turn.active_speaker_id = turn.selected_next_id;
                    turn.selected_next_id  = 0u;
                } else if (listener_masks && listener_masks[i] != 0ULL) {
                    turn.allocation_rule   = TurnAllocationRule::SELF_SELECT_FIRST;
                    const uint32_t next_id = static_cast<uint32_t>(std::countr_zero(listener_masks[i]));
                    turn.active_speaker_id = next_id;
                } else {
                    turn.allocation_rule = TurnAllocationRule::SPEAKER_CONTINUES;
                }

                if (speaker_masks && turn.active_speaker_id < 64u) {
                    speaker_masks[i] = (1ULL << turn.active_speaker_id);
                }
                turn.percent_transmitted = 0.0f;
            }
        }
    }
};

static_assert(sizeof(DiscourseTurnAndProsodyTransformLogic<>) == 64, "DiscourseTurnAndProsodyTransformLogic must be 64 bytes!");

} // namespace ideam::core
