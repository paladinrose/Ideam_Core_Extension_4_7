#include "game_dod.h"
#include "registration/games_metadata_task_registry.h"
#include "registration/games_query_task_registry.h"
#include "registration/games_transform_task_registry.h"

#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

constexpr size_t DEFAULT_GAMES_MASTER_BLOCK_BYTES    = 32ULL * 1024ULL * 1024ULL; // 32 MB
constexpr size_t DEFAULT_GAMES_TRANSIENT_BLOCK_BYTES = 4ULL * 1024ULL * 1024ULL;  // 4 MB

template <typename T>
uint32_t alloc_shadowed_column(
    MemoryManagerDOD& r_mgr,
    uint32_t p_max_elements,
    BufferLayoutType p_layout = BufferLayoutType::FLAT,
    SelectionMode p_sel_mode = SelectionMode::RANGE
) {
    const size_t count = std::max<size_t>(1u, p_max_elements);
    const size_t bytes = count * sizeof(T);
    uint32_t id = r_mgr.create_shadowed_buffer(p_layout, bytes, static_cast<int64_t>(count), p_sel_mode);
    if (MemoryBufferPOD* buf = r_mgr.get_buffer(id)) {
        buf->element_stride = sizeof(T);
        buf->current_count = 0;
        if (buf->master_block_ptr) {
            std::memset(buf->master_block_ptr + buf->memory_offset, 0, bytes);
        }
    }
    return id;
}


template <typename T_DefaultTask, typename T_Logic>
std::unique_ptr<INativeTask> create_transform_or_default(
    GamesTransformLogicID p_logic_id,
    const T_Logic& p_logic,
    const GamesNodeExecutionConfig& p_config,
    const GameDOD& p_game
) {
    if (p_config.use_registry_factory) {
        auto task = GamesTransformTaskRegistry::create(
            static_cast<uint32_t>(p_logic_id),
            static_cast<uint32_t>(p_config.view),
            static_cast<uint32_t>(p_config.strategy),
            static_cast<uint32_t>(p_config.element_type)
        );
        if (task) {
            godot::Dictionary props;
            p_game.populate_task_buffer_dictionary(props, true);
            props["target_buffer_id"] = static_cast<int64_t>(p_logic.get_target_buffer_id());
            task->apply_properties(props);
            return task;
        }
    }
    return std::make_unique<T_DefaultTask>(p_logic);
}


template <typename T_Logic>
std::unique_ptr<INativeTask> create_query_or_default(
    GamesQueryLogicID p_logic_id,
    QueryOp p_op,
    const T_Logic& p_logic,
    const GamesNodeExecutionConfig& p_config,
    const GameDOD& p_game
) {
    if (p_config.use_registry_factory) {
        auto task = GamesQueryTaskRegistry::create(
            static_cast<uint32_t>(p_op),
            static_cast<uint32_t>(p_logic_id),
            static_cast<uint32_t>(p_config.view),
            static_cast<uint32_t>(p_config.strategy),
            static_cast<uint32_t>(p_config.element_type)
        );
        if (task) {
            godot::Dictionary props;
            p_game.populate_task_buffer_dictionary(props, true);
            props["target_buffer_id"] = static_cast<int64_t>(p_logic.get_target_buffer_id());
            task->apply_properties(props);
            return task;
        }
    }
    if (p_op == QueryOp::ADD) {
        return std::make_unique<QueryTask<T_Logic, QueryOp::ADD>>(p_logic);
    }
    return std::make_unique<QueryTask<T_Logic, QueryOp::CULL>>(p_logic);
}

template <typename T_Logic>
std::unique_ptr<INativeTask> create_metadata_or_default(
    GamesMetadataLogicID p_logic_id,
    const T_Logic& p_logic,
    const GamesNodeExecutionConfig& p_config,
    const GameDOD& p_game
) {
    if (p_config.use_registry_factory) {
        auto task = GamesMetadataTaskRegistry::create(
            static_cast<uint32_t>(p_logic_id),
            static_cast<uint32_t>(p_config.view),
            static_cast<uint32_t>(p_config.strategy),
            static_cast<uint32_t>(p_config.element_type)
        );
        if (task) {
            godot::Dictionary props;
            p_game.populate_task_buffer_dictionary(props, true);
            props["target_buffer_id"] = static_cast<int64_t>(p_logic.get_target_buffer_id());
            task->apply_properties(props);
            return task;
        }
    }
    return std::make_unique<MetadataTask<T_Logic>>(p_logic);
}

} // anonymous namespace

GameDOD::GameDOD(
    std::shared_ptr<MemoryManagerDOD> p_memory_manager,
    const GamesBufferCapacities& p_capacities
)
    : memory_manager_(std::move(p_memory_manager)),
      capacities_(p_capacities) {
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_GAMES_MASTER_BLOCK_BYTES,
            DEFAULT_GAMES_TRANSIENT_BLOCK_BYTES
        );
    }
    default_task_graph_ = std::make_shared<TaskGraphDOD>(memory_manager_.get());
    default_task_graph_->configure_command_arenas(64 * 1024, 64 * 1024);
    initialize_buffers(capacities_);
}

void GameDOD::initialize_buffers(const GamesBufferCapacities& p_capacities) {
    capacities_ = p_capacities;
    if (!memory_manager_) {
        memory_manager_ = std::make_shared<MemoryManagerDOD>(
            DEFAULT_GAMES_MASTER_BLOCK_BYTES,
            DEFAULT_GAMES_TRANSIENT_BLOCK_BYTES
        );
    }

    entity_allocator_.initialize(capacities_.max_entities);
    property_allocator_.initialize(capacities_.max_properties);
    modifier_allocator_.initialize(capacities_.max_modifiers);
    action_allocator_.initialize(capacities_.max_actions);
    interaction_allocator_.initialize(capacities_.max_interactions);
    rule_allocator_.initialize(capacities_.max_compiled_rules);
    player_allocator_.initialize(capacities_.max_players);

    sequence_step_cursor_ = 0;
    piece_binding_cursor_ = 0;
    interaction_participant_cursor_ = 0;

    MemoryManagerDOD& mgr = *memory_manager_;

    // --- 1. EntityHierarchyBuffer Columns ---
    manifest_.entity_parent_id_buffer_id            = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_entities);
    manifest_.entity_board_id_buffer_id             = alloc_shadowed_column<int32_t>(mgr, capacities_.max_entities);
    manifest_.entity_archetype_mask_buffer_id       = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_entities);
    manifest_.entity_group_mask_buffer_id           = alloc_shadowed_column<uint64_t>(mgr, capacities_.max_entities);
    manifest_.entity_active_flags_buffer_id         = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_entities);
    manifest_.entity_local_time_scale_buffer_id     = alloc_shadowed_column<float>(mgr, capacities_.max_entities);
    manifest_.entity_effective_time_scale_buffer_id = alloc_shadowed_column<float>(mgr, capacities_.max_entities);
    manifest_.entity_controlling_player_id_buffer_id= alloc_shadowed_column<int32_t>(mgr, capacities_.max_entities);
    manifest_.entity_exclusivity_buffer_id          = alloc_shadowed_column<uint8_t>(mgr, capacities_.max_entities);
    manifest_.entity_object_id_buffer_id            = alloc_shadowed_column<uint64_t>(mgr, capacities_.max_entities);
    manifest_.entity_consequence_mask_buffer_id     = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_entities);
    manifest_.entity_sim_tier_buffer_id             = alloc_shadowed_column<uint8_t>(mgr, capacities_.max_entities);

    // --- 2. GamePropertyBuffer Columns ---
    manifest_.prop_owner_entity_id_buffer_id        = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_properties);
    manifest_.prop_name_id_buffer_id                = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_properties);
    manifest_.prop_base_value_buffer_id             = alloc_shadowed_column<int32_t>(mgr, capacities_.max_properties);
    manifest_.prop_modded_value_buffer_id           = alloc_shadowed_column<int32_t>(mgr, capacities_.max_properties);
    manifest_.prop_base_min_buffer_id               = alloc_shadowed_column<int32_t>(mgr, capacities_.max_properties);
    manifest_.prop_modded_min_buffer_id             = alloc_shadowed_column<int32_t>(mgr, capacities_.max_properties);
    manifest_.prop_base_max_buffer_id               = alloc_shadowed_column<int32_t>(mgr, capacities_.max_properties);
    manifest_.prop_modded_max_buffer_id             = alloc_shadowed_column<int32_t>(mgr, capacities_.max_properties);
    manifest_.prop_flags_buffer_id                  = alloc_shadowed_column<uint16_t>(mgr, capacities_.max_properties);
    manifest_.prop_object_id_buffer_id              = alloc_shadowed_column<uint64_t>(mgr, capacities_.max_properties);
    manifest_.prop_gauge_object_id_buffer_id        = alloc_shadowed_column<uint64_t>(mgr, capacities_.max_properties);
    manifest_.prop_telemetry_snapshot_buffer_id     = alloc_shadowed_column<PropertyTelemetrySnapshotPOD>(
        mgr, capacities_.max_properties, BufferLayoutType::AOS
    );
    manifest_.prop_exhaustion_event_buffer_id       = alloc_shadowed_column<PropertyExhaustionEventPOD>(
        mgr, capacities_.max_properties, BufferLayoutType::AOS
    );

    // --- 3. PropertyModifierBuffer Columns & 32B AoS Table ---
    manifest_.modifier_pod_buffer_id                = alloc_shadowed_column<PropertyModifierPOD>(
        mgr, capacities_.max_modifiers, BufferLayoutType::AOS
    );
    manifest_.mod_target_property_idx_buffer_id     = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_modifiers);
    manifest_.mod_value_buffer_id                   = alloc_shadowed_column<int32_t>(mgr, capacities_.max_modifiers);
    manifest_.mod_starting_value_buffer_id          = alloc_shadowed_column<int32_t>(mgr, capacities_.max_modifiers);
    manifest_.mod_time_limit_buffer_id              = alloc_shadowed_column<float>(mgr, capacities_.max_modifiers);
    manifest_.mod_current_time_buffer_id            = alloc_shadowed_column<float>(mgr, capacities_.max_modifiers);
    manifest_.mod_type_buffer_id                    = alloc_shadowed_column<uint8_t>(mgr, capacities_.max_modifiers);
    manifest_.mod_channel_buffer_id                 = alloc_shadowed_column<uint8_t>(mgr, capacities_.max_modifiers);
    manifest_.mod_flags_buffer_id                   = alloc_shadowed_column<uint8_t>(mgr, capacities_.max_modifiers);

    // --- 4. ActionSequenceBuffer Columns & Sub-Tables ---
    manifest_.action_owner_agent_id_buffer_id       = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_actions);
    manifest_.action_base_score_buffer_id           = alloc_shadowed_column<int32_t>(mgr, capacities_.max_actions);
    manifest_.action_current_score_buffer_id        = alloc_shadowed_column<int32_t>(mgr, capacities_.max_actions);
    manifest_.action_priority_buffer_id             = alloc_shadowed_column<int32_t>(mgr, capacities_.max_actions);
    manifest_.action_status_buffer_id               = alloc_shadowed_column<int32_t>(mgr, capacities_.max_actions);
    manifest_.action_group_mask_buffer_id           = alloc_shadowed_column<uint64_t>(mgr, capacities_.max_actions);
    manifest_.action_competition_mask_buffer_id     = alloc_shadowed_column<uint64_t>(mgr, capacities_.max_actions);
    manifest_.action_refresh_time_buffer_id         = alloc_shadowed_column<float>(mgr, capacities_.max_actions);
    manifest_.action_current_time_buffer_id         = alloc_shadowed_column<float>(mgr, capacities_.max_actions);
    manifest_.action_current_step_idx_buffer_id     = alloc_shadowed_column<int32_t>(mgr, capacities_.max_actions);
    manifest_.action_step_offset_buffer_id          = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_actions);
    manifest_.action_step_count_buffer_id           = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_actions);
    manifest_.action_sequence_step_pod_buffer_id    = alloc_shadowed_column<SequenceStepPOD>(
        mgr, capacities_.max_sequence_steps, BufferLayoutType::AOS
    );
    manifest_.action_binding_offset_buffer_id       = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_actions);
    manifest_.action_binding_count_buffer_id        = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_actions);
    manifest_.action_binding_pod_buffer_id          = alloc_shadowed_column<PiecePropertyBindingPOD>(
        mgr, capacities_.max_piece_bindings, BufferLayoutType::AOS
    );

    // --- 5. InteractionArenaBuffer & Consequence Ring ---
    manifest_.interaction_instigator_action_idx_buffer_id = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_interactions);
    manifest_.interaction_instigator_score_buffer_id      = alloc_shadowed_column<int32_t>(mgr, capacities_.max_interactions);
    manifest_.interaction_competition_score_buffer_id     = alloc_shadowed_column<int32_t>(mgr, capacities_.max_interactions);
    manifest_.interaction_score_diff_buffer_id            = alloc_shadowed_column<int32_t>(mgr, capacities_.max_interactions);
    manifest_.interaction_instigator_weight_buffer_id     = alloc_shadowed_column<float>(mgr, capacities_.max_interactions);
    manifest_.interaction_competition_weight_buffer_id    = alloc_shadowed_column<float>(mgr, capacities_.max_interactions);
    manifest_.interaction_participant_offset_buffer_id    = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_interactions);
    manifest_.interaction_participant_count_buffer_id     = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_interactions);
    manifest_.interaction_participant_pod_buffer_id       = alloc_shadowed_column<InteractionParticipantPOD>(
        mgr, capacities_.max_interaction_participants, BufferLayoutType::AOS
    );
    manifest_.consequence_ring_buffer_id            = alloc_shadowed_column<ConsequenceCommandPOD>(
        mgr, capacities_.max_consequence_commands, BufferLayoutType::RING
    );
    manifest_.consequence_ring_cursor_buffer_id     = alloc_shadowed_column<uint32_t>(mgr, 4u);
    manifest_.consequence_drained_count_buffer_id   = alloc_shadowed_column<uint32_t>(mgr, 4u);

    // --- 6. Auxiliary GameplayRule, Possession & Input Tables ---
    manifest_.compiled_rule_pod_buffer_id           = alloc_shadowed_column<CompiledGameplayRulePOD>(
        mgr, capacities_.max_compiled_rules, BufferLayoutType::AOS
    );
    manifest_.rule_cascade_layer_buffer_id          = alloc_shadowed_column<int32_t>(mgr, capacities_.max_compiled_rules);
    manifest_.rule_priority_buffer_id               = alloc_shadowed_column<int32_t>(mgr, capacities_.max_compiled_rules);
    manifest_.rule_specificity_buffer_id            = alloc_shadowed_column<int32_t>(mgr, capacities_.max_compiled_rules);
    manifest_.rule_important_flags_buffer_id        = alloc_shadowed_column<uint8_t>(mgr, capacities_.max_compiled_rules);
    manifest_.rule_output_key_buffer_id             = alloc_shadowed_column<int32_t>(mgr, capacities_.max_compiled_rules);

    manifest_.agent_transform_pos_buffer_id         = alloc_shadowed_column<godot::Vector3>(mgr, capacities_.max_entities);
    manifest_.player_transform_pos_buffer_id        = alloc_shadowed_column<godot::Vector3>(mgr, capacities_.max_players);
    manifest_.agent_to_player_idx_buffer_id         = alloc_shadowed_column<int32_t>(mgr, capacities_.max_entities);
    manifest_.possession_match_mode_buffer_id       = alloc_shadowed_column<uint8_t>(mgr, capacities_.max_entities);
    manifest_.possession_elapsed_buffer_id          = alloc_shadowed_column<float>(mgr, capacities_.max_entities);
    manifest_.possession_duration_buffer_id         = alloc_shadowed_column<float>(mgr, capacities_.max_entities);
    manifest_.possession_completion_flags_buffer_id = alloc_shadowed_column<uint32_t>(mgr, capacities_.max_entities);

    manifest_.raw_input_vec2_buffer_id              = alloc_shadowed_column<godot::Vector2>(mgr, capacities_.max_players);
    manifest_.filtered_input_vec2_buffer_id         = alloc_shadowed_column<godot::Vector2>(mgr, capacities_.max_players);
    manifest_.world_move_dir_vec3_buffer_id         = alloc_shadowed_column<godot::Vector3>(mgr, capacities_.max_players);
    manifest_.camera_transform_pod_buffer_id        = alloc_shadowed_column<InputTransform3DPOD>(
        mgr, capacities_.max_players, BufferLayoutType::AOS
    );
    manifest_.agent_transform_pod_buffer_id         = alloc_shadowed_column<InputTransform3DPOD>(
        mgr, capacities_.max_entities, BufferLayoutType::AOS
    );

    // Allocate slot 0 in EntityHierarchyBuffer as the root Game entity
    EntityHierarchyRowPOD root_row{};
    root_row.parent_entity_id = INVALID_ID;
    root_row.board_id = -1;
    root_row.archetype_mask = ENTITY_ARCHETYPE_GAME;
    root_row.active_flags = ENTITY_FLAG_ENABLED | ENTITY_FLAG_INITIALIZED | ENTITY_FLAG_PROCESSED;
    root_row.local_time_scale = root_time_scale_;
    root_row.effective_time_scale = root_time_scale_;
    root_row.simulation_tier = SIM_TIER_PURE_DOD;
    root_game_entity_slot_ = allocate_entity(root_row);

    sync_selection_ranges();
}

