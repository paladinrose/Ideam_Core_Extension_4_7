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

enum class ImageMorphologicalOp : uint8_t {
    ERODE                  = 0,
    DILATE                 = 1,
    OPEN                   = 2,
    CLOSE                  = 3,
    MORPHOLOGICAL_GRADIENT = 4
};

enum class ImageStructuringShape : uint8_t {
    BOX   = 0,
    CROSS = 1,
    DISK  = 2
};

/**
 * ImageMorphologicalFilterTransformLogic<T, T_Strategy, KernelSize> (I4)
 * Grayscale and color mathematical morphology supporting ERODE, DILATE, OPEN, CLOSE,
 * and MORPHOLOGICAL_GRADIENT (dilate - erode) with Box, Cross, or Disk structuring elements.
 */
template <
    typename T = float,
    typename T_Strategy = Spatial2DStrategy,
    size_t KernelSize = 8
>
struct alignas(64) ImageMorphologicalFilterTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StaticStencilView<T, DefaultStrategy, KernelSize>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::COLOR | DataType::VECTOR4;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size          = KernelSize;

    static constexpr size_t transient_workspace_bytes = 256 * 256 * sizeof(godot::Color) * 2 + 256;

    static constexpr std::string_view display_name = "Image Morphological Filter";
    static constexpr std::string_view ui_category  = "Image";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t source_buffer_id = INVALID_ID; // 4B: Source 2D image buffer
    uint32_t output_buffer_id = INVALID_ID; // 4B: Optional destination buffer
    int32_t image_width       = 256;        // 4B: Grid width in pixels
    int32_t image_height      = 256;        // 4B: Grid height in pixels
    int32_t radius            = 1;          // 4B: Structuring element radius [1..16]

    ImageMorphologicalOp operation_mode     = ImageMorphologicalOp::ERODE;   // 1B
    ImageStructuringShape structuring_shape = ImageStructuringShape::DISK;   // 1B
    uint8_t reserved_padding[42]            = {0};                           // 42B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& /*p_context*/) const noexcept {
        const size_t n = static_cast<size_t>(std::max<int32_t>(1, image_width)) *
                         static_cast<size_t>(std::max<int32_t>(1, image_height));
        return n * sizeof(godot::Color) * 2 + 256;
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

        godot::Dictionary op_prop;
        op_prop["name"] = "operation_mode";
        op_prop["type"] = godot::Variant::INT;
        op_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        op_prop["hint_string"] = "Erode:0,Dilate:1,Open:2,Close:3,Morphological Gradient:4";
        props.push_back(op_prop);

        godot::Dictionary shp_prop;
        shp_prop["name"] = "structuring_shape";
        shp_prop["type"] = godot::Variant::INT;
        shp_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        shp_prop["hint_string"] = "Box:0,Cross:1,Disk:2";
        props.push_back(shp_prop);

        godot::Dictionary rad_prop;
        rad_prop["name"] = "radius";
        rad_prop["type"] = godot::Variant::INT;
        rad_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        rad_prop["hint_string"] = "1,16,1,suffix:px";
        props.push_back(rad_prop);

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
        if (p_props.has("operation_mode")) {
            operation_mode = static_cast<ImageMorphologicalOp>(std::clamp<int64_t>(static_cast<int64_t>(p_props["operation_mode"]), 0, 4));
        }
        if (p_props.has("structuring_shape")) {
            structuring_shape = static_cast<ImageStructuringShape>(std::clamp<int64_t>(static_cast<int64_t>(p_props["structuring_shape"]), 0, 2));
        }
        if (p_props.has("radius")) {
            radius = std::clamp<int32_t>(static_cast<int32_t>(p_props["radius"]), 1, 16);
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
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, godot::Color> || std::is_same_v<T, godot::Vector4>) {
            if (!p_context.local_workspace) return;

            const uint32_t src_id = (source_buffer_id != INVALID_ID) ? source_buffer_id : output_buffer_id;
            const uint32_t dst_id = (output_buffer_id != INVALID_ID) ? output_buffer_id : src_id;
            const GrantPartPOD* src_part = p_context.get_grant_part(src_id);
            const GrantPartPOD* dst_part = p_context.get_grant_part(dst_id);
            if (!src_part || !dst_part || !src_part->raw_base_ptr || !dst_part->raw_base_ptr) return;

            const int32_t w = std::max<int32_t>(1, image_width);
            const int32_t h = std::max<int32_t>(1, image_height);
            const size_t total = static_cast<size_t>(w) * static_cast<size_t>(h);

            if (src_part->element_stride >= sizeof(godot::Color)) {
                if (src_part->capacity_bytes / sizeof(godot::Color) < total ||
                    dst_part->capacity_bytes / sizeof(godot::Color) < total) return;

                const auto* src_c = reinterpret_cast<const godot::Color*>(src_part->raw_base_ptr);
                auto* dst_c = reinterpret_cast<godot::Color*>(dst_part->raw_base_ptr);
                auto* tmp_a = reinterpret_cast<godot::Color*>(p_context.local_workspace);
                auto* tmp_b = tmp_a + total;

                if (operation_mode == ImageMorphologicalOp::ERODE) {
                    _pass_color(src_c, tmp_a, w, h, false);
                    for (size_t i = 0; i < total; ++i) dst_c[i] = tmp_a[i];
                } else if (operation_mode == ImageMorphologicalOp::DILATE) {
                    _pass_color(src_c, tmp_a, w, h, true);
                    for (size_t i = 0; i < total; ++i) dst_c[i] = tmp_a[i];
                } else if (operation_mode == ImageMorphologicalOp::OPEN) {
                    _pass_color(src_c, tmp_a, w, h, false);
                    _pass_color(tmp_a, dst_c, w, h, true);
                } else if (operation_mode == ImageMorphologicalOp::CLOSE) {
                    _pass_color(src_c, tmp_a, w, h, true);
                    _pass_color(tmp_a, dst_c, w, h, false);
                } else {
                    _pass_color(src_c, tmp_a, w, h, true);
                    _pass_color(src_c, tmp_b, w, h, false);
                    for (size_t i = 0; i < total; ++i) {
                        dst_c[i] = godot::Color(
                            tmp_a[i].r - tmp_b[i].r,
                            tmp_a[i].g - tmp_b[i].g,
                            tmp_a[i].b - tmp_b[i].b,
                            src_c[i].a
                        );
                    }
                }
            } else {
                if (src_part->capacity_bytes / sizeof(float) < total ||
                    dst_part->capacity_bytes / sizeof(float) < total) return;

                const float* src_f = reinterpret_cast<const float*>(src_part->raw_base_ptr);
                float* dst_f = reinterpret_cast<float*>(dst_part->raw_base_ptr);
                float* tmp_a = reinterpret_cast<float*>(p_context.local_workspace);
                float* tmp_b = tmp_a + total;

                if (operation_mode == ImageMorphologicalOp::ERODE) {
                    _pass_float(src_f, tmp_a, w, h, false);
                    for (size_t i = 0; i < total; ++i) dst_f[i] = tmp_a[i];
                } else if (operation_mode == ImageMorphologicalOp::DILATE) {
                    _pass_float(src_f, tmp_a, w, h, true);
                    for (size_t i = 0; i < total; ++i) dst_f[i] = tmp_a[i];
                } else if (operation_mode == ImageMorphologicalOp::OPEN) {
                    _pass_float(src_f, tmp_a, w, h, false);
                    _pass_float(tmp_a, dst_f, w, h, true);
                } else if (operation_mode == ImageMorphologicalOp::CLOSE) {
                    _pass_float(src_f, tmp_a, w, h, true);
                    _pass_float(tmp_a, dst_f, w, h, false);
                } else {
                    _pass_float(src_f, tmp_a, w, h, true);
                    _pass_float(src_f, tmp_b, w, h, false);
                    for (size_t i = 0; i < total; ++i) dst_f[i] = tmp_a[i] - tmp_b[i];
                }
            }
        }
    }

