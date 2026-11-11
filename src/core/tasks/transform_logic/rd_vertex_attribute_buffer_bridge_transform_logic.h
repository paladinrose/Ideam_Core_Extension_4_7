#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * G_RD6: RDVertexAttributeBufferBridgeTransformLogic
 * Commits DOD / GPU compute output buffers directly to mesh vertex, attribute, or skin
 * region buffers via RenderingServer::mesh_surface_update_vertex_region /
 * mesh_surface_update_attribute_region or RenderingDevice::buffer_update on vertex storage RIDs.
 */
struct RDVertexAttributeBufferBridgeTransformLogic {
    static constexpr std::string_view ui_name = "RD Mesh Vertex & Attribute Buffer Bridge";
    static constexpr std::string_view ui_category = "GPU/Bridge";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 4> supported_types = {
        DataType::VECTOR3,
        DataType::VECTOR4,
        DataType::COLOR,
        DataType::FLOAT32
    };

    godot::RID mesh_or_vertex_buffer_rid;
    int32_t target_region_type = 0; // 0 = Mesh Vertex Region, 1 = Mesh Attribute Region, 2 = Mesh Skin Region, 3 = Raw RD Buffer RID
    int32_t surface_index = 0;
    int32_t byte_offset = 0;

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
        add_prop("mesh_or_vertex_buffer_rid", godot::Variant::RID, mesh_or_vertex_buffer_rid);
        add_prop("target_region_type", godot::Variant::INT, target_region_type, 2, "Mesh Vertex Region,Mesh Attribute Region,Mesh Skin Region,Raw RD Buffer RID");
        add_prop("surface_index", godot::Variant::INT, surface_index);
        add_prop("byte_offset", godot::Variant::INT, byte_offset);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("mesh_or_vertex_buffer_rid")) {
            godot::Variant v = p_props["mesh_or_vertex_buffer_rid"];
            if (v.get_type() == godot::Variant::RID) {
                mesh_or_vertex_buffer_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&mesh_or_vertex_buffer_rid)) = raw;
            }
        }
        if (p_props.has("target_region_type")) target_region_type = std::clamp<int32_t>(static_cast<int32_t>(p_props["target_region_type"]), 0, 3);
        if (p_props.has("surface_index")) surface_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["surface_index"]));
        if (p_props.has("byte_offset")) byte_offset = std::max<int32_t>(0, static_cast<int32_t>(p_props["byte_offset"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !mesh_or_vertex_buffer_rid.is_valid()) return;

        const GrantPartPOD& src_part = p_ctx.grant->parts[0];
        if (!src_part.raw_base_ptr || src_part.capacity_bytes == 0) return;

        const uint32_t stride = std::max<uint32_t>(1, src_part.element_stride);
        const size_t active_bytes = (src_part.selection.element_count > 0)
            ? std::min<size_t>(src_part.capacity_bytes, static_cast<size_t>(src_part.selection.element_count) * stride)
            : src_part.capacity_bytes;
        if (active_bytes == 0) return;

        godot::PackedByteArray payload;
        payload.resize(static_cast<int64_t>(active_bytes));
        std::memcpy(payload.ptrw(), src_part.raw_base_ptr, active_bytes);

        if (target_region_type == 3) {
            godot::RenderingDevice* rd = p_ctx.manager ? p_ctx.manager->get_rendering_device() : nullptr;
            if (!rd && godot::RenderingServer::get_singleton()) {
                rd = godot::RenderingServer::get_singleton()->get_rendering_device();
            }
            if (rd) {
                rd->buffer_update(
                    mesh_or_vertex_buffer_rid,
                    static_cast<uint32_t>(byte_offset),
                    static_cast<uint32_t>(active_bytes),
                    payload
                );
            }
            return;
        }

        godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
        if (!rs) return;

        if (target_region_type == 0) {
            rs->mesh_surface_update_vertex_region(mesh_or_vertex_buffer_rid, surface_index, byte_offset, payload);
        } else if (target_region_type == 1) {
            rs->mesh_surface_update_attribute_region(mesh_or_vertex_buffer_rid, surface_index, byte_offset, payload);
        } else if (target_region_type == 2) {
            rs->mesh_surface_update_skin_region(mesh_or_vertex_buffer_rid, surface_index, byte_offset, payload);
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core