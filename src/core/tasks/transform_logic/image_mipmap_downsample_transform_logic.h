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
 * I7: ImageMipmapDownsampleTransformLogic
 * Hierarchical 2x2 downsample from source resolution (src_width, src_height) in Part 0
 * into destination mip buffer (src_width/2, src_height/2) in Part 1 using 2x2 Box average,
 * Lanczos-2 windowed sinc, Max-pooling, or Min-pooling.
 */
struct ImageMipmapDownsampleTransformLogic {
    static constexpr std::string_view ui_name = "Image Hierarchical Mipmap Downsample";
    static constexpr std::string_view ui_category = "Image";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::BRIDGE,
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

    int32_t src_width = 256;
    int32_t src_height = 256;
    int32_t filter_mode = 0; // 0 = 2x2 Box Average, 1 = Lanczos-2, 2 = Max-Pooling, 3 = Min-Pooling

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
        add_prop("src_width", godot::Variant::INT, src_width);
        add_prop("src_height", godot::Variant::INT, src_height);
        add_prop("filter_mode", godot::Variant::INT, filter_mode, 2, "2x2 Box Average,Lanczos-2 Sinc,Max-Pooling,Min-Pooling");
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("src_width")) src_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["src_width"]));
        if (p_props.has("src_height")) src_height = std::max<int32_t>(2, static_cast<int32_t>(p_props["src_height"]));
        if (p_props.has("filter_mode")) filter_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["filter_mode"]), 0, 3);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline float lanczos2_weight(float x) noexcept {
        const float ax = std::abs(x);
        if (ax < 1e-6f) return 1.0f;
        if (ax >= 2.0f) return 0.0f;
        constexpr float PI = 3.14159265358979323846f;
        const float px = PI * ax;
        return (std::sin(px) / px) * (std::sin(px * 0.5f) / (px * 0.5f));
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& src_part = p_ctx.grant->parts[0];
        GrantPartPOD& dst_part = p_ctx.grant->parts[1];
        if (!src_part.raw_base_ptr || !dst_part.raw_base_ptr) return;

        const int32_t sw = std::max<int32_t>(2, src_width);
        const int32_t sh = std::max<int32_t>(2, src_height);
        const int32_t dw = std::max<int32_t>(1, sw / 2);
        const int32_t dh = std::max<int32_t>(1, sh / 2);

        if (src_part.element_stride >= sizeof(godot::Color)) {
            if (src_part.capacity_bytes / sizeof(godot::Color) < static_cast<size_t>(sw) * sh ||
                dst_part.capacity_bytes / sizeof(godot::Color) < static_cast<size_t>(dw) * dh) return;

            const auto* src_c = reinterpret_cast<const godot::Color*>(src_part.raw_base_ptr);
            auto* dst_c = reinterpret_cast<godot::Color*>(dst_part.raw_base_ptr);

            for (int32_t y = 0; y < dh; ++y) {
                for (int32_t x = 0; x < dw; ++x) {
                    const int32_t sx = x * 2;
                    const int32_t sy = y * 2;
                    const godot::Color& c00 = src_c[static_cast<size_t>(sy) * sw + sx];
                    const godot::Color& c10 = src_c[static_cast<size_t>(sy) * sw + std::min(sw - 1, sx + 1)];
                    const godot::Color& c01 = src_c[static_cast<size_t>(std::min(sh - 1, sy + 1)) * sw + sx];
                    const godot::Color& c11 = src_c[static_cast<size_t>(std::min(sh - 1, sy + 1)) * sw + std::min(sw - 1, sx + 1)];

                    if (filter_mode == 0) {
                        dst_c[static_cast<size_t>(y) * dw + x] = (c00 + c10 + c01 + c11) * 0.25f;
                    } else if (filter_mode == 2) {
                        dst_c[static_cast<size_t>(y) * dw + x] = godot::Color(
                            std::max({c00.r, c10.r, c01.r, c11.r}),
                            std::max({c00.g, c10.g, c01.g, c11.g}),
                            std::max({c00.b, c10.b, c01.b, c11.b}),
                            std::max({c00.a, c10.a, c01.a, c11.a})
                        );
                    } else if (filter_mode == 3) {
                        dst_c[static_cast<size_t>(y) * dw + x] = godot::Color(
                            std::min({c00.r, c10.r, c01.r, c11.r}),
                            std::min({c00.g, c10.g, c01.g, c11.g}),
                            std::min({c00.b, c10.b, c01.b, c11.b}),
                            std::min({c00.a, c10.a, c01.a, c11.a})
                        );
                    } else {
                        godot::Color accum(0, 0, 0, 0);
                        float w_sum = 0.0f;
                        const float cx = static_cast<float>(sx) + 0.5f;
                        const float cy = static_cast<float>(sy) + 0.5f;
                        for (int32_t ky = -1; ky <= 2; ++ky) {
                            const int32_t ny = std::clamp(sy + ky, 0, sh - 1);
                            const float wy = lanczos2_weight((static_cast<float>(ny) - cy) * 0.5f);
                            for (int32_t kx = -1; kx <= 2; ++kx) {
                                const int32_t nx = std::clamp(sx + kx, 0, sw - 1);
                                const float wx = lanczos2_weight((static_cast<float>(nx) - cx) * 0.5f);
                                const float w = wx * wy;
                                accum += src_c[static_cast<size_t>(ny) * sw + nx] * w;
                                w_sum += w;
                            }
                        }
                        dst_c[static_cast<size_t>(y) * dw + x] = (w_sum > 1e-6f) ? (accum / w_sum) : c00;
                    }
                }
            }
        } else {
            if (src_part.capacity_bytes / sizeof(float) < static_cast<size_t>(sw) * sh ||
                dst_part.capacity_bytes / sizeof(float) < static_cast<size_t>(dw) * dh) return;

            const float* src_f = reinterpret_cast<const float*>(src_part.raw_base_ptr);
            float* dst_f = reinterpret_cast<float*>(dst_part.raw_base_ptr);

            for (int32_t y = 0; y < dh; ++y) {
                for (int32_t x = 0; x < dw; ++x) {
                    const int32_t sx = x * 2;
                    const int32_t sy = y * 2;
                    const float v00 = src_f[static_cast<size_t>(sy) * sw + sx];
                    const float v10 = src_f[static_cast<size_t>(sy) * sw + std::min(sw - 1, sx + 1)];
                    const float v01 = src_f[static_cast<size_t>(std::min(sh - 1, sy + 1)) * sw + sx];
                    const float v11 = src_f[static_cast<size_t>(std::min(sh - 1, sy + 1)) * sw + std::min(sw - 1, sx + 1)];

                    if (filter_mode == 2) {
                        dst_f[static_cast<size_t>(y) * dw + x] = std::max({v00, v10, v01, v11});
                    } else if (filter_mode == 3) {
                        dst_f[static_cast<size_t>(y) * dw + x] = std::min({v00, v10, v01, v11});
                    } else {
                        dst_f[static_cast<size_t>(y) * dw + x] = (v00 + v10 + v01 + v11) * 0.25f;
                    }
                }
            }
        }

        dst_part.selection.element_count = static_cast<int64_t>(dw) * dh;
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core