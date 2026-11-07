#include "data_oriented_game_board.h"
#include "data_oriented_game.h"

#include "../../../games/game_entities/game_agent.h"
#include "../../../games/game_entities/game_piece.h"
#include "../../../games/gameplay/gameplay_style.h"
#include "../../../games/gameplay/gameplay_rule.h"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/node2d.hpp>

namespace ideam::godot_ext {

void DataOrientedGameBoard::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo(
        "entity_simulation_tier_changed",
        godot::PropertyInfo(godot::Variant::INT, "entity_slot"),
        godot::PropertyInfo(godot::Variant::INT, "old_tier"),
        godot::PropertyInfo(godot::Variant::INT, "new_tier")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "dod_entity_promoted_to_node",
        godot::PropertyInfo(godot::Variant::INT, "entity_slot"),
        godot::PropertyInfo(godot::Variant::OBJECT, "entity_node", godot::PROPERTY_HINT_NODE_TYPE, "GameEntity")
    ));
    ADD_SIGNAL(godot::MethodInfo(
        "node_demoted_to_dod_entity",
        godot::PropertyInfo(godot::Variant::INT, "entity_slot"),
        godot::PropertyInfo(godot::Variant::OBJECT, "entity_node", godot::PROPERTY_HINT_NODE_TYPE, "GameEntity")
    ));

    // Inspector Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_board_partition_id", "id"), &DataOrientedGameBoard::set_board_partition_id);
    godot::ClassDB::bind_method(godot::D_METHOD("get_board_partition_id"), &DataOrientedGameBoard::get_board_partition_id);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::INT, "board_partition_id"), "set_board_partition_id", "get_board_partition_id");

    godot::ClassDB::bind_method(godot::D_METHOD("set_default_simulation_tier", "tier"), &DataOrientedGameBoard::set_default_simulation_tier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_simulation_tier"), &DataOrientedGameBoard::get_default_simulation_tier);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "default_simulation_tier", godot::PROPERTY_HINT_ENUM, "Pure DOD,DOD Rendered,Hybrid Synced,Full OOP"),
        "set_default_simulation_tier",
        "get_default_simulation_tier"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_auto_register_board_entities", "auto_register"), &DataOrientedGameBoard::set_auto_register_board_entities);
    godot::ClassDB::bind_method(godot::D_METHOD("get_auto_register_board_entities"), &DataOrientedGameBoard::get_auto_register_board_entities);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "auto_register_board_entities"), "set_auto_register_board_entities", "get_auto_register_board_entities");

    godot::ClassDB::bind_method(godot::D_METHOD("set_suppress_oop_process_for_dod_entities", "suppress"), &DataOrientedGameBoard::set_suppress_oop_process_for_dod_entities);
    godot::ClassDB::bind_method(godot::D_METHOD("get_suppress_oop_process_for_dod_entities"), &DataOrientedGameBoard::get_suppress_oop_process_for_dod_entities);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "suppress_oop_process_for_dod_entities"), "set_suppress_oop_process_for_dod_entities", "get_suppress_oop_process_for_dod_entities");

    godot::ClassDB::bind_method(godot::D_METHOD("set_distance_tier_culling_enabled", "enabled"), &DataOrientedGameBoard::set_distance_tier_culling_enabled);
    godot::ClassDB::bind_method(godot::D_METHOD("get_distance_tier_culling_enabled"), &DataOrientedGameBoard::get_distance_tier_culling_enabled);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "distance_tier_culling_enabled"), "set_distance_tier_culling_enabled", "get_distance_tier_culling_enabled");

    godot::ClassDB::bind_method(godot::D_METHOD("set_full_oop_radius", "radius"), &DataOrientedGameBoard::set_full_oop_radius);
    godot::ClassDB::bind_method(godot::D_METHOD("get_full_oop_radius"), &DataOrientedGameBoard::get_full_oop_radius);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "full_oop_radius"), "set_full_oop_radius", "get_full_oop_radius");

    godot::ClassDB::bind_method(godot::D_METHOD("set_hybrid_sync_radius", "radius"), &DataOrientedGameBoard::set_hybrid_sync_radius);
    godot::ClassDB::bind_method(godot::D_METHOD("get_hybrid_sync_radius"), &DataOrientedGameBoard::get_hybrid_sync_radius);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "hybrid_sync_radius"), "set_hybrid_sync_radius", "get_hybrid_sync_radius");

    godot::ClassDB::bind_method(godot::D_METHOD("set_dod_rendered_radius", "radius"), &DataOrientedGameBoard::set_dod_rendered_radius);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dod_rendered_radius"), &DataOrientedGameBoard::get_dod_rendered_radius);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "dod_rendered_radius"), "set_dod_rendered_radius", "get_dod_rendered_radius");

    // Entity Registration, Promotion & Demotion Methods
    godot::ClassDB::bind_method(godot::D_METHOD("register_entity_on_board_dod", "entity", "simulation_tier"), &DataOrientedGameBoard::register_entity_on_board_dod, DEFVAL(-1));
    godot::ClassDB::bind_method(godot::D_METHOD("unregister_entity_from_board_dod", "entity"), &DataOrientedGameBoard::unregister_entity_from_board_dod);
    godot::ClassDB::bind_method(
        godot::D_METHOD("spawn_pure_dod_entity", "archetype_mask", "parent_entity_slot", "local_time_scale", "group_mask"),
        &DataOrientedGameBoard::spawn_pure_dod_entity,
        DEFVAL(static_cast<int>(core::ENTITY_ARCHETYPE_PIECE)),
        DEFVAL(-1),
        DEFVAL(1.0f),
        DEFVAL(0)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("promote_dod_entity_to_node", "entity_slot", "node", "target_tier"),
        &DataOrientedGameBoard::promote_dod_entity_to_node,
        DEFVAL(static_cast<int>(core::SIM_TIER_HYBRID_SYNCED))
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("demote_node_to_pure_dod", "entity", "target_tier"),
        &DataOrientedGameBoard::demote_node_to_pure_dod,
        DEFVAL(static_cast<int>(core::SIM_TIER_PURE_DOD))
    );
    godot::ClassDB::bind_method(godot::D_METHOD("set_entity_simulation_tier", "entity", "tier"), &DataOrientedGameBoard::set_entity_simulation_tier);
    godot::ClassDB::bind_method(godot::D_METHOD("set_slot_simulation_tier", "entity_slot", "tier"), &DataOrientedGameBoard::set_slot_simulation_tier);
    godot::ClassDB::bind_method(godot::D_METHOD("get_slot_simulation_tier", "entity_slot"), &DataOrientedGameBoard::get_slot_simulation_tier);
    godot::ClassDB::bind_method(godot::D_METHOD("set_all_board_entities_simulation_tier", "tier"), &DataOrientedGameBoard::set_all_board_entities_simulation_tier);

    // Distance & Residency Methods
    godot::ClassDB::bind_method(godot::D_METHOD("set_dod_slot_world_position", "entity_slot", "position"), &DataOrientedGameBoard::set_dod_slot_world_position);
    godot::ClassDB::bind_method(godot::D_METHOD("get_dod_slot_world_position", "entity_slot"), &DataOrientedGameBoard::get_dod_slot_world_position);
    godot::ClassDB::bind_method(godot::D_METHOD("evaluate_distance_simulation_tiers", "reference_position"), &DataOrientedGameBoard::evaluate_distance_simulation_tiers);
    godot::ClassDB::bind_method(godot::D_METHOD("get_resident_dod_entity_slots"), &DataOrientedGameBoard::get_resident_dod_entity_slots);
    godot::ClassDB::bind_method(godot::D_METHOD("get_board_dod_entity_slot"), &DataOrientedGameBoard::get_board_dod_entity_slot);

    // Board TaskGraph & Style Compilation
    godot::ClassDB::bind_method(godot::D_METHOD("compile_board_gameplay_style_to_dod"), &DataOrientedGameBoard::compile_board_gameplay_style_to_dod);
    godot::ClassDB::bind_method(
        godot::D_METHOD("inject_board_partition_task", "resolution_mode", "filter_to_this_board", "priority"),
        &DataOrientedGameBoard::inject_board_partition_task,
        DEFVAL(static_cast<int>(core::BOARD_PARTITION_DIRECT_OR_ANCESTOR)),
        DEFVAL(true),
        DEFVAL(0)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("inject_board_rule_tasks", "required_archetype_mask", "required_group_mask", "priority"),
        &DataOrientedGameBoard::inject_board_rule_tasks,
        DEFVAL(static_cast<int>(core::ENTITY_ARCHETYPE_ALL)),
        DEFVAL(0),
        DEFVAL(0)
    );
}

