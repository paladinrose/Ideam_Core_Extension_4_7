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
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class HalfFloatConversionDirection : uint8_t {
    PACK_FP32_TO_FP16   = 0,
    UNPACK_FP16_TO_FP32 = 1
};

/**
 * HalfFloatPackTransformLogic<T> (S6)
 * Packs 32-bit IEEE 754 float / Vector4 lanes into 16-bit IEEE 754 float16 (uint16_t / uint64_t RGBA16F)
 * or unpacks float16 back to 32-bit float.
 */
template <typename T = float>
struct alignas(64) HalfFloatPackTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = FlatStrategy;
    using DefaultView     = AOSOAView<T, 8, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::SOA | BufferLayoutType::TILED_SOA | BufferLayoutType::FLAT;
    static constexpr DataType required_types              = DataType::FLOAT32 | DataType::VECTOR4 | DataType::INT32 | DataType::BYTE;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "IEEE 754 Half-Float (FP16) Pack/Unpack";
    static constexpr std::string_view ui_category  = "Shader/Packing";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t source_buffer_id                   = INVALID_ID; // 4B: Source FP32 or FP16 buffer
    uint32_t output_buffer_id                   = INVALID_ID; // 4B: Destination FP16 or FP32 buffer
    HalfFloatConversionDirection direction_mode = HalfFloatConversionDirection::PACK_FP32_TO_FP16; // 1B
    uint8_t reserved_padding[55]                = {0};        // 55B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary sbuf_prop;
        sbuf_prop["name"] = "source_buffer_id";
        sbuf_prop["type"] = godot::Variant::INT;
        sbuf_prop["hint_string"] = "buffer_option";
        props.push_back(sbuf_prop);

        godot::Dictionary obuf_prop;
        obuf_prop["name"] = "output_buffer_id";
        obuf_prop["type"] = godot::Variant::INT;
        obuf_prop["hint_string"] = "buffer_option";
        props.push_back(obuf_prop);

        godot::Dictionary dir_prop;
        dir_prop["name"] = "direction_mode";
        dir_prop["type"] = godot::Variant::INT;
        dir_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dir_prop["hint_string"] = "Pack FP32 -> FP16:0,Unpack FP16 -> FP32:1";
        props.push_back(dir_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (output_buffer_id != INVALID_ID) ? output_buffer_id : source_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("source_buffer_id")) {
            source_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["source_buffer_id"]));
        }
        if (p_props.has("output_buffer_id")) {
            output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["output_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && output_buffer_id == INVALID_ID) {
            output_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("direction_mode")) {
            direction_mode = static_cast<HalfFloatConversionDirection>(std::clamp<int64_t>(static_cast<int64_t>(p_props["direction_mode"]), 0, 1));
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
        if constexpr (std::is_same_v<T, float> || std::is_same_v<T, godot::Vector4> ||
                      std::is_same_v<T, int32_t> || std::is_same_v<T, uint8_t>) {
            const GrantPartPOD* src_part = p_context.get_grant_part(source_buffer_id);
            const GrantPartPOD* dst_part = p_context.get_grant_part(output_buffer_id);
            if (!src_part || !dst_part || !src_part->raw_base_ptr || !dst_part->raw_base_ptr) return;

            if (direction_mode == HalfFloatConversionDirection::PACK_FP32_TO_FP16) {
                const size_t n_floats = std::min<size_t>(
                    src_part->capacity_bytes / sizeof(float),
                    dst_part->capacity_bytes / sizeof(uint16_t)
                );
                const float* src_f = reinterpret_cast<const float*>(src_part->raw_base_ptr);
                uint16_t* dst_h    = reinterpret_cast<uint16_t*>(dst_part->raw_base_ptr);
                for (size_t i = 0; i < n_floats; ++i) {
                    dst_h[i] = _float32_to_float16(src_f[i]);
                }
            } else {
                const size_t n_halves = std::min<size_t>(
                    src_part->capacity_bytes / sizeof(uint16_t),
                    dst_part->capacity_bytes / sizeof(float)
                );
                const uint16_t* src_h = reinterpret_cast<const uint16_t*>(src_part->raw_base_ptr);
                float* dst_f          = reinterpret_cast<float*>(dst_part->raw_base_ptr);
                for (size_t i = 0; i < n_halves; ++i) {
                    dst_f[i] = _float16_to_float32(src_h[i]);
                }
            }
        }
    }

private:
    static inline uint16_t _float32_to_float16(float p_value) noexcept {
        const uint32_t f32 = std::bit_cast<uint32_t>(p_value);
        const uint32_t sign = (f32 >> 16) & 0x8000u;
        const int32_t exp = static_cast<int32_t>((f32 >> 23) & 0xFFu) - 127 + 15;
        uint32_t mant = f32 & 0x007FFFFFu;

        if (exp <= 0) {
            if (exp < -10) return static_cast<uint16_t>(sign);
            mant = (mant | 0x00800000u) >> (1 - exp);
            return static_cast<uint16_t>(sign | ((mant + 0x00001000u) >> 13));
        }
        if (exp >= 31) {
            if (exp == 143 && mant != 0) {
                return static_cast<uint16_t>(sign | 0x7C00u | (mant >> 13) | 1u);
            }
            return static_cast<uint16_t>(sign | 0x7C00u);
        }
        return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exp) << 10) | ((mant + 0x00001000u) >> 13));
    }

    static inline float _float16_to_float32(uint16_t p_h) noexcept {
        const uint32_t sign = static_cast<uint32_t>(p_h & 0x8000u) << 16;
        uint32_t exp = (p_h >> 10) & 0x1Fu;
        uint32_t mant = p_h & 0x03FFu;

        if (exp == 0) {
            if (mant == 0) return std::bit_cast<float>(sign);
            exp = 1;
            while ((mant & 0x0400u) == 0) {
                mant <<= 1;
                --exp;
            }
            mant &= 0x03FFu;
            const uint32_t f32 = sign | ((exp + (127 - 15)) << 23) | (mant << 13);
            return std::bit_cast<float>(f32);
        }
        if (exp == 31) {
            const uint32_t f32 = sign | 0x7F800000u | (mant << 13);
            return std::bit_cast<float>(f32);
        }
        const uint32_t f32 = sign | ((exp + (127 - 15)) << 23) | (mant << 13);
        return std::bit_cast<float>(f32);
    }
};

static_assert(sizeof(HalfFloatPackTransformLogic<>) == 64, "HalfFloatPackTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<HalfFloatPackTransformLogic<>>, "HalfFloatPackTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core