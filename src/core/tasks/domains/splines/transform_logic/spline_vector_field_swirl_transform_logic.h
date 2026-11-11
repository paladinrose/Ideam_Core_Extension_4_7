#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../tasks/i_native_task.h"
#include "../../tasks/task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * SPL11: SplineVectorFieldSwirlTransformLogic
 * Evaluates a regularized Biot-Savart / Rankine vortex filament velocity field (tangential swirl,
 * radial suction, and axial flow) around a 3D spline polyline/frame buffer from SplineDOD.
 */
struct SplineVectorFieldSwirlTransformLogic {
    static constexpr std::string_view ui_name = "Spline Biot-Savart Vector Field Swirl";
    static constexpr std::string_view ui_category = "Splines/Fields";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::AOSOA,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 1> supported_types = {
        DataType::VECTOR3
    };

    float swirl_strength = 5.0f;
    float radial_suction = 1.5f;
    float axial_flow = 2.0f;
    float core_radius = 1.0f;
    bool integrate_positions = false;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("swirl_strength", godot::Variant::FLOAT, swirl_strength);
        add_prop("radial_suction", godot::Variant::FLOAT, radial_suction);
        add_prop("axial_flow", godot::Variant::FLOAT, axial_flow);
        add_prop("core_radius", godot::Variant::FLOAT, core_radius);
        add_prop("integrate_positions", godot::Variant::BOOL, integrate_positions);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("swirl_strength")) swirl_strength = static_cast<float>(p_props["swirl_strength"]);
        if (p_props.has("radial_suction")) radial_suction = static_cast<float>(p_props["radial_suction"]);
        if (p_props.has("axial_flow")) axial_flow = static_cast<float>(p_props["axial_flow"]);
        if (p_props.has("core_radius")) core_radius = std::max(1e-4f, static_cast<float>(p_props["core_radius"]));
        if (p_props.has("integrate_positions")) integrate_positions = static_cast<bool>(p_props["integrate_positions"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return 0;
    }

    size_t get_transient_requirement() const noexcept {
        return 0;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& pos_part = p_ctx.grant->parts[0];
        const GrantPartPOD& spline_part = p_ctx.grant->parts[1];
        if (!pos_part.raw_base_ptr || !spline_part.raw_base_ptr) return;

        GrantPartPOD* vel_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : &pos_part;

        const uint32_t s_stride = std::max<uint32_t>(sizeof(godot::Vector3), spline_part.element_stride);
        const int32_t s_count = (spline_part.selection.element_count > 0)
            ? static_cast<int32_t>(spline_part.selection.element_count)
            : static_cast<int32_t>(spline_part.capacity_bytes / s_stride);
        if (s_count < 2) return;

        auto spline_pt = [&](int32_t idx) noexcept -> godot::Vector3 {
            return *reinterpret_cast<const godot::Vector3*>(spline_part.raw_base_ptr + static_cast<size_t>(idx) * s_stride);
        };

        const uint32_t p_stride = std::max<uint32_t>(sizeof(godot::Vector3), pos_part.element_stride);
        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vel_part->element_stride);
        const int64_t n_elems = (pos_part.selection.element_count > 0)
            ? pos_part.selection.element_count
            : static_cast<int64_t>(pos_part.capacity_bytes / p_stride);

        const float rc2 = core_radius * core_radius;
        const float dt = (p_ctx.delta > 0.0) ? static_cast<float>(p_ctx.delta) : (1.0f / 60.0f);

        for (int64_t i = 0; i < n_elems; ++i) {
            godot::Vector3& pos = *reinterpret_cast<godot::Vector3*>(pos_part.raw_base_ptr + static_cast<size_t>(i) * p_stride);

            // Closest point projection onto the 3D spline polyline
            float best_d2 = 1e30f;
            godot::Vector3 closest_p = spline_pt(0);
            godot::Vector3 closest_t(0.0f, 0.0f, 1.0f);

            for (int32_t k = 0; k < s_count - 1; ++k) {
                const godot::Vector3 a = spline_pt(k);
                const godot::Vector3 b = spline_pt(k + 1);
                const godot::Vector3 ab = b - a;
                const float ab2 = ab.length_squared();
                const float u = (ab2 > 1e-10f) ? std::clamp((pos - a).dot(ab) / ab2, 0.0f, 1.0f) : 0.0f;
                const godot::Vector3 q = a + ab * u;
                const float d2 = (pos - q).length_squared();
                if (d2 < best_d2) {
                    best_d2 = d2;
                    closest_p = q;
                    closest_t = (ab2 > 1e-10f) ? (ab / std::sqrt(ab2)) : godot::Vector3(0.0f, 0.0f, 1.0f);
                }
            }

            const godot::Vector3 radial_vec = pos - closest_p;
            const float r = std::sqrt(best_d2);
            const godot::Vector3 radial_dir = (r > 1e-6f) ? (radial_vec / r) : godot::Vector3(0.0f, 0.0f, 0.0f);
            const godot::Vector3 swirl_dir = closest_t.cross(radial_dir);

            // Rankine-regularized kernel: r / (r^2 + r_c^2) for swirl, r_c^2 / (r^2 + r_c^2) for core axial transport
            const float inv_denom = 1.0f / (best_d2 + rc2);
            const float swirl_kernel = r * inv_denom;
            const float core_kernel = rc2 * inv_denom;

            const godot::Vector3 field_vel =
                swirl_dir * (swirl_strength * swirl_kernel) -
                radial_dir * (radial_suction * swirl_kernel) +
                closest_t * (axial_flow * core_kernel);

            if (integrate_positions && vel_part == &pos_part) {
                pos += field_vel * dt;
            } else if (static_cast<size_t>(i + 1) * v_stride <= vel_part->capacity_bytes) {
                godot::Vector3& out_v = *reinterpret_cast<godot::Vector3*>(vel_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);
                if (vel_part == &pos_part) {
                    out_v = field_vel;
                } else {
                    out_v += field_vel * dt;
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