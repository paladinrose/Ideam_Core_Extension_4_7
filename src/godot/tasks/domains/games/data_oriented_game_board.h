#pragma once

/**
 * data_oriented_game_board.h
 * Godot-facing Data-Oriented GameBoard subclass in src/godot/tasks/domains/games/.
 *
 * Extends ideam::godot_ext::GameBoard to seamlessly bridge OOP SceneTree GameBoard
 * nodes (GameAgent, GamePiece, GameplayStyle) with an underlying pure C++26
 * ideam::core::GameBoardDOD partition inside DataOrientedGame.
 *
 * Supports:
 *   - Pure-DOD entity spawning on a board without requiring SceneTree nodes
 *   - Hybrid & OOP entity registration, promotion (DOD slot -> SceneTree Node),
 *     and demotion (SceneTree Node -> pure DOD slot)
 *   - Configurable multi-ring distance tier evaluation (Full OOP proximity,
 *     Hybrid Synced close-range, DOD Rendered mid-range, Pure DOD far-range)
 *   - Selective suppression of redundant OOP virtual game_process() calls for
 *     entities already simulated in the DOD TaskGraph
 *   - Board-scoped TaskGraphDOD node injection for custom Kahn wave composition
 */

#include "../../../../core/tasks/domains/games/game_board_dod.h"
#include "../../../games/game_entities/game_board.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector4i.hpp>

#include <unordered_map>
#include <cstdint>

namespace ideam::godot_ext {

class DataOrientedGame;
class GameEntity;
class GameAgent;
class GamePiece;

class DataOrientedGameBoard : public GameBoard {
    GDCLASS(DataOrientedGameBoard, GameBoard)

protected:
    static void _bind_methods();

private:
    DataOrientedGame* dod_game_                  = nullptr;
    core::GameBoardDOD* board_dod_               = nullptr;

    int board_partition_id_                      = -1;
    int default_simulation_tier_                 = static_cast<int>(core::SIM_TIER_HYBRID_SYNCED);
    bool auto_register_board_entities_           = true;
    bool suppress_oop_process_for_dod_entities_  = true;

    // Optional spatial/radial LOD tier thresholds (for open-world / streaming simulation)
    bool distance_tier_culling_enabled_          = false;
    float full_oop_radius_                       = 15.0f;
    float hybrid_sync_radius_                    = 50.0f;
    float dod_rendered_radius_                   = 200.0f;

    // Fast lookup of Pure-DOD entity 3D positions keyed by DOD entity slot
    std::unordered_map<uint32_t, godot::Vector3> dod_slot_positions_;

    DataOrientedGame* _resolve_dod_game() const;
    void _ensure_board_dod_registered();
    bool _should_run_oop_game_process(const GameEntity* p_entity) const;

public:
    DataOrientedGameBoard();
    ~DataOrientedGameBoard() override;

    // --- Core DOD Backend Accessors (C++) ---
    [[nodiscard]] core::GameBoardDOD* get_board_dod() const noexcept { return board_dod_; }
    [[nodiscard]] DataOrientedGame* get_data_oriented_game() const noexcept { return dod_game_; }
    void bind_to_dod_game(DataOrientedGame* p_dod_game, int p_partition_id = -1);

    // --- Inspector Setters / Getters ---
    void set_board_partition_id(int p_id);
    int get_board_partition_id() const;

    void set_default_simulation_tier(int p_tier);
    int get_default_simulation_tier() const;

    void set_auto_register_board_entities(bool p_auto);
    bool get_auto_register_board_entities() const;

    void set_suppress_oop_process_for_dod_entities(bool p_suppress);
    bool get_suppress_oop_process_for_dod_entities() const;

    void set_distance_tier_culling_enabled(bool p_enabled);
    bool get_distance_tier_culling_enabled() const;

    void set_full_oop_radius(float p_radius);
    float get_full_oop_radius() const;

    void set_hybrid_sync_radius(float p_radius);
    float get_hybrid_sync_radius() const;

    void set_dod_rendered_radius(float p_radius);
    float get_dod_rendered_radius() const;

    // --- Entity Registration, Promotion & Demotion on this Board ---
    /**
     * Registers a SceneTree GameEntity (GameAgent, GamePiece, etc.) and its child
     * properties/actions with this board's DOD partition. Returns the DOD entity slot (>= 0) or -1.
     */
    int register_entity_on_board_dod(GameEntity* p_entity, int p_simulation_tier = -1);

