#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/image.hpp>
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

/**
 * ImageTexture2DServerUpdateTransformLogic<T, T_Strategy> (I8)
 * Zero-reallocation commit from a 2D DOD pixel buffer (RGBA8, RGBAF, or RF) directly into a
 * cached godot::Image and RenderingServer::texture_2d_update(texture_rid, image, layer).
 */
template <typename T = godot::Color, typename T_Strategy = Spatial2DStrategy>
struct alignas(64) ImageTexture2DServerUpdateTransformLogic {
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

    static constexpr std::string_view display_name = "Image RenderingServer Texture2D Update";
    static constexpr std::string_view ui_category  = "Image";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t image_buffer_id = INVALID_ID; // 4B: Source 2D pixel buffer
    int32_t image_width      = 256;        // 4B: Texture width in pixels
    int32_t image_height     = 256;        // 4B: Texture height in pixels
    int32_t texture_layer    = 0;          // 4B: Texture2DArray / Cubemap layer index

    godot::RID texture_rid;                           // 8B: Target RenderingServer texture RID
    mutable godot::Ref<godot::Image> target_image;    // 8B: Cached Image reference (also used for Task preview snapshots)
    mutable godot::PackedByteArray staging_bytes;     // 16B: Reusable byte buffer to avoid per-frame allocations
    uint8_t reserved_padding[16] = {0};               // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "image_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary rid_prop;
        rid_prop["name"] = "texture_rid";
        rid_prop["type"] = godot::Variant::RID;
        props.push_back(rid_prop);

        godot::Dictionary w_prop;
        w_prop["name"] = "image_width";
        w_prop["type"] = godot::Variant::INT;
        w_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        w_prop["hint_string"] = "1,8192,1,suffix:px";
        props.push_back(w_prop);

        godot::Dictionary h_prop;
        h_prop["name"] = "image_height";
        h_prop["type"] = godot::Variant::INT;
        h_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        h_prop["hint_string"] = "1,8192,1,suffix:px";
        props.push_back(h_prop);

        godot::Dictionary lyr_prop;
        lyr_prop["name"] = "texture_layer";
        lyr_prop["type"] = godot::Variant::INT;
        lyr_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        lyr_prop["hint_string"] = "0,256,1";
        props.push_back(lyr_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return image_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("image_buffer_id")) {
            image_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["image_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && image_buffer_id == INVALID_ID) {
            image_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && image_buffer_id == INVALID_ID) {
            image_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("texture_rid")) {
            const godot::Variant v = p_props["texture_rid"];
            if (v.get_type() == godot::Variant::RID) {
                texture_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&texture_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("image_width")) {
            image_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_width"]));
        }
        if (p_props.has("image_height")) {
            image_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_height"]));
        }
        if (p_props.has("texture_layer")) {
            texture_layer = std::max<int32_t>(0, static_cast<int32_t>(p_props["texture_layer"]));
        }
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& /*p_context*/, const GrantPartPOD* p_part) const noexcept {
        if constexpr (requires { r_view.strategy.stride_y; }) {
            const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
            r_view.strategy.stride_y = static_cast<int64_t>(std::max<int32_t>(1, image_width)) * static_cast<int64_t>(stride);
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
        if constexpr (std::is_same_v<T, godot::Color> || std::is_same_v<T, godot::Vector4> ||
                      std::is_same_v<T, float> || std::is_same_v<T, uint8_t>) {
            if (!texture_rid.is_valid()) return;

            const GrantPartPOD* img_part = p_context.get_grant_part(image_buffer_id);
            if (!img_part || !img_part->raw_base_ptr) return;

            godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
            if (!rs) return;

            const int32_t w = std::max<int32_t>(1, image_width);
            const int32_t h = std::max<int32_t>(1, image_height);
            const size_t total_pixels = static_cast<size_t>(w) * static_cast<size_t>(h);

            const bool is_color = (img_part->element_stride >= sizeof(godot::Color));
            const godot::Image::Format fmt = is_color ? godot::Image::FORMAT_RGBAF : godot::Image::FORMAT_RF;
            const size_t byte_count = total_pixels * (is_color ? sizeof(godot::Color) : sizeof(float));
            if (img_part->capacity_bytes < byte_count) return;

            if (static_cast<size_t>(staging_bytes.size()) != byte_count) {
                staging_bytes.resize(static_cast<int64_t>(byte_count));
            }
            std::memcpy(staging_bytes.ptrw(), img_part->raw_base_ptr, byte_count);

            if (target_image.is_null() || target_image->get_width() != w || target_image->get_height() != h || target_image->get_format() != fmt) {
                target_image = godot::Image::create_from_data(w, h, false, fmt, staging_bytes);
            } else {
                target_image->set_data(w, h, false, fmt, staging_bytes);
            }

            if (target_image.is_valid()) {
                rs->texture_2d_update(texture_rid, target_image, texture_layer);
            }
        }
    }
};

static_assert(sizeof(ImageTexture2DServerUpdateTransformLogic<>) == 64, "ImageTexture2DServerUpdateTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ImageTexture2DServerUpdateTransformLogic<>>, "ImageTexture2DServerUpdateTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core