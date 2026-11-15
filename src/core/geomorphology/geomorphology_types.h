#pragma once

/**
 * geomorphology_types.h
 * Pure C++26 Data-Oriented Design (DOD) POD structures and enums for `IdeamGeomorphology`
 * (`src/core/geomorphology/`).
 *
 * Covers the 5 core lithosphere & hydrosphere simulation pillars:
 *   1. Hierarchical Deterministic Seed Derivation (`GeomorphologySeedProfilePOD`)
 *   2. Tectonics, Crustal Stress, Orogeny & Isostasy (`TectonicPlatePOD`, `CrustalTectonicCellPOD`)
 *   3. Multi-Layer Stratigraphic Column & Mineralogy (`LithologyArchetypePOD`, `StratigraphicLayerPOD`, `StratigraphicColumnPOD`)
 *   4. 2D Shallow-Water Virtual Pipe Hydrology & Strahler Drainage (`HydrologyCellStatePOD`)
 *   5. Stream-Power Fluvial Incision, Thermal Talus Creep, Karst Dissolution & Soil Pedogenesis
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <string_view>

namespace ideam::core {

static constexpr size_t MAX_STRATA_PER_COLUMN = 6u;
static constexpr size_t DEFAULT_LITHOLOGY_COUNT = 8u;

/**
 * Primary rock and regolith genetic classification.
 */
enum RockGenesisClass : uint8_t {
    ROCK_IGNEOUS_PLUTONIC         = 0, ///< Slow-cooling intrusive felsic/intermediate rock (e.g., Granite, Diorite)
    ROCK_IGNEOUS_VOLCANIC         = 1, ///< Extrusive mafic/felsic lava & tephra (e.g., Basalt, Rhyolite, Obsidian)
    ROCK_SEDIMENTARY_CLASTIC      = 2, ///< Consolidated clastic detritus (e.g., Sandstone, Shale, Conglomerate)
    ROCK_SEDIMENTARY_CHEMICAL     = 3, ///< Soluble carbonate/evaporite strata (e.g., Limestone, Dolomite, Gypsum)
    ROCK_METAMORPHIC_FOLIATED     = 4, ///< High-pressure tectonic belt rock (e.g., Slate, Schist, Gneiss)
    ROCK_METAMORPHIC_NON_FOLIATED = 5, ///< Contact/thermal metamorphic rock (e.g., Quartzite, Marble)
    ROCK_REGOLITH_ALLUVIUM        = 6, ///< Unconsolidated talus, colluvium, and fluvial alluvium
    ROCK_SOIL_PEDOGENIC           = 7  ///< Weathered organic-mineral topsoil & subsoil horizons (O/A/B)
};

/**
 * Canonical canonical lithology IDs in the default 8-entry catalog.
 */
enum CanonicalLithologyId : uint16_t {
    LITHOLOGY_GRANITE   = 0, ///< Igneous Plutonic basement
    LITHOLOGY_BASALT    = 1, ///< Igneous Volcanic oceanic/flood basalt
    LITHOLOGY_SANDSTONE = 2, ///< Sedimentary Clastic
    LITHOLOGY_LIMESTONE = 3, ///< Sedimentary Chemical (Karst-soluble carbonate)
    LITHOLOGY_SCHIST    = 4, ///< Metamorphic Foliated
    LITHOLOGY_MARBLE    = 5, ///< Metamorphic Non-Foliated
    LITHOLOGY_REGOLITH  = 6, ///< Unconsolidated talus / gravel / alluvium
    LITHOLOGY_LOAM_SOIL = 7  ///< Pedogenic topsoil (O/A/B horizon composite)
};

/**
 * Tectonic boundary regime per grid cell.
 */
enum TectonicBoundaryType : uint8_t {
    TECTONIC_BOUNDARY_INTERIOR              = 0, ///< Stable craton / abyssal plain
    TECTONIC_BOUNDARY_CONVERGENT_SUBDUCTION = 1, ///< Oceanic-continental or oceanic-oceanic subduction trench + volcanic arc
    TECTONIC_BOUNDARY_CONVERGENT_OROGENY    = 2, ///< Continental-continental collision fold-and-thrust belt
    TECTONIC_BOUNDARY_DIVERGENT_RIFT        = 3, ///< Mid-ocean ridge or continental rift graben
    TECTONIC_BOUNDARY_TRANSFORM_SHEAR       = 4  ///< Strike-slip fault zone
};

/**
 * Hydrological drainage classification written to `HydrologyCellStatePOD::drainage_class`
 * and `r_selection.group_masks` by `DrainageBasinAndStrahlerPartitionMetadataLogic`.
 */
