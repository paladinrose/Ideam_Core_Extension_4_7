#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include "../transform_logic/gameplay_rule_apply_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_RULE_SPECIFICITY_METADATA_ENUMS_DEFINED
#define IDEAM_GAMES_RULE_SPECIFICITY_METADATA_ENUMS_DEFINED

/**
 * Conflict resolution / stamping modes for GameplayRuleSpecificityMetadataLogic (G17).
 */
enum RuleSpecificityStampOp : uint8_t {
    STAMP_OP_MAX_PRECEDENCE        = 0, // Overwrites partition_ids[idx] only when new_key >= existing_key (or > if !ties_won_by_later)
    STAMP_OP_FORCE_SET             = 1, // Unconditionally writes the packed 32-bit precedence key into partition_ids[idx]
    STAMP_OP_CULL_LOWER_PRECEDENCE = 2  // Stamps when winning; clears selection bit/index when shadowed by an existing higher key
};

/**
 * Selector feature bitflags for computing CSS-like specificity in DOD buffers,
 * mirroring GameplayRule::get_specificity() in src/godot/games/gameplay/gameplay_rule.cpp:
 * - target_class non-empty:          +1
 * - target_title_or_group non-empty: +2
 * - target_property non-empty:       +1
 * - pseudo_state non-empty:          +2
 * - gameplay_condition valid:        +2
 * - required_properties/methods/signals: +1 each (added via part_signature_count)
 */
enum RuleSelectorComponentFlags : uint8_t {
    RULE_SEL_NONE               = 0,
    RULE_SEL_HAS_TARGET_CLASS   = 1 << 0, // +1 specificity
    RULE_SEL_HAS_TITLE_OR_GROUP = 1 << 1, // +2 specificity
    RULE_SEL_HAS_TARGET_PROP    = 1 << 2, // +1 specificity
    RULE_SEL_HAS_PSEUDO_STATE   = 1 << 3, // +2 specificity
    RULE_SEL_HAS_CONDITION      = 1 << 4  // +2 specificity
};

#endif // IDEAM_GAMES_RULE_SPECIFICITY_METADATA_ENUMS_DEFINED

/**
 * GameplayRuleSpecificityMetadataLogic<T> (G17)
 * Stamps r_selection.partition_ids and r_selection.lod_levels with packed
 * ((is_important << 24) | (cascade_layer << 16) | (priority << 8) | specificity)
 * precedence keys for O(1) conflict resolution across GameplayRule waves.
 * Mirrors GameplayRule::get_specificity() and GameplayRule::outweighs().
 */
