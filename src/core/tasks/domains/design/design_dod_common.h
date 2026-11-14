#pragma once

/**
 * design_dod_common.h
 * Buffer slots, capacities, manifests, and pipeline stage masks for `DesignDOD`
 * (`src/core/tasks/domains/design/`).
 */

#include "../../../design/design_common.h"
#include "../../../design/structure_layout_solver.h"
#include "../../../design/parametric_assembly_core.h"
#include "../../../design/blueprint_core.h"
#include "../../../memory/memory_manager_dod.h"
namespace ideam::core { class TaskGraphDOD; }
#include <array>
#include <cstdint>

namespace ideam::core {

enum class DesignBufferSlot : uint32_t {
    STRUCTURE_NODE_POD   = 0, ///< `StructureNodePod` (128B)
    SOCKET_POD           = 1, ///< `SocketPod` (80B)
    STYLE_3D_RULE_POD    = 2, ///< `Style3DRulePod` (64B)
    APERTURE_CUTOUT_POD  = 3, ///< `ApertureCutoutPod` (64B)
    NODE_STATUS_MASK     = 4, ///< `uint32_t` status / role category mask
    COUNT                = 5
};

enum DesignStageFlags : uint32_t {
    DESIGN_STAGE_NONE                        = 0u,
    DESIGN_STAGE_PARAMETRIC_ASSEMBLY_MORPH   = 1u << 0, ///< `DSG4`: `ParametricAssemblyMorphMetadataLogic`
    DESIGN_STAGE_STYLE_3D_THEME_RESKIN       = 1u << 1, ///< `DSG2`: `Style3DThemeReskinTransformLogic`
    DESIGN_STAGE_STRUCTURE_SOCKET_SOLVE      = 1u << 2, ///< `DSG1`: `StructureSocketConstraintSolveTransformLogic`
    DESIGN_STAGE_APERTURE_HOST_CUTOUT_BRIDGE = 1u << 3, ///< `DSG3`: `ApertureHostCutoutBridgeTransformLogic`
    DESIGN_STAGE_DEFAULT_PIPELINE_MASK       =
        DESIGN_STAGE_PARAMETRIC_ASSEMBLY_MORPH |
        DESIGN_STAGE_STYLE_3D_THEME_RESKIN |
        DESIGN_STAGE_STRUCTURE_SOCKET_SOLVE |
        DESIGN_STAGE_APERTURE_HOST_CUTOUT_BRIDGE
};

struct DesignBufferCapacities {
    size_t max_structure_nodes  = 256;
    size_t max_sockets          = 256;
    size_t max_style_rules      = 128;
    size_t max_aperture_cutouts = 64;
};

struct DesignBufferManifest {
    std::array<uint32_t, static_cast<size_t>(DesignBufferSlot::COUNT)> buffer_ids = {
        INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID
    };

    [[nodiscard]] constexpr uint32_t get(DesignBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    constexpr void set(DesignBufferSlot p_slot, uint32_t p_id) noexcept {
        buffer_ids[static_cast<size_t>(p_slot)] = p_id;
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return get(DesignBufferSlot::STRUCTURE_NODE_POD) != INVALID_ID &&
               get(DesignBufferSlot::SOCKET_POD) != INVALID_ID &&
               get(DesignBufferSlot::STYLE_3D_RULE_POD) != INVALID_ID &&
               get(DesignBufferSlot::APERTURE_CUTOUT_POD) != INVALID_ID;
    }
};

} // namespace ideam::core
