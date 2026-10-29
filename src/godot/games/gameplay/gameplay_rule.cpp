#include "gameplay_rule.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include "../game_entity.h"
#include "../game_entities/actions/game_agent_action.h"
#include "../game_entities/game_piece.h"
#include "../game_property.h"
#include "../game_property_modifier.h"

namespace ideam::godot_ext {

void GameplayRule::_bind_methods() {
    // --- Enums ---
    BIND_ENUM_CONSTANT(RuleScope::SELF);
    BIND_ENUM_CONSTANT(RuleScope::DESCENDANTS);
    BIND_ENUM_CONSTANT(RuleScope::ALL);

    BIND_ENUM_CONSTANT(RuleOp::SET);
    BIND_ENUM_CONSTANT(RuleOp::ADD);
    BIND_ENUM_CONSTANT(RuleOp::MULTIPLY);
    BIND_ENUM_CONSTANT(RuleOp::MODIFIER_ADD);

    // --- Scope ---
    godot::ClassDB::bind_method(godot::D_METHOD("set_scope", "scope"), &GameplayRule::set_scope);
    godot::ClassDB::bind_method(godot::D_METHOD("get_scope"), &GameplayRule::get_scope);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "scope", godot::PROPERTY_HINT_ENUM, "Self,Descendants,All"),
        "set_scope",
        "get_scope"
    );

    // --- Selector Group ---
    ADD_GROUP("Selector", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_class", "target_class"), &GameplayRule::set_target_class);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_class"), &GameplayRule::get_target_class);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::STRING_NAME, "target_class"),
        "set_target_class",
        "get_target_class"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_title_or_group", "target_title_or_group"), &GameplayRule::set_target_title_or_group);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_title_or_group"), &GameplayRule::get_target_title_or_group);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::STRING_NAME, "target_title_or_group"),
        "set_target_title_or_group",
        "get_target_title_or_group"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_target_property", "target_property"), &GameplayRule::set_target_property);
    godot::ClassDB::bind_method(godot::D_METHOD("get_target_property"), &GameplayRule::get_target_property);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::STRING_NAME, "target_property"),
        "set_target_property",
        "get_target_property"
    );

    // --- Declaration Group ---
    ADD_GROUP("Declaration", "");

    godot::ClassDB::bind_method(godot::D_METHOD("set_operation", "operation"), &GameplayRule::set_operation);
    godot::ClassDB::bind_method(godot::D_METHOD("get_operation"), &GameplayRule::get_operation);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "operation", godot::PROPERTY_HINT_ENUM, "Set,Add,Multiply,Modifier Add"),
        "set_operation",
        "get_operation"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_value", "value"), &GameplayRule::set_value);
    godot::ClassDB::bind_method(godot::D_METHOD("get_value"), &GameplayRule::get_value);
    ADD_PROPERTY(
        godot::PropertyInfo(
            godot::Variant::NIL,
            "value",
            godot::PROPERTY_HINT_NONE,
            "",
            godot::PROPERTY_USAGE_DEFAULT | godot::PROPERTY_USAGE_NIL_IS_VARIANT
        ),
        "set_value",
        "get_value"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_priority", "priority"), &GameplayRule::set_priority);
    godot::ClassDB::bind_method(godot::D_METHOD("get_priority"), &GameplayRule::get_priority);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::INT, "priority"),
        "set_priority",
        "get_priority"
    );

    godot::ClassDB::bind_method(godot::D_METHOD("set_is_important", "is_important"), &GameplayRule::set_is_important);
    godot::ClassDB::bind_method(godot::D_METHOD("get_is_important"), &GameplayRule::get_is_important);
    ADD_PROPERTY(
        godot::PropertyInfo(godot::Variant::BOOL, "is_important"),
        "set_is_important",
        "get_is_important"
    );

    // --- Methods ---
    godot::ClassDB::bind_method(godot::D_METHOD("applies_to_self"), &GameplayRule::applies_to_self);
    godot::ClassDB::bind_method(godot::D_METHOD("applies_to_descendants"), &GameplayRule::applies_to_descendants);
    godot::ClassDB::bind_method(godot::D_METHOD("get_specificity"), &GameplayRule::get_specificity);
    godot::ClassDB::bind_method(godot::D_METHOD("outweighs", "other"), &GameplayRule::outweighs);
    godot::ClassDB::bind_method(godot::D_METHOD("has_same_target", "other"), &GameplayRule::has_same_target);

    godot::ClassDB::bind_method(godot::D_METHOD("find_target_game_property", "entity"), &GameplayRule::find_target_game_property);
    godot::ClassDB::bind_method(godot::D_METHOD("matches_entity", "entity", "is_self"), &GameplayRule::matches_entity, DEFVAL(true));
    godot::ClassDB::bind_method(godot::D_METHOD("apply_to_entity", "entity", "is_self"), &GameplayRule::apply_to_entity, DEFVAL(true));

    godot::ClassDB::bind_method(godot::D_METHOD("save_rule"), &GameplayRule::save_rule);
    godot::ClassDB::bind_method(godot::D_METHOD("load_rule", "data"), &GameplayRule::load_rule);
}

