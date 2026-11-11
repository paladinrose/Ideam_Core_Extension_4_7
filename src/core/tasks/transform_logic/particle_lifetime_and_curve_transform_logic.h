#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * V2: ParticleLifetimeAndCurveTransformLogic
 * Advances particle Age += delta, computes normalized age t in [0, 1], and evaluates smooth
 * scale and color/opacity curves over the particle's lifetime.
 */
struct ParticleLifetimeAndCurveTransformLogic {
    static constexpr std::string_view ui_name = "Particle Age Advance & Lifetime Curve Ramp";
    static constexpr std::string_view ui_category = "Particles/Lifecycle";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::AOSOA,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::SOA,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::VECTOR2,
        DataType::FLOAT32,
        DataType::COLOR
    };

    float fallback_lifetime = 2.0f;
    float scale_birth = 0.25f;
    float scale_peak = 1.0f;
    float scale_death = 0.0f;
    float peak_normalized_time = 0.25f;
    godot::Color color_birth = godot::Color(1.0f, 0.9f, 0.5f, 1.0f);
    godot::Color color_death = godot::Color(0.8f, 0.2f, 0.05f, 0.0f);

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("fallback_lifetime", godot::Variant::FLOAT, fallback_lifetime);
        add_prop("scale_birth", godot::Variant::FLOAT, scale_birth);
        add_prop("scale_peak", godot::Variant::FLOAT, scale_peak);
        add_prop("scale_death", godot::Variant::FLOAT, scale_death);
        add_prop("peak_normalized_time", godot::Variant::FLOAT, peak_normalized_time);
        add_prop("color_birth", godot::Variant::COLOR, color_birth);
        add_prop("color_death", godot::Variant::COLOR, color_death);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("fallback_lifetime")) fallback_lifetime = std::max(1e-3f, static_cast<float>(p_props["fallback_lifetime"]));
        if (p_props.has("scale_birth")) scale_birth = static_cast<float>(p_props["scale_birth"]);
        if (p_props.has("scale_peak")) scale_peak = static_cast<float>(p_props["scale_peak"]);
        if (p_props.has("scale_death")) scale_death = static_cast<float>(p_props["scale_death"]);
        if (p_props.has("peak_normalized_time")) peak_normalized_time = std::clamp(static_cast<float>(p_props["peak_normalized_time"]), 0.01f, 0.99f);
        if (p_props.has("color_birth")) color_birth = p_props["color_birth"];
        if (p_props.has("color_death")) color_death = p_props["color_death"];
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    inline float evaluate_scale_ramp(float t) const noexcept {
        if (t <= peak_normalized_time) {
            const float u = t / peak_normalized_time;
            const float smooth = u * u * (3.0f - 2.0f * u);
            return scale_birth + (scale_peak - scale_birth) * smooth;
        }
        const float u = (t - peak_normalized_time) / (1.0f - peak_normalized_time);
        const float smooth = u * u * (3.0f - 2.0f * u);
        return scale_peak + (scale_death - scale_peak) * smooth;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& life_part = p_ctx.grant->parts[0];
        if (!life_part.raw_base_ptr) return;

        GrantPartPOD* scale_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;
        GrantPartPOD* color_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        const bool is_packed_v2 = (life_part.element_stride >= sizeof(godot::Vector2));
        const uint32_t l_stride = std::max<uint32_t>(is_packed_v2 ? sizeof(godot::Vector2) : sizeof(float), life_part.element_stride);
        const int64_t n_particles = (life_part.selection.element_count > 0)
            ? life_part.selection.element_count
            : static_cast<int64_t>(life_part.capacity_bytes / l_stride);

        const float dt = (p_ctx.delta > 0.0) ? static_cast<float>(p_ctx.delta) : (1.0f / 60.0f);

        for (int64_t i = 0; i < n_particles; ++i) {
            float age = 0.0f;
            float lifetime = fallback_lifetime;

            if (is_packed_v2) {
                godot::Vector2& al = *reinterpret_cast<godot::Vector2*>(life_part.raw_base_ptr + static_cast<size_t>(i) * l_stride);
                al.x += dt;
                age = al.x;
                lifetime = std::max(1e-4f, al.y);
            } else {
                float& a = *reinterpret_cast<float*>(life_part.raw_base_ptr + static_cast<size_t>(i) * l_stride);
                a += dt;
                age = a;
            }

            const float t = std::clamp(age / lifetime, 0.0f, 1.0f);

            if (scale_part && static_cast<size_t>(i + 1) * sizeof(float) <= scale_part->capacity_bytes) {
                reinterpret_cast<float*>(scale_part->raw_base_ptr)[i] = evaluate_scale_ramp(t);
            }
            if (color_part && static_cast<size_t>(i + 1) * sizeof(godot::Color) <= color_part->capacity_bytes) {
                reinterpret_cast<godot::Color*>(color_part->raw_base_ptr)[i] = color_birth.lerp(color_death, t);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core