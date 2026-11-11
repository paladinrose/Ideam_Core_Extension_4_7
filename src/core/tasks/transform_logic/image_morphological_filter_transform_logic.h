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
 * I4: ImageMorphologicalFilterTransformLogic
 * Grayscale and color mathematical morphology supporting ERODE, DILATE, OPEN, CLOSE,
 * and MORPHOLOGICAL_GRADIENT (dilate - erode) with Box, Cross, or Disk structuring elements.
 */
struct ImageMorphologicalFilterTransformLogic {
    static constexpr std::string_view ui_name = "Image Morphological Filter (Erode/Dilate/Open/Close)";
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
        DataType::FLOAT32,
        DataType::COLOR,
        DataType::VECTOR4
    };

    int32_t image_width = 256;
    int32_t image_height = 256;
    int32_t operation_mode = 0; // 0 = ERODE, 1 = DILATE, 2 = OPEN, 3 = CLOSE, 4 = MORPHOLOGICAL_GRADIENT
    int32_t structuring_shape = 2; // 0 = Box, 1 = Cross, 2 = Disk
    int32_t radius = 1;

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
        add_prop("operation_mode", godot::Variant::INT, operation_mode, 2, "Erode,Dilate,Open,Close,Morphological Gradient");
        add_prop("structuring_shape", godot::Variant::INT, structuring_shape, 2, "Box,Cross,Disk");
        add_prop("radius", godot::Variant::INT, radius);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("image_width")) image_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_width"]));
        if (p_props.has("image_height")) image_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_height"]));
        if (p_props.has("operation_mode")) operation_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["operation_mode"]), 0, 4);
        if (p_props.has("structuring_shape")) structuring_shape = std::clamp<int32_t>(static_cast<int32_t>(p_props["structuring_shape"]), 0, 2);
        if (p_props.has("radius")) radius = std::clamp<int32_t>(static_cast<int32_t>(p_props["radius"]), 1, 16);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        const size_t n = static_cast<size_t>(std::max<int32_t>(1, image_width)) *
                         static_cast<size_t>(std::max<int32_t>(1, image_height));
        return n * sizeof(godot::Color) * 2 + 256;
    }

    size_t get_transient_requirement() const noexcept {
        TaskContextPOD dummy{};
        return get_transient_requirement(dummy);
    }

    inline bool in_structuring_element(int32_t dx, int32_t dy) const noexcept {
        if (structuring_shape == 1) return (dx == 0 || dy == 0);
        if (structuring_shape == 2) return (dx * dx + dy * dy) <= (radius * radius);
        return true;
    }

    void pass_float(const float* src, float* dst, int32_t w, int32_t h, bool is_dilate) const noexcept {
        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
                float best = is_dilate ? -1e30f : 1e30f;
                for (int32_t dy = -radius; dy <= radius; ++dy) {
                    const int32_t ny = std::clamp(y + dy, 0, h - 1);
                    for (int32_t dx = -radius; dx <= radius; ++dx) {
                        if (!in_structuring_element(dx, dy)) continue;
                        const int32_t nx = std::clamp(x + dx, 0, w - 1);
                        const float v = src[static_cast<size_t>(ny) * w + nx];
                        best = is_dilate ? std::max(best, v) : std::min(best, v);
                    }
                }
                dst[static_cast<size_t>(y) * w + x] = best;
            }
        }
    }

    void pass_color(const godot::Color* src, godot::Color* dst, int32_t w, int32_t h, bool is_dilate) const noexcept {
        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
                godot::Color best = src[static_cast<size_t>(y) * w + x];
                for (int32_t dy = -radius; dy <= radius; ++dy) {
                    const int32_t ny = std::clamp(y + dy, 0, h - 1);
                    for (int32_t dx = -radius; dx <= radius; ++dx) {
                        if (!in_structuring_element(dx, dy)) continue;
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

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !p_ctx.local_workspace) return;

        const GrantPartPOD& src_part = p_ctx.grant->parts[0];
        GrantPartPOD& dst_part = (p_ctx.grant->part_count > 1) ? p_ctx.grant->parts[1] : p_ctx.grant->parts[0];
        if (!src_part.raw_base_ptr || !dst_part.raw_base_ptr) return;

        const int32_t w = std::max<int32_t>(1, image_width);
        const int32_t h = std::max<int32_t>(1, image_height);
        const size_t total = static_cast<size_t>(w) * h;

        if (src_part.element_stride >= sizeof(godot::Color)) {
            if (src_part.capacity_bytes / sizeof(godot::Color) < total ||
                dst_part.capacity_bytes / sizeof(godot::Color) < total) return;

            const auto* src_c = reinterpret_cast<const godot::Color*>(src_part.raw_base_ptr);
            auto* dst_c = reinterpret_cast<godot::Color*>(dst_part.raw_base_ptr);
            auto* tmp_a = reinterpret_cast<godot::Color*>(p_ctx.local_workspace);
            auto* tmp_b = tmp_a + total;

            if (operation_mode == 0) {
                pass_color(src_c, tmp_a, w, h, false);
                for (size_t i = 0; i < total; ++i) dst_c[i] = tmp_a[i];
            } else if (operation_mode == 1) {
                pass_color(src_c, tmp_a, w, h, true);
                for (size_t i = 0; i < total; ++i) dst_c[i] = tmp_a[i];
            } else if (operation_mode == 2) {
                // OPEN: erode then dilate
                pass_color(src_c, tmp_a, w, h, false);
                pass_color(tmp_a, dst_c, w, h, true);
            } else if (operation_mode == 3) {
                // CLOSE: dilate then erode
                pass_color(src_c, tmp_a, w, h, true);
                pass_color(tmp_a, dst_c, w, h, false);
            } else {
                // MORPHOLOGICAL_GRADIENT: dilate - erode
                pass_color(src_c, tmp_a, w, h, true);
                pass_color(src_c, tmp_b, w, h, false);
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
            if (src_part.capacity_bytes / sizeof(float) < total ||
                dst_part.capacity_bytes / sizeof(float) < total) return;

            const float* src_f = reinterpret_cast<const float*>(src_part.raw_base_ptr);
            float* dst_f = reinterpret_cast<float*>(dst_part.raw_base_ptr);
            float* tmp_a = reinterpret_cast<float*>(p_ctx.local_workspace);
            float* tmp_b = tmp_a + total;

            if (operation_mode == 0) {
                pass_float(src_f, tmp_a, w, h, false);
                for (size_t i = 0; i < total; ++i) dst_f[i] = tmp_a[i];
            } else if (operation_mode == 1) {
                pass_float(src_f, tmp_a, w, h, true);
                for (size_t i = 0; i < total; ++i) dst_f[i] = tmp_a[i];
            } else if (operation_mode == 2) {
                pass_float(src_f, tmp_a, w, h, false);
                pass_float(tmp_a, dst_f, w, h, true);
            } else if (operation_mode == 3) {
                pass_float(src_f, tmp_a, w, h, true);
                pass_float(tmp_a, dst_f, w, h, false);
            } else {
                pass_float(src_f, tmp_a, w, h, true);
                pass_float(src_f, tmp_b, w, h, false);
                for (size_t i = 0; i < total; ++i) dst_f[i] = tmp_a[i] - tmp_b[i];
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core