DataOrientedGameBoard::DataOrientedGameBoard() = default;
DataOrientedGameBoard::~DataOrientedGameBoard() = default;

DataOrientedGame* DataOrientedGameBoard::_resolve_dod_game() const {
    if (dod_game_) return dod_game_;
    if (Game* g = get_game()) {
        return godot::Object::cast_to<DataOrientedGame>(g);
    }
    godot::Node* p = get_parent();
    while (p) {
        if (DataOrientedGame* dg = godot::Object::cast_to<DataOrientedGame>(p)) {
            return dg;
        }
        p = p->get_parent();
    }
    return nullptr;
}

void DataOrientedGameBoard::_ensure_board_dod_registered() {
    if (board_dod_) return;
    dod_game_ = _resolve_dod_game();
    if (!dod_game_) return;

    core::GameDOD* core_game = dod_game_->get_game_dod();
    if (!core_game) return;

    board_dod_ = core_game->create_board_dod(board_partition_id_, get_time_scale());
    if (board_dod_) {
        board_partition_id_ = board_dod_->get_board_partition_id();
        board_dod_->set_enabled(get_enabled() && !get_entity_is_paused());
    }
}

void DataOrientedGameBoard::bind_to_dod_game(DataOrientedGame* p_dod_game, int p_partition_id) {
    dod_game_ = p_dod_game;
    if (p_partition_id >= 0) {
        board_partition_id_ = p_partition_id;
    }
    board_dod_ = nullptr;
    _ensure_board_dod_registered();
}

