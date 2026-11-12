#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/aosoa_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * CustomDataPackTransformLogic<T> (S2)
 * Packs up to 4 scalar float DOD columns (with per-channel scale and bias) into a contiguous
 * Color / Vector4 buffer for MultiMesh INSTANCE_CUSTOM or mesh ARRAY_COLOR attributes.
 */
template <typename T = godot::Color>
struct alignas(64) CustomDataPackTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = SoAStrategy;
    using DefaultView     = AOSOAView<T, 4, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::SOA | BufferLayoutType::TILED_SOA | BufferLayoutType::FLAT;
    static constexpr DataType required_types              = DataType::COLOR | DataType::VECTOR4 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Shader Custom Data / Color 4-Channel Pack";
    static constexpr std::string_view ui_category  = "Shader/Packing";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t output_buffer_id    = INVALID_ID; // 4B: Destination Color or Vector4 buffer
    uint32_t channel_r_buffer_id = INVALID_ID; // 4B: Scalar float source column for R / X
    uint32_t channel_g_buffer_id = INVALID_ID; // 4B: Scalar float source column for G / Y
    uint32_t channel_b_buffer_id = INVALID_ID; // 4B: Scalar float source column for B / Z
    uint32_t channel_a_buffer_id = INVALID_ID; // 4B: Scalar float source column for A / W

    godot::Vector4 channel_scale{1.0f, 1.0f, 1.0f, 1.0f}; // 16B: Per-channel scale multiplier
    godot::Vector4 channel_bias{0.0f, 0.0f, 0.0f, 0.0f};  // 16B: Per-channel additive offset

    float fallback_alpha        = 1.0f;  // 4B: Default alpha when channel_a_buffer_id is unbound
    bool clamp_unit_range       = false; // 1B: Clamp packed channels to [0.0, 1.0]
    uint8_t reserved_padding[7] = {0};   // 7B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary out_prop;
        out_prop["name"] = "output_buffer_id";
        out_prop["type"] = godot::Variant::INT;
        out_prop["hint_string"] = "buffer_option";
        props.push_back(out_prop);

        godot::Dictionary r_prop;
        r_prop["name"] = "channel_r_buffer_id";
        r_prop["type"] = godot::Variant::INT;
        r_prop["hint_string"] = "buffer_option";
        props.push_back(r_prop);

        godot::Dictionary g_prop;
        g_prop["name"] = "channel_g_buffer_id";
        g_prop["type"] = godot::Variant::INT;
        g_prop["hint_string"] = "buffer_option";
        props.push_back(g_prop);

        godot::Dictionary b_prop;
        b_prop["name"] = "channel_b_buffer_id";
        b_prop["type"] = godot::Variant::INT;
        b_prop["hint_string"] = "buffer_option";
        props.push_back(b_prop);

        godot::Dictionary a_prop;
        a_prop["name"] = "channel_a_buffer_id";
        a_prop["type"] = godot::Variant::INT;
        a_prop["hint_string"] = "buffer_option";
        props.push_back(a_prop);

        godot::Dictionary scl_prop;
        scl_prop["name"] = "channel_scale";
        scl_prop["type"] = godot::Variant::VECTOR4;
        props.push_back(scl_prop);

        godot::Dictionary bis_prop;
        bis_prop["name"] = "channel_bias";
        bis_prop["type"] = godot::Variant::VECTOR4;
        props.push_back(bis_prop);

        godot::Dictionary fba_prop;
        fba_prop["name"] = "fallback_alpha";
        fba_prop["type"] = godot::Variant::FLOAT;
        props.push_back(fba_prop);

        godot::Dictionary clp_prop;
        clp_prop["name"] = "clamp_unit_range";
        clp_prop["type"] = godot::Variant::BOOL;
        props.push_back(clp_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if (output_buffer_id != INVALID_ID) return output_buffer_id;
        return channel_r_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("output_buffer_id")) {
            output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && output_buffer_id == INVALID_ID) {
            output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("channel_r_buffer_id")) {
            channel_r_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["channel_r_buffer_id"]));
        }
        if (p_props.has("channel_g_buffer_id")) {
            channel_g_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["channel_g_buffer_id"]));
        }
        if (p_props.has("channel_b_buffer_id")) {
            channel_b_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["channel_b_buffer_id"]));
        }
        if (p_props.has("channel_a_buffer_id")) {
            channel_a_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["channel_a_buffer_id"]));
        }
        if (p_props.has("channel_scale")) {
            channel_scale = static_cast<godot::Vector4>(p_props["channel_scale"]);
        }
        if (p_props.has("channel_bias")) {
            channel_bias = static_cast<godot::Vector4>(p_props["channel_bias"]);
        }
        if (p_props.has("fallback_alpha")) {
            fallback_alpha = static_cast<float>(p_props["fallback_alpha"]);
        }
        if (p_props.has("clamp_unit_range")) {
            clamp_unit_range = static_cast<bool>(p_props["clamp_unit_range"]);
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
        if constexpr (std::is_same_v<T, godot::Color> || std::is_same_v<T, godot::Vector4> || std::is_same_v<T, float>) {
            const GrantPartPOD* out_part = p_context.get_grant_part(output_buffer_id);
            if (!out_part || !out_part->raw_base_ptr) return;

            const uint32_t out_stride = std::max<uint32_t>(sizeof(godot::Color), out_part->element_stride);
            const int64_t max_out = static_cast<int64_t>(out_part->capacity_bytes / out_stride);
            const int64_t n_elems = (out_part->selection.element_count > 0)
                ? std::min<int64_t>(out_part->selection.element_count, max_out)
                : max_out;
            if (n_elems <= 0) return;

            const uint32_t ch_ids[4] = { channel_r_buffer_id, channel_g_buffer_id, channel_b_buffer_id, channel_a_buffer_id };
            const float scales[4]    = { channel_scale.x, channel_scale.y, channel_scale.z, channel_scale.w };
            const float biases[4]    = { channel_bias.x, channel_bias.y, channel_bias.z, channel_bias.w };
            const float defs[4]      = { 0.0f, 0.0f, 0.0f, fallback_alpha };

            const uint8_t* src_ptrs[4]  = { nullptr, nullptr, nullptr, nullptr };
            uint32_t src_strides[4]     = { sizeof(float), sizeof(float), sizeof(float), sizeof(float) };
            int64_t src_counts[4]       = { 0, 0, 0, 0 };

            for (int c = 0; c < 4; ++c) {
                if (ch_ids[c] != INVALID_ID) {
                    if (const GrantPartPOD* sp = p_context.get_grant_part(ch_ids[c])) {
                        if (sp->raw_base_ptr) {
                            src_ptrs[c]    = sp->raw_base_ptr;
                            src_strides[c] = std::max<uint32_t>(sizeof(float), sp->element_stride);
                            src_counts[c]  = static_cast<int64_t>(sp->capacity_bytes / src_strides[c]);
                        }
                    }
                }
            }

            for (int64_t i = 0; i < n_elems; ++i) {
                float ch[4];
                for (int c = 0; c < 4; ++c) {
                    float raw = defs[c];
                    if (src_ptrs[c] && i < src_counts[c]) {
                        raw = *reinterpret_cast<const float*>(src_ptrs[c] + static_cast<size_t>(i) * src_strides[c]);
                    }
                    float val = raw * scales[c] + biases[c];
                    if (clamp_unit_range) val = std::clamp(val, 0.0f, 1.0f);
                    ch[c] = val;
                }

                *reinterpret_cast<godot::Color*>(out_part->raw_base_ptr + static_cast<size_t>(i) * out_stride) =
                    godot::Color(ch[0], ch[1], ch[2], ch[3]);
            }
        }
    }
};

static_assert(sizeof(CustomDataPackTransformLogic<>) == 64, "CustomDataPackTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<CustomDataPackTransformLogic<>>, "CustomDataPackTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core