#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/bridge_view.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"
#include "symbolic_reference_resolve_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_INCIDENT_STATUS_DEFINED
#define IDEAM_NARRATIVES_INCIDENT_STATUS_DEFINED

/**
 * DOD IncidentStatus enum (mirrors ideam::godot_ext::IncidentStatus in incident.h).
 * Stored in IncidentAndPlotStateBuffer::incident_status (int8_t).
 */
enum IncidentStatusDOD : int8_t {
    INCIDENT_STATUS_HAS_NOT_HAPPENED = 0,
    INCIDENT_STATUS_HAS_HAPPENED     = 1,
    INCIDENT_STATUS_CANNOT_HAPPEN    = 2
};

/**
 * Transition bitflags written to incident_transition_flags_buffer_id for N11 signal emission.
 */
enum IncidentTransitionFlags : uint8_t {
    INCIDENT_TRANSITION_NONE          = 0u,
    INCIDENT_TRANSITION_HAPPENED      = 1u << 0, // Emits Incident "happened" signal
    INCIDENT_TRANSITION_CANNOT_HAPPEN = 1u << 1  // Emits Incident "cannot_happen" signal
};

#endif // IDEAM_NARRATIVES_INCIDENT_STATUS_DEFINED

/**
 * IncidentStatusResolveTransformLogic<T> (N7)
 * Reduces each Incident's child Incident_Condition statuses over BridgeView slices:
 *   - Transitions to CANNOT_HAPPEN (2) if any child condition is CANNOT_MEET (-2).
 *   - Remains HAS_NOT_HAPPENED (0) if any child condition is NOT_MET (-1) or UNKNOWN (0).
 *   - Transitions to HAS_HAPPENED (1) when all required conditions are MET (1) or PERMANENT (2).
 *
 * Replaces Incident::evaluate_incident() virtual condition loops in incident.cpp.
 */
template <typename T = uint8_t>
struct alignas(64) IncidentStatusResolveTransformLogic {
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

    static constexpr std::string_view display_name = "Incident Status Resolve";
    static constexpr std::string_view ui_category  = "Events";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t incident_status_buffer_id           = INVALID_ID; // 4B: Primary target (IncidentAndPlotStateBuffer::incident_status)
    uint32_t cond_slice_offset_buffer_id         = INVALID_ID; // 4B: IncidentAndPlotStateBuffer::cond_slice_offset (uint32_t)
    uint32_t cond_slice_count_buffer_id          = INVALID_ID; // 4B: IncidentAndPlotStateBuffer::cond_count (uint16_t/uint32_t)
    uint32_t incident_cond_id_buffer_id          = INVALID_ID; // 4B: Optional indirection array mapping slice slot -> condition_id
    uint32_t condition_status_buffer_id          = INVALID_ID; // 4B: Bridged ConditionEvaluationTableBuffer::current_status (int8_t)
    uint32_t incident_transition_flags_buffer_id = INVALID_ID; // 4B: Optional uint8_t IncidentTransitionFlags column
    uint32_t dependent_plot_wake_buffer_id       = INVALID_ID; // 4B: Optional target buffer ID to wake when an Incident transitions
    uint32_t resolved_count_buffer_id            = INVALID_ID; // 4B: Optional uint32_t counter recording newly resolved Incidents

    bool treat_unknown_as_not_met                = true;       // 1B: Treat UNKNOWN (0) condition status as NOT_MET (-1)
    bool empty_conditions_means_happened         = true;       // 1B: Match Incident::evaluate_incident() when cond_count == 0
    bool allow_reevaluate_terminal               = false;      // 1B: Re-evaluate Incidents already in HAS_HAPPENED / CANNOT_HAPPEN
    bool wake_on_status_change                   = true;       // 1B: Queue changed incident_id into TaskContextPOD::wave_commands
    bool emit_transition_flags                   = true;       // 1B: Stamp INCIDENT_TRANSITION_HAPPENED / CANNOT_HAPPEN bits
    uint8_t reserved_padding[27]                 = {0};        // 27B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary ist_prop;
        ist_prop["name"] = "incident_status_buffer_id";
        ist_prop["type"] = godot::Variant::INT;
        ist_prop["hint_string"] = "buffer_option";
        props.push_back(ist_prop);

        godot::Dictionary coff_prop;
        coff_prop["name"] = "cond_slice_offset_buffer_id";
        coff_prop["type"] = godot::Variant::INT;
        coff_prop["hint_string"] = "buffer_option";
        props.push_back(coff_prop);

