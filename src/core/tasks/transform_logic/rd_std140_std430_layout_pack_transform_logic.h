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
 * G_RD2: RDStd140Std430LayoutPackTransformLogic
 * Transposes planar SoA columns or tightly packed 12-byte Vector3 arrays into GLSL std140
 * (16-byte aligned vec3 + scalar pad) or std430 interleaved SSBO layouts, or unpacks back to SoA.
 */
struct RDStd140Std430LayoutPackTransformLogic {
    static constexpr std::string_view ui_name = "RD GLSL std140 / std430 SSBO Layout Pack";
    static constexpr std::string_view ui_category = "GPU/Packing";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::AOSOA,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::SOA,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::VECTOR4,
        DataType::VECTOR3,
        DataType::FLOAT32
    };

    int32_t layout_standard = 0; // 0 = std140 / padded vec4 (16B per vec3+scalar), 1 = std430 interleaved SoA columns
    int32_t direction_mode = 0;  // 0 = Pack CPU SoA/Vec3 -> GPU SSBO Layout, 1 = Unpack GPU SSBO Layout -> CPU SoA/Vec3
    float default_w_padding = 0.0f;

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
        add_prop("layout_standard", godot::Variant::INT, layout_standard, 2, "std140 (16B Aligned vec3+w),std430 (Interleaved Columns)");
        add_prop("direction_mode", godot::Variant::INT, direction_mode, 2, "Pack (CPU -> SSBO Layout),Unpack (SSBO Layout -> CPU)");
        add_prop("default_w_padding", godot::Variant::FLOAT, default_w_padding);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("layout_standard")) layout_standard = std::clamp<int32_t>(static_cast<int32_t>(p_props["layout_standard"]), 0, 1);
        if (p_props.has("direction_mode")) direction_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["direction_mode"]), 0, 1);
        if (p_props.has("default_w_padding")) default_w_padding = static_cast<float>(p_props["default_w_padding"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        // Last bound part is the packed SSBO layout buffer; preceding parts are CPU columns
        const uint32_t ssbo_idx = p_ctx.grant->part_count - 1;
        GrantPartPOD& ssbo_part = p_ctx.grant->parts[ssbo_idx];
        GrantPartPOD& col0_part = p_ctx.grant->parts[0];
        if (!ssbo_part.raw_base_ptr || !col0_part.raw_base_ptr) return;

        if (layout_standard == 0 && col0_part.element_stride >= sizeof(godot::Vector3)) {
            // Mode 0: Pack/Unpack 12-byte Vector3 (Part 0) + optional scalar float (Part 1) <-> 16-byte std140 vec4
            GrantPartPOD* w_part = (ssbo_idx > 1 && p_ctx.grant->parts[1].raw_base_ptr)
                ? &p_ctx.grant->parts[1]
                : nullptr;

            const uint32_t v3_stride = std::max<uint32_t>(sizeof(godot::Vector3), col0_part.element_stride);
            const int64_t n_elems = std::min<int64_t>(
                static_cast<int64_t>(col0_part.capacity_bytes / v3_stride),
                static_cast<int64_t>(ssbo_part.capacity_bytes / sizeof(godot::Vector4))
            );

            godot::Vector4* ssbo_v4 = reinterpret_cast<godot::Vector4*>(ssbo_part.raw_base_ptr);
            const uint32_t w_stride = w_part ? std::max<uint32_t>(sizeof(float), w_part->element_stride) : sizeof(float);
            const int64_t max_w = w_part ? static_cast<int64_t>(w_part->capacity_bytes / w_stride) : 0;

            if (direction_mode == 0) {
                for (int64_t i = 0; i < n_elems; ++i) {
                    const godot::Vector3& v = *reinterpret_cast<const godot::Vector3*>(col0_part.raw_base_ptr + static_cast<size_t>(i) * v3_stride);
                    const float w = (w_part && i < max_w)
                        ? *reinterpret_cast<const float*>(w_part->raw_base_ptr + static_cast<size_t>(i) * w_stride)
                        : default_w_padding;
                    ssbo_v4[i] = godot::Vector4(v.x, v.y, v.z, w);
                }
            } else {
                for (int64_t i = 0; i < n_elems; ++i) {
                    const godot::Vector4& v4 = ssbo_v4[i];
                    *reinterpret_cast<godot::Vector3*>(col0_part.raw_base_ptr + static_cast<size_t>(i) * v3_stride) =
                        godot::Vector3(v4.x, v4.y, v4.z);
                    if (w_part && i < max_w) {
                        *reinterpret_cast<float*>(w_part->raw_base_ptr + static_cast<size_t>(i) * w_stride) = v4.w;
                    }
                }
            }
        } else {
            // Mode 1: Transpose K planar float SoA columns (Parts 0..K-1) <-> interleaved K-float std430 struct array
            const uint32_t num_cols = std::min<uint32_t>(8u, ssbo_idx);
            int64_t n_elems = static_cast<int64_t>(ssbo_part.capacity_bytes / (num_cols * sizeof(float)));
            for (uint32_t c = 0; c < num_cols; ++c) {
                const GrantPartPOD& cp = p_ctx.grant->parts[c];
                if (!cp.raw_base_ptr) return;
                const uint32_t c_stride = std::max<uint32_t>(sizeof(float), cp.element_stride);
                n_elems = std::min<int64_t>(n_elems, static_cast<int64_t>(cp.capacity_bytes / c_stride));
            }

            float* ssbo_f = reinterpret_cast<float*>(ssbo_part.raw_base_ptr);
            if (direction_mode == 0) {
                for (int64_t i = 0; i < n_elems; ++i) {
                    for (uint32_t c = 0; c < num_cols; ++c) {
                        const GrantPartPOD& cp = p_ctx.grant->parts[c];
                        const uint32_t c_stride = std::max<uint32_t>(sizeof(float), cp.element_stride);
                        ssbo_f[static_cast<size_t>(i) * num_cols + c] =
                            *reinterpret_cast<const float*>(cp.raw_base_ptr + static_cast<size_t>(i) * c_stride);
                    }
                }
            } else {
                for (int64_t i = 0; i < n_elems; ++i) {
                    for (uint32_t c = 0; c < num_cols; ++c) {
                        GrantPartPOD& cp = p_ctx.grant->parts[c];
                        const uint32_t c_stride = std::max<uint32_t>(sizeof(float), cp.element_stride);
                        *reinterpret_cast<float*>(cp.raw_base_ptr + static_cast<size_t>(i) * c_stride) =
                            ssbo_f[static_cast<size_t>(i) * num_cols + c];
                    }
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