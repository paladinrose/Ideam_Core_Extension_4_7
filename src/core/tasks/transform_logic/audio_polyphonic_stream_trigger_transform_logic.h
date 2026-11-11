#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_playback_polyphonic.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * A3: AudioPolyphonicStreamTriggerTransformLogic
 * Drives an AudioStreamPlaybackPolyphonic instance from a DOD voice command buffer
 * (Part 0: Vector4(volume_db, pitch_scale, start_offset_sec, gate_command) and Part 1: int64_t active voice stream IDs),
 * triggering, modulating, and stopping polyphonic voices without scene-tree node overhead.
 */
struct AudioPolyphonicStreamTriggerTransformLogic {
    static constexpr std::string_view ui_name = "Audio Polyphonic Stream Voice Trigger & Modulator";
    static constexpr std::string_view ui_category = "Audio/Synthesis";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR4,
        DataType::INT32
    };

    godot::Ref<godot::AudioStreamPlaybackPolyphonic> polyphonic_playback;
    godot::Ref<godot::AudioStream> voice_stream;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("polyphonic_playback", godot::Variant::OBJECT, polyphonic_playback);
        add_prop("voice_stream", godot::Variant::OBJECT, voice_stream);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("polyphonic_playback")) polyphonic_playback = p_props["polyphonic_playback"];
        if (p_props.has("voice_stream")) voice_stream = p_props["voice_stream"];
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2 || polyphonic_playback.is_null()) return;

        GrantPartPOD& cmd_part = p_ctx.grant->parts[0]; // Vector4(volume_db, pitch_scale, start_offset, gate: >0.5 play/sustain, <=0 stop)
        GrantPartPOD& id_part = p_ctx.grant->parts[1];  // int64_t polyphonic stream voice IDs (-1 = inactive)
        if (!cmd_part.raw_base_ptr || !id_part.raw_base_ptr) return;

        godot::AudioStreamPlaybackPolyphonic* pb = polyphonic_playback.ptr();
        const uint32_t c_stride = std::max<uint32_t>(sizeof(godot::Vector4), cmd_part.element_stride);
        const uint32_t i_stride = std::max<uint32_t>(sizeof(int64_t), id_part.element_stride);
        const int64_t n_voices = std::min<int64_t>(
            static_cast<int64_t>(cmd_part.capacity_bytes / c_stride),
            static_cast<int64_t>(id_part.capacity_bytes / i_stride)
        );

        for (int64_t i = 0; i < n_voices; ++i) {
            godot::Vector4& cmd = *reinterpret_cast<godot::Vector4*>(cmd_part.raw_base_ptr + static_cast<size_t>(i) * c_stride);
            int64_t& stream_id = *reinterpret_cast<int64_t*>(id_part.raw_base_ptr + static_cast<size_t>(i) * i_stride);

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

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core