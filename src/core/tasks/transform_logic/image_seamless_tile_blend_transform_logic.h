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
 * I6: ImageSeamlessTileBlendTransformLogic
 * Toroidal border blending across horizontal and vertical 2D texture boundaries using
 * smooth raised-cosine crossfade weights so textures tile seamlessly across UV wraps.
 */
struct ImageSeamlessTileBlendTransformLogic {
    static constexpr std::string_view ui_name = "Image Toroidal Seamless Tile Blend";
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

    int32_t image_width = 256;
    int32_t image_height = 256;
    float blend_margin_ratio = 0.20f;
    bool variance_preserving_blend = false;

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
        add_prop("blend_margin_ratio", godot::Variant::FLOAT, blend_margin_ratio);
        add_prop("variance_preserving_blend", godot::Variant::BOOL, variance_preserving_blend);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("image_width")) image_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_width"]));
        if (p_props.has("image_height")) image_height = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_height"]));
        if (p_props.has("blend_margin_ratio")) blend_margin_ratio = std::clamp(static_cast<float>(p_props["blend_margin_ratio"]), 0.01f, 0.5f);
        if (p_props.has("variance_preserving_blend")) variance_preserving_blend = static_cast<bool>(p_props["variance_preserving_blend"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return static_cast<size_t>(std::max<int32_t>(2, image_width)) *
               static_cast<size_t>(std::max<int32_t>(2, image_height)) * sizeof(godot::Color) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        TaskContextPOD dummy{};
        return get_transient_requirement(dummy);
    }

    static inline float smooth_edge_weight(int32_t coord, int32_t dim, float margin_ratio) noexcept {
        const float dist_to_border = static_cast<float>(std::min(coord, dim - 1 - coord));
        const float margin = std::max(1.0f, static_cast<float>(dim) * margin_ratio);
        if (dist_to_border >= margin) return 1.0f;
        const float u = dist_to_border / margin;
        return 0.5f - 0.5f * std::cos(u * 3.14159265358979323846f);
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !p_ctx.local_workspace) return;

        GrantPartPOD& img_part = p_ctx.grant->parts[0];
        if (!img_part.raw_base_ptr) return;

        const int32_t w = std::max<int32_t>(2, image_width);
        const int32_t h = std::max<int32_t>(2, image_height);
        const size_t total = static_cast<size_t>(w) * h;

        if (img_part.element_stride >= sizeof(godot::Color)) {
            if (img_part.capacity_bytes / sizeof(godot::Color) < total) return;
            godot::Color* pixels = reinterpret_cast<godot::Color*>(img_part.raw_base_ptr);
            godot::Color* src_copy = reinterpret_cast<godot::Color*>(p_ctx.local_workspace);
            for (size_t i = 0; i < total; ++i) src_copy[i] = pixels[i];

            for (int32_t y = 0; y < h; ++y) {
                const float wy = smooth_edge_weight(y, h, blend_margin_ratio);
                const int32_t sy = (y + h / 2) % h;
                for (int32_t x = 0; x < w; ++x) {
                    const float wx = smooth_edge_weight(x, w, blend_margin_ratio);
                    const int32_t sx = (x + w / 2) % w;

                    const float alpha = wx * wy;
                    const godot::Color& c00 = src_copy[static_cast<size_t>(y) * w + x];
                    const godot::Color& c11 = src_copy[static_cast<size_t>(sy) * w + sx];
                    pixels[static_cast<size_t>(y) * w + x] = c11.lerp(c00, alpha);
                }
            }
        } else {
            if (img_part.capacity_bytes / sizeof(float) < total) return;
            float* pixels = reinterpret_cast<float*>(img_part.raw_base_ptr);
            float* src_copy = reinterpret_cast<float*>(p_ctx.local_workspace);
            for (size_t i = 0; i < total; ++i) src_copy[i] = pixels[i];

            for (int32_t y = 0; y < h; ++y) {
                const float wy = smooth_edge_weight(y, h, blend_margin_ratio);
                const int32_t sy = (y + h / 2) % h;
                for (int32_t x = 0; x < w; ++x) {
                    const float wx = smooth_edge_weight(x, w, blend_margin_ratio);
                    const int32_t sx = (x + w / 2) % w;

                    const float alpha = wx * wy;
                    const float v00 = src_copy[static_cast<size_t>(y) * w + x];
                    const float v11 = src_copy[static_cast<size_t>(sy) * w + sx];
                    pixels[static_cast<size_t>(y) * w + x] = v11 + (v00 - v11) * alpha;
                }
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core