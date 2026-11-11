#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../tasks/i_native_task.h"
#include "../../tasks/task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * SPL10: Spline4DColorGradientMapTransformLogic
 * Maps scalar DOD columns through a 4D Catmull-Rom Spline4D gradient (in HSVA, Oklab, or direct RGBA
 * space with shortest-arc angular Hue unwrapping) into Color / Vector4 buffers.
 */
struct Spline4DColorGradientMapTransformLogic {
    static constexpr std::string_view ui_name = "Spline 4D HSVA Color Gradient Map";
    static constexpr std::string_view ui_category = "Splines/4D";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::AOSOA,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::COLOR,
        DataType::VECTOR4,
        DataType::FLOAT32
    };

    int32_t color_space_mode = 0; // 0 = HSVA -> Linear RGBA, 1 = Direct RGBA, 2 = Oklab -> Linear RGBA
    float input_min = 0.0f;
    float input_max = 1.0f;
    bool wrap_hue = true;

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
        add_prop("color_space_mode", godot::Variant::INT, color_space_mode, 2, "HSVA -> RGBA,Direct RGBA,Oklab -> RGBA");
        add_prop("input_min", godot::Variant::FLOAT, input_min);
        add_prop("input_max", godot::Variant::FLOAT, input_max);
        add_prop("wrap_hue", godot::Variant::BOOL, wrap_hue);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("color_space_mode")) color_space_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["color_space_mode"]), 0, 2);
        if (p_props.has("input_min")) input_min = static_cast<float>(p_props["input_min"]);
        if (p_props.has("input_max")) input_max = static_cast<float>(p_props["input_max"]);
        if (p_props.has("wrap_hue")) wrap_hue = static_cast<bool>(p_props["wrap_hue"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return 0;
    }

    size_t get_transient_requirement() const noexcept {
        return 0;
    }

    static inline float catmull_rom_scalar(float p0, float p1, float p2, float p3, float t) noexcept {
        const float t2 = t * t;
        const float t3 = t2 * t;
        return 0.5f * ((2.0f * p1) +
                       (-p0 + p2) * t +
                       (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                       (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    static inline godot::Color hsva_to_rgba(const godot::Vector4& hsva) noexcept {
        float h = std::fmod(hsva.x, 1.0f);
        if (h < 0.0f) h += 1.0f;
        const float s = std::clamp(hsva.y, 0.0f, 1.0f);
        const float v = std::clamp(hsva.z, 0.0f, 1.0f);
        const float a = std::clamp(hsva.w, 0.0f, 1.0f);
        return godot::Color::from_hsv(h, s, v, a);
    }

    static inline godot::Color oklab_to_rgba(const godot::Vector4& lab) noexcept {
        const float L = lab.x;
        const float a = lab.y;
        const float b = lab.z;
        const float l_ = L + 0.3963377774f * a + 0.2158037573f * b;
        const float m_ = L - 0.1055613458f * a - 0.0638541728f * b;
        const float s_ = L - 0.0894841775f * a - 1.2914855480f * b;

        const float l = l_ * l_ * l_;
        const float m = m_ * m_ * m_;
        const float s = s_ * s_ * s_;

        const float r = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
        const float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
        const float bl = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;
        return godot::Color(
            std::clamp(r, 0.0f, 1.0f),
            std::clamp(g, 0.0f, 1.0f),
            std::clamp(bl, 0.0f, 1.0f),
            std::clamp(lab.w, 0.0f, 1.0f)
        );
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& scalar_part = p_ctx.grant->parts[0];
        GrantPartPOD& color_out_part = p_ctx.grant->parts[1];
        if (!scalar_part.raw_base_ptr || !color_out_part.raw_base_ptr) return;

        const GrantPartPOD* knot_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        // Default 4-stop HSVA gradient knots if no external Spline4D knot buffer is bound
        static constexpr std::array<godot::Vector4, 4> DEFAULT_KNOTS = {{
            godot::Vector4(0.60f, 0.85f, 0.25f, 1.0f),
            godot::Vector4(0.50f, 0.90f, 0.65f, 1.0f),
            godot::Vector4(0.12f, 0.95f, 0.92f, 1.0f),
            godot::Vector4(0.02f, 0.80f, 1.00f, 1.0f)
        }};

        const godot::Vector4* knots = DEFAULT_KNOTS.data();
        int32_t knot_count = static_cast<int32_t>(DEFAULT_KNOTS.size());
        if (knot_part && knot_part->capacity_bytes >= sizeof(godot::Vector4) * 2) {
            knots = reinterpret_cast<const godot::Vector4*>(knot_part->raw_base_ptr);
            knot_count = (knot_part->selection.element_count > 0)
                ? static_cast<int32_t>(knot_part->selection.element_count)
                : static_cast<int32_t>(knot_part->capacity_bytes / sizeof(godot::Vector4));
            knot_count = std::max<int32_t>(2, knot_count);
        }

        const uint32_t s_stride = std::max<uint32_t>(sizeof(float), scalar_part.element_stride);
        const int64_t n_elems = (scalar_part.selection.element_count > 0)
            ? scalar_part.selection.element_count
            : static_cast<int64_t>(scalar_part.capacity_bytes / s_stride);

        const uint32_t c_stride = std::max<uint32_t>(sizeof(godot::Color), color_out_part.element_stride);
        const float range = std::max(1e-6f, input_max - input_min);

        for (int64_t i = 0; i < n_elems; ++i) {
            if (static_cast<size_t>(i + 1) * c_stride > color_out_part.capacity_bytes) break;

            const float val = *reinterpret_cast<const float*>(scalar_part.raw_base_ptr + static_cast<size_t>(i) * s_stride);
            const float u = std::clamp((val - input_min) / range, 0.0f, 1.0f);

            const float scaled = u * static_cast<float>(knot_count - 1);
            const int32_t i1 = std::clamp<int32_t>(static_cast<int32_t>(std::floor(scaled)), 0, knot_count - 2);
            const int32_t i0 = std::max<int32_t>(0, i1 - 1);
            const int32_t i2 = std::min<int32_t>(knot_count - 1, i1 + 1);
            const int32_t i3 = std::min<int32_t>(knot_count - 1, i1 + 2);
            const float local_t = scaled - static_cast<float>(i1);

            godot::Vector4 p0 = knots[i0];
            godot::Vector4 p1 = knots[i1];
            godot::Vector4 p2 = knots[i2];
            godot::Vector4 p3 = knots[i3];

            if (color_space_mode == 0 && wrap_hue) {
                auto unwrap = [](float ref, float ang) noexcept -> float {
                    float diff = std::fmod(ang - ref, 1.0f);
                    if (diff > 0.5f) diff -= 1.0f;
                    if (diff < -0.5f) diff += 1.0f;
                    return ref + diff;
                };
                p0.x = unwrap(p1.x, p0.x);
                p2.x = unwrap(p1.x, p2.x);
                p3.x = unwrap(p2.x, p3.x);
            }

            const godot::Vector4 eval4(
                catmull_rom_scalar(p0.x, p1.x, p2.x, p3.x, local_t),
                catmull_rom_scalar(p0.y, p1.y, p2.y, p3.y, local_t),
                catmull_rom_scalar(p0.z, p1.z, p2.z, p3.z, local_t),
                catmull_rom_scalar(p0.w, p1.w, p2.w, p3.w, local_t)
            );

            godot::Color out_c;
            if (color_space_mode == 0) {
                out_c = hsva_to_rgba(eval4);
            } else if (color_space_mode == 2) {
                out_c = oklab_to_rgba(eval4);
            } else {
                out_c = godot::Color(
                    std::clamp(eval4.x, 0.0f, 1.0f),
                    std::clamp(eval4.y, 0.0f, 1.0f),
                    std::clamp(eval4.z, 0.0f, 1.0f),
                    std::clamp(eval4.w, 0.0f, 1.0f)
                );
            }

            *reinterpret_cast<godot::Color*>(color_out_part.raw_base_ptr + static_cast<size_t>(i) * c_stride) = out_c;
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core