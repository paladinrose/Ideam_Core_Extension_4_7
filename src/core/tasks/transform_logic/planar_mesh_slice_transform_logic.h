#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * PlanarMeshSliceTransformLogic<T> (M26)
 * Slices an indexed 3D triangle mesh across an arbitrary plane (plane_origin, plane_normal),
 * splitting straddling triangles at exact plane-edge intersections in local_workspace,
 * separating positive and negative half-spaces by separation_offset, and triangulating
 * a watertight planar cap over the cut contour.
 */
template <typename T = godot::Vector3>
struct alignas(64) PlanarMeshSliceTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    static constexpr int32_t MAX_CAP_LOOP_PTS = 1024;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::INT32;

    // --- Explicit Spatial Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = MAX_CAP_LOOP_PTS * sizeof(godot::Vector3) + 256;

    static constexpr std::string_view display_name = "Planar 3D Mesh Slice & Cap";
    static constexpr std::string_view ui_category  = "Model/Destruction";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id = INVALID_ID; // 4B: Primary target vertex buffer (Vector3)
    uint32_t index_buffer_id  = INVALID_ID; // 4B: Triangle index buffer (int32_t triplets)
    uint32_t normal_buffer_id = INVALID_ID; // 4B: Optional vertex normal buffer (Vector3)

    godot::Vector3 plane_origin{0.0f, 0.0f, 0.0f}; // 12B: Local-space point on cutting plane
    godot::Vector3 plane_normal{0.0f, 1.0f, 0.0f}; // 12B: Unit normal of cutting plane

    float separation_offset       = 0.05f; // 4B: Distance separating positive/negative half-spaces (meters)
    bool keep_both_halves         = true;  // 1B: Emit both positive and negative half-spaces
    bool generate_watertight_cap  = true;  // 1B: Triangulate planar cap over intersection contour
    uint8_t reserved_padding[22]  = {0};   // 22B -> Total = 64B

    [[nodiscard]] size_t get_transient_requirement(const TaskContextPOD& /*p_context*/) const noexcept {
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

        godot::Dictionary nrm_prop;
        nrm_prop["name"] = "normal_buffer_id";
        nrm_prop["type"] = godot::Variant::INT;
        nrm_prop["hint_string"] = "buffer_option";
        props.push_back(nrm_prop);

        godot::Dictionary orig_prop;
        orig_prop["name"] = "plane_origin";
        orig_prop["type"] = godot::Variant::VECTOR3;
        orig_prop["hint_string"] = "suffix:m";
        props.push_back(orig_prop);

        godot::Dictionary norm_prop;
        norm_prop["name"] = "plane_normal";
        norm_prop["type"] = godot::Variant::VECTOR3;
        props.push_back(norm_prop);

        godot::Dictionary sep_prop;
        sep_prop["name"] = "separation_offset";
        sep_prop["type"] = godot::Variant::FLOAT;
        sep_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        sep_prop["hint_string"] = "0.0,10.0,0.005,suffix:m";
        props.push_back(sep_prop);

        godot::Dictionary both_prop;
        both_prop["name"] = "keep_both_halves";
        both_prop["type"] = godot::Variant::BOOL;
        props.push_back(both_prop);

        godot::Dictionary cap_prop;
        cap_prop["name"] = "generate_watertight_cap";
        cap_prop["type"] = godot::Variant::BOOL;
        props.push_back(cap_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (vertex_buffer_id != INVALID_ID) ? vertex_buffer_id : index_buffer_id;
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
        if (p_props.has("normal_buffer_id")) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        }
        if (p_props.has("plane_origin")) {
            plane_origin = static_cast<godot::Vector3>(p_props["plane_origin"]);
        }
        if (p_props.has("plane_normal")) {
            const godot::Vector3 n = static_cast<godot::Vector3>(p_props["plane_normal"]);
            plane_normal = (n.length_squared() > 1e-8f) ? n.normalized() : godot::Vector3(0.0f, 1.0f, 0.0f);
        }
        if (p_props.has("separation_offset")) {
            separation_offset = std::max(0.0f, static_cast<float>(p_props["separation_offset"]));
        }
        if (p_props.has("keep_both_halves")) {
            keep_both_halves = static_cast<bool>(p_props["keep_both_halves"]);
        }
        if (p_props.has("generate_watertight_cap")) {
            generate_watertight_cap = static_cast<bool>(p_props["generate_watertight_cap"]);
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
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, int32_t>) {
            GrantPartPOD* vtx_part = const_cast<GrantPartPOD*>(p_context.get_grant_part(vertex_buffer_id));
            GrantPartPOD* idx_part = const_cast<GrantPartPOD*>(p_context.get_grant_part(index_buffer_id));
            if (!vtx_part || !idx_part || !vtx_part->raw_base_ptr || !idx_part->raw_base_ptr) return;

            const GrantPartPOD* nrm_part = (normal_buffer_id != INVALID_ID) ? p_context.get_grant_part(normal_buffer_id) : nullptr;
            godot::Vector3* __restrict normals = (nrm_part && nrm_part->raw_base_ptr)
                ? reinterpret_cast<godot::Vector3*>(nrm_part->raw_base_ptr)
                : nullptr;
            const int32_t max_norms = normals ? static_cast<int32_t>(nrm_part->capacity_bytes / sizeof(godot::Vector3)) : 0;

            const godot::Vector3 n_dir = (plane_normal.length_squared() > 1e-8f)
                ? plane_normal.normalized()
                : godot::Vector3(0.0f, 1.0f, 0.0f);

            godot::Vector3* __restrict verts = reinterpret_cast<godot::Vector3*>(vtx_part->raw_base_ptr);
            const int32_t max_verts = static_cast<int32_t>(vtx_part->capacity_bytes / sizeof(godot::Vector3));
            int32_t cur_verts = (vtx_part->selection.element_count > 0)
                ? std::min<int32_t>(static_cast<int32_t>(vtx_part->selection.element_count), max_verts)
                : max_verts;

            int32_t* __restrict indices = reinterpret_cast<int32_t*>(idx_part->raw_base_ptr);
            const int32_t max_indices = static_cast<int32_t>(idx_part->capacity_bytes / sizeof(int32_t));
            int32_t cur_indices = (idx_part->selection.element_count > 0)
                ? std::min<int32_t>(static_cast<int32_t>(idx_part->selection.element_count), max_indices)
                : max_indices;
            const int32_t orig_tri_count = cur_indices / 3;

            godot::Vector3* cap_pts = p_context.local_workspace
                ? reinterpret_cast<godot::Vector3*>(p_context.local_workspace)
                : nullptr;
            int32_t cap_pt_count = 0;

            auto signed_dist = [&](const godot::Vector3& p) noexcept -> float {
                return (p - plane_origin).dot(n_dir);
            };

            auto append_vertex = [&](const godot::Vector3& p, const godot::Vector3& nrm, int64_t part_id) noexcept -> int32_t {
                if (cur_verts >= max_verts) return std::max<int32_t>(0, cur_verts - 1);
                const int32_t id = cur_verts++;
                verts[id] = p;
                if (normals && id < max_norms) {
                    normals[id] = nrm;
                }
                if (vtx_part->selection.partition_ids && id < vtx_part->selection.capacity) {
                    vtx_part->selection.partition_ids[id] = part_id;
                }
                return id;
            };

            auto append_tri = [&](int32_t a, int32_t b, int32_t c) noexcept {
                if (cur_indices + 3 <= max_indices) {
                    indices[cur_indices++] = a;
                    indices[cur_indices++] = b;
                    indices[cur_indices++] = c;
                }
            };

            // 1. Tag original vertices by half-space
            const int32_t orig_vert_count = cur_verts;
            for (int32_t i = 0; i < orig_vert_count; ++i) {
                const float d = signed_dist(verts[i]);
                if (vtx_part->selection.partition_ids && i < vtx_part->selection.capacity) {
                    vtx_part->selection.partition_ids[i] = (d >= 0.0f) ? 0 : 1;
                }
            }

            // 2. Clip straddling triangles across the cut plane
            for (int32_t t = 0; t < orig_tri_count; ++t) {
                const int32_t i0 = indices[t * 3 + 0];
                const int32_t i1 = indices[t * 3 + 1];
                const int32_t i2 = indices[t * 3 + 2];
                if (i0 < 0 || i0 >= orig_vert_count || i1 < 0 || i1 >= orig_vert_count || i2 < 0 || i2 >= orig_vert_count) continue;

                const float d0 = signed_dist(verts[i0]);
                const float d1 = signed_dist(verts[i1]);
                const float d2 = signed_dist(verts[i2]);

                const int b0 = (d0 >= 0.0f) ? 1 : 0;
                const int b1 = (d1 >= 0.0f) ? 1 : 0;
                const int b2 = (d2 >= 0.0f) ? 1 : 0;
                const int sum = b0 + b1 + b2;

                if (sum == 3 || sum == 0) {
                    if (sum == 0 && !keep_both_halves) {
                        indices[t * 3 + 0] = 0;
                        indices[t * 3 + 1] = 0;
                        indices[t * 3 + 2] = 0;
                    }
                    continue;
                }

                // Rotate triangle indices so the lone vertex on one side of the plane is first (vl)
                int32_t vl = i0, va = i1, vb = i2;
                float dl = d0, da = d1, db = d2;
                bool lone_pos = (sum == 1) ? (b0 == 1) : (b0 == 0);

                if ((sum == 1 && b1 == 1) || (sum == 2 && b1 == 0)) {
                    vl = i1; va = i2; vb = i0;
                    dl = d1; da = d2; db = d0;
                    lone_pos = (sum == 1);
                } else if ((sum == 1 && b2 == 1) || (sum == 2 && b2 == 0)) {
                    vl = i2; va = i0; vb = i1;
                    dl = d2; da = d0; db = d1;
                    lone_pos = (sum == 1);
                }

                const float u_a = std::abs(dl) / std::max(1e-7f, std::abs(dl) + std::abs(da));
                const float u_b = std::abs(dl) / std::max(1e-7f, std::abs(dl) + std::abs(db));

                const godot::Vector3 pa = verts[vl].lerp(verts[va], u_a);
                const godot::Vector3 pb = verts[vl].lerp(verts[vb], u_b);
                const godot::Vector3 na_interp = (normals && vl < max_norms && va < max_norms)
                    ? normals[vl].lerp(normals[va], u_a).normalized()
                    : n_dir;
                const godot::Vector3 nb_interp = (normals && vl < max_norms && vb < max_norms)
                    ? normals[vl].lerp(normals[vb], u_b).normalized()
                    : n_dir;

                if (cap_pts && cap_pt_count + 2 <= MAX_CAP_LOOP_PTS) {
                    cap_pts[cap_pt_count++] = pa;
                    cap_pts[cap_pt_count++] = pb;
                }

                const int32_t na_pos = append_vertex(pa + n_dir * (separation_offset * 0.5f), na_interp, 0);
                const int32_t nb_pos = append_vertex(pb + n_dir * (separation_offset * 0.5f), nb_interp, 0);

                if (lone_pos) {
                    indices[t * 3 + 0] = vl;
                    indices[t * 3 + 1] = na_pos;
                    indices[t * 3 + 2] = nb_pos;

                    if (keep_both_halves) {
                        const int32_t na_neg = append_vertex(pa - n_dir * (separation_offset * 0.5f), na_interp, 1);
                        const int32_t nb_neg = append_vertex(pb - n_dir * (separation_offset * 0.5f), nb_interp, 1);
                        append_tri(na_neg, va, vb);
                        append_tri(na_neg, vb, nb_neg);
                    }
                } else {
                    indices[t * 3 + 0] = na_pos;
                    indices[t * 3 + 1] = va;
                    indices[t * 3 + 2] = vb;
                    append_tri(na_pos, vb, nb_pos);

                    if (keep_both_halves) {
                        const int32_t na_neg = append_vertex(pa - n_dir * (separation_offset * 0.5f), na_interp, 1);
                        const int32_t nb_neg = append_vertex(pb - n_dir * (separation_offset * 0.5f), nb_interp, 1);
                        append_tri(vl, na_neg, nb_neg);
                    }
                }
            }

            // 3. Displace original vertices by half separation_offset
            if (separation_offset > 0.0f) {
                for (int32_t i = 0; i < orig_vert_count; ++i) {
                    const float sign = (signed_dist(verts[i]) >= 0.0f) ? 0.5f : -0.5f;
                    verts[i] += n_dir * (separation_offset * sign);
                }
            }

            // 4. Build planar cap fan around the centroid of the intersection contour
            if (generate_watertight_cap && cap_pts && cap_pt_count >= 3) {
                godot::Vector3 centroid(0.0f, 0.0f, 0.0f);
                for (int32_t i = 0; i < cap_pt_count; ++i) centroid += cap_pts[i];
                centroid /= static_cast<float>(cap_pt_count);

                const int32_t c_pos = append_vertex(centroid + n_dir * (separation_offset * 0.5f), -n_dir, 0);
                const int32_t c_neg = keep_both_halves
                    ? append_vertex(centroid - n_dir * (separation_offset * 0.5f), n_dir, 1)
                    : -1;

                for (int32_t i = 0; i + 1 < cap_pt_count; i += 2) {
                    const int32_t p0_pos = append_vertex(cap_pts[i] + n_dir * (separation_offset * 0.5f), -n_dir, 0);
                    const int32_t p1_pos = append_vertex(cap_pts[i + 1] + n_dir * (separation_offset * 0.5f), -n_dir, 0);
                    append_tri(c_pos, p1_pos, p0_pos);

                    if (keep_both_halves && c_neg >= 0) {
                        const int32_t p0_neg = append_vertex(cap_pts[i] - n_dir * (separation_offset * 0.5f), n_dir, 1);
                        const int32_t p1_neg = append_vertex(cap_pts[i + 1] - n_dir * (separation_offset * 0.5f), n_dir, 1);
                        append_tri(c_neg, p0_neg, p1_neg);
                    }
                }
            }

            vtx_part->selection.element_count = cur_verts;
            idx_part->selection.element_count = cur_indices;
        }
    }
};

static_assert(sizeof(PlanarMeshSliceTransformLogic<godot::Vector3>) == 64, "PlanarMeshSliceTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<PlanarMeshSliceTransformLogic<godot::Vector3>>, "PlanarMeshSliceTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core