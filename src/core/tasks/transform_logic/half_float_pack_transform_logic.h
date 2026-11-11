#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../i_native_task.h"
#include "../task_common.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <string_view>

namespace ideam::core {

/**
 * S6: HalfFloatPackTransformLogic
 * Packs 32-bit IEEE 754 float / Vector4 lanes into 16-bit IEEE 754 float16 (uint16_t / uint64_t RGBA16F)
 * or unpacks float16 back to 32-bit float.
 */
struct HalfFloatPackTransformLogic {
    static constexpr std::string_view ui_name = "IEEE 754 Half-Float (FP16) Pack/Unpack";
    static constexpr std::string_view ui_category = "Shader/Packing";

    static constexpr std::array<MemoryView, 2> supported_views = {
        MemoryView::AOSOA,
        MemoryView::SINGLE_ELEMENT
    };
    static constexpr std::array<MemoryStrategy, 2> supported_strategies = {
        MemoryStrategy::FLAT,
        MemoryStrategy::SOA
    };
    static constexpr std::array<DataType, 3> supported_types = {
        DataType::FLOAT32,
        DataType::VECTOR4,
        DataType::UINT32
    };

    int32_t direction_mode = 0; // 0 = Pack FP32 -> FP16, 1 = Unpack FP16 -> FP32

    godot::Array get_property_list() const {
        godot::Array list;
        godot::Dictionary d;
        d["name"] = "direction_mode";
        d["type"] = static_cast<int>(godot::Variant::INT);
        d["default"] = direction_mode;
        d["hint"] = 2;
        d["hint_string"] = "Pack FP32 -> FP16,Unpack FP16 -> FP32";
        list.push_back(d);
        return list;
    }

    void apply_properties(const godot::Dictionary& p_props) {
        if (p_props.has("direction_mode")) direction_mode = std::clamp<int32_t>(static_cast<int32_t>(p_props["direction_mode"]), 0, 1);
    }

    size_t get_transient_requirement(const TaskContextPOD& /*p_ctx*/) const noexcept { return 0; }
    size_t get_transient_requirement() const noexcept { return 0; }

    static inline uint16_t float32_to_float16(float value) noexcept {
        const uint32_t f32 = std::bit_cast<uint32_t>(value);
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

    static inline float float16_to_float32(uint16_t h) noexcept {
        const uint32_t sign = static_cast<uint32_t>(h & 0x8000u) << 16;
        uint32_t exp = (h >> 10) & 0x1Fu;
        uint32_t mant = h & 0x03FFu;

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

    void execute(const TaskContextPOD& p_ctx) const {
        if (!p_ctx.grant || p_ctx.grant->part_count < 2) return;

        const GrantPartPOD& src_part = p_ctx.grant->parts[0];
        GrantPartPOD& dst_part = p_ctx.grant->parts[1];
        if (!src_part.raw_base_ptr || !dst_part.raw_base_ptr) return;

        if (direction_mode == 0) {
            const size_t n_floats = std::min<size_t>(
                src_part.capacity_bytes / sizeof(float),
                dst_part.capacity_bytes / sizeof(uint16_t)
            );
            const float* src_f = reinterpret_cast<const float*>(src_part.raw_base_ptr);
            uint16_t* dst_h = reinterpret_cast<uint16_t*>(dst_part.raw_base_ptr);
            for (size_t i = 0; i < n_floats; ++i) {
                dst_h[i] = float32_to_float16(src_f[i]);
            }
        } else {
            const size_t n_halves = std::min<size_t>(
                src_part.capacity_bytes / sizeof(uint16_t),
                dst_part.capacity_bytes / sizeof(float)
            );
            const uint16_t* src_h = reinterpret_cast<const uint16_t*>(src_part.raw_base_ptr);
            float* dst_f = reinterpret_cast<float*>(dst_part.raw_base_ptr);
            for (size_t i = 0; i < n_halves; ++i) {
                dst_f[i] = float16_to_float32(src_h[i]);
            }
        }
    }

    template <typename TView>
    void execute(TView& /*p_view*/, const TaskContextPOD& p_ctx) const {
        execute(p_ctx);
    }
};

} // namespace ideam::core