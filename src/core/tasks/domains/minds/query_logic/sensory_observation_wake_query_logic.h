#pragma once

/**
 * sensory_observation_wake_query_logic.h
 * IdeamMinds Query Task (`MND2`): Wakes observable `fact_id` / belief catalogue rows
 * in each agent's `SparseSetView` / `BridgeView` belief selection when decoded
 * `ObservationPod::clarity` (or `belief_confidence_buffer_id`) exceeds the agent's
 * `attention_threshold` sensory floor.
 *
 * Also supports filtering or waking misreferenced catalogue entries (`catalogued_fact_id != true_fact_id`)
 * and filtering by `AkashicTomeDomain` (`tome_id`).
 */

#include "../../../../minds/mind_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

template <typename T = int32_t>
struct alignas(64) SensoryObservationWakeQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = BridgeView<T, T, 1, DefaultStrategy>;

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

    static constexpr std::string_view display_name = "Sensory Observation Wake";
    static constexpr std::string_view ui_category  = "Minds/Perception";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                   = INVALID_ID; // 4B: Primary belief or observation selection buffer
    uint32_t observation_pod_buffer_id          = INVALID_ID; // 4B: ObservationPod (16B AoS)
    uint32_t belief_confidence_buffer_id        = INVALID_ID; // 4B: float confidence column
    uint32_t belief_catalogued_fact_id_buffer_id= INVALID_ID; // 4B: uint32_t catalogued_fact_id column
    uint32_t belief_true_fact_id_buffer_id      = INVALID_ID; // 4B: uint32_t true_fact_id column
    uint32_t belief_believed_tome_id_buffer_id  = INVALID_ID; // 4B: uint16_t believed_tome_id column
    uint32_t agent_attention_threshold_buffer_id= INVALID_ID; // 4B: float per-agent attention_threshold
    float sensory_clarity_floor                 = 0.20f;      // 4B: Default minimum clarity to wake observation/belief
    uint16_t required_tome_id                   = 0u;         // 2B: Optional Akashic Tome filter
    uint8_t allowed_modality_mask               = 0x0Fu;      // 1B: Bitmask of SensoryModality (Sight=1, Hearing=2, Touch=4, Interoception=8)
    bool filter_by_tome_id                      = false;      // 1B: Only wake entries matching required_tome_id
    bool only_misreferenced_entries             = false;      // 1B: Only match rows where catalogued_fact_id != true_fact_id
    bool negate_result                          = false;      // 1B: Invert match result
    uint8_t reserved_padding[26]                = {0};        // 26B -> Total = 64B

    template <typename T_View>
    inline void configure_view(T_View &p_view, const TaskContextPOD &p_context) const noexcept {
        if constexpr (requires { p_view.bind_secondary(nullptr); }) {
            const uint32_t sec_id = (observation_pod_buffer_id != INVALID_ID)
                ? observation_pod_buffer_id
                : get_target_buffer_id();
            if (const GrantPartPOD *sec_part = p_context.get_grant_part(sec_id)) {
                p_view.bind_secondary(sec_part);
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
        add_buf("observation_pod_buffer_id");
        add_buf("belief_confidence_buffer_id");
        add_buf("belief_catalogued_fact_id_buffer_id");
        add_buf("belief_true_fact_id_buffer_id");
        add_buf("belief_believed_tome_id_buffer_id");
        add_buf("agent_attention_threshold_buffer_id");

        godot::Dictionary flr;
        flr["name"]        = "sensory_clarity_floor";
        flr["type"]        = godot::Variant::FLOAT;
        flr["hint"]        = godot::PROPERTY_HINT_RANGE;
        flr["hint_string"] = "0.0,1.0,0.01";
        props.push_back(flr);

        godot::Dictionary mod;
        mod["name"]        = "allowed_modality_mask";
        mod["type"]        = godot::Variant::INT;
        mod["hint"]        = godot::PROPERTY_HINT_FLAGS;
        mod["hint_string"] = "Sight,Hearing,Touch,Interoception";
        props.push_back(mod);

        godot::Dictionary mis;
        mis["name"] = "only_misreferenced_entries";
        mis["type"] = godot::Variant::BOOL;
        props.push_back(mis);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)                    return target_buffer_id;
        if (belief_catalogued_fact_id_buffer_id != INVALID_ID) return belief_catalogued_fact_id_buffer_id;
        if (observation_pod_buffer_id != INVALID_ID)           return observation_pod_buffer_id;
        return belief_confidence_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))                    target_buffer_id                    = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("observation_pod_buffer_id"))           observation_pod_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["observation_pod_buffer_id"]));
        if (p_props.has("belief_confidence_buffer_id"))         belief_confidence_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_confidence_buffer_id"]));
        if (p_props.has("belief_catalogued_fact_id_buffer_id")) belief_catalogued_fact_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_catalogued_fact_id_buffer_id"]));
        if (p_props.has("belief_true_fact_id_buffer_id"))       belief_true_fact_id_buffer_id       = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_true_fact_id_buffer_id"]));
        if (p_props.has("belief_believed_tome_id_buffer_id"))   belief_believed_tome_id_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["belief_believed_tome_id_buffer_id"]));
        if (p_props.has("agent_attention_threshold_buffer_id")) agent_attention_threshold_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_attention_threshold_buffer_id"]));
        if (p_props.has("sensory_clarity_floor"))               sensory_clarity_floor               = std::clamp(static_cast<float>(static_cast<double>(p_props["sensory_clarity_floor"])), 0.0f, 1.0f);
        if (p_props.has("required_tome_id"))                    required_tome_id                    = static_cast<uint16_t>(static_cast<int64_t>(p_props["required_tome_id"]));
        if (p_props.has("allowed_modality_mask"))               allowed_modality_mask               = static_cast<uint8_t>(static_cast<int64_t>(p_props["allowed_modality_mask"]) & 0x0F);
        if (p_props.has("filter_by_tome_id"))                   filter_by_tome_id                   = static_cast<bool>(p_props["filter_by_tome_id"]);
        if (p_props.has("only_misreferenced_entries"))          only_misreferenced_entries          = static_cast<bool>(p_props["only_misreferenced_entries"]);
        if (p_props.has("negate_result"))                       negate_result                       = static_cast<bool>(p_props["negate_result"]);
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
        const uint32_t tgt_id = get_target_buffer_id();
        const GrantPartPOD *obs_part  = (observation_pod_buffer_id != INVALID_ID)           ? p_context.get_grant_part(observation_pod_buffer_id)           : nullptr;
        const GrantPartPOD *conf_part = (belief_confidence_buffer_id != INVALID_ID)         ? p_context.get_grant_part(belief_confidence_buffer_id)         : nullptr;
        const GrantPartPOD *cfid_part = (belief_catalogued_fact_id_buffer_id != INVALID_ID) ? p_context.get_grant_part(belief_catalogued_fact_id_buffer_id) : nullptr;
        const GrantPartPOD *tfid_part = (belief_true_fact_id_buffer_id != INVALID_ID)       ? p_context.get_grant_part(belief_true_fact_id_buffer_id)       : nullptr;
        const GrantPartPOD *tome_part = (belief_believed_tome_id_buffer_id != INVALID_ID)   ? p_context.get_grant_part(belief_believed_tome_id_buffer_id)   : nullptr;

        const auto *obs_rows = (obs_part && obs_part->raw_base_ptr && obs_part->element_stride == sizeof(ObservationPod))
            ? reinterpret_cast<const ObservationPod *>(obs_part->raw_base_ptr)
            : nullptr;
        const int64_t obs_cap = obs_rows ? static_cast<int64_t>(obs_part->capacity_bytes / sizeof(ObservationPod)) : 0;

        const auto *confs = (conf_part && conf_part->raw_base_ptr)
            ? reinterpret_cast<const float *>(conf_part->raw_base_ptr)
            : nullptr;
        const int64_t conf_cap = confs ? static_cast<int64_t>(conf_part->capacity_bytes / sizeof(float)) : 0;

        const auto *cat_fids = (cfid_part && cfid_part->raw_base_ptr)
            ? reinterpret_cast<const uint32_t *>(cfid_part->raw_base_ptr)
            : nullptr;
        const int64_t cfid_cap = cat_fids ? static_cast<int64_t>(cfid_part->capacity_bytes / sizeof(uint32_t)) : 0;

        const auto *true_fids = (tfid_part && tfid_part->raw_base_ptr)
            ? reinterpret_cast<const uint32_t *>(tfid_part->raw_base_ptr)
            : nullptr;
        const int64_t tfid_cap = true_fids ? static_cast<int64_t>(tfid_part->capacity_bytes / sizeof(uint32_t)) : 0;

        const auto *tomes = (tome_part && tome_part->raw_base_ptr)
            ? reinterpret_cast<const uint16_t *>(tome_part->raw_base_ptr)
            : nullptr;
        const int64_t tome_cap = tomes ? static_cast<int64_t>(tome_part->capacity_bytes / sizeof(uint16_t)) : 0;

        auto eval_row = [&](int64_t idx) noexcept -> bool {
            if (idx < 0) return false;

            if (only_misreferenced_entries && cat_fids && true_fids && idx < cfid_cap && idx < tfid_cap) {
                if (cat_fids[idx] == true_fids[idx]) {
                    return negate_result;
                }
            }
            if (filter_by_tome_id) {
                if (tomes && idx < tome_cap && tomes[idx] != required_tome_id) {
                    return negate_result;
                }
                if (obs_rows && idx < obs_cap && obs_rows[idx].tome_id != required_tome_id) {
                    return negate_result;
                }
            }

            float clarity = 0.0f;
            if (obs_rows && idx < obs_cap) {
                const uint8_t mod_bit = static_cast<uint8_t>(1u << (static_cast<uint8_t>(obs_rows[idx].modality) & 0x03u));
                if ((allowed_modality_mask & mod_bit) == 0u) {
                    return negate_result;
                }
                clarity = obs_rows[idx].clarity;
            } else if (confs && idx < conf_cap) {
                clarity = confs[idx];
            } else if (cat_fids && idx < cfid_cap) {
                clarity = (cat_fids[idx] != 0u) ? 1.0f : 0.0f;
            }

            const bool match = (clarity >= sensory_clarity_floor);
            return negate_result ? !match : match;
        };

        _apply_query_op<Op>(r_selection, p_context, tgt_id, eval_row);
    }

