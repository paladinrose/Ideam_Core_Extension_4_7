#pragma once

/**
 * games_dod_common.h
 * Foundational pure C++26 data structures, buffer manifests, slot allocators,
 * simulation tier enums, and flexible TaskGraph topology anchors for the
 * IdeamGames <-> IdeamTasks Data-Oriented Design (DOD) bridge.
 *
 * Implements the 5 core contiguous memory table schemas defined in Section 2.2
 * of the Games Tasks Bridge specification:
 *   1. EntityHierarchyBuffer
 *   2. GamePropertyBuffer
 *   3. PropertyModifierBuffer
 *   4. ActionSequenceBuffer
 *   5. InteractionArenaBuffer (+ ConsequenceCommandPOD ring buffer)
 */

#include <cstring>
#include <cstdint>
#include <cstddef>

#include "../domain_dod_common.h"
#include "../../../memory/memory_common.h"
#include "../../../memory/memory_buffer_pod.h"
#include "../../../memory/memory_grant_pod.h"
#include "../../../memory/memory_manager_dod.h"
#include "../../../graphs/ideam_graph_dod.h"
#include "../../registration/ideam_task_registry.h"

// Include the 18 Games domain logic headers so all shared PODs & enums are available
#include "metadata_logic/board_presence_partition_metadata_logic.h"
#include "metadata_logic/gameplay_rule_specificity_metadata_logic.h"
#include "query_logic/action_competition_conflict_query_logic.h"
#include "query_logic/agent_exclusivity_filter_query_logic.h"
#include "query_logic/gameplay_rule_selector_query_logic.h"
#include "query_logic/property_exhausted_or_locked_query_logic.h"
#include "transform_logic/agent_action_sequence_step_transform_logic.h"
#include "transform_logic/agent_possession_transform_blend_transform_logic.h"
#include "transform_logic/consequence_ring_drain_transform_logic.h"
#include "transform_logic/control_map_input_filter_transform_logic.h"
#include "transform_logic/entity_time_scale_cascade_transform_logic.h"
#include "transform_logic/game_entity_snapshot_sync_transform_logic.h"
#include "transform_logic/gameplay_rule_apply_transform_logic.h"
#include "transform_logic/interaction_tug_of_war_resolve_transform_logic.h"
#include "transform_logic/piece_action_score_refresh_transform_logic.h"
#include "transform_logic/property_modifier_aggregate_transform_logic.h"
#include "transform_logic/property_modifier_tick_transform_logic.h"
#include "transform_logic/property_range_clamp_and_exhaust_transform_logic.h"

#include <cstdint>
#include <cstddef>
#include <vector>
#include <span>
#include <string_view>
#include <algorithm>

namespace ideam::core {

/**
 * EntitySimulationTier
 * Defines where a GameEntity currently resides along the OOP <-> DOD simulation spectrum.
 * Entities can seamlessly promote or demote between tiers at runtime (e.g., based on camera
 * distance, player proximity, board residency, or developer-defined LOD rules).
 */
enum EntitySimulationTier : uint8_t {
    SIM_TIER_PURE_DOD       = 0, // Simulated 100% in DOD buffers; no SceneTree Node required
    SIM_TIER_DOD_RENDERED   = 1, // Simulated in DOD buffers and rendered via DOD/MultiMesh/GPU tasks
    SIM_TIER_HYBRID_SYNCED  = 2, // Simulated in DOD buffers and synced to/from OOP SceneTree Nodes / Gauges
    SIM_TIER_FULL_OOP       = 3  // Simulated in OOP SceneTree Nodes with full Godot signals; DOD row acts as mirror/telemetry
};

/**
 * GamesPipelineStageFlags
 * Composable bitmask flags representing logical stages in the Games DOD simulation.
 * Developers can pass any combination of flags to GameDOD::inject_pipeline_stages(),
 * or omit stages entirely and build custom TaskGraphDOD topologies using GameDOD's
 * individual node factories.
 */
enum GamesPipelineStageFlags : uint32_t {
    GAMES_STAGE_NONE                  = 0u,
    GAMES_STAGE_SNAPSHOT_PULL         = 1u << 0,  // Pull dirty OOP GameEntity/GameProperty state into DOD columns
    GAMES_STAGE_INPUT_AND_POSSESSION  = 1u << 1,  // ControlMapInputFilter + AgentPossessionTransformBlend + AgentExclusivityFilter
    GAMES_STAGE_HIERARCHY_AND_TIME    = 1u << 2,  // EntityTimeScaleCascade + BoardPresencePartition
    GAMES_STAGE_MODIFIER_TICK         = 1u << 3,  // PropertyModifierTick
    GAMES_STAGE_MODIFIER_AGGREGATE    = 1u << 4,  // PropertyModifierAggregate
    GAMES_STAGE_GAMEPLAY_RULES        = 1u << 5,  // GameplayRuleSpecificity + GameplayRuleSelector + GameplayRuleApply
    GAMES_STAGE_PROPERTY_CLAMP        = 1u << 6,  // PropertyRangeClampAndExhaust + PropertyExhaustedOrLockedQuery
    GAMES_STAGE_PIECE_ACTIONS         = 1u << 7,  // PieceActionScoreRefresh
    GAMES_STAGE_AGENT_ACTIONS         = 1u << 8,  // ActionCompetitionConflictQuery + AgentActionSequenceStep
    GAMES_STAGE_INTERACTION_RESOLVE   = 1u << 9,  // InteractionTugOfWarResolve
    GAMES_STAGE_CONSEQUENCE_DRAIN     = 1u << 10, // ConsequenceRingDrain
    GAMES_STAGE_SNAPSHOT_PUSH         = 1u << 11, // Build PropertyTelemetrySnapshotPOD & push solved state to OOP / Gauges