        godot::Dictionary ccnt_prop;
        ccnt_prop["name"] = "cond_slice_count_buffer_id";
        ccnt_prop["type"] = godot::Variant::INT;
        ccnt_prop["hint_string"] = "buffer_option";
        props.push_back(ccnt_prop);

        godot::Dictionary cid_prop;
        cid_prop["name"] = "incident_cond_id_buffer_id";
        cid_prop["type"] = godot::Variant::INT;
        cid_prop["hint_string"] = "buffer_option";
        props.push_back(cid_prop);

        godot::Dictionary cst_prop;
        cst_prop["name"] = "condition_status_buffer_id";
        cst_prop["type"] = godot::Variant::INT;
        cst_prop["hint_string"] = "buffer_option";
        props.push_back(cst_prop);

        godot::Dictionary trn_prop;
        trn_prop["name"] = "incident_transition_flags_buffer_id";
        trn_prop["type"] = godot::Variant::INT;
        trn_prop["hint_string"] = "buffer_option";
        props.push_back(trn_prop);

        godot::Dictionary pwk_prop;
        pwk_prop["name"] = "dependent_plot_wake_buffer_id";
        pwk_prop["type"] = godot::Variant::INT;
        pwk_prop["hint_string"] = "buffer_option";
        props.push_back(pwk_prop);

        godot::Dictionary rcnt_prop;
        rcnt_prop["name"] = "resolved_count_buffer_id";
        rcnt_prop["type"] = godot::Variant::INT;
        rcnt_prop["hint_string"] = "buffer_option";
        props.push_back(rcnt_prop);

        godot::Dictionary unk_prop;
        unk_prop["name"] = "treat_unknown_as_not_met";
        unk_prop["type"] = godot::Variant::BOOL;
        unk_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(unk_prop);

        godot::Dictionary emp_prop;
        emp_prop["name"] = "empty_conditions_means_happened";
        emp_prop["type"] = godot::Variant::BOOL;
        emp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(emp_prop);

