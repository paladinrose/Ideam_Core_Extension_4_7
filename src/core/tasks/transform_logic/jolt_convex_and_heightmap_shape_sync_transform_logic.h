#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class JoltCollisionShapeMode : uint8_t {
    CONVEX_HULL  = 0,
    HEIGHT_FIELD = 1,
    STATIC_MESH  = 2
};

/**
 * JoltConvexAndHeightmapShapeSyncTransformLogic<T>
 * Commits dynamic Jolt 3D collision geometries directly to PhysicsServer3D shape RIDs:
 *   - Mode 0: JoltConvexHullShape3D (SHAPE_CONVEX_POLYGON via PackedVector3Array hull points)
 *   - Mode 1: JoltHeightFieldShape3D (SHAPE_HEIGHTMAP with width, depth, heights, and Jolt's accelerated min_height/max_height bounds)
 *   - Mode 2: Static JoltMeshShape3D (SHAPE_CONCAVE_POLYGON via Dictionary{"faces": PackedVector3Array})
 */
template <typename T = godot::Vector3>
struct alignas(64) JoltConvexAndHeightmapShapeSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Jolt3D Convex Hull & HeightField Shape Sync";
    static constexpr std::string_view ui_category  = "Physics/Shapes";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t geometry_buffer_id            = INVALID_ID; // 4B: Primary target Vector3 points or float heightfield buffer
    int32_t heightmap_width                = 64;         // 4B: HeightField X samples
    int32_t heightmap_depth                = 64;         // 4B: HeightField Z samples
    JoltCollisionShapeMode jolt_shape_mode = JoltCollisionShapeMode::CONVEX_HULL; // 1B
    uint8_t reserved_flags[3]              = {0};        // 3B -> 16B

    godot::RID shape_rid;                                // 8B: Target PhysicsServer3D Shape RID -> 24B
    uint8_t reserved_padding[40]           = {0};        // 40B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary gbuf_prop;
        gbuf_prop["name"] = "geometry_buffer_id";
        gbuf_prop["type"] = godot::Variant::INT;
        gbuf_prop["hint_string"] = "buffer_option";
        props.push_back(gbuf_prop);

        godot::Dictionary srid_prop;
        srid_prop["name"] = "shape_rid";
        srid_prop["type"] = godot::Variant::RID;
        props.push_back(srid_prop);

        godot::Dictionary mode_prop;
        mode_prop["name"] = "jolt_shape_mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Jolt Convex Hull:0,Jolt HeightField:1,Jolt Static Concave Mesh:2";
        props.push_back(mode_prop);

        godot::Dictionary hw_prop;
        hw_prop["name"] = "heightmap_width";
        hw_prop["type"] = godot::Variant::INT;
        hw_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        hw_prop["hint_string"] = "2,4096,1";
        props.push_back(hw_prop);

        godot::Dictionary hd_prop;
        hd_prop["name"] = "heightmap_depth";
        hd_prop["type"] = godot::Variant::INT;
        hd_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        hd_prop["hint_string"] = "2,4096,1";
        props.push_back(hd_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return geometry_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("geometry_buffer_id")) {
            geometry_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["geometry_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && geometry_buffer_id == INVALID_ID) {
            geometry_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && geometry_buffer_id == INVALID_ID) {
            geometry_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("shape_rid")) {
            const godot::Variant v = p_props["shape_rid"];
            if (v.get_type() == godot::Variant::RID) {
                shape_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&shape_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("jolt_shape_mode")) {
            jolt_shape_mode = static_cast<JoltCollisionShapeMode>(std::clamp<int64_t>(static_cast<int64_t>(p_props["jolt_shape_mode"]), 0, 2));
        }
        if (p_props.has("heightmap_width")) {
            heightmap_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["heightmap_width"]));
        }
        if (p_props.has("heightmap_depth")) {
            heightmap_depth = std::max<int32_t>(2, static_cast<int32_t>(p_props["heightmap_depth"]));
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
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, float>) {
            if (!shape_rid.is_valid()) return;

            const GrantPartPOD* geom_part = p_context.get_grant_part(geometry_buffer_id);
            if (!geom_part || !geom_part->raw_base_ptr) return;

            godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
            if (!ps) return;

            if (jolt_shape_mode == JoltCollisionShapeMode::HEIGHT_FIELD) {
                const int32_t w = std::max<int32_t>(2, heightmap_width);
                const int32_t d = std::max<int32_t>(2, heightmap_depth);
                const size_t total = static_cast<size_t>(w) * static_cast<size_t>(d);
                if (geom_part->capacity_bytes / sizeof(float) < total) return;

                const float* src_h = reinterpret_cast<const float*>(geom_part->raw_base_ptr);
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
                shape_data["width"]      = w;
                shape_data["depth"]      = d;
                shape_data["heights"]    = heights;
                shape_data["min_height"] = min_h;
                shape_data["max_height"] = max_h;
                ps->shape_set_data(shape_rid, shape_data);
            } else {
                const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), geom_part->element_stride);
                const int64_t n_pts = (geom_part->selection.element_count > 0)
                    ? geom_part->selection.element_count
                    : static_cast<int64_t>(geom_part->capacity_bytes / stride);
                if (n_pts < 3) return;

                godot::PackedVector3Array pts;
                pts.resize(n_pts);
                godot::Vector3* dst = pts.ptrw();
                for (int64_t i = 0; i < n_pts; ++i) {
                    dst[i] = *reinterpret_cast<const godot::Vector3*>(geom_part->raw_base_ptr + static_cast<size_t>(i) * stride);
                }

                if (jolt_shape_mode == JoltCollisionShapeMode::CONVEX_HULL) {
                    ps->shape_set_data(shape_rid, pts);
                } else {
                    godot::Dictionary mesh_data;
                    mesh_data["faces"] = pts;
                    ps->shape_set_data(shape_rid, mesh_data);
                }
            }
        }
    }
};

static_assert(sizeof(JoltConvexAndHeightmapShapeSyncTransformLogic<>) == 64, "JoltConvexAndHeightmapShapeSyncTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<JoltConvexAndHeightmapShapeSyncTransformLogic<>>, "JoltConvexAndHeightmapShapeSyncTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core