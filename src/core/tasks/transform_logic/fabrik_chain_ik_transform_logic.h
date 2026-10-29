#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * FabrikChainIKTransformLogic<T>
 * Iterative Forward-And-Backward Reaching Inverse Kinematics (FABRIK) solver
 * over variable-length bone chains (N >= 2 joints) with conical inter-segment
 * angle constraints (max_cone_angle_cos) and multi-chain range support.
 */
template <typename T = godot::Vector3>
struct alignas(64) FabrikChainIKTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    // Base transient flag (>0 triggers dynamic get_transient_requirement query in TaskGraphDOD)
    static constexpr size_t transient_workspace_bytes = 64;

    static constexpr std::string_view display_name = "FABRIK Chain IK";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t joint_pos_buffer_id    = INVALID_ID; // 4B: Primary joint positions buffer (Vector3)
    uint32_t target_pos_buffer_id   = INVALID_ID; // 4B: Optional target positions buffer (Vector3, 1 per chain)
    uint32_t bone_length_buffer_id  = INVALID_ID; // 4B: Optional segment lengths buffer (float)
    uint32_t chain_ranges_buffer_id = INVALID_ID; // 4B: Optional Vector2i(start_joint, joint_count) per chain
    uint32_t joint_rot_buffer_id    = INVALID_ID; // 4B: Optional output joint rotations (Vector4 Quaternion)

    godot::Vector3 target_position{0.0f, 0.0f, 0.0f}; // 12B: Fallback single-chain target position

    float tolerance                 = 0.001f;     // 4B: Convergence distance threshold (meters)
    float max_cone_angle_cos        = -1.0f;      // 4B: Cosine of max inter-segment cone angle (-1.0 = unconstrained)
    float ik_weight                 = 1.0f;       // 4B: Blend weight [0.0, 1.0]
    uint16_t max_iterations         = 10;         // 2B: Maximum solver iterations
    bool lock_root                  = true;       // 1B: Anchor root joint at initial position
    uint8_t reserved_padding[17]    = {0};        // 17B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (const GrantPartPOD* j_part = p_context.get_grant_part(joint_pos_buffer_id)) {
            const size_t n = static_cast<size_t>(j_part->capacity_bytes / sizeof(godot::Vector3));
            if (n > 0) {
                // Scratch space for working joint positions (Vector3) + segment lengths (float)
                return n * (sizeof(godot::Vector3) + sizeof(float));
            }
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary jpos_prop;
        jpos_prop["name"] = "joint_pos_buffer_id";
        jpos_prop["type"] = godot::Variant::INT;
        jpos_prop["hint_string"] = "buffer_option";
        props.push_back(jpos_prop);

        godot::Dictionary tpos_prop;
        tpos_prop["name"] = "target_pos_buffer_id";
        tpos_prop["type"] = godot::Variant::INT;
        tpos_prop["hint_string"] = "buffer_option";
        props.push_back(tpos_prop);

        godot::Dictionary blen_prop;
        blen_prop["name"] = "bone_length_buffer_id";
        blen_prop["type"] = godot::Variant::INT;
        blen_prop["hint_string"] = "buffer_option";
        props.push_back(blen_prop);

        godot::Dictionary crng_prop;
        crng_prop["name"] = "chain_ranges_buffer_id";
        crng_prop["type"] = godot::Variant::INT;
        crng_prop["hint_string"] = "buffer_option";
        props.push_back(crng_prop);

        godot::Dictionary jrot_prop;
        jrot_prop["name"] = "joint_rot_buffer_id";
        jrot_prop["type"] = godot::Variant::INT;
        jrot_prop["hint_string"] = "buffer_option";
        props.push_back(jrot_prop);

        godot::Dictionary tgt_prop;
        tgt_prop["name"] = "target_position";
        tgt_prop["type"] = godot::Variant::VECTOR3;
        tgt_prop["hint_string"] = "suffix:m";
        props.push_back(tgt_prop);

        godot::Dictionary iter_prop;
        iter_prop["name"] = "max_iterations";
        iter_prop["type"] = godot::Variant::INT;
        iter_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        iter_prop["hint_string"] = "1,64,1";
        props.push_back(iter_prop);

        godot::Dictionary tol_prop;
        tol_prop["name"] = "tolerance";
        tol_prop["type"] = godot::Variant::FLOAT;
        tol_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        tol_prop["hint_string"] = "0.0001,0.5,0.0001,suffix:m";
        props.push_back(tol_prop);

        godot::Dictionary cone_prop;
        cone_prop["name"] = "max_cone_angle_cos";
        cone_prop["type"] = godot::Variant::FLOAT;
        cone_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        cone_prop["hint_string"] = "-1.0,1.0,0.01";
        props.push_back(cone_prop);

        godot::Dictionary w_prop;
        w_prop["name"] = "ik_weight";
        w_prop["type"] = godot::Variant::FLOAT;
        w_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        w_prop["hint_string"] = "0.0,1.0,0.01";
        props.push_back(w_prop);

        godot::Dictionary root_prop;
        root_prop["name"] = "lock_root";
        root_prop["type"] = godot::Variant::BOOL;
        root_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(root_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return joint_pos_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("joint_pos_buffer_id")) {
            joint_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["joint_pos_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            joint_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("target_pos_buffer_id")) {
            target_pos_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_pos_buffer_id"]));
        }
        if (p_props.has("bone_length_buffer_id")) {
            bone_length_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["bone_length_buffer_id"]));
        }
        if (p_props.has("chain_ranges_buffer_id")) {
            chain_ranges_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["chain_ranges_buffer_id"]));
        }
        if (p_props.has("joint_rot_buffer_id")) {
            joint_rot_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["joint_rot_buffer_id"]));
        }
        if (p_props.has("target_position")) {
            target_position = static_cast<godot::Vector3>(p_props["target_position"]);
        }
        if (p_props.has("tolerance")) {
            tolerance = std::max(1e-6f, static_cast<float>(p_props["tolerance"]));
        }
        if (p_props.has("max_cone_angle_cos")) {
            max_cone_angle_cos = std::clamp(static_cast<float>(p_props["max_cone_angle_cos"]), -1.0f, 1.0f);
        }
        if (p_props.has("ik_weight")) {
            ik_weight = std::clamp(static_cast<float>(p_props["ik_weight"]), 0.0f, 1.0f);
        }
        if (p_props.has("max_iterations")) {
            max_iterations = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["max_iterations"]), 1, 256));
        }
        if (p_props.has("lock_root")) {
            lock_root = static_cast<bool>(p_props["lock_root"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* jpos_part = p_context.get_grant_part(joint_pos_buffer_id);
            if (!jpos_part || !jpos_part->raw_base_ptr) return;

            const int64_t total_joints = static_cast<int64_t>(jpos_part->capacity_bytes / sizeof(godot::Vector3));
            if (total_joints < 2) return;

            const GrantPartPOD* tpos_part = (target_pos_buffer_id != INVALID_ID) ? p_context.get_grant_part(target_pos_buffer_id) : nullptr;
            const godot::Vector3* __restrict targets = (tpos_part && tpos_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector3*>(tpos_part->raw_base_ptr)
                : nullptr;
            const int64_t target_cap = tpos_part ? static_cast<int64_t>(tpos_part->capacity_bytes / sizeof(godot::Vector3)) : 0;

            const GrantPartPOD* blen_part = (bone_length_buffer_id != INVALID_ID) ? p_context.get_grant_part(bone_length_buffer_id) : nullptr;
            const float* __restrict ext_lengths = (blen_part && blen_part->raw_base_ptr)
                ? reinterpret_cast<const float*>(blen_part->raw_base_ptr)
                : nullptr;
            const int64_t blen_cap = blen_part ? static_cast<int64_t>(blen_part->capacity_bytes / sizeof(float)) : 0;

            const GrantPartPOD* crng_part = (chain_ranges_buffer_id != INVALID_ID) ? p_context.get_grant_part(chain_ranges_buffer_id) : nullptr;
            const godot::Vector2i* __restrict chain_ranges = (crng_part && crng_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector2i*>(crng_part->raw_base_ptr)
                : nullptr;
            const int64_t range_cap = crng_part
                ? (crng_part->selection.element_count > 0
                    ? crng_part->selection.element_count
                    : static_cast<int64_t>(crng_part->capacity_bytes / sizeof(godot::Vector2i)))
                : 0;

            const GrantPartPOD* jrot_part = (joint_rot_buffer_id != INVALID_ID) ? p_context.get_grant_part(joint_rot_buffer_id) : nullptr;
            godot::Vector4* __restrict joint_rots = (jrot_part && jrot_part->raw_base_ptr)
                ? reinterpret_cast<godot::Vector4*>(jrot_part->raw_base_ptr)
                : nullptr;
            const int64_t rot_cap = jrot_part ? static_cast<int64_t>(jrot_part->capacity_bytes / sizeof(godot::Vector4)) : 0;

            // Resolve working scratch buffers in local_workspace (or stack fallback for small chains <= 64 joints)
            godot::Vector3 stack_pos[64];
            float stack_len[64];

            godot::Vector3* __restrict work_pos = stack_pos;
            float* __restrict work_len          = stack_len;
            int64_t max_work_joints             = 64;

            if (p_context.local_workspace) {
                work_pos = reinterpret_cast<godot::Vector3*>(p_context.local_workspace);
                work_len = reinterpret_cast<float*>(work_pos + total_joints);
                max_work_joints = total_joints;
            }

            const float tol_sq = tolerance * tolerance;
            const float sin_max_cone = (max_cone_angle_cos > -0.9999f)
                ? std::sqrt(std::max(0.0f, 1.0f - max_cone_angle_cos * max_cone_angle_cos))
                : 0.0f;

            auto solve_chain = [&](int64_t start_idx, int64_t count, const godot::Vector3& chain_target) noexcept {
                if (start_idx < 0 || count < 2 || start_idx + count > total_joints || start_idx + count > max_work_joints) {
                    return;
                }

                godot::Vector3* __restrict p = work_pos + start_idx;
                float* __restrict lengths    = work_len + start_idx;

                for (int64_t k = 0; k < count; ++k) {
                    p[k] = _read_view(p_view, start_idx + k);
                }

                float total_reach = 0.0f;
                for (int64_t k = 0; k < count - 1; ++k) {
                    const int64_t g_idx = start_idx + k;
                    float seg_len = (ext_lengths && g_idx < blen_cap && ext_lengths[g_idx] > 1e-5f)
                        ? ext_lengths[g_idx]
                        : (p[k + 1] - p[k]).length();
                    seg_len = std::max(1e-5f, seg_len);
                    lengths[k] = seg_len;
                    total_reach += seg_len;
                }

                const godot::Vector3 root_origin = p[0];
                const godot::Vector3 to_target   = chain_target - root_origin;
                const float dist_to_target       = to_target.length();

                if (lock_root && dist_to_target >= total_reach) {
                    // Target is unreachable: stretch chain straight toward target
                    const godot::Vector3 dir = (dist_to_target > 1e-6f)
                        ? (to_target * (1.0f / dist_to_target))
                        : godot::Vector3(0.0f, 1.0f, 0.0f);
                    for (int64_t k = 0; k < count - 1; ++k) {
                        p[k + 1] = p[k] + dir * lengths[k];
                    }
                } else {
                    // Iterative Backward & Forward Reaching passes
                    for (uint16_t iter = 0; iter < max_iterations; ++iter) {
                        if ((p[count - 1] - chain_target).length_squared() <= tol_sq) {
                            break;
                        }

                        // Pass 1: Backward Reaching (tip -> root)
                        p[count - 1] = chain_target;
                        for (int64_t k = count - 2; k >= 0; --k) {
                            const godot::Vector3 diff = p[k] - p[k + 1];
                            const float d = diff.length();
                            const godot::Vector3 dir = (d > 1e-6f) ? (diff * (1.0f / d)) : godot::Vector3(0.0f, 1.0f, 0.0f);
                            p[k] = p[k + 1] + dir * lengths[k];
                        }

                        if (!lock_root) break;

                        // Pass 2: Forward Reaching (root -> tip) with optional conical angle constraint
                        p[0] = root_origin;
                        for (int64_t k = 0; k < count - 1; ++k) {
                            const godot::Vector3 diff = p[k + 1] - p[k];
                            const float d = diff.length();
                            godot::Vector3 dir = (d > 1e-6f) ? (diff * (1.0f / d)) : godot::Vector3(0.0f, 1.0f, 0.0f);

                            if (max_cone_angle_cos > -0.9999f && k > 0) {
                                const godot::Vector3 prev_seg = p[k] - p[k - 1];
                                const float prev_len = prev_seg.length();
                                if (prev_len > 1e-6f) {
                                    const godot::Vector3 prev_dir = prev_seg * (1.0f / prev_len);
                                    const float cos_ang = dir.dot(prev_dir);
                                    if (cos_ang < max_cone_angle_cos) {
                                        godot::Vector3 perp = dir - prev_dir * cos_ang;
                                        const float perp_len = perp.length();
                                        if (perp_len > 1e-6f) {
                                            perp *= (1.0f / perp_len);
                                            dir = (prev_dir * max_cone_angle_cos + perp * sin_max_cone).normalized();
                                        } else {
                                            dir = prev_dir;
                                        }
                                    }
                                }
                            }

                            p[k + 1] = p[k] + dir * lengths[k];
                        }
                    }
                }

                // Write blended joint positions and optional segment rotations
                for (int64_t k = 0; k < count; ++k) {
                    const int64_t g_idx = start_idx + k;
                    const godot::Vector3 orig_p = _read_view(p_view, g_idx);
                    const godot::Vector3 final_p = orig_p + (p[k] - orig_p) * ik_weight;
                    p[k] = final_p;
                    _write_view(p_view, g_idx, final_p);
                }

                if (joint_rots) {
                    for (int64_t k = 0; k < count - 1; ++k) {
                        const int64_t g_idx = start_idx + k;
                        if (g_idx >= rot_cap) break;
                        const godot::Vector3 seg_dir = (p[k + 1] - p[k]).normalized();
                        const godot::Quaternion q = _shortest_arc_quat(godot::Vector3(0.0f, 1.0f, 0.0f), seg_dir);
                        joint_rots[g_idx] = godot::Vector4(q.x, q.y, q.z, q.w);
                    }
                }
            };

            if (chain_ranges && range_cap > 0) {
                for (int64_t c = 0; c < range_cap; ++c) {
                    const godot::Vector2i rng = chain_ranges[c];
                    const godot::Vector3 tgt  = (targets && c < target_cap) ? targets[c] : target_position;
                    solve_chain(rng.x, rng.y, tgt);
                }
            } else {
                const MemoryBufferSelectionPOD& sel = jpos_part->selection;
                const int64_t start_idx = (sel.mode == SelectionMode::RANGE) ? sel.start_index : 0;
                const int64_t count     = (sel.element_count >= 2) ? sel.element_count : total_joints;
                const godot::Vector3 tgt = (targets && target_cap > 0) ? targets[0] : target_position;
                solve_chain(start_idx, count, tgt);
            }
        }
    }

private:
    [[nodiscard]] static inline godot::Quaternion _shortest_arc_quat(const godot::Vector3& p_from, const godot::Vector3& p_to) noexcept {
        const float d = p_from.dot(p_to);
        if (d >= 0.999999f) return godot::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        if (d <= -0.999999f) {
            godot::Vector3 axis = godot::Vector3(1.0f, 0.0f, 0.0f).cross(p_from);
            if (axis.length_squared() < 1e-6f) axis = godot::Vector3(0.0f, 1.0f, 0.0f).cross(p_from);
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

static_assert(sizeof(FabrikChainIKTransformLogic<>) == 64, "FabrikChainIKTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core