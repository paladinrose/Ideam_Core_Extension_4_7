#pragma once

/**
 * cascade_rules.h
 * Pure C++26 CSS-Style Cascading `AnalysisRulePOD` and `GenerationRulePOD` structures
 * (`src/core/procedural_generation/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * 1. TWO DISTINCT CASCADING RULE FAMILIES:
 *    - `AnalysisRulePOD`: Governs how `ProceduralPatterner` inspects a source Godot artifact
 *      (`PackedScene` via `SceneState`, live `Node` hierarchy, `Resource`, `RefCounted`).
 *      Example cascade:
 *        * Rule 1 (Base): "Follow the hierarchy of objects in the scene, using the Names of
 *          those nodes to define connection rules."
 *        * Rule 2 (Child Cascade): "...but only in cases where the object is of type `Node3D`
 *          and has suffix `_cell`. Ignore other types of nodes from consideration."
 *    - `GenerationRulePOD`: Emitted by `ProceduralPatterner` into a `ProceduralPattern`
 *      (or hand-authored in the Inspector) to govern how downstream solvers (`WFCSolverCore`,
 *      `QuantumDarwinismSolver`) populate the `ProceduralEnvironment`.
 *      Example rubric:
 *        * Root Selector Rule: "Consider only `Node3D` tokens with suffix `_cell`."
 *        * Cascading Parent Rule: "If a cell is named `CastleTower`, it accepts parents
 *          named `CastleGrounds` or `CastleRuins`."
 *        * Cascading Sibling / Children Rule: "If a cell is named `CastleTower`, it accepts
 *          children cells `TowerGate` (min 1, max 2), `TowerStairs` (max 1), and
 *          `TowerLanding` (max 3; requires `TowerStairs` be present)."
 *
 * 2. SYNERGY WITH QUANTUM DARWINISM:
 *    - The `CascadeLayer`, `priority`, `specificity`, and `is_important` fields on
 *      `GenerationRulePOD` define the coherence hierarchy for `IdeamQuantumDarwinism`:
 *      high-specificity structural rules (`LAYER_DOMAIN`, `LAYER_HIERARCHY_PARENT`) remain
 *      locked as Pointer State invariants, while lower-precedence sibling cardinality or
 *      variant rules (`LAYER_SIBLING_CARDINALITY`, `LAYER_VARIANT_DETAIL`) can be selectively
 *      relaxed along pointer-to-conflict paths without losing cell coherence.
 */

#include "procgen_common.h"
#include <cstdint>