GameplayRule::GameplayRule() {
    // DOD NOTE: Minimalist initialization keeps Resource instantiation fast.
}

GameplayRule::~GameplayRule() {
}

// --- Scope Setters / Getters ---

void GameplayRule::set_scope(RuleScope p_scope) {
    if (scope == p_scope) return;
    scope = p_scope;
    emit_changed();
}

RuleScope GameplayRule::get_scope() const {
    return scope;
}

bool GameplayRule::applies_to_self() const {
    return scope == RuleScope::SELF || scope == RuleScope::ALL;
}

bool GameplayRule::applies_to_descendants() const {
    return scope == RuleScope::DESCENDANTS || scope == RuleScope::ALL;
}

// --- Selector Setters / Getters ---

void GameplayRule::set_target_class(const godot::StringName &p_class) {
    if (target_class == p_class) return;
    target_class = p_class;
    emit_changed();
}

godot::StringName GameplayRule::get_target_class() const {
    return target_class;
}

void GameplayRule::set_target_title_or_group(const godot::StringName &p_title_or_group) {
    if (target_title_or_group == p_title_or_group) return;
    target_title_or_group = p_title_or_group;
    emit_changed();
}

godot::StringName GameplayRule::get_target_title_or_group() const {
    return target_title_or_group;
}

void GameplayRule::set_target_property(const godot::StringName &p_property) {
    if (target_property == p_property) return;
    target_property = p_property;
    emit_changed();
}

godot::StringName GameplayRule::get_target_property() const {
    return target_property;
}

// --- Declaration Setters / Getters ---

void GameplayRule::set_operation(RuleOp p_operation) {
    if (operation == p_operation) return;
    operation = p_operation;
    emit_changed();
}

RuleOp GameplayRule::get_operation() const {
    return operation;
}

void GameplayRule::set_value(const godot::Variant &p_value) {
    if (value == p_value) return;
    value = p_value;
    emit_changed();
}

godot::Variant GameplayRule::get_value() const {
    return value;
}

void GameplayRule::set_priority(int p_priority) {
    if (priority == p_priority) return;
    priority = p_priority;
    emit_changed();
}

int GameplayRule::get_priority() const {
    return priority;
}

void GameplayRule::set_is_important(bool p_important) {
    if (is_important == p_important) return;
    is_important = p_important;
    emit_changed();
}

bool GameplayRule::get_is_important() const {
    return is_important;
}

// --- Specificity & Cascade Precedence ---

int GameplayRule::get_specificity() const {
    // CSS-inspired specificity weighting:
    // - Specific title/group match carries higher weight (+2) than a class-wide match (+1)
    // - Explicit target property adds (+1)
    int score = 0;
    if (!target_class.is_empty()) {
        score += 1;
    }
    if (!target_title_or_group.is_empty()) {
        score += 2;
    }
    if (!target_property.is_empty()) {
        score += 1;
    }
    return score;
}

bool GameplayRule::outweighs(const godot::Ref<GameplayRule> &p_other) const {
    if (!p_other.is_valid()) {
        return true;
    }
    if (is_important != p_other->get_is_important()) {
        return is_important;
    }
    if (priority != p_other->get_priority()) {
        return priority > p_other->get_priority();
    }
    return get_specificity() >= p_other->get_specificity();
}

