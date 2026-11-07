#include "game_board_dod.h"
#include "game_dod.h"

#include <algorithm>

namespace ideam::core {

GameBoardDOD::GameBoardDOD(
    GameDOD* p_owner_game,
    int32_t p_board_partition_id,
    uint32_t p_board_entity_slot,
    float p_time_scale
)
    : owner_game_(p_owner_game),
      board_partition_id_(p_board_partition_id),
      board_entity_slot_(p_board_entity_slot),
      local_time_scale_(p_time_scale),
      enabled_(true) {
    if (owner_game_ && board_entity_slot_ != INVALID_ID) {
        owner_game_->set_entity_local_time_scale(board_entity_slot_, local_time_scale_);
    }
}

void GameBoardDOD::set_local_time_scale(float p_time_scale) noexcept {
    local_time_scale_ = p_time_scale;
    if (owner_game_ && board_entity_slot_ != INVALID_ID) {
        owner_game_->set_entity_local_time_scale(board_entity_slot_, p_time_scale);
    }
}

float GameBoardDOD::get_effective_time_scale() const noexcept {
    if (owner_game_ && board_entity_slot_ != INVALID_ID) {
        return owner_game_->get_entity_effective_time_scale(board_entity_slot_);
    }
    return enabled_ ? local_time_scale_ : 0.0f;
}

void GameBoardDOD::set_enabled(bool p_enabled) noexcept {
    enabled_ = p_enabled;
    if (owner_game_ && board_entity_slot_ != INVALID_ID) {
        EntityHierarchyRowPOD row = owner_game_->get_entity_row(board_entity_slot_);
        if (p_enabled) {
            row.active_flags |= ENTITY_FLAG_ENABLED;
        } else {
            row.active_flags &= ~static_cast<uint32_t>(ENTITY_FLAG_ENABLED);
        }
        owner_game_->set_entity_row(board_entity_slot_, row);
    }
}

bool GameBoardDOD::register_entity_on_board(
    uint32_t p_entity_slot,
    EntitySimulationTier p_tier,
    bool p_reparent_to_board
) {
    if (!owner_game_ || !owner_game_->is_entity_slot_valid(p_entity_slot)) {
        return false;
    }

    EntityHierarchyRowPOD row = owner_game_->get_entity_row(p_entity_slot);
    row.board_id = board_partition_id_;
    row.simulation_tier = static_cast<uint8_t>(p_tier);
    if (p_reparent_to_board && board_entity_slot_ != INVALID_ID && p_entity_slot != board_entity_slot_) {
        if (row.parent_entity_id == INVALID_ID || row.parent_entity_id == owner_game_->get_root_game_entity_slot()) {
            row.parent_entity_id = board_entity_slot_;
        }
    }
    owner_game_->set_entity_row(p_entity_slot, row);

    if (std::find(resident_entities_.begin(), resident_entities_.end(), p_entity_slot) == resident_entities_.end()) {
        resident_entities_.push_back(p_entity_slot);
    }
    return true;
}

bool GameBoardDOD::unregister_entity_from_board(uint32_t p_entity_slot) {
    auto it = std::find(resident_entities_.begin(), resident_entities_.end(), p_entity_slot);
    if (it != resident_entities_.end()) {
        resident_entities_.erase(it);
    }

    if (!owner_game_ || !owner_game_->is_entity_slot_valid(p_entity_slot)) {
        return false;
    }

    EntityHierarchyRowPOD row = owner_game_->get_entity_row(p_entity_slot);
    if (row.board_id == board_partition_id_) {
        row.board_id = -1;
        if (row.parent_entity_id == board_entity_slot_) {
            row.parent_entity_id = owner_game_->get_root_game_entity_slot();
        }
        owner_game_->set_entity_row(p_entity_slot, row);
    }
    return true;
}

bool GameBoardDOD::set_entity_simulation_tier(
    uint32_t p_entity_slot,
    EntitySimulationTier p_tier,
    uint64_t p_godot_object_id
) {
    if (!owner_game_) {
        return false;
    }
    return owner_game_->set_entity_simulation_tier(p_entity_slot, p_tier, p_godot_object_id);
}

void GameBoardDOD::set_all_resident_simulation_tiers(EntitySimulationTier p_tier) {
    if (!owner_game_) return;
    for (uint32_t slot : resident_entities_) {
        if (owner_game_->is_entity_slot_valid(slot)) {
            owner_game_->set_entity_simulation_tier(slot, p_tier);
        }
    }
}

uint32_t GameBoardDOD::register_board_rule(
    const CompiledGameplayRulePOD& p_rule,
    int32_t p_priority,
    uint8_t p_specificity,
    bool p_important
) {
    if (!owner_game_) {
        return INVALID_ID;
    }
    uint32_t slot = owner_game_->allocate_compiled_rule(p_rule, p_priority, p_specificity, p_important);
    if (slot != INVALID_ID) {
        board_rule_slots_.push_back(slot);
    }
    return slot;
}

void GameBoardDOD::clear_board_rules() {
    if (owner_game_) {
        for (uint32_t slot : board_rule_slots_) {
            owner_game_->free_compiled_rule(slot);
        }
    }
    board_rule_slots_.clear();
}

NodeID GameBoardDOD::add_board_partition_node(
    TaskGraphDOD& r_graph,
    BoardPartitionResolutionMode p_resolution_mode,
    bool p_filter_to_this_board,
    const GamesNodeExecutionConfig& p_config
) const {
    if (!owner_game_) return INVALID_ID;

    BoardPresencePartitionMetadataLogic<int32_t> logic{};
    const GamesBufferManifest& m = owner_game_->get_buffer_manifest();
    logic.target_buffer_id = m.entity_board_id_buffer_id;
    logic.board_id_buffer_id = m.entity_board_id_buffer_id;
    logic.parent_entity_id_buffer_id = m.entity_parent_id_buffer_id;
    logic.archetype_mask_buffer_id = m.entity_archetype_mask_buffer_id;
    logic.active_flags_buffer_id = m.entity_active_flags_buffer_id;
    logic.output_board_id_buffer_id = m.entity_board_id_buffer_id;
    logic.filter_board_id = p_filter_to_this_board ? board_partition_id_ : -1;
    logic.resolution_mode = static_cast<uint8_t>(p_resolution_mode);
    logic.require_enabled = true;
    logic.write_hop_depth_to_lod = true;

    return owner_game_->add_board_partition_node(r_graph, logic, p_config);
}

NodeID GameBoardDOD::add_board_rule_selector_node(
    TaskGraphDOD& r_graph,
    uint32_t p_required_archetype_mask,
    uint64_t p_required_group_mask,
    uint32_t p_required_property_name_id,
    uint8_t p_pseudo_state,
    const GamesNodeExecutionConfig& p_config
) const {
    if (!owner_game_) return INVALID_ID;

    GameplayRuleSelectorQueryLogic<int32_t> logic{};
    const GamesBufferManifest& m = owner_game_->get_buffer_manifest();
    logic.target_buffer_id = m.prop_modded_value_buffer_id;
    logic.archetype_mask_buffer_id = m.entity_archetype_mask_buffer_id;
    logic.group_mask_buffer_id = m.entity_group_mask_buffer_id;
    logic.parent_entity_id_buffer_id = m.entity_parent_id_buffer_id;
    logic.entity_active_flags_buffer_id = m.entity_active_flags_buffer_id;
    logic.property_flags_buffer_id = m.prop_flags_buffer_id;
    logic.property_name_id_buffer_id = m.prop_name_id_buffer_id;
    logic.scope_root_entity_id = board_entity_slot_;
    logic.required_archetype_mask = p_required_archetype_mask;
    logic.required_group_mask = p_required_group_mask;
    logic.required_property_name_id = p_required_property_name_id;
    logic.rule_scope = (board_entity_slot_ != INVALID_ID) ? RULE_SCOPE_DESCENDANTS : RULE_SCOPE_GLOBAL;
    logic.pseudo_state = p_pseudo_state;

    return owner_game_->add_rule_selector_node(r_graph, logic, QueryOp::CULL, p_config);
}

NodeID GameBoardDOD::add_board_rule_apply_node(
    TaskGraphDOD& r_graph,
    const CompiledGameplayRulePOD& p_rule_override,
    bool p_use_compiled_rule_buffer,
    const GamesNodeExecutionConfig& p_config
) const {
    if (!owner_game_) return INVALID_ID;

    GameplayRuleApplyTransformLogic<int32_t> logic{};
    const GamesBufferManifest& m = owner_game_->get_buffer_manifest();
    logic.target_buffer_id = m.prop_modded_value_buffer_id;
    logic.source_buffer_id = m.prop_base_value_buffer_id;
    logic.property_flags_buffer_id = m.prop_flags_buffer_id;
    logic.archetype_mask_buffer_id = m.entity_archetype_mask_buffer_id;
    logic.group_mask_buffer_id = m.entity_group_mask_buffer_id;
    logic.compiled_rule_buffer_id = p_use_compiled_rule_buffer ? m.compiled_rule_pod_buffer_id : INVALID_ID;
    logic.required_archetype_mask = p_rule_override.required_archetype_mask;
    logic.required_group_mask_lo = static_cast<uint32_t>(p_rule_override.required_group_mask & 0xFFFFFFFFULL);
    logic.rule_value = p_rule_override.value;
    logic.clamp_min = p_rule_override.clamp_min;
    logic.clamp_max = p_rule_override.clamp_max;
    logic.operation = p_rule_override.operation;
    logic.cascade_layer = p_rule_override.cascade_layer;
    logic.target_channel = p_rule_override.target_channel;

    return owner_game_->add_rule_apply_node(r_graph, logic, p_config);
}

} // namespace ideam::core