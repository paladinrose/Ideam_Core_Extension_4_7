#pragma once

#include "gameplay_resource.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameEntity;
class GameProperty;

/**
 * DOD NOTE: Extracting RuleScope and RuleOp at namespace scope allows them to be
 * packed directly into dense Structure of Arrays (SoA) columns or Task manifests
 * without pulling in the full GameplayRule Resource definition.
 *
 * In the future DOD compilation pipeline (Phase 2+):
 * - RuleScope and Selector fields compile into QueryTask nodes (ArchetypeQueryLogic,
 *   BitmaskQueryLogic, HierarchicalBridgeQueryLogic) producing a MemoryBufferSelectionPOD.
 * - RuleOp and Declaration fields compile into TransformTask nodes
 *   (ValueAccumulationTransformLogic, BoundaryConstraintTransformLogic) operating
 *   on contiguous BaseValue and ComputedValue columns in a MemoryBufferPOD.
 */
enum RuleScope : int32_t {
    SELF = 0,        // Applies strictly to the owner holding the GameplayStyle (:host)
    DESCENDANTS = 1, // Cascades down to owned/child GameEntities (*)
    ALL = 2          // Applies to both the owner and all descendant GameEntities
};

enum RuleOp : int32_t {
    SET = 0,         // Overwrites the target property or GameProperty base value
    ADD = 1,         // Adds to the current property or GameProperty base value
    MULTIPLY = 2,    // Scales the current property or GameProperty base value
    MODIFIER_ADD = 3 // Injects a GamePropertyModifier onto a matching GameProperty
};

/**
 * DOD NOTE: GameplayRule represents a single CSS-like cascading rule (Selector + Declaration)
 * targeting the GameEntity hierarchy (GameBoard, GameAgent, GamePiece, GameAgentAction,
 * GameInteraction) and their child GameProperty nodes.
 *
 * For Phase 1, GameplayRule is implemented as a GameplayResource (AoS) so rules can be
 * authored directly in the standard Godot Inspector and cascaded via GameplayStyle.
 * Using StringName for selector keys ensures O(1) pointer/hash equality checks during
 * tree traversal while keeping the door open for future DOD table flattening.
 */
class GameplayRule : public GameplayResource {
    GDCLASS(GameplayRule, GameplayResource)

protected:
    static void _bind_methods();

private:
    // --- Scope (The Address Routing) ---
    RuleScope scope = RuleScope::SELF;

    // --- Selector (Who & Where) ---
    // Matches GameEntity class or subclass (e.g., "GameAgent", "GamePiece", or "" for any GameEntity).
    godot::StringName target_class;

    // Matches GameEntity::title, Godot Node group, or GameAgentAction::competes_with_groups ("" for any).
    godot::StringName target_title_or_group;

    // Matches a property on the GameEntity (e.g., "time_scale", "priority") or a child GameProperty title (e.g., "Health").
    godot::StringName target_property;

    // --- Declaration (The Payload) ---
    RuleOp operation = RuleOp::SET;
    godot::Variant value;
    int priority = 0;
    bool is_important = false;

    // Internal helper to check if a GameEntity exposes a property in ClassDB/Object property list.
    bool _entity_has_direct_property(const GameEntity *p_entity, const godot::StringName &p_property) const;

public:
    GameplayRule();
    ~GameplayRule();

    // --- Scope Setters / Getters ---
    void set_scope(RuleScope p_scope);
    RuleScope get_scope() const;

    bool applies_to_self() const;
    bool applies_to_descendants() const;

    // --- Selector Setters / Getters ---
    void set_target_class(const godot::StringName &p_class);
    godot::StringName get_target_class() const;

    void set_target_title_or_group(const godot::StringName &p_title_or_group);
    godot::StringName get_target_title_or_group() const;

    void set_target_property(const godot::StringName &p_property);
    godot::StringName get_target_property() const;

    // --- Declaration Setters / Getters ---
    void set_operation(RuleOp p_operation);
    RuleOp get_operation() const;

    void set_value(const godot::Variant &p_value);
    godot::Variant get_value() const;

    void set_priority(int p_priority);
    int get_priority() const;

    void set_is_important(bool p_important);
    bool get_is_important() const;

    // --- Specificity & Cascade Precedence ---
    int get_specificity() const;
    bool outweighs(const godot::Ref<GameplayRule> &p_other) const;
    bool has_same_target(const godot::Ref<GameplayRule> &p_other) const;

    // --- GameEntity Matching & Application ---
    GameProperty *find_target_game_property(GameEntity *p_entity) const;
    bool matches_entity(GameEntity *p_entity, bool p_is_self = true) const;
    bool apply_to_entity(GameEntity *p_entity, bool p_is_self = true) const;

    // --- Persistence ---
    godot::Dictionary save_rule() const;
    void load_rule(const godot::Dictionary &p_data);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::RuleScope);
VARIANT_ENUM_CAST(ideam::godot_ext::RuleOp);