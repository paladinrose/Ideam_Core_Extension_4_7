#include "gameplay_style.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../game_entity.h"
#include "../game_property.h"
#include "gameplay_mode.h"
#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayStyle::_bind_methods() {
    // --- Properties ---
    godot::ClassDB::bind_method(godot::D_METHOD("set_default_layer", "layer"), &GameplayStyle::set_default_layer);
    godot::ClassDB::bind_method(godot::D_METHOD("get_default_layer"), &GameplayStyle::get_default_layer);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::INT,
            "default_layer",
            godot::PROPERTY_HINT_ENUM,
            "Base (Style),Mode,Difficulty,Environment (Board),Player & Accessibility,Inline (Entity)"
        ),
        "set_default_layer",
        "get_default_layer"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_modes", "modes"), &GameplayStyle::set_modes);
    godot::ClassDB::bind_method(godot::D_METHOD("get_modes"), &GameplayStyle::get_modes);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "modes", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayMode"),
        "set_modes",
        "get_modes"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_rules", "rules"), &GameplayStyle::set_rules);
    godot::ClassDB::bind_method(godot::D_METHOD("get_rules"), &GameplayStyle::get_rules);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::ARRAY, "rules", godot::PROPERTY_HINT_TYPE_STRING, "24/34:GameplayRule"),
        "set_rules",
        "get_rules"
    );

    // --- Rule Management Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("add_rule", "rule"), &GameplayStyle::add_rule);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_rule", "rule"), &GameplayStyle::remove_rule);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_rule_at", "index"), &GameplayStyle::remove_rule_at);
    godot::ClassDB::bind_method(godot::D_METHOD("clear_rules"), &GameplayStyle::clear_rules);
    godot::ClassDB::bind_method(godot::D_METHOD("get_rule_count"), &GameplayStyle::get_rule_count);
    godot::ClassDB::bind_method(godot::D_METHOD("get_rule", "index"), &GameplayStyle::get_rule);

    // --- Cascade & Style Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("merge_with_child", "child_style"), &GameplayStyle::merge_with_child);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_style", "style"), &GameplayStyle::apply_style);
    godot::ClassDB::bind_method(godot::D_METHOD("apply_rules_to_object", "object", "is_self", "restore_base_first"), &GameplayStyle::apply_rules_to_object, DEFVAL(true), DEFVAL(true));
    godot::ClassDB::bind_method(godot::D_METHOD("apply_rules_to_entity", "entity", "is_self"), &GameplayStyle::apply_rules_to_entity, DEFVAL(true));
    godot::ClassDB::bind_method(godot::D_METHOD("revert_rules_from_object", "object"), &GameplayStyle::revert_rules_from_object);
    godot::ClassDB::bind_method(godot::D_METHOD("revert_rules_from_entity", "entity"), &GameplayStyle::revert_rules_from_entity);
    godot::ClassDB::bind_method(godot::D_METHOD("inspect_cascade_for_object", "object", "is_self"), &GameplayStyle::inspect_cascade_for_object, DEFVAL(true));

    // --- Persistence Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("save_style"), &GameplayStyle::save_style);
    godot::ClassDB::bind_method(godot::D_METHOD("load_style", "data"), &GameplayStyle::load_style);
}

GameplayStyle::GameplayStyle() {
    // DOD NOTE: Constructors are kept lean to minimize initialization
    // overhead and support future bulk allocation strategies.
}

GameplayStyle::~GameplayStyle() {
}

// --- Layer & Modes Setters / Getters ---

void GameplayStyle::set_default_layer(RuleCascadeLayer p_layer) {
    if (default_layer == p_layer) return;
    default_layer = p_layer;
    emit_changed();
}

RuleCascadeLayer GameplayStyle::get_default_layer() const {
    return default_layer;
}

void GameplayStyle::set_modes(const godot::TypedArray<GameplayMode> &p_modes) {
    if (p_modes == modes) return;
    modes = p_modes;
    emit_changed();
}

godot::TypedArray<GameplayMode> GameplayStyle::get_modes() const {
    return modes;
}

// --- Rules Setters / Getters & Helpers ---

void GameplayStyle::set_rules(const godot::TypedArray<GameplayRule> &p_rules) {
    if (p_rules == rules) return;
    rules = p_rules;
    emit_changed();
}

godot::TypedArray<GameplayRule> GameplayStyle::get_rules() const {
    return rules;
}

void GameplayStyle::add_rule(const godot::Ref<GameplayRule> &p_rule) {
    if (!p_rule.is_valid()) return;
    _insert_or_override_rule(p_rule);
    emit_changed();
}