    /**
     * Unregisters a SceneTree GameEntity from this board's DOD partition.
     */
    bool unregister_entity_from_board_dod(GameEntity* p_entity);

    /**
     * Spawns a pure-DOD entity row directly in this board's partition without any Godot Node.
     * Returns the allocated DOD entity slot (>= 0) or -1.
     */
    int spawn_pure_dod_entity(
        int p_archetype_mask = static_cast<int>(core::ENTITY_ARCHETYPE_PIECE),
        int p_parent_entity_slot = -1,
        float p_local_time_scale = 1.0f,
        int64_t p_group_mask = 0
    );

    /**
     * Promotes an existing pure-DOD entity slot on this board to a live SceneTree GameEntity node,
     * immediately pushing the DOD buffer state into the Node and setting its tier to p_target_tier.
     */
    bool promote_dod_entity_to_node(
        int p_entity_slot,
        GameEntity* p_node,
        int p_target_tier = static_cast<int>(core::SIM_TIER_HYBRID_SYNCED)
    );

    /**
     * Demotes a live SceneTree GameEntity node on this board to a pure-DOD (or DOD-rendered) slot,
     * flushing its latest state to the DOD buffers and unbinding its ObjectID so the Node can
     * be safely pooled or freed while simulation continues in DOD memory.
     * Returns the retained DOD entity slot (>= 0) or -1.
     */
    int demote_node_to_pure_dod(
        GameEntity* p_entity,
        int p_target_tier = static_cast<int>(core::SIM_TIER_PURE_DOD)
    );

    bool set_entity_simulation_tier(GameEntity* p_entity, int p_tier);
    bool set_slot_simulation_tier(int p_entity_slot, int p_tier);
    int get_slot_simulation_tier(int p_entity_slot) const;
    void set_all_board_entities_simulation_tier(int p_tier);

    // --- Spatial / Distance Tier Evaluation (Open-World / Streaming Support) ---
    void set_dod_slot_world_position(int p_entity_slot, const godot::Vector3& p_pos);
    godot::Vector3 get_dod_slot_world_position(int p_entity_slot) const;

    /**
     * Evaluates resident entities on this board against full_oop_radius, hybrid_sync_radius,
     * and dod_rendered_radius relative to p_reference_position, updating their simulation tiers.
     * Returns Vector4i(pure_dod_count, dod_rendered_count, hybrid_synced_count, full_oop_count).
     */
    godot::Vector4i evaluate_distance_simulation_tiers(const godot::Vector3& p_reference_position);

    godot::PackedInt32Array get_resident_dod_entity_slots() const;
    int get_board_dod_entity_slot() const;

    // --- Board-Scoped GameplayStyle & TaskGraph Injection ---
    int compile_board_gameplay_style_to_dod();

    /**
     * Injects a BoardPresencePartitionMetadataLogic (G18) node for this board into the active TaskGraphDOD.
     * Returns the created TaskGraph NodeID (or -1 on failure).
     */
    int inject_board_partition_task(
        int p_resolution_mode = static_cast<int>(core::BOARD_PARTITION_DIRECT_OR_ANCESTOR),
        bool p_filter_to_this_board = true,
        int p_priority = 0
    );

    /**
     * Injects a board-scoped GameplayRuleSelectorQueryLogic (G16) + GameplayRuleApplyTransformLogic (G5)
     * pair into the active TaskGraphDOD and returns PackedInt32Array([selector_node_id, apply_node_id]).
     */
    godot::PackedInt32Array inject_board_rule_tasks(
        int p_required_archetype_mask = static_cast<int>(core::ENTITY_ARCHETYPE_ALL),
        int64_t p_required_group_mask = 0,
        int p_priority = 0
    );

    // --- GameBoard / GameEntity Lifecycle Overrides ---
    virtual void enter_game() override;
    virtual void game_start() override;
    virtual void game_pause() override;
    virtual void game_continue() override;
    virtual void game_end() override;
    virtual void exit_game() override;
    virtual void apply_gameplay_style(const godot::Ref<GameplayStyle>& newStyle) override;
    virtual void game_process(double delta) override;
    virtual void game_process_clear() override;
};

} // namespace ideam::godot_ext