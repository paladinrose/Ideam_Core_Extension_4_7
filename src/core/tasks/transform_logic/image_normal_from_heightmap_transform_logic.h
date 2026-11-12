#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/stencil_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

enum class HeightmapGradientKernel : uint8_t {
    SOBEL_3X3  = 0,
    SCHARR_3X3 = 1
};

/**
 * ImageNormalFromHeightmapTransformLogic<T, T_Strategy, KernelSize> (I1)
 * Computes 3x3 Sobel or Scharr height gradients across a 2D heightmap buffer and packs
 * tangent-space RGBA normal vectors ((nx*0.5+0.5, ny*0.5+0.5, nz*0.5+0.5, height)) into
 * a Color / Vector4 or float output buffer.
 */
template <
    typename T = godot::Color,
    typename T_Strategy = Spatial2DStrategy,
    size_t KernelSize = 8
>
struct alignas(64) ImageNormalFromHeightmapTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = StaticStencilView<T, DefaultStrategy, KernelSize>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts    = BufferLayoutType::ANY_SPATIAL;
    static constexpr DataType required_types              = DataType::COLOR | DataType::VECTOR4 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size          = KernelSize;

    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Image Normal Map from Heightmap";
    static constexpr std::string_view ui_category  = "Image";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t heightmap_buffer_id = INVALID_ID; // 4B: Source scalar/color heightmap buffer
    uint32_t normal_buffer_id    = INVALID_ID; // 4B: Destination tangent-space RGBA normal map buffer
    int32_t image_width          = 256;        // 4B: 2D grid width in pixels
    int32_t image_height         = 256;        // 4B: 2D grid height in pixels
    float bump_strength          = 4.0f;       // 4B: Gradient elevation multiplier

    HeightmapGradientKernel kernel_mode = HeightmapGradientKernel::SOBEL_3X3; // 1B
    bool wrap_seamless_borders          = true;  // 1B: Toroidal wrap at image borders
    bool invert_green_y                 = false; // 1B: Flip normal Y channel (DirectX vs OpenGL convention)
    uint8_t reserved_padding[41]        = {0};   // 41B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary hbuf_prop;
        hbuf_prop["name"] = "heightmap_buffer_id";
        hbuf_prop["type"] = godot::Variant::INT;
        hbuf_prop["hint_string"] = "buffer_option";
        props.push_back(hbuf_prop);

        godot::Dictionary nbuf_prop;
        nbuf_prop["name"] = "normal_buffer_id";
        nbuf_prop["type"] = godot::Variant::INT;
        nbuf_prop["hint_string"] = "buffer_option";
        props.push_back(nbuf_prop);

        godot::Dictionary w_prop;
        w_prop["name"] = "image_width";
        w_prop["type"] = godot::Variant::INT;
        w_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        w_prop["hint_string"] = "2,8192,1,suffix:px";
        props.push_back(w_prop);

        godot::Dictionary h_prop;
        h_prop["name"] = "image_height";
        h_prop["type"] = godot::Variant::INT;
        h_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        h_prop["hint_string"] = "2,8192,1,suffix:px";
        props.push_back(h_prop);

        godot::Dictionary bmp_prop;
        bmp_prop["name"] = "bump_strength";
        bmp_prop["type"] = godot::Variant::FLOAT;
        bmp_prop["hint"] = godot::PROPERTY_HINT_RANGE;
        bmp_prop["hint_string"] = "0.0,64.0,0.05";
        props.push_back(bmp_prop);

        godot::Dictionary kmode_prop;
        kmode_prop["name"] = "kernel_mode";
        kmode_prop["type"] = godot::Variant::INT;
        kmode_prop["hint"] = godot::PROPERTY_HINT_ENUM;
        kmode_prop["hint_string"] = "Sobel 3x3:0,Scharr 3x3:1";
        props.push_back(kmode_prop);

        godot::Dictionary wrp_prop;
        wrp_prop["name"] = "wrap_seamless_borders";
        wrp_prop["type"] = godot::Variant::BOOL;
        props.push_back(wrp_prop);

        godot::Dictionary inv_prop;
        inv_prop["name"] = "invert_green_y";
        inv_prop["type"] = godot::Variant::BOOL;
        props.push_back(inv_prop);

        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return (normal_buffer_id != INVALID_ID) ? normal_buffer_id : heightmap_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("heightmap_buffer_id")) {
            heightmap_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["heightmap_buffer_id"]));
        } else if (p_props.has("grid_buffer_id") && heightmap_buffer_id == INVALID_ID) {
            heightmap_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_buffer_id"]));
        }
        if (p_props.has("normal_buffer_id")) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["normal_buffer_id"]));
        } else if (p_props.has("target_buffer_id") && normal_buffer_id == INVALID_ID) {
            normal_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        }
        if (p_props.has("image_width")) {
            image_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_width"]));
        }
        if (p_props.has("image_height")) {
            image_height = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_height"]));
        }
        if (p_props.has("bump_strength")) {
            bump_strength = std::max(0.0f, static_cast<float>(p_props["bump_strength"]));
        }
        if (p_props.has("kernel_mode")) {
            kernel_mode = static_cast<HeightmapGradientKernel>(std::clamp<int64_t>(static_cast<int64_t>(p_props["kernel_mode"]), 0, 1));
        }
        if (p_props.has("wrap_seamless_borders")) {
            wrap_seamless_borders = static_cast<bool>(p_props["wrap_seamless_borders"]);
        }
        if (p_props.has("invert_green_y")) {
            invert_green_y = static_cast<bool>(p_props["invert_green_y"]);
        }
    }

    template <typename T_View>
    void configure_view(T_View& r_view, const TaskContextPOD& /*p_context*/, const GrantPartPOD* p_part) const noexcept {
        const size_t stride = (p_part && p_part->element_stride > 0) ? p_part->element_stride : sizeof(T);
        if constexpr (requires { r_view.strategy.stride_y; }) {
            r_view.strategy.stride_y = static_cast<int64_t>(std::max<int32_t>(2, image_width)) * static_cast<int64_t>(stride);
        }
        if constexpr (requires { r_view.baked_offsets; } && KernelSize >= 8) {
            const intptr_t sx = static_cast<intptr_t>(stride);
            const intptr_t sy = static_cast<intptr_t>(std::max<int32_t>(2, image_width)) * sx;
            r_view.baked_offsets[0] = -sx - sy;
            r_view.baked_offsets[1] = -sy;
            r_view.baked_offsets[2] =  sx - sy;
            r_view.baked_offsets[3] = -sx;
            r_view.baked_offsets[4] =  sx;
            r_view.baked_offsets[5] = -sx + sy;
            r_view.baked_offsets[6] =  sy;
            r_view.baked_offsets[7] =  sx + sy;
        }
    }

    template <typename T_View, typename T_Strat>
    #if defined(_MSC_VER)
        [[msvc::forceinline]]
    #else
        [[gnu::always_inline]]
    #endif
    void execute(const TaskContextPOD& p_context, T_View& p_view) const {
        (void)p_view;
        if constexpr (std::is_same_v<T, godot::Color> || std::is_same_v<T, godot::Vector4> || std::is_same_v<T, float>) {
            const GrantPartPOD* h_part   = p_context.get_grant_part(heightmap_buffer_id != INVALID_ID ? heightmap_buffer_id : normal_buffer_id);
            const GrantPartPOD* nrm_part = p_context.get_grant_part(normal_buffer_id != INVALID_ID ? normal_buffer_id : heightmap_buffer_id);
            if (!h_part || !nrm_part || !h_part->raw_base_ptr || !nrm_part->raw_base_ptr) return;

            const int32_t w = std::max<int32_t>(2, image_width);
            const int32_t h = std::max<int32_t>(2, image_height);
            const size_t total_pixels = static_cast<size_t>(w) * static_cast<size_t>(h);

            const uint32_t h_stride = std::max<uint32_t>(sizeof(float), h_part->element_stride);
            const uint32_t n_stride = std::max<uint32_t>(sizeof(T), nrm_part->element_stride);
            if (h_part->capacity_bytes / h_stride < total_pixels || nrm_part->capacity_bytes / n_stride < total_pixels) return;

            auto sample_h = [&](int32_t x, int32_t y) noexcept -> float {
                const int32_t cx = wrap_seamless_borders ? (((x % w) + w) % w) : std::clamp(x, 0, w - 1);
                const int32_t cy = wrap_seamless_borders ? (((y % h) + h) % h) : std::clamp(y, 0, h - 1);
                const size_t idx = static_cast<size_t>(cy) * static_cast<size_t>(w) + static_cast<size_t>(cx);
                return *reinterpret_cast<const float*>(h_part->raw_base_ptr + idx * h_stride);
            };

            const float w_corner = (kernel_mode == HeightmapGradientKernel::SCHARR_3X3) ? 3.0f : 1.0f;
            const float w_center = (kernel_mode == HeightmapGradientKernel::SCHARR_3X3) ? 10.0f : 2.0f;
            const float norm_div = 2.0f * (2.0f * w_corner + w_center);
            const float y_sign   = invert_green_y ? -1.0f : 1.0f;

            for (int32_t y = 0; y < h; ++y) {
                for (int32_t x = 0; x < w; ++x) {
                    const float h00 = sample_h(x - 1, y - 1);
                    const float h10 = sample_h(x,     y - 1);
                    const float h20 = sample_h(x + 1, y - 1);
                    const float h01 = sample_h(x - 1, y);
                    const float h11 = sample_h(x,     y);
                    const float h21 = sample_h(x + 1, y);
                    const float h02 = sample_h(x - 1, y + 1);
                    const float h12 = sample_h(x,     y + 1);
                    const float h22 = sample_h(x + 1, y + 1);

                    const float gx = ((w_corner * h20 + w_center * h21 + w_corner * h22) -
                                      (w_corner * h00 + w_center * h01 + w_corner * h02)) / norm_div;
                    const float gy = ((w_corner * h02 + w_center * h12 + w_corner * h22) -
                                      (w_corner * h00 + w_center * h10 + w_corner * h20)) / norm_div;

                    const godot::Vector3 n = godot::Vector3(-gx * bump_strength, -gy * bump_strength * y_sign, 1.0f).normalized();
                    const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);
                    const float nx = n.x * 0.5f + 0.5f;
                    const float ny = n.y * 0.5f + 0.5f;
                    const float nz = n.z * 0.5f + 0.5f;
                    const float a  = std::clamp(h11, 0.0f, 1.0f);

                    if constexpr (std::is_same_v<T, godot::Color>) {
                        *reinterpret_cast<godot::Color*>(nrm_part->raw_base_ptr + idx * n_stride) = godot::Color(nx, ny, nz, a);
                    } else if constexpr (std::is_same_v<T, godot::Vector4>) {
                        *reinterpret_cast<godot::Vector4*>(nrm_part->raw_base_ptr + idx * n_stride) = godot::Vector4(nx, ny, nz, a);
                    } else {
                        *reinterpret_cast<float*>(nrm_part->raw_base_ptr + idx * n_stride) = nz;
                    }
                }
            }
        }
    }
};

static_assert(sizeof(ImageNormalFromHeightmapTransformLogic<>) == 64, "ImageNormalFromHeightmapTransformLogic must be exactly 64 bytes!");
static_assert(IsTransformLogic<ImageNormalFromHeightmapTransformLogic<>>, "ImageNormalFromHeightmapTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core