namespace ideam::core {

enum class PatternRuleScope : uint8_t {
    SELF        = 0, // Applies strictly to the matched node/slot (:host)
    CHILDREN    = 1, // Direct parent -> child relationship (`>`)
    DESCENDANTS = 2, // Any ancestor -> descendant relationship (` `)
    SIBLINGS    = 3, // Co-occurring sibling nodes under the same parent (`~` / `+`)
    ALL         = 4  // Applies to both self and descendants
};

enum class PatternCascadeLayer : uint8_t {
    LAYER_DOMAIN              = 0, // Root token admission (e.g., `Node3D[name$="_cell"]`)
    LAYER_HIERARCHY_PARENT    = 1, // Parent acceptance (e.g., `CastleGrounds | CastleRuins`)
    LAYER_SIBLING_DEPENDENCY  = 2, // Required/forbidden siblings (e.g., `requires TowerStairs`)
    LAYER_CHILD_CARDINALITY   = 3, // Min/max child counts (e.g., `TowerLanding [0..3]`)
    LAYER_SPATIAL_SOCKET      = 4, // 2D/3D spatial port/socket adjacency
    LAYER_VARIANT_DETAIL      = 5, // Decorative / property range overrides (first to relax in QD)
    LAYER_PINNED_OVERRIDE     = 6  // Designer-pinned landmark overrides (`!important`)
};

enum class AnalysisDirectiveFlags : uint32_t {
    NONE                         = 0u,
    FOLLOW_SCENE_HIERARCHY       = 1u << 0,
    USE_NODE_NAMES_AS_TOKENS     = 1u << 1,
    EXTRACT_PARENT_ACCEPTANCE    = 1u << 2,
    EXTRACT_CHILD_CARDINALITY    = 1u << 3,
    EXTRACT_SIBLING_REQUIREMENTS = 1u << 4,
    EXTRACT_SPATIAL_OFFSETS      = 1u << 5,
    EXTRACT_PROPERTY_RANGES      = 1u << 6,
    IGNORE_NON_MATCHING_NODES    = 1u << 7
};

enum class GenerationRuleOp : uint8_t {
    ADMIT_ARCHETYPE            = 0, // Filters valid tokens for a slot by ReflectionSignaturePOD
    ACCEPT_PARENTS             = 1, // Restricts parent slot token mask
    ACCEPT_CHILDREN_CARDINALITY= 2, // Enforces [min_count, max_count] + sibling dependencies
    REQUIRE_SIBLING            = 3, // Requires a sibling token to be present under the same parent
    FORBID_SIBLING             = 4, // Forbids a sibling token under the same parent
    ALLOW_SPATIAL_NEIGHBOR     = 5, // Port/socket directional adjacency constraint
    CONSTRAIN_PROPERTY_RANGE   = 6  // Constrains reflected numeric/vector property to [min, max]
};

/**
 * AnalysisRulePOD (64 Bytes, alignas(64))
 * Flat DOD representation of a cascading `AnalysisRule` evaluated by `ProceduralPatterner`.
 */
struct alignas(64) AnalysisRulePOD {
    ReflectionSignaturePOD selector{};           // 32B: Class, name prefix/suffix, props/methods/signals
    uint32_t rule_id                 = 0;        // 4B
    uint32_t parent_rule_id          = 0xFFFFFFFFu; // 4B: Parent AnalysisRule in the cascade
    uint32_t directives_mask         = 0;        // 4B: AnalysisDirectiveFlags bitmask
    int32_t  priority                = 0;        // 4B
    uint32_t specificity             = 0;        // 4B: Computed CSS-style selector specificity
    float    property_range_min      = 0.0f;     // 4B
    float    property_range_max      = 1.0f;     // 4B
    PatternRuleScope scope           = PatternRuleScope::ALL; // 1B
    PatternCascadeLayer layer        = PatternCascadeLayer::LAYER_DOMAIN; // 1B
    uint8_t  is_important            = 0;        // 1B
    uint8_t  ignore_unmatched        = 0;        // 1B
    uint8_t  reserved_padding[4]     = {0};      // 4B -> 64B total
};
static_assert(sizeof(AnalysisRulePOD) == 64, "AnalysisRulePOD must be 64 bytes.");

/**
 * GenerationRulePOD (64 Bytes, alignas(64))
 * Flat DOD representation of a cascading `GenerationRule` inside a `ProceduralPattern`.
 */
struct alignas(64) GenerationRulePOD {
    ReflectionSignaturePOD selector{};           // 32B: Selector matching target token/cell
    uint32_t rule_id                 = 0;        // 4B
    uint32_t parent_rule_id          = 0xFFFFFFFFu; // 4B: Parent GenerationRule in the cascade
    uint32_t target_token_mask_index = 0;        // 4B: Index into BitmaskDomain256POD table (allowed parents/children)
    uint32_t required_sibling_token  = 0xFFFFFFFFu; // 4B: e.g., `TowerStairs` required by `TowerLanding`
    uint32_t forbidden_sibling_token = 0xFFFFFFFFu; // 4B
    uint16_t min_occurrences         = 0;        // 2B: e.g., `TowerGate` min = 1
    uint16_t max_occurrences         = 1;        // 2B: e.g., `TowerLanding` max = 3
    int16_t  priority                = 0;        // 2B
    uint16_t specificity             = 0;        // 2B: CSS specificity score
    PatternRuleScope scope           = PatternRuleScope::SELF; // 1B
    PatternCascadeLayer layer        = PatternCascadeLayer::LAYER_DOMAIN; // 1B
    GenerationRuleOp operation       = GenerationRuleOp::ADMIT_ARCHETYPE; // 1B
    uint8_t  is_important            = 0;        // 1B -> 64B total
};
static_assert(sizeof(GenerationRulePOD) == 64, "GenerationRulePOD must be 64 bytes.");

} // namespace ideam::core
