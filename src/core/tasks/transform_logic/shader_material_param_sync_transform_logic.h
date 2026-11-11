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
 * S4: ShaderMaterialParamSyncTransformLogic
 * Updates shared ShaderMaterial RIDs via RenderingServer::material_set_param from DOD buffer values.
 */
struct ShaderMaterialParamSyncTransformLogic {
    static constexpr std::string_view ui_name = "ShaderMaterial Parameter Sync";
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

    godot::RID material_rid;
    godot::StringName parameter_name = godot::StringName("shader_param");
    int32_t value_type = 0; // 0 = float, 1 = int, 2 = Vector2, 3 = Vector3, 4 = Vector4, 5 = Color
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
        add_prop("material_rid", godot::Variant::RID, material_rid);
        add_prop("parameter_name", godot::Variant::STRING_NAME, parameter_name);
        add_prop("value_type", godot::Variant::INT, value_type, 2, "Float32,Int32,Vector2,Vector3,Vector4,Color");
        add_prop("source_index", godot::Variant::INT, source_index);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("material_rid")) {
            godot::Variant v = p_props["material_rid"];
            if (v.get_type() == godot::Variant::RID) {
                material_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                *(reinterpret_cast<uint64_t*>(&material_rid)) = raw;
            }
        }
        if (p_props.has("parameter_name")) parameter_name = p_props["parameter_name"];
        if (p_props.has("value_type")) value_type = std::clamp<int32_t>(static_cast<int32_t>(p_props["value_type"]), 0, 5);
        if (p_props.has("source_index")) source_index = std::max<int32_t>(0, static_cast<int32_t>(p_props["source_index"]));
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
        if (!p_ctx.grant || p_ctx.grant->part_count == 0 || parameter_name.is_empty()) return;

        godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
        if (!rs) return;

        if (p_ctx.grant->part_count >= 2 && p_ctx.grant->parts[0].raw_base_ptr && p_ctx.grant->parts[1].raw_base_ptr) {
            // Part 0 = array of material RIDs (uint64_t), Part 1 = array of parameter values
            const GrantPartPOD& rid_part = p_ctx.grant->parts[0];
            const GrantPartPOD& val_part = p_ctx.grant->parts[1];
            const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part.element_stride);
            const int64_t n_mats = (rid_part.selection.element_count > 0)
                ? rid_part.selection.element_count
                : static_cast<int64_t>(rid_part.capacity_bytes / r_stride);
            for (int64_t i = 0; i < n_mats; ++i) {
                const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(rid_part.raw_base_ptr + static_cast<size_t>(i) * r_stride);
                if (raw_id == 0) continue;
                godot::RID mat_rid;
                *(reinterpret_cast<uint64_t*>(&mat_rid)) = raw_id;
                if (mat_rid.is_valid()) {
                    rs->material_set_param(mat_rid, parameter_name, read_value(val_part, i));
                }
            }
        } else if (material_rid.is_valid() && p_ctx.grant->parts[0].raw_base_ptr) {
            const GrantPartPOD& val_part = p_ctx.grant->parts[0];
            const uint32_t stride = std::max<uint32_t>(4, val_part.element_stride);
            const int64_t count = static_cast<int64_t>(val_part.capacity_bytes / stride);
            if (count > 0) {
                const int64_t idx = std::min<int64_t>(source_index, count - 1);
                rs->material_set_param(material_rid, parameter_name, read_value(val_part, idx));
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core