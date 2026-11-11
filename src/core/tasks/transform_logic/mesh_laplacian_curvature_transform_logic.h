#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

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
 * M30: MeshLaplacianCurvatureTransformLogic
 * Computes the 1-ring cotangent or uniform graph-Laplacian over indexed 3D triangle meshes
 * in transient workspace, performing volume-preserving Taubin (lambda, mu) smoothing and
 * extracting discrete Mean (H) and Gaussian (K) surface curvatures.
 */
struct MeshLaplacianCurvatureTransformLogic {
    static constexpr std::string_view ui_name = "Mesh Cotangent Laplacian & Curvature";
    static constexpr std::string_view ui_category = "Model/SurfacePDE";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::STENCIL,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::VECTOR3,
        DataType::VECTOR2,
        DataType::FLOAT32
    };

    static constexpr int32_t MAX_LAPLACIAN_VERTS = 16384;

    int32_t weighting_mode = 0; // 0 = Cotangent (Discrete Laplace-Beltrami), 1 = Uniform Umbrella
    int32_t iterations = 2;
    float taubin_lambda = 0.50f;
    float taubin_mu = -0.53f;
    bool apply_taubin_smoothing = true;

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
        add_prop("weighting_mode", godot::Variant::INT, weighting_mode, 2, "Cotangent (Laplace-Beltrami),Uniform Umbrella");
        add_prop("iterations", godot::Variant::INT, iterations);
        add_prop("taubin_lambda", godot::Variant::FLOAT, taubin_lambda);
        add_prop("taubin_mu", godot::Variant::FLOAT, taubin_mu);
        add_prop("apply_taubin_smoothing", godot::Variant::BOOL, apply_taubin_smoothing);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("weighting_mode")) weighting_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["weighting_mode"]), 0, 1);
        if (p_props.has("iterations")) iterations = std::clamp<int32_t>(static_cast<int32_t>(p_props["iterations"]), 1, 32);
        if (p_props.has("taubin_lambda")) taubin_lambda = static_cast<float>(p_props["taubin_lambda"]);
        if (p_props.has("taubin_mu")) taubin_mu = static_cast<float>(p_props["taubin_mu"]);
        if (p_props.has("apply_taubin_smoothing")) apply_taubin_smoothing = static_cast<bool>(p_props["apply_taubin_smoothing"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_LAPLACIAN_VERTS * (sizeof(godot::Vector3) + sizeof(float) * 3) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_LAPLACIAN_VERTS * (sizeof(godot::Vector3) + sizeof(float) * 3) + 256;
    }

    static inline float cotangent_angle(const godot::Vector3& pivot, const godot::Vector3& a, const godot::Vector3& b) noexcept {
        const godot::Vector3 u = a - pivot;
        const godot::Vector3 v = b - pivot;
        const float sin_len = u.cross(v).length();
        if (sin_len < 1e-6f) return 0.0f;
        return std::clamp(u.dot(v) / sin_len, -100.0f, 100.0f);
    }

    static inline float corner_angle(const godot::Vector3& pivot, const godot::Vector3& a, const godot::Vector3& b) noexcept {
        const godot::Vector3 u = (a - pivot).normalized();
        const godot::Vector3 v = (b - pivot).normalized();
        return std::acos(std::clamp(u.dot(v), -1.0f, 1.0f));
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2 || !p_ctx.local_workspace) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        const GrantPartPOD& idx_part = p_ctx.grant->parts[1];
        if (!vtx_part.raw_base_ptr || !idx_part.raw_base_ptr) return;

        GrantPartPOD* curv_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr)
            ? &p_ctx.grant->parts[2]
            : nullptr;

        godot::Vector3* verts = reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr);
        const int32_t raw_verts = (vtx_part.selection.element_count > 0)
            ? static_cast<int32_t>(vtx_part.selection.element_count)
            : static_cast<int32_t>(vtx_part.capacity_bytes / sizeof(godot::Vector3));
        const int32_t n_verts = std::min<int32_t>(raw_verts, MAX_LAPLACIAN_VERTS);

        const int32_t* indices = reinterpret_cast<const int32_t*>(idx_part.raw_base_ptr);
        const int32_t n_indices = (idx_part.selection.element_count > 0)
            ? static_cast<int32_t>(idx_part.selection.element_count)
            : static_cast<int32_t>(idx_part.capacity_bytes / sizeof(int32_t));
        const int32_t n_tris = n_indices / 3;
        if (n_verts <= 0 || n_tris <= 0) return;

        uint8_t* ws = static_cast<uint8_t*>(p_ctx.local_workspace);
        godot::Vector3* lap_vec = reinterpret_cast<godot::Vector3*>(ws);
        float* weight_sum = reinterpret_cast<float*>(lap_vec + MAX_LAPLACIAN_VERTS);
        float* mixed_area = weight_sum + MAX_LAPLACIAN_VERTS;
        float* angle_sum = mixed_area + MAX_LAPLACIAN_VERTS;

        auto compute_laplacian_pass = [&](float step_scale, bool record_curvature) {
            for (int32_t i = 0; i < n_verts; ++i) {
                lap_vec[i] = godot::Vector3(0.0f, 0.0f, 0.0f);
                weight_sum[i] = 0.0f;
                if (record_curvature) {
                    mixed_area[i] = 0.0f;
                    angle_sum[i] = 0.0f;
                }
            }

            for (int32_t t = 0; t < n_tris; ++t) {
                const int32_t i0 = indices[t * 3 + 0];
                const int32_t i1 = indices[t * 3 + 1];
                const int32_t i2 = indices[t * 3 + 2];
                if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;

                const godot::Vector3& v0 = verts[i0];
                const godot::Vector3& v1 = verts[i1];
                const godot::Vector3& v2 = verts[i2];

                float w01 = 1.0f, w12 = 1.0f, w20 = 1.0f;
                if (weighting_mode == 0) {
                    w01 = std::max(0.01f, 0.5f * cotangent_angle(v2, v0, v1));
                    w12 = std::max(0.01f, 0.5f * cotangent_angle(v0, v1, v2));
                    w20 = std::max(0.01f, 0.5f * cotangent_angle(v1, v2, v0));
                }

                lap_vec[i0] += (v1 - v0) * w01 + (v2 - v0) * w20;
                weight_sum[i0] += w01 + w20;

                lap_vec[i1] += (v0 - v1) * w01 + (v2 - v1) * w12;
                weight_sum[i1] += w01 + w12;

                lap_vec[i2] += (v1 - v2) * w12 + (v0 - v2) * w20;
                weight_sum[i2] += w12 + w20;

                if (record_curvature) {
                    const float tri_area_third = (v1 - v0).cross(v2 - v0).length() / 6.0f;
                    mixed_area[i0] += tri_area_third;
                    mixed_area[i1] += tri_area_third;
                    mixed_area[i2] += tri_area_third;

                    angle_sum[i0] += corner_angle(v0, v1, v2);
                    angle_sum[i1] += corner_angle(v1, v2, v0);
                    angle_sum[i2] += corner_angle(v2, v0, v1);
                }
            }

            if (record_curvature && curv_part) {
                constexpr float TWO_PI = 6.283185307179586f;
                if (curv_part->element_stride >= sizeof(godot::Vector2)) {
                    auto* out_hk = reinterpret_cast<godot::Vector2*>(curv_part->raw_base_ptr);
                    const int32_t max_c = static_cast<int32_t>(curv_part->capacity_bytes / sizeof(godot::Vector2));
                    for (int32_t i = 0; i < std::min(n_verts, max_c); ++i) {
                        const float a = std::max(1e-6f, mixed_area[i]);
                        const float H = 0.5f * (lap_vec[i].length() / a);
                        const float K = (TWO_PI - angle_sum[i]) / a;
                        out_hk[i] = godot::Vector2(H, K);
                    }
                } else {
                    auto* out_h = reinterpret_cast<float*>(curv_part->raw_base_ptr);
                    const int32_t max_c = static_cast<int32_t>(curv_part->capacity_bytes / sizeof(float));
                    for (int32_t i = 0; i < std::min(n_verts, max_c); ++i) {
                        const float a = std::max(1e-6f, mixed_area[i]);
                        out_h[i] = 0.5f * (lap_vec[i].length() / a);
                    }
                }
            }

            if (apply_taubin_smoothing && std::abs(step_scale) > 1e-6f) {
                for (int32_t i = 0; i < n_verts; ++i) {
                    if (weight_sum[i] > 1e-6f) {
                        verts[i] += (lap_vec[i] / weight_sum[i]) * step_scale;
                    }
                }
            }
        };

        for (int32_t iter = 0; iter < iterations; ++iter) {
            const bool is_last = (iter == iterations - 1);
            compute_laplacian_pass(taubin_lambda, is_last && !apply_taubin_smoothing);
            if (apply_taubin_smoothing) {
                compute_laplacian_pass(taubin_mu, is_last);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core