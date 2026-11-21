#pragma once

/**
 * conflict_horizon_box_partition_metadata_logic.h
 * 64-byte cache-aligned MetadataLogic task struct (`QD1`) for `IdeamQuantumDarwinism`.
 *
 * Supports both:
 *   - Conflict-centered bounded E_box BFS (`seed_from_pinned_landmarks == 0`)
 *   - Outward landmark-seeded wavefront distance partition (`seed_from_pinned_landmarks == 1`)
 * Uses `WAVEFRONT_FRONTIER_QUEUE` when bound (lifting the 512-slot stack ring cap to full environment size).
 */

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../metadata_logic/metadata_logic_traits.h"
#include "../../../../quantum_darwinism/quantum_darwinism_common.h"
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) ConflictHorizonBoxPartitionMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32;

    static constexpr size_t dimensions             = 0;
    static constexpr bool requires_static_kernel   = false;
    static constexpr size_t kernel_size            = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "QD Conflict & Wavefront Horizon Partition";
    static constexpr std::string_view ui_category  = "QuantumDarwinism";

    uint32_t target_buffer_id            = INVALID_ID; ///< `ENVIRONMENT_SLOT_POD` (`EnvironmentSlotPOD`)
    uint32_t metadata_buffer_id          = INVALID_ID; ///< `HORIZON_PARTITION_POD` (`HorizonSlotPartitionPOD`)
    uint32_t child_csr_buffer_id         = INVALID_ID; ///< `CHILD_SLOT_CSR` (`uint32_t`)
    uint32_t spatial_csr_buffer_id       = INVALID_ID; ///< `SPATIAL_NEIGHBOR_CSR` (`SpatialNeighborEdgePOD`)
    uint32_t pointer_conflict_buffer_id  = INVALID_ID; ///< `POINTER_OR_CONFLICT_MASK` (`uint32_t`)
    uint32_t frontier_queue_buffer_id    = INVALID_ID; ///< `WAVEFRONT_FRONTIER_QUEUE` (`uint32_t`, lifts 512 cap)
    uint32_t max_horizon_radius          = 4;
    uint32_t max_horizon_slots           = 4096;
    PatternCascadeLayer min_locked_layer = PatternCascadeLayer::LAYER_HIERARCHY_PARENT;
    uint8_t  seed_from_pinned_landmarks  = 0;          ///< `1` = outward BFS from pinned landmarks / Pointer States
    uint8_t  reserved_padding[30]        = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",           godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("metadata_buffer_id",         godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("child_csr_buffer_id",        godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("spatial_csr_buffer_id",      godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("pointer_conflict_buffer_id", godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("frontier_queue_buffer_id",   godot::Variant::INT,  static_cast<int64_t>(INVALID_ID));
        add_prop("max_horizon_radius",         godot::Variant::INT,  static_cast<int64_t>(4));
        add_prop("max_horizon_slots",          godot::Variant::INT,  static_cast<int64_t>(4096));
        add_prop("seed_from_pinned_landmarks", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept { return target_buffer_id; }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))           target_buffer_id           = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("metadata_buffer_id"))         metadata_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["metadata_buffer_id"]));
        if (p_props.has("child_csr_buffer_id"))        child_csr_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["child_csr_buffer_id"]));
        if (p_props.has("spatial_csr_buffer_id"))      spatial_csr_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["spatial_csr_buffer_id"]));
        if (p_props.has("pointer_conflict_buffer_id")) pointer_conflict_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["pointer_conflict_buffer_id"]));
        if (p_props.has("frontier_queue_buffer_id"))   frontier_queue_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["frontier_queue_buffer_id"]));
        if (p_props.has("max_horizon_radius"))         max_horizon_radius         = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_horizon_radius"]));
        if (p_props.has("max_horizon_slots"))          max_horizon_slots          = static_cast<uint32_t>(static_cast<int64_t>(p_props["max_horizon_slots"]));
        if (p_props.has("seed_from_pinned_landmarks")) seed_from_pinned_landmarks = static_cast<bool>(p_props["seed_from_pinned_landmarks"]) ? 1u : 0u;
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_Elem>
    [[nodiscard]] static T_Elem *_resolve_ptr(const TaskContextPOD &p_ctx, uint32_t p_buf_id, size_t &r_cap) noexcept {
        r_cap = 0u;
        if (p_buf_id == INVALID_ID) return nullptr;
        if (const GrantPartPOD *part = p_ctx.get_grant_part(p_buf_id)) {
            if (part->raw_base_ptr) {
                const size_t cap = part->capacity_bytes / sizeof(T_Elem);
                r_cap = (part->selection.element_count > 0)
                    ? std::min<size_t>(static_cast<size_t>(part->selection.element_count), cap)
                    : cap;
                return reinterpret_cast<T_Elem *>(part->raw_base_ptr);
            }
        }
        if (p_ctx.manager) {
            if (MemoryBufferPOD *buf = p_ctx.manager->get_buffer(p_buf_id)) {
                if (buf->master_block_ptr) {
                    const size_t cap = static_cast<size_t>(std::max<int64_t>(0, buf->max_elements));
                    r_cap = (buf->current_count > 0)
                        ? std::min<size_t>(static_cast<size_t>(buf->current_count), cap)
                        : cap;
                    return reinterpret_cast<T_Elem *>(buf->master_block_ptr + buf->memory_offset);
                }
            }
        }
        return nullptr;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t slot_cap = 0, part_cap = 0, child_cap = 0, spatial_cap = 0, status_cap = 0, queue_cap = 0;
        const auto *slots       = _resolve_ptr<const EnvironmentSlotPOD>(p_ctx, target_buffer_id, slot_cap);
        auto *partitions        = _resolve_ptr<HorizonSlotPartitionPOD>(p_ctx, metadata_buffer_id, part_cap);
        const auto *child_csr   = _resolve_ptr<const uint32_t>(p_ctx, child_csr_buffer_id, child_cap);
        const auto *spatial_csr = _resolve_ptr<const SpatialNeighborEdgePOD>(p_ctx, spatial_csr_buffer_id, spatial_cap);
        const auto *status_mask = _resolve_ptr<const uint32_t>(p_ctx, pointer_conflict_buffer_id, status_cap);
        auto *ext_queue         = _resolve_ptr<uint32_t>(p_ctx, frontier_queue_buffer_id, queue_cap);

        if (!slots || slot_cap == 0 || !partitions || part_cap == 0) {
            return;
        }

        const uint32_t n_slots       = static_cast<uint32_t>(std::min(slot_cap, part_cap));
        const uint32_t n_child_csr   = static_cast<uint32_t>(child_cap);
        const uint32_t n_spatial_csr = static_cast<uint32_t>(spatial_cap);

        for (uint32_t i = 0; i < n_slots; ++i) {
            partitions[i].slot_id                 = i;
            partitions[i].nearest_conflict_slot   = 0xFFFFFFFFu;
            partitions[i].predecessor_toward_conf = 0xFFFFFFFFu;
            partitions[i].hop_distance            = 0xFFFFu;
            partitions[i].role_flags              = HORIZON_ROLE_OUTSIDE;
            partitions[i].locked_cascade_floor    = static_cast<uint8_t>(min_locked_layer);
        }

        // Use DOD frontier queue when bound (lifts 512-slot cap); fall back to 512-slot stack ring otherwise
        constexpr uint32_t FALLBACK_CAP = 512u;
        uint32_t fallback_ring[FALLBACK_CAP];
        uint32_t *bfs_queue = (ext_queue && queue_cap > 0u) ? ext_queue : fallback_ring;
        const uint32_t q_cap = (ext_queue && queue_cap > 0u) ? static_cast<uint32_t>(queue_cap) : FALLBACK_CAP;

        uint32_t q_head = 0u;
        uint32_t q_tail = 0u;
        uint32_t box_count = 0u;
        const uint32_t max_box    = std::min<uint32_t>(q_cap, std::max<uint32_t>(1u, max_horizon_slots));
        const uint16_t max_radius = (seed_from_pinned_landmarks != 0u)
            ? static_cast<uint16_t>(std::min<uint32_t>(0xFFFEu, std::max<uint32_t>(max_horizon_radius, n_slots)))
            : static_cast<uint16_t>(std::max<uint32_t>(1u, max_horizon_radius));

        for (uint32_t i = 0; i < n_slots && box_count < max_box && q_tail < q_cap; ++i) {
            bool flagged = false;
            if (seed_from_pinned_landmarks != 0u) {
                flagged = (slots[i].locked_layer == PatternCascadeLayer::LAYER_PINNED_OVERRIDE) ||
                          (slots[i].is_pointer_state != 0u) ||
                          (status_mask && i < status_cap && (status_mask[i] & (SLOT_STATUS_PINNED | SLOT_STATUS_POINTER_STATE)) != 0u);
            } else {
                flagged = (slots[i].has_conflict != 0u) ||
                          slots[i].domain.empty() ||
                          (status_mask && i < status_cap && (status_mask[i] & SLOT_STATUS_HAS_CONFLICT) != 0u);
            }
            if (flagged) {
                partitions[i].nearest_conflict_slot   = i;
                partitions[i].predecessor_toward_conf = i;
                partitions[i].hop_distance            = 0u;
                partitions[i].role_flags              = (seed_from_pinned_landmarks != 0u)
                    ? HORIZON_ROLE_POINTER_STATE
                    : HORIZON_ROLE_CONFLICT_CENTER;
                bfs_queue[q_tail++]                   = i;
                ++box_count;
            }
        }

        while (q_head < q_tail && box_count < max_box) {
            const uint32_t u    = bfs_queue[q_head++];
            const uint16_t dist = partitions[u].hop_distance;
            if (dist >= max_radius) {
                continue;
            }

            auto try_visit = [&](uint32_t v) {
                if (v >= n_slots || box_count >= max_box || q_tail >= q_cap) return;
                if (partitions[v].hop_distance != 0xFFFFu) return;

                const uint16_t next_dist = static_cast<uint16_t>(dist + 1u);
                partitions[v].hop_distance            = next_dist;
                partitions[v].nearest_conflict_slot   = partitions[u].nearest_conflict_slot;
                partitions[v].predecessor_toward_conf = u;
                partitions[v].role_flags              = (next_dist == max_radius)
                    ? HORIZON_ROLE_PERIMETER
                    : HORIZON_ROLE_INTERIOR;
                ++box_count;
                if (next_dist < max_radius) {
                    bfs_queue[q_tail++] = v;
                }
            };

            const EnvironmentSlotPOD &u_slot = slots[u];
            if (u_slot.parent_slot_id < n_slots) {
                const uint32_t p_id = u_slot.parent_slot_id;
                try_visit(p_id);
                if (child_csr) {
                    const EnvironmentSlotPOD &p_slot = slots[p_id];
                    if (p_slot.children_count > 0u &&
                        (p_slot.children_csr_offset + p_slot.children_count) <= n_child_csr) {
                        for (uint32_t k = 0; k < p_slot.children_count; ++k) {
                            try_visit(child_csr[p_slot.children_csr_offset + k]);
                        }
                    }
                }
            }
            if (child_csr && u_slot.children_count > 0u &&
                (u_slot.children_csr_offset + u_slot.children_count) <= n_child_csr) {
                for (uint32_t k = 0; k < u_slot.children_count; ++k) {
                    try_visit(child_csr[u_slot.children_csr_offset + k]);
                }
            }
            if (spatial_csr && u_slot.spatial_neighbor_count > 0u &&
                static_cast<uint32_t>(u_slot.spatial_csr_offset + u_slot.spatial_neighbor_count) <= n_spatial_csr) {
                for (uint16_t k = 0; k < u_slot.spatial_neighbor_count; ++k) {
                    try_visit(spatial_csr[u_slot.spatial_csr_offset + k].neighbor_slot_id);
                }
            }
        }

        if (GrantPartPOD *target_part = const_cast<GrantPartPOD *>(p_ctx.get_grant_part(target_buffer_id))) {
            MemoryBufferSelectionPOD &sel = target_part->selection;
            const size_t sel_lim = static_cast<size_t>(std::max<int64_t>(0, sel.element_count));
            for (uint32_t i = 0; i < n_slots && i < sel_lim; ++i) {
                if (sel.lod_levels) {
                    sel.lod_levels[i] = (partitions[i].hop_distance == 0xFFFFu)
                        ? 255u
                        : static_cast<uint8_t>(std::min<uint16_t>(254u, partitions[i].hop_distance));
                }
                if (sel.group_masks) {
                    sel.group_masks[i] = static_cast<uint32_t>(partitions[i].role_flags);
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD &r_selection,
        const TaskContextPOD &p_ctx,
        T_ActiveView &p_view
    ) const noexcept {
        (void)r_selection;
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_metadata(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(ConflictHorizonBoxPartitionMetadataLogic<>) == 64, "ConflictHorizonBoxPartitionMetadataLogic must be 64 bytes!");
static_assert(IsMetadataLogic<ConflictHorizonBoxPartitionMetadataLogic<>>, "ConflictHorizonBoxPartitionMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core
