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
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class JoltJointMotorType : uint8_t {
    HINGE_JOINT           = 0,
    SLIDER_JOINT          = 1,
    GENERIC_6DOF_ANGULAR  = 2
};

/**
 * JoltJointMotorAndLimitSyncTransformLogic<T>
 * Drives Jolt 3D joints (Hinge, Slider, and 6DOF angular/linear motors, spring limits,
 * and structural break thresholds via joint_clear) from DOD control buffers.
 */
template <typename T = godot::Vector4>
struct alignas(64) JoltJointMotorAndLimitSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR4 | DataType::VECTOR3 | DataType::FLOAT32 | DataType::INT64;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Jolt3D Joint Motor, Spring & Break Sync";
    static constexpr std::string_view ui_category  = "Physics/Joints";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t control_buffer_id           = INVALID_ID; // 4B: Primary target Vector4(target_vel, max_impulse, limit_span, applied_strain)
    uint32_t joint_rid_buffer_id         = INVALID_ID; // 4B: Jolt 3D joint RIDs buffer (uint64_t handles)
    float break_threshold                = 0.0f;       // 4B: > 0 severes joint via joint_clear when control.w >= break_threshold
    JoltJointMotorType jolt_joint_type   = JoltJointMotorType::HINGE_JOINT; // 1B
    uint8_t reserved_padding[51]         = {0};        // 51B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary cbuf_prop;
        cbuf_prop["name"] = "control_buffer_id";
        cbuf_prop["type"] = godot::Variant::INT;
        cbuf_prop["hint_string"] = "buffer_option";
        props.push_back(cbuf_prop);

        godot::Dictionary rbuf_prop;
        rbuf_prop["name"] = "joint_rid_buffer_id";
        rbuf_prop["type"] = godot::Variant::INT;
        rbuf_prop["hint_string"] = "buffer_option";
        props.push_back(rbuf_prop);

        godot::Dictionary jtype_prop;
        jtype_prop["name"] = "jolt_joint_type";
        jtype_prop["type"] = godot::Variant::INT;
        jtype_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        jtype_prop["hint_string"] = "Jolt Hinge Joint:0,Jolt Slider Joint:1,Jolt 6DOF Angular Motor:2";
        props.push_back(jtype_prop);

        godot::Dictionary brk_prop;
        brk_prop["name"] = "break_threshold";
        brk_prop["type"] = godot::Variant::FLOAT;
        props.push_back(brk_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (control_buffer_id != INVALID_ID) ? control_buffer_id : joint_rid_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("control_buffer_id")) {
            control_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["control_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && control_buffer_id == INVALID_ID) {
            control_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && control_buffer_id == INVALID_ID) {
            control_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("joint_rid_buffer_id")) {
            joint_rid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["joint_rid_buffer_id"]));
        }
        if (p_props.has("jolt_joint_type")) {
            jolt_joint_type = static_cast<JoltJointMotorType>(std::clamp<int64_t>(static_cast<int64_t>(p_props["jolt_joint_type"]), 0, 2));
        }
        if (p_props.has("break_threshold")) {
            break_threshold = std::max(0.0f, static_cast<float>(p_props["break_threshold"]));
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
        if constexpr (std::is_same_v<T, godot::Vector4> || std::is_same_v<T, godot::Vector3> ||
                      std::is_same_v<T, float> || std::is_same_v<T, int64_t>) {
            const GrantPartPOD* rid_part  = p_context.get_grant_part(joint_rid_buffer_id);
            const GrantPartPOD* ctrl_part = p_context.get_grant_part(control_buffer_id);
            if (!rid_part || !ctrl_part || !rid_part->raw_base_ptr || !ctrl_part->raw_base_ptr) return;

            godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
            if (!ps) return;

            const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part->element_stride);
            const uint32_t c_stride = std::max<uint32_t>(sizeof(godot::Vector4), ctrl_part->element_stride);
            const int64_t n_joints = std::min<int64_t>(
                static_cast<int64_t>(rid_part->capacity_bytes / r_stride),
                static_cast<int64_t>(ctrl_part->capacity_bytes / c_stride)
            );

            for (int64_t i = 0; i < n_joints; ++i) {
                uint64_t& raw_id = *reinterpret_cast<uint64_t*>(rid_part->raw_base_ptr + static_cast<size_t>(i) * r_stride);
                if (raw_id == 0) continue;
                godot::RID j_rid;
                std::memcpy(&j_rid, &raw_id, sizeof(uint64_t));
                if (!j_rid.is_valid()) continue;

                const godot::Vector4& ctrl = *reinterpret_cast<const godot::Vector4*>(ctrl_part->raw_base_ptr + static_cast<size_t>(i) * c_stride);

                if (break_threshold > 0.0f && ctrl.w >= break_threshold) {
                    ps->joint_clear(j_rid);
                    raw_id = 0;
                    continue;
                }

                if (jolt_joint_type == JoltJointMotorType::HINGE_JOINT) {
                    ps->hinge_joint_set_flag(j_rid, godot::PhysicsServer3D::HINGE_JOINT_FLAG_ENABLE_MOTOR, std::abs(ctrl.y) > 1e-6f);
                    ps->hinge_joint_set_param(j_rid, godot::PhysicsServer3D::HINGE_JOINT_MOTOR_TARGET_VELOCITY, ctrl.x);
                    ps->hinge_joint_set_param(j_rid, godot::PhysicsServer3D::HINGE_JOINT_MOTOR_MAX_IMPULSE, ctrl.y);
                    if (ctrl.z > 0.0f) {
                        ps->hinge_joint_set_flag(j_rid, godot::PhysicsServer3D::HINGE_JOINT_FLAG_USE_LIMIT, true);
                        ps->hinge_joint_set_param(j_rid, godot::PhysicsServer3D::HINGE_JOINT_LIMIT_LOWER, -ctrl.z);
                        ps->hinge_joint_set_param(j_rid, godot::PhysicsServer3D::HINGE_JOINT_LIMIT_UPPER, +ctrl.z);
                    }
                } else if (jolt_joint_type == JoltJointMotorType::SLIDER_JOINT) {
                    if (ctrl.z > 0.0f) {
                        ps->slider_joint_set_param(j_rid, godot::PhysicsServer3D::SLIDER_JOINT_LINEAR_LIMIT_LOWER, -ctrl.z);
                        ps->slider_joint_set_param(j_rid, godot::PhysicsServer3D::SLIDER_JOINT_LINEAR_LIMIT_UPPER, +ctrl.z);
                    }
                } else {
                    for (int axis = 0; axis < 3; ++axis) {
                        const auto v_axis = static_cast<godot::Vector3::Axis>(axis);
                        ps->generic_6dof_joint_set_flag(j_rid, v_axis, godot::PhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_MOTOR, std::abs(ctrl.y) > 1e-6f);
                        ps->generic_6dof_joint_set_param(j_rid, v_axis, godot::PhysicsServer3D::G6DOF_JOINT_ANGULAR_MOTOR_TARGET_VELOCITY, ctrl.x);
                        ps->generic_6dof_joint_set_param(j_rid, v_axis, godot::PhysicsServer3D::G6DOF_JOINT_ANGULAR_MOTOR_FORCE_LIMIT, ctrl.y);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(JoltJointMotorAndLimitSyncTransformLogic<>) == 64, "JoltJointMotorAndLimitSyncTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<JoltJointMotorAndLimitSyncTransformLogic<>>, "JoltJointMotorAndLimitSyncTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core