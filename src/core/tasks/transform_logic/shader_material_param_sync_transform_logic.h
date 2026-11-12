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
 * ShaderMaterialParamSyncTransformLogic<T> (S4)
 * Updates shared ShaderMaterial RIDs via RenderingServer::material_set_param from DOD buffer values.
 */
template <typename T = float>
struct alignas(64) ShaderMaterialParamSyncTransformLogic {
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

    static constexpr std::string_view display_name = "ShaderMaterial Parameter Sync";
    static constexpr std::string_view ui_category  = "Shader/Uniforms";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t value_buffer_id        = INVALID_ID; // 4B: Primary target parameter value buffer
    uint32_t material_rid_buffer_id = INVALID_ID; // 4B: Optional buffer of uint64_t Material RIDs
    int32_t source_index            = 0;          // 4B: Element index when updating a single material_rid
    uint8_t value_type              = 0;          // 1B: 0=float, 1=int32, 2=Vector2, 3=Vector3, 4=Vector4, 5=Color
    uint8_t reserved_flags[3]       = {0};        // 3B -> 16B
    godot::RID material_rid;                      // 8B -> 24B
    godot::StringName parameter_name;             // sizeof(godot::StringName) (8B) -> 32B
    uint8_t reserved_padding[40 - sizeof(godot::StringName)] = {0}; // Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary vbuf_prop;
        vbuf_prop["name"] = "value_buffer_id";
        vbuf_prop["type"] = godot::Variant::INT;
        vbuf_prop["hint_string"] = "buffer_option";
        props.push_back(vbuf_prop);

        godot::Dictionary mbuf_prop;
        mbuf_prop["name"] = "material_rid_buffer_id";
        mbuf_prop["type"] = godot::Variant::INT;
        mbuf_prop["hint_string"] = "buffer_option";
        props.push_back(mbuf_prop);

        godot::Dictionary mrid_prop;
        mrid_prop["name"] = "material_rid";
        mrid_prop["type"] = godot::Variant::RID;
        props.push_back(mrid_prop);

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

        godot::Dictionary idx_prop;
        idx_prop["name"] = "source_index";
        idx_prop["type"] = godot::Variant::INT;
        idx_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        idx_prop["hint_string"] = "0,65536,1";
        props.push_back(idx_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (value_buffer_id != INVALID_ID) ? value_buffer_id : material_rid_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("value_buffer_id")) {
            value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["value_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && value_buffer_id == INVALID_ID) {
            value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("buffer_id") && value_buffer_id == INVALID_ID) {
            value_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["buffer_id"]));
        }

        if (p_props.has("material_rid_buffer_id")) {
            material_rid_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["material_rid_buffer_id"]));
        }
        if (p_props.has("material_rid")) {
            const godot::Variant v = p_props["material_rid"];
            if (v.get_type() == godot::Variant::RID) {
                material_rid = v;
            } else if (v.get_type() == godot::Variant::INT) {
                const uint64_t raw = static_cast<uint64_t>(static_cast<int64_t>(v));
                std::memcpy(&material_rid, &raw, sizeof(uint64_t));
            }
        }
        if (p_props.has("parameter_name")) {
            parameter_name = p_props["parameter_name"];
        }
        if (p_props.has("value_type")) {
            value_type = static_cast<uint8_t>(std::clamp<int64_t>(static_cast<int64_t>(p_props["value_type"]), 0, 5));
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

            const GrantPartPOD* val_part = p_context.get_grant_part(value_buffer_id);
            if (!val_part || !val_part->raw_base_ptr) return;

            godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
            if (!rs) return;

            const GrantPartPOD* rid_part = (material_rid_buffer_id != INVALID_ID)
                ? p_context.get_grant_part(material_rid_buffer_id)
                : nullptr;

            if (rid_part && rid_part->raw_base_ptr) {
                const uint32_t r_stride = std::max<uint32_t>(sizeof(uint64_t), rid_part->element_stride);
                const int64_t n_mats = (rid_part->selection.element_count > 0)
                    ? rid_part->selection.element_count
                    : static_cast<int64_t>(rid_part->capacity_bytes / r_stride);
                for (int64_t i = 0; i < n_mats; ++i) {
                    const uint64_t raw_id = *reinterpret_cast<const uint64_t*>(rid_part->raw_base_ptr + static_cast<size_t>(i) * r_stride);
                    if (raw_id == 0) continue;
                    godot::RID mat_rid;
                    std::memcpy(&mat_rid, &raw_id, sizeof(uint64_t));
                    if (mat_rid.is_valid()) {
                        rs->material_set_param(mat_rid, parameter_name, _read_value(*val_part, i));
                    }
                }
            } else if (material_rid.is_valid()) {
                const uint32_t stride = std::max<uint32_t>(4, val_part->element_stride);
                const int64_t count = static_cast<int64_t>(val_part->capacity_bytes / stride);
                if (count > 0) {
                    const int64_t idx = std::min<int64_t>(source_index, count - 1);
                    rs->material_set_param(material_rid, parameter_name, _read_value(*val_part, idx));
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

static_assert(sizeof(ShaderMaterialParamSyncTransformLogic<>) == 64, "ShaderMaterialParamSyncTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ShaderMaterialParamSyncTransformLogic<>>, "ShaderMaterialParamSyncTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core