void GameDOD::_sync_buffer_selection(uint32_t p_buffer_id, const SlotIndexAllocator& p_allocator) noexcept {
    if (!memory_manager_ || p_buffer_id == INVALID_ID) return;
    if (MemoryBufferPOD* buf = memory_manager_->get_buffer(p_buffer_id)) {
        buf->current_count = static_cast<int64_t>(p_allocator.get_high_water_mark());
        buf->used_bytes = static_cast<size_t>(buf->current_count) * buf->element_stride;
    }
}

void GameDOD::_sync_flat_count_selection(uint32_t p_buffer_id, uint32_t p_active_count) noexcept {
    if (!memory_manager_ || p_buffer_id == INVALID_ID) return;
    if (MemoryBufferPOD* buf = memory_manager_->get_buffer(p_buffer_id)) {
        buf->current_count = static_cast<int64_t>(p_active_count);
        buf->used_bytes = static_cast<size_t>(buf->current_count) * buf->element_stride;
    }
}

void GameDOD::sync_selection_ranges() noexcept {
    // Table 1
    _sync_buffer_selection(manifest_.entity_parent_id_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_board_id_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_archetype_mask_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_group_mask_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_active_flags_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_local_time_scale_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_effective_time_scale_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_controlling_player_id_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_exclusivity_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_object_id_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_consequence_mask_buffer_id, entity_allocator_);
    _sync_buffer_selection(manifest_.entity_sim_tier_buffer_id, entity_allocator_);

    // Table 2
    _sync_buffer_selection(manifest_.prop_owner_entity_id_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_name_id_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_base_value_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_modded_value_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_base_min_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_modded_min_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_base_max_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_modded_max_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_flags_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_object_id_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_gauge_object_id_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_telemetry_snapshot_buffer_id, property_allocator_);
    _sync_buffer_selection(manifest_.prop_exhaustion_event_buffer_id, property_allocator_);

    // Table 3
    _sync_buffer_selection(manifest_.modifier_pod_buffer_id, modifier_allocator_);
    _sync_buffer_selection(manifest_.mod_target_property_idx_buffer_id, modifier_allocator_);
    _sync_buffer_selection(manifest_.mod_value_buffer_id, modifier_allocator_);
    _sync_buffer_selection(manifest_.mod_starting_value_buffer_id, modifier_allocator_);
    _sync_buffer_selection(manifest_.mod_time_limit_buffer_id, modifier_allocator_);
    _sync_buffer_selection(manifest_.mod_current_time_buffer_id, modifier_allocator_);
    _sync_buffer_selection(manifest_.mod_type_buffer_id, modifier_allocator_);
    _sync_buffer_selection(manifest_.mod_channel_buffer_id, modifier_allocator_);
    _sync_buffer_selection(manifest_.mod_flags_buffer_id, modifier_allocator_);

    // Table 4
    _sync_buffer_selection(manifest_.action_owner_agent_id_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_base_score_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_current_score_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_priority_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_status_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_group_mask_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_competition_mask_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_refresh_time_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_current_time_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_current_step_idx_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_step_offset_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_step_count_buffer_id, action_allocator_);
    _sync_flat_count_selection(manifest_.action_sequence_step_pod_buffer_id, sequence_step_cursor_);
    _sync_buffer_selection(manifest_.action_binding_offset_buffer_id, action_allocator_);
    _sync_buffer_selection(manifest_.action_binding_count_buffer_id, action_allocator_);
    _sync_flat_count_selection(manifest_.action_binding_pod_buffer_id, piece_binding_cursor_);

    // Table 5
    _sync_buffer_selection(manifest_.interaction_instigator_action_idx_buffer_id, interaction_allocator_);
    _sync_buffer_selection(manifest_.interaction_instigator_score_buffer_id, interaction_allocator_);
    _sync_buffer_selection(manifest_.interaction_competition_score_buffer_id, interaction_allocator_);
    _sync_buffer_selection(manifest_.interaction_score_diff_buffer_id, interaction_allocator_);
    _sync_buffer_selection(manifest_.interaction_instigator_weight_buffer_id, interaction_allocator_);
    _sync_buffer_selection(manifest_.interaction_competition_weight_buffer_id, interaction_allocator_);
    _sync_buffer_selection(manifest_.interaction_participant_offset_buffer_id, interaction_allocator_);
    _sync_buffer_selection(manifest_.interaction_participant_count_buffer_id, interaction_allocator_);
    _sync_flat_count_selection(manifest_.interaction_participant_pod_buffer_id, interaction_participant_cursor_);

    // Auxiliary
    _sync_buffer_selection(manifest_.compiled_rule_pod_buffer_id, rule_allocator_);
    _sync_buffer_selection(manifest_.rule_cascade_layer_buffer_id, rule_allocator_);
    _sync_buffer_selection(manifest_.rule_priority_buffer_id, rule_allocator_);
    _sync_buffer_selection(manifest_.rule_specificity_buffer_id, rule_allocator_);
    _sync_buffer_selection(manifest_.rule_important_flags_buffer_id, rule_allocator_);
    _sync_buffer_selection(manifest_.rule_output_key_buffer_id, rule_allocator_);
}

void GameDOD::reset_all_slots() noexcept {
    boards_.clear();
    entity_allocator_.clear();
    property_allocator_.clear();
    modifier_allocator_.clear();
    action_allocator_.clear();
    interaction_allocator_.clear();
    rule_allocator_.clear();
    player_allocator_.clear();

    sequence_step_cursor_ = 0;
    piece_binding_cursor_ = 0;
    interaction_participant_cursor_ = 0;

    EntityHierarchyRowPOD root_row{};
    root_row.parent_entity_id = INVALID_ID;
    root_row.board_id = -1;
    root_row.archetype_mask = ENTITY_ARCHETYPE_GAME;
    root_row.active_flags = ENTITY_FLAG_ENABLED | ENTITY_FLAG_INITIALIZED | ENTITY_FLAG_PROCESSED;
    root_row.local_time_scale = root_time_scale_;
    root_row.effective_time_scale = root_time_scale_;
    root_row.simulation_tier = SIM_TIER_PURE_DOD;
    root_game_entity_slot_ = allocate_entity(root_row);

    sync_selection_ranges();
}

