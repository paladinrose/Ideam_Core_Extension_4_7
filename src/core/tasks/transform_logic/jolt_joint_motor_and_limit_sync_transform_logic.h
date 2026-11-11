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
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * P4: JoltJointMotorAndLimitSyncTransformLogic
 * Drives Jolt 3D joints (Hinge, Slider, and 6DOF angular/linear motors, spring stiffness/damping,
 * and structural break thresholds via joint_clear) from DOD control buffers, omitting deprecated
 * legacy GodotPhysics pin-joint bias/damping parameters.
 */
struct JoltJointMotorAndLimitSyncTransformLogic {
    static constexpr std::string_view ui_name = "Jolt3D Joint Motor, Spring & Break Sync";
    static constexpr std::string_view ui_category = "Physics/Joints";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::VECTOR4,
        DataType::VECTOR3,
        DataType::FLOAT32
    };

    int32_t jolt_joint_type = 0; // 0 = Hinge (JoltHingeJoint3D), 1 = Slider (JoltSliderJoint3D), 2 = 6DOF Angular (JoltGeneric6DOFJoint3D)
    float break_threshold = 0.0f; // > 0 severes joint via joint_clear when control/strain channel .w exceeds break_threshold

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
        add_prop("jolt_joint_type", godot::Variant::INT, jolt_joint_type, 2, "Jolt Hinge Joint,Jolt Slider Joint,Jolt 6DOF Angular Motor");
        add_prop("break_threshold", godot::Variant::FLOAT, break_threshold);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("jolt_joint_type")) jolt_joint_type = std::clamp<int32_t>(static_cast<int32_t>(p_props["jolt_joint_type"]), 0, 2);
        if (p_props.has("break_threshold")) break_threshold = std::max(0.0f, static_cast<float>(p_props["break_threshold"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& rid_part = p_ctx.grant->parts[0];
        const GrantPartPOD& ctrl_part = p_ctx.grant->parts[1]; // Vector4(target_vel, max_impulse, limit_span, applied_strain)
        if (!rid_part.raw_base_ptr || !ctrl_part.raw_base_ptr) return;

        godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
        if (!ps) return;

        const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part.element_stride);
        const uint32_t c_stride = std::max<uint32_t>(sizeof(godot::Vector4), ctrl_part.element_stride);
        const int64_t n_joints = std::min<int64_t>(
            static_cast<int64_t>(rid_part.capacity_bytes / r_stride),
            static_cast<int64_t>(ctrl_part.capacity_bytes / c_stride)
        );

        for (int64_t i = 0; i < n_joints; ++i) {
            uint64_t& raw_id = *reinterpret_cast<uint64_t*>(rid_part.raw_base_ptr + static_cast<size_t>(i) * r_stride);
            if (raw_id == 0) continue;
            godot::RID j_rid;
            *(reinterpret_cast<uint64_t*>(&j_rid)) = raw_id;
            if (!j_rid.is_valid()) continue;

            const godot::Vector4& ctrl = *reinterpret_cast<const godot::Vector4*>(ctrl_part.raw_base_ptr + static_cast<size_t>(i) * c_stride);

            // Check structural joint breaking threshold
            if (break_threshold > 0.0f && ctrl.w >= break_threshold) {
                ps->joint_clear(j_rid);
                raw_id = 0;
                continue;
            }

            if (jolt_joint_type == 0) {
                // JoltHingeJoint3D
                ps->hinge_joint_set_flag(j_rid, godot::PhysicsServer3D::HINGE_JOINT_FLAG_ENABLE_MOTOR, std::abs(ctrl.y) > 1e-6f);
                ps->hinge_joint_set_param(j_rid, godot::PhysicsServer3D::HINGE_JOINT_MOTOR_TARGET_VELOCITY, ctrl.x);
                ps->hinge_joint_set_param(j_rid, godot::PhysicsServer3D::HINGE_JOINT_MOTOR_MAX_IMPULSE, ctrl.y);
                if (ctrl.z > 0.0f) {
                    ps->hinge_joint_set_flag(j_rid, godot::PhysicsServer3D::HINGE_JOINT_FLAG_USE_LIMIT, true);
                    ps->hinge_joint_set_param(j_rid, godot::PhysicsServer3D::HINGE_JOINT_LIMIT_LOWER, -ctrl.z);
                    ps->hinge_joint_set_param(j_rid, godot::PhysicsServer3D::HINGE_JOINT_LIMIT_UPPER, +ctrl.z);
                }
            } else if (jolt_joint_type == 1) {
                // JoltSliderJoint3D
                if (ctrl.z > 0.0f) {
                    ps->slider_joint_set_param(j_rid, godot::PhysicsServer3D::SLIDER_JOINT_LINEAR_LIMIT_LOWER, -ctrl.z);
                    ps->slider_joint_set_param(j_rid, godot::PhysicsServer3D::SLIDER_JOINT_LINEAR_LIMIT_UPPER, +ctrl.z);
                }
            } else {
                // JoltGeneric6DOFJoint3D (X/Y/Z angular motors)
                for (int axis = 0; axis < 3; ++axis) {
                    const auto v_axis = static_cast<godot::Vector3::Axis>(axis);
                    ps->generic_6dof_joint_set_flag(j_rid, v_axis, godot::PhysicsServer3D::G6DOF_JOINT_FLAG_ENABLE_MOTOR, std::abs(ctrl.y) > 1e-6f);
                    ps->generic_6dof_joint_set_param(j_rid, v_axis, godot::PhysicsServer3D::G6DOF_JOINT_ANGULAR_MOTOR_TARGET_VELOCITY, ctrl.x);
                    ps->generic_6dof_joint_set_param(j_rid, v_axis, godot::PhysicsServer3D::G6DOF_JOINT_ANGULAR_MOTOR_FORCE_LIMIT, ctrl.y);
                }
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core