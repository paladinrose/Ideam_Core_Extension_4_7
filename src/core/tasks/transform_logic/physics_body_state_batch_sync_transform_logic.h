#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class PhysicsBodySyncDirection : uint8_t {
    IMPORT_JOLT_TO_DOD = 0,
    EXPORT_DOD_TO_JOLT = 1
};

/**
 * PhysicsBodyStateBatchSyncTransformLogic<T> (P1/P2)
 * Bi-directional batch synchronization between DOD buffers (Transform3D or position /
 * linear_velocity / angular_velocity Vector3 columns) and Jolt 3D body RIDs via PhysicsServer3D.
 */
template <typename T = godot::Vector3>
struct alignas(64) PhysicsBodyStateBatchSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = SoAStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::INT64 | DataType::CUSTOM;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Jolt3D Body Transform & Velocity Batch Sync";
    static constexpr std::string_view ui_category  = "Physics/Bodies";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t position_buffer_id         = INVALID_ID; // 4B: Primary target Vector3 position or Transform3D buffer
    uint32_t body_rid_buffer_id         = INVALID_ID; // 4B: Jolt 3D body RID buffer (uint64_t handles)
    uint32_t linear_velocity_buffer_id  = INVALID_ID; // 4B: Optional Vector3 linear velocity buffer
    uint32_t angular_velocity_buffer_id = INVALID_ID; // 4B: Optional Vector3 angular velocity buffer

    PhysicsBodySyncDirection sync_direction = PhysicsBodySyncDirection::IMPORT_JOLT_TO_DOD; // 1B
    bool sync_linear_velocity               = true;  // 1B
    bool sync_angular_velocity              = false; // 1B
    bool only_awake_in_selection            = true;  // 1B
    uint8_t reserved_padding[44]            = {0};   // 44B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary pos_prop;
        pos_prop["name"] = "position_buffer_id";
        pos_prop["type"] = godot::Variant::INT;
        pos_prop["hint_string"] = "buffer_option";
        props.push_back(pos_prop);

        godot::Dictionary rid_prop;
        rid_prop["name"] = "body_rid_buffer_id";
        rid_prop["type"] = godot::Variant::INT;
        rid_prop["hint_string"] = "buffer_option";
        props.push_back(rid_prop);

        godot::Dictionary lvel_prop;
        lvel_prop["name"] = "linear_velocity_buffer_id";
        lvel_prop["type"] = godot::Variant::INT;
        lvel_prop["hint_string"] = "buffer_option";
        props.push_back(lvel_prop);

        godot::Dictionary avel_prop;
        avel_prop["name"] = "angular_velocity_buffer_id";
        avel_prop["type"] = godot::Variant::INT;
        avel_prop["hint_string"] = "buffer_option";
        props.push_back(avel_prop);

        godot::Dictionary dir_prop;
        dir_prop["name"] = "sync_direction";
        dir_prop["type"] = godot::Variant::INT;
        dir_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dir_prop["hint_string"] = "Read Jolt3D -> DOD:0,Push DOD -> Jolt3D:1";
        props.push_back(dir_prop);

        godot::Dictionary lv_prop;
        lv_prop["name"] = "sync_linear_velocity";
        lv_prop["type"] = godot::Variant::BOOL;
        props.push_back(lv_prop);

        godot::Dictionary av_prop;
        av_prop["name"] = "sync_angular_velocity";
        av_prop["type"] = godot::Variant::BOOL;
        props.push_back(av_prop);

        godot::Dictionary awk_prop;
        awk_prop["name"] = "only_awake_in_selection";
        awk_prop["type"] = godot::Variant::BOOL;
        props.push_back(awk_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (position_buffer_id != INVALID_ID) ? position_buffer_id : body_rid_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("position_buffer_id")) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("body_rid_buffer_id")) {
            body_rid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["body_rid_buffer_id"]));
        }
        if (p_props.has("linear_velocity_buffer_id")) {
            linear_velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["linear_velocity_buffer_id"]));
        }
        if (p_props.has("angular_velocity_buffer_id")) {
            angular_velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["angular_velocity_buffer_id"]));
        }
        if (p_props.has("sync_direction")) {
            sync_direction = static_cast<PhysicsBodySyncDirection>(std::clamp<int64_t>(static_cast<int64_t>(p_props["sync_direction"]), 0, 1));
        }
        if (p_props.has("sync_linear_velocity")) {
            sync_linear_velocity = static_cast<bool>(p_props["sync_linear_velocity"]);
        }
        if (p_props.has("sync_angular_velocity")) {
            sync_angular_velocity = static_cast<bool>(p_props["sync_angular_velocity"]);
        }
        if (p_props.has("only_awake_in_selection")) {
            only_awake_in_selection = static_cast<bool>(p_props["only_awake_in_selection"]);
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
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, int64_t> || std::is_same_v<T, void*>) {
            const GrantPartPOD* rid_part = p_context.get_grant_part(body_rid_buffer_id);
            const GrantPartPOD* pos_part = p_context.get_grant_part(position_buffer_id);
            if (!rid_part || !pos_part || !rid_part->raw_base_ptr || !pos_part->raw_base_ptr) return;

            const GrantPartPOD* lin_vel_part = (linear_velocity_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(linear_velocity_buffer_id)
                : nullptr;
            const GrantPartPOD* ang_vel_part = (angular_velocity_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(angular_velocity_buffer_id)
                : nullptr;

            godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
            if (!ps) return;

            const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part->element_stride);
            const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const bool is_full_transform = (p_stride >= sizeof(godot::Transform3D));

            const int64_t n_bodies = std::min<int64_t>(
                static_cast<int64_t>(rid_part->capacity_bytes / r_stride),
                static_cast<int64_t>(pos_part->capacity_bytes / p_stride)
            );
            const MemoryBufferSelectionPOD& sel = pos_part->selection;

            auto sync_body = [&](int64_t i) {
                const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(rid_part->raw_base_ptr + static_cast<size_t>(i) * r_stride);
                if (raw_id == 0) return;
                godot::RID body_rid;
                std::memcpy(&body_rid, &raw_id, sizeof(uint64_t));
                if (!body_rid.is_valid()) return;

                uint8_t* pos_ptr = pos_part->raw_base_ptr + static_cast<size_t>(i) * p_stride;

                if (sync_direction == PhysicsBodySyncDirection::IMPORT_JOLT_TO_DOD) {
                    const godot::Transform3D xform = ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_TRANSFORM);
                    if (is_full_transform) {
                        *reinterpret_cast<godot::Transform3D*>(pos_ptr) = xform;
                    } else {
                        *reinterpret_cast<godot::Vector3*>(pos_ptr) = xform.origin;
                    }

                    if (sync_linear_velocity && lin_vel_part && lin_vel_part->raw_base_ptr) {
                        const uint32_t lv_stride = std::max<uint32_t>(sizeof(godot::Vector3), lin_vel_part->element_stride);
                        if (static_cast<size_t>(i + 1) * lv_stride <= lin_vel_part->capacity_bytes) {
                            *reinterpret_cast<godot::Vector3*>(lin_vel_part->raw_base_ptr + static_cast<size_t>(i) * lv_stride) =
                                ps->body_get_state(body_rid, godot::PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY);
                        }
                    }
                    if (sync_angular_velocity && ang_vel_part && ang_vel_part->raw_base_ptr) {
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

                    if (sync_linear_velocity && lin_vel_part && lin_vel_part->raw_base_ptr) {
                        const uint32_t lv_stride = std::max<uint32_t>(sizeof(godot::Vector3), lin_vel_part->element_stride);
                        if (static_cast<size_t>(i + 1) * lv_stride <= lin_vel_part->capacity_bytes) {
                            ps->body_set_state(
                                body_rid,
                                godot::PhysicsServer3D::BODY_STATE_LINEAR_VELOCITY,
                                *reinterpret_cast<const godot::Vector3*>(lin_vel_part->raw_base_ptr + static_cast<size_t>(i) * lv_stride)
                            );
                        }
                    }
                    if (sync_angular_velocity && ang_vel_part && ang_vel_part->raw_base_ptr) {
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
    }
};

static_assert(sizeof(PhysicsBodyStateBatchSyncTransformLogic<>) == 64, "PhysicsBodyStateBatchSyncTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<PhysicsBodyStateBatchSyncTransformLogic<>>, "PhysicsBodyStateBatchSyncTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core