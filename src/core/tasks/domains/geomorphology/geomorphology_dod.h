#pragma once

/**
 * @file geomorphology_dod.h
 * @brief Master Data-Oriented Design (DOD) orchestrator for the IdeamGeomorphology domain.
 *
 * Wraps the 6 core geomorphology tasks (GEO_T1..T4, GEO_M1, GEO_Q1) and exposes
 * both TaskGraph-driven and standalone simulation stepping for crust uplift,
 * 2D virtual pipe hydrology, stream-power/talus erosion, groundwater karst/pedogenesis,
 * D8 drainage basin / Strahler partitioning, and hazard/resource vein queries.
 */

#include "core/geomorphology/geomorphology_core.h"
#include "core/tasks/domains/geomorphology/geomorphology_dod_common.h"
#include "core/tasks/domains/geomorphology/metadata_logic/drainage_basin_and_strahler_partition_metadata_logic.h"
#include "core/tasks/domains/geomorphology/query_logic/geomorphic_hazard_and_resource_vein_query_logic.h"
#include "core/tasks/domains/geomorphology/transform_logic/groundwater_karst_and_pedogenesis_transform_logic.h"
#include "core/tasks/domains/geomorphology/transform_logic/shallow_water_pipe_hydrology_transform_logic.h"
#include "core/tasks/domains/geomorphology/transform_logic/stream_power_and_talus_erosion_transform_logic.h"
#include "core/tasks/domains/geomorphology/transform_logic/tectonic_uplift_and_orogeny_transform_logic.h"
#include "core/tasks/task_graph_dod.h"

#include <cstdint>
#include <span>

namespace ideam::core {

class GeomorphologyDOD {
public:
    using GeomorphologySeedProfilePOD = ideam::core::GeomorphologySeedProfilePOD;
    using LithologyArchetypePOD       = ideam::core::LithologyArchetypePOD;
    using StratigraphicColumnPOD      = ideam::core::StratigraphicColumnPOD;
    using TectonicPlatePOD            = ideam::core::TectonicPlatePOD;
    using CrustalTectonicCellPOD      = ideam::core::CrustalTectonicCellPOD;
    using HydrologyCellStatePOD       = ideam::core::HydrologyCellStatePOD;
    using GeomorphicHazardSamplePOD   = ideam::core::GeomorphicHazardSamplePOD;

    /**
     * @brief Populates a TaskGraph with the complete Geomorphology simulation pipeline.
     *
     * Stage Order:
     * 1. GEO_T1: TectonicUpliftAndOrogeny
     * 2. GEO_T2: ShallowWaterPipeHydrology
     * 3. GEO_T3: StreamPowerAndTalusErosion
     * 4. GEO_T4: GroundwaterKarstAndPedogenesis
     * 5. GEO_M1: DrainageBasinAndStrahlerPartition
     * 6. GEO_Q1: GeomorphicHazardAndResourceVein
     */
    static void build_geomorphology_pipeline(
        TaskGraphDOD& graph,
        uint32_t stage_mask = static_cast<uint32_t>(GeomorphologyStageFlags::ALL_STAGES));

    /**
     * @brief Generates initial world crust, tectonic plates, and multi-stratum columns from seed.
     */
    static void initialize_world_from_seed(
        const GeomorphologySeedProfilePOD& seed,
        std::span<const LithologyArchetypePOD> catalog,
        std::span<TectonicPlatePOD> out_plates,
        std::span<CrustalTectonicCellPOD> out_crust,
        std::span<StratigraphicColumnPOD> out_strata,
        std::span<HydrologyCellStatePOD> out_hydrology) noexcept;

    /**
     * @brief Executes a synchronous step of the selected geomorphology stages over raw spans.
     */
    static void execute_step(
        const GeomorphologySeedProfilePOD& seed,
        std::span<const LithologyArchetypePOD> catalog,
        std::span<const TectonicPlatePOD> plates,
        std::span<CrustalTectonicCellPOD> crust,
        std::span<StratigraphicColumnPOD> strata,
        std::span<HydrologyCellStatePOD> hydrology,
        std::span<GeomorphicHazardSamplePOD> hazards,
        float dt_geological_myr = 0.02f,
        float dt_hydrological_sec = 0.25f,
        float uniform_precipitation_rate = 0.015f,
        uint32_t stage_mask = static_cast<uint32_t>(GeomorphologyStageFlags::ALL_STAGES)) noexcept;
};

} // namespace ideam::core
