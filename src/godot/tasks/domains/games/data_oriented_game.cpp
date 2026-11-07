#include "data_oriented_game.h"

#include "../../../../core/tasks/domains/games/registration/games_metadata_task_registry.h"
#include "../../../../core/tasks/domains/games/registration/games_query_task_registry.h"
#include "../../../../core/tasks/domains/games/registration/games_transform_task_registry.h"

#include "../../../games/game_entity.h"
#include "../../../games/game_property.h"
#include "../../../games/game_property_modifier.h"
#include "../../../games/game_entities/game_agent.h"
#include "../../../games/game_entities/game_board.h"
#include "../../../games/game_entities/game_piece.h"
#include "../../../games/game_entities/actions/game_agent_action.h"
#include "../../../games/game_entities/actions/game_interaction.h"
#include "../../../games/game_entities/actions/game_piece_action.h"
#include "../../../games/gameplay/gameplay_rule.h"
#include "../../../games/gameplay/gameplay_style.h"
#include "../../../games/ui/game_property_gauge.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>
#include <string>

namespace ideam::godot_ext {

void DataOrientedGame::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("dod_pipeline_ready"));
    ADD_SIGNAL(godot::MethodInfo("dod_step_completed", godot::PropertyInfo(godot::Variant::FLOAT, "delta")));
    ADD_SIGNAL(godot::MethodInfo(
        "dod_consequence_applied",
        godot::PropertyInfo(godot::Variant::INT, "target_slot"),
        godot::PropertyInfo(godot::Variant::INT, "score_delta"),
        godot::PropertyInfo(godot::Variant::INT, "consequence_mask")
    ));

    // Enums
    BIND_ENUM_CONSTANT(DODExecutionMode::DOD_EXEC_DISABLED);
    BIND_ENUM_CONSTANT(DODExecutionMode::DOD_EXEC_COMPOSABLE_STAGES);
    BIND_ENUM_CONSTANT(DODExecutionMode::DOD_EXEC_CUSTOM_GRAPH_RESOURCE);
    BIND_ENUM_CONSTANT(DODExecutionMode::DOD_EXEC_SHARED_TASK_HOST);
    BIND_ENUM_CONSTANT(DODExecutionMode::DOD_EXEC_MANUAL_GRAPH);

    BIND_ENUM_CONSTANT(DODSimulationTier::DOD_SIM_TIER_PURE_DOD);
    BIND_ENUM_CONSTANT(DODSimulationTier::DOD_SIM_TIER_DOD_RENDERED);
    BIND_ENUM_CONSTANT(DODSimulationTier::DOD_SIM_TIER_HYBRID_SYNCED);
    BIND_ENUM_CONSTANT(DODSimulationTier::DOD_SIM_TIER_FULL_OOP);

    // Inspector Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_dod_execution_mode", "mode"), &DataOrientedGame::set_dod_execution_mode);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dod_execution_mode"), &DataOrientedGame::get_dod_execution_mode);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "dod_execution_mode",
            godot::PROPERTY_HINT_ENUM,
            "Disabled (Pure OOP),Composable Stages,Custom TaskGraphResource,Shared TaskGraphHost,Manual TaskGraph"
        ),
        "set_dod_execution_mode",
        "get_dod_execution_mode"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_pipeline_stage_mask", "mask"), &DataOrientedGame::set_pipeline_stage_mask);
    godot::ClassDB::bind_method(godot::D_METHOD("get_pipeline_stage_mask"), &DataOrientedGame::get_pipeline_stage_mask);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "pipeline_stage_mask",
            godot::PROPERTY_HINT_FLAGS,
            "Snapshot Pull,Input & Possession,Hierarchy & Time,Modifier Tick,Modifier Aggregate,Gameplay Rules,Property Clamp,Piece Actions,Agent Actions,Interaction Resolve,Consequence Drain,Snapshot Push"
        ),
        "set_pipeline_stage_mask",
        "get_pipeline_stage_mask"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_connect_pipeline_stages", "auto_connect"), &DataOrientedGame::set_auto_connect_pipeline_stages);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_connect_pipeline_stages"), &DataOrientedGame::get_auto_connect_pipeline_stages);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_connect_pipeline_stages"), "set_auto_connect_pipeline_stages", "get_auto_connect_pipeline_stages");

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_drain_consequences_to_oop", "auto_drain"), &DataOrientedGame::set_auto_drain_consequences_to_oop);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_drain_consequences_to_oop"), &DataOrientedGame::get_auto_drain_consequences_to_oop);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_drain_consequences_to_oop"), "set_auto_drain_consequences_to_oop", "get_auto_drain_consequences_to_oop");

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_sync_gauges", "auto_sync"), &DataOrientedGame::set_auto_sync_gauges);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_sync_gauges"), &DataOrientedGame::get_auto_sync_gauges);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_sync_gauges"), "set_auto_sync_gauges", "get_auto_sync_gauges");

    godot::ClassDB::bind_method(godot::D_METHOD("set_custom_task_graph_resource", "resource"), &DataOrientedGame::set_custom_task_graph_resource);
    godot::ClassDB::bind_method(godot::D_METHOD("get_custom_task_graph_resource"), &DataOrientedGame::get_custom_task_graph_resource);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::OBJECT, "custom_task_graph_resource", godot::PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"),
        "set_custom_task_graph_resource",
        "get_custom_task_graph_resource"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_shared_task_host_path", "path"), &DataOrientedGame::set_shared_task_host_path);
    godot::ClassDB::bind_method(godot::D_METHOD("get_shared_task_host_path"), &DataOrientedGame::get_shared_task_host_path);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::NODE_PATH, "shared_task_host_path", godot::PROPERTY_HINT_NODE_PATH_VALID_TYPES, "TaskGraphHost"),
        "set_shared_task_host_path",
        "get_shared_task_host_path"
    );

    // Capacity Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_max_dod_entities", "max_entities"), &DataOrientedGame::set_max_dod_entities);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_dod_entities"), &DataOrientedGame::get_max_dod_entities);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_dod_entities"), "set_max_dod_entities", "get_max_dod_entities");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_dod_properties", "max_properties"), &DataOrientedGame::set_max_dod_properties);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_dod_properties"), &DataOrientedGame::get_max_dod_properties);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_dod_properties"), "set_max_dod_properties", "get_max_dod_properties");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_dod_modifiers", "max_modifiers"), &DataOrientedGame::set_max_dod_modifiers);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_dod_modifiers"), &DataOrientedGame::get_max_dod_modifiers);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_dod_modifiers"), "set_max_dod_modifiers", "get_max_dod_modifiers");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_dod_actions", "max_actions"), &DataOrientedGame::set_max_dod_actions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_dod_actions"), &DataOrientedGame::get_max_dod_actions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_dod_actions"), "set_max_dod_actions", "get_max_dod_actions");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_dod_interactions", "max_interactions"), &DataOrientedGame::set_max_dod_interactions);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_dod_interactions"), &DataOrientedGame::get_max_dod_interactions);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_dod_interactions"), "set_max_dod_interactions", "get_max_dod_interactions");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_dod_rules", "max_rules"), &DataOrientedGame::set_max_dod_rules);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_dod_rules"), &DataOrientedGame::get_max_dod_rules);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_dod_rules"), "set_max_dod_rules", "get_max_dod_rules");

    godot::ClassDB::bind_method(godot::D_METHOD("set_max_dod_consequence_commands", "max_commands"), &DataOrientedGame::set_max_dod_consequence_commands);
    godot::ClassDB::bind_method(godot::D_METHOD("get_max_dod_consequence_commands"), &DataOrientedGame::get_max_dod_consequence_commands);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "max_dod_consequence_commands"), "set_max_dod_consequence_commands", "get_max_dod_consequence_commands");

    // Pipeline & TaskGraph Construction Bindings
    godot::ClassDB::bind_method(godot::D_METHOD("setup_dod_pipeline"), &DataOrientedGame::setup_dod_pipeline);
    godot::ClassDB::bind_method(godot::D_METHOD("rebuild_dod_task_graph"), &DataOrientedGame::rebuild_dod_task_graph);
    godot::ClassDB::bind_method(godot::D_METHOD("execute_dod_step", "delta"), &DataOrientedGame::execute_dod_step);
    godot::ClassDB::bind_method(godot::D_METHOD("get_buffer_id", "semantic_name"), &DataOrientedGame::get_buffer_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_buffer_manifest_dictionary"), &DataOrientedGame::get_buffer_manifest_dictionary);

    godot::ClassDB::bind_method(
        godot::D_METHOD("add_games_transform_node", "logic_id", "properties", "priority", "use_registry_factory", "view", "strategy", "element_type"),
        &DataOrientedGame::add_games_transform_node,
        DEFVAL(godot::Dictionary()),
        DEFVAL(0),
        DEFVAL(false),
        DEFVAL(0),
        DEFVAL(0),
        DEFVAL(2)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("add_games_query_node", "logic_id", "query_op", "properties", "priority", "use_registry_factory", "view", "strategy", "element_type"),
        &DataOrientedGame::add_games_query_node,
        DEFVAL(0),
        DEFVAL(godot::Dictionary()),
        DEFVAL(0),
        DEFVAL(false),
        DEFVAL(0),
        DEFVAL(0),
        DEFVAL(2)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("add_games_metadata_node", "logic_id", "properties", "priority", "use_registry_factory", "view", "strategy", "element_type"),
        &DataOrientedGame::add_games_metadata_node,
        DEFVAL(godot::Dictionary()),
        DEFVAL(0),
        DEFVAL(false),
        DEFVAL(0),
        DEFVAL(0),
        DEFVAL(2)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("add_custom_reflection_task_node", "target", "method", "priority"),
        &DataOrientedGame::add_custom_reflection_task_node,
        DEFVAL(0)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("connect_task_nodes", "from_node", "to_node", "from_port", "to_port"),
        &DataOrientedGame::connect_task_nodes,
        DEFVAL(0),
        DEFVAL(0)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("schedule_node_with_stage", "stage_flag", "custom_node_id", "join_downstream"),
        &DataOrientedGame::schedule_node_with_stage,
        DEFVAL(false)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("schedule_node_between_stages", "upstream_stage_flag", "downstream_stage_flag", "custom_node_id"),
        &DataOrientedGame::schedule_node_between_stages
    );
    godot::ClassDB::bind_method(godot::D_METHOD("get_stage_entry_node", "stage_flag"), &DataOrientedGame::get_stage_entry_node);
    godot::ClassDB::bind_method(godot::D_METHOD("get_stage_exit_node", "stage_flag"), &DataOrientedGame::get_stage_exit_node);

    // Entity Registration, Promotion & Demotion Bindings
    godot::ClassDB::bind_method(
        godot::D_METHOD("register_game_entity", "entity", "simulation_tier", "board_partition_id"),
        &DataOrientedGame::register_game_entity,
        DEFVAL(static_cast<int>(DOD_SIM_TIER_HYBRID_SYNCED)),
        DEFVAL(-1)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("unregister_game_entity", "entity"), &DataOrientedGame::unregister_game_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("get_entity_dod_slot", "entity"), &DataOrientedGame::get_entity_dod_slot);
    godot::ClassDB::bind_method(godot::D_METHOD("get_entity_for_dod_slot", "entity_slot"), &DataOrientedGame::get_entity_for_dod_slot);
    godot::ClassDB::bind_method(godot::D_METHOD("set_entity_simulation_tier", "entity", "tier"), &DataOrientedGame::set_entity_simulation_tier);
    godot::ClassDB::bind_method(godot::D_METHOD("set_slot_simulation_tier", "entity_slot", "tier"), &DataOrientedGame::set_slot_simulation_tier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_slot_simulation_tier", "entity_slot"), &DataOrientedGame::get_slot_simulation_tier);
    godot::ClassDB::bind_method(
        godot::D_METHOD("promote_dod_slot_to_node", "entity_slot", "node", "target_tier"),
        &DataOrientedGame::promote_dod_slot_to_node,
        DEFVAL(static_cast<int>(DOD_SIM_TIER_HYBRID_SYNCED))
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("demote_node_to_dod_slot", "entity", "target_tier"),
        &DataOrientedGame::demote_node_to_dod_slot,
        DEFVAL(static_cast<int>(DOD_SIM_TIER_PURE_DOD))
    );
    godot::ClassDB::bind_method(godot::D_METHOD("sync_entity_to_dod", "entity"), &DataOrientedGame::sync_entity_to_dod);
    godot::ClassDB::bind_method(godot::D_METHOD("sync_entity_from_dod", "entity"), &DataOrientedGame::sync_entity_from_dod);

    // Pure-DOD & Property / Modifier / Action / Interaction / Rule Bindings
    godot::ClassDB::bind_method(
        godot::D_METHOD("create_pure_dod_entity", "archetype_mask", "parent_slot", "board_partition_id", "local_time_scale", "group_mask"),
        &DataOrientedGame::create_pure_dod_entity,
        DEFVAL(static_cast<int>(core::ENTITY_ARCHETYPE_PIECE)),
        DEFVAL(-1),
        DEFVAL(-1),
        DEFVAL(1.0f),
        DEFVAL(0)
    );
    godot::ClassDB::bind_method(godot::D_METHOD("free_dod_entity", "entity_slot"), &DataOrientedGame::free_dod_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("register_game_property", "property", "owner_entity_slot"), &DataOrientedGame::register_game_property, DEFVAL(-1));
    godot::ClassDB::bind_method(
        godot::D_METHOD("create_dod_property", "owner_entity_slot", "property_name", "base_value", "min_value", "max_value", "flags"),
        &DataOrientedGame::create_dod_property,
        DEFVAL(100),
        DEFVAL(0),
        DEFVAL(100),
        DEFVAL(static_cast<int>(core::PROP_FLAG_DEFAULT_MASK))
    );
    godot::ClassDB::bind_method(godot::D_METHOD("get_property_dod_slot", "property"), &DataOrientedGame::get_property_dod_slot);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dod_property_modded_value", "property_slot"), &DataOrientedGame::get_dod_property_modded_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dod_property_base_value", "property_slot"), &DataOrientedGame::get_dod_property_base_value);
    godot::ClassDB::bind_method(godot::D_METHOD("set_dod_property_base_value", "property_slot", "base_value"), &DataOrientedGame::set_dod_property_base_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dod_property_snapshot_dict", "property_slot"), &DataOrientedGame::get_dod_property_snapshot_dict);
    godot::ClassDB::bind_method(godot::D_METHOD("bind_property_gauge_to_dod_slot", "gauge", "property_slot"), &DataOrientedGame::bind_property_gauge_to_dod_slot);
    godot::ClassDB::bind_method(godot::D_METHOD("unbind_property_gauge", "gauge"), &DataOrientedGame::unbind_property_gauge);

    godot::ClassDB::bind_method(
        godot::D_METHOD("register_game_property_modifier", "modifier", "target_property_slot", "target_channel"),
        &DataOrientedGame::register_game_property_modifier,
        DEFVAL(static_cast<int>(core::MOD_CHANNEL_VALUE))
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("add_dod_property_modifier", "target_property_slot", "value", "mod_type", "time_limit", "target_channel"),
        &DataOrientedGame::add_dod_property_modifier,
        DEFVAL(static_cast<int>(core::MOD_TYPE_STATIC)),
        DEFVAL(0.0f),
        DEFVAL(static_cast<int>(core::MOD_CHANNEL_VALUE))
    );
    godot::ClassDB::bind_method(godot::D_METHOD("remove_dod_property_modifier", "modifier_slot"), &DataOrientedGame::remove_dod_property_modifier);

    godot::ClassDB::bind_method(godot::D_METHOD("register_game_piece_action", "action", "owner_piece_slot"), &DataOrientedGame::register_game_piece_action, DEFVAL(-1));
    godot::ClassDB::bind_method(godot::D_METHOD("register_game_agent_action", "action", "owner_agent_slot"), &DataOrientedGame::register_game_agent_action, DEFVAL(-1));
    godot::ClassDB::bind_method(godot::D_METHOD("register_game_interaction", "interaction"), &DataOrientedGame::register_game_interaction);

    godot::ClassDB::bind_method(
        godot::D_METHOD("compile_gameplay_style_to_dod", "style", "scope_root_entity_slot"),
        &DataOrientedGame::compile_gameplay_style_to_dod,
        DEFVAL(-1)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("compile_gameplay_rule_to_dod", "rule", "scope_root_entity_slot"),
        &DataOrientedGame::compile_gameplay_rule_to_dod,
        DEFVAL(-1)
    );

    godot::ClassDB::bind_method(godot::D_METHOD("get_active_dod_entity_count"), &DataOrientedGame::get_active_dod_entity_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_dod_property_count"), &DataOrientedGame::get_active_dod_property_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_dod_modifier_count"), &DataOrientedGame::get_active_dod_modifier_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_dod_action_count"), &DataOrientedGame::get_active_dod_action_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_active_dod_interaction_count"), &DataOrientedGame::get_active_dod_interaction_count);
}

