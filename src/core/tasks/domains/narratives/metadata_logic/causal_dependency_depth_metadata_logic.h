#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/stencil_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include "../transform_logic/symbolic_reference_resolve_transform_logic.h"
#include "../transform_logic/causal_condition_wave_propagate_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_CAUSAL_DEPTH_METADATA_ENUMS_DEFINED
#define IDEAM_NARRATIVES_CAUSAL_DEPTH_METADATA_ENUMS_DEFINED

/**
 * Topological depth evaluation modes for CausalDependencyDepthMetadataLogic (N15).
 */
enum CausalDepthResolutionMode : uint8_t {
    CAUSAL_DEPTH_INCIDENT_CHAIN  = 0, // Computes depth per Incident via causal condition slice -> ConditionEvaluationRowPOD::resolved_lhs_id
    CAUSAL_DEPTH_CONDITION_CHAIN = 1, // Computes depth per ConditionEvaluationRowPOD via cause_incident_id -> incident's causal slice
    CAUSAL_DEPTH_DIRECT_CSR      = 2  // Computes depth via direct Incident-to-Incident CSR parent offsets/edges
};

#endif // IDEAM_NARRATIVES_CAUSAL_DEPTH_METADATA_ENUMS_DEFINED

/**
 * CausalDependencyDepthMetadataLogic<T> (N15)
 * Computes topological causal depth across Incident -> Causal_Condition -> Incident chains
 * and writes depth tiers into r_selection.lod_levels (and optionally r_selection.partition_ids
 * or output_depth_buffer_id) so multi-hop causal waves execute in deterministic topological order.
 */
