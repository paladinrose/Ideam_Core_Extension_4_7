#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/aosoa_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "property_modifier_aggregate_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_PROPERTY_EXHAUSTION_EVENT_POD_DEFINED
#define IDEAM_GAMES_PROPERTY_EXHAUSTION_EVENT_POD_DEFINED

/**
 * PropertyExhaustionEventPOD (16-Byte Aligned)
 * Compact command/telemetry record emitted when a GamePropertyBuffer row reaches
 * or transitions into exhaustion (modded_value <= modded_min).
 */
struct alignas(16) PropertyExhaustionEventPOD {
    uint32_t property_idx    = INVALID_ID; // 4B: Row index in GamePropertyBuffer
    uint32_t owner_entity_id = INVALID_ID; // 4B: Owning entity ID in EntityHierarchyBuffer
    int32_t clamped_value    = 0;          // 4B: Clamped modded_value
    int32_t min_value        = 0;          // 4B: Active modded_min threshold
};

static_assert(sizeof(PropertyExhaustionEventPOD) == 16, "PropertyExhaustionEventPOD must be 16 bytes!");

#endif // IDEAM_GAMES_PROPERTY_EXHAUSTION_EVENT_POD_DEFINED

/**
 * PropertyRangeClampAndExhaustTransformLogic<T> (G4)
 * Enforces GameProperty::restrict_value_to_current_min_value and
 * restrict_value_to_current_max_value across SIMD lanes (AOSOAView) or arbitrary
 * selections, updates the is_exhausted bit (PROP_FLAG_IS_EXHAUSTED) in property_flags
 * when modded_value <= modded_min, and queues exhaustion events into wave/graph commands.
 */
