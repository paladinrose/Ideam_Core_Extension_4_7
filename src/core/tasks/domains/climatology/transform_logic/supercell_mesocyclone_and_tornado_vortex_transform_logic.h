#pragma once

/**
 * @file supercell_mesocyclone_and_tornado_vortex_transform_logic.h
 * @brief TransformLogic task (CLM_T4: SupercellMesocycloneAndTornadoVortex) for IdeamClimatology.
 *
 * Evaluates Convective Available Potential Energy (CAPE), 0-3 km Storm-Relative
 * Helicity (SRH), and Energy-Helicity Index (EHI) to spawn and advect Burgers-Rott /
 * Modified Rankine supercell tornado vortices with radial inflow, cyclostrophic
 * pressure deficit, axial suction updraft, and EF0-EF5 tangential wind fields.
 */

#include "../../../../climatology/climatology_core.h"
#include "../climatology_dod_common.h"
#include "../../../../memory/memory_common.h"
#include "../../../../memory/memory_grant_pod.h"
#include "../../../../memory/views/single_element_view.h"
#include "../../../../memory/views/strategies.h"
#include "../../../i_native_task.h"
#include "../../../transform_logic/transform_logic_traits.h"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <cstdint>
#include <span>
#include <string_view>

namespace ideam::core {

template <
    typename T = uint32_t,
    typename T_Strategy = FlatStrategy,
    template <typename, typename> class T_View = SingleElementView
>
struct alignas(64) SupercellMesocycloneAndTornadoVortexTransformLogic {
    using ValueType       = T;
    using DefaultStrategy = T_Strategy;
    using DefaultView     = T_View<T, T_Strategy>;

    static constexpr ViewCapability required_capabilities = ViewCapability::LINEAR_ACCESS;
    static constexpr BufferLayoutType required_layouts =
        BufferLayoutType::FLAT | BufferLayoutType::AOS | BufferLayoutType::SOA;
    static constexpr DataType required_types =
        DataType::BYTE | DataType::INT32 | DataType::FLOAT32 | DataType::VECTOR3;

    static constexpr size_t dimensions                = 0;
    static constexpr bool requires_static_kernel      = false;
    static constexpr size_t kernel_size               = 0;
    static constexpr size_t transient_workspace_bytes = 0;

    static constexpr std::string_view display_name = "Supercell Mesocyclone & Tornado Vortex";
    static constexpr std::string_view ui_category  = "Climatology";

    uint32_t target_buffer_id          = INVALID_ID;
    uint32_t seed_profile_buffer_id    = INVALID_ID;
    uint32_t atmo_column_buffer_id     = INVALID_ID;
    uint32_t moisture_cloud_buffer_id  = INVALID_ID;
    uint32_t tornado_vortex_buffer_id  = INVALID_ID;
    float    dt_seconds                = 15.0f;
    uint8_t  reserved_padding[40]      = {0};