DataOrientedGame::DataOrientedGame() = default;
DataOrientedGame::~DataOrientedGame() = default;

void DataOrientedGame::_ensure_game_dod_initialized() {
    if (game_dod_) return;

    core::GamesBufferCapacities caps{};
    caps.max_entities            = static_cast<uint32_t>(std::max(16, max_dod_entities_));
    caps.max_properties          = static_cast<uint32_t>(std::max(16, max_dod_properties_));
    caps.max_modifiers           = static_cast<uint32_t>(std::max(16, max_dod_modifiers_));
    caps.max_actions             = static_cast<uint32_t>(std::max(16, max_dod_actions_));
    caps.max_interactions        = static_cast<uint32_t>(std::max(16, max_dod_interactions_));
    caps.max_compiled_rules      = static_cast<uint32_t>(std::max(16, max_dod_rules_));
    caps.max_consequence_commands= static_cast<uint32_t>(std::max(16, max_dod_consequence_commands_));

    std::shared_ptr<core::MemoryManagerDOD> shared_mgr = nullptr;
    if (dod_execution_mode_ == DODExecutionMode::DOD_EXEC_SHARED_TASK_HOST && !shared_task_host_path_.is_empty()) {
        if (godot::Node* host_node = get_node_or_null(shared_task_host_path_)) {
            if (TaskGraphHost* host = godot::Object::cast_to<TaskGraphHost>(host_node)) {
                if (!host->is_ready()) {
                    host->setup();
                }
                shared_mgr = host->get_active_manager();
                active_task_graph_ = host->get_active_graph();
            }
        }
    }

    game_dod_ = std::make_unique<core::GameDOD>(shared_mgr, caps);
    game_dod_->set_root_time_scale(get_time_scale());
    if (!active_task_graph_) {
        active_task_graph_ = game_dod_->get_default_task_graph();
    }
}

void DataOrientedGame::_ready() {
    Game::_ready();
    if (dod_execution_mode_ != DODExecutionMode::DOD_EXEC_DISABLED) {
        setup_dod_pipeline();
    }
}

void DataOrientedGame::_process(double delta) {
    if (dod_execution_mode_ == DODExecutionMode::DOD_EXEC_DISABLED) {
        Game::_process(delta);
        return;
    }

    _ensure_game_dod_initialized();
    if (game_dod_) {
        game_dod_->set_root_time_scale(get_time_scale());
    }

    const double scaled_delta = delta * get_time_scale();

    // 1. Pull dirty state from HYBRID_SYNCED OOP nodes into DOD columns before graph execution
    _pull_hybrid_entities_to_dod();

    // 2. Run the DOD TaskGraph (unless shared with an external TaskGraphHost that ticks itself)
    if (dod_execution_mode_ != DODExecutionMode::DOD_EXEC_SHARED_TASK_HOST) {
        execute_dod_step(scaled_delta);
    }

    // 3. Push solved DOD values back to HYBRID_SYNCED OOP nodes, drain consequences, and update HUD Gauges
    _push_hybrid_entities_from_dod();
    if (auto_drain_consequences_to_oop_) {
        _drain_consequence_ring_to_oop();
    }
    if (auto_sync_gauges_) {
        _push_telemetry_to_bound_gauges();
    }

    // 4. Run base Game::_process (DataOrientedGameBoard automatically skips OOP game_process for DOD-tier entities)
    Game::_process(delta);
}

// ============================================================================
// Inspector Setters / Getters
// ============================================================================

