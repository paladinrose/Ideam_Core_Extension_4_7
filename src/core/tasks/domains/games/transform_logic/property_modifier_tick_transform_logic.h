#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_PROPERTY_MODIFIER_POD_DEFINED
#define IDEAM_GAMES_PROPERTY_MODIFIER_POD_DEFINED

/**
 * Mirrors ideam::godot_ext::ModifierType for DOD PropertyModifierBuffer columns.
 */
enum PropertyModifierType : uint8_t {
    MOD_TYPE_STATIC            = 0,
    MOD_TYPE_TIMED             = 1,
    MOD_TYPE_TIMED_DIMINISHING = 2,
    MOD_TYPE_RESOURCE          = 3
};

/**
 * Target channel within GamePropertyBuffer modified by a PropertyModifierBuffer entry.
 */
enum PropertyModifierChannel : uint8_t {
    MOD_CHANNEL_VALUE = 0, // GamePropertyBuffer::modded_value
    MOD_CHANNEL_MAX   = 1, // GamePropertyBuffer::modded_max
    MOD_CHANNEL_MIN   = 2  // GamePropertyBuffer::modded_min
};

/**
 * Bitflags for PropertyModifierBuffer::flags (uint8_t).
 */
enum PropertyModifierFlags : uint8_t {
    MOD_FLAG_NONE      = 0u,
    MOD_FLAG_ACTIVE    = 1u << 0,
    MOD_FLAG_EXPIRED   = 1u << 1,
    MOD_FLAG_PROCESSED = 1u << 2
};

/**
 * PropertyModifierPOD (Strictly 32-Byte Aligned)
 * Contiguous AoS representation of a GamePropertyModifier inside PropertyModifierBuffer.
 * First 24 bytes hold the hot mathematical & routing fields; 8 bytes of tail padding
 * lock each record to 32 bytes (2 records per 64-byte CPU cache line).
 */
struct alignas(32) PropertyModifierPOD {
    uint32_t target_property_idx = INVALID_ID;            // 4B: Target row in GamePropertyBuffer
    int32_t value                = 0;                     // 4B: Current active delta
    int32_t starting_value       = 0;                     // 4B: Initial value captured at modifier_start()
    float time_limit             = 0.0f;                  // 4B: Total duration in seconds
    float current_time           = 0.0f;                  // 4B: Remaining (countdown) or elapsed time in seconds
    uint8_t mod_type             = MOD_TYPE_STATIC;       // 1B: PropertyModifierType
    uint8_t target_channel       = MOD_CHANNEL_VALUE;     // 1B: PropertyModifierChannel (0=value, 1=max, 2=min)
    uint8_t flags                = MOD_FLAG_ACTIVE;       // 1B: PropertyModifierFlags
    uint8_t reserved_pad[9]      = {0};                   // 9B -> Total = 32B
};

static_assert(sizeof(PropertyModifierPOD) == 32, "PropertyModifierPOD must be 32-byte aligned!");

#endif // IDEAM_GAMES_PROPERTY_MODIFIER_POD_DEFINED

/**
 * PropertyModifierTickTransformLogic<T> (G2)
 * Batch-decrements (or increments in elapsed mode) TIMED and TIMED_DIMINISHING modifier
 * timers by scaled delta, computes linearly interpolated diminishing values, and flags
 * expired modifiers for removal without per-Node virtual calls or signal overhead.
 * Supports both SoA column buffers (float/int32_t) and packed AoS PropertyModifierPOD buffers.
 */
