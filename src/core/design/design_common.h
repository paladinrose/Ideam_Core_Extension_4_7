#pragma once

/**
 * design_common.h
 * Pure C++26 POD types, 3D Box-Model flags, CSS cascade layers, `SocketPod`,
 * `StructureNodePod`, `Style3DRulePod`, `ApertureCutoutPod`, and hierarchical
 * `SemanticRoleID` bitpaths for `IdeamDesign` (`src/core/design/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (`IdeamDesign` Core Substrate)
 * ============================================================================
 * 1. HIERARCHICAL `SemanticRoleID` BITPATHS (`[Category:8][Role:8][SubRole:8][Variant:8]`):
 *    - Encodes taxonomic inheritance directly in 32-bit integers so CSS-like `Style3DRulePod`
 *      selectors can match either an exact leaf role (`SEMANTIC_ROLE_SURFACE_CHAIR = 0x01040101`)
 *      or any ancestor role prefix (`SEMANTIC_ROLE_SURFACE_SEAT = 0x01040100`,
 *      `SEMANTIC_ROLE_SURFACE_SUPPORT = 0x01040000`, `SEMANTIC_ROLE_SURFACE = 0x01000000`)
 *      using zero-branch bitmasks.
 *    - Covers Architectural Surfaces, Apertures, Assemblies, Parametric Components
 *      (`Leg`, `Armrest`, `Backrest`, `CasterWheel`, `HingeMount`), Organic Botanical forms
 *      (`Trunk`, `Branch`, `FoliageCanopy`, `RootTendril`), and Geologic formations
 *      (`StrataBed`, `BoulderOutcrop`, `StalactiteSpire`, `CrystalCluster`).
 *
 * 2. 3D BOX-MODEL & SOCKET/JOINT LAYOUT (`StructureNodePod` & `SocketPod`):
 *    - Brings Godot 2D `Control` layout ergonomics (`margin`, `padding`, `SizeFlags`,
 *      `stretch_ratio`) into 3D space while pairing `SocketPod` anchors with optional
 *      `SocketPhysicsBindingMode` (`PinJoint3D`, `HingeJoint3D`, `SliderJoint3D`,
 *      `Generic6DOFJoint3D`) when physics bodies are present.
 *
 * 3. 3D CSS THEME CASCADE & DREAM-STATE MORPHING (`Style3DRulePod`):
 *    - Mirrors `GameplayStyle` and `ProceduralPattern` specificity (`!important >
 *      DesignCascadeLayer > priority > Sememe nuclear/peripheral match > SemanticRoleID depth`).
 */

#include <algorithm>
#include <bit>
#include <cstdint>
#include <glm/glm.hpp>

namespace ideam::core {

/**
 * Hierarchical 32-bit `SemanticRoleID` bitpath (`[Category:8][Role:8][SubRole:8][Variant:8]`).
 */
enum SemanticRoleCategory : uint32_t {
    SEMANTIC_ROLE_NONE                     = 0x00000000u,

    // 0x01xxxxxx — Surface & Furniture Support Family
    SEMANTIC_ROLE_SURFACE                  = 0x01000000u,
    SEMANTIC_ROLE_SURFACE_WALL             = 0x01010000u,
    SEMANTIC_ROLE_SURFACE_WALL_EXTERIOR    = 0x01010100u,
    SEMANTIC_ROLE_SURFACE_WALL_INTERIOR    = 0x01010200u,
    SEMANTIC_ROLE_SURFACE_FLOOR            = 0x01020000u,
    SEMANTIC_ROLE_SURFACE_CEILING          = 0x01030000u,
    SEMANTIC_ROLE_SURFACE_ROOF             = 0x01030100u,
    SEMANTIC_ROLE_SURFACE_SUPPORT          = 0x01040000u,
    SEMANTIC_ROLE_SURFACE_SEAT             = 0x01040100u,
    SEMANTIC_ROLE_SURFACE_CHAIR            = 0x01040101u,
    SEMANTIC_ROLE_SURFACE_STOOL            = 0x01040102u,
    SEMANTIC_ROLE_SURFACE_BENCH            = 0x01040103u,
    SEMANTIC_ROLE_SURFACE_TABLE            = 0x01040200u,
    SEMANTIC_ROLE_SURFACE_DESK             = 0x01040201u,
    SEMANTIC_ROLE_SURFACE_SHELF            = 0x01040300u,

