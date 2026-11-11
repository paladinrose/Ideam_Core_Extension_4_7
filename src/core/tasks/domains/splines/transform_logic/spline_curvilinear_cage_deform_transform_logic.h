#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../tasks/i_native_task.h"
#include "../../tasks/task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * SPL8: SplineCurvilinearCageDeformTransformLogic
 * Deforms 2D/3D vertex or tile buffers inside the curvilinear (s, n, b) ribbon coordinate space
 * between a rest-pose reference spline and a deformed target spline (or directly projects/reconstructs
 * curvilinear coordinates).
 */
struct SplineCurvilinearCageDeformTransformLogic {
    static constexpr std::string_view ui_name = "Spline Curvilinear Ribbon Cage Deform";
    static constexpr std::string_view ui_category = "Splines/Deform";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::SINGLE_ELEMENT,
        MemoryView::AOSOA
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::VECTOR2
    };

    float ribbon_half_width = 5.0f;
    float falloff_exponent = 2.0f;
    int32_t encode_mode = 0; // 0 = Rest -> Target Cage Deform, 1 = World -> (s,n,b), 2 = (s,n,b) -> World

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
        add_prop("ribbon_half_width", godot::Variant::FLOAT, ribbon_half_width);
        add_prop("falloff_exponent", godot::Variant::FLOAT, falloff_exponent);
        add_prop("encode_mode", godot::Variant::INT, encode_mode, 2, "Rest -> Target Cage Deform,Project World -> (s,n,b),Reconstruct (s,n,b) -> World");
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("ribbon_half_width")) ribbon_half_width = std::max(1e-4f, static_cast<float>(p_props["ribbon_half_width"]));
        if (p_props.has("falloff_exponent")) falloff_exponent = std::max(0.1f, static_cast<float>(p_props["falloff_exponent"]));
        if (p_props.has("encode_mode")) encode_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["encode_mode"]), 0, 2);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return 0;
    }

    size_t get_transient_requirement() const noexcept {
        return 0;
    }

    static void evaluate_polyline_frame(
        const GrantPartPOD& poly_part, float s_norm,
        godot::Vector3& r_pos, godot::Vector3& r_tang, godot::Vector3& r_nrm, godot::Vector3& r_bin) noexcept
    {
        const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), poly_part.element_stride);
        const int32_t count = (poly_part.selection.element_count > 0)
            ? static_cast<int32_t>(poly_part.selection.element_count)
            : static_cast<int32_t>(poly_part.capacity_bytes / stride);
        if (count < 2) {
            r_pos = godot::Vector3();
            r_tang = godot::Vector3(0.0f, 0.0f, 1.0f);
            r_nrm = godot::Vector3(1.0f, 0.0f, 0.0f);
            r_bin = godot::Vector3(0.0f, 1.0f, 0.0f);
            return;
        }

        auto pt = [&](int32_t idx) -> godot::Vector3 {
            return *reinterpret_cast<const godot::Vector3*>(poly_part.raw_base_ptr + static_cast<size_t>(idx) * stride);
        };

        const float f_idx = std::clamp(s_norm, 0.0f, 1.0f) * static_cast<float>(count - 1);
        const int32_t i0 = std::clamp<int32_t>(static_cast<int32_t>(std::floor(f_idx)), 0, count - 2);
        const float alpha = f_idx - static_cast<float>(i0);
        const godot::Vector3 p0 = pt(i0);
        const godot::Vector3 p1 = pt(i0 + 1);
        r_pos = p0.lerp(p1, alpha);
        const godot::Vector3 d = p1 - p0;
        r_tang = (d.length_squared() > 1e-10f) ? d.normalized() : godot::Vector3(0.0f, 0.0f, 1.0f);
        const godot::Vector3 up = (std::abs(r_tang.y) < 0.99f) ? godot::Vector3(0.0f, 1.0f, 0.0f) : godot::Vector3(1.0f, 0.0f, 0.0f);
        r_nrm = r_tang.cross(up).normalized();
        r_bin = r_tang.cross(r_nrm).normalized();
    }

    static float project_closest_s(const GrantPartPOD& poly_part, const godot::Vector3& p) noexcept {
        const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), poly_part.element_stride);
        const int32_t count = (poly_part.selection.element_count > 0)
            ? static_cast<int32_t>(poly_part.selection.element_count)
            : static_cast<int32_t>(poly_part.capacity_bytes / stride);
        if (count < 2) return 0.0f;

        auto pt = [&](int32_t idx) -> godot::Vector3 {
            return *reinterpret_cast<const godot::Vector3*>(poly_part.raw_base_ptr + static_cast<size_t>(idx) * stride);
        };

        float best_d2 = 1e30f;
        float best_s = 0.0f;
        for (int32_t i = 0; i < count - 1; ++i) {
            const godot::Vector3 a = pt(i);
            const godot::Vector3 b = pt(i + 1);
            const godot::Vector3 ab = b - a;
            const float ab2 = ab.length_squared();
            const float t = (ab2 > 1e-10f) ? std::clamp((p - a).dot(ab) / ab2, 0.0f, 1.0f) : 0.0f;
            const godot::Vector3 proj = a + ab * t;
            const float d2 = (p - proj).length_squared();
            if (d2 < best_d2) {
                best_d2 = d2;
                best_s = (static_cast<float>(i) + t) / static_cast<float>(count - 1);
            }
        }
        return best_s;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        const GrantPartPOD& ref_spline = p_ctx.grant->parts[1];
        const GrantPartPOD& tgt_spline = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? p_ctx.grant->parts[2]
            : ref_spline;
        if (!vtx_part.raw_base_ptr || !ref_spline.raw_base_ptr) return;

        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part.element_stride);
        const int64_t n_verts = (vtx_part.selection.element_count > 0)
            ? vtx_part.selection.element_count
            : static_cast<int64_t>(vtx_part.capacity_bytes / v_stride);

        for (int64_t i = 0; i < n_verts; ++i) {
            godot::Vector3& v = *reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i) * v_stride);

            if (encode_mode == 2) {
                // Reconstruct (s, n, b) -> World on target spline
                godot::Vector3 pos, tang, nrm, bin;
                evaluate_polyline_frame(tgt_spline, v.x, pos, tang, nrm, bin);
                v = pos + nrm * v.y + bin * v.z;
                continue;
            }

            const float s = project_closest_s(ref_spline, v);
            godot::Vector3 ref_pos, ref_tang, ref_nrm, ref_bin;
            evaluate_polyline_frame(ref_spline, s, ref_pos, ref_tang, ref_nrm, ref_bin);

            const godot::Vector3 rel = v - ref_pos;
            const float coord_n = rel.dot(ref_nrm);
            const float coord_b = rel.dot(ref_bin);
            const float coord_t = rel.dot(ref_tang);

            if (encode_mode == 1) {
                v = godot::Vector3(s, coord_n, coord_b);
                continue;
            }

            // Mode 0: Cage deform with ribbon width falloff
            const float radial_dist = std::sqrt(coord_n * coord_n + coord_b * coord_b);
            if (radial_dist >= ribbon_half_width) continue;

            const float weight = std::pow(std::clamp(1.0f - (radial_dist / ribbon_half_width), 0.0f, 1.0f), falloff_exponent);
            godot::Vector3 tgt_pos, tgt_tang, tgt_nrm, tgt_bin;
            evaluate_polyline_frame(tgt_spline, s, tgt_pos, tgt_tang, tgt_nrm, tgt_bin);

            const godot::Vector3 deformed = tgt_pos + tgt_nrm * coord_n + tgt_bin * coord_b + tgt_tang * coord_t;
            v = v.lerp(deformed, weight);
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core