#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/atomic_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "property_modifier_tick_transform_logic.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_PROPERTY_FLAGS_DEFINED
#define IDEAM_GAMES_PROPERTY_FLAGS_DEFINED

/**
 * Bitfield packing all 11 boolean flags of GameProperty (plus runtime transition bits)
 * into a 16-bit (or 32-bit) integer column in GamePropertyBuffer::property_flags.
 */
enum GamePropertyFlags : uint16_t {
    PROP_FLAG_NONE                 = 0u,
    PROP_FLAG_RESTRICT_MAX         = 1u << 0,  // GameProperty::restrict_value_to_current_max_value
    PROP_FLAG_RESTRICT_MIN         = 1u << 1,  // GameProperty::restrict_value_to_current_min_value
    PROP_FLAG_ALLOW_VALUE_SET      = 1u << 2,  // GameProperty::allow_value_set
    PROP_FLAG_ALLOW_VALUE_MODS     = 1u << 3,  // GameProperty::allow_value_modifiers
    PROP_FLAG_ALLOW_MAX_SET        = 1u << 4,  // GameProperty::allow_max_value_set
    PROP_FLAG_ALLOW_MAX_MODS       = 1u << 5,  // GameProperty::allow_max_modifiers
    PROP_FLAG_ALLOW_MIN_SET        = 1u << 6,  // GameProperty::allow_min_value_set
    PROP_FLAG_ALLOW_MIN_MODS       = 1u << 7,  // GameProperty::allow_min_modifiers
    PROP_FLAG_GAME_PROCESSED       = 1u << 8,  // GameProperty::game_processed
    PROP_FLAG_IS_EXHAUSTED         = 1u << 9,  // GameProperty::is_exhausted
    PROP_FLAG_LOCKED               = 1u << 10, // GameProperty::locked
    PROP_FLAG_AT_MAX               = 1u << 11, // Runtime pseudo-state helper (:at_max)
    PROP_FLAG_EXHAUSTED_TRANSITION = 1u << 12, // Set when transitioning into exhaustion this frame
    PROP_FLAG_RESTORED_TRANSITION  = 1u << 13, // Set when transitioning out of exhaustion this frame
    PROP_FLAG_DEFAULT_MASK         =
        PROP_FLAG_RESTRICT_MAX    | PROP_FLAG_RESTRICT_MIN     |
        PROP_FLAG_ALLOW_VALUE_SET | PROP_FLAG_ALLOW_VALUE_MODS |
        PROP_FLAG_ALLOW_MAX_SET   | PROP_FLAG_ALLOW_MAX_MODS   |
        PROP_FLAG_ALLOW_MIN_SET   | PROP_FLAG_ALLOW_MIN_MODS
};

#endif // IDEAM_GAMES_PROPERTY_FLAGS_DEFINED

/**
 * PropertyModifierAggregateTransformLogic<T> (G3)
 * Resets GamePropertyBuffer::modded_value / modded_min / modded_max to base_value / base_min / base_max
 * and accumulates active PropertyModifierBuffer deltas into GamePropertyBuffer in a single pass
 * using lock-free std::atomic_ref<int32_t>::fetch_add operations.
 * Replaces GameProperty::get_modded_value(), get_modded_max_value(), and get_modded_min_value().
 */
