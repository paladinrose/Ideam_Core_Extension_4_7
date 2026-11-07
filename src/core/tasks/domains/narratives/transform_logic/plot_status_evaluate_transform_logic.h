#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "incident_status_resolve_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_PLOT_EVENT_POD_DEFINED
#define IDEAM_NARRATIVES_PLOT_EVENT_POD_DEFINED

/**
 * DOD PlotStatus enum (mirrors ideam::godot_ext::PlotStatus in plot.h).
 * Stored in IncidentAndPlotStateBuffer::plot_status (int8_t).
 */
enum PlotStatusDOD : int8_t {
    PLOT_STATUS_NOT_BEGUN       = 0,
    PLOT_STATUS_IN_PROGRESS     = 1,
    PLOT_STATUS_COMPLETE        = 2,
    PLOT_STATUS_CANNOT_COMPLETE = 3,
    PLOT_STATUS_FAILED          = 4
};

/**
 * DOD IncidentRequirement enum (mirrors ideam::godot_ext::IncidentRequirement in plot_event.h).
 */
enum IncidentRequirementDOD : uint8_t {
    INCIDENT_REQ_DOD_HAS_NOT_HAPPENED = 0,
    INCIDENT_REQ_DOD_HAS_HAPPENED     = 1,
    INCIDENT_REQ_DOD_CANNOT_HAPPEN    = 2,
    INCIDENT_REQ_DOD_INSTANT_SUCCESS  = 3,
    INCIDENT_REQ_DOD_INSTANT_FAILURE  = 4
};

/**
 * PlotEventPOD (8-Byte Aligned)
 * Contiguous POD entry backing Plot::plot_events in IncidentAndPlotStateBuffer (Section 3.2 Buffer 5).
 */
struct alignas(8) PlotEventPOD {
    uint32_t incident_id = INVALID_ID;                       // 4B: Target Incident ID in IncidentAndPlotStateBuffer
    uint8_t requirement  = INCIDENT_REQ_DOD_HAS_NOT_HAPPENED;// 1B: IncidentRequirementDOD
    uint8_t instant_used = 0u;                               // 1B: One-shot latch mirroring Plot_Event::_instant
    uint16_t reserved    = 0u;                               // 2B -> Total = 8B
};

static_assert(sizeof(PlotEventPOD) == 8, "PlotEventPOD must be 8 bytes!");

/**
 * Transition bitflags written to plot_transition_flags_buffer_id for N11 signal emission.
 */
enum PlotTransitionFlags : uint8_t {
    PLOT_TRANSITION_NONE            = 0u,
    PLOT_TRANSITION_BEGUN           = 1u << 0, // Emits Plot "begun" signal
    PLOT_TRANSITION_COMPLETED       = 1u << 1, // Emits Plot "completed" signal
    PLOT_TRANSITION_CANNOT_COMPLETE = 1u << 2, // Emits Plot "cannot_complete" signal
    PLOT_TRANSITION_FAILED          = 1u << 3  // Emits Plot "failed" signal
};

#endif // IDEAM_NARRATIVES_PLOT_EVENT_POD_DEFINED

/**
 * PlotStatusEvaluateTransformLogic<T> (N8)
 * Evaluates PlotEventPOD slices (INCIDENT_REQ_HAS_HAPPENED, CANNOT_HAPPEN, INSTANT_SUCCESS,
 * INSTANT_FAILURE, HAS_NOT_HAPPENED) to transition Plot status (PLOT_NOT_BEGUN, PLOT_IN_PROGRESS,
 * PLOT_COMPLETE, PLOT_CANNOT_COMPLETE, PLOT_FAILED) over BridgeView.
 *
 * Replaces Plot::evaluate_plot_status() and Plot_Event::check_requirement() in plot.cpp / plot_event.cpp.
 */