void DataOrientedGameBoard::set_board_partition_id(int p_id) {
    board_partition_id_ = p_id;
}

int DataOrientedGameBoard::get_board_partition_id() const {
    return board_dod_ ? board_dod_->get_board_partition_id() : board_partition_id_;
}

void DataOrientedGameBoard::set_default_simulation_tier(int p_tier) {
    default_simulation_tier_ = std::clamp(p_tier, 0, 3);
}

int DataOrientedGameBoard::get_default_simulation_tier() const {
    return default_simulation_tier_;
}

void DataOrientedGameBoard::set_auto_register_board_entities(bool p_auto) {
    auto_register_board_entities_ = p_auto;
}

bool DataOrientedGameBoard::get_auto_register_board_entities() const {
    return auto_register_board_entities_;
}

void DataOrientedGameBoard::set_suppress_oop_process_for_dod_entities(bool p_suppress) {
    suppress_oop_process_for_dod_entities_ = p_suppress;
}

bool DataOrientedGameBoard::get_suppress_oop_process_for_dod_entities() const {
    return suppress_oop_process_for_dod_entities_;
}

void DataOrientedGameBoard::set_distance_tier_culling_enabled(bool p_enabled) {
    distance_tier_culling_enabled_ = p_enabled;
}

bool DataOrientedGameBoard::get_distance_tier_culling_enabled() const {
    return distance_tier_culling_enabled_;
}

void DataOrientedGameBoard::set_full_oop_radius(float p_radius) {
    full_oop_radius_ = std::max(0.0f, p_radius);
}

float DataOrientedGameBoard::get_full_oop_radius() const {
    return full_oop_radius_;
}

void DataOrientedGameBoard::set_hybrid_sync_radius(float p_radius) {
    hybrid_sync_radius_ = std::max(full_oop_radius_, p_radius);
}

float DataOrientedGameBoard::get_hybrid_sync_radius() const {
    return hybrid_sync_radius_;
}

void DataOrientedGameBoard::set_dod_rendered_radius(float p_radius) {
    dod_rendered_radius_ = std::max(hybrid_sync_radius_, p_radius);
}

float DataOrientedGameBoard::get_dod_rendered_radius() const {
    return dod_rendered_radius_;
}

// ============================================================================
// Entity Registration, Promotion & Demotion on this Board
// ============================================================================