void GameplayStyle::remove_rule(const godot::Ref<GameplayRule> &p_rule) {
    if (!p_rule.is_valid()) return;
    int idx = rules.find(p_rule);
    if (idx >= 0) {
        rules.remove_at(idx);
        emit_changed();
    }
}

void GameplayStyle::remove_rule_at(int p_index) {
    if (p_index < 0 || p_index >= rules.size()) return;
    rules.remove_at(p_index);
    emit_changed();
}

void GameplayStyle::clear_rules() {
    if (rules.is_empty()) return;
    rules.clear();
    emit_changed();
}

int GameplayStyle::get_rule_count() const {
    return rules.size();
}

godot::Ref<GameplayRule> GameplayStyle::get_rule(int p_index) const {
    if (p_index < 0 || p_index >= rules.size()) {
        return godot::Ref<GameplayRule>();
    }
    return godot::Ref<GameplayRule>(godot::Object::cast_to<GameplayRule>(rules[p_index]));
}

// --- Internal Cascade Precedence Helpers ---

void GameplayStyle::_insert_or_override_into_array(godot::TypedArray<GameplayRule> &r_target_rules, const godot::Ref<GameplayRule> &p_rule) {
    if (!p_rule.is_valid() || !p_rule->get_enabled()) {
        return;
    }

    // 1. Check if an existing rule has the exact same selector target and scope applicability.
    //    If both are SET operations on the same target, resolve precedence via outweighs()
    //    so dead overridden SET rules don't accumulate down the tree.
    for (int i = 0; i < r_target_rules.size(); ++i) {
        godot::Ref<GameplayRule> existing(godot::Object::cast_to<GameplayRule>(r_target_rules[i]));
        if (!existing.is_valid()) continue;

        if (p_rule->has_same_target(existing) &&
            p_rule->get_operation() == RuleOp::SET &&
            existing->get_operation() == RuleOp::SET &&
            p_rule->get_scope() == existing->get_scope()) {
            if (p_rule->outweighs(existing)) {
                r_target_rules.remove_at(i);
                break;
            } else {
                // Existing rule is strictly higher precedence (e.g., parent !important vs normal child)
                return;
            }
        }
    }

    // 2. Insert in ascending precedence order (lowest precedence first, highest precedence last)
    //    so sequential iteration naturally applies broader baseline rules before specific/important overrides.
    int insert_idx = r_target_rules.size();
    for (int i = 0; i < r_target_rules.size(); ++i) {
        godot::Ref<GameplayRule> existing(godot::Object::cast_to<GameplayRule>(r_target_rules[i]));
        if (!existing.is_valid()) continue;

        if (!p_rule->outweighs(existing)) {
            insert_idx = i;
            break;
        }
    }

    r_target_rules.insert(insert_idx, p_rule);
}

void GameplayStyle::_insert_or_override_rule(const godot::Ref<GameplayRule> &p_rule) {
    _insert_or_override_into_array(rules, p_rule);
}

// --- Cascade & Application Functions ---

godot::TypedArray<GameplayRule> GameplayStyle::collect_rules() const {
    godot::TypedArray<GameplayRule> collected;
    if (!get_enabled()) {
        return collected;
    }

    // 1. Collect associated_rules from GameplayResource base
    godot::TypedArray<GameplayRule> base_rules = GameplayResource::collect_rules();
    for (int i = 0; i < base_rules.size(); ++i) {
        godot::Ref<GameplayRule> r(godot::Object::cast_to<GameplayRule>(base_rules[i]));
        if (r.is_valid()) {
            _insert_or_override_into_array(collected, r);
        }
    }

    // 2. Collect synthesized and associated rules from all enabled GameplayModes
    for (int i = 0; i < modes.size(); ++i) {
        GameplayMode *mode_obj = godot::Object::cast_to<GameplayMode>(modes[i]);
        if (!mode_obj || !mode_obj->get_enabled()) continue;

        godot::TypedArray<GameplayRule> mode_rules = mode_obj->collect_rules();
        for (int j = 0; j < mode_rules.size(); ++j) {
            godot::Ref<GameplayRule> mr(godot::Object::cast_to<GameplayRule>(mode_rules[j]));
            if (mr.is_valid()) {
                _insert_or_override_into_array(collected, mr);
            }
        }
    }

    // 3. Collect explicit rules on this GameplayStyle (promoting LAYER_BASE to default_layer if set)
    for (int i = 0; i < rules.size(); ++i) {
        godot::Ref<GameplayRule> r(godot::Object::cast_to<GameplayRule>(rules[i]));
        if (!r.is_valid() || !r->get_enabled()) continue;

        if (default_layer != RuleCascadeLayer::LAYER_BASE && r->get_cascade_layer() == RuleCascadeLayer::LAYER_BASE) {
            godot::Ref<GameplayRule> layered_copy(godot::Object::cast_to<GameplayRule>(r->duplicate(true).ptr()));
            if (layered_copy.is_valid()) {
                layered_copy->set_cascade_layer(default_layer);
                _insert_or_override_into_array(collected, layered_copy);
                continue;
            }
        }
        _insert_or_override_into_array(collected, r);
    }

    return collected;
}

