#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../query_logic/query_logic_traits.h"
#include "../transform_logic/property_modifier_aggregate_transform_logic.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_PROPERTY_FLAG_QUERY_MODE_DEFINED
#define IDEAM_GAMES_PROPERTY_FLAG_QUERY_MODE_DEFINED

/**
 * High-level predicate presets for PropertyExhaustedOrLockedQueryLogic (G13).
 */
enum PropertyFlagQueryMode : uint8_t {
    PROP_QUERY_AVAILABLE_FOR_ACTION = 0, // Matches when neither PROP_FLAG_IS_EXHAUSTED nor PROP_FLAG_LOCKED is set
    PROP_QUERY_IS_EXHAUSTED         = 1, // Matches when PROP_FLAG_IS_EXHAUSTED is set (or modded_value <= modded_min)
    PROP_QUERY_IS_LOCKED            = 2, // Matches when PROP_FLAG_LOCKED is set
    PROP_QUERY_EXHAUSTED_OR_LOCKED  = 3, // Matches when either PROP_FLAG_IS_EXHAUSTED or PROP_FLAG_LOCKED is set
    PROP_QUERY_ALLOWS_MODIFIERS     = 4, // Matches when target modifier_channel allows modifiers (and is unlocked)
    PROP_QUERY_CUSTOM_MASK          = 5  // Matches (flags & required_flags_mask) == required && (flags & forbidden) == 0
};

#endif // IDEAM_GAMES_PROPERTY_FLAG_QUERY_MODE_DEFINED

/**
 * PropertyExhaustedOrLockedQueryLogic<T> (G13)
 * Filters (QueryOp::CULL / QueryOp::ADD) entities or properties based on GamePropertyBuffer::property_flags
 * (is_exhausted, locked, allow_value_modifiers, allow_max_modifiers, allow_min_modifiers) and optional
 * live modded_value / modded_min / modded_max comparisons.
 * Replaces per-Node GameProperty::get_is_exhausted() and get_locked() checks in GameAgent::find_best_action()
 * and GamePieceAction::refresh_action().
 */