    // 0x02xxxxxx — Aperture Family (triggers DSG3 / CSGMath2D boolean cutouts on host Wall surfaces)
    SEMANTIC_ROLE_APERTURE                 = 0x02000000u,
    SEMANTIC_ROLE_APERTURE_DOOR            = 0x02010000u,
    SEMANTIC_ROLE_APERTURE_WINDOW          = 0x02020000u,
    SEMANTIC_ROLE_APERTURE_ARCHWAY         = 0x02030000u,
    SEMANTIC_ROLE_APERTURE_STAIR_OPENING   = 0x02040000u,

    // 0x03xxxxxx — Assembly & Container Family
    SEMANTIC_ROLE_ASSEMBLY                 = 0x03000000u,
    SEMANTIC_ROLE_ASSEMBLY_ROOM            = 0x03010000u,
    SEMANTIC_ROLE_ASSEMBLY_CORRIDOR        = 0x03020000u,
    SEMANTIC_ROLE_ASSEMBLY_BUILDING        = 0x03030000u,
    SEMANTIC_ROLE_ASSEMBLY_FURNITURE       = 0x03040000u,
    SEMANTIC_ROLE_ASSEMBLY_PROP_TOOL       = 0x03050000u,
    SEMANTIC_ROLE_ASSEMBLY_ORGANIC         = 0x03060000u,
    SEMANTIC_ROLE_ASSEMBLY_GEOLOGIC        = 0x03070000u,

    // 0x04xxxxxx — Parametric Sub-Component Family (Chair/Table/Door/Tool parts)
    SEMANTIC_ROLE_COMPONENT                = 0x04000000u,
    SEMANTIC_ROLE_COMPONENT_LEG            = 0x04010000u,
    SEMANTIC_ROLE_COMPONENT_CASTER_WHEEL   = 0x04010100u,
    SEMANTIC_ROLE_COMPONENT_BACKREST       = 0x04020000u,
    SEMANTIC_ROLE_COMPONENT_ARMREST        = 0x04030000u,
    SEMANTIC_ROLE_COMPONENT_CUSHION        = 0x04040000u,
    SEMANTIC_ROLE_COMPONENT_HINGE          = 0x04050000u,
    SEMANTIC_ROLE_COMPONENT_HANDLE         = 0x04060000u,
    SEMANTIC_ROLE_COMPONENT_FRAME_RAIL     = 0x04070000u,
    SEMANTIC_ROLE_COMPONENT_FINIAL         = 0x04080000u,

    // 0x05xxxxxx — Organic / Botanical Family
    SEMANTIC_ROLE_ORGANIC                  = 0x05000000u,
    SEMANTIC_ROLE_ORGANIC_TRUNK            = 0x05010000u,
    SEMANTIC_ROLE_ORGANIC_BRANCH           = 0x05020000u,
    SEMANTIC_ROLE_ORGANIC_TWIG_THORN       = 0x05020100u,
    SEMANTIC_ROLE_ORGANIC_FOLIAGE_CANOPY   = 0x05030000u,
    SEMANTIC_ROLE_ORGANIC_ROOT             = 0x05040000u,