godot::Ref<GameplayStyle> GameplayStyle::merge_with_child(const godot::Ref<GameplayStyle> &p_child_style) const {
    godot::Ref<GameplayStyle> resolved;
    resolved.instantiate();
    resolved->set_title(get_title());

    // 1. Copy modes from parent (duplicating sub-resources so runtime merging stays isolated)
    godot::TypedArray<GameplayMode> inherited_modes;
    for (int i = 0; i < modes.size(); ++i) {
        if (GameplayMode *mode_obj = godot::Object::cast_to<GameplayMode>(modes[i])) {
            godot::Ref<GameplayMode> mode_copy(godot::Object::cast_to<GameplayMode>(mode_obj->duplicate(true).ptr()));
            if (mode_copy.is_valid()) {
                inherited_modes.append(mode_copy);
            }
        }
    }
    resolved->set_modes(inherited_modes);

    // 2. Copy all DESCENDANTS / ALL rules from this parent style (including associated_rules and rules).
    //    RuleScope::SELF rules on the parent are intentionally excluded from cascading.
    //    RuleScope::DESCENDANTS rules on the parent are promoted to RuleScope::ALL on a duplicate
    //    so the receiving child both evaluates them on itself (applies_to_self() == true)
    //    and continues cascading them to its own descendants (applies_to_descendants() == true).
    godot::TypedArray<GameplayRule> parent_explicit = GameplayResource::collect_rules();
    for (int i = 0; i < rules.size(); ++i) {
        parent_explicit.append(rules[i]);
    }

    for (int i = 0; i < parent_explicit.size(); ++i) {
        godot::Ref<GameplayRule> parent_rule(godot::Object::cast_to<GameplayRule>(parent_explicit[i]));
        if (!parent_rule.is_valid() || !parent_rule->get_enabled() || !parent_rule->applies_to_descendants()) {
            continue;
        }

        if (parent_rule->get_scope() == RuleScope::DESCENDANTS ||
            (default_layer != RuleCascadeLayer::LAYER_BASE && parent_rule->get_cascade_layer() == RuleCascadeLayer::LAYER_BASE)) {
            godot::Ref<GameplayRule> promoted_rule(godot::Object::cast_to<GameplayRule>(parent_rule->duplicate(true).ptr()));
            if (promoted_rule.is_valid()) {
                if (promoted_rule->get_scope() == RuleScope::DESCENDANTS) {
                    promoted_rule->set_scope(RuleScope::ALL);
                }
                if (default_layer != RuleCascadeLayer::LAYER_BASE && promoted_rule->get_cascade_layer() == RuleCascadeLayer::LAYER_BASE) {
                    promoted_rule->set_cascade_layer(default_layer);
                }
                resolved->_insert_or_override_rule(promoted_rule);
            }
        } else {
            resolved->_insert_or_override_rule(parent_rule);
        }
    }

    // 3. Append/override with the child's local gameplay_style modes and rules
    if (p_child_style.is_valid()) {
        resolved->apply_style(p_child_style);
    }

    return resolved;
}