template <typename T = float>
struct alignas(64) PropertyModifierTickTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::FLOAT32 | DataType::INT32 | DataType::CUSTOM;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Property Modifier Tick";
    static constexpr std::string_view ui_category  = "Properties";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t current_time_buffer_id         = INVALID_ID; // 4B: Primary float current_time column (or AoS POD buffer)
    uint32_t time_limit_buffer_id           = INVALID_ID; // 4B: Float time_limit column
    uint32_t value_buffer_id                = INVALID_ID; // 4B: Int32 current modifier value column
    uint32_t starting_value_buffer_id       = INVALID_ID; // 4B: Int32 starting_value column
    uint32_t mod_type_buffer_id             = INVALID_ID; // 4B: uint8_t or int32_t mod_type column
    uint32_t expired_flags_buffer_id        = INVALID_ID; // 4B: Optional uint8_t or uint32_t modifier flags/expired column
    uint32_t target_property_idx_buffer_id  = INVALID_ID; // 4B: Optional uint32_t target_property_idx column
    uint32_t effective_time_scale_buffer_id = INVALID_ID; // 4B: Optional float per-modifier or per-property time_scale buffer
    uint32_t modifier_pod_buffer_id         = INVALID_ID; // 4B: Optional packed PropertyModifierPOD AoS buffer

    float time_scale                        = 1.0f;       // 4B: Global time scale multiplier
    bool countdown_mode                     = true;       // 1B: true = decrement current_time to 0; false = increment to time_limit
    bool zero_value_on_expire               = true;       // 1B: Zeroes modifier value upon expiration
    bool queue_expired_commands             = false;      // 1B: Queues expired modifier indices into Tier 2 wave_commands
    uint8_t reserved_padding[21]            = {0};        // 21B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary cur_prop;
        cur_prop["name"] = "current_time_buffer_id";
        cur_prop["type"] = godot::Variant::INT;
        cur_prop["hint_string"] = "buffer_option";
        props.push_back(cur_prop);

        godot::Dictionary lim_prop;
        lim_prop["name"] = "time_limit_buffer_id";
        lim_prop["type"] = godot::Variant::INT;
        lim_prop["hint_string"] = "buffer_option";
        props.push_back(lim_prop);

        godot::Dictionary val_prop;
        val_prop["name"] = "value_buffer_id";
        val_prop["type"] = godot::Variant::INT;
        val_prop["hint_string"] = "buffer_option";
        props.push_back(val_prop);

        godot::Dictionary start_prop;
        start_prop["name"] = "starting_value_buffer_id";
        start_prop["type"] = godot::Variant::INT;
        start_prop["hint_string"] = "buffer_option";
        props.push_back(start_prop);

        godot::Dictionary type_prop;
        type_prop["name"] = "mod_type_buffer_id";
        type_prop["type"] = godot::Variant::INT;
        type_prop["hint_string"] = "buffer_option";
        props.push_back(type_prop);

        godot::Dictionary exp_prop;
        exp_prop["name"] = "expired_flags_buffer_id";
        exp_prop["type"] = godot::Variant::INT;
        exp_prop["hint_string"] = "buffer_option";
        props.push_back(exp_prop);

        godot::Dictionary tprop_prop;
        tprop_prop["name"] = "target_property_idx_buffer_id";
        tprop_prop["type"] = godot::Variant::INT;
        tprop_prop["hint_string"] = "buffer_option";
        props.push_back(tprop_prop);

        godot::Dictionary tsbuf_prop;
        tsbuf_prop["name"] = "effective_time_scale_buffer_id";
        tsbuf_prop["type"] = godot::Variant::INT;
        tsbuf_prop["hint_string"] = "buffer_option";
        props.push_back(tsbuf_prop);

        godot::Dictionary pod_prop;
        pod_prop["name"] = "modifier_pod_buffer_id";
        pod_prop["type"] = godot::Variant::INT;
        pod_prop["hint_string"] = "buffer_option";
        props.push_back(pod_prop);

        godot::Dictionary ts_prop;
        ts_prop["name"] = "time_scale";
        ts_prop["type"] = godot::Variant::FLOAT;
        ts_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        ts_prop["hint_string"] = "0.0,10.0,0.01,suffix:x";
        props.push_back(ts_prop);

        godot::Dictionary cd_prop;
        cd_prop["name"] = "countdown_mode";
        cd_prop["type"] = godot::Variant::BOOL;
        cd_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cd_prop);

        godot::Dictionary zv_prop;
        zv_prop["name"] = "zero_value_on_expire";
        zv_prop["type"] = godot::Variant::BOOL;
        zv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(zv_prop);

        godot::Dictionary q_prop;
        q_prop["name"] = "queue_expired_commands";
        q_prop["type"] = godot::Variant::BOOL;
        q_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(q_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (current_time_buffer_id != INVALID_ID) return current_time_buffer_id;
        if (modifier_pod_buffer_id != INVALID_ID) return modifier_pod_buffer_id;
        return value_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("current_time_buffer_id")) {
            current_time_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["current_time_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && current_time_buffer_id == INVALID_ID) {
            current_time_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("time_limit_buffer_id")) {
            time_limit_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["time_limit_buffer_id"]));
        }
        if (p_props.has("value_buffer_id")) {
            value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["value_buffer_id"]));
        }
        if (p_props.has("starting_value_buffer_id")) {
            starting_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["starting_value_buffer_id"]));
        }
        if (p_props.has("mod_type_buffer_id")) {
            mod_type_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mod_type_buffer_id"]));
        }
        if (p_props.has("expired_flags_buffer_id")) {
            expired_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["expired_flags_buffer_id"]));
        }
        if (p_props.has("target_property_idx_buffer_id")) {
            target_property_idx_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_property_idx_buffer_id"]));
        }
        if (p_props.has("effective_time_scale_buffer_id")) {
            effective_time_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["effective_time_scale_buffer_id"]));
        }
        if (p_props.has("modifier_pod_buffer_id")) {
            modifier_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modifier_pod_buffer_id"]));
        }
        if (p_props.has("time_scale")) {
            time_scale = static_cast<float>(p_props["time_scale"]);
        }
        if (p_props.has("countdown_mode")) {
            countdown_mode = static_cast<bool>(p_props["countdown_mode"]);
        }
        if (p_props.has("zero_value_on_expire")) {
            zero_value_on_expire = static_cast<bool>(p_props["zero_value_on_expire"]);
        }
        if (p_props.has("queue_expired_commands")) {
            queue_expired_commands = static_cast<bool>(p_props["queue_expired_commands"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        const float base_dt = static_cast<float>(p_context.delta) * time_scale;
        if (base_dt <= 0.0f) return;

        const GrantPartPOD* ts_part = (effective_time_scale_buffer_id != INVALID_ID)
            ? p_context.get_grant_part(effective_time_scale_buffer_id)
            : nullptr;
        const float* __restrict time_scales = (ts_part && ts_part->raw_base_ptr)
            ? reinterpret_cast<const float*>(ts_part->raw_base_ptr)
            : nullptr;
        const int64_t ts_cap = ts_part ? static_cast<int64_t>(ts_part->capacity_bytes / sizeof(float)) : 0;

        // --- PATH A: Packed AoS PropertyModifierPOD Buffer ---
        const uint32_t pod_id = (modifier_pod_buffer_id != INVALID_ID)
            ? modifier_pod_buffer_id
            : ((std::is_same_v<T, void*> && current_time_buffer_id != INVALID_ID) ? current_time_buffer_id : INVALID_ID);

        if (pod_id != INVALID_ID) {
            const GrantPartPOD* pod_part = p_context.get_grant_part(pod_id);
            if (!pod_part || !pod_part->raw_base_ptr) return;

            PropertyModifierPOD* __restrict pods = reinterpret_cast<PropertyModifierPOD*>(pod_part->raw_base_ptr);
            const int64_t pod_cap = static_cast<int64_t>(pod_part->capacity_bytes / sizeof(PropertyModifierPOD));
            if (pod_cap <= 0) return;

            auto tick_pod = [&](int64_t i) noexcept {
                PropertyModifierPOD& mod = pods[i];
                if ((mod.flags & MOD_FLAG_ACTIVE) == 0u || (mod.flags & MOD_FLAG_EXPIRED) != 0u) return;
                if (mod.mod_type != MOD_TYPE_TIMED && mod.mod_type != MOD_TYPE_TIMED_DIMINISHING) return;

                float dt = base_dt;
                if (time_scales) {
                    const int64_t ts_idx = (mod.target_property_idx != INVALID_ID && static_cast<int64_t>(mod.target_property_idx) < ts_cap)
                        ? static_cast<int64_t>(mod.target_property_idx)
                        : ((i < ts_cap) ? i : -1);
                    if (ts_idx >= 0) dt *= time_scales[ts_idx];
                }
                if (dt <= 0.0f) return;

                if (countdown_mode) {
                    mod.current_time -= dt;
                    if (mod.current_time <= 0.0f || mod.time_limit <= 0.0f) {
                        mod.current_time = 0.0f;
                        if (mod.mod_type == MOD_TYPE_TIMED_DIMINISHING || zero_value_on_expire) {
                            mod.value = 0;
                        }
                        mod.flags = static_cast<uint8_t>((mod.flags & ~MOD_FLAG_ACTIVE) | MOD_FLAG_EXPIRED);
                        if (queue_expired_commands) {
                            p_context.queue_selection_command(pod_id, i);
                        }
                        return;
                    }
                    if (mod.mod_type == MOD_TYPE_TIMED_DIMINISHING) {
                        const float ratio = std::clamp(mod.current_time / mod.time_limit, 0.0f, 1.0f);
                        mod.value = static_cast<int32_t>(static_cast<float>(mod.starting_value) * ratio);
                    }
                } else {
                    mod.current_time += dt;
                    if (mod.current_time >= mod.time_limit) {
                        mod.current_time = mod.time_limit;
                        if (mod.mod_type == MOD_TYPE_TIMED_DIMINISHING || zero_value_on_expire) {
                            mod.value = 0;
                        }
                        mod.flags = static_cast<uint8_t>((mod.flags & ~MOD_FLAG_ACTIVE) | MOD_FLAG_EXPIRED);
                        if (queue_expired_commands) {
                            p_context.queue_selection_command(pod_id, i);
                        }
                        return;
                    }
                    if (mod.mod_type == MOD_TYPE_TIMED_DIMINISHING && mod.time_limit > 0.0f) {
                        const float p = std::clamp(mod.current_time / mod.time_limit, 0.0f, 1.0f);
                        mod.value = static_cast<int32_t>(static_cast<float>(mod.starting_value) * (1.0f - p));
                    }
                }
            };

            _iterate_selection(pod_part->selection, pod_cap, tick_pod);
            return;
        }

        // --- PATH B: SoA Column Buffers ---
        const GrantPartPOD* cur_part   = (current_time_buffer_id != INVALID_ID)        ? p_context.get_grant_part(current_time_buffer_id)        : nullptr;
        const GrantPartPOD* lim_part   = (time_limit_buffer_id != INVALID_ID)          ? p_context.get_grant_part(time_limit_buffer_id)          : nullptr;
        const GrantPartPOD* val_part   = (value_buffer_id != INVALID_ID)               ? p_context.get_grant_part(value_buffer_id)               : nullptr;
        const GrantPartPOD* start_part = (starting_value_buffer_id != INVALID_ID)      ? p_context.get_grant_part(starting_value_buffer_id)      : nullptr;
        const GrantPartPOD* type_part  = (mod_type_buffer_id != INVALID_ID)            ? p_context.get_grant_part(mod_type_buffer_id)            : nullptr;
        const GrantPartPOD* exp_part   = (expired_flags_buffer_id != INVALID_ID)       ? p_context.get_grant_part(expired_flags_buffer_id)       : nullptr;
        const GrantPartPOD* tprop_part = (target_property_idx_buffer_id != INVALID_ID) ? p_context.get_grant_part(target_property_idx_buffer_id) : nullptr;

        if (!cur_part || !cur_part->raw_base_ptr) return;

        float* __restrict cur_times           = reinterpret_cast<float*>(cur_part->raw_base_ptr);
        const float* __restrict lim_times     = (lim_part && lim_part->raw_base_ptr)     ? reinterpret_cast<const float*>(lim_part->raw_base_ptr)       : nullptr;
        int32_t* __restrict values            = (val_part && val_part->raw_base_ptr)     ? reinterpret_cast<int32_t*>(val_part->raw_base_ptr)           : nullptr;
        const int32_t* __restrict start_vals  = (start_part && start_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(start_part->raw_base_ptr)   : nullptr;
        const uint32_t* __restrict tprop_idxs = (tprop_part && tprop_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(tprop_part->raw_base_ptr)  : nullptr;

        int64_t mod_count = static_cast<int64_t>(cur_part->capacity_bytes / sizeof(float));
        if (lim_times)  mod_count = std::min<int64_t>(mod_count, static_cast<int64_t>(lim_part->capacity_bytes / sizeof(float)));
        if (values)     mod_count = std::min<int64_t>(mod_count, static_cast<int64_t>(val_part->capacity_bytes / sizeof(int32_t)));
        if (start_vals) mod_count = std::min<int64_t>(mod_count, static_cast<int64_t>(start_part->capacity_bytes / sizeof(int32_t)));
        if (mod_count <= 0) return;

        const uint32_t type_stride = type_part ? type_part->element_stride : 0;
        const uint8_t* __restrict type_raw = (type_part && type_part->raw_base_ptr) ? type_part->raw_base_ptr : nullptr;

        const uint32_t exp_stride = exp_part ? exp_part->element_stride : 0;
        uint8_t* __restrict exp_raw = (exp_part && exp_part->raw_base_ptr) ? exp_part->raw_base_ptr : nullptr;

        auto read_mod_type = [&](int64_t idx) noexcept -> uint8_t {
            if (!type_raw) return MOD_TYPE_TIMED;
            if (type_stride == sizeof(int32_t)) {
                return static_cast<uint8_t>(reinterpret_cast<const int32_t*>(type_raw)[idx]);
            }
            return type_raw[idx];
        };

        auto mark_expired = [&](int64_t idx) noexcept {
            if (exp_raw) {
                if (exp_stride == sizeof(uint32_t)) {
                    uint32_t& flg = reinterpret_cast<uint32_t*>(exp_raw)[idx];
                    flg = (flg & ~static_cast<uint32_t>(MOD_FLAG_ACTIVE)) | static_cast<uint32_t>(MOD_FLAG_EXPIRED);
                } else {
                    exp_raw[idx] = static_cast<uint8_t>((exp_raw[idx] & ~MOD_FLAG_ACTIVE) | MOD_FLAG_EXPIRED);
                }
            }
            if (queue_expired_commands) {
                p_context.queue_selection_command(current_time_buffer_id, idx);
            }
        };

        auto tick_soa = [&](int64_t i) noexcept {
            const uint8_t mtype = read_mod_type(i);
            if (mtype != MOD_TYPE_TIMED && mtype != MOD_TYPE_TIMED_DIMINISHING) return;

            float dt = base_dt;
            if (time_scales) {
                const int64_t ts_idx = (tprop_idxs && tprop_idxs[i] != INVALID_ID && static_cast<int64_t>(tprop_idxs[i]) < ts_cap)
                    ? static_cast<int64_t>(tprop_idxs[i])
                    : ((i < ts_cap) ? i : -1);
                if (ts_idx >= 0) dt *= time_scales[ts_idx];
            }
            if (dt <= 0.0f) return;

            const float limit = lim_times ? lim_times[i] : 0.0f;
            float cur         = cur_times[i];

            if (countdown_mode) {
                cur -= dt;
                if (cur <= 0.0f || (lim_times && limit <= 0.0f)) {
                    cur_times[i] = 0.0f;
                    if (values && (mtype == MOD_TYPE_TIMED_DIMINISHING || zero_value_on_expire)) {
                        values[i] = 0;
                    }
                    mark_expired(i);
                    return;
                }
                cur_times[i] = cur;
                if (mtype == MOD_TYPE_TIMED_DIMINISHING && values && start_vals && limit > 0.0f) {
                    const float ratio = std::clamp(cur / limit, 0.0f, 1.0f);
                    values[i] = static_cast<int32_t>(static_cast<float>(start_vals[i]) * ratio);
                }
            } else {
                cur += dt;
                if (lim_times && cur >= limit) {
                    cur_times[i] = limit;
                    if (values && (mtype == MOD_TYPE_TIMED_DIMINISHING || zero_value_on_expire)) {
                        values[i] = 0;
                    }
                    mark_expired(i);
                    return;
                }
                cur_times[i] = cur;
                if (mtype == MOD_TYPE_TIMED_DIMINISHING && values && start_vals && limit > 0.0f) {
                    const float p = std::clamp(cur / limit, 0.0f, 1.0f);
                    values[i] = static_cast<int32_t>(static_cast<float>(start_vals[i]) * (1.0f - p));
                }
            }
        };

        _iterate_selection(cur_part->selection, mod_count, tick_soa);
    }

private:
    template <typename Fn>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _iterate_selection(const MemoryBufferSelectionPOD& p_sel, int64_t p_cap, Fn&& p_fn) noexcept {
        if (p_sel.mode == SelectionMode::DENSE && p_sel.data.bitset) {
            const uint64_t* bitset = p_sel.data.bitset;
            const int64_t cap = std::min<int64_t>(p_sel.capacity, p_cap);
            for (int64_t i = 0; i < cap; ++i) {
                if (bitset[i >> 6] & (1ULL << (i & 63))) {
                    p_fn(i);
                }
            }
        } else if (p_sel.mode == SelectionMode::SPARSE && p_sel.data.indices) {
            for (int64_t i = 0; i < p_sel.element_count; ++i) {
                const int64_t idx = p_sel.data.indices[i];
                if (idx >= 0 && idx < p_cap) {
                    p_fn(idx);
                }
            }
        } else {
            const int64_t start = std::max<int64_t>(0, p_sel.start_index);
            const int64_t count = (p_sel.element_count > 0) ? p_sel.element_count : p_cap;
            const int64_t end   = std::min<int64_t>(start + count, p_cap);
            for (int64_t i = start; i < end; ++i) {
                p_fn(i);
            }
        }
    }
};

static_assert(sizeof(PropertyModifierTickTransformLogic<>) == 64, "PropertyModifierTickTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core