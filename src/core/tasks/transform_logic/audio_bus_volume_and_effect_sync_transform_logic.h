#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * A2: AudioBusVolumeAndEffectSyncTransformLogic
 * Drives AudioServer bus volume_db (or linear gain -> dB conversion) and AudioEffect resource
 * properties (such as cutoff_hz, room_size, wet, pitch_scale) from DOD float buffers.
 */
struct AudioBusVolumeAndEffectSyncTransformLogic {
    static constexpr std::string_view ui_name = "AudioServer Bus Volume & Effect Parameter Sync";
    static constexpr std::string_view ui_category = "Audio/Mixing";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 1> supported_types = {
        DataType::FLOAT32
    };

    int32_t target_mode = 0; // 0 = Bus Volume (dB), 1 = Bus Volume (Linear Gain -> dB), 2 = Bus AudioEffect Property
    int32_t bus_index = 0;
    int32_t effect_index = 0;
    godot::StringName effect_property = godot::StringName("cutoff_hz");
    int32_t source_element_index = 0;
    float value_scale = 1.0f;
    float value_bias = 0.0f;

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
        add_prop("target_mode", godot::Variant::INT, target_mode, 2, "Bus Volume (dB),Bus Volume (Linear -> dB),Bus Effect Property");
        add_prop("bus_index", godot::Variant::INT, bus_index);
        add_prop("effect_index", godot::Variant::INT, effect_index);
        add_prop("effect_property", godot::Variant::STRING_NAME, effect_property);
        add_prop("source_element_index", godot::Variant::INT, source_element_index);
        add_prop("value_scale", godot::Variant::FLOAT, value_scale);
        add_prop("value_bias", godot::Variant::FLOAT, value_bias);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("target_mode")) target_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["target_mode"]), 0, 2);
        if (p_props.has("bus_index")) bus_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["bus_index"]));
        if (p_props.has("effect_index")) effect_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["effect_index"]));
        if (p_props.has("effect_property")) effect_property = p_props["effect_property"];
        if (p_props.has("source_element_index")) source_element_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["source_element_index"]));
        if (p_props.has("value_scale")) value_scale = static_cast<float>(p_props["value_scale"]);
        if (p_props.has("value_bias")) value_bias = static_cast<float>(p_props["value_bias"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        const GrantPartPOD& part = p_ctx.grant->parts[0];
        if (!part.raw_base_ptr) return;

        godot::AudioServer* as = godot::AudioServer::get_singleton();
        if (!as || bus_index < 0 || bus_index >= as->get_bus_count()) return;

        const uint32_t stride = std::max<uint32_t>(sizeof(float), part.element_stride);
        const int64_t count = static_cast<int64_t>(part.capacity_bytes / stride);
        if (count <= 0) return;

        const int64_t idx = std::min<int64_t>(source_element_index, count - 1);
        const float raw_val = *reinterpret_cast<const float*>(part.raw_base_ptr + static_cast<size_t>(idx) * stride);
        const float mapped_val = raw_val * value_scale + value_bias;

        if (target_mode == 0) {
            as->set_bus_volume_db(bus_index, std::clamp(mapped_val, -80.0f, 24.0f));
        } else if (target_mode == 1) {
            const float lin = std::max(1e-4f, mapped_val);
            const float db = std::clamp(20.0f * std::log10(lin), -80.0f, 24.0f);
            as->set_bus_volume_db(bus_index, db);
        } else if (effect_index >= 0 && effect_index < as->get_bus_effect_count(bus_index) && !effect_property.is_empty()) {
            godot::Ref<godot::AudioEffect> fx = as->get_bus_effect(bus_index, effect_index);
            if (fx.is_valid()) {
                fx->set(effect_property, mapped_val);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core