void GameplayStyle::apply_style(const godot::Ref<GameplayStyle> &p_style) {
    if (!p_style.is_valid()) {
        return;
    }

    if (!p_style->get_title().is_empty()) {
        set_title(p_style->get_title());
    }

    // 1. Merge Modes
    godot::TypedArray<GameplayMode> other_modes = p_style->get_modes();
    for (int i = 0; i < other_modes.size(); ++i) {
        GameplayMode *style_mode_obj = godot::Object::cast_to<GameplayMode>(other_modes[i]);
        if (!style_mode_obj) continue;

        godot::Ref<GameplayMode> style_mode_ref(style_mode_obj);
        bool add_style_mode = true;

        for (int j = 0; j < modes.size(); ++j) {
            GameplayMode *my_mode_obj = godot::Object::cast_to<GameplayMode>(modes[j]);
            if (!my_mode_obj) continue;

            if (my_mode_obj->matches_identity(style_mode_ref)) {
                // DOD NOTE: Direct static-typed call avoids Object::call lookup overhead.
                my_mode_obj->apply_mode(style_mode_ref);
                add_style_mode = false;
                break;
            }
        }

        if (add_style_mode) {
            modes.append(style_mode_obj);
        }
    }

    // 2. Merge associated_rules and explicit rules from p_style (respecting p_style's default_layer)
    RuleCascadeLayer incoming_layer = p_style->get_default_layer();
    godot::TypedArray<GameplayRule> incoming_rules = p_style->get_associated_rules();
    godot::TypedArray<GameplayRule> explicit_rules = p_style->get_rules();
    for (int i = 0; i < explicit_rules.size(); ++i) {
        incoming_rules.append(explicit_rules[i]);
    }

    for (int i = 0; i < incoming_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule_ref(godot::Object::cast_to<GameplayRule>(incoming_rules[i]));
        if (!rule_ref.is_valid()) continue;

        if (incoming_layer != RuleCascadeLayer::LAYER_BASE && rule_ref->get_cascade_layer() == RuleCascadeLayer::LAYER_BASE) {
            godot::Ref<GameplayRule> layered_copy(godot::Object::cast_to<GameplayRule>(rule_ref->duplicate(true).ptr()));
            if (layered_copy.is_valid()) {
                layered_copy->set_cascade_layer(incoming_layer);
                _insert_or_override_rule(layered_copy);
                continue;
            }
        }
        _insert_or_override_rule(rule_ref);
    }

    emit_changed();
}

int GameplayStyle::apply_rules_to_object(godot::Object *p_object, bool p_is_self, bool p_restore_base_first) const {
    if (!p_object || !get_enabled()) {
        return 0;
    }

    // 1. Cleanly revert any previously applied style overrides/modifiers/hooks on p_object
    //    so re-evaluating or switching boards/styles never compounds ADD/MULTIPLY rules.
    if (p_restore_base_first) {
        GameplayRule::revert_all_from_object(p_object);
    }

    // 2. Gather the complete precedence-sorted rule cascade (associated_rules + mode rules + explicit rules)
    godot::TypedArray<GameplayRule> all_rules = collect_rules();

    int applied_count = 0;
    for (int i = 0; i < all_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(all_rules[i]));
        if (!rule.is_valid()) continue;

        // Synthesized rules from GameplayModes have Scope::DESCENDANTS by default; when a style
        // is evaluated on a descendant entity, treat mode-synthesized DESCENDANTS rules as applicable.
        bool effective_is_self = p_is_self;
        if (p_is_self && !rule->applies_to_self() && rule->applies_to_descendants() &&
            rule->get_cascade_layer() != RuleCascadeLayer::LAYER_BASE &&
            rule->get_cascade_layer() != RuleCascadeLayer::LAYER_INLINE_LOCAL) {
            effective_is_self = false;
        }

        if (rule->apply_to_object(p_object, effective_is_self)) {
            ++applied_count;
        }
    }
    return applied_count;
}

int GameplayStyle::apply_rules_to_entity(GameEntity *p_entity, bool p_is_self) const {
    return apply_rules_to_object(p_entity, p_is_self, true);
}

void GameplayStyle::revert_rules_from_object(godot::Object *p_object) const {
    if (!p_object) {
        return;
    }
    GameplayRule::revert_all_from_object(p_object);
}

void GameplayStyle::revert_rules_from_entity(GameEntity *p_entity) const {
    GameplayRule::revert_all_from_object(p_entity);
}

