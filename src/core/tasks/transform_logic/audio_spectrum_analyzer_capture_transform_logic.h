#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/audio_effect_instance.hpp>
#include <godot_cpp/classes/audio_effect_spectrum_analyzer_instance.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * AudioSpectrumAnalyzerCaptureTransformLogic<T, T_Strategy, T_View> (A1)
 * Queries Godot's AudioEffectSpectrumAnalyzerInstance on a target AudioServer bus and effect slot
 * across logarithmically or linearly spaced frequency bands [min_hz, max_hz], writing smoothed
 * magnitude values into a DOD float or Vector2 buffer.
 */
template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) AudioSpectrumAnalyzerCaptureTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::VECTOR2;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "AudioServer Spectrum Analyzer Band Capture";
    static constexpr std::string_view ui_category  = "Audio/Analysis";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id    = INVALID_ID; // 4B
    int32_t bus_index            = 0;          // 4B
    int32_t effect_index         = 0;          // 4B
    int32_t num_bands            = 32;         // 4B -> 16B
    float min_frequency_hz       = 20.0f;      // 4B
    float max_frequency_hz       = 16000.0f;   // 4B
    int32_t magnitude_mode       = 1;          // 4B: 0 = Average, 1 = Max
    float attack_smoothing       = 0.65f;      // 4B -> 32B
    float release_smoothing      = 0.20f;      // 4B
    bool logarithmic_bands       = true;       // 1B
    uint8_t reserved_padding[27] = {0};        // 27B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("bus_index", godot::Variant::INT, 0);
        add_prop("effect_index", godot::Variant::INT, 0);
        add_prop("num_bands", godot::Variant::INT, 32);
        add_prop("min_frequency_hz", godot::Variant::FLOAT, 20.0f);
        add_prop("max_frequency_hz", godot::Variant::FLOAT, 16000.0f);
        add_prop("magnitude_mode", godot::Variant::INT, 1, 2, "Average Magnitude,Peak Max Magnitude");
        add_prop("attack_smoothing", godot::Variant::FLOAT, 0.65f);
        add_prop("release_smoothing", godot::Variant::FLOAT, 0.20f);
        add_prop("logarithmic_bands", godot::Variant::BOOL, true);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
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

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            GrantPartPOD* out_part = (target_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(target_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!out_part || !out_part->raw_base_ptr) return;

            godot::AudioServer* as = godot::AudioServer::get_singleton();
            if (!as || bus_index < 0 || bus_index >= as->get_bus_count()) return;
            if (effect_index < 0 || effect_index >= as->get_bus_effect_count(bus_index)) return;

            godot::Ref<godot::AudioEffectInstance> fx_inst = as->get_bus_effect_instance(bus_index, effect_index);
            godot::AudioEffectSpectrumAnalyzerInstance* spec =
                godot::Object::cast_to<godot::AudioEffectSpectrumAnalyzerInstance>(fx_inst.ptr());
            if (!spec) return;

            const bool is_stereo_v2 = (out_part->element_stride >= sizeof(godot::Vector2));
            const uint32_t stride = std::max<uint32_t>(is_stereo_v2 ? sizeof(godot::Vector2) : sizeof(float), out_part->element_stride);
            const int32_t max_elems = static_cast<int32_t>(out_part->capacity_bytes / stride);
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
                uint8_t* dst = out_part->raw_base_ptr + static_cast<size_t>(b) * stride;

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

            out_part->selection.element_count = bands;
        }
    }
};

static_assert(sizeof(AudioSpectrumAnalyzerCaptureTransformLogic<>) == 64, "AudioSpectrumAnalyzerCaptureTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<AudioSpectrumAnalyzerCaptureTransformLogic<>>, "AudioSpectrumAnalyzerCaptureTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core