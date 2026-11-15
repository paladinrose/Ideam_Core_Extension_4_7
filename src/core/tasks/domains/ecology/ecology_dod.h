#pragma once

/**
 * @file ecology_dod.h
 * @brief Master Data-Oriented Design (DOD) orchestrator for the IdeamEcology domain.
 *
 * Wraps the 6 core ecology tasks (ECO_T1..T4, ECO_M1, ECO_Q1) and exposes
 * both TaskGraph-driven and standalone simulation stepping for Whittaker-Holdridge
 * niche fitness & canopy NPP, Century-lite soil C/N/P cycling & seral succession,
 * Rothermel cellular wildfire contagion & windthrow disturbance, spatial Lotka-Volterra
 * trophic webs & alchemical phytochemistry, Holdridge life-zone partitioning,
 * and harvest/wildfire hazard queries.
 */

#include "core/ecology/ecology_core.h"
#include "core/tasks/domains/ecology/ecology_dod_common.h"
#include "core/tasks/domains/ecology/metadata_logic/holdridge_life_zone_and_seral_partition_metadata_logic.h"
#include "core/tasks/domains/ecology/query_logic/ecological_harvest_and_wildfire_hazard_query_logic.h"
#include "core/tasks/domains/ecology/transform_logic/biogeochemical_nutrient_and_succession_transform_logic.h"
#include "core/tasks/domains/ecology/transform_logic/lotka_volterra_trophic_web_transform_logic.h"
#include "core/tasks/domains/ecology/transform_logic/rothermel_wildfire_and_disturbance_transform_logic.h"
#include "core/tasks/domains/ecology/transform_logic/whittaker_niche_and_canopy_productivity_transform_logic.h"
#include "core/tasks/task_graph.h"

#include <cstdint>
#include <span>

namespace ideam::tasks::ecology {

class EcologyDOD {
public:
    using EcologySeedProfilePOD               = ideam::ecology::EcologySeedProfilePOD;
    using FloraGuildArchetypePOD              = ideam::ecology::FloraGuildArchetypePOD;
    using EcologicalCellStatePOD              = ideam::ecology::EcologicalCellStatePOD;
    using BiogeochemicalAndTrophicCellPOD     = ideam::ecology::BiogeochemicalAndTrophicCellPOD;
    using EcologyHarvestAndDisturbanceSamplePOD = ideam::ecology::EcologyHarvestAndDisturbanceSamplePOD;

    /**
     * @brief Ensures the Ecology domain is registered with IdeamTaskRegistry.
     */
    static void build_ecology_pipeline(
        TaskGraph& graph,
        uint32_t stage_mask = static_cast<uint32_t>(EcologyStageFlags::ALL_STAGES));

    /**
     * @brief Initializes biosphere cells, soil NPK pools, and multi-guild trophic densities from seed.
     */
    static void initialize_biosphere_from_seed(
        const EcologySeedProfilePOD& seed,
        std::span<const FloraGuildArchetypePOD> guild_catalog,
        std::span<EcologicalCellStatePOD> out_eco,
        std::span<BiogeochemicalAndTrophicCellPOD> out_biogeo) noexcept;

    /**
     * @brief Executes a synchronous step of the selected ecology stages over raw spans.
     */
    static void execute_step(
        const EcologySeedProfilePOD& seed,
        std::span<const FloraGuildArchetypePOD> guild_catalog,
        std::span<EcologicalCellStatePOD> eco_cells,
        std::span<BiogeochemicalAndTrophicCellPOD> biogeo_cells,
        std::span<EcologyHarvestAndDisturbanceSamplePOD> harvest_samples,
        float dt_years = 0.25f,
        uint32_t stage_mask = static_cast<uint32_t>(EcologyStageFlags::ALL_STAGES)) noexcept;
};

} // namespace ideam::tasks::ecology
