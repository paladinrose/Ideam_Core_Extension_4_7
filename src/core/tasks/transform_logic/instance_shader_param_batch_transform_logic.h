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
#include <godot_cpp/variant/rid.hpp>
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
 * S3: InstanceShaderParamBatchTransformLogic
 * Calls RenderingServer::instance_set_shader_parameter across active/selected visual instance
 * RIDs (Part 0, uint64_t RID handles) and per-instance parameter values (Part 1).
 */
struct InstanceShaderParamBatchTransformLogic {
    static constexpr std::string_view ui_name = "Instance Shader Parameter Batch Sync";
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

    godot::StringName parameter_name = godot::StringName("instance_param");
    int32_t value_type = 0; // 0 = float, 1 = int, 2 = Vector2, 3 = Vector3, 4 = Vector4, 5 = Color
    bool respect_selection_mask = true;

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
        add_prop("respect_selection_mask", godot::Variant::BOOL, respect_selection_mask);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("parameter_name")) parameter_name = p_props["parameter_name"];
        if (p_props.has("value_type")) value_type = std::clamp<int32_t>(static_cast<int32_t>(p_props["value_type"]), 0, 5);
        if (p_props.has("respect_selection_mask")) respect_selection_mask = static_cast<bool>(p_props["respect_selection_mask"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    inline godot::Variant read_value(const GrantPartPOD& val_part, int64_t idx) const noexcept {
        const uint8_t* base = val_part.raw_base_ptr;
        switch (value_type) {
            case 0: return *reinterpret_cast<const float*>(base + static_cast<size_t>(idx) * std::max<uint32_t>(4, val_part.element_stride));
            case 1: return *reinterpret_cast<const int32_t*>(base + static_cast<size_t>(idx) * std::max<uint32_t>(4, val_part.element_stride));
            case 2: return *reinterpret_cast<const godot::Vector2*>(base + static_cast<size_t>(idx) * std::max<uint32_t>(8, val_part.element_stride));
            case 3: return *reinterpret_cast<const godot::Vector3*>(base + static_cast<size_t>(idx) * std::max<uint32_t>(12, val_part.element_stride));
            case 4: return *reinterpret_cast<const godot::Vector4*>(base + static_cast<size_t>(idx) * std::max<uint32_t>(16, val_part.element_stride));
            case 5: return *reinterpret_cast<const godot::Color*>(base + static_cast<size_t>(idx) * std::max<uint32_t>(16, val_part.element_stride));
            default: return godot::Variant();
        }
    }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2 || parameter_name.is_empty()) return;

        const GrantPartPOD& rid_part = p_ctx.grant->parts[0];
        const GrantPartPOD& val_part = p_ctx.grant->parts[1];
        if (!rid_part.raw_base_ptr || !val_part.raw_base_ptr) return;

        godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
        if (!rs) return;

        const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part.element_stride);
        const int64_t max_rids = static_cast<int64_t>(rid_part.capacity_bytes / r_stride);
        const MemoryBufferSelectionPOD& sel = val_part.selection;

        auto dispatch_one = [&](int64_t idx) {
            if (idx < 0 || idx >= max_rids) return;
            const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(rid_part.raw_base_ptr + static_cast<size_t>(idx) * r_stride);
            if (raw_id == 0) return;
            godot::RID inst_rid;
            *(reinterpret_cast<uint64_t*>(&inst_rid)) = raw_id;
            if (inst_rid.is_valid()) {
                rs->instance_set_shader_parameter(inst_rid, parameter_name, read_value(val_part, idx));
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

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core