    // Preset masks for common execution paradigms:
    GAMES_STAGE_CORE_SIMULATION_MASK  =
        GAMES_STAGE_HIERARCHY_AND_TIME   |
        GAMES_STAGE_MODIFIER_TICK        |
        GAMES_STAGE_MODIFIER_AGGREGATE   |
        GAMES_STAGE_GAMEPLAY_RULES       |
        GAMES_STAGE_PROPERTY_CLAMP       |
        GAMES_STAGE_PIECE_ACTIONS        |
        GAMES_STAGE_AGENT_ACTIONS        |
        GAMES_STAGE_INTERACTION_RESOLVE  |
        GAMES_STAGE_CONSEQUENCE_DRAIN,

    GAMES_STAGE_PURE_DOD_MASK         =
        GAMES_STAGE_INPUT_AND_POSSESSION |
        GAMES_STAGE_CORE_SIMULATION_MASK,

    GAMES_STAGE_HYBRID_DEFAULT_MASK   =
        GAMES_STAGE_SNAPSHOT_PULL        |
        GAMES_STAGE_INPUT_AND_POSSESSION |
        GAMES_STAGE_CORE_SIMULATION_MASK |
        GAMES_STAGE_SNAPSHOT_PUSH,

    GAMES_STAGE_ALL_MASK              = 0x0FFFu
};

/**
 * GamesNodeExecutionConfig
 * Optional overrides when instantiating any Games task node in a TaskGraphDOD.
 * Allows developers to customize the View, Strategy, DataType, Kahn priority,
 * or GrantPartPOD requirements without losing automatic buffer ID binding.
 */
struct GamesNodeExecutionConfig {
    MemoryView view                    = MemoryView::SingleElementView;
    MemoryStrategy strategy            = MemoryStrategy::FlatStrategy;
    MemoryTypes element_type           = MemoryTypes::INT32;
    int32_t execution_priority         = 0;
    bool use_registry_factory          = false; // If false, uses direct C++ template instantiation (zero Dictionary overhead)
    std::span<const GrantPartPOD> custom_requirements{};
};

/**
 * GamesStageAnchor
 * Topological handle returned for each injected stage in a TaskGraphDOD.
 * Exposes entry_node, exit_node, and all internal nodes so developers can wire
 * custom tasks in parallel within the same Kahn wave or splice custom tasks between stages.
 */
struct GamesStageAnchor {
    GamesPipelineStageFlags stage = GAMES_STAGE_NONE;
    NodeID entry_node             = INVALID_ID;
    NodeID exit_node              = INVALID_ID;
    std::vector<NodeID> nodes;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return entry_node != INVALID_ID && exit_node != INVALID_ID;
    }
};

/**
 * GamesPipelineTopology
 * Describes the set of GamesStageAnchors injected into a TaskGraphDOD.
 */
struct GamesPipelineTopology {
    NodeID first_node = INVALID_ID;
    NodeID last_node  = INVALID_ID;
    std::vector<GamesStageAnchor> stages;

    [[nodiscard]] const GamesStageAnchor* get_stage_anchor(GamesPipelineStageFlags p_stage) const noexcept {
        for (const auto& anchor : stages) {
            if (anchor.stage == p_stage) {
                return &anchor;
            }
        }
        return nullptr;
    }

