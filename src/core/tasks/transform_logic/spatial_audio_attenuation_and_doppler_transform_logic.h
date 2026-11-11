#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * A5: SpatialAudioAttenuationAndDopplerTransformLogic
 * Computes 3D distance attenuation, azimuth stereo panning, air-absorption low-pass filter
 * cutoff frequency, and Doppler pitch shift f' = f * (c + v_L) / (c + v_S) for 3D audio emitters
 * (Part 0 positions, Part 1 velocities), writing Vector4(gain_linear, pitch_scale, pan, lpf_hz) to Part 2.
 */
struct SpatialAudioAttenuationAndDopplerTransformLogic {
    static constexpr std::string_view ui_name = "Spatial 3D Audio Attenuation, Pan & Doppler";
    static constexpr std::string_view ui_category = "Audio/Spatial";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::AOSOA,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::VECTOR4
    };

    godot::Vector3 listener_position = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 listener_velocity = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 listener_right = godot::Vector3(1.0f, 0.0f, 0.0f);
    float speed_of_sound = 343.0f;
    float doppler_scale = 1.0f;
    float unit_distance = 2.0f;
    float max_distance = 100.0f;
    float air_absorption_lpf_min_hz = 800.0f;
    float air_absorption_lpf_max_hz = 20000.0f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("listener_position", godot::Variant::VECTOR3, listener_position);
        add_prop("listener_velocity", godot::Variant::VECTOR3, listener_velocity);
        add_prop("listener_right", godot::Variant::VECTOR3, listener_right);
        add_prop("speed_of_sound", godot::Variant::FLOAT, speed_of_sound);
        add_prop("doppler_scale", godot::Variant::FLOAT, doppler_scale);
        add_prop("unit_distance", godot::Variant::FLOAT, unit_distance);
        add_prop("max_distance", godot::Variant::FLOAT, max_distance);
        add_prop("air_absorption_lpf_min_hz", godot::Variant::FLOAT, air_absorption_lpf_min_hz);
        add_prop("air_absorption_lpf_max_hz", godot::Variant::FLOAT, air_absorption_lpf_max_hz);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("listener_position")) listener_position = p_props["listener_position"];
        if (p_props.has("listener_velocity")) listener_velocity = p_props["listener_velocity"];
        if (p_props.has("listener_right")) {
            godot::Vector3 r = p_props["listener_right"];
            listener_right = (r.length_squared() > 1e-8f) ? r.normalized() : godot::Vector3(1.0f, 0.0f, 0.0f);
        }
        if (p_props.has("speed_of_sound")) speed_of_sound = std::max(10.0f, static_cast<float>(p_props["speed_of_sound"]));
        if (p_props.has("doppler_scale")) doppler_scale = std::max(0.0f, static_cast<float>(p_props["doppler_scale"]));
        if (p_props.has("unit_distance")) unit_distance = std::max(1e-3f, static_cast<float>(p_props["unit_distance"]));
        if (p_props.has("max_distance")) max_distance = std::max(unit_distance + 1e-2f, static_cast<float>(p_props["max_distance"]));
        if (p_props.has("air_absorption_lpf_min_hz")) air_absorption_lpf_min_hz = std::max(20.0f, static_cast<float>(p_props["air_absorption_lpf_min_hz"]));
        if (p_props.has("air_absorption_lpf_max_hz")) air_absorption_lpf_max_hz = std::max(air_absorption_lpf_min_hz, static_cast<float>(p_props["air_absorption_lpf_max_hz"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        const bool has_vel = (p_ctx.grant->part_count > 2);
        const GrantPartPOD* vel_part = has_vel ? &p_ctx.grant->parts[1] : nullptr;
        GrantPartPOD& out_part = p_ctx.grant->parts[p_ctx.grant->part_count - 1];
        if (!pos_part.raw_base_ptr || !out_part.raw_base_ptr) return;

        const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const uint32_t o_stride = std::max<uint32_t>(sizeof(godot::Vector4), out_part.element_stride);
        const int64_t n_emitters = std::min<int64_t>(
            static_cast<int64_t>(pos_part.capacity_bytes / p_stride),
            static_cast<int64_t>(out_part.capacity_bytes / o_stride)
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
            const godot::Vector3& src_pos = *reinterpret_cast<const godot::Vector3*>(pos_part.raw_base_ptr + static_cast<size_t>(i) * p_stride);
            const godot::Vector3 to_src = src_pos - listener_position;
            const float dist = to_src.length();

            float gain = 0.0f;
            float pan = 0.0f;
            float pitch = 1.0f;
            float lpf_hz = air_absorption_lpf_max_hz;

            if (dist < max_distance) {
                const godot::Vector3 dir = (dist > 1e-5f) ? (to_src / dist) : godot::Vector3(0.0f, 0.0f, -1.0f);

                // Smooth inverse-distance attenuation clamped at unit_distance and tapering to 0 at max_distance
                const float clamped_d = std::max(unit_distance, dist);
                const float window = 1.0f - std::clamp((dist - unit_distance) / (max_distance - unit_distance), 0.0f, 1.0f);
                gain = (unit_distance / clamped_d) * (window * window);

                // Azimuth stereo pan [-1, +1]
                pan = std::clamp(dir.dot(right_dir), -1.0f, 1.0f);

                // Air-absorption LPF rolloff with distance
                const float dist_u = std::clamp(dist / max_distance, 0.0f, 1.0f);
                lpf_hz = air_absorption_lpf_max_hz + (air_absorption_lpf_min_hz - air_absorption_lpf_max_hz) * dist_u;

                // Relativistic / acoustic Doppler pitch shift along line-of-sight dir
                if (doppler_scale > 0.0f && vel_part && i < max_v) {
                    const godot::Vector3& src_vel = *reinterpret_cast<const godot::Vector3*>(vel_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);
                    const float v_listener = listener_velocity.dot(dir) * doppler_scale;
                    const float v_source = src_vel.dot(dir) * doppler_scale;
                    const float denom = std::max(10.0f, speed_of_sound + v_source);
                    const float numer = std::max(10.0f, speed_of_sound + v_listener);
                    pitch = std::clamp(numer / denom, 0.25f, 4.0f);
                }
            }

            *reinterpret_cast<godot::Vector4*>(out_part.raw_base_ptr + static_cast<size_t>(i) * o_stride) =
                godot::Vector4(gain, pitch, pan, lpf_hz);
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core