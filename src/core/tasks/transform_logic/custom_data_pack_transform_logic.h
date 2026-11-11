#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * S2: CustomDataPackTransformLogic
 * Packs up to 4 scalar float DOD columns (with per-channel scale and bias) into a contiguous
 * Color / Vector4 buffer for MultiMesh INSTANCE_CUSTOM or mesh ARRAY_COLOR attributes.
 */
struct CustomDataPackTransformLogic {
    static constexpr std::string_view ui_name = "Shader Custom Data / Color 4-Channel Pack";
    static constexpr std::string_view ui_category = "Shader/Packing";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::AOSOA,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::SOA,
        MemoryStrategy::FLAT
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::COLOR,
        DataType::VECTOR4,
        DataType::FLOAT32
    };

    godot::Vector4 channel_scale = godot::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    godot::Vector4 channel_bias = godot::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    godot::Vector4 fallback_defaults = godot::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    bool clamp_unit_range = false;

    godot::Array get_property_list() const {
        godot::Array list;
        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            list.push_back(d);
        };
        add_prop("channel_scale", godot::Variant::VECTOR4, channel_scale);
        add_prop("channel_bias", godot::Variant::VECTOR4, channel_bias);
        add_prop("fallback_defaults", godot::Variant::VECTOR4, fallback_defaults);
        add_prop("clamp_unit_range", godot::Variant::BOOL, clamp_unit_range);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("channel_scale")) channel_scale = p_props["channel_scale"];
        if (p_props.has("channel_bias")) channel_bias = p_props["channel_bias"];
        if (p_props.has("fallback_defaults")) fallback_defaults = p_props["fallback_defaults"];
        if (p_props.has("clamp_unit_range")) clamp_unit_range = static_cast<bool>(p_props["clamp_unit_range"]);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        // The last bound GrantPart is the destination Color/Vector4 buffer; preceding parts are scalar channels 0..3
        const uint32_t out_idx = p_ctx.grant->part_count - 1;
        GrantPartPOD& out_part = p_ctx.grant->parts[out_idx];
        if (!out_part.raw_base_ptr) return;

        const uint32_t out_stride = std::max<uint32_t>(sizeof(godot::Color), out_part.element_stride);
        const int64_t max_out = static_cast<int64_t>(out_part.capacity_bytes / out_stride);
        const int64_t n_elems = (out_part.selection.element_count > 0)
            ? std::min<int64_t>(out_part.selection.element_count, max_out)
            : max_out;
        if (n_elems <= 0) return;

        const float scales[4] = { channel_scale.x, channel_scale.y, channel_scale.z, channel_scale.w };
        const float biases[4] = { channel_bias.x, channel_bias.y, channel_bias.z, channel_bias.w };
        const float defs[4]   = { fallback_defaults.x, fallback_defaults.y, fallback_defaults.z, fallback_defaults.w };

        const uint32_t num_src_channels = std::min<uint32_t>(4u, out_idx);
        const float* src_ptrs[4] = { nullptr, nullptr, nullptr, nullptr };
        uint32_t src_strides[4] = { sizeof(float), sizeof(float), sizeof(float), sizeof(float) };
        int64_t src_counts[4] = { 0, 0, 0, 0 };

        for (uint32_t c = 0; c < num_src_channels; ++c) {
            const GrantPartPOD& sp = p_ctx.grant->parts[c];
            if (sp.raw_base_ptr) {
                src_ptrs[c] = reinterpret_cast<const float*>(sp.raw_base_ptr);
                src_strides[c] = std::max<uint32_t>(sizeof(float), sp.element_stride);
                src_counts[c] = static_cast<int64_t>(sp.capacity_bytes / src_strides[c]);
            }
        }

        for (int64_t i = 0; i < n_elems; ++i) {
            float ch[4];
            for (int c = 0; c < 4; ++c) {
                float raw = defs[c];
                if (src_ptrs[c] && i < src_counts[c]) {
                    raw = *reinterpret_cast<const float*>(
                        reinterpret_cast<const uint8_t*>(src_ptrs[c]) + static_cast<size_t>(i) * src_strides[c]
                    );
                }
                float val = raw * scales[c] + biases[c];
                if (clamp_unit_range) val = std::clamp(val, 0.0f, 1.0f);
                ch[c] = val;
            }

            *reinterpret_cast<godot::Color*>(out_part.raw_base_ptr + static_cast<size_t>(i) * out_stride) =
                godot::Color(ch[0], ch[1], ch[2], ch[3]);
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core