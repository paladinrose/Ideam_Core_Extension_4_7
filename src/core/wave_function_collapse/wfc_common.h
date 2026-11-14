#pragma once

/**
 * wfc_common.h
 * Pure C++26 POD types for `IdeamWaveFunctionCollapse` / `IdeamWFC` (`src/core/wave_function_collapse/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES (GODOT-ENVIRONMENT WAVE FUNCTION COLLAPSE)
 * ============================================================================
 * 1. DOMAIN SPACE IS THE GODOT ENVIRONMENT ITSELF:
 *    - Unlike traditional 2D texture or 3D voxel-grid WFC solvers, `IdeamWaveFunctionCollapse`
 *      operates over the generalized `ProceduralEnvironment` (`src/core/procedural_generation/`),
 *      where slots represent nodes, resources, gameplay rules, scene subtrees, or whole-game
 *      configuration shapes bounded by Godot's reflection capabilities (`ClassDB`, `SceneState`,
 *      property ranges, methods, signals, and `IdeamLinguistics` `Sememe` tags).
 *
 * 2. MULTI-AXIS CONSTRAINT PROPAGATION:
 *    - Simultaneously propagates constraints across:
 *        a) `AXIS_SPATIAL_PORT`: Directional 2D/3D/hypergraph port or `Socket3D` adjacency.
 *        b) `AXIS_HIERARCHY_PARENT_TO_CHILD` & `AXIS_HIERARCHY_CHILD_TO_PARENT`: Cascading
 *           `ACCEPT_PARENTS` scene/configuration hierarchy rules (e.g., `CastleTower` accepts
 *           `CastleGrounds | CastleRuins`).
 *        c) `AXIS_SIBLING_CO_OCCURRENCE`: Child cardinality budgets (`ACCEPT_CHILDREN_CARDINALITY`
 *           `[min..max]`) and sibling dependencies (`REQUIRE_SIBLING` / `FORBID_SIBLING`, e.g.,
 *           `TowerLanding [0..3; requires TowerStairs]`).
 *        d) `AXIS_REFLECTION_PROPERTY_LINK` & `AXIS_SEMEME_AFFINITY_LINK`: Cross-slot property,
 *           signal, method, and nuclear/peripheral seme compatibility.
 *
 * 3. QUANTUM DARWINISM HANDOFF (`IdeamQuantumDarwinism`):
 *    - When forward collapse encounters a domain emptiness or cardinality/sibling contradiction,
 *      `WFCContradictionReportPOD` and `EnvironmentSlotPOD::has_conflict` (`POINTER_OR_CONFLICT_MASK`)
 *      record the exact failure locus so `IdeamQuantumDarwinism` can isolate a bounded horizon box
 *      (`E_box`) and repair coherence via Einselected Pointer States without full backtracking.
 */

#include "../procedural_generation/procgen_common.h"
#include "../procedural_generation/cascade_rules.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace ideam::core {

/**
 * Constraint propagation axis for a WFC rule or directed hypergraph edge.
 */
enum class WFCConstraintAxis : uint8_t {
    AXIS_SPATIAL_PORT              = 0, ///< 2D/3D directional port or `Socket3D` adjacency
    AXIS_HIERARCHY_PARENT_TO_CHILD = 1, ///< Downward parent -> child admissibility
    AXIS_HIERARCHY_CHILD_TO_PARENT = 2, ///< Upward child -> parent acceptance (`ACCEPT_PARENTS`)
    AXIS_SIBLING_CO_OCCURRENCE     = 3, ///< Sibling co-occurrence (`REQUIRE_SIBLING` / `FORBID_SIBLING`)
    AXIS_REFLECTION_PROPERTY_LINK  = 4, ///< Reflected property/method/signal compatibility link
    AXIS_SEMEME_AFFINITY_LINK      = 5  ///< Linguistic `Sememe` thematic coherence link
};

/**
 * Entropy selection heuristic used when choosing the next uncollapsed slot in `ProceduralEnvironment`.
 */
