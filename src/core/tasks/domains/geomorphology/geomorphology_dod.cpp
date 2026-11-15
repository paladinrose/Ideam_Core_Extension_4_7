#include "core/tasks/domains/geomorphology/geomorphology_dod.h"
#include "core/tasks/domains/geomorphology/registration/geomorphology_task_domain_pack.h"

namespace ideam::tasks::geomorphology {

void GeomorphologyDOD::build_geomorphology_pipeline(
    TaskGraph& graph,
    uint32_t stage_mask) {
    (void)graph;
    (void)stage_mask;
    ideam::core::GeomorphologyTaskDomainPack::register_domain();
}

void GeomorphologyDOD::initialize_world_from_seed(
    const GeomorphologySeedProfilePOD& seed,
    std::span<const LithologyArchetypePOD> catalog,
    std::span<TectonicPlatePOD> out_plates,
    std::span<CrustalTectonicCellPOD> out_crust,
    std::span<StratigraphicColumnPOD> out_strata,
    std::span<HydrologyCellStatePOD> out_hydrology) noexcept {
    ideam::geomorphology::GeomorphologyKernelMath::generate_tectonic_plates(seed, out_plates);
    ideam::geomorphology::GeomorphologyKernelMath::initialize_crust_and_stratigraphy(
        seed,
        out_plates,
        catalog,
        out_crust,
        out_strata,
        out_hydrology);
    ideam::geomorphology::GeomorphologyKernelMath::compute_d8_drainage_and_strahler(
        seed,
        out_strata,
        out_hydrology);
}

void GeomorphologyDOD::execute_step(
    const GeomorphologySeedProfilePOD& seed,
    std::span<const LithologyArchetypePOD> catalog,
    std::span<const TectonicPlatePOD> plates,
    std::span<CrustalTectonicCellPOD> crust,
    std::span<StratigraphicColumnPOD> strata,
    std::span<HydrologyCellStatePOD> hydrology,
    std::span<GeomorphicHazardSamplePOD> hazards,
    float dt_geological_myr,
    float dt_hydrological_sec,
    float uniform_precipitation_rate,
    uint32_t stage_mask) noexcept {
    if ((stage_mask & static_cast<uint32_t>(GeomorphologyStageFlags::TECTONIC_UPLIFT)) != 0u &&
        dt_geological_myr > 0.0f) {
        ideam::geomorphology::GeomorphologyKernelMath::step_tectonic_uplift_and_isostasy(
            seed,
            plates,
            catalog,
            dt_geological_myr,
            crust,
            strata);
    }

    if ((stage_mask & static_cast<uint32_t>(GeomorphologyStageFlags::SHALLOW_WATER_HYDRO)) != 0u &&
        dt_hydrological_sec > 0.0f) {
        ideam::geomorphology::GeomorphologyKernelMath::step_shallow_water_pipe_hydrology(
            seed,
            strata,
            {},
            uniform_precipitation_rate,
            dt_hydrological_sec,
            hydrology);
    }

    if ((stage_mask & static_cast<uint32_t>(GeomorphologyStageFlags::FLUVIAL_TALUS_EROSION)) != 0u &&
        dt_hydrological_sec > 0.0f) {
        ideam::geomorphology::GeomorphologyKernelMath::step_stream_power_and_talus_erosion(
            seed,
            catalog,
            dt_hydrological_sec,
            crust,
            strata,
            hydrology);
    }

    if ((stage_mask & static_cast<uint32_t>(GeomorphologyStageFlags::KARST_PEDOGENESIS)) != 0u &&
        dt_hydrological_sec > 0.0f) {
        ideam::geomorphology::GeomorphologyKernelMath::step_groundwater_karst_and_pedogenesis(
            seed,
            catalog,
            dt_hydrological_sec,
            strata,
            hydrology);
    }

    if ((stage_mask & static_cast<uint32_t>(GeomorphologyStageFlags::DRAINAGE_STRAHLER)) != 0u) {
        ideam::geomorphology::GeomorphologyKernelMath::compute_d8_drainage_and_strahler(
            seed,
            strata,
            hydrology);
    }

    if ((stage_mask & static_cast<uint32_t>(GeomorphologyStageFlags::HAZARD_RESOURCE_QUERY)) != 0u &&
        !hazards.empty()) {
        ideam::geomorphology::GeomorphologyKernelMath::evaluate_hazards_and_resource_veins(
            seed,
            crust,
            strata,
            hydrology,
            hazards);
    }
}

} // namespace ideam::tasks::geomorphology
