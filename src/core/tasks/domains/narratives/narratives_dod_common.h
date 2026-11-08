#pragma once

/**
 * narratives_dod_common.h
 * Foundational pure C++26 data structures, table-driven buffer manifest descriptors,
 * capacity configurations, and composable TaskGraph pipeline stage flags for the
 * IdeamNarratives <-> IdeamTasks Data-Oriented Design (DOD) bridge.
 *
 * Implements the 5 core contiguous memory table schemas defined in Section 3.2
 * of the Narratives Tasks Bridge specification plus the 4 reactive event rings:
 *   1. NarremeIdentityAndTraitBuffer  (Literal & Symbolic Narreme identity, 128-bit traits, Vector4 affinity, spatial & goal state)
 *   2. SymbolicAssignmentMapBuffer    (O(1) bidirectional Symbolic <-> Literal casting & replacement policy table)
 *   3. NarrativeRelationshipCSRBuffer (Compressed Sparse Row relationship edges & title tokens)
 *   4. ConditionEvaluationTableBuffer (Atomic Narrative_Condition, hierarchical Causal_Condition CSR, & bridged Gameplay_Condition rows)
 *   5. IncidentAndPlotStateBuffer     (Incident status/prerequisite/causal slices & Plot progression/PlotEventPOD slices)
 *   + 4 Event Rings:
 *      - NarremeSpatialTransferCommandPOD ring (N10 batched spatial & possession transfers)
 *      - GameplayConditionEventPOD ring        (N4 external gameplay trigger/property events)
 *      - CharacterGoalEventPOD ring            (N9 reactive character goal completion/failure events)
 *      - NarrativeSignalCommandPOD ring        (N11 deferred main-thread Godot Resource signal emission)
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <vector>

#include "../domain_dod_common.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

// Include the 3 Narratives task registry headers for compile-time LogicIDs
#include "registration/narratives_metadata_task_registry.h"
#include "registration/narratives_query_task_registry.h"
#include "registration/narratives_transform_task_registry.h"

// Include the 16 Narratives domain logic headers (N1 - N16) so all shared PODs & enums are available
#include "metadata_logic/causal_dependency_depth_metadata_logic.h"
#include "metadata_logic/story_arc_island_cluster_metadata_logic.h"
#include "query_logic/location_occupancy_query_logic.h"
#include "query_logic/narreme_condition_bitmask_query_logic.h"
#include "query_logic/relationship_graph_traversal_query_logic.h"
#include "transform_logic/causal_condition_wave_propagate_transform_logic.h"
#include "transform_logic/character_goal_plot_sync_transform_logic.h"
#include "transform_logic/gameplay_condition_bridge_transform_logic.h"
#include "transform_logic/incident_status_resolve_transform_logic.h"
#include "transform_logic/narrative_condition_evaluate_transform_logic.h"
#include "transform_logic/narrative_resource_commit_transform_logic.h"
#include "transform_logic/narreme_location_transfer_transform_logic.h"
#include "transform_logic/narreme_symbolic_similarity_transform_logic.h"
#include "transform_logic/plot_status_evaluate_transform_logic.h"
#include "transform_logic/role_best_fit_assignment_transform_logic.h"
#include "transform_logic/symbolic_reference_resolve_transform_logic.h"

namespace ideam::core {

/**
 * NarrativesPipelineStageFlags
 * Composable bitmask flags representing logical stages in the Narratives DOD evaluation pipeline.
 * Developers can pass any combination of flags to NarrativeDOD::inject_pipeline_stages(),
 * or omit stages and construct custom TaskGraphDOD topologies using NarrativeDOD's node builders.
 */
enum NarrativesPipelineStageFlags : uint32_t {
    NARRATIVES_STAGE_NONE                      = 0u,

    // Stage 1: N10 NarremeLocationTransfer + N13 LocationOccupancy
    NARRATIVES_STAGE_SPATIAL_TRANSFERS         = 1u << 0,

    // Stage 2: N1 NarremeSymbolicSimilarity -> N2 RoleBestFitAssignment -> N3 SymbolicReferenceResolve
    NARRATIVES_STAGE_SYMBOLIC_CASTING          = 1u << 1,

    // Stage 3: N15 CausalDependencyDepth + N16 StoryArcIslandCluster (parallel in the same Kahn wave)
    NARRATIVES_STAGE_TOPOLOGY_METADATA         = 1u << 2,

    // Stage 4: N4 GameplayConditionBridge + N5 NarrativeConditionEvaluate (parallel in the same Kahn wave)
    NARRATIVES_STAGE_ATOMIC_CONDITIONS         = 1u << 3,

    // Stage 5: N6 CausalConditionWavePropagate + N12 NarremeConditionBitmask
    NARRATIVES_STAGE_CAUSAL_PROPAGATION        = 1u << 4,

    // Stage 6: N7 IncidentStatusResolve -> N8 PlotStatusEvaluate -> N9 CharacterGoalPlotSync
    NARRATIVES_STAGE_INCIDENT_AND_PLOT_RESOLVE = 1u << 5,

    // Stage 7: N11 NarrativeResourceCommit
    NARRATIVES_STAGE_RESOURCE_COMMIT           = 1u << 6,

    // Preset masks for common execution paradigms:
    NARRATIVES_STAGE_CORE_EVALUATION_MASK      =
        NARRATIVES_STAGE_TOPOLOGY_METADATA         |
        NARRATIVES_STAGE_ATOMIC_CONDITIONS         |
        NARRATIVES_STAGE_CAUSAL_PROPAGATION        |
        NARRATIVES_STAGE_INCIDENT_AND_PLOT_RESOLVE,

    NARRATIVES_STAGE_PURE_DOD_MASK             =
        NARRATIVES_STAGE_SPATIAL_TRANSFERS         |
        NARRATIVES_STAGE_SYMBOLIC_CASTING          |
        NARRATIVES_STAGE_CORE_EVALUATION_MASK,

    NARRATIVES_STAGE_DEFAULT_PIPELINE_MASK     =
        NARRATIVES_STAGE_PURE_DOD_MASK             |
        NARRATIVES_STAGE_RESOURCE_COMMIT,

    NARRATIVES_STAGE_HYBRID_DEFAULT_MASK       = NARRATIVES_STAGE_DEFAULT_PIPELINE_MASK,

    NARRATIVES_STAGE_ALL_MASK                  = 0x007Fu
};

/**
 * NarrativesNodeExecutionConfig
 * Optional overrides when instantiating any Narratives task node in a TaskGraphDOD.
 */
struct NarrativesNodeExecutionConfig {
    MemoryView view                    = MemoryView::SingleElementView;
    MemoryStrategy strategy            = MemoryStrategy::FlatStrategy;
    MemoryTypes element_type           = MemoryTypes::INT32;
    QueryOp query_op                   = QueryOp::CULL;
    int32_t execution_priority         = 0;
    bool use_registry_factory          = false; // If false, uses direct C++ template instantiation (zero Dictionary overhead)
    std::span<const GrantPartPOD> custom_requirements{};
};

