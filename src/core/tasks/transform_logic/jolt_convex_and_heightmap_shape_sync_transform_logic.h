#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * P3: JoltConvexAndHeightmapShapeSyncTransformLogic
 * Commits dynamic Jolt 3D collision geometries directly to PhysicsServer3D shape RIDs:
 *   - Mode 0: JoltConvexHullShape3D (SHAPE_CONVEX_POLYGON via PackedVector3Array hull points)
 *   - Mode 1: JoltHeightFieldShape3D (SHAPE_HEIGHTMAP with width, depth, heights, and Jolt's accelerated min_height/max_height bounds)
 *   - Mode 2: Static JoltMeshShape3D (SHAPE_CONCAVE_POLYGON via Dictionary{"faces": PackedVector3Array})
 */
struct JoltConvexAndHeightmapShapeSyncTransformLogic {
    static constexpr std::string_view ui_name = "Jolt3D Convex Hull & HeightField Shape Sync";
    static constexpr std::string_view ui_category = "Physics/Shapes";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SPATIAL_2D
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::VECTOR3,
        DataType::FLOAT32
    };

    godot::RID shape_rid;
    int32_t jolt_shape_mode = 0; // 0 = Convex Hull (JoltConvexHullShape3D), 1 = HeightField (JoltHeightFieldShape3D), 2 = Static Mesh Faces (JoltMeshShape3D)
    int32_t heightmap_width = 64;
    int32_t heightmap_depth = 64;

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
        add_prop("shape_rid", godot::Variant::RID, shape_rid);
        add_prop("jolt_shape_mode", godot::Variant::INT, jolt_shape_mode, 2, "Jolt Convex Hull,Jolt HeightField,Jolt Static Concave Mesh");
        add_prop("heightmap_width", godot::Variant::INT, heightmap_width);
        add_prop("heightmap_depth", godot::Variant::INT, heightmap_depth);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("shape_rid")) {
            godot::Variant v = p_props["shape_rid"];
            if (v.get_type() == godot::Variant::RID) {
                shape_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&shape_rid)) = raw;
            }
        }
        if (p_props.has("jolt_shape_mode")) jolt_shape_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["jolt_shape_mode"]), 0, 2);
        if (p_props.has("heightmap_width")) heightmap_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["heightmap_width"]));
        if (p_props.has("heightmap_depth")) heightmap_depth = std::max<int32_t>(2, static_cast<int32_t>(p_props["heightmap_depth"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !shape_rid.is_valid()) return;

        const GrantPartPOD& geom_part = p_ctx.grant->parts[0];
        if (!geom_part.raw_base_ptr) return;

        godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
        if (!ps) return;

        if (jolt_shape_mode == 1) {
            // JoltHeightFieldShape3D expects Dictionary{"width", "depth", "heights", "min_height", "max_height"}
            const int32_t w = std::max<int32_t>(2, heightmap_width);
            const int32_t d = std::max<int32_t>(2, heightmap_depth);
            const size_t total = static_cast<size_t>(w) * d;
            if (geom_part.capacity_bytes / sizeof(float) < total) return;

            const float* src_h = reinterpret_cast<const float*>(geom_part.raw_base_ptr);
            float min_h = src_h[0];
            float max_h = src_h[0];
            for (size_t i = 1; i < total; ++i) {
                min_h = std::min(min_h, src_h[i]);
                max_h = std::max(max_h, src_h[i]);
            }

            godot::PackedFloat32Array heights;
            heights.resize(static_cast<int64_t>(total));
            std::memcpy(heights.ptrw(), src_h, total * sizeof(float));

            godot::Dictionary shape_data;
            shape_data["width"] = w;
            shape_data["depth"] = d;
            shape_data["heights"] = heights;
            shape_data["min_height"] = min_h;
            shape_data["max_height"] = max_h;
            ps->shape_set_data(shape_rid, shape_data);
        } else {
            const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), geom_part.element_stride);
            const int64_t n_pts = (geom_part.selection.element_count > 0)
                ? geom_part.selection.element_count
                : static_cast<int64_t>(geom_part.capacity_bytes / stride);
            if (n_pts < 3) return;

            godot::PackedVector3Array pts;
            pts.resize(n_pts);
            godot::Vector3* dst = pts.ptrw();
            for (int64_t i = 0; i < n_pts; ++i) {
                dst[i] = *reinterpret_cast<const godot::Vector3*>(geom_part.raw_base_ptr + static_cast<size_t>(i) * stride);
            }

            if (jolt_shape_mode == 0) {
                // JoltConvexHullShape3D
                ps->shape_set_data(shape_rid, pts);
            } else {
                // Static JoltMeshShape3D
                godot::Dictionary mesh_data;
                mesh_data["faces"] = pts;
                ps->shape_set_data(shape_rid, mesh_data);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core