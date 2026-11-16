#include "core/tasks/domains/ecology/ecology_dod.h"
#include "core/tasks/domains/ecology/registration/ecology_task_domain_pack.h"

namespace ideam::core {

void EcologyDOD::build_ecology_pipeline(
    TaskGraphDOD& graph,
    uint32_t stage_mask) {
    (void)graph;
    (void)stage_mask;
    ideam::core::EcologyTaskDomainPack::register_domain();
}

void EcologyDOD::initialize_biosphere_from_seed(
    const EcologySeedProfilePOD& seed,
    std::span<const FloraGuildArchetypePOD> guild_catalog,
    std::span<EcologicalCellStatePOD> out_eco,
    std::span<BiogeochemicalAndTrophicCellPOD> out_biogeo) noexcept {
    ideam::core::EcologyKernelMath::initialize_biosphere_from_seed(
        seed,
        guild_catalog,
        out_eco,
        out_biogeo);
}

void EcologyDOD::execute_step(
    const EcologySeedProfilePOD& seed,
    std::span<const FloraGuildArchetypePOD> guild_catalog,
    std::span<EcologicalCellStatePOD> eco_cells,
    std::span<BiogeochemicalAndTrophicCellPOD> biogeo_cells,
    std::span<EcologyHarvestAndDisturbanceSamplePOD> harvest_samples,
    float dt_years,
    uint32_t stage_mask) noexcept {
    if ((stage_mask & static_cast<uint32_t>(EcologyStageFlags::WHITTAKER_CANOPY_PRODUCTIVITY)) != 0u &&
        dt_years > 0.0f) {
        ideam::core::EcologyKernelMath::step_whittaker_niche_and_canopy_npp(
            seed, guild_catalog, dt_years, eco_cells, biogeo_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(EcologyStageFlags::BIOGEOCHEMICAL_SUCCESSION)) != 0u &&
        dt_years > 0.0f) {
        ideam::core::EcologyKernelMath::step_biogeochemical_npk_and_succession(
            seed, guild_catalog, dt_years, eco_cells, biogeo_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(EcologyStageFlags::ROTHERMEL_WILDFIRE_DISTURBANCE)) != 0u &&
        dt_years > 0.0f) {
        ideam::core::EcologyKernelMath::step_rothermel_wildfire_and_disturbance(
            seed, guild_catalog, dt_years, eco_cells, biogeo_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(EcologyStageFlags::LOTKA_VOLTERRA_TROPHIC_WEB)) != 0u &&
        dt_years > 0.0f) {
        ideam::core::EcologyKernelMath::step_lotka_volterra_trophic_web(
            seed, dt_years, eco_cells, biogeo_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(EcologyStageFlags::HOLDRIDGE_SERAL_PARTITION)) != 0u) {
        ideam::core::EcologyKernelMath::compute_holdridge_and_seral_partition(
            seed, eco_cells, biogeo_cells);
    }

    if ((stage_mask & static_cast<uint32_t>(EcologyStageFlags::HARVEST_WILDFIRE_HAZARD_QUERY)) != 0u &&
        !harvest_samples.empty()) {
        ideam::core::EcologyKernelMath::evaluate_ecological_harvest_and_hazards(
            seed, eco_cells, biogeo_cells, harvest_samples);
    }
}

} // namespace ideam::core
