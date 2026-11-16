#pragma once

/**
 * @file geomorphology_types.h
 * @brief Pure C++26 Data-Oriented Design (DOD) POD structures and enums for `IdeamGeomorphology`
 * (`src/core/geomorphology/`).
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <string_view>

namespace ideam::core {

static constexpr size_t MAX_STRATA_PER_COLUMN = 6u;
static constexpr size_t DEFAULT_LITHOLOGY_COUNT = 8u;

enum RockGenesisClass : uint8_t {
    IGNEOUS_PLUTONIC              = 0,
    ROCK_IGNEOUS_PLUTONIC         = 0,
    IGNEOUS_VOLCANIC              = 1,
    ROCK_IGNEOUS_VOLCANIC         = 1,
    SEDIMENTARY_CLASTIC           = 2,
    ROCK_SEDIMENTARY_CLASTIC      = 2,
    SEDIMENTARY_CARBONATE         = 3,
    ROCK_SEDIMENTARY_CHEMICAL     = 3,
    METAMORPHIC_FOLIATED          = 4,
    ROCK_METAMORPHIC_FOLIATED     = 4,
    METAMORPHIC_NON_FOLIATED      = 5,
    ROCK_METAMORPHIC_NON_FOLIATED = 5,
    UNCONSOLIDATED_REGOLITH       = 6,
    ROCK_REGOLITH_ALLUVIUM        = 6,
    PEDOGENIC_SOIL                = 7,
    ROCK_SOIL_PEDOGENIC           = 7
};

enum CanonicalLithologyId : uint16_t {
    GRANITE             = 0,
    LITHOLOGY_GRANITE   = 0,
    BASALT              = 1,
    LITHOLOGY_BASALT    = 1,
    SANDSTONE           = 2,
    LITHOLOGY_SANDSTONE = 2,
    LIMESTONE           = 3,
    LITHOLOGY_LIMESTONE = 3,
    SCHIST              = 4,
    LITHOLOGY_SCHIST    = 4,
    MARBLE              = 5,
    LITHOLOGY_MARBLE    = 5,
    REGOLITH            = 6,
    LITHOLOGY_REGOLITH  = 6,
    LOAM_SOIL           = 7,
    LITHOLOGY_LOAM_SOIL = 7
};

enum TectonicBoundaryType : uint8_t {
    TECTONIC_BOUNDARY_INTERIOR              = 0,
    TECTONIC_BOUNDARY_CONVERGENT_SUBDUCTION = 1,
    TECTONIC_BOUNDARY_CONVERGENT_OROGENY    = 2,
    TECTONIC_BOUNDARY_DIVERGENT_RIFT        = 3,
    TECTONIC_BOUNDARY_TRANSFORM_SHEAR       = 4
};

enum DrainageBasinClass : uint8_t {
    BASIN_EXORHEIC_OCEANIC = 0,
    BASIN_ENDORHEIC_CLOSED = 1,
    BASIN_FLUVIAL_CHANNEL  = 2,
    BASIN_LACUSTRINE_LAKE  = 3,
    BASIN_KARST_SINK       = 4,
    BASIN_GLACIAL_VALLEY   = 5
};

enum class GeomorphologyMapLayer : uint8_t {
    ELEVATION_SHADED_RELIEF    = 0,
    TECTONIC_PLATES_AND_STRESS = 1,
    TOP_EXPOSED_LITHOLOGY      = 2,
    SURFACE_WATER_DEPTH        = 3,
    STREAM_POWER_EROSION       = 4,
    STRAHLER_STREAM_ORDER      = 5,
    KARST_CAVITY_POTENTIAL     = 6,
    SOIL_HUMUS_FERTILITY       = 7,
    MINERAL_VEIN_POTENTIAL     = 8,
    COUNT                      = 9
};

[[nodiscard]] constexpr std::string_view rock_genesis_to_string(uint8_t p_class) noexcept {
    switch (p_class) {
        case ROCK_IGNEOUS_PLUTONIC:         return "Igneous (Plutonic)";
        case ROCK_IGNEOUS_VOLCANIC:         return "Igneous (Volcanic)";
        case ROCK_SEDIMENTARY_CLASTIC:      return "Sedimentary (Clastic)";
        case ROCK_SEDIMENTARY_CHEMICAL:     return "Sedimentary (Chemical / Karst)";
        case ROCK_METAMORPHIC_FOLIATED:     return "Metamorphic (Foliated)";
        case ROCK_METAMORPHIC_NON_FOLIATED: return "Metamorphic (Non-Foliated)";
        case ROCK_REGOLITH_ALLUVIUM:        return "Regolith / Alluvium";
        case ROCK_SOIL_PEDOGENIC:           return "Pedogenic Soil (O/A/B)";
        default:                            return "Unknown Lithology";
    }
}

struct alignas(64) GeomorphologySeedProfilePOD {
    uint64_t master_seed              = 0x494445414D47454FULL; // 8B
    uint64_t tectonic_seed            = 0ULL;                  // 8B
    uint64_t stratigraphy_seed        = 0ULL;                  // 8B
    uint64_t hydrology_seed           = 0ULL;                  // 8B
    uint64_t karst_seed               = 0ULL;                  // 8B
    uint64_t resource_vein_seed       = 0ULL;                  // 8B
    uint32_t grid_width               = 64u;                   // 4B
    uint32_t grid_height              = 64u;                   // 4B
    uint32_t plate_count              = 8u;                    // 4B
    uint32_t active_strata_per_column = 4u;                    // 4B -> 64B boundary
    float    cell_size_meters         = 50.0f;                 // 4B
    float    base_crust_thickness_m   = 1200.0f;               // 4B
    float    max_orogenic_uplift_m    = 950.0f;                // 4B
    float    sea_level_elevation_m    = 1150.0f;               // 4B
    float    tectonic_noise_frequency = 0.035f;                // 4B
    float    hydro_gravity_mps2       = 9.81f;                 // 4B
    float    karst_dissolution_scale  = 1.0f;                  // 4B
    float    pedogenesis_rate_scale   = 1.0f;                  // 4B
    uint32_t reserved_pad[8]          = {0};                   // 32B -> 128B total
};
static_assert(sizeof(GeomorphologySeedProfilePOD) == 128, "GeomorphologySeedProfilePOD must be 128 bytes.");

struct alignas(64) LithologyArchetypePOD {
    uint16_t lithology_id                  = 0u;                        // 2B
    uint8_t  genesis_class                 = ROCK_IGNEOUS_PLUTONIC;     // 1B
    uint8_t  reserved_u8                   = 0u;                        // 1B
    float    density_kg_m3                 = 2700.0f;                   // 4B
    float    hardness_mohs                 = 6.5f;                      // 4B
    float    fluvial_erodibility_k         = 0.0008f;                   // 4B
    float    talus_angle_of_repose_rad     = 0.78f;                     // 4B
    float    thermal_weathering_rate       = 0.0012f;                   // 4B
    float    porosity                      = 0.015f;                    // 4B
    float    hydraulic_conductivity        = 0.0005f;                   // 4B
    float    carbonate_solubility          = 0.0001f;                   // 4B
    float    compressive_strength_mpa      = 200.0f;                    // 4B
    float    sediment_grain_diameter_mm    = 2.5f;                      // 4B
    float    pedogenesis_humus_retention   = 0.15f;                     // 4B
    uint16_t next_metamorphic_lithology_id = LITHOLOGY_SCHIST;          // 2B
    uint16_t weathering_product_id         = LITHOLOGY_REGOLITH;        // 2B
    uint32_t reserved_pad[3]               = {0};                       // 12B -> 64B total
};
static_assert(sizeof(LithologyArchetypePOD) == 64, "LithologyArchetypePOD must be 64 bytes.");

struct alignas(16) StratigraphicLayerPOD {
    uint16_t lithology_id       = LITHOLOGY_GRANITE; // 2B
    uint16_t fault_fracture_flags = 0u;              // 2B
    float    thickness_m        = 0.0f;              // 4B
    float    porosity_fraction  = 0.02f;             // 4B
    float    fracture_density   = 0.0f;              // 4B
    float    mineral_vein_grade = 0.0f;              // 4B
    float    water_saturation   = 0.0f;              // 4B
    uint32_t reserved_pad       = 0u;                // 4B -> 32B total
};
static_assert(sizeof(StratigraphicLayerPOD) == 32, "StratigraphicLayerPOD must be 32 bytes.");

struct alignas(64) StratigraphicColumnPOD {
    StratigraphicLayerPOD layers[MAX_STRATA_PER_COLUMN] = {}; // 192B
    float    base_elevation_m          = 100.0f;              // 4B
    float    total_column_height_m     = 1200.0f;             // 4B
    float    regolith_thickness_m      = 0.5f;                // 4B
    float    topsoil_thickness_m       = 0.2f;                // 4B
    float    soil_organic_humus        = 0.15f;               // 4B
    float    karst_cavity_void_m       = 0.0f;                // 4B
    float    cumulative_erosion_delta_m= 0.0f;                // 4B
    uint16_t dominant_surface_lithology= LITHOLOGY_GRANITE;   // 2B
    uint8_t  active_layer_count        = 4u;                  // 1B
    uint8_t  dominant_soil_horizon     = 1u;                  // 1B
    uint32_t reserved_pad[8]           = {0};                 // 32B -> 256B total

    [[nodiscard]] constexpr uint16_t top_exposed_lithology() const noexcept {
        if (active_layer_count > 0u && active_layer_count <= MAX_STRATA_PER_COLUMN) {
            return layers[active_layer_count - 1u].lithology_id;
        }
        return dominant_surface_lithology;
    }
};
static_assert(sizeof(StratigraphicColumnPOD) == 256, "StratigraphicColumnPOD must be 256 bytes.");

struct alignas(64) TectonicPlatePOD {
    glm::vec2 seed_position_uv       = glm::vec2(0.5f, 0.5f);      // 8B
    glm::vec2 drift_velocity_m_kyr   = glm::vec2(0.02f, 0.0f);     // 8B
    float     angular_spin_rad_myr   = 0.0f;                       // 4B
    float     crustal_density_kg_m3  = 2700.0f;                    // 4B
    float     base_thickness_m       = 35000.0f;                   // 4B
    float     buoyancy_offset_m      = 400.0f;                     // 4B
    float     thermal_age_myr        = 120.0f;                     // 4B
    uint16_t  plate_id               = 0u;                         // 2B
    uint16_t  primary_lithology_id   = LITHOLOGY_GRANITE;          // 2B
    uint8_t   is_oceanic             = 0u;                         // 1B
    uint8_t   reserved_padding[23]   = {0};                        // 23B -> 64B total
};
static_assert(sizeof(TectonicPlatePOD) == 64, "TectonicPlatePOD must be 64 bytes.");

struct alignas(32) CrustalTectonicCellPOD {
    uint16_t primary_plate_id         = 0u;                         // 2B
    uint16_t secondary_plate_id       = 0u;                         // 2B
    uint8_t  boundary_type            = TECTONIC_BOUNDARY_INTERIOR; // 1B
    uint8_t  volcanic_activity_mask   = 0u;                         // 1B
    uint16_t reserved_u16             = 0u;                         // 2B
    float    crust_thickness_m        = 1200.0f;                    // 4B
    float    convergent_stress        = 0.0f;                       // 4B
    float    shear_stress_mpa         = 0.0f;                       // 4B
    float    uplift_rate_mm_yr        = 0.0f;                       // 4B
    float    geothermal_flux_w_m2     = 0.065f;                     // 4B
    float    isostatic_equilibrium_m  = 0.0f;                       // 4B -> 32B total
};
static_assert(sizeof(CrustalTectonicCellPOD) == 32, "CrustalTectonicCellPOD must be 32 bytes.");

struct alignas(64) HydrologyCellStatePOD {
    glm::vec4 outflow_flux_n_e_s_w    = glm::vec4(0.0f);            // 16B
    glm::vec2 velocity_uv             = glm::vec2(0.0f);            // 8B
    float     water_depth_m           = 0.0f;                       // 4B
    float     groundwater_table_m     = 0.0f;                       // 4B
    float     suspended_sediment_kg   = 0.0f;                       // 4B
    float     volumetric_discharge_m3s= 0.0f;                       // 4B
    float     stream_power_index      = 0.0f;                       // 4B
    float     slope_magnitude         = 0.0f;                       // 4B
    uint32_t  drainage_basin_id       = 0u;                         // 4B
    uint32_t  upstream_catchment_cells= 1u;                         // 4B
    uint8_t   strahler_order          = 1u;                         // 1B
    uint8_t   basin_class             = BASIN_EXORHEIC_OCEANIC;     // 1B
    uint8_t   receiver_dir_d8         = 255u;                       // 1B
    uint8_t   is_inundated_mask       = 0u;                         // 1B
    uint32_t  reserved_pad            = 0u;                         // 4B -> 64B total
};
static_assert(sizeof(HydrologyCellStatePOD) == 64, "HydrologyCellStatePOD must be 64 bytes.");

struct alignas(32) GeomorphicHazardSamplePOD {
    float    landslide_hazard_probability = 0.0f; // 4B
    float    karst_sinkhole_hazard        = 0.0f; // 4B
    float    flash_flood_hazard           = 0.0f; // 4B
    float    hydrothermal_ore_grade       = 0.0f; // 4B
    float    placer_alluvial_grade        = 0.0f; // 4B
    float    evaporite_brine_grade        = 0.0f; // 4B
    uint32_t cell_index                   = 0u;   // 4B
    uint32_t hazard_flags                 = 0u;   // 4B -> 32B total
};
static_assert(sizeof(GeomorphicHazardSamplePOD) == 32, "GeomorphicHazardSamplePOD must be 32 bytes.");

} // namespace ideam::core