uint32_t GameDOD::get_buffer_id_by_name(std::string_view p_name) const noexcept {
    // Table 1
    if (p_name == "parent_entity_id_buffer_id" || p_name == "entity_parent_id_buffer_id") return manifest_.entity_parent_id_buffer_id;
    if (p_name == "board_id_buffer_id" || p_name == "output_board_id_buffer_id" || p_name == "entity_board_id_buffer_id") return manifest_.entity_board_id_buffer_id;
    if (p_name == "archetype_mask_buffer_id" || p_name == "entity_archetype_mask_buffer_id") return manifest_.entity_archetype_mask_buffer_id;
    if (p_name == "group_mask_buffer_id" || p_name == "entity_group_mask_buffer_id") return manifest_.entity_group_mask_buffer_id;
    if (p_name == "active_flags_buffer_id" || p_name == "entity_active_flags_buffer_id" || p_name == "piece_enabled_flags_buffer_id") return manifest_.entity_active_flags_buffer_id;
    if (p_name == "local_time_scale_buffer_id" || p_name == "entity_local_time_scale_buffer_id") return manifest_.entity_local_time_scale_buffer_id;
    if (p_name == "effective_time_scale_buffer_id" || p_name == "time_scale_buffer_id" || p_name == "entity_effective_time_scale_buffer_id") return manifest_.entity_effective_time_scale_buffer_id;
    if (p_name == "controlling_player_id_buffer_id" || p_name == "entity_controlling_player_id_buffer_id") return manifest_.entity_controlling_player_id_buffer_id;
    if (p_name == "exclusivity_buffer_id" || p_name == "entity_exclusivity_buffer_id") return manifest_.entity_exclusivity_buffer_id;
    if (p_name == "entity_consequence_buffer_id" || p_name == "entity_consequence_mask_buffer_id") return manifest_.entity_consequence_mask_buffer_id;

    // Table 2
    if (p_name == "owner_entity_id_buffer_id" || p_name == "prop_owner_entity_id_buffer_id") return manifest_.prop_owner_entity_id_buffer_id;
    if (p_name == "property_name_id_buffer_id" || p_name == "prop_name_id_buffer_id") return manifest_.prop_name_id_buffer_id;
    if (p_name == "base_value_buffer_id" || p_name == "prop_base_value_buffer_id") return manifest_.prop_base_value_buffer_id;
    if (p_name == "modded_value_buffer_id" || p_name == "property_modded_value_buffer_id" || p_name == "target_score_buffer_id" || p_name == "prop_modded_value_buffer_id") return manifest_.prop_modded_value_buffer_id;
    if (p_name == "base_min_buffer_id" || p_name == "prop_base_min_buffer_id") return manifest_.prop_base_min_buffer_id;
    if (p_name == "modded_min_buffer_id" || p_name == "target_min_buffer_id" || p_name == "prop_modded_min_buffer_id") return manifest_.prop_modded_min_buffer_id;
    if (p_name == "base_max_buffer_id" || p_name == "prop_base_max_buffer_id") return manifest_.prop_base_max_buffer_id;
    if (p_name == "modded_max_buffer_id" || p_name == "target_max_buffer_id" || p_name == "prop_modded_max_buffer_id") return manifest_.prop_modded_max_buffer_id;
    if (p_name == "property_flags_buffer_id" || p_name == "target_flags_buffer_id" || p_name == "prop_flags_buffer_id") return manifest_.prop_flags_buffer_id;
    if (p_name == "object_id_buffer_id" || p_name == "prop_object_id_buffer_id") return manifest_.prop_object_id_buffer_id;
    if (p_name == "gauge_object_id_buffer_id" || p_name == "prop_gauge_object_id_buffer_id") return manifest_.prop_gauge_object_id_buffer_id;
    if (p_name == "telemetry_snapshot_buffer_id" || p_name == "prop_telemetry_snapshot_buffer_id") return manifest_.prop_telemetry_snapshot_buffer_id;
    if (p_name == "exhaustion_event_buffer_id" || p_name == "prop_exhaustion_event_buffer_id") return manifest_.prop_exhaustion_event_buffer_id;

    // Table 3
    if (p_name == "modifier_pod_buffer_id") return manifest_.modifier_pod_buffer_id;
    if (p_name == "target_property_idx_buffer_id" || p_name == "mod_target_property_buffer_id") return manifest_.mod_target_property_idx_buffer_id;
    if (p_name == "value_buffer_id" || p_name == "mod_value_buffer_id") return manifest_.mod_value_buffer_id;
    if (p_name == "starting_value_buffer_id" || p_name == "mod_starting_value_buffer_id") return manifest_.mod_starting_value_buffer_id;
    if (p_name == "time_limit_buffer_id" || p_name == "mod_time_limit_buffer_id") return manifest_.mod_time_limit_buffer_id;
    if (p_name == "mod_current_time_buffer_id") return manifest_.mod_current_time_buffer_id;
    if (p_name == "mod_type_buffer_id") return manifest_.mod_type_buffer_id;
    if (p_name == "mod_channel_buffer_id") return manifest_.mod_channel_buffer_id;
    if (p_name == "expired_flags_buffer_id" || p_name == "mod_flags_buffer_id") return manifest_.mod_flags_buffer_id;

    // Table 4
    if (p_name == "owner_agent_id_buffer_id" || p_name == "action_owner_agent_id_buffer_id") return manifest_.action_owner_agent_id_buffer_id;
    if (p_name == "base_score_buffer_id" || p_name == "agent_base_score_buffer_id") return manifest_.action_base_score_buffer_id;
    if (p_name == "current_score_buffer_id" || p_name == "agent_current_score_buffer_id" || p_name == "piece_action_score_buffer_id" || p_name == "action_score_buffer_id") return manifest_.action_current_score_buffer_id;
    if (p_name == "priority_buffer_id" || p_name == "action_priority_buffer_id") return manifest_.action_priority_buffer_id;
    if (p_name == "action_status_buffer_id" || p_name == "status_buffer_id" || p_name == "agent_status_buffer_id" || p_name == "piece_action_status_buffer_id") return manifest_.action_status_buffer_id;
    if (p_name == "action_group_mask_buffer_id") return manifest_.action_group_mask_buffer_id;
    if (p_name == "competition_mask_buffer_id" || p_name == "action_competition_mask_buffer_id") return manifest_.action_competition_mask_buffer_id;
    if (p_name == "refresh_time_buffer_id") return manifest_.action_refresh_time_buffer_id;
    if (p_name == "current_time_buffer_id") return manifest_.action_current_time_buffer_id;
    if (p_name == "current_step_idx_buffer_id") return manifest_.action_current_step_idx_buffer_id;
    if (p_name == "step_offset_buffer_id") return manifest_.action_step_offset_buffer_id;
    if (p_name == "step_count_buffer_id") return manifest_.action_step_count_buffer_id;
    if (p_name == "sequence_step_pod_buffer_id") return manifest_.action_sequence_step_pod_buffer_id;
    if (p_name == "binding_offset_buffer_id") return manifest_.action_binding_offset_buffer_id;
    if (p_name == "binding_count_buffer_id") return manifest_.action_binding_count_buffer_id;
    if (p_name == "binding_pod_buffer_id") return manifest_.action_binding_pod_buffer_id;

    // Table 5
    if (p_name == "instigator_action_idx_buffer_id") return manifest_.interaction_instigator_action_idx_buffer_id;
    if (p_name == "instigator_score_buffer_id") return manifest_.interaction_instigator_score_buffer_id;
    if (p_name == "competition_score_buffer_id") return manifest_.interaction_competition_score_buffer_id;
    if (p_name == "score_diff_buffer_id") return manifest_.interaction_score_diff_buffer_id;
    if (p_name == "instigator_weight_buffer_id") return manifest_.interaction_instigator_weight_buffer_id;
    if (p_name == "competition_weight_buffer_id") return manifest_.interaction_competition_weight_buffer_id;
    if (p_name == "participant_offset_buffer_id") return manifest_.interaction_participant_offset_buffer_id;
    if (p_name == "participant_count_buffer_id") return manifest_.interaction_participant_count_buffer_id;
    if (p_name == "participant_pod_buffer_id") return manifest_.interaction_participant_pod_buffer_id;
    if (p_name == "consequence_ring_buffer_id") return manifest_.consequence_ring_buffer_id;
    if (p_name == "ring_cursor_buffer_id") return manifest_.consequence_ring_cursor_buffer_id;
    if (p_name == "drained_count_buffer_id") return manifest_.consequence_drained_count_buffer_id;

    // Rules, Possession & Input
    if (p_name == "compiled_rule_buffer_id") return manifest_.compiled_rule_pod_buffer_id;
    if (p_name == "cascade_layer_buffer_id") return manifest_.rule_cascade_layer_buffer_id;
    if (p_name == "specificity_buffer_id") return manifest_.rule_specificity_buffer_id;
    if (p_name == "important_flags_buffer_id") return manifest_.rule_important_flags_buffer_id;
    if (p_name == "output_key_buffer_id") return manifest_.rule_output_key_buffer_id;
    if (p_name == "agent_transform_buffer_id") return manifest_.agent_transform_pos_buffer_id;
    if (p_name == "player_transform_buffer_id") return manifest_.player_transform_pos_buffer_id;
    if (p_name == "agent_to_player_idx_buffer_id") return manifest_.agent_to_player_idx_buffer_id;
    if (p_name == "match_mode_buffer_id") return manifest_.possession_match_mode_buffer_id;
    if (p_name == "match_elapsed_buffer_id") return manifest_.possession_elapsed_buffer_id;
    if (p_name == "match_duration_buffer_id") return manifest_.possession_duration_buffer_id;
    if (p_name == "completion_flags_buffer_id") return manifest_.possession_completion_flags_buffer_id;
    if (p_name == "raw_input_buffer_id") return manifest_.raw_input_vec2_buffer_id;
    if (p_name == "target_input_buffer_id") return manifest_.filtered_input_vec2_buffer_id;
    if (p_name == "world_move_dir_buffer_id") return manifest_.world_move_dir_vec3_buffer_id;
    if (p_name == "camera_transform_buffer_id") return manifest_.camera_transform_pod_buffer_id;

    return INVALID_ID;
}

void GameDOD::populate_task_buffer_dictionary(godot::Dictionary& r_props, bool p_overwrite_existing) const {
    auto set_key = [&](const char* p_key, uint32_t p_val) {
        if (p_val == INVALID_ID) return;
        if (p_overwrite_existing || !r_props.has(p_key)) {
            r_props[p_key] = static_cast<int64_t>(p_val);
        }
    };

    set_key("parent_entity_id_buffer_id", manifest_.entity_parent_id_buffer_id);
    set_key("board_id_buffer_id", manifest_.entity_board_id_buffer_id);
    set_key("output_board_id_buffer_id", manifest_.entity_board_id_buffer_id);
    set_key("archetype_mask_buffer_id", manifest_.entity_archetype_mask_buffer_id);
    set_key("group_mask_buffer_id", manifest_.entity_group_mask_buffer_id);
    set_key("active_flags_buffer_id", manifest_.entity_active_flags_buffer_id);
    set_key("entity_active_flags_buffer_id", manifest_.entity_active_flags_buffer_id);
    set_key("local_time_scale_buffer_id", manifest_.entity_local_time_scale_buffer_id);
    set_key("effective_time_scale_buffer_id", manifest_.entity_effective_time_scale_buffer_id);
    set_key("time_scale_buffer_id", manifest_.entity_effective_time_scale_buffer_id);
    set_key("controlling_player_id_buffer_id", manifest_.entity_controlling_player_id_buffer_id);
    set_key("exclusivity_buffer_id", manifest_.entity_exclusivity_buffer_id);
    set_key("entity_consequence_buffer_id", manifest_.entity_consequence_mask_buffer_id);

    set_key("owner_entity_id_buffer_id", manifest_.prop_owner_entity_id_buffer_id);
    set_key("property_name_id_buffer_id", manifest_.prop_name_id_buffer_id);
    set_key("base_value_buffer_id", manifest_.prop_base_value_buffer_id);
    set_key("modded_value_buffer_id", manifest_.prop_modded_value_buffer_id);
    set_key("property_modded_value_buffer_id", manifest_.prop_modded_value_buffer_id);
    set_key("base_min_buffer_id", manifest_.prop_base_min_buffer_id);
    set_key("modded_min_buffer_id", manifest_.prop_modded_min_buffer_id);
    set_key("base_max_buffer_id", manifest_.prop_base_max_buffer_id);
    set_key("modded_max_buffer_id", manifest_.prop_modded_max_buffer_id);
    set_key("property_flags_buffer_id", manifest_.prop_flags_buffer_id);
    set_key("object_id_buffer_id", manifest_.prop_object_id_buffer_id);
    set_key("gauge_object_id_buffer_id", manifest_.prop_gauge_object_id_buffer_id);
    set_key("telemetry_snapshot_buffer_id", manifest_.prop_telemetry_snapshot_buffer_id);
    set_key("exhaustion_event_buffer_id", manifest_.prop_exhaustion_event_buffer_id);

    set_key("modifier_pod_buffer_id", manifest_.modifier_pod_buffer_id);
    set_key("owner_agent_id_buffer_id", manifest_.action_owner_agent_id_buffer_id);
    set_key("base_score_buffer_id", manifest_.action_base_score_buffer_id);
    set_key("current_score_buffer_id", manifest_.action_current_score_buffer_id);
    set_key("agent_current_score_buffer_id", manifest_.action_current_score_buffer_id);
    set_key("piece_action_score_buffer_id", manifest_.action_current_score_buffer_id);
    set_key("action_score_buffer_id", manifest_.action_current_score_buffer_id);
    set_key("priority_buffer_id", manifest_.action_priority_buffer_id);
    set_key("action_status_buffer_id", manifest_.action_status_buffer_id);
    set_key("status_buffer_id", manifest_.action_status_buffer_id);
    set_key("action_group_mask_buffer_id", manifest_.action_group_mask_buffer_id);
    set_key("competition_mask_buffer_id", manifest_.action_competition_mask_buffer_id);
    set_key("refresh_time_buffer_id", manifest_.action_refresh_time_buffer_id);
    set_key("current_time_buffer_id", manifest_.action_current_time_buffer_id);
    set_key("current_step_idx_buffer_id", manifest_.action_current_step_idx_buffer_id);
    set_key("step_offset_buffer_id", manifest_.action_step_offset_buffer_id);
    set_key("step_count_buffer_id", manifest_.action_step_count_buffer_id);
    set_key("sequence_step_pod_buffer_id", manifest_.action_sequence_step_pod_buffer_id);
    set_key("binding_offset_buffer_id", manifest_.action_binding_offset_buffer_id);
    set_key("binding_count_buffer_id", manifest_.action_binding_count_buffer_id);
    set_key("binding_pod_buffer_id", manifest_.action_binding_pod_buffer_id);

    set_key("instigator_action_idx_buffer_id", manifest_.interaction_instigator_action_idx_buffer_id);
    set_key("instigator_score_buffer_id", manifest_.interaction_instigator_score_buffer_id);
    set_key("competition_score_buffer_id", manifest_.interaction_competition_score_buffer_id);
    set_key("score_diff_buffer_id", manifest_.interaction_score_diff_buffer_id);
    set_key("instigator_weight_buffer_id", manifest_.interaction_instigator_weight_buffer_id);
    set_key("competition_weight_buffer_id", manifest_.interaction_competition_weight_buffer_id);
    set_key("participant_offset_buffer_id", manifest_.interaction_participant_offset_buffer_id);
    set_key("participant_count_buffer_id", manifest_.interaction_participant_count_buffer_id);
    set_key("participant_pod_buffer_id", manifest_.interaction_participant_pod_buffer_id);
    set_key("consequence_ring_buffer_id", manifest_.consequence_ring_buffer_id);
    set_key("ring_cursor_buffer_id", manifest_.consequence_ring_cursor_buffer_id);
    set_key("drained_count_buffer_id", manifest_.consequence_drained_count_buffer_id);
    set_key("compiled_rule_buffer_id", manifest_.compiled_rule_pod_buffer_id);
}

uint32_t GameDOD::intern_name_token(std::string_view p_name) {
    if (p_name.empty()) return 0u;
    std::string key(p_name);
    auto it = interned_tokens_.find(key);
    if (it != interned_tokens_.end()) {
        return it->second;
    }
    const uint32_t id = next_token_id_++;
    interned_tokens_.emplace(std::move(key), id);
    return id;
}

uint64_t GameDOD::get_or_register_group_bitmask(std::string_view p_group_name) {
    if (p_group_name.empty()) return 0ULL;
    std::string key(p_group_name);
    auto it = group_bit_map_.find(key);
    if (it != group_bit_map_.end()) {
        return (1ULL << it->second);
    }
    if (next_group_bit_ >= 64u) {
        // Wrap around deterministically via FNV-1a hash if >64 distinct groups exist
        uint64_t hash = 14695981039346656037ULL;
        for (char c : key) {
            hash ^= static_cast<uint8_t>(c);
            hash *= 1099511628211ULL;
        }
        return (1ULL << (hash & 63ULL));
    }
    const uint8_t bit = next_group_bit_++;
    group_bit_map_.emplace(std::move(key), bit);
    return (1ULL << bit);
}

