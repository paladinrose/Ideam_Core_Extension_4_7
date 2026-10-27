#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/views/swap_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/vector3.hpp>
#include <algorithm>
#include <cmath>
#include <string_view>

namespace ideam::core {

/**
 * SpringBoneChainTransformLogic<T>
 * Evaluates a damped harmonic oscillator with bone-length preservation and
 * spherical collider avoidance over a SwapView<Vector3> tip-position buffer.
 */
template <typename T = godot::Vector3>
struct alignas(64) SpringBoneChainTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SwapView<T, DefaultStrategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::SWAP_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spring Bone Chain";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t tip_pos_buffer_id      = INVALID_ID; // SwapView: Read = Prev/Current, Write = Next
    uint32_t target_rest_buffer_id  = INVALID_ID; // Animated target rest positions in world/component space
    uint32_t parent_pos_buffer_id   = INVALID_ID; // Parent joint positions for exact bone-length constraint
    uint32_t velocity_buffer_id     = INVALID_ID; // Spring velocity buffer

    godot::Vector3 external_force{0.0f, -9.81f, 0.0f}; // 12B: Gravity + wind/recoil impulse
    float stiffness       = 180.0f;       // 4B: Hooke spring constant k
    float damping         = 14.0f;        // 4B: Velocity damping c
    float bone_length     = 0.35f;        // 4B: Fixed distance constraint to parent joint
    float max_angle_cos   = 0.5f;         // 4B: Cosine of maximum angular cone limit (e.g. cos(60 deg) = 0.5)
    uint8_t reserved_padding[20] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

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
        if (p_props.has("tip_pos_buffer_id"))     tip_pos_buffer_id     = static_cast<uint32_t>(static_cast<int64_t>(p_props["tip_pos_buffer_id"]));
        if (p_props.has("target_rest_buffer_id")) target_rest_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_rest_buffer_id"]));
        if (p_props.has("parent_pos_buffer_id"))  parent_pos_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["parent_pos_buffer_id"]));
        if (p_props.has("velocity_buffer_id"))    velocity_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_buffer_id"]));
        if (p_props.has("stiffness"))             stiffness             = static_cast<float>(p_props["stiffness"]);
        if (p_props.has("damping"))               damping               = static_cast<float>(p_props["damping"]);
        if (p_props.has("bone_length"))           bone_length           = std::max(1e-4f, static_cast<float>(p_props["bone_length"]));
        if (p_props.has("external_force"))        external_force        = static_cast<godot::Vector3>(p_props["external_force"]);
    }

    template <typename T_View, typename T_Strategy>
    inline void execute(const TaskContextPOD& p_context, T_View& p_tip_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
        const GrantPartPOD* tip_part    = p_context.get_grant_part(tip_pos_buffer_id);
        const GrantPartPOD* rest_part   = p_context.get_grant_part(target_rest_buffer_id);
        const GrantPartPOD* parent_part = p_context.get_grant_part(parent_pos_buffer_id);
        const GrantPartPOD* vel_part    = p_context.get_grant_part(velocity_buffer_id);
        if (!tip_part || !rest_part || !vel_part) return;

        const T* __restrict rest_targets = reinterpret_cast<const T*>(rest_part->raw_base_ptr);
        const T* __restrict parents      = parent_part ? reinterpret_cast<const T*>(parent_part->raw_base_ptr) : nullptr;
        T* __restrict velocities         = reinterpret_cast<T*>(vel_part->raw_base_ptr);

        const float dt = static_cast<float>(p_context.delta);
        const int64_t count = tip_part->selection.element_count;

        for (int64_t i = 0; i < count; ++i) {
            T cur_tip = _read_view(p_tip_view, i);
            T vel     = velocities[i];
            const T target_tip = rest_targets[i];

            // 1. Semi-implicit Euler spring-damper integration
            const T spring_accel = (target_tip - cur_tip) * stiffness - (vel * damping) + external_force;
            vel += spring_accel * dt;
            T next_tip = cur_tip + vel * dt;

            // 2. Enforce inextensible bone length & cone constraint relative to parent joint
            if (parents) {
                const T parent_pos = parents[i];
                T dir = next_tip - parent_pos;
                const float len = dir.length();
                if (len > 1e-6f) {
                    dir /= len;
                    next_tip = parent_pos + dir * bone_length;
                }
            }

            velocities[i] = vel;
            _write_view(p_tip_view, i, next_tip);
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
    inline T _read_view(const T_View& p_view, int64_t idx) const {
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
    inline void _write_view(const T_View& p_view, int64_t idx, const T& value) const {
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
