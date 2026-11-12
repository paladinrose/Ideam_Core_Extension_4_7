#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * PhysicsCollisionLayerAndIslandMetadataLogic<T, T_Strategy, T_View> (P8)
 * Synchronizes Jolt 3D collision_layer / collision_mask bitflags with r_selection.group_masks
 * and assigns spatial broadphase cell / island partition IDs into r_selection.partition_ids.
 */
template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) PhysicsCollisionLayerAndIslandMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::UINT32 | DataType::UINT64 | DataType::VECTOR3;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Jolt3D Collision Layer & Broadphase Island Metadata";
    static constexpr std::string_view ui_category  = "Physics/Metadata";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id                 = INVALID_ID; // 4B: Body RID uint64 buffer
    int32_t sync_mode                         = 0;          // 4B: 0 = Read Layer -> group_masks, 1 = Write group_masks -> Layer
    float spatial_cell_size                   = 16.0f;      // 4B
    bool compute_spatial_island_partitions    = true;       // 1B
    uint8_t reserved_padding[51]              = {0};        // 51B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("sync_mode", godot::Variant::INT, 0, 2, "Read Jolt3D Layer -> group_masks,Write group_masks -> Jolt3D Layer");
        add_prop("spatial_cell_size", godot::Variant::FLOAT, 16.0f);
        add_prop("compute_spatial_island_partitions", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("sync_mode")) sync_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["sync_mode"]), 0, 1);
        if (p_props.has("spatial_cell_size")) spatial_cell_size = std::max(1e-3f, static_cast<float>(p_props["spatial_cell_size"]));
        if (p_props.has("compute_spatial_island_partitions")) compute_spatial_island_partitions = static_cast<bool>(p_props["compute_spatial_island_partitions"]);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* primary = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!primary || !primary->raw_base_ptr || r_selection.capacity <= 0) return;

            godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
            if (!ps) return;

            const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), primary->element_stride);
            const int64_t n_bodies = std::min<int64_t>(r_selection.capacity, static_cast<int64_t>(primary->capacity_bytes / r_stride));
            const float inv_cell = 1.0f / std::max(1e-3f, spatial_cell_size);

            for (int64_t i = 0; i < n_bodies; ++i) {
                const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(primary->raw_base_ptr + static_cast<size_t>(i) * r_stride);
                if (raw_id == 0) continue;
                godot::RID body_rid;
                std::memcpy(&body_rid, &raw_id, sizeof(uint64_t));
                if (!body_rid.is_valid()) continue;

                if (r_selection.group_masks) {
                    if (sync_mode == 0) {
                        r_selection.group_masks[i] = ps->body_get_collision_layer(body_rid);
                    } else {
                        ps->body_set_collision_layer(body_rid, r_selection.group_masks[i]);
                    }
                }

                if (compute_spatial_island_partitions && r_selection.partition_ids) {
                    const godot::Transform3D xform = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                    const int64_t cx = static_cast<int64_t>(std::floor(xform.origin.x * inv_cell));
                    const int64_t cy = static_cast<int64_t>(std::floor(xform.origin.y * inv_cell));
                    const int64_t cz = static_cast<int64_t>(std::floor(xform.origin.z * inv_cell));
                    r_selection.partition_ids[i] = (cx * 73856093LL) ^ (cy * 19349663LL) ^ (cz * 83492791LL);
                }
            }
        }
    }
};

static_assert(sizeof(PhysicsCollisionLayerAndIslandMetadataLogic<>) == 64, "PhysicsCollisionLayerAndIslandMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<PhysicsCollisionLayerAndIslandMetadataLogic<>>, "PhysicsCollisionLayerAndIslandMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core