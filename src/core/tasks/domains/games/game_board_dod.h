#pragma once

/**
 * game_board_dod.h
 * Pure C++26 board partition coordinator for the IdeamGames <-> IdeamTasks bridge.
 *
 * Associates a logical GameBoard partition ID (board_id) and an ENTITY_ARCHETYPE_BOARD
 * row in EntityHierarchyBuffer with board-scoped entity residency, simulation tier
 * promotion/demotion (pure DOD <-> hybrid <-> full OOP SceneTree), board-local
 * CompiledGameplayRulePOD slices, and modular TaskGraphDOD node builders.
 */

#include "games_dod_common.h"
#include "../../task_graph_dod.h"

#include <vector>
#include <span>
#include <cstdint>

namespace ideam::core {

class GameDOD;

class GameBoardDOD {
private:
    GameDOD* owner_game_                   = nullptr;
    int32_t board_partition_id_            = -1;
    uint32_t board_entity_slot_            = INVALID_ID;
    float local_time_scale_                = 1.0f;
    bool enabled_                          = true;

    // Dense list of entity slots currently residing on this board partition
    std::vector<uint32_t> resident_entities_;

    // Rule slot indices in GameDOD's compiled_rule_pod_buffer_id owned by this board
    std::vector<uint32_t> board_rule_slots_;

public:
    GameBoardDOD() = default;
    GameBoardDOD(GameDOD* p_owner_game, int32_t p_board_partition_id, uint32_t p_board_entity_slot, float p_time_scale = 1.0f);
    ~GameBoardDOD() = default;

    // --- Identity & Hierarchy State ---
    [[nodiscard]] constexpr GameDOD* get_owner_game() const noexcept { return owner_game_; }
    [[nodiscard]] constexpr int32_t get_board_partition_id() const noexcept { return board_partition_id_; }
    [[nodiscard]] constexpr uint32_t get_board_entity_slot() const noexcept { return board_entity_slot_; }

    void set_local_time_scale(float p_time_scale) noexcept;
    [[nodiscard]] constexpr float get_local_time_scale() const noexcept { return local_time_scale_; }
    [[nodiscard]] float get_effective_time_scale() const noexcept;

    void set_enabled(bool p_enabled) noexcept;
    [[nodiscard]] constexpr bool is_enabled() const noexcept { return enabled_; }

    // --- Board Residency & Simulation Tier Management ---
    /**
     * Assigns an existing entity slot in EntityHierarchyBuffer to this board partition,
     * updating its board_id and (if parent_entity_id is INVALID_ID) parenting it to board_entity_slot_.
     */
    bool register_entity_on_board(
        uint32_t p_entity_slot,
        EntitySimulationTier p_tier = SIM_TIER_PURE_DOD,
        bool p_reparent_to_board = true
    );

    /**
     * Removes an entity slot from this board partition, resetting its board_id to -1.
     */
    bool unregister_entity_from_board(uint32_t p_entity_slot);

    /**
     * Seamlessly promotes or demotes an entity on this board between simulation tiers
     * (e.g., SIM_TIER_PURE_DOD, SIM_TIER_DOD_RENDERED, SIM_TIER_HYBRID_SYNCED, SIM_TIER_FULL_OOP).
     */
    bool set_entity_simulation_tier(uint32_t p_entity_slot, EntitySimulationTier p_tier, uint64_t p_godot_object_id = 0ULL);

    /**
     * Batch-updates simulation tiers for all resident entities on this board.
     */
    void set_all_resident_simulation_tiers(EntitySimulationTier p_tier);

    [[nodiscard]] std::span<const uint32_t> get_resident_entities() const noexcept {
        return resident_entities_;
    }

    // --- Board-Scoped GameplayStyle / Rule Management ---
    /**
     * Registers a compiled board-level GameplayRule (e.g., from a board's GameplayStyle)
     * scoped to this board's hierarchy and returns its rule slot index in GameDOD.
     */
    uint32_t register_board_rule(
        const CompiledGameplayRulePOD& p_rule,
        int32_t p_priority = 0,
        uint8_t p_specificity = 1u,
        bool p_important = false
    );

    void clear_board_rules();

    [[nodiscard]] std::span<const uint32_t> get_board_rule_slots() const noexcept {
        return board_rule_slots_;
    }

    // --- Flexible TaskGraph Node Builders (Board-Scoped) ---
    /**
     * Adds a BoardPresencePartitionMetadataLogic (G18) task node to r_graph that stamps
     * partition_ids for this board (or all boards if p_filter_to_this_board == false).
     * Returns the created NodeID so developers can wire it into any custom Kahn wave.
     */
    NodeID add_board_partition_node(
        TaskGraphDOD& r_graph,
        BoardPartitionResolutionMode p_resolution_mode = BOARD_PARTITION_DIRECT_OR_ANCESTOR,
        bool p_filter_to_this_board = true,
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    /**
     * Adds a GameplayRuleSelectorQueryLogic (G16) task node scoped to this board's
     * board_entity_slot_ hierarchy (RULE_SCOPE_DESCENDANTS by default).
     */
    NodeID add_board_rule_selector_node(
        TaskGraphDOD& r_graph,
        uint32_t p_required_archetype_mask = ENTITY_ARCHETYPE_ALL,
        uint64_t p_required_group_mask = 0ULL,
        uint32_t p_required_property_name_id = 0u,
        uint8_t p_pseudo_state = PSEUDO_STATE_NONE,
        const GamesNodeExecutionConfig& p_config = {}
    ) const;

    /**
     * Adds a GameplayRuleApplyTransformLogic (G5) task node for this board's rules.
     */
    NodeID add_board_rule_apply_node(
        TaskGraphDOD& r_graph,
        const CompiledGameplayRulePOD& p_rule_override = {},
        bool p_use_compiled_rule_buffer = true,
        const GamesNodeExecutionConfig& p_config = {}
    ) const;
};

} // namespace ideam::core