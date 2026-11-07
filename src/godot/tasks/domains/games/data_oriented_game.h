#pragma once

/**
 * data_oriented_game.h
 * Godot-facing master bridge class in src/godot/tasks/domains/games/.
 *
 * Extends ideam::godot_ext::Game to unite the Object-Oriented SceneTree architecture
 * in src/godot/games with the pure C++26 Data-Oriented Design (DOD) memory buffers
 * and Kahn-sorted TaskGraphDOD execution engine in src/core/tasks/domains/games.
 *
 * Key Architectural Capabilities:
 *   1. Configures and owns an ideam::core::GameDOD instance (or shares a
 *      MemoryManagerDOD / TaskGraphDOD with an external TaskGraphHost).
 *   2. Supports 5 flexible execution modes:
 *      - DOD_EXEC_DISABLED: Pure OOP fallback (zero DOD graph execution)
 *      - DOD_EXEC_COMPOSABLE_STAGES: Bitmask-driven stage injection via
 *        GamesPipelineStageFlags with topological wave anchors for custom task injection
 *      - DOD_EXEC_CUSTOM_GRAPH_RESOURCE: Compiles a user-authored TaskGraphResource
 *        and automatically binds all Section 2.2 buffer IDs
 *      - DOD_EXEC_SHARED_TASK_HOST: Shares MemoryManagerDOD & TaskGraphDOD with a
 *        TaskGraphHost node in the SceneTree so Games tasks and custom user tasks
 *        co-execute within the same Kahn waves
 *      - DOD_EXEC_MANUAL_GRAPH: Allocates all 5 DOD buffer tables and leaves the
 *        TaskGraphDOD open for 100% manual node-by-node assembly
 *   3. Enables fluid entity mobility across simulation tiers (SIM_TIER_PURE_DOD,
 *      SIM_TIER_DOD_RENDERED, SIM_TIER_HYBRID_SYNCED, SIM_TIER_FULL_OOP),
 *      zero-indirection GamePropertyGauge telemetry pushing, and deferred
 *      ConsequenceCommandPOD ring buffer draining.
 */

#include "../../../../core/tasks/domains/games/game_dod.h"
#include "../../../games/game.h"
#include "../../task_graph_host.h"
#include "../../task_graph_resource.h"
#include "data_oriented_game_board.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace ideam::godot_ext {

class GameEntity;
class GameAgent;
class GamePiece;
class GameProperty;
class GamePropertyModifier;
class GamePieceAction;
class GameAgentAction;
class GameInteraction;
class GamePropertyGauge;
class GameplayStyle;
class GameplayRule;

enum DODExecutionMode : int32_t {
    DOD_EXEC_DISABLED              = 0, // Pure OOP SceneTree execution
    DOD_EXEC_COMPOSABLE_STAGES     = 1, // Bitmask-configurable stage pipeline + custom wave co-scheduling
    DOD_EXEC_CUSTOM_GRAPH_RESOURCE = 2, // User-authored TaskGraphResource bound to GameDOD buffers
    DOD_EXEC_SHARED_TASK_HOST      = 3, // Shared MemoryManagerDOD / TaskGraphDOD with a TaskGraphHost node
    DOD_EXEC_MANUAL_GRAPH          = 4  // Buffers allocated; graph left open for manual node construction
};

enum DODSimulationTier : int32_t {
    DOD_SIM_TIER_PURE_DOD      = 0, // Simulated purely in DOD buffers; no SceneTree Node required
    DOD_SIM_TIER_DOD_RENDERED  = 1, // Simulated & rendered via DOD tasks
    DOD_SIM_TIER_HYBRID_SYNCED = 2, // Simulated in DOD buffers & synchronized with SceneTree Nodes / Gauges
    DOD_SIM_TIER_FULL_OOP      = 3  // Simulated in OOP SceneTree Nodes with full signal support
};

class DataOrientedGame : public Game {
    GDCLASS(DataOrientedGame, Game)

protected:
    static void _bind_methods();

private:
    std::unique_ptr<core::GameDOD> game_dod_;
    std::shared_ptr<core::TaskGraphDOD> active_task_graph_;
    godot::HashMap<godot::StringName, core::NodeID> ui_to_dod_map_;

    // --- Inspector Configuration ---
    DODExecutionMode dod_execution_mode_         = DODExecutionMode::DOD_EXEC_COMPOSABLE_STAGES;
    uint32_t pipeline_stage_mask_                = core::GAMES_STAGE_HYBRID_DEFAULT_MASK;
    bool auto_connect_pipeline_stages_           = true;
    bool auto_drain_consequences_to_oop_         = true;
    bool auto_sync_gauges_                       = true;

    godot::Ref<TaskGraphResource> custom_task_graph_resource_;
    godot::NodePath shared_task_host_path_;

    // Configurable Section 2.2 Buffer Capacities
    int max_dod_entities_                        = 4096;
    int max_dod_properties_                      = 16384;
    int max_dod_modifiers_                       = 32768;
    int max_dod_actions_                         = 8192;
    int max_dod_interactions_                    = 2048;
    int max_dod_rules_                           = 1024;
    int max_dod_consequence_commands_            = 4096;