uint64_t GameDOD::compute_combined_group_mask(std::span<const std::string_view> p_groups) {
    uint64_t mask = 0ULL;
    for (std::string_view g : p_groups) {
        mask |= get_or_register_group_bitmask(g);
    }
    return mask;
}

void GameDOD::set_root_time_scale(float p_time_scale) noexcept {
    root_time_scale_ = p_time_scale;
    if (root_game_entity_slot_ != INVALID_ID) {
        set_entity_local_time_scale(root_game_entity_slot_, p_time_scale);
    }
}

GameBoardDOD* GameDOD::create_board_dod(int32_t p_board_partition_id, float p_time_scale) {
    const int32_t part_id = (p_board_partition_id >= 0) ? p_board_partition_id : next_board_partition_id_++;
    if (part_id >= next_board_partition_id_) {
        next_board_partition_id_ = part_id + 1;
    }

    if (GameBoardDOD* existing = get_board_dod(part_id)) {
        existing->set_local_time_scale(p_time_scale);
        return existing;
    }

    EntityHierarchyRowPOD board_row{};
    board_row.parent_entity_id = root_game_entity_slot_;
    board_row.board_id = part_id;
    board_row.archetype_mask = ENTITY_ARCHETYPE_BOARD;
    board_row.active_flags = ENTITY_FLAG_ENABLED | ENTITY_FLAG_INITIALIZED | ENTITY_FLAG_PROCESSED;
    board_row.local_time_scale = p_time_scale;
    board_row.effective_time_scale = root_time_scale_ * p_time_scale;
    board_row.simulation_tier = SIM_TIER_PURE_DOD;

    const uint32_t board_slot = allocate_entity(board_row);
    auto board = std::make_unique<GameBoardDOD>(this, part_id, board_slot, p_time_scale);
    GameBoardDOD* ptr = board.get();
    boards_.push_back(std::move(board));
    return ptr;
}

GameBoardDOD* GameDOD::get_board_dod(int32_t p_board_partition_id) const noexcept {
    for (const auto& b : boards_) {
        if (b && b->get_board_partition_id() == p_board_partition_id) {
            return b.get();
        }
    }
    return nullptr;
}

bool GameDOD::remove_board_dod(int32_t p_board_partition_id) {
    for (auto it = boards_.begin(); it != boards_.end(); ++it) {
        if (*it && (*it)->get_board_partition_id() == p_board_partition_id) {
            (*it)->clear_board_rules();
            const uint32_t slot = (*it)->get_board_entity_slot();
            if (slot != INVALID_ID) {
                free_entity(slot);
            }
            boards_.erase(it);
            return true;
        }
    }
    return false;
}

// ============================================================================
// Table 1: EntityHierarchyBuffer Operations
// ============================================================================

uint32_t GameDOD::allocate_entity(const EntityHierarchyRowPOD& p_initial_row) {
    const uint32_t slot = entity_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    EntityHierarchyRowPOD row = p_initial_row;
    if (slot != 0u && row.parent_entity_id == INVALID_ID && root_game_entity_slot_ != INVALID_ID) {
        row.parent_entity_id = root_game_entity_slot_;
    }
    set_entity_row(slot, row);
    _sync_buffer_selection(manifest_.entity_effective_time_scale_buffer_id, entity_allocator_);
    return slot;
}

bool GameDOD::free_entity(uint32_t p_entity_slot) {
    if (!entity_allocator_.is_valid(p_entity_slot)) return false;

    EntityHierarchyRowPOD cleared{};
    cleared.parent_entity_id = INVALID_ID;
    cleared.board_id = -1;
    cleared.archetype_mask = ENTITY_ARCHETYPE_NONE;
    cleared.active_flags = ENTITY_FLAG_NONE;
    cleared.local_time_scale = 0.0f;
    cleared.effective_time_scale = 0.0f;
    set_entity_row(p_entity_slot, cleared);

    return entity_allocator_.release(p_entity_slot);
}

EntityHierarchyRowPOD GameDOD::get_entity_row(uint32_t p_entity_slot) const noexcept {
    EntityHierarchyRowPOD row{};
    if (p_entity_slot >= capacities_.max_entities) return row;

    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.entity_parent_id_buffer_id)) row.parent_entity_id = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.entity_board_id_buffer_id)) row.board_id = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.entity_archetype_mask_buffer_id)) row.archetype_mask = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<uint64_t>(manifest_.entity_group_mask_buffer_id)) row.group_mask = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.entity_active_flags_buffer_id)) row.active_flags = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<float>(manifest_.entity_local_time_scale_buffer_id)) row.local_time_scale = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<float>(manifest_.entity_effective_time_scale_buffer_id)) row.effective_time_scale = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.entity_controlling_player_id_buffer_id)) row.controlling_player_id = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<uint8_t>(manifest_.entity_exclusivity_buffer_id)) row.exclusivity = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<uint8_t>(manifest_.entity_sim_tier_buffer_id)) row.simulation_tier = p[p_entity_slot];
    if (const auto* p = _get_column_ptr<uint64_t>(manifest_.entity_object_id_buffer_id)) row.object_id = p[p_entity_slot];

    return row;
}

void GameDOD::set_entity_row(uint32_t p_entity_slot, const EntityHierarchyRowPOD& p_row) noexcept {
    if (p_entity_slot >= capacities_.max_entities) return;

    if (auto* p = _get_column_ptr<uint32_t>(manifest_.entity_parent_id_buffer_id)) p[p_entity_slot] = p_row.parent_entity_id;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.entity_board_id_buffer_id)) p[p_entity_slot] = p_row.board_id;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.entity_archetype_mask_buffer_id)) p[p_entity_slot] = p_row.archetype_mask;
    if (auto* p = _get_column_ptr<uint64_t>(manifest_.entity_group_mask_buffer_id)) p[p_entity_slot] = p_row.group_mask;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.entity_active_flags_buffer_id)) p[p_entity_slot] = p_row.active_flags;
    if (auto* p = _get_column_ptr<float>(manifest_.entity_local_time_scale_buffer_id)) p[p_entity_slot] = p_row.local_time_scale;
    if (auto* p = _get_column_ptr<float>(manifest_.entity_effective_time_scale_buffer_id)) p[p_entity_slot] = p_row.effective_time_scale;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.entity_controlling_player_id_buffer_id)) p[p_entity_slot] = p_row.controlling_player_id;
    if (auto* p = _get_column_ptr<uint8_t>(manifest_.entity_exclusivity_buffer_id)) p[p_entity_slot] = p_row.exclusivity;
    if (auto* p = _get_column_ptr<uint8_t>(manifest_.entity_sim_tier_buffer_id)) p[p_entity_slot] = p_row.simulation_tier;
    if (auto* p = _get_column_ptr<uint64_t>(manifest_.entity_object_id_buffer_id)) p[p_entity_slot] = p_row.object_id;
}

bool GameDOD::set_entity_simulation_tier(
    uint32_t p_entity_slot,
    EntitySimulationTier p_tier,
    uint64_t p_godot_object_id
) noexcept {
    if (!entity_allocator_.is_valid(p_entity_slot)) return false;

    if (auto* tier_col = _get_column_ptr<uint8_t>(manifest_.entity_sim_tier_buffer_id)) {
        tier_col[p_entity_slot] = static_cast<uint8_t>(p_tier);
    }
    if (auto* obj_col = _get_column_ptr<uint64_t>(manifest_.entity_object_id_buffer_id)) {
        if (p_tier == SIM_TIER_PURE_DOD || p_tier == SIM_TIER_DOD_RENDERED) {
            if (p_godot_object_id == 0ULL) obj_col[p_entity_slot] = 0ULL;
        } else if (p_godot_object_id != 0ULL) {
            obj_col[p_entity_slot] = p_godot_object_id;
        }
    }
    if (auto* flags_col = _get_column_ptr<uint32_t>(manifest_.entity_active_flags_buffer_id)) {
        if (p_tier == SIM_TIER_FULL_OOP) {
            flags_col[p_entity_slot] &= ~static_cast<uint32_t>(ENTITY_FLAG_PROCESSED);
        } else {
            flags_col[p_entity_slot] |= ENTITY_FLAG_PROCESSED;
        }
    }
    return true;
}

EntitySimulationTier GameDOD::get_entity_simulation_tier(uint32_t p_entity_slot) const noexcept {
    if (p_entity_slot < capacities_.max_entities) {
        if (const auto* tier_col = _get_column_ptr<uint8_t>(manifest_.entity_sim_tier_buffer_id)) {
            return static_cast<EntitySimulationTier>(tier_col[p_entity_slot]);
        }
    }
    return SIM_TIER_PURE_DOD;
}

void GameDOD::set_entity_local_time_scale(uint32_t p_entity_slot, float p_time_scale) noexcept {
    if (p_entity_slot >= capacities_.max_entities) return;
    if (auto* p = _get_column_ptr<float>(manifest_.entity_local_time_scale_buffer_id)) {
        p[p_entity_slot] = p_time_scale;
    }
}

float GameDOD::get_entity_local_time_scale(uint32_t p_entity_slot) const noexcept {
    if (p_entity_slot >= capacities_.max_entities) return 1.0f;
    if (const auto* p = _get_column_ptr<float>(manifest_.entity_local_time_scale_buffer_id)) {
        return p[p_entity_slot];
    }
    return 1.0f;
}

float GameDOD::get_entity_effective_time_scale(uint32_t p_entity_slot) const noexcept {
    if (p_entity_slot >= capacities_.max_entities) return 1.0f;
    if (const auto* p = _get_column_ptr<float>(manifest_.entity_effective_time_scale_buffer_id)) {
        return p[p_entity_slot];
    }
    return 1.0f;
}

// ============================================================================
// Table 2: GamePropertyBuffer & Telemetry Operations
// ============================================================================

uint32_t GameDOD::allocate_property(const GamePropertyRowPOD& p_initial_row) {
    const uint32_t slot = property_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    set_property_row(slot, p_initial_row);
    _sync_buffer_selection(manifest_.prop_modded_value_buffer_id, property_allocator_);
    return slot;
}

bool GameDOD::free_property(uint32_t p_property_slot) {
    if (!property_allocator_.is_valid(p_property_slot)) return false;

    GamePropertyRowPOD cleared{};
    cleared.property_flags = PROP_FLAG_NONE;
    set_property_row(p_property_slot, cleared);
    if (auto* snap = _get_column_ptr<PropertyTelemetrySnapshotPOD>(manifest_.prop_telemetry_snapshot_buffer_id)) {
        snap[p_property_slot] = PropertyTelemetrySnapshotPOD{};
    }
    return property_allocator_.release(p_property_slot);
}

GamePropertyRowPOD GameDOD::get_property_row(uint32_t p_property_slot) const noexcept {
    GamePropertyRowPOD row{};
    if (p_property_slot >= capacities_.max_properties) return row;

    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.prop_owner_entity_id_buffer_id)) row.owner_entity_id = p[p_property_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.prop_name_id_buffer_id)) row.property_name_id = p[p_property_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.prop_base_value_buffer_id)) row.base_value = p[p_property_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.prop_modded_value_buffer_id)) row.modded_value = p[p_property_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.prop_base_min_buffer_id)) row.base_min = p[p_property_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.prop_modded_min_buffer_id)) row.modded_min = p[p_property_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.prop_base_max_buffer_id)) row.base_max = p[p_property_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.prop_modded_max_buffer_id)) row.modded_max = p[p_property_slot];
    if (const auto* p = _get_column_ptr<uint16_t>(manifest_.prop_flags_buffer_id)) row.property_flags = p[p_property_slot];
    if (const auto* p = _get_column_ptr<uint64_t>(manifest_.prop_object_id_buffer_id)) row.object_id = p[p_property_slot];
    if (const auto* p = _get_column_ptr<uint64_t>(manifest_.prop_gauge_object_id_buffer_id)) row.gauge_object_id = p[p_property_slot];

    return row;
}

void GameDOD::set_property_row(uint32_t p_property_slot, const GamePropertyRowPOD& p_row) noexcept {
    if (p_property_slot >= capacities_.max_properties) return;

    if (auto* p = _get_column_ptr<uint32_t>(manifest_.prop_owner_entity_id_buffer_id)) p[p_property_slot] = p_row.owner_entity_id;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.prop_name_id_buffer_id)) p[p_property_slot] = p_row.property_name_id;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.prop_base_value_buffer_id)) p[p_property_slot] = p_row.base_value;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.prop_modded_value_buffer_id)) p[p_property_slot] = p_row.modded_value;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.prop_base_min_buffer_id)) p[p_property_slot] = p_row.base_min;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.prop_modded_min_buffer_id)) p[p_property_slot] = p_row.modded_min;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.prop_base_max_buffer_id)) p[p_property_slot] = p_row.base_max;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.prop_modded_max_buffer_id)) p[p_property_slot] = p_row.modded_max;
    if (auto* p = _get_column_ptr<uint16_t>(manifest_.prop_flags_buffer_id)) p[p_property_slot] = p_row.property_flags;
    if (auto* p = _get_column_ptr<uint64_t>(manifest_.prop_object_id_buffer_id)) p[p_property_slot] = p_row.object_id;
    if (auto* p = _get_column_ptr<uint64_t>(manifest_.prop_gauge_object_id_buffer_id)) p[p_property_slot] = p_row.gauge_object_id;

    if (auto* snap = _get_column_ptr<PropertyTelemetrySnapshotPOD>(manifest_.prop_telemetry_snapshot_buffer_id)) {
        PropertyTelemetrySnapshotPOD s{};
        s.base_value = p_row.base_value;
        s.modded_value = p_row.modded_value;
        s.base_min_value = p_row.base_min;
        s.modded_min_value = p_row.modded_min;
        s.base_max_value = p_row.base_max;
        s.modded_max_value = p_row.modded_max;
        s.is_exhausted = (p_row.property_flags & PROP_FLAG_IS_EXHAUSTED) != 0u;
        s.is_locked = (p_row.property_flags & PROP_FLAG_LOCKED) != 0u;
        s.valid = true;
        snap[p_property_slot] = s;
    }
}

