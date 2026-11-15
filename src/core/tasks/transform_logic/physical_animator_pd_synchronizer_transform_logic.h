#pragma once

/**
 * physical_animator_pd_synchronizer_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`P14`) for `@Physics`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.2 — `P14` `PhysicalAnimator.cs` Port)
 * ============================================================================
 * Ports `PhysicalAnimator.cs` (`SynchronizerSettings`) into a 64-byte DOD task:
 *   - Computes Proportional-Derivative (PD) linear forces and angular torques driving
 *     active-ragdoll physics bones toward animated target bone poses:
 *       * `F_linear  = clamp_magnitude(k_p * (x_target - x_cur) - k_d * v_linear,  min_force,  max_force)`
 *       * `T_angular = clamp_magnitude(k_r * axis_angle(q_target * q_cur^-1) - k_dr * v_angular, min_torque, max_torque)`
 *   - Supports both direct DOD velocity/force buffer integration (`linear_velocity_buffer_id`,
 *     `angular_velocity_buffer_id`) and live Jolt/Godot 3D rigid body impulse dispatch via
 *     `PhysicsServer3D` when `body_rid_buffer_id` is bound.
 */

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../characters/character_common.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace ideam::core {

template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) PhysicalAnimatorPDSynchronizerTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::VECTOR3 | DataType::VECTOR3D | DataType::VECTOR4 | DataType::BYTE | DataType::FLOAT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Physical Animator Active-Ragdoll PD Synchronizer";
    static constexpr std::string_view ui_category  = "Physics/Bodies";

    uint32_t current_position_buffer_id = INVALID_ID; ///< Active ragdoll bone positions (`Vector3`)
    uint32_t target_position_buffer_id  = INVALID_ID; ///< Animated target bone positions (`Vector3`)
    uint32_t current_rotation_buffer_id = INVALID_ID; ///< Active ragdoll bone rotations (`Vector4` Quaternion)
    uint32_t target_rotation_buffer_id  = INVALID_ID; ///< Animated target bone rotations (`Vector4` Quaternion)
    uint32_t linear_velocity_buffer_id  = INVALID_ID; ///< Linear velocities (`Vector3`)
    uint32_t angular_velocity_buffer_id = INVALID_ID; ///< Angular velocities (`Vector3`)
    uint32_t body_rid_buffer_id         = INVALID_ID; ///< Optional `uint64_t` Jolt3D `RID` handles
    uint32_t sync_pod_buffer_id         = INVALID_ID; ///< Optional per-bone `PhysicalAnimatorSyncPod` buffer

    float position_spring     = 800.0f;  ///< 4B: `SynchronizerSettings.positionSpring`
    float position_damper     = 45.0f;   ///< 4B: `SynchronizerSettings.positionDamper`
    float position_min_force  = 0.0f;    ///< 4B: `SynchronizerSettings.positionMinForce`
    float position_max_force  = 2500.0f; ///< 4B: `SynchronizerSettings.positionMaxForce`
    float rotation_spring     = 400.0f;  ///< 4B: `SynchronizerSettings.rotationSpring`
    float rotation_damper     = 25.0f;   ///< 4B: `SynchronizerSettings.rotationDamper`
    float rotation_min_torque = 0.0f;    ///< 4B: `SynchronizerSettings.rotationMinForce`
    float rotation_max_torque = 1200.0f; ///< 4B: `SynchronizerSettings.rotationMaxForce` -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("current_position_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("target_position_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("current_rotation_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("target_rotation_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("linear_velocity_buffer_id",  godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("angular_velocity_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("body_rid_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("sync_pod_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("position_spring",            godot::Variant::FLOAT, 800.0);
        add_prop("position_damper",            godot::Variant::FLOAT, 45.0);
        add_prop("position_min_force",         godot::Variant::FLOAT, 0.0);
        add_prop("position_max_force",         godot::Variant::FLOAT, 2500.0);
        add_prop("rotation_spring",            godot::Variant::FLOAT, 400.0);
        add_prop("rotation_damper",            godot::Variant::FLOAT, 25.0);
        add_prop("rotation_min_torque",        godot::Variant::FLOAT, 0.0);
        add_prop("rotation_max_torque",        godot::Variant::FLOAT, 1200.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (current_position_buffer_id != INVALID_ID) return current_position_buffer_id;
        if (linear_velocity_buffer_id != INVALID_ID)  return linear_velocity_buffer_id;
        return current_rotation_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("current_position_buffer_id"))      current_position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["current_position_buffer_id"]));
        else if (p_props.has("target_buffer_id"))           current_position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("target_position_buffer_id"))       target_position_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_position_buffer_id"]));
        if (p_props.has("current_rotation_buffer_id"))      current_rotation_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["current_rotation_buffer_id"]));
        if (p_props.has("target_rotation_buffer_id"))       target_rotation_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_rotation_buffer_id"]));
        if (p_props.has("linear_velocity_buffer_id"))       linear_velocity_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["linear_velocity_buffer_id"]));
        if (p_props.has("angular_velocity_buffer_id"))      angular_velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["angular_velocity_buffer_id"]));
        if (p_props.has("body_rid_buffer_id"))              body_rid_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["body_rid_buffer_id"]));
        if (p_props.has("sync_pod_buffer_id"))              sync_pod_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["sync_pod_buffer_id"]));
        if (p_props.has("position_spring"))                 position_spring            = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["position_spring"])));
        if (p_props.has("position_damper"))                 position_damper            = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["position_damper"])));
        if (p_props.has("position_min_force"))              position_min_force         = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["position_min_force"])));
        if (p_props.has("position_max_force"))              position_max_force         = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["position_max_force"])));
        if (p_props.has("rotation_spring"))                 rotation_spring            = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["rotation_spring"])));
        if (p_props.has("rotation_damper"))                 rotation_damper            = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["rotation_damper"])));
        if (p_props.has("rotation_min_torque"))             rotation_min_torque        = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["rotation_min_torque"])));
        if (p_props.has("rotation_max_torque"))             rotation_max_torque        = std::max(0.0f, static_cast<float>(static_cast<double>(p_props["rotation_max_torque"])));
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
                    uint8_t *raw = buf->master_block_ptr + buf->memory_offset;
                    r_cap = static_cast<size_t>(std::max<int64_t>(0, buf->current_count > 0 ? buf->current_count : buf->max_elements));
                    return reinterpret_cast<T_Elem *>(raw);
                }
            }
        }
        return nullptr;
    }

    [[nodiscard]] static godot::Vector3 _clamp_vector_magnitude(
        const godot::Vector3 &p_vec,
        float p_min_mag,
        float p_max_mag
    ) noexcept {
        const float len = p_vec.length();
        if (len < 1e-6f) return godot::Vector3(0.0f, 0.0f, 0.0f);
        const float clamped_len = std::clamp(len, p_min_mag, std::max(p_min_mag, p_max_mag));
        return (p_vec / len) * clamped_len;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t cpos_cap = 0, tpos_cap = 0, crot_cap = 0, trot_cap = 0, lvel_cap = 0, avel_cap = 0, rid_cap = 0, pod_cap = 0;
        auto *cur_pos        = _resolve_ptr<godot::Vector3>(p_ctx, current_position_buffer_id, cpos_cap);
        const auto *tgt_pos  = _resolve_ptr<godot::Vector3>(p_ctx, target_position_buffer_id, tpos_cap);
        auto *cur_rot        = _resolve_ptr<godot::Vector4>(p_ctx, current_rotation_buffer_id, crot_cap);
        const auto *tgt_rot  = _resolve_ptr<godot::Vector4>(p_ctx, target_rotation_buffer_id, trot_cap);
        auto *lin_vel        = _resolve_ptr<godot::Vector3>(p_ctx, linear_velocity_buffer_id, lvel_cap);
        auto *ang_vel        = _resolve_ptr<godot::Vector3>(p_ctx, angular_velocity_buffer_id, avel_cap);
        const auto *rids     = _resolve_ptr<uint64_t>(p_ctx, body_rid_buffer_id, rid_cap);
        const auto *sync_cfg = _resolve_ptr<PhysicalAnimatorSyncPod>(p_ctx, sync_pod_buffer_id, pod_cap);

        const size_t n_bones = std::max(
            std::min<size_t>(cpos_cap, tpos_cap),
            std::min<size_t>(crot_cap, trot_cap)
        );
        if (n_bones == 0u) return;

        const float dt = (p_ctx.delta > 1e-5) ? std::min(0.05f, static_cast<float>(p_ctx.delta)) : (1.0f / 60.0f);
        godot::PhysicsServer3D *ps = godot::PhysicsServer3D::get_singleton();

        for (size_t i = 0; i < n_bones; ++i) {
            float kp  = position_spring;
            float kd  = position_damper;
            float fmn = position_min_force;
            float fmx = position_max_force;
            float kr  = rotation_spring;
            float kdr = rotation_damper;
            float tmn = rotation_min_torque;
            float tmx = rotation_max_torque;
            float w   = 1.0f;

            if (sync_cfg && i < pod_cap) {
                const PhysicalAnimatorSyncPod &cfg = sync_cfg[i];
                if (cfg.is_active == 0u) continue;
                kp  = cfg.position_spring;
                kd  = cfg.position_damper;
                fmn = cfg.position_min_force;
                fmx = cfg.position_max_force;
                kr  = cfg.rotation_spring;
                kdr = cfg.rotation_damper;
                tmn = cfg.rotation_min_torque;
                tmx = cfg.rotation_max_torque;
                w   = std::clamp(cfg.blend_weight, 0.0f, 1.0f);
            }

            godot::Vector3 linear_force(0.0f, 0.0f, 0.0f);
            if (cur_pos && tgt_pos && i < cpos_cap && i < tpos_cap) {
                const godot::Vector3 v = (lin_vel && i < lvel_cap) ? lin_vel[i] : godot::Vector3(0.0f, 0.0f, 0.0f);
                const godot::Vector3 raw_f = (tgt_pos[i] - cur_pos[i]) * kp - v * kd;
                linear_force = _clamp_vector_magnitude(raw_f * w, fmn, fmx);

                if (lin_vel && i < lvel_cap) {
                    lin_vel[i] += linear_force * dt;
                    cur_pos[i] += lin_vel[i] * dt;
                } else {
                    cur_pos[i] = cur_pos[i].lerp(tgt_pos[i], std::clamp(kp * dt * dt * w, 0.0f, 1.0f));
                }
            }

            godot::Vector3 angular_torque(0.0f, 0.0f, 0.0f);
            if (cur_rot && tgt_rot && i < crot_cap && i < trot_cap) {
                const godot::Quaternion qc(cur_rot[i].x, cur_rot[i].y, cur_rot[i].z, cur_rot[i].w);
                const godot::Quaternion qt(tgt_rot[i].x, tgt_rot[i].y, tgt_rot[i].z, tgt_rot[i].w);
                godot::Quaternion q_err = (qt.normalized() * qc.normalized().inverse()).normalized();
                if (q_err.w < 0.0f) {
                    q_err = -q_err;
                }
                const godot::Vector3 axis_angle = q_err.get_axis() * q_err.get_angle();
                const godot::Vector3 omega = (ang_vel && i < avel_cap) ? ang_vel[i] : godot::Vector3(0.0f, 0.0f, 0.0f);
                const godot::Vector3 raw_t = axis_angle * kr - omega * kdr;
                angular_torque = _clamp_vector_magnitude(raw_t * w, tmn, tmx);

                if (ang_vel && i < avel_cap) {
                    ang_vel[i] += angular_torque * dt;
                }
                const float slerp_t = std::clamp(kr * dt * dt * w, 0.0f, 1.0f);
                const godot::Quaternion q_next = qc.normalized().slerp(qt.normalized(), slerp_t).normalized();
                cur_rot[i] = godot::Vector4(q_next.x, q_next.y, q_next.z, q_next.w);
            }

            // If live Jolt3D body RIDs are bound, apply PD impulse and torque impulse
            if (rids && ps && i < rid_cap && rids[i] != 0ULL) {
                godot::RID body_rid;
                std::memcpy(&body_rid, &rids[i], sizeof(uint64_t));
                if (body_rid.is_valid()) {
                    ps->body_apply_central_impulse(body_rid, linear_force * dt);
                    ps->body_apply_torque_impulse(body_rid, angular_torque * dt);
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(PhysicalAnimatorPDSynchronizerTransformLogic<>) == 64, "PhysicalAnimatorPDSynchronizerTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<PhysicalAnimatorPDSynchronizerTransformLogic<>>, "PhysicalAnimatorPDSynchronizerTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
