#pragma once

/**
 * contradiction_and_trust_reconcile_transform_logic.h
 * IdeamMinds Transform Task (`MND11` — Wave 5 Epistemic Reconciliation):
 * Batch DOD execution of `InformationMatrixDOD::ingest_heard_claim()`:
 *   Drains `SpeechClaimEventRing` (`ClaimPod`), detects contradictions against high-conviction
 *   beliefs (`confidence + acceptance > contradiction_threshold`), flags `FACT_FLAG_CONTRADICTED`,
 *   decays `trust_in_listener` / speaker trust in `InstinctiveTraitWeightsPod`, detects when a
 *   heard claim corrects a misreferenced catalogue entry (`catalogued_fact_id != true_fact_id`),
 *   and pushes `MindEpistemicEventPod` notifications to `MindIntuitionAndContradictionRing`.
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
struct alignas(64) ContradictionAndTrustReconcileTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = RingView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::RING;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Contradiction & Trust Reconcile";
    static constexpr std::string_view ui_category  = "Minds/Reflection";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                             = INVALID_ID; // 4B
    uint32_t speech_claim_event_ring_buffer_id            = INVALID_ID; // 4B: ClaimPod (32B Ring)
    uint32_t speech_claim_ring_cursor_buffer_id           = INVALID_ID; // 4B: uint32_t[2] ring cursor
    uint32_t belief_catalogued_fact_id_buffer_id          = INVALID_ID; // 4B: uint32_t catalogued_fact_id
    uint32_t belief_true_fact_id_buffer_id                = INVALID_ID; // 4B: uint32_t true_fact_id
    uint32_t belief_believed_value_buffer_id              = INVALID_ID; // 4B: int64_t believed_value
    uint32_t belief_confidence_buffer_id                  = INVALID_ID; // 4B: float confidence
    uint32_t belief_acceptance_buffer_id                  = INVALID_ID; // 4B: float acceptance
    uint32_t belief_flags_buffer_id                       = INVALID_ID; // 4B: uint8_t FactFlagBits
    uint32_t agent_trait_weights_buffer_id                = INVALID_ID; // 4B: InstinctiveTraitWeightsPod
    uint32_t mind_intuition_and_contradiction_ring_buffer_id = INVALID_ID; // 4B: MindEpistemicEventPod ring
    uint32_t mind_intuition_ring_cursor_buffer_id         = INVALID_ID; // 4B: uint32_t[2] ring cursor
    float contradiction_conviction_threshold              = 0.70f;      // 4B: (confidence + acceptance) above which contradiction triggers
    float trust_decay_on_contradiction                    = 0.18f;      // 4B: Amount subtracted from trust_in_listener on contradiction
    bool drain_claim_ring_on_execute                      = true;       // 1B
    uint8_t reserved_padding[7]                           = {0};        // 7B -> Total = 64B

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
        add_buf("speech_claim_event_ring_buffer_id");
        add_buf("speech_claim_ring_cursor_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("belief_true_fact_id_buffer_id");
        add_buf("belief_believed_value_buffer_id");
        add_buf("belief_confidence_buffer_id");
        add_buf("belief_acceptance_buffer_id");
        add_buf("belief_flags_buffer_id");
        add_buf("agent_trait_weights_buffer_id");
        add_buf("mind_intuition_and_contradiction_ring_buffer_id");
        add_buf("mind_intuition_ring_cursor_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                               target_buffer_id                               = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("speech_claim_event_ring_buffer_id"))              speech_claim_event_ring_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["speech_claim_event_ring_buffer_id"]));
        if (p_props.has("speech_claim_ring_cursor_buffer_id"))             speech_claim_ring_cursor_buffer_id             = static_cast<uint32_t>(static_cast<int64_t>(p_props["speech_claim_ring_cursor_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id"))            belief_catalogued_fact_id_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("belief_true_fact_id_buffer_id"))                  belief_true_fact_id_buffer_id                  = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_true_fact_id_buffer_id"]));
        if (p_props.has("belief_believed_value_buffer_id"))                belief_believed_value_buffer_id                = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_believed_value_buffer_id"]));
        if (p_props.has("belief_confidence_buffer_id"))                    belief_confidence_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_confidence_buffer_id"]));
        if (p_props.has("belief_acceptance_buffer_id"))                    belief_acceptance_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_acceptance_buffer_id"]));
        if (p_props.has("belief_flags_buffer_id"))                         belief_flags_buffer_id                         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_flags_buffer_id"]));
        if (p_props.has("agent_trait_weights_buffer_id"))                  agent_trait_weights_buffer_id                  = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_trait_weights_buffer_id"]));
        if (p_props.has("mind_intuition_and_contradiction_ring_buffer_id"))mind_intuition_and_contradiction_ring_buffer_id= static_cast<uint32_t>(static_cast<int64_t>(p_props["mind_intuition_and_contradiction_ring_buffer_id"]));
        if (p_props.has("mind_intuition_ring_cursor_buffer_id"))           mind_intuition_ring_cursor_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["mind_intuition_ring_cursor_buffer_id"]));
        if (p_props.has("contradiction_conviction_threshold"))             contradiction_conviction_threshold             = static_cast<float>(static_cast<double>(p_props["contradiction_conviction_threshold"]));
        if (p_props.has("trust_decay_on_contradiction"))                   trust_decay_on_contradiction                   = static_cast<float>(static_cast<double>(p_props["trust_decay_on_contradiction"]));
        if (p_props.has("drain_claim_ring_on_execute"))                    drain_claim_ring_on_execute                    = static_cast<bool>(p_props["drain_claim_ring_on_execute"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                  return target_buffer_id;
        if (speech_claim_event_ring_buffer_id != INVALID_ID) return speech_claim_event_ring_buffer_id;
        return belief_flags_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *cring_part = p_context.get_grant_part(speech_claim_event_ring_buffer_id);
        const GrantPartPOD *ccur_part  = p_context.get_grant_part(speech_claim_ring_cursor_buffer_id);
        const GrantPartPOD *cfid_part  = p_context.get_grant_part(belief_catalogued_fact_id_buffer_id);
        const GrantPartPOD *tfid_part  = p_context.get_grant_part(belief_true_fact_id_buffer_id);
        const GrantPartPOD *bval_part  = p_context.get_grant_part(belief_believed_value_buffer_id);
        const GrantPartPOD *conf_part  = p_context.get_grant_part(belief_confidence_buffer_id);
        const GrantPartPOD *acc_part   = p_context.get_grant_part(belief_acceptance_buffer_id);
        const GrantPartPOD *flg_part   = p_context.get_grant_part(belief_flags_buffer_id);
        const GrantPartPOD *trt_part   = p_context.get_grant_part(agent_trait_weights_buffer_id);
        const GrantPartPOD *ering_part = p_context.get_grant_part(mind_intuition_and_contradiction_ring_buffer_id);
        const GrantPartPOD *ecur_part  = p_context.get_grant_part(mind_intuition_ring_cursor_buffer_id);

        if (!cring_part || !cring_part->raw_base_ptr || !ccur_part || !ccur_part->raw_base_ptr || !cfid_part || !cfid_part->raw_base_ptr) return;

        const auto *claims   = reinterpret_cast<const ClaimPod *>(cring_part->raw_base_ptr);
        const uint32_t c_cap = static_cast<uint32_t>(cring_part->capacity_bytes / sizeof(ClaimPod));
        auto *c_cursors      = reinterpret_cast<uint32_t *>(ccur_part->raw_base_ptr);

        auto *cat_fids       = reinterpret_cast<uint32_t *>(cfid_part->raw_base_ptr);
        auto *true_fids      = (tfid_part && tfid_part->raw_base_ptr) ? reinterpret_cast<uint32_t *>(tfid_part->raw_base_ptr) : cat_fids;
        auto *bvals          = (bval_part && bval_part->raw_base_ptr) ? reinterpret_cast<int64_t *>(bval_part->raw_base_ptr)  : nullptr;
        auto *confs          = (conf_part && conf_part->raw_base_ptr) ? reinterpret_cast<float *>(conf_part->raw_base_ptr)    : nullptr;
        auto *accs           = (acc_part && acc_part->raw_base_ptr)   ? reinterpret_cast<float *>(acc_part->raw_base_ptr)     : nullptr;
        auto *flags          = (flg_part && flg_part->raw_base_ptr)   ? reinterpret_cast<uint8_t *>(flg_part->raw_base_ptr)   : nullptr;
        auto *traits         = (trt_part && trt_part->raw_base_ptr)   ? reinterpret_cast<InstinctiveTraitWeightsPod *>(trt_part->raw_base_ptr) : nullptr;
        const int64_t b_cap  = static_cast<int64_t>(cfid_part->capacity_bytes / sizeof(uint32_t));
        const int64_t a_cap  = traits ? static_cast<int64_t>(trt_part->capacity_bytes / sizeof(InstinctiveTraitWeightsPod)) : 0;

        auto *ev_ring        = (ering_part && ering_part->raw_base_ptr) ? reinterpret_cast<MindEpistemicEventPod *>(ering_part->raw_base_ptr) : nullptr;
        const uint32_t e_cap = ev_ring ? static_cast<uint32_t>(ering_part->capacity_bytes / sizeof(MindEpistemicEventPod)) : 0u;
        auto *e_cursors      = (ecur_part && ecur_part->raw_base_ptr) ? reinterpret_cast<uint32_t *>(ecur_part->raw_base_ptr) : nullptr;

        const uint32_t write_idx = std::min(c_cursors[1], c_cap);
        uint32_t read_idx        = std::min(c_cursors[0], write_idx);

        while (read_idx < write_idx) {
            const ClaimPod &claim = claims[read_idx++];
            for (int64_t i = 0; i < b_cap; ++i) {
                if (cat_fids[i] != claim.fact_id) continue;

                const float conviction = (confs ? confs[i] : 0.0f) + (accs ? accs[i] : 0.0f);
                const int64_t cur_val  = bvals ? bvals[i] : 0;

                if (cur_val != claim.claimed_value && conviction >= contradiction_conviction_threshold) {
                    // Contradiction detected!
                    if (flags) {
                        flags[i] |= static_cast<uint8_t>(FACT_FLAG_CONTRADICTED);
                    }
                    if (traits && static_cast<int64_t>(claim.listener_agent_id) < a_cap) {
                        float &trust = traits[claim.listener_agent_id].trust_in_listener;
                        trust = std::clamp(trust - trust_decay_on_contradiction, 0.0f, 1.0f);
                    }
                    if (ev_ring && e_cursors && e_cursors[1] < e_cap) {
                        MindEpistemicEventPod ev{};
                        ev.agent_id       = claim.listener_agent_id;
                        ev.fact_id        = claim.fact_id;
                        ev.other_agent_id = claim.speaker_agent_id;
                        ev.tome_id        = claim.tome_id;
                        ev.event_kind     = MIND_EVENT_CONTRADICTION_DETECTED;
                        ev.priority       = static_cast<uint8_t>(StimulusPriority::OLD_THREAT_CHANGED);
                        ev_ring[e_cursors[1]++] = ev;
                    }
                } else {
                    // Reconcile belief & check if a misreference was corrected
                    if (bvals) bvals[i] = claim.claimed_value;
                    if (confs) confs[i] = std::clamp(std::max(confs[i], claim.claimed_confidence * 0.75f), 0.0f, 1.0f);
                    if (true_fids && true_fids[i] != cat_fids[i] && claim.referenced_fact_id == cat_fids[i]) {
                        true_fids[i] = cat_fids[i];
                        if (flags) flags[i] &= ~static_cast<uint8_t>(FACT_FLAG_MISREFERENCED);
                        if (ev_ring && e_cursors && e_cursors[1] < e_cap) {
                            MindEpistemicEventPod ev{};
                            ev.agent_id       = claim.listener_agent_id;
                            ev.fact_id        = claim.fact_id;
                            ev.other_agent_id = claim.speaker_agent_id;
                            ev.tome_id        = claim.tome_id;
                            ev.event_kind     = MIND_EVENT_MISREFERENCE_CORRECTED;
                            ev.priority       = static_cast<uint8_t>(StimulusPriority::OLD_BENEFICIAL_CHANGED);
                            ev_ring[e_cursors[1]++] = ev;
                        }
                    }
                }
            }
        }

        if (drain_claim_ring_on_execute) {
            c_cursors[0] = 0u;
            c_cursors[1] = 0u;
        }
    }
};

static_assert(sizeof(ContradictionAndTrustReconcileTransformLogic<>) == 64, "ContradictionAndTrustReconcileTransformLogic must be 64 bytes!");

} // namespace ideam::core