int DataOrientedGameBoard::register_entity_on_board_dod(GameEntity* p_entity, int p_simulation_tier) {
    if (!p_entity) return -1;
    _ensure_board_dod_registered();
    if (!dod_game_ || !board_dod_) return -1;

    const int tier = (p_simulation_tier >= 0) ? p_simulation_tier : default_simulation_tier_;
    const int slot = dod_game_->register_game_entity(p_entity, tier, board_dod_->get_board_partition_id());
    if (slot >= 0) {
        board_dod_->register_entity_on_board(
            static_cast<uint32_t>(slot),
            static_cast<core::EntitySimulationTier>(tier),
            true
        );
    }
    return slot;
}

bool DataOrientedGameBoard::unregister_entity_from_board_dod(GameEntity* p_entity) {
    if (!p_entity || !dod_game_) return false;
    const int slot = dod_game_->get_entity_dod_slot(p_entity);
    if (slot >= 0 && board_dod_) {
        board_dod_->unregister_entity_from_board(static_cast<uint32_t>(slot));
        dod_slot_positions_.erase(static_cast<uint32_t>(slot));
    }
    return dod_game_->unregister_game_entity(p_entity);
}

int DataOrientedGameBoard::spawn_pure_dod_entity(
    int p_archetype_mask,
    int p_parent_entity_slot,
    float p_local_time_scale,
    int64_t p_group_mask
) {
    _ensure_board_dod_registered();
    if (!dod_game_ || !board_dod_) return -1;

    const int parent_slot = (p_parent_entity_slot >= 0)
        ? p_parent_entity_slot
        : static_cast<int>(board_dod_->get_board_entity_slot());

    const int slot = dod_game_->create_pure_dod_entity(
        p_archetype_mask,
        parent_slot,
        board_dod_->get_board_partition_id(),
        p_local_time_scale,
        p_group_mask
    );
    if (slot >= 0) {
        board_dod_->register_entity_on_board(
            static_cast<uint32_t>(slot),
            core::SIM_TIER_PURE_DOD,
            false
        );
    }
    return slot;
}

bool DataOrientedGameBoard::promote_dod_entity_to_node(
    int p_entity_slot,
    GameEntity* p_node,
    int p_target_tier
) {
    if (p_entity_slot < 0 || !p_node) return false;
    _ensure_board_dod_registered();
    if (!dod_game_ || !board_dod_) return false;

    const int old_tier = get_slot_simulation_tier(p_entity_slot);
    if (!dod_game_->promote_dod_slot_to_node(p_entity_slot, p_node, p_target_tier)) {
        return false;
    }

    board_dod_->register_entity_on_board(
        static_cast<uint32_t>(p_entity_slot),
        static_cast<core::EntitySimulationTier>(p_target_tier),
        false
    );

    if (GameAgent* agent = godot::Object::cast_to<GameAgent>(p_node)) {
        if (!has_game_agent(agent)) add_game_agent(agent);
    } else if (GamePiece* piece = godot::Object::cast_to<GamePiece>(p_node)) {
        if (!has_game_piece(piece)) add_game_piece(piece);
    }

    emit_signal("dod_entity_promoted_to_node", p_entity_slot, p_node);
    if (old_tier != p_target_tier) {
        emit_signal("entity_simulation_tier_changed", p_entity_slot, old_tier, p_target_tier);
    }
    return true;
}

int DataOrientedGameBoard::demote_node_to_pure_dod(GameEntity* p_entity, int p_target_tier) {
    if (!p_entity) return -1;
    _ensure_board_dod_registered();
    if (!dod_game_ || !board_dod_) return -1;

    const int slot = dod_game_->get_entity_dod_slot(p_entity);
    const int old_tier = (slot >= 0) ? get_slot_simulation_tier(slot) : static_cast<int>(core::SIM_TIER_FULL_OOP);

    // Capture world position before unbinding if it is a spatial Node3D or Node2D
    godot::Vector3 cached_pos{0.0f, 0.0f, 0.0f};
    if (godot::Node* root = p_entity->get_root_node() ? p_entity->get_root_node() : p_entity) {
        if (const auto* n3d = godot::Object::cast_to<godot::Node3D>(root)) {
            cached_pos = n3d->get_global_position();
        } else if (const auto* n2d = godot::Object::cast_to<godot::Node2D>(root)) {
            const godot::Vector2 p2 = n2d->get_global_position();
            cached_pos = godot::Vector3(p2.x, p2.y, 0.0f);
        }
    }

    const int retained_slot = dod_game_->demote_node_to_dod_slot(p_entity, p_target_tier);
    if (retained_slot >= 0) {
        dod_slot_positions_[static_cast<uint32_t>(retained_slot)] = cached_pos;
        board_dod_->set_entity_simulation_tier(
            static_cast<uint32_t>(retained_slot),
            static_cast<core::EntitySimulationTier>(p_target_tier),
            0ULL
        );
        emit_signal("node_demoted_to_dod_entity", retained_slot, p_entity);
        if (old_tier != p_target_tier) {
            emit_signal("entity_simulation_tier_changed", retained_slot, old_tier, p_target_tier);
        }
    }
    return retained_slot;
}

