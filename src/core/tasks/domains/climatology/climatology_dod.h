#pragma once

/**
 * @file climatology_dod.h
 * @brief Master Data-Oriented Design (DOD) orchestrator for the IdeamClimatology domain.
 *
 * Wraps the 6 core climatology tasks (CLM_T1..T4, CLM_M1, CLM_Q1) and exposes
 * both TaskGraph-driven and standalone simulation stepping for solar radiative balance,
 * geostrophic/Coriolis/orographic wind advection, Clausius-Clapeyron moisture & Foehn
 * rain-shadow microphysics, CAPE/SRH supercell tornado vortices, Köppen-Geiger climate
 * classification, and severe weather hazard queries.
 */

#include "core/climatology/climatology_core.h"
#include "core/tasks/domains/climatology/climatology_dod_common.h"
#include "core/tasks/domains/climatology/metadata_logic/koppen_and_air_mass_front_partition_metadata_logic.h"
#include "core/tasks/domains/climatology/query_logic/severe_weather_and_vortex_hazard_query_logic.h"
#include "core/tasks/domains/climatology/transform_logic/clausius_clapeyron_and_foehn_microphysics_transform_logic.h"
#include "core/tasks/domains/climatology/transform_logic/geostrophic_and_orographic_wind_transform_logic.h"
#include "core/tasks/domains/climatology/transform_logic/solar_radiative_and_synoptic_pressure_transform_logic.h"
#include "core/tasks/domains/climatology/transform_logic/supercell_mesocyclone_and_tornado_vortex_transform_logic.h"
#include "core/tasks/task_graph.h"

#include <cstdint>
#include <span>

namespace ideam::tasks::climatology {

class ClimatologyDOD {
public:
    using ClimatologySeedProfilePOD          = ideam::climatology::ClimatologySeedProfilePOD;
    using AtmosphericColumnCellPOD           = ideam::climatology::AtmosphericColumnCellPOD;
    using MoistureAndCloudCellPOD            = ideam::climatology::MoistureAndCloudCellPOD;
    using TornadoSupercellVortexPOD          = ideam::climatology::TornadoSupercellVortexPOD;
    using ClimatologyHazardAndBiomeSamplePOD = ideam::climatology::ClimatologyHazardAndBiomeSamplePOD;

    /**
     * @brief Ensures the Climatology domain is registered with IdeamTaskRegistry.
     */
    static void build_climatology_pipeline(
        TaskGraph& graph,
        uint32_t stage_mask = static_cast<uint32_t>(ClimatologyStageFlags::ALL_STAGES));

    /**
     * @brief Initializes atmospheric columns, moisture/cloud states, and tornado vortices from seed.
     */
    static void initialize_atmosphere_from_seed(
        const ClimatologySeedProfilePOD& seed,
        std::span<const float> optional_terrain_elevations_m,
        std::span<AtmosphericColumnCellPOD> out_atmo,
        std::span<MoistureAndCloudCellPOD> out_moisture,
        std::span<TornadoSupercellVortexPOD> out_vortices) noexcept;

    /**
     * @brief Executes a synchronous step of the selected climatology stages over raw spans.
     */
    static void execute_step(
        const ClimatologySeedProfilePOD& seed,
        std::span<AtmosphericColumnCellPOD> atmo_cells,
        std::span<MoistureAndCloudCellPOD> moisture_cells,
        std::span<TornadoSupercellVortexPOD> vortices,
        std::span<ClimatologyHazardAndBiomeSamplePOD> hazards,
        float dt_synoptic_sec = 30.0f,
        float dt_vortex_sec = 10.0f,
        uint32_t stage_mask = static_cast<uint32_t>(ClimatologyStageFlags::ALL_STAGES)) noexcept;
};

} // namespace ideam::tasks::climatology