enum DrainageBasinClass : uint8_t {
    BASIN_EXORHEIC_OCEANIC = 0, ///< Drains to marine/oceanic base level
    BASIN_ENDORHEIC_CLOSED = 1, ///< Internal closed basin / playa / salt flat
    BASIN_FLUVIAL_CHANNEL  = 2, ///< Active perennial river channel (Strahler order >= 2)
    BASIN_LACUSTRINE_LAKE  = 3, ///< Freshwater or saline lake pool
    BASIN_KARST_SINK       = 4, ///< Subterranean karst swallow hole / doline
    BASIN_GLACIAL_VALLEY   = 5  ///< High-altitude cryospheric trough
};

/**
 * Cartographic visualization layer selector for 2D/3D terrain inspection.
 */
enum GeomorphologyMapLayer : uint8_t {
    GEO_MAP_ELEVATION_HYPSOMETRIC = 0,
    GEO_MAP_TECTONIC_STRESS       = 1,
    GEO_MAP_SURFACE_LITHOLOGY     = 2,
    GEO_MAP_HYDROLOGY_DISCHARGE   = 3,
    GEO_MAP_SEDIMENT_EROSION      = 4,
    GEO_MAP_SOIL_AND_KARST        = 5,
    GEO_MAP_LAYER_COUNT           = 6
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

/**
 * `GeomorphologySeedProfilePOD` (64 Bytes, alignas(64))
 * Hierarchical seed tree derived deterministically via SplitMix64 from `master_seed`,
 * accompanied by planetary physical constants.
 */
struct alignas(64) GeomorphologySeedProfilePOD {
    uint64_t master_seed              = 0x494445414D47454FULL; ///< 8B (0..8): Root world seed ("IDEAMGEO")
    uint64_t tectonic_seed            = 0ULL;                  ///< 8B (8..16): Plate Voronoi & mantle convection seed
    uint64_t lithology_seed           = 0ULL;                  ///< 8B (16..24): Stratigraphic province & mineral vein seed
    uint64_t hydrology_seed           = 0ULL;                  ///< 8B (24..32): Precipitation & initial aquifer seed
    uint64_t erosion_pedology_seed    = 0ULL;                  ///< 8B (32..40): Micro-channel & karst perturbation seed
    float    planet_radius_km         = 6371.0f;               ///< 4B (40..44)
    float    sea_level_m              = 0.0f;                  ///< 4B (44..48)
    float    crust_base_thickness_m   = 35000.0f;              ///< 4B (48..52)
    float    mantle_density_kg_m3     = 3300.0f;               ///< 4B (52..56)
    float    gravity_m_s2             = 9.81f;                 ///< 4B (56..60)
    uint32_t tectonic_plate_count     = 8u;                    ///< 4B (60..64) -> 64B total
};
static_assert(sizeof(GeomorphologySeedProfilePOD) == 64, "GeomorphologySeedProfilePOD must be 64 bytes.");

/**
 * `LithologyArchetypePOD` (64 Bytes, alignas(64))
 * Physical, mechanical, hydrological, and chemical parameters of a rock/soil type.
 */
struct alignas(64) LithologyArchetypePOD {
    uint16_t lithology_id                  = 0u;                        ///< 2B (0..2)
    uint8_t  genesis_class                 = ROCK_IGNEOUS_PLUTONIC;     ///< 1B (2)
    uint8_t  reserved_u8                   = 0u;                        ///< 1B (3)
    uint16_t next_metamorphic_lithology_id = LITHOLOGY_SCHIST;          ///< 2B (4..6)
    uint16_t weathering_product_id         = LITHOLOGY_REGOLITH;        ///< 2B (6..8)
    float    density_kg_m3                 = 2700.0f;                   ///< 4B (8..12)
    float    compressive_strength_mpa      = 150.0f;                    ///< 4B (12..16)
    float    erodibility_kf                = 0.00025f;                  ///< 4B (16..20): Stream-power incision coefficient K_f
    float    thermal_talus_angle_rad       = 0.68f;                     ///< 4B (20..24): Critical angle of repose (~39 deg)
    float    hydraulic_conductivity_m_s    = 1.0e-5f;                   ///< 4B (24..28): Darcy permeability K_sat
    float    porosity                      = 0.08f;                     ///< 4B (28..32): Void fraction [0..0.6]
    float    carbonate_solubility          = 0.0f;                      ///< 4B (32..36): Karst dissolution rate [0..1]
    float    silica_fraction               = 0.65f;                     ///< 4B (36..40): Felsic vs. mafic index
    float    mineral_fertility_yield       = 0.35f;                     ///< 4B (40..44): Weathering nutrient release
    float    metamorphic_stress_mpa        = 220.0f;                    ///< 4B (44..48): Threshold for solid-state metamorphism
    glm::vec4 display_color_rgba           = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); ///< 16B (48..64) -> 64B total
};
static_assert(sizeof(LithologyArchetypePOD) == 64, "LithologyArchetypePOD must be 64 bytes.");

