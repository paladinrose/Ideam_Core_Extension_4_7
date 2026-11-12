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
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class RDLayoutStandard : uint8_t {
    STD140_VEC4_PADDED      = 0,
    STD430_INTERLEAVED_COLS = 1
};

enum class RDLayoutDirection : uint8_t {
    PACK_CPU_TO_SSBO   = 0,
    UNPACK_SSBO_TO_CPU = 1
};

/**
 * RDStd140Std430LayoutPackTransformLogic<T> (G_RD2)
 * Transposes planar SoA columns or tightly packed 12-byte Vector3 arrays into GLSL std140
 * (16-byte aligned vec3 + scalar pad) or std430 interleaved SSBO layouts, or unpacks back to SoA.
 */
template <typename T = godot::Vector4>
struct alignas(64) RDStd140Std430LayoutPackTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = SoAStrategy;
    using DefaultView     = AOSOAView<T, 4, DefaultStrategy>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::SOA | BufferLayoutType::TILED_SOA | BufferLayoutType::FLAT;
    static constexpr DataType required_types              = DataType::VECTOR4 | DataType::VECTOR3 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = 0;
    static constexpr bool requires_static_kernel = false;
    static constexpr size_t kernel_size          = 0;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "RD GLSL std140 / std430 SSBO Layout Pack";
    static constexpr std::string_view ui_category  = "GPU/Packing";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t ssbo_layout_buffer_id = INVALID_ID; // 4B: Packed std140/std430 SSBO buffer
    uint32_t column_0_buffer_id    = INVALID_ID; // 4B: Primary Vector3 or float column 0
    uint32_t column_1_buffer_id    = INVALID_ID; // 4B: Optional scalar W or float column 1
    uint32_t column_2_buffer_id    = INVALID_ID; // 4B: Optional float column 2
    uint32_t column_3_buffer_id    = INVALID_ID; // 4B: Optional float column 3
    float default_w_padding        = 0.0f;       // 4B: Fallback W value in std140 mode

    RDLayoutStandard layout_standard = RDLayoutStandard::STD140_VEC4_PADDED; // 1B
    RDLayoutDirection direction_mode = RDLayoutDirection::PACK_CPU_TO_SSBO;  // 1B
    uint8_t reserved_padding[38]     = {0};                                  // 38B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary ssbo_prop;
        ssbo_prop["name"] = "ssbo_layout_buffer_id";
        ssbo_prop["type"] = godot::Variant::INT;
        ssbo_prop["hint_string"] = "buffer_option";
        props.push_back(ssbo_prop);

        godot::Dictionary c0_prop;
        c0_prop["name"] = "column_0_buffer_id";
        c0_prop["type"] = godot::Variant::INT;
        c0_prop["hint_string"] = "buffer_option";
        props.push_back(c0_prop);

        godot::Dictionary c1_prop;
        c1_prop["name"] = "column_1_buffer_id";
        c1_prop["type"] = godot::Variant::INT;
        c1_prop["hint_string"] = "buffer_option";
        props.push_back(c1_prop);

        godot::Dictionary c2_prop;
        c2_prop["name"] = "column_2_buffer_id";
        c2_prop["type"] = godot::Variant::INT;
        c2_prop["hint_string"] = "buffer_option";
        props.push_back(c2_prop);

        godot::Dictionary c3_prop;
        c3_prop["name"] = "column_3_buffer_id";
        c3_prop["type"] = godot::Variant::INT;
        c3_prop["hint_string"] = "buffer_option";
        props.push_back(c3_prop);

        godot::Dictionary std_prop;
        std_prop["name"] = "layout_standard";
        std_prop["type"] = godot::Variant::INT;
        std_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        std_prop["hint_string"] = "std140 (16B Aligned vec3+w):0,std430 (Interleaved Columns):1";
        props.push_back(std_prop);

        godot::Dictionary dir_prop;
        dir_prop["name"] = "direction_mode";
        dir_prop["type"] = godot::Variant::INT;
        dir_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        dir_prop["hint_string"] = "Pack (CPU -> SSBO Layout):0,Unpack (SSBO Layout -> CPU):1";
        props.push_back(dir_prop);

        godot::Dictionary wpad_prop;
        wpad_prop["name"] = "default_w_padding";
        wpad_prop["type"] = godot::Variant::FLOAT;
        props.push_back(wpad_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (ssbo_layout_buffer_id != INVALID_ID) ? ssbo_layout_buffer_id : column_0_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("ssbo_layout_buffer_id")) {
            ssbo_layout_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["ssbo_layout_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && ssbo_layout_buffer_id == INVALID_ID) {
            ssbo_layout_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("column_0_buffer_id")) {
            column_0_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["column_0_buffer_id"]));
        }
        if (p_props.has("column_1_buffer_id")) {
            column_1_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["column_1_buffer_id"]));
        }
        if (p_props.has("column_2_buffer_id")) {
            column_2_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["column_2_buffer_id"]));
        }
        if (p_props.has("column_3_buffer_id")) {
            column_3_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["column_3_buffer_id"]));
        }
        if (p_props.has("layout_standard")) {
            layout_standard = static_cast<RDLayoutStandard>(std::clamp<int64_t>(static_cast<int64_t>(p_props["layout_standard"]), 0, 1));
        }
        if (p_props.has("direction_mode")) {
            direction_mode = static_cast<RDLayoutDirection>(std::clamp<int64_t>(static_cast<int64_t>(p_props["direction_mode"]), 0, 1));
        }
        if (p_props.has("default_w_padding")) {
            default_w_padding = static_cast<float>(p_props["default_w_padding"]);
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
        if constexpr (std::is_same_v<T, godot::Vector4> || std::is_same_v<T, godot::Vector3> || std::is_same_v<T, float>) {
            const GrantPartPOD* ssbo_part = p_context.get_grant_part(ssbo_layout_buffer_id);
            const GrantPartPOD* col0_part = p_context.get_grant_part(column_0_buffer_id);
            if (!ssbo_part || !col0_part || !ssbo_part->raw_base_ptr || !col0_part->raw_base_ptr) return;

            if (layout_standard == RDLayoutStandard::STD140_VEC4_PADDED && col0_part->element_stride >= sizeof(godot::Vector3)) {
                const GrantPartPOD* w_part = (column_1_buffer_id != INVALID_ID)
                    ? p_context.get_grant_part(column_1_buffer_id)
                    : nullptr;

                const uint32_t v3_stride = std::max<uint32_t>(sizeof(godot::Vector3), col0_part->element_stride);
                const int64_t n_elems = std::min<int64_t>(
                    static_cast<int64_t>(col0_part->capacity_bytes / v3_stride),
                    static_cast<int64_t>(ssbo_part->capacity_bytes / sizeof(godot::Vector4))
                );

                godot::Vector4* ssbo_v4 = reinterpret_cast<godot::Vector4*>(ssbo_part->raw_base_ptr);
                const uint32_t w_stride = (w_part && w_part->raw_base_ptr) ? std::max<uint32_t>(sizeof(float), w_part->element_stride) : sizeof(float);
                const int64_t max_w = (w_part && w_part->raw_base_ptr) ? static_cast<int64_t>(w_part->capacity_bytes / w_stride) : 0;

                if (direction_mode == RDLayoutDirection::PACK_CPU_TO_SSBO) {
                    for (int64_t i = 0; i < n_elems; ++i) {
                        const godot::Vector3& v = *reinterpret_cast<const godot::Vector3*>(col0_part->raw_base_ptr + static_cast<size_t>(i) * v3_stride);
                        const float w = (w_part && w_part->raw_base_ptr && i < max_w)
                            ? *reinterpret_cast<const float*>(w_part->raw_base_ptr + static_cast<size_t>(i) * w_stride)
                            : default_w_padding;
                        ssbo_v4[i] = godot::Vector4(v.x, v.y, v.z, w);
                    }
                } else {
                    for (int64_t i = 0; i < n_elems; ++i) {
                        const godot::Vector4& v4 = ssbo_v4[i];
                        *reinterpret_cast<godot::Vector3*>(col0_part->raw_base_ptr + static_cast<size_t>(i) * v3_stride) =
                            godot::Vector3(v4.x, v4.y, v4.z);
                        if (w_part && w_part->raw_base_ptr && i < max_w) {
                            *reinterpret_cast<float*>(w_part->raw_base_ptr + static_cast<size_t>(i) * w_stride) = v4.w;
                        }
                    }
                }
            } else {
                const uint32_t col_ids[4] = { column_0_buffer_id, column_1_buffer_id, column_2_buffer_id, column_3_buffer_id };
                const GrantPartPOD* col_parts[4] = { nullptr, nullptr, nullptr, nullptr };
                uint32_t num_cols = 0;
                for (int c = 0; c < 4; ++c) {
                    if (col_ids[c] != INVALID_ID) {
                        if (const GrantPartPOD* cp = p_context.get_grant_part(col_ids[c])) {
                            if (cp->raw_base_ptr) {
                                col_parts[num_cols++] = cp;
                            }
                        }
                    }
                }
                if (num_cols == 0) return;

                int64_t n_elems = static_cast<int64_t>(ssbo_part->capacity_bytes / (num_cols * sizeof(float)));
                for (uint32_t c = 0; c < num_cols; ++c) {
                    const uint32_t c_stride = std::max<uint32_t>(sizeof(float), col_parts[c]->element_stride);
                    n_elems = std::min<int64_t>(n_elems, static_cast<int64_t>(col_parts[c]->capacity_bytes / c_stride));
                }

                float* ssbo_f = reinterpret_cast<float*>(ssbo_part->raw_base_ptr);
                if (direction_mode == RDLayoutDirection::PACK_CPU_TO_SSBO) {
                    for (int64_t i = 0; i < n_elems; ++i) {
                        for (uint32_t c = 0; c < num_cols; ++c) {
                            const uint32_t c_stride = std::max<uint32_t>(sizeof(float), col_parts[c]->element_stride);
                            ssbo_f[static_cast<size_t>(i) * num_cols + c] =
                                *reinterpret_cast<const float*>(col_parts[c]->raw_base_ptr + static_cast<size_t>(i) * c_stride);
                        }
                    }
                } else {
                    for (int64_t i = 0; i < n_elems; ++i) {
                        for (uint32_t c = 0; c < num_cols; ++c) {
                            const uint32_t c_stride = std::max<uint32_t>(sizeof(float), col_parts[c]->element_stride);
                            *reinterpret_cast<float*>(col_parts[c]->raw_base_ptr + static_cast<size_t>(i) * c_stride) =
                                ssbo_f[static_cast<size_t>(i) * num_cols + c];
                        }
                    }
                }
            }
        }
    }
};

static_assert(sizeof(RDStd140Std430LayoutPackTransformLogic<>) == 64, "RDStd140Std430LayoutPackTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<RDStd140Std430LayoutPackTransformLogic<>>, "RDStd140Std430LayoutPackTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core