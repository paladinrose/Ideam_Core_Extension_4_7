#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/classes/audio_effect_spectrum_analyzer_instance.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * A1: AudioSpectrumAnalyzerCaptureTransformLogic
 * Queries Godot's AudioEffectSpectrumAnalyzerInstance on a target AudioServer bus and effect slot
 * across logarithmically or linearly spaced frequency bands [min_hz, max_hz], writing smoothed
 * magnitude or dB values into a DOD float or Vector2 buffer.
 */
struct AudioSpectrumAnalyzerCaptureTransformLogic {
    static constexpr std::string_view ui_name = "AudioServer Spectrum Analyzer Band Capture";
    static constexpr std::string_view ui_category = "Audio/Analysis";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::FLOAT32,
        DataType::VECTOR2
    };

    int32_t bus_index = 0;
    int32_t effect_index = 0;
    int32_t num_bands = 32;
    float min_frequency_hz = 20.0f;
    float max_frequency_hz = 16000.0f;
    int32_t magnitude_mode = 1; // 0 = Average, 1 = Max
    float attack_smoothing = 0.65f;
    float release_smoothing = 0.20f;
    bool logarithmic_bands = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            list.push_back(d);
        };
        add_prop("bus_index", godot::Variant::INT, bus_index);
        add_prop("effect_index", godot::Variant::INT, effect_index);
        add_prop("num_bands", godot::Variant::INT, num_bands);
        add_prop("min_frequency_hz", godot::Variant::FLOAT, min_frequency_hz);
        add_prop("max_frequency_hz", godot::Variant::FLOAT, max_frequency_hz);
        add_prop("magnitude_mode", godot::Variant::INT, magnitude_mode, 2, "Average Magnitude,Peak Max Magnitude");
        add_prop("attack_smoothing", godot::Variant::FLOAT, attack_smoothing);
        add_prop("release_smoothing", godot::Variant::FLOAT, release_smoothing);
        add_prop("logarithmic_bands", godot::Variant::BOOL, logarithmic_bands);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("bus_index")) bus_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["bus_index"]));
        if (p_props.has("effect_index")) effect_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["effect_index"]));
        if (p_props.has("num_bands")) num_bands = std::clamp<int32_t>(static_cast<int32_t>(p_props["num_bands"]), 1, 2048);
        if (p_props.has("min_frequency_hz")) min_frequency_hz = std::max(1.0f, static_cast<float>(p_props["min_frequency_hz"]));
        if (p_props.has("max_frequency_hz")) max_frequency_hz = std::max(min_frequency_hz + 1.0f, static_cast<float>(p_props["max_frequency_hz"]));
        if (p_props.has("magnitude_mode")) magnitude_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["magnitude_mode"]), 0, 1);
        if (p_props.has("attack_smoothing")) attack_smoothing = std::clamp(static_cast<float>(p_props["attack_smoothing"]), 0.01f, 1.0f);
        if (p_props.has("release_smoothing")) release_smoothing = std::clamp(static_cast<float>(p_props["release_smoothing"]), 0.01f, 1.0f);
        if (p_props.has("logarithmic_bands")) logarithmic_bands = static_cast<bool>(p_props["logarithmic_bands"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& out_part = p_ctx.grant->parts[0];
        if (!out_part.raw_base_ptr) return;

        godot::AudioServer* as = godot::AudioServer::get_singleton();
        if (!as || bus_index < 0 || bus_index >= as->get_bus_count()) return;
        if (effect_index < 0 || effect_index >= as->get_bus_effect_count(bus_index)) return;

        godot::Ref<godot::AudioEffectInstance> fx_inst = as->get_bus_effect_instance(bus_index, effect_index);
        godot::AudioEffectSpectrumAnalyzerInstance* spec =
            godot::Object::cast_to<godot::AudioEffectSpectrumAnalyzerInstance>(fx_inst.ptr());
        if (!spec) return;

        const bool is_stereo_v2 = (out_part.element_stride >= sizeof(godot::Vector2));
        const uint32_t stride = std::max<uint32_t>(is_stereo_v2 ? sizeof(godot::Vector2) : sizeof(float), out_part.element_stride);
        const int32_t max_elems = static_cast<int32_t>(out_part.capacity_bytes / stride);
        const int32_t bands = std::min<int32_t>(num_bands, max_elems);
        if (bands <= 0) return;

        const float log_ratio = std::log(max_frequency_hz / min_frequency_hz);
        const auto mode_enum = (magnitude_mode == 1)
            ? godot::AudioEffectSpectrumAnalyzerInstance::MAGNITUDE_MAX
            : godot::AudioEffectSpectrumAnalyzerInstance::MAGNITUDE_AVERAGE;

        for (int32_t b = 0; b < bands; ++b) {
            const float u0 = static_cast<float>(b) / static_cast<float>(bands);
            const float u1 = static_cast<float>(b + 1) / static_cast<float>(bands);

            const float f0 = logarithmic_bands
                ? (min_frequency_hz * std::exp(log_ratio * u0))
                : (min_frequency_hz + (max_frequency_hz - min_frequency_hz) * u0);
            const float f1 = logarithmic_bands
                ? (min_frequency_hz * std::exp(log_ratio * u1))
                : (min_frequency_hz + (max_frequency_hz - min_frequency_hz) * u1);

            const godot::Vector2 stereo_mag = spec->get_magnitude_for_frequency_range(f0, f1, mode_enum);
            uint8_t* dst = out_part.raw_base_ptr + static_cast<size_t>(b) * stride;

            if (is_stereo_v2) {
                godot::Vector2& cur = *reinterpret_cast<godot::Vector2*>(dst);
                const float ax = (stereo_mag.x >= cur.x) ? attack_smoothing : release_smoothing;
                const float ay = (stereo_mag.y >= cur.y) ? attack_smoothing : release_smoothing;
                cur.x += (stereo_mag.x - cur.x) * ax;
                cur.y += (stereo_mag.y - cur.y) * ay;
            } else {
                float& cur = *reinterpret_cast<float*>(dst);
                const float mono = 0.5f * (stereo_mag.x + stereo_mag.y);
                const float alpha = (mono >= cur) ? attack_smoothing : release_smoothing;
                cur += (mono - cur) * alpha;
            }
        }

        out_part.selection.element_count = bands;
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core