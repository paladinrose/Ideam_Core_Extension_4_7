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
 * G_RD1: RDStorageBufferSyncTransformLogic
 * Explicit upload (CPU -> VRAM SSBO via RenderingDevice::buffer_update) or readback
 * (VRAM SSBO -> CPU via RenderingDevice::buffer_get_data) between GrantPartPOD slices and
 * RenderingDevice storage buffers.
 */
struct RDStorageBufferSyncTransformLogic {
    static constexpr std::string_view ui_name = "RD Storage Buffer (SSBO) Upload/Readback Sync";
    static constexpr std::string_view ui_category = "GPU/Buffers";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 4> supported_types = {
        DataType::FLOAT32,
        DataType::INT32,
        DataType::VECTOR3,
        DataType::VECTOR4
    };

    godot::RID storage_buffer_rid;
    int32_t sync_direction = 0; // 0 = Upload CPU -> GPU SSBO, 1 = Readback GPU SSBO -> CPU
    int32_t byte_offset = 0;
    int32_t byte_count = 0; // 0 = full active grant capacity

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
        add_prop("storage_buffer_rid", godot::Variant::RID, storage_buffer_rid);
        add_prop("sync_direction", godot::Variant::INT, sync_direction, 2, "Upload (CPU -> GPU SSBO),Readback (GPU SSBO -> CPU)");
        add_prop("byte_offset", godot::Variant::INT, byte_offset);
        add_prop("byte_count", godot::Variant::INT, byte_count);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("storage_buffer_rid")) {
            godot::Variant v = p_props["storage_buffer_rid"];
            if (v.get_type() == godot::Variant::RID) {
                storage_buffer_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&storage_buffer_rid)) = raw;
            }
        }
        if (p_props.has("sync_direction")) sync_direction = std::clamp<int32_t>(static_cast<int32_t>(p_props["sync_direction"]), 0, 1);
        if (p_props.has("byte_offset")) byte_offset = std::max<int32_t>(0, static_cast<int32_t>(p_props["byte_offset"]));
        if (p_props.has("byte_count")) byte_count = std::max<int32_t>(0, static_cast<int32_t>(p_props["byte_count"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& part = p_ctx.grant->parts[0];
        if (!part.raw_base_ptr || part.capacity_bytes == 0) return;

        godot::RenderingDevice* rd = p_ctx.manager ? p_ctx.manager->get_rendering_device() : nullptr;
        if (!rd && godot::RenderingServer::get_singleton()) {
            rd = godot::RenderingServer::get_singleton()->get_rendering_device();
        }
        if (!rd) return;

        godot::RID target_rid = storage_buffer_rid;
        if (!target_rid.is_valid() && p_ctx.grant->part_count > 1 && p_ctx.grant->parts[1].raw_base_ptr &&
            p_ctx.grant->parts[1].capacity_bytes >= sizeof(uint64_t)) {
            const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(p_ctx.grant->parts[1].raw_base_ptr);
            if (raw_id != 0) {
                *(reinterpret_cast<uint64_t*>(&target_rid)) = raw_id;
            }
        }
        if (!target_rid.is_valid()) return;

        const size_t offset = std::min<size_t>(static_cast<size_t>(byte_offset), part.capacity_bytes);
        const size_t max_avail = part.capacity_bytes - offset;
        const size_t transfer_bytes = (byte_count > 0)
            ? std::min<size_t>(static_cast<size_t>(byte_count), max_avail)
            : max_avail;
        if (transfer_bytes == 0) return;

        if (sync_direction == 0) {
            godot::PackedByteArray payload;
            payload.resize(static_cast<int64_t>(transfer_bytes));
            std::memcpy(payload.ptrw(), part.raw_base_ptr + offset, transfer_bytes);
            rd->buffer_update(target_rid, static_cast<uint32_t>(offset), static_cast<uint32_t>(transfer_bytes), payload);
        } else {
            godot::PackedByteArray readback = rd->buffer_get_data(
                target_rid,
                static_cast<uint32_t>(offset),
                static_cast<uint32_t>(transfer_bytes)
            );
            const size_t copy_len = std::min<size_t>(transfer_bytes, static_cast<size_t>(readback.size()));
            if (copy_len > 0) {
                std::memcpy(part.raw_base_ptr + offset, readback.ptr(), copy_len);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core