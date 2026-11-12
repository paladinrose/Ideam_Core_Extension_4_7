#pragma once

/**
 * gauge_batch_telemetry_transform_logic.h
 * Ecosystem Companion Bridge (UI2): Batch Telemetry Sync for `CircularGauge` & `GamePropertyGauge`.
 *
 * Reduces and packages DOD telemetry columns (`value_1_buffer_id`, `value_2_buffer_id`,
 * `min_value_buffer_id`, `max_value_buffer_id`, `status_flags_buffer_id`, `ring_cursor_buffer_id`)
 * into `GaugeBatchTelemetryPOD` / `PropertyTelemetrySnapshotPOD` buffers and batch-updates bound
 * `CircularGauge` and `GamePropertyGauge` controls at frame end.
 */

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../../godot/controls/circular_gauge.h"
#include "../../../godot/games/ui/game_property_gauge.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_GAUGE_BATCH_TELEMETRY_POD_DEFINED
#define IDEAM_GAUGE_BATCH_TELEMETRY_POD_DEFINED

enum GaugeTelemetryStatusFlags : uint32_t {
    GAUGE_TELEMETRY_FLAG_NONE        = 0u,
    GAUGE_TELEMETRY_FLAG_WRAPPED     = 1u << 0, // CircularGauge::is_wrapped / Property exhausted
    GAUGE_TELEMETRY_FLAG_FULL        = 1u << 1, // CircularGauge::is_full / Property locked
    GAUGE_TELEMETRY_FLAG_OVERFLOWING = 1u << 2, // CircularGauge::is_overflowing
    GAUGE_TELEMETRY_FLAG_VALID       = 1u << 3
};

/**
 * GaugeBatchTelemetryPOD (32 Bytes, 16-Byte Aligned)
 * Unified telemetry snapshot consumed by both `CircularGauge` and `GamePropertyGauge`.
 */
struct alignas(16) GaugeBatchTelemetryPOD {
    float value_1         = 0.0f;   // Primary value / Read index / Modded value
    float value_2         = 0.0f;   // Secondary value / Write index / Base value
    float min_value       = 0.0f;   // Minimum dial bound
    float max_value       = 100.0f; // Maximum dial bound
    uint64_t gauge_obj_id = 0ULL;   // Optional bound CircularGauge / GamePropertyGauge ObjectID
    uint32_t status_flags = GAUGE_TELEMETRY_FLAG_VALID;
    int32_t slot_count    = 0;      // Optional CircularGauge slot subdivision count
};

static_assert(sizeof(GaugeBatchTelemetryPOD) == 32, "GaugeBatchTelemetryPOD must be 32 bytes!");

#endif // IDEAM_GAUGE_BATCH_TELEMETRY_POD_DEFINED

