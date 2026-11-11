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
 * I9: ImageGradientMagnitudeQueryLogic
 * Evaluates the 3x3 Sobel gradient magnitude |grad I| = sqrt(Gx^2 + Gy^2) across a 2D image
 * buffer, waking or sleeping edge pixels whose gradient magnitude exceeds edge_threshold.
 */
struct ImageGradientMagnitudeQueryLogic {
    static constexpr std::string_view ui_name = "Image Sobel Gradient Magnitude Query";
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
    static constexpr std::array<QueryOp, 3> supported_ops = {
        QueryOp::WAKE,
        QueryOp::SLEEP,
        QueryOp::INVERT
    };

    int32_t image_width = 256;
    int32_t image_height = 256;
    float edge_threshold = 0.15f;

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
        add_prop("edge_threshold", godot::Variant::FLOAT, edge_threshold);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("image_width")) image_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_width"]));
        if (p_props.has("image_height")) image_height = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_height"]));
        if (p_props.has("edge_threshold")) edge_threshold = std::max(0.0f, static_cast<float>(p_props["edge_threshold"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline void apply_selection_op(MemoryBufferSelectionPOD& r_sel, int64_t idx, QueryOp p_op) noexcept {
        if (idx < 0 || idx >= r_sel.capacity) return;
        if (r_sel.mode == SelectionMode::DENSE && r_sel.data.bitset) {
            const uint64_t word = static_cast<uint64_t>(idx) >> 6;
            const uint64_t bit = 1ULL << (idx & 63);
            const bool was_set = (r_sel.data.bitset[word] & bit) != 0;
            if (p_op == QueryOp::WAKE && !was_set) {
                r_sel.data.bitset[word] |= bit;
                r_sel.element_count++;
            } else if (p_op == QueryOp::SLEEP && was_set) {
                r_sel.data.bitset[word] &= ~bit;
                r_sel.element_count = std::max<int64_t>(0, r_sel.element_count - 1);
            } else if (p_op == QueryOp::INVERT) {
                r_sel.data.bitset[word] ^= bit;
                r_sel.element_count += was_set ? -1 : 1;
            }
        } else if (r_sel.mode == SelectionMode::SPARSE && r_sel.data.indices) {
            if (p_op == QueryOp::WAKE && r_sel.element_count < r_sel.capacity) {
                r_sel.data.indices[r_sel.element_count++] = idx;
            }
        }
    }

    void execute_with_op(const TaskContextPOD& p_ctx, QueryOp p_op) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& img_part = p_ctx.grant->parts[0];
        if (!img_part.raw_base_ptr) return;

        MemoryBufferSelectionPOD& sel = img_part.selection;
        const int32_t w = std::max<int32_t>(2, image_width);
        const int32_t h = std::max<int32_t>(2, image_height);
        const size_t total = static_cast<size_t>(w) * h;

        const bool is_color = (img_part.element_stride >= sizeof(godot::Color));
        const uint32_t stride = std::max<uint32_t>(is_color ? sizeof(godot::Color) : sizeof(float), img_part.element_stride);
        if (img_part.capacity_bytes / stride < total) return;

        auto sample_lum = [&](int32_t x, int32_t y) noexcept -> float {
            const int32_t cx = std::clamp(x, 0, w - 1);
            const int32_t cy = std::clamp(y, 0, h - 1);
            const uint8_t* ptr = img_part.raw_base_ptr + (static_cast<size_t>(cy) * w + cx) * stride;
            if (is_color) {
                const godot::Color& c = *reinterpret_cast<const godot::Color*>(ptr);
                return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
            }
            return *reinterpret_cast<const float*>(ptr);
        };

        const float thresh2 = edge_threshold * edge_threshold;

        for (int32_t y = 0; y < h; ++y) {
            for (int32_t x = 0; x < w; ++x) {
                const float i00 = sample_lum(x - 1, y - 1);
                const float i10 = sample_lum(x,     y - 1);
                const float i20 = sample_lum(x + 1, y - 1);
                const float i01 = sample_lum(x - 1, y);
                const float i21 = sample_lum(x + 1, y);
                const float i02 = sample_lum(x - 1, y + 1);
                const float i12 = sample_lum(x,     y + 1);
                const float i22 = sample_lum(x + 1, y + 1);

                const float gx = ((i20 + 2.0f * i21 + i22) - (i00 + 2.0f * i01 + i02)) * 0.125f;
                const float gy = ((i02 + 2.0f * i12 + i22) - (i00 + 2.0f * i10 + i20)) * 0.125f;

                if (gx * gx + gy * gy >= thresh2) {
                    apply_selection_op(sel, static_cast<int64_t>(y) * w + x, p_op);
                }
            }
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, MemoryBufferSelectionPOD& /*r_selection*/, QueryOp p_op, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, p_op);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute_with_op(p_ctx, QueryOp::WAKE);
    }
};

} // namespace ideam::core