void DataOrientedGame::set_dod_execution_mode(DODExecutionMode p_mode) {
    dod_execution_mode_ = p_mode;
}
DODExecutionMode DataOrientedGame::get_dod_execution_mode() const {
    return dod_execution_mode_;
}

void DataOrientedGame::set_pipeline_stage_mask(int p_mask) {
    pipeline_stage_mask_ = static_cast<uint32_t>(p_mask) & core::GAMES_STAGE_ALL_MASK;
}
int DataOrientedGame::get_pipeline_stage_mask() const {
    return static_cast<int>(pipeline_stage_mask_);
}

void DataOrientedGame::set_auto_connect_pipeline_stages(bool p_auto_connect) {
    auto_connect_pipeline_stages_ = p_auto_connect;
}
bool DataOrientedGame::get_auto_connect_pipeline_stages() const {
    return auto_connect_pipeline_stages_;
}

void DataOrientedGame::set_auto_drain_consequences_to_oop(bool p_auto_drain) {
    auto_drain_consequences_to_oop_ = p_auto_drain;
}
bool DataOrientedGame::get_auto_drain_consequences_to_oop() const {
    return auto_drain_consequences_to_oop_;
}

void DataOrientedGame::set_auto_sync_gauges(bool p_auto_sync) {
    auto_sync_gauges_ = p_auto_sync;
}
bool DataOrientedGame::get_auto_sync_gauges() const {
    return auto_sync_gauges_;
}

void DataOrientedGame::set_custom_task_graph_resource(const godot::Ref<TaskGraphResource>& p_resource) {
    custom_task_graph_resource_ = p_resource;
}
godot::Ref<TaskGraphResource> DataOrientedGame::get_custom_task_graph_resource() const {
    return custom_task_graph_resource_;
}

void DataOrientedGame::set_shared_task_host_path(const godot::NodePath& p_path) {
    shared_task_host_path_ = p_path;
}
godot::NodePath DataOrientedGame::get_shared_task_host_path() const {
    return shared_task_host_path_;
}

void DataOrientedGame::set_max_dod_entities(int p_max) { max_dod_entities_ = std::max(16, p_max); }
int DataOrientedGame::get_max_dod_entities() const { return max_dod_entities_; }

void DataOrientedGame::set_max_dod_properties(int p_max) { max_dod_properties_ = std::max(16, p_max); }
int DataOrientedGame::get_max_dod_properties() const { return max_dod_properties_; }

void DataOrientedGame::set_max_dod_modifiers(int p_max) { max_dod_modifiers_ = std::max(16, p_max); }
int DataOrientedGame::get_max_dod_modifiers() const { return max_dod_modifiers_; }

void DataOrientedGame::set_max_dod_actions(int p_max) { max_dod_actions_ = std::max(16, p_max); }
int DataOrientedGame::get_max_dod_actions() const { return max_dod_actions_; }

void DataOrientedGame::set_max_dod_interactions(int p_max) { max_dod_interactions_ = std::max(16, p_max); }
int DataOrientedGame::get_max_dod_interactions() const { return max_dod_interactions_; }

void DataOrientedGame::set_max_dod_rules(int p_max) { max_dod_rules_ = std::max(16, p_max); }
int DataOrientedGame::get_max_dod_rules() const { return max_dod_rules_; }

void DataOrientedGame::set_max_dod_consequence_commands(int p_max) { max_dod_consequence_commands_ = std::max(16, p_max); }
int DataOrientedGame::get_max_dod_consequence_commands() const { return max_dod_consequence_commands_; }

// ============================================================================
// Pipeline Setup, Graph Compilation & Custom Wave Co-Scheduling
// ============================================================================

void DataOrientedGame::setup_dod_pipeline() {
    _ensure_game_dod_initialized();
    rebuild_dod_task_graph();

    if (godot::Ref<GameplayStyle> style = get_resolved_gameplay_style().is_valid() ? get_resolved_gameplay_style() : get_gameplay_style(); style.is_valid()) {
        compile_gameplay_style_to_dod(style, static_cast<int>(game_dod_->get_root_game_entity_slot()));
    }

    emit_signal("dod_pipeline_ready");
}

void DataOrientedGame::rebuild_dod_task_graph() {
    _ensure_game_dod_initialized();
    if (!game_dod_) return;

    switch (dod_execution_mode_) {
        case DODExecutionMode::DOD_EXEC_DISABLED:
            return;

        case DODExecutionMode::DOD_EXEC_COMPOSABLE_STAGES: {
            active_task_graph_ = game_dod_->get_default_task_graph();
            if (active_task_graph_) {
                active_task_graph_->clear();
                active_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
                game_dod_->inject_pipeline_stages(*active_task_graph_, pipeline_stage_mask_, auto_connect_pipeline_stages_);
            }
            break;
        }

        case DODExecutionMode::DOD_EXEC_CUSTOM_GRAPH_RESOURCE: {
            if (custom_task_graph_resource_.is_valid() && game_dod_->get_memory_manager_raw()) {
                ui_to_dod_map_.clear();
                active_task_graph_ = custom_task_graph_resource_->compile_to_task_graph(
                    game_dod_->get_memory_manager_raw(),
                    ui_to_dod_map_
                );
                if (active_task_graph_) {
                    game_dod_->set_default_task_graph(active_task_graph_);
                }
            }
            break;
        }

        case DODExecutionMode::DOD_EXEC_SHARED_TASK_HOST: {
            if (!shared_task_host_path_.is_empty()) {
                if (godot::Node* host_node = get_node_or_null(shared_task_host_path_)) {
                    if (TaskGraphHost* host = godot::Object::cast_to<TaskGraphHost>(host_node)) {
                        if (!host->is_ready()) host->setup();
                        active_task_graph_ = host->get_active_graph();
                        if (active_task_graph_ && pipeline_stage_mask_ != 0u) {
                            game_dod_->inject_pipeline_stages(*active_task_graph_, pipeline_stage_mask_, auto_connect_pipeline_stages_);
                        }
                    }
                }
            }
            break;
        }

        case DODExecutionMode::DOD_EXEC_MANUAL_GRAPH: {
            active_task_graph_ = game_dod_->get_default_task_graph();
            if (active_task_graph_) {
                active_task_graph_->clear();
                active_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
            }
            break;
        }
    }
}

void DataOrientedGame::execute_dod_step(double p_delta) {
    if (!game_dod_) return;
    game_dod_->execute_step(p_delta, active_task_graph_.get(), true);
    if (dod_execution_mode_ == DODExecutionMode::DOD_EXEC_CUSTOM_GRAPH_RESOURCE &&
        custom_task_graph_resource_.is_valid() && active_task_graph_) {
        custom_task_graph_resource_->sync_execution_snapshots(active_task_graph_, ui_to_dod_map_);
    }
    emit_signal("dod_step_completed", p_delta);
}

int DataOrientedGame::get_buffer_id(const godot::StringName& p_semantic_name) const {
    if (!game_dod_) return -1;
    godot::String s(p_semantic_name);
    const uint32_t id = game_dod_->get_buffer_id_by_name(s.utf8().get_data());
    return (id == core::INVALID_ID) ? -1 : static_cast<int>(id);
}

godot::Dictionary DataOrientedGame::get_buffer_manifest_dictionary() const {
    godot::Dictionary d;
    if (game_dod_) {
        game_dod_->populate_task_buffer_dictionary(d, true);
    }
    return d;
}