template <typename T = int32_t>
struct alignas(64) PropertyRangeClampAndExhaustTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = AOSOAView<T, 8, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Property Range Clamp & Exhaust";
    static constexpr std::string_view ui_category  = "Properties";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t modded_value_buffer_id     = INVALID_ID;             // 4B: Primary target buffer (GamePropertyBuffer::modded_value)
    uint32_t modded_min_buffer_id       = INVALID_ID;             // 4B: GamePropertyBuffer::modded_min (int32_t)
    uint32_t modded_max_buffer_id       = INVALID_ID;             // 4B: GamePropertyBuffer::modded_max (int32_t)
    uint32_t base_value_buffer_id       = INVALID_ID;             // 4B: Optional GamePropertyBuffer::base_value (int32_t)
    uint32_t property_flags_buffer_id   = INVALID_ID;             // 4B: GamePropertyBuffer::property_flags (uint16_t/uint32_t)
    uint32_t owner_entity_id_buffer_id  = INVALID_ID;             // 4B: Optional GamePropertyBuffer::owner_entity_id (uint32_t)
    uint32_t exhaustion_event_buffer_id = INVALID_ID;             // 4B: Optional target buffer ID for selection/event queuing

    int32_t fallback_min                = 0;                      // 4B: Fallback minimum when modded_min_buffer_id is unbound
    int32_t fallback_max                = 10;                     // 4B: Fallback maximum when modded_max_buffer_id is unbound
    uint16_t default_property_flags     = PROP_FLAG_DEFAULT_MASK; // 2B: Default flags when property_flags_buffer_id is unbound

    bool clamp_base_value               = false;                  // 1B: Also clamp base_value to [modded_min, modded_max]
    bool queue_wave_selection_command   = true;                   // 1B: Queue exhausted property indices via wave_commands
    bool queue_tier1_exhaust_event      = false;                  // 1B: Push PropertyExhaustionEventPOD to graph_commands
    bool queue_on_transition_only       = true;                   // 1B: Queue events only when newly transitioning to is_exhausted
    uint8_t reserved_padding[22]        = {0};                    // 22B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

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

        godot::Dictionary bval_prop;
        bval_prop["name"] = "base_value_buffer_id";
        bval_prop["type"] = godot::Variant::INT;
        bval_prop["hint_string"] = "buffer_option";
        props.push_back(bval_prop);

        godot::Dictionary pflg_prop;
        pflg_prop["name"] = "property_flags_buffer_id";
        pflg_prop["type"] = godot::Variant::INT;
        pflg_prop["hint_string"] = "buffer_option";
        props.push_back(pflg_prop);

        godot::Dictionary own_prop;
        own_prop["name"] = "owner_entity_id_buffer_id";
        own_prop["type"] = godot::Variant::INT;
        own_prop["hint_string"] = "buffer_option";
        props.push_back(own_prop);

        godot::Dictionary evt_prop;
        evt_prop["name"] = "exhaustion_event_buffer_id";
        evt_prop["type"] = godot::Variant::INT;
        evt_prop["hint_string"] = "buffer_option";
        props.push_back(evt_prop);

        godot::Dictionary fmin_prop;
        fmin_prop["name"] = "fallback_min";
        fmin_prop["type"] = godot::Variant::INT;
        fmin_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(fmin_prop);

        godot::Dictionary fmax_prop;
        fmax_prop["name"] = "fallback_max";
        fmax_prop["type"] = godot::Variant::INT;
        fmax_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(fmax_prop);

        godot::Dictionary cb_prop;
        cb_prop["name"] = "clamp_base_value";
        cb_prop["type"] = godot::Variant::BOOL;
        cb_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cb_prop);

        godot::Dictionary qw_prop;
        qw_prop["name"] = "queue_wave_selection_command";
        qw_prop["type"] = godot::Variant::BOOL;
        qw_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(qw_prop);

        godot::Dictionary qt_prop;
        qt_prop["name"] = "queue_tier1_exhaust_event";
        qt_prop["type"] = godot::Variant::BOOL;
        qt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(qt_prop);

        godot::Dictionary qtr_prop;
        qtr_prop["name"] = "queue_on_transition_only";
        qtr_prop["type"] = godot::Variant::BOOL;
        qtr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(qtr_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return modded_value_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("modded_value_buffer_id")) {
            modded_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_value_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && modded_value_buffer_id == INVALID_ID) {
            modded_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("modded_min_buffer_id")) {
            modded_min_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_min_buffer_id"]));
        }
        if (p_props.has("modded_max_buffer_id")) {
            modded_max_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_max_buffer_id"]));
        }
        if (p_props.has("base_value_buffer_id")) {
            base_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["base_value_buffer_id"]));
        }
        if (p_props.has("property_flags_buffer_id")) {
            property_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_flags_buffer_id"]));
        }
        if (p_props.has("owner_entity_id_buffer_id")) {
            owner_entity_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["owner_entity_id_buffer_id"]));
        }
        if (p_props.has("exhaustion_event_buffer_id")) {
            exhaustion_event_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["exhaustion_event_buffer_id"]));
        }
        if (p_props.has("fallback_min")) {
            fallback_min = static_cast<int32_t>(static_cast<int64_t>(p_props["fallback_min"]));
        }
        if (p_props.has("fallback_max")) {
            fallback_max = static_cast<int32_t>(static_cast<int64_t>(p_props["fallback_max"]));
        }
        if (p_props.has("default_property_flags")) {
            default_property_flags = static_cast<uint16_t>(static_cast<int64_t>(p_props["default_property_flags"]));
        }
        if (p_props.has("clamp_base_value")) {
            clamp_base_value = static_cast<bool>(p_props["clamp_base_value"]);
        }
        if (p_props.has("queue_wave_selection_command")) {
            queue_wave_selection_command = static_cast<bool>(p_props["queue_wave_selection_command"]);
        }
        if (p_props.has("queue_tier1_exhaust_event")) {
            queue_tier1_exhaust_event = static_cast<bool>(p_props["queue_tier1_exhaust_event"]);
        }
        if (p_props.has("queue_on_transition_only")) {
            queue_on_transition_only = static_cast<bool>(p_props["queue_on_transition_only"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, int32_t>) {
            const GrantPartPOD* mval_part = p_context.get_grant_part(modded_value_buffer_id);
            if (!mval_part || !mval_part->raw_base_ptr) return;

            const GrantPartPOD* mmin_part = (modded_min_buffer_id != INVALID_ID)       ? p_context.get_grant_part(modded_min_buffer_id)       : nullptr;
            const GrantPartPOD* mmax_part = (modded_max_buffer_id != INVALID_ID)       ? p_context.get_grant_part(modded_max_buffer_id)       : nullptr;
            const GrantPartPOD* bval_part = (base_value_buffer_id != INVALID_ID)       ? p_context.get_grant_part(base_value_buffer_id)       : nullptr;
            const GrantPartPOD* pflg_part = (property_flags_buffer_id != INVALID_ID)   ? p_context.get_grant_part(property_flags_buffer_id)   : nullptr;
            const GrantPartPOD* own_part  = (owner_entity_id_buffer_id != INVALID_ID)  ? p_context.get_grant_part(owner_entity_id_buffer_id)  : nullptr;

            const int32_t* __restrict modded_mins = (mmin_part && mmin_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(mmin_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict modded_maxs = (mmax_part && mmax_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(mmax_part->raw_base_ptr) : nullptr;
            int32_t* __restrict base_vals         = (bval_part && bval_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(bval_part->raw_base_ptr)       : nullptr;
            const uint32_t* __restrict owner_ids  = (own_part  && own_part->raw_base_ptr)  ? reinterpret_cast<const uint32_t*>(own_part->raw_base_ptr) : nullptr;

            uint8_t* __restrict pflg_raw  = (pflg_part && pflg_part->raw_base_ptr) ? pflg_part->raw_base_ptr : nullptr;
            const uint32_t pflg_stride    = pflg_part ? pflg_part->element_stride : 0;

            int64_t prop_cap = static_cast<int64_t>(mval_part->capacity_bytes / sizeof(int32_t));
            if (mmin_part) prop_cap = std::min<int64_t>(prop_cap, static_cast<int64_t>(mmin_part->capacity_bytes / sizeof(int32_t)));
            if (mmax_part) prop_cap = std::min<int64_t>(prop_cap, static_cast<int64_t>(mmax_part->capacity_bytes / sizeof(int32_t)));
            if (bval_part && clamp_base_value) {
                prop_cap = std::min<int64_t>(prop_cap, static_cast<int64_t>(bval_part->capacity_bytes / sizeof(int32_t)));
            }
            if (pflg_part && pflg_stride > 0) {
                prop_cap = std::min<int64_t>(prop_cap, static_cast<int64_t>(pflg_part->capacity_bytes / pflg_stride));
            }
            if (prop_cap <= 0) return;

            const int64_t own_cap = own_part ? static_cast<int64_t>(own_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const uint32_t cmd_buf_id = (exhaustion_event_buffer_id != INVALID_ID)
                ? exhaustion_event_buffer_id
                : modded_value_buffer_id;

            auto read_flags = [&](int64_t idx) noexcept -> uint16_t {
                if (!pflg_raw) return default_property_flags;
                if (pflg_stride == sizeof(uint32_t)) {
                    return static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pflg_raw)[idx]);
                }
                return reinterpret_cast<const uint16_t*>(pflg_raw)[idx];
            };

            auto write_flags = [&](int64_t idx, uint16_t flags) noexcept {
                if (!pflg_raw) return;
                if (pflg_stride == sizeof(uint32_t)) {
                    uint32_t& dst = reinterpret_cast<uint32_t*>(pflg_raw)[idx];
                    dst = (dst & 0xFFFF0000u) | static_cast<uint32_t>(flags);
                } else {
                    reinterpret_cast<uint16_t*>(pflg_raw)[idx] = flags;
                }
            };

            auto eval_property = [&](int32_t in_val, int64_t idx) noexcept -> int32_t {
                int32_t min_v = modded_mins ? modded_mins[idx] : fallback_min;
                int32_t max_v = modded_maxs ? modded_maxs[idx] : fallback_max;
                if (max_v < min_v) max_v = min_v;

                uint16_t flags = read_flags(idx);
                int32_t out_val = in_val;

                if ((flags & PROP_FLAG_RESTRICT_MAX) != 0u && out_val > max_v) {
                    out_val = max_v;
                }
                if ((flags & PROP_FLAG_RESTRICT_MIN) != 0u && out_val < min_v) {
                    out_val = min_v;
                }

                if (clamp_base_value && base_vals && (flags & PROP_FLAG_LOCKED) == 0u) {
                    int32_t bval = base_vals[idx];
                    if ((flags & PROP_FLAG_RESTRICT_MAX) != 0u && bval > max_v) bval = max_v;
                    if ((flags & PROP_FLAG_RESTRICT_MIN) != 0u && bval < min_v) bval = min_v;
                    base_vals[idx] = bval;
                }

                const bool was_exhausted = (flags & PROP_FLAG_IS_EXHAUSTED) != 0u;
                const bool now_exhausted = (out_val <= min_v);
                const bool now_at_max    = (out_val >= max_v);

                uint16_t new_flags = static_cast<uint16_t>(
                    flags & ~(PROP_FLAG_IS_EXHAUSTED | PROP_FLAG_AT_MAX |
                              PROP_FLAG_EXHAUSTED_TRANSITION | PROP_FLAG_RESTORED_TRANSITION)
                );
                if (now_exhausted) {
                    new_flags |= PROP_FLAG_IS_EXHAUSTED;
                    if (!was_exhausted) new_flags |= PROP_FLAG_EXHAUSTED_TRANSITION;
                } else if (was_exhausted) {
                    new_flags |= PROP_FLAG_RESTORED_TRANSITION;
                }
                if (now_at_max) {
                    new_flags |= PROP_FLAG_AT_MAX;
                }

                if (new_flags != flags) {
                    write_flags(idx, new_flags);
                }

                const bool should_queue = now_exhausted && (!queue_on_transition_only || !was_exhausted);
                if (should_queue) {
                    if (queue_wave_selection_command) {
                        p_context.queue_selection_command(cmd_buf_id, idx);
                    }
                    if (queue_tier1_exhaust_event && p_context.graph_commands) {
                        PropertyExhaustionEventPOD evt;
                        evt.property_idx    = static_cast<uint32_t>(idx);
                        evt.owner_entity_id = (owner_ids && idx < own_cap) ? owner_ids[idx] : INVALID_ID;
                        evt.clamped_value   = out_val;
                        evt.min_value       = min_v;
                        p_context.graph_commands->push_command(evt);
                    }
                }

                return out_val;
            };

            const MemoryBufferSelectionPOD& sel = mval_part->selection;

            // Fast-path SIMD lane iteration when bound to an AOSOAView on a contiguous RANGE selection
            if constexpr (requires { p_view.lane_count(); p_view.get_lane(0); T_View::lane_width; }) {
                if (sel.mode == SelectionMode::RANGE && sel.start_index == 0 && sel.element_count <= prop_cap) {
                    constexpr size_t L_WIDTH = T_View::lane_width;
                    const size_t lanes = p_view.lane_count();
                    for (size_t l = 0; l < lanes; ++l) {
                        T* __restrict lane_ptr = p_view.get_lane(l);
                        const int64_t base_idx = static_cast<int64_t>(l * L_WIDTH);
                        for (size_t k = 0; k < L_WIDTH; ++k) {
                            const int64_t idx = base_idx + static_cast<int64_t>(k);
                            lane_ptr[k] = eval_property(lane_ptr[k], idx);
                        }
                    }
                    const int64_t tail_start = static_cast<int64_t>(lanes * L_WIDTH);
                    const int64_t tail_end   = std::min<int64_t>(sel.element_count, prop_cap);
                    for (int64_t i = tail_start; i < tail_end; ++i) {
                        const T in_v = _read_view(p_view, i);
                        _write_view(p_view, i, eval_property(in_v, i));
                    }
                    return;
                }
            }

            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                const int64_t cap = std::min<int64_t>(sel.capacity, prop_cap);
                for (int64_t i = 0; i < cap; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        const T in_v = _read_view(p_view, i);
                        _write_view(p_view, i, eval_property(in_v, i));
                    }
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    const int64_t real_idx = sel.data.indices[i];
                    if (real_idx >= 0 && real_idx < prop_cap) {
                        const T in_v = _read_view(p_view, i);
                        _write_view(p_view, i, eval_property(in_v, real_idx));
                    }
                }
            } else {
                const int64_t count = (sel.element_count > 0)
                    ? std::min<int64_t>(sel.element_count, prop_cap - sel.start_index)
                    : prop_cap;
                for (int64_t i = 0; i < count; ++i) {
                    const int64_t real_idx = sel.start_index + i;
                    const T in_v = _read_view(p_view, i);
                    _write_view(p_view, i, eval_property(in_v, real_idx));
                }
            }
        }
    }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T _read_view(const T_View& p_view, int64_t idx) noexcept {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T>(p_view[idx].load());
        } else if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else {
            return static_cast<T>(p_view[idx]);
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _write_view(const T_View& p_view, int64_t idx, const T& value) noexcept {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<T*>(p_view[idx]) = value;
        } else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value;
        } else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } else {
            (void)(p_view[idx] = value);
        }
    }
};

static_assert(sizeof(PropertyRangeClampAndExhaustTransformLogic<>) == 64, "PropertyRangeClampAndExhaustTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core