template <typename T = float, typename T_Strategy = FlatStrategy>
struct alignas(64) GaugeBatchTelemetryTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32 | DataType::CUSTOM;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Gauge Batch Telemetry";
    static constexpr std::string_view ui_category  = "UI/Controls";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t value_1_buffer_id           = INVALID_ID; // 4B: Primary telemetry value column (float / int32_t)
    uint32_t value_2_buffer_id           = INVALID_ID; // 4B: Secondary telemetry value column (float / int32_t)
    uint32_t min_value_buffer_id         = INVALID_ID; // 4B: Optional min_value column (float / int32_t)
    uint32_t max_value_buffer_id         = INVALID_ID; // 4B: Optional max_value column (float / int32_t)
    uint32_t status_flags_buffer_id      = INVALID_ID; // 4B: Optional uint32_t / uint16_t status flags column
    uint32_t ring_cursor_buffer_id       = INVALID_ID; // 4B: Optional uint32_t[2] [read_idx, write_idx] ring telemetry source
    uint32_t gauge_object_id_buffer_id   = INVALID_ID; // 4B: Optional uint64_t ObjectID column of target gauges
    uint32_t telemetry_output_buffer_id  = INVALID_ID; // 4B: Optional GaugeBatchTelemetryPOD (32B) output buffer
    float default_min_value              = 0.0f;       // 4B: Fallback minimum dial bound
    float default_max_value              = 100.0f;     // 4B: Fallback maximum dial bound
    float value_scale                    = 1.0f;       // 4B: Scalar multiplier applied to raw values -> 44B

    uint8_t reduction_mode               = 0u;         // 1B: 0 = Per-Row Direct, 1 = Sum/Mean/Min/Max across selection into slot 0
    bool sync_ring_cursors_to_markers    = false;      // 1B: Map ring_cursor_buffer_id[0..1] to value_1/value_2 & wrapped flag
    bool push_to_live_gauges             = true;       // 1B: Update live CircularGauge / GamePropertyGauge instances via ObjectDB
    bool clamp_to_bounds                 = true;       // 1B: Clamp value_1 and value_2 to [min_value, max_value] -> 48B
    uint8_t reserved_padding[16]         = {0};        // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"]        = p_name;
            d["type"]        = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("value_1_buffer_id");
        add_buf("value_2_buffer_id");
        add_buf("min_value_buffer_id");
        add_buf("max_value_buffer_id");
        add_buf("status_flags_buffer_id");
        add_buf("ring_cursor_buffer_id");
        add_buf("gauge_object_id_buffer_id");
        add_buf("telemetry_output_buffer_id");

        godot::Dictionary red_prop;
        red_prop["name"]        = "reduction_mode";
        red_prop["type"]        = godot::Variant::INT;
        red_prop["hint"]        = godot::PROPERTY_HINT_ENUM;
        red_prop["hint_string"] = "Per-Element Direct:0,Reduce Mean & Max To Slot 0:1,Reduce Sum To Slot 0:2";
        props.push_back(red_prop);

        godot::Dictionary dmin_prop;
        dmin_prop["name"] = "default_min_value";
        dmin_prop["type"] = godot::Variant::FLOAT;
        dmin_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(dmin_prop);

        godot::Dictionary dmax_prop;
        dmax_prop["name"] = "default_max_value";
        dmax_prop["type"] = godot::Variant::FLOAT;
        dmax_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(dmax_prop);

        godot::Dictionary scl_prop;
        scl_prop["name"] = "value_scale";
        scl_prop["type"] = godot::Variant::FLOAT;
        scl_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(scl_prop);

        godot::Dictionary rng_prop;
        rng_prop["name"] = "sync_ring_cursors_to_markers";
        rng_prop["type"] = godot::Variant::BOOL;
        rng_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rng_prop);

        godot::Dictionary psh_prop;
        psh_prop["name"] = "push_to_live_gauges";
        psh_prop["type"] = godot::Variant::BOOL;
        psh_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(psh_prop);

        godot::Dictionary clp_prop;
        clp_prop["name"] = "clamp_to_bounds";
        clp_prop["type"] = godot::Variant::BOOL;
        clp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(clp_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (value_1_buffer_id != INVALID_ID)          return value_1_buffer_id;
        if (telemetry_output_buffer_id != INVALID_ID) return telemetry_output_buffer_id;
        if (value_2_buffer_id != INVALID_ID)          return value_2_buffer_id;
        return ring_cursor_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("value_1_buffer_id")) {
            value_1_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["value_1_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && value_1_buffer_id == INVALID_ID) {
            value_1_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("value_2_buffer_id")) {
            value_2_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["value_2_buffer_id"]));
        }
        if (p_props.has("min_value_buffer_id")) {
            min_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["min_value_buffer_id"]));
        }
        if (p_props.has("max_value_buffer_id")) {
            max_value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_value_buffer_id"]));
        }
        if (p_props.has("status_flags_buffer_id")) {
            status_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["status_flags_buffer_id"]));
        }
        if (p_props.has("ring_cursor_buffer_id")) {
            ring_cursor_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ring_cursor_buffer_id"]));
        }
        if (p_props.has("gauge_object_id_buffer_id")) {
            gauge_object_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["gauge_object_id_buffer_id"]));
        }
        if (p_props.has("telemetry_output_buffer_id")) {
            telemetry_output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["telemetry_output_buffer_id"]));
        }
        if (p_props.has("default_min_value")) {
            default_min_value = static_cast<float>(p_props["default_min_value"]);
        }
        if (p_props.has("default_max_value")) {
            default_max_value = static_cast<float>(p_props["default_max_value"]);
        }
        if (p_props.has("value_scale")) {
            value_scale = static_cast<float>(p_props["value_scale"]);
        }
        if (p_props.has("reduction_mode")) {
            reduction_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["reduction_mode"]), 0, 2));
        }
        if (p_props.has("sync_ring_cursors_to_markers")) {
            sync_ring_cursors_to_markers = static_cast<bool>(p_props["sync_ring_cursors_to_markers"]);
        }
        if (p_props.has("push_to_live_gauges")) {
            push_to_live_gauges = static_cast<bool>(p_props["push_to_live_gauges"]);
        }
        if (p_props.has("clamp_to_bounds")) {
            clamp_to_bounds = static_cast<bool>(p_props["clamp_to_bounds"]);
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (!std::is_same_v<T, bool>) {
            const GrantPartPOD* v1_part   = (value_1_buffer_id != INVALID_ID)          ? p_context.get_grant_part(value_1_buffer_id)          : nullptr;
            const GrantPartPOD* v2_part   = (value_2_buffer_id != INVALID_ID)          ? p_context.get_grant_part(value_2_buffer_id)          : nullptr;
            const GrantPartPOD* min_part  = (min_value_buffer_id != INVALID_ID)        ? p_context.get_grant_part(min_value_buffer_id)        : nullptr;
            const GrantPartPOD* max_part  = (max_value_buffer_id != INVALID_ID)        ? p_context.get_grant_part(max_value_buffer_id)        : nullptr;
            const GrantPartPOD* flg_part  = (status_flags_buffer_id != INVALID_ID)     ? p_context.get_grant_part(status_flags_buffer_id)     : nullptr;
            const GrantPartPOD* rcur_part = (ring_cursor_buffer_id != INVALID_ID)      ? p_context.get_grant_part(ring_cursor_buffer_id)      : nullptr;
            const GrantPartPOD* goid_part = (gauge_object_id_buffer_id != INVALID_ID)  ? p_context.get_grant_part(gauge_object_id_buffer_id)  : nullptr;
            const GrantPartPOD* out_part  = (telemetry_output_buffer_id != INVALID_ID) ? p_context.get_grant_part(telemetry_output_buffer_id) : nullptr;

            const GrantPartPOD* primary = v1_part ? v1_part : (out_part ? out_part : (v2_part ? v2_part : rcur_part));
            if (!primary || !primary->raw_base_ptr) return;

            auto read_scalar = [](const GrantPartPOD* part, int64_t idx, float fallback) noexcept -> float {
                if (!part || !part->raw_base_ptr || idx < 0) return fallback;
                const uint32_t stride = (part->element_stride > 0u) ? part->element_stride : static_cast<uint32_t>(sizeof(float));
                const int64_t cap     = static_cast<int64_t>(part->capacity_bytes / stride);
                if (idx >= cap) return fallback;
                if (stride == sizeof(int32_t)) {
                    // Distinguish float vs int32_t based on T when matching primary buffer
                    if constexpr (std::is_same_v<T, int32_t>) {
                        return static_cast<float>(reinterpret_cast<const int32_t*>(part->raw_base_ptr)[idx]);
                    } else {
                        return reinterpret_cast<const float*>(part->raw_base_ptr)[idx];
                    }
                }
                if (stride == sizeof(double)) {
                    return static_cast<float>(reinterpret_cast<const double*>(part->raw_base_ptr)[idx]);
                }
                return static_cast<float>(part->raw_base_ptr[idx * stride]);
            };

            auto read_flags = [&](int64_t idx) noexcept -> uint32_t {
                if (!flg_part || !flg_part->raw_base_ptr || idx < 0) return GAUGE_TELEMETRY_FLAG_VALID;
                const uint32_t stride = (flg_part->element_stride > 0u) ? flg_part->element_stride : static_cast<uint32_t>(sizeof(uint32_t));
                const int64_t cap     = static_cast<int64_t>(flg_part->capacity_bytes / stride);
                if (idx >= cap) return GAUGE_TELEMETRY_FLAG_VALID;
                if (stride >= sizeof(uint32_t)) {
                    return reinterpret_cast<const uint32_t*>(flg_part->raw_base_ptr)[idx] | GAUGE_TELEMETRY_FLAG_VALID;
                }
                if (stride == sizeof(uint16_t)) {
                    return static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(flg_part->raw_base_ptr)[idx]) | GAUGE_TELEMETRY_FLAG_VALID;
                }
                return static_cast<uint32_t>(flg_part->raw_base_ptr[idx]) | GAUGE_TELEMETRY_FLAG_VALID;
            };

            const uint64_t* __restrict gauge_ids = (goid_part && goid_part->raw_base_ptr)
                ? reinterpret_cast<const uint64_t*>(goid_part->raw_base_ptr)
                : nullptr;
            const int64_t goid_cap = gauge_ids ? static_cast<int64_t>(goid_part->capacity_bytes / sizeof(uint64_t)) : 0;

            GaugeBatchTelemetryPOD* __restrict out_pods = (out_part && out_part->raw_base_ptr)
                ? reinterpret_cast<GaugeBatchTelemetryPOD*>(out_part->raw_base_ptr)
                : nullptr;
            const int64_t out_cap = out_pods ? static_cast<int64_t>(out_part->capacity_bytes / sizeof(GaugeBatchTelemetryPOD)) : 0;

            auto push_to_control = [&](uint64_t obj_id, const GaugeBatchTelemetryPOD& pod) noexcept {
                if (!push_to_live_gauges || obj_id == 0ULL) return;
                godot::Object* obj = godot::ObjectDB::get_instance(godot::ObjectID(obj_id));
                if (!obj) return;

                if (auto* circ = godot::Object::cast_to<godot_ext::CircularGauge>(obj)) {
                    circ->set_min_value(pod.min_value);
                    circ->set_max_value(pod.max_value);
                    circ->set_value_1(pod.value_1);
                    circ->set_value_2(pod.value_2);
                    circ->set_is_wrapped((pod.status_flags & GAUGE_TELEMETRY_FLAG_WRAPPED) != 0u);
                    circ->set_is_full((pod.status_flags & GAUGE_TELEMETRY_FLAG_FULL) != 0u);
                    circ->set_is_overflowing((pod.status_flags & GAUGE_TELEMETRY_FLAG_OVERFLOWING) != 0u);
                    if (pod.slot_count > 0) {
                        circ->set_slot_count(pod.slot_count);
                    }
                } else if (auto* prop_gauge = godot::Object::cast_to<godot_ext::GamePropertyGauge>(obj)) {
                    godot::Dictionary snap;
                    snap["modded_value"]     = static_cast<int32_t>(std::lround(pod.value_1));
                    snap["base_value"]       = static_cast<int32_t>(std::lround(pod.value_2));
                    snap["modded_min_value"] = static_cast<int32_t>(std::lround(pod.min_value));
                    snap["base_min_value"]   = static_cast<int32_t>(std::lround(pod.min_value));
                    snap["modded_max_value"] = static_cast<int32_t>(std::lround(pod.max_value));
                    snap["base_max_value"]   = static_cast<int32_t>(std::lround(pod.max_value));
                    snap["is_exhausted"]     = (pod.status_flags & GAUGE_TELEMETRY_FLAG_WRAPPED) != 0u || (pod.value_1 <= pod.min_value);
                    snap["is_locked"]        = (pod.status_flags & GAUGE_TELEMETRY_FLAG_FULL) != 0u;
                    prop_gauge->apply_property_snapshot(snap);
                }
            };

            const uint32_t p_stride = (primary->element_stride > 0u) ? primary->element_stride : static_cast<uint32_t>(sizeof(T));
            int64_t count = static_cast<int64_t>(primary->capacity_bytes / p_stride);
            if (out_pods && out_part != primary) count = std::min<int64_t>(count, out_cap);
            if (count <= 0) return;

            // Optional Ring Buffer Cursor Telemetry Mode
            if (sync_ring_cursors_to_markers && rcur_part && rcur_part->raw_base_ptr &&
                rcur_part->capacity_bytes >= 2 * sizeof(uint32_t)) {
                const uint32_t* cursors = reinterpret_cast<const uint32_t*>(rcur_part->raw_base_ptr);
                const float r_idx = static_cast<float>(cursors[0]);
                const float w_idx = static_cast<float>(cursors[1]);
                const float cap_v = std::max(1.0f, default_max_value);

                GaugeBatchTelemetryPOD pod{};
                pod.min_value    = 0.0f;
                pod.max_value    = cap_v;
                pod.value_1      = std::fmod(r_idx, cap_v);
                pod.value_2      = std::fmod(w_idx, cap_v);
                pod.slot_count   = static_cast<int32_t>(cap_v);
                pod.status_flags = GAUGE_TELEMETRY_FLAG_VALID;
                if (pod.value_2 < pod.value_1) {
                    pod.status_flags |= GAUGE_TELEMETRY_FLAG_WRAPPED;
                }
                if (((static_cast<uint32_t>(w_idx) + 1u) % static_cast<uint32_t>(cap_v)) == (static_cast<uint32_t>(r_idx) % static_cast<uint32_t>(cap_v))) {
                    pod.status_flags |= GAUGE_TELEMETRY_FLAG_FULL;
                }
                if (gauge_ids && goid_cap > 0) {
                    pod.gauge_obj_id = gauge_ids[0];
                } else if (out_pods && out_cap > 0) {
                    pod.gauge_obj_id = out_pods[0].gauge_obj_id;
                }
                if (out_pods && out_cap > 0) {
                    out_pods[0] = pod;
                }
                push_to_control(pod.gauge_obj_id, pod);
                return;
            }

            // Reduction Mode (1 = Mean & Max, 2 = Sum) across selection into Slot 0
            if (reduction_mode == 1u || reduction_mode == 2u) {
                double sum_v1 = 0.0;
                float max_v1  = -std::numeric_limits<float>::infinity();
                int64_t active_n = 0;

                auto accum = [&](int64_t i) noexcept {
                    const float v = read_scalar(v1_part ? v1_part : primary, i, 0.0f) * value_scale;
                    sum_v1 += static_cast<double>(v);
                    if (v > max_v1) max_v1 = v;
                    ++active_n;
                };

                const MemoryBufferSelectionPOD& sel = primary->selection;
                if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                    const int64_t cap = std::min<int64_t>(sel.capacity, count);
                    for (int64_t i = 0; i < cap; ++i) {
                        if (sel.data.bitset[i >> 6] & (1ULL << (i & 63))) accum(i);
                    }
                } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                    for (int64_t k = 0; k < sel.element_count; ++k) {
                        const int64_t idx = sel.data.indices[k];
                        if (idx >= 0 && idx < count) accum(idx);
                    }
                } else {
                    const int64_t start = std::max<int64_t>(0, sel.start_index);
                    const int64_t end   = std::min<int64_t>(start + (sel.element_count > 0 ? sel.element_count : count), count);
                    for (int64_t i = start; i < end; ++i) accum(i);
                }

                if (active_n > 0) {
                    GaugeBatchTelemetryPOD pod{};
                    pod.min_value = read_scalar(min_part, 0, default_min_value);
                    pod.max_value = read_scalar(max_part, 0, default_max_value);
                    if (pod.min_value > pod.max_value) std::swap(pod.min_value, pod.max_value);

                    pod.value_1 = (reduction_mode == 1u)
                        ? static_cast<float>(sum_v1 / static_cast<double>(active_n))
                        : static_cast<float>(sum_v1);
                    pod.value_2 = (reduction_mode == 1u) ? max_v1 : static_cast<float>(active_n);
                    if (clamp_to_bounds) {
                        pod.value_1 = std::clamp(pod.value_1, pod.min_value, pod.max_value);
                        pod.value_2 = std::clamp(pod.value_2, pod.min_value, pod.max_value);
                    }
                    pod.status_flags = read_flags(0);
                    if (gauge_ids && goid_cap > 0) {
                        pod.gauge_obj_id = gauge_ids[0];
                    } else if (out_pods && out_cap > 0) {
                        pod.gauge_obj_id = out_pods[0].gauge_obj_id;
                    }
                    if (out_pods && out_cap > 0) {
                        out_pods[0] = pod;
                    }
                    push_to_control(pod.gauge_obj_id, pod);
                }
                return;
            }

            // Per-Element Direct Batch Sync
            auto eval_row = [&](int64_t idx) noexcept {
                GaugeBatchTelemetryPOD pod = (out_pods && idx < out_cap) ? out_pods[idx] : GaugeBatchTelemetryPOD{};
                pod.min_value = read_scalar(min_part, idx, default_min_value);
                pod.max_value = read_scalar(max_part, idx, default_max_value);
                if (pod.min_value > pod.max_value) std::swap(pod.min_value, pod.max_value);

                const float v1 = read_scalar(v1_part, idx, pod.value_1) * value_scale;
                const float v2 = v2_part ? (read_scalar(v2_part, idx, pod.value_2) * value_scale) : v1;

                pod.value_1 = clamp_to_bounds ? std::clamp(v1, pod.min_value, pod.max_value) : v1;
                pod.value_2 = clamp_to_bounds ? std::clamp(v2, pod.min_value, pod.max_value) : v2;
                pod.status_flags = read_flags(idx);
                if (v1 > pod.max_value) {
                    pod.status_flags |= GAUGE_TELEMETRY_FLAG_OVERFLOWING;
                }
                if (gauge_ids && idx < goid_cap) {
                    pod.gauge_obj_id = gauge_ids[idx];
                }
                if (out_pods && idx < out_cap) {
                    out_pods[idx] = pod;
                }
                push_to_control(pod.gauge_obj_id, pod);
            };

            const MemoryBufferSelectionPOD& sel = primary->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const int64_t cap = std::min<int64_t>(sel.capacity, count);
                for (int64_t i = 0; i < cap; ++i) {
                    if (sel.data.bitset[i >> 6] & (1ULL << (i & 63))) eval_row(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t k = 0; k < sel.element_count; ++k) {
                    const int64_t idx = sel.data.indices[k];
                    if (idx >= 0 && idx < count) eval_row(idx);
                }
            } else {
                const int64_t start = std::max<int64_t>(0, sel.start_index);
                const int64_t end   = std::min<int64_t>(start + (sel.element_count > 0 ? sel.element_count : count), count);
                for (int64_t i = start; i < end; ++i) eval_row(i);
            }
        }
    }
};

static_assert(sizeof(GaugeBatchTelemetryTransformLogic<>) == 64, "GaugeBatchTelemetryTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<GaugeBatchTelemetryTransformLogic<>>, "GaugeBatchTelemetryTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
