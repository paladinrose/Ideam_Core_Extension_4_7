#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "metadata_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * AudioFrequencyOctaveBandMetadataLogic<T, T_Strategy, T_View> (A11)
 * Classifies FFT/spectrum bins or frequency-tagged emitters into 7 standard perceptual
 * acoustic bands (Sub-Bass, Bass, Low-Mid, Mid, Upper-Mid, Presence, Brilliance), writing
 * band IDs (0..6) into r_selection.partition_ids, bitflags into r_selection.group_masks,
 * and octave numbers into r_selection.lod_levels.
 */
template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) AudioFrequencyOctaveBandMetadataLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Audio ISO Octave & Perceptual Band Metadata";
    static constexpr std::string_view ui_category  = "Audio/Metadata";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID; // 4B
    float min_frequency_hz              = 20.0f;      // 4B
    float max_frequency_hz              = 20000.0f;   // 4B
    bool logarithmic_bin_spacing        = true;       // 1B
    bool read_frequency_from_buffer     = false;      // 1B
    uint8_t reserved_padding[50]        = {0};        // 50B -> Total = 64B

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
        add_prop("min_frequency_hz", godot::Variant::FLOAT, 20.0f);
        add_prop("max_frequency_hz", godot::Variant::FLOAT, 20000.0f);
        add_prop("logarithmic_bin_spacing", godot::Variant::BOOL, true);
        add_prop("read_frequency_from_buffer", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("min_frequency_hz")) min_frequency_hz = std::max(1.0f, static_cast<float>(p_props["min_frequency_hz"]));
        if (p_props.has("max_frequency_hz")) max_frequency_hz = std::max(min_frequency_hz + 1.0f, static_cast<float>(p_props["max_frequency_hz"]));
        if (p_props.has("logarithmic_bin_spacing")) logarithmic_bin_spacing = static_cast<bool>(p_props["logarithmic_bin_spacing"]);
        if (p_props.has("read_frequency_from_buffer")) read_frequency_from_buffer = static_cast<bool>(p_props["read_frequency_from_buffer"]);
    }

    static inline int32_t classify_perceptual_band(float hz) noexcept {
        if (hz < 60.0f) return 0;
        if (hz < 250.0f) return 1;
        if (hz < 500.0f) return 2;
        if (hz < 2000.0f) return 3;
        if (hz < 4000.0f) return 4;
        if (hz < 6000.0f) return 5;
        return 6;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!r_selection.partition_ids || r_selection.capacity <= 0) return;

            const GrantPartPOD* part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);

            const int64_t n_elems = (r_selection.element_count > 0)
                ? std::min<int64_t>(r_selection.element_count, r_selection.capacity)
                : r_selection.capacity;
            if (n_elems <= 0) return;

            const uint32_t stride = part ? std::max<uint32_t>(sizeof(float), part->element_stride) : sizeof(float);
            const int64_t max_buf = (part && part->raw_base_ptr)
                ? static_cast<int64_t>(part->capacity_bytes / stride)
                : 0;
            const float log_ratio = std::log(max_frequency_hz / min_frequency_hz);

            for (int64_t i = 0; i < n_elems; ++i) {
                float hz = min_frequency_hz;
                if (read_frequency_from_buffer && part && part->raw_base_ptr && i < max_buf) {
                    hz = std::max(1.0f, *reinterpret_cast<const float*>(part->raw_base_ptr + static_cast<size_t>(i) * stride));
                } else {
                    const float u = (n_elems > 1) ? (static_cast<float>(i) / static_cast<float>(n_elems - 1)) : 0.0f;
                    hz = logarithmic_bin_spacing
                        ? (min_frequency_hz * std::exp(log_ratio * u))
                        : (min_frequency_hz + (max_frequency_hz - min_frequency_hz) * u);
                }

                const int32_t band_id = classify_perceptual_band(hz);
                r_selection.partition_ids[i] = band_id;

                if (r_selection.group_masks) {
                    r_selection.group_masks[i] = (1u << band_id);
                }
                if (r_selection.lod_levels) {
                    const int32_t octave = std::clamp<int32_t>(static_cast<int32_t>(std::log2(std::max(1.0f, hz / 16.3516f))), 0, 12);
                    r_selection.lod_levels[i] = static_cast<uint8_t>(octave);
                }
            }
        }
    }
};

static_assert(sizeof(AudioFrequencyOctaveBandMetadataLogic<>) == 64, "AudioFrequencyOctaveBandMetadataLogic must be exactly 64 bytes!");
static_assert(IsMetadataLogic<AudioFrequencyOctaveBandMetadataLogic<>>, "AudioFrequencyOctaveBandMetadataLogic must satisfy IsMetadataLogic!");

} // namespace ideam::core