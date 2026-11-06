#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "property_modifier_aggregate_transform_logic.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_ACTION_STATUS_DEFINED
#define IDEAM_GAMES_ACTION_STATUS_DEFINED

/**
 * DOD ActionStatus enum (mirrors ideam::godot_ext::ActionStatus in game_piece_action.h).
 */
enum GamePieceActionStatus : int32_t {
    ACTION_STATUS_AVAILABLE   = 0,
    ACTION_STATUS_UNAVAILABLE = 1,
    ACTION_STATUS_IN_USE      = 2,
    ACTION_STATUS_REFRESHING  = 3
};

/**
 * PiecePropertyBindingPOD
 * Flat 12-byte POD binding linking a GamePieceAction to a GameProperty index in GamePropertyBuffer,
 * replacing the parallel TypedArray<GameProperty>, PackedInt32Array property_use_values, and
 * PackedByteArray property_locks arrays on GamePieceAction.
 */
struct PiecePropertyBindingPOD {
    uint32_t property_idx       = INVALID_ID; // 4B: Target row in GamePropertyBuffer
    int32_t property_use_value  = 1;          // 4B: Score weight multiplier (modded_value * property_use_value)
    uint8_t lock_when_in_use    = 0u;         // 1B: Non-zero if property is locked while action is IN_USE
    uint8_t require_available   = 1u;         // 1B: Non-zero if exhausted/locked property makes action UNAVAILABLE
    uint16_t reserved           = 0u;         // 2B -> Total = 12B
};

static_assert(sizeof(PiecePropertyBindingPOD) == 12, "PiecePropertyBindingPOD must be 12 bytes!");

#endif // IDEAM_GAMES_ACTION_STATUS_DEFINED

/**
 * PieceActionScoreRefreshTransformLogic<T> (G6)
 * Sums (modded_value * property_use_value) across bound GameProperty indices for each active
 * GamePieceAction in ActionSequenceBuffer, ticks refresh_time cooldowns by delta * effective_time_scale,
 * and updates ActionStatus (AVAILABLE / UNAVAILABLE / IN_USE / REFRESHING).
 * Replaces GamePieceAction::refresh_action() and GamePieceAction::update_action(delta).
 */
