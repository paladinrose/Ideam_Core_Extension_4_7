#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/ring_view.h"
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
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class RDReadbackSourceType : uint8_t {
    STORAGE_BUFFER_SSBO   = 0,
    STORAGE_TEXTURE_LAYER = 1
};

/**
 * RDAsyncReadbackPollTransformLogic<T> (G_RD8)
 * Manages frame-cadence throttled and ring-buffered GPU-to-CPU readbacks from a RenderingDevice
 * storage buffer or texture RID into a DOD Ring or Flat buffer so CPU consumers read recent
 * GPU state without stalling every frame.
 */
template <typename T = float>
struct alignas(64) RDAsyncReadbackPollTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = RingView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::FLOAT32 | DataType::INT32 | DataType::VECTOR3 | DataType::VECTOR4 | DataType::BYTE;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD Ring-Buffered GPU Readback Poll";
    static constexpr std::string_view ui_category  = "GPU/Buffers";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t destination_buffer_id            = INVALID_ID; // 4B: Primary target CPU Ring/Flat buffer
    int32_t poll_interval_frames              = 2;          // 4B: Poll cadence in frames
    int32_t ring_slot_count                   = 3;          // 4B: Number of rotating slots when backed by Flat buffer
    int32_t readback_byte_size                = 0;          // 4B: Bytes per readback (0 = auto from slot capacity) -> 16B

    godot::RID source_gpu_rid;                              // 8B: Source RenderingDevice SSBO or Texture RID -> 24B
    mutable uint64_t frame_counter            = 0;          // 8B: Monotonic frame counter -> 32B
    mutable int32_t current_ring_slot         = 0;          // 4B: Current rotating slot index
    RDReadbackSourceType source_resource_type = RDReadbackSourceType::STORAGE_BUFFER_SSBO; // 1B
    uint8_t reserved_padding[27]              = {0};        // 27B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary dbuf_prop;
        dbuf_prop["name"] = "destination_buffer_id";
        dbuf_prop["type"] = godot::Variant::INT;
        dbuf_prop["hint_string"] = "buffer_option";
        props.push_back(dbuf_prop);

        godot::Dictionary rid_prop;
        rid_prop["name"] = "source_gpu_rid";
        rid_prop["type"] = godot::Variant::RID;
        props.push_back(rid_prop);

        godot::Dictionary rtype_prop;
        rtype_prop["name"] = "source_resource_type";
        rtype_prop["type"] = godot::Variant::INT;
        rtype_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        rtype_prop["hint_string"] = "Storage Buffer (SSBO):0,Storage Texture Layer:1";
        props.push_back(rtype_prop);

        godot::Dictionary poll_prop;
        poll_prop["name"] = "poll_interval_frames";
        poll_prop["type"] = godot::Variant::INT;
        poll_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        poll_prop["hint_string"] = "1,60,1";
        props.push_back(poll_prop);

        godot::Dictionary slt_prop;
        slt_prop["name"] = "ring_slot_count";
        slt_prop["type"] = godot::Variant::INT;
        slt_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        slt_prop["hint_string"] = "1,8,1";
        props.push_back(slt_prop);

        godot::Dictionary bsz_prop;
        bsz_prop["name"] = "readback_byte_size";
        bsz_prop["type"] = godot::Variant::INT;
        props.push_back(bsz_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return destination_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("destination_buffer_id")) {
            destination_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["destination_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && destination_buffer_id == INVALID_ID) {
            destination_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && destination_buffer_id == INVALID_ID) {
            destination_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("source_gpu_rid")) {
            const godot::Variant v = p_props["source_gpu_rid"];
            if (v.get_type() == godot::Variant::RID) {
                source_gpu_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&source_gpu_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("source_resource_type")) {
            source_resource_type = static_cast<RDReadbackSourceType>(std::clamp<int64_t>(static_cast<int64_t>(p_props["source_resource_type"]), 0, 1));
        }
        if (p_props.has("poll_interval_frames")) {
            poll_interval_frames = std::max<int32_t>(1, static_cast<int32_t>(p_props["poll_interval_frames"]));
        }
        if (p_props.has("ring_slot_count")) {
            ring_slot_count = std::clamp<int32_t>(static_cast<int32_t>(p_props["ring_slot_count"]), 1, 8);
        }
        if (p_props.has("readback_byte_size")) {
            readback_byte_size = std::max<int32_t>(0, static_cast<int32_t>(p_props["readback_byte_size"]));
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
            if (!source_gpu_rid.is_valid()) return;

            const uint64_t cur_frame = frame_counter++;
            if ((cur_frame % static_cast<uint64_t>(std::max<int32_t>(1, poll_interval_frames))) != 0) {
                return;
            }

            const GrantPartPOD* dst_part = p_context.get_grant_part(destination_buffer_id);
            if (!dst_part || !dst_part->raw_base_ptr || dst_part->capacity_bytes == 0) return;

            godot::RenderingDevice* rd = p_context.manager ? p_context.manager->get_rendering_device() : nullptr;
            if (!rd && godot::RenderingServer::get_singleton()) {
                rd = godot::RenderingServer::get_singleton()->get_rendering_device();
            }
            if (!rd) return;

            const int32_t slots = std::max<int32_t>(1, ring_slot_count);
            const size_t slot_capacity = dst_part->capacity_bytes / static_cast<size_t>(slots);
            if (slot_capacity == 0) return;

            const size_t req_bytes = (readback_byte_size > 0)
                ? std::min<size_t>(static_cast<size_t>(readback_byte_size), slot_capacity)
                : slot_capacity;

            godot::PackedByteArray data;
            if (source_resource_type == RDReadbackSourceType::STORAGE_BUFFER_SSBO) {
                data = rd->buffer_get_data(source_gpu_rid, 0, static_cast<uint32_t>(req_bytes));
            } else {
                data = rd->texture_get_data(source_gpu_rid, 0);
            }

            const size_t copy_len = std::min<size_t>(req_bytes, static_cast<size_t>(data.size()));
            if (copy_len == 0) return;

            if (p_context.manager && !p_context.manager->is_ring_full(dst_part->buffer_id) &&
                p_context.manager->get_ring_available_write_bytes(dst_part->buffer_id) >= copy_len) {
                p_context.manager->ring_push(dst_part->buffer_id, data.ptr(), copy_len);
            } else {
                const int32_t slot = current_ring_slot % slots;
                current_ring_slot = (slot + 1) % slots;
                std::memcpy(dst_part->raw_base_ptr + static_cast<size_t>(slot) * slot_capacity, data.ptr(), copy_len);
            }
        }
    }
};

static_assert(sizeof(RDAsyncReadbackPollTransformLogic<>) == 64, "RDAsyncReadbackPollTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<RDAsyncReadbackPollTransformLogic<>>, "RDAsyncReadbackPollTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core