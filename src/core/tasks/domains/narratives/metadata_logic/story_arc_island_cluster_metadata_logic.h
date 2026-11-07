#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/memory_buffer_selection_pod.h"
#include "../../../../memory/views/stencil_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include "../transform_logic/plot_status_evaluate_transform_logic.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

#ifndef IDEAM_NARRATIVES_STORY_ARC_CLUSTER_ENUMS_DEFINED
#define IDEAM_NARRATIVES_STORY_ARC_CLUSTER_ENUMS_DEFINED

/**
 * Coupling criteria bitmask for StoryArcIslandClusterMetadataLogic (N16) when unifying
 * Narrative_Plot or Narrative_Incident rows into Disjoint Set Union (DSU) story-arc islands.
 */
enum StoryArcClusterLinkFlags : uint8_t {
    ARC_CLUSTER_LINK_NONE               = 0u,
    ARC_CLUSTER_LINK_SHARED_INCIDENTS   = 1u << 0, // Unify plots that share at least one PlotEventPOD::incident_id
    ARC_CLUSTER_LINK_SHARED_PARTICIPANTS= 1u << 1, // Unify plots/incidents whose 64-bit participant_mask (Characters/Props) intersect
    ARC_CLUSTER_LINK_SHARED_LOCATIONS   = 1u << 2, // Unify plots/incidents that occur at the same primary_location_id
    ARC_CLUSTER_LINK_SHARED_GROUP_MASK  = 1u << 3, // Unify plots/incidents whose 64-bit narrative group_mask intersect
    ARC_CLUSTER_LINK_DEFAULT_ALL        = ARC_CLUSTER_LINK_SHARED_INCIDENTS |
                                          ARC_CLUSTER_LINK_SHARED_PARTICIPANTS |
                                          ARC_CLUSTER_LINK_SHARED_LOCATIONS
};

#endif // IDEAM_NARRATIVES_STORY_ARC_CLUSTER_ENUMS_DEFINED

/**
 * StoryArcIslandClusterMetadataLogic<T> (N16)
 * Uses Disjoint Set Union (DSU) over shared Incidents, participant Characters/Props bitmasks,
 * primary Locations, and narrative group masks to partition independent subplots or incidents
 * into isolated r_selection.partition_ids waves for lock-free parallel execution.
 * Works with either p_context.local_workspace or a zero-allocation stack DSU fallback (up to 512 active rows).
 */
template <typename T = int32_t>
struct alignas(64) StoryArcIslandClusterMetadataLogic {
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

    static constexpr std::string_view display_name = "Story Arc Island Cluster";
    static constexpr std::string_view ui_category  = "Plots";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                = INVALID_ID;                   // 4B: Primary Plot or Incident selection buffer to partition
    uint32_t plot_event_offset_buffer_id     = INVALID_ID;                   // 4B: PlotProgressionSoABuffer::event_offset (uint32_t)
    uint32_t plot_event_count_buffer_id      = INVALID_ID;                   // 4B: PlotProgressionSoABuffer::event_count (uint16_t/uint32_t)
    uint32_t plot_events_buffer_id           = INVALID_ID;                   // 4B: Flattened PlotEventPOD array
    uint32_t participant_mask_buffer_id      = INVALID_ID;                   // 4B: Optional uint64_t/uint32_t participant character/prop bitmask per row
    uint32_t primary_location_id_buffer_id   = INVALID_ID;                   // 4B: Optional uint32_t primary_location_id per plot/incident
    uint32_t group_mask_buffer_id            = INVALID_ID;                   // 4B: Optional uint64_t/uint32_t narrative group mask per row
    uint32_t status_buffer_id                = INVALID_ID;                   // 4B: Optional plot_status / incident_status column for terminal filtering
    uint32_t output_cluster_id_buffer_id     = INVALID_ID;                   // 4B: Optional int32_t column to persist assigned story-arc cluster ID
    int32_t partition_id_offset              = 0;                            // 4B: Base offset added to compact cluster IDs (0, 1, 2, ...)
    int32_t isolated_or_terminal_partition_id= -1;                           // 4B: Partition ID assigned to terminal or culled rows

