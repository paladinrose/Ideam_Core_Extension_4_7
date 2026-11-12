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
 * ConvexHullAndOBBFitTransformLogic<T, T_Strategy, T_View> (GEO11)
 * Computes 2D Monotone-Chain Convex Hulls + Rotating Calipers minimum-area OBBs, or
 * 3D PCA Jacobi Eigendecomposition + 2D orthogonal Rotating Calipers minimum-volume OBBs
 * and extremal hull vertices for procedural debris and Jolt collision shapes.
 */
template <
    typename T = godot::Vector3,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) ConvexHullAndOBBFitTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr int32_t MAX_HULL_WORK_PTS = 4096;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR3 | DataType::VECTOR2 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_HULL_WORK_PTS * sizeof(godot::Vector2) * 2 + 256;

    static constexpr std::string_view display_name = "Convex Hull & Minimum OBB Fit";
    static constexpr std::string_view ui_category  = "Geometry/Bounds";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id       = INVALID_ID; // 4B
    uint32_t input_vertex_buffer_id = INVALID_ID; // 4B
    uint32_t hull_out_buffer_id     = INVALID_ID; // 4B
    uint32_t obb_out_buffer_id      = INVALID_ID; // 4B -> 16B
    int32_t dimension_mode          = 0;          // 4B: 0 = 3D OBB + Extremal Hull, 1 = 2D Convex Hull + OBB
    float hull_margin               = 0.0f;       // 4B
    uint8_t reserved_padding[40]    = {0};        // 40B -> Total = 64B

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
        add_buf("input_vertex_buffer_id");
        add_buf("hull_out_buffer_id");
        add_buf("obb_out_buffer_id");

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
        add_prop("dimension_mode", godot::Variant::INT, 0, 2, "3D (PCA Jacobi + Calipers OBB),2D (Monotone Chain + Calipers OBB)");
        add_prop("hull_margin", godot::Variant::FLOAT, 0.0f);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (hull_out_buffer_id != INVALID_ID) ? hull_out_buffer_id : input_vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("input_vertex_buffer_id")) input_vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["input_vertex_buffer_id"]));
        if (p_props.has("hull_out_buffer_id")) hull_out_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["hull_out_buffer_id"]));
        if (p_props.has("obb_out_buffer_id")) obb_out_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["obb_out_buffer_id"]));
        if (p_props.has("dimension_mode")) dimension_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["dimension_mode"]), 0, 1);
        if (p_props.has("hull_margin")) hull_margin = std::max(0.0f, static_cast<float>(p_props["hull_margin"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    static inline float cross_2d(const godot::Vector2& o, const godot::Vector2& a, const godot::Vector2& b) noexcept {
        return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
    }

    static int32_t build_convex_hull_2d(godot::Vector2* pts, int32_t n, godot::Vector2* hull) noexcept {
        if (n <= 0) return 0;
        std::sort(pts, pts + n, [](const godot::Vector2& a, const godot::Vector2& b) {
            return (a.x < b.x) || (a.x == b.x && a.y < b.y);
        });
        if (n <= 2) {
            for (int32_t i = 0; i < n; ++i) hull[i] = pts[i];
            return n;
        }
        int32_t k = 0;
        for (int32_t i = 0; i < n; ++i) {
            while (k >= 2 && cross_2d(hull[k - 2], hull[k - 1], pts[i]) <= 0.0f) --k;
            hull[k++] = pts[i];
        }
        for (int32_t i = n - 2, t = k + 1; i >= 0; --i) {
            while (k >= t && cross_2d(hull[k - 2], hull[k - 1], pts[i]) <= 0.0f) --k;
            hull[k++] = pts[i];
        }
        return k - 1;
    }

    static void fit_min_obb_2d(
        const godot::Vector2* hull, int32_t h_count,
        godot::Vector2& r_center, godot::Vector2& r_axis_u, godot::Vector2& r_axis_v, godot::Vector2& r_half_extents) noexcept
    {
        if (h_count <= 0) {
            r_center = godot::Vector2();
            r_axis_u = godot::Vector2(1.0f, 0.0f);
            r_axis_v = godot::Vector2(0.0f, 1.0f);
            r_half_extents = godot::Vector2(0.5f, 0.5f);
            return;
        }
        if (h_count == 1) {
            r_center = hull[0];
            r_axis_u = godot::Vector2(1.0f, 0.0f);
            r_axis_v = godot::Vector2(0.0f, 1.0f);
            r_half_extents = godot::Vector2(0.0f, 0.0f);
            return;
        }

        float best_area = 1e30f;
        r_axis_u = godot::Vector2(1.0f, 0.0f);
        r_axis_v = godot::Vector2(0.0f, 1.0f);

        for (int32_t i = 0; i < h_count; ++i) {
            const godot::Vector2 edge = hull[(i + 1) % h_count] - hull[i];
            const float len = edge.length();
            if (len < 1e-6f) continue;
            const godot::Vector2 u = edge / len;
            const godot::Vector2 v(-u.y, u.x);

            float min_u = 1e30f, max_u = -1e30f;
            float min_v = 1e30f, max_v = -1e30f;
            for (int32_t j = 0; j < h_count; ++j) {
                const float pu = hull[j].dot(u);
                const float pv = hull[j].dot(v);
                min_u = std::min(min_u, pu);
                max_u = std::max(max_u, pu);
                min_v = std::min(min_v, pv);
                max_v = std::max(max_v, pv);
            }

            const float area = (max_u - min_u) * (max_v - min_v);
            if (area < best_area) {
                best_area = area;
                r_axis_u = u;
                r_axis_v = v;
                r_half_extents = godot::Vector2((max_u - min_u) * 0.5f, (max_v - min_v) * 0.5f);
                const float cu = (min_u + max_u) * 0.5f;
                const float cv = (min_v + max_v) * 0.5f;
                r_center = u * cu + v * cv;
            }
        }
    }

    static void jacobi_eigen_3x3(float A[3][3], float V[3][3]) noexcept {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) V[i][j] = (i == j) ? 1.0f : 0.0f;
        }
        for (int iter = 0; iter < 16; ++iter) {
            int p = 0, q = 1;
            float max_off = std::abs(A[0][1]);
            if (std::abs(A[0][2]) > max_off) { p = 0; q = 2; max_off = std::abs(A[0][2]); }
            if (std::abs(A[1][2]) > max_off) { p = 1; q = 2; max_off = std::abs(A[1][2]); }
            if (max_off < 1e-7f) break;

            const float app = A[p][p], aqq = A[q][q], apq = A[p][q];
            const float tau = (aqq - app) / (2.0f * apq);
            const float t = (tau >= 0.0f)
                ? (1.0f / (tau + std::sqrt(1.0f + tau * tau)))
                : (-1.0f / (-tau + std::sqrt(1.0f + tau * tau)));
            const float c = 1.0f / std::sqrt(1.0f + t * t);
            const float s = t * c;

            A[p][p] = app - t * apq;
            A[q][q] = aqq + t * apq;
            A[p][q] = A[q][p] = 0.0f;

            for (int r = 0; r < 3; ++r) {
                if (r != p && r != q) {
                    const float arp = A[r][p];
                    const float arq = A[r][q];
                    A[r][p] = A[p][r] = c * arp - s * arq;
                    A[r][q] = A[q][r] = s * arp + c * arq;
                }
                const float vrp = V[r][p];
                const float vrq = V[r][q];
                V[r][p] = c * vrp - s * vrq;
                V[r][q] = s * vrp + c * vrq;
            }
        }
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
            if (!p_ctx.local_workspace) return;

            const GrantPartPOD* in_part = (input_vertex_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(input_vertex_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            GrantPartPOD* out_part = (hull_out_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(hull_out_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!in_part || !in_part->raw_base_ptr || !out_part || !out_part->raw_base_ptr) return;

            GrantPartPOD* obb_part = (obb_out_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(obb_out_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);

            uint8_t* ws = static_cast<uint8_t*>(p_ctx.local_workspace);
            godot::Vector2* pts_2d = reinterpret_cast<godot::Vector2*>(ws);
            godot::Vector2* hull_2d = pts_2d + MAX_HULL_WORK_PTS;

            if (dimension_mode == 1) {
                const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector2), in_part->element_stride);
                const int32_t raw_n = (in_part->selection.element_count > 0)
                    ? static_cast<int32_t>(in_part->selection.element_count)
                    : static_cast<int32_t>(in_part->capacity_bytes / stride);
                const int32_t n = std::min<int32_t>(raw_n, MAX_HULL_WORK_PTS);
                if (n <= 0) return;

                for (int32_t i = 0; i < n; ++i) {
                    const float* fp = reinterpret_cast<const float*>(in_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                    pts_2d[i] = godot::Vector2(fp[0], fp[1]);
                }

                const int32_t h_count = build_convex_hull_2d(pts_2d, n, hull_2d);
                godot::Vector2 center, axis_u, axis_v, half_ext;
                fit_min_obb_2d(hull_2d, h_count, center, axis_u, axis_v, half_ext);
                half_ext += godot::Vector2(hull_margin, hull_margin);

                godot::Vector2* out_v2 = reinterpret_cast<godot::Vector2*>(out_part->raw_base_ptr);
                const int32_t max_out = static_cast<int32_t>(out_part->capacity_bytes / sizeof(godot::Vector2));
                const int32_t write_n = std::min(h_count, max_out);
                for (int32_t i = 0; i < write_n; ++i) {
                    const godot::Vector2 dir = (hull_2d[i] - center).normalized();
                    out_v2[i] = hull_2d[i] + dir * hull_margin;
                }
                out_part->selection.element_count = write_n;

                if (obb_part && obb_part->raw_base_ptr && obb_part->capacity_bytes >= sizeof(godot::Vector2) * 4) {
                    godot::Vector2* obb_out = reinterpret_cast<godot::Vector2*>(obb_part->raw_base_ptr);
                    obb_out[0] = center;
                    obb_out[1] = axis_u;
                    obb_out[2] = axis_v;
                    obb_out[3] = half_ext;
                    obb_part->selection.element_count = 4;
                }
            } else {
                const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), in_part->element_stride);
                const int32_t n = (in_part->selection.element_count > 0)
                    ? static_cast<int32_t>(in_part->selection.element_count)
                    : static_cast<int32_t>(in_part->capacity_bytes / stride);
                if (n <= 0) return;

                godot::Vector3 mean(0.0f, 0.0f, 0.0f);
                for (int32_t i = 0; i < n; ++i) {
                    const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(in_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                    mean += p;
                }
                mean /= static_cast<float>(n);

                float C[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
                for (int32_t i = 0; i < n; ++i) {
                    const godot::Vector3 d = *reinterpret_cast<const godot::Vector3*>(in_part->raw_base_ptr + static_cast<size_t>(i) * stride) - mean;
                    C[0][0] += d.x * d.x; C[0][1] += d.x * d.y; C[0][2] += d.x * d.z;
                    C[1][1] += d.y * d.y; C[1][2] += d.y * d.z;
                    C[2][2] += d.z * d.z;
                }
                C[1][0] = C[0][1]; C[2][0] = C[0][2]; C[2][1] = C[1][2];

                float V[3][3];
                jacobi_eigen_3x3(C, V);

                int primary_col = 0;
                if (C[1][1] > C[primary_col][primary_col]) primary_col = 1;
                if (C[2][2] > C[primary_col][primary_col]) primary_col = 2;

                const int sec_col = (primary_col + 1) % 3;
                godot::Vector3 axis_x = godot::Vector3(V[0][primary_col], V[1][primary_col], V[2][primary_col]).normalized();
                godot::Vector3 temp_y = godot::Vector3(V[0][sec_col], V[1][sec_col], V[2][sec_col]).normalized();
                godot::Vector3 axis_z = axis_x.cross(temp_y).normalized();
                godot::Vector3 axis_y = axis_z.cross(axis_x).normalized();

                const int32_t proj_n = std::min<int32_t>(n, MAX_HULL_WORK_PTS);
                for (int32_t i = 0; i < proj_n; ++i) {
                    const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(in_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                    pts_2d[i] = godot::Vector2(p.dot(axis_y), p.dot(axis_z));
                }
                const int32_t h_count = build_convex_hull_2d(pts_2d, proj_n, hull_2d);
                godot::Vector2 c2d, u2d, v2d, ext2d;
                fit_min_obb_2d(hull_2d, h_count, c2d, u2d, v2d, ext2d);

                const godot::Vector3 refined_y = (axis_y * u2d.x + axis_z * u2d.y).normalized();
                const godot::Vector3 refined_z = axis_x.cross(refined_y).normalized();
                axis_y = refined_y;
                axis_z = refined_z;

                float min_x = 1e30f, max_x = -1e30f;
                float min_y = 1e30f, max_y = -1e30f;
                float min_z = 1e30f, max_z = -1e30f;
                for (int32_t i = 0; i < n; ++i) {
                    const godot::Vector3& p = *reinterpret_cast<const godot::Vector3*>(in_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                    const float px = p.dot(axis_x);
                    const float py = p.dot(axis_y);
                    const float pz = p.dot(axis_z);
                    min_x = std::min(min_x, px); max_x = std::max(max_x, px);
                    min_y = std::min(min_y, py); max_y = std::max(max_y, py);
                    min_z = std::min(min_z, pz); max_z = std::max(max_z, pz);
                }

                const godot::Vector3 obb_center =
                    axis_x * ((min_x + max_x) * 0.5f) +
                    axis_y * ((min_y + max_y) * 0.5f) +
                    axis_z * ((min_z + max_z) * 0.5f);
                const godot::Vector3 obb_half_extents(
                    (max_x - min_x) * 0.5f + hull_margin,
                    (max_y - min_y) * 0.5f + hull_margin,
                    (max_z - min_z) * 0.5f + hull_margin
                );

                godot::Vector3* out_v3 = reinterpret_cast<godot::Vector3*>(out_part->raw_base_ptr);
                const size_t max_v3 = out_part->capacity_bytes / sizeof(godot::Vector3);
                if (max_v3 >= 5) {
                    out_v3[0] = obb_center;
                    out_v3[1] = axis_x;
                    out_v3[2] = axis_y;
                    out_v3[3] = axis_z;
                    out_v3[4] = obb_half_extents;
                    out_part->selection.element_count = 5;
                }
                if (max_v3 >= 13) {
                    int idx = 5;
                    for (int sx = -1; sx <= 1; sx += 2) {
                        for (int sy = -1; sy <= 1; sy += 2) {
                            for (int sz = -1; sz <= 1; sz += 2) {
                                out_v3[idx++] = obb_center +
                                    axis_x * (obb_half_extents.x * static_cast<float>(sx)) +
                                    axis_y * (obb_half_extents.y * static_cast<float>(sy)) +
                                    axis_z * (obb_half_extents.z * static_cast<float>(sz));
                            }
                        }
                    }
                    out_part->selection.element_count = 13;
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(ConvexHullAndOBBFitTransformLogic<>) == 64, "ConvexHullAndOBBFitTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ConvexHullAndOBBFitTransformLogic<>>, "ConvexHullAndOBBFitTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core