/**
 * NarrativesStageAnchor
 * Topological handle returned for each injected stage in a TaskGraphDOD.
 * Exposes entry_node, exit_node, and all internal nodes so developers can wire
 * custom tasks in parallel within the same Kahn wave or splice custom tasks between stages.
 */
struct NarrativesStageAnchor {
    NarrativesPipelineStageFlags stage = NARRATIVES_STAGE_NONE;
    NodeID entry_node                  = INVALID_ID;
    NodeID exit_node                   = INVALID_ID;
    std::vector<NodeID> nodes;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return entry_node != INVALID_ID && exit_node != INVALID_ID;
    }
};

/**
 * NarrativesPipelineTopology
 * Describes the set of NarrativesStageAnchors injected into a TaskGraphDOD.
 */
struct NarrativesPipelineTopology {
    NodeID first_node = INVALID_ID;
    NodeID last_node  = INVALID_ID;
    std::vector<NarrativesStageAnchor> stages;

    [[nodiscard]] const NarrativesStageAnchor* get_stage_anchor(NarrativesPipelineStageFlags p_stage) const noexcept {
        for (const auto& anchor : stages) {
            if (anchor.stage == p_stage) {
                return &anchor;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool has_stage(NarrativesPipelineStageFlags p_stage) const noexcept {
        const NarrativesStageAnchor* anchor = get_stage_anchor(p_stage);
        return anchor != nullptr && anchor->is_valid();
    }
};

/**
 * NarrativeCapacityKind
 * Identifies which configurable capacity field in NarrativesBufferCapacities governs
 * a given NarrativeBufferSlot column or ring buffer.
 */
enum class NarrativeCapacityKind : uint8_t {
    NARREMES = 0,               // Table 1: NarremeIdentityAndTraitBuffer rows
    SYMBOLIC_SLOTS,             // Table 2: SymbolicAssignmentMapBuffer rows
    RELATIONSHIP_OFFSETS,       // Table 3: CSR row_offsets (max_narremes + 1)
    RELATIONSHIP_EDGES,         // Table 3: Flattened RelationshipEdgePOD array
    CONDITIONS,                 // Table 4: ConditionEvaluationTableBuffer rows
    CAUSAL_LINKS,               // Table 4: Flattened CausalLinkPOD / child condition slices
    INCIDENTS,                  // Table 5: IncidentStateSoABuffer rows
    INCIDENT_CONDITION_REFS,    // Table 5: Flattened incident condition index / parent CSR IDs
    INCIDENT_CSR_OFFSETS,       // Table 5: Incident parent CSR offsets (max_incidents + 1)
    PLOTS,                      // Table 5: PlotProgressionSoABuffer rows
    PLOT_EVENTS,                // Table 5: Flattened PlotEventPOD array
    CHARACTER_GOALS,            // Table 1/5: Flattened character goal Plot IDs
    LOCATION_NEIGHBORS,         // Table 1: Flattened location adjacency CSR neighbor IDs
    BRIDGED_PROPERTIES,         // Table 4: Optional bridged GamePropertyBuffer columns
    SPATIAL_TRANSFER_RING,      // Ring 1: NarremeSpatialTransferCommandPOD capacity
    GAMEPLAY_EVENT_RING,        // Ring 2: GameplayConditionEventPOD capacity
    CHARACTER_GOAL_EVENT_RING,  // Ring 3: CharacterGoalEventPOD capacity
    SIGNAL_COMMAND_RING,        // Ring 4: NarrativeSignalCommandPOD capacity
    RING_CURSOR_OR_COUNTER      // 4-element uint32_t cursor / atomic counter buffers
};

/**
 * NarrativeTableGroup
 * Logical table ownership tag for each buffer slot, enabling NarrativeDOD to synchronize
 * active selection counts in a single loop over NARRATIVE_BUFFER_SPECS.
 */
enum class NarrativeTableGroup : uint8_t {
    NARREME_IDENTITY = 0,       // Synced with narreme_allocator_
    SYMBOLIC_ASSIGNMENT,        // Synced with symbolic_slot_allocator_
    RELATIONSHIP_CSR_ROW,       // Synced with narreme_allocator_ (+1 for CSR offsets)
    RELATIONSHIP_CSR_EDGE,      // Synced with relationship_edge_cursor_
    CONDITION_TABLE,            // Synced with condition_allocator_
    CAUSAL_LINK_SLICE,          // Synced with causal_link_cursor_
    INCIDENT_STATE,             // Synced with incident_allocator_
    INCIDENT_COND_SLICE,        // Synced with incident_cond_ref_cursor_
    PLOT_STATE,                 // Synced with plot_allocator_
    PLOT_EVENT_SLICE,           // Synced with plot_event_cursor_
    CHARACTER_GOAL_SLICE,       // Synced with character_goal_cursor_
    LOCATION_NEIGHBOR_SLICE,    // Synced with location_neighbor_cursor_
    BRIDGED_GAMEPLAY_PROPERTY,  // Synced with bridged_property_count_
    EVENT_RING_OR_COUNTER       // Managed via ring cursors / fixed scalar capacity
};

/**
 * NarrativesBufferCapacities
 * Configurable row capacities for the 5 Section 3.2 buffer tables, CSR slices, and 4 event rings.
 */
struct NarrativesBufferCapacities {
    uint32_t max_narremes                  = 2048;
    uint32_t max_symbolic_slots            = 2048;
    uint32_t max_relationships             = 8192;
    uint32_t max_conditions                = 4096;
    uint32_t max_causal_links              = 8192;
    uint32_t max_incidents                 = 2048;
    uint32_t max_incident_condition_refs   = 8192;
    uint32_t max_plots                     = 1024;
    uint32_t max_plot_events               = 4096;
    uint32_t max_character_goals           = 4096;
    uint32_t max_location_neighbors        = 2048;
    uint32_t max_bridged_properties        = 4096;
    uint32_t max_spatial_transfer_commands = 1024;
    uint32_t max_gameplay_condition_events = 1024;
    uint32_t max_character_goal_events     = 1024;
    uint32_t max_signal_commands           = 2048;
    uint32_t max_ring_cursors              = 4;

    [[nodiscard]] constexpr uint32_t get_capacity(NarrativeCapacityKind p_kind) const noexcept {
        switch (p_kind) {
            case NarrativeCapacityKind::NARREMES:                  return max_narremes;
            case NarrativeCapacityKind::SYMBOLIC_SLOTS:            return max_symbolic_slots;
            case NarrativeCapacityKind::RELATIONSHIP_OFFSETS:      return max_narremes + 1u;
            case NarrativeCapacityKind::RELATIONSHIP_EDGES:        return max_relationships;
            case NarrativeCapacityKind::CONDITIONS:                return max_conditions;
            case NarrativeCapacityKind::CAUSAL_LINKS:              return max_causal_links;
            case NarrativeCapacityKind::INCIDENTS:                 return max_incidents;
            case NarrativeCapacityKind::INCIDENT_CONDITION_REFS:   return max_incident_condition_refs;
            case NarrativeCapacityKind::INCIDENT_CSR_OFFSETS:      return max_incidents + 1u;
            case NarrativeCapacityKind::PLOTS:                     return max_plots;
            case NarrativeCapacityKind::PLOT_EVENTS:               return max_plot_events;
            case NarrativeCapacityKind::CHARACTER_GOALS:           return max_character_goals;
            case NarrativeCapacityKind::LOCATION_NEIGHBORS:        return max_location_neighbors;
            case NarrativeCapacityKind::BRIDGED_PROPERTIES:        return max_bridged_properties;
            case NarrativeCapacityKind::SPATIAL_TRANSFER_RING:     return max_spatial_transfer_commands;
            case NarrativeCapacityKind::GAMEPLAY_EVENT_RING:       return max_gameplay_condition_events;
            case NarrativeCapacityKind::CHARACTER_GOAL_EVENT_RING: return max_character_goal_events;
            case NarrativeCapacityKind::SIGNAL_COMMAND_RING:       return max_signal_commands;
            case NarrativeCapacityKind::RING_CURSOR_OR_COUNTER:    return max_ring_cursors;
            default:                                               return max_narremes;
        }
    }
};

/**
 * NarrativeBufferSlot
 * Enumerates every DOD buffer column and event ring across the 5 Section 3.2 tables
 * and 4 event rings. Indexed 1:1 into NARRATIVE_BUFFER_SPECS and NarrativesBufferManifest.
 */
enum class NarrativeBufferSlot : uint8_t {
    // --- Table 1: NarremeIdentityAndTraitBuffer (Literal & Symbolic Narreme Columns) ---
    NARREME_TRAIT_BITMASK_LO = 0,       // uint64_t: Low 64 bits of 128-bit trait signature
    NARREME_TRAIT_BITMASK_HI,           // uint64_t: High 64 bits of 128-bit trait signature
    NARREME_AFFINITY_VECTOR,            // godot::Vector4: 4D affinity vector
    NARREME_TYPE,                       // uint8_t: NarremeType (Character, Prop, Location, Incident, Plot, Narrative)
    NARREME_IS_SYMBOLIC,                // uint8_t: 0 = Literal, 1 = Symbolic (Role, Symbol, Place, Moment, Story)
    NARREME_LOCATION_ID,                // uint32_t: Current Location Narreme ID (or INVALID_ID)
    NARREME_POSSESSOR_ID,               // uint32_t: Current Possessor Character ID for Props (or INVALID_ID)
    NARREME_STRUCTURAL_SIG,             // uint64_t: Packed structural signature bitmask (relationships/goals/possessions)
    NARREME_CANDIDATE_MASK,             // uint8_t: Candidate exclusion mask for N1/N2 casting
    NARREME_CANDIDATE_SCORE,            // int32_t: Similarity/discrepancy score output from N1 for N2
    NARREME_PENDING_LOCATION_ID,        // uint32_t: Staged destination location_id for N10
    NARREME_PENDING_POSSESSOR_ID,       // uint32_t: Staged destination possessor_id for N10
    NARREME_PREVIOUS_LOCATION_ID,       // uint32_t: Exited location_id recorded by N10
    NARREME_PREVIOUS_POSSESSOR_ID,      // uint32_t: Previous possessor_id recorded by N10
    NARREME_SPATIAL_TRANSITION_FLAGS,   // uint8_t: NarremeSpatialTransitionFlags (N10 -> N11)
    LOCATION_CHAR_COUNT,                // int32_t: Active Character occupancy count per Location
    LOCATION_PROP_COUNT,                // int32_t: Active Prop occupancy count per Location
    LOCATION_PARENT_ID,                 // uint32_t: Optional parent Location ID for hierarchical regions (N13)
    LOCATION_CSR_OFFSETS,               // uint32_t: Location adjacency CSR row offsets (N13)
    LOCATION_CSR_NEIGHBORS,             // uint32_t: Location adjacency CSR neighbor Location IDs (N13)
    LOCATION_SELECTION,                 // int32_t: Auxiliary Location selection buffer (N13)
    CHARACTER_GOAL_STATUS,              // int8_t: Evaluated PlotStatusDOD for Character goals (N9)
    CHARACTER_PRIMARY_GOAL_PLOT_ID,     // uint32_t: 1:1 primary goal Plot ID per Character (N9)
    CHARACTER_GOAL_MASK,                // uint64_t: 64-bit goal/occupancy bitmask per Narreme (N5, N9)
    CHARACTER_GOAL_SLICE_OFFSET,        // uint32_t: CSR goal slice start offset per Character (N9)
    CHARACTER_GOAL_SLICE_COUNT,         // uint16_t: CSR goal slice count per Character (N9)
    CHARACTER_GOAL_PLOT_ID,             // uint32_t: Contiguous CSR array of goal Plot IDs (N9)
    CHARACTER_GOAL_TRANSITION_FLAGS,    // uint8_t: CharacterGoalOutcomeFlags (N9)
    NARREME_OBJECT_ID,                  // uint64_t: Godot Resource ObjectID handle for OOP <-> DOD sync
    NARREME_TELEMETRY_SNAPSHOT,         // NarrativeTelemetrySnapshotPOD (32B AoS): Boundary snapshot row (N11)

    // --- Table 2: SymbolicAssignmentMapBuffer (Symbolic <-> Literal Casting) ---
    SYMBOLIC_ASSIGNMENT_POD,            // SymbolicAssignmentRecordPOD (16B AoS)
    SYMBOLIC_TO_LITERAL,                // int32_t: Symbolic slot ID -> assigned Literal Narreme ID (-1 if vacant)
    LITERAL_TO_SYMBOLIC,                // int32_t: Literal Narreme ID -> assigned Symbolic slot ID (-1 if none)
    ASSIGNMENT_FIT_SCORE,               // int32_t: Fit/discrepancy score of current symbolic assignment
    ASSIGNMENT_FIT_MODE,                // uint8_t: NarrativeBestFitMode per symbolic slot
    ASSIGNMENT_REPLACE_POLICY,          // uint8_t: NarrativeReplacementPolicy per symbolic slot
    PREVIOUS_LITERAL_ID,                // int32_t: Outgoing Literal Narreme ID recorded on rebind/vacate
    ASSIGNMENT_DIRTY_FLAGS,             // uint8_t: SymbolicAssignmentDirtyFlags (N2 -> N11)
    SYMBOLIC_TITLE_TOKEN,               // uint32_t: Interned official_name token per Symbolic slot (N3)

    // --- Table 3: NarrativeRelationshipCSRBuffer (Relationship Graph CSR) ---
    RELATIONSHIP_ROW_OFFSETS,           // uint32_t: Per-Narreme CSR row start offset (N+1 elements)
    RELATIONSHIP_ROW_COUNTS,            // uint16_t: Per-Narreme outgoing relationship count
    RELATIONSHIP_EDGE_POD,              // RelationshipEdgePOD (16B AoS): Contiguous CSR edges
    RELATIONSHIP_SOURCE_SELECTION,      // int32_t: Auxiliary source selection buffer for N14 traversal

    // --- Table 4: ConditionEvaluationTableBuffer (Narrative, Causal & Gameplay Conditions) ---
    CONDITION_ROW_POD,                  // ConditionEvaluationRowPOD (32B AoS)
    CONDITION_SUBJECT_ID,               // uint32_t: Subject Narreme / Symbolic / Property ID
    CONDITION_OBJECT_ID,                // uint32_t: Object Narreme / Symbolic / Threshold ID
    CONDITION_RESOLVED_SUBJECT_ID,      // uint32_t: Resolved Literal subject ID written by N3
    CONDITION_RESOLVED_OBJECT_ID,       // uint32_t: Resolved Literal object ID written by N3
    CONDITION_SUBJECT_IS_SYMBOLIC,      // uint8_t: Non-zero when subject_id references a Symbolic slot
    CONDITION_OBJECT_IS_SYMBOLIC,       // uint8_t: Non-zero when object_id references a Symbolic slot
    CONDITION_PREDICATE_OP,             // uint16_t: NarrativeAtomicPredicateOp / GameplayConditionPredicateOp
    CONDITION_TYPE,                     // uint8_t: NarrativeConditionType (Narrative=0, Causal=1, Gameplay=2)
    CONDITION_EXPECTED_STATUS,          // int8_t: Expected NarremeConditionStatusDOD
    CONDITION_STATUS,                   // int8_t: Current evaluated NarremeConditionStatusDOD (-2..2)
    CONDITION_READ_STATUS,              // int8_t: SwapView ping-pong read status buffer for N6
    CONDITION_TRANSITION_FLAGS,         // uint8_t: ConditionStatusTransitionFlags (N4, N5, N6 -> N11, N12)
    CAUSAL_SLICE_OFFSET,                // uint32_t: Per-condition slice offset into CausalLinkPOD array (N6)
    CAUSAL_SLICE_COUNT,                 // uint16_t: Per-condition slice count of child links (N6)
    CAUSAL_LINK_POD,                    // CausalLinkPOD (8B AoS): Flattened Causal_Condition child links (N6)
    CAUSAL_CHILD_CONDITION_ID,          // uint32_t: SoA child_condition_id array for Causal_Condition (N6)
    CAUSAL_EXPECTED_CAUSE_STATUS,       // int8_t: SoA required_cause_status array for Causal_Condition (N6)
    CAUSAL_CHANGED_COUNT,               // uint32_t: Convergence transition counter across N6 wave passes
    GAMEPLAY_TRIGGER_OR_STATUS,         // int32_t: External trigger volume or direct gameplay status buffer (N4)
    BRIDGED_PROP_MODDED_VALUE,          // int32_t: Bridged GamePropertyBuffer::modded_value (N4)
    BRIDGED_PROP_MODDED_MIN,            // int32_t: Bridged GamePropertyBuffer::modded_min (N4)
    BRIDGED_PROP_MODDED_MAX,            // int32_t: Bridged GamePropertyBuffer::modded_max (N4)
    BRIDGED_PROP_FLAGS,                 // uint16_t: Bridged GamePropertyBuffer::property_flags (N4)

    // --- Table 5: IncidentAndPlotStateBuffer (Incident State & Plot Progression) ---
    INCIDENT_STATUS,                    // int8_t: IncidentStatusDOD (0=HAS_NOT_HAPPENED, 1=HAS_HAPPENED, 2=CANNOT_HAPPEN)
    INCIDENT_COND_SLICE_OFFSET,         // uint32_t: Per-Incident prerequisite condition slice offset (N7, N12)
    INCIDENT_COND_SLICE_COUNT,          // uint16_t: Per-Incident prerequisite condition slice count (N7, N12)
    INCIDENT_COND_INDEX_TABLE,          // uint32_t: Flattened indirection array mapping slice slot -> condition_id
    INCIDENT_CAUSAL_COND_OFFSET,        // uint32_t: Per-Incident causal condition slice offset (N12, N15)
    INCIDENT_CAUSAL_COND_COUNT,         // uint16_t: Per-Incident causal condition slice count (N12, N15)
    INCIDENT_PARENT_CSR_OFFSETS,        // uint32_t: Direct Incident-to-Incident parent CSR offsets (N15)
    INCIDENT_PARENT_CSR_IDS,            // uint32_t: Direct Incident-to-Incident parent CSR IDs (N15)
    INCIDENT_CAUSAL_DEPTH,              // int32_t: Computed topological causal depth tier per Incident (N15)
    INCIDENT_TRANSITION_FLAGS,          // uint8_t: IncidentTransitionFlags (N7 -> N11)
    INCIDENT_RESOLVED_COUNT,            // uint32_t: Counter of newly resolved Incidents (N7)
    UNRESOLVED_SYMBOLIC_COUNT,          // uint16_t: Per-Incident/Plot count of unresolved symbolic references (N12)
    PLOT_STATUS,                        // int8_t: PlotStatusDOD (0..4)
    PLOT_EVENT_SLICE_OFFSET,            // uint32_t: Per-Plot event slice start offset into PlotEventPOD array (N8, N16)
    PLOT_EVENT_SLICE_COUNT,             // uint16_t: Per-Plot event count (N8, N16)
    PLOT_EVENT_POD,                     // PlotEventPOD (8B AoS): Flattened Plot_Event entries (N8, N16)
    PLOT_TRANSITION_FLAGS,              // uint8_t: PlotTransitionFlags (N8 -> N11)
    PLOT_RESOLVED_COUNT,                // uint32_t: Counter of newly completed/failed Plots (N8)
    PLOT_PARTICIPANT_MASK,              // uint64_t: 64-bit participant Character/Prop bitmask per Plot (N16)
    PLOT_PRIMARY_LOCATION_ID,           // uint32_t: Primary Location ID per Plot/Incident (N16)
    PLOT_GROUP_MASK,                    // uint64_t: 64-bit narrative group mask per Plot/Incident (N16)
    PLOT_CLUSTER_ID,                    // int32_t: Persisted DSU story-arc island cluster ID (N16)

    // --- 4 Event Rings & Ring Cursors ---
    SPATIAL_TRANSFER_COMMAND_RING,      // NarremeSpatialTransferCommandPOD (16B Ring): N10 command queue
    SPATIAL_TRANSFER_RING_CURSOR,       // uint32_t[2]: [read_idx, write_idx] cursor for SPATIAL_TRANSFER_COMMAND_RING
    GAMEPLAY_CONDITION_EVENT_RING,      // GameplayConditionEventPOD (16B Ring): N4 event queue
    GAMEPLAY_EVENT_RING_CURSOR,         // uint32_t[2]: [read_idx, write_idx] cursor for GAMEPLAY_CONDITION_EVENT_RING
    CHARACTER_GOAL_EVENT_RING,          // CharacterGoalEventPOD (16B Ring): N9 goal outcome event queue
    CHARACTER_GOAL_RING_CURSOR,         // uint32_t[2]: [read_idx, write_idx] cursor for CHARACTER_GOAL_EVENT_RING
    NARRATIVE_SIGNAL_COMMAND_RING,      // NarrativeSignalCommandPOD (16B Ring): N11 Godot signal dispatch queue
    NARRATIVE_SIGNAL_RING_CURSOR,       // uint32_t[2]: [read_idx, write_idx] cursor for NARRATIVE_SIGNAL_COMMAND_RING

    COUNT
};

static constexpr size_t NARRATIVE_BUFFER_SLOT_COUNT = static_cast<size_t>(NarrativeBufferSlot::COUNT);

/**
 * NarrativeBufferSpec
 * Compile-time descriptor row in NARRATIVE_BUFFER_SPECS driving loop-based buffer allocation,
 * semantic property lookup (get_buffer_id_by_name), and TaskGraphResource dictionary population.
 */
struct NarrativeBufferSpec {
    NarrativeBufferSlot slot                  = NarrativeBufferSlot::COUNT;
    std::string_view canonical_name           = {};
    std::array<std::string_view, 3> aliases   = {};
    uint32_t element_stride                   = sizeof(uint32_t);
    BufferLayoutType layout                   = BufferLayoutType::FLAT;
    SelectionMode selection_mode              = SelectionMode::RANGE;
    NarrativeCapacityKind capacity_kind       = NarrativeCapacityKind::NARREMES;
    NarrativeTableGroup table_group           = NarrativeTableGroup::NARREME_IDENTITY;

    [[nodiscard]] constexpr bool matches_name(std::string_view p_query) const noexcept {
        if (p_query.empty()) return false;
        if (canonical_name == p_query) return true;
        for (const auto& alias : aliases) {
            if (!alias.empty() && alias == p_query) {
                return true;
            }
        }
        return false;
    }
};

/**
 * NARRATIVE_BUFFER_SPECS
 * Table-driven compile-time manifest covering all columns across the 5 Section 3.2 tables
 * and 4 event rings, including all property name aliases used by N1-N16 logic structs.
 */
inline constexpr std::array<NarrativeBufferSpec, NARRATIVE_BUFFER_SLOT_COUNT> NARRATIVE_BUFFER_SPECS = {{
    // ========================================================================
    // Table 1: NarremeIdentityAndTraitBuffer
    // ========================================================================
    {
        NarrativeBufferSlot::NARREME_TRAIT_BITMASK_LO,
        "trait_bitmask_lo_buffer_id",
        {"narreme_trait_bitmask_lo_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_TRAIT_BITMASK_HI,
        "trait_bitmask_hi_buffer_id",
        {"narreme_trait_bitmask_hi_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_AFFINITY_VECTOR,
        "affinity_vector_buffer_id",
        {"narreme_affinity_vector_buffer_id", "", ""},
        sizeof(godot::Vector4), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_TYPE,
        "narreme_type_buffer_id",
        {"candidate_type_buffer_id", "symbolic_type_buffer_id", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_IS_SYMBOLIC,
        "is_symbolic_buffer_id",
        {"narreme_is_symbolic_buffer_id", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_LOCATION_ID,
        "location_id_buffer_id",
        {"current_location_id_buffer_id", "narreme_location_id_buffer_id", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_POSSESSOR_ID,
        "possessor_id_buffer_id",
        {"owner_character_id_buffer_id", "narreme_possessor_id_buffer_id", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_STRUCTURAL_SIG,
        "structural_sig_buffer_id",
        {"narreme_structural_sig_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_CANDIDATE_MASK,
        "candidate_mask_buffer_id",
        {"narreme_candidate_mask_buffer_id", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_CANDIDATE_SCORE,
        "output_score_buffer_id",
        {"candidate_score_buffer_id", "narreme_candidate_score_buffer_id", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_PENDING_LOCATION_ID,
        "pending_location_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_PENDING_POSSESSOR_ID,
        "pending_possessor_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_PREVIOUS_LOCATION_ID,
        "previous_location_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_PREVIOUS_POSSESSOR_ID,
        "previous_possessor_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_SPATIAL_TRANSITION_FLAGS,
        "spatial_transition_flags_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::LOCATION_CHAR_COUNT,
        "location_char_count_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::LOCATION_PROP_COUNT,
        "location_prop_count_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::LOCATION_PARENT_ID,
        "location_parent_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::LOCATION_CSR_OFFSETS,
        "location_csr_offsets_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RELATIONSHIP_OFFSETS, NarrativeTableGroup::RELATIONSHIP_CSR_ROW
    },
    {
        NarrativeBufferSlot::LOCATION_CSR_NEIGHBORS,
        "location_csr_neighbors_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::LOCATION_NEIGHBORS, NarrativeTableGroup::LOCATION_NEIGHBOR_SLICE
    },
    {
        NarrativeBufferSlot::LOCATION_SELECTION,
        "location_selection_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::CHARACTER_GOAL_STATUS,
        "character_goal_status_buffer_id",
        {"character_goal_wake_buffer_id", "", ""},
        sizeof(int8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::CHARACTER_PRIMARY_GOAL_PLOT_ID,
        "primary_goal_plot_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::CHARACTER_GOAL_MASK,
        "character_goal_mask_buffer_id",
        {"goal_or_occupancy_mask_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::CHARACTER_GOAL_SLICE_OFFSET,
        "goal_slice_offset_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::CHARACTER_GOAL_SLICE_COUNT,
        "goal_slice_count_buffer_id",
        {"", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::CHARACTER_GOAL_PLOT_ID,
        "goal_plot_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CHARACTER_GOALS, NarrativeTableGroup::CHARACTER_GOAL_SLICE
    },
    {
        NarrativeBufferSlot::CHARACTER_GOAL_TRANSITION_FLAGS,
        "goal_transition_flags_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_OBJECT_ID,
        "narreme_object_id_buffer_id",
        {"", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::NARREME_TELEMETRY_SNAPSHOT,
        "telemetry_snapshot_buffer_id",
        {"narrative_telemetry_snapshot_buffer_id", "", ""},
        sizeof(NarrativeTelemetrySnapshotPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },

    // ========================================================================
    // Table 2: SymbolicAssignmentMapBuffer
    // ========================================================================
    {
        NarrativeBufferSlot::SYMBOLIC_ASSIGNMENT_POD,
        "symbolic_assignment_pod_buffer_id",
        {"", "", ""},
        sizeof(SymbolicAssignmentRecordPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        NarrativeCapacityKind::SYMBOLIC_SLOTS, NarrativeTableGroup::SYMBOLIC_ASSIGNMENT
    },
    {
        NarrativeBufferSlot::SYMBOLIC_TO_LITERAL,
        "symbolic_to_literal_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::SYMBOLIC_SLOTS, NarrativeTableGroup::SYMBOLIC_ASSIGNMENT
    },
    {
        NarrativeBufferSlot::LITERAL_TO_SYMBOLIC,
        "literal_to_symbolic_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::ASSIGNMENT_FIT_SCORE,
        "assignment_fit_score_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::SYMBOLIC_SLOTS, NarrativeTableGroup::SYMBOLIC_ASSIGNMENT
    },
    {
        NarrativeBufferSlot::ASSIGNMENT_FIT_MODE,
        "assignment_fit_mode_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::SYMBOLIC_SLOTS, NarrativeTableGroup::SYMBOLIC_ASSIGNMENT
    },
    {
        NarrativeBufferSlot::ASSIGNMENT_REPLACE_POLICY,
        "assignment_replace_buffer_id",
        {"assignment_replace_policy_buffer_id", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::SYMBOLIC_SLOTS, NarrativeTableGroup::SYMBOLIC_ASSIGNMENT
    },
    {
        NarrativeBufferSlot::PREVIOUS_LITERAL_ID,
        "previous_literal_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::SYMBOLIC_SLOTS, NarrativeTableGroup::SYMBOLIC_ASSIGNMENT
    },
    {
        NarrativeBufferSlot::ASSIGNMENT_DIRTY_FLAGS,
        "assignment_dirty_flags_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::SYMBOLIC_SLOTS, NarrativeTableGroup::SYMBOLIC_ASSIGNMENT
    },
    {
        NarrativeBufferSlot::SYMBOLIC_TITLE_TOKEN,
        "symbolic_title_token_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::SYMBOLIC_SLOTS, NarrativeTableGroup::SYMBOLIC_ASSIGNMENT
    },

    // ========================================================================
    // Table 3: NarrativeRelationshipCSRBuffer
    // ========================================================================
    {
        NarrativeBufferSlot::RELATIONSHIP_ROW_OFFSETS,
        "relationship_row_offsets_buffer_id",
        {"rel_csr_offset_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RELATIONSHIP_OFFSETS, NarrativeTableGroup::RELATIONSHIP_CSR_ROW
    },
    {
        NarrativeBufferSlot::RELATIONSHIP_ROW_COUNTS,
        "rel_csr_count_buffer_id",
        {"relationship_row_counts_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },
    {
        NarrativeBufferSlot::RELATIONSHIP_EDGE_POD,
        "relationship_edges_buffer_id",
        {"relationship_edge_pod_buffer_id", "rel_csr_edge_pod_buffer_id", ""},
        sizeof(RelationshipEdgePOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        NarrativeCapacityKind::RELATIONSHIP_EDGES, NarrativeTableGroup::RELATIONSHIP_CSR_EDGE
    },
    {
        NarrativeBufferSlot::RELATIONSHIP_SOURCE_SELECTION,
        "source_selection_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::NARREMES, NarrativeTableGroup::NARREME_IDENTITY
    },

    // ========================================================================
    // Table 4: ConditionEvaluationTableBuffer
    // ========================================================================
    {
        NarrativeBufferSlot::CONDITION_ROW_POD,
        "condition_row_buffer_id",
        {"condition_row_pod_buffer_id", "", ""},
        sizeof(ConditionEvaluationRowPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_SUBJECT_ID,
        "subject_id_buffer_id",
        {"condition_subject_id_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_OBJECT_ID,
        "object_id_buffer_id",
        {"condition_object_id_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_RESOLVED_SUBJECT_ID,
        "resolved_subject_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_RESOLVED_OBJECT_ID,
        "resolved_object_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_SUBJECT_IS_SYMBOLIC,
        "subject_is_symbolic_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_OBJECT_IS_SYMBOLIC,
        "object_is_symbolic_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_PREDICATE_OP,
        "predicate_op_buffer_id",
        {"condition_predicate_op_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_TYPE,
        "condition_type_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_EXPECTED_STATUS,
        "condition_expected_status_buffer_id",
        {"", "", ""},
        sizeof(int8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_STATUS,
        "condition_status_buffer_id",
        {"write_status_buffer_id", "", ""},
        sizeof(int8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_READ_STATUS,
        "read_status_buffer_id",
        {"condition_read_status_buffer_id", "", ""},
        sizeof(int8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CONDITION_TRANSITION_FLAGS,
        "transition_flags_buffer_id",
        {"condition_transition_flags_buffer_id", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CAUSAL_SLICE_OFFSET,
        "causal_slice_offset_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CAUSAL_SLICE_COUNT,
        "causal_slice_count_buffer_id",
        {"", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CONDITIONS, NarrativeTableGroup::CONDITION_TABLE
    },
    {
        NarrativeBufferSlot::CAUSAL_LINK_POD,
        "causal_link_pod_buffer_id",
        {"", "", ""},
        sizeof(CausalLinkPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        NarrativeCapacityKind::CAUSAL_LINKS, NarrativeTableGroup::CAUSAL_LINK_SLICE
    },
    {
        NarrativeBufferSlot::CAUSAL_CHILD_CONDITION_ID,
        "child_condition_id_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CAUSAL_LINKS, NarrativeTableGroup::CAUSAL_LINK_SLICE
    },
    {
        NarrativeBufferSlot::CAUSAL_EXPECTED_CAUSE_STATUS,
        "expected_cause_status_buffer_id",
        {"", "", ""},
        sizeof(int8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::CAUSAL_LINKS, NarrativeTableGroup::CAUSAL_LINK_SLICE
    },
    {
        NarrativeBufferSlot::CAUSAL_CHANGED_COUNT,
        "changed_count_buffer_id",
        {"causal_changed_count_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RING_CURSOR_OR_COUNTER, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::GAMEPLAY_TRIGGER_OR_STATUS,
        "trigger_or_status_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::BRIDGED_PROPERTIES, NarrativeTableGroup::BRIDGED_GAMEPLAY_PROPERTY
    },
    {
        NarrativeBufferSlot::BRIDGED_PROP_MODDED_VALUE,
        "prop_modded_value_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::BRIDGED_PROPERTIES, NarrativeTableGroup::BRIDGED_GAMEPLAY_PROPERTY
    },
    {
        NarrativeBufferSlot::BRIDGED_PROP_MODDED_MIN,
        "prop_modded_min_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::BRIDGED_PROPERTIES, NarrativeTableGroup::BRIDGED_GAMEPLAY_PROPERTY
    },
    {
        NarrativeBufferSlot::BRIDGED_PROP_MODDED_MAX,
        "prop_modded_max_buffer_id",
        {"", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::BRIDGED_PROPERTIES, NarrativeTableGroup::BRIDGED_GAMEPLAY_PROPERTY
    },
    {
        NarrativeBufferSlot::BRIDGED_PROP_FLAGS,
        "prop_flags_buffer_id",
        {"", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::BRIDGED_PROPERTIES, NarrativeTableGroup::BRIDGED_GAMEPLAY_PROPERTY
    },

    // ========================================================================
    // Table 5: IncidentAndPlotStateBuffer
    // ========================================================================
    {
        NarrativeBufferSlot::INCIDENT_STATUS,
        "incident_status_buffer_id",
        {"incident_or_plot_status_buffer_id", "", ""},
        sizeof(int8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENTS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::INCIDENT_COND_SLICE_OFFSET,
        "cond_slice_offset_buffer_id",
        {"prereq_cond_offset_buffer_id", "incident_cond_slice_offset_buffer_id", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENTS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::INCIDENT_COND_SLICE_COUNT,
        "cond_slice_count_buffer_id",
        {"prereq_cond_count_buffer_id", "incident_cond_slice_count_buffer_id", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENTS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::INCIDENT_COND_INDEX_TABLE,
        "incident_cond_id_buffer_id",
        {"condition_index_table_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENT_CONDITION_REFS, NarrativeTableGroup::INCIDENT_COND_SLICE
    },
    {
        NarrativeBufferSlot::INCIDENT_CAUSAL_COND_OFFSET,
        "causal_cond_offset_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENTS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::INCIDENT_CAUSAL_COND_COUNT,
        "causal_cond_count_buffer_id",
        {"", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENTS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::INCIDENT_PARENT_CSR_OFFSETS,
        "parent_csr_offsets_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENT_CSR_OFFSETS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::INCIDENT_PARENT_CSR_IDS,
        "parent_csr_ids_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENT_CONDITION_REFS, NarrativeTableGroup::INCIDENT_COND_SLICE
    },
    {
        NarrativeBufferSlot::INCIDENT_CAUSAL_DEPTH,
        "output_depth_buffer_id",
        {"incident_causal_depth_buffer_id", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENTS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::INCIDENT_TRANSITION_FLAGS,
        "incident_transition_flags_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENTS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::INCIDENT_RESOLVED_COUNT,
        "resolved_count_buffer_id",
        {"incident_resolved_count_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RING_CURSOR_OR_COUNTER, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::UNRESOLVED_SYMBOLIC_COUNT,
        "unresolved_symbolic_count_buf_id",
        {"unresolved_symbolic_count_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::INCIDENTS, NarrativeTableGroup::INCIDENT_STATE
    },
    {
        NarrativeBufferSlot::PLOT_STATUS,
        "plot_status_buffer_id",
        {"status_buffer_id", "dependent_plot_wake_buffer_id", ""},
        sizeof(int8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOTS, NarrativeTableGroup::PLOT_STATE
    },
    {
        NarrativeBufferSlot::PLOT_EVENT_SLICE_OFFSET,
        "event_slice_offset_buffer_id",
        {"plot_event_offset_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOTS, NarrativeTableGroup::PLOT_STATE
    },
    {
        NarrativeBufferSlot::PLOT_EVENT_SLICE_COUNT,
        "event_slice_count_buffer_id",
        {"plot_event_count_buffer_id", "", ""},
        sizeof(uint16_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOTS, NarrativeTableGroup::PLOT_STATE
    },
    {
        NarrativeBufferSlot::PLOT_EVENT_POD,
        "plot_event_pod_buffer_id",
        {"plot_events_buffer_id", "", ""},
        sizeof(PlotEventPOD), BufferLayoutType::AOS, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOT_EVENTS, NarrativeTableGroup::PLOT_EVENT_SLICE
    },
    {
        NarrativeBufferSlot::PLOT_TRANSITION_FLAGS,
        "plot_transition_flags_buffer_id",
        {"", "", ""},
        sizeof(uint8_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOTS, NarrativeTableGroup::PLOT_STATE
    },
    {
        NarrativeBufferSlot::PLOT_RESOLVED_COUNT,
        "resolved_plot_count_buffer_id",
        {"plot_resolved_count_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RING_CURSOR_OR_COUNTER, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::PLOT_PARTICIPANT_MASK,
        "participant_mask_buffer_id",
        {"plot_participant_mask_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOTS, NarrativeTableGroup::PLOT_STATE
    },
    {
        NarrativeBufferSlot::PLOT_PRIMARY_LOCATION_ID,
        "primary_location_id_buffer_id",
        {"plot_primary_location_id_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOTS, NarrativeTableGroup::PLOT_STATE
    },
    {
        NarrativeBufferSlot::PLOT_GROUP_MASK,
        "group_mask_buffer_id",
        {"plot_group_mask_buffer_id", "", ""},
        sizeof(uint64_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOTS, NarrativeTableGroup::PLOT_STATE
    },
    {
        NarrativeBufferSlot::PLOT_CLUSTER_ID,
        "output_cluster_id_buffer_id",
        {"plot_cluster_id_buffer_id", "", ""},
        sizeof(int32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::PLOTS, NarrativeTableGroup::PLOT_STATE
    },

    // ========================================================================
    // 4 Event Rings & Ring Cursors
    // ========================================================================
    {
        NarrativeBufferSlot::SPATIAL_TRANSFER_COMMAND_RING,
        "transfer_command_ring_buffer_id",
        {"spatial_transfer_command_ring_buffer_id", "", ""},
        sizeof(NarremeSpatialTransferCommandPOD), BufferLayoutType::RING, SelectionMode::RANGE,
        NarrativeCapacityKind::SPATIAL_TRANSFER_RING, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::SPATIAL_TRANSFER_RING_CURSOR,
        "transfer_ring_cursor_buffer_id",
        {"ring_cursor_buffer_id", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RING_CURSOR_OR_COUNTER, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::GAMEPLAY_CONDITION_EVENT_RING,
        "gameplay_event_ring_buffer_id",
        {"gameplay_condition_event_ring_buffer_id", "", ""},
        sizeof(GameplayConditionEventPOD), BufferLayoutType::RING, SelectionMode::RANGE,
        NarrativeCapacityKind::GAMEPLAY_EVENT_RING, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::GAMEPLAY_EVENT_RING_CURSOR,
        "gameplay_ring_cursor_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RING_CURSOR_OR_COUNTER, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::CHARACTER_GOAL_EVENT_RING,
        "goal_event_ring_buffer_id",
        {"character_goal_event_ring_buffer_id", "", ""},
        sizeof(CharacterGoalEventPOD), BufferLayoutType::RING, SelectionMode::RANGE,
        NarrativeCapacityKind::CHARACTER_GOAL_EVENT_RING, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::CHARACTER_GOAL_RING_CURSOR,
        "goal_ring_cursor_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RING_CURSOR_OR_COUNTER, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::NARRATIVE_SIGNAL_COMMAND_RING,
        "signal_command_ring_buffer_id",
        {"narrative_signal_command_ring_buffer_id", "", ""},
        sizeof(NarrativeSignalCommandPOD), BufferLayoutType::RING, SelectionMode::RANGE,
        NarrativeCapacityKind::SIGNAL_COMMAND_RING, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    },
    {
        NarrativeBufferSlot::NARRATIVE_SIGNAL_RING_CURSOR,
        "signal_ring_cursor_buffer_id",
        {"", "", ""},
        sizeof(uint32_t), BufferLayoutType::FLAT, SelectionMode::RANGE,
        NarrativeCapacityKind::RING_CURSOR_OR_COUNTER, NarrativeTableGroup::EVENT_RING_OR_COUNTER
    }
}};

[[nodiscard]] consteval bool validate_narrative_buffer_specs() noexcept {
    for (size_t i = 0; i < NARRATIVE_BUFFER_SLOT_COUNT; ++i) {
        if (static_cast<size_t>(NARRATIVE_BUFFER_SPECS[i].slot) != i) {
            return false;
        }
        if (NARRATIVE_BUFFER_SPECS[i].canonical_name.empty() || NARRATIVE_BUFFER_SPECS[i].element_stride == 0u) {
            return false;
        }
    }
    return true;
}

static_assert(validate_narrative_buffer_specs(), "NARRATIVE_BUFFER_SPECS indices must match NarrativeBufferSlot enum order!");

/**
 * NarrativesBufferManifest
 * Table-driven directory of all MemoryManagerDOD buffer IDs backing the 5 Section 3.2
 * tables and 4 event rings. Supports O(1) indexed lookup by NarrativeBufferSlot and
 * constexpr loop-based resolution by semantic property name.
 */
struct NarrativesBufferManifest {
    std::array<uint32_t, NARRATIVE_BUFFER_SLOT_COUNT> buffer_ids = []() consteval {
        std::array<uint32_t, NARRATIVE_BUFFER_SLOT_COUNT> arr{};
        arr.fill(INVALID_ID);
        return arr;
    }();

    [[nodiscard]] constexpr uint32_t& operator[](NarrativeBufferSlot p_slot) noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr const uint32_t& operator[](NarrativeBufferSlot p_slot) const noexcept {
        return buffer_ids[static_cast<size_t>(p_slot)];
    }

    [[nodiscard]] constexpr uint32_t get(NarrativeBufferSlot p_slot) const noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        return (idx < NARRATIVE_BUFFER_SLOT_COUNT) ? buffer_ids[idx] : INVALID_ID;
    }

    constexpr void set(NarrativeBufferSlot p_slot, uint32_t p_buffer_id) noexcept {
        const size_t idx = static_cast<size_t>(p_slot);
        if (idx < NARRATIVE_BUFFER_SLOT_COUNT) {
            buffer_ids[idx] = p_buffer_id;
        }
    }

    constexpr void reset() noexcept {
        buffer_ids.fill(INVALID_ID);
    }

    [[nodiscard]] constexpr uint32_t get_by_name(std::string_view p_name) const noexcept {
        for (size_t i = 0; i < NARRATIVE_BUFFER_SLOT_COUNT; ++i) {
            if (NARRATIVE_BUFFER_SPECS[i].matches_name(p_name)) {
                return buffer_ids[i];
            }
        }
        return INVALID_ID;
    }

    [[nodiscard]] constexpr bool is_initialized() const noexcept {
        return get(NarrativeBufferSlot::NARREME_TYPE) != INVALID_ID &&
               get(NarrativeBufferSlot::SYMBOLIC_TO_LITERAL) != INVALID_ID &&
               get(NarrativeBufferSlot::RELATIONSHIP_EDGE_POD) != INVALID_ID &&
               get(NarrativeBufferSlot::CONDITION_STATUS) != INVALID_ID &&
               get(NarrativeBufferSlot::INCIDENT_STATUS) != INVALID_ID &&
               get(NarrativeBufferSlot::PLOT_STATUS) != INVALID_ID;
    }
};

using NarrativeBufferManifest = NarrativesBufferManifest;

/**
 * NarremeIdentityRowPOD
 * Convenience descriptor for reading/writing a complete row across the
 * NarremeIdentityAndTraitBuffer (Section 3.2 Table 1) columns.
 */
struct NarremeIdentityRowPOD {
    uint64_t trait_bitmask_lo    = 0ULL;
    uint64_t trait_bitmask_hi    = 0ULL;
    godot::Vector4 affinity      = godot::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    uint32_t location_id         = INVALID_ID;
    uint32_t possessor_id        = INVALID_ID;
    uint64_t structural_sig      = 0ULL;
    uint64_t character_goal_mask = 0ULL;
    uint32_t primary_goal_plot_id= INVALID_ID;
    uint64_t object_id           = 0ULL;
    uint8_t narreme_type         = NARREME_TYPE_CHARACTER;
    uint8_t is_symbolic          = 0u;
    uint8_t candidate_mask       = 0u;
};

/**
 * IncidentStateRowPOD
 * Convenience descriptor for reading/writing an Incident row across the
 * IncidentAndPlotStateBuffer (Section 3.2 Table 5) columns.
 */
struct IncidentStateRowPOD {
    uint32_t cond_slice_offset        = 0u;
    uint16_t cond_slice_count         = 0u;
    uint32_t causal_cond_offset       = 0u;
    uint16_t causal_cond_count        = 0u;
    int32_t causal_depth              = 0;
    uint16_t unresolved_symbolic_count= 0u;
    int8_t status                     = INCIDENT_STATUS_HAS_NOT_HAPPENED;
    uint8_t transition_flags          = INCIDENT_TRANSITION_NONE;
};

/**
 * PlotProgressionRowPOD
 * Convenience descriptor for reading/writing a Plot row across the
 * IncidentAndPlotStateBuffer (Section 3.2 Table 5) columns.
 */
struct PlotProgressionRowPOD {
    uint32_t event_slice_offset = 0u;
    uint16_t event_slice_count  = 0u;
    uint64_t participant_mask   = 0ULL;
    uint32_t primary_location_id= INVALID_ID;
    uint64_t group_mask         = 0ULL;
    int32_t cluster_id          = -1;
    int8_t status               = PLOT_STATUS_NOT_BEGUN;
    uint8_t transition_flags    = PLOT_TRANSITION_NONE;
};

} // namespace ideam::core