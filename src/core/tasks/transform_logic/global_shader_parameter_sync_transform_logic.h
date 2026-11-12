#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class ShaderUniformValueType : uint8_t {
    FLOAT32 = 0,
    INT32   = 1,
    VECTOR2 = 2,
    VECTOR3 = 3,
    VECTOR4 = 4,
    COLOR   = 5
};

enum class ShaderReductionMode : uint8_t {
    SINGLE_ELEMENT = 0,
    AVERAGE        = 1,
    SUM            = 2,
    MAX_VALUE      = 3
};

/**
 * GlobalShaderParameterSyncTransformLogic<T> (S1)
 * Reads or reduces a DOD buffer value (float, int32, Vector2, Vector3, Vector4, or Color)
 * and commits it to RenderingServer::global_shader_parameter_set(parameter_name, value).
 */
template <typename T = float>
struct alignas(64) GlobalShaderParameterSyncTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              =
        DataType::FLOAT32 | DataType::INT32 | DataType::VECTOR2 |
        DataType::VECTOR3 | DataType::VECTOR4 | DataType::COLOR;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Global Shader Parameter Sync";
    static constexpr std::string_view ui_category  = "Shader/Uniforms";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t source_buffer_id              = INVALID_ID;                            // 4B: Source DOD buffer
    int32_t source_index                   = 0;                                     // 4B: Element index when reduction_mode == SINGLE_ELEMENT
    ShaderUniformValueType value_type      = ShaderUniformValueType::FLOAT32;       // 1B
    ShaderReductionMode reduction_mode     = ShaderReductionMode::SINGLE_ELEMENT;   // 1B
    uint8_t reserved_flags[6]              = {0};                                   // 6B -> 16B
    godot::StringName parameter_name;                                               // sizeof(godot::StringName) (8B)
    uint8_t reserved_padding[48 - sizeof(godot::StringName)] = {0};                 // Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "source_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        godot::Dictionary pname_prop;
        pname_prop["name"] = "parameter_name";
        pname_prop["type"] = godot::Variant::STRING_NAME;
        props.push_back(pname_prop);

        godot::Dictionary vtype_prop;
        vtype_prop["name"] = "value_type";
        vtype_prop["type"] = godot::Variant::INT;
        vtype_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        vtype_prop["hint_string"] = "Float32:0,Int32:1,Vector2:2,Vector3:3,Vector4:4,Color:5";
        props.push_back(vtype_prop);

        godot::Dictionary rmode_prop;
        rmode_prop["name"] = "reduction_mode";
        rmode_prop["type"] = godot::Variant::INT;
        rmode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        rmode_prop["hint_string"] = "Single Element:0,Average:1,Sum:2,Max:3";
        props.push_back(rmode_prop);

        godot::Dictionary idx_prop;
        idx_prop["name"] = "source_index";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        idx_prop["hint_string"] = "0,65536,1";
        props.push_back(idx_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return source_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && source_buffer_id == INVALID_ID) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && source_buffer_id == INVALID_ID) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("parameter_name")) {
            parameter_name = p_props["parameter_name"];
        }
        if (p_props.has("value_type")) {
            value_type = static_cast<ShaderUniformValueType>(std::clamp<int64_t>(static_cast<int64_t>(p_props["value_type"]), 0, 5));
        }
        if (p_props.has("reduction_mode")) {
            reduction_mode = static_cast<ShaderReductionMode>(std::clamp<int64_t>(static_cast<int64_t>(p_props["reduction_mode"]), 0, 3));
        }
        if (p_props.has("source_index")) {
            source_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["source_index"]));
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
            if (parameter_name.is_empty()) return;

            const GrantPartPOD* part = p_context.get_grant_part(source_buffer_id);
            if (!part || !part->raw_base_ptr) return;

            godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
            if (!rs) return;

            godot::Variant out_val;
            switch (value_type) {
                case ShaderUniformValueType::FLOAT32: {
                    const uint32_t stride = std::max<uint32_t>(sizeof(float), part->element_stride);
                    const int64_t count = (part->selection.element_count > 0)
                        ? part->selection.element_count
                        : static_cast<int64_t>(part->capacity_bytes / stride);
                    if (count <= 0) return;
                    if (reduction_mode == ShaderReductionMode::SINGLE_ELEMENT) {
                        const int64_t idx = std::min<int64_t>(source_index, count - 1);
                        out_val = *reinterpret_cast<const float*>(part->raw_base_ptr + static_cast<size_t>(idx) * stride);
                    } else {
                        float acc = (reduction_mode == ShaderReductionMode::MAX_VALUE) ? -1e30f : 0.0f;
                        for (int64_t i = 0; i < count; ++i) {
                            const float v = *reinterpret_cast<const float*>(part->raw_base_ptr + static_cast<size_t>(i) * stride);
                            acc = (reduction_mode == ShaderReductionMode::MAX_VALUE) ? std::max(acc, v) : (acc + v);
                        }
                        if (reduction_mode == ShaderReductionMode::AVERAGE) acc /= static_cast<float>(count);
                        out_val = acc;
                    }
                    break;
                }
                case ShaderUniformValueType::INT32: {
                    const uint32_t stride = std::max<uint32_t>(sizeof(int32_t), part->element_stride);
                    const int64_t count = static_cast<int64_t>(part->capacity_bytes / stride);
                    if (count <= 0) return;
                    const int64_t idx = std::min<int64_t>(source_index, count - 1);
                    out_val = *reinterpret_cast<const int32_t*>(part->raw_base_ptr + static_cast<size_t>(idx) * stride);
                    break;
                }
                case ShaderUniformValueType::VECTOR2: {
                    const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector2), part->element_stride);
                    const int64_t count = static_cast<int64_t>(part->capacity_bytes / stride);
                    if (count <= 0) return;
                    const int64_t idx = std::min<int64_t>(source_index, count - 1);
                    out_val = *reinterpret_cast<const godot::Vector2*>(part->raw_base_ptr + static_cast<size_t>(idx) * stride);
                    break;
                }
                case ShaderUniformValueType::VECTOR3: {
                    const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector3), part->element_stride);
                    const int64_t count = static_cast<int64_t>(part->capacity_bytes / stride);
                    if (count <= 0) return;
                    const int64_t idx = std::min<int64_t>(source_index, count - 1);
                    out_val = *reinterpret_cast<const godot::Vector3*>(part->raw_base_ptr + static_cast<size_t>(idx) * stride);
                    break;
                }
                case ShaderUniformValueType::VECTOR4: {
                    const uint32_t stride = std::max<uint32_t>(sizeof(godot::Vector4), part->element_stride);
                    const int64_t count = static_cast<int64_t>(part->capacity_bytes / stride);
                    if (count <= 0) return;
                    const int64_t idx = std::min<int64_t>(source_index, count - 1);
                    out_val = *reinterpret_cast<const godot::Vector4*>(part->raw_base_ptr + static_cast<size_t>(idx) * stride);
                    break;
                }
                case ShaderUniformValueType::COLOR: {
                    const uint32_t stride = std::max<uint32_t>(sizeof(godot::Color), part->element_stride);
                    const int64_t count = static_cast<int64_t>(part->capacity_bytes / stride);
                    if (count <= 0) return;
                    const int64_t idx = std::min<int64_t>(source_index, count - 1);
                    out_val = *reinterpret_cast<const godot::Color*>(part->raw_base_ptr + static_cast<size_t>(idx) * stride);
                    break;
                }
                default:
                    return;
            }

            rs->global_shader_parameter_set(parameter_name, out_val);
        }
    }
};

static_assert(sizeof(GlobalShaderParameterSyncTransformLogic<>) == 64, "GlobalShaderParameterSyncTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<GlobalShaderParameterSyncTransformLogic<>>, "GlobalShaderParameterSyncTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core