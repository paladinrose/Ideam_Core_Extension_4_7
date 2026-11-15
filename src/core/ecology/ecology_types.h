#pragma once

/**
 * @file ecology_types.h
 * @brief Pure C++26 cache-aligned POD structures and enumerations for IdeamEcology.
 *
 * Implements the data model for Section 2.3 (Biogeography, Seral Succession,
 * Century-Lite Soil C/N/P Cycling, Rothermel Wildfire Contagion, Spatial Lotka-Volterra
 * Trophic Webs & Alchemical Phytochemistry):
 * - Holdridge life zones & Whittaker biome envelopes
 * - Functional flora/fauna guild archetypes (`FloraGuildArchetypePOD`)
 * - Canopy/understory/root/litter biomass & Penman-Monteith evapotranspiration (`EcologicalCellStatePOD`)
 * - Soil organic carbon, nitrogen/phosphorus stoichiometry, wildfire intensity & trophic densities (`BiogeochemicalAndTrophicCellPOD`)
 */

#include <array>
#include <cstddef>
#include <cstdint>

namespace ideam::ecology {

inline constexpr size_t DEFAULT_ECOLOGY_GUILD_COUNT = 8;

/**
 * @enum HoldridgeLifeZone
 * @brief Canonical Holdridge / Whittaker biome classifications.
 */
enum class HoldridgeLifeZone : uint8_t {
    TROPICAL_RAIN_FOREST        = 0,
    TROPICAL_SEASONAL_FOREST    = 1,
    SUBTROPICAL_THORN_STEPPE    = 2,
    HOT_SUBTROPICAL_DESERT      = 3,
    WARM_TEMPERATE_WOODLAND     = 4,
    TEMPERATE_DECIDUOUS_FOREST  = 5,
    TEMPERATE_GRASSLAND_PRAIRIE = 6,
    BOREAL_CONIFEROUS_TAIGA     = 7,
    SUBALPINE_KRUMMHOLZ_MEADOW  = 8,
    POLAR_ALPINE_TUNDRA         = 9,
    RIPARIAN_WETLAND_GALLERY    = 10,
    KARST_SUBTERRANEAN_REFUGIUM = 11,
    COUNT                       = 12
};

/**
 * @enum SuccessionSeralStage
 * @brief Ecological succession seral stage from bare substrate to old-growth climax.
 */
enum class SuccessionSeralStage : uint8_t {
    BARE_SUBSTRATE            = 0,
    PIONEER_LICHEN_BRYOPHYTE  = 1,
    EARLY_HERBACEOUS_MEADOW   = 2,
    INTERMEDIATE_SHRUB_SCRUB  = 3,
    YOUNG_SUBCLIMAX_WOODLAND  = 4,
    OLD_GROWTH_CLIMAX_CANOPY  = 5,
    POST_DISTURBANCE_SNAG_BURN = 6
};

/**
 * @enum TrophicGuildRole
 * @brief Functional role of an ecological guild in the biosphere web.
 */
enum class TrophicGuildRole : uint8_t {
    PRIMARY_PRODUCER_CANOPY     = 0,
    PRIMARY_PRODUCER_UNDERSTORY = 1,
    NITROGEN_FIXING_PIONEER     = 2,
    MYCORRHIZAL_DECOMPOSER      = 3,
    GRAZING_MEGAHERBIVORE       = 4,
    BROWSING_MESOHERBIVORE      = 5,
    POLLINATOR_SEED_DISPERSER   = 6,
    APEX_PREDATOR_CARNIVORE     = 7,
    COUNT                       = 8
};

/**
 * @enum EcologicalDisturbanceState
 * @brief Active or recent ecological disturbance regime in a cell.
 */
enum class EcologicalDisturbanceState : uint8_t {
    UNDISTURBED                 = 0,
    ACTIVE_CROWN_WILDFIRE       = 1,
    ACTIVE_SURFACE_BRUSHFIRE    = 2,
    SMOLDERING_PEAT_EMBER       = 3,
    WINDTHROW_TORNADO_BLOWDOWN  = 4,
    DROUGHT_BARK_BEETLE_DIEBACK = 5,
    FLOOD_SCOUR_ALLUVIAL        = 6
};

/**
 * @enum EcologyMapLayer
 * @brief Diagnostic 2D heatmap visualization layers for the Ecology editor canvas.
 */
enum class EcologyMapLayer : uint8_t {
    WHITTAKER_HOLDRIDGE_BIOME             = 0,
    SUCCESSION_SERAL_STAGE                = 1,
    CANOPY_AND_UNDERSTORY_BIOMASS         = 2,
    SOIL_CARBON_AND_NITROGEN_NPK          = 3,
    ROTHERMEL_WILDFIRE_INTENSITY          = 4,
    TROPHIC_HERBIVORE_PREDATOR_DENSITY    = 5,
    EVAPOTRANSPIRATION_AND_ALBEDO_FEEDBACK = 6,
    SHANNON_BIODIVERSITY_INDEX            = 7,
    ALCHEMICAL_PHYTOCHEMICAL_POTENCY      = 8,
    COUNT                                 = 9
};

/**
 * @struct EcologySeedProfilePOD
 * @brief 128-byte cache-aligned biosphere seed & ecophysiological constants profile.
 */
struct alignas(64) EcologySeedProfilePOD {
    uint64_t master_seed                 = 0x494445414D45434FULL; // 8B ("IDEAMECO")
    uint64_t whittaker_niche_seed        = 0;                     // 8B
    uint64_t succession_dispersal_seed   = 0;                     // 8B
    uint64_t trophic_lotka_volterra_seed = 0;                     // 8B
    uint64_t rothermel_wildfire_seed     = 0;                     // 8B
    uint32_t grid_width                  = 64;                    // 4B
    uint32_t grid_height                 = 64;                    // 4B
    uint32_t active_guild_count          = 8;                     // 4B
    float    cell_size_meters            = 50.0f;                 // 4B
    float    ambient_co2_ppm             = 410.0f;                // 4B
    float    beer_lambert_extinction_k   = 0.52f;                 // 4B
    // --- 64B boundary ---
    float    base_npp_efficiency         = 1.0f;                  // 4B
    float    q10_decomposition_factor    = 2.1f;                  // 4B
    float    nitrogen_fixation_scale     = 1.0f;                  // 4B
    float    rothermel_wind_spread_scale = 1.0f;                  // 4B
    float    lightning_ignition_threshold = 0.68f;                // 4B
    float    herbivore_grazing_rate      = 0.14f;                 // 4B
    float    predator_attack_rate        = 0.22f;                 // 4B
    float    predator_conversion_eff     = 0.28f;                 // 4B
    float    trophic_diffusion_rate      = 0.18f;                 // 4B
    float    alchemical_leyline_resonance = 1.0f;                 // 4B
    uint32_t reserved_pad[6]             = {0, 0, 0, 0, 0, 0};    // 24B -> Total = 128B
};
static_assert(sizeof(EcologySeedProfilePOD) == 128, "EcologySeedProfilePOD must be 128 bytes");

/**
 * @struct FloraGuildArchetypePOD
 * @brief 64-byte cache-aligned functional plant/fungi guild definition.
 */
struct alignas(64) FloraGuildArchetypePOD {
    uint16_t         guild_id                   = 0;     // 2B
    TrophicGuildRole trophic_role               = TrophicGuildRole::PRIMARY_PRODUCER_CANOPY; // 1B
    uint8_t          preferred_seral_stage      = 5;     // 1B
    float            optimal_temp_c             = 16.0f; // 4B
    float            temp_tolerance_sigma_c     = 9.5f;  // 4B
    float            optimal_precip_mm          = 1100.0f;// 4B
    float            precip_tolerance_sigma_mm  = 650.0f;// 4B
    float            drought_wilting_point_01   = 0.18f; // 4B
    float            shade_tolerance_01         = 0.55f; // 4B
    float            fire_resistance_bark_01    = 0.40f; // 4B
    float            serotinous_reseed_boost_01 = 0.25f; // 4B
    float            intrinsic_growth_rate_r    = 0.32f; // 4B
    float            max_carrying_biomass_kg_m2 = 28.0f; // 4B
    float            nitrogen_fixation_g_m2_yr  = 1.5f;  // 4B
    float            litter_lignin_recalcitrance = 0.35f;// 4B
    float            alchemical_reagent_affinity = 0.50f;// 4B
    float            specific_leaf_area_m2_kg   = 0.24f; // 4B -> Total = 64B
};
static_assert(sizeof(FloraGuildArchetypePOD) == 64, "FloraGuildArchetypePOD must be 64 bytes");

/**
 * @struct EcologicalCellStatePOD
 * @brief 64-byte cache-aligned per-cell vegetation structure, ecophysiology & biome state.
 */
struct alignas(64) EcologicalCellStatePOD {
    float                      mean_temperature_c         = 16.0f;  // 4B
    float                      annual_precipitation_mm    = 950.0f; // 4B
    float                      soil_moisture_01           = 0.55f;  // 4B
    float                      terrain_slope_gradient     = 0.08f;  // 4B
    float                      canopy_biomass_kg_m2       = 12.0f;  // 4B
    float                      understory_biomass_kg_m2   = 3.2f;   // 4B
    float                      root_mycorrhizal_kg_m2     = 4.5f;   // 4B
    float                      dead_fuel_litter_kg_m2     = 1.8f;   // 4B
    float                      leaf_area_index_lai        = 3.4f;   // 4B
    float                      canopy_height_m            = 18.0f;  // 4B
    float                      evapotranspiration_mm_day  = 3.1f;   // 4B
    float                      biogenic_albedo            = 0.14f;  // 4B
    float                      aerodynamic_roughness_z0   = 0.85f;  // 4B
    float                      shannon_diversity_h        = 1.65f;  // 4B
    float                      wind_speed_mps             = 5.5f;   // 4B
    HoldridgeLifeZone          life_zone                  = HoldridgeLifeZone::TEMPERATE_DECIDUOUS_FOREST; // 1B
    SuccessionSeralStage       seral_stage                = SuccessionSeralStage::OLD_GROWTH_CLIMAX_CANOPY;// 1B
    EcologicalDisturbanceState disturbance_state          = EcologicalDisturbanceState::UNDISTURBED;       // 1B
    uint8_t                    dominant_guild_id          = 0;      // 1B -> Total = 64B
};
static_assert(sizeof(EcologicalCellStatePOD) == 64, "EcologicalCellStatePOD must be 64 bytes");

/**
 * @struct BiogeochemicalAndTrophicCellPOD
 * @brief 64-byte cache-aligned per-cell soil C/N/P stoichiometry, Rothermel wildfire & Lotka-Volterra state.
 */
struct alignas(64) BiogeochemicalAndTrophicCellPOD {
    float    soil_organic_carbon_kg_m2      = 8.5f;  // 4B
    float    mineral_nitrogen_g_m2          = 42.0f; // 4B
    float    available_phosphorus_g_m2      = 12.0f; // 4B
    float    decomposer_microbial_activity  = 0.65f; // 4B
    float    wildfire_intensity_kw_m        = 0.0f;  // 4B (Rothermel fireline intensity)
    float    burn_scar_recovery_years       = 50.0f; // 4B
    float    dead_fuel_moisture_01          = 0.28f; // 4B
    float    windthrow_damage_01            = 0.0f;  // 4B
    float    grazer_herbivore_density       = 0.35f; // 4B
    float    browser_herbivore_density      = 0.28f; // 4B
    float    pollinator_disperser_density   = 0.62f; // 4B
    float    apex_predator_density          = 0.12f; // 4B
    float    net_primary_productivity_npp   = 1.25f; // 4B (kg/m2/yr)
    float    alchemical_phytochemical_grade = 0.30f; // 4B
    float    karst_refugium_humidity_bonus  = 0.0f;  // 4B
    uint32_t active_guild_presence_mask     = 0xFFu; // 4B -> Total = 64B
};
static_assert(sizeof(BiogeochemicalAndTrophicCellPOD) == 64, "BiogeochemicalAndTrophicCellPOD must be 64 bytes");

/**
 * @struct EcologyHarvestAndDisturbanceSamplePOD
 * @brief 32-byte output structure for ecological harvest, wildfire hazard & botanical queries (ECO_Q1).
 */
struct alignas(32) EcologyHarvestAndDisturbanceSamplePOD {
    float                      net_primary_productivity   = 0.0f; // 4B
    float                      wildfire_spread_risk_01    = 0.0f; // 4B
    float                      timber_canopy_yield_kg_m2  = 0.0f; // 4B
    float                      forage_carrying_capacity   = 0.0f; // 4B
    float                      alchemical_botanical_grade = 0.0f; // 4B
    float                      trophic_cascade_stability  = 0.0f; // 4B
    uint32_t                   cell_index                 = 0;    // 4B
    HoldridgeLifeZone          life_zone                  = HoldridgeLifeZone::TEMPERATE_DECIDUOUS_FOREST; // 1B
    SuccessionSeralStage       seral_stage                = SuccessionSeralStage::OLD_GROWTH_CLIMAX_CANOPY;// 1B
    EcologicalDisturbanceState disturbance_state          = EcologicalDisturbanceState::UNDISTURBED;       // 1B
    uint8_t                    dominant_guild_id          = 0;    // 1B -> Total = 32B
};
static_assert(sizeof(EcologyHarvestAndDisturbanceSamplePOD) == 32, "EcologyHarvestAndDisturbanceSamplePOD must be 32 bytes");

} // namespace ideam::ecology
