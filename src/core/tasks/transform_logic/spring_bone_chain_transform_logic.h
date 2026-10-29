#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * SpringBoneChainTransformLogic<T>
 * Evaluates a damped harmonic oscillator with inextensible bone-length preservation,
 * angular cone clamping relative to the rest axis (max_angle_cos), and sphere-collider
 * pushout (collider_sphere_buffer_id of Vector4(cx, cy, cz, radius)) over a
 * SwapView<Vector3> tip-position buffer.
 */
template <typename T = godot::Vector3>
struct alignas(64) SpringBoneChainTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::SWAP_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spring Bone Chain";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t tip_pos_buffer_id         = INVALID_ID; // 4B: SwapView: Read = Prev/Current, Write = Next
    uint32_t target_rest_buffer_id     = INVALID_ID; // 4B: Animated target rest positions in world/component space
    uint32_t parent_pos_buffer_id      = INVALID_ID; // 4B: Parent joint positions for bone-length & cone constraint
    uint32_t velocity_buffer_id        = INVALID_ID; // 4B: Spring velocity buffer (Vector3)
    uint32_t collider_sphere_buffer_id = INVALID_ID; // 4B: Optional Vector4(cx, cy, cz, radius) spherical colliders
    uint32_t bone_length_buffer_id     = INVALID_ID; // 4B: Optional per-bone float length buffer

    godot::Vector3 external_force{0.0f, -9.81f, 0.0f}; // 12B: Gravity + wind/recoil impulse
    float stiffness       = 180.0f;       // 4B: Hooke spring constant k
    float damping         = 14.0f;        // 4B: Velocity damping c
    float bone_length     = 0.35f;        // 4B: Default fixed distance constraint to parent joint
    float max_angle_cos   = 0.5f;         // 4B: Cosine of maximum angular cone limit (e.g. cos(60 deg) = 0.5)
    float tip_radius      = 0.02f;        // 4B: Spring bone tip radius for sphere collider pushout
    uint8_t reserved_padding[8] = {0};    // 8B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary tip_prop;
        tip_prop["name"] = "tip_pos_buffer_id";
        tip_prop["type"] = godot::Variant::INT;
        tip_prop["hint_string"] = "buffer_option";
        props.push_back(tip_prop);

        godot::Dictionary rest_prop;
        rest_prop["name"] = "target_rest_buffer_id";
        rest_prop["type"] = godot::Variant::INT;
        rest_prop["hint_string"] = "buffer_option";
        props.push_back(rest_prop);

        godot::Dictionary par_prop;
        par_prop["name"] = "parent_pos_buffer_id";
        par_prop["type"] = godot::Variant::INT;
        par_prop["hint_string"] = "buffer_option";
        props.push_back(par_prop);

        godot::Dictionary vel_prop;
        vel_prop["name"] = "velocity_buffer_id";
        vel_prop["type"] = godot::Variant::INT;
        vel_prop["hint_string"] = "buffer_option";
        props.push_back(vel_prop);

        godot::Dictionary col_prop;
        col_prop["name"] = "collider_sphere_buffer_id";
        col_prop["type"] = godot::Variant::INT;
        col_prop["hint_string"] = "buffer_option";
        props.push_back(col_prop);

        godot::Dictionary blen_prop;
        blen_prop["name"] = "bone_length_buffer_id";
        blen_prop["type"] = godot::Variant::INT;
        blen_prop["hint_string"] = "buffer_option";
        props.push_back(blen_prop);

        godot::Dictionary stiff_prop;
        stiff_prop["name"] = "stiffness";
        stiff_prop["type"] = godot::Variant::FLOAT;
        stiff_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        stiff_prop["hint_string"] = "1.0,1000.0,1.0";
        props.push_back(stiff_prop);

        godot::Dictionary damp_prop;
        damp_prop["name"] = "damping";
        damp_prop["type"] = godot::Variant::FLOAT;
        damp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        damp_prop["hint_string"] = "0.1,100.0,0.1";
        props.push_back(damp_prop);

        godot::Dictionary len_prop;
        len_prop["name"] = "bone_length";
        len_prop["type"] = godot::Variant::FLOAT;
        len_prop["hint_string"] = "suffix:m";
        props.push_back(len_prop);

        godot::Dictionary cone_prop;
        cone_prop["name"] = "max_angle_cos";
        cone_prop["type"] = godot::Variant::FLOAT;
        cone_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        cone_prop["hint_string"] = "-1.0,1.0,0.01";
        props.push_back(cone_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "tip_radius";
        rad_prop["type"] = godot::Variant::FLOAT;
        rad_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rad_prop["hint_string"] = "0.0,2.0,0.005,suffix:m";
        props.push_back(rad_prop);

        godot::Dictionary force_prop;
        force_prop["name"] = "external_force";
        force_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(force_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return tip_pos_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("tip_pos_buffer_id"))         tip_pos_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["tip_pos_buffer_id"]));
        if (p_props.has("target_rest_buffer_id"))     target_rest_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_rest_buffer_id"]));
        if (p_props.has("parent_pos_buffer_id"))      parent_pos_buffer_id      = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_pos_buffer_id"]));
        if (p_props.has("velocity_buffer_id"))        velocity_buffer_id        = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_buffer_id"]));
        if (p_props.has("collider_sphere_buffer_id")) collider_sphere_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["collider_sphere_buffer_id"]));
        if (p_props.has("bone_length_buffer_id"))     bone_length_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["bone_length_buffer_id"]));
        if (p_props.has("stiffness"))                 stiffness                 = static_cast<float>(p_props["stiffness"]);
        if (p_props.has("damping"))                   damping                   = static_cast<float>(p_props["damping"]);
        if (p_props.has("bone_length"))               bone_length               = std::max(1e-4f, static_cast<float>(p_props["bone_length"]));
        if (p_props.has("max_angle_cos"))             max_angle_cos             = std::clamp(static_cast<float>(p_props["max_angle_cos"]), -1.0f, 1.0f);
        if (p_props.has("tip_radius"))                tip_radius                = std::max(0.0f, static_cast<float>(p_props["tip_radius"]));
        if (p_props.has("external_force"))            external_force            = static_cast<godot::Vector3>(p_props["external_force"]);
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_tip_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* tip_part    = p_context.get_grant_part(tip_pos_buffer_id);
            const GrantPartPOD* rest_part   = p_context.get_grant_part(target_rest_buffer_id);
            const GrantPartPOD* parent_part = (parent_pos_buffer_id != INVALID_ID) ? p_context.get_grant_part(parent_pos_buffer_id) : nullptr;
            const GrantPartPOD* vel_part    = p_context.get_grant_part(velocity_buffer_id);
            if (!tip_part || !rest_part || !vel_part || !rest_part->raw_base_ptr || !vel_part->raw_base_ptr) return;

            const T* __restrict rest_targets = reinterpret_cast<const T*>(rest_part->raw_base_ptr);
            const T* __restrict parents      = (parent_part && parent_part->raw_base_ptr) ? reinterpret_cast<const T*>(parent_part->raw_base_ptr) : nullptr;
            T* __restrict velocities         = reinterpret_cast<T*>(vel_part->raw_base_ptr);

            const GrantPartPOD* blen_part = (bone_length_buffer_id != INVALID_ID) ? p_context.get_grant_part(bone_length_buffer_id) : nullptr;
            const float* __restrict bone_lengths = (blen_part && blen_part->raw_base_ptr)
                ? reinterpret_cast<const float*>(blen_part->raw_base_ptr)
                : nullptr;
            const int64_t blen_cap = blen_part ? static_cast<int64_t>(blen_part->capacity_bytes / sizeof(float)) : 0;

            const GrantPartPOD* col_part = (collider_sphere_buffer_id != INVALID_ID) ? p_context.get_grant_part(collider_sphere_buffer_id) : nullptr;
            const godot::Vector4* __restrict colliders = (col_part && col_part->raw_base_ptr)
                ? reinterpret_cast<const godot::Vector4*>(col_part->raw_base_ptr)
                : nullptr;
            const int64_t col_count = col_part
                ? (col_part->selection.element_count > 0
                    ? col_part->selection.element_count
                    : static_cast<int64_t>(col_part->capacity_bytes / sizeof(godot::Vector4)))
                : 0;

            const float dt = static_cast<float>(p_context.delta);
            if (dt <= 1e-7f) return;
            const float inv_dt = 1.0f / dt;

            const float sin_max_angle = std::sqrt(std::max(0.0f, 1.0f - max_angle_cos * max_angle_cos));

            auto eval_spring = [&](int64_t i) noexcept {
                const T cur_tip    = _read_view(p_tip_view, i);
                T vel              = velocities[i];
                const T target_tip = rest_targets[i];

                // 1. Semi-implicit Euler spring-damper integration
                const T spring_accel = (target_tip - cur_tip) * stiffness - (vel * damping) + external_force;
                vel += spring_accel * dt;
                T next_tip = cur_tip + vel * dt;

                // 2. Spherical collider pushout (Vector4: xyz = sphere center, w = sphere radius)
                if (colliders && col_count > 0) {
                    for (int64_t c = 0; c < col_count; ++c) {
                        const godot::Vector4& sph = colliders[c];
                        const T center(sph.x, sph.y, sph.z);
                        const float combined_r = std::max(0.0f, sph.w) + tip_radius;
                        const T diff = next_tip - center;
                        const float dist_sq = diff.length_squared();
                        if (dist_sq < combined_r * combined_r) {
                            if (dist_sq > 1e-10f) {
                                next_tip = center + diff * (combined_r / std::sqrt(dist_sq));
                            } else {
                                next_tip = center + T(0.0f, combined_r, 0.0f);
                            }
                        }
                    }
                }

                // 3. Enforce inextensible bone length & angular cone limit relative to parent joint
                if (parents) {
                    const T parent_pos = parents[i];
                    const float target_len = (bone_lengths && i < blen_cap && bone_lengths[i] > 1e-4f)
                        ? bone_lengths[i]
                        : bone_length;

                    T dir = next_tip - parent_pos;
                    const float len = dir.length();
                    if (len > 1e-6f) {
                        dir *= (1.0f / len);

                        // Clamp dir within angular cone around rest direction (target_tip - parent_pos)
                        if (max_angle_cos > -0.9999f) {
                            const T rest_vec = target_tip - parent_pos;
                            const float rest_len = rest_vec.length();
                            if (rest_len > 1e-6f) {
                                const T rest_dir = rest_vec * (1.0f / rest_len);
                                const float cos_theta = dir.dot(rest_dir);
                                if (cos_theta < max_angle_cos) {
                                    T perp = dir - rest_dir * cos_theta;
                                    const float perp_len = perp.length();
                                    if (perp_len > 1e-6f) {
                                        perp *= (1.0f / perp_len);
                                        dir = (rest_dir * max_angle_cos + perp * sin_max_angle).normalized();
                                    } else {
                                        dir = rest_dir;
                                    }
                                }
                            }
                        }

                        next_tip = parent_pos + dir * target_len;
                    } else {
                        next_tip = target_tip;
                    }
                }

                // Update velocity from constrained displacement to prevent momentum buildup into constraints
                velocities[i] = (next_tip - cur_tip) * inv_dt;
                _write_view(p_tip_view, i, next_tip);
            };

            const MemoryBufferSelectionPOD& sel = tip_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) eval_spring(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    eval_spring(sel.data.indices[i]);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end; ++i) {
                    eval_spring(i);
                }
            }
        }
    }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    inline T _read_view(const T_View& p_view, int64_t idx) const noexcept {
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
    inline void _write_view(const T_View& p_view, int64_t idx, const T& value) const noexcept {
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

static_assert(sizeof(SpringBoneChainTransformLogic<godot::Vector3>) == 64, "SpringBoneChainTransformLogic must be 64 bytes!");

} // namespace ideam::core