    // --- Bidirectional ObjectID <-> DOD Slot Registries ---
    std::unordered_map<uint64_t, uint32_t> object_id_to_entity_slot_;
    std::unordered_map<uint32_t, uint64_t> entity_slot_to_object_id_;

    std::unordered_map<uint64_t, uint32_t> object_id_to_property_slot_;
    std::unordered_map<uint32_t, uint64_t> property_slot_to_object_id_;

    std::unordered_map<uint64_t, uint32_t> object_id_to_modifier_slot_;
    std::unordered_map<uint64_t, uint32_t> object_id_to_action_slot_;
    std::unordered_map<uint64_t, uint32_t> object_id_to_interaction_slot_;

    // Registered HUD Gauges bound to DOD property slots (Gauge ObjectID -> Property Slot)
    struct BoundGaugeEntry {
        uint64_t gauge_object_id = 0ULL;
        uint32_t property_slot   = core::INVALID_ID;
    };
    std::vector<BoundGaugeEntry> bound_gauges_;

    // --- Internal Bridge Helpers ---
    void _ensure_game_dod_initialized();
    uint32_t _infer_entity_archetype_mask(const GameEntity* p_entity) const;
    uint64_t _extract_node_group_mask(const godot::Node* p_node);
    uint16_t _pack_property_flags(const GameProperty* p_prop) const;
    void _register_entity_children_recursive(GameEntity* p_entity, uint32_t p_entity_slot);
    void _pull_hybrid_entities_to_dod();
    void _push_hybrid_entities_from_dod();
    void _drain_consequence_ring_to_oop();
    void _push_telemetry_to_bound_gauges();

public:
    DataOrientedGame();
    ~DataOrientedGame() override;

    virtual void _ready() override;
    virtual void _process(double delta) override;

    // --- Direct C++ Accessors ---
    [[nodiscard]] core::GameDOD* get_game_dod() const noexcept { return game_dod_.get(); }
    [[nodiscard]] core::TaskGraphDOD* get_active_task_graph_dod() const noexcept { return active_task_graph_.get(); }
    [[nodiscard]] core::MemoryManagerDOD* get_memory_manager_dod() const noexcept {
        return game_dod_ ? game_dod_->get_memory_manager_raw() : nullptr;
    }

    // --- Inspector Setters / Getters ---
    void set_dod_execution_mode(DODExecutionMode p_mode);
    DODExecutionMode get_dod_execution_mode() const;

    void set_pipeline_stage_mask(int p_mask);
    int get_pipeline_stage_mask() const;

    void set_auto_connect_pipeline_stages(bool p_auto_connect);
    bool get_auto_connect_pipeline_stages() const;

    void set_auto_drain_consequences_to_oop(bool p_auto_drain);
    bool get_auto_drain_consequences_to_oop() const;

    void set_auto_sync_gauges(bool p_auto_sync);
    bool get_auto_sync_gauges() const;

    void set_custom_task_graph_resource(const godot::Ref<TaskGraphResource>& p_resource);
    godot::Ref<TaskGraphResource> get_custom_task_graph_resource() const;

    void set_shared_task_host_path(const godot::NodePath& p_path);
    godot::NodePath get_shared_task_host_path() const;

    void set_max_dod_entities(int p_max);
    int get_max_dod_entities() const;

    void set_max_dod_properties(int p_max);
    int get_max_dod_properties() const;

    void set_max_dod_modifiers(int p_max);
    int get_max_dod_modifiers() const;

    void set_max_dod_actions(int p_max);
    int get_max_dod_actions() const;

    void set_max_dod_interactions(int p_max);
    int get_max_dod_interactions() const;

    void set_max_dod_rules(int p_max);
    int get_max_dod_rules() const;

    void set_max_dod_consequence_commands(int p_max);
    int get_max_dod_consequence_commands() const;

    // ========================================================================
    // PIPELINE SETUP, GRAPH COMPILATION & CUSTOM WAVE CO-SCHEDULING API
    // ========================================================================
    void setup_dod_pipeline();
    void rebuild_dod_task_graph();
    void execute_dod_step(double p_delta);

    int get_buffer_id(const godot::StringName& p_semantic_name) const;
    godot::Dictionary get_buffer_manifest_dictionary() const;

    /**
     * Dynamically adds any of the 12 Games Transform tasks (GamesTransformLogicID),
     * 4 Games Query tasks (GamesQueryLogicID), or 2 Games Metadata tasks (GamesMetadataLogicID)
     * to the active TaskGraphDOD, returning the new NodeID (or -1 on failure).
     */
    int add_games_transform_node(
        int p_logic_id,
        const godot::Dictionary& p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false,
        int p_view = 0,
        int p_strategy = 0,
        int p_element_type = 2
    );

    int add_games_query_node(
        int p_logic_id,
        int p_query_op = 0,
        const godot::Dictionary& p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false,
        int p_view = 0,
        int p_strategy = 0,
        int p_element_type = 2
    );

    int add_games_metadata_node(
        int p_logic_id,
        const godot::Dictionary& p_properties = godot::Dictionary(),
        int p_priority = 0,
        bool p_use_registry_factory = false,
        int p_view = 0,
        int p_strategy = 0,
        int p_element_type = 2
    );