PropertyTelemetrySnapshotPOD GameDOD::get_property_telemetry_snapshot(uint32_t p_property_slot) const noexcept {
    if (const PropertyTelemetrySnapshotPOD* ptr = get_property_telemetry_snapshot_ptr(p_property_slot)) {
        return *ptr;
    }
    return PropertyTelemetrySnapshotPOD{};
}

const PropertyTelemetrySnapshotPOD* GameDOD::get_property_telemetry_snapshot_ptr(uint32_t p_property_slot) const noexcept {
    if (!property_allocator_.is_valid(p_property_slot)) return nullptr;
    if (const auto* snap = _get_column_ptr<PropertyTelemetrySnapshotPOD>(manifest_.prop_telemetry_snapshot_buffer_id)) {
        return &snap[p_property_slot];
    }
    return nullptr;
}

// ============================================================================
// Table 3: PropertyModifierBuffer Operations
// ============================================================================

uint32_t GameDOD::allocate_modifier(const PropertyModifierPOD& p_modifier) {
    const uint32_t slot = modifier_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    set_modifier_pod(slot, p_modifier);
    _sync_buffer_selection(manifest_.modifier_pod_buffer_id, modifier_allocator_);
    return slot;
}

bool GameDOD::free_modifier(uint32_t p_modifier_slot) {
    if (!modifier_allocator_.is_valid(p_modifier_slot)) return false;

    PropertyModifierPOD cleared{};
    cleared.target_property_idx = INVALID_ID;
    cleared.flags = MOD_FLAG_NONE;
    set_modifier_pod(p_modifier_slot, cleared);
    return modifier_allocator_.release(p_modifier_slot);
}

PropertyModifierPOD GameDOD::get_modifier_pod(uint32_t p_modifier_slot) const noexcept {
    if (p_modifier_slot < capacities_.max_modifiers) {
        if (const auto* p = _get_column_ptr<PropertyModifierPOD>(manifest_.modifier_pod_buffer_id)) {
            return p[p_modifier_slot];
        }
    }
    return PropertyModifierPOD{};
}

void GameDOD::set_modifier_pod(uint32_t p_modifier_slot, const PropertyModifierPOD& p_modifier) noexcept {
    if (p_modifier_slot >= capacities_.max_modifiers) return;

    if (auto* p = _get_column_ptr<PropertyModifierPOD>(manifest_.modifier_pod_buffer_id)) p[p_modifier_slot] = p_modifier;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.mod_target_property_idx_buffer_id)) p[p_modifier_slot] = p_modifier.target_property_idx;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.mod_value_buffer_id)) p[p_modifier_slot] = p_modifier.value;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.mod_starting_value_buffer_id)) p[p_modifier_slot] = p_modifier.starting_value;
    if (auto* p = _get_column_ptr<float>(manifest_.mod_time_limit_buffer_id)) p[p_modifier_slot] = p_modifier.time_limit;
    if (auto* p = _get_column_ptr<float>(manifest_.mod_current_time_buffer_id)) p[p_modifier_slot] = p_modifier.current_time;
    if (auto* p = _get_column_ptr<uint8_t>(manifest_.mod_type_buffer_id)) p[p_modifier_slot] = p_modifier.mod_type;
    if (auto* p = _get_column_ptr<uint8_t>(manifest_.mod_channel_buffer_id)) p[p_modifier_slot] = p_modifier.target_channel;
    if (auto* p = _get_column_ptr<uint8_t>(manifest_.mod_flags_buffer_id)) p[p_modifier_slot] = p_modifier.flags;
}

uint32_t GameDOD::reclaim_expired_modifiers() noexcept {
    const auto* pod_col = _get_column_ptr<PropertyModifierPOD>(manifest_.modifier_pod_buffer_id);
    if (!pod_col) return 0u;

    uint32_t reclaimed = 0u;
    const uint32_t hwm = modifier_allocator_.get_high_water_mark();
    for (uint32_t i = 0; i < hwm; ++i) {
        if (modifier_allocator_.is_valid(i) && (pod_col[i].flags & MOD_FLAG_EXPIRED) != 0u) {
            free_modifier(i);
            ++reclaimed;
        }
    }
    return reclaimed;
}

// ============================================================================
// Table 4: ActionSequenceBuffer Operations
// ============================================================================

uint32_t GameDOD::allocate_action(
    const ActionSequenceRowPOD& p_initial_row,
    std::span<const SequenceStepPOD> p_sequence_steps,
    std::span<const PiecePropertyBindingPOD> p_property_bindings
) {
    const uint32_t slot = action_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    ActionSequenceRowPOD row = p_initial_row;

    if (!p_sequence_steps.empty() &&
        sequence_step_cursor_ + p_sequence_steps.size() <= capacities_.max_sequence_steps) {
        row.step_offset = sequence_step_cursor_;
        row.step_count = static_cast<uint32_t>(p_sequence_steps.size());
        if (auto* steps = _get_column_ptr<SequenceStepPOD>(manifest_.action_sequence_step_pod_buffer_id)) {
            for (size_t i = 0; i < p_sequence_steps.size(); ++i) {
                steps[sequence_step_cursor_ + i] = p_sequence_steps[i];
            }
        }
        sequence_step_cursor_ += row.step_count;
    }

    if (!p_property_bindings.empty() &&
        piece_binding_cursor_ + p_property_bindings.size() <= capacities_.max_piece_bindings) {
        row.binding_offset = piece_binding_cursor_;
        row.binding_count = static_cast<uint32_t>(p_property_bindings.size());
        if (auto* binds = _get_column_ptr<PiecePropertyBindingPOD>(manifest_.action_binding_pod_buffer_id)) {
            for (size_t i = 0; i < p_property_bindings.size(); ++i) {
                binds[piece_binding_cursor_ + i] = p_property_bindings[i];
            }
        }
        piece_binding_cursor_ += row.binding_count;
    }

    set_action_row(slot, row);
    _sync_buffer_selection(manifest_.action_current_score_buffer_id, action_allocator_);
    return slot;
}

bool GameDOD::free_action(uint32_t p_action_slot) {
    if (!action_allocator_.is_valid(p_action_slot)) return false;

    ActionSequenceRowPOD cleared{};
    cleared.status = ACTION_STATUS_UNAVAILABLE;
    set_action_row(p_action_slot, cleared);
    return action_allocator_.release(p_action_slot);
}

ActionSequenceRowPOD GameDOD::get_action_row(uint32_t p_action_slot) const noexcept {
    ActionSequenceRowPOD row{};
    if (p_action_slot >= capacities_.max_actions) return row;

    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.action_owner_agent_id_buffer_id)) row.owner_agent_id = p[p_action_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.action_base_score_buffer_id)) row.base_score = p[p_action_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.action_current_score_buffer_id)) row.current_score = p[p_action_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.action_priority_buffer_id)) row.priority = p[p_action_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.action_status_buffer_id)) row.status = p[p_action_slot];
    if (const auto* p = _get_column_ptr<uint64_t>(manifest_.action_group_mask_buffer_id)) row.action_group_mask = p[p_action_slot];
    if (const auto* p = _get_column_ptr<uint64_t>(manifest_.action_competition_mask_buffer_id)) row.competition_mask = p[p_action_slot];
    if (const auto* p = _get_column_ptr<float>(manifest_.action_refresh_time_buffer_id)) row.refresh_time = p[p_action_slot];
    if (const auto* p = _get_column_ptr<float>(manifest_.action_current_time_buffer_id)) row.current_time = p[p_action_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.action_current_step_idx_buffer_id)) row.current_step_idx = p[p_action_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.action_step_offset_buffer_id)) row.step_offset = p[p_action_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.action_step_count_buffer_id)) row.step_count = p[p_action_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.action_binding_offset_buffer_id)) row.binding_offset = p[p_action_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.action_binding_count_buffer_id)) row.binding_count = p[p_action_slot];

    return row;
}

void GameDOD::set_action_row(uint32_t p_action_slot, const ActionSequenceRowPOD& p_row) noexcept {
    if (p_action_slot >= capacities_.max_actions) return;

    if (auto* p = _get_column_ptr<uint32_t>(manifest_.action_owner_agent_id_buffer_id)) p[p_action_slot] = p_row.owner_agent_id;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.action_base_score_buffer_id)) p[p_action_slot] = p_row.base_score;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.action_current_score_buffer_id)) p[p_action_slot] = p_row.current_score;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.action_priority_buffer_id)) p[p_action_slot] = p_row.priority;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.action_status_buffer_id)) p[p_action_slot] = p_row.status;
    if (auto* p = _get_column_ptr<uint64_t>(manifest_.action_group_mask_buffer_id)) p[p_action_slot] = p_row.action_group_mask;
    if (auto* p = _get_column_ptr<uint64_t>(manifest_.action_competition_mask_buffer_id)) p[p_action_slot] = p_row.competition_mask;
    if (auto* p = _get_column_ptr<float>(manifest_.action_refresh_time_buffer_id)) p[p_action_slot] = p_row.refresh_time;
    if (auto* p = _get_column_ptr<float>(manifest_.action_current_time_buffer_id)) p[p_action_slot] = p_row.current_time;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.action_current_step_idx_buffer_id)) p[p_action_slot] = p_row.current_step_idx;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.action_step_offset_buffer_id)) p[p_action_slot] = p_row.step_offset;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.action_step_count_buffer_id)) p[p_action_slot] = p_row.step_count;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.action_binding_offset_buffer_id)) p[p_action_slot] = p_row.binding_offset;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.action_binding_count_buffer_id)) p[p_action_slot] = p_row.binding_count;
}

// ============================================================================
// Table 5: InteractionArenaBuffer & Consequence Ring Operations
// ============================================================================

uint32_t GameDOD::allocate_interaction(
    const InteractionArenaRowPOD& p_initial_row,
    std::span<const InteractionParticipantPOD> p_participants
) {
    const uint32_t slot = interaction_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    InteractionArenaRowPOD row = p_initial_row;
    if (!p_participants.empty() &&
        interaction_participant_cursor_ + p_participants.size() <= capacities_.max_interaction_participants) {
        row.participant_offset = interaction_participant_cursor_;
        row.participant_count = static_cast<uint32_t>(p_participants.size());
        if (auto* parts = _get_column_ptr<InteractionParticipantPOD>(manifest_.interaction_participant_pod_buffer_id)) {
            for (size_t i = 0; i < p_participants.size(); ++i) {
                parts[interaction_participant_cursor_ + i] = p_participants[i];
            }
        }
        interaction_participant_cursor_ += row.participant_count;
    }

    set_interaction_row(slot, row);
    _sync_buffer_selection(manifest_.interaction_score_diff_buffer_id, interaction_allocator_);
    return slot;
}

bool GameDOD::free_interaction(uint32_t p_interaction_slot) {
    if (!interaction_allocator_.is_valid(p_interaction_slot)) return false;

    InteractionArenaRowPOD cleared{};
    set_interaction_row(p_interaction_slot, cleared);
    return interaction_allocator_.release(p_interaction_slot);
}

InteractionArenaRowPOD GameDOD::get_interaction_row(uint32_t p_interaction_slot) const noexcept {
    InteractionArenaRowPOD row{};
    if (p_interaction_slot >= capacities_.max_interactions) return row;

    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.interaction_instigator_action_idx_buffer_id)) row.instigator_action_idx = p[p_interaction_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.interaction_instigator_score_buffer_id)) row.instigator_score = p[p_interaction_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.interaction_competition_score_buffer_id)) row.competition_score = p[p_interaction_slot];
    if (const auto* p = _get_column_ptr<int32_t>(manifest_.interaction_score_diff_buffer_id)) row.score_diff = p[p_interaction_slot];
    if (const auto* p = _get_column_ptr<float>(manifest_.interaction_instigator_weight_buffer_id)) row.instigator_weight = p[p_interaction_slot];
    if (const auto* p = _get_column_ptr<float>(manifest_.interaction_competition_weight_buffer_id)) row.competition_weight = p[p_interaction_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.interaction_participant_offset_buffer_id)) row.participant_offset = p[p_interaction_slot];
    if (const auto* p = _get_column_ptr<uint32_t>(manifest_.interaction_participant_count_buffer_id)) row.participant_count = p[p_interaction_slot];

    return row;
}

void GameDOD::set_interaction_row(uint32_t p_interaction_slot, const InteractionArenaRowPOD& p_row) noexcept {
    if (p_interaction_slot >= capacities_.max_interactions) return;

    if (auto* p = _get_column_ptr<uint32_t>(manifest_.interaction_instigator_action_idx_buffer_id)) p[p_interaction_slot] = p_row.instigator_action_idx;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.interaction_instigator_score_buffer_id)) p[p_interaction_slot] = p_row.instigator_score;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.interaction_competition_score_buffer_id)) p[p_interaction_slot] = p_row.competition_score;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.interaction_score_diff_buffer_id)) p[p_interaction_slot] = p_row.score_diff;
    if (auto* p = _get_column_ptr<float>(manifest_.interaction_instigator_weight_buffer_id)) p[p_interaction_slot] = p_row.instigator_weight;
    if (auto* p = _get_column_ptr<float>(manifest_.interaction_competition_weight_buffer_id)) p[p_interaction_slot] = p_row.competition_weight;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.interaction_participant_offset_buffer_id)) p[p_interaction_slot] = p_row.participant_offset;
    if (auto* p = _get_column_ptr<uint32_t>(manifest_.interaction_participant_count_buffer_id)) p[p_interaction_slot] = p_row.participant_count;
}

