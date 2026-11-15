#pragma once

/**
 * @file climatology_types.h
 * @brief Pure C++26 cache-aligned POD structures and enumerations for IdeamClimatology.
 *
 * Implements the data model for Section 2.3 (Climatology, Synoptic Meteorology,
 * Orographic Foehn Thermodynamics & Supercell Tornado Vortex Dynamics):
 * - Hierarchical SplitMix64 seed profile & planetary boundary parameters
 * - Atmospheric column thermodynamics, barometric pressure & 3D wind vectors
 * - Clausius-Clapeyron moisture, cloud microphysics & precipitation/snowpack
 * - Burgers-Rott / Modified Rankine supercell mesocyclone & tornado vortex state
 * - Köppen-Geiger climate zones, Beaufort wind scale & Enhanced Fujita (EF0-EF5) tiers
 */

#include <array>
#include <cstddef>
#include <cstdint>

namespace ideam::climatology {

inline constexpr size_t MAX_ACTIVE_TORNADO_VORTICES = 16;

/**
 * @enum KoppenClimateZone
 * @brief Canonical Köppen-Geiger climate classification zones.
 */
enum class KoppenClimateZone : uint8_t {
    TROPICAL_RAINFOREST_AF = 0,
    TROPICAL_SAVANNA_AW    = 1,
    HOT_ARID_DESERT_BWH    = 2,
    COLD_STEPPE_BSK        = 3,
    MEDITERRANEAN_CSA      = 4,
    HUMID_SUBTROPICAL_CFA  = 5,
    OCEANIC_TEMPERATE_CFB  = 6,
    CONTINENTAL_BOREAL_DFC = 7,
    ALPINE_TUNDRA_ET       = 8,
    POLAR_ICE_CAP_EF       = 9,
    COUNT                  = 10
};

/**
 * @enum BeaufortWindScale
 * @brief Standard Beaufort wind force scale (0..12).
 */
enum class BeaufortWindScale : uint8_t {
    CALM_0            = 0,
    LIGHT_AIR_1       = 1,
    LIGHT_BREEZE_2    = 2,
    GENTLE_BREEZE_3   = 3,
    MODERATE_BREEZE_4 = 4,
    FRESH_BREEZE_5    = 5,
    STRONG_BREEZE_6   = 6,
    NEAR_GALE_7       = 7,
    GALE_8            = 8,
    STRONG_GALE_9     = 9,
    STORM_10          = 10,
    VIOLENT_STORM_11  = 11,
    HURRICANE_12      = 12
};

/**
 * @enum HydrometeorPhaseType
 * @brief Dominant cloud & precipitation phase in an atmospheric column.
 */
enum class HydrometeorPhaseType : uint8_t {
    CLEAR_SKY               = 0,
    STRATUS_FOG             = 1,
    CUMULUS_FAIR            = 2,
    NIMBOSTRATUS_RAIN       = 3,
    CUMULONIMBUS_STORM      = 4,
    GRAUPEL_HAIL_SQUALL     = 5,
    BLIZZARD_SNOW           = 6,
    SUPERCELL_MESOCYCLONE   = 7
};

/**
 * @enum EnhancedFujitaScale
 * @brief Enhanced Fujita (EF) tornado intensity rating based on 3-second wind gusts.
 */
enum class EnhancedFujitaScale : uint8_t {
    EF_NONE        = 0, ///< < 29 m/s (No tornado / sub-tornadic swirl)
    EF0_GALE       = 1, ///< 29 - 38 m/s
    EF1_MODERATE   = 2, ///< 38 - 49 m/s
    EF2_SIGNIFICANT = 3, ///< 49 - 60 m/s
    EF3_SEVERE     = 4, ///< 60 - 74 m/s
    EF4_DEVASTATING = 5, ///< 74 - 89 m/s
    EF5_INCREDIBLE = 6  ///< > 89 m/s
};

/**
 * @enum ClimatologyMapLayer
 * @brief Diagnostic 2D heatmap visualization layers for the Climatology editor canvas.
 */
enum class ClimatologyMapLayer : uint8_t {
    POTENTIAL_TEMPERATURE_KELVIN     = 0,
    SURFACE_PRESSURE_HPA             = 1,
    GEOSTROPHIC_AND_OROGRAPHIC_WIND  = 2,
    RELATIVE_HUMIDITY_AND_CLOUD_WATER = 3,
    PRECIPITATION_AND_SNOWPACK       = 4,
    CONVECTIVE_CAPE_AND_HELICITY     = 5,
    SUPERCELL_TORNADO_VORTICITY      = 6,
    KOPPEN_CLIMATE_CLASSIFICATION    = 7,
    SURFACE_RADIATIVE_ENERGY_BUDGET  = 8,
    COUNT                            = 9
};

/**
 * @struct ClimatologySeedProfilePOD
 * @brief 128-byte cache-aligned planetary climate seed & physical constants profile.
 */
struct alignas(64) ClimatologySeedProfilePOD {
    uint64_t master_seed              = 0x494445414D434C4DULL; // 8B ("IDEAMCLM")
    uint64_t solar_insolation_seed    = 0;                     // 8B
    uint64_t baroclinic_wave_seed     = 0;                     // 8B
    uint64_t convective_storm_seed    = 0;                     // 8B
    uint64_t tornado_vortex_seed      = 0;                     // 8B
    uint32_t grid_width               = 64;                    // 4B
    uint32_t grid_height              = 64;                    // 4B
    uint32_t max_vortices             = 8;                     // 4B
    float    cell_size_meters         = 50.0f;                 // 4B
    float    planet_latitude_deg      = 38.0f;                 // 4B
    float    seasonal_solar_decl_deg  = 12.5f;                 // 4B
    // --- 64B boundary ---
    float    diurnal_hour_0_24        = 14.5f;                 // 4B (Afternoon peak convection default)
    float    solar_constant_w_m2      = 1361.0f;               // 4B
    float    coriolis_parameter_f0    = 0.00009f;              // 4B (s^-1 at mid-latitudes)
    float    dry_adiabatic_lapse_k_m  = 0.0098f;               // 4B (9.8 K/km)
    float    moist_adiabatic_lapse_k_m = 0.0055f;              // 4B (5.5 K/km)
    float    sea_level_pressure_hpa   = 1013.25f;              // 4B
    float    sea_level_temp_k         = 291.15f;               // 4B (18 C reference)
    float    prevailing_zonal_wind_mps = 9.5f;                 // 4B (Westerly jet baseline)
    float    prevailing_merid_wind_mps = 2.0f;                 // 4B
    float    synoptic_wave_amplitude_hpa = 14.0f;              // 4B
    float    orographic_lift_scale    = 1.0f;                  // 4B
    float    tornado_spawn_ehi_threshold = 1.25f;              // 4B (Energy-Helicity Index trigger)
    uint32_t reserved_pad[4]          = {0, 0, 0, 0};          // 16B -> Total = 128B
};
static_assert(sizeof(ClimatologySeedProfilePOD) == 128, "ClimatologySeedProfilePOD must be 128 bytes");

/**
 * @struct AtmosphericColumnCellPOD
 * @brief 64-byte cache-aligned per-cell boundary layer & tropospheric dynamics state.
 */
struct alignas(64) AtmosphericColumnCellPOD {
    float             surface_elevation_m       = 0.0f;   // 4B
    float             surface_albedo            = 0.22f;  // 4B
    float             surface_roughness_z0      = 0.15f;  // 4B
    float             ocean_water_fraction      = 0.0f;   // 4B
    float             shortwave_insolation_w_m2 = 650.0f; // 4B
    float             longwave_outgoing_w_m2    = 240.0f; // 4B
    float             potential_temperature_k   = 292.0f; // 4B
    float             surface_air_temperature_k = 290.0f; // 4B
    float             boundary_pressure_hpa     = 1013.25f; // 4B
    float             upper_troposphere_temp_k  = 238.0f; // 4B (500 hPa temperature for CAPE)
    float             wind_velocity_u_mps       = 6.0f;   // 4B (Zonal +X wind)
    float             wind_velocity_v_mps       = 1.5f;   // 4B (Meridional +Y wind)
    float             vertical_updraft_w_mps    = 0.0f;   // 4B (Total vertical velocity w)
    float             orographic_lift_mps       = 0.0f;   // 4B (Terrain-forced slope lift)
    float             air_density_kg_m3         = 1.225f; // 4B
    BeaufortWindScale beaufort_scale            = BeaufortWindScale::MODERATE_BREEZE_4; // 1B
    KoppenClimateZone koppen_zone               = KoppenClimateZone::HUMID_SUBTROPICAL_CFA; // 1B
    uint8_t           air_mass_front_id         = 0;      // 1B (0=Warm/Tropical, 1=Baroclinic Front, 2=Cold/Polar)
    uint8_t           reserved_flags            = 0;      // 1B -> Total = 64B
};
static_assert(sizeof(AtmosphericColumnCellPOD) == 64, "AtmosphericColumnCellPOD must be 64 bytes");

/**
 * @struct MoistureAndCloudCellPOD
 * @brief 64-byte cache-aligned per-cell moisture, cloud microphysics & convective instability state.
 */
struct alignas(64) MoistureAndCloudCellPOD {
    float                specific_humidity_kg_kg     = 0.009f; // 4B
    float                relative_humidity_01        = 0.65f;  // 4B
    float                cloud_liquid_water_kg_m2    = 0.10f;  // 4B
    float                cloud_ice_water_kg_m2       = 0.0f;   // 4B
    float                precipitable_water_mm       = 18.0f;  // 4B
    float                rainfall_rate_mm_hr         = 0.0f;   // 4B
    float                snowfall_rate_mm_hr         = 0.0f;   // 4B
    float                snowpack_water_equiv_mm     = 0.0f;   // 4B
    float                surface_evaporation_mm_hr   = 0.15f;  // 4B
    float                latent_heat_flux_w_m2       = 85.0f;  // 4B
    float                lifting_condensation_lvl_m  = 1100.0f;// 4B
    float                convective_cape_j_kg        = 450.0f; // 4B (Convective Available Potential Energy)
    float                storm_rel_helicity_m2_s2    = 75.0f;  // 4B (0-3 km SRH)
    float                cloud_cover_fraction        = 0.25f;  // 4B
    float                vortex_tangential_wind_mps  = 0.0f;   // 4B (Superimposed tornado wind at cell)
    HydrometeorPhaseType hydrometeor_phase           = HydrometeorPhaseType::CUMULUS_FAIR; // 1B
    EnhancedFujitaScale  local_ef_tier               = EnhancedFujitaScale::EF_NONE;       // 1B
    uint16_t             annual_precip_estimate_mm   = 850;    // 2B -> Total = 64B
};
static_assert(sizeof(MoistureAndCloudCellPOD) == 64, "MoistureAndCloudCellPOD must be 64 bytes");

/**
 * @struct TornadoSupercellVortexPOD
 * @brief 64-byte cache-aligned Burgers-Rott / Modified Rankine supercell tornado vortex entity.
 */
struct alignas(64) TornadoSupercellVortexPOD {
    uint16_t            vortex_id                    = 0;     // 2B
    uint16_t            parent_cell_x                = 0;     // 2B
    uint16_t            parent_cell_y                = 0;     // 2B
    EnhancedFujitaScale ef_rating                    = EnhancedFujitaScale::EF_NONE; // 1B
    uint8_t             is_active                    = 0;     // 1B
    float               position_x_m                 = 0.0f;  // 4B
    float               position_z_m                 = 0.0f;  // 4B
    float               translation_u_mps            = 12.0f; // 4B
    float               translation_v_mps            = 6.0f;  // 4B
    float               core_radius_m                = 85.0f; // 4B (Radius of maximum wind R_c)
    float               outer_damage_radius_m        = 450.0f;// 4B
    float               max_tangential_wind_mps      = 58.0f; // 4B (V_max at R_c)
    float               central_pressure_deficit_hpa = 45.0f; // 4B (Cyclostrophic core drop)
    float               vertical_suction_updraft_mps = 36.0f; // 4B (Core axial updraft)
    float               radial_inflow_ratio          = 0.42f; // 4B (Inward spiral tangent angle)
    float               debris_load_kg_m3            = 0.35f; // 4B (Lofted debris density)
    float               swirl_ratio_s                = 1.15f; // 4B (Corner flow swirl parameter)
    float               remaining_lifetime_sec       = 180.0f;// 4B -> Total = 64B
};
static_assert(sizeof(TornadoSupercellVortexPOD) == 64, "TornadoSupercellVortexPOD must be 64 bytes");

/**
 * @struct ClimatologyHazardAndBiomeSamplePOD
 * @brief 32-byte output structure for severe weather hazard & climate query tasks (CLM_Q1).
 */
struct alignas(32) ClimatologyHazardAndBiomeSamplePOD {
    float                tornado_wind_gust_mps       = 0.0f; // 4B
    float                barometric_suction_pa       = 0.0f; // 4B
    float                debris_ballistic_flux       = 0.0f; // 4B
    float                blizzard_whiteout_index     = 0.0f; // 4B
    float                drought_aridity_index       = 0.0f; // 4B
    float                lightning_strike_prob       = 0.0f; // 4B
    uint32_t             cell_index                  = 0;    // 4B
    KoppenClimateZone    koppen_zone                 = KoppenClimateZone::HUMID_SUBTROPICAL_CFA; // 1B
    EnhancedFujitaScale  ef_damage_tier              = EnhancedFujitaScale::EF_NONE;             // 1B
    BeaufortWindScale    beaufort_scale              = BeaufortWindScale::CALM_0;                // 1B
    HydrometeorPhaseType hydrometeor_phase           = HydrometeorPhaseType::CLEAR_SKY;          // 1B -> Total = 32B
};
static_assert(sizeof(ClimatologyHazardAndBiomeSamplePOD) == 32, "ClimatologyHazardAndBiomeSamplePOD must be 32 bytes");

} // namespace ideam::climatology
