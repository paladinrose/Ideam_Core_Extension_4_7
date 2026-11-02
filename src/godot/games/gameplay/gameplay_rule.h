#pragma once

#include "gameplay_resource.h"
#include "../game_property_modifier.h"
#include "../../narratives/helpers/gameplay_condition.h"
#include "../../utilities/node_retargeter.h"
#include "../../utilities/signal_connector.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameEntity;
class GameProperty;

/**
 * DOD NOTE: Extracting RuleScope, RuleCascadeLayer, RuleOp, and RulePropertyTarget
 * at namespace scope allows them to be packed directly into dense Structure of Arrays
 * (SoA) columns or Task manifests without pulling in the full GameplayRule Resource definition.
 *
 * In the future DOD compilation pipeline (Phase 2+):
 * - RuleScope and Selector fields compile into QueryTask nodes (ArchetypeQueryLogic,
 *   BitmaskQueryLogic, DataComparisonQueryLogic, HierarchicalBridgeQueryLogic)
 *   producing a MemoryBufferSelectionPOD.
 * - RuleCascadeLayer, priority, and specificity map 1:1 to sorting and masking selections.
 * - RuleOp and Declaration fields compile into TransformTask nodes
 *   (ValueAccumulationTransformLogic, BoundaryConstraintTransformLogic) operating
 *   on contiguous BaseValue and ComputedValue columns in a MemoryBufferPOD.
 */
enum RuleScope : int32_t {
    SELF = 0,        // Applies strictly to the owner holding the GameplayStyle (:host)
    DESCENDANTS = 1, // Cascades down to owned/child GameEntities (*)
    ALL = 2          // Applies to both the owner and all descendant GameEntities
};

/**
 * CSS @layer / origin equivalents ordered from broadest baseline (lowest precedence)
 * to most specific local/assist override (highest precedence).
 * Evaluated after `is_important` and before `priority` and `get_specificity()`.
 */
enum RuleCascadeLayer : int32_t {
    LAYER_BASE = 0,                 // Genre/baseline rules on GameplayStyle
    LAYER_MODE = 1,                 // Match-specific rules on GameplayMode & domain options
    LAYER_DIFFICULTY = 2,           // Global scalars and AI shifts from GameplayDifficulty
    LAYER_ENVIRONMENT = 3,          // Board-wide or spatial rules from GameBoard
    LAYER_PLAYER_ACCESSIBILITY = 4, // Per-player handicaps, control remaps, or accessibility assists
    LAYER_INLINE_LOCAL = 5          // Local passives, buffs, or unique overrides on an individual GameEntity
};

enum RuleOp : int32_t {
    SET = 0,                // Overwrites the target property or GameProperty sub-target (caching Base Value)
    ADD = 1,                // Adds to the target property or GameProperty sub-target (caching Base Value)
    MULTIPLY = 2,           // Scales the target property or GameProperty sub-target (caching Base Value)
    MODIFIER_ADD = 3,       // Injects or updates a GamePropertyModifier on a matching GameProperty
    CLAMP = 4,              // Clamps the target property or GameProperty sub-target between [clamp_min, clamp_max]
    MODIFIER_REMOVE = 5,    // Removes this rule's injected GamePropertyModifier (or matching modifier)
    GROUP_ADD = 6,          // Dynamically adds group tag(s) to GameAgentAction::competes_with_groups or Node groups
    GROUP_REMOVE = 7,       // Dynamically removes group tag(s) from GameAgentAction::competes_with_groups or Node groups
    INTERACTION_WEIGHT = 8, // Alters how GameInteraction weights instigator_value vs. competition_value and consequences
    SIGNAL_CONNECT = 9,     // Dynamically connects signals via SignalConnector while the style is active
    NODE_RETARGET = 10      // Dynamically retargets node properties via NodeRetargeter while the style is active
};

/**
 * Identifies which aspect of a resolved child GameProperty a rule reads/modifies.
 * Can be set explicitly via `property_target` or implicitly via `"PropertyName:sub_target"`
 * syntax in `target_property` (e.g., `"Health:max_value"`, `"Stamina:locked"`).
 */
