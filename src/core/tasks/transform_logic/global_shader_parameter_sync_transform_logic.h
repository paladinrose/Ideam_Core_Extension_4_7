#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * S1: GlobalShaderParameterSyncTransformLogic
 * Reads or reduces a DOD buffer value (float, int32, Vector2, Vector3, Vector4, or Color)
 * and commits it to RenderingServer::global_shader_parameter_set(parameter_name, value).
 */
struct GlobalShaderParameterSyncTransformLogic {
    static constexpr std::string_view ui_name = "Global Shader Parameter Sync";
    static constexpr std::string_view ui_category = "Shader/Uniforms";

    static constexpr std::array<MemoryView, 1> supported_views = {
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 6> supported_types = {
        DataType::FLOAT32,
        DataType::INT32,
        DataType::VECTOR2,
        DataType::VECTOR3,
        DataType::VECTOR4,
        DataType::COLOR
    };

    godot::StringName parameter_name = godot::StringName("global_dod_param");
    int32_t value_type = 0; // 0 = float, 1 = int, 2 = Vector2, 3 = Vector3, 4 = Vector4, 5 = Color
    int32_t reduction_mode = 0; // 0 = Element at source_index, 1 = Average, 2 = Sum, 3 = Max
    int32_t source_index = 0;

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
        add_prop("parameter_name", godot::Variant::STRING_NAME, parameter_name);
        add_prop("value_type", godot::Variant::INT, value_type, 2, "Float32,Int32,Vector2,Vector3,Vector4,Color");
        add_prop("reduction_mode", godot::Variant::INT, reduction_mode, 2, "Single Element,Average,Sum,Max");
        add_prop("source_index", godot::Variant::INT, source_index);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("parameter_name")) parameter_name = p_props["parameter_name"];
        if (p_props.has("value_type")) value_type = std::clamp<int32_t>(static_cast<int32_t>(p_props["value_type"]), 0, 5);
        if (p_props.has("reduction_mode")) reduction_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["reduction_mode"]), 0, 3);
        if (p_props.has("source_index")) source_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["source_index"]));
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || parameter_name.is_empty()) return;

        const GrantPartPOD& part = p_ctx.grant->parts[0];
        if (!part.raw_base_ptr) return;

        godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
        if (!rs) return;

        godot::Variant out_val;
        switch (value_type) {
            case 0: {
                const uint32_t stride = std::max<uint32_t>(sizeof(float), part.element_stride);
                const int64_t count = (part.selection.element_count > 0)
                    ? part.selection.element_count
                    : static_cast<int64_t>(part.capacity_bytes / stride);
                if (count <= 0) return;
                if (reduction_mode == 0) {
                    const int64_t idx = std::min<int64_t>(source_index, count - 1);
                    out_val = *reinterpret_cast<const float*>(part.raw_base_ptr + static_cast<size_t>(idx) * stride);
                } else {
                    float acc = (reduction_mode == 3) ? -1e30f : 0.0f;
                    for (int64_t i = 0; i < count; ++i) {
                        const float v = *reinterpret_cast<const float*>(part.raw_base_ptr + static_cast<size_t>(i) * stride);
                        acc = (reduction_mode == 3) ? std::max(acc, v) : (acc + v);
                    }
                    if (reduction_mode == 1) acc /= static_cast<float>(count);
                    out_val = acc;
                }
                break;
            }
            case 1: {
                const uint32_t stride = std::max<uint32_t>(sizeof(int32_t), part.element_stride);
                const int64_t count = static_cast<int64_t>(part.capacity_bytes / stride);
                if (count <= 0) return;
                const int64_t idx = std::min<int64_t>(source_index, count - 1);
                out_val = *reinterpret_cast<const int32_t*>(part.raw_base_ptr + static_cast<size_t>(idx) * stride);
                break;
            }
            case 2: {
                const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector2), part.element_stride);
                const int64_t count = static_cast<int64_t>(part.capacity_bytes / stride);
                if (count <= 0) return;
                const int64_t idx = std::min<int64_t>(source_index, count - 1);
                out_val = *reinterpret_cast<const godot::Vector2*>(part.raw_base_ptr + static_cast<size_t>(idx) * stride);
                break;
            }
            case 3: {
                const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), part.element_stride);
                const int64_t count = static_cast<int64_t>(part.capacity_bytes / stride);
                if (count <= 0) return;
                const int64_t idx = std::min<int64_t>(source_index, count - 1);
                out_val = *reinterpret_cast<const godot::Vector3*>(part.raw_base_ptr + static_cast<size_t>(idx) * stride);
                break;
            }
            case 4: {
                const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector4), part.element_stride);
                const int64_t count = static_cast<int64_t>(part.capacity_bytes / stride);
                if (count <= 0) return;
                const int64_t idx = std::min<int64_t>(source_index, count - 1);
                out_val = *reinterpret_cast<const godot::Vector4*>(part.raw_base_ptr + static_cast<size_t>(idx) * stride);
                break;
            }
            case 5: {
                const uint32_t stride = std::max<uint32_t>(sizeof(godot::Color), part.element_stride);
                const int64_t count = static_cast<int64_t>(part.capacity_bytes / stride);
                if (count <= 0) return;
                const int64_t idx = std::min<int64_t>(source_index, count - 1);
                out_val = *reinterpret_cast<const godot::Color*>(part.raw_base_ptr + static_cast<size_t>(idx) * stride);
                break;
            }
            default:
                return;
        }

        rs->global_shader_parameter_set(parameter_name, out_val);
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core