/**
 * `StratigraphicLayerPOD` (16 Bytes, alignas(16))
 * Single horizontal or folded rock/regolith stratum within a cell's vertical column.
 */
struct alignas(16) StratigraphicLayerPOD {
    uint16_t lithology_id             = LITHOLOGY_GRANITE; ///< 2B (0..2)
    uint16_t fault_fracture_flags     = 0u;                ///< 2B (2..4): Bitmask of jointing/fault damage
    float    thickness_m              = 0.0f;              ///< 4B (4..8): Vertical thickness in meters
    float    folding_dip_rad          = 0.0f;              ///< 4B (8..12): Structural dip angle in radians
    float    mineral_enrichment_grade = 0.0f;              ///< 4B (12..16): Hydrothermal/placer ore grade [0..1] -> 16B total
};
static_assert(sizeof(StratigraphicLayerPOD) == 16, "StratigraphicLayerPOD must be 16 bytes.");

/**
 * `StratigraphicColumnPOD` (128 Bytes, alignas(64))
 * Complete vertical geological column for a single terrain cell:
 *   - `layers[0]` is the deep basement stratum; `layers[active_layer_count - 1]` is the exposed surface rock/regolith.
 *   - Coupled with unconsolidated regolith, pedogenic soil horizons (`topsoil_thickness_m`, `soil_humus_fraction`),
 *     and cached surface elevation (`surface_elevation_m`).
 */
struct alignas(64) StratigraphicColumnPOD {
    StratigraphicLayerPOD layers[MAX_STRATA_PER_COLUMN] = {}; ///< 96B (0..96): Ordered bottom-to-top (0 = basement)
    float    basement_elevation_m         = -2000.0f;         ///< 4B  (96..100): Elevation of the base of layer[0]
    float    surface_elevation_m          = 0.0f;             ///< 4B  (100..104): Total top elevation (basement + sum(layers) + regolith + topsoil)
    float    regolith_thickness_m         = 0.5f;             ///< 4B  (104..108): Unconsolidated C-horizon saprolite/alluvium
    float    topsoil_thickness_m          = 0.2f;             ///< 4B  (108..112): Pedogenic O/A/B soil thickness
    float    soil_humus_fraction          = 0.15f;            ///< 4B  (112..116): Organic matter content [0..1]
    float    soil_mineral_fertility       = 0.4f;             ///< 4B  (116..120): Available mineral nutrient index [0..1]
    uint16_t dominant_surface_lithology   = LITHOLOGY_GRANITE;///< 2B  (120..122): Exposed or shallowest bedrock lithology ID
    uint8_t  active_layer_count           = 3u;               ///< 1B  (122): Number of populated bedrock/sedimentary strata (1..6)
    uint8_t  dominant_soil_horizon        = 1u;               ///< 1B  (123): SoilHorizonType
    float    cumulative_erosion_delta_m   = 0.0f;             ///< 4B  (124..128): Net height change from erosion (-) / deposition (+) -> 128B total
};
static_assert(sizeof(StratigraphicColumnPOD) == 128, "StratigraphicColumnPOD must be 128 bytes.");

/**
 * `TectonicPlatePOD` (64 Bytes, alignas(64))
 * Lithospheric plate descriptor for Voronoi/centroid tectonic velocity and buoyancy synthesis.
 */
struct alignas(64) TectonicPlatePOD {
    glm::vec2 seed_position_uv       = glm::vec2(0.5f, 0.5f);      ///< 8B  (0..8): Normalized domain position [0..1]^2
    glm::vec2 drift_velocity_m_kyr   = glm::vec2(0.02f, 0.0f);     ///< 8B  (8..16): Plate horizontal velocity vector
    float     angular_spin_rad_myr   = 0.0f;                       ///< 4B  (16..20): Euler rotation rate
    float     crustal_density_kg_m3  = 2700.0f;                    ///< 4B  (20..24): 2700 continental, 3000 oceanic
    float     base_thickness_m       = 35000.0f;                   ///< 4B  (24..28): 35km continental, 7km oceanic
    float     buoyancy_offset_m      = 400.0f;                     ///< 4B  (28..32): Base isostatic freeboard
    float     thermal_age_myr        = 120.0f;                     ///< 4B  (32..36)
    uint16_t  plate_id               = 0u;                         ///< 2B  (36..38)
    uint16_t  primary_lithology_id   = LITHOLOGY_GRANITE;          ///< 2B  (38..40)
    uint8_t   is_oceanic             = 0u;                         ///< 1B  (40)
    uint8_t   reserved_padding[23]   = {0};                        ///< 23B (41..64) -> 64B total
};
static_assert(sizeof(TectonicPlatePOD) == 64, "TectonicPlatePOD must be 64 bytes.");

