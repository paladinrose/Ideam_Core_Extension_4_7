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
 * G_RD4: RDComputeDispatchIndirectPrepTransformLogic
 * Computes a 16-byte Vulkan/RD DispatchIndirectCommand { groups_x, groups_y, groups_z, 0 }
 * from the active selection element_count of Part 0 and writes it to Part 1 (and optionally
 * uploads it to indirect_buffer_rid via RenderingDevice::buffer_update).
 */
struct RDComputeDispatchIndirectPrepTransformLogic {
    static constexpr std::string_view ui_name = "RD Compute DispatchIndirect Command Prep";
    static constexpr std::string_view ui_category = "GPU/Dispatch";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 1> supported_strategies = {
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 2> supported_types = {
        DataType::UINT32,
        DataType::INT32
    };

    struct DispatchIndirectCmdPod {
        uint32_t groups_x;
        uint32_t groups_y;
        uint32_t groups_z;
        uint32_t reserved_pad;
    };

    godot::RID indirect_buffer_rid;
    int32_t local_size_x = 64;
    int32_t local_size_y = 1;
    int32_t local_size_z = 1;
    int32_t fixed_groups_y = 1;
    int32_t fixed_groups_z = 1;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("indirect_buffer_rid", godot::Variant::RID, indirect_buffer_rid);
        add_prop("local_size_x", godot::Variant::INT, local_size_x);
        add_prop("local_size_y", godot::Variant::INT, local_size_y);
        add_prop("local_size_z", godot::Variant::INT, local_size_z);
        add_prop("fixed_groups_y", godot::Variant::INT, fixed_groups_y);
        add_prop("fixed_groups_z", godot::Variant::INT, fixed_groups_z);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("indirect_buffer_rid")) {
            godot::Variant v = p_props["indirect_buffer_rid"];
            if (v.get_type() == godot::Variant::RID) {
                indirect_buffer_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&indirect_buffer_rid)) = raw;
            }
        }
        if (p_props.has("local_size_x")) local_size_x = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_x"]));
        if (p_props.has("local_size_y")) local_size_y = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_y"]));
        if (p_props.has("local_size_z")) local_size_z = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_z"]));
        if (p_props.has("fixed_groups_y")) fixed_groups_y = std::max<int32_t>(1, static_cast<int32_t>(p_props["fixed_groups_y"]));
        if (p_props.has("fixed_groups_z")) fixed_groups_z = std::max<int32_t>(1, static_cast<int32_t>(p_props["fixed_groups_z"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        const GrantPartPOD& src_part = p_ctx.grant->parts[0];
        GrantPartPOD& cmd_part = (p_ctx.grant->part_count > 1) ? p_ctx.grant->parts[1] : p_ctx.grant->parts[0];

        const int64_t active_elems = std::max<int64_t>(0, src_part.selection.element_count);
        const int64_t lx = std::max<int64_t>(1, local_size_x);
        const uint32_t gx = (active_elems > 0)
            ? static_cast<uint32_t>((active_elems + lx - 1) / lx)
            : 0u;

        DispatchIndirectCmdPod cmd{
            gx,
            (gx > 0) ? static_cast<uint32_t>(fixed_groups_y) : 0u,
            (gx > 0) ? static_cast<uint32_t>(fixed_groups_z) : 0u,
            static_cast<uint32_t>(active_elems)
        };

        if (cmd_part.raw_base_ptr && cmd_part.capacity_bytes >= sizeof(DispatchIndirectCmdPod)) {
            std::memcpy(cmd_part.raw_base_ptr, &cmd, sizeof(DispatchIndirectCmdPod));
        }

        if (indirect_buffer_rid.is_valid()) {
            godot::RenderingDevice* rd = p_ctx.manager ? p_ctx.manager->get_rendering_device() : nullptr;
            if (!rd && godot::RenderingServer::get_singleton()) {
                rd = godot::RenderingServer::get_singleton()->get_rendering_device();
            }
            if (rd) {
                godot::PackedByteArray bytes;
                bytes.resize(sizeof(DispatchIndirectCmdPod));
                std::memcpy(bytes.ptrw(), &cmd, sizeof(DispatchIndirectCmdPod));
                rd->buffer_update(indirect_buffer_rid, 0, sizeof(DispatchIndirectCmdPod), bytes);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core