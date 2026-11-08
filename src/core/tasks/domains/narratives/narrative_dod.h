#pragma once

/**
 * narrative_dod.h
 * Lean, table-driven C++26 Data-Oriented Design (DOD) coordinator for the IdeamNarratives domain.
 *
 * Responsibilities:
 *   1. Loop-driven allocation, selection-range synchronization, semantic name lookup, and
 *      TaskGraphResource dictionary population across the 5 Section 3.2 tables and 4 event rings
 *      via NARRATIVE_BUFFER_SPECS.
 *   2. O(1) slot allocation, CSR slice flattening (Relationship, Causal_Condition, Incident,
 *      Plot_Event, Character goals, Location adjacency), and generic ring push/drain helpers.
 *   3. Single templated add_narrative_task_node<T_Logic>() builder + bitmask-driven
 *      inject_pipeline_stages() for all 16 Narrative tasks (N1-N16).
 */

#include "narratives_dod_common.h"
#include "../../task_graph_dod.h"
#include "../../transform_task.h"
#include "../../query_task.h"
#include "../../metadata_task.h"

#include <godot_cpp/variant/dictionary.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ideam::core {

class NarrativeDOD {
private:
    std::shared_ptr<MemoryManagerDOD> memory_manager_;
    std::shared_ptr<TaskGraphDOD> default_task_graph_;

    NarrativesBufferCapacities capacities_{};
    NarrativesBufferManifest manifest_{};
    NarrativesPipelineTopology active_topology_{};

    // --- O(1) Slot Allocators per Primary Table ---
    SlotIndexAllocator narreme_allocator_;
    SlotIndexAllocator symbolic_slot_allocator_;
    SlotIndexAllocator condition_allocator_;
    SlotIndexAllocator incident_allocator_;
    SlotIndexAllocator plot_allocator_;

    // --- Contiguous CSR / Sub-Slice Bump Cursors ---
    uint32_t relationship_edge_cursor_ = 0u;
    uint32_t causal_link_cursor_       = 0u;
    uint32_t incident_cond_ref_cursor_ = 0u;
    uint32_t plot_event_cursor_        = 0u;
    uint32_t character_goal_cursor_    = 0u;
    uint32_t location_neighbor_cursor_ = 0u;
    uint32_t bridged_property_count_   = 0u;

    // --- Token, 128-Bit Trait & 64-Bit Group Bitmask Interning ---
    std::unordered_map<std::string, uint32_t> interned_tokens_;
    std::unordered_map<std::string, uint8_t> trait_bit_map_;
    std::unordered_map<std::string, uint8_t> group_bit_map_;
    uint32_t next_token_id_ = 1u;
    uint8_t next_trait_bit_ = 0u;
    uint8_t next_group_bit_ = 0u;

    // --- Generic Ring Push / Drain Helpers (Shared by All 4 Event Rings) ---
    template <typename T_Event>
    bool _push_ring_event(
        NarrativeBufferSlot p_ring_slot,
        NarrativeBufferSlot p_cursor_slot,
        uint32_t p_max_capacity,
        const T_Event& p_event
    ) noexcept {
        auto* ring    = get_column_ptr<T_Event>(p_ring_slot);
        auto* cursors = get_column_ptr<uint32_t>(p_cursor_slot);
        if (!ring || !cursors || p_max_capacity == 0u) return false;

        const uint32_t write_idx = cursors[1];
        if (write_idx >= p_max_capacity) return false;

        ring[write_idx] = p_event;
        cursors[1]      = write_idx + 1u;
        return true;
    }

    template <typename T_Event>
    uint32_t _drain_ring_events(
        NarrativeBufferSlot p_ring_slot,
        NarrativeBufferSlot p_cursor_slot,
        uint32_t p_max_capacity,
        std::vector<T_Event>& r_out_events,
        uint32_t p_max_drain
    ) noexcept {
        const auto* ring = get_column_ptr<T_Event>(p_ring_slot);
        auto* cursors    = get_column_ptr<uint32_t>(p_cursor_slot);
        if (!ring || !cursors) return 0u;

        const uint32_t write_idx = std::min(cursors[1], p_max_capacity);
        uint32_t read_idx        = std::min(cursors[0], write_idx);
        uint32_t drained         = 0u;

        while (read_idx < write_idx && drained < p_max_drain) {
            r_out_events.push_back(ring[read_idx++]);
            ++drained;
        }

        if (read_idx >= write_idx) {
            cursors[0] = 0u;
            cursors[1] = 0u;
        } else {
            cursors[0] = read_idx;
        }
        return drained;
    }

