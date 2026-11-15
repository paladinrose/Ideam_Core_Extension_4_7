#pragma once

/**
 * omni_rotation_constraint_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`M45`) for `@Model/Rigging`.
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (SECTION 2.2 — `M45` `OmniRotationConstraint.cs` Port)
 * ============================================================================
 * Ports the Unity Burst `OmniRotationJob` (`OmniRotationConstraint.cs`) into the
 * IDEAM DOD pipeline:
 *   - Evaluates per-axis `bindX / bindY / bindZ` (`axis_bind_mask`), `invertX / invertY / invertZ`
 *     (`axis_invert_mask`), `isLocal` (`is_local_space`), `angleOffset` (`angle_offset_deg`),
 *     and `blend_weight` Euler rotation constraints over skeleton bone rotation buffers
 *     (`godot::Vector4` Quaternions `(x, y, z, w)` or `godot::Vector3` Euler radians)
 *     and optional `OmniRotationConstraintPod` per-bone descriptor tables.
 */

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"
#include "../../characters/character_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = godot::Vector4,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) OmniRotationConstraintTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::VECTOR4 | DataType::VECTOR4D | DataType::VECTOR3 | DataType::VECTOR3D | DataType::BYTE | DataType::INT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Omni Rotation Euler Bone Constraint";
    static constexpr std::string_view ui_category  = "Model/Rigging";

    uint32_t target_rotation_buffer_id   = INVALID_ID;     ///< Target bone rotations (`Vector4` Quaternion or `Vector3` Euler)
    uint32_t source_rotation_buffer_id   = INVALID_ID;     ///< Source bone rotations (`Vector4` Quaternion or `Vector3` Euler)
    uint32_t constraint_pod_buffer_id    = INVALID_ID;     ///< Optional `OmniRotationConstraintPod` buffer
    uint32_t parent_rotation_buffer_id   = INVALID_ID;     ///< Optional parent bone rotation buffer when `is_local == false`
    godot::Vector3 angle_offset_deg{0.0f, 0.0f, 0.0f};     ///< 12B: Default `(X, Y, Z)` Euler angle offset in degrees
    float    blend_weight                = 1.0f;           ///< 4B: Constraint blend factor in `[0, 1]`
    uint8_t  axis_bind_mask              = OMNI_AXIS_ALL;  ///< 1B: Bit 0 = bindX, Bit 1 = bindY, Bit 2 = bindZ
    uint8_t  axis_invert_mask            = OMNI_AXIS_NONE; ///< 1B: Bit 0 = invertX, Bit 1 = invertY, Bit 2 = invertZ
    uint8_t  is_local_space              = 1u;             ///< 1B: `1` = local space, `0` = world/component space
    uint8_t  reserved_padding[29]        = {0};            ///< 29B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_rotation_buffer_id", godot::Variant::INT,     static_cast<int64_t>(INVALID_ID));
        add_prop("source_rotation_buffer_id", godot::Variant::INT,     static_cast<int64_t>(INVALID_ID));
        add_prop("constraint_pod_buffer_id",  godot::Variant::INT,     static_cast<int64_t>(INVALID_ID));
        add_prop("parent_rotation_buffer_id", godot::Variant::INT,     static_cast<int64_t>(INVALID_ID));
        add_prop("angle_offset_deg",          godot::Variant::VECTOR3, godot::Vector3(0.0f, 0.0f, 0.0f));
        add_prop("blend_weight",              godot::Variant::FLOAT,   1.0);
        add_prop("axis_bind_mask",            godot::Variant::INT,     static_cast<int64_t>(OMNI_AXIS_ALL));
        add_prop("axis_invert_mask",          godot::Variant::INT,     static_cast<int64_t>(OMNI_AXIS_NONE));
        add_prop("is_local_space",            godot::Variant::BOOL,    true);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_rotation_buffer_id != INVALID_ID) ? target_rotation_buffer_id : constraint_pod_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_rotation_buffer_id"))      target_rotation_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_rotation_buffer_id"]));
        else if (p_props.has("target_buffer_id"))          target_rotation_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("source_rotation_buffer_id"))      source_rotation_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_rotation_buffer_id"]));
        if (p_props.has("constraint_pod_buffer_id"))       constraint_pod_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["constraint_pod_buffer_id"]));
        if (p_props.has("parent_rotation_buffer_id"))      parent_rotation_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_rotation_buffer_id"]));
        if (p_props.has("angle_offset_deg"))               angle_offset_deg          = static_cast<godot::Vector3>(p_props["angle_offset_deg"]);
        if (p_props.has("blend_weight"))                   blend_weight              = std::clamp(static_cast<float>(static_cast<double>(p_props["blend_weight"])), 0.0f, 1.0f);
        if (p_props.has("axis_bind_mask"))                 axis_bind_mask            = static_cast<uint8_t>(static_cast<int64_t>(p_props["axis_bind_mask"]) & 0x07);
        if (p_props.has("axis_invert_mask"))               axis_invert_mask          = static_cast<uint8_t>(static_cast<int64_t>(p_props["axis_invert_mask"]) & 0x07);
        if (p_props.has("is_local_space"))                 is_local_space            = static_cast<bool>(p_props["is_local_space"]) ? 1u : 0u;
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

    [[nodiscard]] static godot::Vector3 evaluate_constrained_euler_rad(
        const godot::Vector3 &p_src_euler_rad,
        const godot::Vector3 &p_dst_euler_rad,
        const godot::Vector3 &p_offset_deg,
        uint8_t p_bind_mask,
        uint8_t p_invert_mask,
        float p_weight
    ) noexcept {
        static constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
        const godot::Vector3 offset_rad   = p_offset_deg * DEG_TO_RAD;

        godot::Vector3 target_euler = p_dst_euler_rad;
        if ((p_bind_mask & OMNI_AXIS_X) != 0u) {
            const float sign_x = ((p_invert_mask & OMNI_AXIS_X) != 0u) ? -1.0f : 1.0f;
            target_euler.x     = p_src_euler_rad.x * sign_x + offset_rad.x;
        }
        if ((p_bind_mask & OMNI_AXIS_Y) != 0u) {
            const float sign_y = ((p_invert_mask & OMNI_AXIS_Y) != 0u) ? -1.0f : 1.0f;
            target_euler.y     = p_src_euler_rad.y * sign_y + offset_rad.y;
        }
        if ((p_bind_mask & OMNI_AXIS_Z) != 0u) {
            const float sign_z = ((p_invert_mask & OMNI_AXIS_Z) != 0u) ? -1.0f : 1.0f;
            target_euler.z     = p_src_euler_rad.z * sign_z + offset_rad.z;
        }

        const float w = std::clamp(p_weight, 0.0f, 1.0f);
        return p_dst_euler_rad.lerp(target_euler, w);
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t dst_cap = 0, src_cap = 0, pod_cap = 0;
        auto *dst_q             = _resolve_ptr<godot::Vector4>(p_ctx, target_rotation_buffer_id, dst_cap);
        const auto *src_q       = _resolve_ptr<godot::Vector4>(p_ctx, (source_rotation_buffer_id != INVALID_ID) ? source_rotation_buffer_id : target_rotation_buffer_id, src_cap);
        const auto *constraints = _resolve_ptr<OmniRotationConstraintPod>(p_ctx, constraint_pod_buffer_id, pod_cap);

        if (!dst_q || dst_cap == 0u || !src_q || src_cap == 0u) return;

        auto eval_pair = [&](uint32_t p_src_idx, uint32_t p_dst_idx, const godot::Vector3 &p_off_deg, uint8_t p_bind, uint8_t p_inv, float p_w) noexcept {
            if (p_src_idx >= src_cap || p_dst_idx >= dst_cap) return;
            const godot::Vector4 &sv = src_q[p_src_idx];
            const godot::Vector4 &dv = dst_q[p_dst_idx];

            const godot::Quaternion sq(sv.x, sv.y, sv.z, sv.w);
            const godot::Quaternion dq(dv.x, dv.y, dv.z, dv.w);

            const godot::Vector3 constrained_euler = evaluate_constrained_euler_rad(
                sq.normalized().get_euler(),
                dq.normalized().get_euler(),
                p_off_deg,
                p_bind,
                p_inv,
                p_w
            );

            const godot::Quaternion out_q = godot::Quaternion::from_euler(constrained_euler).normalized();
            dst_q[p_dst_idx] = godot::Vector4(out_q.x, out_q.y, out_q.z, out_q.w);
        };

        if (constraints && pod_cap > 0u) {
            for (size_t i = 0; i < pod_cap; ++i) {
                const OmniRotationConstraintPod &c = constraints[i];
                eval_pair(
                    c.source_bone_index,
                    c.target_bone_index,
                    godot::Vector3(c.angle_offset_deg.x, c.angle_offset_deg.y, c.angle_offset_deg.z),
                    c.axis_bind_mask,
                    c.axis_invert_mask,
                    c.blend_weight * blend_weight
                );
            }
        } else {
            const size_t n = std::min<size_t>(dst_cap, src_cap);
            for (size_t i = 0; i < n; ++i) {
                eval_pair(
                    static_cast<uint32_t>(i),
                    static_cast<uint32_t>(i),
                    angle_offset_deg,
                    axis_bind_mask,
                    axis_invert_mask,
                    blend_weight
                );
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(OmniRotationConstraintTransformLogic<>) == 64, "OmniRotationConstraintTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<OmniRotationConstraintTransformLogic<>>, "OmniRotationConstraintTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
