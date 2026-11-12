#pragma once

/**
 * design_common.h
 * Pure C++26 POD types and hierarchical `SemanticRoleID` bitpaths for `IdeamDesign`
 * (`src/core/design/`).
 *
 * Implements Section 8 DOD Architecture (absorbing Procedural Structures):
 *   - Composable DOD records tagged with a hierarchical `SemanticRoleID` bitpath
 *     (e.g., `Surface -> Support -> Seat -> Chair`, `Surface -> Wall/Floor/Ceiling`,
 *     `Aperture -> Door/Window`, `Assembly`).
 *   - `StructureNodePod` (`SOA` layout fields: `semantic_tag_id`, `parent_id`, `rest_obb`,
 *     `fitted_transform`, `style_slot_id`).
 *   - `SocketPod` (`FLAT` socket table: `position`, `normal`, `tangent`, `clearance_aabb`,
 *     `compatibility_bitmask`).
 */

#include <cstdint>
#include <glm/glm.hpp>

namespace ideam::core {

/**
 * Hierarchical 32-bit `SemanticRoleID` bitpath (`[Category:8][Role:8][SubRole:8][Variant:8]`).
 */
enum SemanticRoleCategory : uint32_t {
    SEMANTIC_ROLE_NONE              = 0x00000000u,
    // Surface family (0x01xxxxxx)
    SEMANTIC_ROLE_SURFACE           = 0x01000000u,
    SEMANTIC_ROLE_SURFACE_WALL      = 0x01010000u,
    SEMANTIC_ROLE_SURFACE_FLOOR     = 0x01020000u,
    SEMANTIC_ROLE_SURFACE_CEILING   = 0x01030000u,
    SEMANTIC_ROLE_SURFACE_SUPPORT   = 0x01040000u,
    SEMANTIC_ROLE_SURFACE_SEAT      = 0x01040100u,
    SEMANTIC_ROLE_SURFACE_CHAIR     = 0x01040101u,
    SEMANTIC_ROLE_SURFACE_TABLE     = 0x01040200u,
    // Aperture family (0x02xxxxxx) — triggers M29 / CSGMath2D boolean cutouts on host Wall surfaces
    SEMANTIC_ROLE_APERTURE          = 0x02000000u,
    SEMANTIC_ROLE_APERTURE_DOOR     = 0x02010000u,
    SEMANTIC_ROLE_APERTURE_WINDOW   = 0x02020000u,
    SEMANTIC_ROLE_APERTURE_ARCHWAY  = 0x02030000u,
    // Assembly / Container family (0x03xxxxxx)
    SEMANTIC_ROLE_ASSEMBLY          = 0x03000000u,
    SEMANTIC_ROLE_ASSEMBLY_ROOM     = 0x03010000u,
    SEMANTIC_ROLE_ASSEMBLY_CORRIDOR = 0x03020000u
};

enum class DesignScaleMode : uint8_t {
    UNIFORM_FIT    = 0,
    STRETCH_MIDDLE = 1,
    REPEAT_TILES   = 2
};

enum class EdgeSnapPolicy : uint8_t {
    SNAP_NONE          = 0,
    SNAP_WATERTIGHT    = 1,
    SNAP_MITER_CORNER  = 2,
    SNAP_SOCKET_ANCHOR = 3
};

/**
 * `FLAT` Socket Table record (`SocketPod`).
 */
struct SocketPod {
    uint32_t  socket_id             = 0;
    uint32_t  owner_node_id         = 0;
    uint64_t  compatibility_bitmask = 0xFFFFFFFFFFFFFFFFULL;
    glm::vec3 position              = glm::vec3(0.0f);
    glm::vec3 normal                = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 tangent               = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 clearance_half_extents= glm::vec3(0.25f);
};

/**
 * `StructureNodePod` (`StructureNodeSoA` element record).
 */
struct StructureNodePod {
    uint32_t        node_id          = 0;
    uint32_t        parent_id        = 0xFFFFFFFFu;
    uint32_t        semantic_tag_id  = SEMANTIC_ROLE_SURFACE;
    uint32_t        style_slot_id    = 0;
    DesignScaleMode scale_mode       = DesignScaleMode::UNIFORM_FIT;
    EdgeSnapPolicy  edge_snap_policy = EdgeSnapPolicy::SNAP_WATERTIGHT;
    glm::vec3       rest_obb_center  = glm::vec3(0.0f);
    glm::vec3       rest_obb_extents = glm::vec3(0.5f);
    glm::mat4       fitted_transform = glm::mat4(1.0f);
};

} // namespace ideam::core
