#pragma once

/**
 * game_dod.h
 * Master pure C++26 Data-Oriented Design (DOD) coordinator for the IdeamGames domain.
 *
 * Responsibilities:
 *   1. Allocates and manages the 5 Section 2.2 contiguous DOD buffer tables
 *      (EntityHierarchyBuffer, GamePropertyBuffer, PropertyModifierBuffer,
 *      ActionSequenceBuffer, InteractionArenaBuffer) inside MemoryManagerDOD.
 *   2. Provides O(1) slot allocation, recycling, simulation tier promotion/demotion
 *      (pure DOD <-> DOD rendered <-> hybrid synced <-> full OOP SceneTree),
 *      and zero-indirection telemetry access for GamePropertyGauge.
 *   3. Provides a flexible, non-prescribed TaskGraphDOD construction & injection API:
 *      - 18 standalone node builder methods (one for each G1-G18 logic struct)
 *      - Selective bitmask-driven stage injection (inject_pipeline_stages)
 *      - Topological wave anchors (GamesPipelineTopology) and co-scheduling helpers
 *        so developers can run custom tasks in the same Kahn waves or bind custom
 *        TaskGraphResources directly to GameDOD's buffer manifest.
 */

#include "games_dod_common.h"
#include "game_board_dod.h"
#include "../../task_graph_dod.h"
#include "../../transform_task.h"
#include "../../query_task.h"
#include "../../metadata_task.h"

#include <godot_cpp/variant/dictionary.hpp>

#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <span>
#include <cstdint>

namespace ideam::core {

class GameDOD {
private:
    std::shared_ptr<MemoryManagerDOD> memory_manager_;
    std::shared_ptr<TaskGraphDOD> default_task_graph_;

    GamesBufferCapacities capacities_{};
    GamesBufferManifest manifest_{};
    GamesPipelineTopology active_topology_{};

    // Root Game entity slot in EntityHierarchyBuffer (archetype = ENTITY_ARCHETYPE_GAME)
    uint32_t root_game_entity_slot_ = INVALID_ID;
    float root_time_scale_          = 1.0f;

    // --- O(1) Slot Allocators per Table ---
    SlotIndexAllocator entity_allocator_;
    SlotIndexAllocator property_allocator_;
    SlotIndexAllocator modifier_allocator_;
    SlotIndexAllocator action_allocator_;
    SlotIndexAllocator interaction_allocator_;
    SlotIndexAllocator rule_allocator_;
    SlotIndexAllocator player_allocator_;

    // --- Contiguous Sub-Slice Bump Cursors (with simple free-reset on clear) ---
    uint32_t sequence_step_cursor_           = 0;
    uint32_t piece_binding_cursor_           = 0;
    uint32_t interaction_participant_cursor_ = 0;

    // --- Board Coordinators ---
    std::vector<std::unique_ptr<GameBoardDOD>> boards_;
    int32_t next_board_partition_id_ = 1;

    // --- String/Token Interning (for 64-bit group_mask bits & property_name_id) ---
    std::unordered_map<std::string, uint32_t> interned_tokens_;
    std::unordered_map<std::string, uint8_t> group_bit_map_;
    uint32_t next_token_id_ = 1u;
    uint8_t next_group_bit_ = 0u;

    // --- Internal Buffer Pointer Helpers ---
    template <typename T>
    [[nodiscard]] T* _get_column_ptr(uint32_t p_buffer_id) const noexcept {
        if (!memory_manager_ || p_buffer_id == INVALID_ID) return nullptr;
        MemoryBufferPOD* buf = memory_manager_->get_buffer(p_buffer_id);
        if (!buf || !buf->master_block_ptr) return nullptr;
        return reinterpret_cast<T*>(buf->master_block_ptr + buf->memory_offset);
    }

