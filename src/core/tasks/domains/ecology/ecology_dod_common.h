#pragma once

/**
 * @file ecology_dod_common.h
 * @brief Shared buffer slot constants, stage masks, and manifest helpers for the
 *        IdeamEcology Tasks domain.
 */

#include "core/ecology/ecology_types.h"
#include <cstdint>

namespace ideam::core {

/**
 * @enum EcologyBufferSlot
 * @brief Canonical buffer slots used by Ecology Tasks (ECO_T1..T4, ECO_M1, ECO_Q1).
 */
enum class EcologyBufferSlot : uint16_t {
    SEED_PROFILE_BUFFER      = 0, ///< EcologySeedProfilePOD (1 element)
    GUILD_CATALOG_BUFFER     = 1, ///< FloraGuildArchetypePOD (active_guild_count)
    ECOLOGICAL_CELL_BUFFER   = 2, ///< EcologicalCellStatePOD (grid_width * grid_height)
    BIOGEO_TROPHIC_BUFFER    = 3, ///< BiogeochemicalAndTrophicCellPOD (grid_width * grid_height)
    HARVEST_HAZARD_OUT_BUFFER = 4, ///< EcologyHarvestAndDisturbanceSamplePOD (grid_width * grid_height)
    COUNT                    = 5
};

/**
 * @enum EcologyStageFlags
 * @brief Bitmask flags controlling which ecological stages execute in a step.
 */
enum class EcologyStageFlags : uint32_t {
    NONE                          = 0,
    WHITTAKER_CANOPY_PRODUCTIVITY = 1u << 0, ///< ECO_T1: Whittaker niche, Beer-Lambert light & NPP
    BIOGEOCHEMICAL_SUCCESSION     = 1u << 1, ///< ECO_T2: Soil C/N/P stoichiometry & seral succession
    ROTHERMEL_WILDFIRE_DISTURBANCE = 1u << 2, ///< ECO_T3: Rothermel wildfire contagion & windthrow
    LOTKA_VOLTERRA_TROPHIC_WEB    = 1u << 3, ///< ECO_T4: Multi-guild herbivore/predator/pollinator web
    HOLDRIDGE_SERAL_PARTITION     = 1u << 4, ///< ECO_M1: Holdridge life-zone & seral stage partitioning
    HARVEST_WILDFIRE_HAZARD_QUERY = 1u << 5, ///< ECO_Q1: Timber/forage/alchemical yield & wildfire risk
    ALL_STAGES                    = 0x3Fu
};

} // namespace ideam::core