        godot::Dictionary rev_prop;
        rev_prop["name"] = "allow_reevaluate_terminal";
        rev_prop["type"] = godot::Variant::BOOL;
        rev_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rev_prop);

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
        if (incident_status_buffer_id != INVALID_ID) return incident_status_buffer_id;
        return condition_status_buffer_id;
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
            const uint32_t sec_id = (condition_status_buffer_id != INVALID_ID)
                ? condition_status_buffer_id
                : get_target_buffer_id();
            if (sec_id != INVALID_ID) {
                p_view.bind_secondary(p_context.get_grant_part(sec_id));
            }
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("incident_status_buffer_id")) {
            incident_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["incident_status_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && incident_status_buffer_id == INVALID_ID) {
            incident_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("cond_slice_offset_buffer_id")) {
            cond_slice_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["cond_slice_offset_buffer_id"]));
        }
        if (p_props.has("cond_slice_count_buffer_id")) {
            cond_slice_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["cond_slice_count_buffer_id"]));
        }
        if (p_props.has("incident_cond_id_buffer_id")) {
            incident_cond_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["incident_cond_id_buffer_id"]));
        }
        if (p_props.has("condition_status_buffer_id")) {
            condition_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_status_buffer_id"]));
        }
        if (p_props.has("incident_transition_flags_buffer_id")) {
            incident_transition_flags_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["incident_transition_flags_buffer_id"]));
        }
        if (p_props.has("dependent_plot_wake_buffer_id")) {
            dependent_plot_wake_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["dependent_plot_wake_buffer_id"]));
        }
        if (p_props.has("resolved_count_buffer_id")) {
            resolved_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["resolved_count_buffer_id"]));
        }
        if (p_props.has("treat_unknown_as_not_met")) {
            treat_unknown_as_not_met = static_cast<bool>(p_props["treat_unknown_as_not_met"]);
        }
        if (p_props.has("empty_conditions_means_happened")) {
            empty_conditions_means_happened = static_cast<bool>(p_props["empty_conditions_means_happened"]);
        }
        if (p_props.has("allow_reevaluate_terminal")) {
            allow_reevaluate_terminal = static_cast<bool>(p_props["allow_reevaluate_terminal"]);
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
            const uint32_t ist_id = get_target_buffer_id();
            const GrantPartPOD* ist_part  = (ist_id != INVALID_ID) ? p_context.get_grant_part(ist_id) : nullptr;
            const GrantPartPOD* cst_part  = (condition_status_buffer_id != INVALID_ID)          ? p_context.get_grant_part(condition_status_buffer_id)          : nullptr;
            if (!ist_part || !ist_part->raw_base_ptr || !cst_part || !cst_part->raw_base_ptr) return;

            const GrantPartPOD* coff_part = (cond_slice_offset_buffer_id != INVALID_ID)         ? p_context.get_grant_part(cond_slice_offset_buffer_id)         : nullptr;
            const GrantPartPOD* ccnt_part = (cond_slice_count_buffer_id != INVALID_ID)          ? p_context.get_grant_part(cond_slice_count_buffer_id)          : nullptr;
            const GrantPartPOD* cid_part  = (incident_cond_id_buffer_id != INVALID_ID)          ? p_context.get_grant_part(incident_cond_id_buffer_id)          : nullptr;
            const GrantPartPOD* trn_part  = (incident_transition_flags_buffer_id != INVALID_ID) ? p_context.get_grant_part(incident_transition_flags_buffer_id) : nullptr;
            const GrantPartPOD* rcnt_part = (resolved_count_buffer_id != INVALID_ID)            ? p_context.get_grant_part(resolved_count_buffer_id)            : nullptr;

            uint8_t* __restrict ist_raw       = ist_part->raw_base_ptr;
            const uint32_t ist_stride         = (ist_part->element_stride > 0) ? ist_part->element_stride : sizeof(T);
            const int64_t inc_cap             = static_cast<int64_t>(ist_part->capacity_bytes / ist_stride);

            const uint8_t* __restrict cst_raw = cst_part->raw_base_ptr;
            const uint32_t cst_stride         = (cst_part->element_stride > 0) ? cst_part->element_stride : sizeof(int8_t);
            const int64_t cst_cap             = static_cast<int64_t>(cst_part->capacity_bytes / cst_stride);
            if (inc_cap <= 0 || cst_cap <= 0) return;

            const uint32_t* __restrict cond_offsets = (coff_part && coff_part->raw_base_ptr) ? reinterpret_cast<const uint32_t*>(coff_part->raw_base_ptr) : nullptr;
            const int64_t coff_cap                  = cond_offsets ? static_cast<int64_t>(coff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict ccnt_raw      = (ccnt_part && ccnt_part->raw_base_ptr) ? ccnt_part->raw_base_ptr : nullptr;
            const uint32_t ccnt_stride              = ccnt_part ? ccnt_part->element_stride : 0;
            const int64_t ccnt_cap                  = (ccnt_raw && ccnt_stride > 0) ? static_cast<int64_t>(ccnt_part->capacity_bytes / ccnt_stride) : 0;

            const uint32_t* __restrict cond_ids     = (cid_part && cid_part->raw_base_ptr)   ? reinterpret_cast<const uint32_t*>(cid_part->raw_base_ptr)  : nullptr;
            const int64_t cid_cap                   = cond_ids ? static_cast<int64_t>(cid_part->capacity_bytes / sizeof(uint32_t)) : 0;

            uint8_t* __restrict trn_raw             = (trn_part && trn_part->raw_base_ptr)   ? trn_part->raw_base_ptr : nullptr;
            const uint32_t trn_stride               = trn_part ? trn_part->element_stride : 0;
            const int64_t trn_cap                   = (trn_raw && trn_stride > 0) ? static_cast<int64_t>(trn_part->capacity_bytes / trn_stride) : 0;

            uint32_t* __restrict resolved_counter   = (rcnt_part && rcnt_part->raw_base_ptr) ? reinterpret_cast<uint32_t*>(rcnt_part->raw_base_ptr) : nullptr;
            uint32_t newly_resolved_count           = 0u;

            auto read_inc_status = [&](int64_t idx) noexcept -> int8_t {
                if (ist_stride == sizeof(int32_t)) {
                    return static_cast<int8_t>(reinterpret_cast<const int32_t*>(ist_raw)[idx]);
                }
                return reinterpret_cast<const int8_t*>(ist_raw)[idx];
            };

            auto write_inc_status = [&](int64_t idx, int8_t val) noexcept {
                if (ist_stride == sizeof(int32_t)) {
                    reinterpret_cast<int32_t*>(ist_raw)[idx] = static_cast<int32_t>(val);
                } else {
                    reinterpret_cast<int8_t*>(ist_raw)[idx] = val;
                }
            };

            auto read_cond_status = [&](int64_t cidx) noexcept -> int8_t {
                if (cst_stride == sizeof(int32_t)) {
                    return static_cast<int8_t>(reinterpret_cast<const int32_t*>(cst_raw)[cidx]);
                }
                return reinterpret_cast<const int8_t*>(cst_raw)[cidx];
            };

            auto read_cond_count = [&](int64_t idx) noexcept -> uint32_t {
                if (!ccnt_raw || idx < 0 || idx >= ccnt_cap) return 1u;
                if (ccnt_stride == sizeof(uint32_t)) {
                    return reinterpret_cast<const uint32_t*>(ccnt_raw)[idx];
                }
                return static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(ccnt_raw)[idx]);
            };

            const uint32_t wake_id = (dependent_plot_wake_buffer_id != INVALID_ID)
                ? dependent_plot_wake_buffer_id
                : ist_part->buffer_id;

            auto eval_incident = [&](int64_t idx) noexcept {
                const int8_t cur_status = read_inc_status(idx);
                if (!allow_reevaluate_terminal &&
                    (cur_status == INCIDENT_STATUS_HAS_HAPPENED || cur_status == INCIDENT_STATUS_CANNOT_HAPPEN)) {
                    return;
                }

                const int64_t slice_off = (cond_offsets && idx < coff_cap) ? static_cast<int64_t>(cond_offsets[idx]) : idx;
                const int64_t slice_cnt = (ccnt_raw && idx < ccnt_cap)     ? static_cast<int64_t>(read_cond_count(idx)) : 1;

                if (slice_cnt <= 0) {
                    if (empty_conditions_means_happened && cur_status != INCIDENT_STATUS_HAS_HAPPENED) {
                        write_inc_status(idx, INCIDENT_STATUS_HAS_HAPPENED);
                        ++newly_resolved_count;
                        if (emit_transition_flags && trn_raw && idx < trn_cap) {
                            if (trn_stride == sizeof(uint32_t)) reinterpret_cast<uint32_t*>(trn_raw)[idx] |= INCIDENT_TRANSITION_HAPPENED;
                            else                                trn_raw[idx] |= INCIDENT_TRANSITION_HAPPENED;
                        }
                        if (wake_on_status_change) p_context.queue_selection_command(wake_id, idx);
                    }
                    return;
                }

                int8_t new_status = INCIDENT_STATUS_HAS_HAPPENED;

                for (int64_t k = 0; k < slice_cnt; ++k) {
                    const int64_t slot_idx = slice_off + k;
                    int64_t cidx = slot_idx;
                    if (cond_ids) {
                        if (slot_idx < 0 || slot_idx >= cid_cap) continue;
                        const uint32_t mapped = cond_ids[slot_idx];
                        if (mapped == INVALID_ID) continue;
                        cidx = static_cast<int64_t>(mapped);
                    }
                    if (cidx < 0 || cidx >= cst_cap) continue;

                    const int8_t c_st = read_cond_status(cidx);
                    if (c_st == COND_STATUS_CANNOT_MEET) {
                        new_status = INCIDENT_STATUS_CANNOT_HAPPEN;
                        break;
                    }
                    if (c_st == COND_STATUS_NOT_MET || (treat_unknown_as_not_met && c_st == COND_STATUS_UNKNOWN)) {
                        new_status = INCIDENT_STATUS_HAS_NOT_HAPPENED;
                    }
                }

                if (new_status != cur_status) {
                    write_inc_status(idx, new_status);
                    if (new_status == INCIDENT_STATUS_HAS_HAPPENED || new_status == INCIDENT_STATUS_CANNOT_HAPPEN) {
                        ++newly_resolved_count;
                    }

                    if (emit_transition_flags && trn_raw && idx < trn_cap) {
                        uint8_t tflag = INCIDENT_TRANSITION_NONE;
                        if (new_status == INCIDENT_STATUS_HAS_HAPPENED)      tflag = INCIDENT_TRANSITION_HAPPENED;
                        else if (new_status == INCIDENT_STATUS_CANNOT_HAPPEN) tflag = INCIDENT_TRANSITION_CANNOT_HAPPEN;
                        if (tflag != INCIDENT_TRANSITION_NONE) {
                            if (trn_stride == sizeof(uint32_t)) reinterpret_cast<uint32_t*>(trn_raw)[idx] |= static_cast<uint32_t>(tflag);
                            else                                trn_raw[idx] |= tflag;
                        }
                    }

                    if (wake_on_status_change) {
                        p_context.queue_selection_command(wake_id, idx);
                    }
                }
            };

            _iterate_selection(ist_part->selection, inc_cap, eval_incident);

            if (resolved_counter) {
                *resolved_counter = newly_resolved_count;
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

static_assert(sizeof(IncidentStatusResolveTransformLogic<>) == 64, "IncidentStatusResolveTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core