    [[nodiscard]] bool has_stage(GamesPipelineStageFlags p_stage) const noexcept {
        const GamesStageAnchor* anchor = get_stage_anchor(p_stage);
        return anchor != nullptr && anchor->is_valid();
    }
};

// Note: SlotIndexAllocator and make_grant_requirement() are provided by "../domain_dod_common.h".

/**
 * GamesBufferCapacities
 * Configurable row capacities for the 5 Section 2.2 buffer tables and auxiliary arenas.
 */
struct GamesBufferCapacities {
    uint32_t max_entities                 = 4096;
    uint32_t max_properties               = 16384;
    uint32_t max_modifiers                = 32768;
    uint32_t max_actions                  = 8192;
    uint32_t max_sequence_steps           = 16384;
    uint32_t max_piece_bindings           = 16384;
    uint32_t max_interactions             = 2048;
    uint32_t max_interaction_participants = 8192;
    uint32_t max_compiled_rules           = 1024;
    uint32_t max_consequence_commands     = 4096;
    uint32_t max_players                  = 64;
};

/**
 * GamesBufferManifest
 * Central directory of all MemoryManagerDOD buffer IDs backing the 5 Section 2.2
 * logical buffer tables and their constituent columns / POD arenas.
 * Every buffer ID can be bound directly into any of the 18 Games logic structs
 * or referenced by custom user TaskGraphResource nodes.
 */
struct GamesBufferManifest {
    // --- Table 1: EntityHierarchyBuffer Columns ---
    uint32_t entity_parent_id_buffer_id           = INVALID_ID; // uint32_t
    uint32_t entity_board_id_buffer_id            = INVALID_ID; // int32_t
    uint32_t entity_archetype_mask_buffer_id      = INVALID_ID; // uint32_t (EntityArchetypeMask)
    uint32_t entity_group_mask_buffer_id          = INVALID_ID; // uint64_t (64-bit group bitmask)
    uint32_t entity_active_flags_buffer_id        = INVALID_ID; // uint32_t (EntityActiveFlags)
    uint32_t entity_local_time_scale_buffer_id    = INVALID_ID; // float
    uint32_t entity_effective_time_scale_buffer_id= INVALID_ID; // float
    uint32_t entity_controlling_player_id_buffer_id = INVALID_ID; // int32_t
    uint32_t entity_exclusivity_buffer_id         = INVALID_ID; // uint8_t (GameAgentExclusivityMode)
    uint32_t entity_object_id_buffer_id           = INVALID_ID; // uint64_t (Godot ObjectID, 0 if pure DOD)
    uint32_t entity_consequence_mask_buffer_id    = INVALID_ID; // uint32_t (Last applied ConsequenceMaskFlags)
    uint32_t entity_sim_tier_buffer_id            = INVALID_ID; // uint8_t (EntitySimulationTier)

    // --- Table 2: GamePropertyBuffer Columns ---
    uint32_t prop_owner_entity_id_buffer_id       = INVALID_ID; // uint32_t
    uint32_t prop_name_id_buffer_id               = INVALID_ID; // uint32_t (Interned property name token)
    uint32_t prop_base_value_buffer_id            = INVALID_ID; // int32_t
    uint32_t prop_modded_value_buffer_id          = INVALID_ID; // int32_t
    uint32_t prop_base_min_buffer_id              = INVALID_ID; // int32_t
    uint32_t prop_modded_min_buffer_id            = INVALID_ID; // int32_t
    uint32_t prop_base_max_buffer_id            = INVALID_ID; // int32_t
    uint32_t prop_modded_max_buffer_id            = INVALID_ID; // int32_t
    uint32_t prop_flags_buffer_id                 = INVALID_ID; // uint16_t (GamePropertyFlags)
    uint32_t prop_object_id_buffer_id             = INVALID_ID; // uint64_t (Godot GameProperty ObjectID)
    uint32_t prop_gauge_object_id_buffer_id       = INVALID_ID; // uint64_t (Godot GamePropertyGauge ObjectID)
    uint32_t prop_telemetry_snapshot_buffer_id    = INVALID_ID; // PropertyTelemetrySnapshotPOD (28B)
    uint32_t prop_exhaustion_event_buffer_id      = INVALID_ID; // PropertyExhaustionEventPOD (16B)

