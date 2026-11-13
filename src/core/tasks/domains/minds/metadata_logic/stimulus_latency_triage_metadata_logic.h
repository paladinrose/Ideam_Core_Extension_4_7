#pragma once

/**
 * stimulus_latency_triage_metadata_logic.h
 * IdeamMinds Metadata Task (`MND4` — Wave 3 Latency Triage):
 * Ports `UnityMind.Latency()` and `StimulusPriority` (`1..12`) from `UnityMind.cs`:
 *   Compares `SwapView` (`prev_believed_value` vs. `curr_believed_value`), misreferencing status
 *   (`catalogued_fact_id != true_fact_id`), and Wave 2 affective valence (`emotional_charge`)
 *   to classify each belief/observation into the 12 `StimulusPriority` tiers, writing priority
 *   buckets into `r_selection.lod_levels` (and `belief_stimulus_priority_buffer_id`).
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/swap_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) StimulusLatencyTriageMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE | DataType::FLOAT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Stimulus Latency Triage";
    static constexpr std::string_view ui_category  = "Minds/Latency";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                    = INVALID_ID; // 4B: Primary belief selection buffer
    uint32_t belief_believed_value_buffer_id     = INVALID_ID; // 4B: int64_t current believed_value
    uint32_t belief_prev_believed_value_buffer_id= INVALID_ID; // 4B: int64_t previous believed_value (SwapView)
    uint32_t belief_emotional_charge_buffer_id   = INVALID_ID; // 4B: float emotional_charge from Wave 2
    uint32_t belief_catalogued_fact_id_buffer_id = INVALID_ID; // 4B: uint32_t catalogued_fact_id
    uint32_t belief_true_fact_id_buffer_id       = INVALID_ID; // 4B: uint32_t true_fact_id
    uint32_t belief_stimulus_priority_buffer_id  = INVALID_ID; // 4B: uint8_t output StimulusPriority (1..12)
    float threat_charge_threshold                = 0.55f;      // 4B: Emotional charge above which valence = THREAT
    float beneficial_charge_threshold            = 0.25f;      // 4B: Emotional charge in [beneficial, threat) -> BENEFICIAL
    bool write_priority_to_lod_levels            = true;       // 1B: Stamp StimulusPriority (1..12) into r_selection.lod_levels
    bool sync_prev_value_after_triage            = true;       // 1B: Copy curr_believed_value -> prev_believed_value
    uint8_t reserved_padding[26]                 = {0};        // 26B -> Total = 64B

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context, const GrantPartPOD *p_part) const noexcept {
        (void)p_part;
        if constexpr (requires { p_view.bind_secondary(p_part); }) {
            const uint32_t sec_id = (belief_prev_believed_value_buffer_id != INVALID_ID)
                ? belief_prev_believed_value_buffer_id
                : get_target_buffer_id();
            if (sec_id != INVALID_ID) {
                p_view.bind_secondary(p_context.get_grant_part(sec_id));
            }
        }
    }

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
        add_buf("belief_believed_value_buffer_id");
        add_buf("belief_prev_believed_value_buffer_id");
        add_buf("belief_emotional_charge_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("belief_true_fact_id_buffer_id");
        add_buf("belief_stimulus_priority_buffer_id");

        godot::Dictionary thr;
        thr["name"]        = "threat_charge_threshold";
        thr["type"]        = godot::Variant::FLOAT;
        thr["hint"]        = godot::PROPERTY_HINT_RANGE;
        thr["hint_string"] = "0.0,1.0,0.01";
        props.push_back(thr);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                  return target_buffer_id;
        if (belief_stimulus_priority_buffer_id != INVALID_ID)return belief_stimulus_priority_buffer_id;
        return belief_believed_value_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                     target_buffer_id                     = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("belief_believed_value_buffer_id"))      belief_believed_value_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_believed_value_buffer_id"]));
        if (p_props.has("belief_prev_believed_value_buffer_id")) belief_prev_believed_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_prev_believed_value_buffer_id"]));
        if (p_props.has("belief_emotional_charge_buffer_id"))    belief_emotional_charge_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_emotional_charge_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id"))  belief_catalogued_fact_id_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("belief_true_fact_id_buffer_id"))        belief_true_fact_id_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_true_fact_id_buffer_id"]));
        if (p_props.has("belief_stimulus_priority_buffer_id"))   belief_stimulus_priority_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_stimulus_priority_buffer_id"]));
        if (p_props.has("threat_charge_threshold"))              threat_charge_threshold              = static_cast<float>(static_cast<double>(p_props["threat_charge_threshold"]));
        if (p_props.has("beneficial_charge_threshold"))          beneficial_charge_threshold          = static_cast<float>(static_cast<double>(p_props["beneficial_charge_threshold"]));
        if (p_props.has("write_priority_to_lod_levels"))         write_priority_to_lod_levels         = static_cast<bool>(p_props["write_priority_to_lod_levels"]);
        if (p_props.has("sync_prev_value_after_triage"))         sync_prev_value_after_triage         = static_cast<bool>(p_props["sync_prev_value_after_triage"]);
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context,
        T_View &p_view
    ) const {
        (void)p_view;
        const GrantPartPOD *cval_part = (belief_believed_value_buffer_id != INVALID_ID)      ? p_context.get_grant_part(belief_believed_value_buffer_id)      : nullptr;
        const GrantPartPOD *pval_part = (belief_prev_believed_value_buffer_id != INVALID_ID) ? p_context.get_grant_part(belief_prev_believed_value_buffer_id) : nullptr;
        const GrantPartPOD *emo_part  = (belief_emotional_charge_buffer_id != INVALID_ID)    ? p_context.get_grant_part(belief_emotional_charge_buffer_id)    : nullptr;
        const GrantPartPOD *cfid_part = (belief_catalogued_fact_id_buffer_id != INVALID_ID)  ? p_context.get_grant_part(belief_catalogued_fact_id_buffer_id)  : nullptr;
        const GrantPartPOD *tfid_part = (belief_true_fact_id_buffer_id != INVALID_ID)        ? p_context.get_grant_part(belief_true_fact_id_buffer_id)        : nullptr;
        const GrantPartPOD *prio_part = (belief_stimulus_priority_buffer_id != INVALID_ID)   ? p_context.get_grant_part(belief_stimulus_priority_buffer_id)   : nullptr;

        const auto *curr_vals = (cval_part && cval_part->raw_base_ptr) ? reinterpret_cast<const int64_t *>(cval_part->raw_base_ptr) : nullptr;
        auto *prev_vals       = (pval_part && pval_part->raw_base_ptr) ? reinterpret_cast<int64_t *>(pval_part->raw_base_ptr)       : nullptr;
        const auto *emo_vals  = (emo_part && emo_part->raw_base_ptr)   ? reinterpret_cast<const float *>(emo_part->raw_base_ptr)   : nullptr;
        const auto *cat_fids  = (cfid_part && cfid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr): nullptr;
        const auto *true_fids = (tfid_part && tfid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(tfid_part->raw_base_ptr): nullptr;
        auto *prio_out        = (prio_part && prio_part->raw_base_ptr) ? reinterpret_cast<uint8_t *>(prio_part->raw_base_ptr)       : nullptr;

        const int64_t cap = r_selection.capacity;
        for (int64_t i = 0; i < cap; ++i) {
            const int64_t cval = curr_vals ? curr_vals[i] : 0;
            const int64_t pval = prev_vals ? prev_vals[i] : cval;
            const uint8_t prev_prio = prio_out ? prio_out[i] : 0u;

            const bool is_new      = (prev_prio == 0u) || (prev_prio >= static_cast<uint8_t>(StimulusPriority::NEW_NEUTRAL));
            const bool has_changed = (cval != pval);
            const bool is_misref   = (cat_fids && true_fids) ? (cat_fids[i] != true_fids[i]) : false;
            const float charge     = emo_vals ? std::abs(emo_vals[i]) : 0.0f;

            StimulusValence valence = StimulusValence::NEUTRAL;
            if (charge >= threat_charge_threshold) {
                valence = StimulusValence::THREAT;
            } else if (is_misref) {
                valence = StimulusValence::UNKNOWN;
            } else if (charge >= beneficial_charge_threshold) {
                valence = StimulusValence::BENEFICIAL;
            }

            const uint8_t tier = static_cast<uint8_t>(classify_stimulus_priority(is_new, has_changed, valence));
            if (prio_out) {
                prio_out[i] = tier;
            }
            if (write_priority_to_lod_levels && r_selection.lod_levels) {
                r_selection.lod_levels[i] = tier;
            }
            if (sync_prev_value_after_triage && prev_vals) {
                prev_vals[i] = cval;
            }
        }
    }
};

static_assert(sizeof(StimulusLatencyTriageMetadataLogic<>) == 64, "StimulusLatencyTriageMetadataLogic must be 64 bytes!");

} // namespace ideam::core
