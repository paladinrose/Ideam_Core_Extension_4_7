#pragma once

/**
 * cymatics_dod_common.h
 * DOD buffer slots and stage bitmasks for `IdeamCymatics` (`src/core/tasks/domains/cymatics/`).
 */

#include "../../../cymatics/cymatics_common.h"

#include <cstdint>

namespace ideam::core {

enum CymaticsStageFlags : uint32_t {
    CYM_STAGE_WAVE_PROPAGATE  = 1u << 0, // CYM1
    CYM_STAGE_CHLADNI_MODAL   = 1u << 1, // CYM2
    CYM_STAGE_NODAL_DRIFT     = 1u << 2, // CYM3
    CYM_STAGE_MESH_RESONANCE  = 1u << 3, // CYM4
    CYM_STAGE_SECTOR_REDUCE   = 1u << 4, // CYM5
    CYM_STAGE_SHADOW_ZONE     = 1u << 5, // CYM6
    CYM_STAGE_DEFAULT_MASK    = CYM_STAGE_WAVE_PROPAGATE | CYM_STAGE_CHLADNI_MODAL |
                                CYM_STAGE_SECTOR_REDUCE | CYM_STAGE_SHADOW_ZONE
};

} // namespace ideam::core
