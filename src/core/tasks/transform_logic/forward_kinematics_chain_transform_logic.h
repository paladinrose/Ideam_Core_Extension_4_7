#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ForwardKinematicsChainTransformLogic<T>
 * Evaluates parent-to-child bone transform hierarchies in topological depth order
 * (parent_idx < child_idx, guaranteed by Godot's Skeleton3D) across contiguous bone buffers:
 *   global_pose[i] = (parent[i] >= 0) ? (global_pose[parent[i]] * local_pose[i]) : local_pose[i]
 * Supports both packed Transform3D buffers and SoA (Position Vector3, Rotation Vector4 Quaternion, Scale Vector3).
 */
template <typename T = godot::Vector3>
struct alignas(64) ForwardKinematicsChainTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR3 | DataType::VECTOR3D | DataType::VECTOR4 | DataType::VECTOR4D |
        DataType::FLOAT32 | DataType::CUSTOM;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    // Base transient flag (>0 triggers dynamic get_transient_requirement query in TaskGraphDOD)
    static constexpr size_t transient_workspace_bytes = 64;

    static constexpr std::string_view display_name = "Forward Kinematics Chain";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t global_pos_buffer_id       = INVALID_ID; // 4B: Output global bone positions (Vector3)
    uint32_t global_rot_buffer_id       = INVALID_ID; // 4B: Output global bone rotations (Vector4 Quaternion)
    uint32_t global_scale_buffer_id     = INVALID_ID; // 4B: Output global bone scales (Vector3)
    uint32_t global_transform_buffer_id = INVALID_ID; // 4B: Output global Transform3D buffer

    uint32_t local_pos_buffer_id        = INVALID_ID; // 4B: Input local bone positions (Vector3)
    uint32_t local_rot_buffer_id        = INVALID_ID; // 4B: Input local bone rotations (Vector4 Quaternion)
    uint32_t local_scale_buffer_id      = INVALID_ID; // 4B: Input local bone scales (Vector3)
    uint32_t local_transform_buffer_id  = INVALID_ID; // 4B: Input local Transform3D buffer

    uint32_t parent_index_buffer_id     = INVALID_ID; // 4B: Topological int32_t parent bone indices (-1 = root)
    godot::Vector3 root_offset{0.0f, 0.0f, 0.0f};     // 12B: World/component offset applied to root bones
    bool normalize_quaternions          = true;       // 1B: Normalize input/output quaternions
    uint8_t reserved_padding[15]        = {0};        // 15B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (global_transform_buffer_id != INVALID_ID) return 0;
        const int64_t bones = _resolve_bone_count(p_context);
        if (bones > 0) {
            return static_cast<size_t>(bones) * sizeof(godot::Transform3D);
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary gpos_prop;
        gpos_prop["name"] = "global_pos_buffer_id";
        gpos_prop["type"] = godot::Variant::INT;
        gpos_prop["hint_string"] = "buffer_option";
        props.push_back(gpos_prop);

        godot::Dictionary grot_prop;
        grot_prop["name"] = "global_rot_buffer_id";
        grot_prop["type"] = godot::Variant::INT;
        grot_prop["hint_string"] = "buffer_option";
        props.push_back(grot_prop);

        godot::Dictionary gscl_prop;
        gscl_prop["name"] = "global_scale_buffer_id";
        gscl_prop["type"] = godot::Variant::INT;
        gscl_prop["hint_string"] = "buffer_option";
        props.push_back(gscl_prop);

        godot::Dictionary gxfm_prop;
        gxfm_prop["name"] = "global_transform_buffer_id";
        gxfm_prop["type"] = godot::Variant::INT;
        gxfm_prop["hint_string"] = "buffer_option";
        props.push_back(gxfm_prop);

        godot::Dictionary lpos_prop;
        lpos_prop["name"] = "local_pos_buffer_id";
        lpos_prop["type"] = godot::Variant::INT;
        lpos_prop["hint_string"] = "buffer_option";
        props.push_back(lpos_prop);

        godot::Dictionary lrot_prop;
        lrot_prop["name"] = "local_rot_buffer_id";
        lrot_prop["type"] = godot::Variant::INT;
        lrot_prop["hint_string"] = "buffer_option";
        props.push_back(lrot_prop);

        godot::Dictionary lscl_prop;
        lscl_prop["name"] = "local_scale_buffer_id";
        lscl_prop["type"] = godot::Variant::INT;
        lscl_prop["hint_string"] = "buffer_option";
        props.push_back(lscl_prop);

        godot::Dictionary lxfm_prop;
        lxfm_prop["name"] = "local_transform_buffer_id";
        lxfm_prop["type"] = godot::Variant::INT;
        lxfm_prop["hint_string"] = "buffer_option";
        props.push_back(lxfm_prop);

        godot::Dictionary par_prop;
        par_prop["name"] = "parent_index_buffer_id";
        par_prop["type"] = godot::Variant::INT;
        par_prop["hint_string"] = "buffer_option";
        props.push_back(par_prop);

        godot::Dictionary roff_prop;
        roff_prop["name"] = "root_offset";
        roff_prop["type"] = godot::Variant::VECTOR3;
        roff_prop["hint_string"] = "suffix:m";
        props.push_back(roff_prop);

        godot::Dictionary norm_prop;
        norm_prop["name"] = "normalize_quaternions";
        norm_prop["type"] = godot::Variant::BOOL;
        norm_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(norm_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (global_pos_buffer_id != INVALID_ID) return global_pos_buffer_id;
        if (global_transform_buffer_id != INVALID_ID) return global_transform_buffer_id;
        if (global_rot_buffer_id != INVALID_ID) return global_rot_buffer_id;
        return global_scale_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("global_pos_buffer_id")) {
            global_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["global_pos_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && global_pos_buffer_id == INVALID_ID) {
            global_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("global_rot_buffer_id")) {
            global_rot_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["global_rot_buffer_id"]));
        }
        if (p_props.has("global_scale_buffer_id")) {
            global_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["global_scale_buffer_id"]));
        }
        if (p_props.has("global_transform_buffer_id")) {
            global_transform_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["global_transform_buffer_id"]));
        }
        if (p_props.has("local_pos_buffer_id")) {
            local_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["local_pos_buffer_id"]));
        }
        if (p_props.has("local_rot_buffer_id")) {
            local_rot_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["local_rot_buffer_id"]));
        }
        if (p_props.has("local_scale_buffer_id")) {
            local_scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["local_scale_buffer_id"]));
        }
        if (p_props.has("local_transform_buffer_id")) {
            local_transform_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["local_transform_buffer_id"]));
        }
        if (p_props.has("parent_index_buffer_id")) {
            parent_index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_index_buffer_id"]));
        }
        if (p_props.has("root_offset")) {
            root_offset = static_cast<godot::Vector3>(p_props["root_offset"]);
        }
        if (p_props.has("normalize_quaternions")) {
            normalize_quaternions = static_cast<bool>(p_props["normalize_quaternions"]);
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
        if constexpr (!std::is_same_v<T, bool>) {
            const int64_t bone_count = _resolve_bone_count(p_context);
            if (bone_count <= 0) return;

            const GrantPartPOD* par_part  = (parent_index_buffer_id != INVALID_ID)     ? p_context.get_grant_part(parent_index_buffer_id)     : nullptr;
            const GrantPartPOD* lpos_part = (local_pos_buffer_id != INVALID_ID)        ? p_context.get_grant_part(local_pos_buffer_id)        : nullptr;
            const GrantPartPOD* lrot_part = (local_rot_buffer_id != INVALID_ID)        ? p_context.get_grant_part(local_rot_buffer_id)        : nullptr;
            const GrantPartPOD* lscl_part = (local_scale_buffer_id != INVALID_ID)      ? p_context.get_grant_part(local_scale_buffer_id)      : nullptr;
            const GrantPartPOD* lxfm_part = (local_transform_buffer_id != INVALID_ID)  ? p_context.get_grant_part(local_transform_buffer_id)  : nullptr;

            const GrantPartPOD* gpos_part = (global_pos_buffer_id != INVALID_ID)       ? p_context.get_grant_part(global_pos_buffer_id)       : nullptr;
            const GrantPartPOD* grot_part = (global_rot_buffer_id != INVALID_ID)       ? p_context.get_grant_part(global_rot_buffer_id)       : nullptr;
            const GrantPartPOD* gscl_part = (global_scale_buffer_id != INVALID_ID)     ? p_context.get_grant_part(global_scale_buffer_id)     : nullptr;
            const GrantPartPOD* gxfm_part = (global_transform_buffer_id != INVALID_ID) ? p_context.get_grant_part(global_transform_buffer_id) : nullptr;

            const int32_t* __restrict parent_indices              = (par_part && par_part->raw_base_ptr)   ? reinterpret_cast<const int32_t*>(par_part->raw_base_ptr)              : nullptr;
            const godot::Vector3* __restrict local_positions      = (lpos_part && lpos_part->raw_base_ptr) ? reinterpret_cast<const godot::Vector3*>(lpos_part->raw_base_ptr)      : nullptr;
            const godot::Vector4* __restrict local_rotations      = (lrot_part && lrot_part->raw_base_ptr) ? reinterpret_cast<const godot::Vector4*>(lrot_part->raw_base_ptr)      : nullptr;
            const godot::Vector3* __restrict local_scales         = (lscl_part && lscl_part->raw_base_ptr) ? reinterpret_cast<const godot::Vector3*>(lscl_part->raw_base_ptr)      : nullptr;
            const godot::Transform3D* __restrict local_transforms = (lxfm_part && lxfm_part->raw_base_ptr) ? reinterpret_cast<const godot::Transform3D*>(lxfm_part->raw_base_ptr) : nullptr;

            godot::Vector3* __restrict global_positions      = (gpos_part && gpos_part->raw_base_ptr) ? reinterpret_cast<godot::Vector3*>(gpos_part->raw_base_ptr)      : nullptr;
            godot::Vector4* __restrict global_rotations      = (grot_part && grot_part->raw_base_ptr) ? reinterpret_cast<godot::Vector4*>(grot_part->raw_base_ptr)      : nullptr;
            godot::Vector3* __restrict global_scales         = (gscl_part && gscl_part->raw_base_ptr) ? reinterpret_cast<godot::Vector3*>(gscl_part->raw_base_ptr)      : nullptr;
            godot::Transform3D* __restrict global_transforms = (gxfm_part && gxfm_part->raw_base_ptr) ? reinterpret_cast<godot::Transform3D*>(gxfm_part->raw_base_ptr) : nullptr;

            // Use local_workspace to hold intermediate global Transform3D matrices if global_transform_buffer_id is not bound
            if (!global_transforms && p_context.local_workspace) {
                global_transforms = reinterpret_cast<godot::Transform3D*>(p_context.local_workspace);
            }

            if (!local_transforms && !local_positions && !local_rotations) return;
            if (!global_transforms && !global_positions && !global_rotations) return;

            for (int64_t i = 0; i < bone_count; ++i) {
                godot::Transform3D local_xf;
                if (local_transforms) {
                    local_xf = local_transforms[i];
                } else {
                    const godot::Vector3 l_pos = local_positions ? local_positions[i] : godot::Vector3(0.0f, 0.0f, 0.0f);
                    godot::Basis l_basis;
                    if (local_rotations) {
                        const godot::Vector4& qv = local_rotations[i];
                        godot::Quaternion q(qv.x, qv.y, qv.z, qv.w);
                        if (normalize_quaternions) q = q.normalized();
                        l_basis = godot::Basis(q);
                    }
                    if (local_scales) {
                        l_basis.scale(local_scales[i]);
                    }
                    local_xf = godot::Transform3D(l_basis, l_pos);
                }

                // Resolve parent index (if parent_indices is omitted, assumes sequential chain parent = i - 1)
                const int32_t parent_idx = parent_indices ? parent_indices[i] : static_cast<int32_t>(i - 1);

                godot::Transform3D global_xf;
                if (parent_idx >= 0 && parent_idx < static_cast<int32_t>(i)) {
                    if (global_transforms) {
                        global_xf = global_transforms[parent_idx] * local_xf;
                    } else {
                        // Reconstruct parent global transform from SoA outputs
                        const godot::Vector3 p_pos = global_positions ? global_positions[parent_idx] : godot::Vector3(0.0f, 0.0f, 0.0f);
                        godot::Basis p_basis;
                        if (global_rotations) {
                            const godot::Vector4& pq = global_rotations[parent_idx];
                            p_basis = godot::Basis(godot::Quaternion(pq.x, pq.y, pq.z, pq.w));
                        }
                        if (global_scales) {
                            p_basis.scale(global_scales[parent_idx]);
                        }
                        global_xf = godot::Transform3D(p_basis, p_pos) * local_xf;
                    }
                } else {
                    global_xf = local_xf;
                    global_xf.origin += root_offset;
                }

                if (global_transforms) {
                    global_transforms[i] = global_xf;
                }
                if (global_positions) {
                    global_positions[i] = global_xf.origin;
                }
                if (global_rotations) {
                    godot::Quaternion gq = global_xf.basis.get_rotation_quaternion();
                    if (normalize_quaternions) gq = gq.normalized();
                    global_rotations[i] = godot::Vector4(gq.x, gq.y, gq.z, gq.w);
                }
                if (global_scales) {
                    global_scales[i] = global_xf.basis.get_scale();
                }
            }
        }
    }

