#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * I1: ImageNormalFromHeightmapTransformLogic
 * Computes 3x3 Sobel or Scharr height gradients across a 2D heightmap buffer and packs
 * tangent-space RGBA normal vectors ((nx*0.5+0.5, ny*0.5+0.5, nz*0.5+0.5, height)) into
 * a Color/Vector4 or float buffer.
 */
struct ImageNormalFromHeightmapTransformLogic {
    static constexpr std::string_view ui_name = "Image Normal Map from Heightmap";
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
    float bump_strength = 4.0f;
    int32_t kernel_mode = 0; // 0 = Sobel (3x3), 1 = Scharr (3x3 rotational symmetry)
    bool wrap_seamless_borders = true;
    bool invert_green_y = false;

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
        add_prop("image_width", godot::Variant::INT, image_width);
        add_prop("image_height", godot::Variant::INT, image_height);
        add_prop("bump_strength", godot::Variant::FLOAT, bump_strength);
        add_prop("kernel_mode", godot::Variant::INT, kernel_mode, 2, "Sobel 3x3,Scharr 3x3");
        add_prop("wrap_seamless_borders", godot::Variant::BOOL, wrap_seamless_borders);
        add_prop("invert_green_y", godot::Variant::BOOL, invert_green_y);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("image_width")) image_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_width"]));
        if (p_props.has("image_height")) image_height = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_height"]));
        if (p_props.has("bump_strength")) bump_strength = std::max(0.0f, static_cast<float>(p_props["bump_strength"]));
        if (p_props.has("kernel_mode")) kernel_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["kernel_mode"]), 0, 1);
        if (p_props.has("wrap_seamless_borders")) wrap_seamless_borders = static_cast<bool>(p_props["wrap_seamless_borders"]);
        if (p_props.has("invert_green_y")) invert_green_y = static_cast<bool>(p_props["invert_green_y"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& h_part = p_ctx.grant->parts[0];
        GrantPartPOD& nrm_part = p_ctx.grant->parts[1];
        if (!h_part.raw_base_ptr || !nrm_part.raw_base_ptr) return;

        const int32_t w = std::max<int32_t>(2, image_width);
        const int32_t h = std::max<int32_t>(2, image_height);
        const size_t total_pixels = static_cast<size_t>(w) * h;

        const uint32_t h_stride = std::max<uint32_t>(sizeof(float), h_part.element_stride);
        const uint32_t n_stride = std::max<uint32_t>(sizeof(godot::Color), nrm_part.element_stride);
        if (h_part.capacity_bytes / h_stride < total_pixels || nrm_part.capacity_bytes / n_stride < total_pixels) return;

        auto sample_h = [&](int32_t x, int32_t y) noexcept -> float {
            int32_t cx = x;
            int32_t cy = y;
            if (wrap_seamless_borders) {
                cx = ((x % w) + w) % w;
                cy = ((y % h) + h) % h;
            } else {
                cx = std::clamp(x, 0, w - 1);
                cy = std::clamp(y, 0, h - 1);
            }
            const size_t idx = static_cast<size_t>(cy) * w + cx;
            return *reinterpret_cast<const float*>(h_part.raw_base_ptr + idx * h_stride);
        };

        const float w_corner = (kernel_mode == 1) ? 3.0f : 1.0f;
        const float w_center = (kernel_mode == 1) ? 10.0f : 2.0f;
        const float norm_div = 2.0f * (2.0f * w_corner + w_center);
        const float y_sign = invert_green_y ? -1.0f : 1.0f;

        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
                const float h00 = sample_h(x - 1, y - 1);
                const float h10 = sample_h(x,     y - 1);
                const float h20 = sample_h(x + 1, y - 1);
                const float h01 = sample_h(x - 1, y);
                const float h11 = sample_h(x,     y);
                const float h21 = sample_h(x + 1, y);
                const float h02 = sample_h(x - 1, y + 1);
                const float h12 = sample_h(x,     y + 1);
                const float h22 = sample_h(x + 1, y + 1);

                const float gx = ((w_corner * h20 + w_center * h21 + w_corner * h22) -
                                  (w_corner * h00 + w_center * h01 + w_corner * h02)) / norm_div;
                const float gy = ((w_corner * h02 + w_center * h12 + w_corner * h22) -
                                  (w_corner * h00 + w_center * h10 + w_corner * h20)) / norm_div;

                const godot::Vector3 n = godot::Vector3(-gx * bump_strength, -gy * bump_strength * y_sign, 1.0f).normalized();
                const size_t idx = static_cast<size_t>(y) * w + x;
                *reinterpret_cast<godot::Color*>(nrm_part.raw_base_ptr + idx * n_stride) = godot::Color(
                    n.x * 0.5f + 0.5f,
                    n.y * 0.5f + 0.5f,
                    n.z * 0.5f + 0.5f,
                    std::clamp(h11, 0.0f, 1.0f)
                );
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core