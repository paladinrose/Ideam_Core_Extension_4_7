#pragma once

/**
 * quantum_darwinism_common.h
 * Pure C++26 POD types for `IdeamQuantumDarwinism` (`src/core/quantum_darwinism/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (QUANTUM DARWINISM FOR WFC & PATTERNS)
 * ============================================================================
 * 1. THEORETICAL GROUNDING (ZUREK'S QUANTUM DARWINISM):
 *    - In quantum mechanics (Wojciech Zurek, "Quantum Darwinism", Rev. Mod. Phys. 2003;
 *      Phys. Rev. A 2006/2017), interaction with the environment E causes *einselection*
 *      (environment-induced superselection). Fragile superpositions decohere, while
 *      **Pointer States** survive because they imprint redundant copies (R_delta) of their
 *      state across many independent fragments of the environment.
 *
 * 2. APPLICATION TO WAVE FUNCTION COLLAPSE & PROCEDURAL PATTERNS:
 *    - Forward solvers like WFC generate most of the plan cleanly, but occasionally hit
 *      unavoidable local rule conflicts (contradictions where `domain.empty()` or where
 *      sibling/child cardinality rules such as `TowerLanding requires TowerStairs` fail).
 *    - Instead of chronological backtracking (which thrashes unrelated work) or blind block
 *      erosion (which destroys key landmarks), Quantum Darwinism:
 *        a) Draws a bounded **Environment Horizon Box (E_box)** around the problematic slot
 *           in the `ProceduralEnvironment` (both spatial neighbors and scene-hierarchy relatives).
 *        b) Evaluates rule scores and mutual-information redundancy (R_delta) to establish
 *           **Pointer States** from the "most settled" cells in E_box.
 *        c) Traces paths from each Pointer State toward the problematic cell and consults
 *           the **CSS `GenerationRule` Cascade (`PatternCascadeLayer` & specificity)** to
 *           identify which lower-priority/leaf properties on intermediate cells can be
 *           altered without causing those cells to lose structural coherence.
 *        d) Propagates properties inward from the Pointer States to the problematic cell,
 *           clearing the insoluble conflict while reinforcing macro-coherence across the
 *           generated environment.
 */

#include "../procedural_generation/procedural_environment.h"
#include "../wave_function_collapse/wfc_common.h"
#include <cstdint>