template <typename T = int32_t>
struct alignas(64) PropertyModifierAggregateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using AtomicElemType  = std::conditional_t<std::is_integral_v<T> || std::is_floating_point_v<T>, T, int32_t>;
    using DefaultView     = AtomicView<AtomicElemType, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Property Modifier Aggregate";
    static constexpr std::string_view ui_category  = "Properties";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t modded_value_buffer_id        = INVALID_ID; // 4B: Primary target buffer (GamePropertyBuffer::modded_value)
    uint32_t base_value_buffer_id          = INVALID_ID; // 4B: GamePropertyBuffer::base_value (int32_t)
    uint32_t modded_min_buffer_id          = INVALID_ID; // 4B: GamePropertyBuffer::modded_min (int32_t)
    uint32_t base_min_buffer_id            = INVALID_ID; // 4B: GamePropertyBuffer::base_min (int32_t)
    uint32_t modded_max_buffer_id          = INVALID_ID; // 4B: GamePropertyBuffer::modded_max (int32_t)
    uint32_t base_max_buffer_id            = INVALID_ID; // 4B: GamePropertyBuffer::base_max (int32_t)
    uint32_t property_flags_buffer_id      = INVALID_ID; // 4B: Optional GamePropertyBuffer::property_flags (uint16_t/uint32_t)

    uint32_t modifier_pod_buffer_id        = INVALID_ID; // 4B: Packed PropertyModifierPOD AoS buffer
    uint32_t mod_target_property_buffer_id = INVALID_ID; // 4B: SoA target_property_idx (uint32_t) column
    uint32_t mod_value_buffer_id           = INVALID_ID; // 4B: SoA modifier value (int32_t) column
    uint32_t mod_channel_buffer_id         = INVALID_ID; // 4B: SoA target_channel (uint8_t: 0=value, 1=max, 2=min) column
    uint32_t mod_flags_buffer_id           = INVALID_ID; // 4B: Optional SoA modifier flags (uint8_t) column

    bool reset_to_base                     = true;       // 1B: Reset modded_value/min/max to base_value/min/max first
    bool respect_allow_flags               = true;       // 1B: Enforce allow_value/max/min_modifiers bits in property_flags
    bool skip_locked_properties            = false;      // 1B: Skip modifier accumulation when PROP_FLAG_LOCKED is set
    uint8_t default_channel                = MOD_CHANNEL_VALUE; // 1B: Fallback channel when mod_channel_buffer_id is unbound
    uint8_t reserved_padding[12]           = {0};        // 12B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary mval_prop;
        mval_prop["name"] = "modded_value_buffer_id";
        mval_prop["type"] = godot::Variant::INT;
        mval_prop["hint_string"] = "buffer_option";
        props.push_back(mval_prop);

        godot::Dictionary bval_prop;
        bval_prop["name"] = "base_value_buffer_id";
        bval_prop["type"] = godot::Variant::INT;
        bval_prop["hint_string"] = "buffer_option";
        props.push_back(bval_prop);

        godot::Dictionary mmin_prop;
        mmin_prop["name"] = "modded_min_buffer_id";
        mmin_prop["type"] = godot::Variant::INT;
        mmin_prop["hint_string"] = "buffer_option";
        props.push_back(mmin_prop);

        godot::Dictionary bmin_prop;
        bmin_prop["name"] = "base_min_buffer_id";
        bmin_prop["type"] = godot::Variant::INT;
        bmin_prop["hint_string"] = "buffer_option";
        props.push_back(bmin_prop);

        godot::Dictionary mmax_prop;
        mmax_prop["name"] = "modded_max_buffer_id";
        mmax_prop["type"] = godot::Variant::INT;
        mmax_prop["hint_string"] = "buffer_option";
        props.push_back(mmax_prop);

        godot::Dictionary bmax_prop;
        bmax_prop["name"] = "base_max_buffer_id";
        bmax_prop["type"] = godot::Variant::INT;
        bmax_prop["hint_string"] = "buffer_option";
        props.push_back(bmax_prop);

        godot::Dictionary pflg_prop;
        pflg_prop["name"] = "property_flags_buffer_id";
        pflg_prop["type"] = godot::Variant::INT;
        pflg_prop["hint_string"] = "buffer_option";
        props.push_back(pflg_prop);

        godot::Dictionary mpod_prop;
        mpod_prop["name"] = "modifier_pod_buffer_id";
        mpod_prop["type"] = godot::Variant::INT;
        mpod_prop["hint_string"] = "buffer_option";
        props.push_back(mpod_prop);

        godot::Dictionary mtgt_prop;
        mtgt_prop["name"] = "mod_target_property_buffer_id";
        mtgt_prop["type"] = godot::Variant::INT;
        mtgt_prop["hint_string"] = "buffer_option";
        props.push_back(mtgt_prop);

        godot::Dictionary mdv_prop;
        mdv_prop["name"] = "mod_value_buffer_id";
        mdv_prop["type"] = godot::Variant::INT;
        mdv_prop["hint_string"] = "buffer_option";
        props.push_back(mdv_prop);

        godot::Dictionary mch_prop;
        mch_prop["name"] = "mod_channel_buffer_id";
        mch_prop["type"] = godot::Variant::INT;
        mch_prop["hint_string"] = "buffer_option";
        props.push_back(mch_prop);

        godot::Dictionary mflg_prop;
        mflg_prop["name"] = "mod_flags_buffer_id";
        mflg_prop["type"] = godot::Variant::INT;
        mflg_prop["hint_string"] = "buffer_option";
        props.push_back(mflg_prop);

        godot::Dictionary rst_prop;
        rst_prop["name"] = "reset_to_base";
        rst_prop["type"] = godot::Variant::BOOL;
        rst_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rst_prop);

        godot::Dictionary alw_prop;
        alw_prop["name"] = "respect_allow_flags";
        alw_prop["type"] = godot::Variant::BOOL;
        alw_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(alw_prop);

        godot::Dictionary lck_prop;
        lck_prop["name"] = "skip_locked_properties";
        lck_prop["type"] = godot::Variant::BOOL;
        lck_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(lck_prop);

        godot::Dictionary dch_prop;
        dch_prop["name"] = "default_channel";
        dch_prop["type"] = godot::Variant::INT;
        dch_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dch_prop["hint_string"] = "Value:0,Max Value:1,Min Value:2";
        props.push_back(dch_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (modded_value_buffer_id != INVALID_ID) return modded_value_buffer_id;
        if (modded_max_buffer_id != INVALID_ID)   return modded_max_buffer_id;
        return modded_min_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("modded_value_buffer_id")) {
            modded_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_value_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && modded_value_buffer_id == INVALID_ID) {
            modded_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("base_value_buffer_id")) {
            base_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["base_value_buffer_id"]));
        }
        if (p_props.has("modded_min_buffer_id")) {
            modded_min_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_min_buffer_id"]));
        }
        if (p_props.has("base_min_buffer_id")) {
            base_min_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["base_min_buffer_id"]));
        }
        if (p_props.has("modded_max_buffer_id")) {
            modded_max_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modded_max_buffer_id"]));
        }
        if (p_props.has("base_max_buffer_id")) {
            base_max_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["base_max_buffer_id"]));
        }
        if (p_props.has("property_flags_buffer_id")) {
            property_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_flags_buffer_id"]));
        }
        if (p_props.has("modifier_pod_buffer_id")) {
            modifier_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["modifier_pod_buffer_id"]));
        }
        if (p_props.has("mod_target_property_buffer_id")) {
            mod_target_property_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mod_target_property_buffer_id"]));
        }
        if (p_props.has("mod_value_buffer_id")) {
            mod_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mod_value_buffer_id"]));
        }
        if (p_props.has("mod_channel_buffer_id")) {
            mod_channel_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mod_channel_buffer_id"]));
        }
        if (p_props.has("mod_flags_buffer_id")) {
            mod_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mod_flags_buffer_id"]));
        }
        if (p_props.has("reset_to_base")) {
            reset_to_base = static_cast<bool>(p_props["reset_to_base"]);
        }
        if (p_props.has("respect_allow_flags")) {
            respect_allow_flags = static_cast<bool>(p_props["respect_allow_flags"]);
        }
        if (p_props.has("skip_locked_properties")) {
            skip_locked_properties = static_cast<bool>(p_props["skip_locked_properties"]);
        }
        if (p_props.has("default_channel")) {
            default_channel = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["default_channel"]), 0, 2));
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
        if constexpr (std::is_same_v<T, int32_t>) {
            const GrantPartPOD* mval_part = (modded_value_buffer_id != INVALID_ID)   ? p_context.get_grant_part(modded_value_buffer_id)   : nullptr;
            const GrantPartPOD* bval_part = (base_value_buffer_id != INVALID_ID)     ? p_context.get_grant_part(base_value_buffer_id)     : nullptr;
            const GrantPartPOD* mmin_part = (modded_min_buffer_id != INVALID_ID)     ? p_context.get_grant_part(modded_min_buffer_id)     : nullptr;
            const GrantPartPOD* bmin_part = (base_min_buffer_id != INVALID_ID)       ? p_context.get_grant_part(base_min_buffer_id)       : nullptr;
            const GrantPartPOD* mmax_part = (modded_max_buffer_id != INVALID_ID)     ? p_context.get_grant_part(modded_max_buffer_id)     : nullptr;
            const GrantPartPOD* bmax_part = (base_max_buffer_id != INVALID_ID)       ? p_context.get_grant_part(base_max_buffer_id)       : nullptr;
            const GrantPartPOD* pflg_part = (property_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(property_flags_buffer_id) : nullptr;

            int32_t* __restrict modded_vals     = (mval_part && mval_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(mval_part->raw_base_ptr)       : nullptr;
            const int32_t* __restrict base_vals = (bval_part && bval_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(bval_part->raw_base_ptr) : nullptr;
            int32_t* __restrict modded_mins     = (mmin_part && mmin_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(mmin_part->raw_base_ptr)       : nullptr;
            const int32_t* __restrict base_mins = (bmin_part && bmin_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(bmin_part->raw_base_ptr) : nullptr;
            int32_t* __restrict modded_maxs     = (mmax_part && mmax_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(mmax_part->raw_base_ptr)       : nullptr;
            const int32_t* __restrict base_maxs = (bmax_part && bmax_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(bmax_part->raw_base_ptr) : nullptr;

            const GrantPartPOD* primary_prop_part = mval_part ? mval_part : (mmax_part ? mmax_part : mmin_part);
            if (!primary_prop_part) return;

            int64_t prop_count = static_cast<int64_t>(primary_prop_part->capacity_bytes / sizeof(int32_t));
            if (mval_part) prop_count = std::min<int64_t>(prop_count, static_cast<int64_t>(mval_part->capacity_bytes / sizeof(int32_t)));
            if (bval_part) prop_count = std::min<int64_t>(prop_count, static_cast<int64_t>(bval_part->capacity_bytes / sizeof(int32_t)));
            if (mmin_part) prop_count = std::min<int64_t>(prop_count, static_cast<int64_t>(mmin_part->capacity_bytes / sizeof(int32_t)));
            if (bmin_part) prop_count = std::min<int64_t>(prop_count, static_cast<int64_t>(bmin_part->capacity_bytes / sizeof(int32_t)));
            if (mmax_part) prop_count = std::min<int64_t>(prop_count, static_cast<int64_t>(mmax_part->capacity_bytes / sizeof(int32_t)));
            if (bmax_part) prop_count = std::min<int64_t>(prop_count, static_cast<int64_t>(bmax_part->capacity_bytes / sizeof(int32_t)));
            if (prop_count <= 0) return;

            // --- Phase 1: Reset modded_value / modded_min / modded_max to base_* ---
            if (reset_to_base) {
                auto reset_prop = [&](int64_t idx) noexcept {
                    if (modded_vals && base_vals) {
                        std::atomic_ref<int32_t>(modded_vals[idx]).store(base_vals[idx], std::memory_order_relaxed);
                    }
                    if (modded_mins && base_mins) {
                        std::atomic_ref<int32_t>(modded_mins[idx]).store(base_mins[idx], std::memory_order_relaxed);
                    }
                    if (modded_maxs && base_maxs) {
                        std::atomic_ref<int32_t>(modded_maxs[idx]).store(base_maxs[idx], std::memory_order_relaxed);
                    }
                };
                _iterate_selection(primary_prop_part->selection, prop_count, reset_prop);
            }

            // Helper to read property_flags (supports both uint16_t 2B and uint32_t/int32_t 4B strides)
            const uint8_t* __restrict pflg_raw = (pflg_part && pflg_part->raw_base_ptr) ? pflg_part->raw_base_ptr : nullptr;
            const uint32_t pflg_stride         = pflg_part ? pflg_part->element_stride : 0;
            const int64_t pflg_cap             = (pflg_part && pflg_stride > 0)
                ? static_cast<int64_t>(pflg_part->capacity_bytes / pflg_stride)
                : 0;

            auto read_prop_flags = [&](int64_t prop_idx) noexcept -> uint16_t {
                if (!pflg_raw || prop_idx < 0 || prop_idx >= pflg_cap) return PROP_FLAG_DEFAULT_MASK;
                if (pflg_stride == sizeof(uint32_t)) {
                    return static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pflg_raw)[prop_idx]);
                }
                return reinterpret_cast<const uint16_t*>(pflg_raw)[prop_idx];
            };

            auto accumulate_delta = [&](uint32_t target_prop, int32_t delta, uint8_t channel) noexcept {
                if (delta == 0 || target_prop == INVALID_ID) return;
                const int64_t pidx = static_cast<int64_t>(target_prop);
                if (pidx < 0 || pidx >= prop_count) return;

                if (pflg_raw && (respect_allow_flags || skip_locked_properties)) {
                    const uint16_t flags = read_prop_flags(pidx);
                    if (skip_locked_properties && (flags & PROP_FLAG_LOCKED) != 0u) return;
                    if (respect_allow_flags) {
                        if (channel == MOD_CHANNEL_VALUE && (flags & PROP_FLAG_ALLOW_VALUE_MODS) == 0u) return;
                        if (channel == MOD_CHANNEL_MAX   && (flags & PROP_FLAG_ALLOW_MAX_MODS)   == 0u) return;
                        if (channel == MOD_CHANNEL_MIN   && (flags & PROP_FLAG_ALLOW_MIN_MODS)   == 0u) return;
                    }
                }

                if (channel == MOD_CHANNEL_VALUE && modded_vals) {
                    std::atomic_ref<int32_t>(modded_vals[pidx]).fetch_add(delta, std::memory_order_relaxed);
                } else if (channel == MOD_CHANNEL_MAX && modded_maxs) {
                    std::atomic_ref<int32_t>(modded_maxs[pidx]).fetch_add(delta, std::memory_order_relaxed);
                } else if (channel == MOD_CHANNEL_MIN && modded_mins) {
                    std::atomic_ref<int32_t>(modded_mins[pidx]).fetch_add(delta, std::memory_order_relaxed);
                }
            };

            // --- Phase 2A: Accumulate from Packed AoS PropertyModifierPOD Buffer ---
            if (modifier_pod_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* mpod_part = p_context.get_grant_part(modifier_pod_buffer_id)) {
                    if (mpod_part->raw_base_ptr) {
                        const PropertyModifierPOD* __restrict pods =
                            reinterpret_cast<const PropertyModifierPOD*>(mpod_part->raw_base_ptr);
                        const int64_t mpod_cap = static_cast<int64_t>(mpod_part->capacity_bytes / sizeof(PropertyModifierPOD));
                        if (mpod_cap > 0) {
                            auto accum_pod = [&](int64_t m) noexcept {
                                const PropertyModifierPOD& mod = pods[m];
                                if ((mod.flags & MOD_FLAG_ACTIVE) == 0u || (mod.flags & MOD_FLAG_EXPIRED) != 0u) return;
                                accumulate_delta(mod.target_property_idx, mod.value, mod.target_channel);
                            };
                            _iterate_selection(mpod_part->selection, mpod_cap, accum_pod);
                        }
                    }
                }
            }

            // --- Phase 2B: Accumulate from SoA Modifier Columns ---
            if (mod_target_property_buffer_id != INVALID_ID && mod_value_buffer_id != INVALID_ID) {
                const GrantPartPOD* mtgt_part = p_context.get_grant_part(mod_target_property_buffer_id);
                const GrantPartPOD* mdv_part  = p_context.get_grant_part(mod_value_buffer_id);
                const GrantPartPOD* mch_part  = (mod_channel_buffer_id != INVALID_ID) ? p_context.get_grant_part(mod_channel_buffer_id) : nullptr;
                const GrantPartPOD* mflg_part = (mod_flags_buffer_id != INVALID_ID)   ? p_context.get_grant_part(mod_flags_buffer_id)   : nullptr;

                if (mtgt_part && mtgt_part->raw_base_ptr && mdv_part && mdv_part->raw_base_ptr) {
                    const uint32_t* __restrict mod_targets = reinterpret_cast<const uint32_t*>(mtgt_part->raw_base_ptr);
                    const int32_t* __restrict mod_values   = reinterpret_cast<const int32_t*>(mdv_part->raw_base_ptr);
                    const uint8_t* __restrict mch_raw      = (mch_part && mch_part->raw_base_ptr)   ? mch_part->raw_base_ptr   : nullptr;
                    const uint8_t* __restrict mflg_raw     = (mflg_part && mflg_part->raw_base_ptr) ? mflg_part->raw_base_ptr  : nullptr;
                    const uint32_t mch_stride              = mch_part  ? mch_part->element_stride  : 0;
                    const uint32_t mflg_stride             = mflg_part ? mflg_part->element_stride : 0;

                    int64_t mod_count = std::min<int64_t>(
                        static_cast<int64_t>(mtgt_part->capacity_bytes / sizeof(uint32_t)),
                        static_cast<int64_t>(mdv_part->capacity_bytes / sizeof(int32_t))
                    );
                    if (mod_count > 0) {
                        auto accum_soa = [&](int64_t m) noexcept {
                            if (mflg_raw) {
                                const uint8_t flg = (mflg_stride == sizeof(uint32_t))
                                    ? static_cast<uint8_t>(reinterpret_cast<const uint32_t*>(mflg_raw)[m])
                                    : mflg_raw[m];
                                if ((flg & MOD_FLAG_ACTIVE) == 0u || (flg & MOD_FLAG_EXPIRED) != 0u) return;
                            }
                            uint8_t channel = default_channel;
                            if (mch_raw) {
                                channel = (mch_stride == sizeof(uint32_t))
                                    ? static_cast<uint8_t>(reinterpret_cast<const uint32_t*>(mch_raw)[m])
                                    : mch_raw[m];
                            }
                            accumulate_delta(mod_targets[m], mod_values[m], channel);
                        };
                        _iterate_selection(mtgt_part->selection, mod_count, accum_soa);
                    }
                }
            }
        }
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

static_assert(sizeof(PropertyModifierAggregateTransformLogic<>) == 64, "PropertyModifierAggregateTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core