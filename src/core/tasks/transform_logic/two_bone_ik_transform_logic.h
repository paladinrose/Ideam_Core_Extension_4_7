#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * TwoBoneIKTransformLogic<T>
 * Analytical Law-of-Cosines 2-bone Inverse Kinematics solver with pole-vector bend-plane
 * alignment, axial twist, and exponential soft-reach damping near full extension (d -> L1 + L2)
 * to eliminate knee/elbow popping on terrain-adaptive robot/player limbs.
 * Supports single-limb indices or batched limb triplets via chain_indices_buffer_id (Vector3i).
 */
template <typename T = godot::Vector3>
struct alignas(64) TwoBoneIKTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR3 | DataType::VECTOR3D | DataType::VECTOR4 | DataType::VECTOR4D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Two-Bone IK";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t joint_pos_buffer_id     = INVALID_ID; // 4B: Primary joint positions buffer (Vector3)
    uint32_t joint_rot_buffer_id     = INVALID_ID; // 4B: Optional output joint rotations (Vector4 Quaternion)
    uint32_t target_pos_buffer_id    = INVALID_ID; // 4B: IK effector target positions (Vector3, 1 per limb chain)
    uint32_t pole_pos_buffer_id      = INVALID_ID; // 4B: Optional pole target positions (Vector3, 1 per limb chain)
    uint32_t chain_indices_buffer_id = INVALID_ID; // 4B: Optional Vector3i(root_idx, mid_idx, tip_idx) triplets per limb

    int32_t root_bone_index          = 0;          // 4B: Fallback single-chain root joint index
    int32_t mid_bone_index           = 1;          // 4B: Fallback single-chain mid (knee/elbow) joint index
    int32_t tip_bone_index           = 2;          // 4B: Fallback single-chain tip (ankle/wrist) joint index

    godot::Vector3 default_pole_offset{0.0f, 0.0f, 1.0f}; // 12B: Fallback pole vector offset relative to root joint

    float upper_length               = 0.0f;       // 4B: Override L1 if > 0 (otherwise measured from input joints)
    float lower_length               = 0.0f;       // 4B: Override L2 if > 0 (otherwise measured from input joints)
    float soft_reach_ratio           = 0.95f;      // 4B: Fraction of (L1 + L2) where soft-reach damping engages
    float ik_weight                  = 1.0f;       // 4B: Blend weight [0.0, 1.0]
    float twist_angle_rad            = 0.0f;       // 4B: Pole twist angle around effector axis (radians) -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary jpos_prop;
        jpos_prop["name"] = "joint_pos_buffer_id";
        jpos_prop["type"] = godot::Variant::INT;
        jpos_prop["hint_string"] = "buffer_option";
        props.push_back(jpos_prop);

        godot::Dictionary jrot_prop;
        jrot_prop["name"] = "joint_rot_buffer_id";
        jrot_prop["type"] = godot::Variant::INT;
        jrot_prop["hint_string"] = "buffer_option";
        props.push_back(jrot_prop);

        godot::Dictionary tpos_prop;
        tpos_prop["name"] = "target_pos_buffer_id";
        tpos_prop["type"] = godot::Variant::INT;
        tpos_prop["hint_string"] = "buffer_option";
        props.push_back(tpos_prop);

        godot::Dictionary ppos_prop;
        ppos_prop["name"] = "pole_pos_buffer_id";
        ppos_prop["type"] = godot::Variant::INT;
        ppos_prop["hint_string"] = "buffer_option";
        props.push_back(ppos_prop);

        godot::Dictionary cidx_prop;
        cidx_prop["name"] = "chain_indices_buffer_id";
        cidx_prop["type"] = godot::Variant::INT;
        cidx_prop["hint_string"] = "buffer_option";
        props.push_back(cidx_prop);

        godot::Dictionary ridx_prop;
        ridx_prop["name"] = "root_bone_index";
        ridx_prop["type"] = godot::Variant::INT;
        ridx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        ridx_prop["hint_string"] = "0,1024,1";
        props.push_back(ridx_prop);

        godot::Dictionary midx_prop;
        midx_prop["name"] = "mid_bone_index";
        midx_prop["type"] = godot::Variant::INT;
        midx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        midx_prop["hint_string"] = "0,1024,1";
        props.push_back(midx_prop);

        godot::Dictionary tidx_prop;
        tidx_prop["name"] = "tip_bone_index";
        tidx_prop["type"] = godot::Variant::INT;
        tidx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        tidx_prop["hint_string"] = "0,1024,1";
        props.push_back(tidx_prop);

        godot::Dictionary pole_prop;
        pole_prop["name"] = "default_pole_offset";
        pole_prop["type"] = godot::Variant::VECTOR3;
        pole_prop["hint_string"] = "suffix:m";
        props.push_back(pole_prop);

        godot::Dictionary l1_prop;
        l1_prop["name"] = "upper_length";
        l1_prop["type"] = godot::Variant::FLOAT;
        l1_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        l1_prop["hint_string"] = "0.0,20.0,0.001,suffix:m";
        props.push_back(l1_prop);

        godot::Dictionary l2_prop;
        l2_prop["name"] = "lower_length";
        l2_prop["type"] = godot::Variant::FLOAT;
        l2_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        l2_prop["hint_string"] = "0.0,20.0,0.001,suffix:m";
        props.push_back(l2_prop);

        godot::Dictionary soft_prop;
        soft_prop["name"] = "soft_reach_ratio";
        soft_prop["type"] = godot::Variant::FLOAT;
        soft_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        soft_prop["hint_string"] = "0.5,1.0,0.01";
        props.push_back(soft_prop);

        godot::Dictionary w_prop;
        w_prop["name"] = "ik_weight";
        w_prop["type"] = godot::Variant::FLOAT;
        w_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        w_prop["hint_string"] = "0.0,1.0,0.01";
        props.push_back(w_prop);

        godot::Dictionary tw_prop;
        tw_prop["name"] = "twist_angle_rad";
        tw_prop["type"] = godot::Variant::FLOAT;
        tw_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        tw_prop["hint_string"] = "-3.14159,3.14159,0.01,suffix:rad";
        props.push_back(tw_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (joint_pos_buffer_id != INVALID_ID) ? joint_pos_buffer_id : joint_rot_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("joint_pos_buffer_id")) {
            joint_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["joint_pos_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && joint_pos_buffer_id == INVALID_ID) {
            joint_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("joint_rot_buffer_id")) {
            joint_rot_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["joint_rot_buffer_id"]));
        }
        if (p_props.has("target_pos_buffer_id")) {
            target_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_pos_buffer_id"]));
        }
        if (p_props.has("pole_pos_buffer_id")) {
            pole_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["pole_pos_buffer_id"]));
        }
        if (p_props.has("chain_indices_buffer_id")) {
            chain_indices_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["chain_indices_buffer_id"]));
        }
        if (p_props.has("root_bone_index")) {
            root_bone_index = static_cast<int32_t>(static_cast<int64_t>(p_props["root_bone_index"]));
        }
        if (p_props.has("mid_bone_index")) {
            mid_bone_index = static_cast<int32_t>(static_cast<int64_t>(p_props["mid_bone_index"]));
        }
        if (p_props.has("tip_bone_index")) {
            tip_bone_index = static_cast<int32_t>(static_cast<int64_t>(p_props["tip_bone_index"]));
        }
        if (p_props.has("default_pole_offset")) {
            default_pole_offset = static_cast<godot::Vector3>(p_props["default_pole_offset"]);
        }
        if (p_props.has("upper_length")) {
            upper_length = std::max(0.0f, static_cast<float>(p_props["upper_length"]));
        }
        if (p_props.has("lower_length")) {
            lower_length = std::max(0.0f, static_cast<float>(p_props["lower_length"]));
        }
        if (p_props.has("soft_reach_ratio")) {
            soft_reach_ratio = std::clamp(static_cast<float>(p_props["soft_reach_ratio"]), 0.5f, 1.0f);
        }
        if (p_props.has("ik_weight")) {
            ik_weight = std::clamp(static_cast<float>(p_props["ik_weight"]), 0.0f, 1.0f);
        }
        if (p_props.has("twist_angle_rad")) {
            twist_angle_rad = static_cast<float>(p_props["twist_angle_rad"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, godot::Vector4>) {
            const GrantPartPOD* jpos_part = p_context.get_grant_part(joint_pos_buffer_id);
            const GrantPartPOD* tpos_part = p_context.get_grant_part(target_pos_buffer_id);
            if (!jpos_part || !jpos_part->raw_base_ptr || !tpos_part || !tpos_part->raw_base_ptr) return;

            const int64_t joint_cap = static_cast<int64_t>(jpos_part->capacity_bytes / sizeof(godot::Vector3));
            const int64_t target_cap = static_cast<int64_t>(tpos_part->capacity_bytes / sizeof(godot::Vector3));
            if (joint_cap < 3 || target_cap <= 0) return;

            godot::Vector3* __restrict raw_joints         = reinterpret_cast<godot::Vector3*>(jpos_part->raw_base_ptr);
            const godot::Vector3* __restrict targets      = reinterpret_cast<const godot::Vector3*>(tpos_part->raw_base_ptr);

            const GrantPartPOD* pole_part = (pole_pos_buffer_id != INVALID_ID) ? p_context.get_grant_part(pole_pos_buffer_id) : nullptr;
            const godot::Vector3* __restrict poles = (pole_part && pole_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector3*>(pole_part->raw_base_ptr)
                : nullptr;
            const int64_t pole_cap = pole_part ? static_cast<int64_t>(pole_part->capacity_bytes / sizeof(godot::Vector3)) : 0;

            const GrantPartPOD* cidx_part = (chain_indices_buffer_id != INVALID_ID) ? p_context.get_grant_part(chain_indices_buffer_id) : nullptr;
            const godot::Vector3i* __restrict chain_triplets = (cidx_part && cidx_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector3i*>(cidx_part->raw_base_ptr)
                : nullptr;
            const int64_t chain_cap = cidx_part ? static_cast<int64_t>(cidx_part->capacity_bytes / sizeof(godot::Vector3i)) : 0;

            const GrantPartPOD* jrot_part = (joint_rot_buffer_id != INVALID_ID) ? p_context.get_grant_part(joint_rot_buffer_id) : nullptr;
            godot::Vector4* __restrict joint_rots = (jrot_part && jrot_part->raw_base_ptr)
                ? reinterpret_cast<godot::Vector4*>(jrot_part->raw_base_ptr)
                : nullptr;
            const int64_t rot_cap = jrot_part ? static_cast<int64_t>(jrot_part->capacity_bytes / sizeof(godot::Vector4)) : 0;

            auto read_joint = [&](int64_t idx) noexcept -> godot::Vector3 {
                if constexpr (std::is_same_v<T, godot::Vector3>) {
                    return _read_view(p_view, idx);
                } else {
                    return raw_joints[idx];
                }
            };

            auto write_joint = [&](int64_t idx, const godot::Vector3& val) noexcept {
                if constexpr (std::is_same_v<T, godot::Vector3>) {
                    _write_view(p_view, idx, val);
                } else {
                    raw_joints[idx] = val;
                }
            };

            const int64_t num_chains = chain_triplets
                ? std::min(chain_cap, target_cap)
                : std::min<int64_t>(target_cap, std::max<int64_t>(1, joint_cap / 3));

            const float weight = std::clamp(ik_weight, 0.0f, 1.0f);
            const float cos_tw = std::cos(twist_angle_rad);
            const float sin_tw = std::sin(twist_angle_rad);

            for (int64_t c = 0; c < num_chains; ++c) {
                int32_t i_root = root_bone_index;
                int32_t i_mid  = mid_bone_index;
                int32_t i_tip  = tip_bone_index;

                if (chain_triplets) {
                    const godot::Vector3i& trip = chain_triplets[c];
                    i_root = trip.x;
                    i_mid  = trip.y;
                    i_tip  = trip.z;
                } else if (num_chains > 1) {
                    i_root = static_cast<int32_t>(c * 3);
                    i_mid  = static_cast<int32_t>(c * 3 + 1);
                    i_tip  = static_cast<int32_t>(c * 3 + 2);
                }

                if (i_root < 0 || i_mid < 0 || i_tip < 0 ||
                    i_root >= joint_cap || i_mid >= joint_cap || i_tip >= joint_cap) {
                    continue;
                }

                const godot::Vector3 p0 = read_joint(i_root);
                const godot::Vector3 p1 = read_joint(i_mid);
                const godot::Vector3 p2 = read_joint(i_tip);

                const godot::Vector3 orig_u = p1 - p0;
                const godot::Vector3 orig_l = p2 - p1;

                const float l1 = (upper_length > 1e-5f) ? upper_length : std::max(1e-5f, orig_u.length());
                const float l2 = (lower_length > 1e-5f) ? lower_length : std::max(1e-5f, orig_l.length());
                const float max_reach = l1 + l2;
                const float min_reach = std::max(1e-4f, std::abs(l1 - l2) + 1e-4f);

                const godot::Vector3 target = targets[c];
                const godot::Vector3 pole   = (poles && c < pole_cap) ? poles[c] : (p0 + default_pole_offset);

                godot::Vector3 to_target = target - p0;
                float dist = to_target.length();
                godot::Vector3 dir = (dist > 1e-6f) ? (to_target * (1.0f / dist)) : godot::Vector3(0.0f, -1.0f, 0.0f);

                // Soft-reach exponential damping near full extension to prevent knee/elbow snapping
                const float soft_start = max_reach * soft_reach_ratio;
                if (dist > soft_start && max_reach > soft_start + 1e-5f) {
                    const float soft_span = max_reach - soft_start;
                    const float excess    = dist - soft_start;
                    dist = soft_start + soft_span * (1.0f - std::exp(-excess / soft_span));
                }
                dist = std::clamp(dist, min_reach, max_reach - 1e-4f);

                // Law of Cosines: cos(alpha) = (L1^2 + d^2 - L2^2) / (2 * L1 * d)
                const float cos_alpha = std::clamp(
                    (l1 * l1 + dist * dist - l2 * l2) / (2.0f * l1 * dist),
                    -1.0f, 1.0f
                );
                const float sin_alpha = std::sqrt(std::max(0.0f, 1.0f - cos_alpha * cos_alpha));

                // Construct orthonormal bend frame from dir and pole vector
                const godot::Vector3 to_pole = pole - p0;
                godot::Vector3 bend_dir = to_pole - dir * dir.dot(to_pole);
                const float bend_len_sq = bend_dir.length_squared();
                if (bend_len_sq > 1e-8f) {
                    bend_dir *= (1.0f / std::sqrt(bend_len_sq));
                } else {
                    const godot::Vector3 fallback_axis = (std::abs(dir.x) < 0.9f)
                        ? godot::Vector3(1.0f, 0.0f, 0.0f)
                        : godot::Vector3(0.0f, 1.0f, 0.0f);
                    bend_dir = dir.cross(fallback_axis).normalized();
                }

                // Apply optional twist angle around the effector axis (Rodrigues rotation)
                if (std::abs(twist_angle_rad) > 1e-6f) {
                    const godot::Vector3 ortho_b = dir.cross(bend_dir);
                    bend_dir = (bend_dir * cos_tw + ortho_b * sin_tw).normalized();
                }

                const godot::Vector3 solved_p1 = p0 + (dir * cos_alpha + bend_dir * sin_alpha) * l1;
                const godot::Vector3 solved_p2 = p0 + dir * dist;

                const godot::Vector3 final_p1 = p1 + (solved_p1 - p1) * weight;
                const godot::Vector3 final_p2 = p2 + (solved_p2 - p2) * weight;

                write_joint(i_root, p0);
                write_joint(i_mid,  final_p1);
                write_joint(i_tip,  final_p2);

                if (joint_rots && i_root < rot_cap && i_mid < rot_cap) {
                    const godot::Vector3 new_u = (final_p1 - p0).normalized();
                    const godot::Vector3 new_l = (final_p2 - final_p1).normalized();
                    const godot::Quaternion q_root = _shortest_arc_quat(orig_u.normalized(), new_u);
                    const godot::Quaternion q_mid  = _shortest_arc_quat(orig_l.normalized(), new_l);
                    joint_rots[i_root] = godot::Vector4(q_root.x, q_root.y, q_root.z, q_root.w);
                    joint_rots[i_mid]  = godot::Vector4(q_mid.x,  q_mid.y,  q_mid.z,  q_mid.w);
                }
            }
        }
    }

private:
    [[nodiscard]] static inline godot::Quaternion _shortest_arc_quat(const godot::Vector3& p_from, const godot::Vector3& p_to) noexcept {
        const float d = p_from.dot(p_to);
        if (d >= 0.999999f) {
            return godot::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        }
        if (d <= -0.999999f) {
            godot::Vector3 axis = godot::Vector3(1.0f, 0.0f, 0.0f).cross(p_from);
            if (axis.length_squared() < 1e-6f) {
                axis = godot::Vector3(0.0f, 1.0f, 0.0f).cross(p_from);
            }
            axis = axis.normalized();
            return godot::Quaternion(axis.x, axis.y, axis.z, 0.0f);
        }
        const godot::Vector3 c = p_from.cross(p_to);
        const float s = std::sqrt((1.0f + d) * 2.0f);
        const float inv_s = 1.0f / s;
        return godot::Quaternion(c.x * inv_s, c.y * inv_s, c.z * inv_s, s * 0.5f).normalized();
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T _read_view(const T_View& p_view, int64_t idx) noexcept {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T>(p_view[idx].load());
        } else if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else {
            return static_cast<T>(p_view[idx]);
        }
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline void _write_view(const T_View& p_view, int64_t idx, const T& value) noexcept {
        if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            *reinterpret_cast<T*>(p_view[idx]) = value;
        } else if constexpr (requires { p_view[idx].write(); }) {
            p_view[idx] = value;
        } else if constexpr (requires { p_view[idx].store(value); }) {
            p_view[idx].store(value);
        } else {
            (void)(p_view[idx] = value);
        }
    }
};

static_assert(sizeof(TwoBoneIKTransformLogic<>) == 64, "TwoBoneIKTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core