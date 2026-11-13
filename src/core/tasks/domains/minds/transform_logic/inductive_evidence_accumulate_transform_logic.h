#pragma once

/**
 * inductive_evidence_accumulate_transform_logic.h
 * IdeamMinds Transform Task (`MND7` — Wave 3 Induction):
 * Ports `Induction.cs` (`Evidence`, `ProbableConclusion`, `Cogence`) and `Reason.cs::ApplyEvidence`:
 *   Aggregates repeated observations of `(subject_id, predicate_id)` / `target_fact_id` across ticks
 *   in `EvidencePod` rows and updates `BeliefPod::confidence` via asymptotic evidence strength
 *   accumulation (`confidence += (1 - confidence) * strength * cogence`).
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/atomic_view.h"
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
struct alignas(64) InductiveEvidenceAccumulateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = AtomicView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Inductive Evidence Accumulate";
    static constexpr std::string_view ui_category  = "Minds/Reasoning";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                    = INVALID_ID; // 4B
    uint32_t belief_confidence_buffer_id         = INVALID_ID; // 4B: float confidence column
    uint32_t belief_catalogued_fact_id_buffer_id = INVALID_ID; // 4B: uint32_t catalogued_fact_id column
    uint32_t evidence_pod_buffer_id              = INVALID_ID; // 4B: EvidencePod (16B AoS)
    uint32_t observation_pod_buffer_id           = INVALID_ID; // 4B: ObservationPod (16B AoS)
    float base_evidence_step                     = 0.12f;      // 4B: Default inductive increment per supporting sample
    float cogence_multiplier                     = 1.0f;       // 4B: Multiplier on EvidencePod::cogence
    float max_inductive_confidence               = 0.99f;      // 4B: Asymptotic inductive ceiling (< 1.0 since induction is probabilistic)
    uint8_t reserved_padding[32]                 = {0};        // 32B -> Total = 64B

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
        add_buf("belief_confidence_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("evidence_pod_buffer_id");
        add_buf("observation_pod_buffer_id");

        godot::Dictionary stp;
        stp["name"]        = "base_evidence_step";
        stp["type"]        = godot::Variant::FLOAT;
        stp["hint"]        = godot::PROPERTY_HINT_RANGE;
        stp["hint_string"] = "0.0,1.0,0.01";
        props.push_back(stp);

        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                    target_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("belief_confidence_buffer_id"))         belief_confidence_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_confidence_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id")) belief_catalogued_fact_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("evidence_pod_buffer_id"))              evidence_pod_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["evidence_pod_buffer_id"]));
        if (p_props.has("observation_pod_buffer_id"))           observation_pod_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["observation_pod_buffer_id"]));
        if (p_props.has("base_evidence_step"))                  base_evidence_step                  = static_cast<float>(static_cast<double>(p_props["base_evidence_step"]));
        if (p_props.has("cogence_multiplier"))                  cogence_multiplier                  = static_cast<float>(static_cast<double>(p_props["cogence_multiplier"]));
        if (p_props.has("max_inductive_confidence"))            max_inductive_confidence            = static_cast<float>(static_cast<double>(p_props["max_inductive_confidence"]));
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)            return target_buffer_id;
        if (belief_confidence_buffer_id != INVALID_ID) return belief_confidence_buffer_id;
        return evidence_pod_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *conf_part = p_context.get_grant_part(belief_confidence_buffer_id);
        const GrantPartPOD *cfid_part = p_context.get_grant_part(belief_catalogued_fact_id_buffer_id);
        const GrantPartPOD *ev_part   = p_context.get_grant_part(evidence_pod_buffer_id);

        if (!conf_part || !conf_part->raw_base_ptr) return;

        auto *confs          = reinterpret_cast<float *>(conf_part->raw_base_ptr);
        const int64_t b_cap  = static_cast<int64_t>(conf_part->capacity_bytes / sizeof(float));
        const auto *cat_fids = (cfid_part && cfid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr) : nullptr;
        auto *ev_rows        = (ev_part && ev_part->raw_base_ptr)     ? reinterpret_cast<EvidencePod *>(ev_part->raw_base_ptr)      : nullptr;
        const int64_t ev_cap = ev_rows ? static_cast<int64_t>(ev_part->capacity_bytes / sizeof(EvidencePod)) : 0;

        for (int64_t i = 0; i < b_cap; ++i) {
            const uint32_t fid = cat_fids ? cat_fids[i] : static_cast<uint32_t>(i + 1);
            float strength     = base_evidence_step;
            float cogence      = cogence_multiplier;

            if (ev_rows && ev_cap > 0) {
                for (int64_t e = 0; e < ev_cap; ++e) {
                    if (ev_rows[e].target_fact_id == fid) {
                        ++ev_rows[e].sample_count;
                        strength = std::max(strength, ev_rows[e].strength);
                        cogence  = ev_rows[e].cogence * cogence_multiplier;
                        break;
                    }
                }
            }

            const float delta = std::clamp(strength * cogence, 0.0f, 1.0f);
            confs[i] = std::clamp(confs[i] + (max_inductive_confidence - confs[i]) * delta, 0.0f, max_inductive_confidence);
        }
    }
};

static_assert(sizeof(InductiveEvidenceAccumulateTransformLogic<>) == 64, "InductiveEvidenceAccumulateTransformLogic must be 64 bytes!");

} // namespace ideam::core
