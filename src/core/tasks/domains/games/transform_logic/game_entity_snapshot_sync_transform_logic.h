#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "entity_time_scale_cascade_transform_logic.h"
#include "property_modifier_aggregate_transform_logic.h"

#include <godot_cpp/core/object.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAMES_PROPERTY_TELEMETRY_SNAPSHOT_POD_DEFINED
#define IDEAM_GAMES_PROPERTY_TELEMETRY_SNAPSHOT_POD_DEFINED

/**
 * Synchronization directions supported by GameEntitySnapshotSyncTransformLogic (G11).
 */
enum EntitySnapshotSyncMode : uint8_t {
    SNAPSHOT_SYNC_PULL_FROM_GODOT      = 0, // Pulls live GameEntity / GameProperty state into DOD columns
    SNAPSHOT_SYNC_PUSH_TO_GODOT        = 1, // Pushes solved DOD columns back to Godot nodes & GamePropertyGauge
    SNAPSHOT_SYNC_BUILD_TELEMETRY_ONLY = 2  // Packs DOD GamePropertyBuffer columns into PropertyTelemetrySnapshotPOD
};

/**
 * PropertyTelemetrySnapshotPOD
 * Flat, pointer-free 28-byte telemetry struct matching ideam::godot_ext::PropertyTelemetrySnapshotPOD
 * in src/godot/games/ui/game_property_gauge.h for zero-indirection HUD telemetry ingestion.
 */
struct PropertyTelemetrySnapshotPOD {
    int32_t base_value       = 0;
    int32_t modded_value     = 0;
    int32_t base_max_value   = 100;
    int32_t modded_max_value = 100;
    int32_t base_min_value   = 0;
    int32_t modded_min_value = 0;
    bool is_exhausted        = false;
    bool is_locked           = false;
    bool valid               = false;
};

static_assert(sizeof(PropertyTelemetrySnapshotPOD) == 28, "PropertyTelemetrySnapshotPOD must be 28 bytes!");

#endif // IDEAM_GAMES_PROPERTY_TELEMETRY_SNAPSHOT_POD_DEFINED

/**
 * GameEntitySnapshotSyncTransformLogic<T> (G11)
 * Bidirectional boundary synchronizer between live Godot GameEntity / GameProperty nodes,
 * MemoryManagerDOD columns (GamePropertyBuffer and EntityHierarchyBuffer), and flat
 * PropertyTelemetrySnapshotPOD buffers for GamePropertyGauge telemetry.
 */