    /**
     * Adds a custom Godot reflection task node (TaskTypeDOD::GODOT_REFLECTION) to the
     * active TaskGraphDOD so developers can run custom GDScript/C++/C# methods inside
     * the same Kahn waves as native DOD buffer tasks.
     */
    int add_custom_reflection_task_node(
        godot::Object* p_target,
        const godot::StringName& p_method,
        int p_priority = 0
    );

    int connect_task_nodes(int p_from_node, int p_to_node, int p_from_port = 0, int p_to_port = 0);
    bool schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream = false);
    bool schedule_node_between_stages(int p_upstream_stage_flag, int p_downstream_stage_flag, int p_custom_node_id);
    int get_stage_entry_node(int p_stage_flag) const;
    int get_stage_exit_node(int p_stage_flag) const;

    // ========================================================================
    // OOP <-> DOD ENTITY REGISTRATION, PROMOTION & DEMOTION
    // ========================================================================
    int register_game_entity(
        GameEntity* p_entity,
        int p_simulation_tier = static_cast<int>(DOD_SIM_TIER_HYBRID_SYNCED),
        int p_board_partition_id = -1
    );
    bool unregister_game_entity(GameEntity* p_entity);

    int get_entity_dod_slot(const GameEntity* p_entity) const;
    GameEntity* get_entity_for_dod_slot(int p_entity_slot) const;

    bool set_entity_simulation_tier(GameEntity* p_entity, int p_tier);
    bool set_slot_simulation_tier(int p_entity_slot, int p_tier);
    int get_slot_simulation_tier(int p_entity_slot) const;

    bool promote_dod_slot_to_node(
        int p_entity_slot,
        GameEntity* p_node,
        int p_target_tier = static_cast<int>(DOD_SIM_TIER_HYBRID_SYNCED)
    );
    int demote_node_to_dod_slot(
        GameEntity* p_entity,
        int p_target_tier = static_cast<int>(DOD_SIM_TIER_PURE_DOD)
    );

    void sync_entity_to_dod(GameEntity* p_entity);
    void sync_entity_from_dod(GameEntity* p_entity);

    // ========================================================================
    // PURE-DOD & HYBRID PROPERTY, MODIFIER, ACTION, INTERACTION & RULE API
    // ========================================================================
    int create_pure_dod_entity(
        int p_archetype_mask = static_cast<int>(core::ENTITY_ARCHETYPE_PIECE),
        int p_parent_slot = -1,
        int p_board_partition_id = -1,
        float p_local_time_scale = 1.0f,
        int64_t p_group_mask = 0
    );
    bool free_dod_entity(int p_entity_slot);

    int register_game_property(GameProperty* p_property, int p_owner_entity_slot = -1);
    int create_dod_property(
        int p_owner_entity_slot,
        const godot::StringName& p_property_name,
        int p_base_value = 100,
        int p_min_value = 0,
        int p_max_value = 100,
        int p_flags = static_cast<int>(core::PROP_FLAG_DEFAULT_MASK)
    );
    int get_property_dod_slot(const GameProperty* p_property) const;
    int get_dod_property_modded_value(int p_property_slot) const;
    int get_dod_property_base_value(int p_property_slot) const;
    void set_dod_property_base_value(int p_property_slot, int p_base_value);
    godot::Dictionary get_dod_property_snapshot_dict(int p_property_slot) const;

    void bind_property_gauge_to_dod_slot(GamePropertyGauge* p_gauge, int p_property_slot);
    void unbind_property_gauge(GamePropertyGauge* p_gauge);

    int register_game_property_modifier(
        GamePropertyModifier* p_modifier,
        int p_target_property_slot,
        int p_target_channel = static_cast<int>(core::MOD_CHANNEL_VALUE)
    );
    int add_dod_property_modifier(
        int p_target_property_slot,
        int p_value,
        int p_mod_type = static_cast<int>(core::MOD_TYPE_STATIC),
        float p_time_limit = 0.0f,
        int p_target_channel = static_cast<int>(core::MOD_CHANNEL_VALUE)
    );
    bool remove_dod_property_modifier(int p_modifier_slot);

    int register_game_piece_action(GamePieceAction* p_action, int p_owner_piece_slot = -1);
    int register_game_agent_action(GameAgentAction* p_action, int p_owner_agent_slot = -1);
    int register_game_interaction(GameInteraction* p_interaction);

    godot::PackedInt32Array compile_gameplay_style_to_dod(
        const godot::Ref<GameplayStyle>& p_style,
        int p_scope_root_entity_slot = -1
    );
    int compile_gameplay_rule_to_dod(
        const godot::Ref<GameplayRule>& p_rule,
        int p_scope_root_entity_slot = -1
    );

    int get_active_dod_entity_count() const;
    int get_active_dod_property_count() const;
    int get_active_dod_modifier_count() const;
    int get_active_dod_action_count() const;
    int get_active_dod_interaction_count() const;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::DODExecutionMode);
VARIANT_ENUM_CAST(ideam::godot_ext::DODSimulationTier);