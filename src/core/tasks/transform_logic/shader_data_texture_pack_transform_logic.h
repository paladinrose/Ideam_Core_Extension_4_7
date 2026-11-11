#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

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
#include <array>
#include <cstring>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * S5: ShaderDataTexturePackTransformLogic
 * Packs 1D/2D entity attribute arrays (Vector3 + scalar or Vector4 columns) into a FORMAT_RGBAF
 * 2D data texture buffer and commits it via RenderingServer::texture_2d_update for vertex-shader
 * texelFetch crowd/foliage/particle animation.
 */
struct ShaderDataTexturePackTransformLogic {
    static constexpr std::string_view ui_name = "Shader texelFetch Data Texture Pack";
    static constexpr std::string_view ui_category = "Shader/Packing";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 4> supported_types = {
        DataType::VECTOR4,
        DataType::COLOR,
        DataType::VECTOR3,
        DataType::FLOAT32
    };

    godot::RID data_texture_rid;
    int32_t texture_width = 256;
    int32_t texture_height = 64;
    float default_w_component = 1.0f;
    bool commit_to_rendering_server = true;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("data_texture_rid", godot::Variant::RID, data_texture_rid);
        add_prop("texture_width", godot::Variant::INT, texture_width);
        add_prop("texture_height", godot::Variant::INT, texture_height);
        add_prop("default_w_component", godot::Variant::FLOAT, default_w_component);
        add_prop("commit_to_rendering_server", godot::Variant::BOOL, commit_to_rendering_server);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("data_texture_rid")) {
            godot::Variant v = p_props["data_texture_rid"];
            if (v.get_type() == godot::Variant::RID) {
                data_texture_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&data_texture_rid)) = raw;
            }
        }
        if (p_props.has("texture_width")) texture_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["texture_width"]));
        if (p_props.has("texture_height")) texture_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["texture_height"]));
        if (p_props.has("default_w_component")) default_w_component = static_cast<float>(p_props["default_w_component"]);
        if (p_props.has("commit_to_rendering_server")) commit_to_rendering_server = static_cast<bool>(p_props["commit_to_rendering_server"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& xyz_part = p_ctx.grant->parts[0];
        GrantPartPOD& tex_part = p_ctx.grant->parts[p_ctx.grant->part_count - 1];
        if (!xyz_part.raw_base_ptr || !tex_part.raw_base_ptr) return;

        const GrantPartPOD* w_part = (p_ctx.grant->part_count > 2 && p_ctx.grant->parts[1].raw_base_ptr)
            ? &p_ctx.grant->parts[1]
            : nullptr;

        const int32_t w = std::max<int32_t>(1, texture_width);
        const int32_t h = std::max<int32_t>(1, texture_height);
        const size_t total_texels = static_cast<size_t>(w) * h;
        if (tex_part.capacity_bytes / sizeof(godot::Vector4) < total_texels) return;

        godot::Vector4* dst_texels = reinterpret_cast<godot::Vector4*>(tex_part.raw_base_ptr);
        const uint32_t xyz_stride = std::max<uint32_t>(sizeof(godot::Vector3), xyz_part.element_stride);
        const size_t avail_elems = std::min<size_t>(
            total_texels,
            (xyz_part.selection.element_count > 0)
                ? static_cast<size_t>(xyz_part.selection.element_count)
                : (xyz_part.capacity_bytes / xyz_stride)
        );

        const uint32_t w_stride = w_part ? std::max<uint32_t>(sizeof(float), w_part->element_stride) : sizeof(float);
        const size_t avail_w = w_part ? (w_part->capacity_bytes / w_stride) : 0;

        for (size_t i = 0; i < avail_elems; ++i) {
            const float* fp = reinterpret_cast<const float*>(xyz_part.raw_base_ptr + i * xyz_stride);
            float w_val = default_w_component;
            if (w_part && i < avail_w) {
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
                godot::PackedByteArray bytes;
                bytes.resize(static_cast<int64_t>(byte_size));
                std::memcpy(bytes.ptrw(), dst_texels, byte_size);
                godot::Ref<godot::Image> img = godot::Image::create_from_data(w, h, false, godot::Image::FORMAT_RGBAF, bytes);
                if (img.is_valid()) {
                    rs->texture_2d_update(data_texture_rid, img, 0);
                }
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core