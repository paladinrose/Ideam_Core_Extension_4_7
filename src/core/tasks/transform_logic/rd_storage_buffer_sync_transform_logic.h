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
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class RDStorageSyncDirection : uint8_t {
    UPLOAD_CPU_TO_SSBO   = 0,
    READBACK_SSBO_TO_CPU = 1
};

/**
 * RDStorageBufferSyncTransformLogic<T> (G_RD1)
 * Explicit upload (CPU -> VRAM SSBO via RenderingDevice::buffer_update) or readback
 * (VRAM SSBO -> CPU via RenderingDevice::buffer_get_data) between GrantPartPOD slices and
 * RenderingDevice storage buffers.
 */
template <typename T = float>
struct alignas(64) RDStorageBufferSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

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

    static constexpr std::string_view display_name = "RD Storage Buffer (SSBO) Upload/Readback Sync";
    static constexpr std::string_view ui_category  = "GPU/Buffers";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t cpu_buffer_id    = INVALID_ID; // 4B: Primary target CPU DOD buffer
    uint32_t rid_buffer_id    = INVALID_ID; // 4B: Optional DOD buffer storing a uint64_t SSBO RID handle
    int32_t byte_offset       = 0;          // 4B: Byte offset inside SSBO and CPU buffer
    int32_t byte_count        = 0;          // 4B: Bytes to transfer (0 = full active grant capacity)

    godot::RID storage_buffer_rid;                        // 8B: Target RenderingDevice SSBO RID
    mutable godot::PackedByteArray staging_bytes;         // 16B: Reusable byte buffer for upload
    RDStorageSyncDirection sync_direction = RDStorageSyncDirection::UPLOAD_CPU_TO_SSBO; // 1B
    uint8_t reserved_padding[23]          = {0};          // 23B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary cbuf_prop;
        cbuf_prop["name"] = "cpu_buffer_id";
        cbuf_prop["type"] = godot::Variant::INT;
        cbuf_prop["hint_string"] = "buffer_option";
        props.push_back(cbuf_prop);

        godot::Dictionary rbuf_prop;
        rbuf_prop["name"] = "rid_buffer_id";
        rbuf_prop["type"] = godot::Variant::INT;
        rbuf_prop["hint_string"] = "buffer_option";
        props.push_back(rbuf_prop);

        godot::Dictionary srid_prop;
        srid_prop["name"] = "storage_buffer_rid";
        srid_prop["type"] = godot::Variant::RID;
        props.push_back(srid_prop);

        godot::Dictionary dir_prop;
        dir_prop["name"] = "sync_direction";
        dir_prop["type"] = godot::Variant::INT;
        dir_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dir_prop["hint_string"] = "Upload (CPU -> GPU SSBO):0,Readback (GPU SSBO -> CPU):1";
        props.push_back(dir_prop);

        godot::Dictionary off_prop;
        off_prop["name"] = "byte_offset";
        off_prop["type"] = godot::Variant::INT;
        props.push_back(off_prop);

        godot::Dictionary cnt_prop;
        cnt_prop["name"] = "byte_count";
        cnt_prop["type"] = godot::Variant::INT;
        props.push_back(cnt_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (cpu_buffer_id != INVALID_ID) ? cpu_buffer_id : rid_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("cpu_buffer_id")) {
            cpu_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["cpu_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && cpu_buffer_id == INVALID_ID) {
            cpu_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && cpu_buffer_id == INVALID_ID) {
            cpu_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("rid_buffer_id")) {
            rid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["rid_buffer_id"]));
        }
        if (p_props.has("storage_buffer_rid")) {
            const godot::Variant v = p_props["storage_buffer_rid"];
            if (v.get_type() == godot::Variant::RID) {
                storage_buffer_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&storage_buffer_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("sync_direction")) {
            sync_direction = static_cast<RDStorageSyncDirection>(std::clamp<int64_t>(static_cast<int64_t>(p_props["sync_direction"]), 0, 1));
        }
        if (p_props.has("byte_offset")) {
            byte_offset = std::max<int32_t>(0, static_cast<int32_t>(p_props["byte_offset"]));
        }
        if (p_props.has("byte_count")) {
            byte_count = std::max<int32_t>(0, static_cast<int32_t>(p_props["byte_count"]));
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
            const GrantPartPOD* part = p_context.get_grant_part(cpu_buffer_id);
            if (!part || !part->raw_base_ptr || part->capacity_bytes == 0) return;

            godot::RenderingDevice* rd = p_context.manager ? p_context.manager->get_rendering_device() : nullptr;
            if (!rd && godot::RenderingServer::get_singleton()) {
                rd = godot::RenderingServer::get_singleton()->get_rendering_device();
            }
            if (!rd) return;

            godot::RID target_rid = storage_buffer_rid;
            if (!target_rid.is_valid() && rid_buffer_id != INVALID_ID) {
                if (const GrantPartPOD* r_part = p_context.get_grant_part(rid_buffer_id)) {
                    if (r_part->raw_base_ptr && r_part->capacity_bytes >= sizeof(uint64_t)) {
                        const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(r_part->raw_base_ptr);
                        if (raw_id != 0) {
                            std::memcpy(&target_rid, &raw_id, sizeof(uint64_t));
                        }
                    }
                }
            }
            if (!target_rid.is_valid()) return;

            const size_t offset = std::min<size_t>(static_cast<size_t>(byte_offset), part->capacity_bytes);
            const size_t max_avail = part->capacity_bytes - offset;
            const size_t transfer_bytes = (byte_count > 0)
                ? std::min<size_t>(static_cast<size_t>(byte_count), max_avail)
                : max_avail;
            if (transfer_bytes == 0) return;

            if (sync_direction == RDStorageSyncDirection::UPLOAD_CPU_TO_SSBO) {
                if (static_cast<size_t>(staging_bytes.size()) != transfer_bytes) {
                    staging_bytes.resize(static_cast<int64_t>(transfer_bytes));
                }
                std::memcpy(staging_bytes.ptrw(), part->raw_base_ptr + offset, transfer_bytes);
                rd->buffer_update(target_rid, static_cast<uint32_t>(offset), static_cast<uint32_t>(transfer_bytes), staging_bytes);
            } else {
                const godot::PackedByteArray readback = rd->buffer_get_data(
                    target_rid,
                    static_cast<uint32_t>(offset),
                    static_cast<uint32_t>(transfer_bytes)
                );
                const size_t copy_len = std::min<size_t>(transfer_bytes, static_cast<size_t>(readback.size()));
                if (copy_len > 0) {
                    std::memcpy(part->raw_base_ptr + offset, readback.ptr(), copy_len);
                }
            }
        }
    }
};

static_assert(sizeof(RDStorageBufferSyncTransformLogic<>) == 64, "RDStorageBufferSyncTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<RDStorageBufferSyncTransformLogic<>>, "RDStorageBufferSyncTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core