    void _sync_buffer_selection(uint32_t p_buffer_id, const SlotIndexAllocator& p_allocator) noexcept;
    void _sync_flat_count_selection(uint32_t p_buffer_id, uint32_t p_active_count) noexcept;

public:
    explicit GameDOD(
        std::shared_ptr<MemoryManagerDOD> p_memory_manager = nullptr,
        const GamesBufferCapacities& p_capacities = GamesBufferCapacities{}
    );
    ~GameDOD() = default;

    // Non-copyable, movable
    GameDOD(const GameDOD&) = delete;
    GameDOD& operator=(const GameDOD&) = delete;
    GameDOD(GameDOD&&) noexcept = default;
    GameDOD& operator=(GameDOD&&) noexcept = default;

    // --- Core Manager & Buffer Manifest Access ---
    void initialize_buffers(const GamesBufferCapacities& p_capacities = GamesBufferCapacities{});
    void sync_selection_ranges() noexcept;
    void reset_all_slots() noexcept;

    [[nodiscard]] std::shared_ptr<MemoryManagerDOD> get_memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] MemoryManagerDOD* get_memory_manager_raw() const noexcept { return memory_manager_.get(); }

    [[nodiscard]] std::shared_ptr<TaskGraphDOD> get_default_task_graph() const noexcept { return default_task_graph_; }
    void set_default_task_graph(std::shared_ptr<TaskGraphDOD> p_graph) noexcept { default_task_graph_ = std::move(p_graph); }

    [[nodiscard]] const GamesBufferCapacities& get_capacities() const noexcept { return capacities_; }
    [[nodiscard]] const GamesBufferManifest& get_buffer_manifest() const noexcept { return manifest_; }
    [[nodiscard]] const GamesPipelineTopology& get_active_topology() const noexcept { return active_topology_; }

    /**
     * Resolves any semantic buffer property name used by G1-G18 logic structs
     * (e.g., "modded_value_buffer_id", "consequence_ring_buffer_id", "modifier_pod_buffer_id")
     * to its allocated MemoryManagerDOD buffer ID.
     */
    [[nodiscard]] uint32_t get_buffer_id_by_name(std::string_view p_name) const noexcept;

    /**
     * Populates a Godot Dictionary with all default *_buffer_id keys from GamesBufferManifest
     * so custom TaskResources or user-authored TaskGraphResources automatically bind to GameDOD's buffers.
     */
    void populate_task_buffer_dictionary(godot::Dictionary& r_props, bool p_overwrite_existing = false) const;

    // --- Token & Group Bitmask Interning ---
    /**
     * Interns a string (e.g., a GameProperty name or action identifier) to a stable non-zero uint32_t ID.
     */
    uint32_t intern_name_token(std::string_view p_name);

    /**
     * Assigns or looks up a bit position [0..63] for a group name and returns its 64-bit mask (1ULL << bit).
     */
    uint64_t get_or_register_group_bitmask(std::string_view p_group_name);
    [[nodiscard]] uint64_t compute_combined_group_mask(std::span<const std::string_view> p_groups);

    // --- Root Game & Board Management ---
    [[nodiscard]] constexpr uint32_t get_root_game_entity_slot() const noexcept { return root_game_entity_slot_; }
    void set_root_time_scale(float p_time_scale) noexcept;
    [[nodiscard]] constexpr float get_root_time_scale() const noexcept { return root_time_scale_; }

    GameBoardDOD* create_board_dod(int32_t p_board_partition_id = -1, float p_time_scale = 1.0f);
    [[nodiscard]] GameBoardDOD* get_board_dod(int32_t p_board_partition_id) const noexcept;
    bool remove_board_dod(int32_t p_board_partition_id);
    [[nodiscard]] std::span<const std::unique_ptr<GameBoardDOD>> get_boards() const noexcept { return boards_; }

