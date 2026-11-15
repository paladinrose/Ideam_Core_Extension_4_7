#pragma once

/**
 * geomorphology_dod_common.h
 * Buffer slots, capacities, manifests, and pipeline stage masks for `GeomorphologyDOD`
 * (`src/core/tasks/domains/geomorphology/`).
 */

#include "../../../geomorphology/geomorphology_types.h"
#include "../../../geomorphology/geomorphology_core.h"
#include "../../../memory/memory_manager_dod.h"
namespace ideam::core { class TaskGraphDOD; }
#include <array>
#include <cstdint>

namespace ideam::core {

enum class GeomorphologyBufferSlot : uint32_t {
    SEED_PROFILE_POD         = 0, ///< `GeomorphologySeedProfilePOD` (64B)
    LITHOLOGY_CATALOG_POD    = 1, ///< `LithologyArchetypePOD` (64B x 8+)
    TECTONIC_PLATE_POD       = 2, ///< `TectonicPlatePOD` (64B x N_plates)
    TECTONIC_CELL_POD        = 3, ///< `CrustalTectonicCellPOD` (32B per grid cell)
    STRATIGRAPHIC_COLUMN_POD = 4, ///< `StratigraphicColumnPOD` (128B per grid cell)
    HYDROLOGY_CELL_POD       = 5, ///< `HydrologyCellStatePOD` (64B per grid cell)
    SURFACE_ELEVATION_FLOAT  = 6, ///< `float` (4B per grid cell)
    HAZARD_RESOURCE_MASK_U32 = 7, ///< `uint32_t` (4B per grid cell) hazard/mineral vein flags
    COUNT                    = 8
};

enum GeomorphologyStageFlags : uint32_t {
    GEOMORPHOLOGY_STAGE_NONE                      = 0u,
    GEOMORPHOLOGY_STAGE_TECTONIC_OROGENY          = 1u << 0, ///< `GEO_T1`: `TectonicUpliftAndOrogenyTransformLogic`
    GEOMORPHOLOGY_STAGE_SHALLOW_WATER_HYDROLOGY   = 1u << 1, ///< `GEO_T2`: `ShallowWaterPipeHydrologyTransformLogic`
    GEOMORPHOLOGY_STAGE_STREAM_POWER_EROSION      = 1u << 2, ///< `GEO_T3`: `StreamPowerAndTalusErosionTransformLogic`
    GEOMORPHOLOGY_STAGE_KARST_AND_PEDOGENESIS     = 1u << 3, ///< `GEO_T4`: `GroundwaterKarstAndPedogenesisTransformLogic`
    GEOMORPHOLOGY_STAGE_DRAINAGE_STRAHLER_METADATA= 1u << 4, ///< `GEO_M1`: `DrainageBasinAndStrahlerPartitionMetadataLogic`
    GEOMORPHOLOGY_STAGE_HAZARD_VEIN_QUERY         = 1u << 5, ///< `GEO_Q1`: `GeomorphicHazardAndResourceVeinQueryLogic`
    GEOMORPHOLOGY_STAGE_DEFAULT_PIPELINE_MASK     =
        GEOMORPHOLOGY_STAGE_TECTONIC_OROGENY |
        GEOMORPHOLOGY_STAGE_SHALLOW_WATER_HYDROLOGY |
        GEOMORPHOLOGY_STAGE_STREAM_POWER_EROSION |
        GEOMORPHOLOGY_STAGE_KARST_AND_PEDOGENESIS |
        GEOMORPHOLOGY_STAGE_DRAINAGE_STRAHLER_METADATA
};

struct GeomorphologyBufferCapacities {
    uint32_t grid_width        = 64u;
    uint32_t grid_height       = 64u;
    size_t   max_plates        = 16u;
    size_t   max_lithologies   = 16u;

    [[nodiscard]] constexpr size_t cell_count() const noexcept {
        return static_cast<size_t>(grid_width) * static_cast<size_t>(grid_height);
    }
};

struct GeomorphologyBufferManifest {
    std::array<uint32_t, static_cast<size_t>(GeomorphologyBufferSlot::COUNT)> buffer_ids = {
        INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID,
        INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID
    };

    [[nodiscard]] constexpr uint32_t get(GeomorphologyBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    constexpr void set(GeomorphologyBufferSlot p_slot, uint32_t p_id) noexcept {
        buffer_ids[static_cast<size_t>(p_slot)] = p_id;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return get(GeomorphologyBufferSlot::LITHOLOGY_CATALOG_POD) != INVALID_ID &&
               get(GeomorphologyBufferSlot::TECTONIC_CELL_POD) != INVALID_ID &&
               get(GeomorphologyBufferSlot::STRATIGRAPHIC_COLUMN_POD) != INVALID_ID &&
               get(GeomorphologyBufferSlot::HYDROLOGY_CELL_POD) != INVALID_ID &&
               get(GeomorphologyBufferSlot::SURFACE_ELEVATION_FLOAT) != INVALID_ID;
    }
};

} // namespace ideam::core
