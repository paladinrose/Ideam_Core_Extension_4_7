#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/vector_traits.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * MeshBoundsSyncTransformLogic<T>
 * Performs a SIMD-friendly min/max AABB and bounding sphere reduction over active
 * Vector3 vertices and synchronizes the expanded custom AABB directly to
 * Godot's RenderingServer::mesh_set_custom_aabb() (and optional Bounds buffer)
 * to prevent premature frustum culling on deformed or fractured meshes.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshBoundsSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Mesh Bounds Sync";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id = INVALID_ID; // Source deformed vertex buffer (Vector3)
    uint32_t bounds_buffer_id = INVALID_ID; // Optional output buffer (writes [min_bounds, max_bounds, center, Vector3(radius, 0, 0)])
    float margin              = 0.05f;      // Safety padding added around AABB (meters)
    bool sync_to_server       = true;       // Call RenderingServer::mesh_set_custom_aabb
    uint8_t reserved_flags[3] = {0};

    godot::RID target_mesh_rid;
    godot::Ref<godot::Mesh> target_mesh;

    uint8_t reserved_padding[32] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vert_prop;
        vert_prop["name"] = "vertex_buffer_id";
        vert_prop["type"] = godot::Variant::INT;
        vert_prop["hint_string"] = "buffer_option";
        props.push_back(vert_prop);

        godot::Dictionary bnd_prop;
        bnd_prop["name"] = "bounds_buffer_id";
        bnd_prop["type"] = godot::Variant::INT;
        bnd_prop["hint_string"] = "buffer_option";
        props.push_back(bnd_prop);

        godot::Dictionary mesh_prop;
        mesh_prop["name"] = "target_mesh";
        mesh_prop["type"] = godot::Variant::OBJECT;
        mesh_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        mesh_prop["hint_string"] = "Mesh";
        props.push_back(mesh_prop);

        godot::Dictionary mar_prop;
        mar_prop["name"] = "margin";
        mar_prop["type"] = godot::Variant::FLOAT;
        mar_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        mar_prop["hint_string"] = "0.0,10.0,0.01,suffix:m";
        props.push_back(mar_prop);

        godot::Dictionary sync_prop;
        sync_prop["name"] = "sync_to_server";
        sync_prop["type"] = godot::Variant::BOOL;
        sync_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(sync_prop);

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
        if (p_props.has("bounds_buffer_id")) {
            bounds_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["bounds_buffer_id"]));
        }
        if (p_props.has("margin")) {
            margin = std::max(0.0f, static_cast<float>(p_props["margin"]));
        }
        if (p_props.has("sync_to_server")) {
            sync_to_server = static_cast<bool>(p_props["sync_to_server"]);
        }
        if (p_props.has("target_mesh_rid")) {
            target_mesh_rid = p_props["target_mesh_rid"];
        }
        if (p_props.has("target_mesh")) {
            target_mesh = p_props["target_mesh"];
            if (target_mesh.is_valid()) {
                target_mesh_rid = target_mesh->get_rid();
            }
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_same_v<T, godot::Vector3>) {
            const GrantPartPOD* vert_part = p_context.get_grant_part(vertex_buffer_id);
            if (!vert_part || !vert_part->raw_base_ptr) return;

            const MemoryBufferSelectionPOD& sel = vert_part->selection;
            if (sel.element_count <= 0) return;

            godot::Vector3 min_b(
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()
            );
            godot::Vector3 max_b(
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest(),
                std::numeric_limits<float>::lowest()
            );

            int64_t valid_count = 0;

            auto accumulate_vertex = [&](int64_t idx) noexcept {
                const godot::Vector3 p = _read_view(p_view, idx);
                min_b.x = std::min(min_b.x, p.x);
                min_b.y = std::min(min_b.y, p.y);
                min_b.z = std::min(min_b.z, p.z);
                max_b.x = std::max(max_b.x, p.x);
                max_b.y = std::max(max_b.y, p.y);
                max_b.z = std::max(max_b.z, p.z);
                ++valid_count;
            };

            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                for (int64_t i = 0; i < sel.capacity; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) {
                        accumulate_vertex(i);
                    }
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    accumulate_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = sel.start_index + sel.element_count;
                for (int64_t i = sel.start_index; i < end; ++i) {
                    accumulate_vertex(i);
                }
            }

            if (valid_count <= 0) return;

            const godot::Vector3 pad(margin, margin, margin);
            min_b = min_b - pad;
            max_b = max_b + pad;

            const godot::Vector3 size   = max_b - min_b;
            const godot::Vector3 center = (min_b + max_b) * 0.5f;
            const float radius          = (size * 0.5f).length();

            // Optional DOD output buffer write: [0]=min, [1]=max, [2]=center, [3]=Vector3(radius, valid_count, 0)
            if (bounds_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* bnd_part = p_context.get_grant_part(bounds_buffer_id)) {
                    if (bnd_part->raw_base_ptr) {
                        godot::Vector3* out_vecs = reinterpret_cast<godot::Vector3*>(bnd_part->raw_base_ptr);
                        const size_t max_vecs = bnd_part->capacity_bytes / sizeof(godot::Vector3);
                        if (max_vecs >= 1) out_vecs[0] = min_b;
                        if (max_vecs >= 2) out_vecs[1] = max_b;
                        if (max_vecs >= 3) out_vecs[2] = center;
                        if (max_vecs >= 4) out_vecs[3] = godot::Vector3(radius, static_cast<float>(valid_count), 0.0f);
                    }
                }
            }

            if (sync_to_server && target_mesh_rid.is_valid()) {
                godot::RenderingServer::get_singleton()->mesh_set_custom_aabb(
                    target_mesh_rid,
                    godot::AABB(min_b, size)
                );
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

static_assert(sizeof(MeshBoundsSyncTransformLogic<>) == 64, "MeshBoundsSyncTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core