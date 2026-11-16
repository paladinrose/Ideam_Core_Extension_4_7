#include "core/tasks/domains/climatology/climatology_dod.h"
#include "core/tasks/domains/climatology/registration/climatology_task_domain_pack.h"

namespace ideam::core {

void ClimatologyDOD::build_climatology_pipeline(
    TaskGraphDOD& graph,
    uint32_t stage_mask) {
    (void)graph;
    (void)stage_mask;
    ideam::core::ClimatologyTaskDomainPack::register_domain();
}

void ClimatologyDOD::initialize_atmosphere_from_seed(
    const ClimatologySeedProfilePOD& seed,
    std::span<const float> optional_terrain_elevations_m,
    std::span<AtmosphericColumnCellPOD> out_atmo,
    std::span<MoistureAndCloudCellPOD> out_moisture,
    std::span<TornadoSupercellVortexPOD> out_vortices) noexcept {
    ideam::core::ClimatologyKernelMath::initialize_atmosphere_from_seed(
        seed,
        optional_terrain_elevations_m,
        out_atmo,
        out_moisture,
        out_vortices);
}

void ClimatologyDOD::execute_step(
    const ClimatologySeedProfilePOD& seed,
    std::span<AtmosphericColumnCellPOD> atmo_cells,
    std::span<MoistureAndCloudCellPOD> moisture_cells,
    std::span<TornadoSupercellVortexPOD> vortices,
    std::span<ClimatologyHazardAndBiomeSamplePOD> hazards,
    float dt_synoptic_sec,
    float dt_vortex_sec,
    uint32_t stage_mask) noexcept {
    if ((stage_mask & static_cast<uint32_t>(ClimatologyStageFlags::SOLAR_RADIATIVE_PRESSURE)) != 0u &&
        dt_synoptic_sec > 0.0f) {
        ideam::core::ClimatologyKernelMath::step_radiative_and_synoptic_pressure(
            seed, dt_synoptic_sec, atmo_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(ClimatologyStageFlags::GEOSTROPHIC_OROGRAPHIC_WIND)) != 0u &&
        dt_synoptic_sec > 0.0f) {
        ideam::core::ClimatologyKernelMath::step_geostrophic_and_orographic_wind(
            seed, dt_synoptic_sec, atmo_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(ClimatologyStageFlags::CLAUSIUS_FOEHN_MICROPHYSICS)) != 0u &&
        dt_synoptic_sec > 0.0f) {
        ideam::core::ClimatologyKernelMath::step_thermodynamics_and_microphysics(
            seed, dt_synoptic_sec, atmo_cells, moisture_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(ClimatologyStageFlags::SUPERCELL_TORNADO_VORTICES)) != 0u &&
        dt_vortex_sec > 0.0f) {
        ideam::core::ClimatologyKernelMath::step_convective_cape_and_tornado_vortices(
            seed, dt_vortex_sec, atmo_cells, moisture_cells, vortices);
    }

    if ((stage_mask & static_cast<uint32_t>(ClimatologyStageFlags::KOPPEN_FRONT_PARTITION)) != 0u) {
        ideam::core::ClimatologyKernelMath::compute_koppen_and_air_mass_partition(
            seed, atmo_cells, moisture_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(ClimatologyStageFlags::SEVERE_WEATHER_HAZARD_QUERY)) != 0u &&
        !hazards.empty()) {
        ideam::core::ClimatologyKernelMath::evaluate_severe_weather_hazards(
            seed, atmo_cells, moisture_cells, hazards);
    }
}

} // namespace ideam::core