enum RulePropertyTarget : int32_t {
    PROP_TARGET_VALUE = 0,               // GameProperty::value (or `modifiers` when MODIFIER_ADD/REMOVE)
    PROP_TARGET_MAX_VALUE = 1,           // GameProperty::max_value (or `max_modifiers` when MODIFIER_ADD/REMOVE)
    PROP_TARGET_MIN_VALUE = 2,           // GameProperty::min_value (or `min_modifiers` when MODIFIER_ADD/REMOVE)
    PROP_TARGET_LOCKED = 3,              // GameProperty::locked
    PROP_TARGET_RESTRICT_MAX = 4,        // GameProperty::restrict_value_to_current_max_value
    PROP_TARGET_RESTRICT_MIN = 5,        // GameProperty::restrict_value_to_current_min_value
    PROP_TARGET_ALLOW_VALUE_SET = 6,     // GameProperty::allow_value_set
    PROP_TARGET_ALLOW_VALUE_MODS = 7,    // GameProperty::allow_value_modifiers
    PROP_TARGET_ALLOW_MAX_SET = 8,       // GameProperty::allow_max_value_set
    PROP_TARGET_ALLOW_MAX_MODS = 9,      // GameProperty::allow_max_modifiers
    PROP_TARGET_ALLOW_MIN_SET = 10,      // GameProperty::allow_min_value_set
    PROP_TARGET_ALLOW_MIN_MODS = 11      // GameProperty::allow_min_modifiers
};

/**
 * DOD NOTE: GameplayRule represents a single CSS-like cascading rule (Selector + Declaration)
 * targeting the Uniform "DOM" Hierarchy (Game, GameBoard, GamePlayer, GameEntity, GameAgent,
 * GamePiece, GameAgentAction, GamePieceAction, GameInteraction, and GameProperty) as well as
 * duck-typed reflection targets (PartSignature) and runtime pseudo-states (:state / Gameplay_Condition).
 *
 * It supports all 4 Rule Payloads from the Cascading Rules Architecture:
 * 1. GameProperty Modifiers (value, max_value, min_value, flags, and STATIC/TIMED/DIMINISHING/RESOURCE modifiers).
 * 2. Direct Property Overrides with authoritative Base Value caching, SET/ADD/MULTIPLY/CLAMP, and clean restoration.
 * 3. Action & Interaction Laws (GameAgentAction::priority, competes_with_groups tag injection/removal,
 *    and GameInteraction instigator vs. competition score weighting).
 * 4. Reactive Signal/Method & Node Retarget Hooks via SignalConnector and NodeRetargeter.
 */
class GameplayRule : public GameplayResource {
    GDCLASS(GameplayRule, GameplayResource)

protected:
    static void _bind_methods();

private:
    // --- Scope & Cascade Layer (Address Routing & @layer Precedence) ---
    RuleScope scope = RuleScope::SELF;
    RuleCascadeLayer cascade_layer = RuleCascadeLayer::LAYER_BASE;

    // --- Core Hierarchy Selector (Who & Where) ---
    // Matches target class or subclass via ClassDB::is_class (e.g., "Game", "GameAgent", "GamePiece", or "" for any).
    godot::StringName target_class;

    // Matches title (GameEntity::title, Game::title, GameProperty::title), Node name,
    // Node group, or GameAgentAction::competes_with_groups ("" for any).
    godot::StringName target_title_or_group;

    // Matches a direct property on the target Object (e.g., "time_scale", "maximum_interactions")
    // or a child GameProperty title (e.g., "Health", or "Health:max_value").
    godot::StringName target_property;

    // --- Duck-Typed Structural Reflection Selector (PartSignature) ---
    godot::TypedArray<godot::StringName> required_properties;
    godot::TypedArray<godot::StringName> required_methods;
    godot::TypedArray<godot::StringName> required_signals;

    // --- Pseudo-State (:state) & Narrative Condition Selector ---
    // Evaluates runtime state such as ":is_exhausted", ":in_progress", ":enabled", ":disabled",
    // ":paused", ":possessed", ":active", ":locked", ":at_max", ":at_min", or boolean property names.
    godot::StringName pseudo_state;
    godot::Ref<Incident_Condition> gameplay_condition;

