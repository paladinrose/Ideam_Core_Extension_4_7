#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

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

enum class LaplacianWeightingMode : uint8_t {
    COTANGENT_BELTRAMI = 0,
    UNIFORM_UMBRELLA   = 1
};

/**
 * MeshLaplacianCurvatureTransformLogic<T> (M30)
 * Computes the 1-ring cotangent or uniform graph-Laplacian over indexed 3D triangle meshes
 * in transient workspace, performing volume-preserving Taubin (lambda, mu) smoothing and
 * extracting discrete Mean (H) and Gaussian (K) surface curvatures.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshLaplacianCurvatureTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = StencilView<T, DefaultStrategy>;

    static constexpr int32_t MAX_LAPLACIAN_VERTS = 16384;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::VECTOR2 | DataType::FLOAT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_LAPLACIAN_VERTS * (sizeof(godot::Vector3) + sizeof(float) * 3) + 256;

    static constexpr std::string_view display_name = "Mesh Cotangent Laplacian & Curvature";
    static constexpr std::string_view ui_category  = "Model/SurfacePDE";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id    = INVALID_ID; // 4B: Primary target vertex buffer (Vector3)
    uint32_t index_buffer_id     = INVALID_ID; // 4B: Triangle index buffer (int32_t triplets)
    uint32_t curvature_buffer_id = INVALID_ID; // 4B: Optional output Vector2(H, K) or float(H) buffer

    float taubin_lambda                   = 0.50f;  // 4B: Positive forward Laplacian shrink step
    float taubin_mu                       = -0.53f; // 4B: Negative inflation pass step (mu < -lambda)
    int32_t iterations                    = 2;      // 4B: Number of Taubin (lambda, mu) pass pairs
    LaplacianWeightingMode weighting_mode = LaplacianWeightingMode::COTANGENT_BELTRAMI; // 1B
    bool apply_taubin_smoothing           = true;   // 1B
    uint8_t reserved_padding[38]          = {0};    // 38B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& p_context) const noexcept {
        if (const GrantPartPOD* vtx_part = p_context.get_grant_part(vertex_buffer_id)) {
            if (vtx_part->selection.capacity > 0) {
                const size_t n = std::min<size_t>(static_cast<size_t>(vtx_part->selection.capacity), MAX_LAPLACIAN_VERTS);
                return n * (sizeof(godot::Vector3) + sizeof(float) * 3) + 256;
            }
        }
        return transient_workspace_bytes;
    }

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vtx_prop;
        vtx_prop["name"] = "vertex_buffer_id";
        vtx_prop["type"] = godot::Variant::INT;
        vtx_prop["hint_string"] = "buffer_option";
        props.push_back(vtx_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary crv_prop;
        crv_prop["name"] = "curvature_buffer_id";
        crv_prop["type"] = godot::Variant::INT;
        crv_prop["hint_string"] = "buffer_option";
        props.push_back(crv_prop);

        godot::Dictionary wmode_prop;
        wmode_prop["name"] = "weighting_mode";
        wmode_prop["type"] = godot::Variant::INT;
        wmode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        wmode_prop["hint_string"] = "Cotangent (Laplace-Beltrami):0,Uniform Umbrella:1";
        props.push_back(wmode_prop);

        godot::Dictionary iter_prop;
        iter_prop["name"] = "iterations";
        iter_prop["type"] = godot::Variant::INT;
        iter_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        iter_prop["hint_string"] = "1,32,1";
        props.push_back(iter_prop);

        godot::Dictionary lam_prop;
        lam_prop["name"] = "taubin_lambda";
        lam_prop["type"] = godot::Variant::FLOAT;
        lam_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        lam_prop["hint_string"] = "0.0,1.0,0.01";
        props.push_back(lam_prop);

        godot::Dictionary mu_prop;
        mu_prop["name"] = "taubin_mu";
        mu_prop["type"] = godot::Variant::FLOAT;
        mu_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        mu_prop["hint_string"] = "-1.0,0.0,0.01";
        props.push_back(mu_prop);

        godot::Dictionary sm_prop;
        sm_prop["name"] = "apply_taubin_smoothing";
        sm_prop["type"] = godot::Variant::BOOL;
        props.push_back(sm_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (vertex_buffer_id != INVALID_ID) ? vertex_buffer_id : curvature_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && vertex_buffer_id == INVALID_ID) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("curvature_buffer_id")) {
            curvature_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["curvature_buffer_id"]));
        }
        if (p_props.has("weighting_mode")) {
            weighting_mode = static_cast<LaplacianWeightingMode>(std::clamp<int64_t>(static_cast<int64_t>(p_props["weighting_mode"]), 0, 1));
        }
        if (p_props.has("iterations")) {
            iterations = std::clamp<int32_t>(static_cast<int32_t>(p_props["iterations"]), 1, 32);
        }
        if (p_props.has("taubin_lambda")) {
            taubin_lambda = static_cast<float>(p_props["taubin_lambda"]);
        }
        if (p_props.has("taubin_mu")) {
            taubin_mu = static_cast<float>(p_props["taubin_mu"]);
        }
        if (p_props.has("apply_taubin_smoothing")) {
            apply_taubin_smoothing = static_cast<bool>(p_props["apply_taubin_smoothing"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, godot::Vector2> || std::is_same_v<T, float>) {
            if (!p_context.local_workspace) return;

            const GrantPartPOD* vtx_part = p_context.get_grant_part(vertex_buffer_id);
            const GrantPartPOD* idx_part = p_context.get_grant_part(index_buffer_id);
            if (!vtx_part || !idx_part || !vtx_part->raw_base_ptr || !idx_part->raw_base_ptr) return;

            const GrantPartPOD* curv_part = (curvature_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(curvature_buffer_id)
                : nullptr;

            godot::Vector3* __restrict verts = reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr);
            const int32_t raw_verts = (vtx_part->selection.element_count > 0)
                ? static_cast<int32_t>(vtx_part->selection.element_count)
                : static_cast<int32_t>(vtx_part->capacity_bytes / sizeof(godot::Vector3));
            const int32_t n_verts = std::min<int32_t>(raw_verts, MAX_LAPLACIAN_VERTS);

            const int32_t* __restrict indices = reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_indices = (idx_part->selection.element_count > 0)
                ? static_cast<int32_t>(idx_part->selection.element_count)
                : static_cast<int32_t>(idx_part->capacity_bytes / sizeof(int32_t));
            const int32_t n_tris = n_indices / 3;
            if (n_verts <= 0 || n_tris <= 0) return;

            uint8_t* ws = static_cast<uint8_t*>(p_context.local_workspace);
            godot::Vector3* lap_vec = reinterpret_cast<godot::Vector3*>(ws);
            float* weight_sum = reinterpret_cast<float*>(lap_vec + n_verts);
            float* mixed_area = weight_sum + n_verts;
            float* angle_sum  = mixed_area + n_verts;

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
                    if (weighting_mode == LaplacianWeightingMode::COTANGENT_BELTRAMI) {
                        w01 = std::max(0.01f, 0.5f * _cotangent_angle(v2, v0, v1));
                        w12 = std::max(0.01f, 0.5f * _cotangent_angle(v0, v1, v2));
                        w20 = std::max(0.01f, 0.5f * _cotangent_angle(v1, v2, v0));
                    }

                    lap_vec[i0] += (v1 - v0) * w01 + (v2 - v0) * w20;
                    weight_sum[i0] += w01 + w20;

                    lap_vec[i1] += (v0 - v1) * w01 + (v2 - v1) * w12;
                    weight_sum[i1] += w01 + w12;

                    lap_vec[i2] += (v1 - v2) * w12 + (v0 - v2) * w20;
                    weight_sum[i2] += w12 + w20;

                    if (record_curvature) {
                        const float tri_area_third = (v1 - v0).cross(v2 - v0).length() * (1.0f / 6.0f);
                        mixed_area[i0] += tri_area_third;
                        mixed_area[i1] += tri_area_third;
                        mixed_area[i2] += tri_area_third;

                        angle_sum[i0] += _corner_angle(v0, v1, v2);
                        angle_sum[i1] += _corner_angle(v1, v2, v0);
                        angle_sum[i2] += _corner_angle(v2, v0, v1);
                    }
                }

                if (record_curvature && curv_part && curv_part->raw_base_ptr) {
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
    }

private:
    static inline float _cotangent_angle(const godot::Vector3& p_pivot, const godot::Vector3& p_a, const godot::Vector3& p_b) noexcept {
        const godot::Vector3 u = p_a - p_pivot;
        const godot::Vector3 v = p_b - p_pivot;
        const float sin_len = u.cross(v).length();
        if (sin_len < 1e-6f) return 0.0f;
        return std::clamp(u.dot(v) / sin_len, -100.0f, 100.0f);
    }

    static inline float _corner_angle(const godot::Vector3& p_pivot, const godot::Vector3& p_a, const godot::Vector3& p_b) noexcept {
        const godot::Vector3 u = (p_a - p_pivot).normalized();
        const godot::Vector3 v = (p_b - p_pivot).normalized();
        return std::acos(std::clamp(u.dot(v), -1.0f, 1.0f));
    }
};

static_assert(sizeof(MeshLaplacianCurvatureTransformLogic<godot::Vector3>) == 64, "MeshLaplacianCurvatureTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<MeshLaplacianCurvatureTransformLogic<godot::Vector3>>, "MeshLaplacianCurvatureTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core