bool DataOrientedGameBoard::set_entity_simulation_tier(GameEntity* p_entity, int p_tier) {
    if (!p_entity) return false;
    _ensure_board_dod_registered();
    if (!dod_game_) return false;

    const int slot = dod_game_->get_entity_dod_slot(p_entity);
    if (slot < 0) {
        return register_entity_on_board_dod(p_entity, p_tier) >= 0;
    }
    return set_slot_simulation_tier(slot, p_tier);
}

bool DataOrientedGameBoard::set_slot_simulation_tier(int p_entity_slot, int p_tier) {
    if (p_entity_slot < 0) return false;
    _ensure_board_dod_registered();
    if (!dod_game_) return false;

    const int clamped_tier = std::clamp(p_tier, 0, 3);
    const int old_tier = get_slot_simulation_tier(p_entity_slot);
    if (!dod_game_->set_slot_simulation_tier(p_entity_slot, clamped_tier)) {
        return false;
    }
    if (old_tier != clamped_tier) {
        emit_signal("entity_simulation_tier_changed", p_entity_slot, old_tier, clamped_tier);
    }
    return true;
}

int DataOrientedGameBoard::get_slot_simulation_tier(int p_entity_slot) const {
    if (!dod_game_ || p_entity_slot < 0) return default_simulation_tier_;
    return dod_game_->get_slot_simulation_tier(p_entity_slot);
}

void DataOrientedGameBoard::set_all_board_entities_simulation_tier(int p_tier) {
    _ensure_board_dod_registered();
    if (!board_dod_) return;
    const auto clamped = static_cast<core::EntitySimulationTier>(std::clamp(p_tier, 0, 3));
    board_dod_->set_all_resident_simulation_tiers(clamped);
}

// ============================================================================
// Spatial / Distance Tier Evaluation
// ============================================================================

void DataOrientedGameBoard::set_dod_slot_world_position(int p_entity_slot, const godot::Vector3& p_pos) {
    if (p_entity_slot < 0) return;
    dod_slot_positions_[static_cast<uint32_t>(p_entity_slot)] = p_pos;
}

godot::Vector3 DataOrientedGameBoard::get_dod_slot_world_position(int p_entity_slot) const {
    if (p_entity_slot < 0) return godot::Vector3();
    const uint32_t u_slot = static_cast<uint32_t>(p_entity_slot);

    if (dod_game_) {
        if (GameEntity* ent = dod_game_->get_entity_for_dod_slot(p_entity_slot)) {
            godot::Node* spatial_node = ent->get_root_node() ? ent->get_root_node() : ent;
            if (const auto* n3d = godot::Object::cast_to<godot::Node3D>(spatial_node)) {
                return n3d->get_global_position();
            }
            if (const auto* n2d = godot::Object::cast_to<godot::Node2D>(spatial_node)) {
                const godot::Vector2 p2 = n2d->get_global_position();
                return godot::Vector3(p2.x, p2.y, 0.0f);
            }
        }
    }

    auto it = dod_slot_positions_.find(u_slot);
    if (it != dod_slot_positions_.end()) {
        return it->second;
    }
    return godot::Vector3();
}