    // --- Declaration (The Payload) ---
    RuleOp operation = RuleOp::SET;
    godot::Variant value;
    godot::Variant clamp_min = 0.0;
    godot::Variant clamp_max = 1.0;
    int priority = 0;
    bool is_important = false;

    // --- Payload 1: GameProperty Sub-Target & Modifier Settings ---
    RulePropertyTarget property_target = RulePropertyTarget::PROP_TARGET_VALUE;
    ModifierType modifier_type = ModifierType::STATIC;
    float modifier_time_limit = 0.0f;

    // --- Payload 3: Action & Interaction Law Settings ---
    float instigator_weight = 1.0f;
    float competition_weight = 1.0f;
    float consequence_score_multiplier = 1.0f;

    // --- Payload 4: Reactive Signal & Node Retarget Hooks ---
    godot::Ref<SignalConnector> signal_connector;
    godot::Ref<NodeRetargeter> node_retargeter;

    // --- Internal Helpers ---
    bool _object_has_direct_property(const godot::Object *p_object, const godot::StringName &p_property) const;
    void _parse_target_property(godot::StringName &r_base_property, RulePropertyTarget &r_sub_target) const;
    godot::StringName _get_modifier_meta_tag() const;
    void _resolve_clamp_bounds(double &r_min, double &r_max) const;

    bool _apply_to_game_property(GameProperty *p_prop, RulePropertyTarget p_sub_target) const;
    bool _revert_from_game_property(GameProperty *p_prop, RulePropertyTarget p_sub_target) const;

    bool _apply_group_operation(godot::Object *p_object, bool p_add) const;
    bool _apply_interaction_weights(godot::Object *p_object) const;
    bool _apply_reactive_hooks(godot::Object *p_object) const;
    bool _revert_reactive_hooks(godot::Object *p_object) const;

public:
    // Metadata keys stored on target Objects for non-destructive Base Value caching and rollback
    static constexpr const char *META_BASE_VALUES = "__ideam_rule_base_values";
    static constexpr const char *META_ACTIVE_MODIFIERS = "__ideam_rule_modifiers";
    static constexpr const char *META_INTERACTION_INSTIGATOR_WEIGHT = "__ideam_instigator_weight";
    static constexpr const char *META_INTERACTION_COMPETITION_WEIGHT = "__ideam_competition_weight";
    static constexpr const char *META_INTERACTION_CONSEQUENCE_MULT = "__ideam_consequence_score_multiplier";
    static constexpr const char *META_ACTIVE_SIGNAL_CONNECTORS = "__ideam_rule_signal_connectors";
    static constexpr const char *META_ACTIVE_NODE_RETARGETERS = "__ideam_rule_node_retargeters";

    GameplayRule();
    ~GameplayRule();

    // --- Scope & Layer Setters / Getters ---
    void set_scope(RuleScope p_scope);
    RuleScope get_scope() const;

    void set_cascade_layer(RuleCascadeLayer p_layer);
    RuleCascadeLayer get_cascade_layer() const;

    bool applies_to_self() const;
    bool applies_to_descendants() const;

    // --- Core Selector Setters / Getters ---
    void set_target_class(const godot::StringName &p_class);
    godot::StringName get_target_class() const;

    void set_target_title_or_group(const godot::StringName &p_title_or_group);
    godot::StringName get_target_title_or_group() const;

    void set_target_property(const godot::StringName &p_property);
    godot::StringName get_target_property() const;

    // --- Structural Reflection (PartSignature) Setters / Getters ---
    void set_required_properties(const godot::TypedArray<godot::StringName> &p_properties);
    godot::TypedArray<godot::StringName> get_required_properties() const;

    void set_required_methods(const godot::TypedArray<godot::StringName> &p_methods);
    godot::TypedArray<godot::StringName> get_required_methods() const;

    void set_required_signals(const godot::TypedArray<godot::StringName> &p_signals);
    godot::TypedArray<godot::StringName> get_required_signals() const;

    // --- Pseudo-State & Condition Setters / Getters ---
    void set_pseudo_state(const godot::StringName &p_pseudo_state);
    godot::StringName get_pseudo_state() const;

    void set_gameplay_condition(const godot::Ref<Incident_Condition> &p_condition);
    godot::Ref<Incident_Condition> get_gameplay_condition() const;