godot::Array GameplayStyle::inspect_cascade_for_object(godot::Object *p_object, bool p_is_self) const {
    godot::Array report;
    if (!p_object) {
        return report;
    }

    godot::TypedArray<GameplayRule> all_rules = collect_rules();

    // First pass: collect all rules that match p_object
    godot::TypedArray<GameplayRule> matched_rules;
    for (int i = 0; i < all_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(all_rules[i]));
        if (!rule.is_valid()) continue;

        bool effective_is_self = p_is_self;
        if (p_is_self && !rule->applies_to_self() && rule->applies_to_descendants() &&
            rule->get_cascade_layer() != RuleCascadeLayer::LAYER_BASE &&
            rule->get_cascade_layer() != RuleCascadeLayer::LAYER_INLINE_LOCAL) {
            effective_is_self = false;
        }

        if (rule->matches_object(p_object, effective_is_self)) {
            matched_rules.append(rule);
        }
    }

    // Second pass: determine which rules are shadowed/superseded by a later SET rule on the same property
    for (int i = 0; i < matched_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(matched_rules[i]));
        godot::Dictionary entry;
        entry["rule"] = rule;
        entry["title"] = rule->get_title();
        entry["cascade_layer"] = static_cast<int>(rule->get_cascade_layer());
        entry["priority"] = rule->get_priority();
        entry["specificity"] = rule->get_specificity();
        entry["is_important"] = rule->get_is_important();
        entry["operation"] = static_cast<int>(rule->get_operation());
        entry["target_property"] = rule->get_target_property();
        entry["value"] = rule->get_value();

        bool overridden = false;
        godot::Ref<GameplayRule> superseded_by;
        for (int j = i + 1; j < matched_rules.size(); ++j) {
            godot::Ref<GameplayRule> later(godot::Object::cast_to<GameplayRule>(matched_rules[j]));
            if (later.is_valid() &&
                later->get_target_property() == rule->get_target_property() &&
                later->get_property_target() == rule->get_property_target() &&
                later->get_operation() == RuleOp::SET) {
                overridden = true;
                superseded_by = later;
            }
        }

        entry["overridden"] = overridden;
        entry["superseded_by"] = superseded_by;

        godot::StringName prop_name = rule->get_target_property();
        if (!prop_name.is_empty()) {
            if (GameProperty *gp = rule->find_target_game_property_on_object(p_object)) {
                entry["base_value"] = GameplayRule::get_cached_base_value(gp, godot::StringName("value"), gp->get_value());
                entry["computed_value"] = gp->get_modded_value();
            } else {
                entry["base_value"] = GameplayRule::get_cached_base_value(p_object, prop_name, p_object->get(prop_name));
                entry["computed_value"] = p_object->get(prop_name);
            }
        }

        report.append(entry);
    }

    return report;
}

// --- Persistence & GameplayResource Overrides ---

void GameplayStyle::apply_resource(const godot::Ref<GameplayResource> &p_other) {
    GameplayResource::apply_resource(p_other);
    godot::Ref<GameplayStyle> other_style = p_other;
    if (other_style.is_valid()) {
        default_layer = other_style->get_default_layer();
        apply_style(other_style);
    }
}

godot::Dictionary GameplayStyle::save_resource() const {
    return save_style();
}

void GameplayStyle::load_resource(const godot::Dictionary &p_data) {
    load_style(p_data);
}

godot::Dictionary GameplayStyle::save_style() const {
    godot::Dictionary data = GameplayResource::save_resource();
    data["default_layer"] = static_cast<int>(default_layer);

    godot::Array saved_modes;
    for (int i = 0; i < modes.size(); ++i) {
        if (GameplayMode *mode_obj = godot::Object::cast_to<GameplayMode>(modes[i])) {
            saved_modes.append(mode_obj->save_resource());
        }
    }
    data["modes"] = saved_modes;

    godot::Array saved_rules;
    for (int i = 0; i < rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(rules[i]));
        if (rule.is_valid()) {
            saved_rules.append(rule->save_rule());
        }
    }
    data["rules"] = saved_rules;

    return data;
}

void GameplayStyle::load_style(const godot::Dictionary &p_data) {
    GameplayResource::load_resource(p_data);

    if (p_data.has("default_layer")) {
        default_layer = static_cast<RuleCascadeLayer>(static_cast<int>(p_data["default_layer"]));
    }

    if (p_data.has("modes") && p_data["modes"].get_type() == godot::Variant::ARRAY) {
        modes.clear();
        godot::Array saved_modes = p_data["modes"];
        for (int i = 0; i < saved_modes.size(); ++i) {
            if (saved_modes[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayMode> mode;
                mode.instantiate();
                mode->load_resource(saved_modes[i]);
                modes.append(mode);
            }
        }
    }

    if (p_data.has("rules") && p_data["rules"].get_type() == godot::Variant::ARRAY) {
        rules.clear();
        godot::Array saved_rules = p_data["rules"];
        for (int i = 0; i < saved_rules.size(); ++i) {
            if (saved_rules[i].get_type() == godot::Variant::DICTIONARY) {
                godot::Ref<GameplayRule> rule;
                rule.instantiate();
                rule->load_rule(saved_rules[i]);
                _insert_or_override_rule(rule);
            }
        }
    }

    emit_changed();
}

} // namespace ideam::godot_ext