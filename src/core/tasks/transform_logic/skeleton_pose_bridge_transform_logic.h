#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/variant/basis.hpp>
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
#include <type_traits>

namespace ideam::core {

enum class SkeletonBridgeMode : uint8_t {
    IMPORT_REST            = 0, // Imports rest-pose transforms and parent bone indices from Skeleton3D
    IMPORT_POSE            = 1, // Imports current animated local/global bone poses into DOD buffers
    COMMIT_LOCAL_POSE      = 2, // Commits solved DOD local poses back to Skeleton3D / RenderingServer
    COMMIT_GLOBAL_OVERRIDE = 3  // Commits solved DOD global poses back to Skeleton3D / RenderingServer
};

/**
 * SkeletonPoseBridgeTransformLogic<T>
 * Bidirectional boundary bridge between a godot::Skeleton3D (or RenderingServer skeleton RID)
 * and contiguous DOD bone buffers (Position Vector3, Rotation Vector4 Quaternion, Scale Vector3,
 * Transform3D, and optional int32_t parent bone indices).
 */
template <typename T = godot::Vector3>
struct alignas(64) SkeletonPoseBridgeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR3 | DataType::VECTOR3D | DataType::VECTOR4 | DataType::VECTOR4D |
        DataType::FLOAT32 | DataType::INT32 | DataType::CUSTOM;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Skeleton Pose Bridge";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t position_buffer_id     = INVALID_ID; // 4B: Bone positions (Vector3)
    uint32_t rotation_buffer_id     = INVALID_ID; // 4B: Bone rotations as Quaternion (Vector4: x, y, z, w)
    uint32_t scale_buffer_id        = INVALID_ID; // 4B: Bone scales (Vector3)
    uint32_t transform_buffer_id    = INVALID_ID; // 4B: Packed Transform3D (48B) buffer
    uint32_t parent_index_buffer_id = INVALID_ID; // 4B: Optional int32_t parent bone index buffer (populated on IMPORT)
    uint32_t inv_bind_buffer_id     = INVALID_ID; // 4B: Optional Transform3D inverse-bind matrix buffer (populated on IMPORT_REST)

    godot::Skeleton3D* target_skeleton = nullptr; // 8B: Direct Skeleton3D node pointer (optional if skeleton_rid is set)
    godot::RID skeleton_rid;                      // 8B: Low-level RenderingServer skeleton RID

    SkeletonBridgeMode mode         = SkeletonBridgeMode::IMPORT_POSE; // 1B
    bool one_shot                   = false;      // 1B: Execute once (for static rest-pose/hierarchy ingestion)
    mutable bool already_executed   = false;      // 1B: Runtime one-shot latch
    bool commit_to_rendering_server = true;       // 1B: Push directly to RenderingServer::skeleton_bone_set_transform
    uint8_t reserved_padding[20]    = {0};        // 20B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Import Rest Pose:0,Import Current Pose:1,Commit Local Pose:2,Commit Global Override:3";
        props.push_back(mode_prop);

        godot::Dictionary pos_prop;
        pos_prop["name"] = "position_buffer_id";
        pos_prop["type"] = godot::Variant::INT;
        pos_prop["hint_string"] = "buffer_option";
        props.push_back(pos_prop);

        godot::Dictionary rot_prop;
        rot_prop["name"] = "rotation_buffer_id";
        rot_prop["type"] = godot::Variant::INT;
        rot_prop["hint_string"] = "buffer_option";
        props.push_back(rot_prop);

        godot::Dictionary scl_prop;
        scl_prop["name"] = "scale_buffer_id";
        scl_prop["type"] = godot::Variant::INT;
        scl_prop["hint_string"] = "buffer_option";
        props.push_back(scl_prop);

        godot::Dictionary xfm_prop;
        xfm_prop["name"] = "transform_buffer_id";
        xfm_prop["type"] = godot::Variant::INT;
        xfm_prop["hint_string"] = "buffer_option";
        props.push_back(xfm_prop);

        godot::Dictionary par_prop;
        par_prop["name"] = "parent_index_buffer_id";
        par_prop["type"] = godot::Variant::INT;
        par_prop["hint_string"] = "buffer_option";
        props.push_back(par_prop);

        godot::Dictionary ibind_prop;
        ibind_prop["name"] = "inv_bind_buffer_id";
        ibind_prop["type"] = godot::Variant::INT;
        ibind_prop["hint_string"] = "buffer_option";
        props.push_back(ibind_prop);

        godot::Dictionary os_prop;
        os_prop["name"] = "one_shot";
        os_prop["type"] = godot::Variant::BOOL;
        os_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(os_prop);