    uint8_t cluster_link_flags               = ARC_CLUSTER_LINK_DEFAULT_ALL; // 1B: StoryArcClusterLinkFlags bitmask
    uint8_t min_cluster_size                 = 1u;                           // 1B: Minimum cluster size (rows in smaller clusters get isolated_or_terminal_partition_id)
    bool ignore_forbidden_incidents          = true;                         // 1B: Ignore INCIDENT_REQ_FORBIDDEN when linking plots via shared incidents
    bool exclude_terminal_plots              = true;                         // 1B: Skip plots in PLOT_STATUS_SUCCESS (3) or PLOT_STATUS_FAILURE (4)
    bool write_cluster_size_to_lod           = true;                         // 1B: Stamp island size (clamped to 255) into r_selection.lod_levels
    bool cull_below_min_cluster_size         = false;                        // 1B: Clear selection bit/index when cluster size < min_cluster_size
    uint8_t reserved_padding[14]             = {0};                          // 14B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_buffer_id";
        tgt_prop["type"] = godot::Variant::INT;
        tgt_prop["hint_string"] = "buffer_option";
        props.push_back(tgt_prop);

        godot::Dictionary eoff_prop;
        eoff_prop["name"] = "plot_event_offset_buffer_id";
        eoff_prop["type"] = godot::Variant::INT;
        eoff_prop["hint_string"] = "buffer_option";
        props.push_back(eoff_prop);

        godot::Dictionary ecnt_prop;
        ecnt_prop["name"] = "plot_event_count_buffer_id";
        ecnt_prop["type"] = godot::Variant::INT;
        ecnt_prop["hint_string"] = "buffer_option";
        props.push_back(ecnt_prop);

        godot::Dictionary ev_prop;
        ev_prop["name"] = "plot_events_buffer_id";
        ev_prop["type"] = godot::Variant::INT;
        ev_prop["hint_string"] = "buffer_option";
        props.push_back(ev_prop);

        godot::Dictionary pmsk_prop;
        pmsk_prop["name"] = "participant_mask_buffer_id";
        pmsk_prop["type"] = godot::Variant::INT;
        pmsk_prop["hint_string"] = "buffer_option";
        props.push_back(pmsk_prop);

        godot::Dictionary loc_prop;
        loc_prop["name"] = "primary_location_id_buffer_id";
        loc_prop["type"] = godot::Variant::INT;
        loc_prop["hint_string"] = "buffer_option";
        props.push_back(loc_prop);

        godot::Dictionary gmsk_prop;
        gmsk_prop["name"] = "group_mask_buffer_id";
        gmsk_prop["type"] = godot::Variant::INT;
        gmsk_prop["hint_string"] = "buffer_option";
        props.push_back(gmsk_prop);

        godot::Dictionary st_prop;
        st_prop["name"] = "status_buffer_id";
        st_prop["type"] = godot::Variant::INT;
        st_prop["hint_string"] = "buffer_option";
        props.push_back(st_prop);

        godot::Dictionary out_prop;
        out_prop["name"] = "output_cluster_id_buffer_id";
        out_prop["type"] = godot::Variant::INT;
        out_prop["hint_string"] = "buffer_option";
        props.push_back(out_prop);

        godot::Dictionary flg_prop;
        flg_prop["name"] = "cluster_link_flags";
        flg_prop["type"] = godot::Variant::INT;
        flg_prop["hint"] = godot::PROPERTY_HINT_FLAGS;
        flg_prop["hint_string"] = "Shared Incidents,Shared Participants,Shared Locations,Shared Group Mask";
        props.push_back(flg_prop);

        godot::Dictionary min_prop;
        min_prop["name"] = "min_cluster_size";
        min_prop["type"] = godot::Variant::INT;
        min_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        min_prop["hint_string"] = "1,64,1";
        props.push_back(min_prop);