/**
 * `CrustalTectonicCellPOD` (32 Bytes, alignas(16))
 * Per-cell tectonic stress, boundary regime, geothermal heat flux, and uplift rate.
 */
struct alignas(16) CrustalTectonicCellPOD {
    uint16_t plate_id                   = 0u;                         ///< 2B (0..2)
    uint8_t  boundary_type              = TECTONIC_BOUNDARY_INTERIOR; ///< 1B (2): TectonicBoundaryType
    uint8_t  volcanic_activity_mask     = 0u;                         ///< 1B (3): Bit 0 = shield, Bit 1 = stratovolcano, Bit 2 = hydrothermal
    float    crust_thickness_m          = 35000.0f;                   ///< 4B (4..8)
    float    compressive_stress_mpa     = 0.0f;                       ///< 4B (8..12): Positive = compression/orogeny, Negative = tension/rift
    float    shear_stress_mpa           = 0.0f;                       ///< 4B (12..16): Transform strike-slip shear stress
    float    uplift_rate_m_kyr          = 0.0f;                       ///< 4B (16..20): Vertical tectonic uplift (+) / subsidence (-)
    float    geothermal_heat_flux_w_m2  = 0.065f;                     ///< 4B (20..24)
    float    isostatic_equilibrium_m    = 0.0f;                       ///< 4B (24..28)
    float    fault_slip_accumulated_m   = 0.0f;                       ///< 4B (28..32) -> 32B total
};
static_assert(sizeof(CrustalTectonicCellPOD) == 32, "CrustalTectonicCellPOD must be 32 bytes.");

/**
 * `HydrologyCellStatePOD` (64 Bytes, alignas(64))
 * Per-cell surface water, 4-neighbor Virtual Pipe fluxes, suspended/bedload sediment,
 * Darcy groundwater water-table elevation, karst cavern void, and Strahler drainage metadata.
 */
struct alignas(64) HydrologyCellStatePOD {
    glm::vec4 outflow_flux_n_e_s_w   = glm::vec4(0.0f);            ///< 16B (0..16): Virtual pipe fluxes (+Z=S, -Z=N, +X=E, -X=W) in m^3/s
    glm::vec2 velocity_uv            = glm::vec2(0.0f);            ///< 8B  (16..24): Resolved 2D horizontal water velocity (m/s)
    float     surface_water_depth_m  = 0.0f;                       ///< 4B  (24..28): Standing/flowing water column depth d >= 0
    float     suspended_sediment_m   = 0.0f;                       ///< 4B  (28..32): Suspended sediment concentration (equivalent m)
    float     bedload_sediment_m     = 0.0f;                       ///< 4B  (32..36): Coarse bedload traction sediment (m)
    float     dissolved_mineral_load = 0.0f;                       ///< 4B  (36..40): Dissolved carbonate/solute load
    float     discharge_m3_s         = 0.0f;                       ///< 4B  (40..44): Volumetric discharge Q (m^3/s)
    float     water_table_elevation_m= 0.0f;                       ///< 4B  (44..48): Phreatic groundwater table elevation (m)
    float     karst_cavern_void_m    = 0.0f;                       ///< 4B  (48..52): Subterranean dissolved cavity height (m)
    uint32_t  basin_id               = 0u;                         ///< 4B  (52..56): Watershed drainage basin ID
    uint8_t   stream_order_strahler  = 1u;                         ///< 1B  (56): Strahler stream order (1..8)
    uint8_t   drainage_class         = BASIN_EXORHEIC_OCEANIC;     ///< 1B  (57): DrainageBasinClass
    uint8_t   receiver_dir_d8        = 0u;                         ///< 1B  (58): Steepest-descent D8 neighbor index (0..7, 255 = sink)
    uint8_t   is_inundated_mask      = 0u;                         ///< 1B  (59): Bit 0 = ocean, Bit 1 = lake, Bit 2 = flood
    float     slope_magnitude        = 0.0f;                       ///< 4B  (60..64): Local terrain gradient |nabla H| -> 64B total
};
static_assert(sizeof(HydrologyCellStatePOD) == 64, "HydrologyCellStatePOD must be 64 bytes.");

} // namespace ideam::core
