#pragma once

/**
 * deductive_syllogism_propagate_transform_logic.h
 * IdeamMinds Transform Task (`MND8` — Wave 4 Deduction):
 * Ports `Deduction.cs` (`Syllogism`, `SyllogisticProposition`, `TransitiveInference`, `Explanation`):
 *   Propagates categorical and transitive deductions across `BeliefPod` rows using the 1-cycle
 *   256-bit `SYLLOGISM_VALIDITY_LUT` (`is_syllogism_valid()`), tests `BeliefPod::is_knowledge()`
 *   against the Multi-Tome Akashic Ground Truth (`WorldGroundTruthFactTable`) while verifying
 *   that neither premise nor conclusion is misreferenced (`catalogued_fact_id == true_fact_id`),
 *   and flags `FACT_FLAG_DEDUCTIVE_CHAIN`.
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/swap_view.h"
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
struct alignas(64) DeductiveSyllogismPropagateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32 | DataType::BYTE;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Deductive Syllogism Propagate";
    static constexpr std::string_view ui_category  = "Minds/Reasoning";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                             = INVALID_ID; // 4B
    uint32_t syllogism_proposition_pod_buffer_id          = INVALID_ID; // 4B: SyllogisticPropositionPod (16B AoS)
    uint32_t belief_catalogued_fact_id_buffer_id          = INVALID_ID; // 4B: uint32_t catalogued_fact_id
    uint32_t belief_true_fact_id_buffer_id                = INVALID_ID; // 4B: uint32_t true_fact_id (misreference check)
    uint32_t belief_believed_value_buffer_id              = INVALID_ID; // 4B: int64_t believed_value
    uint32_t belief_confidence_buffer_id                  = INVALID_ID; // 4B: float confidence
    uint32_t belief_acceptance_buffer_id                  = INVALID_ID; // 4B: float acceptance
    uint32_t belief_flags_buffer_id                       = INVALID_ID; // 4B: uint8_t FactFlagBits
    uint32_t fact_truth_value_buffer_id                   = INVALID_ID; // 4B: int64_t Akashic Tome truth_value
    uint32_t mind_intuition_and_contradiction_ring_buffer_id = INVALID_ID; // 4B: MindEpistemicEventPod ring
    uint32_t mind_intuition_ring_cursor_buffer_id         = INVALID_ID; // 4B: uint32_t[2] ring cursor
    float knowledge_threshold                             = 1.0f;       // 4B: Reason.cs KnowledgeThreshold
    uint8_t max_deductive_passes                          = 3u;         // 1B: Multi-hop transitive/syllogistic passes
    bool emit_knowledge_attained_events                   = true;       // 1B: Push MIND_EVENT_KNOWLEDGE_ATTAINED
    uint8_t reserved_padding[14]                          = {0};        // 14B -> Total = 64B

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context, const GrantPartPOD *p_part) const noexcept {
        (void)p_part;
        if constexpr (requires { p_view.bind_secondary(p_part); }) {
            const uint32_t sec_id = (syllogism_proposition_pod_buffer_id != INVALID_ID)
                ? syllogism_proposition_pod_buffer_id
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
        add_buf("syllogism_proposition_pod_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("belief_true_fact_id_buffer_id");
        add_buf("belief_believed_value_buffer_id");
        add_buf("belief_confidence_buffer_id");
        add_buf("belief_acceptance_buffer_id");
        add_buf("belief_flags_buffer_id");
        add_buf("fact_truth_value_buffer_id");
        add_buf("mind_intuition_and_contradiction_ring_buffer_id");
        add_buf("mind_intuition_ring_cursor_buffer_id");

        godot::Dictionary kno;
        kno["name"]        = "knowledge_threshold";
        kno["type"]        = godot::Variant::FLOAT;
        kno["hint"]        = godot::PROPERTY_HINT_RANGE;
        kno["hint_string"] = "0.0,2.0,0.01";
        props.push_back(kno);

        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                               target_buffer_id                               = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("syllogism_proposition_pod_buffer_id"))            syllogism_proposition_pod_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["syllogism_proposition_pod_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id"))            belief_catalogued_fact_id_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("belief_true_fact_id_buffer_id"))                  belief_true_fact_id_buffer_id                  = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_true_fact_id_buffer_id"]));
        if (p_props.has("belief_believed_value_buffer_id"))                belief_believed_value_buffer_id                = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_believed_value_buffer_id"]));
        if (p_props.has("belief_confidence_buffer_id"))                    belief_confidence_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_confidence_buffer_id"]));
        if (p_props.has("belief_acceptance_buffer_id"))                    belief_acceptance_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_acceptance_buffer_id"]));
        if (p_props.has("belief_flags_buffer_id"))                         belief_flags_buffer_id                         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_flags_buffer_id"]));
        if (p_props.has("fact_truth_value_buffer_id"))                     fact_truth_value_buffer_id                     = static_cast<uint32_t>(static_cast<int64_t>(p_props["fact_truth_value_buffer_id"]));
        if (p_props.has("mind_intuition_and_contradiction_ring_buffer_id"))mind_intuition_and_contradiction_ring_buffer_id= static_cast<uint32_t>(static_cast<int64_t>(p_props["mind_intuition_and_contradiction_ring_buffer_id"]));
        if (p_props.has("mind_intuition_ring_cursor_buffer_id"))           mind_intuition_ring_cursor_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["mind_intuition_ring_cursor_buffer_id"]));
        if (p_props.has("knowledge_threshold"))                            knowledge_threshold                            = static_cast<float>(static_cast<double>(p_props["knowledge_threshold"]));
        if (p_props.has("max_deductive_passes"))                           max_deductive_passes                           = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_deductive_passes"]), 1, 16));
        if (p_props.has("emit_knowledge_attained_events"))                 emit_knowledge_attained_events                 = static_cast<bool>(p_props["emit_knowledge_attained_events"]);
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                    return target_buffer_id;
        if (belief_catalogued_fact_id_buffer_id != INVALID_ID) return belief_catalogued_fact_id_buffer_id;
        return syllogism_proposition_pod_buffer_id;
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD &p_context, T_View &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *syl_part  = p_context.get_grant_part(syllogism_proposition_pod_buffer_id);
        const GrantPartPOD *cfid_part = p_context.get_grant_part(belief_catalogued_fact_id_buffer_id);
        const GrantPartPOD *tfid_part = p_context.get_grant_part(belief_true_fact_id_buffer_id);
        const GrantPartPOD *bval_part = p_context.get_grant_part(belief_believed_value_buffer_id);
        const GrantPartPOD *conf_part = p_context.get_grant_part(belief_confidence_buffer_id);
        const GrantPartPOD *acc_part  = p_context.get_grant_part(belief_acceptance_buffer_id);
        const GrantPartPOD *flg_part  = p_context.get_grant_part(belief_flags_buffer_id);
        const GrantPartPOD *tval_part = p_context.get_grant_part(fact_truth_value_buffer_id);
        const GrantPartPOD *ring_part = p_context.get_grant_part(mind_intuition_and_contradiction_ring_buffer_id);
        const GrantPartPOD *cur_part  = p_context.get_grant_part(mind_intuition_ring_cursor_buffer_id);

        if (!cfid_part || !cfid_part->raw_base_ptr || !conf_part || !conf_part->raw_base_ptr) return;

        const auto *cat_fids  = reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr);
        const auto *true_fids = (tfid_part && tfid_part->raw_base_ptr) ? reinterpret_cast<const uint32_t *>(tfid_part->raw_base_ptr) : cat_fids;
        auto *bvals           = (bval_part && bval_part->raw_base_ptr) ? reinterpret_cast<int64_t *>(bval_part->raw_base_ptr)        : nullptr;
        auto *confs           = reinterpret_cast<float *>(conf_part->raw_base_ptr);
        auto *accs            = (acc_part && acc_part->raw_base_ptr)   ? reinterpret_cast<float *>(acc_part->raw_base_ptr)           : nullptr;
        auto *flags           = (flg_part && flg_part->raw_base_ptr)   ? reinterpret_cast<uint8_t *>(flg_part->raw_base_ptr)         : nullptr;
        const auto *tvals     = (tval_part && tval_part->raw_base_ptr) ? reinterpret_cast<const int64_t *>(tval_part->raw_base_ptr)  : nullptr;

        const int64_t b_cap   = static_cast<int64_t>(cfid_part->capacity_bytes / sizeof(uint32_t));
        const int64_t f_cap   = tvals ? static_cast<int64_t>(tval_part->capacity_bytes / sizeof(int64_t)) : 0;

        const auto *syls      = (syl_part && syl_part->raw_base_ptr) ? reinterpret_cast<const SyllogisticPropositionPod *>(syl_part->raw_base_ptr) : nullptr;
        const int64_t syl_cap = syls ? static_cast<int64_t>(syl_part->capacity_bytes / sizeof(SyllogisticPropositionPod)) : 0;

        auto *ev_ring         = (ring_part && ring_part->raw_base_ptr) ? reinterpret_cast<MindEpistemicEventPod *>(ring_part->raw_base_ptr) : nullptr;
        const uint32_t r_cap  = ev_ring ? static_cast<uint32_t>(ring_part->capacity_bytes / sizeof(MindEpistemicEventPod)) : 0u;
        auto *cursors         = (cur_part && cur_part->raw_base_ptr) ? reinterpret_cast<uint32_t *>(cur_part->raw_base_ptr) : nullptr;

        auto find_belief_slot = [&](uint32_t fact_id) noexcept -> int64_t {
            for (int64_t i = 0; i < b_cap; ++i) {
                if (cat_fids[i] == fact_id) return i;
            }
            return -1;
        };

        if (syls && syl_cap > 0) {
            const uint8_t passes = std::max<uint8_t>(1u, max_deductive_passes);
            for (uint8_t p = 0u; p < passes; ++p) {
                bool any_updated = false;
                for (int64_t s = 0; s < syl_cap; ++s) {
                    const SyllogisticPropositionPod &prop = syls[s];
                    // 1-cycle LUT check from mind_common.h (`SYLLOGISM_VALIDITY_LUT`)
                    if (!prop.is_valid_deduction()) continue;

                    const int64_t maj_slot = find_belief_slot(prop.major_premise_fact_id);
                    const int64_t min_slot = find_belief_slot(prop.minor_premise_fact_id);
                    const int64_t con_slot = find_belief_slot(prop.conclusion_fact_id);
                    if (maj_slot < 0 || min_slot < 0 || con_slot < 0) continue;

                    const float deduced_conf = std::min(confs[maj_slot], confs[min_slot]);
                    const float deduced_acc  = accs ? std::min(accs[maj_slot], accs[min_slot]) : 0.0f;

                    if (deduced_conf > confs[con_slot]) {
                        confs[con_slot] = deduced_conf;
                        if (accs) accs[con_slot] = std::max(accs[con_slot], deduced_acc);
                        if (bvals && prop.is_transitive != 0u && bvals[maj_slot] == bvals[min_slot]) {
                            bvals[con_slot] = bvals[maj_slot];
                        }
                        if (flags) {
                            flags[con_slot] |= static_cast<uint8_t>(FACT_FLAG_DEDUCTIVE_CHAIN);
                        }
                        any_updated = true;
                    }
                }
                if (!any_updated) break;
            }
        }

        // Verify Knowledge attainment against Akashic Tome ground truth & reference fidelity
        if (emit_knowledge_attained_events && tvals && bvals && ev_ring && cursors) {
            for (int64_t i = 0; i < b_cap; ++i) {
                const uint32_t cfid = cat_fids[i];
                const uint32_t tfid = true_fids[i];
                if (cfid == 0u || cfid != tfid) continue; // Misreferenced beliefs cannot be true Knowledge!

                const int64_t f_idx = static_cast<int64_t>(tfid - 1u);
                if (f_idx < 0 || f_idx >= f_cap) continue;

                const float total_conviction = confs[i] + (accs ? accs[i] : 0.0f);
                if (bvals[i] == tvals[f_idx] && total_conviction >= knowledge_threshold && cursors[1] < r_cap) {
                    MindEpistemicEventPod ev{};
                    ev.agent_id   = 0u;
                    ev.fact_id    = cfid;
                    ev.event_kind = MIND_EVENT_KNOWLEDGE_ATTAINED;
                    ev.priority   = static_cast<uint8_t>(StimulusPriority::OLD_BENEFICIAL_CHANGED);
                    ev_ring[cursors[1]++] = ev;
                }
            }
        }
    }
};

static_assert(sizeof(DeductiveSyllogismPropagateTransformLogic<>) == 64, "DeductiveSyllogismPropagateTransformLogic must be 64 bytes!");

} // namespace ideam::core
