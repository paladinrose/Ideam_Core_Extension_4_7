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
 * M29: MeshHolePunchBooleanTransformLogic
 * Cylindrical or spherical CSG hole punch through thin-walled 3D meshes along
 * (punch_origin, punch_axis, punch_radius), radially projecting interior vertices onto
 * the bore wall with entry/exit crater lip flaring.
 */
struct MeshHolePunchBooleanTransformLogic {
    static constexpr std::string_view ui_name = "CSG Cylindrical/Spherical Hole Punch";
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

    int32_t shape_mode = 0; // 0 = Cylindrical Bore, 1 = Spherical Cavity
    godot::Vector3 punch_origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    godot::Vector3 punch_axis = godot::Vector3(0.0f, 0.0f, -1.0f);
    float punch_radius = 0.5f;
    float punch_depth = 4.0f;
    float rim_flare_height = 0.12f;
    float rim_flare_width = 0.25f;
    bool cull_fully_interior_triangles = true;

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
        add_prop("shape_mode", godot::Variant::INT, shape_mode, 2, "Cylindrical Bore,Spherical Cavity");
        add_prop("punch_origin", godot::Variant::VECTOR3, punch_origin);
        add_prop("punch_axis", godot::Variant::VECTOR3, punch_axis);
        add_prop("punch_radius", godot::Variant::FLOAT, punch_radius);
        add_prop("punch_depth", godot::Variant::FLOAT, punch_depth);
        add_prop("rim_flare_height", godot::Variant::FLOAT, rim_flare_height);
        add_prop("rim_flare_width", godot::Variant::FLOAT, rim_flare_width);
        add_prop("cull_fully_interior_triangles", godot::Variant::BOOL, cull_fully_interior_triangles);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("shape_mode")) shape_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["shape_mode"]), 0, 1);
        if (p_props.has("punch_origin")) punch_origin = p_props["punch_origin"];
        if (p_props.has("punch_axis")) {
            godot::Vector3 a = p_props["punch_axis"];
            punch_axis = (a.length_squared() > 1e-8f) ? a.normalized() : godot::Vector3(0.0f, 0.0f, -1.0f);
        }
        if (p_props.has("punch_radius")) punch_radius = std::max(1e-4f, static_cast<float>(p_props["punch_radius"]));
        if (p_props.has("punch_depth")) punch_depth = std::max(1e-4f, static_cast<float>(p_props["punch_depth"]));
        if (p_props.has("rim_flare_height")) rim_flare_height = static_cast<float>(p_props["rim_flare_height"]);
        if (p_props.has("rim_flare_width")) rim_flare_width = std::max(1e-4f, static_cast<float>(p_props["rim_flare_width"]));
        if (p_props.has("cull_fully_interior_triangles")) cull_fully_interior_triangles = static_cast<bool>(p_props["cull_fully_interior_triangles"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& vtx_part = p_ctx.grant->parts[0];
        if (!vtx_part.raw_base_ptr) return;

        GrantPartPOD* idx_part = (p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        const uint32_t v_stride = std::max<uint32_t>(sizeof(godot::Vector3), vtx_part.element_stride);
        const int32_t n_verts = (vtx_part.selection.element_count > 0)
            ? static_cast<int32_t>(vtx_part.selection.element_count)
            : static_cast<int32_t>(vtx_part.capacity_bytes / v_stride);

        const godot::Vector3 axis = (punch_axis.length_squared() > 1e-8f)
            ? punch_axis.normalized()
            : godot::Vector3(0.0f, 0.0f, -1.0f);
        const float outer_band = punch_radius + rim_flare_width;

        for (int32_t i = 0; i < n_verts; ++i) {
            godot::Vector3& v = *reinterpret_cast<godot::Vector3*>(vtx_part.raw_base_ptr + static_cast<size_t>(i) * v_stride);
            const godot::Vector3 rel = v - punch_origin;

            if (shape_mode == 1) {
                // Spherical CSG punch
                const float d = rel.length();
                if (d < outer_band) {
                    const godot::Vector3 dir = (d > 1e-6f) ? (rel / d) : axis;
                    if (d < punch_radius) {
                        v = punch_origin + dir * punch_radius;
                        if (vtx_part.selection.partition_ids && i < vtx_part.selection.capacity) {
                            vtx_part.selection.partition_ids[i] = 1;
                        }
                    } else {
                        const float t = 1.0f - (d - punch_radius) / rim_flare_width;
                        v += dir * (rim_flare_height * t * t);
                    }
                }
            } else {
                // Cylindrical Bore CSG punch
                const float axial = rel.dot(axis);
                if (axial < -punch_depth * 0.5f || axial > punch_depth) continue;

                const godot::Vector3 radial = rel - axis * axial;
                const float r = radial.length();
                if (r >= outer_band) continue;

                const godot::Vector3 r_dir = (r > 1e-6f)
                    ? (radial / r)
                    : ((std::abs(axis.y) < 0.99f) ? axis.cross(godot::Vector3(0.0f, 1.0f, 0.0f)).normalized()
                                                  : axis.cross(godot::Vector3(1.0f, 0.0f, 0.0f)).normalized());

                if (r < punch_radius) {
                    // Project radially outward onto the cylindrical bore wall + exit/entry lip flare
                    const float sign_axial = (axial >= 0.0f) ? 1.0f : -1.0f;
                    v = punch_origin + axis * (axial + sign_axial * rim_flare_height) + r_dir * punch_radius;
                    if (vtx_part.selection.partition_ids && i < vtx_part.selection.capacity) {
                        vtx_part.selection.partition_ids[i] = 1;
                    }
                } else {
                    const float t = 1.0f - (r - punch_radius) / rim_flare_width;
                    const float sign_axial = (axial >= 0.0f) ? -1.0f : 1.0f;
                    v += axis * (sign_axial * rim_flare_height * t * t);
                }
            }
        }

        // Collapse triangles whose all 3 vertices were inside the bore core
        if (cull_fully_interior_triangles && idx_part && vtx_part.selection.partition_ids) {
            int32_t* indices = reinterpret_cast<int32_t*>(idx_part->raw_base_ptr);
            const int32_t n_indices = (idx_part->selection.element_count > 0)
                ? static_cast<int32_t>(idx_part->selection.element_count)
                : static_cast<int32_t>(idx_part->capacity_bytes / sizeof(int32_t));
            const int32_t n_tris = n_indices / 3;

            for (int32_t t = 0; t < n_tris; ++t) {
                const int32_t i0 = indices[t * 3 + 0];
                const int32_t i1 = indices[t * 3 + 1];
                const int32_t i2 = indices[t * 3 + 2];
                if (i0 < 0 || i0 >= n_verts || i1 < 0 || i1 >= n_verts || i2 < 0 || i2 >= n_verts) continue;
                if (vtx_part.selection.partition_ids[i0] == 1 &&
                    vtx_part.selection.partition_ids[i1] == 1 &&
                    vtx_part.selection.partition_ids[i2] == 1) {
                    indices[t * 3 + 1] = i0;
                    indices[t * 3 + 2] = i0;
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