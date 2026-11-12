#pragma once

#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/memory_manager_dod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * SplineCurvilinearCageDeformTransformLogic<T, T_Strategy, T_View> (SPL8)
 * Deforms 2D/3D vertex or tile buffers inside the curvilinear (s, n, b) ribbon coordinate space
 * between a rest-pose reference spline and a deformed target spline (or directly projects/reconstructs
 * curvilinear coordinates).
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SplineCurvilinearCageDeformTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR2;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Spline Curvilinear Ribbon Cage Deform";
    static constexpr std::string_view ui_category  = "Splines/Deform";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID; // 4B
    uint32_t vertex_buffer_id           = INVALID_ID; // 4B
    uint32_t reference_spline_buffer_id = INVALID_ID; // 4B
    uint32_t deformed_spline_buffer_id  = INVALID_ID; // 4B -> 16B
    float ribbon_half_width             = 5.0f;       // 4B
    float falloff_exponent              = 2.0f;       // 4B
    int32_t encode_mode                 = 0;          // 4B: 0 = Rest->Target Deform, 1 = World->(s,n,b), 2 = (s,n,b)->World
    uint8_t reserved_padding[36]        = {0};        // 36B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char* p_name) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = godot::Variant::INT;
            d["hint_string"] = "buffer_option";
            props.push_back(d);
        };
        add_buf("target_buffer_id");
        add_buf("vertex_buffer_id");
        add_buf("reference_spline_buffer_id");
        add_buf("deformed_spline_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def,
                            int p_hint = 0, const char* p_hint_str = "") {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            d["hint"] = p_hint;
            d["hint_string"] = p_hint_str;
            props.push_back(d);
        };
        add_prop("ribbon_half_width", godot::Variant::FLOAT, 5.0f);
        add_prop("falloff_exponent", godot::Variant::FLOAT, 2.0f);
        add_prop("encode_mode", godot::Variant::INT, 0, 2, "Rest -> Target Cage Deform,Project World -> (s,n,b),Reconstruct (s,n,b) -> World");
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("vertex_buffer_id")) vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        if (p_props.has("reference_spline_buffer_id")) reference_spline_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["reference_spline_buffer_id"]));
        if (p_props.has("deformed_spline_buffer_id")) deformed_spline_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["deformed_spline_buffer_id"]));
        if (p_props.has("ribbon_half_width")) ribbon_half_width = std::max(1e-4f, static_cast<float>(p_props["ribbon_half_width"]));
        if (p_props.has("falloff_exponent")) falloff_exponent = std::max(0.1f, static_cast<float>(p_props["falloff_exponent"]));
        if (p_props.has("encode_mode")) encode_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["encode_mode"]), 0, 2);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
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

    template <typename T_ActiveView, typename T_ActiveStrategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_ctx, T_ActiveView& p_view) const noexcept {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            GrantPartPOD* vtx_part = (vertex_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(vertex_buffer_id))
                : ((target_buffer_id != INVALID_ID)
                    ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(target_buffer_id))
                    : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr));
            const GrantPartPOD* ref_spline = (reference_spline_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(reference_spline_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!vtx_part || !vtx_part->raw_base_ptr || !ref_spline || !ref_spline->raw_base_ptr) return;

            const GrantPartPOD* tgt_spline = (deformed_spline_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(deformed_spline_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : ref_spline);
            if (!tgt_spline || !tgt_spline->raw_base_ptr) tgt_spline = ref_spline;

            const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part->element_stride);
            const int64_t n_verts = (vtx_part->selection.element_count > 0)
                ? vtx_part->selection.element_count
                : static_cast<int64_t>(vtx_part->capacity_bytes / v_stride);

            for (int64_t i = 0; i < n_verts; ++i) {
                godot::Vector3& v = *reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr + static_cast<size_t>(i) * v_stride);

                if (encode_mode == 2) {
                    godot::Vector3 pos, tang, nrm, bin;
                    evaluate_polyline_frame(*tgt_spline, v.x, pos, tang, nrm, bin);
                    v = pos + nrm * v.y + bin * v.z;
                    continue;
                }

                const float s = project_closest_s(*ref_spline, v);
                godot::Vector3 ref_pos, ref_tang, ref_nrm, ref_bin;
                evaluate_polyline_frame(*ref_spline, s, ref_pos, ref_tang, ref_nrm, ref_bin);

                const godot::Vector3 rel = v - ref_pos;
                const float coord_n = rel.dot(ref_nrm);
                const float coord_b = rel.dot(ref_bin);
                const float coord_t = rel.dot(ref_tang);

                if (encode_mode == 1) {
                    v = godot::Vector3(s, coord_n, coord_b);
                    continue;
                }

                const float radial_dist = std::sqrt(coord_n * coord_n + coord_b * coord_b);
                if (radial_dist >= ribbon_half_width) continue;

                const float weight = std::pow(std::clamp(1.0f - (radial_dist / ribbon_half_width), 0.0f, 1.0f), falloff_exponent);
                godot::Vector3 tgt_pos, tgt_tang, tgt_nrm, tgt_bin;
                evaluate_polyline_frame(*tgt_spline, s, tgt_pos, tgt_tang, tgt_nrm, tgt_bin);

                const godot::Vector3 deformed = tgt_pos + tgt_nrm * coord_n + tgt_bin * coord_b + tgt_tang * coord_t;
                v = v.lerp(deformed, weight);
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(SplineCurvilinearCageDeformTransformLogic<>) == 64, "SplineCurvilinearCageDeformTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SplineCurvilinearCageDeformTransformLogic<>>, "SplineCurvilinearCageDeformTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core