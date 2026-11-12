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
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class RDTextureSyncDirection : uint8_t {
    UPLOAD_CPU_TO_TEXTURE   = 0,
    READBACK_TEXTURE_TO_CPU = 1
};

/**
 * RDTexture2DRDBridgeTransformLogic<T, T_Strategy> (G_RD5)
 * Bridges 2D/3D spatial DOD buffers (RGBA32F Color or R32F float) and RenderingDevice
 * storage textures (Texture2DRD / Texture3DRD) via RenderingDevice::texture_update and
 * RenderingDevice::texture_get_data.
 */
template <typename T = godot::Color, typename T_Strategy = Spatial2DStrategy>
struct alignas(64) RDTexture2DRDBridgeTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL | BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::COLOR | DataType::VECTOR4 | DataType::FLOAT32 | DataType::BYTE;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD Texture2DRD / Texture3DRD Storage Bridge";
    static constexpr std::string_view ui_category  = "GPU/Bridge";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t texture_buffer_id            = INVALID_ID; // 4B: Primary target 2D/3D spatial DOD buffer
    int32_t texture_layer                 = 0;          // 4B: Layer / slice index
    int32_t buffer_width                  = 256;        // 4B: Spatial width for view stride configuration
    RDTextureSyncDirection sync_direction = RDTextureSyncDirection::UPLOAD_CPU_TO_TEXTURE; // 1B
    uint8_t reserved_flags[3]             = {0};        // 3B -> 16B

    godot::RID rd_texture_rid;                          // 8B -> 24B
    mutable godot::PackedByteArray staging_bytes;       // 16B -> 40B
    uint8_t reserved_padding[24]          = {0};        // 24B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "texture_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary rid_prop;
        rid_prop["name"] = "rd_texture_rid";
        rid_prop["type"] = godot::Variant::RID;
        props.push_back(rid_prop);

        godot::Dictionary dir_prop;
        dir_prop["name"] = "sync_direction";
        dir_prop["type"] = godot::Variant::INT;
        dir_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dir_prop["hint_string"] = "Upload (CPU -> RD Texture):0,Readback (RD Texture -> CPU):1";
        props.push_back(dir_prop);

        godot::Dictionary lyr_prop;
        lyr_prop["name"] = "texture_layer";
        lyr_prop["type"] = godot::Variant::INT;
        props.push_back(lyr_prop);

        godot::Dictionary w_prop;
        w_prop["name"] = "buffer_width";
        w_prop["type"] = godot::Variant::INT;
        props.push_back(w_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return texture_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("texture_buffer_id")) {
            texture_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["texture_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && texture_buffer_id == INVALID_ID) {
            texture_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && texture_buffer_id == INVALID_ID) {
            texture_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("rd_texture_rid")) {
            const godot::Variant v = p_props["rd_texture_rid"];
            if (v.get_type() == godot::Variant::RID) {
                rd_texture_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&rd_texture_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("sync_direction")) {
            sync_direction = static_cast<RDTextureSyncDirection>(std::clamp<int64_t>(static_cast<int64_t>(p_props["sync_direction"]), 0, 1));
        }
        if (p_props.has("texture_layer")) {
            texture_layer = std::max<int32_t>(0, static_cast<int32_t>(p_props["texture_layer"]));
        }
        if (p_props.has("buffer_width")) {
            buffer_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["buffer_width"]));
        }
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& /*p_context*/, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { r_view.strategy.stride_y; }) {
            const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
            r_view.strategy.stride_y = static_cast<int64_t>(std::max<int32_t>(1, buffer_width)) * static_cast<int64_t>(stride);
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            if (!rd_texture_rid.is_valid()) return;

            const GrantPartPOD* tex_part = p_context.get_grant_part(texture_buffer_id);
            if (!tex_part || !tex_part->raw_base_ptr || tex_part->capacity_bytes == 0) return;

            godot::RenderingDevice* rd = p_context.manager ? p_context.manager->get_rendering_device() : nullptr;
            if (!rd && godot::RenderingServer::get_singleton()) {
                rd = godot::RenderingServer::get_singleton()->get_rendering_device();
            }
            if (!rd) return;

            const uint32_t layer = static_cast<uint32_t>(std::max<int32_t>(0, texture_layer));

            if (sync_direction == RDTextureSyncDirection::UPLOAD_CPU_TO_TEXTURE) {
                if (static_cast<size_t>(staging_bytes.size()) != tex_part->capacity_bytes) {
                    staging_bytes.resize(static_cast<int64_t>(tex_part->capacity_bytes));
                }
                std::memcpy(staging_bytes.ptrw(), tex_part->raw_base_ptr, tex_part->capacity_bytes);
                rd->texture_update(rd_texture_rid, layer, staging_bytes);
            } else {
                const godot::PackedByteArray data = rd->texture_get_data(rd_texture_rid, layer);
                const size_t copy_bytes = std::min<size_t>(tex_part->capacity_bytes, static_cast<size_t>(data.size()));
                if (copy_bytes > 0) {
                    std::memcpy(tex_part->raw_base_ptr, data.ptr(), copy_bytes);
                }
            }
        }
    }
};

static_assert(sizeof(RDTexture2DRDBridgeTransformLogic<>) == 64, "RDTexture2DRDBridgeTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<RDTexture2DRDBridgeTransformLogic<>>, "RDTexture2DRDBridgeTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core