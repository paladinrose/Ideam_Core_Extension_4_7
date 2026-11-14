#pragma once

/**
 * parametric_assembly_core.h
 * Pure C++26 Parametric Object Synthesis Engine (`src/core/design/parametric_assembly_core.h`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES ("Add a Chair to This Room")
 * ============================================================================
 * Synthesizes complete, runtime-mutable 3D assemblies (`StructureNodePod` + `SocketPod`)
 * from declarative `AssemblyPartTemplatePOD` and `AssemblyParameterSlotPOD` recipes:
 *   - Supports Furniture (`Chair`, `Table`, `Shelf`), Architectural Apertures (`Door` with
 *     `HingeJoint3D` socket), Organic/Botanical forms (`Tree` with `Trunk -> Branch -> Foliage`),
 *     and Geologic formations (`BoulderOutcrop` / `CrystalCluster`).
 *   - Supports live parameter tuning at both Edit-Time and Runtime:
 *       * `leg_count` (`3..8`, arranged via `LAYOUT_RADIAL_RING` or `LAYOUT_GRID_3D`)
 *       * `has_wheels` (`0` or `1`, spawning `SEMANTIC_ROLE_COMPONENT_CASTER_WHEEL` children
 *         mated to each `Leg` via `PHYSICS_BIND_PIN_JOINT_3D` swivel sockets!)
 *       * `has_armrests` (`0` or `1`, spawning left/right `SEMANTIC_ROLE_COMPONENT_ARMREST`)
 *       * `seat_height`, `seat_width`, `backrest_height`, `backrest_tilt`
 *       * `dream_morph_factor` (`[0..1]`, warping peripheral proportions while locking the
 *         nuclear `Sememe` and `SemanticRoleID` identity!)
 */