godot::Vector4i DataOrientedGameBoard::evaluate_distance_simulation_tiers(const godot::Vector3& p_reference_position) {
    godot::Vector4i counts(0, 0, 0, 0);
    _ensure_board_dod_registered();
    if (!board_dod_ || !dod_game_) return counts;

    const float oop_sq    = full_oop_radius_ * full_oop_radius_;
    const float hybrid_sq = hybrid_sync_radius_ * hybrid_sync_radius_;
    const float render_sq = dod_rendered_radius_ * dod_rendered_radius_;

    for (uint32_t slot : board_dod_->get_resident_entities()) {
        const godot::Vector3 pos = get_dod_slot_world_position(static_cast<int>(slot));
        const godot::Vector3 diff = pos - p_reference_position;
        const float dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        int target_tier = static_cast<int>(core::SIM_TIER_PURE_DOD);
        if (dist_sq <= oop_sq) {
            target_tier = static_cast<int>(core::SIM_TIER_FULL_OOP);
            counts.w += 1;
        } else if (dist_sq <= hybrid_sq) {
            target_tier = static_cast<int>(core::SIM_TIER_HYBRID_SYNCED);
            counts.z += 1;
        } else if (dist_sq <= render_sq) {
            target_tier = static_cast<int>(core::SIM_TIER_DOD_RENDERED);
            counts.y += 1;
        } else {
            counts.x += 1;
        }

        set_slot_simulation_tier(static_cast<int>(slot), target_tier);
    }
    return counts;
}

godot::PackedInt32Array DataOrientedGameBoard::get_resident_dod_entity_slots() const {
    godot::PackedInt32Array out;
    if (!board_dod_) return out;
    const auto span = board_dod_->get_resident_entities();
    out.resize(static_cast<int64_t>(span.size()));
    for (size_t i = 0; i < span.size(); ++i) {
        out.set(static_cast<int64_t>(i), static_cast<int32_t>(span[i]));
    }
    return out;
}

int DataOrientedGameBoard::get_board_dod_entity_slot() const {
    if (!board_dod_ || board_dod_->get_board_entity_slot() == core::INVALID_ID) {
        return -1;
    }
    return static_cast<int>(board_dod_->get_board_entity_slot());
}

// ============================================================================
// Board-Scoped GameplayStyle & TaskGraph Injection
// ============================================================================

int DataOrientedGameBoard::compile_board_gameplay_style_to_dod() {
    _ensure_board_dod_registered();
    if (!dod_game_ || !board_dod_) return 0;

    board_dod_->clear_board_rules();
    godot::Ref<GameplayStyle> style = get_resolved_gameplay_style();
    if (style.is_null()) {
        style = get_gameplay_style();
    }
    if (style.is_null()) return 0;

    godot::PackedInt32Array slots = dod_game_->compile_gameplay_style_to_dod(
        style,
        static_cast<int>(board_dod_->get_board_entity_slot())
    );
    return static_cast<int>(slots.size());
}

int DataOrientedGameBoard::inject_board_partition_task(
    int p_resolution_mode,
    bool p_filter_to_this_board,
    int p_priority
) {
    _ensure_board_dod_registered();
    if (!dod_game_ || !board_dod_) return -1;

    core::TaskGraphDOD* graph = dod_game_->get_active_task_graph_dod();
    if (!graph) return -1;

    core::GamesNodeExecutionConfig cfg{};
    cfg.execution_priority = p_priority;
    const core::NodeID node = board_dod_->add_board_partition_node(
        *graph,
        static_cast<core::BoardPartitionResolutionMode>(std::clamp(p_resolution_mode, 0, 2)),
        p_filter_to_this_board,
        cfg
    );
    return (node == core::INVALID_ID) ? -1 : static_cast<int>(node);
}

godot::PackedInt32Array DataOrientedGameBoard::inject_board_rule_tasks(
    int p_required_archetype_mask,
    int64_t p_required_group_mask,
    int p_priority
) {
    godot::PackedInt32Array result;
    _ensure_board_dod_registered();
    if (!dod_game_ || !board_dod_) return result;

    core::TaskGraphDOD* graph = dod_game_->get_active_task_graph_dod();
    if (!graph) return result;

    core::GamesNodeExecutionConfig cfg{};
    cfg.execution_priority = p_priority;

    const core::NodeID sel_node = board_dod_->add_board_rule_selector_node(
        *graph,
        static_cast<uint32_t>(p_required_archetype_mask),
        static_cast<uint64_t>(p_required_group_mask),
        0u,
        core::PSEUDO_STATE_NONE,
        cfg
    );
    const core::NodeID apply_node = board_dod_->add_board_rule_apply_node(
        *graph,
        core::CompiledGameplayRulePOD{},
        true,
        cfg
    );

    if (sel_node != core::INVALID_ID && apply_node != core::INVALID_ID) {
        graph->connect_nodes(sel_node, 0, apply_node, 0);
        result.append(static_cast<int32_t>(sel_node));
        result.append(static_cast<int32_t>(apply_node));
    }
    return result;
}

