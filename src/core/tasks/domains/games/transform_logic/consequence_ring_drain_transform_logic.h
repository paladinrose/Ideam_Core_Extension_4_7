#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/ring_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "interaction_tug_of_war_resolve_transform_logic.h"
#include "property_modifier_aggregate_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ConsequenceRingDrainTransformLogic<T> (G12)
 * Drains ConsequenceCommandPOD items from a RingView / RingStrategy buffer at frame end
 * to apply score_delta updates to target GameEntity / GameProperty columns (with optional
 * min/max clamping and exhaustion flag updates) and record applied consequence masks.
 * Replaces deferred per-frame GameEntity::action_consequences() Dictionary dispatch.
 */
template <typename T = void*>
struct alignas(64) ConsequenceRingDrainTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = RingStrategy;
    using DefaultView     = RingView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::QUEUE_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::RING;
    static constexpr DataType required_types              = DataType::CUSTOM | DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Consequence Ring Drain";
    static constexpr std::string_view ui_category  = "Boundary";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t consequence_ring_buffer_id   = INVALID_ID; // 4B: Primary ConsequenceCommandPOD ring buffer
    uint32_t ring_cursor_buffer_id        = INVALID_ID; // 4B: Optional uint32_t[2] [read_idx, write_idx] cursor buffer
    uint32_t target_score_buffer_id       = INVALID_ID; // 4B: Target GamePropertyBuffer::modded_value / base_value (int32_t)
    uint32_t target_min_buffer_id         = INVALID_ID; // 4B: Optional GamePropertyBuffer::modded_min / base_min (int32_t)
    uint32_t target_max_buffer_id         = INVALID_ID; // 4B: Optional GamePropertyBuffer::modded_max / base_max (int32_t)
    uint32_t target_flags_buffer_id       = INVALID_ID; // 4B: Optional GamePropertyBuffer::property_flags (uint16_t/uint32_t)
    uint32_t entity_consequence_buffer_id = INVALID_ID; // 4B: Optional uint32_t column recording last applied consequence_mask
    uint32_t drained_count_buffer_id      = INVALID_ID; // 4B: Optional uint32_t/int32_t counter recording commands drained
    uint32_t max_drain_per_frame          = 0u;         // 4B: Maximum commands to pop per execution (0 = drain all)

    mutable uint32_t fallback_read_index  = 0u;         // 4B: Fallback RingView read cursor when ring_cursor_buffer_id is unbound
    mutable uint32_t fallback_write_index = 0u;         // 4B: Fallback RingView write cursor when ring_cursor_buffer_id is unbound

    bool apply_score_deltas               = true;       // 1B: Add command.score_delta to target_score_buffer_id[target_entity_idx]
    bool clamp_to_bounds                  = true;       // 1B: Clamp updated score to [target_min, target_max] and update exhausted flag
    bool wake_affected_entities           = true;       // 1B: Queue affected target_entity_idx into TaskContextPOD wave_commands
    bool reset_selection_count_on_drain   = true;       // 1B: Reset selection.element_count to 0 after draining non-cursor rings
    uint8_t reserved_padding[16]          = {0};        // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary cring_prop;
        cring_prop["name"] = "consequence_ring_buffer_id";
        cring_prop["type"] = godot::Variant::INT;
        cring_prop["hint_string"] = "buffer_option";
        props.push_back(cring_prop);

        godot::Dictionary rcur_prop;
        rcur_prop["name"] = "ring_cursor_buffer_id";
        rcur_prop["type"] = godot::Variant::INT;
        rcur_prop["hint_string"] = "buffer_option";
        props.push_back(rcur_prop);

        godot::Dictionary tscr_prop;
        tscr_prop["name"] = "target_score_buffer_id";
        tscr_prop["type"] = godot::Variant::INT;
        tscr_prop["hint_string"] = "buffer_option";
        props.push_back(tscr_prop);

        godot::Dictionary tmin_prop;
        tmin_prop["name"] = "target_min_buffer_id";
        tmin_prop["type"] = godot::Variant::INT;
        tmin_prop["hint_string"] = "buffer_option";
        props.push_back(tmin_prop);

        godot::Dictionary tmax_prop;
        tmax_prop["name"] = "target_max_buffer_id";
        tmax_prop["type"] = godot::Variant::INT;
        tmax_prop["hint_string"] = "buffer_option";
        props.push_back(tmax_prop);

        godot::Dictionary tflg_prop;
        tflg_prop["name"] = "target_flags_buffer_id";
        tflg_prop["type"] = godot::Variant::INT;
        tflg_prop["hint_string"] = "buffer_option";
        props.push_back(tflg_prop);

        godot::Dictionary ecns_prop;
        ecns_prop["name"] = "entity_consequence_buffer_id";
        ecns_prop["type"] = godot::Variant::INT;
        ecns_prop["hint_string"] = "buffer_option";
        props.push_back(ecns_prop);

        godot::Dictionary dcnt_prop;
        dcnt_prop["name"] = "drained_count_buffer_id";
        dcnt_prop["type"] = godot::Variant::INT;
        dcnt_prop["hint_string"] = "buffer_option";
        props.push_back(dcnt_prop);

        godot::Dictionary maxd_prop;
        maxd_prop["name"] = "max_drain_per_frame";
        maxd_prop["type"] = godot::Variant::INT;
        maxd_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        maxd_prop["hint_string"] = "0,4096,1";
        props.push_back(maxd_prop);

        godot::Dictionary asd_prop;
        asd_prop["name"] = "apply_score_deltas";
        asd_prop["type"] = godot::Variant::BOOL;
        asd_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(asd_prop);

        godot::Dictionary clp_prop;
        clp_prop["name"] = "clamp_to_bounds";
        clp_prop["type"] = godot::Variant::BOOL;
        clp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clp_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_affected_entities";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        godot::Dictionary rst_prop;
        rst_prop["name"] = "reset_selection_count_on_drain";
        rst_prop["type"] = godot::Variant::BOOL;
        rst_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rst_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (consequence_ring_buffer_id != INVALID_ID) return consequence_ring_buffer_id;
        return target_score_buffer_id;
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline void configure_view(T_View& p_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { p_view.read_index_ptr; p_view.write_index_ptr; }) {
            if (ring_cursor_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* rcur = p_context.get_grant_part(ring_cursor_buffer_id)) {
                    if (rcur->raw_base_ptr && rcur->capacity_bytes >= 2 * sizeof(uint32_t)) {
                        uint32_t* cursors = reinterpret_cast<uint32_t*>(rcur->raw_base_ptr);
                        p_view.read_index_ptr  = &cursors[0];
                        p_view.write_index_ptr = &cursors[1];
                        return;
                    }
                }
            }
            fallback_read_index    = 0u;
            fallback_write_index   = p_part ? static_cast<uint32_t>(std::max<int64_t>(0, p_part->selection.element_count)) : 0u;
            p_view.read_index_ptr  = &fallback_read_index;
            p_view.write_index_ptr = &fallback_write_index;
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("consequence_ring_buffer_id")) {
            consequence_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["consequence_ring_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && consequence_ring_buffer_id == INVALID_ID) {
            consequence_ring_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("ring_cursor_buffer_id")) {
            ring_cursor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ring_cursor_buffer_id"]));
        }
        if (p_props.has("target_score_buffer_id")) {
            target_score_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_score_buffer_id"]));
        }
        if (p_props.has("target_min_buffer_id")) {
            target_min_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_min_buffer_id"]));
        }
        if (p_props.has("target_max_buffer_id")) {
            target_max_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_max_buffer_id"]));
        }
        if (p_props.has("target_flags_buffer_id")) {
            target_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_flags_buffer_id"]));
        }
        if (p_props.has("entity_consequence_buffer_id")) {
            entity_consequence_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["entity_consequence_buffer_id"]));
        }
        if (p_props.has("drained_count_buffer_id")) {
            drained_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["drained_count_buffer_id"]));
        }
        if (p_props.has("max_drain_per_frame")) {
            max_drain_per_frame = static_cast<uint32_t>(std::max<int64_t>(0, static_cast<int64_t>(p_props["max_drain_per_frame"])));
        }
        if (p_props.has("apply_score_deltas")) {
            apply_score_deltas = static_cast<bool>(p_props["apply_score_deltas"]);
        }
        if (p_props.has("clamp_to_bounds")) {
            clamp_to_bounds = static_cast<bool>(p_props["clamp_to_bounds"]);
        }
        if (p_props.has("wake_affected_entities")) {
            wake_affected_entities = static_cast<bool>(p_props["wake_affected_entities"]);
        }
        if (p_props.has("reset_selection_count_on_drain")) {
            reset_selection_count_on_drain = static_cast<bool>(p_props["reset_selection_count_on_drain"]);
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
        if constexpr (std::is_same_v<T, void*> || std::is_same_v<T, int32_t>) {
            const uint32_t ring_id = get_target_buffer_id();
            const GrantPartPOD* cring_part = (ring_id != INVALID_ID) ? p_context.get_grant_part(ring_id) : nullptr;
            if (!cring_part || !cring_part->raw_base_ptr) return;

            const GrantPartPOD* rcur_part = (ring_cursor_buffer_id != INVALID_ID)        ? p_context.get_grant_part(ring_cursor_buffer_id)        : nullptr;
            const GrantPartPOD* tscr_part = (target_score_buffer_id != INVALID_ID)       ? p_context.get_grant_part(target_score_buffer_id)       : nullptr;
            const GrantPartPOD* tmin_part = (target_min_buffer_id != INVALID_ID)         ? p_context.get_grant_part(target_min_buffer_id)         : nullptr;
            const GrantPartPOD* tmax_part = (target_max_buffer_id != INVALID_ID)         ? p_context.get_grant_part(target_max_buffer_id)         : nullptr;
            const GrantPartPOD* tflg_part = (target_flags_buffer_id != INVALID_ID)       ? p_context.get_grant_part(target_flags_buffer_id)       : nullptr;
            const GrantPartPOD* ecns_part = (entity_consequence_buffer_id != INVALID_ID) ? p_context.get_grant_part(entity_consequence_buffer_id) : nullptr;
            const GrantPartPOD* dcnt_part = (drained_count_buffer_id != INVALID_ID)      ? p_context.get_grant_part(drained_count_buffer_id)      : nullptr;

            const ConsequenceCommandPOD* __restrict ring_cmds =
                reinterpret_cast<const ConsequenceCommandPOD*>(cring_part->raw_base_ptr);
            const uint32_t ring_cap = static_cast<uint32_t>(cring_part->capacity_bytes / sizeof(ConsequenceCommandPOD));
            if (ring_cap == 0u) return;

            int32_t* __restrict target_scores       = (tscr_part && tscr_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(tscr_part->raw_base_ptr)       : nullptr;
            const int32_t* __restrict target_mins   = (tmin_part && tmin_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(tmin_part->raw_base_ptr) : nullptr;
            const int32_t* __restrict target_maxs   = (tmax_part && tmax_part->raw_base_ptr) ? reinterpret_cast<const int32_t*>(tmax_part->raw_base_ptr) : nullptr;
            uint32_t* __restrict entity_consequences= (ecns_part && ecns_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(ecns_part->raw_base_ptr)      : nullptr;
            uint32_t* __restrict drained_counter    = (dcnt_part && dcnt_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(dcnt_part->raw_base_ptr)      : nullptr;

            uint8_t* __restrict tflg_raw = (tflg_part && tflg_part->raw_base_ptr) ? tflg_part->raw_base_ptr : nullptr;
            const uint32_t tflg_stride   = tflg_part ? tflg_part->element_stride : 0;
            const int64_t tflg_cap       = (tflg_part && tflg_stride > 0) ? static_cast<int64_t>(tflg_part->capacity_bytes / tflg_stride) : 0;

            const int64_t score_cap = target_scores       ? static_cast<int64_t>(tscr_part->capacity_bytes / sizeof(int32_t))  : 0;
            const int64_t min_cap   = target_mins         ? static_cast<int64_t>(tmin_part->capacity_bytes / sizeof(int32_t))  : 0;
            const int64_t max_cap   = target_maxs         ? static_cast<int64_t>(tmax_part->capacity_bytes / sizeof(int32_t))  : 0;
            const int64_t ecns_cap  = entity_consequences ? static_cast<int64_t>(ecns_part->capacity_bytes / sizeof(uint32_t)) : 0;

            uint32_t* __restrict cursors = (rcur_part && rcur_part->raw_base_ptr && rcur_part->capacity_bytes >= 2 * sizeof(uint32_t))
                ? reinterpret_cast<uint32_t*>(rcur_part->raw_base_ptr)
                : nullptr;

            auto apply_command = [&](const ConsequenceCommandPOD& cmd) noexcept {
                if (cmd.target_entity_idx == INVALID_ID) return;
                const int64_t tidx = static_cast<int64_t>(cmd.target_entity_idx);

                if (apply_score_deltas && target_scores && tidx >= 0 && tidx < score_cap) {
                    int64_t updated = static_cast<int64_t>(target_scores[tidx]) + static_cast<int64_t>(cmd.score_delta);
                    int32_t lo_bound = INT32_MIN;
                    int32_t hi_bound = INT32_MAX;

                    if (clamp_to_bounds) {
                        if (target_mins && tidx < min_cap) lo_bound = target_mins[tidx];
                        if (target_maxs && tidx < max_cap) hi_bound = target_maxs[tidx];
                        if (lo_bound > hi_bound) std::swap(lo_bound, hi_bound);
                        updated = std::clamp<int64_t>(updated, lo_bound, hi_bound);
                    }
                    const int32_t final_val = static_cast<int32_t>(updated);
                    target_scores[tidx] = final_val;

                    if (tflg_raw && tidx < tflg_cap) {
                        uint16_t flags = (tflg_stride == sizeof(uint32_t))
                            ? static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(tflg_raw)[tidx])
                            : reinterpret_cast<const uint16_t*>(tflg_raw)[tidx];
                        const bool was_exhausted = (flags & PROP_FLAG_IS_EXHAUSTED) != 0u;
                        const bool now_exhausted = (target_mins && tidx < min_cap) ? (final_val <= lo_bound) : (final_val <= 0);
                        if (now_exhausted) {
                            flags |= PROP_FLAG_IS_EXHAUSTED;
                            if (!was_exhausted) flags |= PROP_FLAG_EXHAUSTED_TRANSITION;
                        } else {
                            flags &= static_cast<uint16_t>(~PROP_FLAG_IS_EXHAUSTED);
                            if (was_exhausted) flags |= PROP_FLAG_RESTORED_TRANSITION;
                        }
                        if (tflg_stride == sizeof(uint32_t)) {
                            reinterpret_cast<uint32_t*>(tflg_raw)[tidx] = static_cast<uint32_t>(flags);
                        } else {
                            reinterpret_cast<uint16_t*>(tflg_raw)[tidx] = flags;
                        }
                    }
                }

                if (entity_consequences && tidx >= 0 && tidx < ecns_cap) {
                    entity_consequences[tidx] |= cmd.consequence_mask;
                }

                if (wake_affected_entities && target_score_buffer_id != INVALID_ID) {
                    p_context.queue_selection_command(target_score_buffer_id, tidx);
                }
            };

            uint32_t drained = 0u;
            const uint32_t limit = (max_drain_per_frame > 0u) ? max_drain_per_frame : ring_cap;

            if (cursors) {
                uint32_t r_idx = cursors[0] % ring_cap;
                const uint32_t w_idx = cursors[1] % ring_cap;
                while (r_idx != w_idx && drained < limit) {
                    apply_command(ring_cmds[r_idx]);
                    r_idx = (r_idx + 1u) % ring_cap;
                    ++drained;
                }
                cursors[0] = r_idx;
            } else {
                const uint32_t elem_cnt = static_cast<uint32_t>(std::clamp<int64_t>(
                    cring_part->selection.element_count, 0, static_cast<int64_t>(ring_cap)
                ));
                const uint32_t to_drain = std::min(elem_cnt, limit);
                for (uint32_t i = 0u; i < to_drain; ++i) {
                    apply_command(ring_cmds[i]);
                    ++drained;
                }
                if (reset_selection_count_on_drain && to_drain == elem_cnt) {
                    const_cast<GrantPartPOD*>(cring_part)->selection.element_count = 0;
                }
                fallback_read_index = drained;
            }

            if (drained_counter) {
                *drained_counter = drained;
            }
        }
    }
};

static_assert(sizeof(ConsequenceRingDrainTransformLogic<>) == 64, "ConsequenceRingDrainTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core