    // 0x06xxxxxx — Geologic / Mineral Family
    SEMANTIC_ROLE_GEOLOGIC                 = 0x06000000u,
    SEMANTIC_ROLE_GEOLOGIC_STRATA          = 0x06010000u,
    SEMANTIC_ROLE_GEOLOGIC_BOULDER         = 0x06020000u,
    SEMANTIC_ROLE_GEOLOGIC_STALACTITE      = 0x06030000u,
    SEMANTIC_ROLE_GEOLOGIC_CRYSTAL         = 0x06040000u
};

/**
 * Evaluates whether `p_candidate_role` matches `p_selector_role` along the
 * 4-byte `[Category:8][Role:8][SubRole:8][Variant:8]` taxonomic hierarchy,
 * returning the matched byte depth (`0` = no match, `1..4` = specificity depth;
 * `p_selector_role == 0` is wildcard and returns `1`).
 */
[[nodiscard]] constexpr uint8_t match_semantic_role_hierarchy(
    uint32_t p_selector_role,
    uint32_t p_candidate_role
) noexcept {
    if (p_selector_role == 0u) return 1u;
    if (p_selector_role == p_candidate_role) return 4u;
    if ((p_selector_role & 0x000000FFu) == 0u &&
        (p_selector_role & 0xFFFFFF00u) == (p_candidate_role & 0xFFFFFF00u)) {
        return 3u;
    }
    if ((p_selector_role & 0x0000FFFFu) == 0u &&
        (p_selector_role & 0xFFFF0000u) == (p_candidate_role & 0xFFFF0000u)) {
        return 2u;
    }
    if ((p_selector_role & 0x00FFFFFFu) == 0u &&
        (p_selector_role & 0xFF000000u) == (p_candidate_role & 0xFF000000u)) {
        return 1u;
    }
    return 0u;
}

enum class DesignScaleMode : uint8_t {
    UNIFORM_FIT        = 0, ///< Uniform isotropic scale to fit inside target OBB
    STRETCH_MIDDLE     = 1, ///< 3D 9-slice / 27-slice: preserves end-caps while stretching interior span
    REPEAT_TILES       = 2, ///< Repeats fundamental tile/segment along span (`IdeamTiles` bridge)
    SPLINE_LOFT_DEFORM = 3, ///< Lofts and tapers along a `Spline3D` curve (`IdeamSplines` bridge)
    PROPORTIONAL_AXIS  = 4  ///< Independent per-axis `(X, Y, Z)` bounding box stretch
};

enum class EdgeSnapPolicy : uint8_t {
    SNAP_NONE                  = 0,
    SNAP_WATERTIGHT            = 1,
    SNAP_MITER_CORNER          = 2,
    SNAP_SOCKET_ANCHOR         = 3,
    SNAP_SURFACE_NORMAL_DRAPE  = 4
};

/**
 * 3D Box-Model size flags per axis (`X`, `Y`, `Z`), analogous to Godot 2D `Control::SizeFlags`.
 */
enum DesignSizeFlags : uint8_t {
    DESIGN_SIZE_FIXED              = 0u,
    DESIGN_SIZE_FILL               = 1u << 0,
    DESIGN_SIZE_EXPAND             = 1u << 1,
    DESIGN_SIZE_SHRINK_CENTER      = 1u << 2,
    DESIGN_SIZE_SHRINK_END         = 1u << 3,
    DESIGN_SIZE_PROPORTIONAL_RATIO = 1u << 4
};

/**
 * CSS-style `@layer` cascade precedence for `Style3DRulePod` and `Theme3DResource`.
 */
enum class DesignCascadeLayer : uint8_t {
    LAYER_ARCHETYPE_DEFAULT  = 0, ///< Default component geometry & proportions
    LAYER_BLUEPRINT_ROOM     = 1, ///< Architectural room/zone baseline style
    LAYER_THEME_ERA          = 2, ///< Cultural/historical era theme (e.g. Victorian, Steampunk, Greek)
    LAYER_DAMAGE_OR_WEAR     = 3, ///< Weathering, decay, or structural damage overrides
    LAYER_DREAM_SURREALISM   = 4, ///< Dynamic dream-world / sanity morphing overrides
    LAYER_USER_CUSTOMIZATION = 5  ///< Player or designer bespoke overrides
};

/**
 * Physics joint handshake mode for `SocketPod` and `Socket3D`.
 */
enum class SocketPhysicsBindingMode : uint8_t {
    PHYSICS_BIND_NONE                  = 0, ///< Pure kinematic / Transform3D attachment
    PHYSICS_BIND_STATIC_WELD           = 1, ///< Fixed rigid weld
    PHYSICS_BIND_PIN_JOINT_3D          = 2, ///< Ball-and-socket / caster swivel (`PinJoint3D`)
    PHYSICS_BIND_HINGE_JOINT_3D        = 3, ///< Single-axis door/lid/wheel axle (`HingeJoint3D`)
    PHYSICS_BIND_SLIDER_JOINT_3D       = 4, ///< Linear drawer/piston slide (`SliderJoint3D`)
    PHYSICS_BIND_GENERIC_6DOF_JOINT_3D = 5  ///< Spring/damped 6-DoF attachment (`Generic6DOFJoint3D`)
};

/**
 * 3D Container layout algorithm selector.
 */
enum class DesignContainerLayoutMode : uint8_t {
    LAYOUT_LINEAR_BOX      = 0, ///< 1D linear stack along primary axis (`X=0, Y=1, Z=2`)
    LAYOUT_GRID_3D         = 1, ///< 3D lattice grid (`columns_x`, `rows_y`, `layers_z`)
    LAYOUT_RADIAL_RING     = 2, ///< Circular / helical arrangement around an axis (e.g. chair legs)
    LAYOUT_SPLINE_PATH     = 3, ///< Distributed along a `Spline3D` curve with parallel-transport frames
    LAYOUT_SURFACE_SCATTER = 4  ///< Draped onto a surface / Voronoi territory along surface normals
};

/**
 * `SocketPod` (80 Bytes, alignas(16))
 * 3D connection anchor with compatibility bitmask, clearance AABB, and optional
 * Godot `Joint3D` physics binding metadata.
 */
struct alignas(16) SocketPod {
    uint32_t                 socket_id              = 0;                                           ///< 4B (0..4)
    uint32_t                 owner_node_id          = 0;                                           ///< 4B (4..8)
    uint64_t                 compatibility_bitmask  = 0xFFFFFFFFFFFFFFFFULL;                       ///< 8B (8..16)
    glm::vec3                position               = glm::vec3(0.0f);                             ///< 12B (16..28)
    glm::vec3                normal                 = glm::vec3(0.0f, 1.0f, 0.0f);                 ///< 12B (28..40)
    glm::vec3                tangent                = glm::vec3(1.0f, 0.0f, 0.0f);                 ///< 12B (40..52)
    glm::vec3                clearance_half_extents = glm::vec3(0.25f);                            ///< 12B (52..64)
    uint32_t                 mated_socket_id        = 0xFFFFFFFFu;                                 ///< 4B (64..68)
    float                    joint_limit_lower      = 0.0f;                                        ///< 4B (68..72)
    float                    joint_limit_upper      = 0.0f;                                        ///< 4B (72..76)
    SocketPhysicsBindingMode physics_binding_mode   = SocketPhysicsBindingMode::PHYSICS_BIND_NONE; ///< 1B (76)
    uint8_t                  is_male_plug           = 0;                                           ///< 1B (77)
    uint8_t                  is_occupied            = 0;                                           ///< 1B (78)
    uint8_t                  joint_axis_index       = 1;                                           ///< 1B (79) -> 80B total
};
static_assert(sizeof(SocketPod) == 80, "SocketPod must be 80 bytes.");

/**
 * `StructureNodePod` (128 Bytes, alignas(64))
 * 3D Box-Model & Semantic Structure record representing a Functional or Layout Object.
 */
struct alignas(64) StructureNodePod {
    uint32_t                  node_id                   = 0;                                              ///< 4B (0..4)
    uint32_t                  parent_id                 = 0xFFFFFFFFu;                                    ///< 4B (4..8)
    uint32_t                  semantic_tag_id           = SEMANTIC_ROLE_SURFACE;                          ///< 4B (8..12)
    uint32_t                  style_slot_id             = 0;                                              ///< 4B (12..16)
    uint32_t                  nuclear_seme_mask_lo32    = 0;                                              ///< 4B (16..20)
    uint32_t                  peripheral_seme_mask_lo32 = 0;                                              ///< 4B (20..24)
    DesignScaleMode           scale_mode                = DesignScaleMode::UNIFORM_FIT;                   ///< 1B (24)
    EdgeSnapPolicy            edge_snap_policy          = EdgeSnapPolicy::SNAP_WATERTIGHT;                ///< 1B (25)
    uint8_t                   size_flags_x              = DESIGN_SIZE_FILL;                               ///< 1B (26)
    uint8_t                   size_flags_y              = DESIGN_SIZE_FILL;                               ///< 1B (27)
    uint8_t                   size_flags_z              = DESIGN_SIZE_FILL;                               ///< 1B (28)
    DesignCascadeLayer        cascade_layer             = DesignCascadeLayer::LAYER_ARCHETYPE_DEFAULT;    ///< 1B (29)
    DesignContainerLayoutMode container_mode            = DesignContainerLayoutMode::LAYOUT_LINEAR_BOX;   ///< 1B (30)
    uint8_t                   is_aperture_cutout_dirty  = 0;                                              ///< 1B (31)
    float                     stretch_ratio             = 1.0f;                                           ///< 4B (32..36)
    glm::vec3                 rest_obb_center           = glm::vec3(0.0f);                                ///< 12B (36..48)
    glm::vec3                 rest_obb_extents          = glm::vec3(0.5f);                                ///< 12B (48..60)
    float                     margin_uniform            = 0.0f;                                           ///< 4B (60..64)
    glm::mat4                 fitted_transform          = glm::mat4(1.0f);                                ///< 64B (64..128)
};
static_assert(sizeof(StructureNodePod) == 128, "StructureNodePod must be 128 bytes.");

/**
 * `Style3DRulePod` (64 Bytes, alignas(64))
 * Flat DOD representation of a cascading 3D style rule (`Style3DResource` / `Theme3DResource`).
 */
struct alignas(64) Style3DRulePod {
    uint32_t           rule_id                       = 0;                                           ///< 4B (0..4)
    uint32_t           semantic_role_selector        = SEMANTIC_ROLE_SURFACE;                       ///< 4B (4..8)
    uint32_t           parent_role_selector          = 0u;                                          ///< 4B (8..12)
    uint32_t           required_nuclear_seme_mask    = 0u;                                          ///< 4B (12..16)
    uint32_t           required_peripheral_seme_mask = 0u;                                          ///< 4B (16..20)
    uint32_t           mesh_asset_id                 = 0u;                                          ///< 4B (20..24)
    uint32_t           material_asset_id             = 0u;                                          ///< 4B (24..28)
    int16_t            priority                      = 0;                                           ///< 2B (28..30)
    uint16_t           specificity                   = 0;                                           ///< 2B (30..32)
    glm::vec3          target_extents_scale          = glm::vec3(1.0f);                             ///< 12B (32..44)
    glm::vec3          margin_override               = glm::vec3(0.0f);                             ///< 12B (44..56)
    float              tile_repeat_density           = 1.0f;                                        ///< 4B (56..60)
    DesignCascadeLayer layer                         = DesignCascadeLayer::LAYER_ARCHETYPE_DEFAULT; ///< 1B (60)
    DesignScaleMode    scale_mode                    = DesignScaleMode::UNIFORM_FIT;                ///< 1B (61)
    EdgeSnapPolicy     edge_snap_policy              = EdgeSnapPolicy::SNAP_WATERTIGHT;             ///< 1B (62)
    uint8_t            is_important                  = 0;                                           ///< 1B (63) -> 64B total
};
static_assert(sizeof(Style3DRulePod) == 64, "Style3DRulePod must be 64 bytes.");

/**
 * `ApertureCutoutPod` (64 Bytes, alignas(64))
 * Describes a boolean aperture subtraction (`SEMANTIC_ROLE_APERTURE_*`) on a host
 * wall surface (`SEMANTIC_ROLE_SURFACE_WALL*`) evaluated by `DSG3`.
 */
struct alignas(64) ApertureCutoutPod {
    uint32_t  aperture_node_id        = 0xFFFFFFFFu;                  ///< 4B (0..4)
    uint32_t  host_wall_node_id       = 0xFFFFFFFFu;                  ///< 4B (4..8)
    uint32_t  aperture_semantic_role  = SEMANTIC_ROLE_APERTURE_DOOR;  ///< 4B (8..12)
    uint32_t  csg_operation           = 2u;                           ///< 4B (12..16): 2 = CSGOperation2D::DIFFERENCE
    glm::vec3 local_center_on_wall    = glm::vec3(0.0f);              ///< 12B (16..28)
    float     wall_thickness          = 0.25f;                        ///< 4B (28..32)
    glm::vec3 cutout_half_extents     = glm::vec3(0.45f, 1.05f, 0.3f);///< 12B (32..44)
    float     arch_crown_radius       = 0.0f;                         ///< 4B (44..48)
    uint32_t  resulting_segment_count = 0u;                           ///< 4B (48..52)
    uint8_t   is_applied              = 0u;                           ///< 1B (52)
    uint8_t   spawns_hinge_socket     = 1u;                           ///< 1B (53)
    uint8_t   reserved[10]            = {0};                          ///< 10B (54..64) -> 64B total
};
static_assert(sizeof(ApertureCutoutPod) == 64, "ApertureCutoutPod must be 64 bytes.");

} // namespace ideam::core