    uint32_t _active_count_for_group(NarrativeTableGroup p_group) const noexcept;
    void _bind_default_logic_buffers(void* r_logic_bytes, size_t p_size, const void* p_default_bytes) const noexcept;

public:
    explicit NarrativeDOD(
        std::shared_ptr<MemoryManagerDOD> p_memory_manager = nullptr,
        const NarrativesBufferCapacities& p_capacities = NarrativesBufferCapacities{}
    );
    ~NarrativeDOD() = default;

    NarrativeDOD(const NarrativeDOD&) = delete;
    NarrativeDOD& operator=(const NarrativeDOD&) = delete;
    NarrativeDOD(NarrativeDOD&&) noexcept = default;
    NarrativeDOD& operator=(NarrativeDOD&&) noexcept = default;

    // --- Core Manager & Table-Driven Manifest Lifecycle ---
    void initialize_buffers(const NarrativesBufferCapacities& p_capacities = NarrativesBufferCapacities{});
    void sync_selection_ranges() noexcept;
    void reset_all_slots() noexcept;

    [[nodiscard]] std::shared_ptr<MemoryManagerDOD> get_memory_manager() const noexcept { return memory_manager_; }
    [[nodiscard]] MemoryManagerDOD* get_memory_manager_raw() const noexcept { return memory_manager_.get(); }

    [[nodiscard]] std::shared_ptr<TaskGraphDOD> get_default_task_graph() const noexcept { return default_task_graph_; }
    void set_default_task_graph(std::shared_ptr<TaskGraphDOD> p_graph) noexcept { default_task_graph_ = std::move(p_graph); }

    [[nodiscard]] const NarrativesBufferCapacities& get_capacities() const noexcept { return capacities_; }
    [[nodiscard]] const NarrativesBufferManifest& get_buffer_manifest() const noexcept { return manifest_; }
    [[nodiscard]] const NarrativesPipelineTopology& get_active_topology() const noexcept { return active_topology_; }

    [[nodiscard]] constexpr uint32_t get_buffer_id(NarrativeBufferSlot p_slot) const noexcept { return manifest_.get(p_slot); }
    [[nodiscard]] constexpr uint32_t get_buffer_id_by_name(std::string_view p_name) const noexcept { return manifest_.get_by_name(p_name); }

    void populate_task_buffer_dictionary(godot::Dictionary& r_props, bool p_overwrite_existing = false) const;
    [[nodiscard]] GrantPartPOD make_buffer_requirement(uint32_t p_buffer_id, BufferAccessMode p_access) const noexcept;
    [[nodiscard]] GrantPartPOD make_slot_requirement(NarrativeBufferSlot p_slot, BufferAccessMode p_access) const noexcept {
        return make_buffer_requirement(manifest_.get(p_slot), p_access);
    }