template <typename T = int32_t>
struct alignas(64) GameplayRuleSpecificityMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Gameplay Rule Specificity";
    static constexpr std::string_view ui_category  = "Rules";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID;              // 4B: Primary selection buffer whose metadata is stamped
    uint32_t cascade_layer_buffer_id    = INVALID_ID;              // 4B: Optional per-row cascade_layer (int32_t/uint8_t) column
    uint32_t priority_buffer_id         = INVALID_ID;              // 4B: Optional per-row priority (int32_t) column
    uint32_t specificity_buffer_id      = INVALID_ID;              // 4B: Optional per-row precomputed specificity (int32_t) column
    uint32_t important_flags_buffer_id  = INVALID_ID;              // 4B: Optional per-row is_important flag/bitmask column
    uint32_t output_key_buffer_id       = INVALID_ID;              // 4B: Optional int32_t/uint32_t column to persist winning packed key
    int32_t priority                    = 0;                       // 4B: Rule priority (clamped/masked into bits [15:8])

    uint8_t cascade_layer               = RULE_LAYER_STYLE;        // 1B: GameplayRuleCascadeLayer -> bits [23:16]
    uint8_t specificity                 = 0u;                      // 1B: Explicit specificity override (0 = compute from selector flags)
    uint8_t selector_component_mask     = RULE_SEL_NONE;           // 1B: RuleSelectorComponentFlags bitmask
    uint8_t part_signature_count        = 0u;                      // 1B: Count of required_properties + methods + signals (+1 each)
    uint8_t stamp_op                    = STAMP_OP_MAX_PRECEDENCE; // 1B: RuleSpecificityStampOp
    bool is_important                   = false;                   // 1B: !important flag -> bit 24
    bool ties_won_by_later              = true;                    // 1B: True matches GameplayRule::outweighs() (>= comparison)
    bool write_lod_levels               = true;                    // 1B: Also stamp 8-bit cascade summary into r_selection.lod_levels
    bool bias_signed_priority           = false;                   // 1B: Add +128 bias before clamping priority to [0, 255]
    uint8_t reserved_padding[27]        = {0};                     // 27B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary lyr_buf_prop;
        lyr_buf_prop["name"] = "cascade_layer_buffer_id";
        lyr_buf_prop["type"] = godot::Variant::INT;
        lyr_buf_prop["hint_string"] = "buffer_option";
        props.push_back(lyr_buf_prop);

        godot::Dictionary pri_buf_prop;
        pri_buf_prop["name"] = "priority_buffer_id";
        pri_buf_prop["type"] = godot::Variant::INT;
        pri_buf_prop["hint_string"] = "buffer_option";
        props.push_back(pri_buf_prop);

        godot::Dictionary spc_buf_prop;
        spc_buf_prop["name"] = "specificity_buffer_id";
        spc_buf_prop["type"] = godot::Variant::INT;
        spc_buf_prop["hint_string"] = "buffer_option";
        props.push_back(spc_buf_prop);

        godot::Dictionary imp_buf_prop;
        imp_buf_prop["name"] = "important_flags_buffer_id";
        imp_buf_prop["type"] = godot::Variant::INT;
        imp_buf_prop["hint_string"] = "buffer_option";
        props.push_back(imp_buf_prop);

        godot::Dictionary out_buf_prop;
        out_buf_prop["name"] = "output_key_buffer_id";
        out_buf_prop["type"] = godot::Variant::INT;
        out_buf_prop["hint_string"] = "buffer_option";
        props.push_back(out_buf_prop);

        godot::Dictionary op_prop;
        op_prop["name"] = "stamp_op";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Max Precedence (Keep Highest):0,Force Set:1,Cull Lower Precedence:2";
        props.push_back(op_prop);

        godot::Dictionary lyr_prop;
        lyr_prop["name"] = "cascade_layer";
        lyr_prop["type"] = godot::Variant::INT;
        lyr_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        lyr_prop["hint_string"] = "Base (Style):0,Mode:1,Difficulty:2,Environment (Board):3,Player & Accessibility:4,Inline (Entity):5";
        props.push_back(lyr_prop);

        godot::Dictionary imp_prop;
        imp_prop["name"] = "is_important";
        imp_prop["type"] = godot::Variant::BOOL;
        imp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(imp_prop);

        godot::Dictionary pri_prop;
        pri_prop["name"] = "priority";
        pri_prop["type"] = godot::Variant::INT;
        pri_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        pri_prop["hint_string"] = "-128,255,1";
        props.push_back(pri_prop);

        godot::Dictionary spc_prop;
        spc_prop["name"] = "specificity";
        spc_prop["type"] = godot::Variant::INT;
        spc_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        spc_prop["hint_string"] = "0,255,1";
        props.push_back(spc_prop);

        godot::Dictionary sel_prop;
        sel_prop["name"] = "selector_component_mask";
        sel_prop["type"] = godot::Variant::INT;
        sel_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        sel_prop["hint_string"] = "Target Class (+1),Title Or Group (+2),Target Property (+1),Pseudo State (+2),Gameplay Condition (+2)";
        props.push_back(sel_prop);

        godot::Dictionary sig_prop;
        sig_prop["name"] = "part_signature_count";
        sig_prop["type"] = godot::Variant::INT;
        sig_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        sig_prop["hint_string"] = "0,64,1";
        props.push_back(sig_prop);

        godot::Dictionary tie_prop;
        tie_prop["name"] = "ties_won_by_later";
        tie_prop["type"] = godot::Variant::BOOL;
        tie_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(tie_prop);

        godot::Dictionary lod_prop;
        lod_prop["name"] = "write_lod_levels";
        lod_prop["type"] = godot::Variant::BOOL;
        lod_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(lod_prop);

        godot::Dictionary bias_prop;
        bias_prop["name"] = "bias_signed_priority";
        bias_prop["type"] = godot::Variant::BOOL;
        bias_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(bias_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)        return target_buffer_id;
        if (output_key_buffer_id != INVALID_ID)    return output_key_buffer_id;
        if (specificity_buffer_id != INVALID_ID)   return specificity_buffer_id;
        if (priority_buffer_id != INVALID_ID)      return priority_buffer_id;
        return cascade_layer_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("cascade_layer_buffer_id")) {
            cascade_layer_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["cascade_layer_buffer_id"]));
        }
        if (p_props.has("priority_buffer_id")) {
            priority_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["priority_buffer_id"]));
        }
        if (p_props.has("specificity_buffer_id")) {
            specificity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["specificity_buffer_id"]));
        }
        if (p_props.has("important_flags_buffer_id")) {
            important_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["important_flags_buffer_id"]));
        }
        if (p_props.has("output_key_buffer_id")) {
            output_key_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_key_buffer_id"]));
        }
        if (p_props.has("priority")) {
            priority = static_cast<int32_t>(static_cast<int64_t>(p_props["priority"]));
        }
        if (p_props.has("cascade_layer")) {
            cascade_layer = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["cascade_layer"]), 0, 255));
        }
        if (p_props.has("specificity")) {
            specificity = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["specificity"]), 0, 255));
        }
        if (p_props.has("selector_component_mask")) {
            selector_component_mask = static_cast<uint8_t>(static_cast<int64_t>(p_props["selector_component_mask"]));
        }
        if (p_props.has("part_signature_count")) {
            part_signature_count = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["part_signature_count"]), 0, 255));
        }
        if (p_props.has("stamp_op")) {
            stamp_op = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["stamp_op"]), 0, 2));
        }
        if (p_props.has("is_important")) {
            is_important = static_cast<bool>(p_props["is_important"]);
        }
        if (p_props.has("ties_won_by_later")) {
            ties_won_by_later = static_cast<bool>(p_props["ties_won_by_later"]);
        }
        if (p_props.has("write_lod_levels")) {
            write_lod_levels = static_cast<bool>(p_props["write_lod_levels"]);
        }
        if (p_props.has("bias_signed_priority")) {
            bias_signed_priority = static_cast<bool>(p_props["bias_signed_priority"]);
        }
    }

    /**
     * Computes the CSS-inspired selector specificity score matching GameplayRule::get_specificity().
     */
    [[nodiscard]] constexpr uint8_t compute_effective_specificity() const noexcept {
        if (specificity > 0u) {
            return specificity;
        }
        uint32_t score = static_cast<uint32_t>(part_signature_count);
        if ((selector_component_mask & RULE_SEL_HAS_TARGET_CLASS) != 0u)   score += 1u;
        if ((selector_component_mask & RULE_SEL_HAS_TITLE_OR_GROUP) != 0u) score += 2u;
        if ((selector_component_mask & RULE_SEL_HAS_TARGET_PROP) != 0u)    score += 1u;
        if ((selector_component_mask & RULE_SEL_HAS_PSEUDO_STATE) != 0u)   score += 2u;
        if ((selector_component_mask & RULE_SEL_HAS_CONDITION) != 0u)      score += 2u;
        return static_cast<uint8_t>(std::min<uint32_t>(score, 255u));
    }

    /**
     * Packs (is_important << 24) | (cascade_layer << 16) | (priority << 8) | specificity into a 32-bit key.
     */
    [[nodiscard]] constexpr uint32_t pack_precedence_key(
        bool p_important,
        uint8_t p_layer,
        int32_t p_priority,
        uint8_t p_specificity
    ) const noexcept {
        const int32_t adj_prio = bias_signed_priority ? (p_priority + 128) : p_priority;
        const uint32_t prio_byte = static_cast<uint32_t>(std::clamp<int32_t>(adj_prio, 0, 255)) & 0xFFu;
        return ((p_important ? 1u : 0u) << 24) |
               (static_cast<uint32_t>(p_layer) << 16) |
               (prio_byte << 8) |
               static_cast<uint32_t>(p_specificity);
    }

    /**
     * Packs an 8-bit summary tier for r_selection.lod_levels:
     * bit 7 = is_important, bits [6:4] = cascade_layer (0..7), bits [3:0] = min(specificity, 15).
     */
    [[nodiscard]] static constexpr uint8_t pack_lod_tier(
        bool p_important,
        uint8_t p_layer,
        uint8_t p_specificity
    ) noexcept {
        return static_cast<uint8_t>(
            ((p_important ? 1u : 0u) << 7) |
            ((static_cast<uint32_t>(p_layer) & 0x07u) << 4) |
            (std::min<uint32_t>(p_specificity, 0x0Fu))
        );
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_context,
        const T_View& p_view
    ) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, int32_t>) {
            if (r_selection.element_count == 0) return;

            const GrantPartPOD* lyr_part = (cascade_layer_buffer_id != INVALID_ID)   ? p_context.get_grant_part(cascade_layer_buffer_id)   : nullptr;
            const GrantPartPOD* pri_part = (priority_buffer_id != INVALID_ID)        ? p_context.get_grant_part(priority_buffer_id)        : nullptr;
            const GrantPartPOD* spc_part = (specificity_buffer_id != INVALID_ID)     ? p_context.get_grant_part(specificity_buffer_id)     : nullptr;
            const GrantPartPOD* imp_part = (important_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(important_flags_buffer_id) : nullptr;
            const GrantPartPOD* out_part = (output_key_buffer_id != INVALID_ID)      ? p_context.get_grant_part(output_key_buffer_id)      : nullptr;

            const int32_t* __restrict lyr_col = (lyr_part && lyr_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(lyr_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict pri_col = (pri_part && pri_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(pri_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict spc_col = (spc_part && spc_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(spc_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict imp_col = (imp_part && imp_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(imp_part->raw_base_ptr) : nullptr;
            int32_t* __restrict out_keys      = (out_part && out_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(out_part->raw_base_ptr)       : nullptr;

            const int64_t lyr_cap = lyr_col  ? static_cast<int64_t>(lyr_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t pri_cap = pri_col  ? static_cast<int64_t>(pri_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t spc_cap = spc_col  ? static_cast<int64_t>(spc_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t imp_cap = imp_col  ? static_cast<int64_t>(imp_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t out_cap = out_keys ? static_cast<int64_t>(out_part->capacity_bytes / sizeof(int32_t)) : 0;

            const uint8_t base_spec = compute_effective_specificity();

            auto eval_and_stamp = [&](int64_t idx) noexcept -> bool {
                const bool row_imp = (imp_col && idx >= 0 && idx < imp_cap) ? (imp_col[idx] != 0) : is_important;
                const uint8_t row_lyr = (lyr_col && idx >= 0 && idx < lyr_cap)
                    ? static_cast<uint8_t>(std::clamp<int32_t>(lyr_col[idx], 0, 255))
                    : cascade_layer;
                const int32_t row_pri = (pri_col && idx >= 0 && idx < pri_cap) ? pri_col[idx] : priority;
                const uint8_t row_spc = (spc_col && idx >= 0 && idx < spc_cap)
                    ? static_cast<uint8_t>(std::clamp<int32_t>(spc_col[idx], 0, 255))
                    : base_spec;

                const uint32_t new_key  = pack_precedence_key(row_imp, row_lyr, row_pri, row_spc);
                const int64_t new_key64 = static_cast<int64_t>(new_key);

                bool wins = true;
                if (stamp_op != STAMP_OP_FORCE_SET && r_selection.partition_ids && idx >= 0 && idx < r_selection.capacity) {
                    const int64_t cur_key64 = r_selection.partition_ids[idx];
                    if (cur_key64 >= 0) {
                        wins = ties_won_by_later ? (new_key64 >= cur_key64) : (new_key64 > cur_key64);
                    }
                }

                if (wins) {
                    if (r_selection.partition_ids && idx >= 0 && idx < r_selection.capacity) {
                        r_selection.partition_ids[idx] = new_key64;
                    }
                    if (write_lod_levels && r_selection.lod_levels && idx >= 0 && idx < r_selection.capacity) {
                        r_selection.lod_levels[idx] = pack_lod_tier(row_imp, row_lyr, row_spc);
                    }
                    if (out_keys && idx >= 0 && idx < out_cap) {
                        out_keys[idx] = static_cast<int32_t>(new_key);
                    }
                }
                return wins;
            };

            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                uint64_t* bitset  = r_selection.data.bitset;
                const int64_t cap = r_selection.capacity;
                for (int64_t i = 0; i < cap; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        const bool won = eval_and_stamp(i);
                        if (!won && stamp_op == STAMP_OP_CULL_LOWER_PRECEDENCE) {
                            bitset[i >> 6] &= ~(1ULL << (i & 63));
                            --r_selection.element_count;
                        }
                    }
                }
            } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                int64_t* indices        = r_selection.data.indices;
                const int64_t count     = r_selection.element_count;
                int64_t write_ptr       = 0;
                const bool cull_losers  = (stamp_op == STAMP_OP_CULL_LOWER_PRECEDENCE);
                for (int64_t i = 0; i < count; ++i) {
                    const int64_t idx = indices[i];
                    const bool won    = eval_and_stamp(idx);
                    if (!cull_losers || won) {
                        indices[write_ptr++] = idx;
                    }
                }
                if (cull_losers) {
                    r_selection.element_count = write_ptr;
                }
            } else if (r_selection.mode == SelectionMode::RANGE) {
                const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                const int64_t end   = std::min<int64_t>(start + r_selection.element_count, r_selection.capacity);
                if (stamp_op == STAMP_OP_CULL_LOWER_PRECEDENCE && r_selection.data.indices) {
                    int64_t write_ptr = 0;
                    for (int64_t i = start; i < end; ++i) {
                        if (eval_and_stamp(i)) {
                            r_selection.data.indices[write_ptr++] = i;
                        }
                    }
                    r_selection.mode          = SelectionMode::SPARSE;
                    r_selection.start_index   = 0;
                    r_selection.element_count = write_ptr;
                } else {
                    for (int64_t i = start; i < end; ++i) {
                        eval_and_stamp(i);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(GameplayRuleSpecificityMetadataLogic<>) == 64, "GameplayRuleSpecificityMetadataLogic must be exactly 64 bytes!");

} // namespace ideam::core