enum class WFCEntropyHeuristic : uint8_t {
    HEURISTIC_SHANNON_WEIGHTED           = 0, ///< Frequency-weighted Shannon entropy with SplitMix64 tie-break
    HEURISTIC_POPCOUNT_MRV               = 1, ///< Minimum Remaining Values (`BitmaskDomain256POD::popcount()`)
    HEURISTIC_HIERARCHY_TOP_DOWN_SHANNON = 2, ///< Prioritizes shallower hierarchy roots before leaf slots
    HEURISTIC_CASCADE_LAYER_PRIORITY     = 3  ///< Prioritizes higher-precedence `PatternCascadeLayer` slots
};

/**
 * Bitflags describing the cause of a WFC contradiction on an `EnvironmentSlotPOD`.
 */
enum WFCContradictionCauseFlags : uint32_t {
    WFC_CONTRADICTION_NONE                     = 0u,
    WFC_CONTRADICTION_EMPTY_DOMAIN             = 1u << 0, ///< Slot's `BitmaskDomain256POD` became all-zero
    WFC_CONTRADICTION_PARENT_INCOMPATIBLE      = 1u << 1, ///< No token in child domain accepts any token in parent domain
    WFC_CONTRADICTION_CARDINALITY_MIN_DEFICIT  = 1u << 2, ///< Remaining sibling slots cannot satisfy `min_occurrences`
    WFC_CONTRADICTION_CARDINALITY_MAX_EXCEEDED = 1u << 3, ///< Collapsed sibling slots exceed `max_occurrences`
    WFC_CONTRADICTION_MISSING_REQUIRED_SIBLING = 1u << 4, ///< `REQUIRE_SIBLING` token unavailable across all siblings
    WFC_CONTRADICTION_FORBIDDEN_SIBLING_ACTIVE = 1u << 5, ///< `FORBID_SIBLING` violated by collapsed sibling
    WFC_CONTRADICTION_PORT_ADJACENCY_EMPTY     = 1u << 6  ///< Spatial/port edge pruned all remaining candidate states
};

/**
 * WFCRuleEntryPOD (48 Bytes, alignas(16))
 * Flat `(port_id, source_state) -> allowed_mask` entry in the WFC rule tensor.
 * Supports aggregate initialization `{p_port_id, p_source_state, p_allowed_mask}`.
 */
struct alignas(16) WFCRuleEntryPOD {
    uint32_t            port_id         = 0;                                       ///< 4B: Port/socket/axis identifier
    uint32_t            source_state    = 0;                                       ///< 4B: Source token index `[0..255]`
    BitmaskDomain256POD allowed_mask{};                                            ///< 32B: Allowed neighbor/relative tokens
    float               rule_weight     = 1.0f;                                    ///< 4B: Relative rule weight / confidence
    WFCConstraintAxis   axis            = WFCConstraintAxis::AXIS_SPATIAL_PORT;    ///< 1B
    uint8_t             min_cardinality = 0;                                       ///< 1B
    uint8_t             max_cardinality = 255;                                     ///< 1B
    uint8_t             flags           = 0;                                       ///< 1B -> 48B total
};
static_assert(sizeof(WFCRuleEntryPOD) == 48, "WFCRuleEntryPOD must be 48 bytes.");

/**
 * WFCDirectedEdgePOD (16 Bytes, alignas(16))
 * Directed hypergraph / dual-graph / hierarchy / `Socket3D` adjacency link.
 * Supports aggregate initialization `{p_from_slot, p_to_slot, p_port_id}`.
 */
struct alignas(16) WFCDirectedEdgePOD {
    uint32_t          from_slot        = 0;                                    ///< 4B
    uint32_t          to_slot          = 0;                                    ///< 4B
    uint32_t          port_id          = 0;                                    ///< 4B
    WFCConstraintAxis axis             = WFCConstraintAxis::AXIS_SPATIAL_PORT; ///< 1B
    uint8_t           is_bidirectional = 0;                                    ///< 1B
    uint16_t          edge_flags       = 0;                                    ///< 2B -> 16B total
};
static_assert(sizeof(WFCDirectedEdgePOD) == 16, "WFCDirectedEdgePOD must be 16 bytes.");

/**
 * WFCObservedCoOccurrencePod (48 Bytes, alignas(16))
 * Structural hierarchy, property link, or spatial adjacency rule extracted by
 * `WFCSceneAnalyzer` from `PackedScene::get_state()` (`SceneState`) or live Godot Objects.
 */
