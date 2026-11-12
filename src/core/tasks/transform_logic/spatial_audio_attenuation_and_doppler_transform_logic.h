#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * SpatialAudioAttenuationAndDopplerTransformLogic<T, T_Strategy, T_View> (A5)
 * Computes 3D distance attenuation, azimuth stereo panning, air-absorption low-pass filter
 * cutoff frequency, and Doppler pitch shift f' = f * (c + v_L) / (c + v_S) for 3D audio emitters
 * (Part 0 positions, Part 1 velocities), writing Vector4(gain_linear, pitch_scale, pan, lpf_hz) to Part 2.
 */
template <
    typename T = godot::Vector4,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SpatialAudioAttenuationAndDopplerTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR4;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spatial 3D Audio Attenuation, Pan & Doppler";
    static constexpr std::string_view ui_category  = "Audio/Spatial";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Vector3 listener_position = godot::Vector3(0.0f, 0.0f, 0.0f); // 12B
    uint32_t target_buffer_id        = INVALID_ID;                       // 4B -> 16B: Output Vector4(gain, pitch, pan, lpf_hz)
    godot::Vector3 listener_velocity = godot::Vector3(0.0f, 0.0f, 0.0f); // 12B
    uint32_t position_buffer_id      = INVALID_ID;                       // 4B -> 32B: Emitter 3D positions
    godot::Vector3 listener_right    = godot::Vector3(1.0f, 0.0f, 0.0f); // 12B
    uint32_t velocity_buffer_id      = INVALID_ID;                       // 4B -> 48B: Emitter 3D velocities
    float speed_of_sound             = 343.0f;                           // 4B
    float doppler_scale              = 1.0f;                             // 4B
    float unit_distance              = 2.0f;                             // 4B
    float max_distance               = 100.0f;                           // 4B -> Total = 64B

    static constexpr float DEFAULT_LPF_MIN_HZ = 800.0f;
    static constexpr float DEFAULT_LPF_MAX_HZ = 20000.0f;

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("position_buffer_id");
        add_buf("velocity_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("listener_position", godot::Variant::VECTOR3, godot::Vector3(0.0f, 0.0f, 0.0f));
        add_prop("listener_velocity", godot::Variant::VECTOR3, godot::Vector3(0.0f, 0.0f, 0.0f));
        add_prop("listener_right", godot::Variant::VECTOR3, godot::Vector3(1.0f, 0.0f, 0.0f));
        add_prop("speed_of_sound", godot::Variant::FLOAT, 343.0f);
        add_prop("doppler_scale", godot::Variant::FLOAT, 1.0f);
        add_prop("unit_distance", godot::Variant::FLOAT, 2.0f);
        add_prop("max_distance", godot::Variant::FLOAT, 100.0f);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : position_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("position_buffer_id")) position_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["position_buffer_id"]));
        if (p_props.has("velocity_buffer_id")) velocity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["velocity_buffer_id"]));
        if (p_props.has("listener_position")) listener_position = p_props["listener_position"];
        if (p_props.has("listener_velocity")) listener_velocity = p_props["listener_velocity"];
        if (p_props.has("listener_right")) {
            const godot::Vector3 r = p_props["listener_right"];
            listener_right = (r.length_squared() > 1e-8f) ? r.normalized() : godot::Vector3(1.0f, 0.0f, 0.0f);
        }
        if (p_props.has("speed_of_sound")) speed_of_sound = std::max(10.0f, static_cast<float>(p_props["speed_of_sound"]));
        if (p_props.has("doppler_scale")) doppler_scale = std::max(0.0f, static_cast<float>(p_props["doppler_scale"]));
        if (p_props.has("unit_distance")) unit_distance = std::max(1e-3f, static_cast<float>(p_props["unit_distance"]));
        if (p_props.has("max_distance")) max_distance = std::max(unit_distance + 1e-2f, static_cast<float>(p_props["max_distance"]));
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* pos_part = (position_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(position_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);

            const GrantPartPOD* vel_part = (velocity_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(velocity_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 2) ? &p_ctx.grant->parts[1] : nullptr);

            GrantPartPOD* out_part = (target_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(target_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count >= 2) ? &p_ctx.grant->parts[p_ctx.grant->part_count - 1] : nullptr);

            if (!pos_part || !pos_part->raw_base_ptr || !out_part || !out_part->raw_base_ptr) return;

            const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part->element_stride);
            const uint32_t o_stride = std::max<uint32_t>(sizeof(godot::Vector4), out_part->element_stride);
            const int64_t n_emitters = std::min<int64_t>(
                static_cast<int64_t>(pos_part->capacity_bytes / p_stride),
                static_cast<int64_t>(out_part->capacity_bytes / o_stride)
            );

            const uint32_t v_stride = (vel_part && vel_part->raw_base_ptr)
                ? std::max<uint32_t>(sizeof(godot::Vector3), vel_part->element_stride)
                : sizeof(godot::Vector3);
            const int64_t max_v = (vel_part && vel_part->raw_base_ptr)
                ? static_cast<int64_t>(vel_part->capacity_bytes / v_stride)
                : 0;

            const godot::Vector3 right_dir = (listener_right.length_squared() > 1e-8f)
                ? listener_right.normalized()
                : godot::Vector3(1.0f, 0.0f, 0.0f);

            for (int64_t i = 0; i < n_emitters; ++i) {
                const godot::Vector3& src_pos = *reinterpret_cast<const godot::Vector3*>(pos_part->raw_base_ptr + static_cast<size_t>(i) * p_stride);
                const godot::Vector3 to_src = src_pos - listener_position;
                const float dist = to_src.length();

                float gain = 0.0f;
                float pan = 0.0f;
                float pitch = 1.0f;
                float lpf_hz = DEFAULT_LPF_MAX_HZ;

                if (dist < max_distance) {
                    const godot::Vector3 dir = (dist > 1e-5f) ? (to_src / dist) : godot::Vector3(0.0f, 0.0f, -1.0f);

                    const float clamped_d = std::max(unit_distance, dist);
                    const float window = 1.0f - std::clamp((dist - unit_distance) / (max_distance - unit_distance), 0.0f, 1.0f);
                    gain = (unit_distance / clamped_d) * (window * window);

                    pan = std::clamp(dir.dot(right_dir), -1.0f, 1.0f);

                    const float dist_u = std::clamp(dist / max_distance, 0.0f, 1.0f);
                    lpf_hz = DEFAULT_LPF_MAX_HZ + (DEFAULT_LPF_MIN_HZ - DEFAULT_LPF_MAX_HZ) * dist_u;

                    if (doppler_scale > 0.0f && vel_part && vel_part->raw_base_ptr && i < max_v) {
                        const godot::Vector3& src_vel = *reinterpret_cast<const godot::Vector3*>(vel_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);
                        const float v_listener = listener_velocity.dot(dir) * doppler_scale;
                        const float v_source = src_vel.dot(dir) * doppler_scale;
                        const float denom = std::max(10.0f, speed_of_sound + v_source);
                        const float numer = std::max(10.0f, speed_of_sound + v_listener);
                        pitch = std::clamp(numer / denom, 0.25f, 4.0f);
                    }
                }

                *reinterpret_cast<godot::Vector4*>(out_part->raw_base_ptr + static_cast<size_t>(i) * o_stride) =
                    godot::Vector4(gain, pitch, pan, lpf_hz);
            }
        }
    }
};

static_assert(sizeof(SpatialAudioAttenuationAndDopplerTransformLogic<>) == 64, "SpatialAudioAttenuationAndDopplerTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SpatialAudioAttenuationAndDopplerTransformLogic<>>, "SpatialAudioAttenuationAndDopplerTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core