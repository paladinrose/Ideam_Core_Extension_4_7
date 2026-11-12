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
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * InstanceShaderParamBatchTransformLogic<T> (S3)
 * Calls RenderingServer::instance_set_shader_parameter across active/selected visual instance
 * RIDs (instance_rid_buffer_id, uint64_t RID handles) and per-instance parameter values (value_buffer_id).
 */
template <typename T = float>
struct alignas(64) InstanceShaderParamBatchTransformLogic {
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

    static constexpr std::string_view display_name = "Instance Shader Parameter Batch Sync";
    static constexpr std::string_view ui_category  = "Shader/Uniforms";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t value_buffer_id        = INVALID_ID; // 4B: Primary target per-instance uniform value buffer
    uint32_t instance_rid_buffer_id = INVALID_ID; // 4B: Visual instance RIDs buffer (uint64_t handles)
    uint8_t value_type              = 0;          // 1B: 0=float, 1=int32, 2=Vector2, 3=Vector3, 4=Vector4, 5=Color
    bool respect_selection_mask     = true;       // 1B: Only update instances active in selection mask
    uint8_t reserved_flags[6]       = {0};        // 6B -> 16B
    godot::StringName parameter_name;             // sizeof(godot::StringName) (8B)
    uint8_t reserved_padding[48 - sizeof(godot::StringName)] = {0}; // Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vbuf_prop;
        vbuf_prop["name"] = "value_buffer_id";
        vbuf_prop["type"] = godot::Variant::INT;
        vbuf_prop["hint_string"] = "buffer_option";
        props.push_back(vbuf_prop);

        godot::Dictionary rbuf_prop;
        rbuf_prop["name"] = "instance_rid_buffer_id";
        rbuf_prop["type"] = godot::Variant::INT;
        rbuf_prop["hint_string"] = "buffer_option";
        props.push_back(rbuf_prop);

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

        godot::Dictionary sel_prop;
        sel_prop["name"] = "respect_selection_mask";
        sel_prop["type"] = godot::Variant::BOOL;
        props.push_back(sel_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (value_buffer_id != INVALID_ID) ? value_buffer_id : instance_rid_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("value_buffer_id")) {
            value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["value_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && value_buffer_id == INVALID_ID) {
            value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && value_buffer_id == INVALID_ID) {
            value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("instance_rid_buffer_id")) {
            instance_rid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["instance_rid_buffer_id"]));
        }
        if (p_props.has("parameter_name")) {
            parameter_name = p_props["parameter_name"];
        }
        if (p_props.has("value_type")) {
            value_type = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["value_type"]), 0, 5));
        }
        if (p_props.has("respect_selection_mask")) {
            respect_selection_mask = static_cast<bool>(p_props["respect_selection_mask"]);
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

            const GrantPartPOD* rid_part = p_context.get_grant_part(instance_rid_buffer_id);
            const GrantPartPOD* val_part = p_context.get_grant_part(value_buffer_id);
            if (!rid_part || !val_part || !rid_part->raw_base_ptr || !val_part->raw_base_ptr) return;

            godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
            if (!rs) return;

            const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part->element_stride);
            const int64_t max_rids = static_cast<int64_t>(rid_part->capacity_bytes / r_stride);
            const MemoryBufferSelectionPOD& sel = val_part->selection;

            auto dispatch_one = [&](int64_t idx) {
                if (idx < 0 || idx >= max_rids) return;
                const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(rid_part->raw_base_ptr + static_cast<size_t>(idx) * r_stride);
                if (raw_id == 0) return;
                godot::RID inst_rid;
                std::memcpy(&inst_rid, &raw_id, sizeof(uint64_t));
                if (inst_rid.is_valid()) {
                    rs->instance_set_shader_parameter(inst_rid, parameter_name, _read_value(*val_part, idx));
                }
            };

            if (respect_selection_mask && sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t k = 0; k < sel.element_count; ++k) {
                    dispatch_one(sel.data.indices[k]);
                }
            } else if (respect_selection_mask && sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const int64_t cap = std::min<int64_t>(max_rids, sel.capacity);
                for (int64_t i = 0; i < cap; ++i) {
                    if (sel.data.bitset[i >> 6] & (1ULL << (i & 63))) {
                        dispatch_one(i);
                    }
                }
            } else {
                const int64_t count = (sel.element_count > 0) ? std::min<int64_t>(sel.element_count, max_rids) : max_rids;
                for (int64_t i = 0; i < count; ++i) {
                    dispatch_one(i);
                }
            }
        }
    }

private:
    inline godot::Variant _read_value(const GrantPartPOD& p_val_part, int64_t p_idx) const noexcept {
        const uint8_t* base = p_val_part.raw_base_ptr;
        switch (value_type) {
            case 0: return *reinterpret_cast<const float*>(base + static_cast<size_t>(p_idx) * std::max<uint32_t>(4, p_val_part.element_stride));
            case 1: return *reinterpret_cast<const int32_t*>(base + static_cast<size_t>(p_idx) * std::max<uint32_t>(4, p_val_part.element_stride));
            case 2: return *reinterpret_cast<const godot::Vector2*>(base + static_cast<size_t>(p_idx) * std::max<uint32_t>(8, p_val_part.element_stride));
            case 3: return *reinterpret_cast<const godot::Vector3*>(base + static_cast<size_t>(p_idx) * std::max<uint32_t>(12, p_val_part.element_stride));
            case 4: return *reinterpret_cast<const godot::Vector4*>(base + static_cast<size_t>(p_idx) * std::max<uint32_t>(16, p_val_part.element_stride));
            case 5: return *reinterpret_cast<const godot::Color*>(base + static_cast<size_t>(p_idx) * std::max<uint32_t>(16, p_val_part.element_stride));
            default: return godot::Variant();
        }
    }
};

static_assert(sizeof(InstanceShaderParamBatchTransformLogic<>) == 64, "InstanceShaderParamBatchTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<InstanceShaderParamBatchTransformLogic<>>, "InstanceShaderParamBatchTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core