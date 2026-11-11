#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * I5: ImageHistogramEqualizeTransformLogic
 * Computes a 256-bin luminance histogram and prefix-sum Cumulative Distribution Function (CDF)
 * in transient workspace (with optional CLAHE contrast-limiting clip threshold), equalizing
 * image luminance while preserving chromaticity.
 */
struct ImageHistogramEqualizeTransformLogic {
    static constexpr std::string_view ui_name = "Image Luminance Histogram Equalization";
    static constexpr std::string_view ui_category = "Image";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::SPATIAL_2D,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::COLOR,
        DataType::VECTOR4,
        DataType::FLOAT32
    };

    float blend_factor = 1.0f;
    float clahe_clip_limit = 0.0f; // 0 = standard full histogram equalization; > 0 clips bin counts to clip_limit * avg

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("blend_factor", godot::Variant::FLOAT, blend_factor);
        add_prop("clahe_clip_limit", godot::Variant::FLOAT, clahe_clip_limit);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("blend_factor")) blend_factor = std::clamp(static_cast<float>(p_props["blend_factor"]), 0.0f, 1.0f);
        if (p_props.has("clahe_clip_limit")) clahe_clip_limit = std::max(0.0f, static_cast<float>(p_props["clahe_clip_limit"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return 256 * sizeof(uint32_t) + 256 * sizeof(float) + 128;
    }

    size_t get_transient_requirement() const noexcept {
        return 256 * sizeof(uint32_t) + 256 * sizeof(float) + 128;
    }

    static inline float rec709_luma(const godot::Color& c) noexcept {
        return std::clamp(0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b, 0.0f, 1.0f);
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !p_ctx.local_workspace) return;

        GrantPartPOD& img_part = p_ctx.grant->parts[0];
        if (!img_part.raw_base_ptr) return;

        uint32_t* bins = static_cast<uint32_t*>(p_ctx.local_workspace);
        float* cdf_lut = reinterpret_cast<float*>(bins + 256);
        for (int i = 0; i < 256; ++i) {
            bins[i] = 0;
            cdf_lut[i] = 0.0f;
        }

        const bool is_color = (img_part.element_stride >= sizeof(godot::Color));
        const uint32_t stride = std::max<uint32_t>(is_color ? sizeof(godot::Color) : sizeof(float), img_part.element_stride);
        const int64_t n_pixels = (img_part.selection.element_count > 0)
            ? img_part.selection.element_count
            : static_cast<int64_t>(img_part.capacity_bytes / stride);
        if (n_pixels <= 0) return;

        // 1. Accumulate 256-bin luminance histogram
        for (int64_t i = 0; i < n_pixels; ++i) {
            float lum = 0.0f;
            if (is_color) {
                lum = rec709_luma(*reinterpret_cast<const godot::Color*>(img_part.raw_base_ptr + static_cast<size_t>(i) * stride));
            } else {
                lum = std::clamp(*reinterpret_cast<const float*>(img_part.raw_base_ptr + static_cast<size_t>(i) * stride), 0.0f, 1.0f);
            }
            const int32_t b = std::clamp<int32_t>(static_cast<int32_t>(lum * 255.0f + 0.5f), 0, 255);
            bins[b]++;
        }

        // 2. Optional CLAHE contrast-limiting bin clip + uniform redistribution
        if (clahe_clip_limit > 0.0f) {
            const uint32_t limit = std::max<uint32_t>(1u, static_cast<uint32_t>((static_cast<float>(n_pixels) / 256.0f) * clahe_clip_limit));
            uint64_t excess = 0;
            for (int i = 0; i < 256; ++i) {
                if (bins[i] > limit) {
                    excess += (bins[i] - limit);
                    bins[i] = limit;
                }
            }
            const uint32_t redist = static_cast<uint32_t>(excess / 256u);
            for (int i = 0; i < 256; ++i) {
                bins[i] += redist;
            }
        }

        // 3. Prefix-sum CDF construction
        uint64_t running = 0;
        uint64_t cdf_min = 0;
        for (int i = 0; i < 256; ++i) {
            running += bins[i];
            if (cdf_min == 0 && running > 0) cdf_min = running;
            cdf_lut[i] = static_cast<float>(running);
        }

        const float denom = std::max(1.0f, static_cast<float>(running - cdf_min));
        for (int i = 0; i < 256; ++i) {
            cdf_lut[i] = std::clamp((cdf_lut[i] - static_cast<float>(cdf_min)) / denom, 0.0f, 1.0f);
        }

        // 4. Remap pixel luminance using CDF LUT
        for (int64_t i = 0; i < n_pixels; ++i) {
            if (is_color) {
                godot::Color& c = *reinterpret_cast<godot::Color*>(img_part.raw_base_ptr + static_cast<size_t>(i) * stride);
                const float old_lum = rec709_luma(c);
                const int32_t b = std::clamp<int32_t>(static_cast<int32_t>(old_lum * 255.0f + 0.5f), 0, 255);
                const float eq_lum = cdf_lut[b];
                const float new_lum = old_lum + (eq_lum - old_lum) * blend_factor;
                if (old_lum > 1e-5f) {
                    const float scale = new_lum / old_lum;
                    c.r = std::clamp(c.r * scale, 0.0f, 1.0f);
                    c.g = std::clamp(c.g * scale, 0.0f, 1.0f);
                    c.b = std::clamp(c.b * scale, 0.0f, 1.0f);
                } else {
                    c.r = c.g = c.b = new_lum;
                }
            } else {
                float& v = *reinterpret_cast<float*>(img_part.raw_base_ptr + static_cast<size_t>(i) * stride);
                const float clamped = std::clamp(v, 0.0f, 1.0f);
                const int32_t b = std::clamp<int32_t>(static_cast<int32_t>(clamped * 255.0f + 0.5f), 0, 255);
                v = clamped + (cdf_lut[b] - clamped) * blend_factor;
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core