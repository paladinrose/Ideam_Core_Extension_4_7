#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class VertexColorTargetChannel : uint8_t {
    ALL_RGBA_GRADIENT = 0, // Interpolates [color_min, color_max] or samples palette LUT
    CHANNEL_RED       = 1, // Writes normalized [0, 1] scalar into COLOR.r
    CHANNEL_GREEN     = 2, // Writes normalized [0, 1] scalar into COLOR.g
    CHANNEL_BLUE      = 3, // Writes normalized [0, 1] scalar into COLOR.b
    CHANNEL_ALPHA     = 4, // Writes normalized [0, 1] scalar into COLOR.a
    PALETTE_INDEX     = 5  // Uses discrete integer scalar as direct index into palette LUT
};

/**
 * MeshVertexColorMapTransformLogic<T_Scalar>
 * Maps per-vertex DOD simulation scalars (heat, plastic strain, shield energy, curvature)
 * or discrete indices into a mesh's ARRAY_COLOR buffer (godot::Color, Vector4, or RGBA8 uint32_t).
 * Supports single-channel injection (R, G, B, or A) for multi-property shader encoding
 * as well as full RGBA two-stop or PackedColorArray LUT gradients.
 */
template <typename T_Scalar = float>
struct alignas(64) MeshVertexColorMapTransformLogic {
    using ValueType       = T_Scalar;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = SingleElementView<T_Scalar, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_LINEAR;
    static constexpr DataType required_types              = DataType::ANY_NUMERIC | DataType::COLOR;

    // --- Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Mesh Vertex Color Map";

    // --- Configuration State (64-Byte Cache-Aligned) ---
    uint32_t scalar_buffer_id  = INVALID_ID; // 4B: Source scalar/index buffer (bound to primary view if T_Scalar is numeric)
    uint32_t color_buffer_id   = INVALID_ID; // 4B: Target ARRAY_COLOR buffer (godot::Color, Vector4, or packed uint32_t)
    float min_value            = 0.0f;       // 4B: Scalar normalization lower bound
    float max_value            = 1.0f;       // 4B: Scalar normalization upper bound
    uint32_t color_min_rgba8   = 0x000000FFu;// 4B: Low stop RGBA8 (default opaque black)
    uint32_t color_max_rgba8   = 0xFFFFFFFFu;// 4B: High stop RGBA8 (default opaque white)
    VertexColorTargetChannel target_channel = VertexColorTargetChannel::ALL_RGBA_GRADIENT; // 1B
    bool invert                = false;      // 1B: Invert normalized [0, 1] value
    bool pack_as_rgba8         = false;      // 1B: Write 4-byte RGBA8 instead of 16-byte float Color
    uint8_t reserved_flags     = 0;          // 1B
    uint32_t reserved_pad0     = 0;          // 4B (Aligns PackedColorArray to 32B boundary)

    godot::PackedColorArray palette;         // 16B: Optional multi-stop gradient / discrete LUT

    uint8_t reserved_padding[16] = {0};      // 16B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary sbuf_prop;
        sbuf_prop["name"] = "scalar_buffer_id";
        sbuf_prop["type"] = godot::Variant::INT;
        sbuf_prop["hint_string"] = "buffer_option";
        props.push_back(sbuf_prop);

        godot::Dictionary cbuf_prop;
        cbuf_prop["name"] = "color_buffer_id";
        cbuf_prop["type"] = godot::Variant::INT;
        cbuf_prop["hint_string"] = "buffer_option";
        props.push_back(cbuf_prop);

        godot::Dictionary ch_prop;
        ch_prop["name"] = "target_channel";
        ch_prop["type"] = godot::Variant::INT;
        ch_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        ch_prop["hint_string"] = "All RGBA (Gradient/LUT):0,Red Channel (R):1,Green Channel (G):2,Blue Channel (B):3,Alpha Channel (A):4,Discrete Palette Index:5";
        props.push_back(ch_prop);

