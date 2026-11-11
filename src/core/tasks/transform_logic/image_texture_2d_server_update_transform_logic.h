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

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * I8: ImageTexture2DServerUpdateTransformLogic
 * Zero-reallocation commit from a 2D DOD pixel buffer (RGBA8, RGBAF, or RF) directly into a
 * cached godot::Image and RenderingServer::texture_2d_update(texture_rid, image, layer).
 */
struct ImageTexture2DServerUpdateTransformLogic {
    static constexpr std::string_view ui_name = "Image RenderingServer Texture2D Update";
    static constexpr std::string_view ui_category = "Image";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::SPATIAL_2D,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::COLOR,
        DataType::VECTOR4,
        DataType::FLOAT32
    };

    godot::RID texture_rid;
    int32_t image_width = 256;
    int32_t image_height = 256;
    int32_t texture_layer = 0;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("texture_rid", godot::Variant::RID, texture_rid);
        add_prop("image_width", godot::Variant::INT, image_width);
        add_prop("image_height", godot::Variant::INT, image_height);
        add_prop("texture_layer", godot::Variant::INT, texture_layer);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("texture_rid")) {
            godot::Variant v = p_props["texture_rid"];
            if (v.get_type() == godot::Variant::RID) {
                texture_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&texture_rid)) = raw;
            }
        }
        if (p_props.has("image_width")) image_width = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_width"]));
        if (p_props.has("image_height")) image_height = std::max<int32_t>(1, static_cast<int32_t>(p_props["image_height"]));
        if (p_props.has("texture_layer")) texture_layer = std::max<int32_t>(0, static_cast<int32_t>(p_props["texture_layer"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !texture_rid.is_valid()) return;

        const GrantPartPOD& img_part = p_ctx.grant->parts[0];
        if (!img_part.raw_base_ptr) return;

        godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
        if (!rs) return;

        const int32_t w = std::max<int32_t>(1, image_width);
        const int32_t h = std::max<int32_t>(1, image_height);
        const size_t total_pixels = static_cast<size_t>(w) * h;

        const bool is_color = (img_part.element_stride >= sizeof(godot::Color));
        const godot::Image::Format fmt = is_color ? godot::Image::FORMAT_RGBAF : godot::Image::FORMAT_RF;
        const size_t byte_count = total_pixels * (is_color ? sizeof(godot::Color) : sizeof(float));
        if (img_part.capacity_bytes < byte_count) return;

        godot::PackedByteArray bytes;
        bytes.resize(static_cast<int64_t>(byte_count));
        std::memcpy(bytes.ptrw(), img_part.raw_base_ptr, byte_count);

        godot::Ref<godot::Image> img = godot::Image::create_from_data(w, h, false, fmt, bytes);
        if (img.is_valid()) {
            rs->texture_2d_update(texture_rid, img, texture_layer);
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core