#include "structure_layout_solver.h"
#include "../procedural_generation/procgen_common.h"
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ideam::core {

enum class AssemblyParamTargetOp : uint8_t {
    PARAM_OP_PART_COUNT      = 0, ///< Controls integer cardinality of target part role (e.g. `leg_count = 4`)
    PARAM_OP_SCALE_X         = 1, ///< Scales `rest_obb_extents.x` of target part role
    PARAM_OP_SCALE_Y         = 2, ///< Scales `rest_obb_extents.y` of target part role
    PARAM_OP_SCALE_Z         = 3, ///< Scales `rest_obb_extents.z` of target part role
    PARAM_OP_OFFSET_Y        = 4, ///< Offsets `rest_obb_center.y` of target part role (e.g. `seat_height`)
    PARAM_OP_RADIAL_RADIUS   = 5, ///< Adjusts radial placement radius of target part role
    PARAM_OP_TOGGLE_PART     = 6  ///< Enables (`>= 0.5`) or disables (`< 0.5`) optional part (e.g. `has_wheels`)
};

/**
 * `AssemblyParameterSlotPOD` (32 Bytes, alignas(16))
 * Exposed parametric control on a `ParametricAssemblyRecipe`.
 */
struct alignas(16) AssemblyParameterSlotPOD {
    uint32_t              param_name_hash     = 0;                                         ///< 4B: FNV-1a hash of parameter name
    uint32_t              target_part_role_id = SEMANTIC_ROLE_NONE;                        ///< 4B: Affected `SemanticRoleID`
    float                 current_value       = 1.0f;                                      ///< 4B
    float                 min_value           = 0.0f;                                      ///< 4B
    float                 max_value           = 10.0f;                                     ///< 4B
    float                 step_value          = 0.1f;                                      ///< 4B
    float                 dream_morph_weight  = 0.0f;                                      ///< 4B: Amplitude under dream/sanity morph
    AssemblyParamTargetOp operation           = AssemblyParamTargetOp::PARAM_OP_SCALE_Y;   ///< 1B
    uint8_t               reserved[3]         = {0, 0, 0};                                 ///< 3B -> 32B total
};
static_assert(sizeof(AssemblyParameterSlotPOD) == 32, "AssemblyParameterSlotPOD must be 32 bytes.");

/**
 * `AssemblyPartTemplatePOD` (64 Bytes, alignas(64))
 * Defines one structural or functional component slot in a parametric assembly recipe.
 */
struct alignas(64) AssemblyPartTemplatePOD {
    uint32_t                  part_role_id          = SEMANTIC_ROLE_SURFACE;                        ///< 4B (0..4)
    uint32_t                  parent_part_role_id   = SEMANTIC_ROLE_NONE;                           ///< 4B (4..8)
    uint32_t                  required_sibling_role = SEMANTIC_ROLE_NONE;                           ///< 4B (8..12)
    uint32_t                  nuclear_seme_mask     = 0u;                                           ///< 4B (12..16)
    uint32_t                  peripheral_seme_mask  = 0u;                                           ///< 4B (16..20)
    uint16_t                  min_count             = 1u;                                           ///< 2B (20..22)
    uint16_t                  max_count             = 1u;                                           ///< 2B (22..24)
    uint16_t                  default_count         = 1u;                                           ///< 2B (24..26)
    DesignContainerLayoutMode container_mode        = DesignContainerLayoutMode::LAYOUT_LINEAR_BOX; ///< 1B (26)
    SocketPhysicsBindingMode  socket_physics_mode   = SocketPhysicsBindingMode::PHYSICS_BIND_NONE;  ///< 1B (27)
    DesignScaleMode           scale_mode            = DesignScaleMode::UNIFORM_FIT;                 ///< 1B (28)
    EdgeSnapPolicy            edge_snap_policy      = EdgeSnapPolicy::SNAP_WATERTIGHT;              ///< 1B (29)
    uint8_t                   size_flags_y          = DESIGN_SIZE_FILL;                             ///< 1B (30)
    uint8_t                   spawns_socket_pair    = 1u;                                           ///< 1B (31)
    glm::vec3                 base_center_offset    = glm::vec3(0.0f);                              ///< 12B (32..44)
    glm::vec3                 base_half_extents     = glm::vec3(0.25f);                             ///< 12B (44..56)
    float                     radial_ring_radius    = 0.0f;                                         ///< 4B (56..60)
    uint32_t                  style_slot_id         = 0u;                                           ///< 4B (60..64) -> 64B total
};
static_assert(sizeof(AssemblyPartTemplatePOD) == 64, "AssemblyPartTemplatePOD must be 64 bytes.");

class ParametricAssemblyRecipe {
private:
    std::string archetype_name_             = "Chair";
    uint32_t root_semantic_role_id_         = SEMANTIC_ROLE_SURFACE_CHAIR;
    uint32_t root_nuclear_seme_mask_        = 0x0Fu;
    uint32_t root_peripheral_seme_mask_     = 0xF0u;
    std::vector<AssemblyPartTemplatePOD> part_templates_;
    std::vector<AssemblyParameterSlotPOD> parameters_;
    std::vector<std::string> parameter_names_;

public:
    ParametricAssemblyRecipe() = default;
    ~ParametricAssemblyRecipe() = default;

    void clear() noexcept;

    void set_archetype_name(std::string_view p_name) { archetype_name_ = std::string(p_name); }
    [[nodiscard]] std::string_view archetype_name() const noexcept { return archetype_name_; }

    void set_root_semantic_role_id(uint32_t p_role) noexcept { root_semantic_role_id_ = p_role; }
    [[nodiscard]] uint32_t root_semantic_role_id() const noexcept { return root_semantic_role_id_; }

    void set_root_seme_masks(uint32_t p_nuclear, uint32_t p_peripheral) noexcept {
        root_nuclear_seme_mask_    = p_nuclear;
        root_peripheral_seme_mask_ = p_peripheral;
    }
    [[nodiscard]] uint32_t root_nuclear_seme_mask() const noexcept { return root_nuclear_seme_mask_; }
    [[nodiscard]] uint32_t root_peripheral_seme_mask() const noexcept { return root_peripheral_seme_mask_; }

    uint32_t add_part_template(const AssemblyPartTemplatePOD &p_part);
    uint32_t add_parameter(std::string_view p_name, const AssemblyParameterSlotPOD &p_param);

    bool set_parameter_value(std::string_view p_name, float p_value) noexcept;
    [[nodiscard]] float get_parameter_value(std::string_view p_name, float p_default = 0.0f) const noexcept;

    [[nodiscard]] const std::vector<AssemblyPartTemplatePOD> &part_templates() const noexcept { return part_templates_; }
    [[nodiscard]] const std::vector<AssemblyParameterSlotPOD> &parameters() const noexcept { return parameters_; }
    [[nodiscard]] const std::vector<std::string> &parameter_names() const noexcept { return parameter_names_; }

    /**
     * Synthesizes `StructureNodePod`s and `SocketPod`s into `r_solver` from the current
     * parameter values and optional `p_dream_morph_factor` (`[0..1]`), then runs
     * `r_solver.solve_layout_constraints()`.
     */
    void synthesize_to_layout_solver(
        StructureLayoutSolver &r_solver,
        float p_dream_morph_factor = 0.0f,
        uint64_t p_seed = 1337ULL
    ) const;

    // --- Canonical Built-In Archetype Recipes ---
    [[nodiscard]] static ParametricAssemblyRecipe create_chair_recipe();
    [[nodiscard]] static ParametricAssemblyRecipe create_table_recipe();
    [[nodiscard]] static ParametricAssemblyRecipe create_organic_tree_recipe();
    [[nodiscard]] static ParametricAssemblyRecipe create_geologic_outcrop_recipe();
};

} // namespace ideam::core