        godot::Dictionary min_prop;
        min_prop["name"] = "min_value";
        min_prop["type"] = godot::Variant::FLOAT;
        min_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(min_prop);

        godot::Dictionary max_prop;
        max_prop["name"] = "max_value";
        max_prop["type"] = godot::Variant::FLOAT;
        max_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(max_prop);

        godot::Dictionary cmin_prop;
        cmin_prop["name"] = "color_min";
        cmin_prop["type"] = godot::Variant::COLOR;
        cmin_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cmin_prop);

        godot::Dictionary cmax_prop;
        cmax_prop["name"] = "color_max";
        cmax_prop["type"] = godot::Variant::COLOR;
        cmax_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(cmax_prop);

        godot::Dictionary pal_prop;
        pal_prop["name"] = "palette";
        pal_prop["type"] = godot::Variant::PACKED_COLOR_ARRAY;
        pal_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(pal_prop);

        godot::Dictionary inv_prop;
        inv_prop["name"] = "invert";
        inv_prop["type"] = godot::Variant::BOOL;
        inv_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(inv_prop);

        godot::Dictionary rgba8_prop;
        rgba8_prop["name"] = "pack_as_rgba8";
        rgba8_prop["type"] = godot::Variant::BOOL;
        rgba8_prop["hint"] = godot::PROPERTY_HINT_NONE;
        props.push_back(rgba8_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        if constexpr (std::is_same_v<T_Scalar, godot::Color>) {
            return (color_buffer_id != INVALID_ID) ? color_buffer_id : scalar_buffer_id;
        } else {
            return (scalar_buffer_id != INVALID_ID) ? scalar_buffer_id : color_buffer_id;
        }
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("scalar_buffer_id")) {
            scalar_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["scalar_buffer_id"]));
        } else if (p_props.has("source_buffer_id")) {
            scalar_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        }
        if (p_props.has("color_buffer_id")) {
            color_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["color_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && color_buffer_id == INVALID_ID) {
            color_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("target_channel")) {
            target_channel = static_cast<VertexColorTargetChannel>(static_cast<uint8_t>(static_cast<int64_t>(p_props["target_channel"])));
        }
        if (p_props.has("min_value")) {
            min_value = static_cast<float>(p_props["min_value"]);
        }
        if (p_props.has("max_value")) {
            max_value = static_cast<float>(p_props["max_value"]);
        }
        if (p_props.has("color_min")) {
            const godot::Color c = static_cast<godot::Color>(p_props["color_min"]);
            color_min_rgba8 = _pack_rgba8(c);
        }
        if (p_props.has("color_max")) {
            const godot::Color c = static_cast<godot::Color>(p_props["color_max"]);
            color_max_rgba8 = _pack_rgba8(c);
        }
        if (p_props.has("palette")) {
            palette = p_props["palette"];
        }
        if (p_props.has("invert")) {
            invert = static_cast<bool>(p_props["invert"]);
        }
        if (p_props.has("pack_as_rgba8")) {
            pack_as_rgba8 = static_cast<bool>(p_props["pack_as_rgba8"]);
        }
    }

    template <typename T_View, typename T_Strategy>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        if constexpr (std::is_arithmetic_v<T_Scalar> || std::is_same_v<T_Scalar, godot::Color>) {
            const GrantPartPOD* s_part = p_context.get_grant_part(scalar_buffer_id);
            const GrantPartPOD* c_part = p_context.get_grant_part(color_buffer_id);
            if (!s_part || !c_part || !s_part->raw_base_ptr || !c_part->raw_base_ptr) return;

            const float diff     = max_value - min_value;
            const float inv_diff = (std::abs(diff) > 1e-7f) ? (1.0f / diff) : 0.0f;

            const godot::Color c_min = _unpack_rgba8(color_min_rgba8);
            const godot::Color c_max = _unpack_rgba8(color_max_rgba8);

            const int64_t pal_size = palette.size();
            const godot::Color* __restrict pal_ptr = (pal_size > 0) ? palette.ptr() : nullptr;

            const size_t scalar_elem_size = std::is_same_v<T_Scalar, godot::Color> ? sizeof(float) : sizeof(T_Scalar);
            const size_t max_scalars = s_part->capacity_bytes / scalar_elem_size;
            const size_t color_stride = pack_as_rgba8 ? sizeof(uint32_t) : sizeof(godot::Color);
            const size_t max_colors  = c_part->capacity_bytes / color_stride;
            const int64_t max_count  = static_cast<int64_t>(std::min(max_scalars, max_colors));
            if (max_count <= 0) return;

            const float* __restrict fallback_float_ptr = reinterpret_cast<const float*>(s_part->raw_base_ptr);
            godot::Color* __restrict dst_colors        = reinterpret_cast<godot::Color*>(c_part->raw_base_ptr);
            uint32_t* __restrict dst_rgba8             = reinterpret_cast<uint32_t*>(c_part->raw_base_ptr);

            auto sample_gradient = [&](float t) noexcept -> godot::Color {
                if (pal_ptr && pal_size > 0) {
                    if (pal_size == 1) return pal_ptr[0];
                    const float scaled = t * static_cast<float>(pal_size - 1);
                    const int64_t i0   = std::clamp<int64_t>(static_cast<int64_t>(scaled), 0, pal_size - 1);
                    const int64_t i1   = std::min<int64_t>(i0 + 1, pal_size - 1);
                    const float frac   = scaled - static_cast<float>(i0);
                    return godot::Color(
                        pal_ptr[i0].r + (pal_ptr[i1].r - pal_ptr[i0].r) * frac,
                        pal_ptr[i0].g + (pal_ptr[i1].g - pal_ptr[i0].g) * frac,
                        pal_ptr[i0].b + (pal_ptr[i1].b - pal_ptr[i0].b) * frac,
                        pal_ptr[i0].a + (pal_ptr[i1].a - pal_ptr[i0].a) * frac
                    );
                }
                return godot::Color(
                    c_min.r + (c_max.r - c_min.r) * t,
                    c_min.g + (c_max.g - c_min.g) * t,
                    c_min.b + (c_max.b - c_min.b) * t,
                    c_min.a + (c_max.a - c_min.a) * t
                );
            };

            auto process_vertex = [&](int64_t idx) noexcept {
                if (idx < 0 || idx >= max_count) return;

                float raw_scalar = 0.0f;
                if constexpr (std::is_arithmetic_v<T_Scalar>) {
                    raw_scalar = static_cast<float>(_read_view(p_view, idx));
                } else {
                    raw_scalar = fallback_float_ptr[idx];
                }

                if (target_channel == VertexColorTargetChannel::PALETTE_INDEX && pal_ptr && pal_size > 0) {
                    const int64_t lut_idx = std::clamp<int64_t>(static_cast<int64_t>(raw_scalar), 0, pal_size - 1);
                    const godot::Color resolved = pal_ptr[lut_idx];
                    if (pack_as_rgba8) {
                        dst_rgba8[idx] = _pack_rgba8(resolved);
                    } else {
                        dst_colors[idx] = resolved;
                    }
                    return;
                }

                float norm = std::clamp((raw_scalar - min_value) * inv_diff, 0.0f, 1.0f);
                if (invert) norm = 1.0f - norm;

                if (target_channel == VertexColorTargetChannel::ALL_RGBA_GRADIENT) {
                    const godot::Color resolved = sample_gradient(norm);
                    if (pack_as_rgba8) {
                        dst_rgba8[idx] = _pack_rgba8(resolved);
                    } else {
                        dst_colors[idx] = resolved;
                    }
                } else {
                    if (pack_as_rgba8) {
                        uint8_t rgba[4];
                        std::memcpy(rgba, &dst_rgba8[idx], 4);
                        const uint8_t byte_val = static_cast<uint8_t>(std::clamp(norm * 255.0f + 0.5f, 0.0f, 255.0f));
                        const size_t ch = static_cast<size_t>(target_channel) - 1;
                        if (ch < 4) rgba[ch] = byte_val;
                        std::memcpy(&dst_rgba8[idx], rgba, 4);
                    } else {
                        godot::Color cur = dst_colors[idx];
                        switch (target_channel) {
                            case VertexColorTargetChannel::CHANNEL_RED:   cur.r = norm; break;
                            case VertexColorTargetChannel::CHANNEL_GREEN: cur.g = norm; break;
                            case VertexColorTargetChannel::CHANNEL_BLUE:  cur.b = norm; break;
                            case VertexColorTargetChannel::CHANNEL_ALPHA: cur.a = norm; break;
                            default: break;
                        }
                        dst_colors[idx] = cur;
                    }
                }
            };

            const MemoryBufferSelectionPOD& sel = s_part->selection;
            if (sel.mode == SelectionMode::DENSE && sel.data.bitset) {
                const uint64_t* bitset = sel.data.bitset;
                const int64_t limit = std::min<int64_t>(sel.capacity, max_count);
                for (int64_t i = 0; i < limit; ++i) {
                    if (bitset[i >> 6] & (1ULL << (i & 63))) process_vertex(i);
                }
            } else if (sel.mode == SelectionMode::SPARSE && sel.data.indices) {
                for (int64_t i = 0; i < sel.element_count; ++i) {
                    process_vertex(sel.data.indices[i]);
                }
            } else {
                const int64_t end = std::min<int64_t>(sel.start_index + sel.element_count, max_count);
                for (int64_t i = sel.start_index; i < end; ++i) {
                    process_vertex(i);
                }
            }
        }
    }