    // --- Table 3: PropertyModifierBuffer Columns & AoS POD Table ---
    uint32_t modifier_pod_buffer_id               = INVALID_ID; // PropertyModifierPOD (32B aligned AoS)
    uint32_t mod_target_property_idx_buffer_id    = INVALID_ID; // uint32_t (SoA mirror when using SoA views)
    uint32_t mod_value_buffer_id                  = INVALID_ID; // int32_t
    uint32_t mod_starting_value_buffer_id         = INVALID_ID; // int32_t
    uint32_t mod_time_limit_buffer_id             = INVALID_ID; // float
    uint32_t mod_current_time_buffer_id           = INVALID_ID; // float
    uint32_t mod_type_buffer_id                   = INVALID_ID; // uint8_t (PropertyModifierType)
    uint32_t mod_channel_buffer_id                = INVALID_ID; // uint8_t (PropertyModifierChannel)
    uint32_t mod_flags_buffer_id                  = INVALID_ID; // uint8_t (PropertyModifierFlags)

    // --- Table 4: ActionSequenceBuffer Columns & Sub-Tables ---
    uint32_t action_owner_agent_id_buffer_id      = INVALID_ID; // uint32_t
    uint32_t action_base_score_buffer_id          = INVALID_ID; // int32_t
    uint32_t action_current_score_buffer_id       = INVALID_ID; // int32_t
    uint32_t action_priority_buffer_id            = INVALID_ID; // int32_t
    uint32_t action_status_buffer_id              = INVALID_ID; // int32_t (GamePieceActionStatus)
    uint32_t action_group_mask_buffer_id          = INVALID_ID; // uint64_t
    uint32_t action_competition_mask_buffer_id    = INVALID_ID; // uint64_t
    uint32_t action_refresh_time_buffer_id        = INVALID_ID; // float
    uint32_t action_current_time_buffer_id        = INVALID_ID; // float
    uint32_t action_current_step_idx_buffer_id    = INVALID_ID; // int32_t
    uint32_t action_step_offset_buffer_id         = INVALID_ID; // uint32_t
    uint32_t action_step_count_buffer_id          = INVALID_ID; // uint32_t
    uint32_t action_sequence_step_pod_buffer_id   = INVALID_ID; // SequenceStepPOD (8B)
    uint32_t action_binding_offset_buffer_id      = INVALID_ID; // uint32_t
    uint32_t action_binding_count_buffer_id       = INVALID_ID; // uint32_t
    uint32_t action_binding_pod_buffer_id         = INVALID_ID; // PiecePropertyBindingPOD (12B)

    // --- Table 5: InteractionArenaBuffer & Consequence Ring ---
    uint32_t interaction_instigator_action_idx_buffer_id = INVALID_ID; // uint32_t
    uint32_t interaction_instigator_score_buffer_id      = INVALID_ID; // int32_t
    uint32_t interaction_competition_score_buffer_id     = INVALID_ID; // int32_t
    uint32_t interaction_score_diff_buffer_id            = INVALID_ID; // int32_t
    uint32_t interaction_instigator_weight_buffer_id     = INVALID_ID; // float
    uint32_t interaction_competition_weight_buffer_id    = INVALID_ID; // float
    uint32_t interaction_participant_offset_buffer_id    = INVALID_ID; // uint32_t
    uint32_t interaction_participant_count_buffer_id     = INVALID_ID; // uint32_t
    uint32_t interaction_participant_pod_buffer_id       = INVALID_ID; // InteractionParticipantPOD (16B)
    uint32_t consequence_ring_buffer_id                  = INVALID_ID; // ConsequenceCommandPOD (12B Ring)
    uint32_t consequence_ring_cursor_buffer_id           = INVALID_ID; // uint32_t[2] {write_idx, read_idx}
    uint32_t consequence_drained_count_buffer_id         = INVALID_ID; // uint32_t

    // --- Auxiliary Gameplay Rule, Possession & Input Tables ---
    uint32_t compiled_rule_pod_buffer_id          = INVALID_ID; // CompiledGameplayRulePOD (32B)
    uint32_t rule_cascade_layer_buffer_id         = INVALID_ID; // int32_t
    uint32_t rule_priority_buffer_id              = INVALID_ID; // int32_t
    uint32_t rule_specificity_buffer_id           = INVALID_ID; // int32_t
    uint32_t rule_important_flags_buffer_id       = INVALID_ID; // uint8_t
    uint32_t rule_output_key_buffer_id            = INVALID_ID; // int32_t

    uint32_t agent_transform_pos_buffer_id        = INVALID_ID; // godot::Vector3
    uint32_t player_transform_pos_buffer_id       = INVALID_ID; // godot::Vector3
    uint32_t agent_to_player_idx_buffer_id        = INVALID_ID; // int32_t
    uint32_t possession_match_mode_buffer_id      = INVALID_ID; // uint8_t (AgentPossessionMatchMode)
    uint32_t possession_elapsed_buffer_id         = INVALID_ID; // float
    uint32_t possession_duration_buffer_id        = INVALID_ID; // float
    uint32_t possession_completion_flags_buffer_id= INVALID_ID; // uint32_t (AgentPossessionCompletionFlags)

