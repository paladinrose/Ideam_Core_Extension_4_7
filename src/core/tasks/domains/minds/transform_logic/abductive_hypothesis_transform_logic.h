#pragma once

/**
 * abductive_hypothesis_transform_logic.h
 * IdeamMinds Transform Task (`MND5` — Wave 3 Abduction & Intuition):
 * Ports `Abduction.cs` (`Observation`, `Hypothesis`, `TheoryGroup`, `AbductiveArgument.Consistency`)
 * and `InformationMatrix.cs` near-miss intuition:
 *   Evaluates candidate `HypothesisPod` rows (`theory_group_id`, `consistency_score`) against
 *   observed `ObservationPod::clarity` (or `belief_confidence_buffer_id`). Updates
 *   `BeliefPod::acceptance`; when an abductive check fails within `intuition_margin`,
 *   increments `BeliefPod::intuition_charge` so `MND6 IntuitiveLeapStochasticQueryLogic`
 *   can trigger an intuitive insight.
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
struct alignas(64) AbductiveHypothesisTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = AOSOAView<T, 8, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Abductive Hypothesis";
    static constexpr std::string_view ui_category  = "Minds/Reasoning";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                    = INVALID_ID; // 4B
    uint32_t belief_acceptance_buffer_id         = INVALID_ID; // 4B: float acceptance column
    uint32_t belief_intuition_charge_buffer_id   = INVALID_ID; // 4B: float intuition_charge column
    uint32_t belief_confidence_buffer_id         = INVALID_ID; // 4B: float confidence / clarity column
    uint32_t belief_catalogued_fact_id_buffer_id = INVALID_ID; // 4B: uint32_t catalogued_fact_id column
    uint32_t hypothesis_pod_buffer_id            = INVALID_ID; // 4B: HypothesisPod (16B AoS)
    uint32_t observation_pod_buffer_id           = INVALID_ID; // 4B: ObservationPod (16B AoS)
    float acceptance_floor                       = 0.50f;      // 4B: Minimum (clarity * consistency) to accept hypothesis
    float intuition_margin                       = 0.25f;      // 4B: Near-miss margin below acceptance_floor that charges intuition
    float intuition_charge_gain                  = 0.25f;      // 4B: Gain added to intuition_charge on near-miss
    float rival_theory_suppression               = 0.15f;      // 4B: Acceptance dampening applied to competing theory_group_id rows
    uint8_t reserved_padding[20]                 = {0};        // 20B -> Total = 64B

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
        add_buf("belief_acceptance_buffer_id");
        add_buf("belief_intuition_charge_buffer_id");
        add_buf("belief_confidence_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("hypothesis_pod_buffer_id");
        add_buf("observation_pod_buffer_id");

        godot::Dictionary flr;
        flr["name"]        = "acceptance_floor";
        flr["type"]        = godot::Variant::FLOAT;
        flr["hint"]        = godot::PROPERTY_HINT_RANGE;
        flr["hint_string"] = "0.0,1.0,0.01";
        props.push_back(flr);

        godot::Dictionary mrg;
        mrg["name"]        = "intuition_margin";
        mrg["type"]        = godot::Variant::FLOAT;
        mrg["hint"]        = godot::PROPERTY_HINT_RANGE;
        mrg["hint_string"] = "0.0,1.0,0.01";
        props.push_back(mrg);

        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                    target_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("belief_acceptance_buffer_id"))         belief_acceptance_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_acceptance_buffer_id"]));
        if (p_props.has("belief_intuition_charge_buffer_id"))   belief_intuition_charge_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_intuition_charge_buffer_id"]));
        if (p_props.has("belief_confidence_buffer_id"))         belief_confidence_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_confidence_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id")) belief_catalogued_fact_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("hypothesis_pod_buffer_id"))            hypothesis_pod_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["hypothesis_pod_buffer_id"]));
        if (p_props.has("observation_pod_buffer_id"))           observation_pod_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["observation_pod_buffer_id"]));
        if (p_props.has("acceptance_floor"))                    acceptance_floor                    = static_cast<float>(static_cast<double>(p_props["acceptance_floor"]));
        if (p_props.has("intuition_margin"))                    intuition_margin                    = static_cast<float>(static_cast<double>(p_props["intuition_margin"]));
        if (p_props.has("intuition_charge_gain"))               intuition_charge_gain               = static_cast<float>(static_cast<double>(p_props["intuition_charge_gain"]));
        if (p_props.has("rival_theory_suppression"))            rival_theory_suppression            = static_cast<float>(static_cast<double>(p_props["rival_theory_suppression"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)          return target_buffer_id;
        if (belief_acceptance_buffer_id != INVALID_ID) return belief_acceptance_buffer_id;
        return belief_intuition_charge_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *acc_part  = p_context.get_grant_part(belief_acceptance_buffer_id);
        const GrantPartPOD *intu_part = p_context.get_grant_part(belief_intuition_charge_buffer_id);
        const GrantPartPOD *conf_part = p_context.get_grant_part(belief_confidence_buffer_id);
        const GrantPartPOD *cfid_part = p_context.get_grant_part(belief_catalogued_fact_id_buffer_id);
        const GrantPartPOD *hyp_part  = p_context.get_grant_part(hypothesis_pod_buffer_id);
        const GrantPartPOD *obs_part  = p_context.get_grant_part(observation_pod_buffer_id);

        if (!acc_part || !acc_part->raw_base_ptr) return;

        auto *acceptances     = reinterpret_cast<float *>(acc_part->raw_base_ptr);
        const int64_t b_cap   = static_cast<int64_t>(acc_part->capacity_bytes / sizeof(float));
        auto *intu_charges    = (intu_part && intu_part->raw_base_ptr) ? reinterpret_cast<float *>(intu_part->raw_base_ptr)              : nullptr;
        const auto *confs     = (conf_part && conf_part->raw_base_ptr) ? reinterpret_cast<const float *>(conf_part->raw_base_ptr)        : nullptr;
        const auto *cat_fids  = (cfid_part && cfid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr)     : nullptr;
        const auto *hyps      = (hyp_part && hyp_part->raw_base_ptr)   ? reinterpret_cast<const HypothesisPod *>(hyp_part->raw_base_ptr) : nullptr;
        const int64_t h_cap   = hyps ? static_cast<int64_t>(hyp_part->capacity_bytes / sizeof(HypothesisPod)) : 0;
        const auto *obs_rows  = (obs_part && obs_part->raw_base_ptr)   ? reinterpret_cast<const ObservationPod *>(obs_part->raw_base_ptr): nullptr;
        const int64_t obs_cap = obs_rows ? static_cast<int64_t>(obs_part->capacity_bytes / sizeof(ObservationPod)) : 0;

        for (int64_t i = 0; i < b_cap; ++i) {
            const uint32_t fid = cat_fids ? cat_fids[i] : static_cast<uint32_t>(i + 1);
            float clarity      = confs ? confs[i] : 0.5f;
            if (obs_rows && i < obs_cap && obs_rows[i].fact_id == fid) {
                clarity = std::max(clarity, obs_rows[i].clarity);
            }

            float consistency = 1.0f;
            if (hyps && h_cap > 0) {
                for (int64_t h = 0; h < h_cap; ++h) {
                    if (hyps[h].hypothesis_fact_id == fid) {
                        consistency = hyps[h].consistency_score;
                        break;
                    }
                }
            }

            const float score = std::clamp(clarity * consistency, 0.0f, 1.0f);
            if (score >= acceptance_floor) {
                acceptances[i] = std::clamp(std::max(acceptances[i], score), 0.0f, 1.0f);
            } else if (intu_charges && (score + intuition_margin >= acceptance_floor)) {
                const float proximity = 1.0f - ((acceptance_floor - score) / std::max(0.0001f, intuition_margin));
                intu_charges[i] = std::clamp(intu_charges[i] + intuition_charge_gain * proximity, 0.0f, 1.0f);
            }
        }
    }
};

static_assert(sizeof(AbductiveHypothesisTransformLogic<>) == 64, "AbductiveHypothesisTransformLogic must be 64 bytes!");

} // namespace ideam::core