bool GameplayRule::has_same_target(const godot::Ref<GameplayRule> &p_other) const {
    if (!p_other.is_valid()) {
        return false;
    }
    return target_class == p_other->get_target_class() &&
           target_title_or_group == p_other->get_target_title_or_group() &&
           target_property == p_other->get_target_property();
}

// --- GameEntity Matching & Application ---

bool GameplayRule::_entity_has_direct_property(const GameEntity *p_entity, const godot::StringName &p_property) const {
    if (!p_entity || p_property.is_empty()) {
        return false;
    }

    godot::TypedArray<godot::Dictionary> prop_list = p_entity->get_property_list();
    godot::String prop_str(p_property);
    for (int i = 0; i < prop_list.size(); ++i) {
        godot::Dictionary prop_info = prop_list[i];
        if (prop_info.has("name") && godot::String(prop_info["name"]) == prop_str) {
            return true;
        }
    }
    return false;
}

GameProperty *GameplayRule::find_target_game_property(GameEntity *p_entity) const {
    if (!p_entity || target_property.is_empty()) {
        return nullptr;
    }

    godot::String target_prop_str(target_property);

    // 1. Fast path for GamePiece, which explicitly indexes its GameProperty array
    if (GamePiece *piece = godot::Object::cast_to<GamePiece>(p_entity)) {
        if (GameProperty *prop = piece->get_property(target_prop_str)) {
            return prop;
        }
    }

    // 2. General path for any GameEntity holding child GameProperty nodes
    int child_count = p_entity->get_child_count();
    for (int i = 0; i < child_count; ++i) {
        if (GameProperty *prop = godot::Object::cast_to<GameProperty>(p_entity->get_child(i))) {
            if (prop->get_title() == target_prop_str || prop->get_name() == target_property) {
                return prop;
            }
        }
    }

    return nullptr;
}

bool GameplayRule::matches_entity(GameEntity *p_entity, bool p_is_self) const {
    if (!p_entity) {
        return false;
    }

    // 1. Check Scope
    if (p_is_self && !applies_to_self()) {
        return false;
    }
    if (!p_is_self && !applies_to_descendants()) {
        return false;
    }

    // 2. Check Target Class (respects Godot ClassDB inheritance, e.g. "GameEntity", "GameAgent", "GamePiece")
    if (!target_class.is_empty()) {
        if (!p_entity->is_class(godot::String(target_class))) {
            return false;
        }
    }

    // 3. Check Target Title or Group
    if (!target_title_or_group.is_empty()) {
        godot::String target_str(target_title_or_group);
        bool title_or_group_matched = false;

        if (p_entity->get_title() == target_str || p_entity->get_name() == target_title_or_group) {
            title_or_group_matched = true;
        } else if (p_entity->is_in_group(target_title_or_group)) {
            title_or_group_matched = true;
        } else if (GameAgentAction *agent_action = godot::Object::cast_to<GameAgentAction>(p_entity)) {
            if (agent_action->get_competes_with_groups().has(target_str)) {
                title_or_group_matched = true;
            }
        }

        if (!title_or_group_matched) {
            return false;
        }
    }

    // 4. Check Target Property (must exist either as a child GameProperty or a direct property on the GameEntity)
    if (!target_property.is_empty()) {
        bool has_game_prop = (find_target_game_property(p_entity) != nullptr);
        if (!has_game_prop && !_entity_has_direct_property(p_entity, target_property)) {
            return false;
        }
    }

    return true;
}

