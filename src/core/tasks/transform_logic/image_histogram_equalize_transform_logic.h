#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ImageHistogramEqualizeTransformLogic<T, T_Strategy> (I5)
 * Computes a 256-bin luminance histogram and prefix-sum Cumulative Distribution Function (CDF)
 * in transient workspace (with optional CLAHE contrast-limiting clip threshold), equalizing
 * image luminance while preserving chromaticity.
 */
template <typename T = godot::Color, typename T_Strategy = Spatial2DStrategy>
struct alignas(64) ImageHistogramEqualizeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL | BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::COLOR | DataType::VECTOR4 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 256 * sizeof(uint32_t) + 256 * sizeof(float) + 128;

    static constexpr std::string_view display_name = "Image Luminance Histogram Equalization";
    static constexpr std::string_view ui_category  = "Image";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t image_buffer_id     = INVALID_ID; // 4B: Primary target 2D image buffer
    int32_t image_width          = 256;        // 4B: Grid width in pixels
    int32_t image_height         = 256;        // 4B: Grid height in pixels
    float blend_factor           = 1.0f;       // 4B: Blend weight between original and equalized luma [0, 1]
    float clahe_clip_limit       = 0.0f;       // 4B: Contrast limit multiplier (0 = standard full equalization)
    uint8_t reserved_padding[44] = {0};        // 44B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& /*p_context*/) const noexcept {
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "image_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary w_prop;
        w_prop["name"] = "image_width";
        w_prop["type"] = godot::Variant::INT;
        w_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        w_prop["hint_string"] = "1,8192,1,suffix:px";
        props.push_back(w_prop);

        godot::Dictionary h_prop;
        h_prop["name"] = "image_height";
        h_prop["type"] = godot::Variant::INT;
        h_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        h_prop["hint_string"] = "1,8192,1,suffix:px";
        props.push_back(h_prop);

        godot::Dictionary bld_prop;
        bld_prop["name"] = "blend_factor";
        bld_prop["type"] = godot::Variant::FLOAT;
        bld_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        bld_prop["hint_string"] = "0.0,1.0,0.01";
        props.push_back(bld_prop);

        godot::Dictionary clp_prop;
        clp_prop["name"] = "clahe_clip_limit";
        clp_prop["type"] = godot::Variant::FLOAT;
        clp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        clp_prop["hint_string"] = "0.0,16.0,0.1";
        props.push_back(clp_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return image_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("image_buffer_id")) {
            image_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["image_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && image_buffer_id == INVALID_ID) {
            image_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && image_buffer_id == INVALID_ID) {
            image_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("image_width")) {
            image_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_width"]));
        }
        if (p_props.has("image_height")) {
            image_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_height"]));
        }
        if (p_props.has("blend_factor")) {
            blend_factor = std::clamp(static_cast<float>(p_props["blend_factor"]), 0.0f, 1.0f);
        }
        if (p_props.has("clahe_clip_limit")) {
            clahe_clip_limit = std::max(0.0f, static_cast<float>(p_props["clahe_clip_limit"]));
        }
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& /*p_context*/, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { r_view.strategy.stride_y; }) {
            const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
            r_view.strategy.stride_y = static_cast<int64_t>(std::max<int32_t>(1, image_width)) * static_cast<int64_t>(stride);
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Color> || std::is_same_v<T, godot::Vector4> || std::is_same_v<T, float>) {
            if (!p_context.local_workspace) return;

            const GrantPartPOD* img_part = p_context.get_grant_part(image_buffer_id);
            if (!img_part || !img_part->raw_base_ptr) return;

            uint32_t* bins = static_cast<uint32_t*>(p_context.local_workspace);
            float* cdf_lut = reinterpret_cast<float*>(bins + 256);
            for (int i = 0; i < 256; ++i) {
                bins[i] = 0;
                cdf_lut[i] = 0.0f;
            }

            const bool is_color = (img_part->element_stride >= sizeof(godot::Color));
            const uint32_t stride = std::max<uint32_t>(is_color ? sizeof(godot::Color) : sizeof(float), img_part->element_stride);
            const int64_t n_pixels = (img_part->selection.element_count > 0)
                ? img_part->selection.element_count
                : static_cast<int64_t>(img_part->capacity_bytes / stride);
            if (n_pixels <= 0) return;

            // 1. Accumulate 256-bin luminance histogram
            for (int64_t i = 0; i < n_pixels; ++i) {
                float lum = 0.0f;
                if (is_color) {
                    lum = _rec709_luma(*reinterpret_cast<const godot::Color*>(img_part->raw_base_ptr + static_cast<size_t>(i) * stride));
                } else {
                    lum = std::clamp(*reinterpret_cast<const float*>(img_part->raw_base_ptr + static_cast<size_t>(i) * stride), 0.0f, 1.0f);
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
                    godot::Color& c = *reinterpret_cast<godot::Color*>(img_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                    const float old_lum = _rec709_luma(c);
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
                    float& v = *reinterpret_cast<float*>(img_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                    const float clamped = std::clamp(v, 0.0f, 1.0f);
                    const int32_t b = std::clamp<int32_t>(static_cast<int32_t>(clamped * 255.0f + 0.5f), 0, 255);
                    v = clamped + (cdf_lut[b] - clamped) * blend_factor;
                }
            }
        }
    }

private:
    static inline float _rec709_luma(const godot::Color& c) noexcept {
        return std::clamp(0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b, 0.0f, 1.0f);
    }
};

static_assert(sizeof(ImageHistogramEqualizeTransformLogic<>) == 64, "ImageHistogramEqualizeTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ImageHistogramEqualizeTransformLogic<>>, "ImageHistogramEqualizeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core