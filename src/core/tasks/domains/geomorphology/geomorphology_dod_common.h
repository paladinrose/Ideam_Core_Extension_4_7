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
    SEED_PROFILE_POD         = 0,
    LITHOLOGY_CATALOG_POD    = 1,
    TECTONIC_PLATE_POD       = 2,
    TECTONIC_CELL_POD        = 3,
    STRATIGRAPHIC_COLUMN_POD = 4,
    HYDROLOGY_CELL_POD       = 5,
    SURFACE_ELEVATION_FLOAT  = 6,
    HAZARD_RESOURCE_MASK_U32 = 7,
    COUNT                    = 8
};

enum class GeomorphologyStageFlags : uint32_t {
    NONE                  = 0u,
    TECTONIC_UPLIFT       = 1u << 0,
    SHALLOW_WATER_HYDRO   = 1u << 1,
    FLUVIAL_TALUS_EROSION = 1u << 2,
    KARST_PEDOGENESIS     = 1u << 3,
    DRAINAGE_STRAHLER     = 1u << 4,
    HAZARD_RESOURCE_QUERY = 1u << 5,
    ALL_STAGES            = 0x3Fu
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