bool GameplayRule::apply_to_entity(GameEntity *p_entity, bool p_is_self) const {
    if (!matches_entity(p_entity, p_is_self) || target_property.is_empty()) {
        return false;
    }

    // Case 1: Target resolves to a child GameProperty (e.g., "Health", "Stamina")
    if (GameProperty *game_prop = find_target_game_property(p_entity)) {
        switch (operation) {
            case RuleOp::SET: {
                game_prop->set_value(static_cast<int>(value));
                return true;
            }
            case RuleOp::ADD: {
                game_prop->set_value(game_prop->get_value() + static_cast<int>(value));
                return true;
            }
            case RuleOp::MULTIPLY: {
                double scaled = static_cast<double>(game_prop->get_value()) * static_cast<double>(value);
                game_prop->set_value(static_cast<int>(scaled));
                return true;
            }
            case RuleOp::MODIFIER_ADD: {
                if (value.get_type() == godot::Variant::OBJECT) {
                    if (GamePropertyModifier *existing_mod = godot::Object::cast_to<GamePropertyModifier>(value.operator godot::Object *())) {
                        game_prop->add_modifier(existing_mod);
                        return true;
                    }
                }
                // Instantiate a static GamePropertyModifier for numeric rule payloads
                GamePropertyModifier *mod = memnew(GamePropertyModifier);
                mod->set_type(ModifierType::STATIC);
                mod->set_value(static_cast<int>(value));
                if (!get_title().is_empty()) {
                    mod->set_name(get_title());
                }
                game_prop->add_child(mod);
                game_prop->add_modifier(mod);
                return true;
            }
            default:
                return false;
        }
    }

    // Case 2: Target is a direct property on the GameEntity (e.g., "time_scale", "priority", "exclusivity")
    switch (operation) {
        case RuleOp::SET: {
            p_entity->set(target_property, value);
            return true;
        }
        case RuleOp::ADD: {
            godot::Variant current = p_entity->get(target_property);
            if (current.get_type() == godot::Variant::FLOAT || value.get_type() == godot::Variant::FLOAT) {
                p_entity->set(target_property, static_cast<double>(current) + static_cast<double>(value));
                return true;
            } else if (current.get_type() == godot::Variant::INT) {
                p_entity->set(target_property, static_cast<int64_t>(current) + static_cast<int64_t>(value));
                return true;
            } else if (current.get_type() == godot::Variant::STRING) {
                p_entity->set(target_property, godot::String(current) + godot::String(value));
                return true;
            }
            return false;
        }
        case RuleOp::MULTIPLY: {
            godot::Variant current = p_entity->get(target_property);
            if (current.get_type() == godot::Variant::FLOAT || value.get_type() == godot::Variant::FLOAT) {
                p_entity->set(target_property, static_cast<double>(current) * static_cast<double>(value));
                return true;
            } else if (current.get_type() == godot::Variant::INT) {
                p_entity->set(target_property, static_cast<int64_t>(current) * static_cast<int64_t>(value));
                return true;
            }
            return false;
        }
        case RuleOp::MODIFIER_ADD: {
            // Direct properties do not host GamePropertyModifier nodes
            return false;
        }
        default:
            return false;
    }
}

// --- Persistence ---

godot::Dictionary GameplayRule::save_rule() const {
    godot::Dictionary data;
    data["title"] = get_title();
    data["scope"] = static_cast<int>(scope);
    data["target_class"] = godot::String(target_class);
    data["target_title_or_group"] = godot::String(target_title_or_group);
    data["target_property"] = godot::String(target_property);
    data["operation"] = static_cast<int>(operation);
    data["value"] = value;
    data["priority"] = priority;
    data["is_important"] = is_important;
    return data;
}

void GameplayRule::load_rule(const godot::Dictionary &p_data) {
    if (p_data.has("title")) {
        set_title(p_data["title"]);
    }
    if (p_data.has("scope")) {
        scope = static_cast<RuleScope>(static_cast<int>(p_data["scope"]));
    }
    if (p_data.has("target_class")) {
        target_class = godot::StringName(godot::String(p_data["target_class"]));
    }
    if (p_data.has("target_title_or_group")) {
        target_title_or_group = godot::StringName(godot::String(p_data["target_title_or_group"]));
    }
    if (p_data.has("target_property")) {
        target_property = godot::StringName(godot::String(p_data["target_property"]));
    }
    if (p_data.has("operation")) {
        operation = static_cast<RuleOp>(static_cast<int>(p_data["operation"]));
    }
    if (p_data.has("value")) {
        value = p_data["value"];
    }
    if (p_data.has("priority")) {
        priority = p_data["priority"];
    }
    if (p_data.has("is_important")) {
        is_important = p_data["is_important"];
    }
    emit_changed();
}

} // namespace ideam::godot_ext