    // --- Direct Typed Column Accessors (Zero-Boilerplate Column Reads/Writes) ---
    template <typename T>
    [[nodiscard]] T* get_column_ptr(NarrativeBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        if (!memory_manager_ || buf_id == INVALID_ID) return nullptr;
        MemoryBufferPOD* buf = memory_manager_->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr) return nullptr;
        return reinterpret_cast<T*>(buf->master_block_ptr + buf->memory_offset);
    }

    template <typename T>
    [[nodiscard]] std::span<T> get_column_span(NarrativeBufferSlot p_slot) const noexcept {
        const uint32_t buf_id = manifest_.get(p_slot);
        if (!memory_manager_ || buf_id == INVALID_ID) return {};
        MemoryBufferPOD* buf = memory_manager_->get_buffer(buf_id);
        if (!buf || !buf->master_block_ptr || buf->max_elements <= 0) return {};
        return std::span<T>(
            reinterpret_cast<T*>(buf->master_block_ptr + buf->memory_offset),
            static_cast<size_t>(buf->max_elements)
        );
    }

    // --- Token, 128-Bit Trait & 64-Bit Group Interning ---
    uint32_t intern_name_token(std::string_view p_name);
    uint8_t get_or_register_trait_bit(std::string_view p_trait_name);
    void compute_trait_bitmask_128(std::span<const std::string_view> p_traits, uint64_t& r_lo, uint64_t& r_hi);
    uint64_t get_or_register_group_bitmask(std::string_view p_group_name);

    // --- Table 1: NarremeIdentityAndTraitBuffer Operations ---
    uint32_t allocate_narreme(
        const NarremeIdentityRowPOD& p_row = {},
        std::span<const uint32_t> p_goal_plot_ids = {}
    );
    bool free_narreme(uint32_t p_narreme_slot);
    [[nodiscard]] bool is_narreme_slot_valid(uint32_t p_slot) const noexcept { return narreme_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_narreme_count() const noexcept { return narreme_allocator_.get_active_count(); }
    [[nodiscard]] uint32_t get_narreme_high_water_mark() const noexcept { return narreme_allocator_.get_high_water_mark(); }

    [[nodiscard]] NarremeIdentityRowPOD get_narreme_row(uint32_t p_slot) const noexcept;
    void set_narreme_row(uint32_t p_slot, const NarremeIdentityRowPOD& p_row) noexcept;
    [[nodiscard]] NarrativeTelemetrySnapshotPOD get_telemetry_snapshot(uint32_t p_slot) const noexcept;
    [[nodiscard]] const NarrativeTelemetrySnapshotPOD* get_telemetry_snapshot_ptr(uint32_t p_slot) const noexcept;

    // --- Table 2: SymbolicAssignmentMapBuffer Operations ---
    uint32_t allocate_symbolic_slot(
        const SymbolicAssignmentRecordPOD& p_record = {},
        uint32_t p_title_token = 0u
    );
    bool free_symbolic_slot(uint32_t p_symbolic_slot);
    [[nodiscard]] bool is_symbolic_slot_valid(uint32_t p_slot) const noexcept { return symbolic_slot_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_symbolic_slot_count() const noexcept { return symbolic_slot_allocator_.get_active_count(); }

    [[nodiscard]] SymbolicAssignmentRecordPOD get_symbolic_assignment(uint32_t p_symbolic_slot) const noexcept;
    void set_symbolic_assignment(uint32_t p_symbolic_slot, const SymbolicAssignmentRecordPOD& p_record, uint32_t p_title_token = 0u) noexcept;
    bool assign_symbolic_to_literal(uint32_t p_symbolic_slot, int32_t p_literal_narreme_id, int32_t p_fit_score = 0) noexcept;

    // --- Table 3: NarrativeRelationshipCSRBuffer & Location CSR Builders ---
    bool append_narreme_relationships(uint32_t p_source_narreme_slot, std::span<const RelationshipEdgePOD> p_edges) noexcept;
    void rebuild_relationship_csr(std::span<const std::vector<RelationshipEdgePOD>> p_per_narreme_edges) noexcept;
    void rebuild_location_adjacency_csr(std::span<const std::vector<uint32_t>> p_per_location_neighbors) noexcept;

    // --- Table 4: ConditionEvaluationTableBuffer & Causal CSR Operations ---
    uint32_t allocate_condition(
        const ConditionEvaluationRowPOD& p_row = {},
        std::span<const CausalLinkPOD> p_causal_links = {}
    );
    bool free_condition(uint32_t p_condition_slot);
    [[nodiscard]] bool is_condition_slot_valid(uint32_t p_slot) const noexcept { return condition_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_condition_count() const noexcept { return condition_allocator_.get_active_count(); }

    [[nodiscard]] ConditionEvaluationRowPOD get_condition_row(uint32_t p_condition_slot) const noexcept;
    void set_condition_row(uint32_t p_condition_slot, const ConditionEvaluationRowPOD& p_row) noexcept;
    bool append_causal_links(uint32_t p_condition_slot, std::span<const CausalLinkPOD> p_links) noexcept;

    void bind_external_gameplay_property_buffers(
        uint32_t p_modded_value_buf_id,
        uint32_t p_modded_min_buf_id = INVALID_ID,
        uint32_t p_modded_max_buf_id = INVALID_ID,
        uint32_t p_flags_buf_id = INVALID_ID,
        uint32_t p_active_property_count = 0u
    ) noexcept;

    // --- Table 5: IncidentAndPlotStateBuffer & PlotEventPOD CSR Operations ---
    uint32_t allocate_incident(
        const IncidentStateRowPOD& p_row = {},
        std::span<const uint32_t> p_prereq_condition_ids = {},
        std::span<const uint32_t> p_causal_condition_ids = {}
    );
    bool free_incident(uint32_t p_incident_slot);
    [[nodiscard]] bool is_incident_slot_valid(uint32_t p_slot) const noexcept { return incident_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_incident_count() const noexcept { return incident_allocator_.get_active_count(); }

    [[nodiscard]] IncidentStateRowPOD get_incident_row(uint32_t p_incident_slot) const noexcept;
    void set_incident_row(uint32_t p_incident_slot, const IncidentStateRowPOD& p_row) noexcept;

    uint32_t allocate_plot(
        const PlotProgressionRowPOD& p_row = {},
        std::span<const PlotEventPOD> p_events = {}
    );
    bool free_plot(uint32_t p_plot_slot);
    [[nodiscard]] bool is_plot_slot_valid(uint32_t p_slot) const noexcept { return plot_allocator_.is_valid(p_slot); }
    [[nodiscard]] uint32_t get_active_plot_count() const noexcept { return plot_allocator_.get_active_count(); }

    [[nodiscard]] PlotProgressionRowPOD get_plot_row(uint32_t p_plot_slot) const noexcept;
    void set_plot_row(uint32_t p_plot_slot, const PlotProgressionRowPOD& p_row) noexcept;
    bool append_plot_events(uint32_t p_plot_slot, std::span<const PlotEventPOD> p_events) noexcept;

    // --- 4 Event Ring Push / Drain Wrappers ---
    bool push_spatial_transfer_command(const NarremeSpatialTransferCommandPOD& p_cmd) noexcept {
        return _push_ring_event(NarrativeBufferSlot::SPATIAL_TRANSFER_COMMAND_RING, NarrativeBufferSlot::SPATIAL_TRANSFER_RING_CURSOR, capacities_.max_spatial_transfer_commands, p_cmd);
    }
    uint32_t drain_spatial_transfer_commands(std::vector<NarremeSpatialTransferCommandPOD>& r_out, uint32_t p_max_drain = 4096u) noexcept {
        return _drain_ring_events(NarrativeBufferSlot::SPATIAL_TRANSFER_COMMAND_RING, NarrativeBufferSlot::SPATIAL_TRANSFER_RING_CURSOR, capacities_.max_spatial_transfer_commands, r_out, p_max_drain);
    }

    bool push_gameplay_condition_event(const GameplayConditionEventPOD& p_ev) noexcept {
        return _push_ring_event(NarrativeBufferSlot::GAMEPLAY_CONDITION_EVENT_RING, NarrativeBufferSlot::GAMEPLAY_EVENT_RING_CURSOR, capacities_.max_gameplay_condition_events, p_ev);
    }
    uint32_t drain_gameplay_condition_events(std::vector<GameplayConditionEventPOD>& r_out, uint32_t p_max_drain = 4096u) noexcept {
        return _drain_ring_events(NarrativeBufferSlot::GAMEPLAY_CONDITION_EVENT_RING, NarrativeBufferSlot::GAMEPLAY_EVENT_RING_CURSOR, capacities_.max_gameplay_condition_events, r_out, p_max_drain);
    }

    bool push_character_goal_event(const CharacterGoalEventPOD& p_ev) noexcept {
        return _push_ring_event(NarrativeBufferSlot::CHARACTER_GOAL_EVENT_RING, NarrativeBufferSlot::CHARACTER_GOAL_RING_CURSOR, capacities_.max_character_goal_events, p_ev);
    }
    uint32_t drain_character_goal_events(std::vector<CharacterGoalEventPOD>& r_out, uint32_t p_max_drain = 4096u) noexcept {
        return _drain_ring_events(NarrativeBufferSlot::CHARACTER_GOAL_EVENT_RING, NarrativeBufferSlot::CHARACTER_GOAL_RING_CURSOR, capacities_.max_character_goal_events, r_out, p_max_drain);
    }

    bool push_signal_command(const NarrativeSignalCommandPOD& p_cmd) noexcept {
        return _push_ring_event(NarrativeBufferSlot::NARRATIVE_SIGNAL_COMMAND_RING, NarrativeBufferSlot::NARRATIVE_SIGNAL_RING_CURSOR, capacities_.max_signal_commands, p_cmd);
    }
    uint32_t drain_signal_commands(std::vector<NarrativeSignalCommandPOD>& r_out, uint32_t p_max_drain = 4096u) noexcept {
        return _drain_ring_events(NarrativeBufferSlot::NARRATIVE_SIGNAL_COMMAND_RING, NarrativeBufferSlot::NARRATIVE_SIGNAL_RING_CURSOR, capacities_.max_signal_commands, r_out, p_max_drain);
    }

    // ========================================================================
    // TEMPLATED TASK NODE BUILDER (N1 - N16) & PIPELINE STAGE INJECTION
    // ========================================================================
    template <typename T_Logic>
    NodeID add_narrative_task_node(
        TaskGraphDOD& r_graph,
        const T_Logic& p_logic_override = T_Logic{},
        const NarrativesNodeExecutionConfig& p_config = {}
    ) const;

    // Single-line convenience aliases forwarding to add_narrative_task_node<T_Logic>
    NodeID add_symbolic_similarity_node(TaskGraphDOD& g, const NarremeSymbolicSimilarityTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_role_best_fit_node(TaskGraphDOD& g, const RoleBestFitAssignmentTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_symbolic_reference_resolve_node(TaskGraphDOD& g, const SymbolicReferenceResolveTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_gameplay_condition_bridge_node(TaskGraphDOD& g, const GameplayConditionBridgeTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_narrative_condition_evaluate_node(TaskGraphDOD& g, const NarrativeConditionEvaluateTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_causal_wave_propagate_node(TaskGraphDOD& g, const CausalConditionWavePropagateTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_incident_status_resolve_node(TaskGraphDOD& g, const IncidentStatusResolveTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_plot_status_evaluate_node(TaskGraphDOD& g, const PlotStatusEvaluateTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_character_goal_sync_node(TaskGraphDOD& g, const CharacterGoalPlotSyncTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_location_transfer_node(TaskGraphDOD& g, const NarremeLocationTransferTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_resource_commit_node(TaskGraphDOD& g, const NarrativeResourceCommitTransformLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_condition_bitmask_query_node(TaskGraphDOD& g, const NarremeConditionBitmaskQueryLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_location_occupancy_query_node(TaskGraphDOD& g, const LocationOccupancyQueryLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_relationship_traversal_query_node(TaskGraphDOD& g, const RelationshipGraphTraversalQueryLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_causal_depth_metadata_node(TaskGraphDOD& g, const CausalDependencyDepthMetadataLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }
    NodeID add_story_arc_cluster_metadata_node(TaskGraphDOD& g, const StoryArcIslandClusterMetadataLogic<>& l = {}, const NarrativesNodeExecutionConfig& c = {}) const { return add_narrative_task_node(g, l, c); }

    NodeID add_transform_node_by_id(
        TaskGraphDOD& r_graph,
        NarrativesTransformLogicID p_logic_id,
        const godot::Dictionary& p_custom_props = godot::Dictionary(),
        const NarrativesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_query_node_by_id(
        TaskGraphDOD& r_graph,
        NarrativesQueryLogicID p_logic_id,
        const godot::Dictionary& p_custom_props = godot::Dictionary(),
        const NarrativesNodeExecutionConfig& p_config = {}
    ) const;

    NodeID add_metadata_node_by_id(
        TaskGraphDOD& r_graph,
        NarrativesMetadataLogicID p_logic_id,
        const godot::Dictionary& p_custom_props = godot::Dictionary(),
        const NarrativesNodeExecutionConfig& p_config = {}
    ) const;

    NarrativesPipelineTopology inject_pipeline_stages(
        TaskGraphDOD& r_graph,
        uint32_t p_stage_mask = NARRATIVES_STAGE_DEFAULT_PIPELINE_MASK,
        bool p_auto_connect_stages = true
    );

    static bool schedule_node_with_stage(
        TaskGraphDOD& r_graph,
        const NarrativesPipelineTopology& p_topology,
        NarrativesPipelineStageFlags p_target_stage,
        NodeID p_custom_node,
        bool p_join_downstream = false
    );

    static bool schedule_node_between_stages(
        TaskGraphDOD& r_graph,
        const NarrativesPipelineTopology& p_topology,
        NarrativesPipelineStageFlags p_upstream_stage,
        NarrativesPipelineStageFlags p_downstream_stage,
        NodeID p_custom_node
    );

    void execute_step(double p_delta, TaskGraphDOD* p_override_graph = nullptr);
};

} // namespace ideam::core