bool GameDOD::push_consequence_command(const ConsequenceCommandPOD& p_cmd) noexcept {
    auto* ring = _get_column_ptr<ConsequenceCommandPOD>(manifest_.consequence_ring_buffer_id);
    auto* cursors = _get_column_ptr<uint32_t>(manifest_.consequence_ring_cursor_buffer_id);
    if (!ring || !cursors || capacities_.max_consequence_commands == 0u) return false;

    const uint32_t write_idx = cursors[0];
    if (write_idx >= capacities_.max_consequence_commands) return false;

    ring[write_idx] = p_cmd;
    cursors[0] = write_idx + 1u;
    return true;
}

uint32_t GameDOD::drain_consequence_commands(
    std::vector<ConsequenceCommandPOD>& r_out_commands,
    uint32_t p_max_drain
) noexcept {
    const auto* ring = _get_column_ptr<ConsequenceCommandPOD>(manifest_.consequence_ring_buffer_id);
    auto* cursors = _get_column_ptr<uint32_t>(manifest_.consequence_ring_cursor_buffer_id);
    if (!ring || !cursors) return 0u;

    const uint32_t write_idx = std::min(cursors[0], capacities_.max_consequence_commands);
    uint32_t read_idx = std::min(cursors[1], write_idx);
    uint32_t drained = 0u;

    while (read_idx < write_idx && drained < p_max_drain) {
        r_out_commands.push_back(ring[read_idx]);
        ++read_idx;
        ++drained;
    }

    if (read_idx >= write_idx) {
        cursors[0] = 0u;
        cursors[1] = 0u;
    } else {
        cursors[1] = read_idx;
    }
    return drained;
}

// ============================================================================
// Compiled GameplayRule Operations
// ============================================================================

uint32_t GameDOD::allocate_compiled_rule(
    const CompiledGameplayRulePOD& p_rule,
    int32_t p_priority,
    uint8_t p_specificity,
    bool p_important
) {
    const uint32_t slot = rule_allocator_.allocate();
    if (slot == INVALID_ID) return INVALID_ID;

    if (auto* p = _get_column_ptr<CompiledGameplayRulePOD>(manifest_.compiled_rule_pod_buffer_id)) p[slot] = p_rule;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.rule_cascade_layer_buffer_id)) p[slot] = static_cast<int32_t>(p_rule.cascade_layer);
    if (auto* p = _get_column_ptr<int32_t>(manifest_.rule_priority_buffer_id)) p[slot] = p_priority;
    if (auto* p = _get_column_ptr<int32_t>(manifest_.rule_specificity_buffer_id)) p[slot] = static_cast<int32_t>(p_specificity);
    if (auto* p = _get_column_ptr<uint8_t>(manifest_.rule_important_flags_buffer_id)) p[slot] = p_important ? 1u : 0u;

    _sync_buffer_selection(manifest_.compiled_rule_pod_buffer_id, rule_allocator_);
    return slot;
}

bool GameDOD::free_compiled_rule(uint32_t p_rule_slot) {
    if (!rule_allocator_.is_valid(p_rule_slot)) return false;
    if (auto* p = _get_column_ptr<CompiledGameplayRulePOD>(manifest_.compiled_rule_pod_buffer_id)) {
        p[p_rule_slot].enabled = 0u;
    }
    return rule_allocator_.release(p_rule_slot);
}

void GameDOD::clear_compiled_rules() noexcept {
    rule_allocator_.clear();
    _sync_buffer_selection(manifest_.compiled_rule_pod_buffer_id, rule_allocator_);
}

// ============================================================================
// FLEXIBLE TASK GRAPH NODE BUILDERS (G1 - G18)
// ============================================================================