namespace ideam::core {

enum class EinselectionStatus : uint8_t {
    IDLE                   = 0,
    HORIZON_ISOLATED       = 1,
    POINTERS_ESTABLISHED   = 2,
    COHERENCE_RESTORED     = 3,
    UNRESOLVABLE_HORIZON   = 4
};

/**
 * Classification of a slot inside or around an Environment Horizon Box (`E_box`).
 */
enum HorizonSlotRoleFlags : uint8_t {
    HORIZON_ROLE_OUTSIDE         = 0u,
    HORIZON_ROLE_CONFLICT_CENTER = 1u << 0, ///< Distance 0: conflicted slot or parent
    HORIZON_ROLE_INTERIOR        = 1u << 1, ///< Distance `1 .. radius-1` inside `E_box`
    HORIZON_ROLE_PERIMETER       = 1u << 2, ///< Distance `== radius` boundary anchor
    HORIZON_ROLE_POINTER_STATE   = 1u << 3, ///< Selected as immutable Pointer State ($R_\delta \ge \tau$)
    HORIZON_ROLE_BRIDGE_RELAXED  = 1u << 4  ///< Intermediate bridge slot whose leaf layer was relaxed
};

/**
 * HorizonBoxConfigPOD (32 Bytes, alignas(16))
 * Parameters governing how Quantum Darwinism bounds the local environment around a conflict.
 */
struct alignas(16) HorizonBoxConfigPOD {
    uint32_t max_graph_hop_radius        = 4;     ///< Max spatial/hierarchy hops from conflict slot
    uint32_t max_horizon_slots           = 128;   ///< Hard upper bound on slots inside E_box
    float    pointer_redundancy_thresh   = 0.75f; ///< R_delta threshold to qualify as a Pointer State
    float    fragile_redundancy_thresh   = 0.30f; ///< R_delta below which a slot fully re-opens
    PatternCascadeLayer min_locked_layer = PatternCascadeLayer::LAYER_HIERARCHY_PARENT;
    uint8_t  allow_sibling_rebalancing   = 1;     ///< Allow adjusting [min..max] child/sibling counts
    uint8_t  lock_nuclear_semes          = 1;     ///< Preserve `IdeamLinguistics` nuclear seme bits
    uint8_t  promote_perimeter_anchors   = 1;     ///< Promote settled perimeter slots on E_box boundary
    uint8_t  reserved[8]                 = {0};
};
static_assert(sizeof(HorizonBoxConfigPOD) == 32, "HorizonBoxConfigPOD must be 32 bytes.");

/**
 * PointerStateRecordPOD (32 Bytes, alignas(16))
 * Represents a high-redundancy settled cell selected as an immutable anchor on or near E_box.
 */
struct alignas(16) PointerStateRecordPOD {
    uint32_t slot_id                    = 0;
    uint32_t settled_token_id           = 0;
    float    einselection_redundancy    = 1.0f;  ///< R_delta mutual-information witness score
    uint32_t highest_satisfied_spec     = 0;     ///< Highest CSS GenerationRule specificity satisfied
    uint32_t preserved_trait_mask       = 0;     ///< Structural trait & nuclear seme bits propagated inward
    uint32_t hop_distance_to_conflict   = 0;
    uint8_t  reserved[8]                = {0};
};
static_assert(sizeof(PointerStateRecordPOD) == 32, "PointerStateRecordPOD must be 32 bytes.");

/**
 * CoherenceBridgeSlotPOD (32 Bytes, alignas(16))
 * Represents an intermediate slot lying between a Pointer State and the problematic slot,
 * tracking which CSS cascade layers must stay locked to preserve coherence and which
 * leaf/variant masks can be relaxed.
 */
struct alignas(16) CoherenceBridgeSlotPOD {
    uint32_t slot_id                    = 0;
    uint32_t source_pointer_slot_id     = 0;
    uint32_t target_conflict_slot_id    = 0;
    uint32_t hops_from_pointer          = 0;
    uint32_t hops_to_conflict           = 0;
    uint32_t mutable_trait_mask         = 0;     ///< Properties/siblings alterable without losing coherence
    PatternCascadeLayer relax_down_to   = PatternCascadeLayer::LAYER_CHILD_CARDINALITY;
    uint8_t  reserved[7]                = {0};
};
static_assert(sizeof(CoherenceBridgeSlotPOD) == 32, "CoherenceBridgeSlotPOD must be 32 bytes.");

/**
 * HorizonSlotPartitionPOD (16 Bytes, alignas(16))
 * Per-slot horizon metadata computed by `ConflictHorizonBoxPartitionMetadataLogic` (`QD1`).
 */
struct alignas(16) HorizonSlotPartitionPOD {
    uint32_t slot_id                 = 0;
    uint32_t nearest_conflict_slot   = 0xFFFFFFFFu;
    uint32_t predecessor_toward_conf = 0xFFFFFFFFu; ///< Next hop along shortest path toward conflict
    uint16_t hop_distance            = 0xFFFFu;     ///< `0` = conflict center, `0xFFFF` = outside E_box
    uint8_t  role_flags              = HORIZON_ROLE_OUTSIDE;
    uint8_t  locked_cascade_floor    = static_cast<uint8_t>(PatternCascadeLayer::LAYER_HIERARCHY_PARENT);
};
static_assert(sizeof(HorizonSlotPartitionPOD) == 16, "HorizonSlotPartitionPOD must be 16 bytes.");

} // namespace ideam::core