private:
    template <QueryOp Op, typename PredFn>
    static inline void _apply_query_op(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_context,
        uint32_t p_target_id,
        PredFn &&p_eval
    ) noexcept {
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                uint64_t *bitset = r_selection.data.bitset;
                for (int64_t i = 0; i < r_selection.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        if (!p_eval(i)) {
                            bitset[i >> 6] &= ~(1ULL << (i & 63));
                            --r_selection.element_count;
                        }
                    }
                }
            } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                int64_t write_ptr = 0;
                int64_t *indices  = r_selection.data.indices;
                for (int64_t i = 0; i < r_selection.element_count; ++i) {
                    const int64_t idx = indices[i];
                    if (p_eval(idx)) {
                        indices[write_ptr++] = idx;
                    }
                }
                r_selection.element_count = write_ptr;
            } else if (r_selection.mode == SelectionMode::RANGE && r_selection.data.indices) {
                const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                const int64_t end   = std::min<int64_t>(start + r_selection.element_count, r_selection.capacity);
                int64_t write_ptr   = 0;
                for (int64_t i = start; i < end; ++i) {
                    if (p_eval(i)) {
                        r_selection.data.indices[write_ptr++] = i;
                    }
                }
                r_selection.mode          = SelectionMode::SPARSE;
                r_selection.start_index   = 0;
                r_selection.element_count = write_ptr;
            }
        } else if constexpr (Op == QueryOp::ADD) {
            for (int64_t i = 0; i < r_selection.capacity; ++i) {
                if (p_eval(i)) {
                    p_context.queue_selection_command(p_target_id, i);
                }
            }
        }
    }
};

static_assert(sizeof(SensoryObservationWakeQueryLogic<>) == 64, "SensoryObservationWakeQueryLogic must be 64 bytes!");

} // namespace ideam::core
