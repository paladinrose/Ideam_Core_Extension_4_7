#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/ring_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * AudioGeneratorProceduralPCMTransformLogic<T, T_Strategy, T_View> (A4)
 * Pushes procedural stereo/mono PCM audio frames from a DOD Ring or Flat buffer into
 * an AudioStreamGeneratorPlayback buffer with constant-power stereo pan and soft-clip limiting.
 */
template <
    typename T = godot::Vector2,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = RingView
>
struct alignas(64) AudioGeneratorProceduralPCMTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities =
        ViewCapability::LINEAR_ACCESS | ViewCapability::QUEUE_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::RING | BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types =
        DataType::VECTOR2 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "AudioStreamGenerator Procedural PCM Push";
    static constexpr std::string_view ui_category  = "Audio/Synthesis";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Ref<godot::AudioStreamGeneratorPlayback> generator_playback; // 8B
    uint32_t target_buffer_id       = INVALID_ID;                       // 4B: PCM sample buffer (Vector2 or float)
    float master_gain               = 1.0f;                             // 4B -> 16B
    float stereo_pan                = 0.0f;                             // 4B: -1 = Left, 0 = Center, +1 = Right
    bool apply_soft_clip_limiter    = true;                             // 1B
    uint8_t reserved_padding[47 - sizeof(godot::Ref<godot::AudioStreamGeneratorPlayback>)] = {0}; // Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("generator_playback", godot::Variant::OBJECT, godot::Variant());
        add_prop("master_gain", godot::Variant::FLOAT, 1.0f);
        add_prop("stereo_pan", godot::Variant::FLOAT, 0.0f);
        add_prop("apply_soft_clip_limiter", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("generator_playback")) generator_playback = p_props["generator_playback"];
        if (p_props.has("master_gain")) master_gain = std::max(0.0f, static_cast<float>(p_props["master_gain"]));
        if (p_props.has("stereo_pan")) stereo_pan = std::clamp(static_cast<float>(p_props["stereo_pan"]), -1.0f, 1.0f);
        if (p_props.has("apply_soft_clip_limiter")) apply_soft_clip_limiter = static_cast<bool>(p_props["apply_soft_clip_limiter"]);
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
            if (generator_playback.is_null()) return;

            const GrantPartPOD* pcm_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!pcm_part || !pcm_part->raw_base_ptr) return;

            godot::AudioStreamGeneratorPlayback* pb = generator_playback.ptr();
            const int32_t avail_frames = pb->get_frames_available();
            if (avail_frames <= 0) return;

            const bool is_stereo = (pcm_part->element_stride >= sizeof(godot::Vector2));
            const uint32_t stride = std::max<uint32_t>(is_stereo ? sizeof(godot::Vector2) : sizeof(float), pcm_part->element_stride);
            const int32_t src_frames = (pcm_part->selection.element_count > 0)
                ? static_cast<int32_t>(pcm_part->selection.element_count)
                : static_cast<int32_t>(pcm_part->capacity_bytes / stride);
            const int32_t frames_to_push = std::min(avail_frames, src_frames);
            if (frames_to_push <= 0) return;

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
                    sample = *reinterpret_cast<const godot::Vector2*>(pcm_part->raw_base_ptr + static_cast<size_t>(i) * stride) * master_gain;
                } else {
                    const float m = *reinterpret_cast<const float*>(pcm_part->raw_base_ptr + static_cast<size_t>(i) * stride);
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
    }
};

static_assert(sizeof(AudioGeneratorProceduralPCMTransformLogic<>) == 64, "AudioGeneratorProceduralPCMTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<AudioGeneratorProceduralPCMTransformLogic<>>, "AudioGeneratorProceduralPCMTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core