    uint32_t raw_input_vec2_buffer_id             = INVALID_ID; // godot::Vector2
    uint32_t filtered_input_vec2_buffer_id        = INVALID_ID; // godot::Vector2
    uint32_t world_move_dir_vec3_buffer_id        = INVALID_ID; // godot::Vector3
    uint32_t camera_transform_pod_buffer_id       = INVALID_ID; // InputTransform3DPOD (48B)
    uint32_t agent_transform_pod_buffer_id        = INVALID_ID; // InputTransform3DPOD (48B)

    [[nodiscard]] constexpr bool is_initialized() const noexcept {
        return entity_effective_time_scale_buffer_id != INVALID_ID &&
               prop_modded_value_buffer_id != INVALID_ID &&
               modifier_pod_buffer_id != INVALID_ID &&
               action_current_score_buffer_id != INVALID_ID &&
               interaction_score_diff_buffer_id != INVALID_ID;
    }
};

/**
 * EntityHierarchyRowPOD
 * Convenience descriptor for reading/writing a complete row across the
 * EntityHierarchyBuffer columns when registering, promoting, or demoting an entity.
 */
struct EntityHierarchyRowPOD {
    uint32_t parent_entity_id       = INVALID_ID;
    int32_t board_id                = -1;
    uint32_t archetype_mask         = ENTITY_ARCHETYPE_PIECE;
    uint64_t group_mask             = 0ULL;
    uint32_t active_flags           = ENTITY_FLAG_ENABLED | ENTITY_FLAG_INITIALIZED | ENTITY_FLAG_PROCESSED;
    float local_time_scale          = 1.0f;
    float effective_time_scale      = 1.0f;
    int32_t controlling_player_id   = -1;
    uint8_t exclusivity             = AGENT_EXCLUSIVITY_NONE;
    uint8_t simulation_tier         = SIM_TIER_PURE_DOD;
    uint64_t object_id              = 0ULL; // 0 when pure DOD without a SceneTree node
};

/**
 * GamePropertyRowPOD
 * Convenience descriptor for reading/writing a complete row across the
 * GamePropertyBuffer columns.
 */
struct GamePropertyRowPOD {
    uint32_t owner_entity_id   = INVALID_ID;
    uint32_t property_name_id  = 0u;
    int32_t base_value         = 100;
    int32_t modded_value       = 100;
    int32_t base_min           = 0;
    int32_t modded_min         = 0;
    int32_t base_max           = 100;
    int32_t modded_max         = 100;
    uint16_t property_flags    = PROP_FLAG_DEFAULT_MASK;
    uint64_t object_id         = 0ULL; // Optional Godot GameProperty ObjectID
    uint64_t gauge_object_id   = 0ULL; // Optional Godot GamePropertyGauge ObjectID
};

/**
 * ActionSequenceRowPOD
 * Convenience descriptor for reading/writing a complete row across the
 * ActionSequenceBuffer columns (for both GamePieceAction and GameAgentAction).
 */
struct ActionSequenceRowPOD {
    uint32_t owner_agent_id     = INVALID_ID;
    int32_t base_score          = 0;
    int32_t current_score       = 0;
    int32_t priority            = 0;
    int32_t status              = ACTION_STATUS_AVAILABLE;
    uint64_t action_group_mask  = 0ULL;
    uint64_t competition_mask   = 0ULL;
    float refresh_time          = 0.0f;
    float current_time          = 0.0f;
    int32_t current_step_idx    = 0;
    uint32_t step_offset        = 0u;
    uint32_t step_count         = 0u;
    uint32_t binding_offset     = 0u;
    uint32_t binding_count      = 0u;
};

/**
 * InteractionArenaRowPOD
 * Convenience descriptor for reading/writing a complete row across the
 * InteractionArenaBuffer columns.
 */
struct InteractionArenaRowPOD {
    uint32_t instigator_action_idx = INVALID_ID;
    int32_t instigator_score       = 0;
    int32_t competition_score      = 0;
    int32_t score_diff             = 0;
    float instigator_weight        = 1.0f;
    float competition_weight       = 1.0f;
    uint32_t participant_offset    = 0u;
    uint32_t participant_count     = 0u;
};

} // namespace ideam::core