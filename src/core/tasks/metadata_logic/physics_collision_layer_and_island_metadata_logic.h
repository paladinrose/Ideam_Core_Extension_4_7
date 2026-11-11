#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * P8: PhysicsCollisionLayerAndIslandMetadataLogic
 * Synchronizes Jolt 3D collision_layer / collision_mask bitflags with r_selection.group_masks
 * and assigns spatial broadphase cell / island partition IDs into r_selection.partition_ids.
 */
struct PhysicsCollisionLayerAndIslandMetadataLogic {
    static constexpr std::string_view ui_name = "Jolt3D Collision Layer & Broadphase Island Metadata";
    static constexpr std::string_view ui_category = "Physics/Metadata";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::UINT32,
        DataType::VECTOR3
    };

    int32_t sync_mode = 0; // 0 = Read Jolt3D Collision Layer -> group_masks, 1 = Write group_masks -> Jolt3D Collision Layer
    float spatial_cell_size = 16.0f;
    bool compute_spatial_island_partitions = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("sync_mode", godot::Variant::INT, sync_mode, 2, "Read Jolt3D Layer -> group_masks,Write group_masks -> Jolt3D Layer");
        add_prop("spatial_cell_size", godot::Variant::FLOAT, spatial_cell_size);
        add_prop("compute_spatial_island_partitions", godot::Variant::BOOL, compute_spatial_island_partitions);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("sync_mode")) sync_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["sync_mode"]), 0, 1);
        if (p_props.has("spatial_cell_size")) spatial_cell_size = std::max(1e-3f, static_cast<float>(p_props["spatial_cell_size"]));
        if (p_props.has("compute_spatial_island_partitions")) compute_spatial_island_partitions = static_cast<bool>(p_props["compute_spatial_island_partitions"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& primary = p_ctx.grant->parts[0];
        MemoryBufferSelectionPOD& sel = primary.selection;
        if (!primary.raw_base_ptr || sel.capacity <= 0) return;

        godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
        if (!ps) return;

        const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), primary.element_stride);
        const int64_t n_bodies = std::min<int64_t>(sel.capacity, static_cast<int64_t>(primary.capacity_bytes / r_stride));
        const float inv_cell = 1.0f / std::max(1e-3f, spatial_cell_size);

        for (int64_t i = 0; i < n_bodies; ++i) {
            const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(primary.raw_base_ptr + static_cast<size_t>(i) * r_stride);
            if (raw_id == 0) continue;
            godot::RID body_rid;
            *(reinterpret_cast<uint64_t*>(&body_rid)) = raw_id;
            if (!body_rid.is_valid()) continue;

            if (sel.group_masks) {
                if (sync_mode == 0) {
                    sel.group_masks[i] = ps->body_get_collision_layer(body_rid);
                } else {
                    ps->body_set_collision_layer(body_rid, sel.group_masks[i]);
                }
            }

            if (compute_spatial_island_partitions && sel.partition_ids) {
                const godot::Transform3D xform = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                const int64_t cx = static_cast<int64_t>(std::floor(xform.origin.x * inv_cell));
                const int64_t cy = static_cast<int64_t>(std::floor(xform.origin.y * inv_cell));
                const int64_t cz = static_cast<int64_t>(std::floor(xform.origin.z * inv_cell));
                sel.partition_ids[i] = (cx * 73856093LL) ^ (cy * 19349663LL) ^ (cz * 83492791LL);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core