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
 * I3: ImageBilateralDenoiseTransformLogic
 * Edge-preserving bilateral spatial + photometric range Gaussian filter over 2D Color / float
 * image buffers.
 */
struct ImageBilateralDenoiseTransformLogic {
    static constexpr std::string_view ui_name = "Image Edge-Preserving Bilateral Denoise";
    static constexpr std::string_view ui_category = "Image";

    static constexpr std::array<MemoryView, 3> supported_views = {
        MemoryView::STATIC_STENCIL,
        MemoryView::STENCIL,
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

    int32_t image_width = 256;
    int32_t image_height = 256;
    int32_t kernel_radius = 2;
    float sigma_spatial = 2.0f;
    float sigma_range = 0.12f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("image_width", godot::Variant::INT, image_width);
        add_prop("image_height", godot::Variant::INT, image_height);
        add_prop("kernel_radius", godot::Variant::INT, kernel_radius);
        add_prop("sigma_spatial", godot::Variant::FLOAT, sigma_spatial);
        add_prop("sigma_range", godot::Variant::FLOAT, sigma_range);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("image_width")) image_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_width"]));
        if (p_props.has("image_height")) image_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_height"]));
        if (p_props.has("kernel_radius")) kernel_radius = std::clamp<int32_t>(static_cast<int32_t>(p_props["kernel_radius"]), 1, 8);
        if (p_props.has("sigma_spatial")) sigma_spatial = std::max(1e-4f, static_cast<float>(p_props["sigma_spatial"]));
        if (p_props.has("sigma_range")) sigma_range = std::max(1e-4f, static_cast<float>(p_props["sigma_range"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return static_cast<size_t>(std::max<int32_t>(1, image_width)) *
               static_cast<size_t>(std::max<int32_t>(1, image_height)) * sizeof(godot::Color) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        TaskContextPOD dummy{};
        return get_transient_requirement(dummy);
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        const GrantPartPOD& src_part = p_ctx.grant->parts[0];
        GrantPartPOD& dst_part = (p_ctx.grant->part_count > 1) ? p_ctx.grant->parts[1] : p_ctx.grant->parts[0];
        if (!src_part.raw_base_ptr || !dst_part.raw_base_ptr) return;

        const int32_t w = std::max<int32_t>(1, image_width);
        const int32_t h = std::max<int32_t>(1, image_height);
        const size_t total_pixels = static_cast<size_t>(w) * h;

        const float inv_2sigma_s2 = -1.0f / (2.0f * sigma_spatial * sigma_spatial);
        const float inv_2sigma_r2 = -1.0f / (2.0f * sigma_range * sigma_range);
        const int32_t rad = std::clamp<int32_t>(kernel_radius, 1, 8);

        if (src_part.element_stride >= sizeof(godot::Color)) {
            if (src_part.capacity_bytes / sizeof(godot::Color) < total_pixels ||
                dst_part.capacity_bytes / sizeof(godot::Color) < total_pixels) return;

            const godot::Color* src_c = reinterpret_cast<const godot::Color*>(src_part.raw_base_ptr);
            godot::Color* dst_c = reinterpret_cast<godot::Color*>(dst_part.raw_base_ptr);

            godot::Color* scratch = (src_c == dst_c && p_ctx.local_workspace)
                ? reinterpret_cast<godot::Color*>(p_ctx.local_workspace)
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
            // Scalar float image mode
            if (src_part.capacity_bytes / sizeof(float) < total_pixels ||
                dst_part.capacity_bytes / sizeof(float) < total_pixels) return;

            const float* src_f = reinterpret_cast<const float*>(src_part.raw_base_ptr);
            float* dst_f = reinterpret_cast<float*>(dst_part.raw_base_ptr);
            float* scratch = (src_f == dst_f && p_ctx.local_workspace)
                ? reinterpret_cast<float*>(p_ctx.local_workspace)
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

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core