        godot::Dictionary off_prop;
        off_prop["name"] = "partition_id_offset";
        off_prop["type"] = godot::Variant::INT;
        off_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(off_prop);

        godot::Dictionary iso_prop;
        iso_prop["name"] = "isolated_or_terminal_partition_id";
        iso_prop["type"] = godot::Variant::INT;
        iso_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(iso_prop);

        godot::Dictionary frb_prop;
        frb_prop["name"] = "ignore_forbidden_incidents";
        frb_prop["type"] = godot::Variant::BOOL;
        frb_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(frb_prop);

        godot::Dictionary trm_prop;
        trm_prop["name"] = "exclude_terminal_plots";
        trm_prop["type"] = godot::Variant::BOOL;
        trm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(trm_prop);

        godot::Dictionary lod_prop;
        lod_prop["name"] = "write_cluster_size_to_lod";
        lod_prop["type"] = godot::Variant::BOOL;
        lod_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(lod_prop);

        godot::Dictionary cul_prop;
        cul_prop["name"] = "cull_below_min_cluster_size";
        cul_prop["type"] = godot::Variant::BOOL;
        cul_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cul_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID)            return target_buffer_id;
        if (plot_event_offset_buffer_id != INVALID_ID) return plot_event_offset_buffer_id;
        if (participant_mask_buffer_id != INVALID_ID)  return participant_mask_buffer_id;
        if (status_buffer_id != INVALID_ID)            return status_buffer_id;
        return output_cluster_id_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("plot_event_offset_buffer_id")) {
            plot_event_offset_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["plot_event_offset_buffer_id"]));
        }
        if (p_props.has("plot_event_count_buffer_id")) {
            plot_event_count_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["plot_event_count_buffer_id"]));
        }
        if (p_props.has("plot_events_buffer_id")) {
            plot_events_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["plot_events_buffer_id"]));
        }
        if (p_props.has("participant_mask_buffer_id")) {
            participant_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["participant_mask_buffer_id"]));
        }
        if (p_props.has("primary_location_id_buffer_id")) {
            primary_location_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["primary_location_id_buffer_id"]));
        }
        if (p_props.has("group_mask_buffer_id")) {
            group_mask_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["group_mask_buffer_id"]));
        }
        if (p_props.has("status_buffer_id")) {
            status_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["status_buffer_id"]));
        }
        if (p_props.has("output_cluster_id_buffer_id")) {
            output_cluster_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_cluster_id_buffer_id"]));
        }
        if (p_props.has("partition_id_offset")) {
            partition_id_offset = static_cast<int32_t>(static_cast<int64_t>(p_props["partition_id_offset"]));
        }
        if (p_props.has("isolated_or_terminal_partition_id")) {
            isolated_or_terminal_partition_id = static_cast<int32_t>(static_cast<int64_t>(p_props["isolated_or_terminal_partition_id"]));
        }
        if (p_props.has("cluster_link_flags")) {
            cluster_link_flags = static_cast<uint8_t>(static_cast<int64_t>(p_props["cluster_link_flags"]) & 0x0F);
        }
        if (p_props.has("min_cluster_size")) {
            min_cluster_size = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["min_cluster_size"]), 1, 255));
        }
        if (p_props.has("ignore_forbidden_incidents")) {
            ignore_forbidden_incidents = static_cast<bool>(p_props["ignore_forbidden_incidents"]);
        }
        if (p_props.has("exclude_terminal_plots")) {
            exclude_terminal_plots = static_cast<bool>(p_props["exclude_terminal_plots"]);
        }
        if (p_props.has("write_cluster_size_to_lod")) {
            write_cluster_size_to_lod = static_cast<bool>(p_props["write_cluster_size_to_lod"]);
        }
        if (p_props.has("cull_below_min_cluster_size")) {
            cull_below_min_cluster_size = static_cast<bool>(p_props["cull_below_min_cluster_size"]);
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
            if (!r_selection.partition_ids || r_selection.element_count == 0) return;

            const GrantPartPOD* eoff_part = (plot_event_offset_buffer_id != INVALID_ID)   ? p_context.get_grant_part(plot_event_offset_buffer_id)   : nullptr;
            const GrantPartPOD* ecnt_part = (plot_event_count_buffer_id != INVALID_ID)    ? p_context.get_grant_part(plot_event_count_buffer_id)    : nullptr;
            const GrantPartPOD* ev_part   = (plot_events_buffer_id != INVALID_ID)         ? p_context.get_grant_part(plot_events_buffer_id)         : nullptr;
            const GrantPartPOD* pmsk_part = (participant_mask_buffer_id != INVALID_ID)    ? p_context.get_grant_part(participant_mask_buffer_id)    : nullptr;
            const GrantPartPOD* loc_part  = (primary_location_id_buffer_id != INVALID_ID) ? p_context.get_grant_part(primary_location_id_buffer_id) : nullptr;
            const GrantPartPOD* gmsk_part = (group_mask_buffer_id != INVALID_ID)          ? p_context.get_grant_part(group_mask_buffer_id)          : nullptr;
            const GrantPartPOD* st_part   = (status_buffer_id != INVALID_ID)              ? p_context.get_grant_part(status_buffer_id)              : nullptr;
            const GrantPartPOD* out_part  = (output_cluster_id_buffer_id != INVALID_ID)   ? p_context.get_grant_part(output_cluster_id_buffer_id)   : nullptr;

            const uint32_t* __restrict ev_offsets = (eoff_part && eoff_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(eoff_part->raw_base_ptr)
                : nullptr;
            const int64_t eoff_cap = ev_offsets ? static_cast<int64_t>(eoff_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict ecnt_raw = (ecnt_part && ecnt_part->raw_base_ptr) ? ecnt_part->raw_base_ptr : nullptr;
            const uint32_t ecnt_stride         = ecnt_part ? ecnt_part->element_stride : 0u;
            const int64_t ecnt_cap             = (ecnt_part && ecnt_stride > 0u) ? static_cast<int64_t>(ecnt_part->capacity_bytes / ecnt_stride) : 0;

            const PlotEventPOD* __restrict plot_events = (ev_part && ev_part->raw_base_ptr && ev_part->element_stride == sizeof(PlotEventPOD))
                ? reinterpret_cast<const PlotEventPOD*>(ev_part->raw_base_ptr)
                : nullptr;
            const int64_t ev_cap = plot_events ? static_cast<int64_t>(ev_part->capacity_bytes / sizeof(PlotEventPOD)) : 0;

            const uint8_t* __restrict pmsk_raw = (pmsk_part && pmsk_part->raw_base_ptr) ? pmsk_part->raw_base_ptr : nullptr;
            const uint32_t pmsk_stride         = pmsk_part ? pmsk_part->element_stride : 0u;
            const int64_t pmsk_cap             = (pmsk_part && pmsk_stride > 0u) ? static_cast<int64_t>(pmsk_part->capacity_bytes / pmsk_stride) : 0;

            const uint32_t* __restrict loc_ids = (loc_part && loc_part->raw_base_ptr)
                ? reinterpret_cast<const uint32_t*>(loc_part->raw_base_ptr)
                : nullptr;
            const int64_t loc_cap = loc_ids ? static_cast<int64_t>(loc_part->capacity_bytes / sizeof(uint32_t)) : 0;

            const uint8_t* __restrict gmsk_raw = (gmsk_part && gmsk_part->raw_base_ptr) ? gmsk_part->raw_base_ptr : nullptr;
            const uint32_t gmsk_stride         = gmsk_part ? gmsk_part->element_stride : 0u;
            const int64_t gmsk_cap             = (gmsk_part && gmsk_stride > 0u) ? static_cast<int64_t>(gmsk_part->capacity_bytes / gmsk_stride) : 0;

            const uint8_t* __restrict st_raw = (st_part && st_part->raw_base_ptr) ? st_part->raw_base_ptr : nullptr;
            const uint32_t st_stride         = st_part ? st_part->element_stride : 0u;
            const int64_t st_cap             = (st_part && st_stride > 0u) ? static_cast<int64_t>(st_part->capacity_bytes / st_stride) : 0;

            int32_t* __restrict out_clusters = (out_part && out_part->raw_base_ptr)
                ? reinterpret_cast<int32_t*>(out_part->raw_base_ptr)
                : nullptr;
            const int64_t out_cap = out_clusters ? static_cast<int64_t>(out_part->capacity_bytes / sizeof(int32_t)) : 0;

            // Stack-safe DSU workspace (up to 512 active rows on stack, or uses p_context.local_workspace if bound)
            static constexpr int32_t STACK_MAX_NODES = 512;
            int64_t stack_local_to_global[STACK_MAX_NODES];
            int32_t stack_dsu_parent[STACK_MAX_NODES];
            int32_t stack_root_size[STACK_MAX_NODES];
            int32_t stack_root_to_part[STACK_MAX_NODES];

            int64_t* local_to_global = stack_local_to_global;
            int32_t* dsu_parent      = stack_dsu_parent;
            int32_t* root_size       = stack_root_size;
            int32_t* root_to_part    = stack_root_to_part;
            int32_t max_nodes        = STACK_MAX_NODES;

            if (p_context.local_workspace && r_selection.capacity > STACK_MAX_NODES) {
                const int64_t cap = r_selection.capacity;
                local_to_global = reinterpret_cast<int64_t*>(p_context.local_workspace);
                dsu_parent      = reinterpret_cast<int32_t*>(local_to_global + cap);
                root_size       = dsu_parent + cap;
                root_to_part    = root_size + cap;
                max_nodes       = static_cast<int32_t>(cap);
            }

            auto is_terminal_row = [&](int64_t idx) noexcept -> bool {
                if (!exclude_terminal_plots || !st_raw || idx < 0 || idx >= st_cap) return false;
                const int8_t st = (st_stride == sizeof(int32_t))
                    ? static_cast<int8_t>(reinterpret_cast<const int32_t*>(st_raw)[idx])
                    : reinterpret_cast<const int8_t*>(st_raw)[idx];
                return (st == PLOT_STATUS_SUCCESS || st == PLOT_STATUS_FAILURE);
            };

            int32_t n = 0;
            auto collect_row = [&](int64_t idx) noexcept {
                if (idx < 0 || idx >= r_selection.capacity) return;
                if (is_terminal_row(idx)) {
                    r_selection.partition_ids[idx] = static_cast<int64_t>(isolated_or_terminal_partition_id);
                    if (write_cluster_size_to_lod && r_selection.lod_levels) r_selection.lod_levels[idx] = 0u;
                    if (out_clusters && idx < out_cap) out_clusters[idx] = isolated_or_terminal_partition_id;
                    return;
                }
                if (n < max_nodes) {
                    local_to_global[n] = idx;
                    dsu_parent[n]      = n;
                    root_size[n]       = 0;
                    root_to_part[n]    = -1;
                    ++n;
                }
            };

            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                const uint64_t* bitset = r_selection.data.bitset;
                for (int64_t i = 0; i < r_selection.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        collect_row(i);
                    }
                }
            } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                for (int64_t i = 0; i < r_selection.element_count; ++i) {
                    collect_row(r_selection.data.indices[i]);
                }
            } else if (r_selection.mode == SelectionMode::RANGE) {
                const int64_t start = std::max<int64_t>(0, r_selection.start_index);
                const int64_t end   = std::min<int64_t>(start + r_selection.element_count, r_selection.capacity);
                for (int64_t i = start; i < end; ++i) {
                    collect_row(i);
                }
            }

            if (n == 0) return;

            auto read_count = [](const uint8_t* raw, uint32_t stride, int64_t cap, int64_t idx) noexcept -> uint32_t {
                if (!raw || idx < 0 || idx >= cap) return 0u;
                if (stride == sizeof(uint32_t)) return reinterpret_cast<const uint32_t*>(raw)[idx];
                if (stride == sizeof(uint16_t)) return static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(raw)[idx]);
                return static_cast<uint32_t>(raw[idx]);
            };

            auto read_mask64 = [](const uint8_t* raw, uint32_t stride, int64_t cap, int64_t idx, const uint32_t* fallback32, int64_t fb_cap) noexcept -> uint64_t {
                if (raw && idx >= 0 && idx < cap) {
                    if (stride == sizeof(uint64_t)) return reinterpret_cast<const uint64_t*>(raw)[idx];
                    if (stride == sizeof(uint32_t)) return static_cast<uint64_t>(reinterpret_cast<const uint32_t*>(raw)[idx]);
                }
                if (fallback32 && idx >= 0 && idx < fb_cap) {
                    return static_cast<uint64_t>(fallback32[idx]);
                }
                return 0ULL;
            };

            auto find_root = [&](int32_t i) noexcept -> int32_t {
                while (i != dsu_parent[i]) {
                    dsu_parent[i] = dsu_parent[dsu_parent[i]];
                    i = dsu_parent[i];
                }
                return i;
            };

            auto unite_nodes = [&](int32_t a, int32_t b) noexcept {
                const int32_t ra = find_root(a);
                const int32_t rb = find_root(b);
                if (ra != rb) {
                    if (ra < rb) dsu_parent[rb] = ra;
                    else         dsu_parent[ra] = rb;
                }
            };

            auto shares_incident = [&](int64_t ga, int64_t gb) noexcept -> bool {
                if (!plot_events || !ev_offsets || !ecnt_raw) return false;
                if (ga < 0 || ga >= eoff_cap || gb < 0 || gb >= eoff_cap) return false;
                const uint32_t off_a = ev_offsets[ga];
                const uint32_t cnt_a = read_count(ecnt_raw, ecnt_stride, ecnt_cap, ga);
                const uint32_t end_a = std::min<uint32_t>(off_a + cnt_a, static_cast<uint32_t>(ev_cap));

                const uint32_t off_b = ev_offsets[gb];
                const uint32_t cnt_b = read_count(ecnt_raw, ecnt_stride, ecnt_cap, gb);
                const uint32_t end_b = std::min<uint32_t>(off_b + cnt_b, static_cast<uint32_t>(ev_cap));

                for (uint32_t ea = off_a; ea < end_a; ++ea) {
                    const PlotEventPOD& pa = plot_events[ea];
                    if (pa.incident_id == INVALID_ID) continue;
                    if (ignore_forbidden_incidents && pa.requirement_type == INCIDENT_REQ_FORBIDDEN) continue;
                    for (uint32_t eb = off_b; eb < end_b; ++eb) {
                        const PlotEventPOD& pb = plot_events[eb];
                        if (ignore_forbidden_incidents && pb.requirement_type == INCIDENT_REQ_FORBIDDEN) continue;
                        if (pa.incident_id == pb.incident_id) return true;
                    }
                }
                return false;
            };

            auto are_rows_coupled = [&](int64_t ga, int64_t gb) noexcept -> bool {
                if ((cluster_link_flags & ARC_CLUSTER_LINK_SHARED_INCIDENTS) != 0u) {
                    if (shares_incident(ga, gb)) return true;
                }
                if ((cluster_link_flags & ARC_CLUSTER_LINK_SHARED_PARTICIPANTS) != 0u && pmsk_raw) {
                    const uint64_t ma = read_mask64(pmsk_raw, pmsk_stride, pmsk_cap, ga, nullptr, 0);
                    const uint64_t mb = read_mask64(pmsk_raw, pmsk_stride, pmsk_cap, gb, nullptr, 0);
                    if ((ma & mb) != 0ULL) return true;
                }
                if ((cluster_link_flags & ARC_CLUSTER_LINK_SHARED_LOCATIONS) != 0u && loc_ids) {
                    if (ga < loc_cap && gb < loc_cap) {
                        const uint32_t la = loc_ids[ga];
                        const uint32_t lb = loc_ids[gb];
                        if (la != INVALID_ID && la == lb) return true;
                    }
                }
                if ((cluster_link_flags & ARC_CLUSTER_LINK_SHARED_GROUP_MASK) != 0u) {
                    const uint64_t gma = read_mask64(gmsk_raw, gmsk_stride, gmsk_cap, ga, r_selection.group_masks, r_selection.capacity);
                    const uint64_t gmb = read_mask64(gmsk_raw, gmsk_stride, gmsk_cap, gb, r_selection.group_masks, r_selection.capacity);
                    if ((gma & gmb) != 0ULL) return true;
                }
                return false;
            };

            // Pairwise DSU unification over active selected rows
            for (int32_t i = 0; i < n; ++i) {
                const int64_t ga = local_to_global[i];
                for (int32_t j = i + 1; j < n; ++j) {
                    if (find_root(i) == find_root(j)) continue;
                    const int64_t gb = local_to_global[j];
                    if (are_rows_coupled(ga, gb)) {
                        unite_nodes(i, j);
                    }
                }
            }

            // Compute island sizes per DSU root
            for (int32_t i = 0; i < n; ++i) {
                ++root_size[find_root(i)];
            }

            // Assign compact partition IDs and optional LOD cluster sizes
            int32_t next_partition_id = partition_id_offset;
            bool need_sparse_repack   = false;

            for (int32_t i = 0; i < n; ++i) {
                const int64_t gidx  = local_to_global[i];
                const int32_t root  = find_root(i);
                const int32_t csize = root_size[root];

                if (write_cluster_size_to_lod && r_selection.lod_levels) {
                    r_selection.lod_levels[gidx] = static_cast<uint8_t>(std::min<int32_t>(csize, 255));
                }

                if (csize < static_cast<int32_t>(min_cluster_size)) {
                    r_selection.partition_ids[gidx] = static_cast<int64_t>(isolated_or_terminal_partition_id);
                    if (out_clusters && gidx < out_cap) {
                        out_clusters[gidx] = isolated_or_terminal_partition_id;
                    }
                    if (cull_below_min_cluster_size) {
                        if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                            r_selection.data.bitset[gidx >> 6] &= ~(1ULL << (gidx & 63));
                            --r_selection.element_count;
                        } else if (r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                            for (int64_t s = 0; s < r_selection.element_count; ++s) {
                                if (r_selection.data.indices[s] == gidx) {
                                    r_selection.data.indices[s] = -1;
                                    need_sparse_repack = true;
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    if (root_to_part[root] < 0) {
                        root_to_part[root] = next_partition_id++;
                    }
                    const int32_t assigned_part = root_to_part[root];
                    r_selection.partition_ids[gidx] = static_cast<int64_t>(assigned_part);
                    if (out_clusters && gidx < out_cap) {
                        out_clusters[gidx] = assigned_part;
                    }
                }
            }

            if (need_sparse_repack && r_selection.mode == SelectionMode::SPARSE && r_selection.data.indices) {
                int64_t write_ptr = 0;
                for (int64_t s = 0; s < r_selection.element_count; ++s) {
                    if (r_selection.data.indices[s] != -1) {
                        r_selection.data.indices[write_ptr++] = r_selection.data.indices[s];
                    }
                }
                r_selection.element_count = write_ptr;
            }
        }
    }
};

static_assert(sizeof(StoryArcIslandClusterMetadataLogic<>) == 64, "StoryArcIslandClusterMetadataLogic must be exactly 64 bytes!");

} // namespace ideam::core