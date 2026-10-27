#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_buffer_pod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <algorithm>
#include <bit>
#include <cstring>
#include <string_view>

namespace ideam::core {

/**
 * MeshVertexRegionCommitTransformLogic<T>
 * High-speed runtime byte-slice commit from a DOD vertex buffer directly to
 * Godot's RenderingServer::mesh_surface_update_vertex_region().
 * Supports full-surface blits as well as dirty-range sub-slice updates.
 */
template <typename T = godot::Vector3>
struct alignas(64) MeshVertexRegionCommitTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::FLAT | BufferLayoutType::AOS;
    static constexpr DataType required_types              = DataType::VECTOR3 | DataType::VECTOR3D | DataType::BYTE;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Mesh Vertex Region Commit";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t vertex_buffer_id = INVALID_ID;
    int32_t surface_index     = 0;
    uint32_t vertex_stride    = sizeof(T); // Auto-defaults to 12B for Vector3 or 1B for Byte
    bool dirty_span_only      = true;                   // Upload only [min_active, max_active] span
    uint8_t reserved_flags[3] = {0};

    godot::RID target_mesh_rid;
    godot::Ref<godot::Mesh> target_mesh;

    // Mutable staging PackedByteArray to avoid per-frame Variant vector allocations
    mutable godot::PackedByteArray staging_bytes;

    uint8_t reserved_padding[16] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "vertex_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary surf_prop;
        surf_prop["name"] = "surface_index";
        surf_prop["type"] = godot::Variant::INT;
        surf_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        surf_prop["hint_string"] = "0,16,1";
        props.push_back(surf_prop);

        godot::Dictionary stride_prop;
        stride_prop["name"] = "vertex_stride";
        stride_prop["type"] = godot::Variant::INT;
        stride_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        stride_prop["hint_string"] = "1,64,1,suffix:B";
        props.push_back(stride_prop);

        godot::Dictionary dirty_prop;
        dirty_prop["name"] = "dirty_span_only";
        dirty_prop["type"] = godot::Variant::BOOL;
        dirty_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(dirty_prop);

        godot::Dictionary mesh_prop;
        mesh_prop["name"] = "target_mesh";
        mesh_prop["type"] = godot::Variant::OBJECT;
        mesh_prop["hint"] = godot::PROPERTY_HINT_RESOURCE_TYPE;
        mesh_prop["hint_string"] = "Mesh";
        props.push_back(mesh_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return vertex_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("vertex_buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_buffer_id"]));
        } else if (p_props.has("buffer_id")) {
            vertex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }
        if (p_props.has("surface_index")) {
            surface_index = static_cast<int32_t>(static_cast<int64_t>(p_props["surface_index"]));
        }
        if (p_props.has("vertex_stride")) {
            vertex_stride = static_cast<uint32_t>(static_cast<int64_t>(p_props["vertex_stride"]));
        }
        if (p_props.has("dirty_span_only")) {
            dirty_span_only = static_cast<bool>(p_props["dirty_span_only"]);
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
        if (!target_mesh_rid.is_valid()) return;

        const GrantPartPOD* part = p_context.get_grant_part(vertex_buffer_id);
        if (!part || !part->raw_base_ptr) return;

        const uint32_t stride = (part->element_stride > 0) ? part->element_stride : vertex_stride;
        const MemoryBufferSelectionPOD& sel = part->selection;
        if (sel.element_count <= 0) return;

        int64_t first_idx = 0;
        int64_t last_idx  = sel.capacity - 1;

        if (dirty_span_only) {
            if (sel.mode == SelectionMode::RANGE) {
                first_idx = sel.start_index;
                last_idx  = sel.start_index + sel.element_count - 1;
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                first_idx = sel.capacity;
                last_idx  = -1;
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    const int64_t idx = sel.data.indices[i];
                    if (idx < first_idx) first_idx = idx;
                    if (idx > last_idx)  last_idx  = idx;
                }
            } else if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const int64_t words = (sel.capacity + 63) >> 6;
                first_idx = sel.capacity;
                last_idx  = -1;
                for (int64_t w = 0; w < words; ++w) {
                    if (sel.data.bitset[w] != 0) {
                        const int64_t low_bit = (w << 6) + std::countr_zero(sel.data.bitset[w]);
                        const int64_t high_bit = (w << 6) + (63 - std::countl_zero(sel.data.bitset[w]));
                        if (low_bit < first_idx) first_idx = low_bit;
                        if (high_bit > last_idx) last_idx = std::min(high_bit, sel.capacity - 1);
                    }
                }
            }
        }

        if (first_idx > last_idx || first_idx < 0) return;

        const size_t byte_offset = static_cast<size_t>(first_idx) * stride;
        const size_t span_count  = static_cast<size_t>(last_idx - first_idx + 1);
        const size_t copy_bytes  = std::min(span_count * stride, static_cast<size_t>(part->capacity_bytes) - byte_offset);
        if (copy_bytes == 0) return;

        if (static_cast<size_t>(staging_bytes.size()) != copy_bytes) {
            staging_bytes.resize(static_cast<int64_t>(copy_bytes));
        }

        std::memcpy(staging_bytes.ptrw(), part->raw_base_ptr + byte_offset, copy_bytes);

        godot::RenderingServer::get_singleton()->mesh_surface_update_vertex_region(
            target_mesh_rid,
            surface_index,
            static_cast<int64_t>(byte_offset),
            staging_bytes
        );
    }
};

static_assert(sizeof(MeshVertexRegionCommitTransformLogic<>) <= 64, "MeshVertexRegionCommitTransformLogic exceeded cache line boundary.");

} // namespace ideam::core
