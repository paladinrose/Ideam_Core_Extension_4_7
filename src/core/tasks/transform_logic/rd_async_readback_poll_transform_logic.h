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
 * G_RD8: RDAsyncReadbackPollTransformLogic
 * Manages frame-cadence throttled and ring-buffered GPU-to-CPU readbacks from a RenderingDevice
 * storage buffer or texture RID into a DOD Ring or Flat buffer so CPU consumers read recent
 * GPU state without stalling every frame.
 */
struct RDAsyncReadbackPollTransformLogic {
    static constexpr std::string_view ui_name = "RD Ring-Buffered GPU Readback Poll";
    static constexpr std::string_view ui_category = "GPU/Buffers";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::RING,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 1> supported_strategies = {
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 4> supported_types = {
        DataType::FLOAT32,
        DataType::INT32,
        DataType::VECTOR3,
        DataType::VECTOR4
    };

    godot::RID source_gpu_rid;
    int32_t source_resource_type = 0; // 0 = Storage Buffer (SSBO), 1 = Texture2DRD / Texture3DRD Layer
    int32_t poll_interval_frames = 2;
    int32_t ring_slot_count = 3;
    int32_t readback_byte_size = 0; // 0 = auto from destination slot capacity

    mutable uint64_t frame_counter = 0;
    mutable int32_t current_ring_slot = 0;

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
        add_prop("source_gpu_rid", godot::Variant::RID, source_gpu_rid);
        add_prop("source_resource_type", godot::Variant::INT, source_resource_type, 2, "Storage Buffer (SSBO),Storage Texture Layer");
        add_prop("poll_interval_frames", godot::Variant::INT, poll_interval_frames);
        add_prop("ring_slot_count", godot::Variant::INT, ring_slot_count);
        add_prop("readback_byte_size", godot::Variant::INT, readback_byte_size);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("source_gpu_rid")) {
            godot::Variant v = p_props["source_gpu_rid"];
            if (v.get_type() == godot::Variant::RID) {
                source_gpu_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&source_gpu_rid)) = raw;
            }
        }
        if (p_props.has("source_resource_type")) source_resource_type = std::clamp<int32_t>(static_cast<int32_t>(p_props["source_resource_type"]), 0, 1);
        if (p_props.has("poll_interval_frames")) poll_interval_frames = std::max<int32_t>(1, static_cast<int32_t>(p_props["poll_interval_frames"]));
        if (p_props.has("ring_slot_count")) ring_slot_count = std::clamp<int32_t>(static_cast<int32_t>(p_props["ring_slot_count"]), 1, 8);
        if (p_props.has("readback_byte_size")) readback_byte_size = std::max<int32_t>(0, static_cast<int32_t>(p_props["readback_byte_size"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !source_gpu_rid.is_valid()) return;

        const uint64_t cur_frame = frame_counter++;
        if ((cur_frame % static_cast<uint64_t>(std::max<int32_t>(1, poll_interval_frames))) != 0) {
            return;
        }

        GrantPartPOD& dst_part = p_ctx.grant->parts[0];
        if (!dst_part.raw_base_ptr || dst_part.capacity_bytes == 0) return;

        godot::RenderingDevice* rd = p_ctx.manager ? p_ctx.manager->get_rendering_device() : nullptr;
        if (!rd && godot::RenderingServer::get_singleton()) {
            rd = godot::RenderingServer::get_singleton()->get_rendering_device();
        }
        if (!rd) return;

        const int32_t slots = std::max<int32_t>(1, ring_slot_count);
        const size_t slot_capacity = dst_part.capacity_bytes / static_cast<size_t>(slots);
        if (slot_capacity == 0) return;

        const size_t req_bytes = (readback_byte_size > 0)
            ? std::min<size_t>(static_cast<size_t>(readback_byte_size), slot_capacity)
            : slot_capacity;

        godot::PackedByteArray data;
        if (source_resource_type == 0) {
            data = rd->buffer_get_data(source_gpu_rid, 0, static_cast<uint32_t>(req_bytes));
        } else {
            data = rd->texture_get_data(source_gpu_rid, 0);
        }

        const size_t copy_len = std::min<size_t>(req_bytes, static_cast<size_t>(data.size()));
        if (copy_len == 0) return;

        // If backed by a MemoryManagerDOD RING buffer, push directly into the ring; otherwise write to rotating slot
        if (p_ctx.manager && !p_ctx.manager->is_ring_full(dst_part.buffer_id) &&
            p_ctx.manager->get_ring_available_write_bytes(dst_part.buffer_id) >= copy_len) {
            p_ctx.manager->ring_push(dst_part.buffer_id, data.ptr(), copy_len);
        } else {
            const int32_t slot = current_ring_slot % slots;
            current_ring_slot = (slot + 1) % slots;
            std::memcpy(dst_part.raw_base_ptr + static_cast<size_t>(slot) * slot_capacity, data.ptr(), copy_len);
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core