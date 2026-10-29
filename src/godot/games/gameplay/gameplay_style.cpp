#include "gameplay_style.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../game_entity.h"
#include "gameplay_mode.h"
#include "gameplay_rule.h"

namespace ideam::godot_ext {

void GameplayStyle::_bind_methods() {
    // --- Properties ---
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
    godot::ClassDB::bind_method(godot::D_METHOD("apply_rules_to_entity", "entity", "is_self"), &GameplayStyle::apply_rules_to_entity, DEFVAL(true));

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

// --- Modes Setters / Getters ---

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

// --- Internal Cascade Precedence Helper ---

void GameplayStyle::_insert_or_override_rule(const godot::Ref<GameplayRule> &p_rule) {
    if (!p_rule.is_valid()) {
        return;
    }

    // 1. Check if an existing rule has the exact same selector target and scope applicability.
    //    If both are SET operations (or identical operations on the same target), resolve
    //    precedence via outweighs() so dead overridden SET rules don't accumulate down the tree.
    for (int i = 0; i < rules.size(); ++i) {
        godot::Ref<GameplayRule> existing(godot::Object::cast_to<GameplayRule>(rules[i]));
        if (!existing.is_valid()) continue;

        if (p_rule->has_same_target(existing) &&
            p_rule->get_operation() == RuleOp::SET &&
            existing->get_operation() == RuleOp::SET &&
            p_rule->get_scope() == existing->get_scope()) {
            // Incoming rule wins if it outweighs (or ties in importance/priority/specificity,
            // since child/later rules win ties in CSS cascade order).
            if (p_rule->outweighs(existing)) {
                rules.remove_at(i);
                break;
            } else {
                // Existing rule is strictly higher precedence (e.g., parent !important vs normal child)
                return;
            }
        }
    }

    // 2. Insert in ascending precedence order (lowest precedence first, highest precedence last)
    //    so sequential iteration naturally applies broader baseline rules before specific/important overrides.
    int insert_idx = rules.size();
    for (int i = 0; i < rules.size(); ++i) {
        godot::Ref<GameplayRule> existing(godot::Object::cast_to<GameplayRule>(rules[i]));
        if (!existing.is_valid()) continue;

        // If existing strictly outweighs p_rule (and p_rule does not outweigh existing),
        // p_rule belongs before existing.
        if (!p_rule->outweighs(existing)) {
            insert_idx = i;
            break;
        }
    }

    rules.insert(insert_idx, p_rule);
}

// --- Cascade & Application Functions ---

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

    // 2. Copy all DESCENDANTS / ALL rules from this parent style.
    //    RuleScope::SELF rules on the parent are intentionally excluded from cascading.
    //    RuleScope::DESCENDANTS rules on the parent are promoted to RuleScope::ALL on a duplicate
    //    so the receiving child both evaluates them on itself (applies_to_self() == true)
    //    and continues cascading them to its own descendants (applies_to_descendants() == true).
    for (int i = 0; i < rules.size(); ++i) {
        godot::Ref<GameplayRule> parent_rule(godot::Object::cast_to<GameplayRule>(rules[i]));
        if (!parent_rule.is_valid() || !parent_rule->applies_to_descendants()) {
            continue;
        }

        if (parent_rule->get_scope() == RuleScope::DESCENDANTS) {
            godot::Ref<GameplayRule> promoted_rule(godot::Object::cast_to<GameplayRule>(parent_rule->duplicate(true).ptr()));
            if (promoted_rule.is_valid()) {
                promoted_rule->set_scope(RuleScope::ALL);
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

        godot::String other_title = style_mode_obj->get_title();
        bool add_style_mode = true;

        for (int j = 0; j < modes.size(); ++j) {
            GameplayMode *my_mode_obj = godot::Object::cast_to<GameplayMode>(modes[j]);
            if (!my_mode_obj) continue;

            if (my_mode_obj->get_title() == other_title) {
                // DOD NOTE: Direct static-typed call avoids Object::call lookup overhead.
                my_mode_obj->apply_mode(style_mode_obj);
                add_style_mode = false;
                break;
            }
        }

        if (add_style_mode) {
            modes.append(style_mode_obj);
        }
    }

    // 2. Merge Rules (appending/overriding in cascade precedence order)
    godot::TypedArray<GameplayRule> other_rules = p_style->get_rules();
    for (int i = 0; i < other_rules.size(); ++i) {
        godot::Ref<GameplayRule> rule_ref(godot::Object::cast_to<GameplayRule>(other_rules[i]));
        if (rule_ref.is_valid()) {
            _insert_or_override_rule(rule_ref);
        }
    }

    emit_changed();
}

int GameplayStyle::apply_rules_to_entity(GameEntity *p_entity, bool p_is_self) const {
    if (!p_entity) {
        return 0;
    }

    int applied_count = 0;
    for (int i = 0; i < rules.size(); ++i) {
        godot::Ref<GameplayRule> rule(godot::Object::cast_to<GameplayRule>(rules[i]));
        if (rule.is_valid() && rule->apply_to_entity(p_entity, p_is_self)) {
            ++applied_count;
        }
    }
    return applied_count;
}

// --- Persistence ---

godot::Dictionary GameplayStyle::save_style() const {
    godot::Dictionary data;
    data["title"] = get_title();

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
    if (p_data.has("title")) {
        set_title(p_data["title"]);
    }

    if (p_data.has("rules")) {
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