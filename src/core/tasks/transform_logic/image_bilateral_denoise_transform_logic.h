#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ImageBilateralDenoiseTransformLogic<T, T_Strategy, KernelSize> (I3)
 * Edge-preserving bilateral spatial + photometric range Gaussian filter over 2D Color,
 * Vector4, or float image buffers.
 */
template <
    typename T = godot::Color,
    typename T_Strategy = Spatial2DStrategy,
    size_t KernelSize = 8
>
struct alignas(64) ImageBilateralDenoiseTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StaticStencilView<T, DefaultStrategy, KernelSize>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::COLOR | DataType::VECTOR4 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size          = KernelSize;

    static constexpr size_t transient_workspace_bytes = 256 * 256 * sizeof(godot::Color) + 256;

    static constexpr std::string_view display_name = "Image Edge-Preserving Bilateral Denoise";
    static constexpr std::string_view ui_category  = "Image";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t source_buffer_id    = INVALID_ID; // 4B: Source 2D image buffer
    uint32_t output_buffer_id    = INVALID_ID; // 4B: Optional destination buffer (in-place via workspace if equal/unbound)
    int32_t image_width          = 256;        // 4B: Image width in pixels
    int32_t image_height         = 256;        // 4B: Image height in pixels
    int32_t kernel_radius        = 2;          // 4B: Spatial neighborhood radius [1..8]
    float sigma_spatial          = 2.0f;       // 4B: Spatial Gaussian standard deviation (pixels)
    float sigma_range            = 0.12f;      // 4B: Photometric range standard deviation
    uint8_t reserved_padding[36] = {0};        // 36B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& /*p_context*/) const noexcept {
        return static_cast<size_t>(std::max<int32_t>(1, image_width)) *
               static_cast<size_t>(std::max<int32_t>(1, image_height)) * sizeof(godot::Color) + 256;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary sbuf_prop;
        sbuf_prop["name"] = "source_buffer_id";
        sbuf_prop["type"] = godot::Variant::INT;
        sbuf_prop["hint_string"] = "buffer_option";
        props.push_back(sbuf_prop);

        godot::Dictionary obuf_prop;
        obuf_prop["name"] = "output_buffer_id";
        obuf_prop["type"] = godot::Variant::INT;
        obuf_prop["hint_string"] = "buffer_option";
        props.push_back(obuf_prop);

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

        godot::Dictionary rad_prop;
        rad_prop["name"] = "kernel_radius";
        rad_prop["type"] = godot::Variant::INT;
        rad_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rad_prop["hint_string"] = "1,8,1,suffix:px";
        props.push_back(rad_prop);

        godot::Dictionary ss_prop;
        ss_prop["name"] = "sigma_spatial";
        ss_prop["type"] = godot::Variant::FLOAT;
        ss_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        ss_prop["hint_string"] = "0.1,16.0,0.05";
        props.push_back(ss_prop);

        godot::Dictionary sr_prop;
        sr_prop["name"] = "sigma_range";
        sr_prop["type"] = godot::Variant::FLOAT;
        sr_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        sr_prop["hint_string"] = "0.005,2.0,0.005";
        props.push_back(sr_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (source_buffer_id != INVALID_ID) ? source_buffer_id : output_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        } else if (p_props.has("grid_buffer_id") && source_buffer_id == INVALID_ID) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && source_buffer_id == INVALID_ID) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("output_buffer_id")) {
            output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_buffer_id"]));
        }
        if (p_props.has("image_width")) {
            image_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_width"]));
        }
        if (p_props.has("image_height")) {
            image_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_height"]));
        }
        if (p_props.has("kernel_radius")) {
            kernel_radius = std::clamp<int32_t>(static_cast<int32_t>(p_props["kernel_radius"]), 1, 8);
        }
        if (p_props.has("sigma_spatial")) {
            sigma_spatial = std::max(1e-4f, static_cast<float>(p_props["sigma_spatial"]));
        }
        if (p_props.has("sigma_range")) {
            sigma_range = std::max(1e-4f, static_cast<float>(p_props["sigma_range"]));
        }
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& /*p_context*/, const GrantPartPOD* p_part) const noexcept {
        const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
        if constexpr (requires { r_view.strategy.stride_y; }) {
            r_view.strategy.stride_y = static_cast<int64_t>(std::max<int32_t>(1, image_width)) * static_cast<int64_t>(stride);
        }
        if constexpr (requires { r_view.baked_offsets; } && KernelSize >= 8) {
            const intptr_t sx = static_cast<intptr_t>(stride);
            const intptr_t sy = static_cast<intptr_t>(std::max<int32_t>(1, image_width)) * sx;
            r_view.baked_offsets[0] = -sx - sy;
            r_view.baked_offsets[1] = -sy;
            r_view.baked_offsets[2] =  sx - sy;
            r_view.baked_offsets[3] = -sx;
            r_view.baked_offsets[4] =  sx;
            r_view.baked_offsets[5] = -sx + sy;
            r_view.baked_offsets[6] =  sy;
            r_view.baked_offsets[7] =  sx + sy;
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
            const uint32_t src_id = (source_buffer_id != INVALID_ID) ? source_buffer_id : output_buffer_id;
            const uint32_t dst_id = (output_buffer_id != INVALID_ID) ? output_buffer_id : src_id;
            const GrantPartPOD* src_part = p_context.get_grant_part(src_id);
            const GrantPartPOD* dst_part = p_context.get_grant_part(dst_id);
            if (!src_part || !dst_part || !src_part->raw_base_ptr || !dst_part->raw_base_ptr) return;

            const int32_t w = std::max<int32_t>(1, image_width);
            const int32_t h = std::max<int32_t>(1, image_height);
            const size_t total_pixels = static_cast<size_t>(w) * static_cast<size_t>(h);

            const float inv_2sigma_s2 = -1.0f / (2.0f * sigma_spatial * sigma_spatial);
            const float inv_2sigma_r2 = -1.0f / (2.0f * sigma_range * sigma_range);
            const int32_t rad = std::clamp<int32_t>(kernel_radius, 1, 8);

            if (src_part->element_stride >= sizeof(godot::Color)) {
                if (src_part->capacity_bytes / sizeof(godot::Color) < total_pixels ||
                    dst_part->capacity_bytes / sizeof(godot::Color) < total_pixels) return;

                const godot::Color* src_c = reinterpret_cast<const godot::Color*>(src_part->raw_base_ptr);
                godot::Color* dst_c = reinterpret_cast<godot::Color*>(dst_part->raw_base_ptr);
                godot::Color* scratch = (src_c == dst_c && p_context.local_workspace)
                    ? reinterpret_cast<godot::Color*>(p_context.local_workspace)
                    : dst_c;

                for (int32_t y = 0; y < h; ++y) {
                    for (int32_t x = 0; x < w; ++x) {
                        const godot::Color center = src_c[static_cast<size_t>(y) * w + x];
                        godot::Color accum(0.0f, 0.0f, 0.0f, 0.0f);
                        float w_sum = 0.0f;

                        for (int32_t dy = -rad; dy <= rad; ++dy) {
                            const int32_t ny = std::clamp(y + dy, 0, h - 1);
                            for (int32_t dx = -rad; dx <= rad; ++dx) {
                                const int32_t nx = std::clamp(x + dx, 0, w - 1);
                                const godot::Color sample = src_c[static_cast<size_t>(ny) * w + nx];

                                const float d_spatial2 = static_cast<float>(dx * dx + dy * dy);
                                const float dr = sample.r - center.r;
                                const float dg = sample.g - center.g;
                                const float db = sample.b - center.b;
                                const float d_range2 = dr * dr + dg * dg + db * db;

                                const float weight = std::exp(d_spatial2 * inv_2sigma_s2 + d_range2 * inv_2sigma_r2);
                                accum += sample * weight;
                                w_sum += weight;
                            }
                        }

                        if (w_sum > 1e-7f) {
                            accum /= w_sum;
                            accum.a = center.a;
                            scratch[static_cast<size_t>(y) * w + x] = accum;
                        } else {
                            scratch[static_cast<size_t>(y) * w + x] = center;
                        }
                    }
                }

                if (scratch != dst_c) {
                    for (size_t i = 0; i < total_pixels; ++i) dst_c[i] = scratch[i];
                }
            } else {
                if (src_part->capacity_bytes / sizeof(float) < total_pixels ||
                    dst_part->capacity_bytes / sizeof(float) < total_pixels) return;

                const float* src_f = reinterpret_cast<const float*>(src_part->raw_base_ptr);
                float* dst_f = reinterpret_cast<float*>(dst_part->raw_base_ptr);
                float* scratch = (src_f == dst_f && p_context.local_workspace)
                    ? reinterpret_cast<float*>(p_context.local_workspace)
                    : dst_f;

                for (int32_t y = 0; y < h; ++y) {
                    for (int32_t x = 0; x < w; ++x) {
                        const float center = src_f[static_cast<size_t>(y) * w + x];
                        float accum = 0.0f;
                        float w_sum = 0.0f;

                        for (int32_t dy = -rad; dy <= rad; ++dy) {
                            const int32_t ny = std::clamp(y + dy, 0, h - 1);
                            for (int32_t dx = -rad; dx <= rad; ++dx) {
                                const int32_t nx = std::clamp(x + dx, 0, w - 1);
                                const float sample = src_f[static_cast<size_t>(ny) * w + nx];
                                const float d_s2 = static_cast<float>(dx * dx + dy * dy);
                                const float d_r2 = (sample - center) * (sample - center);
                                const float weight = std::exp(d_s2 * inv_2sigma_s2 + d_r2 * inv_2sigma_r2);
                                accum += sample * weight;
                                w_sum += weight;
                            }
                        }
                        scratch[static_cast<size_t>(y) * w + x] = (w_sum > 1e-7f) ? (accum / w_sum) : center;
                    }
                }

                if (scratch != dst_f) {
                    for (size_t i = 0; i < total_pixels; ++i) dst_f[i] = scratch[i];
                }
            }
        }
    }
};

static_assert(sizeof(ImageBilateralDenoiseTransformLogic<>) == 64, "ImageBilateralDenoiseTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ImageBilateralDenoiseTransformLogic<>>, "ImageBilateralDenoiseTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core