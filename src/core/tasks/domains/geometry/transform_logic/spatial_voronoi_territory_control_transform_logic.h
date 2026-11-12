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
 * SpatialVoronoiTerritoryControlTransformLogic<T, T_Strategy, T_View> (GEO10)
 * Computes exact clipped 2D Voronoi cell areas and centroids per agent/seed via Sutherland-Hodgman
 * half-plane bisector clipping inside a bounded domain, writing normalized control percentages
 * (or raw areas) and cell centroids for GameDOD / NarrativeDOD territory control.
 */
template <
    typename T = godot::Vector2,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SpatialVoronoiTerritoryControlTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr int32_t MAX_POLY_VERTS = 128;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR2 | DataType::VECTOR3 | DataType::FLOAT32 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_POLY_VERTS * sizeof(godot::Vector2) * 2 + 256;

    static constexpr std::string_view display_name = "Spatial Voronoi Territory Control";
    static constexpr std::string_view ui_category  = "Geometry/Voronoi";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    godot::Vector2 domain_min       = godot::Vector2(-100.0f, -100.0f); // 8B
    godot::Vector2 domain_max       = godot::Vector2(100.0f, 100.0f);   // 8B -> 16B
    uint32_t target_buffer_id       = INVALID_ID;                       // 4B
    uint32_t seed_buffer_id         = INVALID_ID;                       // 4B
    uint32_t area_out_buffer_id     = INVALID_ID;                       // 4B
    uint32_t centroid_out_buffer_id = INVALID_ID;                       // 4B -> 32B
    float max_influence_radius      = 0.0f;                             // 4B
    bool normalize_percentages      = true;                             // 1B
    bool project_xz_from_3d         = false;                            // 1B
    uint8_t reserved_padding[26]    = {0};                              // 26B -> Total = 64B

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
        add_buf("seed_buffer_id");
        add_buf("area_out_buffer_id");
        add_buf("centroid_out_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("domain_min", godot::Variant::VECTOR2, godot::Vector2(-100.0f, -100.0f));
        add_prop("domain_max", godot::Variant::VECTOR2, godot::Vector2(100.0f, 100.0f));
        add_prop("max_influence_radius", godot::Variant::FLOAT, 0.0f);
        add_prop("normalize_percentages", godot::Variant::BOOL, true);
        add_prop("project_xz_from_3d", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (area_out_buffer_id != INVALID_ID) ? area_out_buffer_id : seed_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("seed_buffer_id")) seed_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_buffer_id"]));
        if (p_props.has("area_out_buffer_id")) area_out_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["area_out_buffer_id"]));
        if (p_props.has("centroid_out_buffer_id")) centroid_out_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["centroid_out_buffer_id"]));
        if (p_props.has("domain_min")) domain_min = p_props["domain_min"];
        if (p_props.has("domain_max")) domain_max = p_props["domain_max"];
        if (p_props.has("max_influence_radius")) max_influence_radius = std::max(0.0f, static_cast<float>(p_props["max_influence_radius"]));
        if (p_props.has("normalize_percentages")) normalize_percentages = static_cast<bool>(p_props["normalize_percentages"]);
        if (p_props.has("project_xz_from_3d")) project_xz_from_3d = static_cast<bool>(p_props["project_xz_from_3d"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    static int32_t clip_polygon_half_plane(
        const godot::Vector2* in_poly, int32_t in_count,
        godot::Vector2* out_poly,
        const godot::Vector2& mid, const godot::Vector2& normal) noexcept
    {
        if (in_count < 3) return 0;
        int32_t out_count = 0;
        godot::Vector2 prev = in_poly[in_count - 1];
        float prev_d = (prev - mid).dot(normal);

        for (int32_t i = 0; i < in_count; ++i) {
            const godot::Vector2 curr = in_poly[i];
            const float curr_d = (curr - mid).dot(normal);

            if (curr_d <= 0.0f) {
                if (prev_d > 0.0f && out_count < MAX_POLY_VERTS) {
                    const float t = prev_d / std::max(1e-7f, prev_d - curr_d);
                    out_poly[out_count++] = prev + (curr - prev) * t;
                }
                if (out_count < MAX_POLY_VERTS) {
                    out_poly[out_count++] = curr;
                }
            } else if (prev_d <= 0.0f && out_count < MAX_POLY_VERTS) {
                const float t = prev_d / std::min(-1e-7f, prev_d - curr_d);
                out_poly[out_count++] = prev + (curr - prev) * t;
            }

            prev = curr;
            prev_d = curr_d;
        }
        return out_count;
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

            const GrantPartPOD* seed_part = (seed_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(seed_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            GrantPartPOD* area_part = (area_out_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(area_out_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!seed_part || !seed_part->raw_base_ptr || !area_part || !area_part->raw_base_ptr) return;

            GrantPartPOD* centroid_part = (centroid_out_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(centroid_out_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);

            const uint32_t seed_stride = std::max<uint32_t>(sizeof(godot::Vector2), seed_part->element_stride);
            const int32_t n_seeds = (seed_part->selection.element_count > 0)
                ? static_cast<int32_t>(seed_part->selection.element_count)
                : static_cast<int32_t>(seed_part->capacity_bytes / seed_stride);
            if (n_seeds <= 0) return;

            auto read_seed = [&](int32_t idx) noexcept -> godot::Vector2 {
                const float* fp = reinterpret_cast<const float*>(seed_part->raw_base_ptr + static_cast<size_t>(idx) * seed_stride);
                if (project_xz_from_3d && seed_stride >= sizeof(godot::Vector3)) {
                    return godot::Vector2(fp[0], fp[2]);
                }
                return godot::Vector2(fp[0], fp[1]);
            };

            uint8_t* ws = static_cast<uint8_t*>(p_ctx.local_workspace);
            godot::Vector2* poly_a = reinterpret_cast<godot::Vector2*>(ws);
            godot::Vector2* poly_b = poly_a + MAX_POLY_VERTS;

            float* out_areas = reinterpret_cast<float*>(area_part->raw_base_ptr);
            const size_t max_areas = area_part->capacity_bytes / sizeof(float);
            godot::Vector2* out_centroids = (centroid_part && centroid_part->raw_base_ptr)
                ? reinterpret_cast<godot::Vector2*>(centroid_part->raw_base_ptr)
                : nullptr;
            const size_t max_centroids = out_centroids ? (centroid_part->capacity_bytes / sizeof(godot::Vector2)) : 0;

            const float domain_w = std::max(1e-4f, domain_max.x - domain_min.x);
            const float domain_h = std::max(1e-4f, domain_max.y - domain_min.y);
            const float total_domain_area = domain_w * domain_h;

            for (int32_t i = 0; i < n_seeds; ++i) {
                const godot::Vector2 si = read_seed(i);

                int32_t v_count = 4;
                poly_a[0] = godot::Vector2(domain_min.x, domain_min.y);
                poly_a[1] = godot::Vector2(domain_max.x, domain_min.y);
                poly_a[2] = godot::Vector2(domain_max.x, domain_max.y);
                poly_a[3] = godot::Vector2(domain_min.x, domain_max.y);

                if (max_influence_radius > 0.0f) {
                    constexpr int32_t N_RAD = 16;
                    constexpr float TWO_PI = 6.283185307179586f;
                    for (int32_t k = 0; k < N_RAD && v_count >= 3; ++k) {
                        const float ang = TWO_PI * static_cast<float>(k) / static_cast<float>(N_RAD);
                        const godot::Vector2 nrm(std::cos(ang), std::sin(ang));
                        const godot::Vector2 boundary_pt = si + nrm * max_influence_radius;
                        v_count = clip_polygon_half_plane(poly_a, v_count, poly_b, boundary_pt, nrm);
                        std::swap(poly_a, poly_b);
                    }
                }

                for (int32_t j = 0; j < n_seeds && v_count >= 3; ++j) {
                    if (i == j) continue;
                    const godot::Vector2 sj = read_seed(j);
                    const godot::Vector2 diff = sj - si;
                    const float len2 = diff.length_squared();
                    if (len2 < 1e-10f) continue;

                    if (max_influence_radius > 0.0f && len2 > (4.0f * max_influence_radius * max_influence_radius)) {
                        continue;
                    }

                    const godot::Vector2 mid = (si + sj) * 0.5f;
                    const godot::Vector2 normal = diff / std::sqrt(len2);
                    v_count = clip_polygon_half_plane(poly_a, v_count, poly_b, mid, normal);
                    std::swap(poly_a, poly_b);
                }

                float signed_area2 = 0.0f;
                godot::Vector2 centroid = si;
                if (v_count >= 3) {
                    float cx = 0.0f, cy = 0.0f;
                    for (int32_t k = 0; k < v_count; ++k) {
                        const godot::Vector2& p0 = poly_a[k];
                        const godot::Vector2& p1 = poly_a[(k + 1) % v_count];
                        const float cross = p0.x * p1.y - p1.x * p0.y;
                        signed_area2 += cross;
                        cx += (p0.x + p1.x) * cross;
                        cy += (p0.y + p1.y) * cross;
                    }
                    if (std::abs(signed_area2) > 1e-7f) {
                        centroid = godot::Vector2(cx / (3.0f * signed_area2), cy / (3.0f * signed_area2));
                    }
                }

                const float cell_area = 0.5f * std::abs(signed_area2);
                if (static_cast<size_t>(i) < max_areas) {
                    out_areas[i] = normalize_percentages ? (cell_area / total_domain_area) : cell_area;
                }
                if (out_centroids && static_cast<size_t>(i) < max_centroids) {
                    out_centroids[i] = centroid;
                }
            }
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(SpatialVoronoiTerritoryControlTransformLogic<>) == 64, "SpatialVoronoiTerritoryControlTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<SpatialVoronoiTerritoryControlTransformLogic<>>, "SpatialVoronoiTerritoryControlTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core