#pragma once

/**
 * intuitive_leap_stochastic_query_logic.h
 * IdeamMinds Query Task (`MND6` — Wave 3 Intuition):
 * Ports `InformationMatrix.cs` Intuitive Leap:
 *   "When a check is made using logical methods (predominantly abduction, through observation)
 *    if the check fails, but is within a margin of error, it increases the odds of having
 *    an intuitive leap, which can eventually grant a piece of information."
 *
 * Evaluates `BeliefPod::intuition_charge` + agent `curiosity` weight against a deterministic
 * hash or threshold; when triggered, wakes the unconfirmed hypothesis `fact_id` in the agent's
 * belief selection as an intuitive insight and optionally pushes a `MIND_EVENT_INTUITIVE_LEAP`
 * notification into `MindIntuitionAndContradictionRing`.
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) IntuitiveLeapStochasticQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SparseSetView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Intuitive Leap Stochastic";
    static constexpr std::string_view ui_category  = "Minds/Reasoning";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                             = INVALID_ID; // 4B: Primary belief selection buffer
    uint32_t belief_intuition_charge_buffer_id            = INVALID_ID; // 4B: float intuition_charge column
    uint32_t agent_trait_weights_buffer_id                = INVALID_ID; // 4B: InstinctiveTraitWeightsPod (curiosity boost)
    uint32_t belief_catalogued_fact_id_buffer_id          = INVALID_ID; // 4B: uint32_t catalogued_fact_id column
    uint32_t belief_flags_buffer_id                       = INVALID_ID; // 4B: uint8_t FactFlagBits column
    uint32_t mind_intuition_and_contradiction_ring_buffer_id = INVALID_ID; // 4B: MindEpistemicEventPod ring
    uint32_t mind_intuition_ring_cursor_buffer_id         = INVALID_ID; // 4B: uint32_t[2] ring cursor
    uint32_t random_seed                                  = 0x9E3779B9u;// 4B: Deterministic SplitMix32 seed
    float intuition_leap_threshold                        = 0.65f;      // 4B: Threshold on (intuition_charge + curiosity * curiosity_scale)
    float curiosity_boost_scale                           = 0.25f;      // 4B: Weight of agent curiosity trait
    bool stochastic_roll_enabled                          = true;       // 1B: Roll pseudo-random check vs deterministic threshold
    bool emit_epistemic_ring_event                        = true;       // 1B: Push MIND_EVENT_INTUITIVE_LEAP on leap
    uint8_t reserved_padding[22]                          = {0};        // 22B -> Total = 64B

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        (void)p_view;
        (void)p_context;
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
        add_buf("belief_intuition_charge_buffer_id");
        add_buf("agent_trait_weights_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("belief_flags_buffer_id");
        add_buf("mind_intuition_and_contradiction_ring_buffer_id");
        add_buf("mind_intuition_ring_cursor_buffer_id");

        godot::Dictionary thr;
        thr["name"]        = "intuition_leap_threshold";
        thr["type"]        = godot::Variant::FLOAT;
        thr["hint"]        = godot::PROPERTY_HINT_RANGE;
        thr["hint_string"] = "0.0,1.5,0.01";
        props.push_back(thr);

        godot::Dictionary cur;
        cur["name"]        = "curiosity_boost_scale";
        cur["type"]        = godot::Variant::FLOAT;
        cur["hint"]        = godot::PROPERTY_HINT_RANGE;
        cur["hint_string"] = "0.0,1.0,0.01";
        props.push_back(cur);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                  return target_buffer_id;
        if (belief_intuition_charge_buffer_id != INVALID_ID) return belief_intuition_charge_buffer_id;
        return belief_catalogued_fact_id_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                               target_buffer_id                               = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("belief_intuition_charge_buffer_id"))              belief_intuition_charge_buffer_id              = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_intuition_charge_buffer_id"]));
        if (p_props.has("agent_trait_weights_buffer_id"))                  agent_trait_weights_buffer_id                  = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_trait_weights_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id"))            belief_catalogued_fact_id_buffer_id            = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("belief_flags_buffer_id"))                         belief_flags_buffer_id                         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_flags_buffer_id"]));
        if (p_props.has("mind_intuition_and_contradiction_ring_buffer_id"))mind_intuition_and_contradiction_ring_buffer_id= static_cast<uint32_t>(static_cast<int64_t>(p_props["mind_intuition_and_contradiction_ring_buffer_id"]));
        if (p_props.has("mind_intuition_ring_cursor_buffer_id"))           mind_intuition_ring_cursor_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["mind_intuition_ring_cursor_buffer_id"]));
        if (p_props.has("random_seed"))                                    random_seed                                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["random_seed"]));
        if (p_props.has("intuition_leap_threshold"))                       intuition_leap_threshold                       = static_cast<float>(static_cast<double>(p_props["intuition_leap_threshold"]));
        if (p_props.has("curiosity_boost_scale"))                          curiosity_boost_scale                          = static_cast<float>(static_cast<double>(p_props["curiosity_boost_scale"]));
        if (p_props.has("stochastic_roll_enabled"))                        stochastic_roll_enabled                        = static_cast<bool>(p_props["stochastic_roll_enabled"]);
        if (p_props.has("emit_epistemic_ring_event"))                      emit_epistemic_ring_event                      = static_cast<bool>(p_props["emit_epistemic_ring_event"]);
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context,
        const T_View &p_view
    ) const {
        (void)p_view;
        const uint32_t tgt_id   = get_target_buffer_id();
        const GrantPartPOD *int_part  = (belief_intuition_charge_buffer_id != INVALID_ID)               ? p_context.get_grant_part(belief_intuition_charge_buffer_id)               : nullptr;
        const GrantPartPOD *trt_part  = (agent_trait_weights_buffer_id != INVALID_ID)                   ? p_context.get_grant_part(agent_trait_weights_buffer_id)                   : nullptr;
        const GrantPartPOD *cfid_part = (belief_catalogued_fact_id_buffer_id != INVALID_ID)             ? p_context.get_grant_part(belief_catalogued_fact_id_buffer_id)             : nullptr;
        const GrantPartPOD *flg_part  = (belief_flags_buffer_id != INVALID_ID)                          ? p_context.get_grant_part(belief_flags_buffer_id)                          : nullptr;
        const GrantPartPOD *ring_part = (mind_intuition_and_contradiction_ring_buffer_id != INVALID_ID) ? p_context.get_grant_part(mind_intuition_and_contradiction_ring_buffer_id) : nullptr;
        const GrantPartPOD *cur_part  = (mind_intuition_ring_cursor_buffer_id != INVALID_ID)            ? p_context.get_grant_part(mind_intuition_ring_cursor_buffer_id)            : nullptr;

        const auto *intu = (int_part && int_part->raw_base_ptr)
            ? reinterpret_cast<const float *>(int_part->raw_base_ptr)
            : nullptr;
        const int64_t int_cap = intu ? static_cast<int64_t>(int_part->capacity_bytes / sizeof(float)) : 0;

        const auto *traits = (trt_part && trt_part->raw_base_ptr)
            ? reinterpret_cast<const InstinctiveTraitWeightsPod *>(trt_part->raw_base_ptr)
            : nullptr;
        const int64_t trt_cap = traits ? static_cast<int64_t>(trt_part->capacity_bytes / sizeof(InstinctiveTraitWeightsPod)) : 0;

        const auto *cat_fids = (cfid_part && cfid_part->raw_base_ptr)
            ? reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr)
            : nullptr;
        const int64_t cfid_cap = cat_fids ? static_cast<int64_t>(cfid_part->capacity_bytes / sizeof(uint32_t)) : 0;

        auto *flags = (flg_part && flg_part->raw_base_ptr)
            ? reinterpret_cast<uint8_t *>(flg_part->raw_base_ptr)
            : nullptr;
        const int64_t flg_cap = flags ? static_cast<int64_t>(flg_part->capacity_bytes / sizeof(uint8_t)) : 0;

        auto *ev_ring = (ring_part && ring_part->raw_base_ptr)
            ? reinterpret_cast<MindEpistemicEventPod *>(ring_part->raw_base_ptr)
            : nullptr;
        const uint32_t ring_cap = ev_ring ? static_cast<uint32_t>(ring_part->capacity_bytes / sizeof(MindEpistemicEventPod)) : 0u;
        auto *cursors = (cur_part && cur_part->raw_base_ptr)
            ? reinterpret_cast<uint32_t *>(cur_part->raw_base_ptr)
            : nullptr;

        const float base_curiosity = (traits && trt_cap > 0) ? traits[0].curiosity : 0.5f;

        auto eval_row = [&](int64_t idx) noexcept -> bool {
            if (!intu || idx < 0 || idx >= int_cap) return false;
            const float charge = intu[idx];
            if (charge <= 0.001f) return false;

            const float effective_potential = charge + base_curiosity * curiosity_boost_scale;
            bool leaped = false;

            if (stochastic_roll_enabled) {
                uint32_t z = random_seed + static_cast<uint32_t>(idx) * 0x9E3779B9u;
                z = (z ^ (z >> 16u)) * 0x85EBCA6Bu;
                z = (z ^ (z >> 13u)) * 0xC2B2AE35u;
                z = z ^ (z >> 16u);
                const float u01 = static_cast<float>(z & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
                leaped = (effective_potential >= intuition_leap_threshold) && (u01 <= effective_potential);
            } else {
                leaped = (effective_potential >= intuition_leap_threshold);
            }

            if (leaped) {
                if (flags && idx < flg_cap) {
                    flags[idx] |= static_cast<uint8_t>(FACT_FLAG_INTUITIVE_LEAP);
                }
                if (emit_epistemic_ring_event && ev_ring && cursors && cursors[1] < ring_cap) {
                    MindEpistemicEventPod ev{};
                    ev.agent_id   = 0u;
                    ev.fact_id    = (cat_fids && idx < cfid_cap) ? cat_fids[idx] : static_cast<uint32_t>(idx);
                    ev.event_kind = MIND_EVENT_INTUITIVE_LEAP;
                    ev.priority   = static_cast<uint8_t>(StimulusPriority::NEW_BENEFICIAL);
                    ev_ring[cursors[1]++] = ev;
                }
            }
            return leaped;
        };

        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                int64_t write_ptr = 0;
                for (int64_t i = 0; i < r_selection.element_count; ++i) {
                    const int64_t idx = r_selection.data.indices[i];
                    if (eval_row(idx)) {
                        r_selection.data.indices[write_ptr++] = idx;
                    }
                }
                r_selection.element_count = write_ptr;
            }
        } else if constexpr (Op == QueryOp::ADD) {
            const int64_t cap = std::min(r_selection.capacity, int_cap);
            for (int64_t i = 0; i < cap; ++i) {
                if (eval_row(i)) {
                    p_context.queue_selection_command(tgt_id, i);
                }
            }
        }
    }
};

static_assert(sizeof(IntuitiveLeapStochasticQueryLogic<>) == 64, "IntuitiveLeapStochasticQueryLogic must be 64 bytes!");

} // namespace ideam::core