private:
    [[nodiscard]] static inline uint32_t _pack_rgba8(const godot::Color& p_c) noexcept {
        const uint32_t r = static_cast<uint32_t>(std::clamp(p_c.r * 255.0f + 0.5f, 0.0f, 255.0f));
        const uint32_t g = static_cast<uint32_t>(std::clamp(p_c.g * 255.0f + 0.5f, 0.0f, 255.0f));
        const uint32_t b = static_cast<uint32_t>(std::clamp(p_c.b * 255.0f + 0.5f, 0.0f, 255.0f));
        const uint32_t a = static_cast<uint32_t>(std::clamp(p_c.a * 255.0f + 0.5f, 0.0f, 255.0f));
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    [[nodiscard]] static inline godot::Color _unpack_rgba8(uint32_t p_rgba) noexcept {
        constexpr float inv_255 = 1.0f / 255.0f;
        return godot::Color(
            static_cast<float>((p_rgba >> 24) & 0xFFu) * inv_255,
            static_cast<float>((p_rgba >> 16) & 0xFFu) * inv_255,
            static_cast<float>((p_rgba >> 8)  & 0xFFu) * inv_255,
            static_cast<float>(p_rgba & 0xFFu)         * inv_255
        );
    }

    template <typename T_View>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    static inline T_Scalar _read_view(const T_View& p_view, int64_t idx) noexcept {
        if constexpr (requires { p_view[idx].read(); }) {
            return static_cast<T_Scalar>(p_view[idx].read());
        } else if constexpr (requires { p_view[idx].load(); }) {
            return static_cast<T_Scalar>(p_view[idx].load());
        } else if constexpr (std::is_pointer_v<decltype(p_view[idx])>) {
            return *reinterpret_cast<const T_Scalar*>(p_view[idx]);
        } else {
            return static_cast<T_Scalar>(p_view[idx]);
        }
    }
};

static_assert(sizeof(MeshVertexColorMapTransformLogic<>) == 64, "MeshVertexColorMapTransformLogic must be exactly 64 bytes!");

} // namespace ideam::core