private:
    [[nodiscard]] inline int64_t _resolve_bone_count(const TaskContextPOD& p_context) const noexcept {
        int64_t count = 0;
        auto clamp_cap = [&](uint32_t buf_id, size_t elem_size) noexcept {
            if (buf_id == INVALID_ID) return;
            if (const GrantPartPOD* part = p_context.get_grant_part(buf_id)) {
                const int64_t cap = static_cast<int64_t>(part->capacity_bytes / elem_size);
                if (cap > 0) {
                    count = (count == 0) ? cap : std::min(count, cap);
                }
            }
        };
        clamp_cap(global_pos_buffer_id,       sizeof(godot::Vector3));
        clamp_cap(global_rot_buffer_id,       sizeof(godot::Vector4));
        clamp_cap(global_scale_buffer_id,     sizeof(godot::Vector3));
        clamp_cap(global_transform_buffer_id, sizeof(godot::Transform3D));
        clamp_cap(local_pos_buffer_id,        sizeof(godot::Vector3));
        clamp_cap(local_rot_buffer_id,        sizeof(godot::Vector4));
        clamp_cap(local_scale_buffer_id,      sizeof(godot::Vector3));
        clamp_cap(local_transform_buffer_id,  sizeof(godot::Transform3D));
        clamp_cap(parent_index_buffer_id,     sizeof(int32_t));
        return count;
    }
};

static_assert(sizeof(ForwardKinematicsChainTransformLogic<>) == 64, "ForwardKinematicsChainTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core