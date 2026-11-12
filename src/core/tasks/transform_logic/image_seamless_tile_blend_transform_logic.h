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
 * ImageSeamlessTileBlendTransformLogic<T, T_Strategy> (I6)
 * Toroidal border blending across horizontal and vertical 2D texture boundaries using
 * smooth raised-cosine crossfade weights so textures tile seamlessly across UV wraps.
 */
template <typename T = godot::Color, typename T_Strategy = Spatial2DStrategy>
struct alignas(64) ImageSeamlessTileBlendTransformLogic {
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

    static constexpr size_t transient_workspace_bytes = 256 * 256 * sizeof(godot::Color) + 256;

    static constexpr std::string_view display_name = "Image Toroidal Seamless Tile Blend";
    static constexpr std::string_view ui_category  = "Image";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t image_buffer_id         = INVALID_ID; // 4B: Primary target 2D image buffer
    int32_t image_width              = 256;        // 4B: Grid width in pixels
    int32_t image_height             = 256;        // 4B: Grid height in pixels
    float blend_margin_ratio         = 0.20f;      // 4B: Border crossfade band ratio in [0.01, 0.5]
    bool variance_preserving_blend   = false;      // 1B: Preserve contrast variance across crossfade
    uint8_t reserved_padding[47]     = {0};        // 47B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& /*p_context*/) const noexcept {
        return static_cast<size_t>(std::max<int32_t>(2, image_width)) *
               static_cast<size_t>(std::max<int32_t>(2, image_height)) * sizeof(godot::Color) + 256;
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
        w_prop["hint_string"] = "2,8192,1,suffix:px";
        props.push_back(w_prop);

        godot::Dictionary h_prop;
        h_prop["name"] = "image_height";
        h_prop["type"] = godot::Variant::INT;
        h_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        h_prop["hint_string"] = "2,8192,1,suffix:px";
        props.push_back(h_prop);

        godot::Dictionary mrg_prop;
        mrg_prop["name"] = "blend_margin_ratio";
        mrg_prop["type"] = godot::Variant::FLOAT;
        mrg_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        mrg_prop["hint_string"] = "0.01,0.5,0.01";
        props.push_back(mrg_prop);

        godot::Dictionary var_prop;
        var_prop["name"] = "variance_preserving_blend";
        var_prop["type"] = godot::Variant::BOOL;
        props.push_back(var_prop);

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
            image_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_width"]));
        }
        if (p_props.has("image_height")) {
            image_height = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_height"]));
        }
        if (p_props.has("blend_margin_ratio")) {
            blend_margin_ratio = std::clamp(static_cast<float>(p_props["blend_margin_ratio"]), 0.01f, 0.5f);
        }
        if (p_props.has("variance_preserving_blend")) {
            variance_preserving_blend = static_cast<bool>(p_props["variance_preserving_blend"]);
        }
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& /*p_context*/, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { r_view.strategy.stride_y; }) {
            const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
            r_view.strategy.stride_y = static_cast<int64_t>(std::max<int32_t>(2, image_width)) * static_cast<int64_t>(stride);
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

            const int32_t w = std::max<int32_t>(2, image_width);
            const int32_t h = std::max<int32_t>(2, image_height);
            const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);

            if (img_part->element_stride >= sizeof(godot::Color)) {
                if (img_part->capacity_bytes / sizeof(godot::Color) < total) return;
                godot::Color* pixels   = reinterpret_cast<godot::Color*>(img_part->raw_base_ptr);
                godot::Color* src_copy = reinterpret_cast<godot::Color*>(p_context.local_workspace);
                for (size_t i = 0; i < total; ++i) src_copy[i] = pixels[i];

                for (int32_t y = 0; y < h; ++y) {
                    const float wy = _smooth_edge_weight(y, h, blend_margin_ratio);
                    const int32_t sy = (y + h / 2) % h;
                    for (int32_t x = 0; x < w; ++x) {
                        const float wx = _smooth_edge_weight(x, w, blend_margin_ratio);
                        const int32_t sx = (x + w / 2) % w;

                        const float alpha = wx * wy;
                        const godot::Color& c00 = src_copy[static_cast<size_t>(y) * w + x];
                        const godot::Color& c11 = src_copy[static_cast<size_t>(sy) * w + sx];
                        pixels[static_cast<size_t>(y) * w + x] = c11.lerp(c00, alpha);
                    }
                }
            } else {
                if (img_part->capacity_bytes / sizeof(float) < total) return;
                float* pixels   = reinterpret_cast<float*>(img_part->raw_base_ptr);
                float* src_copy = reinterpret_cast<float*>(p_context.local_workspace);
                for (size_t i = 0; i < total; ++i) src_copy[i] = pixels[i];

                for (int32_t y = 0; y < h; ++y) {
                    const float wy = _smooth_edge_weight(y, h, blend_margin_ratio);
                    const int32_t sy = (y + h / 2) % h;
                    for (int32_t x = 0; x < w; ++x) {
                        const float wx = _smooth_edge_weight(x, w, blend_margin_ratio);
                        const int32_t sx = (x + w / 2) % w;

                        const float alpha = wx * wy;
                        const float v00 = src_copy[static_cast<size_t>(y) * w + x];
                        const float v11 = src_copy[static_cast<size_t>(sy) * w + sx];
                        pixels[static_cast<size_t>(y) * w + x] = v11 + (v00 - v11) * alpha;
                    }
                }
            }
        }
    }

private:
    static inline float _smooth_edge_weight(int32_t p_coord, int32_t p_dim, float p_margin_ratio) noexcept {
        const float dist_to_border = static_cast<float>(std::min(p_coord, p_dim - 1 - p_coord));
        const float margin = std::max(1.0f, static_cast<float>(p_dim) * p_margin_ratio);
        if (dist_to_border >= margin) return 1.0f;
        const float u = dist_to_border / margin;
        return 0.5f - 0.5f * std::cos(u * 3.14159265358979323846f);
    }
};

static_assert(sizeof(ImageSeamlessTileBlendTransformLogic<>) == 64, "ImageSeamlessTileBlendTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ImageSeamlessTileBlendTransformLogic<>>, "ImageSeamlessTileBlendTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core