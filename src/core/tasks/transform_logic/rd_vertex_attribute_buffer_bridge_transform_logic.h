#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class RDVertexRegionTargetType : uint8_t {
    MESH_VERTEX_REGION    = 0,
    MESH_ATTRIBUTE_REGION = 1,
    MESH_SKIN_REGION      = 2,
    RAW_RD_BUFFER_RID     = 3
};

/**
 * RDVertexAttributeBufferBridgeTransformLogic<T> (G_RD6)
 * Commits DOD / GPU compute output buffers directly to mesh vertex, attribute, or skin
 * region buffers via RenderingServer::mesh_surface_update_vertex_region /
 * mesh_surface_update_attribute_region or RenderingDevice::buffer_update on vertex storage RIDs.
 */
template <typename T = godot::Vector3>
struct alignas(64) RDVertexAttributeBufferBridgeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::VECTOR3 | DataType::VECTOR4 | DataType::COLOR | DataType::FLOAT32 | DataType::BYTE;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD Mesh Vertex & Attribute Buffer Bridge";
    static constexpr std::string_view ui_category  = "GPU/Bridge";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t source_buffer_id                   = INVALID_ID; // 4B: Primary target DOD vertex/attribute buffer
    int32_t surface_index                       = 0;          // 4B: Mesh surface index
    int32_t byte_offset                         = 0;          // 4B: Destination region byte offset
    RDVertexRegionTargetType target_region_type = RDVertexRegionTargetType::MESH_VERTEX_REGION; // 1B
    uint8_t reserved_flags[3]                   = {0};        // 3B -> 16B

    godot::RID mesh_or_vertex_buffer_rid;                     // 8B -> 24B
    mutable godot::PackedByteArray staging_bytes;             // 16B -> 40B
    uint8_t reserved_padding[24]                = {0};        // 24B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary sbuf_prop;
        sbuf_prop["name"] = "source_buffer_id";
        sbuf_prop["type"] = godot::Variant::INT;
        sbuf_prop["hint_string"] = "buffer_option";
        props.push_back(sbuf_prop);

        godot::Dictionary rid_prop;
        rid_prop["name"] = "mesh_or_vertex_buffer_rid";
        rid_prop["type"] = godot::Variant::RID;
        props.push_back(rid_prop);

        godot::Dictionary rtype_prop;
        rtype_prop["name"] = "target_region_type";
        rtype_prop["type"] = godot::Variant::INT;
        rtype_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        rtype_prop["hint_string"] = "Mesh Vertex Region:0,Mesh Attribute Region:1,Mesh Skin Region:2,Raw RD Buffer RID:3";
        props.push_back(rtype_prop);

        godot::Dictionary sidx_prop;
        sidx_prop["name"] = "surface_index";
        sidx_prop["type"] = godot::Variant::INT;
        props.push_back(sidx_prop);

        godot::Dictionary boff_prop;
        boff_prop["name"] = "byte_offset";
        boff_prop["type"] = godot::Variant::INT;
        props.push_back(boff_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return source_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && source_buffer_id == INVALID_ID) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && source_buffer_id == INVALID_ID) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("mesh_or_vertex_buffer_rid")) {
            const godot::Variant v = p_props["mesh_or_vertex_buffer_rid"];
            if (v.get_type() == godot::Variant::RID) {
                mesh_or_vertex_buffer_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&mesh_or_vertex_buffer_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("target_region_type")) {
            target_region_type = static_cast<RDVertexRegionTargetType>(std::clamp<int64_t>(static_cast<int64_t>(p_props["target_region_type"]), 0, 3));
        }
        if (p_props.has("surface_index")) {
            surface_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["surface_index"]));
        }
        if (p_props.has("byte_offset")) {
            byte_offset = std::max<int32_t>(0, static_cast<int32_t>(p_props["byte_offset"]));
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
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!mesh_or_vertex_buffer_rid.is_valid()) return;

            const GrantPartPOD* src_part = p_context.get_grant_part(source_buffer_id);
            if (!src_part || !src_part->raw_base_ptr || src_part->capacity_bytes == 0) return;

            const uint32_t stride = std::max<uint32_t>(1, src_part->element_stride);
            const size_t active_bytes = (src_part->selection.element_count > 0)
                ? std::min<size_t>(src_part->capacity_bytes, static_cast<size_t>(src_part->selection.element_count) * stride)
                : src_part->capacity_bytes;
            if (active_bytes == 0) return;

            if (static_cast<size_t>(staging_bytes.size()) != active_bytes) {
                staging_bytes.resize(static_cast<int64_t>(active_bytes));
            }
            std::memcpy(staging_bytes.ptrw(), src_part->raw_base_ptr, active_bytes);

            if (target_region_type == RDVertexRegionTargetType::RAW_RD_BUFFER_RID) {
                godot::RenderingDevice* rd = p_context.manager ? p_context.manager->get_rendering_device() : nullptr;
                if (!rd && godot::RenderingServer::get_singleton()) {
                    rd = godot::RenderingServer::get_singleton()->get_rendering_device();
                }
                if (rd) {
                    rd->buffer_update(
                        mesh_or_vertex_buffer_rid,
                        static_cast<uint32_t>(byte_offset),
                        static_cast<uint32_t>(active_bytes),
                        staging_bytes
                    );
                }
                return;
            }

            godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
            if (!rs) return;

            if (target_region_type == RDVertexRegionTargetType::MESH_VERTEX_REGION) {
                rs->mesh_surface_update_vertex_region(mesh_or_vertex_buffer_rid, surface_index, byte_offset, staging_bytes);
            } else if (target_region_type == RDVertexRegionTargetType::MESH_ATTRIBUTE_REGION) {
                rs->mesh_surface_update_attribute_region(mesh_or_vertex_buffer_rid, surface_index, byte_offset, staging_bytes);
            } else if (target_region_type == RDVertexRegionTargetType::MESH_SKIN_REGION) {
                rs->mesh_surface_update_skin_region(mesh_or_vertex_buffer_rid, surface_index, byte_offset, staging_bytes);
            }
        }
    }
};

static_assert(sizeof(RDVertexAttributeBufferBridgeTransformLogic<>) == 64, "RDVertexAttributeBufferBridgeTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<RDVertexAttributeBufferBridgeTransformLogic<>>, "RDVertexAttributeBufferBridgeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core