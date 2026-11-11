#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * A11: AudioFrequencyOctaveBandMetadataLogic
 * Classifies FFT/spectrum bins or frequency-tagged emitters into 7 standard perceptual
 * acoustic bands (Sub-Bass, Bass, Low-Mid, Mid, Upper-Mid, Presence, Brilliance), writing
 * band IDs (0..6) into r_selection.partition_ids, bitflags into r_selection.group_masks,
 * and octave numbers into r_selection.lod_levels.
 */
struct AudioFrequencyOctaveBandMetadataLogic {
    static constexpr std::string_view ui_name = "Audio ISO Octave & Perceptual Band Metadata";
    static constexpr std::string_view ui_category = "Audio/Metadata";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::FLOAT32,
        DataType::INT32
    };

    float min_frequency_hz = 20.0f;
    float max_frequency_hz = 20000.0f;
    bool logarithmic_bin_spacing = true;
    bool read_frequency_from_buffer = false;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("min_frequency_hz", godot::Variant::FLOAT, min_frequency_hz);
        add_prop("max_frequency_hz", godot::Variant::FLOAT, max_frequency_hz);
        add_prop("logarithmic_bin_spacing", godot::Variant::BOOL, logarithmic_bin_spacing);
        add_prop("read_frequency_from_buffer", godot::Variant::BOOL, read_frequency_from_buffer);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("min_frequency_hz")) min_frequency_hz = std::max(1.0f, static_cast<float>(p_props["min_frequency_hz"]));
        if (p_props.has("max_frequency_hz")) max_frequency_hz = std::max(min_frequency_hz + 1.0f, static_cast<float>(p_props["max_frequency_hz"]));
        if (p_props.has("logarithmic_bin_spacing")) logarithmic_bin_spacing = static_cast<bool>(p_props["logarithmic_bin_spacing"]);
        if (p_props.has("read_frequency_from_buffer")) read_frequency_from_buffer = static_cast<bool>(p_props["read_frequency_from_buffer"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    // 7 perceptual band upper crossover frequencies (Hz):
    // 0: Sub-Bass (<60), 1: Bass (60-250), 2: Low-Mid (250-500), 3: Mid (500-2000),
    // 4: Upper-Mid (2000-4000), 5: Presence (4000-6000), 6: Brilliance (>6000)
    static inline int32_t classify_perceptual_band(float hz) noexcept {
        if (hz < 60.0f) return 0;
        if (hz < 250.0f) return 1;
        if (hz < 500.0f) return 2;
        if (hz < 2000.0f) return 3;
        if (hz < 4000.0f) return 4;
        if (hz < 6000.0f) return 5;
        return 6;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& part = p_ctx.grant->parts[0];
        MemoryBufferSelectionPOD& sel = part.selection;
        if (!sel.partition_ids || sel.capacity <= 0) return;

        const int64_t n_elems = (sel.element_count > 0)
            ? std::min<int64_t>(sel.element_count, sel.capacity)
            : sel.capacity;
        if (n_elems <= 0) return;

        const uint32_t stride = std::max<uint32_t>(sizeof(float), part.element_stride);
        const int64_t max_buf = part.raw_base_ptr ? static_cast<int64_t>(part.capacity_bytes / stride) : 0;
        const float log_ratio = std::log(max_frequency_hz / min_frequency_hz);

        for (int64_t i = 0; i < n_elems; ++i) {
            float hz = min_frequency_hz;
            if (read_frequency_from_buffer && part.raw_base_ptr && i < max_buf) {
                hz = std::max(1.0f, *reinterpret_cast<const float*>(part.raw_base_ptr + static_cast<size_t>(i) * stride));
            } else {
                const float u = (n_elems > 1) ? (static_cast<float>(i) / static_cast<float>(n_elems - 1)) : 0.0f;
                hz = logarithmic_bin_spacing
                    ? (min_frequency_hz * std::exp(log_ratio * u))
                    : (min_frequency_hz + (max_frequency_hz - min_frequency_hz) * u);
            }

            const int32_t band_id = classify_perceptual_band(hz);
            sel.partition_ids[i] = band_id;

            if (sel.group_masks) {
                sel.group_masks[i] = (1u << band_id);
            }
            if (sel.lod_levels) {
                const int32_t octave = std::clamp<int32_t>(static_cast<int32_t>(std::log2(std::max(1.0f, hz / 16.3516f))), 0, 12);
                sel.lod_levels[i] = static_cast<uint8_t>(octave);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core