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
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ShaderDataTexturePackTransformLogic<T> (S5)
 * Packs 1D/2D entity attribute arrays (Vector3 + scalar or Vector4 columns) into a FORMAT_RGBAF
 * 2D data texture buffer and commits it via RenderingServer::texture_2d_update for vertex-shader
 * texelFetch crowd/foliage/particle animation.
 */
template <typename T = godot::Vector4>
struct alignas(64) ShaderDataTexturePackTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR | BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              =
        DataType::VECTOR4 | DataType::COLOR | DataType::VECTOR3 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Shader texelFetch Data Texture Pack";
    static constexpr std::string_view ui_category  = "Shader/Packing";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t xyz_buffer_id           = INVALID_ID; // 4B: Source Vector3 or Vector4 attribute buffer
    uint32_t w_scalar_buffer_id      = INVALID_ID; // 4B: Optional scalar float column for W channel
    uint32_t texture_buffer_id       = INVALID_ID; // 4B: Destination Vector4 / Color data texture buffer
    int32_t texture_width            = 256;        // 4B: Data texture width in texels
    int32_t texture_height           = 64;         // 4B: Data texture height in texels
    float default_w_component        = 1.0f;       // 4B: Fallback W value when w_scalar_buffer_id is unbound

    godot::RID data_texture_rid;                           // 8B: Target RenderingServer Texture2D RID
    mutable godot::Ref<godot::Image> target_image;         // 8B: Cached Image reference for snapshot/upload
    mutable godot::PackedByteArray staging_bytes;          // 16B: Reusable staging buffer
    bool commit_to_rendering_server  = true;               // 1B
    uint8_t reserved_padding[7]      = {0};                // 7B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary xyz_prop;
        xyz_prop["name"] = "xyz_buffer_id";
        xyz_prop["type"] = godot::Variant::INT;
        xyz_prop["hint_string"] = "buffer_option";
        props.push_back(xyz_prop);

        godot::Dictionary w_prop;
        w_prop["name"] = "w_scalar_buffer_id";
        w_prop["type"] = godot::Variant::INT;
        w_prop["hint_string"] = "buffer_option";
        props.push_back(w_prop);

        godot::Dictionary tbuf_prop;
        tbuf_prop["name"] = "texture_buffer_id";
        tbuf_prop["type"] = godot::Variant::INT;
        tbuf_prop["hint_string"] = "buffer_option";
        props.push_back(tbuf_prop);

        godot::Dictionary rid_prop;
        rid_prop["name"] = "data_texture_rid";
        rid_prop["type"] = godot::Variant::RID;
        props.push_back(rid_prop);

        godot::Dictionary tw_prop;
        tw_prop["name"] = "texture_width";
        tw_prop["type"] = godot::Variant::INT;
        tw_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        tw_prop["hint_string"] = "1,4096,1,suffix:px";
        props.push_back(tw_prop);

        godot::Dictionary th_prop;
        th_prop["name"] = "texture_height";
        th_prop["type"] = godot::Variant::INT;
        th_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        th_prop["hint_string"] = "1,4096,1,suffix:px";
        props.push_back(th_prop);

        godot::Dictionary dw_prop;
        dw_prop["name"] = "default_w_component";
        dw_prop["type"] = godot::Variant::FLOAT;
        props.push_back(dw_prop);

        godot::Dictionary cmt_prop;
        cmt_prop["name"] = "commit_to_rendering_server";
        cmt_prop["type"] = godot::Variant::BOOL;
        props.push_back(cmt_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (texture_buffer_id != INVALID_ID) ? texture_buffer_id : xyz_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("xyz_buffer_id")) {
            xyz_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["xyz_buffer_id"]));
        }
        if (p_props.has("w_scalar_buffer_id")) {
            w_scalar_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["w_scalar_buffer_id"]));
        }
        if (p_props.has("texture_buffer_id")) {
            texture_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["texture_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && texture_buffer_id == INVALID_ID) {
            texture_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("data_texture_rid")) {
            const godot::Variant v = p_props["data_texture_rid"];
            if (v.get_type() == godot::Variant::RID) {
                data_texture_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&data_texture_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("texture_width")) {
            texture_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["texture_width"]));
        }
        if (p_props.has("texture_height")) {
            texture_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["texture_height"]));
        }
        if (p_props.has("default_w_component")) {
            default_w_component = static_cast<float>(p_props["default_w_component"]);
        }
        if (p_props.has("commit_to_rendering_server")) {
            commit_to_rendering_server = static_cast<bool>(p_props["commit_to_rendering_server"]);
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
        if constexpr (std::is_same_v<T, godot::Vector4> || std::is_same_v<T, godot::Color> ||
                      std::is_same_v<T, godot::Vector3> || std::is_same_v<T, float>) {
            const GrantPartPOD* xyz_part = p_context.get_grant_part(xyz_buffer_id);
            const GrantPartPOD* tex_part = p_context.get_grant_part(texture_buffer_id);
            if (!xyz_part || !tex_part || !xyz_part->raw_base_ptr || !tex_part->raw_base_ptr) return;

            const GrantPartPOD* w_part = (w_scalar_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(w_scalar_buffer_id)
                : nullptr;

            const int32_t w = std::max<int32_t>(1, texture_width);
            const int32_t h = std::max<int32_t>(1, texture_height);
            const size_t total_texels = static_cast<size_t>(w) * static_cast<size_t>(h);
            if (tex_part->capacity_bytes / sizeof(godot::Vector4) < total_texels) return;

            godot::Vector4* dst_texels = reinterpret_cast<godot::Vector4*>(tex_part->raw_base_ptr);
            const uint32_t xyz_stride = std::max<uint32_t>(sizeof(godot::Vector3), xyz_part->element_stride);
            const size_t avail_elems = std::min<size_t>(
                total_texels,
                (xyz_part->selection.element_count > 0)
                    ? static_cast<size_t>(xyz_part->selection.element_count)
                    : (xyz_part->capacity_bytes / xyz_stride)
            );

            const uint32_t w_stride = (w_part && w_part->raw_base_ptr)
                ? std::max<uint32_t>(sizeof(float), w_part->element_stride)
                : sizeof(float);
            const size_t avail_w = (w_part && w_part->raw_base_ptr) ? (w_part->capacity_bytes / w_stride) : 0;

            for (size_t i = 0; i < avail_elems; ++i) {
                const float* fp = reinterpret_cast<const float*>(xyz_part->raw_base_ptr + i * xyz_stride);
                float w_val = default_w_component;
                if (w_part && w_part->raw_base_ptr && i < avail_w) {
                    w_val = *reinterpret_cast<const float*>(w_part->raw_base_ptr + i * w_stride);
                } else if (xyz_stride >= sizeof(godot::Vector4)) {
                    w_val = fp[3];
                }
                dst_texels[i] = godot::Vector4(fp[0], fp[1], fp[2], w_val);
            }

            for (size_t i = avail_elems; i < total_texels; ++i) {
                dst_texels[i] = godot::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
            }

            if (commit_to_rendering_server && data_texture_rid.is_valid()) {
                if (godot::RenderingServer* rs = godot::RenderingServer::get_singleton()) {
                    const size_t byte_size = total_texels * sizeof(godot::Vector4);
                    if (static_cast<size_t>(staging_bytes.size()) != byte_size) {
                        staging_bytes.resize(static_cast<int64_t>(byte_size));
                    }
                    std::memcpy(staging_bytes.ptrw(), dst_texels, byte_size);
                    if (target_image.is_null() || target_image->get_width() != w || target_image->get_height() != h) {
                        target_image = godot::Image::create_from_data(w, h, false, godot::Image::FORMAT_RGBAF, staging_bytes);
                    } else {
                        target_image->set_data(w, h, false, godot::Image::FORMAT_RGBAF, staging_bytes);
                    }
                    if (target_image.is_valid()) {
                        rs->texture_2d_update(data_texture_rid, target_image, 0);
                    }
                }
            }
        }
    }
};

static_assert(sizeof(ShaderDataTexturePackTransformLogic<>) == 64, "ShaderDataTexturePackTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ShaderDataTexturePackTransformLogic<>>, "ShaderDataTexturePackTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core