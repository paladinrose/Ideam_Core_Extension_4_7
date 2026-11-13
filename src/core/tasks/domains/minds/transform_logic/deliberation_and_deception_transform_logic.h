#pragma once

/**
 * deliberation_and_deception_transform_logic.h
 * IdeamMinds Transform Task (`MND9` — Wave 4 Deliberation & Deception):
 * Batch DOD execution of `InformationMatrixDOD::synthesize_claim()`:
 *   Weighs `self_preservation * 0.65 + guilt * 0.35` against `trust_in_listener * 0.60 + loyalty * 0.40`
 *   to emit truthful, withheld, or fabricated `ClaimPod` speech acts into `SpeechClaimEventRing`,
 *   and flags `FACT_FLAG_ALIBI_RECORD` self-facts when an agent fabricates a lie.
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
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
struct alignas(64) DeliberationAndDeceptionTransformLogic {
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

    static constexpr std::string_view display_name = "Deliberation & Deception";
    static constexpr std::string_view ui_category  = "Minds/Deliberation";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                    = INVALID_ID; // 4B
    uint32_t agent_trait_weights_buffer_id       = INVALID_ID; // 4B: InstinctiveTraitWeightsPod
    uint32_t belief_catalogued_fact_id_buffer_id = INVALID_ID; // 4B: uint32_t catalogued_fact_id
    uint32_t belief_true_fact_id_buffer_id       = INVALID_ID; // 4B: uint32_t true_fact_id
    uint32_t belief_believed_value_buffer_id     = INVALID_ID; // 4B: int64_t believed_value
    uint32_t belief_confidence_buffer_id         = INVALID_ID; // 4B: float confidence
    uint32_t belief_believed_tome_id_buffer_id   = INVALID_ID; // 4B: uint16_t believed_tome_id
    uint32_t belief_flags_buffer_id              = INVALID_ID; // 4B: uint8_t FactFlagBits
    uint32_t speech_claim_event_ring_buffer_id   = INVALID_ID; // 4B: ClaimPod (32B Ring)
    uint32_t speech_claim_ring_cursor_buffer_id  = INVALID_ID; // 4B: uint32_t[2] ring cursor
    int64_t fabricated_alibi_fallback_value      = -1;         // 8B: Default fabricated alibi value when lying
    uint32_t default_listener_agent_id           = 1u;         // 4B: Target listener ID
    float withhold_confidence_floor              = 0.15f;      // 4B: Below this confidence, withhold claim
    uint8_t reserved_padding[8]                  = {0};        // 8B -> Total = 64B

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
        add_buf("agent_trait_weights_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("belief_true_fact_id_buffer_id");
        add_buf("belief_believed_value_buffer_id");
        add_buf("belief_confidence_buffer_id");
        add_buf("belief_believed_tome_id_buffer_id");
        add_buf("belief_flags_buffer_id");
        add_buf("speech_claim_event_ring_buffer_id");
        add_buf("speech_claim_ring_cursor_buffer_id");
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                    target_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("agent_trait_weights_buffer_id"))       agent_trait_weights_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_trait_weights_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id")) belief_catalogued_fact_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("belief_true_fact_id_buffer_id"))       belief_true_fact_id_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_true_fact_id_buffer_id"]));
        if (p_props.has("belief_believed_value_buffer_id"))     belief_believed_value_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_believed_value_buffer_id"]));
        if (p_props.has("belief_confidence_buffer_id"))         belief_confidence_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_confidence_buffer_id"]));
        if (p_props.has("belief_believed_tome_id_buffer_id"))   belief_believed_tome_id_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_believed_tome_id_buffer_id"]));
        if (p_props.has("belief_flags_buffer_id"))              belief_flags_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_flags_buffer_id"]));
        if (p_props.has("speech_claim_event_ring_buffer_id"))   speech_claim_event_ring_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["speech_claim_event_ring_buffer_id"]));
        if (p_props.has("speech_claim_ring_cursor_buffer_id"))  speech_claim_ring_cursor_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["speech_claim_ring_cursor_buffer_id"]));
        if (p_props.has("fabricated_alibi_fallback_value"))     fabricated_alibi_fallback_value     = static_cast<int64_t>(p_props["fabricated_alibi_fallback_value"]);
        if (p_props.has("default_listener_agent_id"))           default_listener_agent_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["default_listener_agent_id"]));
        if (p_props.has("withhold_confidence_floor"))           withhold_confidence_floor           = static_cast<float>(static_cast<double>(p_props["withhold_confidence_floor"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                    return target_buffer_id;
        if (belief_catalogued_fact_id_buffer_id != INVALID_ID) return belief_catalogued_fact_id_buffer_id;
        return speech_claim_event_ring_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *trt_part  = p_context.get_grant_part(agent_trait_weights_buffer_id);
        const GrantPartPOD *cfid_part = p_context.get_grant_part(belief_catalogued_fact_id_buffer_id);
        const GrantPartPOD *tfid_part = p_context.get_grant_part(belief_true_fact_id_buffer_id);
        const GrantPartPOD *bval_part = p_context.get_grant_part(belief_believed_value_buffer_id);
        const GrantPartPOD *conf_part = p_context.get_grant_part(belief_confidence_buffer_id);
        const GrantPartPOD *tome_part = p_context.get_grant_part(belief_believed_tome_id_buffer_id);
        const GrantPartPOD *flg_part  = p_context.get_grant_part(belief_flags_buffer_id);
        const GrantPartPOD *ring_part = p_context.get_grant_part(speech_claim_event_ring_buffer_id);
        const GrantPartPOD *cur_part  = p_context.get_grant_part(speech_claim_ring_cursor_buffer_id);

        if (!cfid_part || !cfid_part->raw_base_ptr || !ring_part || !ring_part->raw_base_ptr || !cur_part || !cur_part->raw_base_ptr) return;

        const auto *traits    = (trt_part && trt_part->raw_base_ptr) ? reinterpret_cast<const InstinctiveTraitWeightsPod *>(trt_part->raw_base_ptr) : nullptr;
        const auto *cat_fids  = reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr);
        const auto *true_fids = (tfid_part && tfid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(tfid_part->raw_base_ptr) : cat_fids;
        const auto *bvals     = (bval_part && bval_part->raw_base_ptr) ? reinterpret_cast<const int64_t *>(bval_part->raw_base_ptr)  : nullptr;
        const auto *confs     = (conf_part && conf_part->raw_base_ptr) ? reinterpret_cast<const float *>(conf_part->raw_base_ptr)    : nullptr;
        const auto *tomes     = (tome_part && tome_part->raw_base_ptr) ? reinterpret_cast<const uint16_t *>(tome_part->raw_base_ptr) : nullptr;
        auto *flags           = (flg_part && flg_part->raw_base_ptr)   ? reinterpret_cast<uint8_t *>(flg_part->raw_base_ptr)         : nullptr;

        auto *claim_ring      = reinterpret_cast<ClaimPod *>(ring_part->raw_base_ptr);
        const uint32_t r_cap  = static_cast<uint32_t>(ring_part->capacity_bytes / sizeof(ClaimPod));
        auto *cursors         = reinterpret_cast<uint32_t *>(cur_part->raw_base_ptr);
        const int64_t b_cap   = static_cast<int64_t>(cfid_part->capacity_bytes / sizeof(uint32_t));

        const InstinctiveTraitWeightsPod tr = (traits != nullptr) ? traits[0] : InstinctiveTraitWeightsPod{};
        const float lie_drive   = tr.self_preservation * 0.65f + tr.guilt * 0.35f;
        const float truth_drive = tr.trust_in_listener * 0.60f + tr.loyalty * 0.40f;
        const bool should_lie   = (lie_drive > truth_drive);

        for (int64_t i = 0; i < b_cap && cursors[1] < r_cap; ++i) {
            const uint32_t cfid = cat_fids[i];
            if (cfid == 0u) continue;
            const float conf = confs ? confs[i] : 1.0f;
            if (conf < withhold_confidence_floor) continue;

            ClaimPod claim{};
            claim.speaker_agent_id   = 0u;
            claim.listener_agent_id  = default_listener_agent_id;
            claim.fact_id            = cfid;
            claim.referenced_fact_id = true_fids[i];
            claim.tome_id            = tomes ? tomes[i] : static_cast<uint16_t>(TOME_PHYSICAL_REALITY);

            if (should_lie) {
                claim.claimed_value      = fabricated_alibi_fallback_value;
                claim.claimed_confidence = 0.9f;
                claim.is_intentional_lie = true;
                claim.sememic_form       = static_cast<uint8_t>(SememicForm::EVALUATIVE_CONNOTATION);
                if (flags) {
                    flags[i] |= static_cast<uint8_t>(FACT_FLAG_ALIBI_RECORD);
                }
            } else {
                claim.claimed_value      = bvals ? bvals[i] : 0;
                claim.claimed_confidence = conf;
                claim.is_intentional_lie = false;
                claim.sememic_form       = static_cast<uint8_t>(SememicForm::PRIMARY_DENOTATION);
            }

            claim_ring[cursors[1]++] = claim;
        }
    }
};

static_assert(sizeof(DeliberationAndDeceptionTransformLogic<>) == 64, "DeliberationAndDeceptionTransformLogic must be 64 bytes!");

} // namespace ideam::core
