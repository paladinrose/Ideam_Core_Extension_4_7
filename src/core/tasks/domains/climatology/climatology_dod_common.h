#pragma once

/**
 * @file climatology_dod_common.h
 * @brief Shared buffer slot constants, stage masks, and manifest helpers for the
 *        IdeamClimatology Tasks domain.
 */

#include "core/climatology/climatology_types.h"
#include <cstdint>

namespace ideam::tasks::climatology {

/**
 * @enum ClimatologyBufferSlot
 * @brief Canonical buffer slots used by Climatology Tasks (CLM_T1..T4, CLM_M1, CLM_Q1).
 */
enum class ClimatologyBufferSlot : uint16_t {
    SEED_PROFILE_BUFFER       = 0, ///< ClimatologySeedProfilePOD (1 element)
    ATMOSPHERIC_CELL_BUFFER   = 1, ///< AtmosphericColumnCellPOD (grid_width * grid_height)
    MOISTURE_CLOUD_BUFFER     = 2, ///< MoistureAndCloudCellPOD (grid_width * grid_height)
    TORNADO_VORTEX_BUFFER     = 3, ///< TornadoSupercellVortexPOD (max_vortices)
    TERRAIN_ELEVATION_BUFFER  = 4, ///< Optional float elevation input array (grid_width * grid_height)
    HAZARD_BIOME_OUT_BUFFER   = 5, ///< ClimatologyHazardAndBiomeSamplePOD (grid_width * grid_height)
    COUNT                     = 6
};

/**
 * @enum ClimatologyStageFlags
 * @brief Bitmask flags controlling which climatology stages execute in a step.
 */
enum class ClimatologyStageFlags : uint32_t {
    NONE                        = 0,
    SOLAR_RADIATIVE_PRESSURE    = 1u << 0, ///< CLM_T1: Solar insolation, longwave & synoptic pressure
    GEOSTROPHIC_OROGRAPHIC_WIND = 1u << 1, ///< CLM_T2: Geostrophic, Coriolis & orographic lift winds
    CLAUSIUS_FOEHN_MICROPHYSICS = 1u << 2, ///< CLM_T3: Clausius-Clapeyron, Foehn rain-shadow & clouds
    SUPERCELL_TORNADO_VORTICES  = 1u << 3, ///< CLM_T4: CAPE/SRH instability & Rankine tornado vortices
    KOPPEN_FRONT_PARTITION      = 1u << 4, ///< CLM_M1: Köppen-Geiger & air-mass front partitioning
    SEVERE_WEATHER_HAZARD_QUERY = 1u << 5, ///< CLM_Q1: Tornado/blizzard/lightning hazard query
    ALL_STAGES                  = 0x3Fu
};

} // namespace ideam::tasks::climatology
