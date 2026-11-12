#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * AudioBusVolumeAndEffectSyncTransformLogic<T, T_Strategy, T_View> (A2)
 * Drives AudioServer bus volume_db (or linear gain -> dB conversion) and AudioEffect resource
 * properties (such as cutoff_hz, room_size, wet, pitch_scale) from DOD float buffers.
 */
template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) AudioBusVolumeAndEffectSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::FLOAT64;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "AudioServer Bus Volume & Effect Parameter Sync";
    static constexpr std::string_view ui_category  = "Audio/Mixing";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID;                         // 4B
    int32_t target_mode                 = 0;                                  // 4B: 0=Volume(dB), 1=Linear->dB, 2=Effect Property
    int32_t bus_index                   = 0;                                  // 4B
    int32_t effect_index                = 0;                                  // 4B -> 16B
    int32_t source_element_index        = 0;                                  // 4B
    float value_scale                   = 1.0f;                               // 4B
    float value_bias                    = 0.0f;                               // 4B
    uint32_t reserved_u32               = 0;                                  // 4B -> 32B
    godot::StringName effect_property   = godot::StringName("cutoff_hz");     // sizeof(StringName) (8B)
    uint8_t reserved_padding[32 - sizeof(godot::StringName)] = {0};           // Total = 64B

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
        add_prop("target_mode", godot::Variant::INT, 0, 2, "Bus Volume (dB),Bus Volume (Linear -> dB),Bus Effect Property");
        add_prop("bus_index", godot::Variant::INT, 0);
        add_prop("effect_index", godot::Variant::INT, 0);
        add_prop("effect_property", godot::Variant::STRING_NAME, godot::StringName("cutoff_hz"));
        add_prop("source_element_index", godot::Variant::INT, 0);
        add_prop("value_scale", godot::Variant::FLOAT, 1.0f);
        add_prop("value_bias", godot::Variant::FLOAT, 0.0f);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("target_mode")) target_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["target_mode"]), 0, 2);
        if (p_props.has("bus_index")) bus_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["bus_index"]));
        if (p_props.has("effect_index")) effect_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["effect_index"]));
        if (p_props.has("effect_property")) effect_property = p_props["effect_property"];
        if (p_props.has("source_element_index")) source_element_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["source_element_index"]));
        if (p_props.has("value_scale")) value_scale = static_cast<float>(p_props["value_scale"]);
        if (p_props.has("value_bias")) value_bias = static_cast<float>(p_props["value_bias"]);
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
            const GrantPartPOD* part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!part || !part->raw_base_ptr) return;

            godot::AudioServer* as = godot::AudioServer::get_singleton();
            if (!as || bus_index < 0 || bus_index >= as->get_bus_count()) return;

            const uint32_t stride = std::max<uint32_t>(sizeof(float), part->element_stride);
            const int64_t count = static_cast<int64_t>(part->capacity_bytes / stride);
            if (count <= 0) return;

            const int64_t idx = std::min<int64_t>(source_element_index, count - 1);
            const float raw_val = *reinterpret_cast<const float*>(part->raw_base_ptr + static_cast<size_t>(idx) * stride);
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
    }
};

static_assert(sizeof(AudioBusVolumeAndEffectSyncTransformLogic<>) == 64, "AudioBusVolumeAndEffectSyncTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<AudioBusVolumeAndEffectSyncTransformLogic<>>, "AudioBusVolumeAndEffectSyncTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core