#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_playback_polyphonic.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * AudioPolyphonicStreamTriggerTransformLogic<T, T_Strategy, T_View> (A3)
 * Drives an AudioStreamPlaybackPolyphonic instance from a DOD voice command buffer
 * (Part 0: Vector4(volume_db, pitch_scale, start_offset_sec, gate_command) and Part 1: int64_t active voice stream IDs),
 * triggering, modulating, and stopping polyphonic voices without scene-tree node overhead.
 */
template <
    typename T = godot::Vector4,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) AudioPolyphonicStreamTriggerTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR4 | DataType::INT32 | DataType::INT64;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Audio Polyphonic Stream Voice Trigger & Modulator";
    static constexpr std::string_view ui_category  = "Audio/Synthesis";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Ref<godot::AudioStreamPlaybackPolyphonic> polyphonic_playback; // 8B
    godot::Ref<godot::AudioStream> voice_stream;                          // 8B -> 16B
    uint32_t target_buffer_id       = INVALID_ID;                         // 4B: Voice command Vector4 buffer
    uint32_t stream_id_buffer_id    = INVALID_ID;                         // 4B: Active int64_t stream ID buffer
    uint8_t reserved_padding[56 - sizeof(godot::Ref<godot::AudioStreamPlaybackPolyphonic>) - sizeof(godot::Ref<godot::AudioStream>)] = {0}; // Total = 64B

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
        add_buf("stream_id_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("polyphonic_playback", godot::Variant::OBJECT, godot::Variant());
        add_prop("voice_stream", godot::Variant::OBJECT, godot::Variant());
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : stream_id_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("stream_id_buffer_id")) stream_id_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["stream_id_buffer_id"]));
        if (p_props.has("polyphonic_playback")) polyphonic_playback = p_props["polyphonic_playback"];
        if (p_props.has("voice_stream")) voice_stream = p_props["voice_stream"];
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
            if (polyphonic_playback.is_null()) return;

            GrantPartPOD* cmd_part = (target_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(target_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            GrantPartPOD* id_part = (stream_id_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(stream_id_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);

            if (!cmd_part || !cmd_part->raw_base_ptr || !id_part || !id_part->raw_base_ptr) return;

            godot::AudioStreamPlaybackPolyphonic* pb = polyphonic_playback.ptr();
            const uint32_t c_stride = std::max<uint32_t>(sizeof(godot::Vector4), cmd_part->element_stride);
            const uint32_t i_stride = std::max<uint32_t>(sizeof(int64_t), id_part->element_stride);
            const int64_t n_voices = std::min<int64_t>(
                static_cast<int64_t>(cmd_part->capacity_bytes / c_stride),
                static_cast<int64_t>(id_part->capacity_bytes / i_stride)
            );

            for (int64_t i = 0; i < n_voices; ++i) {
                godot::Vector4& cmd = *reinterpret_cast<godot::Vector4*>(cmd_part->raw_base_ptr + static_cast<size_t>(i) * c_stride);
                int64_t& stream_id = *reinterpret_cast<int64_t*>(id_part->raw_base_ptr + static_cast<size_t>(i) * i_stride);

                const float vol_db = std::clamp(cmd.x, -80.0f, 24.0f);
                const float pitch = std::max(0.01f, cmd.y);
                const float offset = std::max(0.0f, cmd.z);
                const bool gate_active = (cmd.w > 0.5f);

                if (gate_active) {
                    if (stream_id < 0 || !pb->is_stream_playing(stream_id)) {
                        if (voice_stream.is_valid()) {
                            stream_id = pb->play_stream(voice_stream, offset, vol_db, pitch);
                        }
                    } else {
                        pb->set_stream_volume(stream_id, vol_db);
                        pb->set_stream_pitch_scale(stream_id, pitch);
                    }
                } else if (stream_id >= 0) {
                    if (pb->is_stream_playing(stream_id)) {
                        pb->stop_stream(stream_id);
                    }
                    stream_id = -1;
                }
            }
        }
    }
};

static_assert(sizeof(AudioPolyphonicStreamTriggerTransformLogic<>) == 64, "AudioPolyphonicStreamTriggerTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<AudioPolyphonicStreamTriggerTransformLogic<>>, "AudioPolyphonicStreamTriggerTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core