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
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/rid.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * RDComputeDispatchIndirectPrepTransformLogic<T> (G_RD4)
 * Computes a 16-byte Vulkan/RD DispatchIndirectCommand { groups_x, groups_y, groups_z, element_count }
 * from the active selection element_count of source_buffer_id and writes it to indirect_command_buffer_id
 * (and optionally uploads it to indirect_buffer_rid via RenderingDevice::buffer_update).
 */
template <typename T = int32_t>
struct alignas(64) RDComputeDispatchIndirectPrepTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    struct DispatchIndirectCmdPod {
        uint32_t groups_x;
        uint32_t groups_y;
        uint32_t groups_z;
        uint32_t reserved_pad;
    };

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::INT32 | DataType::BYTE;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD Compute DispatchIndirect Command Prep";
    static constexpr std::string_view ui_category  = "GPU/Dispatch";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t source_buffer_id           = INVALID_ID; // 4B: Buffer whose selection.element_count drives workgroup count
    uint32_t indirect_command_buffer_id = INVALID_ID; // 4B: Destination 16B DispatchIndirectCmdPod DOD buffer
    int32_t local_size_x                = 64;         // 4B: Compute shader local_size_x
    int32_t local_size_y                = 1;          // 4B: Compute shader local_size_y
    int32_t local_size_z                = 1;          // 4B: Compute shader local_size_z
    int32_t fixed_groups_y              = 1;          // 4B: Dispatched workgroups along Y
    int32_t fixed_groups_z              = 1;          // 4B: Dispatched workgroups along Z
    uint32_t reserved_align             = 0;          // 4B -> 32B

    godot::RID indirect_buffer_rid;                   // 8B: Optional RenderingDevice indirect buffer RID -> 40B
    uint8_t reserved_padding[24]        = {0};        // 24B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary sbuf_prop;
        sbuf_prop["name"] = "source_buffer_id";
        sbuf_prop["type"] = godot::Variant::INT;
        sbuf_prop["hint_string"] = "buffer_option";
        props.push_back(sbuf_prop);

        godot::Dictionary ibuf_prop;
        ibuf_prop["name"] = "indirect_command_buffer_id";
        ibuf_prop["type"] = godot::Variant::INT;
        ibuf_prop["hint_string"] = "buffer_option";
        props.push_back(ibuf_prop);

        godot::Dictionary irid_prop;
        irid_prop["name"] = "indirect_buffer_rid";
        irid_prop["type"] = godot::Variant::RID;
        props.push_back(irid_prop);

        godot::Dictionary lx_prop;
        lx_prop["name"] = "local_size_x";
        lx_prop["type"] = godot::Variant::INT;
        lx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        lx_prop["hint_string"] = "1,1024,1";
        props.push_back(lx_prop);

        godot::Dictionary ly_prop;
        ly_prop["name"] = "local_size_y";
        ly_prop["type"] = godot::Variant::INT;
        props.push_back(ly_prop);

        godot::Dictionary lz_prop;
        lz_prop["name"] = "local_size_z";
        lz_prop["type"] = godot::Variant::INT;
        props.push_back(lz_prop);

        godot::Dictionary gy_prop;
        gy_prop["name"] = "fixed_groups_y";
        gy_prop["type"] = godot::Variant::INT;
        props.push_back(gy_prop);

        godot::Dictionary gz_prop;
        gz_prop["name"] = "fixed_groups_z";
        gz_prop["type"] = godot::Variant::INT;
        props.push_back(gz_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (indirect_command_buffer_id != INVALID_ID) ? indirect_command_buffer_id : source_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        }
        if (p_props.has("indirect_command_buffer_id")) {
            indirect_command_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["indirect_command_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && indirect_command_buffer_id == INVALID_ID) {
            indirect_command_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("indirect_buffer_rid")) {
            const godot::Variant v = p_props["indirect_buffer_rid"];
            if (v.get_type() == godot::Variant::RID) {
                indirect_buffer_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&indirect_buffer_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("local_size_x")) {
            local_size_x = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_x"]));
        }
        if (p_props.has("local_size_y")) {
            local_size_y = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_y"]));
        }
        if (p_props.has("local_size_z")) {
            local_size_z = std::max<int32_t>(1, static_cast<int32_t>(p_props["local_size_z"]));
        }
        if (p_props.has("fixed_groups_y")) {
            fixed_groups_y = std::max<int32_t>(1, static_cast<int32_t>(p_props["fixed_groups_y"]));
        }
        if (p_props.has("fixed_groups_z")) {
            fixed_groups_z = std::max<int32_t>(1, static_cast<int32_t>(p_props["fixed_groups_z"]));
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
        if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint8_t>) {
            const GrantPartPOD* src_part = p_context.get_grant_part(source_buffer_id != INVALID_ID ? source_buffer_id : indirect_command_buffer_id);
            const GrantPartPOD* cmd_part = p_context.get_grant_part(indirect_command_buffer_id != INVALID_ID ? indirect_command_buffer_id : source_buffer_id);
            if (!src_part) return;

            const int64_t active_elems = std::max<int64_t>(0, src_part->selection.element_count);
            const int64_t lx = std::max<int64_t>(1, local_size_x);
            const uint32_t gx = (active_elems > 0)
                ? static_cast<uint32_t>((active_elems + lx - 1) / lx)
                : 0u;

            const DispatchIndirectCmdPod cmd{
                gx,
                (gx > 0) ? static_cast<uint32_t>(fixed_groups_y) : 0u,
                (gx > 0) ? static_cast<uint32_t>(fixed_groups_z) : 0u,
                static_cast<uint32_t>(active_elems)
            };

            if (cmd_part && cmd_part->raw_base_ptr && cmd_part->capacity_bytes >= sizeof(DispatchIndirectCmdPod)) {
                std::memcpy(cmd_part->raw_base_ptr, &cmd, sizeof(DispatchIndirectCmdPod));
            }

            if (indirect_buffer_rid.is_valid()) {
                godot::RenderingDevice* rd = p_context.manager ? p_context.manager->get_rendering_device() : nullptr;
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
    }
};

static_assert(sizeof(RDComputeDispatchIndirectPrepTransformLogic<>) == 64, "RDComputeDispatchIndirectPrepTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<RDComputeDispatchIndirectPrepTransformLogic<>>, "RDComputeDispatchIndirectPrepTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core