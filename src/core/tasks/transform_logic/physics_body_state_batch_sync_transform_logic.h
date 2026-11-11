#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * P1: PhysicsBodyStateBatchSyncTransformLogic
 * Bi-directional batch synchronization between DOD buffers (Transform3D or position/linear_velocity/angular_velocity
 * Vector3 columns) and Jolt 3D body RIDs via PhysicsServer3D.
 */
struct PhysicsBodyStateBatchSyncTransformLogic {
    static constexpr std::string_view ui_name = "Jolt3D Body Transform & Velocity Batch Sync";
    static constexpr std::string_view ui_category = "Physics/Bodies";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::UINT32
    };

    int32_t sync_direction = 0; // 0 = Read Jolt3D -> DOD Buffers, 1 = Push DOD Buffers -> Jolt3D (Kinematic/Teleport)
    bool sync_linear_velocity = true;
    bool sync_angular_velocity = false;
    bool only_awake_in_selection = true;

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
        add_prop("sync_direction", godot::Variant::INT, sync_direction, 2, "Read Jolt3D -> DOD,Push DOD -> Jolt3D");
        add_prop("sync_linear_velocity", godot::Variant::BOOL, sync_linear_velocity);
        add_prop("sync_angular_velocity", godot::Variant::BOOL, sync_angular_velocity);
        add_prop("only_awake_in_selection", godot::Variant::BOOL, only_awake_in_selection);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("sync_direction")) sync_direction = std::clamp<int32_t>(static_cast<int32_t>(p_props["sync_direction"]), 0, 1);
        if (p_props.has("sync_linear_velocity")) sync_linear_velocity = static_cast<bool>(p_props["sync_linear_velocity"]);
        if (p_props.has("sync_angular_velocity")) sync_angular_velocity = static_cast<bool>(p_props["sync_angular_velocity"]);
        if (p_props.has("only_awake_in_selection")) only_awake_in_selection = static_cast<bool>(p_props["only_awake_in_selection"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& rid_part = p_ctx.grant->parts[0];
        GrantPartPOD& pos_part = p_ctx.grant->parts[1];
        if (!rid_part.raw_base_ptr || !pos_part.raw_base_ptr) return;

        GrantPartPOD* lin_vel_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;
        GrantPartPOD* ang_vel_part = (p_ctx.grant->part_count > 3 && p_ctx.grant->parts[3].raw_base_ptr)
            ? &p_ctx.grant->parts[3]
            : nullptr;

        godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
        if (!ps) return;

        const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part.element_stride);
        const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const bool is_full_transform = (p_stride >= sizeof(godot::Transform3D));

        const int64_t n_bodies = std::min<int64_t>(
            static_cast<int64_t>(rid_part.capacity_bytes / r_stride),
            static_cast<int64_t>(pos_part.capacity_bytes / p_stride)
        );
        const MemoryBufferSelectionPOD& sel = pos_part.selection;

        auto sync_body = [&](int64_t i) {
            const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(rid_part.raw_base_ptr + static_cast<size_t>(i) * r_stride);
            if (raw_id == 0) return;
            godot::RID body_rid;
            *(reinterpret_cast<uint64_t*>(&body_rid)) = raw_id;
            if (!body_rid.is_valid()) return;

            uint8_t* pos_ptr = pos_part.raw_base_ptr + static_cast<size_t>(i) * p_stride;

            if (sync_direction == 0) {
                const godot::Transform3D xform = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                if (is_full_transform) {
                    *reinterpret_cast<godot::Transform3D*>(pos_ptr) = xform;
                } else {
                    *reinterpret_cast<godot::Vector3*>(pos_ptr) = xform.origin;
                }

                if (sync_linear_velocity && lin_vel_part) {
                    const uint32_t lv_stride = std::max<uint32_t>(sizeof(godot::Vector3), lin_vel_part->element_stride);
                    if (static_cast<size_t>(i + 1) * lv_stride <= lin_vel_part->capacity_bytes) {
                        *reinterpret_cast<godot::Vector3*>(lin_vel_part->raw_base_ptr + static_cast<size_t>(i) * lv_stride) =
                            ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY);
                    }
                }
                if (sync_angular_velocity && ang_vel_part) {
                    const uint32_t av_stride = std::max<uint32_t>(sizeof(godot::Vector3), ang_vel_part->element_stride);
                    if (static_cast<size_t>(i + 1) * av_stride <= ang_vel_part->capacity_bytes) {
                        *reinterpret_cast<godot::Vector3*>(ang_vel_part->raw_base_ptr + static_cast<size_t>(i) * av_stride) =
                            ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_ANGULAR_VELOCITY);
                    }
                }
            } else {
                if (is_full_transform) {
                    ps->body_set_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM, *reinterpret_cast<const godot::Transform3D*>(pos_ptr));
                } else {
                    godot::Transform3D xform = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                    xform.origin = *reinterpret_cast<const godot::Vector3*>(pos_ptr);
                    ps->body_set_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM, xform);
                }

                if (sync_linear_velocity && lin_vel_part) {
                    const uint32_t lv_stride = std::max<uint32_t>(sizeof(godot::Vector3), lin_vel_part->element_stride);
                    if (static_cast<size_t>(i + 1) * lv_stride <= lin_vel_part->capacity_bytes) {
                        ps->body_set_state(
                            body_rid,
                            godot::PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY,
                            *reinterpret_cast<const godot::Vector3*>(lin_vel_part->raw_base_ptr + static_cast<size_t>(i) * lv_stride)
                        );
                    }
                }
                if (sync_angular_velocity && ang_vel_part) {
                    const uint32_t av_stride = std::max<uint32_t>(sizeof(godot::Vector3), ang_vel_part->element_stride);
                    if (static_cast<size_t>(i + 1) * av_stride <= ang_vel_part->capacity_bytes) {
                        ps->body_set_state(
                            body_rid,
                            godot::PhysicsServer3D::BODY_STATE_ANGULAR_VELOCITY,
                            *reinterpret_cast<const godot::Vector3*>(ang_vel_part->raw_base_ptr + static_cast<size_t>(i) * av_stride)
                        );
                    }
                }
            }
        };

        if (only_awake_in_selection && sel.mode == SelectionMode::DENSE && sel.data.bitset) {
            const int64_t limit = std::min<int64_t>(n_bodies, sel.capacity);
            for (int64_t i = 0; i < limit; ++i) {
                if (sel.data.bitset[i >> 6] & (1ULL << (i & 63))) {
                    sync_body(i);
                }
            }
        } else if (only_awake_in_selection && sel.mode == SelectionMode::SPARSE && sel.data.indices) {
            for (int64_t k = 0; k < sel.element_count; ++k) {
                const int64_t idx = sel.data.indices[k];
                if (idx >= 0 && idx < n_bodies) sync_body(idx);
            }
        } else {
            for (int64_t i = 0; i < n_bodies; ++i) {
                sync_body(i);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core