struct alignas(16) WFCObservedCoOccurrencePod {
    uint32_t          parent_or_anchor_state  = 0;                                    ///< 4B
    uint32_t          child_or_neighbor_state = 0;                                    ///< 4B
    uint32_t          port_or_socket_tag      = 0;                                    ///< 4B
    uint32_t          min_occurrences         = 1;                                    ///< 4B
    uint32_t          max_occurrences         = 1;                                    ///< 4B
    glm::vec3         quantized_offset_3d     = glm::vec3(0.0f);                      ///< 12B
    uint32_t          required_sibling_state  = 0xFFFFFFFFu;                          ///< 4B
    uint32_t          forbidden_sibling_state = 0xFFFFFFFFu;                          ///< 4B
    float             observed_frequency      = 1.0f;                                 ///< 4B
    WFCConstraintAxis axis                    = WFCConstraintAxis::AXIS_SPATIAL_PORT; ///< 1B
    uint8_t           reserved[3]             = {0, 0, 0};                            ///< 3B -> 48B total
};
static_assert(sizeof(WFCObservedCoOccurrencePod) == 48, "WFCObservedCoOccurrencePod must be 48 bytes.");

/**
 * WFCContradictionReportPOD (32 Bytes, alignas(16))
 * Emitted whenever a slot or parent hierarchy fails arc consistency or cardinality budgets.
 * Directly feeds `IdeamQuantumDarwinism` (`ConflictHorizonBoxPartitionMetadataLogic`).
 */
struct alignas(16) WFCContradictionReportPOD {
    uint32_t            slot_id                    = 0xFFFFFFFFu;                 ///< 4B: Conflicted slot index
    uint32_t            parent_slot_id             = 0xFFFFFFFFu;                 ///< 4B: Parent slot index (if any)
    uint32_t            violated_rule_id           = 0xFFFFFFFFu;                 ///< 4B: `GenerationRulePOD::rule_id` or port ID
    uint32_t            contradiction_flags        = WFC_CONTRADICTION_NONE;      ///< 4B: `WFCContradictionCauseFlags` bitmask
    uint32_t            deficit_or_excess_token_id = 0xFFFFFFFFu;                 ///< 4B: Token causing cardinality/sibling fault
    uint32_t            recommended_horizon_radius = 4u;                          ///< 4B: Suggested `E_box` hop radius for QD
    float               local_einselection_score   = 0.0f;                        ///< 4B: $R_\delta$ at conflict time
    PatternCascadeLayer failed_cascade_layer       = PatternCascadeLayer::LAYER_DOMAIN; ///< 1B
    uint8_t             reserved[3]                = {0, 0, 0};                   ///< 3B -> 32B total
};
static_assert(sizeof(WFCContradictionReportPOD) == 32, "WFCContradictionReportPOD must be 32 bytes.");

/**
 * WFCCollapseStepRecordPOD (32 Bytes, alignas(16))
 * Audit trail entry for each forward collapse decision.
 */
struct alignas(16) WFCCollapseStepRecordPOD {
    uint32_t            step_index            = 0;                                  ///< 4B
    uint32_t            slot_id               = 0;                                  ///< 4B
    int32_t             chosen_token_id       = -1;                                 ///< 4B
    uint32_t            pre_collapse_popcount = 0;                                  ///< 4B
    float               pre_collapse_entropy  = 0.0f;                               ///< 4B
    uint32_t            propagated_prunes     = 0;                                  ///< 4B: Number of domain bits pruned in wave
    PatternCascadeLayer active_layer          = PatternCascadeLayer::LAYER_DOMAIN;  ///< 1B
    uint8_t             entropy_bucket        = 0;                                  ///< 1B
    uint8_t             caused_contradiction  = 0;                                  ///< 1B
    uint8_t             reserved              = 0;                                  ///< 1B
    uint32_t            reserved_tail         = 0;                                  ///< 4B -> 32B total
};
static_assert(sizeof(WFCCollapseStepRecordPOD) == 32, "WFCCollapseStepRecordPOD must be 32 bytes.");

} // namespace ideam::core