    static godot::Array get_ui_properties() {
        godot::Array props;
        auto add_prop = [&](const char *p_name, godot::Variant::Type p_type, godot::Variant p_def) {
            godot::Dictionary d;
            d["name"]    = p_name;
            d["type"]    = static_cast<int>(p_type);
            d["default"] = p_def;
            props.push_back(d);
        };
        add_prop("target_buffer_id",         godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("seed_profile_buffer_id",   godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("atmo_column_buffer_id",    godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("moisture_cloud_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("tornado_vortex_buffer_id", godot::Variant::INT,   static_cast<int64_t>(INVALID_ID));
        add_prop("dt_seconds",               godot::Variant::FLOAT, 15.0);
        return props;
    }

    [[nodiscard]] constexpr uint32_t get_target_buffer_id() const noexcept {
        return (target_buffer_id != INVALID_ID) ? target_buffer_id : tornado_vortex_buffer_id;
    }

    void apply_properties(const godot::Dictionary &p_props) noexcept {
        if (p_props.has("target_buffer_id"))         target_buffer_id         = static_cast<uint32_t>(static_cast<int64_t>(p_props["target_buffer_id"]));
        if (p_props.has("seed_profile_buffer_id"))   seed_profile_buffer_id   = static_cast<uint32_t>(static_cast<int64_t>(p_props["seed_profile_buffer_id"]));
        if (p_props.has("atmo_column_buffer_id"))    atmo_column_buffer_id    = static_cast<uint32_t>(static_cast<int64_t>(p_props["atmo_column_buffer_id"]));
        if (p_props.has("moisture_cloud_buffer_id")) moisture_cloud_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["moisture_cloud_buffer_id"]));
        if (p_props.has("tornado_vortex_buffer_id")) tornado_vortex_buffer_id = static_cast<uint32_t>(static_cast<int64_t>(p_props["tornado_vortex_buffer_id"]));
        if (p_props.has("dt_seconds"))               dt_seconds               = static_cast<float>(static_cast<double>(p_props["dt_seconds"]));
    }

    template <typename T_ActiveView>
    inline void configure_view(T_ActiveView &p_view, const TaskContextPOD &p_ctx) const noexcept {
        (void)p_view;
        (void)p_ctx;
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    void execute(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        (void)p_view;
        const GrantPartPOD *seed_part  = p_ctx.get_grant_part(seed_profile_buffer_id);
        const GrantPartPOD *atmo_part  = p_ctx.get_grant_part(atmo_column_buffer_id);
        const GrantPartPOD *moist_part = p_ctx.get_grant_part(moisture_cloud_buffer_id);
        const GrantPartPOD *vort_part  = p_ctx.get_grant_part(tornado_vortex_buffer_id);
        if (!seed_part || !seed_part->raw_base_ptr || !atmo_part || !atmo_part->raw_base_ptr || !moist_part || !moist_part->raw_base_ptr) {
            return;
        }
        const auto *seed = reinterpret_cast<const ideam::core::ClimatologySeedProfilePOD *>(seed_part->raw_base_ptr);
        auto *atmo       = reinterpret_cast<ideam::core::AtmosphericColumnCellPOD *>(atmo_part->raw_base_ptr);
        auto *moist      = reinterpret_cast<ideam::core::MoistureAndCloudCellPOD *>(moist_part->raw_base_ptr);
        auto *vortices   = (vort_part && vort_part->raw_base_ptr)
            ? reinterpret_cast<ideam::core::TornadoSupercellVortexPOD *>(vort_part->raw_base_ptr)
            : nullptr;
        const size_t n = static_cast<size_t>(seed->grid_width) * static_cast<size_t>(seed->grid_height);
        const size_t v_cnt = vortices ? static_cast<size_t>(seed->max_vortices) : 0u;
        ideam::core::ClimatologyKernelMath::step_convective_cape_and_tornado_vortices(
            *seed,
            dt_seconds,
            std::span<ideam::core::AtmosphericColumnCellPOD>(atmo, n),
            std::span<ideam::core::MoistureAndCloudCellPOD>(moist, n),
            std::span<ideam::core::TornadoSupercellVortexPOD>(vortices, v_cnt));
    }

    template <typename T_ActiveView, typename T_ActiveStrategy>
    inline void execute_transform(const TaskContextPOD &p_ctx, T_ActiveView &p_view) const noexcept {
        execute<T_ActiveView, T_ActiveStrategy>(p_ctx, p_view);
    }
};

static_assert(sizeof(SupercellMesocycloneAndTornadoVortexTransformLogic<>) == 64, "SupercellMesocycloneAndTornadoVortexTransformLogic must be 64 bytes!");
static_assert(IsTransformLogic<SupercellMesocycloneAndTornadoVortexTransformLogic<>>, "SupercellMesocycloneAndTornadoVortexTransformLogic must satisfy IsTransformLogic!");

} // namespace ideam::core
