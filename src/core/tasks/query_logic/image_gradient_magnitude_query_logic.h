#pragma once

#include "../../memory/memory_common.h"
#include "../../memory/memory_grant_pod.h"
#include "../../memory/memory_manager_dod.h"
#include "../../memory/memory_buffer_selection_pod.h"
#include "../../memory/views/static_stencil_view.h"
#include "../../memory/views/single_element_view.h"
#include "../../memory/views/strategies.h"
#include "../i_native_task.h"
#include "query_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace ideam::core {

/**
 * ImageGradientMagnitudeQueryLogic<T, T_Strategy, T_View, KernelSize> (I9)
 * Evaluates the 3x3 Sobel gradient magnitude |grad I| = sqrt(Gx^2 + Gy^2) across a 2D image
 * buffer, culling or adding edge pixels whose gradient magnitude exceeds edge_threshold.
 */
template <
    typename T = godot::Color,
    typename T_Strategy = Spatial2DStrategy,
    template <typename, typename, size_t> class T_View = StaticStencilView,
    size_t KernelSize = 9
>
struct alignas(64) ImageGradientMagnitudeQueryLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy, KernelSize>;

    // --- DOD Contract Requirements ---
    static constexpr ViewCapability required_capabilities =
        ViewCapability::SPATIAL_ACCESS | ViewCapability::STENCIL_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA | BufferLayoutType::TILED_SOA;
    static constexpr DataType required_types =
        DataType::COLOR | DataType::VECTOR4 | DataType::FLOAT32;

    // --- Explicit Spatial & Kernel Contracts ---
    static constexpr size_t dimensions           = T_Strategy::dimensions;
    static constexpr bool requires_static_kernel = true;
    static constexpr size_t kernel_size          = KernelSize;

    static constexpr bool supports_cull               = true;
    static constexpr bool supports_addition           = true;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Image Sobel Gradient Magnitude Query";
    static constexpr std::string_view ui_category  = "Image";

    // --- Configuration State (Strictly 64-Byte Cache-Aligned) ---
    uint32_t target_buffer_id    = INVALID_ID; // 4B
    int32_t image_width          = 256;        // 4B
    int32_t image_height         = 256;        // 4B
    float edge_threshold         = 0.15f;      // 4B -> 16B
    uint8_t reserved_padding[48] = {0};        // 48B -> Total = 64B

    static godot::Array get_ui_properties() {
        godot::Array props;

        godot::Dictionary buf_prop;
        buf_prop["name"] = "target_buffer_id";
        buf_prop["type"] = godot::Variant::INT;
        buf_prop["hint_string"] = "buffer_option";
        props.push_back(buf_prop);

        auto add_prop = [&](const char* p_name, godot::Variant::Type p_type, const godot::Variant& p_def) {
            godot::Dictionary d;
            d["name"] = p_name;
            d["type"] = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("image_width", godot::Variant::INT, 256);
        add_prop("image_height", godot::Variant::INT, 256);
        add_prop("edge_threshold", godot::Variant::FLOAT, 0.15f);
        return props;
    }

    [[nodiscard]] inline uint32_t get_target_buffer_id() const noexcept {
        return target_buffer_id;
    }

    void apply_properties(const godot::Dictionary& p_props) noexcept {
        if (p_props.has("target_buffer_id")) target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("image_width")) image_width = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_width"]));
        if (p_props.has("image_height")) image_height = std::max<int32_t>(2, static_cast<int32_t>(p_props["image_height"]));
        if (p_props.has("edge_threshold")) edge_threshold = std::max(0.0f, static_cast<float>(p_props["edge_threshold"]));
    }

    template <QueryOp Op, typename T_ActiveView, typename T_ActiveStrategy>
    void execute(
        MemoryBufferSelectionPOD& r_selection,
        const TaskContextPOD& p_ctx,
        const T_ActiveView& p_view) const
    {
        (void)p_view;
        if constexpr (!std::is_pointer_v<T> && !std::is_same_v<T, bool>) {
            const GrantPartPOD* img_part = (target_buffer_id != INVALID_ID)
                ? p_ctx.get_grant_part(target_buffer_id)
                : ((p_ctx.grant && p_ctx.grant->part_count > 0) ? &p_ctx.grant->parts[0] : nullptr);
            if (!img_part || !img_part->raw_base_ptr) return;

            const int32_t w = std::max<int32_t>(2, image_width);
            const int32_t h = std::max<int32_t>(2, image_height);
            const size_t total = static_cast<size_t>(w) * h;

            const bool is_color = (img_part->element_stride >= sizeof(godot::Color));
            const uint32_t stride = std::max<uint32_t>(is_color ? sizeof(godot::Color) : sizeof(float), img_part->element_stride);
            if (img_part->capacity_bytes / stride < total) return;

            auto sample_lum = [&](int32_t x, int32_t y) noexcept -> float {
                const int32_t cx = std::clamp(x, 0, w - 1);
                const int32_t cy = std::clamp(y, 0, h - 1);
                const uint8_t* ptr = img_part->raw_base_ptr + (static_cast<size_t>(cy) * w + cx) * stride;
                if (is_color) {
                    const godot::Color& c = *reinterpret_cast<const godot::Color*>(ptr);
                    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
                }
                return *reinterpret_cast<const float*>(ptr);
            };

            const float thresh2 = edge_threshold * edge_threshold;

            for (int32_t y = 0; y < h; ++y) {
                for (int32_t x = 0; x < w; ++x) {
                    const float i00 = sample_lum(x - 1, y - 1);
                    const float i10 = sample_lum(x,     y - 1);
                    const float i20 = sample_lum(x + 1, y - 1);
                    const float i01 = sample_lum(x - 1, y);
                    const float i21 = sample_lum(x + 1, y);
                    const float i02 = sample_lum(x - 1, y + 1);
                    const float i12 = sample_lum(x,     y + 1);
                    const float i22 = sample_lum(x + 1, y + 1);

                    const float gx = ((i20 + 2.0f * i21 + i22) - (i00 + 2.0f * i01 + i02)) * 0.125f;
                    const float gy = ((i02 + 2.0f * i12 + i22) - (i00 + 2.0f * i10 + i20)) * 0.125f;

                    const bool is_edge = (gx * gx + gy * gy >= thresh2);
                    const int64_t idx = static_cast<int64_t>(y) * w + x;
                    if (idx < 0 || idx >= r_selection.capacity) continue;

                    if constexpr (Op == QueryOp::CULL) {
                        if (!is_edge && r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                            const uint64_t word = static_cast<uint64_t>(idx) >> 6;
                            const uint64_t bit = 1ULL << (idx & 63);
                            if (r_selection.data.bitset[word] & bit) {
                                r_selection.data.bitset[word] &= ~bit;
                                r_selection.element_count = std::max<int64_t>(0, r_selection.element_count - 1);
                            }
                        }
                    } else if constexpr (Op == QueryOp::ADD) {
                        if (is_edge) {
                            p_ctx.queue_selection_command(target_buffer_id, idx);
                            if (r_selection.mode == SelectionMode::DENSE && r_selection.data.bitset) {
                                const uint64_t word = static_cast<uint64_t>(idx) >> 6;
                                const uint64_t bit = 1ULL << (idx & 63);
                                if (!(r_selection.data.bitset[word] & bit)) {
                                    r_selection.data.bitset[word] |= bit;
                                    r_selection.element_count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
};

static_assert(sizeof(ImageGradientMagnitudeQueryLogic<>) == 64, "ImageGradientMagnitudeQueryLogic must be exactly 64 bytes!");
static_assert(IsQueryLogic<ImageGradientMagnitudeQueryLogic<>>, "ImageGradientMagnitudeQueryLogic must satisfy IsQueryLogic!");

} // namespace ideam::core