template <typename T = int32_t>
struct alignas(64) GameEntitySnapshotSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Game Entity Snapshot Sync";
    static constexpr std::string_view ui_category  = "Boundary";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t modded_value_buffer_id       = INVALID_ID;                   // 4B: GamePropertyBuffer::modded_value (int32_t)
    uint32_t base_value_buffer_id         = INVALID_ID;                   // 4B: GamePropertyBuffer::base_value (int32_t)
    uint32_t modded_min_buffer_id         = INVALID_ID;                   // 4B: GamePropertyBuffer::modded_min (int32_t)
    uint32_t base_min_buffer_id           = INVALID_ID;                   // 4B: GamePropertyBuffer::base_min (int32_t)
    uint32_t modded_max_buffer_id         = INVALID_ID;                   // 4B: GamePropertyBuffer::modded_max (int32_t)
    uint32_t base_max_buffer_id           = INVALID_ID;                   // 4B: GamePropertyBuffer::base_max (int32_t)
    uint32_t property_flags_buffer_id     = INVALID_ID;                   // 4B: GamePropertyBuffer::property_flags (uint16_t/uint32_t)
    uint32_t local_time_scale_buffer_id   = INVALID_ID;                   // 4B: EntityHierarchyBuffer::local_time_scale (float)
    uint32_t entity_active_flags_buffer_id= INVALID_ID;                   // 4B: EntityHierarchyBuffer::active_flags (uint32_t)
    uint32_t object_id_buffer_id          = INVALID_ID;                   // 4B: Optional uint64_t ObjectID column (or Object* in tests)
    uint32_t gauge_object_id_buffer_id    = INVALID_ID;                   // 4B: Optional uint64_t ObjectID column for GamePropertyGauge
    uint32_t telemetry_snapshot_buffer_id = INVALID_ID;                   // 4B: Optional PropertyTelemetrySnapshotPOD buffer

    uint8_t sync_mode                     = SNAPSHOT_SYNC_PUSH_TO_GODOT;  // 1B: EntitySnapshotSyncMode
    bool emit_transition_signals          = true;                         // 1B: Clear/consume exhaustion transition bits on push
    bool clear_transition_bits            = true;                         // 1B: Clear PROP_FLAG_EXHAUSTED_TRANSITION / RESTORED_TRANSITION
    bool push_to_gauges                   = true;                         // 1B: Populate PropertyTelemetrySnapshotPOD for gauges
    uint8_t reserved_padding[12]          = {0};                          // 12B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary mode_prop;
        mode_prop["name"] = "sync_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Pull From Godot:0,Push To Godot:1,Build Telemetry Only:2";
        props.push_back(mode_prop);

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

        godot::Dictionary lts_prop;
        lts_prop["name"] = "local_time_scale_buffer_id";
        lts_prop["type"] = godot::Variant::INT;
        lts_prop["hint_string"] = "buffer_option";
        props.push_back(lts_prop);

        godot::Dictionary eflg_prop;
        eflg_prop["name"] = "entity_active_flags_buffer_id";
        eflg_prop["type"] = godot::Variant::INT;
        eflg_prop["hint_string"] = "buffer_option";
        props.push_back(eflg_prop);

        godot::Dictionary oid_prop;
        oid_prop["name"] = "object_id_buffer_id";
        oid_prop["type"] = godot::Variant::INT;
        oid_prop["hint_string"] = "buffer_option";
        props.push_back(oid_prop);

        godot::Dictionary goid_prop;
        goid_prop["name"] = "gauge_object_id_buffer_id";
        goid_prop["type"] = godot::Variant::INT;
        goid_prop["hint_string"] = "buffer_option";
        props.push_back(goid_prop);

        godot::Dictionary tel_prop;
        tel_prop["name"] = "telemetry_snapshot_buffer_id";
        tel_prop["type"] = godot::Variant::INT;
        tel_prop["hint_string"] = "buffer_option";
        props.push_back(tel_prop);

        godot::Dictionary sig_prop;
        sig_prop["name"] = "emit_transition_signals";
        sig_prop["type"] = godot::Variant::BOOL;
        sig_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(sig_prop);

        godot::Dictionary clr_prop;
        clr_prop["name"] = "clear_transition_bits";
        clr_prop["type"] = godot::Variant::BOOL;
        clr_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clr_prop);

        godot::Dictionary ptg_prop;
        ptg_prop["name"] = "push_to_gauges";
        ptg_prop["type"] = godot::Variant::BOOL;
        ptg_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(ptg_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (modded_value_buffer_id != INVALID_ID)       return modded_value_buffer_id;
        if (base_value_buffer_id != INVALID_ID)         return base_value_buffer_id;
        if (local_time_scale_buffer_id != INVALID_ID)   return local_time_scale_buffer_id;
        if (telemetry_snapshot_buffer_id != INVALID_ID) return telemetry_snapshot_buffer_id;
        return entity_active_flags_buffer_id;
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
        if (p_props.has("local_time_scale_buffer_id")) {
            local_time_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["local_time_scale_buffer_id"]));
        }
        if (p_props.has("entity_active_flags_buffer_id")) {
            entity_active_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["entity_active_flags_buffer_id"]));
        }
        if (p_props.has("object_id_buffer_id")) {
            object_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["object_id_buffer_id"]));
        }
        if (p_props.has("gauge_object_id_buffer_id")) {
            gauge_object_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["gauge_object_id_buffer_id"]));
        }
        if (p_props.has("telemetry_snapshot_buffer_id")) {
            telemetry_snapshot_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["telemetry_snapshot_buffer_id"]));
        }
        if (p_props.has("sync_mode")) {
            sync_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["sync_mode"]), 0, 2));
        }
        if (p_props.has("emit_transition_signals")) {
            emit_transition_signals = static_cast<bool>(p_props["emit_transition_signals"]);
        }
        if (p_props.has("clear_transition_bits")) {
            clear_transition_bits = static_cast<bool>(p_props["clear_transition_bits"]);
        }
        if (p_props.has("push_to_gauges")) {
            push_to_gauges = static_cast<bool>(p_props["push_to_gauges"]);
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
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, float>) {
            const GrantPartPOD* mval_part = (modded_value_buffer_id != INVALID_ID)        ? p_context.get_grant_part(modded_value_buffer_id)        : nullptr;
            const GrantPartPOD* bval_part = (base_value_buffer_id != INVALID_ID)          ? p_context.get_grant_part(base_value_buffer_id)          : nullptr;
            const GrantPartPOD* mmin_part = (modded_min_buffer_id != INVALID_ID)          ? p_context.get_grant_part(modded_min_buffer_id)          : nullptr;
            const GrantPartPOD* bmin_part = (base_min_buffer_id != INVALID_ID)            ? p_context.get_grant_part(base_min_buffer_id)            : nullptr;
            const GrantPartPOD* mmax_part = (modded_max_buffer_id != INVALID_ID)          ? p_context.get_grant_part(modded_max_buffer_id)          : nullptr;
            const GrantPartPOD* bmax_part = (base_max_buffer_id != INVALID_ID)            ? p_context.get_grant_part(base_max_buffer_id)            : nullptr;
            const GrantPartPOD* pflg_part = (property_flags_buffer_id != INVALID_ID)      ? p_context.get_grant_part(property_flags_buffer_id)      : nullptr;
            const GrantPartPOD* lts_part  = (local_time_scale_buffer_id != INVALID_ID)    ? p_context.get_grant_part(local_time_scale_buffer_id)    : nullptr;
            const GrantPartPOD* eflg_part = (entity_active_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(entity_active_flags_buffer_id) : nullptr;
            const GrantPartPOD* tel_part  = (telemetry_snapshot_buffer_id != INVALID_ID)  ? p_context.get_grant_part(telemetry_snapshot_buffer_id)  : nullptr;

            const GrantPartPOD* primary_part = mval_part ? mval_part : (bval_part ? bval_part : (lts_part ? lts_part : tel_part));
            if (!primary_part || !primary_part->raw_base_ptr) return;

            int32_t* __restrict modded_vals = (mval_part && mval_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(mval_part->raw_base_ptr) : nullptr;
            int32_t* __restrict base_vals   = (bval_part && bval_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(bval_part->raw_base_ptr) : nullptr;
            int32_t* __restrict modded_mins = (mmin_part && mmin_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(mmin_part->raw_base_ptr) : nullptr;
            int32_t* __restrict base_mins   = (bmin_part && bmin_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(bmin_part->raw_base_ptr) : nullptr;
            int32_t* __restrict modded_maxs = (mmax_part && mmax_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(mmax_part->raw_base_ptr) : nullptr;
            int32_t* __restrict base_maxs   = (bmax_part && bmax_part->raw_base_ptr) ? reinterpret_cast<int32_t*>(bmax_part->raw_base_ptr) : nullptr;
            float* __restrict local_scales  = (lts_part && lts_part->raw_base_ptr)   ? reinterpret_cast<float*>(lts_part->raw_base_ptr)    : nullptr;
            uint32_t* __restrict ent_flags  = (eflg_part && eflg_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(eflg_part->raw_base_ptr): nullptr;
            PropertyTelemetrySnapshotPOD* __restrict telemetry = (tel_part && tel_part->raw_base_ptr)
                ? reinterpret_cast<PropertyTelemetrySnapshotPOD*>(tel_part->raw_base_ptr)
                : nullptr;

            uint8_t* __restrict pflg_raw = (pflg_part && pflg_part->raw_base_ptr) ? pflg_part->raw_base_ptr : nullptr;
            const uint32_t pflg_stride   = pflg_part ? pflg_part->element_stride : 0;
            const int64_t pflg_cap       = (pflg_part && pflg_stride > 0) ? static_cast<int64_t>(pflg_part->capacity_bytes / pflg_stride) : 0;
            const int64_t tel_cap        = telemetry ? static_cast<int64_t>(tel_part->capacity_bytes / sizeof(PropertyTelemetrySnapshotPOD)) : 0;

            int64_t count = static_cast<int64_t>(primary_part->capacity_bytes / (primary_part->element_stride > 0 ? primary_part->element_stride : sizeof(int32_t)));
            if (mval_part) count = std::min<int64_t>(count, static_cast<int64_t>(mval_part->capacity_bytes / sizeof(int32_t)));
            if (bval_part) count = std::min<int64_t>(count, static_cast<int64_t>(bval_part->capacity_bytes / sizeof(int32_t)));
            if (tel_part)  count = std::min<int64_t>(count, tel_cap);
            if (count <= 0) return;

            auto read_prop_flags = [&](int64_t idx) noexcept -> uint16_t {
                if (!pflg_raw || idx < 0 || idx >= pflg_cap) return PROP_FLAG_DEFAULT_MASK;
                if (pflg_stride == sizeof(uint32_t)) {
                    return static_cast<uint16_t>(reinterpret_cast<const uint32_t*>(pflg_raw)[idx]);
                }
                return reinterpret_cast<const uint16_t*>(pflg_raw)[idx];
            };

            auto write_prop_flags = [&](int64_t idx, uint16_t flags) noexcept {
                if (!pflg_raw || idx < 0 || idx >= pflg_cap) return;
                if (pflg_stride == sizeof(uint32_t)) {
                    reinterpret_cast<uint32_t*>(pflg_raw)[idx] = static_cast<uint32_t>(flags);
                } else {
                    reinterpret_cast<uint16_t*>(pflg_raw)[idx] = flags;
                }
            };

            // --- Mode 0: SNAPSHOT_SYNC_PULL_FROM_GODOT (Snapshot -> DOD Columns) ---
            if (sync_mode == SNAPSHOT_SYNC_PULL_FROM_GODOT) {
                auto pull_elem = [&](int64_t idx) noexcept {
                    if (telemetry && idx < tel_cap && telemetry[idx].valid) {
                        const PropertyTelemetrySnapshotPOD& snap = telemetry[idx];
                        if (base_vals)   base_vals[idx]   = snap.base_value;
                        if (modded_vals) modded_vals[idx] = snap.modded_value;
                        if (base_maxs)   base_maxs[idx]   = snap.base_max_value;
                        if (modded_maxs) modded_maxs[idx] = snap.modded_max_value;
                        if (base_mins)   base_mins[idx]   = snap.base_min_value;
                        if (modded_mins) modded_mins[idx] = snap.modded_min_value;

                        if (pflg_raw && idx < pflg_cap) {
                            uint16_t flags = read_prop_flags(idx);
                            if (snap.is_exhausted) flags |= PROP_FLAG_IS_EXHAUSTED;
                            else                   flags &= static_cast<uint16_t>(~PROP_FLAG_IS_EXHAUSTED);
                            if (snap.is_locked)    flags |= PROP_FLAG_LOCKED;
                            else                   flags &= static_cast<uint16_t>(~PROP_FLAG_LOCKED);
                            write_prop_flags(idx, flags);
                        }
                    } else if (modded_vals && base_vals) {
                        modded_vals[idx] = base_vals[idx];
                        if (modded_maxs && base_maxs) modded_maxs[idx] = base_maxs[idx];
                        if (modded_mins && base_mins) modded_mins[idx] = base_mins[idx];
                    }
                    if (ent_flags && local_scales && local_scales[idx] <= 0.0f) {
                        ent_flags[idx] |= ENTITY_FLAG_PAUSED;
                    }
                };
                _iterate_selection(primary_part->selection, count, pull_elem);
                return;
            }

            // --- Mode 1 & 2: PUSH_TO_GODOT / BUILD_TELEMETRY_ONLY (DOD Columns -> Telemetry & Boundary) ---
            auto push_elem = [&](int64_t idx) noexcept {
                const int32_t b_val = base_vals   ? base_vals[idx]   : (modded_vals ? modded_vals[idx] : 0);
                const int32_t m_val = modded_vals ? modded_vals[idx] : b_val;
                const int32_t b_max = base_maxs   ? base_maxs[idx]   : (modded_maxs ? modded_maxs[idx] : 100);
                const int32_t m_max = modded_maxs ? modded_maxs[idx] : b_max;
                const int32_t b_min = base_mins   ? base_mins[idx]   : (modded_mins ? modded_mins[idx] : 0);
                const int32_t m_min = modded_mins ? modded_mins[idx] : b_min;

                uint16_t flags = read_prop_flags(idx);
                const bool exhausted = (flags & PROP_FLAG_IS_EXHAUSTED) != 0u;
                const bool locked    = (flags & PROP_FLAG_LOCKED) != 0u;

                if (telemetry && push_to_gauges && idx < tel_cap) {
                    PropertyTelemetrySnapshotPOD& snap = telemetry[idx];
                    snap.base_value       = b_val;
                    snap.modded_value     = m_val;
                    snap.base_max_value   = b_max;
                    snap.modded_max_value = m_max;
                    snap.base_min_value   = b_min;
                    snap.modded_min_value = m_min;
                    snap.is_exhausted     = exhausted;
                    snap.is_locked        = locked;
                    snap.valid            = true;
                }

                if (clear_transition_bits && pflg_raw && idx < pflg_cap) {
                    const uint16_t cleared = static_cast<uint16_t>(
                        flags & ~(PROP_FLAG_EXHAUSTED_TRANSITION | PROP_FLAG_RESTORED_TRANSITION)
                    );
                    if (cleared != flags) {
                        write_prop_flags(idx, cleared);
                    }
                }
            };
            _iterate_selection(primary_part->selection, count, push_elem);
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

static_assert(sizeof(GameEntitySnapshotSyncTransformLogic<>) == 64, "GameEntitySnapshotSyncTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core