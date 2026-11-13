#pragma once

/**
 * instinctive_affect_appraisal_transform_logic.h
 * IdeamMinds Transform Task (`MND3` — Wave 2 Emotional Appraisal):
 * Ports `UnityEmotion` and `IMentalState.ExcitationLevel` (`UnityMind.cs`):
 *   Cross-multiplies newly perceived or updated beliefs against `InstinctiveTraitWeightsPod`
 *   (`fear`, `curiosity`, `guilt`, `loyalty`, `self_preservation`) in SIMD-friendly contiguous
 *   loops to update `BeliefPod::emotional_charge` and each agent's `excitation_level` and
 *   `active_emotion_mask`.
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/aosoa_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <typename T = float>
struct alignas(64) InstinctiveAffectAppraisalTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = TiledSoAStrategy;
    using DefaultView     = AOSOAView<T, 8, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Instinctive Affect Appraisal";
    static constexpr std::string_view ui_category  = "Minds/Emotion";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                     = INVALID_ID; // 4B
    uint32_t belief_emotional_charge_buffer_id    = INVALID_ID; // 4B: float emotional_charge column
    uint32_t belief_confidence_buffer_id          = INVALID_ID; // 4B: float confidence column
    uint32_t belief_catalogued_fact_id_buffer_id  = INVALID_ID; // 4B: uint32_t catalogued_fact_id column
    uint32_t fact_base_salience_buffer_id         = INVALID_ID; // 4B: float Akashic Tome base_salience column
    uint32_t fact_flags_buffer_id                 = INVALID_ID; // 4B: uint16_t Akashic Tome fact_flags column
    uint32_t agent_trait_weights_buffer_id        = INVALID_ID; // 4B: InstinctiveTraitWeightsPod column
    uint32_t agent_excitation_level_buffer_id     = INVALID_ID; // 4B: float per-agent excitation_level
    uint32_t agent_active_emotion_mask_buffer_id  = INVALID_ID; // 4B: uint32_t per-agent active_emotion_mask
    uint32_t agent_belief_csr_offsets_buffer_id   = INVALID_ID; // 4B: uint32_t CSR offsets
    uint32_t agent_belief_csr_counts_buffer_id    = INVALID_ID; // 4B: uint16_t CSR counts
    float excitation_decay_rate                   = 0.08f;      // 4B: Passive emotional homeostasis decay per tick
    float secret_fact_fear_guilt_multiplier       = 1.50f;      // 4B: Boost when appraising FACT_FLAG_SECRET / ALIBI
    uint8_t reserved_padding[12]                  = {0};        // 12B -> Total = 64B

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
        add_buf("belief_emotional_charge_buffer_id");
        add_buf("belief_confidence_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("fact_base_salience_buffer_id");
        add_buf("fact_flags_buffer_id");
        add_buf("agent_trait_weights_buffer_id");
        add_buf("agent_excitation_level_buffer_id");
        add_buf("agent_active_emotion_mask_buffer_id");
        add_buf("agent_belief_csr_offsets_buffer_id");
        add_buf("agent_belief_csr_counts_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                    target_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("belief_emotional_charge_buffer_id"))   belief_emotional_charge_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_emotional_charge_buffer_id"]));
        if (p_props.has("belief_confidence_buffer_id"))         belief_confidence_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_confidence_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id")) belief_catalogued_fact_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("fact_base_salience_buffer_id"))        fact_base_salience_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["fact_base_salience_buffer_id"]));
        if (p_props.has("fact_flags_buffer_id"))                fact_flags_buffer_id                = static_cast<uint32_t>(static_cast<int64_t>(p_props["fact_flags_buffer_id"]));
        if (p_props.has("agent_trait_weights_buffer_id"))       agent_trait_weights_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_trait_weights_buffer_id"]));
        if (p_props.has("agent_excitation_level_buffer_id"))    agent_excitation_level_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_excitation_level_buffer_id"]));
        if (p_props.has("agent_active_emotion_mask_buffer_id")) agent_active_emotion_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_active_emotion_mask_buffer_id"]));
        if (p_props.has("agent_belief_csr_offsets_buffer_id"))  agent_belief_csr_offsets_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_belief_csr_offsets_buffer_id"]));
        if (p_props.has("agent_belief_csr_counts_buffer_id"))   agent_belief_csr_counts_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_belief_csr_counts_buffer_id"]));
        if (p_props.has("excitation_decay_rate"))               excitation_decay_rate               = static_cast<float>(static_cast<double>(p_props["excitation_decay_rate"]));
        if (p_props.has("secret_fact_fear_guilt_multiplier"))   secret_fact_fear_guilt_multiplier   = static_cast<float>(static_cast<double>(p_props["secret_fact_fear_guilt_multiplier"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                  return target_buffer_id;
        if (belief_emotional_charge_buffer_id != INVALID_ID) return belief_emotional_charge_buffer_id;
        return agent_excitation_level_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *emo_part  = p_context.get_grant_part(belief_emotional_charge_buffer_id);
        const GrantPartPOD *conf_part = p_context.get_grant_part(belief_confidence_buffer_id);
        const GrantPartPOD *cfid_part = p_context.get_grant_part(belief_catalogued_fact_id_buffer_id);
        const GrantPartPOD *sal_part  = p_context.get_grant_part(fact_base_salience_buffer_id);
        const GrantPartPOD *fflg_part = p_context.get_grant_part(fact_flags_buffer_id);
        const GrantPartPOD *trt_part  = p_context.get_grant_part(agent_trait_weights_buffer_id);
        const GrantPartPOD *exc_part  = p_context.get_grant_part(agent_excitation_level_buffer_id);
        const GrantPartPOD *emsk_part = p_context.get_grant_part(agent_active_emotion_mask_buffer_id);
        const GrantPartPOD *coff_part = p_context.get_grant_part(agent_belief_csr_offsets_buffer_id);
        const GrantPartPOD *ccnt_part = p_context.get_grant_part(agent_belief_csr_counts_buffer_id);

        if (!emo_part || !emo_part->raw_base_ptr) return;

        auto *emo_charges     = reinterpret_cast<float *>(emo_part->raw_base_ptr);
        const int64_t b_cap   = static_cast<int64_t>(emo_part->capacity_bytes / sizeof(float));
        const auto *confs     = (conf_part && conf_part->raw_base_ptr) ? reinterpret_cast<const float *>(conf_part->raw_base_ptr)    : nullptr;
        const auto *cat_fids  = (cfid_part && cfid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr) : nullptr;
        const auto *saliences = (sal_part && sal_part->raw_base_ptr)   ? reinterpret_cast<const float *>(sal_part->raw_base_ptr)     : nullptr;
        const auto *fflags    = (fflg_part && fflg_part->raw_base_ptr) ? reinterpret_cast<const uint16_t *>(fflg_part->raw_base_ptr) : nullptr;
        const int64_t f_cap   = saliences ? static_cast<int64_t>(sal_part->capacity_bytes / sizeof(float)) : 0;

        const auto *traits    = (trt_part && trt_part->raw_base_ptr)   ? reinterpret_cast<const InstinctiveTraitWeightsPod *>(trt_part->raw_base_ptr) : nullptr;
        auto *excitations     = (exc_part && exc_part->raw_base_ptr)   ? reinterpret_cast<float *>(exc_part->raw_base_ptr)                            : nullptr;
        auto *emo_masks       = (emsk_part && emsk_part->raw_base_ptr) ? reinterpret_cast<uint32_t *>(emsk_part->raw_base_ptr)                        : nullptr;
        const auto *csr_offs  = (coff_part && coff_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(coff_part->raw_base_ptr)                  : nullptr;
        const auto *csr_cnts  = (ccnt_part && ccnt_part->raw_base_ptr) ? reinterpret_cast<const uint16_t *>(ccnt_part->raw_base_ptr)                  : nullptr;
        const int64_t a_cap   = traits ? static_cast<int64_t>(trt_part->capacity_bytes / sizeof(InstinctiveTraitWeightsPod)) : 0;

        if (traits && csr_offs && csr_cnts && a_cap > 0) {
            for (int64_t a = 0; a < a_cap; ++a) {
                const InstinctiveTraitWeightsPod &tr = traits[a];
                const uint32_t start = csr_offs[a];
                const uint16_t count = csr_cnts[a];
                float agent_exc = excitations ? (excitations[a] * (1.0f - excitation_decay_rate)) : 0.0f;
                uint32_t mask   = 0u;

                for (uint16_t j = 0; j < count; ++j) {
                    const int64_t idx = static_cast<int64_t>(start + j);
                    if (idx >= b_cap) break;

                    const float conf     = confs ? confs[idx] : 1.0f;
                    const uint32_t fid   = cat_fids ? cat_fids[idx] : 0u;
                    const float salience = (saliences && fid > 0u && static_cast<int64_t>(fid - 1u) < f_cap)
                        ? saliences[fid - 1u]
                        : 1.0f;
                    const uint16_t fflg  = (fflags && fid > 0u && static_cast<int64_t>(fid - 1u) < f_cap)
                        ? fflags[fid - 1u]
                        : 0u;

                    float trait_drive = (tr.fear * 0.30f + tr.curiosity * 0.25f + tr.guilt * 0.20f +
                                         tr.loyalty * 0.10f + tr.self_preservation * 0.15f);
                    if ((fflg & (FACT_FLAG_SECRET | FACT_FLAG_ALIBI_RECORD)) != 0u) {
                        trait_drive *= secret_fact_fear_guilt_multiplier;
                        mask |= (1u << 0) | (1u << 2); // Fear + Guilt bits
                    } else {
                        mask |= (1u << 1); // Curiosity bit
                    }

                    const float charge = std::clamp(conf * salience * trait_drive, 0.0f, 1.0f);
                    emo_charges[idx]   = charge;
                    agent_exc          = std::clamp(agent_exc + charge * 0.20f, 0.0f, 1.0f);
                }

                if (excitations) excitations[a] = agent_exc;
                if (emo_masks)   emo_masks[a]   = mask;
            }
        } else {
            for (int64_t i = 0; i < b_cap; ++i) {
                const float conf = confs ? confs[i] : 1.0f;
                emo_charges[i]   = std::clamp(emo_charges[i] * (1.0f - excitation_decay_rate) + conf * 0.25f, 0.0f, 1.0f);
            }
        }
    }
};

static_assert(sizeof(InstinctiveAffectAppraisalTransformLogic<>) == 64, "InstinctiveAffectAppraisalTransformLogic must be 64 bytes!");

} // namespace ideam::core