template <typename T = int32_t>
struct alignas(64) CausalDependencyDepthMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = StencilView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::INT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Causal Dependency Depth";
    static constexpr std::string_view ui_category  = "Events";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                = INVALID_ID;                  // 4B: Primary Incident or Condition selection buffer
    uint32_t causal_cond_offset_buffer_id    = INVALID_ID;                  // 4B: IncidentStateSoABuffer::causal_cond_offset (uint32_t)
    uint32_t causal_cond_count_buffer_id     = INVALID_ID;                  // 4B: IncidentStateSoABuffer::causal_cond_count (uint16_t/uint32_t)
    uint32_t condition_index_table_buffer_id = INVALID_ID;                  // 4B: Optional uint32_t indirection table into ConditionTableBuffer
    uint32_t condition_row_buffer_id         = INVALID_ID;                  // 4B: ConditionEvaluationRowPOD AoS buffer (resolved_lhs_id = cause_incident_id)
    uint32_t parent_csr_offsets_buffer_id    = INVALID_ID;                  // 4B: Optional direct parent incident CSR row_offsets (uint32_t)
    uint32_t parent_csr_ids_buffer_id        = INVALID_ID;                  // 4B: Optional direct parent incident CSR IDs (uint32_t)
    uint32_t incident_status_buffer_id       = INVALID_ID;                  // 4B: Optional IncidentStateSoABuffer::incident_status (int8_t/int32_t)
    uint32_t output_depth_buffer_id          = INVALID_ID;                  // 4B: Optional int32_t/uint8_t column to persist computed causal depth
    int32_t partition_id_base_offset         = 0;                           // 4B: Base offset added when stamping depth into r_selection.partition_ids

    uint8_t resolution_mode                  = CAUSAL_DEPTH_INCIDENT_CHAIN; // 1B: CausalDepthResolutionMode (0..2)
    uint8_t max_causal_depth                 = 16u;                         // 1B: Cycle-safe maximum recursion/walk depth (clamped to [1, 32])
    uint8_t cycle_fallback_depth             = 255u;                        // 1B: Depth value assigned when a causal cycle is detected
    bool write_depth_to_lod_levels           = true;                        // 1B: Write topological depth tier into r_selection.lod_levels
    bool write_depth_to_partition_ids        = true;                        // 1B: Write topological depth tier into r_selection.partition_ids
    bool skip_completed_cause_ancestors      = false;                       // 1B: Treat already-COMPLETE (2) cause incidents as depth 0 roots
    uint8_t reserved_padding[18]             = {0};                         // 18B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary coff_prop;
        coff_prop["name"] = "causal_cond_offset_buffer_id";
        coff_prop["type"] = godot::Variant::INT;
        coff_prop["hint_string"] = "buffer_option";
        props.push_back(coff_prop);

        godot::Dictionary ccnt_prop;
        ccnt_prop["name"] = "causal_cond_count_buffer_id";
        ccnt_prop["type"] = godot::Variant::INT;
        ccnt_prop["hint_string"] = "buffer_option";
        props.push_back(ccnt_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "condition_index_table_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary crow_prop;
        crow_prop["name"] = "condition_row_buffer_id";
        crow_prop["type"] = godot::Variant::INT;
        crow_prop["hint_string"] = "buffer_option";
        props.push_back(crow_prop);

        godot::Dictionary poff_prop;
        poff_prop["name"] = "parent_csr_offsets_buffer_id";
        poff_prop["type"] = godot::Variant::INT;
        poff_prop["hint_string"] = "buffer_option";
        props.push_back(poff_prop);

        godot::Dictionary pid_prop;
        pid_prop["name"] = "parent_csr_ids_buffer_id";
        pid_prop["type"] = godot::Variant::INT;
        pid_prop["hint_string"] = "buffer_option";
        props.push_back(pid_prop);

        godot::Dictionary ist_prop;
        ist_prop["name"] = "incident_status_buffer_id";
        ist_prop["type"] = godot::Variant::INT;
        ist_prop["hint_string"] = "buffer_option";
        props.push_back(ist_prop);

        godot::Dictionary out_prop;
        out_prop["name"] = "output_depth_buffer_id";
        out_prop["type"] = godot::Variant::INT;
        out_prop["hint_string"] = "buffer_option";
        props.push_back(out_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "resolution_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Incident Causal Slice Chain:0,Condition Row Chain:1,Direct Parent CSR:2";
        props.push_back(mode_prop);

        godot::Dictionary max_prop;
        max_prop["name"] = "max_causal_depth";
        max_prop["type"] = godot::Variant::INT;
        max_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        max_prop["hint_string"] = "1,32,1";
        props.push_back(max_prop);

        godot::Dictionary cyc_prop;
        cyc_prop["name"] = "cycle_fallback_depth";
        cyc_prop["type"] = godot::Variant::INT;
        cyc_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        cyc_prop["hint_string"] = "0,255,1";
        props.push_back(cyc_prop);

        godot::Dictionary base_prop;
        base_prop["name"] = "partition_id_base_offset";
        base_prop["type"] = godot::Variant::INT;
        base_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(base_prop);

        godot::Dictionary lod_prop;
        lod_prop["name"] = "write_depth_to_lod_levels";
        lod_prop["type"] = godot::Variant::BOOL;
        lod_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(lod_prop);

        godot::Dictionary prt_prop;
        prt_prop["name"] = "write_depth_to_partition_ids";
        prt_prop["type"] = godot::Variant::BOOL;
        prt_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(prt_prop);

        godot::Dictionary skp_prop;
        skp_prop["name"] = "skip_completed_cause_ancestors";
        skp_prop["type"] = godot::Variant::BOOL;
        skp_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(skp_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)             return target_buffer_id;
        if (causal_cond_offset_buffer_id != INVALID_ID) return causal_cond_offset_buffer_id;
        if (condition_row_buffer_id != INVALID_ID)      return condition_row_buffer_id;
        return output_depth_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("causal_cond_offset_buffer_id")) {
            causal_cond_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["causal_cond_offset_buffer_id"]));
        }
        if (p_props.has("causal_cond_count_buffer_id")) {
            causal_cond_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["causal_cond_count_buffer_id"]));
        }
        if (p_props.has("condition_index_table_buffer_id")) {
            condition_index_table_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_index_table_buffer_id"]));
        }
        if (p_props.has("condition_row_buffer_id")) {
            condition_row_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["condition_row_buffer_id"]));
        }
        if (p_props.has("parent_csr_offsets_buffer_id")) {
            parent_csr_offsets_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_csr_offsets_buffer_id"]));
        }
        if (p_props.has("parent_csr_ids_buffer_id")) {
            parent_csr_ids_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_csr_ids_buffer_id"]));
        }
        if (p_props.has("incident_status_buffer_id")) {
            incident_status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["incident_status_buffer_id"]));
        }
        if (p_props.has("output_depth_buffer_id")) {
            output_depth_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_depth_buffer_id"]));
        }
        if (p_props.has("partition_id_base_offset")) {
            partition_id_base_offset = static_cast<int32_t>(static_cast<int64_t>(p_props["partition_id_base_offset"]));
        }
        if (p_props.has("resolution_mode")) {
            resolution_mode = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["resolution_mode"]), 0, 2));
        }
        if (p_props.has("max_causal_depth")) {
            max_causal_depth = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_causal_depth"]), 1, 32));
        }
        if (p_props.has("cycle_fallback_depth")) {
            cycle_fallback_depth = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["cycle_fallback_depth"]), 0, 255));
        }
        if (p_props.has("write_depth_to_lod_levels")) {
            write_depth_to_lod_levels = static_cast<bool>(p_props["write_depth_to_lod_levels"]);
        }
        if (p_props.has("write_depth_to_partition_ids")) {
            write_depth_to_partition_ids = static_cast<bool>(p_props["write_depth_to_partition_ids"]);
        }
        if (p_props.has("skip_completed_cause_ancestors")) {
            skip_completed_cause_ancestors = static_cast<bool>(p_props["skip_completed_cause_ancestors"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_context,
        T_View& p_view
    ) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, int32_t>) {
            if (r_selection.element_count == 0) return;

            const GrantPartPOD* coff_part = (causal_cond_offset_buffer_id != INVALID_ID)    ? p_context.get_grant_part(causal_cond_offset_buffer_id)    : nullptr;
            const GrantPartPOD* ccnt_part = (causal_cond_count_buffer_id != INVALID_ID)     ? p_context.get_grant_part(causal_cond_count_buffer_id)     : nullptr;
            const GrantPartPOD* idx_part  = (condition_index_table_buffer_id != INVALID_ID) ? p_context.get_grant_part(condition_index_table_buffer_id) : nullptr;
            const GrantPartPOD* crow_part = (condition_row_buffer_id != INVALID_ID)         ? p_context.get_grant_part(condition_row_buffer_id)         : nullptr;
            const GrantPartPOD* poff_part = (parent_csr_offsets_buffer_id != INVALID_ID)    ? p_context.get_grant_part(parent_csr_offsets_buffer_id)    : nullptr;
            const GrantPartPOD* pid_part  = (parent_csr_ids_buffer_id != INVALID_ID)        ? p_context.get_grant_part(parent_csr_ids_buffer_id)        : nullptr;
            const GrantPartPOD* ist_part  = (incident_status_buffer_id != INVALID_ID)       ? p_context.get_grant_part(incident_status_buffer_id)       : nullptr;
            const GrantPartPOD* out_part  = (output_depth_buffer_id != INVALID_ID)          ? p_context.get_grant_part(output_depth_buffer_id)          : nullptr;

            const uint32_t* __restrict c_offsets = (coff_part && coff_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(coff_part->raw_base_ptr)
                : nullptr;
            const int64_t coff_cap = c_offsets ? static_cast<int64_t>(coff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict ccnt_raw = (ccnt_part && ccnt_part->raw_base_ptr) ? ccnt_part->raw_base_ptr : nullptr;
            const uint32_t ccnt_stride         = ccnt_part ? ccnt_part->element_stride : 0u;
            const int64_t ccnt_cap             = (ccnt_part && ccnt_stride > 0u) ? static_cast<int64_t>(ccnt_part->capacity_bytes / ccnt_stride) : 0;

            const uint32_t* __restrict cond_indices = (idx_part && idx_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(idx_part->raw_base_ptr)
                : nullptr;
            const int64_t idx_cap = cond_indices ? static_cast<int64_t>(idx_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const ConditionEvaluationRowPOD* __restrict cond_rows = (crow_part && crow_part->raw_base_ptr && crow_part->element_stride == sizeof(ConditionEvaluationRowPOD))
                ? reinterpret_cast<const ConditionEvaluationRowPOD*>(crow_part->raw_base_ptr)
                : nullptr;
            const int64_t crow_cap = cond_rows ? static_cast<int64_t>(crow_part->capacity_bytes / sizeof(ConditionEvaluationRowPOD)) : 0;

            const uint32_t* __restrict p_offsets = (poff_part && poff_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(poff_part->raw_base_ptr)
                : nullptr;
            const int64_t poff_cap = p_offsets ? static_cast<int64_t>(poff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint32_t* __restrict p_ids = (pid_part && pid_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(pid_part->raw_base_ptr)
                : nullptr;
            const int64_t pid_cap = p_ids ? static_cast<int64_t>(pid_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict ist_raw = (ist_part && ist_part->raw_base_ptr) ? ist_part->raw_base_ptr : nullptr;
            const uint32_t ist_stride         = ist_part ? ist_part->element_stride : 0u;
            const int64_t ist_cap             = (ist_part && ist_stride > 0u) ? static_cast<int64_t>(ist_part->capacity_bytes / ist_stride) : 0;

            uint8_t* __restrict out_raw = (out_part && out_part->raw_base_ptr) ? out_part->raw_base_ptr : nullptr;
            const uint32_t out_stride   = out_part ? out_part->element_stride : 0u;
            const int64_t out_cap       = (out_part && out_stride > 0u) ? static_cast<int64_t>(out_part->capacity_bytes / out_stride) : 0;

            auto read_count = [](const uint8_t* raw, uint32_t stride, int64_t cap, int64_t idx) noexcept -> uint32_t {
                if (!raw || idx < 0 || idx >= cap) return 0u;
                if (stride == sizeof(uint32_t)) return reinterpret_cast<const uint32_t*>(raw)[idx];
                if (stride == sizeof(uint16_t)) return static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(raw)[idx]);
                return static_cast<uint32_t>(raw[idx]);
            };

            auto is_incident_completed = [&](uint32_t inc_id) noexcept -> bool {
                if (!skip_completed_cause_ancestors || !ist_raw || inc_id == INVALID_ID || static_cast<int64_t>(inc_id) >= ist_cap) {
                    return false;
                }
                const int8_t st = (ist_stride == sizeof(int32_t))
                    ? static_cast<int8_t>(reinterpret_cast<const int32_t*>(ist_raw)[inc_id])
                    : reinterpret_cast<const int8_t*>(ist_raw)[inc_id];
                return st == 2; // INCIDENT_STATUS_COMPLETE
            };

            auto resolve_cause_incident_from_slot = [&](uint32_t slot) noexcept -> uint32_t {
                uint32_t cid = slot;
                if (cond_indices) {
                    if (static_cast<int64_t>(slot) >= idx_cap) return INVALID_ID;
                    cid = cond_indices[slot];
                }
                if (!cond_rows || cid == INVALID_ID || static_cast<int64_t>(cid) >= crow_cap) return INVALID_ID;
                const ConditionEvaluationRowPOD& row = cond_rows[cid];
                if (row.condition_type != CONDITION_TYPE_CAUSAL) return INVALID_ID;
                return row.resolved_lhs_id;
            };

            auto compute_incident_depth = [&](auto& self, uint32_t inc_id, uint32_t* path_stack, uint8_t path_len) noexcept -> uint8_t {
                if (inc_id == INVALID_ID || is_incident_completed(inc_id)) return 0u;

                for (uint8_t k = 0u; k < path_len; ++k) {
                    if (path_stack[k] == inc_id) {
                        return cycle_fallback_depth;
                    }
                }
                if (path_len >= max_causal_depth) {
                    return max_causal_depth;
                }

                path_stack[path_len] = inc_id;
                const uint8_t next_len = static_cast<uint8_t>(path_len + 1u);
                const int64_t iidx     = static_cast<int64_t>(inc_id);

                uint8_t max_parent_depth = 0u;
                bool has_active_parent   = false;

                if (resolution_mode == CAUSAL_DEPTH_DIRECT_CSR && p_offsets && p_ids) {
                    if (iidx >= 0 && (iidx + 1) < poff_cap) {
                        const uint32_t start_e = p_offsets[iidx];
                        const uint32_t end_e   = std::min<uint32_t>(p_offsets[iidx + 1], static_cast<uint32_t>(pid_cap));
                        for (uint32_t e = start_e; e < end_e; ++e) {
                            const uint32_t parent_inc = p_ids[e];
                            if (parent_inc == INVALID_ID || is_incident_completed(parent_inc)) continue;
                            has_active_parent = true;
                            const uint8_t pdep = self(self, parent_inc, path_stack, next_len);
                            if (pdep == cycle_fallback_depth) return cycle_fallback_depth;
                            if (pdep > max_parent_depth) max_parent_depth = pdep;
                        }
                    }
                } else if (c_offsets && ccnt_raw && cond_rows) {
                    if (iidx >= 0 && iidx < coff_cap && iidx < ccnt_cap) {
                        const uint32_t off = c_offsets[iidx];
                        const uint32_t cnt = read_count(ccnt_raw, ccnt_stride, ccnt_cap, iidx);
                        for (uint32_t k = 0u; k < cnt; ++k) {
                            const uint32_t parent_inc = resolve_cause_incident_from_slot(off + k);
                            if (parent_inc == INVALID_ID || is_incident_completed(parent_inc)) continue;
                            has_active_parent = true;
                            const uint8_t pdep = self(self, parent_inc, path_stack, next_len);
                            if (pdep == cycle_fallback_depth) return cycle_fallback_depth;
                            if (pdep > max_parent_depth) max_parent_depth = pdep;
                        }
                    }
                }

                if (!has_active_parent) return 0u;
                return static_cast<uint8_t>(std::min<uint32_t>(static_cast<uint32_t>(max_parent_depth) + 1u, max_causal_depth));
            };

            auto stamp_row = [&](int64_t idx) noexcept {
                if (idx < 0 || idx >= r_selection.capacity) return;

                uint32_t path_stack[33];
                uint8_t depth = 0u;

                if (resolution_mode == CAUSAL_DEPTH_CONDITION_CHAIN && cond_rows && idx < crow_cap) {
                    const ConditionEvaluationRowPOD& row = cond_rows[idx];
                    if (row.condition_type == CONDITION_TYPE_CAUSAL && row.resolved_lhs_id != INVALID_ID &&
                        !is_incident_completed(row.resolved_lhs_id)) {
                        const uint8_t inc_dep = compute_incident_depth(compute_incident_depth, row.resolved_lhs_id, path_stack, 0u);
                        depth = (inc_dep == cycle_fallback_depth)
                            ? cycle_fallback_depth
                            : static_cast<uint8_t>(std::min<uint32_t>(static_cast<uint32_t>(inc_dep) + 1u, max_causal_depth));
                    }
                } else {
                    depth = compute_incident_depth(compute_incident_depth, static_cast<uint32_t>(idx), path_stack, 0u);
                }

                if (write_depth_to_lod_levels && r_selection.lod_levels) {
                    r_selection.lod_levels[idx] = depth;
                }
                if (write_depth_to_partition_ids && r_selection.partition_ids) {
                    r_selection.partition_ids[idx] = static_cast<int64_t>(partition_id_base_offset) + static_cast<int64_t>(depth);
                }
                if (out_raw && idx < out_cap) {
                    if (out_stride == sizeof(int32_t)) {
                        reinterpret_cast<int32_t*>(out_raw)[idx] = static_cast<int32_t>(depth);
                    } else {
                        out_raw[idx] = depth;
                    }
                }
            };

            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                const uint64_t* bitset = r_selection.data.bitset;
                const int64_t cap      = r_selection.capacity;
                for (int64_t i = 0; i < cap; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        stamp_row(i);
                    }
                }
            } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                const int64_t* indices = r_selection.data.indices;
                const int64_t count    = r_selection.element_count;
                for (int64_t i = 0; i < count; ++i) {
                    stamp_row(indices[i]);
                }
            } else if (r_selection.mode == SelectionMode::RANGE) {
                const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                const int64_t end   = std::min<int64_t>(start + r_selection.element_count, r_selection.capacity);
                for (int64_t i = start; i < end; ++i) {
                    stamp_row(i);
                }
            }
        }
    }
};

static_assert(sizeof(CausalDependencyDepthMetadataLogic<>) == 64, "CausalDependencyDepthMetadataLogic must be exactly 64 bytes!");

} // namespace ideam::core