    // --- Table 1: EntityHierarchyBuffer Operations ---
    uint32_t allocate_entity(const EntityHierarchyRowPOD& p_initial_row = EntityHierarchyRowPOD{});
    bool free_entity(uint32_t p_entity_slot);
    [[nodiscard]] bool is_entity_slot_valid(uint32_t p_entity_slot) const noexcept { return entity_allocator_.is_valid(p_entity_slot); }
    [[nodiscard]] uint32_t get_active_entity_count() const noexcept { return entity_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_entity_high_water_mark() const noexcept { return entity_allocator_.get_high_water_mark(); }

    [[nodiscard]] EntityHierarchyRowPOD get_entity_row(uint32_t p_entity_slot) const noexcept;
    void set_entity_row(uint32_t p_entity_slot, const EntityHierarchyRowPOD& p_row) noexcept;

    bool set_entity_simulation_tier(uint32_t p_entity_slot, EntitySimulationTier p_tier, uint64_t p_godot_object_id = 0ULL) noexcept;
    [[nodiscard]] EntitySimulationTier get_entity_simulation_tier(uint32_t p_entity_slot) const noexcept;

    void set_entity_local_time_scale(uint32_t p_entity_slot, float p_time_scale) noexcept;
    [[nodiscard]] float get_entity_local_time_scale(uint32_t p_entity_slot) const noexcept;
    [[nodiscard]] float get_entity_effective_time_scale(uint32_t p_entity_slot) const noexcept;

    // --- Table 2: GamePropertyBuffer & Telemetry Operations ---
    uint32_t allocate_property(const GamePropertyRowPOD& p_initial_row = GamePropertyRowPOD{});
    bool free_property(uint32_t p_property_slot);
    [[nodiscard]] bool is_property_slot_valid(uint32_t p_property_slot) const noexcept { return property_allocator_.is_valid(p_property_slot); }
    [[nodiscard]] uint32_t get_active_property_count() const noexcept { return property_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_property_high_water_mark() const noexcept { return property_allocator_.get_high_water_mark(); }

    [[nodiscard]] GamePropertyRowPOD get_property_row(uint32_t p_property_slot) const noexcept;
    void set_property_row(uint32_t p_property_slot, const GamePropertyRowPOD& p_row) noexcept;

    /**
     * Direct O(1) telemetry snapshot read for GamePropertyGauge (28-byte PropertyTelemetrySnapshotPOD).
     */
    [[nodiscard]] PropertyTelemetrySnapshotPOD get_property_telemetry_snapshot(uint32_t p_property_slot) const noexcept;
    [[nodiscard]] const PropertyTelemetrySnapshotPOD* get_property_telemetry_snapshot_ptr(uint32_t p_property_slot) const noexcept;

    // --- Table 3: PropertyModifierBuffer Operations ---
    uint32_t allocate_modifier(const PropertyModifierPOD& p_modifier);
    bool free_modifier(uint32_t p_modifier_slot);
    [[nodiscard]] bool is_modifier_slot_valid(uint32_t p_modifier_slot) const noexcept { return modifier_allocator_.is_valid(p_modifier_slot); }
    [[nodiscard]] uint32_t get_active_modifier_count() const noexcept { return modifier_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_modifier_high_water_mark() const noexcept { return modifier_allocator_.get_high_water_mark(); }

    [[nodiscard]] PropertyModifierPOD get_modifier_pod(uint32_t p_modifier_slot) const noexcept;
    void set_modifier_pod(uint32_t p_modifier_slot, const PropertyModifierPOD& p_modifier) noexcept;

    /**
     * Scans active modifier slots and recycles any slot flagged with MOD_FLAG_EXPIRED.
     * Returns the number of reclaimed modifier slots.
     */
    uint32_t reclaim_expired_modifiers() noexcept;

    // --- Table 4: ActionSequenceBuffer Operations ---
    uint32_t allocate_action(
        const ActionSequenceRowPOD& p_initial_row = ActionSequenceRowPOD{},
        std::span<const SequenceStepPOD> p_sequence_steps = {},
        std::span<const PiecePropertyBindingPOD> p_property_bindings = {}
    );
    bool free_action(uint32_t p_action_slot);
    [[nodiscard]] bool is_action_slot_valid(uint32_t p_action_slot) const noexcept { return action_allocator_.is_valid(p_action_slot); }
    [[nodiscard]] uint32_t get_active_action_count() const noexcept { return action_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_action_high_water_mark() const noexcept { return action_allocator_.get_high_water_mark(); }

    [[nodiscard]] ActionSequenceRowPOD get_action_row(uint32_t p_action_slot) const noexcept;
    void set_action_row(uint32_t p_action_slot, const ActionSequenceRowPOD& p_row) noexcept;

    // --- Table 5: InteractionArenaBuffer & Consequence Ring Operations ---
    uint32_t allocate_interaction(
        const InteractionArenaRowPOD& p_initial_row = InteractionArenaRowPOD{},
        std::span<const InteractionParticipantPOD> p_participants = {}
    );
    bool free_interaction(uint32_t p_interaction_slot);
    [[nodiscard]] bool is_interaction_slot_valid(uint32_t p_interaction_slot) const noexcept { return interaction_allocator_.is_valid(p_interaction_slot); }
    [[nodiscard]] uint32_t get_active_interaction_count() const noexcept { return interaction_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_interaction_high_water_mark() const noexcept { return interaction_allocator_.get_high_water_mark(); }

    [[nodiscard]] InteractionArenaRowPOD get_interaction_row(uint32_t p_interaction_slot) const noexcept;
    void set_interaction_row(uint32_t p_interaction_slot, const InteractionArenaRowPOD& p_row) noexcept;

    bool push_consequence_command(const ConsequenceCommandPOD& p_cmd) noexcept;
    uint32_t drain_consequence_commands(std::vector<ConsequenceCommandPOD>& r_out_commands, uint32_t p_max_drain = 4096u) noexcept;

    // --- Compiled GameplayRule Operations ---
    uint32_t allocate_compiled_rule(
        const CompiledGameplayRulePOD& p_rule,
        int32_t p_priority = 0,
        uint8_t p_specificity = 1u,
        bool p_important = false
    );
    bool free_compiled_rule(uint32_t p_rule_slot);
    void clear_compiled_rules() noexcept;
    [[nodiscard]] uint32_t get_active_rule_count() const noexcept { return rule_allocator_.get_active_count(); }

    // ========================================================================
    // FLEXIBLE TASK GRAPH NODE BUILDERS (G1 - G18)
    // ========================================================================
    // Each builder instantiates and configures a single native task node on r_graph,
    // attaches its 4-part GrantPartPOD buffer requirements from GamesBufferManifest,
    // and returns the NodeID so developers can wire arbitrary DAG topologies.

    // --- Transform Task Builders (G1 - G12) ---
    NodeID add_time_scale_cascade_node(
        TaskGraphDOD& r_graph,
        const EntityTimeScaleCascadeTransformLogic<float>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_modifier_tick_node(
        TaskGraphDOD& r_graph,
        const PropertyModifierTickTransformLogic<float>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_modifier_aggregate_node(
        TaskGraphDOD& r_graph,
        const PropertyModifierAggregateTransformLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_property_clamp_node(
        TaskGraphDOD& r_graph,
        const PropertyRangeClampAndExhaustTransformLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_rule_apply_node(
        TaskGraphDOD& r_graph,
        const GameplayRuleApplyTransformLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_piece_action_refresh_node(
        TaskGraphDOD& r_graph,
        const PieceActionScoreRefreshTransformLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_agent_sequence_step_node(
        TaskGraphDOD& r_graph,
        const AgentActionSequenceStepTransformLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_interaction_resolve_node(
        TaskGraphDOD& r_graph,
        const InteractionTugOfWarResolveTransformLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_possession_blend_node(
        TaskGraphDOD& r_graph,
        const AgentPossessionTransformBlendTransformLogic<godot::Vector3>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_control_map_filter_node(
        TaskGraphDOD& r_graph,
        const ControlMapInputFilterTransformLogic<godot::Vector2>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_snapshot_sync_node(
        TaskGraphDOD& r_graph,
        EntitySnapshotSyncMode p_sync_mode = SNAPSHOT_SYNC_BUILD_TELEMETRY_ONLY,
        const GameEntitySnapshotSyncTransformLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_consequence_drain_node(
        TaskGraphDOD& r_graph,
        const ConsequenceRingDrainTransformLogic<void*>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    // --- Query Task Builders (G13 - G16) ---
    NodeID add_property_exhausted_query_node(
        TaskGraphDOD& r_graph,
        const PropertyExhaustedOrLockedQueryLogic<int32_t>& p_logic_override = {},
        QueryOp p_op = QueryOp::CULL,
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_action_conflict_query_node(
        TaskGraphDOD& r_graph,
        const ActionCompetitionConflictQueryLogic<int64_t>& p_logic_override = {},
        QueryOp p_op = QueryOp::CULL,
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_agent_exclusivity_query_node(
        TaskGraphDOD& r_graph,
        const AgentExclusivityFilterQueryLogic<int32_t>& p_logic_override = {},
        QueryOp p_op = QueryOp::CULL,
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_rule_selector_node(
        TaskGraphDOD& r_graph,
        const GameplayRuleSelectorQueryLogic<int32_t>& p_logic_override = {},
        QueryOp p_op = QueryOp::CULL,
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    // --- Metadata Task Builders (G17 - G18) ---
    NodeID add_rule_specificity_node(
        TaskGraphDOD& r_graph,
        const GameplayRuleSpecificityMetadataLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_board_partition_node(
        TaskGraphDOD& r_graph,
        const BoardPresencePartitionMetadataLogic<int32_t>& p_logic_override = {},
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    // ========================================================================
    // COMPOSABLE PIPELINE STAGE INJECTION & WAVE CO-SCHEDULING
    // ========================================================================
    /**
     * Injects only the requested GamesPipelineStageFlags into r_graph and returns a
     * GamesPipelineTopology with entry/exit NodeID anchors for every created stage.
     *
     * - If p_auto_connect_stages is true, dependent stages are wired in topological
     *   order while independent tasks within a stage (e.g., EntityTimeScaleCascade +
     *   BoardPresencePartition, or ControlMapInputFilter + AgentPossessionTransformBlend)
     *   are scheduled in parallel within the same Kahn wave.
     * - If p_auto_connect_stages is false, stages are created without inter-stage edges
     *   so the caller can wire a completely custom DAG.
     */
    GamesPipelineTopology inject_pipeline_stages(
        TaskGraphDOD& r_graph,
        uint32_t p_stage_mask = GAMES_STAGE_CORE_SIMULATION_MASK,
        bool p_auto_connect_stages = true
    );

    /**
     * Wires a developer's custom task node (p_custom_node) so that Kahn's topological sort
     * schedules it in the SAME parallel wave as p_target_stage (by sharing p_target_stage's
     * upstream predecessor dependencies, and optionally joining p_target_stage's downstream consumers).
     */
    static bool schedule_node_with_stage(
        TaskGraphDOD& r_graph,
        const GamesPipelineTopology& p_topology,
        GamesPipelineStageFlags p_target_stage,
        NodeID p_custom_node,
        bool p_join_downstream = false
    );

    /**
     * Wires a developer's custom task node (p_custom_node) as an intermediate dependency
     * between p_upstream_stage.exit_node and p_downstream_stage.entry_node.
     */
    static bool schedule_node_between_stages(
        TaskGraphDOD& r_graph,
        const GamesPipelineTopology& p_topology,
        GamesPipelineStageFlags p_upstream_stage,
        GamesPipelineStageFlags p_downstream_stage,
        NodeID p_custom_node
    );

    // --- Execution Step ---
    /**
     * Synchronizes buffer selection ranges, executes p_override_graph (or default_task_graph_
     * if nullptr), and optionally reclaims expired timed modifiers.
     */
    void execute_step(
        double p_delta,
        TaskGraphDOD* p_override_graph = nullptr,
        bool p_reclaim_expired_modifiers = true
    );
};

} // namespace ideam::core