    // --- Declaration Setters / Getters ---
    void set_operation(RuleOp p_operation);
    RuleOp get_operation() const;

    void set_value(const godot::Variant &p_value);
    godot::Variant get_value() const;

    void set_clamp_min(const godot::Variant &p_min);
    godot::Variant get_clamp_min() const;

    void set_clamp_max(const godot::Variant &p_max);
    godot::Variant get_clamp_max() const;

    void set_priority(int p_priority);
    int get_priority() const;

    void set_is_important(bool p_important);
    bool get_is_important() const;

    // --- Payload 1: GameProperty Sub-Target & Modifier Setters / Getters ---
    void set_property_target(RulePropertyTarget p_target);
    RulePropertyTarget get_property_target() const;

    void set_modifier_type(ModifierType p_type);
    ModifierType get_modifier_type() const;

    void set_modifier_time_limit(float p_time_limit);
    float get_modifier_time_limit() const;

    // --- Payload 3: Action & Interaction Law Setters / Getters ---
    void set_instigator_weight(float p_weight);
    float get_instigator_weight() const;

    void set_competition_weight(float p_weight);
    float get_competition_weight() const;

    void set_consequence_score_multiplier(float p_multiplier);
    float get_consequence_score_multiplier() const;

    int compute_weighted_interaction_score(int p_instigator_value, int p_competition_value) const;
    int scale_consequence_score(int p_score) const;
    static int evaluate_interaction_score_for_object(const godot::Object *p_interaction, int p_instigator_value, int p_competition_value);
    static int scale_consequence_score_for_object(const godot::Object *p_interaction, int p_score);

    // --- Payload 4: Reactive Signal & Node Retarget Setters / Getters ---
    void set_signal_connector(const godot::Ref<SignalConnector> &p_connector);
    godot::Ref<SignalConnector> get_signal_connector() const;

    void set_node_retargeter(const godot::Ref<NodeRetargeter> &p_retargeter);
    godot::Ref<NodeRetargeter> get_node_retargeter() const;

    // --- Specificity & Cascade Precedence ---
    int get_specificity() const;
    bool outweighs(const godot::Ref<GameplayRule> &p_other) const;
    bool has_same_target(const godot::Ref<GameplayRule> &p_other) const;

    // --- Base Value Caching & Restoration ---
    static void cache_base_value(godot::Object *p_object, const godot::StringName &p_property, const godot::Variant &p_value);
    static bool has_cached_base_value(const godot::Object *p_object, const godot::StringName &p_property);
    static godot::Variant get_cached_base_value(const godot::Object *p_object, const godot::StringName &p_property, const godot::Variant &p_fallback = godot::Variant());
    static bool restore_base_value(godot::Object *p_object, const godot::StringName &p_property);
    static void restore_all_base_values(godot::Object *p_object);
    static void revert_all_from_object(godot::Object *p_object);

    // --- Matching, Application & Rollback (Object & GameEntity) ---
    GameProperty *find_target_game_property_on_object(godot::Object *p_object) const;
    GameProperty *find_target_game_property(GameEntity *p_entity) const;

    bool matches_part_signature(const godot::Object *p_object) const;
    bool matches_pseudo_state(godot::Object *p_object) const;

    bool matches_object(godot::Object *p_object, bool p_is_self = true) const;
    bool matches_entity(GameEntity *p_entity, bool p_is_self = true) const;

    bool apply_to_object(godot::Object *p_object, bool p_is_self = true) const;
    bool apply_to_entity(GameEntity *p_entity, bool p_is_self = true) const;

    bool revert_from_object(godot::Object *p_object) const;
    bool revert_from_entity(GameEntity *p_entity) const;

    // --- Persistence & GameplayResource Overrides ---
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;

    godot::Dictionary save_rule() const;
    void load_rule(const godot::Dictionary &p_data);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::RuleScope);
VARIANT_ENUM_CAST(ideam::godot_ext::RuleCascadeLayer);
VARIANT_ENUM_CAST(ideam::godot_ext::RuleOp);
VARIANT_ENUM_CAST(ideam::godot_ext::RulePropertyTarget);