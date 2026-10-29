#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class CollisionShapeSyncMode : uint8_t {
    CONCAVE_TRIMESH    = 0, // Packs indexed/unindexed triangles into a flat face-vertex array for ConcavePolygonShape3D
    CONVEX_POINT_CLOUD = 1, // Passes active (optionally strided) vertices to ConvexPolygonShape3D
    HEIGHTMAP_GRID     = 2  // Maps a 2D height buffer (float or Vector3.y) into a HeightMapShape3D dictionary
};

/**
 * CollisionShapeMeshSyncTransformLogic<T>
 * Pushes deformed, skinned, or fractured DOD vertex/index/height buffers directly into
 * Godot's PhysicsServer3D::shape_set_data(shape_rid, data_variant) for runtime physical accuracy.
 */
template <typename T = godot::Vector3>
struct alignas(64) CollisionShapeMeshSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR3 | DataType::VECTOR3D | DataType::FLOAT32;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Collision Shape Mesh Sync";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id      = INVALID_ID; // 4B: Primary Vector3 vertex buffer (or float heightmap)
    uint32_t index_buffer_id       = INVALID_ID; // 4B: Optional int32_t triangle index buffer (for CONCAVE_TRIMESH)
    int32_t grid_width             = 64;         // 4B: Width (X) for HEIGHTMAP_GRID mode
    int32_t grid_depth             = 64;         // 4B: Depth (Z) for HEIGHTMAP_GRID mode
    uint16_t vertex_stride_step    = 1;          // 2B: Subsampling stride for CONVEX_POINT_CLOUD (1 = all vertices)
    CollisionShapeSyncMode mode    = CollisionShapeSyncMode::CONCAVE_TRIMESH; // 1B
    bool backface_collision        = false;      // 1B: ConcavePolygonShape3D backface_collision flag
    bool one_shot                  = false;      // 1B: Sync once and skip subsequent frames unless invalidated
    mutable bool already_synced    = false;      // 1B: Runtime one-shot latch
    uint8_t reserved_flags[2]      = {0};        // 2B (24B offset -> 8-byte aligned)

    godot::RID target_shape_rid;                 // 8B (32B offset)
    godot::Ref<godot::Shape3D> target_shape;     // 8B (40B offset)

    // Mutable staging PackedVector3Array to avoid per-frame allocations in CONCAVE/CONVEX modes
    mutable godot::PackedVector3Array staging_points; // 16B (56B offset)

    uint8_t reserved_padding[8]    = {0};        // 8B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary mode_prop;
        mode_prop["name"] = "mode";
        mode_prop["type"] = godot::Variant::INT;
        mode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        mode_prop["hint_string"] = "Concave Trimesh:0,Convex Point Cloud:1,HeightMap Grid:2";
        props.push_back(mode_prop);

        godot::Dictionary vert_prop;
        vert_prop["name"] = "vertex_buffer_id";
        vert_prop["type"] = godot::Variant::INT;
        vert_prop["hint_string"] = "buffer_option";
        props.push_back(vert_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "index_buffer_id";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint_string"] = "buffer_option";
        props.push_back(idx_prop);

        godot::Dictionary shp_prop;
        shp_prop["name"] = "target_shape";
        shp_prop["type"] = godot::Variant::OBJECT;
        shp_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        shp_prop["hint_string"] = "Shape3D";
        props.push_back(shp_prop);

        godot::Dictionary step_prop;
        step_prop["name"] = "vertex_stride_step";
        step_prop["type"] = godot::Variant::INT;
        step_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        step_prop["hint_string"] = "1,64,1";
        props.push_back(step_prop);

        godot::Dictionary bf_prop;
        bf_prop["name"] = "backface_collision";
        bf_prop["type"] = godot::Variant::BOOL;
        bf_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(bf_prop);

        godot::Dictionary gw_prop;
        gw_prop["name"] = "grid_width";
        gw_prop["type"] = godot::Variant::INT;
        gw_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        gw_prop["hint_string"] = "2,4096,1";
        props.push_back(gw_prop);

        godot::Dictionary gd_prop;
        gd_prop["name"] = "grid_depth";
        gd_prop["type"] = godot::Variant::INT;
        gd_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        gd_prop["hint_string"] = "2,4096,1";
        props.push_back(gd_prop);

        godot::Dictionary os_prop;
        os_prop["name"] = "one_shot";
        os_prop["type"] = godot::Variant::BOOL;
        os_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(os_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("target_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("index_buffer_id")) {
            index_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["index_buffer_id"]));
        }
        if (p_props.has("mode")) {
            mode = static_cast<CollisionShapeSyncMode>(static_cast<uint8_t>(static_cast<int64_t>(p_props["mode"])));
        }
        if (p_props.has("vertex_stride_step")) {
            vertex_stride_step = static_cast<uint16_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["vertex_stride_step"]), 1, 1024));
        }
        if (p_props.has("backface_collision")) {
            backface_collision = static_cast<bool>(p_props["backface_collision"]);
        }
        if (p_props.has("grid_width")) {
            grid_width = std::max<int32_t>(2, static_cast<int32_t>(static_cast<int64_t>(p_props["grid_width"])));
        }
        if (p_props.has("grid_depth")) {
            grid_depth = std::max<int32_t>(2, static_cast<int32_t>(static_cast<int64_t>(p_props["grid_depth"])));
        }
        if (p_props.has("one_shot")) {
            one_shot = static_cast<bool>(p_props["one_shot"]);
        }
        if (p_props.has("target_shape_rid")) {
            target_shape_rid = p_props["target_shape_rid"];
        }
        if (p_props.has("target_shape")) {
            target_shape = p_props["target_shape"];
            if (target_shape.is_valid()) {
                target_shape_rid = target_shape->get_rid();
            }
        }
        already_synced = false;
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3> || std::is_same_v<T, float>) {
            if (one_shot && already_synced) return;
            if (!target_shape_rid.is_valid()) return;

            godot::PhysicsServer3D* ps = godot::PhysicsServer3D::get_singleton();
            if (!ps) return;

            const GrantPartPOD* vert_part = p_context.get_grant_part(vertex_buffer_id);
            if (!vert_part || !vert_part->raw_base_ptr) return;

            if (mode == CollisionShapeSyncMode::HEIGHTMAP_GRID) {
                const int64_t total_cells = static_cast<int64_t>(grid_width) * static_cast<int64_t>(grid_depth);
                const int64_t avail_elems = static_cast<int64_t>(vert_part->capacity_bytes / sizeof(T));
                if (total_cells <= 0 || avail_elems < total_cells) return;

                godot::PackedFloat32Array heights;
                heights.resize(total_cells);
                float* __restrict dst_h = heights.ptrw();

                float min_h = std::numeric_limits<float>::max();
                float max_h = std::numeric_limits<float>::lowest();

                for (int64_t i = 0; i < total_cells; ++i) {
                    float h = 0.0f;
                    if constexpr (std::is_same_v<T, float>) {
                        h = _read_view(p_view, i);
                    } else {
                        h = _read_view(p_view, i).y;
                    }
                    dst_h[i] = h;
                    min_h = std::min(min_h, h);
                    max_h = std::max(max_h, h);
                }

                if (min_h > max_h) {
                    min_h = 0.0f;
                    max_h = 0.0f;
                }

                godot::Dictionary shape_data;
                shape_data["width"]      = grid_width;
                shape_data["depth"]      = grid_depth;
                shape_data["heights"]    = heights;
                shape_data["min_height"] = min_h;
                shape_data["max_height"] = max_h;

                ps->shape_set_data(target_shape_rid, shape_data);
                already_synced = true;
                return;
            }

            if constexpr (std::is_same_v<T, godot::Vector3>) {
                const int64_t vert_cap = static_cast<int64_t>(vert_part->capacity_bytes / sizeof(godot::Vector3));
                if (vert_cap < 3) return;

                if (mode == CollisionShapeSyncMode::CONCAVE_TRIMESH) {
                    const GrantPartPOD* idx_part = (index_buffer_id != INVALID_ID)
                        ? p_context.get_grant_part(index_buffer_id)
                        : nullptr;
                    const int32_t* __restrict indices = (idx_part && idx_part->raw_base_ptr)
                        ? reinterpret_cast<const int32_t*>(idx_part->raw_base_ptr)
                        : nullptr;

                    if (indices) {
                        const int64_t idx_count = idx_part->selection.element_count > 0
                            ? idx_part->selection.element_count
                            : static_cast<int64_t>(idx_part->capacity_bytes / sizeof(int32_t));
                        const int64_t face_count = idx_count / 3;
                        if (face_count <= 0) return;

                        const int64_t out_vert_count = face_count * 3;
                        if (staging_points.size() != out_vert_count) {
                            staging_points.resize(out_vert_count);
                        }
                        godot::Vector3* __restrict dst_pts = staging_points.ptrw();

                        for (int64_t f = 0; f < face_count; ++f) {
                            const int64_t base_i = f * 3;
                            const int32_t i0 = indices[base_i];
                            const int32_t i1 = indices[base_i + 1];
                            const int32_t i2 = indices[base_i + 2];

                            if (i0 >= 0 && i1 >= 0 && i2 >= 0 && i0 < vert_cap && i1 < vert_cap && i2 < vert_cap) {
                                dst_pts[base_i]     = _read_view(p_view, i0);
                                dst_pts[base_i + 1] = _read_view(p_view, i1);
                                dst_pts[base_i + 2] = _read_view(p_view, i2);
                            } else {
                                dst_pts[base_i]     = godot::Vector3(0.0f, 0.0f, 0.0f);
                                dst_pts[base_i + 1] = godot::Vector3(0.0f, 0.0f, 0.0f);
                                dst_pts[base_i + 2] = godot::Vector3(0.0f, 0.0f, 0.0f);
                            }
                        }
                    } else {
                        // Unindexed triangle soup
                        const int64_t active_verts = (vert_part->selection.element_count >= 3)
                            ? (vert_part->selection.element_count - (vert_part->selection.element_count % 3))
                            : (vert_cap - (vert_cap % 3));
                        if (active_verts < 3) return;

                        if (staging_points.size() != active_verts) {
                            staging_points.resize(active_verts);
                        }
                        godot::Vector3* __restrict dst_pts = staging_points.ptrw();

                        if (vert_part->element_stride == sizeof(godot::Vector3) && vert_part->selection.mode == SelectionMode::RANGE) {
                            const int64_t start = vert_part->selection.start_index;
                            if (start >= 0 && start + active_verts <= vert_cap) {
                                std::memcpy(
                                    dst_pts,
                                    reinterpret_cast<const godot::Vector3*>(vert_part->raw_base_ptr) + start,
                                    static_cast<size_t>(active_verts) * sizeof(godot::Vector3)
                                );
                            }
                        } else {
                            for (int64_t i = 0; i < active_verts; ++i) {
                                dst_pts[i] = _read_view(p_view, i);
                            }
                        }
                    }

                    godot::Dictionary trimesh_data;
                    trimesh_data["faces"]              = staging_points;
                    trimesh_data["backface_collision"] = backface_collision;
                    ps->shape_set_data(target_shape_rid, trimesh_data);
                    already_synced = true;
                    return;
                }

                // CONVEX_POINT_CLOUD mode
                const MemoryBufferSelectionPOD& sel = vert_part->selection;
                const int64_t step = std::max<int64_t>(1, static_cast<int64_t>(vertex_stride_step));

                if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                    const uint64_t* bitset = sel.data.bitset;
                    const int64_t limit = std::min<int64_t>(sel.capacity, vert_cap);
                    int64_t max_out = ( std::max<int64_t>(0, sel.element_count) + step - 1) / step;
                    if (max_out < 3) max_out = limit;
                    staging_points.resize(max_out);
                    godot::Vector3* __restrict dst_pts = staging_points.ptrw();

                    int64_t seen = 0;
                    int64_t written = 0;
                    for (int64_t i = 0; i < limit; ++i) {
                        if (bitset[i >> 6] & (1ULL << (i & 63))) {
                            if ((seen % step) == 0 && written < max_out) {
                                dst_pts[written++] = _read_view(p_view, i);
                            }
                            ++seen;
                        }
                    }
                    if (written < 3) return;
                    if (staging_points.size() != written) staging_points.resize(written);
                } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                    const int64_t out_count = (sel.element_count + step - 1) / step;
                    if (out_count < 3) return;
                    if (staging_points.size() != out_count) staging_points.resize(out_count);
                    godot::Vector3* __restrict dst_pts = staging_points.ptrw();

                    int64_t written = 0;
                    for (int64_t i = 0; i < sel.element_count && written < out_count; i += step) {
                        const int64_t idx = sel.data.indices[i];
                        if (idx >= 0 && idx < vert_cap) {
                            dst_pts[written++] = _read_view(p_view, idx);
                        }
                    }
                    if (written < 3) return;
                    if (staging_points.size() != written) staging_points.resize(written);
                } else {
                    const int64_t start = std::clamp<int64_t>(sel.start_index, 0, vert_cap);
                    const int64_t elem_cnt = (sel.element_count > 0) ? sel.element_count : (vert_cap - start);
                    const int64_t end = std::min<int64_t>(start + elem_cnt, vert_cap);
                    const int64_t out_count = (end - start + step - 1) / step;
                    if (out_count < 3) return;

                    if (staging_points.size() != out_count) {
                        staging_points.resize(out_count);
                    }
                    godot::Vector3* __restrict dst_pts = staging_points.ptrw();

                    if (step == 1 && vert_part->element_stride == sizeof(godot::Vector3)) {
                        std::memcpy(
                            dst_pts,
                            reinterpret_cast<const godot::Vector3*>(vert_part->raw_base_ptr) + start,
                            static_cast<size_t>(out_count) * sizeof(godot::Vector3)
                        );
                    } else {
                        int64_t written = 0;
                        for (int64_t i = start; i < end && written < out_count; i += step) {
                            dst_pts[written++] = _read_view(p_view, i);
                        }
                    }
                }

                ps->shape_set_data(target_shape_rid, staging_points);
                already_synced = true;
            }
        }
    }

private:
    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T _read_view(const T_View& p_view, int64_t idx) noexcept {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T>(p_view[idx].load());
        } else if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T*>(p_view[idx]);
        } else {
            return static_cast<T>(p_view[idx]);
        }
    }
};

static_assert(sizeof(CollisionShapeMeshSyncTransformLogic<>) == 64, "CollisionShapeMeshSyncTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core