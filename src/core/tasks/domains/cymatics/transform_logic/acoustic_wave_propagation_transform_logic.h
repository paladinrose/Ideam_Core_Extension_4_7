#pragma once

/**
 * acoustic_wave_propagation_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`CYM1`) for `IdeamCymatics`.
 * Solves the 2D/3D FDTD acoustic wave equation with obstacle reflectivity masks.
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

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace ideam::core {

template <
    typename T = float,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) AcousticWavePropagationTransformLogic {
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

    static constexpr std::string_view display_name = "CYM1 Acoustic Wave Propagation (FDTD)";
    static constexpr std::string_view ui_category  = "Cymatics";

    uint32_t prev_buffer_id         = INVALID_ID;
    uint32_t curr_buffer_id         = INVALID_ID;
    uint32_t next_buffer_id         = INVALID_ID;
    uint32_t reflectivity_buffer_id = INVALID_ID;
    uint32_t grid_width             = 32;
    uint32_t grid_height            = 32;
    uint32_t grid_depth             = 1;
    float cell_size                 = 1.0f;
    float wave_speed                = 1.0f;
    float damping                   = 0.985f;
    float dt                        = 0.016f;
    uint8_t reserved_padding[20]    = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add = [&](const char *n, godot::Variant::Type t, godot::Variant d) {
            godot::Dictionary dict;
            dict["name"]    = n;
            dict["type"]    = static_cast<int>(t);
            dict["default"] = d;
            props.push_back(dict);
        };
        add("prev_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("curr_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("next_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("reflectivity_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add("grid_width",             godot::Variant::INT,   static_cast<int64_t>(32));
        add("grid_height",            godot::Variant::INT,   static_cast<int64_t>(32));
        add("grid_depth",             godot::Variant::INT,   static_cast<int64_t>(1));
        add("cell_size",              godot::Variant::FLOAT, 1.0);
        add("wave_speed",             godot::Variant::FLOAT, 1.0);
        add("damping",                godot::Variant::FLOAT, 0.985);
        add("dt",                     godot::Variant::FLOAT, 0.016);
        return props;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("prev_buffer_id"))         prev_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["prev_buffer_id"]));
        if (p_props.has("curr_buffer_id"))         curr_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["curr_buffer_id"]));
        if (p_props.has("next_buffer_id"))         next_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["next_buffer_id"]));
        if (p_props.has("reflectivity_buffer_id")) reflectivity_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["reflectivity_buffer_id"]));
        if (p_props.has("grid_width"))             grid_width             = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_width"]));
        if (p_props.has("grid_height"))            grid_height            = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_height"]));
        if (p_props.has("grid_depth"))             grid_depth             = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_depth"]));
        if (p_props.has("cell_size"))              cell_size              = static_cast<float>(static_cast<double>(p_props["cell_size"]));
        if (p_props.has("wave_speed"))             wave_speed             = static_cast<float>(static_cast<double>(p_props["wave_speed"]));
        if (p_props.has("damping"))                damping                = static_cast<float>(static_cast<double>(p_props["damping"]));
        if (p_props.has("dt"))                     dt                     = static_cast<float>(static_cast<double>(p_props["dt"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_Elem>
    [[nodiscard]] static T_Elem *_resolve_ptr(const TaskContextPOD &p_ctx, uint32_t p_buf_id, size_t &r_cap) noexcept {
        r_cap = 0u;
        if (p_buf_id == INVALID_ID) return nullptr;
        if (const GrantPartPOD *part = p_ctx.get_grant_part(p_buf_id)) {
            if (part->raw_base_ptr) {
                r_cap = part->capacity_bytes / sizeof(T_Elem);
                return reinterpret_cast<T_Elem *>(part->raw_base_ptr);
            }
        }
        return nullptr;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        size_t prev_cap = 0, curr_cap = 0, next_cap = 0, refl_cap = 0;
        const float *prev = _resolve_ptr<const float>(p_ctx, prev_buffer_id, prev_cap);
        const float *curr = _resolve_ptr<const float>(p_ctx, curr_buffer_id, curr_cap);
        float *next       = _resolve_ptr<float>(p_ctx, next_buffer_id, next_cap);
        const float *refl = _resolve_ptr<const float>(p_ctx, reflectivity_buffer_id, refl_cap);

        const size_t needed = static_cast<size_t>(grid_width) * grid_height * std::max<uint32_t>(1u, grid_depth);
        if (!prev || !curr || !next || prev_cap < needed || curr_cap < needed || next_cap < needed) {
            return;
        }

        CymaticGridConfigPOD cfg;
        cfg.width      = grid_width;
        cfg.height     = grid_height;
        cfg.depth      = std::max<uint32_t>(1u, grid_depth);
        cfg.cell_size  = cell_size;
        cfg.wave_speed = wave_speed;
        cfg.damping    = damping;
        cfg.dt         = dt;

        const float *safe_refl = (refl && refl_cap >= needed) ? refl : nullptr;
        if (cfg.depth > 1u) {
            CymaticsSolverCore::step_fdtd_3d(prev, curr, next, safe_refl, cfg);
        } else {
            CymaticsSolverCore::step_fdtd_2d(prev, curr, next, safe_refl, cfg);
        }
    }
};

static_assert(sizeof(AcousticWavePropagationTransformLogic<>) == 64, "CYM1 must be 64 bytes!");
static_assert(IsTransformLogic<AcousticWavePropagationTransformLogic<>>, "CYM1 must satisfy IsTransformLogic!");

} // namespace ideam::core