template <typename T = uint8_t>
struct alignas(64) PlotStatusEvaluateTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = BridgeView<T, T, 1, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::NONE;
    static constexpr BufferLayoutType required_layouts    =
        BufferLayoutType::FLAT | BufferLayoutType::SOA |
        BufferLayoutType::AOS  | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types              = DataType::BYTE | DataType::INT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Plot Status Evaluate";
    static constexpr std::string_view ui_category  = "Plots";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t plot_status_buffer_id           = INVALID_ID; // 4B: Primary target (IncidentAndPlotStateBuffer::plot_status)
    uint32_t event_slice_offset_buffer_id    = INVALID_ID; // 4B: IncidentAndPlotStateBuffer::event_slice_offset (uint32_t)
    uint32_t event_slice_count_buffer_id     = INVALID_ID; // 4B: IncidentAndPlotStateBuffer::event_count (uint16_t/uint32_t)
    uint32_t plot_event_pod_buffer_id        = INVALID_ID; // 4B: Contiguous PlotEventPOD (8B) array buffer
    uint32_t incident_status_buffer_id       = INVALID_ID; // 4B: Bridged IncidentAndPlotStateBuffer::incident_status (int8_t)
    uint32_t plot_transition_flags_buffer_id = INVALID_ID; // 4B: Optional uint8_t PlotTransitionFlags column
    uint32_t character_goal_wake_buffer_id   = INVALID_ID; // 4B: Optional buffer ID to wake Character goal sync (N9)
    uint32_t resolved_plot_count_buffer_id   = INVALID_ID; // 4B: Optional uint32_t counter recording newly completed/failed Plots

    bool allow_reevaluate_terminal           = false;      // 1B: Re-evaluate Plots already in COMPLETE / CANNOT_COMPLETE / FAILED
    bool empty_events_means_complete         = false;      // 1B: Transition Plots with 0 events directly to PLOT_COMPLETE
    bool wake_on_status_change               = true;       // 1B: Queue changed plot_id into TaskContextPOD::wave_commands
    bool emit_transition_flags               = true;       // 1B: Stamp PlotTransitionFlags on status transitions
    uint8_t reserved_padding[28]             = {0};        // 28B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pst_prop;
        pst_prop["name"] = "plot_status_buffer_id";
        pst_prop["type"] = godot::Variant::INT;
        pst_prop["hint_string"] = "buffer_option";
        props.push_back(pst_prop);

        godot::Dictionary eoff_prop;
        eoff_prop["name"] = "event_slice_offset_buffer_id";
        eoff_prop["type"] = godot::Variant::INT;
        eoff_prop["hint_string"] = "buffer_option";
        props.push_back(eoff_prop);

        godot::Dictionary ecnt_prop;
        ecnt_prop["name"] = "event_slice_count_buffer_id";
        ecnt_prop["type"] = godot::Variant::INT;
        ecnt_prop["hint_string"] = "buffer_option";
        props.push_back(ecnt_prop);

        godot::Dictionary epod_prop;
        epod_prop["name"] = "plot_event_pod_buffer_id";
        epod_prop["type"] = godot::Variant::INT;
        epod_prop["hint_string"] = "buffer_option";
        props.push_back(epod_prop);

        godot::Dictionary ist_prop;
        ist_prop["name"] = "incident_status_buffer_id";
        ist_prop["type"] = godot::Variant::INT;
        ist_prop["hint_string"] = "buffer_option";
        props.push_back(ist_prop);

        godot::Dictionary trn_prop;
        trn_prop["name"] = "plot_transition_flags_buffer_id";
        trn_prop["type"] = godot::Variant::INT;
        trn_prop["hint_string"] = "buffer_option";
        props.push_back(trn_prop);

        godot::Dictionary gwk_prop;
        gwk_prop["name"] = "character_goal_wake_buffer_id";
        gwk_prop["type"] = godot::Variant::INT;
        gwk_prop["hint_string"] = "buffer_option";
        props.push_back(gwk_prop);

        godot::Dictionary rcnt_prop;
        rcnt_prop["name"] = "resolved_plot_count_buffer_id";
        rcnt_prop["type"] = godot::Variant::INT;
        rcnt_prop["hint_string"] = "buffer_option";
        props.push_back(rcnt_prop);

        godot::Dictionary rev_prop;
        rev_prop["name"] = "allow_reevaluate_terminal";
        rev_prop["type"] = godot::Variant::BOOL;
        rev_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rev_prop);

        godot::Dictionary emp_prop;
        emp_prop["name"] = "empty_events_means_complete";
        emp_prop["type"] = godot::Variant::BOOL;
        emp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(emp_prop);

        godot::Dictionary wak_prop;
        wak_prop["name"] = "wake_on_status_change";
        wak_prop["type"] = godot::Variant::BOOL;
        wak_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(wak_prop);

        godot::Dictionary emt_prop;
        emt_prop["name"] = "emit_transition_flags";
        emt_prop["type"] = godot::Variant::BOOL;
        emt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(emt_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (plot_status_buffer_id != INVALID_ID) return plot_status_buffer_id;
        return incident_status_buffer_id;
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
            const uint32_t sec_id = (incident_status_buffer_id != INVALID_ID)
                ? incident_status_buffer_id
                : get_target_buffer_id();
            if (sec_id != INVALID_ID) {
                p_view.bind_secondary(p_context.get_grant_part(sec_id));
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("plot_status_buffer_id")) {
            plot_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["plot_status_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && plot_status_buffer_id == INVALID_ID) {
            plot_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("event_slice_offset_buffer_id")) {
            event_slice_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["event_slice_offset_buffer_id"]));
        }
        if (p_props.has("event_slice_count_buffer_id")) {
            event_slice_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["event_slice_count_buffer_id"]));
        }
        if (p_props.has("plot_event_pod_buffer_id")) {
            plot_event_pod_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["plot_event_pod_buffer_id"]));
        }
        if (p_props.has("incident_status_buffer_id")) {
            incident_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["incident_status_buffer_id"]));
        }
        if (p_props.has("plot_transition_flags_buffer_id")) {
            plot_transition_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["plot_transition_flags_buffer_id"]));
        }
        if (p_props.has("character_goal_wake_buffer_id")) {
            character_goal_wake_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["character_goal_wake_buffer_id"]));
        }
        if (p_props.has("resolved_plot_count_buffer_id")) {
            resolved_plot_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["resolved_plot_count_buffer_id"]));
        }
        if (p_props.has("allow_reevaluate_terminal")) {
            allow_reevaluate_terminal = static_cast<bool>(p_props["allow_reevaluate_terminal"]);
        }
        if (p_props.has("empty_events_means_complete")) {
            empty_events_means_complete = static_cast<bool>(p_props["empty_events_means_complete"]);
        }
        if (p_props.has("wake_on_status_change")) {
            wake_on_status_change = static_cast<bool>(p_props["wake_on_status_change"]);
        }
        if (p_props.has("emit_transition_flags")) {
            emit_transition_flags = static_cast<bool>(p_props["emit_transition_flags"]);
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
        if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, int32_t>) {
            const uint32_t pst_id = get_target_buffer_id();
            const GrantPartPOD* pst_part  = (pst_id != INVALID_ID) ? p_context.get_grant_part(pst_id) : nullptr;
            const GrantPartPOD* ist_part  = (incident_status_buffer_id != INVALID_ID) ? p_context.get_grant_part(incident_status_buffer_id) : nullptr;
            const GrantPartPOD* epod_part = (plot_event_pod_buffer_id != INVALID_ID)  ? p_context.get_grant_part(plot_event_pod_buffer_id)  : nullptr;
            if (!pst_part || !pst_part->raw_base_ptr || !ist_part || !ist_part->raw_base_ptr || !epod_part || !epod_part->raw_base_ptr) return;

            const GrantPartPOD* eoff_part = (event_slice_offset_buffer_id != INVALID_ID)    ? p_context.get_grant_part(event_slice_offset_buffer_id)    : nullptr;
            const GrantPartPOD* ecnt_part = (event_slice_count_buffer_id != INVALID_ID)     ? p_context.get_grant_part(event_slice_count_buffer_id)     : nullptr;
            const GrantPartPOD* trn_part  = (plot_transition_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(plot_transition_flags_buffer_id) : nullptr;
            const GrantPartPOD* rcnt_part = (resolved_plot_count_buffer_id != INVALID_ID)   ? p_context.get_grant_part(resolved_plot_count_buffer_id)   : nullptr;

            uint8_t* __restrict pst_raw       = pst_part->raw_base_ptr;
            const uint32_t pst_stride         = (pst_part->element_stride > 0) ? pst_part->element_stride : sizeof(T);
            const int64_t plot_cap            = static_cast<int64_t>(pst_part->capacity_bytes / pst_stride);

            const uint8_t* __restrict ist_raw = ist_part->raw_base_ptr;
            const uint32_t ist_stride         = (ist_part->element_stride > 0) ? ist_part->element_stride : sizeof(int8_t);
            const int64_t inc_cap             = static_cast<int64_t>(ist_part->capacity_bytes / ist_stride);

            PlotEventPOD* __restrict events   = reinterpret_cast<PlotEventPOD*>(epod_part->raw_base_ptr);
            const int64_t epod_cap            = static_cast<int64_t>(epod_part->capacity_bytes / sizeof(PlotEventPOD));
            if (plot_cap <= 0 || inc_cap <= 0 || epod_cap <= 0) return;

            const uint32_t* __restrict event_offsets = (eoff_part && eoff_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(eoff_part->raw_base_ptr) : nullptr;
            const int64_t eoff_cap                   = event_offsets ? static_cast<int64_t>(eoff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict ecnt_raw       = (ecnt_part && ecnt_part->raw_base_ptr) ? ecnt_part->raw_base_ptr : nullptr;
            const uint32_t ecnt_stride               = ecnt_part ? ecnt_part->element_stride : 0;
            const int64_t ecnt_cap                   = (ecnt_raw && ecnt_stride > 0) ? static_cast<int64_t>(ecnt_part->capacity_bytes / ecnt_stride) : 0;

            uint8_t* __restrict trn_raw              = (trn_part && trn_part->raw_base_ptr)   ? trn_part->raw_base_ptr : nullptr;
            const uint32_t trn_stride                = trn_part ? trn_part->element_stride : 0;
            const int64_t trn_cap                    = (trn_raw && trn_stride > 0) ? static_cast<int64_t>(trn_part->capacity_bytes / trn_stride) : 0;

            uint32_t* __restrict resolved_counter    = (rcnt_part && rcnt_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(rcnt_part->raw_base_ptr) : nullptr;
            uint32_t newly_resolved_plots            = 0u;

            auto read_plot_status = [&](int64_t idx) noexcept -> int8_t {
                if (pst_stride == sizeof(int32_t)) {
                    return static_cast<int8_t>(reinterpret_cast<const int32_t*>(pst_raw)[idx]);
                }
                return reinterpret_cast<const int8_t*>(pst_raw)[idx];
            };

            auto write_plot_status = [&](int64_t idx, int8_t val) noexcept {
                if (pst_stride == sizeof(int32_t)) {
                    reinterpret_cast<int32_t*>(pst_raw)[idx] = static_cast<int32_t>(val);
                } else {
                    reinterpret_cast<int8_t*>(pst_raw)[idx] = val;
                }
            };

            auto read_inc_status = [&](int64_t iidx) noexcept -> int8_t {
                if (ist_stride == sizeof(int32_t)) {
                    return static_cast<int8_t>(reinterpret_cast<const int32_t*>(ist_raw)[iidx]);
                }
                return reinterpret_cast<const int8_t*>(ist_raw)[iidx];
            };

            auto read_event_count = [&](int64_t idx) noexcept -> uint32_t {
                if (!ecnt_raw || idx < 0 || idx >= ecnt_cap) return 1u;
                if (ecnt_stride == sizeof(uint32_t)) {
                    return reinterpret_cast<const uint32_t*>(ecnt_raw)[idx];
                }
                return static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(ecnt_raw)[idx]);
            };

            const uint32_t wake_id = (character_goal_wake_buffer_id != INVALID_ID)
                ? character_goal_wake_buffer_id
                : pst_part->buffer_id;

            auto eval_plot = [&](int64_t idx) noexcept {
                const int8_t orig_status = read_plot_status(idx);
                if (!allow_reevaluate_terminal &&
                    (orig_status == PLOT_STATUS_COMPLETE ||
                     orig_status == PLOT_STATUS_CANNOT_COMPLETE ||
                     orig_status == PLOT_STATUS_FAILED)) {
                    return;
                }

                const int64_t slice_off = (event_offsets && idx < eoff_cap) ? static_cast<int64_t>(event_offsets[idx]) : idx;
                const int64_t slice_cnt = (ecnt_raw && idx < ecnt_cap)      ? static_cast<int64_t>(read_event_count(idx)) : 1;

                if (slice_cnt <= 0) {
                    if (empty_events_means_complete && orig_status != PLOT_STATUS_COMPLETE) {
                        write_plot_status(idx, PLOT_STATUS_COMPLETE);
                        ++newly_resolved_plots;
                        if (emit_transition_flags && trn_raw && idx < trn_cap) {
                            if (trn_stride == sizeof(uint32_t)) reinterpret_cast<uint32_t*>(trn_raw)[idx] |= PLOT_TRANSITION_COMPLETED;
                            else                                trn_raw[idx] |= PLOT_TRANSITION_COMPLETED;
                        }
                        if (wake_on_status_change) p_context.queue_selection_command(wake_id, idx);
                    }
                    return;
                }

                // Exact state machine mirroring Plot::evaluate_plot_status() in plot.cpp
                int8_t s            = orig_status;
                bool all_required   = true;
                bool should_break   = false;
                uint8_t tflags      = PLOT_TRANSITION_NONE;

                const int64_t end_e = std::min<int64_t>(slice_off + slice_cnt, epod_cap);
                for (int64_t e = slice_off; e < end_e; ++e) {
                    PlotEventPOD& pev = events[e];
                    if (pev.incident_id == INVALID_ID || static_cast<int64_t>(pev.incident_id) >= inc_cap) {
                        all_required = false;
                        continue;
                    }

                    const int8_t inc_st = read_inc_status(static_cast<int64_t>(pev.incident_id));
                    shouldBreak_label:
                    should_break = false;
                    (void)0;

                    switch (pev.requirement) {
                        case INCIDENT_REQ_DOD_INSTANT_FAILURE:
                            if (pev.instant_used == 0u && inc_st == INCIDENT_STATUS_CANNOT_HAPPEN) {
                                pev.instant_used = 1u;
                                s = PLOT_STATUS_FAILED;
                                tflags |= PLOT_TRANSITION_FAILED;
                                should_break = true;
                            }
                            break;

                        case INCIDENT_REQ_DOD_INSTANT_SUCCESS:
                            if (pev.instant_used == 0u && inc_st == INCIDENT_STATUS_HAS_HAPPENED) {
                                pev.instant_used = 1u;
                                s = PLOT_STATUS_COMPLETE;
                                tflags |= PLOT_TRANSITION_COMPLETED;
                                should_break = true;
                            }
                            break;

                        case INCIDENT_REQ_DOD_HAS_HAPPENED:
                            if (inc_st == INCIDENT_STATUS_HAS_HAPPENED) {
                                if (s != PLOT_STATUS_CANNOT_COMPLETE) {
                                    s = PLOT_STATUS_IN_PROGRESS;
                                }
                            } else if (inc_st == INCIDENT_STATUS_CANNOT_HAPPEN) {
                                s = PLOT_STATUS_CANNOT_COMPLETE;
                                tflags |= PLOT_TRANSITION_CANNOT_COMPLETE;
                                all_required = false;
                            } else {
                                all_required = false;
                            }
                            break;

                        case INCIDENT_REQ_DOD_CANNOT_HAPPEN:
                            if (inc_st == INCIDENT_STATUS_CANNOT_HAPPEN) {
                                if (s != PLOT_STATUS_CANNOT_COMPLETE) {
                                    s = PLOT_STATUS_IN_PROGRESS;
                                }
                            } else if (inc_st == INCIDENT_STATUS_HAS_HAPPENED) {
                                s = PLOT_STATUS_CANNOT_COMPLETE;
                                tflags |= PLOT_TRANSITION_CANNOT_COMPLETE;
                                all_required = false;
                            } else {
                                all_required = false;
                            }
                            break;

                        case INCIDENT_REQ_DOD_HAS_NOT_HAPPENED:
                        default:
                            break;
                    }

                    if (should_break) break;
                }

                if (s == PLOT_STATUS_IN_PROGRESS) {
                    if (orig_status == PLOT_STATUS_NOT_BEGUN) {
                        tflags |= PLOT_TRANSITION_BEGUN;
                    }
                    if (!should_break && all_required) {
                        s = PLOT_STATUS_COMPLETE;
                        tflags |= PLOT_TRANSITION_COMPLETED;
                    }
                }

                if (s != orig_status || tflags != PLOT_TRANSITION_NONE) {
                    write_plot_status(idx, s);
                    if (s == PLOT_STATUS_COMPLETE || s == PLOT_STATUS_CANNOT_COMPLETE || s == PLOT_STATUS_FAILED) {
                        if (s != orig_status) ++newly_resolved_plots;
                    }
                    if (emit_transition_flags && trn_raw && idx < trn_cap && tflags != PLOT_TRANSITION_NONE) {
                        if (trn_stride == sizeof(uint32_t)) reinterpret_cast<uint32_t*>(trn_raw)[idx] |= static_cast<uint32_t>(tflags);
                        else                                trn_raw[idx] |= tflags;
                    }
                    if (wake_on_status_change && s != orig_status) {
                        p_context.queue_selection_command(wake_id, idx);
                    }
                }
            };

            _iterate_selection(pst_part->selection, plot_cap, eval_plot);

            if (resolved_counter) {
                *resolved_counter = newly_resolved_plots;
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

static_assert(sizeof(PlotStatusEvaluateTransformLogic<>) == 64, "PlotStatusEvaluateTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core