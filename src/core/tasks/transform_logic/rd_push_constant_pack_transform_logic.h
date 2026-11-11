#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * G_RD3: RDPushConstantPackTransformLogic
 * Packs per-frame execution constants (delta, element_count, time, user_flags, origin, and custom_vec4)
 * into a 16-byte aligned (32B or 64B) GLSL push-constant struct inside Part 0.
 */
struct RDPushConstantPackTransformLogic {
    static constexpr std::string_view ui_name = "RD Compute Push-Constant Block Packer";
    static constexpr std::string_view ui_category = "GPU/Packing";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 1> supported_strategies = {
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::FLOAT32,
        DataType::VECTOR4,
        DataType::UINT32
    };

    // 64-byte (16-byte aligned) canonical push constant block
    struct alignas(16) PushConstantBlock64Pod {
        float delta;
        float time_seconds;
        uint32_t element_count;
        uint32_t user_flags;
        float origin_x;
        float origin_y;
        float origin_z;
        float scalar_param;
        float custom_vec4[4];
        float extra_vec4[4];
    };

    float time_seconds = 0.0f;
    int32_t user_flags = 0;
    godot::Vector3 origin = godot::Vector3(0.0f, 0.0f, 0.0f);
    float scalar_param = 1.0f;
    godot::Vector4 custom_vec4 = godot::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Vector4 extra_vec4 = godot::Vector4(0.0f, 0.0f, 0.0f, 0.0f);

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("time_seconds", godot::Variant::FLOAT, time_seconds);
        add_prop("user_flags", godot::Variant::INT, user_flags);
        add_prop("origin", godot::Variant::VECTOR3, origin);
        add_prop("scalar_param", godot::Variant::FLOAT, scalar_param);
        add_prop("custom_vec4", godot::Variant::VECTOR4, custom_vec4);
        add_prop("extra_vec4", godot::Variant::VECTOR4, extra_vec4);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("time_seconds")) time_seconds = static_cast<float>(p_props["time_seconds"]);
        if (p_props.has("user_flags")) user_flags = static_cast<int32_t>(p_props["user_flags"]);
        if (p_props.has("origin")) origin = p_props["origin"];
        if (p_props.has("scalar_param")) scalar_param = static_cast<float>(p_props["scalar_param"]);
        if (p_props.has("custom_vec4")) custom_vec4 = p_props["custom_vec4"];
        if (p_props.has("extra_vec4")) extra_vec4 = p_props["extra_vec4"];
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0) return;

        GrantPartPOD& pc_part = p_ctx.grant->parts[0];
        if (!pc_part.raw_base_ptr || pc_part.capacity_bytes < 16) return;

        uint32_t elem_count = static_cast<uint32_t>(std::max<int64_t>(0, pc_part.selection.element_count));
        if (p_ctx.grant->part_count > 1) {
            elem_count = static_cast<uint32_t>(std::max<int64_t>(0, p_ctx.grant->parts[1].selection.element_count));
        }

        PushConstantBlock64Pod block{};
        block.delta = static_cast<float>(p_ctx.delta);
        block.time_seconds = time_seconds + static_cast<float>(p_ctx.delta);
        block.element_count = elem_count;
        block.user_flags = static_cast<uint32_t>(user_flags);
        block.origin_x = origin.x;
        block.origin_y = origin.y;
        block.origin_z = origin.z;
        block.scalar_param = scalar_param;
        block.custom_vec4[0] = custom_vec4.x;
        block.custom_vec4[1] = custom_vec4.y;
        block.custom_vec4[2] = custom_vec4.z;
        block.custom_vec4[3] = custom_vec4.w;
        block.extra_vec4[0] = extra_vec4.x;
        block.extra_vec4[1] = extra_vec4.y;
        block.extra_vec4[2] = extra_vec4.z;
        block.extra_vec4[3] = extra_vec4.w;

        // Write 16-byte aligned slice matching destination capacity (16B, 32B, 48B, or 64B)
        const size_t copy_bytes = std::min<size_t>(sizeof(PushConstantBlock64Pod), (pc_part.capacity_bytes / 16) * 16);
        std::memcpy(pc_part.raw_base_ptr, &block, copy_bytes);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core