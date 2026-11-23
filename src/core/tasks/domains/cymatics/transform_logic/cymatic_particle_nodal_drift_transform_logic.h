#pragma once

/**
 * cymatic_particle_nodal_drift_transform_logic.h
 * 64-byte cache-aligned TransformLogic task struct (`CYM3`) for `IdeamCymatics`.
 * Advects particles down the stochastic gradient of squared Chladni wave amplitude toward nodal lines.
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
struct alignas(64) CymaticParticleNodalDriftTransformLogic {
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

    static constexpr std::string_view display_name = "CYM3 Particle Nodal Drift";
    static constexpr std::string_view ui_category  = "Cymatics";

    uint32_t target_buffer_id    = INVALID_ID; // Primary particle X position buffer
    uint32_t pos_y_buffer_id     = INVALID_ID;
    uint32_t field_buffer_id     = INVALID_ID;
    uint32_t grid_width          = 32;
    uint32_t grid_height         = 32;
    uint32_t rng_seed            = 1337u;
    float drift_strength         = 0.35f;
    float jitter_amplitude       = 0.12f;
    uint8_t reserved_padding[32] = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_buf = [&](const char *n) {
            godot::Dictionary dict;
            dict["name"]        = n;
            dict["type"]        = static_cast<int>(godot::Variant::INT);
            dict["hint"]        = static_cast<int>(godot::PROPERTY_HINT_NONE);
            dict["hint_string"] = "buffer_option";
            dict["default"]     = static_cast<int64_t>(INVALID_ID);
            props.push_back(dict);
        };
        auto add = [&](const char *n, godot::Variant::Type t, godot::Variant d) {
            godot::Dictionary dict;
            dict["name"]    = n;
            dict["type"]    = static_cast<int>(t);
            dict["default"] = d;
            props.push_back(dict);
        };
        add_buf("target_buffer_id");
        add_buf("pos_y_buffer_id");
        add_buf("field_buffer_id");
        add("grid_width",       godot::Variant::INT,   static_cast<int64_t>(32));
        add("grid_height",      godot::Variant::INT,   static_cast<int64_t>(32));
        add("rng_seed",         godot::Variant::INT,   static_cast<int64_t>(1337));
        add("drift_strength",   godot::Variant::FLOAT, 0.35);
        add("jitter_amplitude", godot::Variant::FLOAT, 0.12);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : field_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id")) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        } else if (p_props.has("pos_x_buffer_id") && target_buffer_id == INVALID_ID) {
            target_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["pos_x_buffer_id"]));
        }
        if (p_props.has("pos_y_buffer_id"))  pos_y_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["pos_y_buffer_id"]));
        if (p_props.has("field_buffer_id"))  field_buffer_id  = static_cast<uint32_t>(static_cast<int64_t>(p_props["field_buffer_id"]));
        if (p_props.has("grid_width"))       grid_width       = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_width"]));
        if (p_props.has("grid_height"))      grid_height      = static_cast<uint32_t>(static_cast<int64_t>(p_props["grid_height"]));
        if (p_props.has("rng_seed"))         rng_seed         = static_cast<uint32_t>(static_cast<int64_t>(p_props["rng_seed"]));
        if (p_props.has("drift_strength"))   drift_strength   = static_cast<float>(static_cast<double>(p_props["drift_strength"]));
        if (p_props.has("jitter_amplitude")) jitter_amplitude = static_cast<float>(static_cast<double>(p_props["jitter_amplitude"]));
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *px = p_ctx.get_grant_part(target_buffer_id);
        const GrantPartPOD *py = p_ctx.get_grant_part(pos_y_buffer_id);
        const GrantPartPOD *fd = p_ctx.get_grant_part(field_buffer_id);
        if (!px || !py || !fd || !px->raw_base_ptr || !py->raw_base_ptr || !fd->raw_base_ptr) {
            return;
        }
        const size_t n_particles = std::min(px->capacity_bytes / sizeof(float), py->capacity_bytes / sizeof(float));
        if (fd->capacity_bytes / sizeof(float) < static_cast<size_t>(grid_width) * grid_height) {
            return;
        }

        CymaticGridConfigPOD cfg;
        cfg.width  = grid_width;
        cfg.height = grid_height;

        CymaticsSolverCore::advect_particles_nodal_drift(
            reinterpret_cast<float *>(px->raw_base_ptr),
            reinterpret_cast<float *>(py->raw_base_ptr),
            static_cast<uint32_t>(n_particles),
            reinterpret_cast<const float *>(fd->raw_base_ptr),
            cfg,
            drift_strength,
            jitter_amplitude,
            rng_seed
        );
    }
};

static_assert(sizeof(CymaticParticleNodalDriftTransformLogic<>) == 64, "CYM3 must be 64 bytes!");
static_assert(IsTransformLogic<CymaticParticleNodalDriftTransformLogic<>>, "CYM3 must satisfy IsTransformLogic!");

} // namespace ideam::core
