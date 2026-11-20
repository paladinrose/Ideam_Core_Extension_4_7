#pragma once

/**
 * cymatic_chladni_modal_field_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`CYM2`) for `IdeamCymatics`.
 * Evaluates superposed Chladni plate & circular Bessel eigenmodes driven by FFT peaks.
 */

#include "../../../../cymatics/cymatics_solver_core.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) CymaticChladniModalFieldTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types = DataType::FLOAT32;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "CYM2 Chladni & Bessel Modal Field";
    static constexpr std::string_view ui_category  = "Cymatics";

    uint32_t out_field_buffer_id = INVALID_ID;
    uint32_t fft_peaks_buffer_id = INVALID_ID;
    uint32_t grid_width          = 32;
    uint32_t grid_height         = 32;
    float chladni_m              = 3.0f;
    float chladni_n              = 5.0f;
    float chladni_a              = 1.0f;
    float chladni_b              = -1.0f;
    float bessel_order           = 2.0f;
    float bessel_root            = 5.1356f;
    float time_sec               = 0.0f;
    uint8_t wave_mode            = static_cast<uint8_t>(CymaticWaveMode::MODE_HYBRID);
    uint8_t reserved_padding[19] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add = [&](const char *n, godot::Variant::Type t, godot::Variant d) {
            godot::Dictionary dict;
            dict["name"]    = n;
            dict["type"]    = static_cast<int>(t);
            dict["default"] = d;
            props.push_back(dict);
        };
        add("out_field_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("fft_peaks_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("grid_width",          godot::Variant::INT,   static_cast<int64_t>(32));
        add("grid_height",         godot::Variant::INT,   static_cast<int64_t>(32));
        add("chladni_m",           godot::Variant::FLOAT, 3.0);
        add("chladni_n",           godot::Variant::FLOAT, 5.0);
        add("chladni_a",           godot::Variant::FLOAT, 1.0);
        add("chladni_b",           godot::Variant::FLOAT, -1.0);
        add("bessel_order",        godot::Variant::FLOAT, 2.0);
        add("bessel_root",         godot::Variant::FLOAT, 5.1356);
        add("time_sec",            godot::Variant::FLOAT, 0.0);
        add("wave_mode",           godot::Variant::INT,   static_cast<int64_t>(CymaticWaveMode::MODE_HYBRID));
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("out_field_buffer_id")) out_field_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["out_field_buffer_id"]));
        if (p_props.has("fft_peaks_buffer_id")) fft_peaks_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["fft_peaks_buffer_id"]));
        if (p_props.has("grid_width"))          grid_width          = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_width"]));
        if (p_props.has("grid_height"))         grid_height         = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_height"]));
        if (p_props.has("chladni_m"))           chladni_m           = static_cast<float>(static_cast<double>(p_props["chladni_m"]));
        if (p_props.has("chladni_n"))           chladni_n           = static_cast<float>(static_cast<double>(p_props["chladni_n"]));
        if (p_props.has("chladni_a"))           chladni_a           = static_cast<float>(static_cast<double>(p_props["chladni_a"]));
        if (p_props.has("chladni_b"))           chladni_b           = static_cast<float>(static_cast<double>(p_props["chladni_b"]));
        if (p_props.has("bessel_order"))        bessel_order        = static_cast<float>(static_cast<double>(p_props["bessel_order"]));
        if (p_props.has("bessel_root"))         bessel_root         = static_cast<float>(static_cast<double>(p_props["bessel_root"]));
        if (p_props.has("time_sec"))            time_sec            = static_cast<float>(static_cast<double>(p_props["time_sec"]));
        if (p_props.has("wave_mode"))           wave_mode           = static_cast<uint8_t>(static_cast<int64_t>(p_props["wave_mode"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *out_part = p_ctx.get_grant_part(out_field_buffer_id);
        if (!out_part || !out_part->raw_base_ptr) {
            return;
        }
        const size_t needed = static_cast<size_t>(grid_width) * grid_height;
        if (out_part->capacity_bytes / sizeof(float) < needed) {
            return;
        }

        const float *fft_ptr = nullptr;
        uint32_t fft_count   = 0u;
        if (const GrantPartPOD *fft_part = p_ctx.get_grant_part(fft_peaks_buffer_id); fft_part && fft_part->raw_base_ptr) {
            fft_ptr   = reinterpret_cast<const float *>(fft_part->raw_base_ptr);
            fft_count = static_cast<uint32_t>(fft_part->capacity_bytes / sizeof(float));
        }

        CymaticGridConfigPOD cfg;
        cfg.width        = grid_width;
        cfg.height       = grid_height;
        cfg.chladni_m    = chladni_m;
        cfg.chladni_n    = chladni_n;
        cfg.chladni_a    = chladni_a;
        cfg.chladni_b    = chladni_b;
        cfg.bessel_order = bessel_order;
        cfg.bessel_root  = bessel_root;
        cfg.wave_mode    = wave_mode;

        CymaticsSolverCore::evaluate_chladni_modal_field(
            reinterpret_cast<float *>(out_part->raw_base_ptr), cfg, fft_ptr, fft_count, time_sec
        );
    }
};

static_assert(sizeof(CymaticChladniModalFieldTransformLogic<>) == 64, "CYM2 must be 64 bytes!");
static_assert(IsTransformLogic<CymaticChladniModalFieldTransformLogic<>>, "CYM2 must satisfy IsTransformLogic!");

} // namespace ideam::core
