#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/bridge_view.h"
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

enum class MipmapFilterMode : uint8_t {
    BOX_2X2     = 0,
    LANCZOS_2   = 1,
    MAX_POOLING = 2,
    MIN_POOLING = 3
};

/**
 * ImageMipmapDownsampleTransformLogic<T, T_Strategy> (I7)
 * Hierarchical 2x2 downsample from source resolution (src_width, src_height)
 * into destination mip buffer (src_width/2, src_height/2) using 2x2 Box average,
 * Lanczos-2 windowed sinc, Max-pooling, or Min-pooling.
 */
template <typename T = godot::Color, typename T_Strategy = Spatial2DStrategy>
struct alignas(64) ImageMipmapDownsampleTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = BridgeView<T, T, T_Strategy::dimensions, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::SPATIAL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::COLOR | DataType::VECTOR4 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Image Hierarchical Mipmap Downsample";
    static constexpr std::string_view ui_category  = "Image";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t source_buffer_id     = INVALID_ID; // 4B: Full-resolution source image buffer
    uint32_t mip_output_buffer_id = INVALID_ID; // 4B: Half-resolution mip destination buffer
    int32_t src_width             = 256;        // 4B: Source image width in pixels
    int32_t src_height            = 256;        // 4B: Source image height in pixels
    MipmapFilterMode filter_mode  = MipmapFilterMode::BOX_2X2; // 1B
    uint8_t reserved_padding[47]  = {0};        // 47B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary sbuf_prop;
        sbuf_prop["name"] = "source_buffer_id";
        sbuf_prop["type"] = godot::Variant::INT;
        sbuf_prop["hint_string"] = "buffer_option";
        props.push_back(sbuf_prop);

        godot::Dictionary mbuf_prop;
        mbuf_prop["name"] = "mip_output_buffer_id";
        mbuf_prop["type"] = godot::Variant::INT;
        mbuf_prop["hint_string"] = "buffer_option";
        props.push_back(mbuf_prop);

        godot::Dictionary sw_prop;
        sw_prop["name"] = "src_width";
        sw_prop["type"] = godot::Variant::INT;
        sw_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        sw_prop["hint_string"] = "2,8192,2,suffix:px";
        props.push_back(sw_prop);

        godot::Dictionary sh_prop;
        sh_prop["name"] = "src_height";
        sh_prop["type"] = godot::Variant::INT;
        sh_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        sh_prop["hint_string"] = "2,8192,2,suffix:px";
        props.push_back(sh_prop);

        godot::Dictionary fmode_prop;
        fmode_prop["name"] = "filter_mode";
        fmode_prop["type"] = godot::Variant::INT;
        fmode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        fmode_prop["hint_string"] = "2x2 Box Average:0,Lanczos-2 Sinc:1,Max-Pooling:2,Min-Pooling:3";
        props.push_back(fmode_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (mip_output_buffer_id != INVALID_ID) ? mip_output_buffer_id : source_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        }
        if (p_props.has("mip_output_buffer_id")) {
            mip_output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["mip_output_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && mip_output_buffer_id == INVALID_ID) {
            mip_output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("src_width")) {
            src_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["src_width"]));
        }
        if (p_props.has("src_height")) {
            src_height = std::max<int32_t>(2, static_cast<int32_t>(p_props["src_height"]));
        }
        if (p_props.has("filter_mode")) {
            filter_mode = static_cast<MipmapFilterMode>(std::clamp<int64_t>(static_cast<int64_t>(p_props["filter_mode"]), 0, 3));
        }
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& p_context, const GrantPartPOD* p_part) const noexcept {
        const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
        if constexpr (requires { r_view.strategy.stride_y; }) {
            r_view.strategy.stride_y = static_cast<int64_t>(std::max<int32_t>(1, src_width / 2)) * static_cast<int64_t>(stride);
        }
        if constexpr (requires { r_view.bind_secondary(p_part); }) {
            if (source_buffer_id != INVALID_ID) {
                r_view.bind_secondary(p_context.get_grant_part(source_buffer_id));
            }
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
            const GrantPartPOD* src_part = p_context.get_grant_part(source_buffer_id);
            GrantPartPOD* dst_part       = const_cast<GrantPartPOD*>(p_context.get_grant_part(mip_output_buffer_id));
            if (!src_part || !dst_part || !src_part->raw_base_ptr || !dst_part->raw_base_ptr) return;

            const int32_t sw = std::max<int32_t>(2, src_width);
            const int32_t sh = std::max<int32_t>(2, src_height);
            const int32_t dw = std::max<int32_t>(1, sw / 2);
            const int32_t dh = std::max<int32_t>(1, sh / 2);

            if (src_part->element_stride >= sizeof(godot::Color)) {
                if (src_part->capacity_bytes / sizeof(godot::Color) < static_cast<size_t>(sw) * sh ||
                    dst_part->capacity_bytes / sizeof(godot::Color) < static_cast<size_t>(dw) * dh) return;

                const auto* src_c = reinterpret_cast<const godot::Color*>(src_part->raw_base_ptr);
                auto* dst_c       = reinterpret_cast<godot::Color*>(dst_part->raw_base_ptr);

                for (int32_t y = 0; y < dh; ++y) {
                    for (int32_t x = 0; x < dw; ++x) {
                        const int32_t sx = x * 2;
                        const int32_t sy = y * 2;
                        const godot::Color& c00 = src_c[static_cast<size_t>(sy) * sw + sx];
                        const godot::Color& c10 = src_c[static_cast<size_t>(sy) * sw + std::min(sw - 1, sx + 1)];
                        const godot::Color& c01 = src_c[static_cast<size_t>(std::min(sh - 1, sy + 1)) * sw + sx];
                        const godot::Color& c11 = src_c[static_cast<size_t>(std::min(sh - 1, sy + 1)) * sw + std::min(sw - 1, sx + 1)];

                        if (filter_mode == MipmapFilterMode::BOX_2X2) {
                            dst_c[static_cast<size_t>(y) * dw + x] = (c00 + c10 + c01 + c11) * 0.25f;
                        } else if (filter_mode == MipmapFilterMode::MAX_POOLING) {
                            dst_c[static_cast<size_t>(y) * dw + x] = godot::Color(
                                std::max({c00.r, c10.r, c01.r, c11.r}),
                                std::max({c00.g, c10.g, c01.g, c11.g}),
                                std::max({c00.b, c10.b, c01.b, c11.b}),
                                std::max({c00.a, c10.a, c01.a, c11.a})
                            );
                        } else if (filter_mode == MipmapFilterMode::MIN_POOLING) {
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
                                const float wy = _lanczos2_weight((static_cast<float>(ny) - cy) * 0.5f);
                                for (int32_t kx = -1; kx <= 2; ++kx) {
                                    const int32_t nx = std::clamp(sx + kx, 0, sw - 1);
                                    const float wx = _lanczos2_weight((static_cast<float>(nx) - cx) * 0.5f);
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
                if (src_part->capacity_bytes / sizeof(float) < static_cast<size_t>(sw) * sh ||
                    dst_part->capacity_bytes / sizeof(float) < static_cast<size_t>(dw) * dh) return;

                const float* src_f = reinterpret_cast<const float*>(src_part->raw_base_ptr);
                float* dst_f       = reinterpret_cast<float*>(dst_part->raw_base_ptr);

                for (int32_t y = 0; y < dh; ++y) {
                    for (int32_t x = 0; x < dw; ++x) {
                        const int32_t sx = x * 2;
                        const int32_t sy = y * 2;
                        const float v00 = src_f[static_cast<size_t>(sy) * sw + sx];
                        const float v10 = src_f[static_cast<size_t>(sy) * sw + std::min(sw - 1, sx + 1)];
                        const float v01 = src_f[static_cast<size_t>(std::min(sh - 1, sy + 1)) * sw + sx];
                        const float v11 = src_f[static_cast<size_t>(std::min(sh - 1, sy + 1)) * sw + std::min(sw - 1, sx + 1)];

                        if (filter_mode == MipmapFilterMode::MAX_POOLING) {
                            dst_f[static_cast<size_t>(y) * dw + x] = std::max({v00, v10, v01, v11});
                        } else if (filter_mode == MipmapFilterMode::MIN_POOLING) {
                            dst_f[static_cast<size_t>(y) * dw + x] = std::min({v00, v10, v01, v11});
                        } else {
                            dst_f[static_cast<size_t>(y) * dw + x] = (v00 + v10 + v01 + v11) * 0.25f;
                        }
                    }
                }
            }

            dst_part->selection.element_count = static_cast<int64_t>(dw) * static_cast<int64_t>(dh);
        }
    }

private:
    static inline float _lanczos2_weight(float p_x) noexcept {
        const float ax = std::abs(p_x);
        if (ax < 1e-6f) return 1.0f;
        if (ax >= 2.0f) return 0.0f;
        constexpr float PI = 3.14159265358979323846f;
        const float px = PI * ax;
        return (std::sin(px) / px) * (std::sin(px * 0.5f) / (px * 0.5f));
    }
};

static_assert(sizeof(ImageMipmapDownsampleTransformLogic<>) == 64, "ImageMipmapDownsampleTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ImageMipmapDownsampleTransformLogic<>>, "ImageMipmapDownsampleTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core