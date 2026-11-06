#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_AGENT_POSSESSION_MATCHING_DEFINED
#define IDEAM_GAMES_AGENT_POSSESSION_MATCHING_DEFINED

/**
 * DOD AgentMatching modes (mirrors ideam::godot_ext::AgentMatching in game_agent.h).
 */
enum AgentPossessionMatchMode : uint8_t {
    AGENT_MATCH_NONE            = 0, // Do not modify either transform
    AGENT_MATCH_PLAYER_TO_AGENT = 1, // Interpolate Player transform toward Agent transform
    AGENT_MATCH_AGENT_TO_PLAYER = 2, // Interpolate Agent transform toward Player transform
    AGENT_MATCH_AVERAGE         = 3  // Interpolate both Player and Agent toward their midpoint
};

/**
 * Transform channel being blended by AgentPossessionTransformBlendTransformLogic.
 */
enum AgentPossessionTransformChannel : uint8_t {
    POSSESSION_CHANNEL_POSITION = 0, // Vector3 world/local position (player_reposition_time)
    POSSESSION_CHANNEL_ROTATION = 1, // Vector4 unit quaternion (x,y,z,w) (player_reorient_time)
    POSSESSION_CHANNEL_SCALE    = 2  // Vector3 scale (player_rescale_time)
};

/**
 * Completion bitflags written into completion_flags_buffer_id (mirrors GameAgent::_check_all_matches).
 */
enum AgentPossessionCompletionFlags : uint32_t {
    POSSESSION_COMPLETE_NONE     = 0u,
    POSSESSION_COMPLETE_POSITION = 1u << 0, // _position_match_complete
    POSSESSION_COMPLETE_ROTATION = 1u << 1, // _rotation_match_complete
    POSSESSION_COMPLETE_SCALE    = 1u << 2, // _scale_match_complete
    POSSESSION_COMPLETE_ALL      =
        POSSESSION_COMPLETE_POSITION | POSSESSION_COMPLETE_ROTATION | POSSESSION_COMPLETE_SCALE
};

#endif // IDEAM_GAMES_AGENT_POSSESSION_MATCHING_DEFINED

/**
 * AgentPossessionTransformBlendTransformLogic<T> (G9)
 * Interpolates PLAYER_TO_AGENT, AGENT_TO_PLAYER, or AVERAGE position/scale (Vector3) or
 * quaternion rotation (Vector4) matching timers (player_reposition_time, player_reorient_time,
 * player_rescale_time) across possessed GameAgents, setting completion flags when alpha reaches 1.0.
 * Replaces GameAgent::_position_match(), _rotation_match(), _scale_match(), and _check_all_matches().
 */