template <typename T = int32_t>
struct alignas(64) PieceActionScoreRefreshTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = BridgeView<T, T, 1, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    =
        BufferLayoutType::FLAT | BufferLayoutType::SOA |
        BufferLayoutType::AOS  | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Piece Action Score Refresh";
    static constexpr std::string_view ui_category  = "Actions";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t current_score_buffer_id         = INVALID_ID; // 4B: Primary target (ActionSequenceBuffer::current_score int32_t)
    uint32_t base_score_buffer_id            = INVALID_ID; // 4B: ActionSequenceBuffer::base_score (int32_t)
    uint32_t action_status_buffer_id         = INVALID_ID; // 4B: ActionSequenceBuffer::status (int32_t GamePieceActionStatus)
    uint32_t refresh_time_buffer_id          = INVALID_ID; // 4B: ActionSequenceBuffer::refresh_time (float)
    uint32_t current_time_buffer_id          = INVALID_ID; // 4B: ActionSequenceBuffer::current_time (float)
    uint32_t time_scale_buffer_id            = INVALID_ID; // 4B: Optional EntityHierarchyBuffer::effective_time_scale (float)
    uint32_t property_modded_value_buffer_id = INVALID_ID; // 4B: Bridged GamePropertyBuffer::modded_value (int32_t)
    uint32_t property_flags_buffer_id        = INVALID_ID; // 4B: Optional GamePropertyBuffer::property_flags (uint16_t/uint32_t)
    uint32_t binding_pod_buffer_id           = INVALID_ID; // 4B: Optional PiecePropertyBindingPOD slice buffer
    uint32_t binding_offset_buffer_id        = INVALID_ID; // 4B: Per-action binding slice start offset (uint32_t)
    uint32_t binding_count_buffer_id         = INVALID_ID; // 4B: Per-action binding slice count (uint16_t/uint32_t)
    uint32_t property_use_value_buffer_id    = INVALID_ID; // 4B: Optional 1:1 property_use_value (int32_t) column

    float fixed_delta_override               = 0.0f;       // 4B: Overrides TaskContextPOD::delta when > 0.0f
    int32_t default_use_multiplier           = 1;          // 4B: Default multiplier when property_use_value_buffer_id is unbound

    bool tick_refresh_cooldown               = true;       // 1B: Advance current_time and transition REFRESHING -> AVAILABLE
    bool check_property_availability         = true;       // 1B: Mark action UNAVAILABLE if bound property is locked/exhausted
    bool apply_property_locks                = false;      // 1B: Update PROP_FLAG_LOCKED on bound properties while IN_USE
    uint8_t reserved_padding[5]              = {0};        // 5B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary cscr_prop;
        cscr_prop["name"] = "current_score_buffer_id";
        cscr_prop["type"] = godot::Variant::INT;
        cscr_prop["hint_string"] = "buffer_option";
        props.push_back(cscr_prop);

        godot::Dictionary bscr_prop;
        bscr_prop["name"] = "base_score_buffer_id";
        bscr_prop["type"] = godot::Variant::INT;
        bscr_prop["hint_string"] = "buffer_option";
        props.push_back(bscr_prop);

        godot::Dictionary stat_prop;
        stat_prop["name"] = "action_status_buffer_id";
        stat_prop["type"] = godot::Variant::INT;
        stat_prop["hint_string"] = "buffer_option";
        props.push_back(stat_prop);

        godot::Dictionary rtm_prop;
        rtm_prop["name"] = "refresh_time_buffer_id";
        rtm_prop["type"] = godot::Variant::INT;
        rtm_prop["hint_string"] = "buffer_option";
        props.push_back(rtm_prop);

        godot::Dictionary ctm_prop;
        ctm_prop["name"] = "current_time_buffer_id";
        ctm_prop["type"] = godot::Variant::INT;
        ctm_prop["hint_string"] = "buffer_option";
        props.push_back(ctm_prop);

        godot::Dictionary tsc_prop;
        tsc_prop["name"] = "time_scale_buffer_id";
        tsc_prop["type"] = godot::Variant::INT;
        tsc_prop["hint_string"] = "buffer_option";
        props.push_back(tsc_prop);

        godot::Dictionary pmod_prop;
        pmod_prop["name"] = "property_modded_value_buffer_id";
        pmod_prop["type"] = godot::Variant::INT;
        pmod_prop["hint_string"] = "buffer_option";
        props.push_back(pmod_prop);

        godot::Dictionary pflg_prop;
        pflg_prop["name"] = "property_flags_buffer_id";
        pflg_prop["type"] = godot::Variant::INT;
        pflg_prop["hint_string"] = "buffer_option";
        props.push_back(pflg_prop);

        godot::Dictionary bpod_prop;
        bpod_prop["name"] = "binding_pod_buffer_id";
        bpod_prop["type"] = godot::Variant::INT;
        bpod_prop["hint_string"] = "buffer_option";
        props.push_back(bpod_prop);

        godot::Dictionary boff_prop;
        boff_prop["name"] = "binding_offset_buffer_id";
        boff_prop["type"] = godot::Variant::INT;
        boff_prop["hint_string"] = "buffer_option";
        props.push_back(boff_prop);

        godot::Dictionary bcnt_prop;
        bcnt_prop["name"] = "binding_count_buffer_id";
        bcnt_prop["type"] = godot::Variant::INT;
        bcnt_prop["hint_string"] = "buffer_option";
        props.push_back(bcnt_prop);

        godot::Dictionary puse_prop;
        puse_prop["name"] = "property_use_value_buffer_id";
        puse_prop["type"] = godot::Variant::INT;
        puse_prop["hint_string"] = "buffer_option";
        props.push_back(puse_prop);

        godot::Dictionary fdt_prop;
        fdt_prop["name"] = "fixed_delta_override";
        fdt_prop["type"] = godot::Variant::FLOAT;
        fdt_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        fdt_prop["hint_string"] = "0.0,1.0,0.0001,suffix:s";
        props.push_back(fdt_prop);

        godot::Dictionary dmul_prop;
        dmul_prop["name"] = "default_use_multiplier";
        dmul_prop["type"] = godot::Variant::INT;
        dmul_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(dmul_prop);

        godot::Dictionary trc_prop;
        trc_prop["name"] = "tick_refresh_cooldown";
        trc_prop["type"] = godot::Variant::BOOL;
        trc_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(trc_prop);

        godot::Dictionary cpa_prop;
        cpa_prop["name"] = "check_property_availability";
        cpa_prop["type"] = godot::Variant::BOOL;
        cpa_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cpa_prop);

        godot::Dictionary apl_prop;
        apl_prop["name"] = "apply_property_locks";
        apl_prop["type"] = godot::Variant::BOOL;
        apl_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(apl_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (current_score_buffer_id != INVALID_ID) return current_score_buffer_id;
        return base_score_buffer_id;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        (void)p_part;
        if constexpr (requires { p_view.bind_secondary(p_part); }) {
            const uint32_t sec_id = (property_modded_value_buffer_id != INVALID_ID)
                ? property_modded_value_buffer_id
                : get_target_buffer_id();
            if (sec_id != INVALID_ID) {
                p_view.bind_secondary(p_context.get_grant_part(sec_id));
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("current_score_buffer_id")) {
            current_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["current_score_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && current_score_buffer_id == INVALID_ID) {
            current_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("base_score_buffer_id")) {
            base_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["base_score_buffer_id"]));
        }
        if (p_props.has("action_status_buffer_id")) {
            action_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["action_status_buffer_id"]));
        }
        if (p_props.has("refresh_time_buffer_id")) {
            refresh_time_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["refresh_time_buffer_id"]));
        }
        if (p_props.has("current_time_buffer_id")) {
            current_time_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["current_time_buffer_id"]));
        }
        if (p_props.has("time_scale_buffer_id")) {
            time_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["time_scale_buffer_id"]));
        }
        if (p_props.has("property_modded_value_buffer_id")) {
            property_modded_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_modded_value_buffer_id"]));
        }
        if (p_props.has("property_flags_buffer_id")) {
            property_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_flags_buffer_id"]));
        }
        if (p_props.has("binding_pod_buffer_id")) {
            binding_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["binding_pod_buffer_id"]));
        }
        if (p_props.has("binding_offset_buffer_id")) {
            binding_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["binding_offset_buffer_id"]));
        }
        if (p_props.has("binding_count_buffer_id")) {
            binding_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["binding_count_buffer_id"]));
        }
        if (p_props.has("property_use_value_buffer_id")) {
            property_use_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["property_use_value_buffer_id"]));
        }
        if (p_props.has("fixed_delta_override")) {
            fixed_delta_override = static_cast<float>(p_props["fixed_delta_override"]);
        }
        if (p_props.has("default_use_multiplier")) {
            default_use_multiplier = static_cast<int32_t>(static_cast<int64_t>(p_props["default_use_multiplier"]));
        }
        if (p_props.has("tick_refresh_cooldown")) {
            tick_refresh_cooldown = static_cast<bool>(p_props["tick_refresh_cooldown"]);
        }
        if (p_props.has("check_property_availability")) {
            check_property_availability = static_cast<bool>(p_props["check_property_availability"]);
        }
        if (p_props.has("apply_property_locks")) {
            apply_property_locks = static_cast<bool>(p_props["apply_property_locks"]);
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
            const uint32_t cscr_id = get_target_buffer_id();
            const GrantPartPOD* cscr_part = (cscr_id != INVALID_ID) ? p_context.get_grant_part(cscr_id) : nullptr;
            if (!cscr_part || !cscr_part->raw_base_ptr) return;

            const GrantPartPOD* bscr_part = (base_score_buffer_id != INVALID_ID)            ? p_context.get_grant_part(base_score_buffer_id)            : nullptr;
            const GrantPartPOD* stat_part = (action_status_buffer_id != INVALID_ID)         ? p_context.get_grant_part(action_status_buffer_id)         : nullptr;
            const GrantPartPOD* rtm_part  = (refresh_time_buffer_id != INVALID_ID)          ? p_context.get_grant_part(refresh_time_buffer_id)          : nullptr;
            const GrantPartPOD* ctm_part  = (current_time_buffer_id != INVALID_ID)          ? p_context.get_grant_part(current_time_buffer_id)          : nullptr;
            const GrantPartPOD* tsc_part  = (time_scale_buffer_id != INVALID_ID)            ? p_context.get_grant_part(time_scale_buffer_id)            : nullptr;
            const GrantPartPOD* pmod_part = (property_modded_value_buffer_id != INVALID_ID) ? p_context.get_grant_part(property_modded_value_buffer_id) : nullptr;
            const GrantPartPOD* pflg_part = (property_flags_buffer_id != INVALID_ID)        ? p_context.get_grant_part(property_flags_buffer_id)        : nullptr;
            const GrantPartPOD* bpod_part = (binding_pod_buffer_id != INVALID_ID)           ? p_context.get_grant_part(binding_pod_buffer_id)           : nullptr;
            const GrantPartPOD* boff_part = (binding_offset_buffer_id != INVALID_ID)        ? p_context.get_grant_part(binding_offset_buffer_id)        : nullptr;
            const GrantPartPOD* bcnt_part = (binding_count_buffer_id != INVALID_ID)         ? p_context.get_grant_part(binding_count_buffer_id)         : nullptr;
            const GrantPartPOD* puse_part = (property_use_value_buffer_id != INVALID_ID)    ? p_context.get_grant_part(property_use_value_buffer_id)    : nullptr;

            int32_t* __restrict current_scores        = reinterpret_cast<int32_t*>(cscr_part->raw_base_ptr);
            const int32_t* __restrict base_scores     = (bscr_part && bscr_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(bscr_part->raw_base_ptr) : nullptr;
            int32_t* __restrict statuses              = (stat_part && stat_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(stat_part->raw_base_ptr)       : nullptr;
            const float* __restrict refresh_times     = (rtm_part && rtm_part->raw_base_ptr)   ? reinterpret_cast<const float*>(rtm_part->raw_base_ptr)   : nullptr;
            float* __restrict current_times           = (ctm_part && ctm_part->raw_base_ptr)   ? reinterpret_cast<float*>(ctm_part->raw_base_ptr)         : nullptr;
            const float* __restrict time_scales       = (tsc_part && tsc_part->raw_base_ptr)   ? reinterpret_cast<const float*>(tsc_part->raw_base_ptr)   : nullptr;
            const int32_t* __restrict prop_modded     = (pmod_part && pmod_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(pmod_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict prop_use_vals   = (puse_part && puse_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(puse_part->raw_base_ptr) : nullptr;

            const PiecePropertyBindingPOD* __restrict bindings = (bpod_part && bpod_part->raw_base_ptr)
                ? reinterpret_cast<const PiecePropertyBindingPOD*>(bpod_part->raw_base_ptr)
                : nullptr;
            const int64_t binding_cap = bindings ? static_cast<int64_t>(bpod_part->capacity_bytes / sizeof(PiecePropertyBindingPOD)) : 0;

            const uint32_t* __restrict bind_offsets = (boff_part && boff_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(boff_part->raw_base_ptr)
                : nullptr;
            const uint8_t* __restrict bcnt_raw = (bcnt_part && bcnt_part->raw_base_ptr) ? bcnt_part->raw_base_ptr : nullptr;
            const uint32_t bcnt_stride         = bcnt_part ? bcnt_part->element_stride : 0;

            uint8_t* __restrict pflg_raw = (pflg_part && pflg_part->raw_base_ptr) ? pflg_part->raw_base_ptr : nullptr;
            const uint32_t pflg_stride   = pflg_part ? pflg_part->element_stride : 0;
            const int64_t pflg_cap       = (pflg_part && pflg_stride > 0) ? static_cast<int64_t>(pflg_part->capacity_bytes / pflg_stride) : 0;
            const int64_t prop_cap       = pmod_part ? static_cast<int64_t>(pmod_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t puse_cap       = puse_part ? static_cast<int64_t>(puse_part->capacity_bytes / sizeof(int32_t)) : 0;
            const int64_t tsc_cap        = tsc_part  ? static_cast<int64_t>(tsc_part->capacity_bytes / sizeof(float))    : 0;

            int64_t action_count = static_cast<int64_t>(cscr_part->capacity_bytes / sizeof(int32_t));
            if (bscr_part) action_count = std::min<int64_t>(action_count, static_cast<int64_t>(bscr_part->capacity_bytes / sizeof(int32_t)));
            if (stat_part) action_count = std::min<int64_t>(action_count, static_cast<int64_t>(stat_part->capacity_bytes / sizeof(int32_t)));
            if (rtm_part)  action_count = std::min<int64_t>(action_count, static_cast<int64_t>(rtm_part->capacity_bytes / sizeof(float)));
            if (ctm_part)  action_count = std::min<int64_t>(action_count, static_cast<int64_t>(ctm_part->capacity_bytes / sizeof(float)));
            if (action_count <= 0) return;

            const float base_dt = (fixed_delta_override > 0.0f) ? fixed_delta_override : static_cast<float>(p_context.delta);

            auto read_prop_flags = [&](int64_t pidx) noexcept -> uint16_t {
                if (!pflg_raw || pidx < 0 || pidx >= pflg_cap) return PROP_FLAG_DEFAULT_MASK;
                if (pflg_stride == sizeof(uint32_t)) {
                    return static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pflg_raw)[pidx]);
                }
                return reinterpret_cast<const uint16_t*>(pflg_raw)[pidx];
            };

            auto write_prop_lock = [&](int64_t pidx, bool locked) noexcept {
                if (!pflg_raw || pidx < 0 || pidx >= pflg_cap) return;
                if (pflg_stride == sizeof(uint32_t)) {
                    uint32_t& f = reinterpret_cast<uint32_t*>(pflg_raw)[pidx];
                    if (locked) f |= static_cast<uint32_t>(PROP_FLAG_LOCKED);
                    else        f &= ~static_cast<uint32_t>(PROP_FLAG_LOCKED);
                } else {
                    uint16_t& f = reinterpret_cast<uint16_t*>(pflg_raw)[pidx];
                    if (locked) f |= PROP_FLAG_LOCKED;
                    else        f &= static_cast<uint16_t>(~PROP_FLAG_LOCKED);
                }
            };

            auto eval_action = [&](int64_t idx) noexcept {
                int32_t status = statuses ? statuses[idx] : static_cast<int32_t>(ACTION_STATUS_AVAILABLE);

                // 1. Tick cooldown timer when REFRESHING (mirrors GamePieceAction::update_action)
                if (tick_refresh_cooldown && current_times && refresh_times && status == ACTION_STATUS_REFRESHING) {
                    const float ts  = (time_scales && idx < tsc_cap) ? time_scales[idx] : 1.0f;
                    const float dt  = std::max(0.0f, base_dt * ts);
                    const float lim = refresh_times[idx];
                    float cur_t     = current_times[idx] + dt;
                    if (lim <= 0.0f || cur_t >= lim) {
                        current_times[idx] = 0.0f;
                        status = ACTION_STATUS_AVAILABLE;
                    } else {
                        current_times[idx] = cur_t;
                    }
                }

                // 2. Refresh action score: base_score + sum(modded_value * property_use_value)
                int64_t score_accum = base_scores ? static_cast<int64_t>(base_scores[idx]) : static_cast<int64_t>(current_scores[idx]);
                bool any_required_unavailable = false;

                if (bindings && bind_offsets && bcnt_raw) {
                    const uint32_t slice_off = bind_offsets[idx];
                    const uint32_t slice_cnt = (bcnt_stride == sizeof(uint32_t))
                        ? reinterpret_cast<const uint32_t*>(bcnt_raw)[idx]
                        : static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(bcnt_raw)[idx]);
                    const int64_t start_b = static_cast<int64_t>(slice_off);
                    const int64_t end_b   = std::min<int64_t>(start_b + static_cast<int64_t>(slice_cnt), binding_cap);

                    for (int64_t b = start_b; b < end_b; ++b) {
                        const PiecePropertyBindingPOD& bind = bindings[b];
                        const int64_t pidx = static_cast<int64_t>(bind.property_idx);
                        if (pidx < 0 || pidx >= prop_cap) continue;

                        if (prop_modded) {
                            score_accum += static_cast<int64_t>(prop_modded[pidx]) * static_cast<int64_t>(bind.property_use_value);
                        }

                        if (pflg_raw && pidx < pflg_cap) {
                            const uint16_t pflags = read_prop_flags(pidx);
                            if (check_property_availability && bind.require_available != 0u && status != ACTION_STATUS_IN_USE) {
                                if ((pflags & (PROP_FLAG_IS_EXHAUSTED | PROP_FLAG_LOCKED)) != 0u) {
                                    any_required_unavailable = true;
                                }
                            }
                            if (apply_property_locks && bind.lock_when_in_use != 0u) {
                                write_prop_lock(pidx, status == ACTION_STATUS_IN_USE);
                            }
                        }
                    }
                } else if (prop_modded && idx < prop_cap) {
                    // Direct 1:1 action-to-property bridge fallback
                    const int32_t use_mult = (prop_use_vals && idx < puse_cap) ? prop_use_vals[idx] : default_use_multiplier;
                    score_accum += static_cast<int64_t>(prop_modded[idx]) * static_cast<int64_t>(use_mult);

                    if (pflg_raw && idx < pflg_cap) {
                        const uint16_t pflags = read_prop_flags(idx);
                        if (check_property_availability && status != ACTION_STATUS_IN_USE) {
                            if ((pflags & (PROP_FLAG_IS_EXHAUSTED | PROP_FLAG_LOCKED)) != 0u) {
                                any_required_unavailable = true;
                            }
                        }
                        if (apply_property_locks) {
                            write_prop_lock(idx, status == ACTION_STATUS_IN_USE);
                        }
                    }
                }

                current_scores[idx] = static_cast<int32_t>(std::clamp<int64_t>(score_accum, INT32_MIN, INT32_MAX));

                if (statuses) {
                    if (status != ACTION_STATUS_IN_USE && status != ACTION_STATUS_REFRESHING) {
                        status = any_required_unavailable ? ACTION_STATUS_UNAVAILABLE : ACTION_STATUS_AVAILABLE;
                    }
                    statuses[idx] = status;
                }
            };

            _iterate_selection(cscr_part->selection, action_count, eval_action);
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

static_assert(sizeof(PieceActionScoreRefreshTransformLogic<>) == 64, "PieceActionScoreRefreshTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core