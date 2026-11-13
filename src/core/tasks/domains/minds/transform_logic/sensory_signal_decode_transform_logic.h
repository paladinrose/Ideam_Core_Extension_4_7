#pragma once

/**
 * sensory_signal_decode_transform_logic.h
 * IdeamMinds Transform Task (`MND1` — Wave 1 Sensory Perception):
 * Ports `UnitySense`, `ISignal`, `INoise`, `RaySensor.seenDuration`, and `UnityInteroception` / `UnityNumbness`:
 *   Drains `SensoryObservationEventRing` (`SensoryEventPod`), applies per-modality noise
 *   (`Visual`, `Audio`, `Haptic`, `Numbness`), scales `ObservationPod::clarity` by continuous
 *   `seenDuration` / `percentTransmitted`, and stages observed `BeliefPod` + `BeliefCatalogueEntryPod`
 *   updates (preserving Akashic Tome `tome_id` and sensory misreferencing when
 *   `perceived_fact_id != 0 && perceived_fact_id != target_fact_id`).
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/ring_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) SensorySignalDecodeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = RingStrategy;
    using DefaultView     = RingView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::RING;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Sensory Signal Decode";
    static constexpr std::string_view ui_category  = "Minds/Perception";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                         = INVALID_ID; // 4B
    uint32_t sensory_observation_event_ring_buffer_id = INVALID_ID; // 4B: SensoryEventPod (32B Ring)
    uint32_t sensory_ring_cursor_buffer_id            = INVALID_ID; // 4B: uint32_t[2] ring cursor
    uint32_t observation_pod_buffer_id                = INVALID_ID; // 4B: ObservationPod (16B AoS)
    uint32_t belief_catalogued_fact_id_buffer_id      = INVALID_ID; // 4B: uint32_t catalogued_fact_id
    uint32_t belief_true_fact_id_buffer_id            = INVALID_ID; // 4B: uint32_t true_fact_id
    uint32_t belief_believed_value_buffer_id          = INVALID_ID; // 4B: int64_t believed_value
    uint32_t belief_confidence_buffer_id              = INVALID_ID; // 4B: float confidence
    uint32_t belief_believed_tome_id_buffer_id        = INVALID_ID; // 4B: uint16_t believed_tome_id
    uint32_t belief_true_tome_id_buffer_id            = INVALID_ID; // 4B: uint16_t true_tome_id
    float visual_noise_scale                          = 1.0f;       // 4B: Sight noise multiplier
    float audio_noise_scale                           = 1.0f;       // 4B: Hearing noise multiplier
    float haptic_and_numbness_scale                   = 1.0f;       // 4B: Touch & Interoception/Numbness multiplier
    float misreference_clarity_ceiling                = 0.45f;      // 4B: Below this clarity, noisy signals can misreference
    bool drain_ring_on_execute                        = true;       // 1B: Reset ring cursor after decoding
    uint8_t reserved_padding[7]                       = {0};        // 7B -> Total = 64B

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
        add_buf("sensory_observation_event_ring_buffer_id");
        add_buf("sensory_ring_cursor_buffer_id");
        add_buf("observation_pod_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("belief_true_fact_id_buffer_id");
        add_buf("belief_believed_value_buffer_id");
        add_buf("belief_confidence_buffer_id");
        add_buf("belief_believed_tome_id_buffer_id");
        add_buf("belief_true_tome_id_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                         target_buffer_id                         = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sensory_observation_event_ring_buffer_id")) sensory_observation_event_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["sensory_observation_event_ring_buffer_id"]));
        if (p_props.has("sensory_ring_cursor_buffer_id"))            sensory_ring_cursor_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["sensory_ring_cursor_buffer_id"]));
        if (p_props.has("observation_pod_buffer_id"))                observation_pod_buffer_id                = static_cast<uint32_t>(static_cast<int64_t>(p_props["observation_pod_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id"))      belief_catalogued_fact_id_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("belief_true_fact_id_buffer_id"))            belief_true_fact_id_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_true_fact_id_buffer_id"]));
        if (p_props.has("belief_believed_value_buffer_id"))          belief_believed_value_buffer_id          = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_believed_value_buffer_id"]));
        if (p_props.has("belief_confidence_buffer_id"))              belief_confidence_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_confidence_buffer_id"]));
        if (p_props.has("belief_believed_tome_id_buffer_id"))        belief_believed_tome_id_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_believed_tome_id_buffer_id"]));
        if (p_props.has("belief_true_tome_id_buffer_id"))            belief_true_tome_id_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_true_tome_id_buffer_id"]));
        if (p_props.has("visual_noise_scale"))                       visual_noise_scale                       = static_cast<float>(static_cast<double>(p_props["visual_noise_scale"]));
        if (p_props.has("audio_noise_scale"))                        audio_noise_scale                        = static_cast<float>(static_cast<double>(p_props["audio_noise_scale"]));
        if (p_props.has("haptic_and_numbness_scale"))                haptic_and_numbness_scale                = static_cast<float>(static_cast<double>(p_props["haptic_and_numbness_scale"]));
        if (p_props.has("misreference_clarity_ceiling"))             misreference_clarity_ceiling             = static_cast<float>(static_cast<double>(p_props["misreference_clarity_ceiling"]));
        if (p_props.has("drain_ring_on_execute"))                    drain_ring_on_execute                    = static_cast<bool>(p_props["drain_ring_on_execute"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                         return target_buffer_id;
        if (sensory_observation_event_ring_buffer_id != INVALID_ID) return sensory_observation_event_ring_buffer_id;
        return observation_pod_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *ring_part = p_context.get_grant_part(sensory_observation_event_ring_buffer_id);
        const GrantPartPOD *cur_part  = p_context.get_grant_part(sensory_ring_cursor_buffer_id);
        if (!ring_part || !ring_part->raw_base_ptr || !cur_part || !cur_part->raw_base_ptr) return;

        const auto *events  = reinterpret_cast<const SensoryEventPod *>(ring_part->raw_base_ptr);
        const uint32_t rcap = static_cast<uint32_t>(ring_part->capacity_bytes / sizeof(SensoryEventPod));
        auto *cursors       = reinterpret_cast<uint32_t *>(cur_part->raw_base_ptr);

        const GrantPartPOD *obs_part  = p_context.get_grant_part(observation_pod_buffer_id);
        const GrantPartPOD *cfid_part = p_context.get_grant_part(belief_catalogued_fact_id_buffer_id);
        const GrantPartPOD *tfid_part = p_context.get_grant_part(belief_true_fact_id_buffer_id);
        const GrantPartPOD *bval_part = p_context.get_grant_part(belief_believed_value_buffer_id);
        const GrantPartPOD *conf_part = p_context.get_grant_part(belief_confidence_buffer_id);
        const GrantPartPOD *btom_part = p_context.get_grant_part(belief_believed_tome_id_buffer_id);
        const GrantPartPOD *ttom_part = p_context.get_grant_part(belief_true_tome_id_buffer_id);

        auto *obs_rows  = (obs_part && obs_part->raw_base_ptr)   ? reinterpret_cast<ObservationPod *>(obs_part->raw_base_ptr) : nullptr;
        auto *cat_fids  = (cfid_part && cfid_part->raw_base_ptr) ? reinterpret_cast<uint32_t *>(cfid_part->raw_base_ptr)       : nullptr;
        auto *true_fids = (tfid_part && tfid_part->raw_base_ptr) ? reinterpret_cast<uint32_t *>(tfid_part->raw_base_ptr)       : nullptr;
        auto *bvals     = (bval_part && bval_part->raw_base_ptr) ? reinterpret_cast<int64_t *>(bval_part->raw_base_ptr)        : nullptr;
        auto *confs     = (conf_part && conf_part->raw_base_ptr) ? reinterpret_cast<float *>(conf_part->raw_base_ptr)          : nullptr;
        auto *btomes    = (btom_part && btom_part->raw_base_ptr) ? reinterpret_cast<uint16_t *>(btom_part->raw_base_ptr)       : nullptr;
        auto *ttomes    = (ttom_part && ttom_part->raw_base_ptr) ? reinterpret_cast<uint16_t *>(ttom_part->raw_base_ptr)       : nullptr;

        const uint32_t obs_cap  = obs_rows ? static_cast<uint32_t>(obs_part->capacity_bytes / sizeof(ObservationPod)) : 0u;
        const uint32_t b_cap    = cat_fids ? static_cast<uint32_t>(cfid_part->capacity_bytes / sizeof(uint32_t))      : 0u;

        const uint32_t write_idx = std::min(cursors[1], rcap);
        uint32_t read_idx        = std::min(cursors[0], write_idx);

        while (read_idx < write_idx) {
            const SensoryEventPod &ev = events[read_idx];

            float mod_scale = visual_noise_scale;
            if (ev.modality == SensoryModality::HEARING) {
                mod_scale = audio_noise_scale;
            } else if (ev.modality == SensoryModality::TOUCH || ev.modality == SensoryModality::INTEROCEPTION) {
                mod_scale = haptic_and_numbness_scale;
            }

            const float effective_noise = std::clamp(ev.environmental_noise * mod_scale, 0.0f, 1.0f);
            const float clarity         = std::clamp(ev.raw_clarity * (1.0f - effective_noise), 0.0f, 1.0f);
            const uint32_t cat_id       = ev.resolved_catalogued_fact_id();
            const uint32_t true_id      = (ev.target_fact_id != 0u) ? ev.target_fact_id : cat_id;

            if (obs_rows && read_idx < obs_cap) {
                ObservationPod obs{};
                obs.fact_id   = cat_id;
                obs.sememe_id = cat_id;
                obs.clarity   = clarity;
                obs.tome_id   = ev.tome_id;
                obs.modality  = ev.modality;
                obs.priority  = static_cast<uint8_t>(
                    (cat_id != true_id || clarity < misreference_clarity_ceiling)
                        ? StimulusPriority::NEW_UNKNOWN
                        : StimulusPriority::NEW_NEUTRAL
                );
                obs_rows[read_idx] = obs;
            }

            if (read_idx < b_cap) {
                if (cat_fids)  cat_fids[read_idx]  = cat_id;
                if (true_fids) true_fids[read_idx] = true_id;
                if (bvals)     bvals[read_idx]     = ev.sensed_value;
                if (confs)     confs[read_idx]     = clarity;
                if (btomes)    btomes[read_idx]    = ev.tome_id;
                if (ttomes)    ttomes[read_idx]    = ev.tome_id;
            }

            ++read_idx;
        }

        if (drain_ring_on_execute) {
            cursors[0] = 0u;
            cursors[1] = 0u;
        }
    }
};

static_assert(sizeof(SensorySignalDecodeTransformLogic<>) == 64, "SensorySignalDecodeTransformLogic must be 64 bytes!");

} // namespace ideam::core