        godot::Dictionary rs_prop;
        rs_prop["name"] = "commit_to_rendering_server";
        rs_prop["type"] = godot::Variant::BOOL;
        rs_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rs_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (position_buffer_id != INVALID_ID) return position_buffer_id;
        if (rotation_buffer_id != INVALID_ID) return rotation_buffer_id;
        if (transform_buffer_id != INVALID_ID) return transform_buffer_id;
        if (scale_buffer_id != INVALID_ID) return scale_buffer_id;
        return parent_index_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("position_buffer_id")) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && position_buffer_id == INVALID_ID) {
            position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("rotation_buffer_id")) {
            rotation_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rotation_buffer_id"]));
        }
        if (p_props.has("scale_buffer_id")) {
            scale_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["scale_buffer_id"]));
        }
        if (p_props.has("transform_buffer_id")) {
            transform_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["transform_buffer_id"]));
        }
        if (p_props.has("parent_index_buffer_id")) {
            parent_index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_index_buffer_id"]));
        }
        if (p_props.has("inv_bind_buffer_id")) {
            inv_bind_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["inv_bind_buffer_id"]));
        }
        if (p_props.has("mode")) {
            mode = static_cast<SkeletonBridgeMode>(static_cast<uint8_t>(static_cast<int64_t>(p_props["mode"])));
        }
        if (p_props.has("one_shot")) {
            one_shot = static_cast<bool>(p_props["one_shot"]);
        }
        if (p_props.has("commit_to_rendering_server")) {
            commit_to_rendering_server = static_cast<bool>(p_props["commit_to_rendering_server"]);
        }
        if (p_props.has("skeleton_rid")) {
            skeleton_rid = p_props["skeleton_rid"];
        }
        if (p_props.has("target_skeleton")) {
            godot::Object* obj = p_props["target_skeleton"];
            target_skeleton = godot::Object::cast_to<godot::Skeleton3D>(obj);
        }
        already_executed = false;
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
            if (one_shot && already_executed) return;

            const GrantPartPOD* pos_part   = (position_buffer_id != INVALID_ID)     ? p_context.get_grant_part(position_buffer_id)     : nullptr;
            const GrantPartPOD* rot_part   = (rotation_buffer_id != INVALID_ID)     ? p_context.get_grant_part(rotation_buffer_id)     : nullptr;
            const GrantPartPOD* scl_part   = (scale_buffer_id != INVALID_ID)        ? p_context.get_grant_part(scale_buffer_id)        : nullptr;
            const GrantPartPOD* xfm_part   = (transform_buffer_id != INVALID_ID)    ? p_context.get_grant_part(transform_buffer_id)    : nullptr;
            const GrantPartPOD* par_part   = (parent_index_buffer_id != INVALID_ID) ? p_context.get_grant_part(parent_index_buffer_id) : nullptr;
            const GrantPartPOD* ibind_part = (inv_bind_buffer_id != INVALID_ID)     ? p_context.get_grant_part(inv_bind_buffer_id)     : nullptr;

            godot::Vector3* __restrict positions      = (pos_part && pos_part->raw_base_ptr)     ? reinterpret_cast<godot::Vector3*>(pos_part->raw_base_ptr)         : nullptr;
            godot::Vector4* __restrict rotations      = (rot_part && rot_part->raw_base_ptr)     ? reinterpret_cast<godot::Vector4*>(rot_part->raw_base_ptr)         : nullptr;
            godot::Vector3* __restrict scales         = (scl_part && scl_part->raw_base_ptr)     ? reinterpret_cast<godot::Vector3*>(scl_part->raw_base_ptr)         : nullptr;
            godot::Transform3D* __restrict transforms = (xfm_part && xfm_part->raw_base_ptr)     ? reinterpret_cast<godot::Transform3D*>(xfm_part->raw_base_ptr)     : nullptr;
            int32_t* __restrict parent_indices        = (par_part && par_part->raw_base_ptr)     ? reinterpret_cast<int32_t*>(par_part->raw_base_ptr)                : nullptr;
            godot::Transform3D* __restrict inv_binds  = (ibind_part && ibind_part->raw_base_ptr) ? reinterpret_cast<godot::Transform3D*>(ibind_part->raw_base_ptr)   : nullptr;

            godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
            int64_t bone_count = 0;
            if (target_skeleton) {
                bone_count = target_skeleton->get_bone_count();
            } else if (skeleton_rid.is_valid() && rs) {
                bone_count = rs->skeleton_get_bone_count(skeleton_rid);
            }
            if (bone_count <= 0) return;

            if (pos_part)   bone_count = std::min<int64_t>(bone_count, static_cast<int64_t>(pos_part->capacity_bytes / sizeof(godot::Vector3)));
            if (rot_part)   bone_count = std::min<int64_t>(bone_count, static_cast<int64_t>(rot_part->capacity_bytes / sizeof(godot::Vector4)));
            if (scl_part)   bone_count = std::min<int64_t>(bone_count, static_cast<int64_t>(scl_part->capacity_bytes / sizeof(godot::Vector3)));
            if (xfm_part)   bone_count = std::min<int64_t>(bone_count, static_cast<int64_t>(xfm_part->capacity_bytes / sizeof(godot::Transform3D)));
            if (par_part)   bone_count = std::min<int64_t>(bone_count, static_cast<int64_t>(par_part->capacity_bytes / sizeof(int32_t)));
            if (ibind_part) bone_count = std::min<int64_t>(bone_count, static_cast<int64_t>(ibind_part->capacity_bytes / sizeof(godot::Transform3D)));
            if (bone_count <= 0) return;

            if (mode == SkeletonBridgeMode::IMPORT_REST || mode == SkeletonBridgeMode::IMPORT_POSE) {
                for (int64_t b = 0; b < bone_count; ++b) {
                    const int32_t bone_idx = static_cast<int32_t>(b);
                    godot::Transform3D xf;

                    if (target_skeleton) {
                        if (parent_indices) {
                            parent_indices[b] = target_skeleton->get_bone_parent(bone_idx);
                        }
                        if (mode == SkeletonBridgeMode::IMPORT_REST) {
                            xf = target_skeleton->get_bone_rest(bone_idx);
                            if (inv_binds) {
                                const int32_t p_idx = target_skeleton->get_bone_parent(bone_idx);
                                const godot::Transform3D global_rest = (p_idx >= 0 && p_idx < bone_idx && transforms)
                                    ? (transforms[p_idx] * xf)
                                    : xf;
                                inv_binds[b] = global_rest.affine_inverse();
                            }
                        } else {
                            xf = target_skeleton->get_bone_pose(bone_idx);
                        }
                    } else if (skeleton_rid.is_valid() && rs) {
                        xf = rs->skeleton_bone_get_transform(skeleton_rid, bone_idx);
                    }

                    if (transforms) {
                        transforms[b] = xf;
                    }
                    if (positions) {
                        positions[b] = xf.origin;
                    }
                    if (rotations) {
                        const godot::Quaternion q = xf.basis.get_rotation_quaternion();
                        rotations[b] = godot::Vector4(q.x, q.y, q.z, q.w);
                    }
                    if (scales) {
                        scales[b] = xf.basis.get_scale();
                    }
                }
                already_executed = true;
                return;
            }

            // COMMIT_LOCAL_POSE or COMMIT_GLOBAL_OVERRIDE
            for (int64_t b = 0; b < bone_count; ++b) {
                const int32_t bone_idx = static_cast<int32_t>(b);
                godot::Transform3D xf;

                if (transforms) {
                    xf = transforms[b];
                } else {
                    const godot::Vector3 pos = positions ? positions[b] : godot::Vector3(0.0f, 0.0f, 0.0f);
                    godot::Basis basis;
                    if (rotations) {
                        const godot::Vector4& qv = rotations[b];
                        basis = godot::Basis(godot::Quaternion(qv.x, qv.y, qv.z, qv.w).normalized());
                    }
                    if (scales) {
                        basis.scale(scales[b]);
                    }
                    xf = godot::Transform3D(basis, pos);
                }

                if (commit_to_rendering_server && skeleton_rid.is_valid() && rs) {
                    rs->skeleton_bone_set_transform(skeleton_rid, bone_idx, xf);
                } else if (target_skeleton) {
                    if (mode == SkeletonBridgeMode::COMMIT_GLOBAL_OVERRIDE) {
                        target_skeleton->set_bone_global_pose(bone_idx, xf);
                    } else {
                        if (positions || transforms) {
                            target_skeleton->set_bone_pose_position(bone_idx, xf.origin);
                        }
                        if (rotations) {
                            const godot::Vector4& qv = rotations[b];
                            target_skeleton->set_bone_pose_rotation(bone_idx, godot::Quaternion(qv.x, qv.y, qv.z, qv.w).normalized());
                        } else if (transforms) {
                            target_skeleton->set_bone_pose_rotation(bone_idx, xf.basis.get_rotation_quaternion());
                        }
                        if (scales) {
                            target_skeleton->set_bone_pose_scale(bone_idx, scales[b]);
                        }
                    }
                }
            }
            already_executed = true;
        }
    }
};

static_assert(sizeof(SkeletonPoseBridgeTransformLogic<>) == 64, "SkeletonPoseBridgeTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core