// ============================================================================
// GameBoard / GameEntity Lifecycle Overrides
// ============================================================================

bool DataOrientedGameBoard::_should_run_oop_game_process(const GameEntity* p_entity) const {
    if (!p_entity) return false;
    if (!suppress_oop_process_for_dod_entities_ || !dod_game_) return true;

    const int slot = dod_game_->get_entity_dod_slot(p_entity);
    if (slot < 0) return true;

    const int tier = dod_game_->get_slot_simulation_tier(slot);
    return tier == static_cast<int>(core::SIM_TIER_FULL_OOP);
}

void DataOrientedGameBoard::enter_game() {
    _ensure_board_dod_registered();

    if (auto_register_board_entities_ && dod_game_) {
        godot::TypedArray<GameAgent> agents = get_game_agents();
        for (int i = 0; i < agents.size(); ++i) {
            if (GameAgent* agent = godot::Object::cast_to<GameAgent>(agents[i])) {
                register_entity_on_board_dod(agent, default_simulation_tier_);
            }
        }
        godot::TypedArray<GamePiece> pieces = get_game_pieces();
        for (int i = 0; i < pieces.size(); ++i) {
            if (GamePiece* piece = godot::Object::cast_to<GamePiece>(pieces[i])) {
                register_entity_on_board_dod(piece, default_simulation_tier_);
            }
        }
    }

    GameBoard::enter_game();
}

void DataOrientedGameBoard::game_start() {
    _ensure_board_dod_registered();
    if (board_dod_) {
        board_dod_->set_enabled(true);
        board_dod_->set_local_time_scale(get_time_scale());
    }
    GameBoard::game_start();
}

void DataOrientedGameBoard::game_pause() {
    GameBoard::game_pause();
    if (board_dod_) {
        board_dod_->set_enabled(false);
    }
}

void DataOrientedGameBoard::game_continue() {
    GameBoard::game_continue();
    if (board_dod_) {
        board_dod_->set_enabled(get_enabled());
    }
}

void DataOrientedGameBoard::game_end() {
    if (board_dod_) {
        board_dod_->set_enabled(false);
    }
    GameBoard::game_end();
}

void DataOrientedGameBoard::exit_game() {
    if (board_dod_) {
        board_dod_->clear_board_rules();
    }
    GameBoard::exit_game();
}

void DataOrientedGameBoard::apply_gameplay_style(const godot::Ref<GameplayStyle>& newStyle) {
    GameBoard::apply_gameplay_style(newStyle);
    compile_board_gameplay_style_to_dod();
}

void DataOrientedGameBoard::game_process(double delta) {
    if (board_dod_) {
        board_dod_->set_local_time_scale(get_time_scale());
    }

    if (!suppress_oop_process_for_dod_entities_ || !dod_game_) {
        GameBoard::game_process(delta);
        return;
    }

    const double board_delta = delta * get_time_scale();
    godot::TypedArray<GameAgent> agents = get_game_agents();
    for (int i = 0; i < agents.size(); ++i) {
        if (GameAgent* agent = godot::Object::cast_to<GameAgent>(agents[i])) {
            if (_should_run_oop_game_process(agent)) {
                agent->game_process(board_delta);
            }
        }
    }

    godot::TypedArray<GamePiece> pieces = get_game_pieces();
    for (int i = 0; i < pieces.size(); ++i) {
        if (GamePiece* piece = godot::Object::cast_to<GamePiece>(pieces[i])) {
            if (_should_run_oop_game_process(piece)) {
                piece->game_process(board_delta);
            }
        }
    }
}

void DataOrientedGameBoard::game_process_clear() {
    GameBoard::game_process_clear();
}

} // namespace ideam::godot_ext