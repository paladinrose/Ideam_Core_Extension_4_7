#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/rendering_device.hpp>
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
 * G_RD5: RDTexture2DRDBridgeTransformLogic
 * Bridges 2D/3D spatial DOD buffers (RGBA32F Color or R32F float) and RenderingDevice
 * storage textures (Texture2DRD / Texture3DRD) via RenderingDevice::texture_update and
 * RenderingDevice::texture_get_data.
 */
struct RDTexture2DRDBridgeTransformLogic {
    static constexpr std::string_view ui_name = "RD Texture2DRD / Texture3DRD Storage Bridge";
    static constexpr std::string_view ui_category = "GPU/Bridge";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 3> supported_strategies = {
        MemoryStrategy::SPATIAL_2D,
        MemoryStrategy::SPATIAL_3D,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::COLOR,
        DataType::VECTOR4,
        DataType::FLOAT32
    };

    godot::RID rd_texture_rid;
    int32_t sync_direction = 0; // 0 = Upload CPU DOD -> RD Texture, 1 = Readback RD Texture -> CPU DOD
    int32_t texture_layer = 0;

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
        add_prop("rd_texture_rid", godot::Variant::RID, rd_texture_rid);
        add_prop("sync_direction", godot::Variant::INT, sync_direction, 2, "Upload (CPU -> RD Texture),Readback (RD Texture -> CPU)");
        add_prop("texture_layer", godot::Variant::INT, texture_layer);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("rd_texture_rid")) {
            godot::Variant v = p_props["rd_texture_rid"];
            if (v.get_type() == godot::Variant::RID) {
                rd_texture_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&rd_texture_rid)) = raw;
            }
        }
        if (p_props.has("sync_direction")) sync_direction = std::clamp<int32_t>(static_cast<int32_t>(p_props["sync_direction"]), 0, 1);
        if (p_props.has("texture_layer")) texture_layer = std::max<int32_t>(0, static_cast<int32_t>(p_props["texture_layer"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || !rd_texture_rid.is_valid()) return;

        GrantPartPOD& tex_part = p_ctx.grant->parts[0];
        if (!tex_part.raw_base_ptr || tex_part.capacity_bytes == 0) return;

        godot::RenderingDevice* rd = p_ctx.manager ? p_ctx.manager->get_rendering_device() : nullptr;
        if (!rd && godot::RenderingServer::get_singleton()) {
            rd = godot::RenderingServer::get_singleton()->get_rendering_device();
        }
        if (!rd) return;

        const uint32_t layer = static_cast<uint32_t>(std::max<int32_t>(0, texture_layer));

        if (sync_direction == 0) {
            godot::PackedByteArray payload;
            payload.resize(static_cast<int64_t>(tex_part.capacity_bytes));
            std::memcpy(payload.ptrw(), tex_part.raw_base_ptr, tex_part.capacity_bytes);
            rd->texture_update(rd_texture_rid, layer, payload);
        } else {
            godot::PackedByteArray data = rd->texture_get_data(rd_texture_rid, layer);
            const size_t copy_bytes = std::min<size_t>(tex_part.capacity_bytes, static_cast<size_t>(data.size()));
            if (copy_bytes > 0) {
                std::memcpy(tex_part.raw_base_ptr, data.ptr(), copy_bytes);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core