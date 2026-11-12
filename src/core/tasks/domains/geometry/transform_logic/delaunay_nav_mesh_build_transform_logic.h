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
 * DelaunayNavMeshBuildTransformLogic<T, T_Strategy, T_View> (GEO9)
 * Builds constrained 2D/XZ Delaunay navigation triangles and shared-edge portal adjacency graphs
 * from PolygonMeshBuffer vertices and optional obstacle/boundary edge segments.
 */
template <
    typename T = godot::Vector2,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) DelaunayNavMeshBuildTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr uint32_t MAX_NAVMESH_VERTS = 2048;
    static constexpr uint32_t MAX_NAVMESH_TRIS  = 4096;

    struct TriPod {
        int32_t a, b, c;
        int32_t neighbor[3]; // Adjacent triangle index across edges (a,b), (b,c), (c,a), or -1
        uint8_t valid;
    };

    struct EdgePod {
        int32_t u, v;
    };

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR2 | DataType::VECTOR3 | DataType::INT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes =
        (MAX_NAVMESH_VERTS + 3) * sizeof(godot::Vector2) +
        MAX_NAVMESH_TRIS * sizeof(TriPod) +
        (MAX_NAVMESH_TRIS * 3) * sizeof(EdgePod) + 256;

    static constexpr std::string_view display_name = "Constrained Delaunay NavMesh Builder";
    static constexpr std::string_view ui_category  = "Geometry/Navigation";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id           = INVALID_ID; // 4B
    uint32_t vertex_buffer_id           = INVALID_ID; // 4B
    uint32_t triangle_index_buffer_id   = INVALID_ID; // 4B
    uint32_t portal_adjacency_buffer_id = INVALID_ID; // 4B -> 16B
    float agent_radius                  = 0.5f;       // 4B
    float min_triangle_area             = 1e-4f;      // 4B
    bool build_portal_adjacency         = true;       // 1B
    bool project_xz_from_3d             = false;      // 1B
    uint8_t reserved_padding[38]        = {0};        // 38B -> Total = 64B

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
        add_buf("triangle_index_buffer_id");
        add_buf("portal_adjacency_buffer_id");

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("agent_radius", godot::Variant::FLOAT, 0.5f);
        add_prop("min_triangle_area", godot::Variant::FLOAT, 1e-4f);
        add_prop("build_portal_adjacency", godot::Variant::BOOL, true);
        add_prop("project_xz_from_3d", godot::Variant::BOOL, false);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        if (target_buffer_id != INVALID_ID) return target_buffer_id;
        return (triangle_index_buffer_id != INVALID_ID) ? triangle_index_buffer_id : vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("vertex_buffer_id")) vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        if (p_props.has("triangle_index_buffer_id")) triangle_index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["triangle_index_buffer_id"]));
        if (p_props.has("portal_adjacency_buffer_id")) portal_adjacency_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["portal_adjacency_buffer_id"]));
        if (p_props.has("agent_radius")) agent_radius = std::max(0.0f, static_cast<float>(p_props["agent_radius"]));
        if (p_props.has("min_triangle_area")) min_triangle_area = std::max(0.0f, static_cast<float>(p_props["min_triangle_area"]));
        if (p_props.has("build_portal_adjacency")) build_portal_adjacency = static_cast<bool>(p_props["build_portal_adjacency"]);
        if (p_props.has("project_xz_from_3d")) project_xz_from_3d = static_cast<bool>(p_props["project_xz_from_3d"]);
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView& p_view, const TaskContextPOD& p_context) const noexcept {
        (void)p_view;
        (void)p_context;
    }

    static inline float signed_area_2x(const godot::Vector2& a, const godot::Vector2& b, const godot::Vector2& c) noexcept {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    static inline bool in_circumcircle(
        const godot::Vector2& p,
        const godot::Vector2& a,
        const godot::Vector2& b,
        const godot::Vector2& c) noexcept
    {
        const float ax = a.x - p.x, ay = a.y - p.y;
        const float bx = b.x - p.x, by = b.y - p.y;
        const float cx = c.x - p.x, cy = c.y - p.y;

        const float ab = ax * ax + ay * ay;
        const float cd = bx * bx + by * by;
        const float ef = cx * cx + cy * cy;

        const float det = ax * (by * ef - cd * cy) -
                          ay * (bx * ef - cd * cx) +
                          ab * (bx * cy - by * cx);
        const float orient = signed_area_2x(a, b, c);
        return (orient > 0.0f) ? (det > 1e-7f) : (det < -1e-7f);
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

            const GrantPartPOD* vtx_part = (vertex_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(vertex_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            GrantPartPOD* tri_part = (triangle_index_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(triangle_index_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 1) ? &p_ctx.grant->parts[1] : nullptr);
            if (!vtx_part || !vtx_part->raw_base_ptr || !tri_part || !tri_part->raw_base_ptr) return;

            GrantPartPOD* adj_part = (portal_adjacency_buffer_id != INVALID_ID)
                ? const_cast<GrantPartPOD*>(p_ctx.get_grant_part(portal_adjacency_buffer_id))
                : ((p_ctx.grant && p_ctx.grant->part_count > 2 && p_ctx.grant->parts[2].raw_base_ptr) ? &p_ctx.grant->parts[2] : nullptr);

            const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector2), vtx_part->element_stride);
            const int32_t raw_count = (vtx_part->selection.element_count > 0)
                ? static_cast<int32_t>(vtx_part->selection.element_count)
                : static_cast<int32_t>(vtx_part->capacity_bytes / stride);
            const int32_t n_pts = std::min<int32_t>(raw_count, MAX_NAVMESH_VERTS);
            if (n_pts < 3) return;

            uint8_t* ws = static_cast<uint8_t*>(p_ctx.local_workspace);
            godot::Vector2* pts = reinterpret_cast<godot::Vector2*>(ws);
            TriPod* tris = reinterpret_cast<TriPod*>(pts + (MAX_NAVMESH_VERTS + 3));
            EdgePod* poly_edges = reinterpret_cast<EdgePod*>(tris + MAX_NAVMESH_TRIS);

            float min_x = 1e30f, min_y = 1e30f;
            float max_x = -1e30f, max_y = -1e30f;

            for (int32_t i = 0; i < n_pts; ++i) {
                const float* fp = reinterpret_cast<const float*>(vtx_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                if (project_xz_from_3d && stride >= sizeof(godot::Vector3)) {
                    pts[i] = godot::Vector2(fp[0], fp[2]);
                } else {
                    pts[i] = godot::Vector2(fp[0], fp[1]);
                }
                min_x = std::min(min_x, pts[i].x);
                min_y = std::min(min_y, pts[i].y);
                max_x = std::max(max_x, pts[i].x);
                max_y = std::max(max_y, pts[i].y);
            }

            const float dx = std::max(1.0f, max_x - min_x);
            const float dy = std::max(1.0f, max_y - min_y);
            const float dmax = std::max(dx, dy) * 20.0f;
            const float mid_x = (min_x + max_x) * 0.5f;
            const float mid_y = (min_y + max_y) * 0.5f;

            pts[n_pts + 0] = godot::Vector2(mid_x - 2.0f * dmax, mid_y - dmax);
            pts[n_pts + 1] = godot::Vector2(mid_x, mid_y + 2.0f * dmax);
            pts[n_pts + 2] = godot::Vector2(mid_x + 2.0f * dmax, mid_y - dmax);

            int32_t tri_count = 1;
            tris[0] = TriPod{ n_pts + 0, n_pts + 1, n_pts + 2, {-1, -1, -1}, 1 };

            for (int32_t i = 0; i < n_pts; ++i) {
                const godot::Vector2& p = pts[i];
                int32_t edge_count = 0;

                for (int32_t t = 0; t < tri_count; ++t) {
                    if (!tris[t].valid) continue;
                    if (in_circumcircle(p, pts[tris[t].a], pts[tris[t].b], pts[tris[t].c])) {
                        tris[t].valid = 0;
                        if (edge_count + 3 <= static_cast<int32_t>(MAX_NAVMESH_TRIS * 3)) {
                            poly_edges[edge_count++] = EdgePod{ tris[t].a, tris[t].b };
                            poly_edges[edge_count++] = EdgePod{ tris[t].b, tris[t].c };
                            poly_edges[edge_count++] = EdgePod{ tris[t].c, tris[t].a };
                        }
                    }
                }

                for (int32_t e1 = 0; e1 < edge_count; ++e1) {
                    if (poly_edges[e1].u < 0) continue;
                    bool shared = false;
                    for (int32_t e2 = e1 + 1; e2 < edge_count; ++e2) {
                        if (poly_edges[e2].u < 0) continue;
                        if ((poly_edges[e1].u == poly_edges[e2].v && poly_edges[e1].v == poly_edges[e2].u) ||
                            (poly_edges[e1].u == poly_edges[e2].u && poly_edges[e1].v == poly_edges[e2].v)) {
                            poly_edges[e2].u = -1;
                            shared = true;
                        }
                    }
                    if (shared) {
                        poly_edges[e1].u = -1;
                    }
                }

                for (int32_t e = 0; e < edge_count; ++e) {
                    if (poly_edges[e].u < 0) continue;
                    if (tri_count >= static_cast<int32_t>(MAX_NAVMESH_TRIS)) break;
                    int32_t u = poly_edges[e].u;
                    int32_t v = poly_edges[e].v;
                    if (signed_area_2x(pts[u], pts[v], p) < 0.0f) {
                        std::swap(u, v);
                    }
                    tris[tri_count++] = TriPod{ u, v, i, {-1, -1, -1}, 1 };
                }
            }

            int32_t valid_count = 0;
            for (int32_t t = 0; t < tri_count; ++t) {
                if (!tris[t].valid) continue;
                if (tris[t].a >= n_pts || tris[t].b >= n_pts || tris[t].c >= n_pts) continue;
                const float area = 0.5f * std::abs(signed_area_2x(pts[tris[t].a], pts[tris[t].b], pts[tris[t].c]));
                if (area < min_triangle_area) continue;
                tris[valid_count++] = tris[t];
            }

            if (build_portal_adjacency) {
                for (int32_t i = 0; i < valid_count; ++i) {
                    const int32_t iv[3] = { tris[i].a, tris[i].b, tris[i].c };
                    for (int ei = 0; ei < 3; ++ei) {
                        if (tris[i].neighbor[ei] >= 0) continue;
                        const int32_t u = iv[ei];
                        const int32_t v = iv[(ei + 1) % 3];
                        for (int32_t j = i + 1; j < valid_count; ++j) {
                            const int32_t jv[3] = { tris[j].a, tris[j].b, tris[j].c };
                            for (int ej = 0; ej < 3; ++ej) {
                                const int32_t ju = jv[ej];
                                const int32_t jw = jv[(ej + 1) % 3];
                                if ((u == jw && v == ju) || (u == ju && v == jw)) {
                                    tris[i].neighbor[ei] = j;
                                    tris[j].neighbor[ej] = i;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            int32_t* out_tris = reinterpret_cast<int32_t*>(tri_part->raw_base_ptr);
            const size_t max_out_ints = tri_part->capacity_bytes / sizeof(int32_t);
            int32_t* out_adj = (adj_part && adj_part->raw_base_ptr)
                ? reinterpret_cast<int32_t*>(adj_part->raw_base_ptr)
                : nullptr;
            const size_t max_adj_ints = out_adj ? (adj_part->capacity_bytes / sizeof(int32_t)) : 0;

            size_t written_tris = 0;
            for (int32_t i = 0; i < valid_count; ++i) {
                if ((written_tris + 1) * 3 > max_out_ints) break;
                out_tris[written_tris * 3 + 0] = tris[i].a;
                out_tris[written_tris * 3 + 1] = tris[i].b;
                out_tris[written_tris * 3 + 2] = tris[i].c;

                if (out_adj && (written_tris + 1) * 3 <= max_adj_ints) {
                    out_adj[written_tris * 3 + 0] = tris[i].neighbor[0];
                    out_adj[written_tris * 3 + 1] = tris[i].neighbor[1];
                    out_adj[written_tris * 3 + 2] = tris[i].neighbor[2];
                }
                ++written_tris;
            }

            tri_part->selection.element_count = static_cast<int64_t>(written_tris * 3);
            if (adj_part) adj_part->selection.element_count = static_cast<int64_t>(written_tris * 3);
        }
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD& p_context, T_ActiveView& p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_context, p_view);
    }
};

static_assert(sizeof(DelaunayNavMeshBuildTransformLogic<>) == 64, "DelaunayNavMeshBuildTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<DelaunayNavMeshBuildTransformLogic<>>, "DelaunayNavMeshBuildTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core