private:
    inline bool _in_structuring_element(int32_t dx, int32_t dy) const noexcept {
        if (structuring_shape == ImageStructuringShape::CROSS) return (dx == 0 || dy == 0);
        if (structuring_shape == ImageStructuringShape::DISK)  return (dx * dx + dy * dy) <= (radius * radius);
        return true;
    }

    void _pass_float(const float* src, float* dst, int32_t w, int32_t h, bool is_dilate) const noexcept {
        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
                float best = is_dilate ? -1e30f : 1e30f;
                for (int32_t dy = -radius; dy <= radius; ++dy) {
                    const int32_t ny = std::clamp(y + dy, 0, h - 1);
                    for (int32_t dx = -radius; dx <= radius; ++dx) {
                        if (!_in_structuring_element(dx, dy)) continue;
                        const int32_t nx = std::clamp(x + dx, 0, w - 1);
                        const float v = src[static_cast<size_t>(ny) * w + nx];
                        best = is_dilate ? std::max(best, v) : std::min(best, v);
                    }
                }
                dst[static_cast<size_t>(y) * w + x] = best;
            }
        }
    }

    void _pass_color(const godot::Color* src, godot::Color* dst, int32_t w, int32_t h, bool is_dilate) const noexcept {
        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
                godot::Color best = src[static_cast<size_t>(y) * w + x];
                for (int32_t dy = -radius; dy <= radius; ++dy) {
                    const int32_t ny = std::clamp(y + dy, 0, h - 1);
                    for (int32_t dx = -radius; dx <= radius; ++dx) {
                        if (!_in_structuring_element(dx, dy)) continue;
                        const int32_t nx = std::clamp(x + dx, 0, w - 1);
                        const godot::Color& c = src[static_cast<size_t>(ny) * w + nx];
                        if (is_dilate) {
                            best.r = std::max(best.r, c.r);
                            best.g = std::max(best.g, c.g);
                            best.b = std::max(best.b, c.b);
                        } else {
                            best.r = std::min(best.r, c.r);
                            best.g = std::min(best.g, c.g);
                            best.b = std::min(best.b, c.b);
                        }
                    }
                }
                dst[static_cast<size_t>(y) * w + x] = best;
            }
        }
    }
};

static_assert(sizeof(ImageMorphologicalFilterTransformLogic<>) == 64, "ImageMorphologicalFilterTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ImageMorphologicalFilterTransformLogic<>>, "ImageMorphologicalFilterTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core