template <typename T = int32_t>
struct alignas(64) PropertyExhaustedOrLockedQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::RANDOM_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr bool supports_cull          = true;
    static constexpr bool supports_addition      = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Property Exhausted Or Locked";
    static constexpr std::string_view ui_category  = "Properties";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id          = INVALID_ID;                           // 4B: Primary selection buffer to filter/wake
    uint32_t property_flags_buffer_id  = INVALID_ID;                           // 4B: GamePropertyBuffer::property_flags (uint16_t/uint32_t/uint8_t)
    uint32_t modded_value_buffer_id    = INVALID_ID;                           // 4B: Optional GamePropertyBuffer::modded_value (int32_t)
    uint32_t modded_min_buffer_id      = INVALID_ID;                           // 4B: Optional GamePropertyBuffer::modded_min (int32_t)
    uint32_t modded_max_buffer_id      = INVALID_ID;                           // 4B: Optional GamePropertyBuffer::modded_max (int32_t)
    uint32_t owner_entity_id_buffer_id = INVALID_ID;                           // 4B: Optional GamePropertyBuffer::owner_entity_id (uint32_t)

    uint16_t required_flags_mask       = 0u;                                   // 2B: Required bitmask for PROP_QUERY_CUSTOM_MASK
    uint16_t forbidden_flags_mask      = PROP_FLAG_IS_EXHAUSTED | PROP_FLAG_LOCKED; // 2B: Forbidden bitmask for PROP_QUERY_CUSTOM_MASK

    uint8_t query_mode                 = PROP_QUERY_AVAILABLE_FOR_ACTION;      // 1B: PropertyFlagQueryMode
    uint8_t modifier_channel           = 0u;                                   // 1B: 0=Value, 1=Max, 2=Min (for PROP_QUERY_ALLOWS_MODIFIERS)
    bool check_live_bounds             = true;                                 // 1B: Evaluate modded_value <= modded_min when value buffers are bound
    bool invert_match                  = false;                                // 1B: Invert final boolean evaluation
    uint8_t reserved_padding[32]       = {0};                                  // 32B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary pflg_prop;
        pflg_prop["name"] = "property_flags_buffer_id";
        pflg_prop["type"] = godot::Variant::INT;
        pflg_prop["hint_string"] = "buffer_option";
        props.push_back(pflg_prop);

        godot::Dictionary mval_prop;
        mval_prop["name"] = "modded_value_buffer_id";
        mval_prop["type"] = godot::Variant::INT;
        mval_prop["hint_string"] = "buffer_option";
        props.push_back(mval_prop);

        godot::Dictionary mmin_prop;
        mmin_prop["name"] = "modded_min_buffer_id";
        mmin_prop["type"] = godot::Variant::INT;
        mmin_prop["hint_string"] = "buffer_option";
        props.push_back(mmin_prop);

        godot::Dictionary mmax_prop;
        mmax_prop["name"] = "modded_max_buffer_id";
        mmax_prop["type"] = godot::Variant::INT;
        mmax_prop["hint_string"] = "buffer_option";
        props.push_back(mmax_prop);

        godot::Dictionary own_prop;
        own_prop["name"] = "owner_entity_id_buffer_id";
        own_prop["type"] = godot::Variant::INT;
        own_prop["hint_string"] = "buffer_option";
        props.push_back(own_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "query_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Available (Not Exhausted/Locked):0,Is Exhausted:1,Is Locked:2,Exhausted Or Locked:3,Allows Modifiers:4,Custom Bitmask:5";
        props.push_back(mode_prop);

        godot::Dictionary mch_prop;
        mch_prop["name"] = "modifier_channel";
        mch_prop["type"] = godot::Variant::INT;
        mch_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mch_prop["hint_string"] = "Value:0,Max Value:1,Min Value:2";
        props.push_back(mch_prop);

        godot::Dictionary req_prop;
        req_prop["name"] = "required_flags_mask";
        req_prop["type"] = godot::Variant::INT;
        req_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(req_prop);

        godot::Dictionary frb_prop;
        frb_prop["name"] = "forbidden_flags_mask";
        frb_prop["type"] = godot::Variant::INT;
        frb_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(frb_prop);

        godot::Dictionary bnd_prop;
        bnd_prop["name"] = "check_live_bounds";
        bnd_prop["type"] = godot::Variant::BOOL;
        bnd_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(bnd_prop);

        godot::Dictionary inv_prop;
        inv_prop["name"] = "invert_match";
        inv_prop["type"] = godot::Variant::BOOL;
        inv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(inv_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)         return target_buffer_id;
        if (property_flags_buffer_id != INVALID_ID) return property_flags_buffer_id;
        return modded_value_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("property_flags_buffer_id")) {
            property_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_flags_buffer_id"]));
        }
        if (p_props.has("modded_value_buffer_id")) {
            modded_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_value_buffer_id"]));
        }
        if (p_props.has("modded_min_buffer_id")) {
            modded_min_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_min_buffer_id"]));
        }
        if (p_props.has("modded_max_buffer_id")) {
            modded_max_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_max_buffer_id"]));
        }
        if (p_props.has("owner_entity_id_buffer_id")) {
            owner_entity_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["owner_entity_id_buffer_id"]));
        }
        if (p_props.has("required_flags_mask")) {
            required_flags_mask = static_cast<uint16_t>(static_cast<int64_t>(p_props["required_flags_mask"]));
        }
        if (p_props.has("forbidden_flags_mask")) {
            forbidden_flags_mask = static_cast<uint16_t>(static_cast<int64_t>(p_props["forbidden_flags_mask"]));
        }
        if (p_props.has("query_mode")) {
            query_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["query_mode"]), 0, 5));
        }
        if (p_props.has("modifier_channel")) {
            modifier_channel = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["modifier_channel"]), 0, 2));
        }
        if (p_props.has("check_live_bounds")) {
            check_live_bounds = static_cast<bool>(p_props["check_live_bounds"]);
        }
        if (p_props.has("invert_match")) {
            invert_match = static_cast<bool>(p_props["invert_match"]);
        }
    }

    template <QueryOp Op, typename T_View, typename T_Strategy>
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
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint8_t>) {
            const uint32_t tgt_id  = get_target_buffer_id();
            const uint32_t pflg_id = (property_flags_buffer_id != INVALID_ID) ? property_flags_buffer_id : tgt_id;

            const GrantPartPOD* pflg_part = (pflg_id != INVALID_ID)                   ? p_context.get_grant_part(pflg_id)                   : nullptr;
            const GrantPartPOD* mval_part = (modded_value_buffer_id != INVALID_ID)    ? p_context.get_grant_part(modded_value_buffer_id)    : nullptr;
            const GrantPartPOD* mmin_part = (modded_min_buffer_id != INVALID_ID)      ? p_context.get_grant_part(modded_min_buffer_id)      : nullptr;
            const GrantPartPOD* mmax_part = (modded_max_buffer_id != INVALID_ID)      ? p_context.get_grant_part(modded_max_buffer_id)      : nullptr;
            const GrantPartPOD* own_part  = (owner_entity_id_buffer_id != INVALID_ID) ? p_context.get_grant_part(owner_entity_id_buffer_id) : nullptr;

            const uint8_t* __restrict pflg_raw      = (pflg_part && pflg_part->raw_base_ptr) ? pflg_part->raw_base_ptr : nullptr;
            const uint32_t pflg_stride              = pflg_part ? pflg_part->element_stride : 0;
            const int64_t pflg_cap                  = (pflg_part && pflg_stride > 0)
                ? static_cast<int64_t>(pflg_part->capacity_bytes / pflg_stride)
                : 0;
            const int32_t* __restrict modded_vals   = (mval_part && mval_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(mval_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict modded_mins   = (mmin_part && mmin_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(mmin_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict modded_maxs   = (mmax_part && mmax_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(mmax_part->raw_base_ptr) : nullptr;
            const uint32_t* __restrict owner_ids    = (own_part && own_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(own_part->raw_base_ptr)  : nullptr;

            const int64_t mval_cap = modded_vals ? static_cast<int64_t>(mval_part->capacity_bytes / sizeof(int32_t))  : 0;
            const int64_t mmin_cap = modded_mins ? static_cast<int64_t>(mmin_part->capacity_bytes / sizeof(int32_t))  : 0;
            const int64_t mmax_cap = modded_maxs ? static_cast<int64_t>(mmax_part->capacity_bytes / sizeof(int32_t))  : 0;
            const int64_t own_cap  = owner_ids   ? static_cast<int64_t>(own_part->capacity_bytes / sizeof(uint32_t))  : 0;

            auto eval_single_property = [&](int64_t pidx) noexcept -> bool {
                uint16_t flags = PROP_FLAG_DEFAULT_MASK;
                if (pflg_raw && pidx >= 0 && pidx < pflg_cap) {
                    if (pflg_stride == sizeof(uint32_t)) {
                        flags = static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pflg_raw)[pidx]);
                    } else if (pflg_stride == sizeof(uint16_t)) {
                        flags = reinterpret_cast<const uint16_t*>(pflg_raw)[pidx];
                    } else {
                        flags = static_cast<uint16_t>(pflg_raw[pidx]);
                    }
                }

                if (check_live_bounds && modded_vals && pidx >= 0 && pidx < mval_cap) {
                    const int32_t cur_v = modded_vals[pidx];
                    const int32_t min_v = (modded_mins && pidx < mmin_cap) ? modded_mins[pidx] : 0;
                    if (cur_v <= min_v) {
                        flags |= PROP_FLAG_IS_EXHAUSTED;
                    }
                    if (modded_maxs && pidx < mmax_cap && cur_v >= modded_maxs[pidx]) {
                        flags |= PROP_FLAG_AT_MAX;
                    }
                }

                bool matched = false;
                switch (query_mode) {
                    case PROP_QUERY_AVAILABLE_FOR_ACTION:
                        matched = (flags & (PROP_FLAG_IS_EXHAUSTED | PROP_FLAG_LOCKED)) == 0u;
                        break;
                    case PROP_QUERY_IS_EXHAUSTED:
                        matched = (flags & PROP_FLAG_IS_EXHAUSTED) != 0u;
                        break;
                    case PROP_QUERY_IS_LOCKED:
                        matched = (flags & PROP_FLAG_LOCKED) != 0u;
                        break;
                    case PROP_QUERY_EXHAUSTED_OR_LOCKED:
                        matched = (flags & (PROP_FLAG_IS_EXHAUSTED | PROP_FLAG_LOCKED)) != 0u;
                        break;
                    case PROP_QUERY_ALLOWS_MODIFIERS: {
                        if ((flags & PROP_FLAG_LOCKED) != 0u) {
                            matched = false;
                        } else if (modifier_channel == 1u) {
                            matched = (flags & PROP_FLAG_ALLOW_MAX_MODS) != 0u;
                        } else if (modifier_channel == 2u) {
                            matched = (flags & PROP_FLAG_ALLOW_MIN_MODS) != 0u;
                        } else {
                            matched = (flags & PROP_FLAG_ALLOW_VALUE_MODS) != 0u;
                        }
                        break;
                    }
                    case PROP_QUERY_CUSTOM_MASK:
                    default:
                        matched = ((flags & required_flags_mask) == required_flags_mask) &&
                                  ((flags & forbidden_flags_mask) == 0u);
                        break;
                }
                return matched;
            };

            auto evaluate_index = [&](int64_t idx) noexcept -> bool {
                bool result = false;
                if (owner_ids && own_cap > 0) {
                    // Entity-scoped evaluation across all properties owned by entity `idx`
                    bool any_owned = false;
                    bool all_avail = true;
                    bool any_match = false;
                    for (int64_t p = 0; p < own_cap; ++p) {
                        if (static_cast<int64_t>(owner_ids[p]) != idx) continue;
                        any_owned = true;
                        const bool p_match = eval_single_property(p);
                        all_avail = all_avail && p_match;
                        any_match = any_match || p_match;
                    }
                    if (!any_owned) {
                        result = (query_mode == PROP_QUERY_AVAILABLE_FOR_ACTION || query_mode == PROP_QUERY_ALLOWS_MODIFIERS);
                    } else {
                        result = (query_mode == PROP_QUERY_AVAILABLE_FOR_ACTION || query_mode == PROP_QUERY_ALLOWS_MODIFIERS)
                            ? all_avail
                            : any_match;
                    }
                } else {
                    result = eval_single_property(idx);
                }
                return invert_match ? !result : result;
            };

            _apply_query_op<Op>(r_selection, p_context, tgt_id, evaluate_index);
        }
    }

private:
    template <QueryOp Op, typename PredFn>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _apply_query_op(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_context,
        uint32_t p_target_id,
        PredFn&& p_eval
    ) noexcept {
        if constexpr (Op == QueryOp::CULL) {
            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                uint64_t* bitset = r_selection.data.bitset;
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
                int64_t* indices  = r_selection.data.indices;
                for (int64_t i = 0; i < r_selection.element_count; ++i) {
                    const int64_t idx = indices[i];
                    if (p_eval(idx)) {
                        indices[write_ptr++] = idx;
                    }
                }
                r_selection.element_count = write_ptr;
            } else if (r_selection.mode == SelectionMode::RANGE) {
                const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                const int64_t end   = std::min<int64_t>(start + r_selection.element_count, r_selection.capacity);
                if (r_selection.data.indices) {
                    int64_t write_ptr = 0;
                    for (int64_t i = start; i < end; ++i) {
                        if (p_eval(i)) {
                            r_selection.data.indices[write_ptr++] = i;
                        }
                    }
                    r_selection.mode          = SelectionMode::SPARSE;
                    r_selection.start_index   = 0;
                    r_selection.element_count = write_ptr;
                }
            }
        } else if constexpr (Op == QueryOp::ADD) {
            if (r_selection.unclaimed_mask) {
                const uint64_t* unclaimed = r_selection.unclaimed_mask;
                const int64_t words = (r_selection.capacity + 63) >> 6;
                for (int64_t w = 0; w < words; ++w) {
                    uint64_t mask = unclaimed[w];
                    while (mask != 0ULL) {
                        const int bit_index       = std::countr_zero(mask);
                        const int64_t global_idx  = (w << 6) + bit_index;
                        if (global_idx >= r_selection.capacity) break;
                        if (p_eval(global_idx)) {
                            p_context.queue_selection_command(p_target_id, global_idx);
                        }
                        mask &= (mask - 1ULL);
                    }
                }
            } else {
                for (int64_t i = 0; i < r_selection.capacity; ++i) {
                    if (p_eval(i)) {
                        p_context.queue_selection_command(p_target_id, i);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(PropertyExhaustedOrLockedQueryLogic<>) == 64, "PropertyExhaustedOrLockedQueryLogic must be exactly 64 bytes!");

} // namespace ideam::core