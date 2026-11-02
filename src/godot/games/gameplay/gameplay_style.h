#pragma once

#include "gameplay_resource.h"
#include "gameplay_rule.h"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameplayMode;
class GameEntity;

/**
 * DOD NOTE: GameplayStyle acts as a cascading stylesheet container for gameplay modes
 * and GameplayRules. Storing these as TypedArrays of Resource pointers (AoS) results in
 * pointer indirection when querying style settings across thousands of entities.
 *
 * GameplayStyle manages the object-level cascade via merge_with_child(),
 * filtering out parent RuleScope::SELF rules, promoting inherited RuleScope::DESCENDANTS
 * rules to RuleScope::ALL for descendants, harvesting synthesized rules from attached
 * GameplayModes, and ordering rules by importance, cascade layer (@layer), priority,
 * and selector specificity.
 *
 * When applied to a target Object or GameEntity, GameplayStyle first restores any
 * previously cached Base Values and removes prior rule-injected modifiers/hooks before
 * evaluating the active cascade in precedence order.
 */
class GameplayStyle : public GameplayResource {
    GDCLASS(GameplayStyle, GameplayResource)

protected:
    static void _bind_methods();

private:
    // DOD NOTE: Using TypedArray<GameplayMode> and TypedArray<GameplayRule> forces
    // pointer dereferencing across heap locations. Keep rules ordered by cascade
    // precedence so sequential application resolves overrides deterministically.
    RuleCascadeLayer default_layer = RuleCascadeLayer::LAYER_BASE;
    godot::TypedArray<GameplayMode> modes;
    godot::TypedArray<GameplayRule> rules;

    // Internal helper to insert or override a rule while maintaining cascade precedence order.
    void _insert_or_override_rule(const godot::Ref<GameplayRule> &p_rule);
    static void _insert_or_override_into_array(godot::TypedArray<GameplayRule> &r_target_rules, const godot::Ref<GameplayRule> &p_rule);

public:
    GameplayStyle();
    ~GameplayStyle();

    // --- Layer & Modes Setters / Getters ---
    void set_default_layer(RuleCascadeLayer p_layer);
    RuleCascadeLayer get_default_layer() const;

    void set_modes(const godot::TypedArray<GameplayMode> &p_modes);
    godot::TypedArray<GameplayMode> get_modes() const;

    // --- Rules Setters / Getters & Helpers ---
    void set_rules(const godot::TypedArray<GameplayRule> &p_rules);
    godot::TypedArray<GameplayRule> get_rules() const;

    void add_rule(const godot::Ref<GameplayRule> &p_rule);
    void remove_rule(const godot::Ref<GameplayRule> &p_rule);
    void remove_rule_at(int p_index);
    void clear_rules();
    int get_rule_count() const;
    godot::Ref<GameplayRule> get_rule(int p_index) const;

    // --- Cascade & Application Functions ---

    /**
     * Gathers all active GameplayRules contributed by this style:
     * 1. Base GameplayResource::collect_rules() (`associated_rules`)
     * 2. Synthesized and associated rules from all enabled `modes` (`mode->collect_rules()`)
     * 3. Explicit `rules` authored on this GameplayStyle
     * Returned in ascending cascade precedence order (broadest baseline first, highest precedence last).
     */
    virtual godot::TypedArray<GameplayRule> collect_rules() const override;

    /**
     * Produces a newly minted _resolved_gameplay_style for a child entity by:
     * 1. Copying all cascading rules (RuleScope::DESCENDANTS and RuleScope::ALL)
     *    from this (parent) resolved style (including rules synthesized by parent modes),
     *    promoting DESCENDANTS -> ALL so the child both evaluates them on itself and
     *    cascades them further down.
     * 2. Merging/overriding with the child's local gameplay_style modes and rules.
     */
    godot::Ref<GameplayStyle> merge_with_child(const godot::Ref<GameplayStyle> &p_child_style) const;

    /**
     * Merges another GameplayStyle's modes and rules into this instance in-place.
     */
    void apply_style(const godot::Ref<GameplayStyle> &p_style);

    /**
     * Evaluates and applies all matching rules in this style (including mode-synthesized rules)
     * to a target Object (Game, GameEntity, GameProperty, etc.) or GameEntity.
     * When `p_restore_base_first == true`, restores any cached Base Values and removes prior
     * rule-injected modifiers/hooks before applying the current cascade.
     * Returns the number of rules that matched and were applied.
     */
    int apply_rules_to_object(godot::Object *p_object, bool p_is_self = true, bool p_restore_base_first = true) const;
    int apply_rules_to_entity(GameEntity *p_entity, bool p_is_self = true) const;

    /**
     * Reverts all rules in this style (and restores cached Base Values / removes injected modifiers & hooks)
     * from the target Object or GameEntity.
     */
    void revert_rules_from_object(godot::Object *p_object) const;
    void revert_rules_from_entity(GameEntity *p_entity) const;

    /**
     * Simulates and inspects the rule cascade for `p_object`, returning an Array of Dictionaries
     * detailing each matching rule, its precedence (layer, priority, specificity, !important),
     * whether it was shadowed/overridden by a higher-precedence SET rule, and the base vs. computed value.
     */
    godot::Array inspect_cascade_for_object(godot::Object *p_object, bool p_is_self = true) const;

    // --- Persistence & GameplayResource Overrides ---
    virtual void apply_resource(const godot::Ref<GameplayResource> &p_other) override;
    virtual godot::Dictionary save_resource() const override;
    virtual void load_resource(const godot::Dictionary &p_data) override;

    godot::Dictionary save_style() const;
    void load_style(const godot::Dictionary &p_data);
};

} // namespace ideam::godot_ext