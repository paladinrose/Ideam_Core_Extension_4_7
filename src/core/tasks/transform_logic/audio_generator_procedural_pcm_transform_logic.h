#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * A4: AudioGeneratorProceduralPCMTransformLogic
 * Pushes procedural stereo/mono PCM audio frames from a DOD Ring or Flat buffer into
 * an AudioStreamGeneratorPlayback buffer with constant-power stereo pan and soft-clip limiting.
 */
struct AudioGeneratorProceduralPCMTransformLogic {
    static constexpr std::string_view ui_name = "AudioStreamGenerator Procedural PCM Push";
    static constexpr std::string_view ui_category = "Audio/Synthesis";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::RING,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 1> supported_strategies = {
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR2,
        DataType::FLOAT32
    };

    godot::Ref<godot::AudioStreamGeneratorPlayback> generator_playback;
    float master_gain = 1.0f;
    float stereo_pan = 0.0f; // -1 = Left, 0 = Center, +1 = Right (used when input is mono float)
    bool apply_soft_clip_limiter = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("generator_playback", godot::Variant::OBJECT, generator_playback);
        add_prop("master_gain", godot::Variant::FLOAT, master_gain);
        add_prop("stereo_pan", godot::Variant::FLOAT, stereo_pan);
        add_prop("apply_soft_clip_limiter", godot::Variant::BOOL, apply_soft_clip_limiter);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("generator_playback")) generator_playback = p_props["generator_playback"];
        if (p_props.has("master_gain")) master_gain = std::max(0.0f, static_cast<float>(p_props["master_gain"]));
        if (p_props.has("stereo_pan")) stereo_pan = std::clamp(static_cast<float>(p_props["stereo_pan"]), -1.0f, 1.0f);
        if (p_props.has("apply_soft_clip_limiter")) apply_soft_clip_limiter = static_cast<bool>(p_props["apply_soft_clip_limiter"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || generator_playback.is_null()) return;

        const GrantPartPOD& pcm_part = p_ctx.grant->parts[0];
        if (!pcm_part.raw_base_ptr) return;

        godot::AudioStreamGeneratorPlayback* pb = generator_playback.ptr();
        const int32_t avail_frames = pb->get_frames_available();
        if (avail_frames <= 0) return;

        const bool is_stereo = (pcm_part.element_stride >= sizeof(godot::Vector2));
        const uint32_t stride = std::max<uint32_t>(is_stereo ? sizeof(godot::Vector2) : sizeof(float), pcm_part.element_stride);
        const int32_t src_frames = (pcm_part.selection.element_count > 0)
            ? static_cast<int32_t>(pcm_part.selection.element_count)
            : static_cast<int32_t>(pcm_part.capacity_bytes / stride);
        const int32_t frames_to_push = std::min(avail_frames, src_frames);
        if (frames_to_push <= 0) return;

        // Constant-power pan weights for mono source
        constexpr float QUARTER_PI = 0.7853981633974483f;
        const float pan_angle = (stereo_pan + 1.0f) * QUARTER_PI;
        const float gain_l = std::cos(pan_angle) * master_gain;
        const float gain_r = std::sin(pan_angle) * master_gain;

        godot::PackedVector2Array out_frames;
        out_frames.resize(frames_to_push);
        godot::Vector2* dst = out_frames.ptrw();

        for (int32_t i = 0; i < frames_to_push; ++i) {
            godot::Vector2 sample;
            if (is_stereo) {
                sample = *reinterpret_cast<const godot::Vector2*>(pcm_part.raw_base_ptr + static_cast<size_t>(i) * stride) * master_gain;
            } else {
                const float m = *reinterpret_cast<const float*>(pcm_part.raw_base_ptr + static_cast<size_t>(i) * stride);
                sample = godot::Vector2(m * gain_l, m * gain_r);
            }

            if (apply_soft_clip_limiter) {
                sample.x = std::tanh(sample.x);
                sample.y = std::tanh(sample.y);
            } else {
                sample.x = std::clamp(sample.x, -1.0f, 1.0f);
                sample.y = std::clamp(sample.y, -1.0f, 1.0f);
            }
            dst[i] = sample;
        }

        pb->push_buffer(out_frames);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core