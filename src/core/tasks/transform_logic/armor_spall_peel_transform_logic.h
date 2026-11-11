#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

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
 * M25: ArmorSpallPeelTransformLogic
 * Curls outer surface vertices and normals outward around an impact origin via Rodrigues rotation,
 * creating peeled petal / spall deformations along armor seams.
 */
struct ArmorSpallPeelTransformLogic {
    static constexpr std::string_view ui_name = "Armor Spall & Petal Peel";
    static constexpr std::string_view ui_category = "Model/Deform";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SWAP,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 1> supported_types = {
        DataType::VECTOR3
    };

    godot::Vector3 impact_origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 impact_normal = godot::Vector3(0.0f, 1.0f, 0.0f);
    float peel_radius = 1.5f;
    float max_curl_angle_radians = 1.35f;
    float outward_lift = 0.25f;
    int32_t petal_lobes = 6;
    float lobe_modulation = 0.35f;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("impact_origin", godot::Variant::VECTOR3, impact_origin);
        add_prop("impact_normal", godot::Variant::VECTOR3, impact_normal);
        add_prop("peel_radius", godot::Variant::FLOAT, peel_radius);
        add_prop("max_curl_angle_radians", godot::Variant::FLOAT, max_curl_angle_radians);
        add_prop("outward_lift", godot::Variant::FLOAT, outward_lift);
        add_prop("petal_lobes", godot::Variant::INT, petal_lobes);
        add_prop("lobe_modulation", godot::Variant::FLOAT, lobe_modulation);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("impact_origin")) impact_origin = p_props["impact_origin"];
        if (p_props.has("impact_normal")) {
            godot::Vector3 n = p_props["impact_normal"];
            impact_normal = (n.length_squared() > 1e-8f) ? n.normalized() : godot::Vector3(0.0f, 1.0f, 0.0f);
        }
        if (p_props.has("peel_radius")) peel_radius = std::max(1e-4f, static_cast<float>(p_props["peel_radius"]));
        if (p_props.has("max_curl_angle_radians")) max_curl_angle_radians = static_cast<float>(p_props["max_curl_angle_radians"]);
        if (p_props.has("outward_lift")) outward_lift = static_cast<float>(p_props["outward_lift"]);
        if (p_props.has("petal_lobes")) petal_lobes = std::max<int32_t>(0, static_cast<int32_t>(p_props["petal_lobes"]));
        if (p_props.has("lobe_modulation")) lobe_modulation = std::clamp(static_cast<float>(p_props["lobe_modulation"]), 0.0f, 1.0f);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline godot::Vector3 rodrigues_rotate(const godot::Vector3& v, const godot::Vector3& axis, float theta) noexcept {
        const float c = std::cos(theta);
        const float s = std::sin(theta);
        return v * c + axis.cross(v) * s + axis * (axis.dot(v) * (1.0f - c));
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        if (!vtx_part.raw_base_ptr) return;

        GrantPartPOD* nrm_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part.element_stride);
        const int64_t n_verts = (vtx_part.selection.element_count > 0)
            ? vtx_part.selection.element_count
            : static_cast<int64_t>(vtx_part.capacity_bytes / v_stride);

        const godot::Vector3 n_dir = (impact_normal.length_squared() > 1e-8f)
            ? impact_normal.normalized()
            : godot::Vector3(0.0f, 1.0f, 0.0f);
        const godot::Vector3 ref_up = (std::abs(n_dir.y) < 0.99f)
            ? godot::Vector3(0.0f, 1.0f, 0.0f)
            : godot::Vector3(1.0f, 0.0f, 0.0f);
        const godot::Vector3 tangent_u = n_dir.cross(ref_up).normalized();
        const godot::Vector3 tangent_v = n_dir.cross(tangent_u).normalized();

        for (int64_t i = 0; i < n_verts; ++i) {
            godot::Vector3& v = *reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i) * v_stride);
            const godot::Vector3 rel = v - impact_origin;
            const float dist = rel.length();
            if (dist <= 1e-5f || dist >= peel_radius) continue;

            // Project onto tangent plane of impact
            const float axial = rel.dot(n_dir);
            const godot::Vector3 radial_vec = rel - n_dir * axial;
            const float radial_len = radial_vec.length();
            if (radial_len <= 1e-5f) continue;

            const godot::Vector3 radial_dir = radial_vec / radial_len;
            const godot::Vector3 hinge_axis = n_dir.cross(radial_dir).normalized();

            // Normalized radial coordinate u in [0, 1] (0 = impact center, 1 = outer hinge rim)
            const float u = std::clamp(radial_len / peel_radius, 0.0f, 1.0f);
            float lobe_factor = 1.0f;
            if (petal_lobes > 0) {
                const float ang = std::atan2(radial_dir.dot(tangent_v), radial_dir.dot(tangent_u));
                lobe_factor = 1.0f - lobe_modulation * (0.5f - 0.5f * std::cos(ang * static_cast<float>(petal_lobes)));
            }

            const float weight = (1.0f - u) * (1.0f - u) * lobe_factor;
            const float theta = max_curl_angle_radians * weight;

            // Pivot around the outer rim hinge point on the impact tangent plane
            const godot::Vector3 hinge_pivot = impact_origin + radial_dir * peel_radius + n_dir * axial;
            const godot::Vector3 arm = v - hinge_pivot;
            v = hinge_pivot + rodrigues_rotate(arm, hinge_axis, theta) + n_dir * (outward_lift * weight);

            if (nrm_part) {
                const uint32_t n_stride = std::max<uint32_t>(sizeof(godot::Vector3), nrm_part->element_stride);
                if (static_cast<size_t>(i + 1) * n_stride <= nrm_part->capacity_bytes) {
                    godot::Vector3& nrm = *reinterpret_cast<godot::Vector3*>(nrm_part->raw_base_ptr + static_cast<size_t>(i) * n_stride);
                    nrm = rodrigues_rotate(nrm, hinge_axis, theta).normalized();
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