template <typename T = godot::Vector3>
struct alignas(64) AgentPossessionTransformBlendTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR4;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Agent Possession Transform Blend";
    static constexpr std::string_view ui_category  = "Agents";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t agent_transform_buffer_id     = INVALID_ID;                      // 4B: Primary Vector3/Vector4 buffer for GameAgents
    uint32_t player_transform_buffer_id    = INVALID_ID;                      // 4B: Companion Vector3/Vector4 buffer for GamePlayers
    uint32_t agent_to_player_idx_buffer_id = INVALID_ID;                      // 4B: Optional uint32_t/int32_t mapping agent -> player index
    uint32_t match_mode_buffer_id          = INVALID_ID;                      // 4B: Optional per-agent int32_t/uint8_t AgentMatching column
    uint32_t match_elapsed_buffer_id       = INVALID_ID;                      // 4B: Optional per-agent float elapsed match timer column
    uint32_t match_duration_buffer_id      = INVALID_ID;                      // 4B: Optional per-agent float duration column (reposition_time)
    uint32_t time_scale_buffer_id          = INVALID_ID;                      // 4B: Optional effective_time_scale (float) column
    uint32_t completion_flags_buffer_id    = INVALID_ID;                      // 4B: Optional uint32_t AgentPossessionCompletionFlags column

    float default_match_duration           = 0.1f;                            // 4B: Default player_reposition/reorient/rescale_time
    float fixed_blend_alpha                = -1.0f;                           // 4B: If >= 0.0f, overrides timer progression with [0..1] alpha

    uint8_t default_match_mode             = AGENT_MATCH_PLAYER_TO_AGENT;     // 1B: AgentPossessionMatchMode fallback
    uint8_t transform_channel              = POSSESSION_CHANNEL_POSITION;     // 1B: AgentPossessionTransformChannel
    bool single_player_slot_zero           = false;                           // 1B: Map all agents to player slot 0 when index map is unbound
    uint8_t reserved_padding[21]           = {0};                             // 21B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary agt_prop;
        agt_prop["name"] = "agent_transform_buffer_id";
        agt_prop["type"] = godot::Variant::INT;
        agt_prop["hint_string"] = "buffer_option";
        props.push_back(agt_prop);

        godot::Dictionary plr_prop;
        plr_prop["name"] = "player_transform_buffer_id";
        plr_prop["type"] = godot::Variant::INT;
        plr_prop["hint_string"] = "buffer_option";
        props.push_back(plr_prop);

        godot::Dictionary map_prop;
        map_prop["name"] = "agent_to_player_idx_buffer_id";
        map_prop["type"] = godot::Variant::INT;
        map_prop["hint_string"] = "buffer_option";
        props.push_back(map_prop);

        godot::Dictionary mmd_prop;
        mmd_prop["name"] = "match_mode_buffer_id";
        mmd_prop["type"] = godot::Variant::INT;
        mmd_prop["hint_string"] = "buffer_option";
        props.push_back(mmd_prop);

        godot::Dictionary elp_prop;
        elp_prop["name"] = "match_elapsed_buffer_id";
        elp_prop["type"] = godot::Variant::INT;
        elp_prop["hint_string"] = "buffer_option";
        props.push_back(elp_prop);

        godot::Dictionary dur_prop;
        dur_prop["name"] = "match_duration_buffer_id";
        dur_prop["type"] = godot::Variant::INT;
        dur_prop["hint_string"] = "buffer_option";
        props.push_back(dur_prop);

        godot::Dictionary tsc_prop;
        tsc_prop["name"] = "time_scale_buffer_id";
        tsc_prop["type"] = godot::Variant::INT;
        tsc_prop["hint_string"] = "buffer_option";
        props.push_back(tsc_prop);

        godot::Dictionary cmp_prop;
        cmp_prop["name"] = "completion_flags_buffer_id";
        cmp_prop["type"] = godot::Variant::INT;
        cmp_prop["hint_string"] = "buffer_option";
        props.push_back(cmp_prop);

        godot::Dictionary dmd_prop;
        dmd_prop["name"] = "default_match_mode";
        dmd_prop["type"] = godot::Variant::INT;
        dmd_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dmd_prop["hint_string"] = "Match None:0,Player To Agent:1,Agent To Player:2,Average:3";
        props.push_back(dmd_prop);

        godot::Dictionary ch_prop;
        ch_prop["name"] = "transform_channel";
        ch_prop["type"] = godot::Variant::INT;
        ch_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        ch_prop["hint_string"] = "Position (Vector3):0,Rotation Quaternion (Vector4):1,Scale (Vector3):2";
        props.push_back(ch_prop);

        godot::Dictionary ddur_prop;
        ddur_prop["name"] = "default_match_duration";
        ddur_prop["type"] = godot::Variant::FLOAT;
        ddur_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        ddur_prop["hint_string"] = "0.0,10.0,0.01,suffix:s";
        props.push_back(ddur_prop);

        godot::Dictionary falp_prop;
        falp_prop["name"] = "fixed_blend_alpha";
        falp_prop["type"] = godot::Variant::FLOAT;
        falp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        falp_prop["hint_string"] = "-1.0,1.0,0.01";
        props.push_back(falp_prop);

        godot::Dictionary sps_prop;
        sps_prop["name"] = "single_player_slot_zero";
        sps_prop["type"] = godot::Variant::BOOL;
        sps_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(sps_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (agent_transform_buffer_id != INVALID_ID) return agent_transform_buffer_id;
        return player_transform_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("agent_transform_buffer_id")) {
            agent_transform_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_transform_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && agent_transform_buffer_id == INVALID_ID) {
            agent_transform_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("player_transform_buffer_id")) {
            player_transform_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["player_transform_buffer_id"]));
        }
        if (p_props.has("agent_to_player_idx_buffer_id")) {
            agent_to_player_idx_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["agent_to_player_idx_buffer_id"]));
        }
        if (p_props.has("match_mode_buffer_id")) {
            match_mode_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["match_mode_buffer_id"]));
        }
        if (p_props.has("match_elapsed_buffer_id")) {
            match_elapsed_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["match_elapsed_buffer_id"]));
        }
        if (p_props.has("match_duration_buffer_id")) {
            match_duration_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["match_duration_buffer_id"]));
        }
        if (p_props.has("time_scale_buffer_id")) {
            time_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["time_scale_buffer_id"]));
        }
        if (p_props.has("completion_flags_buffer_id")) {
            completion_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["completion_flags_buffer_id"]));
        }
        if (p_props.has("default_match_duration")) {
            default_match_duration = static_cast<float>(p_props["default_match_duration"]);
        }
        if (p_props.has("fixed_blend_alpha")) {
            fixed_blend_alpha = static_cast<float>(p_props["fixed_blend_alpha"]);
        }
        if (p_props.has("default_match_mode")) {
            default_match_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["default_match_mode"]), 0, 3));
        }
        if (p_props.has("transform_channel")) {
            transform_channel = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["transform_channel"]), 0, 2));
        }
        if (p_props.has("single_player_slot_zero")) {
            single_player_slot_zero = static_cast<bool>(p_props["single_player_slot_zero"]);
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
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, godot::Vector4>) {
            const GrantPartPOD* agt_part = (agent_transform_buffer_id != INVALID_ID)     ? p_context.get_grant_part(agent_transform_buffer_id)     : nullptr;
            const GrantPartPOD* plr_part = (player_transform_buffer_id != INVALID_ID)    ? p_context.get_grant_part(player_transform_buffer_id)    : nullptr;
            const GrantPartPOD* map_part = (agent_to_player_idx_buffer_id != INVALID_ID) ? p_context.get_grant_part(agent_to_player_idx_buffer_id) : nullptr;
            const GrantPartPOD* mmd_part = (match_mode_buffer_id != INVALID_ID)          ? p_context.get_grant_part(match_mode_buffer_id)          : nullptr;
            const GrantPartPOD* elp_part = (match_elapsed_buffer_id != INVALID_ID)       ? p_context.get_grant_part(match_elapsed_buffer_id)       : nullptr;
            const GrantPartPOD* dur_part = (match_duration_buffer_id != INVALID_ID)      ? p_context.get_grant_part(match_duration_buffer_id)      : nullptr;
            const GrantPartPOD* tsc_part = (time_scale_buffer_id != INVALID_ID)          ? p_context.get_grant_part(time_scale_buffer_id)          : nullptr;
            const GrantPartPOD* cmp_part = (completion_flags_buffer_id != INVALID_ID)    ? p_context.get_grant_part(completion_flags_buffer_id)    : nullptr;

            if (!agt_part || !agt_part->raw_base_ptr || !plr_part || !plr_part->raw_base_ptr) return;

            T* __restrict agent_vecs              = reinterpret_cast<T*>(agt_part->raw_base_ptr);
            T* __restrict player_vecs             = reinterpret_cast<T*>(plr_part->raw_base_ptr);
            const uint32_t* __restrict player_map = (map_part && map_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(map_part->raw_base_ptr) : nullptr;
            const uint8_t* __restrict mmd_raw     = (mmd_part && mmd_part->raw_base_ptr) ? mmd_part->raw_base_ptr                                    : nullptr;
            const uint32_t mmd_stride             = mmd_part ? mmd_part->element_stride : 0;
            float* __restrict elapsed_times       = (elp_part && elp_part->raw_base_ptr) ? reinterpret_cast<float*>(elp_part->raw_base_ptr)         : nullptr;
            const float* __restrict durations     = (dur_part && dur_part->raw_base_ptr) ? reinterpret_cast<const float*>(dur_part->raw_base_ptr)   : nullptr;
            const float* __restrict time_scales   = (tsc_part && tsc_part->raw_base_ptr) ? reinterpret_cast<const float*>(tsc_part->raw_base_ptr)   : nullptr;
            uint32_t* __restrict comp_flags       = (cmp_part && cmp_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(cmp_part->raw_base_ptr)      : nullptr;

            const int64_t agent_cap  = static_cast<int64_t>(agt_part->capacity_bytes / sizeof(T));
            const int64_t player_cap = static_cast<int64_t>(plr_part->capacity_bytes / sizeof(T));
            if (agent_cap <= 0 || player_cap <= 0) return;

            const int64_t map_cap = player_map    ? static_cast<int64_t>(map_part->capacity_bytes / sizeof(uint32_t)) : 0;
            const int64_t mmd_cap = (mmd_raw && mmd_stride > 0) ? static_cast<int64_t>(mmd_part->capacity_bytes / mmd_stride) : 0;
            const int64_t elp_cap = elapsed_times ? static_cast<int64_t>(elp_part->capacity_bytes / sizeof(float))    : 0;
            const int64_t dur_cap = durations     ? static_cast<int64_t>(dur_part->capacity_bytes / sizeof(float))    : 0;
            const int64_t tsc_cap = time_scales   ? static_cast<int64_t>(tsc_part->capacity_bytes / sizeof(float))    : 0;
            const int64_t cmp_cap = comp_flags    ? static_cast<int64_t>(cmp_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint32_t channel_complete_bit =
                (transform_channel == POSSESSION_CHANNEL_POSITION) ? POSSESSION_COMPLETE_POSITION :
                (transform_channel == POSSESSION_CHANNEL_ROTATION) ? POSSESSION_COMPLETE_ROTATION :
                                                                     POSSESSION_COMPLETE_SCALE;
            const float base_dt = static_cast<float>(p_context.delta);

            auto eval_agent = [&](int64_t idx) noexcept {
                int64_t p_idx = single_player_slot_zero ? 0 : idx;
                if (player_map && idx < map_cap) {
                    const uint32_t mapped = player_map[idx];
                    if (mapped == INVALID_ID) return;
                    p_idx = static_cast<int64_t>(mapped);
                }
                if (p_idx < 0 || p_idx >= player_cap) return;

                uint8_t mode = default_match_mode;
                if (mmd_raw && idx < mmd_cap) {
                    mode = (mmd_stride == sizeof(int32_t))
                        ? static_cast<uint8_t>(reinterpret_cast<const int32_t*>(mmd_raw)[idx])
                        : mmd_raw[idx];
                }

                if (mode == AGENT_MATCH_NONE) {
                    if (comp_flags && idx < cmp_cap) {
                        comp_flags[idx] |= channel_complete_bit;
                    }
                    return;
                }

                float alpha = 1.0f;
                if (fixed_blend_alpha >= 0.0f) {
                    alpha = std::clamp(fixed_blend_alpha, 0.0f, 1.0f);
                } else {
                    const float dur = (durations && idx < dur_cap) ? durations[idx] : default_match_duration;
                    const float ts  = (time_scales && idx < tsc_cap) ? time_scales[idx] : 1.0f;
                    const float dt  = std::max(0.0f, base_dt * ts);

                    if (dur <= 0.00001f) {
                        alpha = 1.0f;
                        if (elapsed_times && idx < elp_cap) elapsed_times[idx] = dur;
                    } else if (elapsed_times && idx < elp_cap) {
                        const float next_el = std::min(dur, elapsed_times[idx] + dt);
                        elapsed_times[idx]  = next_el;
                        alpha               = std::clamp(next_el / dur, 0.0f, 1.0f);
                    } else {
                        alpha = std::clamp(dt / dur, 0.0f, 1.0f);
                    }
                }

                const T a_val = agent_vecs[idx];
                const T p_val = player_vecs[p_idx];

                if (mode == AGENT_MATCH_PLAYER_TO_AGENT) {
                    player_vecs[p_idx] = _interpolate_value(p_val, a_val, alpha);
                } else if (mode == AGENT_MATCH_AGENT_TO_PLAYER) {
                    agent_vecs[idx] = _interpolate_value(a_val, p_val, alpha);
                } else if (mode == AGENT_MATCH_AVERAGE) {
                    const T mid        = _interpolate_value(p_val, a_val, 0.5f);
                    player_vecs[p_idx] = _interpolate_value(p_val, mid, alpha);
                    agent_vecs[idx]    = _interpolate_value(a_val, mid, alpha);
                }

                if (comp_flags && idx < cmp_cap) {
                    if (alpha >= 0.9999f) {
                        comp_flags[idx] |= channel_complete_bit;
                    } else {
                        comp_flags[idx] &= ~channel_complete_bit;
                    }
                }
            };

            _iterate_selection(agt_part->selection, agent_cap, eval_agent);
        }
    }

private:
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    [[nodiscard]] static inline T _interpolate_value(const T& p_from, const T& p_to, float p_alpha) noexcept {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            return godot::Vector3{
                p_from.x + (p_to.x - p_from.x) * p_alpha,
                p_from.y + (p_to.y - p_from.y) * p_alpha,
                p_from.z + (p_to.z - p_from.z) * p_alpha
            };
        } else {
            // Vector4 quaternion shortest-path SLERP / normalized LERP
            float bx = p_to.x;
            float by = p_to.y;
            float bz = p_to.z;
            float bw = p_to.w;

            float dot = p_from.x * bx + p_from.y * by + p_from.z * bz + p_from.w * bw;
            if (dot < 0.0f) {
                dot = -dot;
                bx  = -bx;
                by  = -by;
                bz  = -bz;
                bw  = -bw;
            }

            float s0 = 1.0f - p_alpha;
            float s1 = p_alpha;
            if (dot < 0.9995f) {
                const float theta     = std::acos(std::clamp(dot, -1.0f, 1.0f));
                const float sin_theta = std::sin(theta);
                if (sin_theta > 0.00001f) {
                    s0 = std::sin((1.0f - p_alpha) * theta) / sin_theta;
                    s1 = std::sin(p_alpha * theta) / sin_theta;
                }
            }

            float rx = s0 * p_from.x + s1 * bx;
            float ry = s0 * p_from.y + s1 * by;
            float rz = s0 * p_from.z + s1 * bz;
            float rw = s0 * p_from.w + s1 * bw;
            const float len_sq = rx * rx + ry * ry + rz * rz + rw * rw;
            if (len_sq > 0.000001f) {
                const float inv_len = 1.0f / std::sqrt(len_sq);
                rx *= inv_len;
                ry *= inv_len;
                rz *= inv_len;
                rw *= inv_len;
            }
            return godot::Vector4{rx, ry, rz, rw};
        }
    }

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

static_assert(sizeof(AgentPossessionTransformBlendTransformLogic<>) == 64, "AgentPossessionTransformBlendTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core