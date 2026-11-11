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
 * M26: PlanarMeshSliceTransformLogic
 * Slices an indexed 3D triangle mesh across an arbitrary plane (plane_origin, plane_normal),
 * splitting straddling triangles at exact plane-edge intersections, separating positive and
 * negative half-spaces by separation_offset, and triangulating a planar cap over the cut contour.
 */
struct PlanarMeshSliceTransformLogic {
    static constexpr std::string_view ui_name = "Planar 3D Mesh Slice & Cap";
    static constexpr std::string_view ui_category = "Model/Destruction";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::INT32
    };

    static constexpr int32_t MAX_CAP_LOOP_PTS = 1024;

    godot::Vector3 plane_origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 plane_normal = godot::Vector3(0.0f, 1.0f, 0.0f);
    float separation_offset = 0.05f;
    bool keep_both_halves = true;
    bool generate_watertight_cap = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("plane_origin", godot::Variant::VECTOR3, plane_origin);
        add_prop("plane_normal", godot::Variant::VECTOR3, plane_normal);
        add_prop("separation_offset", godot::Variant::FLOAT, separation_offset);
        add_prop("keep_both_halves", godot::Variant::BOOL, keep_both_halves);
        add_prop("generate_watertight_cap", godot::Variant::BOOL, generate_watertight_cap);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("plane_origin")) plane_origin = p_props["plane_origin"];
        if (p_props.has("plane_normal")) {
            godot::Vector3 n = p_props["plane_normal"];
            plane_normal = (n.length_squared() > 1e-8f) ? n.normalized() : godot::Vector3(0.0f, 1.0f, 0.0f);
        }
        if (p_props.has("separation_offset")) separation_offset = std::max(0.0f, static_cast<float>(p_props["separation_offset"]));
        if (p_props.has("keep_both_halves")) keep_both_halves = static_cast<bool>(p_props["keep_both_halves"]);
        if (p_props.has("generate_watertight_cap")) generate_watertight_cap = static_cast<bool>(p_props["generate_watertight_cap"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept {
        return MAX_CAP_LOOP_PTS * sizeof(godot::Vector3) + 256;
    }

    size_t get_transient_requirement() const noexcept {
        return MAX_CAP_LOOP_PTS * sizeof(godot::Vector3) + 256;
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        GrantPartPOD& idx_part = p_ctx.grant->parts[1];
        if (!vtx_part.raw_base_ptr || !idx_part.raw_base_ptr) return;

        const godot::Vector3 n_dir = (plane_normal.length_squared() > 1e-8f)
            ? plane_normal.normalized()
            : godot::Vector3(0.0f, 1.0f, 0.0f);

        godot::Vector3* verts = reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr);
        const int32_t max_verts = static_cast<int32_t>(vtx_part.capacity_bytes / sizeof(godot::Vector3));
        int32_t cur_verts = (vtx_part.selection.element_count > 0)
            ? static_cast<int32_t>(vtx_part.selection.element_count)
            : max_verts;

        int32_t* indices = reinterpret_cast<int32_t*>(idx_part.raw_base_ptr);
        const int32_t max_indices = static_cast<int32_t>(idx_part.capacity_bytes / sizeof(int32_t));
        int32_t cur_indices = (idx_part.selection.element_count > 0)
            ? static_cast<int32_t>(idx_part.selection.element_count)
            : max_indices;
        const int32_t orig_tri_count = cur_indices / 3;

        godot::Vector3* cap_pts = p_ctx.local_workspace
            ? reinterpret_cast<godot::Vector3*>(p_ctx.local_workspace)
            : nullptr;
        int32_t cap_pt_count = 0;

        auto signed_dist = [&](const godot::Vector3& p) noexcept -> float {
            return (p - plane_origin).dot(n_dir);
        };

        auto append_vertex = [&](const godot::Vector3& p, int64_t part_id) noexcept -> int32_t {
            if (cur_verts >= max_verts) return cur_verts - 1;
            const int32_t id = cur_verts++;
            verts[id] = p;
            if (vtx_part.selection.partition_ids && id < vtx_part.selection.capacity) {
                vtx_part.selection.partition_ids[id] = part_id;
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

        // Tag original vertices by half-space and apply separation offset
        for (int32_t i = 0; i < cur_verts; ++i) {
            const float d = signed_dist(verts[i]);
            if (vtx_part.selection.partition_ids && i < vtx_part.selection.capacity) {
                vtx_part.selection.partition_ids[i] = (d >= 0.0f) ? 0 : 1;
            }
        }

        // Clip straddling triangles across the cut plane
        for (int32_t t = 0; t < orig_tri_count; ++t) {
            const int32_t i0 = indices[t * 3 + 0];
            const int32_t i1 = indices[t * 3 + 1];
            const int32_t i2 = indices[t * 3 + 2];
            if (i0 < 0 || i0 >= cur_verts || i1 < 0 || i1 >= cur_verts || i2 < 0 || i2 >= cur_verts) continue;

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

            // Rotate triangle indices so the lone vertex on one side of the plane is first (v_lone)
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

            const float ta = std::clamp(dl / std::max(1e-7f, std::abs(dl - da)) * ((dl < da) ? -1.0f : 1.0f), 0.0f, 1.0f);
            const float tb = std::clamp(dl / std::max(1e-7f, std::abs(dl - db)) * ((dl < db) ? -1.0f : 1.0f), 0.0f, 1.0f);
            const float u_a = std::abs(dl) / std::max(1e-7f, std::abs(dl) + std::abs(da));
            const float u_b = std::abs(dl) / std::max(1e-7f, std::abs(dl) + std::abs(db));
            (void)ta; (void)tb;

            const godot::Vector3 pa = verts[vl].lerp(verts[va], u_a);
            const godot::Vector3 pb = verts[vl].lerp(verts[vb], u_b);

            if (cap_pts && cap_pt_count + 2 <= MAX_CAP_LOOP_PTS) {
                cap_pts[cap_pt_count++] = pa;
                cap_pts[cap_pt_count++] = pb;
            }

            const int32_t na_pos = append_vertex(pa + n_dir * (separation_offset * 0.5f), 0);
            const int32_t nb_pos = append_vertex(pb + n_dir * (separation_offset * 0.5f), 0);

            if (lone_pos) {
                // Replace original triangle with the positive lone tip triangle
                indices[t * 3 + 0] = vl;
                indices[t * 3 + 1] = na_pos;
                indices[t * 3 + 2] = nb_pos;

                if (keep_both_halves) {
                    const int32_t na_neg = append_vertex(pa - n_dir * (separation_offset * 0.5f), 1);
                    const int32_t nb_neg = append_vertex(pb - n_dir * (separation_offset * 0.5f), 1);
                    append_tri(na_neg, va, vb);
                    append_tri(na_neg, vb, nb_neg);
                }
            } else {
                // Positive side has the quad (na_pos, va, vb, nb_pos)
                indices[t * 3 + 0] = na_pos;
                indices[t * 3 + 1] = va;
                indices[t * 3 + 2] = vb;
                append_tri(na_pos, vb, nb_pos);

                if (keep_both_halves) {
                    const int32_t na_neg = append_vertex(pa - n_dir * (separation_offset * 0.5f), 1);
                    const int32_t nb_neg = append_vertex(pb - n_dir * (separation_offset * 0.5f), 1);
                    append_tri(vl, na_neg, nb_neg);
                }
            }
        }

        // Displace original vertices by half separation_offset
        if (separation_offset > 0.0f) {
            for (int32_t i = 0; i < cur_verts; ++i) {
                if (vtx_part.selection.partition_ids && i < vtx_part.selection.capacity) {
                    const float sign = (vtx_part.selection.partition_ids[i] == 0) ? 0.5f : -0.5f;
                    verts[i] += n_dir * (separation_offset * sign);
                }
            }
        }

        // Build planar cap fan around the centroid of the intersection contour
        if (generate_watertight_cap && cap_pts && cap_pt_count >= 3) {
            godot::Vector3 centroid(0.0f, 0.0f, 0.0f);
            for (int32_t i = 0; i < cap_pt_count; ++i) centroid += cap_pts[i];
            centroid /= static_cast<float>(cap_pt_count);

            const int32_t c_pos = append_vertex(centroid + n_dir * (separation_offset * 0.5f), 0);
            const int32_t c_neg = keep_both_halves
                ? append_vertex(centroid - n_dir * (separation_offset * 0.5f), 1)
                : -1;

            for (int32_t i = 0; i + 1 < cap_pt_count; i += 2) {
                const int32_t p0_pos = append_vertex(cap_pts[i] + n_dir * (separation_offset * 0.5f), 0);
                const int32_t p1_pos = append_vertex(cap_pts[i + 1] + n_dir * (separation_offset * 0.5f), 0);
                append_tri(c_pos, p1_pos, p0_pos);

                if (keep_both_halves && c_neg >= 0) {
                    const int32_t p0_neg = append_vertex(cap_pts[i] - n_dir * (separation_offset * 0.5f), 1);
                    const int32_t p1_neg = append_vertex(cap_pts[i + 1] - n_dir * (separation_offset * 0.5f), 1);
                    append_tri(c_neg, p0_neg, p1_neg);
                }
            }
        }

        vtx_part.selection.element_count = cur_verts;
        idx_part.selection.element_count = cur_indices;
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core