int DataOrientedGame::add_games_transform_node(
    int p_logic_id,
    const godot::Dictionary& p_properties,
    int p_priority,
    bool p_use_registry_factory,
    int p_view,
    int p_strategy,
    int p_element_type
) {
    _ensure_game_dod_initialized();
    if (!game_dod_ || !active_task_graph_) return -1;

    core::GamesNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    cfg.view                 = static_cast<core::MemoryView>(std::clamp(p_view, 0, static_cast<int>(core::MemoryView::Count) - 1));
    cfg.strategy             = static_cast<core::MemoryStrategy>(std::clamp(p_strategy, 0, static_cast<int>(core::MemoryStrategy::Count) - 1));
    cfg.element_type         = static_cast<core::MemoryTypes>(std::clamp(p_element_type, 0, static_cast<int>(core::MemoryTypes::Count) - 1));

    core::NodeID node_id = core::INVALID_ID;
    switch (static_cast<core::GamesTransformLogicID>(p_logic_id)) {
        case core::GamesTransformLogicID::AgentActionSequenceStep: {
            core::AgentActionSequenceStepTransformLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_agent_sequence_step_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::AgentPossessionTransformBlend: {
            core::AgentPossessionTransformBlendTransformLogic<godot::Vector3> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_possession_blend_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::ConsequenceRingDrain: {
            core::ConsequenceRingDrainTransformLogic<void*> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_consequence_drain_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::ControlMapInputFilter: {
            core::ControlMapInputFilterTransformLogic<godot::Vector2> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_control_map_filter_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::EntityTimeScaleCascade: {
            core::EntityTimeScaleCascadeTransformLogic<float> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_time_scale_cascade_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::GameEntitySnapshotSync: {
            core::GameEntitySnapshotSyncTransformLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_snapshot_sync_node(*active_task_graph_, static_cast<core::EntitySnapshotSyncMode>(l.sync_mode), l, cfg);
            break;
        }
        case core::GamesTransformLogicID::GameplayRuleApply: {
            core::GameplayRuleApplyTransformLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_rule_apply_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::InteractionTugOfWarResolve: {
            core::InteractionTugOfWarResolveTransformLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_interaction_resolve_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::PieceActionScoreRefresh: {
            core::PieceActionScoreRefreshTransformLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_piece_action_refresh_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::PropertyModifierAggregate: {
            core::PropertyModifierAggregateTransformLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_modifier_aggregate_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::PropertyModifierTick: {
            core::PropertyModifierTickTransformLogic<float> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_modifier_tick_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesTransformLogicID::PropertyRangeClampAndExhaust: {
            core::PropertyRangeClampAndExhaustTransformLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_property_clamp_node(*active_task_graph_, l, cfg);
            break;
        }
        default:
            break;
    }

    return (node_id == core::INVALID_ID) ? -1 : static_cast<int>(node_id);
}

int DataOrientedGame::add_games_query_node(
    int p_logic_id,
    int p_query_op,
    const godot::Dictionary& p_properties,
    int p_priority,
    bool p_use_registry_factory,
    int p_view,
    int p_strategy,
    int p_element_type
) {
    _ensure_game_dod_initialized();
    if (!game_dod_ || !active_task_graph_) return -1;

    core::GamesNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    cfg.view                 = static_cast<core::MemoryView>(std::clamp(p_view, 0, static_cast<int>(core::MemoryView::Count) - 1));
    cfg.strategy             = static_cast<core::MemoryStrategy>(std::clamp(p_strategy, 0, static_cast<int>(core::MemoryStrategy::Count) - 1));
    cfg.element_type         = static_cast<core::MemoryTypes>(std::clamp(p_element_type, 0, static_cast<int>(core::MemoryTypes::Count) - 1));

    const core::QueryOp op = (p_query_op == static_cast<int>(core::QueryOp::ADD)) ? core::QueryOp::ADD : core::QueryOp::CULL;
    core::NodeID node_id = core::INVALID_ID;

    switch (static_cast<core::GamesQueryLogicID>(p_logic_id)) {
        case core::GamesQueryLogicID::ActionCompetitionConflict: {
            core::ActionCompetitionConflictQueryLogic<int64_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_action_conflict_query_node(*active_task_graph_, l, op, cfg);
            break;
        }
        case core::GamesQueryLogicID::AgentExclusivityFilter: {
            core::AgentExclusivityFilterQueryLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_agent_exclusivity_query_node(*active_task_graph_, l, op, cfg);
            break;
        }
        case core::GamesQueryLogicID::GameplayRuleSelector: {
            core::GameplayRuleSelectorQueryLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_rule_selector_node(*active_task_graph_, l, op, cfg);
            break;
        }
        case core::GamesQueryLogicID::PropertyExhaustedOrLocked: {
            core::PropertyExhaustedOrLockedQueryLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_property_exhausted_query_node(*active_task_graph_, l, op, cfg);
            break;
        }
        default:
            break;
    }

    return (node_id == core::INVALID_ID) ? -1 : static_cast<int>(node_id);
}

int DataOrientedGame::add_games_metadata_node(
    int p_logic_id,
    const godot::Dictionary& p_properties,
    int p_priority,
    bool p_use_registry_factory,
    int p_view,
    int p_strategy,
    int p_element_type
) {
    _ensure_game_dod_initialized();
    if (!game_dod_ || !active_task_graph_) return -1;

    core::GamesNodeExecutionConfig cfg{};
    cfg.execution_priority   = p_priority;
    cfg.use_registry_factory = p_use_registry_factory;
    cfg.view                 = static_cast<core::MemoryView>(std::clamp(p_view, 0, static_cast<int>(core::MemoryView::Count) - 1));
    cfg.strategy             = static_cast<core::MemoryStrategy>(std::clamp(p_strategy, 0, static_cast<int>(core::MemoryStrategy::Count) - 1));
    cfg.element_type         = static_cast<core::MemoryTypes>(std::clamp(p_element_type, 0, static_cast<int>(core::MemoryTypes::Count) - 1));

    core::NodeID node_id = core::INVALID_ID;
    switch (static_cast<core::GamesMetadataLogicID>(p_logic_id)) {
        case core::GamesMetadataLogicID::BoardPresencePartition: {
            core::BoardPresencePartitionMetadataLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_board_partition_node(*active_task_graph_, l, cfg);
            break;
        }
        case core::GamesMetadataLogicID::GameplayRuleSpecificity: {
            core::GameplayRuleSpecificityMetadataLogic<int32_t> l{};
            if (!p_properties.is_empty()) l.apply_properties(p_properties);
            node_id = game_dod_->add_rule_specificity_node(*active_task_graph_, l, cfg);
            break;
        }
        default:
            break;
    }

    return (node_id == core::INVALID_ID) ? -1 : static_cast<int>(node_id);
}

int DataOrientedGame::add_custom_reflection_task_node(
    godot::Object* p_target,
    const godot::StringName& p_method,
    int p_priority
) {
    _ensure_game_dod_initialized();
    if (!active_task_graph_ || !p_target || p_method.is_empty()) return -1;

    const core::NodeID node_id = active_task_graph_->add_task_node(core::TaskTypeDOD::GODOT_REFLECTION);
    active_task_graph_->set_node_priority(node_id, p_priority);
    active_task_graph_->configure_cpu_task(node_id, p_target, p_method);
    return static_cast<int>(node_id);
}

int DataOrientedGame::connect_task_nodes(int p_from_node, int p_to_node, int p_from_port, int p_to_port) {
    if (!active_task_graph_ || p_from_node < 0 || p_to_node < 0) return -1;
    const core::EdgeID edge = active_task_graph_->connect_nodes(
        static_cast<core::NodeID>(p_from_node),
        static_cast<uint32_t>(std::max(0, p_from_port)),
        static_cast<core::NodeID>(p_to_node),
        static_cast<uint32_t>(std::max(0, p_to_port))
    );
    return (edge == core::INVALID_ID) ? -1 : static_cast<int>(edge);
}

bool DataOrientedGame::schedule_node_with_stage(int p_stage_flag, int p_custom_node_id, bool p_join_downstream) {
    if (!game_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::GameDOD::schedule_node_with_stage(
        *active_task_graph_,
        game_dod_->get_active_topology(),
        static_cast<core::GamesPipelineStageFlags>(p_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id),
        p_join_downstream
    );
}

bool DataOrientedGame::schedule_node_between_stages(
    int p_upstream_stage_flag,
    int p_downstream_stage_flag,
    int p_custom_node_id
) {
    if (!game_dod_ || !active_task_graph_ || p_custom_node_id < 0) return false;
    return core::GameDOD::schedule_node_between_stages(
        *active_task_graph_,
        game_dod_->get_active_topology(),
        static_cast<core::GamesPipelineStageFlags>(p_upstream_stage_flag),
        static_cast<core::GamesPipelineStageFlags>(p_downstream_stage_flag),
        static_cast<core::NodeID>(p_custom_node_id)
    );
}

int DataOrientedGame::get_stage_entry_node(int p_stage_flag) const {
    if (!game_dod_) return -1;
    const auto* anchor = game_dod_->get_active_topology().get_stage_anchor(static_cast<core::GamesPipelineStageFlags>(p_stage_flag));
    return (anchor && anchor->entry_node != core::INVALID_ID) ? static_cast<int>(anchor->entry_node) : -1;
}

int DataOrientedGame::get_stage_exit_node(int p_stage_flag) const {
    if (!game_dod_) return -1;
    const auto* anchor = game_dod_->get_active_topology().get_stage_anchor(static_cast<core::GamesPipelineStageFlags>(p_stage_flag));
    return (anchor && anchor->exit_node != core::INVALID_ID) ? static_cast<int>(anchor->exit_node) : -1;
}

// ============================================================================
// OOP <-> DOD Entity Registration, Promotion & Demotion
// ============================================================================

uint32_t DataOrientedGame::_infer_entity_archetype_mask(const GameEntity* p_entity) const {
    if (!p_entity) return core::ENTITY_ARCHETYPE_NONE;
    if (godot::Object::cast_to<GameBoard>(p_entity))       return core::ENTITY_ARCHETYPE_BOARD;
    if (godot::Object::cast_to<GameAgent>(p_entity))       return core::ENTITY_ARCHETYPE_AGENT;
    if (godot::Object::cast_to<GamePiece>(p_entity))       return core::ENTITY_ARCHETYPE_PIECE;
    if (godot::Object::cast_to<GameAgentAction>(p_entity)) return core::ENTITY_ARCHETYPE_AGENT_ACTION;
    if (godot::Object::cast_to<GameInteraction>(p_entity)) return core::ENTITY_ARCHETYPE_INTERACTION;
    return core::ENTITY_ARCHETYPE_PIECE;
}

uint64_t DataOrientedGame::_extract_node_group_mask(const godot::Node* p_node) {
    if (!p_node || !game_dod_) return 0ULL;
    uint64_t mask = 0ULL;
    const godot::String title_str = [&]() -> godot::String {
        if (const auto* ge = godot::Object::cast_to<GameEntity>(p_node)) return ge->get_title();
        return p_node->get_name();
    }();
    if (!title_str.is_empty()) {
        mask |= game_dod_->get_or_register_group_bitmask(title_str.utf8().get_data());
    }
    return mask;
}

uint16_t DataOrientedGame::_pack_property_flags(const GameProperty* p_prop) const {
    if (!p_prop) return core::PROP_FLAG_DEFAULT_MASK;
    uint16_t flags = core::PROP_FLAG_NONE;
    if (p_prop->get_restrict_value_to_current_max_value()) flags |= core::PROP_FLAG_RESTRICT_MAX;
    if (p_prop->get_restrict_value_to_current_min_value()) flags |= core::PROP_FLAG_RESTRICT_MIN;
    if (p_prop->get_allow_value_set())                     flags |= core::PROP_FLAG_ALLOW_VALUE_SET;
    if (p_prop->get_allow_value_modifiers())               flags |= core::PROP_FLAG_ALLOW_VALUE_MODS;
    if (p_prop->get_allow_max_value_set())                 flags |= core::PROP_FLAG_ALLOW_MAX_SET;
    if (p_prop->get_allow_max_modifiers())                 flags |= core::PROP_FLAG_ALLOW_MAX_MODS;
    if (p_prop->get_allow_min_value_set())                 flags |= core::PROP_FLAG_ALLOW_MIN_SET;
    if (p_prop->get_allow_min_modifiers())                 flags |= core::PROP_FLAG_ALLOW_MIN_MODS;
    if (p_prop->get_is_exhausted())                        flags |= core::PROP_FLAG_IS_EXHAUSTED;
    if (p_prop->get_locked())                              flags |= core::PROP_FLAG_LOCKED;
    return flags;
}

void DataOrientedGame::_register_entity_children_recursive(GameEntity* p_entity, uint32_t p_entity_slot) {
    if (!p_entity || !game_dod_) return;

    if (GamePiece* piece = godot::Object::cast_to<GamePiece>(p_entity)) {
        godot::TypedArray<GameProperty> props = piece->get_game_properties();
        for (int i = 0; i < props.size(); ++i) {
            if (GameProperty* prop = godot::Object::cast_to<GameProperty>(props[i])) {
                register_game_property(prop, static_cast<int>(p_entity_slot));
            }
        }
        godot::TypedArray<GamePieceAction> acts = piece->get_actions();
        for (int i = 0; i < acts.size(); ++i) {
            if (GamePieceAction* act = godot::Object::cast_to<GamePieceAction>(acts[i])) {
                register_game_piece_action(act, static_cast<int>(p_entity_slot));
            }
        }
        godot::TypedArray<GamePiece> sub_pieces = piece->get_sub_game_pieces();
        for (int i = 0; i < sub_pieces.size(); ++i) {
            if (GamePiece* sub = godot::Object::cast_to<GamePiece>(sub_pieces[i])) {
                const int sub_slot = register_game_entity(sub, get_slot_simulation_tier(static_cast<int>(p_entity_slot)));
                if (sub_slot >= 0) {
                    core::EntityHierarchyRowPOD sub_row = game_dod_->get_entity_row(static_cast<uint32_t>(sub_slot));
                    sub_row.parent_entity_id = p_entity_slot;
                    game_dod_->set_entity_row(static_cast<uint32_t>(sub_slot), sub_row);
                }
            }
        }
    } else if (GameAgent* agent = godot::Object::cast_to<GameAgent>(p_entity)) {
        godot::TypedArray<GamePiece> pieces = agent->get_game_pieces();
        for (int i = 0; i < pieces.size(); ++i) {
            if (GamePiece* gp = godot::Object::cast_to<GamePiece>(pieces[i])) {
                const int gp_slot = register_game_entity(gp, get_slot_simulation_tier(static_cast<int>(p_entity_slot)));
                if (gp_slot >= 0) {
                    core::EntityHierarchyRowPOD gp_row = game_dod_->get_entity_row(static_cast<uint32_t>(gp_slot));
                    gp_row.parent_entity_id = p_entity_slot;
                    game_dod_->set_entity_row(static_cast<uint32_t>(gp_slot), gp_row);
                }
            }
        }
        godot::TypedArray<GameAgentAction> actions = agent->get_actions();
        for (int i = 0; i < actions.size(); ++i) {
            if (GameAgentAction* aa = godot::Object::cast_to<GameAgentAction>(actions[i])) {
                register_game_agent_action(aa, static_cast<int>(p_entity_slot));
            }
        }
    }
}

int DataOrientedGame::register_game_entity(
    GameEntity* p_entity,
    int p_simulation_tier,
    int p_board_partition_id
) {
    if (!p_entity) return -1;
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    const uint64_t obj_id = p_entity->get_instance_id();
    auto existing = object_id_to_entity_slot_.find(obj_id);
    if (existing != object_id_to_entity_slot_.end()) {
        const uint32_t slot = existing->second;
        game_dod_->set_entity_simulation_tier(
            slot,
            static_cast<core::EntitySimulationTier>(std::clamp(p_simulation_tier, 0, 3)),
            obj_id
        );
        return static_cast<int>(slot);
    }

    core::EntityHierarchyRowPOD row{};
    row.parent_entity_id = game_dod_->get_root_game_entity_slot();
    row.board_id = p_board_partition_id;
    row.archetype_mask = _infer_entity_archetype_mask(p_entity);
    row.group_mask = _extract_node_group_mask(p_entity);
    row.active_flags = core::ENTITY_FLAG_INITIALIZED |
                       (p_entity->get_enabled() ? core::ENTITY_FLAG_ENABLED : 0u) |
                       (p_entity->get_entity_is_paused() ? core::ENTITY_FLAG_PAUSED : 0u) |
                       ((p_simulation_tier != static_cast<int>(DOD_SIM_TIER_FULL_OOP)) ? core::ENTITY_FLAG_PROCESSED : 0u);
    row.local_time_scale = p_entity->get_time_scale();
    row.effective_time_scale = get_time_scale() * p_entity->get_time_scale();
    row.simulation_tier = static_cast<uint8_t>(std::clamp(p_simulation_tier, 0, 3));
    row.object_id = obj_id;

    if (const GameAgent* agent = godot::Object::cast_to<GameAgent>(p_entity)) {
        row.exclusivity = static_cast<uint8_t>(agent->get_exclusivity());
    }

    // Resolve parent entity slot if parent is already registered in DOD
    if (godot::Node* parent_node = p_entity->get_parent()) {
        if (GameEntity* parent_ent = godot::Object::cast_to<GameEntity>(parent_node)) {
            const int parent_slot = get_entity_dod_slot(parent_ent);
            if (parent_slot >= 0) {
                row.parent_entity_id = static_cast<uint32_t>(parent_slot);
            }
        }
    }

    const uint32_t slot = game_dod_->allocate_entity(row);
    if (slot == core::INVALID_ID) return -1;

    object_id_to_entity_slot_[obj_id] = slot;
    entity_slot_to_object_id_[slot] = obj_id;

    _register_entity_children_recursive(p_entity, slot);
    return static_cast<int>(slot);
}

bool DataOrientedGame::unregister_game_entity(GameEntity* p_entity) {
    if (!p_entity || !game_dod_) return false;
    const uint64_t obj_id = p_entity->get_instance_id();
    auto it = object_id_to_entity_slot_.find(obj_id);
    if (it == object_id_to_entity_slot_.end()) return false;

    const uint32_t slot = it->second;
    object_id_to_entity_slot_.erase(it);
    entity_slot_to_object_id_.erase(slot);
    return game_dod_->free_entity(slot);
}

int DataOrientedGame::get_entity_dod_slot(const GameEntity* p_entity) const {
    if (!p_entity) return -1;
    auto it = object_id_to_entity_slot_.find(p_entity->get_instance_id());
    return (it != object_id_to_entity_slot_.end()) ? static_cast<int>(it->second) : -1;
}

GameEntity* DataOrientedGame::get_entity_for_dod_slot(int p_entity_slot) const {
    if (p_entity_slot < 0) return nullptr;
    auto it = entity_slot_to_object_id_.find(static_cast<uint32_t>(p_entity_slot));
    if (it == entity_slot_to_object_id_.end() || it->second == 0ULL) return nullptr;
    return godot::Object::cast_to<GameEntity>(godot::ObjectDB::get_instance(godot::ObjectID(it->second)));
}

bool DataOrientedGame::set_entity_simulation_tier(GameEntity* p_entity, int p_tier) {
    if (!p_entity) return false;
    const int slot = get_entity_dod_slot(p_entity);
    if (slot < 0) {
        return register_game_entity(p_entity, p_tier) >= 0;
    }
    return set_slot_simulation_tier(slot, p_tier);
}

bool DataOrientedGame::set_slot_simulation_tier(int p_entity_slot, int p_tier) {
    if (!game_dod_ || p_entity_slot < 0) return false;
    const uint32_t u_slot = static_cast<uint32_t>(p_entity_slot);
    uint64_t obj_id = 0ULL;
    if (auto it = entity_slot_to_object_id_.find(u_slot); it != entity_slot_to_object_id_.end()) {
        obj_id = it->second;
    }
    return game_dod_->set_entity_simulation_tier(
        u_slot,
        static_cast<core::EntitySimulationTier>(std::clamp(p_tier, 0, 3)),
        obj_id
    );
}

int DataOrientedGame::get_slot_simulation_tier(int p_entity_slot) const {
    if (!game_dod_ || p_entity_slot < 0) return static_cast<int>(DOD_SIM_TIER_PURE_DOD);
    return static_cast<int>(game_dod_->get_entity_simulation_tier(static_cast<uint32_t>(p_entity_slot)));
}

bool DataOrientedGame::promote_dod_slot_to_node(int p_entity_slot, GameEntity* p_node, int p_target_tier) {
    if (!game_dod_ || p_entity_slot < 0 || !p_node) return false;
    const uint32_t u_slot = static_cast<uint32_t>(p_entity_slot);
    if (!game_dod_->is_entity_slot_valid(u_slot)) return false;

    const uint64_t obj_id = p_node->get_instance_id();
    object_id_to_entity_slot_[obj_id] = u_slot;
    entity_slot_to_object_id_[u_slot] = obj_id;

    game_dod_->set_entity_simulation_tier(
        u_slot,
        static_cast<core::EntitySimulationTier>(std::clamp(p_target_tier, 0, 3)),
        obj_id
    );

    sync_entity_from_dod(p_node);
    return true;
}

int DataOrientedGame::demote_node_to_dod_slot(GameEntity* p_entity, int p_target_tier) {
    if (!p_entity || !game_dod_) return -1;
    int slot = get_entity_dod_slot(p_entity);
    if (slot < 0) {
        slot = register_game_entity(p_entity, p_target_tier);
    } else {
        sync_entity_to_dod(p_entity);
    }
    if (slot < 0) return -1;

    const uint64_t obj_id = p_entity->get_instance_id();
    const uint32_t u_slot = static_cast<uint32_t>(slot);
    object_id_to_entity_slot_.erase(obj_id);
    entity_slot_to_object_id_.erase(u_slot);

    game_dod_->set_entity_simulation_tier(
        u_slot,
        static_cast<core::EntitySimulationTier>(std::clamp(p_target_tier, 0, 3)),
        0ULL
    );
    return slot;
}

void DataOrientedGame::sync_entity_to_dod(GameEntity* p_entity) {
    if (!p_entity || !game_dod_) return;
    const int slot = get_entity_dod_slot(p_entity);
    if (slot < 0) return;

    core::EntityHierarchyRowPOD row = game_dod_->get_entity_row(static_cast<uint32_t>(slot));
    row.local_time_scale = p_entity->get_time_scale();
    if (p_entity->get_enabled()) {
        row.active_flags |= core::ENTITY_FLAG_ENABLED;
    } else {
        row.active_flags &= ~static_cast<uint32_t>(core::ENTITY_FLAG_ENABLED);
    }
    if (p_entity->get_entity_is_paused()) {
        row.active_flags |= core::ENTITY_FLAG_PAUSED;
    } else {
        row.active_flags &= ~static_cast<uint32_t>(core::ENTITY_FLAG_PAUSED);
    }
    game_dod_->set_entity_row(static_cast<uint32_t>(slot), row);

    if (GamePiece* piece = godot::Object::cast_to<GamePiece>(p_entity)) {
        godot::TypedArray<GameProperty> props = piece->get_game_properties();
        for (int i = 0; i < props.size(); ++i) {
            if (GameProperty* prop = godot::Object::cast_to<GameProperty>(props[i])) {
                const int p_slot = get_property_dod_slot(prop);
                if (p_slot >= 0) {
                    core::GamePropertyRowPOD p_row = game_dod_->get_property_row(static_cast<uint32_t>(p_slot));
                    p_row.base_value = prop->get_value();
                    p_row.base_min   = prop->get_min_value();
                    p_row.base_max   = prop->get_max_value();
                    p_row.property_flags = _pack_property_flags(prop);
                    game_dod_->set_property_row(static_cast<uint32_t>(p_slot), p_row);
                }
            }
        }
    }
}

void DataOrientedGame::sync_entity_from_dod(GameEntity* p_entity) {
    if (!p_entity || !game_dod_) return;
    const int slot = get_entity_dod_slot(p_entity);
    if (slot < 0) return;

    const core::EntityHierarchyRowPOD row = game_dod_->get_entity_row(static_cast<uint32_t>(slot));
    p_entity->set_time_scale(row.local_time_scale);
    p_entity->set_enabled((row.active_flags & core::ENTITY_FLAG_ENABLED) != 0u);
    p_entity->set_entity_is_paused((row.active_flags & core::ENTITY_FLAG_PAUSED) != 0u);

    if (GamePiece* piece = godot::Object::cast_to<GamePiece>(p_entity)) {
        godot::TypedArray<GameProperty> props = piece->get_game_properties();
        for (int i = 0; i < props.size(); ++i) {
            if (GameProperty* prop = godot::Object::cast_to<GameProperty>(props[i])) {
                const int p_slot = get_property_dod_slot(prop);
                if (p_slot >= 0) {
                    const core::GamePropertyRowPOD p_row = game_dod_->get_property_row(static_cast<uint32_t>(p_slot));
                    prop->set_is_exhausted((p_row.property_flags & core::PROP_FLAG_IS_EXHAUSTED) != 0u);
                    prop->set_locked((p_row.property_flags & core::PROP_FLAG_LOCKED) != 0u);
                }
            }
        }
    }
}

void DataOrientedGame::_pull_hybrid_entities_to_dod() {
    if (!game_dod_) return;
    for (const auto& [obj_id, prop_slot] : object_id_to_property_slot_) {
        if (GameProperty* prop = godot::Object::cast_to<GameProperty>(godot::ObjectDB::get_instance(godot::ObjectID(obj_id)))) {
            core::GamePropertyRowPOD row = game_dod_->get_property_row(prop_slot);
            if (row.owner_entity_id != core::INVALID_ID) {
                const auto tier = game_dod_->get_entity_simulation_tier(row.owner_entity_id);
                if (tier != core::SIM_TIER_HYBRID_SYNCED && tier != core::SIM_TIER_FULL_OOP) continue;
            }
            row.base_value = prop->get_value();
            row.base_min   = prop->get_min_value();
            row.base_max   = prop->get_max_value();
            row.property_flags = _pack_property_flags(prop);
            game_dod_->set_property_row(prop_slot, row);
        }
    }
}

void DataOrientedGame::_push_hybrid_entities_from_dod() {
    if (!game_dod_) return;
    for (const auto& [prop_slot, obj_id] : property_slot_to_object_id_) {
        if (GameProperty* prop = godot::Object::cast_to<GameProperty>(godot::ObjectDB::get_instance(godot::ObjectID(obj_id)))) {
            const core::GamePropertyRowPOD row = game_dod_->get_property_row(prop_slot);
            if (row.owner_entity_id != core::INVALID_ID) {
                const auto tier = game_dod_->get_entity_simulation_tier(row.owner_entity_id);
                if (tier != core::SIM_TIER_HYBRID_SYNCED) continue;
            }
            prop->set_is_exhausted((row.property_flags & core::PROP_FLAG_IS_EXHAUSTED) != 0u);
            prop->set_locked((row.property_flags & core::PROP_FLAG_LOCKED) != 0u);
        }
    }
}

void DataOrientedGame::_drain_consequence_ring_to_oop() {
    if (!game_dod_) return;
    std::vector<core::ConsequenceCommandPOD> commands;
    const uint32_t count = game_dod_->drain_consequence_commands(commands);
    if (count == 0u) return;

    for (const auto& cmd : commands) {
        emit_signal(
            "dod_consequence_applied",
            static_cast<int>(cmd.target_entity_idx),
            static_cast<int>(cmd.score_delta),
            static_cast<int>(cmd.consequence_mask)
        );

        if ((cmd.consequence_mask & core::CONSEQUENCE_FLAG_EMIT_SIGNAL) != 0u) {
            if (GameEntity* ent = get_entity_for_dod_slot(static_cast<int>(cmd.target_entity_idx))) {
                godot::Dictionary d;
                d["consequence_mask"] = static_cast<int64_t>(cmd.consequence_mask);
                d["score_delta"] = cmd.score_delta;
                ent->action_consequences(cmd.score_delta, d);
            }
        }
    }
}

void DataOrientedGame::_push_telemetry_to_bound_gauges() {
    if (!game_dod_ || bound_gauges_.empty()) return;

    for (auto it = bound_gauges_.begin(); it != bound_gauges_.end();) {
        auto* gauge = godot::Object::cast_to<GamePropertyGauge>(
            godot::ObjectDB::get_instance(godot::ObjectID(it->gauge_object_id))
        );
        if (!gauge || !game_dod_->is_property_slot_valid(it->property_slot)) {
            it = bound_gauges_.erase(it);
            continue;
        }

        gauge->apply_property_snapshot(get_dod_property_snapshot_dict(static_cast<int>(it->property_slot)));
        ++it;
    }
}

// ============================================================================
// Pure-DOD & Hybrid Property, Modifier, Action, Interaction & Rule API
// ============================================================================

int DataOrientedGame::create_pure_dod_entity(
    int p_archetype_mask,
    int p_parent_slot,
    int p_board_partition_id,
    float p_local_time_scale,
    int64_t p_group_mask
) {
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    core::EntityHierarchyRowPOD row{};
    row.parent_entity_id = (p_parent_slot >= 0)
        ? static_cast<uint32_t>(p_parent_slot)
        : game_dod_->get_root_game_entity_slot();
    row.board_id = p_board_partition_id;
    row.archetype_mask = static_cast<uint32_t>(p_archetype_mask);
    row.group_mask = static_cast<uint64_t>(p_group_mask);
    row.active_flags = core::ENTITY_FLAG_ENABLED | core::ENTITY_FLAG_INITIALIZED | core::ENTITY_FLAG_PROCESSED;
    row.local_time_scale = p_local_time_scale;
    row.effective_time_scale = get_time_scale() * p_local_time_scale;
    row.simulation_tier = core::SIM_TIER_PURE_DOD;
    row.object_id = 0ULL;

    const uint32_t slot = game_dod_->allocate_entity(row);
    return (slot == core::INVALID_ID) ? -1 : static_cast<int>(slot);
}

bool DataOrientedGame::free_dod_entity(int p_entity_slot) {
    if (!game_dod_ || p_entity_slot < 0) return false;
    const uint32_t u_slot = static_cast<uint32_t>(p_entity_slot);
    if (auto it = entity_slot_to_object_id_.find(u_slot); it != entity_slot_to_object_id_.end()) {
        object_id_to_entity_slot_.erase(it->second);
        entity_slot_to_object_id_.erase(it);
    }
    return game_dod_->free_entity(u_slot);
}

int DataOrientedGame::register_game_property(GameProperty* p_property, int p_owner_entity_slot) {
    if (!p_property) return -1;
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    const uint64_t obj_id = p_property->get_instance_id();
    if (auto it = object_id_to_property_slot_.find(obj_id); it != object_id_to_property_slot_.end()) {
        return static_cast<int>(it->second);
    }

    uint32_t owner_slot = (p_owner_entity_slot >= 0)
        ? static_cast<uint32_t>(p_owner_entity_slot)
        : game_dod_->get_root_game_entity_slot();

    if (p_owner_entity_slot < 0) {
        if (GameEntity* parent_ent = godot::Object::cast_to<GameEntity>(p_property->get_parent())) {
            const int found = get_entity_dod_slot(parent_ent);
            if (found >= 0) owner_slot = static_cast<uint32_t>(found);
        }
    }

    const godot::String prop_title = p_property->get_title().is_empty() ? godot::String(p_property->get_name()) : p_property->get_title();

    core::GamePropertyRowPOD row{};
    row.owner_entity_id  = owner_slot;
    row.property_name_id = game_dod_->intern_name_token(prop_title.utf8().get_data());
    row.base_value       = p_property->get_value();
    row.modded_value     = p_property->get_modded_value();
    row.base_min         = p_property->get_min_value();
    row.modded_min       = p_property->get_modded_min_value();
    row.base_max         = p_property->get_max_value();
    row.modded_max       = p_property->get_modded_max_value();
    row.property_flags   = _pack_property_flags(p_property);
    row.object_id        = obj_id;

    const uint32_t slot = game_dod_->allocate_property(row);
    if (slot == core::INVALID_ID) return -1;

    object_id_to_property_slot_[obj_id] = slot;
    property_slot_to_object_id_[slot] = obj_id;

    // Also register any attached GamePropertyModifiers across value, max, and min channels
    auto reg_mods = [&](const godot::TypedArray<GamePropertyModifier>& p_mods, int p_channel) {
        for (int i = 0; i < p_mods.size(); ++i) {
            if (GamePropertyModifier* mod = godot::Object::cast_to<GamePropertyModifier>(p_mods[i])) {
                register_game_property_modifier(mod, static_cast<int>(slot), p_channel);
            }
        }
    };
    reg_mods(p_property->get_modifiers(), static_cast<int>(core::MOD_CHANNEL_VALUE));
    reg_mods(p_property->get_max_modifiers(), static_cast<int>(core::MOD_CHANNEL_MAX));
    reg_mods(p_property->get_min_modifiers(), static_cast<int>(core::MOD_CHANNEL_MIN));

    return static_cast<int>(slot);
}

int DataOrientedGame::create_dod_property(
    int p_owner_entity_slot,
    const godot::StringName& p_property_name,
    int p_base_value,
    int p_min_value,
    int p_max_value,
    int p_flags
) {
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    const godot::String name_str(p_property_name);
    core::GamePropertyRowPOD row{};
    row.owner_entity_id  = (p_owner_entity_slot >= 0) ? static_cast<uint32_t>(p_owner_entity_slot) : game_dod_->get_root_game_entity_slot();
    row.property_name_id = game_dod_->intern_name_token(name_str.utf8().get_data());
    row.base_value       = p_base_value;
    row.modded_value     = p_base_value;
    row.base_min         = p_min_value;
    row.modded_min       = p_min_value;
    row.base_max         = p_max_value;
    row.modded_max       = p_max_value;
    row.property_flags   = static_cast<uint16_t>(p_flags);

    const uint32_t slot = game_dod_->allocate_property(row);
    return (slot == core::INVALID_ID) ? -1 : static_cast<int>(slot);
}

int DataOrientedGame::get_property_dod_slot(const GameProperty* p_property) const {
    if (!p_property) return -1;
    auto it = object_id_to_property_slot_.find(p_property->get_instance_id());
    return (it != object_id_to_property_slot_.end()) ? static_cast<int>(it->second) : -1;
}

int DataOrientedGame::get_dod_property_modded_value(int p_property_slot) const {
    if (!game_dod_ || p_property_slot < 0) return 0;
    return game_dod_->get_property_row(static_cast<uint32_t>(p_property_slot)).modded_value;
}

int DataOrientedGame::get_dod_property_base_value(int p_property_slot) const {
    if (!game_dod_ || p_property_slot < 0) return 0;
    return game_dod_->get_property_row(static_cast<uint32_t>(p_property_slot)).base_value;
}

void DataOrientedGame::set_dod_property_base_value(int p_property_slot, int p_base_value) {
    if (!game_dod_ || p_property_slot < 0) return;
    core::GamePropertyRowPOD row = game_dod_->get_property_row(static_cast<uint32_t>(p_property_slot));
    row.base_value = p_base_value;
    game_dod_->set_property_row(static_cast<uint32_t>(p_property_slot), row);
}

godot::Dictionary DataOrientedGame::get_dod_property_snapshot_dict(int p_property_slot) const {
    godot::Dictionary d;
    if (!game_dod_ || p_property_slot < 0) return d;

    const uint32_t u_slot = static_cast<uint32_t>(p_property_slot);
    const core::GamePropertyRowPOD row = game_dod_->get_property_row(u_slot);
    d["base_value"]       = row.base_value;
    d["modded_value"]     = row.modded_value;
    d["base_max_value"]   = row.base_max;
    d["modded_max_value"] = row.modded_max;
    d["base_min_value"]   = row.base_min;
    d["modded_min_value"] = row.modded_min;
    d["is_exhausted"]     = (row.property_flags & core::PROP_FLAG_IS_EXHAUSTED) != 0u;
    d["is_locked"]        = (row.property_flags & core::PROP_FLAG_LOCKED) != 0u;

    godot::Array timed_arr;
    const uint32_t mod_hwm = game_dod_->get_modifier_high_water_mark();
    for (uint32_t i = 0; i < mod_hwm; ++i) {
        if (!game_dod_->is_modifier_slot_valid(i)) continue;
        const core::PropertyModifierPOD mod = game_dod_->get_modifier_pod(i);
        if (mod.target_property_idx != u_slot || (mod.flags & core::MOD_FLAG_ACTIVE) == 0u) continue;
        if (mod.mod_type != core::MOD_TYPE_TIMED && mod.mod_type != core::MOD_TYPE_TIMED_DIMINISHING) continue;
        if (mod.time_limit <= 0.0001f) continue;

        godot::Dictionary md;
        md["value"] = mod.value;
        md["time_limit"] = mod.time_limit;
        md["current_time"] = mod.current_time;
        md["remaining_ratio"] = std::clamp(mod.current_time / mod.time_limit, 0.0f, 1.0f);
        md["is_diminishing"] = (mod.mod_type == core::MOD_TYPE_TIMED_DIMINISHING);
        md["target_channel"] = static_cast<int>(mod.target_channel);
        timed_arr.append(md);
    }
    d["timed_modifiers"] = timed_arr;
    return d;
}

void DataOrientedGame::bind_property_gauge_to_dod_slot(GamePropertyGauge* p_gauge, int p_property_slot) {
    if (!p_gauge || p_property_slot < 0) return;
    p_gauge->set_poll_bound_entity(false); // Driven directly from DOD snapshot buffer!
    const uint64_t g_id = p_gauge->get_instance_id();

    for (auto& entry : bound_gauges_) {
        if (entry.gauge_object_id == g_id) {
            entry.property_slot = static_cast<uint32_t>(p_property_slot);
            p_gauge->apply_property_snapshot(get_dod_property_snapshot_dict(p_property_slot));
            return;
        }
    }

    bound_gauges_.push_back(BoundGaugeEntry{g_id, static_cast<uint32_t>(p_property_slot)});
    p_gauge->apply_property_snapshot(get_dod_property_snapshot_dict(p_property_slot));
}

void DataOrientedGame::unbind_property_gauge(GamePropertyGauge* p_gauge) {
    if (!p_gauge) return;
    const uint64_t g_id = p_gauge->get_instance_id();
    bound_gauges_.erase(
        std::remove_if(bound_gauges_.begin(), bound_gauges_.end(), [&](const BoundGaugeEntry& e) {
            return e.gauge_object_id == g_id;
        }),
        bound_gauges_.end()
    );
}

int DataOrientedGame::register_game_property_modifier(
    GamePropertyModifier* p_modifier,
    int p_target_property_slot,
    int p_target_channel
) {
    if (!p_modifier || p_target_property_slot < 0) return -1;
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    const uint64_t obj_id = p_modifier->get_instance_id();
    if (auto it = object_id_to_modifier_slot_.find(obj_id); it != object_id_to_modifier_slot_.end()) {
        return static_cast<int>(it->second);
    }

    const int slot = add_dod_property_modifier(
        p_target_property_slot,
        p_modifier->get_value(),
        static_cast<int>(p_modifier->get_type()),
        p_modifier->get_time_limit(),
        p_target_channel
    );
    if (slot >= 0) {
        object_id_to_modifier_slot_[obj_id] = static_cast<uint32_t>(slot);
    }
    return slot;
}

int DataOrientedGame::add_dod_property_modifier(
    int p_target_property_slot,
    int p_value,
    int p_mod_type,
    float p_time_limit,
    int p_target_channel
) {
    _ensure_game_dod_initialized();
    if (!game_dod_ || p_target_property_slot < 0) return -1;

    core::PropertyModifierPOD mod{};
    mod.target_property_idx = static_cast<uint32_t>(p_target_property_slot);
    mod.value               = p_value;
    mod.starting_value      = p_value;
    mod.time_limit          = p_time_limit;
    mod.current_time        = p_time_limit;
    mod.mod_type            = static_cast<uint8_t>(std::clamp(p_mod_type, 0, 3));
    mod.target_channel      = static_cast<uint8_t>(std::clamp(p_target_channel, 0, 2));
    mod.flags               = core::MOD_FLAG_ACTIVE | core::MOD_FLAG_PROCESSED;

    const uint32_t slot = game_dod_->allocate_modifier(mod);
    return (slot == core::INVALID_ID) ? -1 : static_cast<int>(slot);
}

bool DataOrientedGame::remove_dod_property_modifier(int p_modifier_slot) {
    if (!game_dod_ || p_modifier_slot < 0) return false;
    return game_dod_->free_modifier(static_cast<uint32_t>(p_modifier_slot));
}

int DataOrientedGame::register_game_piece_action(GamePieceAction* p_action, int p_owner_piece_slot) {
    if (!p_action) return -1;
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    const uint64_t obj_id = p_action->get_instance_id();
    if (auto it = object_id_to_action_slot_.find(obj_id); it != object_id_to_action_slot_.end()) {
        return static_cast<int>(it->second);
    }

    std::vector<core::PiecePropertyBindingPOD> bindings;
    godot::TypedArray<GameProperty> props = p_action->get_game_properties();
    godot::TypedArray<int> use_vals = p_action->get_property_use_values();
    godot::TypedArray<bool> locks = p_action->get_property_locks();

    for (int i = 0; i < props.size(); ++i) {
        if (GameProperty* prop = godot::Object::cast_to<GameProperty>(props[i])) {
            const int p_slot = register_game_property(prop, p_owner_piece_slot);
            if (p_slot >= 0) {
                core::PiecePropertyBindingPOD b{};
                b.property_idx      = static_cast<uint32_t>(p_slot);
                b.property_use_value= (i < use_vals.size()) ? static_cast<int32_t>(use_vals[i]) : 1;
                b.lock_when_in_use  = (i < locks.size() && static_cast<bool>(locks[i])) ? 1u : 0u;
                b.require_available = 1u;
                bindings.push_back(b);
            }
        }
    }

    uint64_t comp_mask = 0ULL;
    godot::TypedArray<godot::String> comp_groups = p_action->get_competes_with_groups();
    for (int i = 0; i < comp_groups.size(); ++i) {
        const godot::String g = comp_groups[i];
        comp_mask |= game_dod_->get_or_register_group_bitmask(g.utf8().get_data());
    }

    core::ActionSequenceRowPOD row{};
    row.owner_agent_id   = (p_owner_piece_slot >= 0) ? static_cast<uint32_t>(p_owner_piece_slot) : core::INVALID_ID;
    row.base_score       = p_action->get_action_value();
    row.current_score    = row.base_score;
    row.status           = core::ACTION_STATUS_AVAILABLE;
    row.competition_mask = comp_mask;
    row.refresh_time     = p_action->get_refresh_time();

    const uint32_t slot = game_dod_->allocate_action(row, {}, bindings);
    if (slot == core::INVALID_ID) return -1;

    object_id_to_action_slot_[obj_id] = slot;
    return static_cast<int>(slot);
}

int DataOrientedGame::register_game_agent_action(GameAgentAction* p_action, int p_owner_agent_slot) {
    if (!p_action) return -1;
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    const uint64_t obj_id = p_action->get_instance_id();
    if (auto it = object_id_to_action_slot_.find(obj_id); it != object_id_to_action_slot_.end()) {
        return static_cast<int>(it->second);
    }

    uint64_t comp_mask = 0ULL;
    godot::TypedArray<godot::String> comp_groups = p_action->get_competes_with_groups();
    for (int i = 0; i < comp_groups.size(); ++i) {
        const godot::String g = comp_groups[i];
        comp_mask |= game_dod_->get_or_register_group_bitmask(g.utf8().get_data());
    }

    std::vector<core::SequenceStepPOD> steps;
    godot::TypedArray<GamePiece> pieces = p_action->get_game_pieces();
    godot::Array seq = p_action->get_sequence();
    for (int i = 0; i < seq.size(); ++i) {
        if (seq[i].get_type() != godot::Variant::ARRAY) continue;
        godot::Array step_arr = seq[i];
        for (int j = 0; j < step_arr.size(); ++j) {
            if (step_arr[j].get_type() != godot::Variant::ARRAY) continue;
            godot::Array pair = step_arr[j];
            if (pair.size() < 2) continue;

            const int piece_local_idx = static_cast<int>(pair[0]);
            const int act_local_idx   = static_cast<int>(pair[1]);
            if (piece_local_idx >= 0 && piece_local_idx < pieces.size()) {
                if (GamePiece* gp = godot::Object::cast_to<GamePiece>(pieces[piece_local_idx])) {
                    const int gp_slot = register_game_entity(gp);
                    godot::TypedArray<GamePieceAction> gp_acts = gp->get_actions();
                    if (act_local_idx >= 0 && act_local_idx < gp_acts.size()) {
                        if (GamePieceAction* gpa = godot::Object::cast_to<GamePieceAction>(gp_acts[act_local_idx])) {
                            const int gpa_slot = register_game_piece_action(gpa, gp_slot);
                            if (gp_slot >= 0 && gpa_slot >= 0) {
                                steps.push_back(core::SequenceStepPOD{
                                    static_cast<uint32_t>(gp_slot),
                                    static_cast<uint32_t>(gpa_slot)
                                });
                            }
                        }
                    }
                }
            }
        }
    }

    core::ActionSequenceRowPOD row{};
    row.owner_agent_id    = (p_owner_agent_slot >= 0) ? static_cast<uint32_t>(p_owner_agent_slot) : core::INVALID_ID;
    row.priority          = p_action->get_priority();
    row.action_group_mask = _extract_node_group_mask(p_action);
    row.competition_mask  = comp_mask;
    row.status            = p_action->get_in_progress() ? core::ACTION_STATUS_IN_USE : core::ACTION_STATUS_AVAILABLE;
    row.current_step_idx  = std::max(0, p_action->get_current_sequence_id());

    const uint32_t slot = game_dod_->allocate_action(row, steps, {});
    if (slot == core::INVALID_ID) return -1;

    object_id_to_action_slot_[obj_id] = slot;
    return static_cast<int>(slot);
}

int DataOrientedGame::register_game_interaction(GameInteraction* p_interaction) {
    if (!p_interaction) return -1;
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    const uint64_t obj_id = p_interaction->get_instance_id();
    if (auto it = object_id_to_interaction_slot_.find(obj_id); it != object_id_to_interaction_slot_.end()) {
        return static_cast<int>(it->second);
    }

    std::vector<core::InteractionParticipantPOD> participants;
    auto parse_participant = [&](const godot::Array& p_arr, core::InteractionParticipantRole p_role) {
        if (p_arr.size() < 2) return;
        GameEntity* ent = godot::Object::cast_to<GameEntity>(p_arr[0]);
        const int act_idx = static_cast<int>(p_arr[1]);
        if (!ent) return;

        const int ent_slot = register_game_entity(ent);
        uint32_t dod_act_slot = core::INVALID_ID;
        if (GameAgent* ag = godot::Object::cast_to<GameAgent>(ent)) {
            godot::TypedArray<GameAgentAction> acts = ag->get_actions();
            if (act_idx >= 0 && act_idx < acts.size()) {
                if (GameAgentAction* aa = godot::Object::cast_to<GameAgentAction>(acts[act_idx])) {
                    const int s = register_game_agent_action(aa, ent_slot);
                    if (s >= 0) dod_act_slot = static_cast<uint32_t>(s);
                }
            }
        }
        if (ent_slot >= 0) {
            core::InteractionParticipantPOD pod{};
            pod.agent_entity_idx  = static_cast<uint32_t>(ent_slot);
            pod.action_idx        = dod_act_slot;
            pod.weight_multiplier = 1.0f;
            pod.role              = static_cast<uint8_t>(p_role);
            pod.active            = 1u;
            participants.push_back(pod);
        }
    };

    parse_participant(p_interaction->get_instigator(), core::PARTICIPANT_ROLE_INSTIGATOR);
    godot::Array coop = p_interaction->get_cooperation();
    for (int i = 0; i < coop.size(); ++i) {
        if (coop[i].get_type() == godot::Variant::ARRAY) {
            parse_participant(coop[i], core::PARTICIPANT_ROLE_COOPERATION);
        }
    }
    godot::Array comp = p_interaction->get_competition();
    for (int i = 0; i < comp.size(); ++i) {
        if (comp[i].get_type() == godot::Variant::ARRAY) {
            parse_participant(comp[i], core::PARTICIPANT_ROLE_COMPETITION);
        }
    }

    core::InteractionArenaRowPOD row{};
    if (!participants.empty()) {
        row.instigator_action_idx = participants.front().action_idx;
    }

    const uint32_t slot = game_dod_->allocate_interaction(row, participants);
    if (slot == core::INVALID_ID) return -1;

    object_id_to_interaction_slot_[obj_id] = slot;
    return static_cast<int>(slot);
}

int DataOrientedGame::compile_gameplay_rule_to_dod(
    const godot::Ref<GameplayRule>& p_rule,
    int p_scope_root_entity_slot
) {
    if (p_rule.is_null()) return -1;
    _ensure_game_dod_initialized();
    if (!game_dod_) return -1;

    core::CompiledGameplayRulePOD pod{};
    pod.target_entity_id = (p_scope_root_entity_slot >= 0 && p_rule->get_scope() == RuleScope::SELF)
        ? static_cast<uint32_t>(p_scope_root_entity_slot)
        : core::INVALID_ID;

    const godot::String t_class(p_rule->get_target_class());
    if (t_class == "GameBoard")            pod.required_archetype_mask = core::ENTITY_ARCHETYPE_BOARD;
    else if (t_class == "GameAgent")       pod.required_archetype_mask = core::ENTITY_ARCHETYPE_AGENT;
    else if (t_class == "GamePiece")       pod.required_archetype_mask = core::ENTITY_ARCHETYPE_PIECE;
    else if (t_class == "GameAgentAction") pod.required_archetype_mask = core::ENTITY_ARCHETYPE_AGENT_ACTION;
    else if (t_class == "GamePieceAction") pod.required_archetype_mask = core::ENTITY_ARCHETYPE_PIECE_ACTION;
    else if (t_class == "GameInteraction") pod.required_archetype_mask = core::ENTITY_ARCHETYPE_INTERACTION;
    else                                   pod.required_archetype_mask = core::ENTITY_ARCHETYPE_ALL;

    const godot::String t_group(p_rule->get_target_title_or_group());
    pod.required_group_mask = t_group.is_empty()
        ? 0ULL
        : game_dod_->get_or_register_group_bitmask(t_group.utf8().get_data());

    pod.value     = static_cast<float>(p_rule->get_value());
    pod.clamp_min = static_cast<float>(p_rule->get_clamp_min());
    pod.clamp_max = static_cast<float>(p_rule->get_clamp_max());

    switch (p_rule->get_operation()) {
        case RuleOp::SET:                pod.operation = core::RULE_OP_SET; break;
        case RuleOp::ADD:
        case RuleOp::MODIFIER_ADD:       pod.operation = core::RULE_OP_ADD; break;
        case RuleOp::MULTIPLY:           pod.operation = core::RULE_OP_MULTIPLY; break;
        case RuleOp::CLAMP:              pod.operation = core::RULE_OP_CLAMP; break;
        case RuleOp::INTERACTION_WEIGHT: pod.operation = core::RULE_OP_INTERACTION_WEIGHT; break;
        default:                         pod.operation = core::RULE_OP_ADD; break;
    }

    pod.cascade_layer  = static_cast<uint8_t>(p_rule->get_cascade_layer());
    pod.target_channel = static_cast<uint8_t>(std::clamp(static_cast<int>(p_rule->get_property_target()), 0, 2));
    pod.enabled        = 1u;

    const uint32_t slot = game_dod_->allocate_compiled_rule(
        pod,
        p_rule->get_priority(),
        static_cast<uint8_t>(std::clamp(p_rule->get_specificity(), 0, 255)),
        p_rule->get_is_important()
    );
    return (slot == core::INVALID_ID) ? -1 : static_cast<int>(slot);
}

godot::PackedInt32Array DataOrientedGame::compile_gameplay_style_to_dod(
    const godot::Ref<GameplayStyle>& p_style,
    int p_scope_root_entity_slot
) {
    godot::PackedInt32Array compiled_slots;
    if (p_style.is_null()) return compiled_slots;
    _ensure_game_dod_initialized();

    godot::TypedArray<GameplayRule> rules = p_style->collect_rules();
    for (int i = 0; i < rules.size(); ++i) {
        godot::Ref<GameplayRule> rule = rules[i];
        if (rule.is_valid()) {
            const int slot = compile_gameplay_rule_to_dod(rule, p_scope_root_entity_slot);
            if (slot >= 0) {
                compiled_slots.append(slot);
            }
        }
    }
    return compiled_slots;
}

int DataOrientedGame::get_active_dod_entity_count() const {
    return game_dod_ ? static_cast<int>(game_dod_->get_active_entity_count()) : 0;
}
int DataOrientedGame::get_active_dod_property_count() const {
    return game_dod_ ? static_cast<int>(game_dod_->get_active_property_count()) : 0;
}
int DataOrientedGame::get_active_dod_modifier_count() const {
    return game_dod_ ? static_cast<int>(game_dod_->get_active_modifier_count()) : 0;
}
int DataOrientedGame::get_active_dod_action_count() const {
    return game_dod_ ? static_cast<int>(game_dod_->get_active_action_count()) : 0;
}
int DataOrientedGame::get_active_dod_interaction_count() const {
    return game_dod_ ? static_cast<int>(game_dod_->get_active_interaction_count()) : 0;
}

} // namespace ideam::godot_ext