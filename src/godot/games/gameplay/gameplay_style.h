#pragma once

#include "gameplay_resource.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class GameplayMode;
class GameplayRule;
class GameEntity;

/**
 * DOD NOTE: GameplayStyle acts as a cascading stylesheet container for gameplay modes
 * and GameplayRules. Storing these as TypedArrays of Resource pointers (AoS) results in
 * pointer indirection when querying style settings across thousands of entities.
 *
 * In Phase 1, GameplayStyle manages the object-level cascade via merge_with_child(),
 * filtering out parent RuleScope::SELF rules, promoting inherited RuleScope::DESCENDANTS
 * rules to RuleScope::ALL for descendants, and ordering rules by importance, priority,
 * and selector specificity.
 *
 * In the future DOD compilation pipeline (Phase 2+), a resolved GameplayStyle will
 * flatten its GameplayRules into contiguous QueryTask and TransformTask manifests
 * operating over MemoryBufferPOD columns without per-entity Resource traversal.
 */
class GameplayStyle : public GameplayResource {
    GDCLASS(GameplayStyle, GameplayResource)

protected:
    static void _bind_methods();

private:
    // DOD NOTE: Using TypedArray<GameplayMode> and TypedArray<GameplayRule> forces
    // pointer dereferencing across heap locations. Keep rules ordered by cascade
    // precedence so sequential application resolves overrides deterministically.
    godot::TypedArray<GameplayMode> modes;
    godot::TypedArray<GameplayRule> rules;

    // Internal helper to insert or override a rule while maintaining cascade precedence order.
    void _insert_or_override_rule(const godot::Ref<GameplayRule> &p_rule);

public:
    GameplayStyle();
    ~GameplayStyle();

    // --- Modes Setters / Getters ---
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
     * Produces a newly minted _resolved_gameplay_style for a child entity by:
     * 1. Copying all cascading rules (RuleScope::DESCENDANTS and RuleScope::ALL)
     *    from this (parent) resolved style, promoting DESCENDANTS -> ALL so the
     *    child both evaluates them on itself and cascades them further down.
     * 2. Merging/overriding with the child's local gameplay_style modes and rules.
     */
    godot::Ref<GameplayStyle> merge_with_child(const godot::Ref<GameplayStyle> &p_child_style) const;

    /**
     * Merges another GameplayStyle's modes and rules into this instance in-place.
     */
    void apply_style(const godot::Ref<GameplayStyle> &p_style);

    /**
     * Evaluates and applies all matching rules in this resolved style to a target GameEntity.
     * Returns the number of rules that matched and were applied.
     */
    int apply_rules_to_entity(GameEntity *p_entity, bool p_is_self = true) const;

    // --- Persistence ---
    godot::Dictionary save_style() const;
    void load_style(const godot::Dictionary &p_data);
};

} // namespace ideam::godot_ext