NodeID GameDOD::add_time_scale_cascade_node(
    TaskGraphDOD& r_graph,
    const EntityTimeScaleCascadeTransformLogic<float>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    EntityTimeScaleCascadeTransformLogic<float> logic = p_logic_override;
    if (logic.effective_time_scale_buffer_id == INVALID_ID) logic.effective_time_scale_buffer_id = manifest_.entity_effective_time_scale_buffer_id;
    if (logic.local_time_scale_buffer_id == INVALID_ID)     logic.local_time_scale_buffer_id     = manifest_.entity_local_time_scale_buffer_id;
    if (logic.parent_entity_id_buffer_id == INVALID_ID)     logic.parent_entity_id_buffer_id     = manifest_.entity_parent_id_buffer_id;
    if (logic.active_flags_buffer_id == INVALID_ID)         logic.active_flags_buffer_id         = manifest_.entity_active_flags_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<EntityTimeScaleCascadeTransformLogic<float>>>(GamesTransformLogicID::EntityTimeScaleCascade, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t count = static_cast<int64_t>(capacities_.max_entities);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.effective_time_scale_buffer_id, BufferAccessMode::READ_WRITE, sizeof(float), count),
            make_grant_requirement(logic.local_time_scale_buffer_id,     BufferAccessMode::READ,       sizeof(float), count),
            make_grant_requirement(logic.parent_entity_id_buffer_id,     BufferAccessMode::READ,       sizeof(uint32_t), count),
            make_grant_requirement(logic.active_flags_buffer_id,         BufferAccessMode::READ,       sizeof(uint32_t), count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_modifier_tick_node(
    TaskGraphDOD& r_graph,
    const PropertyModifierTickTransformLogic<float>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    PropertyModifierTickTransformLogic<float> logic = p_logic_override;
    if (logic.modifier_pod_buffer_id == INVALID_ID)         logic.modifier_pod_buffer_id         = manifest_.modifier_pod_buffer_id;
    if (logic.current_time_buffer_id == INVALID_ID)         logic.current_time_buffer_id         = manifest_.mod_current_time_buffer_id;
    if (logic.effective_time_scale_buffer_id == INVALID_ID) logic.effective_time_scale_buffer_id = manifest_.entity_effective_time_scale_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<PropertyModifierTickTransformLogic<float>>>(GamesTransformLogicID::PropertyModifierTick, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t mod_count = static_cast<int64_t>(capacities_.max_modifiers);
        const int64_t ent_count = static_cast<int64_t>(capacities_.max_entities);
        const GrantPartPOD reqs[3] = {
            make_grant_requirement(logic.modifier_pod_buffer_id,         BufferAccessMode::READ_WRITE, sizeof(PropertyModifierPOD), mod_count),
            make_grant_requirement(logic.current_time_buffer_id,         BufferAccessMode::READ_WRITE, sizeof(float), mod_count),
            make_grant_requirement(logic.effective_time_scale_buffer_id, BufferAccessMode::READ,       sizeof(float), ent_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_modifier_aggregate_node(
    TaskGraphDOD& r_graph,
    const PropertyModifierAggregateTransformLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    PropertyModifierAggregateTransformLogic<int32_t> logic = p_logic_override;
    if (logic.modded_value_buffer_id == INVALID_ID)   logic.modded_value_buffer_id   = manifest_.prop_modded_value_buffer_id;
    if (logic.base_value_buffer_id == INVALID_ID)     logic.base_value_buffer_id     = manifest_.prop_base_value_buffer_id;
    if (logic.modifier_pod_buffer_id == INVALID_ID)   logic.modifier_pod_buffer_id   = manifest_.modifier_pod_buffer_id;
    if (logic.property_flags_buffer_id == INVALID_ID) logic.property_flags_buffer_id = manifest_.prop_flags_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<PropertyModifierAggregateTransformLogic<int32_t>>>(GamesTransformLogicID::PropertyModifierAggregate, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t prop_count = static_cast<int64_t>(capacities_.max_properties);
        const int64_t mod_count  = static_cast<int64_t>(capacities_.max_modifiers);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.modded_value_buffer_id,   BufferAccessMode::READ_WRITE, sizeof(int32_t), prop_count),
            make_grant_requirement(logic.base_value_buffer_id,     BufferAccessMode::READ,       sizeof(int32_t), prop_count),
            make_grant_requirement(logic.modifier_pod_buffer_id,   BufferAccessMode::READ,       sizeof(PropertyModifierPOD), mod_count),
            make_grant_requirement(logic.property_flags_buffer_id, BufferAccessMode::READ,       sizeof(uint16_t), prop_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_property_clamp_node(
    TaskGraphDOD& r_graph,
    const PropertyRangeClampAndExhaustTransformLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    PropertyRangeClampAndExhaustTransformLogic<int32_t> logic = p_logic_override;
    if (logic.modded_value_buffer_id == INVALID_ID)   logic.modded_value_buffer_id   = manifest_.prop_modded_value_buffer_id;
    if (logic.modded_min_buffer_id == INVALID_ID)     logic.modded_min_buffer_id     = manifest_.prop_modded_min_buffer_id;
    if (logic.modded_max_buffer_id == INVALID_ID)     logic.modded_max_buffer_id     = manifest_.prop_modded_max_buffer_id;
    if (logic.property_flags_buffer_id == INVALID_ID) logic.property_flags_buffer_id = manifest_.prop_flags_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<PropertyRangeClampAndExhaustTransformLogic<int32_t>>>(GamesTransformLogicID::PropertyRangeClampAndExhaust, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t prop_count = static_cast<int64_t>(capacities_.max_properties);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.modded_value_buffer_id,   BufferAccessMode::READ_WRITE, sizeof(int32_t), prop_count),
            make_grant_requirement(logic.modded_min_buffer_id,     BufferAccessMode::READ,       sizeof(int32_t), prop_count),
            make_grant_requirement(logic.modded_max_buffer_id,     BufferAccessMode::READ,       sizeof(int32_t), prop_count),
            make_grant_requirement(logic.property_flags_buffer_id, BufferAccessMode::READ_WRITE, sizeof(uint16_t), prop_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_rule_apply_node(
    TaskGraphDOD& r_graph,
    const GameplayRuleApplyTransformLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    GameplayRuleApplyTransformLogic<int32_t> logic = p_logic_override;
    if (logic.target_buffer_id == INVALID_ID)         logic.target_buffer_id         = manifest_.prop_modded_value_buffer_id;
    if (logic.compiled_rule_buffer_id == INVALID_ID)  logic.compiled_rule_buffer_id  = manifest_.compiled_rule_pod_buffer_id;
    if (logic.property_flags_buffer_id == INVALID_ID) logic.property_flags_buffer_id = manifest_.prop_flags_buffer_id;
    if (logic.archetype_mask_buffer_id == INVALID_ID) logic.archetype_mask_buffer_id = manifest_.entity_archetype_mask_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<GameplayRuleApplyTransformLogic<int32_t>>>(GamesTransformLogicID::GameplayRuleApply, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t prop_count = static_cast<int64_t>(capacities_.max_properties);
        const int64_t rule_count = static_cast<int64_t>(capacities_.max_compiled_rules);
        const int64_t ent_count  = static_cast<int64_t>(capacities_.max_entities);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.target_buffer_id,         BufferAccessMode::READ_WRITE, sizeof(int32_t), prop_count),
            make_grant_requirement(logic.compiled_rule_buffer_id,  BufferAccessMode::READ,       sizeof(CompiledGameplayRulePOD), rule_count),
            make_grant_requirement(logic.property_flags_buffer_id, BufferAccessMode::READ,       sizeof(uint16_t), prop_count),
            make_grant_requirement(logic.archetype_mask_buffer_id, BufferAccessMode::READ,       sizeof(uint32_t), ent_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_piece_action_refresh_node(
    TaskGraphDOD& r_graph,
    const PieceActionScoreRefreshTransformLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    PieceActionScoreRefreshTransformLogic<int32_t> logic = p_logic_override;
    if (logic.current_score_buffer_id == INVALID_ID)         logic.current_score_buffer_id         = manifest_.action_current_score_buffer_id;
    if (logic.action_status_buffer_id == INVALID_ID)         logic.action_status_buffer_id         = manifest_.action_status_buffer_id;
    if (logic.property_modded_value_buffer_id == INVALID_ID) logic.property_modded_value_buffer_id = manifest_.prop_modded_value_buffer_id;
    if (logic.binding_pod_buffer_id == INVALID_ID)           logic.binding_pod_buffer_id           = manifest_.action_binding_pod_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<PieceActionScoreRefreshTransformLogic<int32_t>>>(GamesTransformLogicID::PieceActionScoreRefresh, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t act_count  = static_cast<int64_t>(capacities_.max_actions);
        const int64_t prop_count = static_cast<int64_t>(capacities_.max_properties);
        const int64_t bind_count = static_cast<int64_t>(capacities_.max_piece_bindings);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.current_score_buffer_id,         BufferAccessMode::READ_WRITE, sizeof(int32_t), act_count),
            make_grant_requirement(logic.action_status_buffer_id,         BufferAccessMode::READ_WRITE, sizeof(int32_t), act_count),
            make_grant_requirement(logic.property_modded_value_buffer_id, BufferAccessMode::READ,       sizeof(int32_t), prop_count),
            make_grant_requirement(logic.binding_pod_buffer_id,           BufferAccessMode::READ,       sizeof(PiecePropertyBindingPOD), bind_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_agent_sequence_step_node(
    TaskGraphDOD& r_graph,
    const AgentActionSequenceStepTransformLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    AgentActionSequenceStepTransformLogic<int32_t> logic = p_logic_override;
    if (logic.agent_current_score_buffer_id == INVALID_ID) logic.agent_current_score_buffer_id = manifest_.action_current_score_buffer_id;
    if (logic.current_step_idx_buffer_id == INVALID_ID)    logic.current_step_idx_buffer_id    = manifest_.action_current_step_idx_buffer_id;
    if (logic.sequence_step_pod_buffer_id == INVALID_ID)   logic.sequence_step_pod_buffer_id   = manifest_.action_sequence_step_pod_buffer_id;
    if (logic.piece_action_score_buffer_id == INVALID_ID)  logic.piece_action_score_buffer_id  = manifest_.action_current_score_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<AgentActionSequenceStepTransformLogic<int32_t>>>(GamesTransformLogicID::AgentActionSequenceStep, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t act_count  = static_cast<int64_t>(capacities_.max_actions);
        const int64_t step_count = static_cast<int64_t>(capacities_.max_sequence_steps);
        const GrantPartPOD reqs[3] = {
            make_grant_requirement(logic.agent_current_score_buffer_id, BufferAccessMode::READ_WRITE, sizeof(int32_t), act_count),
            make_grant_requirement(logic.current_step_idx_buffer_id,    BufferAccessMode::READ_WRITE, sizeof(int32_t), act_count),
            make_grant_requirement(logic.sequence_step_pod_buffer_id,   BufferAccessMode::READ,       sizeof(SequenceStepPOD), step_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_interaction_resolve_node(
    TaskGraphDOD& r_graph,
    const InteractionTugOfWarResolveTransformLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    InteractionTugOfWarResolveTransformLogic<int32_t> logic = p_logic_override;
    if (logic.score_diff_buffer_id == INVALID_ID)        logic.score_diff_buffer_id        = manifest_.interaction_score_diff_buffer_id;
    if (logic.instigator_score_buffer_id == INVALID_ID)  logic.instigator_score_buffer_id  = manifest_.interaction_instigator_score_buffer_id;
    if (logic.competition_score_buffer_id == INVALID_ID) logic.competition_score_buffer_id = manifest_.interaction_competition_score_buffer_id;
    if (logic.consequence_ring_buffer_id == INVALID_ID)  logic.consequence_ring_buffer_id  = manifest_.consequence_ring_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<InteractionTugOfWarResolveTransformLogic<int32_t>>>(GamesTransformLogicID::InteractionTugOfWarResolve, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t int_count  = static_cast<int64_t>(capacities_.max_interactions);
        const int64_t ring_count = static_cast<int64_t>(capacities_.max_consequence_commands);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.score_diff_buffer_id,        BufferAccessMode::READ_WRITE, sizeof(int32_t), int_count),
            make_grant_requirement(logic.instigator_score_buffer_id,  BufferAccessMode::READ,       sizeof(int32_t), int_count),
            make_grant_requirement(logic.competition_score_buffer_id, BufferAccessMode::READ,       sizeof(int32_t), int_count),
            make_grant_requirement(logic.consequence_ring_buffer_id,  BufferAccessMode::READ_WRITE, sizeof(ConsequenceCommandPOD), ring_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_possession_blend_node(
    TaskGraphDOD& r_graph,
    const AgentPossessionTransformBlendTransformLogic<godot::Vector3>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    AgentPossessionTransformBlendTransformLogic<godot::Vector3> logic = p_logic_override;
    if (logic.agent_transform_buffer_id == INVALID_ID)     logic.agent_transform_buffer_id     = manifest_.agent_transform_pos_buffer_id;
    if (logic.player_transform_buffer_id == INVALID_ID)    logic.player_transform_buffer_id    = manifest_.player_transform_pos_buffer_id;
    if (logic.agent_to_player_idx_buffer_id == INVALID_ID) logic.agent_to_player_idx_buffer_id = manifest_.agent_to_player_idx_buffer_id;
    if (logic.completion_flags_buffer_id == INVALID_ID)    logic.completion_flags_buffer_id    = manifest_.possession_completion_flags_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<AgentPossessionTransformBlendTransformLogic<godot::Vector3>>>(GamesTransformLogicID::AgentPossessionTransformBlend, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t ent_count = static_cast<int64_t>(capacities_.max_entities);
        const int64_t plr_count = static_cast<int64_t>(capacities_.max_players);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.agent_transform_buffer_id,     BufferAccessMode::READ_WRITE, sizeof(godot::Vector3), ent_count),
            make_grant_requirement(logic.player_transform_buffer_id,    BufferAccessMode::READ_WRITE, sizeof(godot::Vector3), plr_count),
            make_grant_requirement(logic.agent_to_player_idx_buffer_id, BufferAccessMode::READ,       sizeof(int32_t), ent_count),
            make_grant_requirement(logic.completion_flags_buffer_id,    BufferAccessMode::READ_WRITE, sizeof(uint32_t), ent_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_control_map_filter_node(
    TaskGraphDOD& r_graph,
    const ControlMapInputFilterTransformLogic<godot::Vector2>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    ControlMapInputFilterTransformLogic<godot::Vector2> logic = p_logic_override;
    if (logic.target_input_buffer_id == INVALID_ID)     logic.target_input_buffer_id     = manifest_.filtered_input_vec2_buffer_id;
    if (logic.raw_input_buffer_id == INVALID_ID)        logic.raw_input_buffer_id        = manifest_.raw_input_vec2_buffer_id;
    if (logic.world_move_dir_buffer_id == INVALID_ID)   logic.world_move_dir_buffer_id   = manifest_.world_move_dir_vec3_buffer_id;
    if (logic.camera_transform_buffer_id == INVALID_ID) logic.camera_transform_buffer_id = manifest_.camera_transform_pod_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<ControlMapInputFilterTransformLogic<godot::Vector2>>>(GamesTransformLogicID::ControlMapInputFilter, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t plr_count = static_cast<int64_t>(capacities_.max_players);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.target_input_buffer_id,     BufferAccessMode::READ_WRITE, sizeof(godot::Vector2), plr_count),
            make_grant_requirement(logic.raw_input_buffer_id,        BufferAccessMode::READ,       sizeof(godot::Vector2), plr_count),
            make_grant_requirement(logic.world_move_dir_buffer_id,   BufferAccessMode::READ_WRITE, sizeof(godot::Vector3), plr_count),
            make_grant_requirement(logic.camera_transform_buffer_id, BufferAccessMode::READ,       sizeof(InputTransform3DPOD), plr_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_snapshot_sync_node(
    TaskGraphDOD& r_graph,
    EntitySnapshotSyncMode p_sync_mode,
    const GameEntitySnapshotSyncTransformLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    GameEntitySnapshotSyncTransformLogic<int32_t> logic = p_logic_override;
    logic.sync_mode = static_cast<uint8_t>(p_sync_mode);
    if (logic.modded_value_buffer_id == INVALID_ID)       logic.modded_value_buffer_id       = manifest_.prop_modded_value_buffer_id;
    if (logic.base_value_buffer_id == INVALID_ID)         logic.base_value_buffer_id         = manifest_.prop_base_value_buffer_id;
    if (logic.property_flags_buffer_id == INVALID_ID)     logic.property_flags_buffer_id     = manifest_.prop_flags_buffer_id;
    if (logic.telemetry_snapshot_buffer_id == INVALID_ID) logic.telemetry_snapshot_buffer_id = manifest_.prop_telemetry_snapshot_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<GameEntitySnapshotSyncTransformLogic<int32_t>>>(GamesTransformLogicID::GameEntitySnapshotSync, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t prop_count = static_cast<int64_t>(capacities_.max_properties);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.modded_value_buffer_id,       BufferAccessMode::READ_WRITE, sizeof(int32_t), prop_count),
            make_grant_requirement(logic.base_value_buffer_id,         BufferAccessMode::READ_WRITE, sizeof(int32_t), prop_count),
            make_grant_requirement(logic.property_flags_buffer_id,     BufferAccessMode::READ_WRITE, sizeof(uint16_t), prop_count),
            make_grant_requirement(logic.telemetry_snapshot_buffer_id, BufferAccessMode::READ_WRITE, sizeof(PropertyTelemetrySnapshotPOD), prop_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_consequence_drain_node(
    TaskGraphDOD& r_graph,
    const ConsequenceRingDrainTransformLogic<void*>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    ConsequenceRingDrainTransformLogic<void*> logic = p_logic_override;
    if (logic.consequence_ring_buffer_id == INVALID_ID)   logic.consequence_ring_buffer_id   = manifest_.consequence_ring_buffer_id;
    if (logic.target_score_buffer_id == INVALID_ID)       logic.target_score_buffer_id       = manifest_.prop_modded_value_buffer_id;
    if (logic.target_flags_buffer_id == INVALID_ID)       logic.target_flags_buffer_id       = manifest_.prop_flags_buffer_id;
    if (logic.entity_consequence_buffer_id == INVALID_ID) logic.entity_consequence_buffer_id = manifest_.entity_consequence_mask_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_transform_or_default<TransformTask<ConsequenceRingDrainTransformLogic<void*>>>(GamesTransformLogicID::ConsequenceRingDrain, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t ring_count = static_cast<int64_t>(capacities_.max_consequence_commands);
        const int64_t prop_count = static_cast<int64_t>(capacities_.max_properties);
        const int64_t ent_count  = static_cast<int64_t>(capacities_.max_entities);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.consequence_ring_buffer_id,   BufferAccessMode::READ_WRITE, sizeof(ConsequenceCommandPOD), ring_count),
            make_grant_requirement(logic.target_score_buffer_id,       BufferAccessMode::READ_WRITE, sizeof(int32_t), prop_count),
            make_grant_requirement(logic.target_flags_buffer_id,       BufferAccessMode::READ_WRITE, sizeof(uint16_t), prop_count),
            make_grant_requirement(logic.entity_consequence_buffer_id, BufferAccessMode::READ_WRITE, sizeof(uint32_t), ent_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_property_exhausted_query_node(
    TaskGraphDOD& r_graph,
    const PropertyExhaustedOrLockedQueryLogic<int32_t>& p_logic_override,
    QueryOp p_op,
    const GamesNodeExecutionConfig& p_config
) const {
    PropertyExhaustedOrLockedQueryLogic<int32_t> logic = p_logic_override;
    if (logic.target_buffer_id == INVALID_ID)         logic.target_buffer_id         = manifest_.prop_modded_value_buffer_id;
    if (logic.property_flags_buffer_id == INVALID_ID) logic.property_flags_buffer_id = manifest_.prop_flags_buffer_id;
    if (logic.modded_value_buffer_id == INVALID_ID)   logic.modded_value_buffer_id   = manifest_.prop_modded_value_buffer_id;
    if (logic.modded_min_buffer_id == INVALID_ID)     logic.modded_min_buffer_id     = manifest_.prop_modded_min_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::QUERY_CULLER);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_query_or_default(GamesQueryLogicID::PropertyExhaustedOrLocked, p_op, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t prop_count = static_cast<int64_t>(capacities_.max_properties);
        const GrantPartPOD reqs[3] = {
            make_grant_requirement(logic.target_buffer_id,         BufferAccessMode::READ, sizeof(int32_t), prop_count),
            make_grant_requirement(logic.property_flags_buffer_id, BufferAccessMode::READ, sizeof(uint16_t), prop_count),
            make_grant_requirement(logic.modded_min_buffer_id,     BufferAccessMode::READ, sizeof(int32_t), prop_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_action_conflict_query_node(
    TaskGraphDOD& r_graph,
    const ActionCompetitionConflictQueryLogic<int64_t>& p_logic_override,
    QueryOp p_op,
    const GamesNodeExecutionConfig& p_config
) const {
    ActionCompetitionConflictQueryLogic<int64_t> logic = p_logic_override;
    if (logic.target_buffer_id == INVALID_ID)            logic.target_buffer_id            = manifest_.action_competition_mask_buffer_id;
    if (logic.competition_mask_buffer_id == INVALID_ID)  logic.competition_mask_buffer_id  = manifest_.action_competition_mask_buffer_id;
    if (logic.action_group_mask_buffer_id == INVALID_ID) logic.action_group_mask_buffer_id = manifest_.action_group_mask_buffer_id;
    if (logic.priority_buffer_id == INVALID_ID)          logic.priority_buffer_id          = manifest_.action_priority_buffer_id;
    if (logic.status_buffer_id == INVALID_ID)            logic.status_buffer_id            = manifest_.action_status_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::QUERY_CULLER);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_query_or_default(GamesQueryLogicID::ActionCompetitionConflict, p_op, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t act_count = static_cast<int64_t>(capacities_.max_actions);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.competition_mask_buffer_id,  BufferAccessMode::READ, sizeof(uint64_t), act_count),
            make_grant_requirement(logic.action_group_mask_buffer_id, BufferAccessMode::READ, sizeof(uint64_t), act_count),
            make_grant_requirement(logic.priority_buffer_id,          BufferAccessMode::READ, sizeof(int32_t), act_count),
            make_grant_requirement(logic.status_buffer_id,            BufferAccessMode::READ, sizeof(int32_t), act_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_agent_exclusivity_query_node(
    TaskGraphDOD& r_graph,
    const AgentExclusivityFilterQueryLogic<int32_t>& p_logic_override,
    QueryOp p_op,
    const GamesNodeExecutionConfig& p_config
) const {
    AgentExclusivityFilterQueryLogic<int32_t> logic = p_logic_override;
    if (logic.target_buffer_id == INVALID_ID)                logic.target_buffer_id                = manifest_.entity_controlling_player_id_buffer_id;
    if (logic.controlling_player_id_buffer_id == INVALID_ID) logic.controlling_player_id_buffer_id = manifest_.entity_controlling_player_id_buffer_id;
    if (logic.exclusivity_buffer_id == INVALID_ID)           logic.exclusivity_buffer_id           = manifest_.entity_exclusivity_buffer_id;
    if (logic.group_mask_buffer_id == INVALID_ID)            logic.group_mask_buffer_id            = manifest_.entity_group_mask_buffer_id;
    if (logic.active_flags_buffer_id == INVALID_ID)          logic.active_flags_buffer_id          = manifest_.entity_active_flags_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::QUERY_CULLER);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_query_or_default(GamesQueryLogicID::AgentExclusivityFilter, p_op, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t ent_count = static_cast<int64_t>(capacities_.max_entities);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.controlling_player_id_buffer_id, BufferAccessMode::READ, sizeof(int32_t), ent_count),
            make_grant_requirement(logic.exclusivity_buffer_id,           BufferAccessMode::READ, sizeof(uint8_t), ent_count),
            make_grant_requirement(logic.group_mask_buffer_id,            BufferAccessMode::READ, sizeof(uint64_t), ent_count),
            make_grant_requirement(logic.active_flags_buffer_id,          BufferAccessMode::READ, sizeof(uint32_t), ent_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_rule_selector_node(
    TaskGraphDOD& r_graph,
    const GameplayRuleSelectorQueryLogic<int32_t>& p_logic_override,
    QueryOp p_op,
    const GamesNodeExecutionConfig& p_config
) const {
    GameplayRuleSelectorQueryLogic<int32_t> logic = p_logic_override;
    if (logic.target_buffer_id == INVALID_ID)           logic.target_buffer_id           = manifest_.prop_modded_value_buffer_id;
    if (logic.archetype_mask_buffer_id == INVALID_ID)   logic.archetype_mask_buffer_id   = manifest_.entity_archetype_mask_buffer_id;
    if (logic.group_mask_buffer_id == INVALID_ID)       logic.group_mask_buffer_id       = manifest_.entity_group_mask_buffer_id;
    if (logic.parent_entity_id_buffer_id == INVALID_ID) logic.parent_entity_id_buffer_id = manifest_.entity_parent_id_buffer_id;
    if (logic.property_flags_buffer_id == INVALID_ID)   logic.property_flags_buffer_id   = manifest_.prop_flags_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::QUERY_CULLER);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_query_or_default(GamesQueryLogicID::GameplayRuleSelector, p_op, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t prop_count = static_cast<int64_t>(capacities_.max_properties);
        const int64_t ent_count  = static_cast<int64_t>(capacities_.max_entities);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.target_buffer_id,           BufferAccessMode::READ, sizeof(int32_t), prop_count),
            make_grant_requirement(logic.archetype_mask_buffer_id,   BufferAccessMode::READ, sizeof(uint32_t), ent_count),
            make_grant_requirement(logic.group_mask_buffer_id,       BufferAccessMode::READ, sizeof(uint64_t), ent_count),
            make_grant_requirement(logic.parent_entity_id_buffer_id, BufferAccessMode::READ, sizeof(uint32_t), ent_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_rule_specificity_node(
    TaskGraphDOD& r_graph,
    const GameplayRuleSpecificityMetadataLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    GameplayRuleSpecificityMetadataLogic<int32_t> logic = p_logic_override;
    if (logic.target_buffer_id == INVALID_ID)        logic.target_buffer_id        = manifest_.rule_output_key_buffer_id;
    if (logic.cascade_layer_buffer_id == INVALID_ID) logic.cascade_layer_buffer_id = manifest_.rule_cascade_layer_buffer_id;
    if (logic.priority_buffer_id == INVALID_ID)      logic.priority_buffer_id      = manifest_.rule_priority_buffer_id;
    if (logic.specificity_buffer_id == INVALID_ID)   logic.specificity_buffer_id   = manifest_.rule_specificity_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_metadata_or_default(GamesMetadataLogicID::GameplayRuleSpecificity, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t rule_count = static_cast<int64_t>(capacities_.max_compiled_rules);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.target_buffer_id,        BufferAccessMode::READ_WRITE, sizeof(int32_t), rule_count),
            make_grant_requirement(logic.cascade_layer_buffer_id, BufferAccessMode::READ,       sizeof(int32_t), rule_count),
            make_grant_requirement(logic.priority_buffer_id,      BufferAccessMode::READ,       sizeof(int32_t), rule_count),
            make_grant_requirement(logic.specificity_buffer_id,   BufferAccessMode::READ,       sizeof(int32_t), rule_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

NodeID GameDOD::add_board_partition_node(
    TaskGraphDOD& r_graph,
    const BoardPresencePartitionMetadataLogic<int32_t>& p_logic_override,
    const GamesNodeExecutionConfig& p_config
) const {
    BoardPresencePartitionMetadataLogic<int32_t> logic = p_logic_override;
    if (logic.target_buffer_id == INVALID_ID)           logic.target_buffer_id           = manifest_.entity_board_id_buffer_id;
    if (logic.board_id_buffer_id == INVALID_ID)         logic.board_id_buffer_id         = manifest_.entity_board_id_buffer_id;
    if (logic.parent_entity_id_buffer_id == INVALID_ID) logic.parent_entity_id_buffer_id = manifest_.entity_parent_id_buffer_id;
    if (logic.archetype_mask_buffer_id == INVALID_ID)   logic.archetype_mask_buffer_id   = manifest_.entity_archetype_mask_buffer_id;
    if (logic.active_flags_buffer_id == INVALID_ID)     logic.active_flags_buffer_id     = manifest_.entity_active_flags_buffer_id;

    NodeID node_id = r_graph.add_task_node(TaskTypeDOD::NATIVE_CPU);
    r_graph.set_node_priority(node_id, p_config.execution_priority);
    r_graph.configure_native_interface(node_id, create_metadata_or_default(GamesMetadataLogicID::BoardPresencePartition, logic, p_config, *this));

    if (!p_config.custom_requirements.empty()) {
        r_graph.set_node_requirements(node_id, p_config.custom_requirements);
    } else {
        const int64_t ent_count = static_cast<int64_t>(capacities_.max_entities);
        const GrantPartPOD reqs[4] = {
            make_grant_requirement(logic.target_buffer_id,           BufferAccessMode::READ_WRITE, sizeof(int32_t), ent_count),
            make_grant_requirement(logic.parent_entity_id_buffer_id, BufferAccessMode::READ,       sizeof(uint32_t), ent_count),
            make_grant_requirement(logic.archetype_mask_buffer_id,   BufferAccessMode::READ,       sizeof(uint32_t), ent_count),
            make_grant_requirement(logic.active_flags_buffer_id,     BufferAccessMode::READ,       sizeof(uint32_t), ent_count)
        };
        r_graph.set_node_requirements(node_id, reqs);
    }
    return node_id;
}

// ============================================================================
// Composable Pipeline Stage Injection & Wave Co-Scheduling
// ============================================================================

GamesPipelineTopology GameDOD::inject_pipeline_stages(
    TaskGraphDOD& r_graph,
    uint32_t p_stage_mask,
    bool p_auto_connect_stages
) {
    GamesPipelineTopology topo{};

    auto append_stage = [&](GamesPipelineStageFlags p_flag, const std::vector<NodeID>& p_stage_nodes, bool p_chain_internal) {
        if (p_stage_nodes.empty()) return;

        GamesStageAnchor anchor{};
        anchor.stage = p_flag;
        anchor.nodes = p_stage_nodes;
        anchor.entry_node = p_stage_nodes.front();
        anchor.exit_node  = p_stage_nodes.back();

        if (p_chain_internal && p_stage_nodes.size() > 1) {
            for (size_t i = 0; i + 1 < p_stage_nodes.size(); ++i) {
                r_graph.connect_nodes(p_stage_nodes[i], 0, p_stage_nodes[i + 1], 0);
            }
        }

        if (p_auto_connect_stages && !topo.stages.empty()) {
            const GamesStageAnchor& prev = topo.stages.back();
            if (p_chain_internal) {
                r_graph.connect_nodes(prev.exit_node, 0, anchor.entry_node, 0);
            } else {
                // Parallel wave stage: connect prev.exit_node to all parallel nodes in this stage
                for (NodeID n : anchor.nodes) {
                    r_graph.connect_nodes(prev.exit_node, 0, n, 0);
                }
            }
        }

        if (topo.first_node == INVALID_ID) {
            topo.first_node = anchor.entry_node;
        }
        topo.last_node = anchor.exit_node;
        topo.stages.push_back(std::move(anchor));
    };

    if ((p_stage_mask & GAMES_STAGE_SNAPSHOT_PULL) != 0u) {
        NodeID n = add_snapshot_sync_node(r_graph, SNAPSHOT_SYNC_PULL_FROM_GODOT);
        append_stage(GAMES_STAGE_SNAPSHOT_PULL, {n}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_INPUT_AND_POSSESSION) != 0u) {
        // Input filtering and possession transform blending touch independent buffers and run in the SAME Kahn wave!
        NodeID n_input = add_control_map_filter_node(r_graph);
        NodeID n_poss  = add_possession_blend_node(r_graph);
        append_stage(GAMES_STAGE_INPUT_AND_POSSESSION, {n_input, n_poss}, false);
    }

    if ((p_stage_mask & GAMES_STAGE_HIERARCHY_AND_TIME) != 0u) {
        // Time scale cascade and board presence partition read parent_entity_id in parallel within the SAME Kahn wave!
        NodeID n_time  = add_time_scale_cascade_node(r_graph);
        NodeID n_board = add_board_partition_node(r_graph);
        append_stage(GAMES_STAGE_HIERARCHY_AND_TIME, {n_time, n_board}, false);
    }

    if ((p_stage_mask & GAMES_STAGE_MODIFIER_TICK) != 0u) {
        NodeID n = add_modifier_tick_node(r_graph);
        append_stage(GAMES_STAGE_MODIFIER_TICK, {n}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_MODIFIER_AGGREGATE) != 0u) {
        NodeID n = add_modifier_aggregate_node(r_graph);
        append_stage(GAMES_STAGE_MODIFIER_AGGREGATE, {n}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_GAMEPLAY_RULES) != 0u) {
        NodeID n_spec  = add_rule_specificity_node(r_graph);
        NodeID n_apply = add_rule_apply_node(r_graph);
        append_stage(GAMES_STAGE_GAMEPLAY_RULES, {n_spec, n_apply}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_PROPERTY_CLAMP) != 0u) {
        NodeID n = add_property_clamp_node(r_graph);
        append_stage(GAMES_STAGE_PROPERTY_CLAMP, {n}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_PIECE_ACTIONS) != 0u) {
        NodeID n = add_piece_action_refresh_node(r_graph);
        append_stage(GAMES_STAGE_PIECE_ACTIONS, {n}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_AGENT_ACTIONS) != 0u) {
        NodeID n_conflict = add_action_conflict_query_node(r_graph);
        NodeID n_seq      = add_agent_sequence_step_node(r_graph);
        append_stage(GAMES_STAGE_AGENT_ACTIONS, {n_conflict, n_seq}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_INTERACTION_RESOLVE) != 0u) {
        NodeID n = add_interaction_resolve_node(r_graph);
        append_stage(GAMES_STAGE_INTERACTION_RESOLVE, {n}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_CONSEQUENCE_DRAIN) != 0u) {
        NodeID n = add_consequence_drain_node(r_graph);
        append_stage(GAMES_STAGE_CONSEQUENCE_DRAIN, {n}, true);
    }

    if ((p_stage_mask & GAMES_STAGE_SNAPSHOT_PUSH) != 0u) {
        NodeID n = add_snapshot_sync_node(r_graph, SNAPSHOT_SYNC_PUSH_TO_GODOT);
        append_stage(GAMES_STAGE_SNAPSHOT_PUSH, {n}, true);
    }

    active_topology_ = topo;
    return topo;
}

bool GameDOD::schedule_node_with_stage(
    TaskGraphDOD& r_graph,
    const GamesPipelineTopology& p_topology,
    GamesPipelineStageFlags p_target_stage,
    NodeID p_custom_node,
    bool p_join_downstream
) {
    if (p_custom_node == INVALID_ID) return false;

    for (size_t i = 0; i < p_topology.stages.size(); ++i) {
        if (p_topology.stages[i].stage == p_target_stage) {
            if (i > 0 && p_topology.stages[i - 1].exit_node != INVALID_ID) {
                r_graph.connect_nodes(p_topology.stages[i - 1].exit_node, 0, p_custom_node, 0);
            }
            if (p_join_downstream && (i + 1) < p_topology.stages.size() && p_topology.stages[i + 1].entry_node != INVALID_ID) {
                r_graph.connect_nodes(p_custom_node, 0, p_topology.stages[i + 1].entry_node, 0);
            }
            return true;
        }
    }
    return false;
}

bool GameDOD::schedule_node_between_stages(
    TaskGraphDOD& r_graph,
    const GamesPipelineTopology& p_topology,
    GamesPipelineStageFlags p_upstream_stage,
    GamesPipelineStageFlags p_downstream_stage,
    NodeID p_custom_node
) {
    if (p_custom_node == INVALID_ID) return false;

    const GamesStageAnchor* up = p_topology.get_stage_anchor(p_upstream_stage);
    const GamesStageAnchor* down = p_topology.get_stage_anchor(p_downstream_stage);
    if (!up || !down || !up->is_valid() || !down->is_valid()) {
        return false;
    }

    r_graph.connect_nodes(up->exit_node, 0, p_custom_node, 0);
    r_graph.connect_nodes(p_custom_node, 0, down->entry_node, 0);
    return true;
}

void GameDOD::execute_step(
    double p_delta,
    TaskGraphDOD* p_override_graph,
    bool p_reclaim_expired_modifiers
) {
    sync_selection_ranges();

    TaskGraphDOD* target_graph = p_override_graph ? p_override_graph : default_task_graph_.get();
    if (target_graph) {
        target_graph->execute_graph_dod(p_delta);
    }

    if (p_reclaim_expired_modifiers) {
        reclaim_expired_modifiers();
    }
}

} // namespace ideam::core